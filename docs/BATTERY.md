# Battery Pack Guide

## Overview

Battery packs are separate from the main unit and connect via a 4-pin
magnetic pogo interface. All three pack sizes share the same 80×50mm
XY footprint and connector — only the Z height (and cell count) differs.

Each pack contains:
- LP805060 LiPo flat cells (protected, with built-in PCB)
- BQ25895 fast charge IC
- USB-C receptacle for direct pack charging
- NTC thermistor wired to DATA pin for temperature monitoring
- Gold contact pads on top face matching main unit pogo pins

## Pack Specs

| Pack | Cells | Capacity | Runtime | Height | Weight |
|------|-------|----------|---------|--------|--------|
| Slim | 1x LP805060 | 2000mAh | ~7hr | ~12mm | ~42g |
| Standard | 2x LP805060 | 4000mAh | ~14hr | ~22mm | ~84g |
| Chonker | 4x LP805060 | 8000mAh | ~28hr | ~42mm | ~168g |

Runtime based on ~280mA draw at 3.7V nominal.

## Wiring (all packs)

All cells wired **in parallel** — positive-to-positive, negative-to-negative.
Voltage stays 3.7V nominal. Capacity multiplies by cell count.

```
Cell 1 (+) ──┬──► BQ25895 BAT+
Cell 2 (+) ──┤
Cell 3 (+) ──┤    (chonker only)
Cell 4 (+) ──┘

Cell 1 (-) ──┬──► GND
Cell 2 (-) ──┤
Cell 3 (-) ──┤
Cell 4 (-) ──┘
```

Use 20AWG silicone wire for cell-to-cell parallel connections.
Use 22AWG for BQ25895 to pogo pads.

## Pogo Contact Pads

Top face of each pack has 4 gold-flashed copper pads matching the
main unit pogo pin layout:

```
  ┌──────────────────────────────┐
  │  [BAT+]  [GND]  [VBUS] [NTC]│ ← pogo contact pads (ENIG)
  │                              │
  │        CELL STACK            │
  │                              │
  │  [N magnet]    [S magnet]    │
  │        USB-C charge port     │
  └──────────────────────────────┘
```

Pad dimensions: 6×6mm, 2mm spacing minimum between pads.
Magnets: N52 8×3mm discs, polarity opposite to main unit side
(so they attract). Mark polarity before installation.

## BQ25895 Fast Charge IC

The BQ25895 handles:
- USB-C input detection and current negotiation (up to 3A @ 5V = 15W)
- Cell charging with proper CC/CV profile
- Cell balancing (parallel cells don't need individual balancing)
- Temperature monitoring via NTC

Key external components per pack PCB:
- RILIM resistor sets input current limit
- ICHG resistor sets charge current
- NTC thermistor (10k at 25°C) for temperature cutoff

Charge current settings:
- Slim pack (2000mAh): ICHG = 1A (0.5C rate, safe and gentle)
- Standard pack (4000mAh): ICHG = 2A (0.5C rate)
- Chonker pack (8000mAh): ICHG = 3A (0.375C rate, max BQ25895)

At 2A charging, standard pack (4000mAh) fully charges in ~2.5 hours.

## Pack PCB

The battery pack PCB is a simple 2-layer board:
- BQ25895 charge IC
- USB-C receptacle (with CC resistors)
- 4 pogo pad footprints (top face, ENIG)
- 2 magnet recesses (plated or unplated, your choice)
- NTC thermistor footprint (0402)
- Cell wire pads

Order from JLCPCB with ENIG surface finish — same as main unit.
Specify that top copper is exposed (not covered by soldermask) on
the pogo pad areas.

## STL Files

```
mechanical/battery-packs/
├── slim_shell_top.stl       Shell for 1-cell pack
├── slim_shell_bottom.stl
├── standard_shell_top.stl   Shell for 2-cell pack
├── standard_shell_bottom.stl
├── chonker_shell_top.stl    Shell for 4-cell pack
├── chonker_shell_bottom.stl
└── parametric_notes.md      Notes on modifying for other cell counts
```

Print in ASA on Bambu X1C:
- 0.20mm layer height
- 4 walls
- 40% gyroid infill
- No supports needed (designed to print flat)

## Assembly Steps

1. Print shell top and bottom
2. Install magnets in recesses — note polarity! Use compass or
   another magnet to verify N/S before gluing. CA glue to secure.
3. Solder cells in parallel on wire harness, heat-shrink joints
4. Solder pack PCB — BQ25895, USB-C, pogo pads, NTC
5. Connect cell harness to pack PCB BAT+/GND pads
6. Route NTC thermistor between cells (tape to center cell)
7. Press PCB into top shell — pogo pads face out through slots
8. Snap top and bottom shell together — 2x M2 screws secure

## Charging

Each pack has its own USB-C port for direct charging without
the main unit. You can also charge through the main unit's
USB-C — VBUS is passed through to the pack via the VBUS pogo pin.

Charging indicator: BQ25895 STAT pin drives an LED on the pack PCB.
- Blinking: charging in progress
- Solid: charge complete
- Off: no input power

## Hot-swap Procedure

1. Locate spare pack (pre-charged)
2. Grab current pack and pull straight away from main unit
   — magnets release with ~2kg pull force
3. Music pauses momentarily... actually no, it doesn't. The
   keep-alive cell on the main unit maintains BT connections
   and continues audio with the on-board reserve.
4. Snap new pack onto main unit — magnets self-align and click
5. Music continues. BT connections were never dropped.
6. Plug depleted pack into USB-C to recharge.

## Safety Notes

- Use only **protected** LP805060 cells — they have a small PCB
  on the connector end with over-discharge, overcharge, and
  short-circuit protection built in.
- Do not short the BAT+ and GND pogo pads.
- Do not exceed the BQ25895 rated input voltage (12V max).
- Store packs at ~50% charge if not using for extended periods.
- Li-Po cells should not be stored or charged below 0°C.
  The NTC thermistor + BQ25895 will cutoff charging if pack
  temperature drops below 5°C.
