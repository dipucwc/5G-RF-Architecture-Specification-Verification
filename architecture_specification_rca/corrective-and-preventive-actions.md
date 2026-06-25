# Corrective & Preventive Actions (CAPA)

The root-cause and escape analyses point to three layers of cause– technical,
specification, and process. The actions below close all three, with corrective
actions fixing the current issue and preventive actions stopping recurrence.

---

## Corrective actions

| # | Action |
|---|--------|
| CA-1 | Update RF system specification to define residual calibration error limits for gain imbalance, phase imbalance, timing error, and CFO. |
| CA-2 | Define EVM requirement coverage across bandwidth, modulation order, output power, carrier configuration, and temperature range. |
| CA-3 | Add RF impairment budget allocation for PA nonlinearity, IQ imbalance, LO leakage, phase noise, timing misalignment, and calibration residuals. |
| CA-4 | Update the verification plan to ensure all high-risk operating conditions are included in RF lab validation. |
| CA-5 | Create a BF/BFC–Timing–Carrier cross-dependency matrix defining supported and restricted combinations. |

## Preventive actions

| # | Action |
|---|--------|
| PA-1 | Add an architecture/specification review checklist item for impairment-budget completeness. |
| PA-2 | Improve traceability between RF requirements, verification methods, and pass/fail criteria. |
| PA-3 | Maintain an architecture assumption register (RF chain, antenna branch, beamforming mode, BFC algorithm, timing budget, carrier setup). |
| PA-4 | Add a risk-based verification matrix to identify high-risk combinations before lab regression. |
| PA-5 | Update the release gate to require requirement-test-evidence linkage before release. |

---

## Impairment budget concept

A core corrective idea is **allocating an EVM budget** across impairment sources so
the total stays within the specification limit. Conceptually:

```
EVM_total ² ≈ EVM_PA² + EVM_IQimb² + EVM_phasenoise² + EVM_CFO² + EVM_timing² + EVM_calib²
```

Each contributor gets a defined ceiling; calibration residual limits (CA-1) and
the impairment budget (CA-3) ensure no single source — or their combination —
pushes total EVM past the limit under any *supported operating condition* (CA-2).

## Operating-condition coverage

The specification must define EVM limits not at a single nominal point but across
the full supported envelope:

| Dimension | Coverage to specify |
|-----------|---------------------|
| Carrier bandwidth | each supported BW (e.g. 20/40/100 MHz) |
| Modulation order | QPSK … 256QAM |
| Output power | nominal … max (PA-compression sensitive) |
| Carrier configuration | single carrier … CA combinations |
| Temperature | full operating range |
| Beamforming mode | each supported BF/BFC state |

The original failure sat at a worst-case corner (100 MHz, 64QAM, high power) that
nominal-only verification never exercised — which is exactly what CA-2 and CA-4
close.

→ See the [requirement traceability matrix](./requirement-traceability-matrix.md)
for how each requirement maps to a verification method and evidence.
