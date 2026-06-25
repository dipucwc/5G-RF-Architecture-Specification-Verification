#!/usr/bin/env python3
"""
rf_iq_verification.py — End-to-end 5G RF IQ signal verification workflow.

Compares a measured RF baseband IQ capture against a known reference waveform by
correcting timing offset, carrier frequency offset (CFO), and complex channel
gain/phase. Then computes EVM and ACLR, applies pass/fail limits, and logs the
result.

Unlike a symbol-rate toy, this version uses an **oversampled, RRC-pulse-shaped**
waveform so the signal is band-limited and ACLR is a genuine, meaningful metric.

Signal model:
    rx[n] = h * ref[n - d] * exp(j*2*pi*CFO*n/Fs) + noise[n]

Run:
    python rf_iq_verification.py
If ref_iq.csv / rx_iq.csv are missing, synthetic demo files are generated.

Author: Md Moklesur Rahman  |  github.com/dipucwc
License: MIT
"""

import csv
import os
import numpy as np

# ----------------------------- configuration ------------------------------ #
FS = 30.72e6            # sampling rate (Hz)
SPS = 8                 # samples per symbol (oversampling factor)
SYM_RATE = FS / SPS     # symbol rate (Hz)
BW = SYM_RATE * 1.2     # occupied bandwidth ≈ symbol rate * (1 + rolloff)
ROLLOFF = 0.22          # RRC roll-off
EVM_LIMIT = 3.5         # max RMS EVM (%)
ACLR_LIMIT = 30.0       # min ACLR (dB)
REF_FILE = "../data/ref_iq.csv"
RX_FILE = "../data/rx_iq.csv"
LOG_FILE = "results/verification_results.csv"


# ----------------------------- IO helpers --------------------------------- #
def _write_iq(path, iq):
    with open(path, "w", newline="") as f:
        w = csv.writer(f)
        w.writerow(["I", "Q"])
        for s in iq:
            w.writerow([f"{s.real:.8f}", f"{s.imag:.8f}"])


def _read_iq(path):
    I, Q = [], []
    with open(path) as f:
        r = csv.reader(f)
        next(r, None)
        for row in r:
            if len(row) >= 2:
                I.append(float(row[0]))
                Q.append(float(row[1]))
    return np.array(I) + 1j * np.array(Q)


# ----------------------------- pulse shaping ------------------------------ #
def rrc_filter(beta, sps, span=10):
    """Root-raised-cosine FIR taps."""
    n = np.arange(-span * sps, span * sps + 1)
    t = n / sps
    taps = np.zeros_like(t, dtype=float)
    for i, ti in enumerate(t):
        if abs(ti) < 1e-8:
            taps[i] = 1.0 - beta + 4 * beta / np.pi
        elif beta > 0 and abs(abs(ti) - 1 / (4 * beta)) < 1e-8:
            taps[i] = (beta / np.sqrt(2)) * (
                (1 + 2 / np.pi) * np.sin(np.pi / (4 * beta))
                + (1 - 2 / np.pi) * np.cos(np.pi / (4 * beta))
            )
        else:
            num = (np.sin(np.pi * ti * (1 - beta))
                   + 4 * beta * ti * np.cos(np.pi * ti * (1 + beta)))
            den = np.pi * ti * (1 - (4 * beta * ti) ** 2)
            taps[i] = num / den
    return taps / np.sqrt(np.sum(taps ** 2))


def make_demo_files(n_sym=2048):
    """Synthetic band-limited QPSK reference + impaired received capture."""
    rng = np.random.default_rng(42)
    bits = rng.integers(0, 2, (n_sym, 2))
    syms = ((bits[:, 0] * 2 - 1) + 1j * (bits[:, 1] * 2 - 1)) / np.sqrt(2)

    # upsample + RRC pulse shape -> band-limited reference
    up = np.zeros(n_sym * SPS, dtype=complex)
    up[::SPS] = syms
    rrc = rrc_filter(ROLLOFF, SPS)
    ref = np.convolve(up, rrc, mode="same")
    ref = ref / np.sqrt(np.mean(np.abs(ref) ** 2))

    # impairments
    d = 5 * SPS + 3                          # timing offset (samples)
    cfo = 1.5e3                              # Hz
    h = 0.85 * np.exp(1j * np.deg2rad(25))   # channel gain/phase
    idx = np.arange(len(ref))
    rx = h * np.roll(ref, d) * np.exp(1j * 2 * np.pi * cfo * idx / FS)
    rx += (rng.standard_normal(len(ref)) + 1j * rng.standard_normal(len(ref))) * 0.01

    _write_iq(REF_FILE, ref)
    _write_iq(RX_FILE, rx)
    print(f"[info] generated demo files: {REF_FILE}, {RX_FILE}")
    return rrc


# ----------------------------- DSP stages --------------------------------- #
def normalize(x):
    x = x - np.mean(x)
    p = np.sqrt(np.mean(np.abs(x) ** 2))
    return x / p if p > 0 else x


def estimate_timing(rx, ref):
    corr = np.correlate(rx, ref, mode="full")
    return np.argmax(np.abs(corr)) - (len(ref) - 1)


