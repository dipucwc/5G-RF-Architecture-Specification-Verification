# Simplified RF Signal Chain

## Transmit Path

```text
Bits / Symbols
   ↓
Modulation and OFDM Processing
   ↓
Digital Front-End Processing
   ↓
DPD / CFR / Filtering
   ↓
DAC
   ↓
RF Upconversion
   ↓
Power Amplifier
   ↓
Antenna Array
```

## Receive Path

```text
Antenna Array
   ↓
LNA / RF Front-End
   ↓
Downconversion
   ↓
ADC
   ↓
Digital Front-End Processing
   ↓
Synchronization / Channel Estimation / Equalization
   ↓
Demodulation
```

## Example RF Impairments

| Impairment | Impact | Possible Metric |
|---|---|---|
| Gain imbalance | Amplitude distortion across paths | EVM, beam gain delta |
| Phase imbalance | Beam direction error and constellation rotation | EVM, phase delta |
| CFO | Phase rotation across IQ samples | EVM, residual CFO |
| Timing error | Incorrect sample alignment | EVM, timing offset |
| PA nonlinearity | In-band distortion and spectral regrowth | EVM, ACLR |
| Calibration residual | Beamforming error and path mismatch | residual gain/phase/timing error |
| Carrier configuration dependency | Hidden cross-domain issue | configuration regression result |

## Link to Requirements

Every significant impairment should map to a measurable requirement and a verification method. This prevents architecture assumptions from remaining undocumented.
