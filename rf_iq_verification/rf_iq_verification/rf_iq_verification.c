/*
 * rf_iq_verification.c - End-to-end 5G RF IQ signal verification workflow.
 *
 * Corrects timing offset, carrier frequency offset (CFO), and complex channel
 * gain/phase, then computes EVM and ACLR, applies pass/fail limits, and logs
 * the result. Uses an oversampled, RRC-pulse-shaped waveform so ACLR is a
 * meaningful metric.
 *
 * Signal model:  rx[n] = h * ref[n-d] * exp(j*2*pi*CFO*n/Fs) + noise[n]
 *
 * Build:  gcc rf_iq_verification.c -O2 -lm -o rf_iq_verification
 * Run:    ./rf_iq_verification
 *
 * Author: Md Moklesur Rahman | github.com/dipucwc
 * License: MIT
 */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <complex.h>

#define FS        30.72e6
#define SPS       8
#define ROLLOFF   0.22
#define EVM_LIMIT 3.5
#define ACLR_LIMIT 30.0
#define NSYM      2048
#define NSAMP     (NSYM * SPS)
#define SPAN      10
#define NTAP      (2 * SPAN * SPS + 1)
#define REF_FILE  "../data/ref_iq.csv"
#define RX_FILE   "../data/rx_iq.csv"
#define LOG_FILE  "results/verification_results.csv"

static inline double complex J(void) { return _Complex_I; }

static double randn(void) {
    double u1 = (rand() + 1.0) / (RAND_MAX + 2.0);
    double u2 = (rand() + 1.0) / (RAND_MAX + 2.0);
    return sqrt(-2.0 * log(u1)) * cos(2.0 * M_PI * u2);
}

static int file_exists(const char *p) {
    FILE *f = fopen(p, "r");
    if (f) { fclose(f); return 1; }
    return 0;
}

static void write_iq(const char *path, double complex *x, int n) {
    FILE *f = fopen(path, "w");
    fprintf(f, "I,Q\n");
    for (int i = 0; i < n; i++)
        fprintf(f, "%.8f,%.8f\n", creal(x[i]), cimag(x[i]));
    fclose(f);
}

static int read_iq(const char *path, double complex *x, int maxn) {
    FILE *f = fopen(path, "r");
    if (!f) return 0;
    char line[128];
    if (!fgets(line, sizeof(line), f)) { fclose(f); return 0; }
    int n = 0; double vi, vq;
    while (n < maxn && fscanf(f, "%lf,%lf", &vi, &vq) == 2)
        x[n++] = vi + vq * J();
    fclose(f);
    return n;
}

/* RRC taps */
static void rrc_filter(double *taps) {
    double beta = ROLLOFF;
    for (int k = 0; k < NTAP; k++) {
        double ti = (double)(k - SPAN * SPS) / SPS;
        double v;
        if (fabs(ti) < 1e-8) {
            v = 1.0 - beta + 4.0 * beta / M_PI;
        } else if (beta > 0 && fabs(fabs(ti) - 1.0 / (4 * beta)) < 1e-8) {
            v = (beta / sqrt(2.0)) * ((1 + 2 / M_PI) * sin(M_PI / (4 * beta))
                + (1 - 2 / M_PI) * cos(M_PI / (4 * beta)));
        } else {
            double num = sin(M_PI * ti * (1 - beta))
                       + 4 * beta * ti * cos(M_PI * ti * (1 + beta));
            double den = M_PI * ti * (1 - (4 * beta * ti) * (4 * beta * ti));
            v = num / den;
        }
        taps[k] = v;
    }
    double e = 0;
    for (int k = 0; k < NTAP; k++) e += taps[k] * taps[k];
    e = sqrt(e);
    for (int k = 0; k < NTAP; k++) taps[k] /= e;
}

/* same-length convolution */
static void conv_same(double complex *x, int n, double *taps, double complex *y) {
    int half = NTAP / 2;
    for (int i = 0; i < n; i++) {
        double complex acc = 0;
        for (int k = 0; k < NTAP; k++) {
            int j = i + k - half;
            if (j >= 0 && j < n) acc += x[j] * taps[k];
        }
        y[i] = acc;
    }
}

