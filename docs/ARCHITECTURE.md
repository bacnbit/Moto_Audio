# System Architecture

## Block Diagram

```
┌──────────────────────────────────────────────────────────────────┐
│                         MAIN UNIT                                │
│                                                                  │
│  ┌─────────────┐    UART0    ┌──────────────┐                   │
│  │             │◄───────────►│  BM62 (U2)   │◄──── BT Radio #2 │
│  │  ESP32-S3   │    UART1    │  Radio HFP   │      (UV-PRO etc) │
│  │    (U1)     │◄───────────►├──────────────┤                   │
│  │             │             │  BM62 (U3)   │◄──── BT Radio #3  │
│  │  BT Radio   │             │  Expansion   │      (future)     │
│  │    #1       │◄────────────┴──────────────┘                   │
│  │  (phone     │  I2S shared bus                                 │
│  │   A2DP)     │                                                 │
│  │             │    I2S      ┌──────────────┐   analog   ┌────┐ │
│  │             │────────────►│  PCM5102A    │───────────►│TPA │ │
│  │             │             │  DAC (U4)    │            │6132│ │
│  │             │◄────────────┤              │            │(U5)│ │
│  │             │  I2S PDM    └──────────────┘            └──┬─┘ │
│  │             │                                            │   │
│  │             │◄── IO40 squelch detect                     │   │
│  │             │──► IO1  PTT drive                          │   │
│  │             │◄── IO2  PTT sense                          │   │
│  │             │◄── IO15 ext PTT                            │   │
│  │             │◄── IO3  BAT_DATA/NTC                       │   │
│  └─────────────┘                                            │   │
│        ▲  ▲                                                 │   │
│        │  │ I2S PDM                                         │   │
│        │  └────────┐                                        │   │
│        │           │                                        │   │
│   ┌────┴────┐  ┌───┴──────┐                                │   │
│   │ IP5306  │  │ Knowles  │                                 │   │
│   │  boost  │  │ SPM0687  │                                 │   │
│   │  (U6)   │  │ mic (J6) │                                 │   │
│   └────┬────┘  └──────────┘                                │   │
│        │                                                    │   │
│   ┌────┴──────────────────────────────────────────────┐    │   │
│   │              TPS2116 power mux (U7)               │    │   │
│   │  IN1 = main pack (J4 pogo)  priority              │    │   │
│   │  IN2 = keep-alive (J5 JST)  fallback <1µs         │    │   │
│   └────────────────────────────────────────────────────┘   │   │
│                                                             │   │
└─────────────────────────────────────────────────────────────┼───┘
                                                              │
                                              ┌───────────────▼───┐
                                              │   J2: 5-pin Pogo  │
                                              │   Helmet connector │
                                              │   (magnetic, keyed)│
                                              └───────┬───────────┘
                                                      │ cable
                                              ┌───────▼───────────┐
                                              │  IEM 3.5mm plug   │
                                              │  + Knowles pigtail│
                                              │  (chin bar mount) │
                                              └───────────────────┘
```

## Audio Mixing Logic

The ESP32 runs a software audio mixer with priority-based routing:

### Input Sources (priority order)
1. **Wired radio** (J3 TRRS) — highest priority; squelch detect on IO40
2. **BT radio** (BM62 U2, HFP) — detected via HFP audio channel activity
3. **Phone** (ESP32 native BT, A2DP) — background; always playing unless ducked

### Ducking Behavior
```
Radio signal detected
    → Phone audio attenuated by -20dB over 50ms
    → Radio audio at full volume
    → Mic enabled (MEMS into radio TX path)

Radio signal ends
    → 5-second silence timer starts
    → Timer expires → phone audio fades back in over 200ms
    → Mic disabled
```

### Sidetone
A -20dB mix of the mic signal is fed back into the IEM output so you
can hear yourself speak. Prevents shouting. Configurable in firmware.

## Power Architecture

