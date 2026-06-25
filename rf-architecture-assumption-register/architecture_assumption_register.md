# RF Architecture Assumption Register

This register captures architecture assumptions that must be reviewed, translated into requirements, and linked to verification evidence.

| ID | Architecture Assumption | Risk if Not Specified | Required Specification Action | Verification Evidence |
|---|---|---|---|---|
| ASM-BF-001 | Beamforming weights require valid antenna/RF path gain and phase assumptions. | Beam direction or beam gain may deviate. | Define gain/phase tolerance per beam set and operating condition. | Beam pattern or beam gain/phase measurement. |
| ASM-BFC-001 | BFC residual error must remain within defined limits after calibration. | Beamforming and EVM degradation after calibration. | Define residual gain, phase, and timing limits. | Calibration logs and residual error table. |
| ASM-TIM-001 | Timing alignment must be within allowed tolerance before IQ/EVM evaluation. | Incorrect EVM or unstable beam-level performance. | Define sample/symbol/slot timing tolerance. | Timing log and alignment report. |
| ASM-CC-001 | Carrier configuration can affect BF/BFC timing and RF path behavior. | Unsupported or high-risk carrier/beam combinations may escape. | Define supported/restricted carrier configuration matrix. | Configuration regression matrix. |
| ASM-EVM-001 | EVM is affected by multiple RF impairments. | EVM failure root cause may be unclear. | Define impairment budget and debug flow. | EVM contribution analysis and RCA report. |
| ASM-TRC-001 | Requirement traceability must be complete before release. | Requirement may be unverified. | Link requirement, method, test case, threshold, and evidence. | Traceability matrix sign-off. |