def estimate_cfo(rx_aligned, ref):
    phase = np.unwrap(np.angle(rx_aligned * np.conj(ref)))
    idx = np.arange(len(phase))
    slope = np.polyfit(idx, phase, 1)[0]
    return slope * FS / (2 * np.pi)


def estimate_channel(rx, ref):
    return np.sum(rx * np.conj(ref)) / np.sum(np.abs(ref) ** 2)


def compute_evm_symbols(rx_eq, ref, rrc, sps):
    """Matched-filter, then sample at symbol centers for a true symbol EVM."""
    mf = np.convolve(rx_eq, rrc, mode="same")
    mf_ref = np.convolve(ref, rrc, mode="same")
    # symbol-center sampling (skip filter transient at both ends)
    start = 4 * sps
    rx_sym = mf[start::sps]
    ref_sym = mf_ref[start::sps]
    m = min(len(rx_sym), len(ref_sym))
    rx_sym, ref_sym = rx_sym[:m], ref_sym[:m]
    # re-normalize symbol streams
    rx_sym = rx_sym / np.sqrt(np.mean(np.abs(rx_sym) ** 2))
    ref_sym = ref_sym / np.sqrt(np.mean(np.abs(ref_sym) ** 2))
    err = rx_sym - ref_sym
    return np.sqrt(np.mean(np.abs(err) ** 2) / np.mean(np.abs(ref_sym) ** 2)) * 100.0


def compute_aclr(x):
    spec = np.abs(np.fft.fftshift(np.fft.fft(x))) ** 2
    freqs = np.fft.fftshift(np.fft.fftfreq(len(x), 1 / FS))

    def band_power(lo, hi):
        m = (freqs >= lo) & (freqs < hi)
        return np.sum(spec[m])

    main = band_power(-BW / 2, BW / 2)
    lower = band_power(-3 * BW / 2, -BW / 2)
    upper = band_power(BW / 2, 3 * BW / 2)
    worst = max(lower, upper, 1e-12)
    return 10 * np.log10(main / worst)


# ----------------------------- workflow ----------------------------------- #
def verify():
    rrc = rrc_filter(ROLLOFF, SPS)
    if not (os.path.exists(REF_FILE) and os.path.exists(RX_FILE)):
        rrc = make_demo_files()

    ref = normalize(_read_iq(REF_FILE))
    rx = normalize(_read_iq(RX_FILE))

    # 1. timing
    d = estimate_timing(rx, ref)
    rx_al = np.roll(rx, -d)
    guard = abs(d) + 4 * SPS
    if guard < len(ref) // 4:
        ref_c, rx_al = ref[guard:-guard], rx_al[guard:-guard]
    else:
        ref_c = ref

    # 2. CFO
    cfo = estimate_cfo(rx_al, ref_c)
    idx = np.arange(len(rx_al))
    rx_cfo = rx_al * np.exp(-1j * 2 * np.pi * cfo * idx / FS)

    # 3. channel + equalize
    h = estimate_channel(rx_cfo, ref_c)
    rx_eq = rx_cfo / h

    # 4. metrics
    evm = compute_evm_symbols(rx_eq, ref_c, rrc, SPS)
    aclr = compute_aclr(rx_cfo)

    # 5. decision
    passed = (evm <= EVM_LIMIT) and (aclr >= ACLR_LIMIT)

    # 6. log
    os.makedirs(os.path.dirname(LOG_FILE), exist_ok=True)
    with open(LOG_FILE, "w", newline="") as f:
        w = csv.writer(f)
        w.writerow(["metric", "value", "limit", "status"])
        w.writerow(["timing_offset_samples", d, "-", "-"])
        w.writerow(["cfo_hz", f"{cfo:.2f}", "-", "-"])
        w.writerow(["channel_mag", f"{abs(h):.4f}", "-", "-"])
        w.writerow(["channel_phase_deg", f"{np.rad2deg(np.angle(h)):.2f}", "-", "-"])
        w.writerow(["evm_percent", f"{evm:.3f}", EVM_LIMIT,
                    "PASS" if evm <= EVM_LIMIT else "FAIL"])
        w.writerow(["aclr_db", f"{aclr:.2f}", ACLR_LIMIT,
                    "PASS" if aclr >= ACLR_LIMIT else "FAIL"])
        w.writerow(["overall", "-", "-", "PASS" if passed else "FAIL"])

    # 7. summary
    print("=" * 48)
    print(" 5G RF IQ Verification Summary")
    print("=" * 48)
    print(f" Timing offset : {d} samples")
    print(f" CFO           : {cfo:.2f} Hz")
    print(f" Channel       : |h|={abs(h):.4f}, ∠h={np.rad2deg(np.angle(h)):.2f}°")
    print(f" EVM           : {evm:.3f} %   (limit {EVM_LIMIT} %)")
    print(f" ACLR          : {aclr:.2f} dB  (limit {ACLR_LIMIT} dB)")
    print(f" RESULT        : {'PASS' if passed else 'FAIL'}")
    print(f" Log written   : {LOG_FILE}")


if __name__ == "__main__":
    verify()
