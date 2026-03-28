# PCBWay PCB Layout Design Brief — Moto Audio Unit Rev 2.0

## Project Summary

We need a **complete PCB layout** from a provided KiCad schematic and BOM. This is a motorcycle helmet audio system that mixes phone music with radio communications. The board integrates an ESP32-S3, two Bluetooth modules, a DAC, headphone amplifier, power management, and multiple connectors.

**Deliverables requested:**
- KiCad `.kicad_pcb` layout file (or Gerber + drill files)
- Component placement file (CPL / pick-and-place) for SMT assembly
- Any DRC reports or design review notes

---

## Board Specifications

| Parameter | Value |
|-----------|-------|
| Board dimensions | **80 × 50 mm** (rectangular, rounded corners R=2mm) |
| Layer count | **4-layer** recommended (signal / GND / power / signal) |
| PCB thickness | 1.6 mm |
| Copper weight | 1 oz outer, 0.5 oz inner |
| Surface finish | **ENIG** (required for pogo contact pads) |
| Minimum trace width | 0.15 mm (signal), 0.3 mm (power 3V3), 0.5 mm (power 5V/VBAT) |
| Minimum via | 0.3 mm drill / 0.6 mm pad |
| Minimum clearance | 0.2 mm |
| Soldermask | Both sides, matte black preferred |
| Silkscreen | White, both sides |
| Impedance control | Not required (no high-speed differential pairs) |

---

## Files Provided

1. **KiCad schematic**: `hardware/kicad/moto_audio_unit_rev2.kicad_sch` — contains full netlist with all pin-to-pin connections and symbol definitions
2. **BOM (CSV)**: `hardware/bom/moto_audio_unit_rev2_bom.csv` — includes MPN, LCSC part numbers, footprints, and placement notes
3. **Interactive schematic diagram**: `docs/schematic-diagram.html` — open in any browser for a visual overview of all connections, filterable by subsystem (power, audio, bluetooth, radio, UI)
4. **Architecture document**: `docs/ARCHITECTURE.md` — detailed block diagrams, signal flow, power tree
5. **This design brief**: Design rules, placement constraints, and layout guidance

---

## Component List (40 parts)

### ICs (9)
| Ref | Part | Package | Function |
|-----|------|---------|----------|
| U1 | ESP32-S3-WROOM-1-N8 | Module (18×25.5mm) | Main MCU + BT Radio #1 |
| U2 | BM62SPKS1MC2 | Module (~14×12mm) | BT Radio #2 (radio HFP) |
| U3 | BM62SPKS1MC2 | Module (~14×12mm) | BT Radio #3 (expansion) |
| U4 | PCM5102APWR | TSSOP-20 | I2S DAC (112dB SNR) |
| U5 | TPA6132A2DCKR | SOT-23-8 | Headphone amplifier |
| U6 | IP5306 | SOP-8 | 5V boost + LiPo charger |
| U7 | TPS2116DRLR | SOT-23-6 | Hot-swap power mux |
| U8 | AMS1117-3.3 | SOT-223-3 | 3.3V LDO |
| U9 | MCP73831T-2ACI/OT | SOT-23-5 | Keep-alive cell charger |

### Connectors (7)
| Ref | Part | Type | Mounting |
|-----|------|------|----------|
| J1 | USB-C (XKB U262-16XN-4BVC11) | SMD + through-hole | Top, board edge |
| J2 | 5-pin pogo array (custom) | Through-hole pads | Top, board edge |
| J3 | 3.5mm TRRS (CUI SJ-3524-SMT) | SMD | Top, board edge |
| J4 | 4-pin pogo array (custom) | Through-hole pads | **Bottom of PCB** |
| J5 | JST-PH 2-pin (S2B-PH-K-S) | Through-hole | Top |
| J6 | JST-GH 2-pin (SM02B-GHS-TB) | SMD | Top |
| J7 | 2.5mm mono jack (CUI SJ-2523-SMT) | SMD | Top, board edge |

### User Interface (2)
| Ref | Part | Type | Mounting |
|-----|------|------|----------|
| SW1 | EC11 rotary encoder | Through-hole (socketed) | Top, board edge |
| D1 | WS2812B-2020 | SMD 2020 | Top, visible through enclosure |

