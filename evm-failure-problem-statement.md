# Problem Statement–Beamforming EVM Degradation

## Summary

During RF / system verification of a 5G radio, beam-level signal quality did not
meet expected RF performance: **EVM increased and beam gain/phase consistency
degraded** under a specific combination of beamforming, beamforming calibration
(BFC), timing alignment, and carrier configuration. The issue points to a
**specification gap** in how these domains interact, not a single isolated bug.

## Technical problem

During RF verification of a 5G radio system, the measured Error Vector Magnitude
exceeds the defined specification limit.

```
RF Requirement:
  RMS EVM ≤ 3.5 %   under specified carrier bandwidth, modulation, output power,
                    and operating conditions

Measured Result:
  RMS EVM = 5.2 %

Verification Status:
  FAILED
```

A high EVM means the transmitted/received IQ signal carries excessive distortion.
Possible contributors: RF-chain nonlinearity, IQ imbalance, phase noise, timing
misalignment, insufficient calibration accuracy, PA compression, or incomplete
specification assumptions.

## Problem definition (5W)

| Element | Description |
|---------|-------------|
| **What failed?** | Measured beam-level signal quality did not meet expected RF performance. EVM increased and beam gain/phase consistency degraded under a specific BF/BFC + carrier configuration + timing condition. |
| **Where observed?** | RF/system verification environment — IQ capture, beam performance measurement, calibration logs, timing logs, carrier configuration logs. |
| **When observed?** | After enabling beamforming with calibration under a specific carrier configuration during system-level verification. |
| **How detected?** | EVM exceeded limit; beam gain/phase delta higher than expected; timing/capture logs showed alignment sensitivity. |
| **Specification concern** | The RF specification described BF/BFC, timing, and carrier configuration separately, but did not fully define their cross-dependency behavior and verification conditions. |
| **Operational impact** | Inconsistent beam performance, coverage degradation, reduced link quality, repeated verification failures across product variants. |

## Example failure condition

Wideband carrier configuration with multiple carriers, beamforming enabled,
calibration active, and a timing-alignment dependency between baseband, RF path,
and antenna branch.

## System impact

Potential beam-pointing error, degraded signal quality, reduced coverage margin,
unstable performance across carrier/beam combinations, and delayed specification
release.

## Investigation objective

Contain the issue, identify the root cause, define corrective and preventive
actions, update the RF system specifications, and strengthen verification
traceability.

→ Continue to [Root-Cause Analysis](../03-root-cause-analysis/).
