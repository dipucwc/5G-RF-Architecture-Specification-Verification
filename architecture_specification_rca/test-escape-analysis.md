# Test-Escape Analysis — Why the Defect Was Not Caught Earlier

A root cause explains *why the fault was introduced*. An **escape analysis**
explains *why existing test stages did not catch it*. Strong RCA covers both — a
fix that only addresses the technical cause but leaves the test gap open will let
similar issues escape again.

This analysis covers two escape points: **Entity Test** and **System Test**.

---

## Escape 1 — Why didn't Entity Test catch this defect?

| Level | Question | Answer |
|-------|----------|--------|
| Why 1 | Why did Entity Test not detect the defect? | Entity-level verification used nominal operating conditions and did not include the failing high-power, wideband EVM scenario. |
| Why 2 | Why was the high-risk scenario not included? | The entity-test scope was derived from incomplete RF specification coverage. |
| Why 3 | Why was the scope derived from incomplete coverage? | The specification did not explicitly identify this operating point as a mandatory calibration-sensitive verification condition. |
| Why 4 | Why was the condition not flagged as calibration-sensitive? | Risk analysis for RF impairments across operating modes was not fully performed during specification definition. |
| Why 5 | Why was the risk analysis not fully performed? | The review process focused more on nominal functionality than on worst-case RF performance sensitivity. |

**Entity-test escape cause:** the entity-level test strategy did not include
worst-case RF performance conditions, especially high-power and wideband
EVM-sensitive cases.

**Action proposal:**
- Expand entity-test coverage to include worst-case RF operating points
- Add a mandatory EVM stress-verification matrix
- Include calibration-sensitive scenarios in early verification

---

## Escape 2 — Why didn't System Test catch it earlier?

| Level | Question | Answer |
|-------|----------|--------|
| Why 1 | Why did System Test not catch it earlier? | The failure was detected only in a later regression cycle. |
| Why 2 | Why was it detected only late in regression? | The required operating-condition combination was not part of the initial mandatory system-test regression set. |
| Why 3 | Why was it not in the mandatory regression set? | The verification traceability matrix did not map this RF requirement to a required regression test case. |
| Why 4 | Why was the requirement not mapped to a regression case? | The RF requirement was written at a high level without detailed measurable verification conditions. |
| Why 5 | Why was it written at high level only? | The specification process lacked sufficient detail in translating RF architecture assumptions into measurable verification criteria. |

**System-test escape cause:** the traceability from RF requirement → verification
condition → regression test case was incomplete.

**Action proposal:**
- Update the verification traceability matrix
- Add regression cases for high-power, wideband, high-order modulation
- Ensure each RF requirement has: measurable condition, verification method,
  pass/fail threshold, and regression ownership

---

## Escape-cause summary

| Area | Category |
|------|----------|
| Process RCA | Specification coverage gap |
| Entity-test escape | Insufficient test coverage |
| System-test escape | Incomplete requirement-to-test traceability |

## Professional summary

This RCA shows the EVM failure was **not caused only by a direct RF implementation
defect**, but mainly by a systematic gap in RF architecture and system
specification definition — particularly in calibration requirement coverage, RF
impairment budgeting, and verification traceability. The corrective actions
therefore focus on strengthening RF specification completeness, verification
mapping, and high-risk operating-condition coverage.