### Passives (12)
| Ref | Value | Package | Notes |
|-----|-------|---------|-------|
| FB1 | BLM18PG221SN1 ferrite | 0402 | DAC AVDD filter |
| C1 | 10µF | 0402 | ESP32 bulk decoupling |
| C2 | 100nF | 0402 | ESP32 HF bypass |
| C3 | 10µF | 0402 | DAC AVDD bulk |
| C4 | 100nF | 0402 | DAC AVDD bypass |
| C5 | 1µF | 0402 | Amp VDD bypass |
| C6 | 10µF | 0402 | BM62-A bulk |
| C7 | 100nF | 0402 | BM62-A bypass |
| C8 | 10µF | 0402 | BM62-B bulk |
| C9 | 100nF | 0402 | BM62-B bypass |
| R1–R11 | Various (see BOM) | 0402 | Pullups, CC resistors, etc. |

### Mic (1)
| Ref | Part | Notes |
|-----|------|-------|
| MIC1 | Knowles SPM0687LR5H-1 | **Not on PCB** — mounted on external pigtail cable, connects via J6 JST-GH2 |

---

## Connector Placement Constraints

The board fits in an 80×50×18mm 3D-printed enclosure. Connector placement is critical for enclosure compatibility.

```
                    TOP EDGE (80mm)
    ┌───────────────────────────────────────────┐
    │ J1(USB-C)                    J2(helmet)   │  ← Top edge: USB-C left, helmet pogo right
    │                                           │
    │    U1(ESP32)     U2(BM62-A)               │
    │                                           │
    │    U4(DAC)→U5(Amp)→          U3(BM62-B)  │  ← Audio chain center-left
    │                                           │
    │ J3(TRRS)  SW1(encoder)  D1(LED)  J7(PTT) │  ← Bottom edge: user-facing controls
    └───────────────────────────────────────────┘
                    BOTTOM EDGE (50mm)

    PCB BOTTOM SIDE:
    ┌───────────────────────────────────────────┐
    │                                           │
    │          J4 (4-pin battery pogo)          │  ← Center of bottom face
    │          with exposed ENIG pads           │
    │                                           │
    └───────────────────────────────────────────┘
```

### Edge connector requirements:
- **J1 (USB-C)**: Top-left edge, connector opening flush with enclosure wall
- **J2 (Helmet pogo)**: Top-right edge, 5 pads exposed at board edge for pogo pin access
- **J3 (TRRS 3.5mm)**: Bottom-left edge, jack opening flush with enclosure wall
- **J7 (2.5mm PTT)**: Bottom-right edge, jack opening flush
- **SW1 (EC11 encoder)**: Bottom edge center-left, shaft protrudes through enclosure
- **D1 (WS2812B)**: Bottom edge center, visible through light pipe or translucent enclosure window

### Bottom-side pogo pads (J4):
- 4 exposed ENIG copper pads on the **bottom** of the PCB (no soldermask over pads)
- Pad size: 6×6mm each, minimum 2mm spacing between pads
- Centered on the board bottom face
- Pin order (left to right when viewing from bottom): BAT+, GND, VBUS_CHGPASS, DATA/NTC

### Internal connectors (no edge constraint):
- **J5 (JST-PH2)**: Near U7/U9 power section, keep-alive cell connects here
- **J6 (JST-GH2)**: Near ESP32 I2S pins, mic pigtail cable exits through enclosure grommet

---

## Layout Design Rules

### Antenna Keep-out Zones (CRITICAL)

All three RF modules have onboard chip antennas. **No copper pour, traces, or components** within the keep-out zones:

- **U1 (ESP32-S3-WROOM-1)**: 10mm keep-out extending from the antenna end of the module (the end opposite the pin row). No ground plane under the antenna overhang area.
- **U2, U3 (BM62)**: 5mm keep-out from the antenna edge of each module. Antenna end is marked on the module silkscreen.
- Do not place U2 and U3 antennas facing each other — orient them pointing toward opposite board edges to minimize coupling.

### Audio Section (U4, U5, FB1, C3–C5)

This is an **audiophile-grade** DAC + headphone amp for in-ear monitors. Audio quality is critical.

