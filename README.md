# Moto Audio Unit

An open-source motorcycle helmet audio system that does what Cardo and Sena won't: mixes music from your phone with radio comms from any radio, wired or wireless, with hot-swap batteries and audiophile-grade IEM audio.

## The Problem

Every helmet comms system on the market locks you into their ecosystem. Cardo and Sena force proprietary headsets, proprietary intercoms, and no wired radio support. If you run a ham radio, you're out of luck. If you want to listen to music *and* hear your radio without manual switching, you're out of luck.

## The Solution

A small, jacket-mountable device that:

- **Mixes audio** from your phone (A2DP music/nav) and your radio (wired or BT), automatically ducking music when the radio has a signal and restoring it after 5 seconds of silence
- **Works with any radio** via a standard TRRS wired jack (included K1 adapter cable covers BTECH, Baofeng, Kenwood, Wouxun, Retevis, and most ham HTs)
- **Works with BT radios** — BTECH UV-PRO, or any HFP-capable radio
- **Three independent Bluetooth radios** — phone A2DP/HFP, radio HFP, expansion (future intercom, BS-PTT direct, second phone)
- **Hot-swap batteries** — magnetic pogo contacts snap the pack on/off; a 400mAh keep-alive cell on the main board holds your BT connections during swaps
- **Modular battery packs** — slim (2000mAh, ~7hr), standard (4000mAh, ~14hr), chonker (8000mAh, ~28hr), all same XY footprint
- **High-quality audio** — PCM5102A DAC (112dB SNR) + TPA6132A2 amp (1Ω output impedance) designed specifically for IEMs
- **Differential MEMS mic** — Knowles SPM0687 differential capsule on a pigtail, mounts in helmet chin bar foam for wind noise rejection
- **5-pin magnetic pogo helmet connector** — single connector carries IEM stereo + differential mic; snaps on magnetically, releases safely if helmet is thrown
- **Fully serviceable** — every wear item on a connector (battery, mic, encoder)

## Hardware Overview

```
┌─────────────────────────────────────────────────┐
│                  Main Unit                       │
│  ESP32-S3 (BT #1 phone)                         │
│  BM62 x2  (BT #2 radio, BT #3 expansion)        │
│  PCM5102A DAC + TPA6132A2 amp                   │
│  Knowles SPM0687 diff MEMS mic (pigtail)         │
│  TPS2116 hot-swap power mux                      │
│  MCP73831 keep-alive charger                     │
│  IP5306 5V boost                                 │
│  AMS1117-3.3 LDO                                │
│                                                  │
│  Connectors:                                     │
│  J1  USB-C (charge + flash)                      │
│  J2  5-pin magnetic pogo → helmet cable          │
│  J3  3.5mm TRRS wired radio (K1 cable incl.)    │
│  J4  4-pin magnetic pogo → battery pack          │
│  J5  JST-PH2 → 400mAh keep-alive cell           │
│  J6  JST-GH2 → mic pigtail                      │
│  J7  2.5mm ext PTT jack                          │
│  SW1 EC11 rotary encoder (vol + mute, socketed)  │
│  D1  WS2812B RGB status LED                      │
└─────────────────────────────────────────────────┘
         │ 4-pin magnetic pogo
┌─────────────────────────────────────────────────┐
│              Battery Pack (swappable)            │
│  Slim:     1x LP805060  2000mAh  ~7hr           │
│  Standard: 2x LP805060  4000mAh  ~14hr  ← default│
│  Chonker:  4x LP805060  8000mAh  ~28hr          │
│  Each pack: BQ25895 fast charge + USB-C          │
└─────────────────────────────────────────────────┘
```

## Audio Signal Flow

```
Phone (A2DP) ──────────────────────┐
                                    ├─► ESP32-S3 mixer ─► PCM5102A DAC ─► TPA6132A2 ─► IEM
BM62 radio (HFP) ──────────────────┤         │
                                    │    duck logic
Wired radio (TRRS J3) ─────────────┘    5s silence timer
         │
         └─► squelch detect on IO40
             PTT drive on IO1
```

## Radio Compatibility

### Wired (J3 TRRS — recommended)
Uses CTIA pinout: Tip=SPK, Ring1=MIC, Ring2=PTT, Sleeve=GND

| Radio | Adapter needed |
|-------|---------------|
| BTECH UV-PRO | K1 cable (included) |
| Baofeng UV-5R series | K1 cable (included) |
| Kenwood TH series | K1 cable (included) |
| Wouxun, TYT, Retevis | K1 cable (included) |
| Yaesu HTs | Yaesu→K1 adapter (common, ~$8) |
| Icom HTs | Icom→K1 adapter (~$8) |
| Yaesu FTM-400 (mobile) | Yaesu SP-10 cable (v2) |

### Bluetooth (BM62 — BT Radio #2)
Any radio supporting Bluetooth HFP profile. Tested: BTECH UV-PRO.

## Repository Layout

```
moto-audio-unit/
├── hardware/
│   ├── kicad/              KiCad schematic + PCB layout files
│   ├── bom/                Bill of materials (CSV, JLCPCB-ready)
│   └── battery-packs/      Battery pack PCB designs (slim/standard/chonker)
├── firmware/
│   ├── main/               ESP-IDF main application
│   ├── components/         ESP-IDF components (BM62, PCM5102, audio mixer)
│   └── config/             sdkconfig and Kconfig defaults
├── mechanical/
│   ├── enclosure/          Main unit enclosure STLs + Fusion source
│   ├── battery-packs/      Battery pack shell STLs (parametric)
│   ├── helmet-connector/   5-pin pogo collar STLs
│   └── magnetic-pogo/      Battery contact pogo array STLs
├── cables/
│   └── k1-adapter/         K1 cable wiring diagram + assembly notes
├── docs/
│   ├── ARCHITECTURE.md     Full system architecture
│   ├── FIRMWARE.md         Firmware build + flash guide
│   ├── HARDWARE.md         PCB assembly + BOM notes
│   ├── BATTERY.md          Battery pack build guide
│   ├── HELMET-CABLE.md     Helmet connector + IEM setup
│   └── RADIO-COMPAT.md     Radio compatibility + cable guide
└── .github/
    └── ISSUE_TEMPLATE/     Bug report + feature request templates
```

## Build Status

| Component | Status |
|-----------|--------|
| Schematic Rev 2.0 | ✅ Complete |
| BOM | ✅ Complete (JLCPCB LCSC part numbers) |
| PCB layout | 🔲 In progress |
| Enclosure STL | 🔲 In progress |
| Battery pack STL | 🔲 In progress |
| Firmware — BT A2DP sink | 🔲 Planned |
| Firmware — BM62 HFP | 🔲 Planned |
| Firmware — audio mixer + duck | 🔲 Planned |
| Firmware — wired radio | 🔲 Planned |
| Firmware — power mux | 🔲 Planned |

## Who Is This For

- Ham radio operators (General or higher) who ride motorcycles
- Anyone frustrated with Cardo/Sena's closed ecosystem
- Riders who want music + comms without manual switching
- Anyone who wants to use their own IEMs instead of helmet speakers

## License

Hardware: CERN-OHL-S v2 (strongly reciprocal open hardware)
Firmware: GPL v3
Documentation: CC BY-SA 4.0

## Builders

- Steve — WR250R — BTECH UV-PRO
- Brad — KTM 890 ADV R — BTECH UV-PRO
- David — Jeep Gladiator support vehicle — Yaesu FTM-400XD (separate unit TBD)
