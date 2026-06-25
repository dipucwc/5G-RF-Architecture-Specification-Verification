# RF IQ Verification Code

A self-contained **IQ-capture RF verification workflow**, implemented identically
in MATLAB, Python, and C. It produces the verification *evidence* referenced by
the [requirement traceability matrix](../04-corrective-actions/requirement-traceability-matrix.md).

## What it does

Given a reference IQ waveform and a measured (impaired) IQ capture, the workflow:

1. Removes DC offset and normalizes power
2. Estimates and corrects **timing offset** via cross-correlation
3. Estimates and corrects **carrier frequency offset (CFO)** from the phase slope
4. Estimates the complex channel coefficient (LS) and **equalizes**
5. Matched-filters (RRC) and samples at symbol centers
6. Computes **EVM** and **ACLR**
7. Applies pass/fail limits and writes a CSV log

**Signal model:**
```
rx[n] = h · ref[n − d] · exp(j·2π·CFO·n/Fs) + noise[n]
```
where `h` is complex channel gain/phase, `d` the timing offset, `CFO` the carrier
frequency offset.

## Reproducible test vectors

All three implementations read the **same committed IQ files** in [`data/`](./data/):

```
data/ref_iq.csv   reference waveform (band-limited, RRC-shaped QPSK)
data/rx_iq.csv    measured capture with known impairments injected
```

This guarantees identical, verifiable results across languages. To regenerate
fresh vectors, delete `data/*.csv` and run the Python version — it creates new
band-limited demo files.

## Run it

```bash
# Python  (requires numpy)
cd python && python rf_iq_verification.py

# MATLAB
cd matlab && matlab -batch "rf_iq_verification"

# C
cd c && gcc rf_iq_verification.c -O2 -lm -o rf_iq_verification && ./rf_iq_verification
```

## Example output

```
================================================
 5G RF IQ Verification Summary
================================================
 Timing offset : 43 samples
 CFO           : 1499.97 Hz
 Channel       : |h|=0.9998, ∠h≈25–27°
 EVM           : 1.7 %    (limit 3.5 %)
 ACLR          : 36 dB    (limit 30 dB)
 RESULT        : PASS
```

The committed vectors inject a known timing offset (43 samples), CFO (1500 Hz),
and channel phase (25°); the workflow recovers all three. Full log in
[`results/`](./results/example_verification_results.csv).

## Reproducing the case-study FAILURE

The [case study](../02-problem-statement/evm-failure-problem-statement.md) is about
an EVM that *exceeds* its limit. To reproduce a FAIL, regenerate the demo with
larger impairments — e.g. raise the noise scale in the Python `make_demo_files`
(`0.01` → `0.06`). This mirrors the real scenario where residual gain/phase/timing
errors push EVM past 3.5 %.

## Notes & limitations

- EVM is **symbol-level**, not 3GPP resource-grid EVM. A full 5G NR test bench adds
  OFDM demod, CP removal, FFT, resource-grid extraction, DMRS-based channel
  estimation, and modulation-specific EVM.
- ACLR uses integrated FFT power bands over a band-limited (RRC-shaped) signal;
  a production tester uses calibrated measurement bandwidths, RBW/VBW, and
  standard-specific masks.
- The CFO estimator assumes a known, roughly aligned reference.

## Mapping to requirements

| Output | Verifies |
|--------|----------|
| Timing offset estimate | RF-SPEC-TIM-001 |
| CFO estimate + correction | RF-SPEC-CFO-001 |
| EVM % | RF-SPEC-EVM-001 |
| ACLR dB | RF-SPEC-ACLR-001 |
| CSV pass/fail log | RF-SPEC-VER-001 |
