# Hardware Guide

## PCB Ordering (JLCPCB)

### Main Unit PCB

1. Export Gerbers from KiCad: File → Fabrication Outputs → Gerbers
2. Go to jlcpcb.com → Order Now → upload Gerbers zip
3. Settings:
   - Layers: 2
   - PCB Qty: 5 (minimum, same price as 2)
   - PCB Thickness: 1.6mm
   - Surface Finish: **ENIG** (gold) — required for pogo contact pads
   - Copper Weight: 1oz
   - Via Covering: Tented
4. Enable SMT Assembly (optional but recommended for SMD parts)
   - Upload BOM CSV from `hardware/bom/`
   - Upload CPL (component placement list) from KiCad
   - Parts marked with LCSC numbers populate automatically
   - Hand-solder: J1 USB-C, J2-J7 connectors, SW1 EC11, D1 WS2812B

### SMT vs Hand-solder

JLCPCB SMT handles everything 0402 and IC-scale well. Parts to
hand-solder after board arrives:

| Part | Reason |
|------|--------|
| J1 USB-C | Through-board connector, SMT misses underside |
| J2 Helmet pogo array | Custom, not in LCSC |
| J3 TRRS jack | Through-hole legs |
| J4 Battery pogo array | Custom, not in LCSC |
| J5 JST-PH2 | Horizontal, check orientation |
| J6 JST-GH2 | Small connector, easy to hand-solder |
| J7 2.5mm jack | Panel mount, wires to PCB pad |
| SW1 EC11 | Through-hole, socket first |
| U1 ESP32-S3 | Module, hand-solder or SMT |

## PCB Assembly Notes

### Critical: PCB bottom pogo contact pads

J4 (battery pogo interface) pads must be **ENIG finish** for reliable
electrical contact. Specify ENIG for the entire board — same cost tier
for this board size, and it also helps with solderability everywhere.

The pogo pin array (P75-E2) presses into printed pockets in the
enclosure bottom. The PCB sits above with pads exposed through slots.
Ensure pad locations match `mechanical/magnetic-pogo/` STL exactly.

### Ferrite bead FB1 orientation

FB1 is bidirectional — orientation doesn't matter electrically.
Place it between the 3V3 digital plane and PCM5102A AVDD pin.
Keep the AVDD trace short after FB1 with C3+C4 decoupling right
at the AVDD pin.

### BM62 module antenna clearance

Both BM62 modules have onboard chip antennas. Keep copper pours
and other components at least 5mm away from the antenna end of
each module. The antenna end is marked on the module silkscreen.

### ESP32-S3 antenna clearance

Same rule — 5mm keepout around the module antenna area.

### Keep-alive cell placement

J5 (JST-PH2) connects to the 400mAh keep-alive LiPo. The cell
sits in a printed pocket inside the enclosure, wired to J5.
Cell dimensions: ~40×12×3mm (LP401230). Verify your cell fits
before ordering — dimensions vary slightly by supplier.

## Pogo Contact Assembly

### Battery contacts (J4, bottom of PCB)

Parts needed per unit:
- 4x P75-E2 spring pogo pins (1.0mm dia, 3.5mm stroke, gold tip)
- 2x N52 neodymium disc magnets, 8×3mm
- Printed pogo carrier (`mechanical/magnetic-pogo/battery_pogo_carrier.stl`)

Assembly:
1. Press pogo pins into carrier pockets (light press fit)
2. Solder pogo pin bases to PCB pads (BAT+, GND, VBUS, DATA)
3. Press N52 magnets into carrier recesses (note polarity — N on
   left, S on right when viewing from below; mark with marker)
4. Apply small drop of CA glue to lock magnets

### Helmet connector (J2, top of PCB)

Parts needed per unit:
- 5x P75-E2 spring pogo pins
- 2x N52 neodymium disc magnets, 6×2mm
- Printed connector collar (`mechanical/helmet-connector/collar_unit_side.stl`)
- Printed cable collar (`mechanical/helmet-connector/collar_cable_side.stl`)

The cable side collar gets the mating gold pads (short PCB stub or
hand-tinned copper pads in printed recesses). Wires run from cable
collar to 3.5mm IEM jack + JST-GH2 mic pigtail.

## Component Substitutions

| Original | Acceptable substitute | Notes |
|----------|--------------------|-------|
| PCM5102A | PCM5101A | 106dB SNR, slight downgrade |
| TPA6132A2 | MAX97220 | Different pinout, reroute needed |
| BM62SPKS1MC2 | BM64SPKS1MC2 | Pin compatible, BT 5.0 |
| IP5306 | IP5306-I2C | Adds I2C charge status |
| MCP73831 | TP4056 | Adjust current resistor |
| AMS1117-3.3 | AP2112K-3.3 | Lower dropout, direct swap |
| SPM0687LR5H-1 | SPH0645LM4H | I2S instead of PDM, firmware change |

## Test Points

The PCB should expose test pads for:

| TP | Net | Expected value |
|----|-----|---------------|
| TP1 | +5V | 5.0V ±0.1V when powered |
| TP2 | +3V3 | 3.3V ±0.05V |
| TP3 | VBAT_MUX | Battery voltage (3.2-4.2V) |
| TP4 | I2S_BCK | Clock signal when audio active |
| TP5 | UART0_TX | UART data to BM62-A |
| TP6 | GND | 0V reference |

## First Power-on Checklist

Before connecting battery or USB for the first time:

- [ ] Visually inspect all SMD components — no bridges, no tombstones
- [ ] Check USB-C CC1/CC2 resistors (R7, R8) are populated
- [ ] Verify FB1 ferrite bead is placed (not a 0-ohm resistor)
- [ ] Confirm no shorts: measure resistance GND→+5V and GND→+3V3
       (should be >1kΩ before any power applied)
- [ ] ESP32-S3 module seated correctly, no bent pins
- [ ] BM62 modules seated, antenna end clear of copper

Power-on sequence:
1. Connect USB-C to bench power supply at 5V current-limited to 500mA
2. Measure TP1 (+5V), TP2 (+3V3), TP3 (VBAT_MUX)
3. If all rails good, connect USB-C to PC
4. Flash firmware: `idf.py -p PORT flash`
5. Monitor serial: `idf.py -p PORT monitor`
6. Confirm boot log shows ESP32 and both BM62 modules initializing