static void make_demo(double *rrc) {
    srand(42);
    static double complex ref[NSAMP], rx[NSAMP], up[NSAMP];
    for (int i = 0; i < NSAMP; i++) up[i] = 0;
    for (int s = 0; s < NSYM; s++) {
        double re = (rand() % 2) * 2 - 1;
        double im = (rand() % 2) * 2 - 1;
        up[s * SPS] = (re + im * J()) / sqrt(2.0);
    }
    conv_same(up, NSAMP, rrc, ref);
    double p = 0;
    for (int i = 0; i < NSAMP; i++) p += creal(ref[i] * conj(ref[i]));
    p = sqrt(p / NSAMP);
    for (int i = 0; i < NSAMP; i++) ref[i] /= p;

    int d = 5 * SPS + 3;
    double cfo = 1.5e3;
    double complex h = 0.85 * cexp(J() * (25.0 * M_PI / 180.0));
    for (int i = 0; i < NSAMP; i++) {
        int j = (i - d % NSAMP + NSAMP) % NSAMP;
        rx[i] = h * ref[j] * cexp(J() * 2 * M_PI * cfo * i / FS)
              + (randn() + randn() * J()) * 0.01;
    }
    write_iq(REF_FILE, ref, NSAMP);
    write_iq(RX_FILE, rx, NSAMP);
    printf("[info] generated demo files: %s, %s\n", REF_FILE, RX_FILE);
}

static void normalize(double complex *x, int n) {
    double complex mean = 0;
    for (int i = 0; i < n; i++) mean += x[i];
    mean /= n;
    double p = 0;
    for (int i = 0; i < n; i++) { x[i] -= mean; p += creal(x[i] * conj(x[i])); }
    p = sqrt(p / n);
    if (p > 0) for (int i = 0; i < n; i++) x[i] /= p;
}

static int estimate_timing(double complex *rx, double complex *ref, int n) {
    int best_lag = 0; double best = -1;
    int range = 128;
    for (int lag = -range; lag <= range; lag++) {
        double complex acc = 0;
        for (int i = 0; i < n; i++) {
            int j = i - lag;
            if (j >= 0 && j < n) acc += rx[i] * conj(ref[j]);
        }
        double mag = cabs(acc);
        if (mag > best) { best = mag; best_lag = lag; }
    }
    return best_lag;
}

/* naive DFT power spectrum for ACLR (NSAMP is fine for a one-shot demo) */
static double compute_aclr(double complex *x, int n) {
    double BW = (FS / SPS) * 1.2;
    double main = 0, lower = 0, upper = 0;
    /* bin frequencies: f_k = k*FS/n for k in [-n/2, n/2) */
    for (int k = 0; k < n; k++) {
        double complex acc = 0;
        for (int i = 0; i < n; i++)
            acc += x[i] * cexp(-J() * 2 * M_PI * k * i / n);
        double pw = creal(acc * conj(acc));
        double f = (k < n / 2) ? (double)k * FS / n
                               : (double)(k - n) * FS / n;
        if (f >= -BW / 2 && f < BW / 2) main += pw;
        else if (f >= -3 * BW / 2 && f < -BW / 2) lower += pw;
        else if (f >= BW / 2 && f < 3 * BW / 2) upper += pw;
    }
    double worst = upper > lower ? upper : lower;
    if (worst < 1e-12) worst = 1e-12;
    return 10 * log10(main / worst);
}

