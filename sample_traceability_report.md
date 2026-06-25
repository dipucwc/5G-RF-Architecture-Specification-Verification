# Sample RF Requirement Traceability Report

| Requirement ID | Domain | Test Case | Metric | Measured | Limit | Result | Evidence |
|---|---|---|---|---:|---:|---|---|
| RF-BF-REQ-001 | Beamforming | TC-BF-001 | beam_gain_delta | 0.72 dB | <= 1.0 dB | PASS | reports/beam_gain_phase_summary.md |
| RF-BFC-REQ-001 | BFC | TC-BFC-001 | residual_phase_error | 2.1 deg | <= 3.0 deg | PASS | reports/bfc_residual_error_table.md |
| RF-TIM-REQ-001 | Timing | TC-TIM-001 | timing_error | 0.18 us | <= 0.25 us | PASS | reports/timing_alignment_summary.md |
| RF-CC-REQ-001 | Carrier Configuration | TC-CC-001 | configuration_coverage | 100 percent | >= 100 percent | PASS | specification/bf_bfc_timing_carrier_dependency_matrix.md |
| RF-EVM-REQ-001 | EVM | TC-EVM-001 | evm_rms | 2.8 percent | <= 3.5 percent | PASS | reports/evm_result_summary.md |
| RF-ACLR-REQ-001 | ACLR | TC-ACLR-001 | aclr | 47.8 dB | >= 45 dB | PASS | reports/aclr_result_summary.md |
| RF-TRC-REQ-001 | Traceability | TC-TRC-001 | traceability_coverage | 100 percent | >= 100 percent | PASS | reports/generated_traceability_report.md |

## Summary

- Total requirements: 7
- Requirement coverage: 100%
- Pass rate: 100%

This sample report demonstrates the expected output of the traceability checker.
