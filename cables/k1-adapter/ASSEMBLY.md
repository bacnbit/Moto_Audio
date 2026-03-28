# K1 Adapter Cable — Assembly Guide

This cable connects the device's 3.5mm TRRS radio jack (J3) to the
Kenwood K1 2-pin connector used by BTECH UV-PRO, Baofeng, and most
ham HTs. One cable is included with each unit. Spare cables can be
assembled for ~$2 in parts.

## Parts

- 1x 3.5mm TRRS plug (CTIA, 4-conductor)
- 1x 3.5mm TRS plug (for K1 speaker/mic connector)
- 1x 2.5mm TRS plug (for K1 PTT connector)
- ~300mm of 4-conductor shielded cable (28AWG)
- Heat shrink tubing (various sizes)

## Wiring Diagram

```
                    ┌─────────── 3.5mm TRRS plug ──────────────┐
                    │ (CTIA standard)                           │
                    │                                           │
Cable from device:  │  Tip   ─────► RADIO_SPK (audio from HT) │
3.5mm TRRS          │  Ring1 ─────► RADIO_MIC (audio to HT)   │
                    │  Ring2 ─────► PTT (active low)           │
                    │  Sleeve────► GND                         │
                    └───────────────────────────────────────────┘
                                          │
                              4-conductor cable
                                          │
                    ┌─────── K1 connector pair ─────────────────┐
                    │                                           │
3.5mm TRS plug:     │  Tip   ◄──── RADIO_SPK                   │
(speaker/mic)       │  Ring  ◄──── RADIO_MIC                   │
                    │  Sleeve◄──── GND                         │
                    │                                           │
2.5mm TRS plug:     │  Tip   ◄──── PTT                         │
(PTT)               │  Sleeve◄──── GND                         │
                    └───────────────────────────────────────────┘
```

## Assembly Steps

1. Cut cable to desired length (~300mm works well for backpack mount)

2. Strip outer jacket ~30mm from each end

3. **Device end (TRRS plug):**
   - Tip (longest contact): solder SPK wire
   - Ring 1 (2nd contact): solder MIC wire
   - Ring 2 (3rd contact): solder PTT wire
   - Sleeve: solder shield/GND wire
   - Slide strain relief and heat shrink before soldering

4. **Radio end — 3.5mm plug (speaker/mic):**
   - Tip: solder SPK wire (same as TRRS tip)
   - Ring: solder MIC wire (same as TRRS Ring1)
   - Sleeve: solder GND wire

5. **Radio end — 2.5mm plug (PTT):**
   - Tip: solder PTT wire (same as TRRS Ring2)
   - Sleeve: solder GND wire (bridge from 3.5mm sleeve)

6. Apply heat shrink, test continuity before sealing

## Testing

Use a multimeter in continuity mode to verify:
- TRRS Tip → 3.5mm Tip ✓
- TRRS Ring1 → 3.5mm Ring ✓
- TRRS Ring2 → 2.5mm Tip ✓
- TRRS Sleeve → 3.5mm Sleeve → 2.5mm Sleeve ✓
- No shorts between any conductors

## Notes

- The 3.5mm and 2.5mm plugs are often sold as a matched pair
  on Amazon/eBay as "K1 connector kit" for ~$3 for 5 pairs
- If your radio PTT is active-HIGH instead of active-low,
  see firmware config (future) for PTT polarity inversion
- Cable length: keep under 600mm to avoid RF pickup on the
  open PTT line near the transmitting radio

## Variants

For radios using a single 3.5mm 4-conductor K1 variant (some
newer BTECH models):

```
TRRS plug → 3.5mm TRRS plug (straight through, all 4 conductors)
```

For Yaesu radios (SP-10 connector):
See docs/RADIO-COMPAT.md for pinout differences.