- **Separate analog and digital ground regions** around U4 (PCM5102A) and U5 (TPA6132A2). Connect analog and digital grounds at a single star point near the DAC.
- **FB1 ferrite bead** must be placed within 3mm of U4 AVDD pin. Route: +3V3 rail → FB1 → short trace → U4 AVDD pin. Place C3 (10µF) and C4 (100nF) immediately at the AVDD pin, after FB1.
- **C5 (1µF)** must be within 1mm of U5 TPA6132A2 VDD pin.
- **Keep I2S traces (BCK, WS, DOUT) away from power traces** — minimum 1mm clearance. Route I2S as a group with matched length (±2mm tolerance is fine for 48kHz audio).
- **Analog output traces** (U4 LOUT/ROUT → U5 INL/INR) should be as short as possible, shielded by ground on adjacent layers, and away from digital signals.
- **DAC FLT, DEMP, XSMT pins**: Tie FLT and DEMP to GND. Tie XSMT to +3V3 (soft mute disabled).

### Power Section (U6, U7, U8, U9)

- **Group power components together** in one area of the board (suggested: left side or top-left).
- **U7 (TPS2116) → U6 (IP5306)**: Keep VOUT trace from TPS2116 to IP5306 BAT+ input short and wide (≥0.5mm). This carries full system current (~300mA typical, 500mA peak).
- **U6 (IP5306) → U8 (AMS1117)**: 5V output trace ≥0.5mm width. Also feeds U5 (TPA6132A2 VDD at 5V) — route this as a short spur from the 5V rail.
- **U8 (AMS1117-3.3)**: Input and output capacitors close to IC. Tab pad (pin 2) is the output — ensure good thermal connection to ground plane for heat dissipation.
- **U9 (MCP73831)**: Place near J1 (USB-C) and J5 (keep-alive JST). PROG resistor (R1 10k) close to pin 3.
- **3V3 rail** feeds: U1 (ESP32), U2 (BM62-A), U3 (BM62-B), U4 (PCM5102A DVDD), D1 (WS2812B), SPM0687 via J6. Use wide traces (≥0.3mm) or copper pour.

### Power Trace Width Guide

| Net | Current | Min trace width |
|-----|---------|----------------|
| VBAT (TPS2116 IN1/IN2) | 500mA peak | 0.5mm |
| VBAT_MUX (TPS2116 → IP5306) | 500mA | 0.5mm |
| +5V (IP5306 → AMS1117, TPA6132) | 400mA | 0.5mm |
| +3V3 (AMS1117 → everything) | 350mA | 0.3mm |
| USB VBUS | 500mA | 0.5mm |
| Signal traces (I2S, UART, GPIO) | <10mA | 0.15mm OK |

### UART Routing (ESP32 → BM62 modules)

- **UART0**: ESP32 IO41 (TX) → U2 BM62-A UART_RX, ESP32 IO42 (RX) ← U2 BM62-A UART_TX
- **UART1**: ESP32 IO43 (TX) → U3 BM62-B UART_RX, ESP32 IO44 (RX) ← U3 BM62-B UART_TX
- UART speed: 115200 baud — no impedance matching needed, just route cleanly
- Route UART pairs together, keep away from I2S and power traces

### GPIO Routing Notes

| ESP32 Pin | Function | Routing notes |
|-----------|----------|---------------|
| IO1 | Wired PTT drive (open-drain) | Route to J3 RING2/PTT via pad |
| IO2 | Wired PTT sense | Route to J3 + R10 10k pullup to 3V3 |
| IO3 | Battery NTC ADC | Route to J4 DATA/NTC pad + R9 10k pullup to 3V3 |
| IO4 | BM62-A RST | Route to U2 RST_N pin |
| IO5 | I2S BCK (shared) | Route to U4 BCK + U2 I2S_CLK + U3 I2S_CLK + MIC1 CLK (via J6) |
| IO6 | I2S WS | Route to U4 LRCK + U2 I2S_WS + U3 I2S_WS |
| IO7 | I2S DOUT | Route to U4 DIN only |
| IO8 | I2S DIN (PDM mic) | Route from J6 mic pigtail (SPM0687 DATA) |
| IO15 | Ext PTT | Route to J7 TIP + R2 10k pullup to 3V3 |
| IO16/17/18 | Encoder A/B/SW | Route to SW1 + R3/R4/R5 10k pullups to 3V3 |
| IO21 | Wired mic enable | Route to analog switch or J3 area |
| IO38 | WS2812B data | Route to D1 DIN via R6 100Ω series resistor |
| IO39 | Amp shutdown | Route to U5 /SD pin |
| IO40 | Squelch detect | Route to J3 SW_DETECT |
| IO41–IO44 | UART0/1 TX/RX | See UART section above |
| IO47 | BM62-B RST | Route to U3 RST_N pin |
| IO48 | BM62-A STATUS | Route from U2 BT_STATUS |
| IO45 | BM62-B STATUS | Route from U3 BT_STATUS |

