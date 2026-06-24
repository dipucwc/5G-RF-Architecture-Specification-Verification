# 5 Whys — Root-Cause Analysis of EVM Degradation

The 5 Whys method asks "Why?" repeatedly until the real cause surfaces, not just
the visible symptom. Below, the technical failure chain is followed to a
specification/process root cause.

## Problem

During 5G RF verification, measured RMS EVM exceeded the specified limit under
100 MHz bandwidth, 64QAM modulation, and high output power.

```
Specification Limit: RMS EVM ≤ 3.5 %
Measured Result:     RMS EVM = 5.1 %
Status:              FAILED
```

## 5-Why chain (technical → process)

| Level | Question | Answer |
|-------|----------|--------|
| **Why 1** | Why did RF verification fail? | The measured RMS EVM exceeded the specified requirement under the defined test condition. |
| **Why 2** | Why was EVM higher than expected? | The received IQ constellation showed amplitude and phase distortion after RF transmission. |
| **Why 3** | Why did the IQ signal have that distortion? | The RF transmit chain introduced gain/phase imbalance, PA nonlinear distortion, and residual carrier/timing errors. |
| **Why 4** | Why were these impairments not compensated? | Calibration and compensation requirements were not fully defined across all operating conditions — bandwidth, carrier configuration, output power, temperature, beamforming state. |
| **Why 5** | Why were the calibration requirements incomplete? | The RF architecture and specification phase did not include complete requirement coverage for calibration accuracy, impairment-budget allocation, verification conditions, and traceability between RF limits and test cases. |

## Root cause

> **Incomplete RF system specification and architecture requirement coverage for
> calibration accuracy and RF impairment budgeting across supported operating
> conditions.**

In other words, the RF architecture may be technically valid, but the
specification did not fully define how much gain error, phase error, timing error,
CFO, PA nonlinearity, and calibration residual error are acceptable under each
verification condition:

- Calibration residual limits were not clearly specified
- RF impairment contribution budget was not allocated
- Verification coverage conditions were incomplete
- Traceability between RF performance requirement and verification case was insufficient

## Why this matters

A 5-Why that stops at "Why 3" would have produced a fix like "improve calibration"
— treating a symptom. Following the chain to Why 5 reveals the fix belongs in the
**specification process**: define the limits, budget the impairments, and make the
requirement-to-test traceability mandatory. That is what the
[corrective actions](../04-corrective-actions/) address.

→ See also the full [8D report](./8d-report.md) and the
[test-escape analysis](./test-escape-analysis.md).