```
USB-C (J1)
    │ VBUS 5V
    ├──► IP5306 VIN (keep-alive charger path)
    └──► VBUS_CHGPASS → J4 pogo pin 3 → battery pack charge IC

Battery pack (J4 pogo)
    │ BAT+ 3.7V
    └──► TPS2116 IN1

Keep-alive cell (J5 JST)
    │ BAT+ 3.7V
    ├──► TPS2116 IN2
    └──► MCP73831 charges from 5V USB

TPS2116 VOUT (muxed 3.7V)
    └──► IP5306 BAT+ input
         │
         └──► IP5306 VOUT 5V
              ├──► AMS1117-3.3 → 3.3V rail
              │    ├── ESP32-S3
              │    ├── BM62 x2
              │    ├── PCM5102A DVDD
              │    ├── PCM5102A AVDD (via FB1 ferrite)
              │    └── Knowles SPM0687
              └──► TPA6132A2 VDD (5V direct)
```

### Hot-swap Sequence
1. User grabs battery pack, pulls away from main unit
2. Pogo contacts break — main pack voltage drops to zero on IN1
3. TPS2116 detects IN1 < IN2, switches to keep-alive IN2 in <1µs
4. System continues running on keep-alive cell
5. All BT connections maintained (ESP32, BM62 x2 stay powered)
6. User snaps new pack onto main unit
7. TPS2116 detects IN1 > IN2, switches back to main pack
8. MCP73831 begins recharging keep-alive cell from USB/pack VBUS
9. Zero user action required — music never stopped

## Bluetooth Architecture

```
┌──────────────┐     BT Classic A2DP     ┌──────────────┐
│    Phone     │◄───────────────────────►│  ESP32-S3    │
│              │     BT Classic HFP      │  (BT Radio   │
│              │◄───────────────────────►│    #1)       │
└──────────────┘                         └──────────────┘

┌──────────────┐     BT Classic HFP      ┌──────────────┐
│  UV-PRO or   │◄───────────────────────►│  BM62 (U2)   │
│  any HFP     │                         │  (BT Radio   │
│  radio       │                         │    #2)       │
└──────────────┘                         └──────────────┘

┌──────────────┐     BT Classic / BLE    ┌──────────────┐
│  BS-PTT /    │◄───────────────────────►│  BM62 (U3)   │
│  2nd phone / │                         │  (BT Radio   │
│  intercom    │                         │    #3)       │
└──────────────┘                         └──────────────┘
```

### BM62 Control
Both BM62 modules are controlled via UART from the ESP32:
- UART0 (IO41/42) → BM62 U2 (radio)
- UART1 (IO43/44) → BM62 U3 (expansion)

The BM62 uses a simple ASCII command protocol over UART. Key commands:
- Pairing mode, connection management
- HFP call state reporting
- Audio routing control
- Volume sync

### TX Detection (Wired Radio)
When the radio enters TX mode via the wired PTT line (J3 Ring2):
- IO2 detects PTT signal (active low, R10 pullup)
- ESP32 routes MEMS mic audio into the radio MIC path (J3 Ring1)
- IO1 drives PTT line to radio (active low open-drain)

When TX via BT radio (BM62 U2 HFP):
- BM62 notifies ESP32 via UART that HFP mic channel opened
- ESP32 routes MEMS mic into BM62 I2S mic path
- BM62 transmits to radio over BT HFP

## Enclosure

Main unit: 80×50×20mm ASA
Battery packs: 80×50×(8/18/36)mm ASA — same XY, different Z height
Helmet connector collar: printed, keyed, N52 ring magnet embedded

Mounting: 3M Dual Lock strip on back of main unit, or printed shoulder
strap clip (25mm webbing), or RAM Mount B-ball base for bike mounting.

## LED Status Codes (WS2812B D1)

| Color | Pattern | Meaning |
|-------|---------|---------|
| Blue | Solid | Phone connected (A2DP) |
| Blue | Slow pulse | Phone pairing mode |
| Green | Solid | Radio BT connected (BM62 U2) |
| Green | Slow pulse | Radio BT pairing mode |
| Cyan | Solid | Both phone + radio connected |
| Amber | Flash | Radio RX active (ducking music) |
| Red | Flash | PTT TX active |
| Purple | Solid | Expansion BT connected (BM62 U3) |
| White | Slow pulse | Booting |
| Red | Solid | Error / low battery |
| Off | — | Sleeping / off |