### Decoupling Capacitor Placement

Every IC gets its bypass caps **within 2mm** of the power pins:

| IC | Caps | Notes |
|----|------|-------|
| U1 (ESP32) | C1 (10µF) + C2 (100nF) | Near 3V3 pin |
| U2 (BM62-A) | C6 (10µF) + C7 (100nF) | Near VDD pin |
| U3 (BM62-B) | C8 (10µF) + C9 (100nF) | Near VDD pin |
| U4 (PCM5102A) | C3 (10µF) + C4 (100nF) | On AVDD, after FB1 |
| U5 (TPA6132A2) | C5 (1µF) | Within 1mm of VDD |

### Test Points

Please add exposed test point pads (1.5mm round, no soldermask) for:

| Label | Net | Location suggestion |
|-------|-----|-------------------|
| TP1 | +5V | Near U6 output |
| TP2 | +3V3 | Near U8 output |
| TP3 | VBAT_MUX | Near U7 output |
| TP4 | I2S_BCK | Near U4 |
| TP5 | UART0_TX | Near U2 |
| TP6 | GND | Anywhere convenient |

### Mounting Holes

4x M2 mounting holes (2.4mm drill, 4.5mm annular ring, plated, connected to GND) at the four corners, 3mm inset from board edges.

---

## Ground Plane Strategy (4-layer)

| Layer | Purpose |
|-------|---------|
| L1 (Top) | Signal routing + component placement |
| L2 (Inner 1) | **Continuous GND plane** — do not break except for vias |
| L3 (Inner 2) | Power plane (+3V3 pour, +5V pour, VBAT_MUX pour) |
| L4 (Bottom) | Signal routing + J4 battery pogo pads (exposed ENIG) |

- **L2 GND plane must be unbroken** under the audio section (U4, U5) — route signals around, not through, the analog ground region
- Split L3 power plane into zones: +3V3 (largest), +5V, VBAT_MUX
- Stitch ground vias every 5mm around the board perimeter and near all IC ground pins

---

## Special Notes

1. **BM62 modules (U2, U3)** are not in LCSC — they will be hand-soldered. Include generous pads and clear silkscreen markings for manual placement.

2. **J2 (helmet pogo)** and **J4 (battery pogo)** are custom pogo pin arrays — provide pad-only footprints (no through-hole barrels). We will solder pogo pins manually. J4 pads must be on the **bottom** copper layer with **no soldermask** (exposed ENIG).

3. **SW1 (EC11 encoder)** uses a 5-pin DIP socket — include through-hole pads sized for standard 2.54mm socket pins, not the encoder directly. This allows field replacement.

4. **Silkscreen**: Please include reference designators, pin 1 dots, polarity markings on all polarized components, and connector pinout labels (especially J3 TRRS pinout and J4 battery pogo pinout).

5. **Board edge cuts**: Rounded corners (R=2mm). No V-scoring or panelization needed — individual boards are fine.

6. **USB-C (J1)**: The XKB U262-16XN-4BVC11 has both SMD and through-hole anchor pins. Ensure proper footprint for mechanical strength.

---

## Design Review Checklist (for PCBWay engineer)

Before delivering, please verify:

- [ ] All nets from schematic are routed (no unconnected ratsnest)
- [ ] DRC passes with specified rules
- [ ] Antenna keep-out zones are clear (U1, U2, U3)
- [ ] No traces cross the analog ground region under U4/U5
- [ ] FB1 is within 3mm of U4 AVDD, with C3/C4 at the pin
- [ ] All bypass caps within 2mm of their IC power pins
- [ ] Power traces meet minimum width requirements
- [ ] J4 bottom pogo pads are exposed ENIG (no soldermask)
- [ ] 4x M2 mounting holes at corners
- [ ] Test points accessible
- [ ] Silkscreen is readable and not overlapping component pads

---

## Contact

If you have questions about the design intent, the interactive schematic diagram (`docs/schematic-diagram.html`) shows all connections visually — hover any component for full pin details, or use the filter buttons to isolate subsystems (Power, Audio, Bluetooth, Radio, UI).

The full project repository is at: https://github.com/bacnbit/Moto_Audio