int main(void) {
    static double rrc[NTAP];
    rrc_filter(rrc);

    if (!file_exists(REF_FILE) || !file_exists(RX_FILE)) make_demo(rrc);

    static double complex ref[NSAMP], rx[NSAMP];
    int n  = read_iq(REF_FILE, ref, NSAMP);
    int n2 = read_iq(RX_FILE, rx, NSAMP);
    if (n == 0 || n2 == 0) { fprintf(stderr, "read error\n"); return 1; }
    if (n2 < n) n = n2;

    normalize(ref, n);
    normalize(rx, n);

    /* 1. timing */
    int d = estimate_timing(rx, ref, n);
    static double complex rx_al[NSAMP];
    for (int i = 0; i < n; i++) {
        int j = (i + d) % n; if (j < 0) j += n;
        rx_al[i] = rx[j];
    }
    int guard = abs(d) + 4 * SPS;
    guard = ((guard + SPS - 1) / SPS) * SPS;   /* round up to multiple of SPS */
    int start = guard, end = n - guard;
    if (end - start < n / 4) { start = 0; end = n; }
    int m = end - start;

    /* 2. CFO */
    static double ph[NSAMP];
    double prev = 0, off = 0;
    for (int k = 0; k < m; k++) {
        int i = start + k;
        double raw = carg(rx_al[i] * conj(ref[i]));
        if (k > 0) {
            double diff = raw + off - prev;
            while (diff >  M_PI) { off -= 2 * M_PI; diff -= 2 * M_PI; }
            while (diff < -M_PI) { off += 2 * M_PI; diff += 2 * M_PI; }
        }
        ph[k] = raw + off; prev = ph[k];
    }
    double sx = 0, sy = 0, sxx = 0, sxy = 0;
    for (int k = 0; k < m; k++) { sx += k; sy += ph[k]; sxx += (double)k * k; sxy += (double)k * ph[k]; }
    double slope = (m * sxy - sx * sy) / (m * sxx - sx * sx);
    double cfo = slope * FS / (2 * M_PI);

    static double complex rx_cfo[NSAMP];
    for (int k = 0; k < m; k++) {
        int i = start + k;
        rx_cfo[k] = rx_al[i] * cexp(-J() * 2 * M_PI * cfo * i / FS);
    }

    /* 3. channel + equalize */
    double complex num = 0; double den = 0;
    for (int k = 0; k < m; k++) {
        num += rx_cfo[k] * conj(ref[start + k]);
        den += creal(ref[start + k] * conj(ref[start + k]));
    }
    double complex h = num / den;
    static double complex rx_eq[NSAMP];
    for (int k = 0; k < m; k++) rx_eq[k] = rx_cfo[k] / h;

    /* 4a. EVM: matched-filter (RRC) both signals -> raised-cosine (Nyquist,
     *     zero ISI at symbol centers), then sample at the symbol phase that
     *     minimizes reference self-error. */
    static double complex mf[NSAMP], mfr[NSAMP], refc[NSAMP];
    for (int k = 0; k < m; k++) refc[k] = ref[start + k];
    conv_same(rx_eq, m, rrc, mf);
    conv_same(refc, m, rrc, mfr);

    /* find best symbol-sampling phase (0..SPS-1) using reference energy peak */
    int best_phase = 0; double best_e = -1;
    for (int ph0 = 0; ph0 < SPS; ph0++) {
        double e = 0; int c = 0;
        for (int i = 4 * SPS + ph0; i < m - 4 * SPS; i += SPS) { e += creal(mfr[i]*conj(mfr[i])); c++; }
        if (c > 0 && e / c > best_e) { best_e = e / c; best_phase = ph0; }
    }

    int s0 = 4 * SPS + best_phase;
    int s_end = m - 4 * SPS;
    double prx = 0, pref = 0; int cnt = 0;
    for (int i = s0; i < s_end; i += SPS) {
        prx  += creal(mf[i] * conj(mf[i]));
        pref += creal(mfr[i] * conj(mfr[i]));
        cnt++;
    }
    prx = sqrt(prx / cnt); pref = sqrt(pref / cnt);
    double enum_ = 0, eden = 0;
    for (int i = s0; i < s_end; i += SPS) {
        double complex a = mf[i] / prx;
        double complex b = mfr[i] / pref;
        double complex e = a - b;
        enum_ += creal(e * conj(e));
        eden  += creal(b * conj(b));
    }
    double evm = sqrt(enum_ / eden) * 100.0;

    /* 4b. ACLR */
    double aclr = compute_aclr(rx_cfo, m);

    int passed = (evm <= EVM_LIMIT) && (aclr >= ACLR_LIMIT);

    /* 5. log */
    if (system("mkdir -p results") != 0) { /* ignore */ }
    FILE *lf = fopen(LOG_FILE, "w");
    if (lf) {
        fprintf(lf, "metric,value,limit,status\n");
        fprintf(lf, "timing_offset_samples,%d,-,-\n", d);
        fprintf(lf, "cfo_hz,%.2f,-,-\n", cfo);
        fprintf(lf, "channel_mag,%.4f,-,-\n", cabs(h));
        fprintf(lf, "channel_phase_deg,%.2f,-,-\n", carg(h) * 180.0 / M_PI);
        fprintf(lf, "evm_percent,%.3f,%.1f,%s\n", evm, EVM_LIMIT, evm <= EVM_LIMIT ? "PASS" : "FAIL");
        fprintf(lf, "aclr_db,%.2f,%.1f,%s\n", aclr, ACLR_LIMIT, aclr >= ACLR_LIMIT ? "PASS" : "FAIL");
        fprintf(lf, "overall,-,-,%s\n", passed ? "PASS" : "FAIL");
        fclose(lf);
    }

    /* 6. summary */
    printf("================================================\n");
    printf(" 5G RF IQ Verification Summary\n");
    printf("================================================\n");
    printf(" Timing offset : %d samples\n", d);
    printf(" CFO           : %.2f Hz\n", cfo);
    printf(" Channel       : |h|=%.4f, arg h=%.2f deg\n", cabs(h), carg(h) * 180.0 / M_PI);
    printf(" EVM           : %.3f %%   (limit %.1f %%)\n", evm, EVM_LIMIT);
    printf(" ACLR          : %.2f dB  (limit %.1f dB)\n", aclr, ACLR_LIMIT);
    printf(" RESULT        : %s\n", passed ? "PASS" : "FAIL");
    printf(" Log written   : %s\n", LOG_FILE);
    return passed ? 0 : 2;
}
