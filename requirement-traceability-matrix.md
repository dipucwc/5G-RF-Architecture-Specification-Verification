# Requirement Traceability Matrix

The process root cause was **insufficient traceability** — RF requirements were
not consistently linked to verification methods, test cases, and pass/fail
evidence. This matrix is the corrective artifact: every RF performance requirement
maps to *how* it is verified and *what evidence* proves it.

## Updated requirement set

| Requirement ID | Requirement | Verification Method | Metric / Evidence |
|----------------|-------------|---------------------|-------------------|
| RF-SPEC-EVM-001 | RMS EVM shall remain below the defined limit for each supported modulation and bandwidth configuration. | IQ-capture EVM analysis | EVM % vs. condition table |
| RF-SPEC-CAL-001 | Residual phase error after calibration shall remain within tolerance across supported operating conditions. | Calibration log analysis + RF measurement | Residual phase error table |
| RF-SPEC-CAL-002 | Residual gain imbalance after calibration shall remain within tolerance across supported RF paths. | Calibration log analysis | Residual gain imbalance table |
| RF-SPEC-TIM-001 | Timing alignment error shall remain within sample-level tolerance before EVM evaluation. | Timing log + sample alignment verification | Timing offset evidence |
| RF-SPEC-CFO-001 | Residual carrier frequency offset shall be estimated and compensated before final signal-quality evaluation. | Phase-slope CFO estimation + correction | CFO estimate (Hz) + corrected EVM |
| RF-SPEC-ACLR-001 | Adjacent Channel Leakage Ratio shall meet the defined limit using FFT-based spectral analysis. | ACLR calculation | ACLR (dB) vs. limit |
| RF-SPEC-VER-001 | Each RF performance requirement shall be linked to a verification method, measurement setup, and pass/fail threshold. | Traceability review | Requirement-to-test matrix |
| RF-BF-REQ-001 | Beamforming shall meet beam gain/phase accuracy limits per supported BF mode and carrier configuration. | Beam measurement + IQ-capture analysis | Beam gain/phase delta + EVM report |
| RF-BFC-REQ-001 | Residual BFC error shall remain within gain/phase/timing limits for all supported RF paths. | Calibration log + RF measurement | Residual error table |
| RF-CC-REQ-001 | Carrier configurations impacting BF/BFC shall be explicitly defined, validated, or restricted. | Configuration matrix review + regression | Supported/restricted carrier matrix |
| RF-TRC-REQ-001 | Each requirement shall link to architecture assumptions, verification method, test case, and pass/fail criteria. | Traceability review | Requirement-to-test matrix |

## Traceability chain (the rule going forward)

```
Architecture assumption
        │
        ▼
RF requirement (measurable, with limit)
        │
        ▼
Verification method (how it's measured)
        │
        ▼
Test case (entity + system, incl. worst-case corners)
        │
        ▼
Evidence (logs, IQ captures, EVM/ACLR results)
        │
        ▼
Pass/fail threshold → release gate
```

## How the code links to this matrix

The [verification code](../05-verification-code/) directly produces evidence for
several requirements above:

| Code output | Satisfies |
|-------------|-----------|
| Timing offset estimate | RF-SPEC-TIM-001 |
| CFO estimate + correction | RF-SPEC-CFO-001 |
| EVM % result | RF-SPEC-EVM-001 |
| ACLR dB result | RF-SPEC-ACLR-001 |
| CSV pass/fail log | RF-SPEC-VER-001 |

This is the point of the whole case study: requirements, verification, and
evidence form one connected, auditable chain.
