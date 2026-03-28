# Claude Code Handoff Brief — Moto Audio Unit

This document gives Claude Code CLI full context to continue development
without re-explaining the entire project. Read this before starting any work.

## What This Is

An open-source motorcycle helmet audio system. ESP32-S3 based device that:
- Mixes phone music (BT A2DP) with radio audio (wired or BT HFP)
- Ducks music automatically when radio has a signal, restores after 5s silence
- Three independent Bluetooth radios: phone, radio HFP, expansion
- Wired radio via TRRS 3.5mm jack (K1 adapter cable for ham HTs)
- Hot-swap battery system with magnetic pogo contacts
- High quality IEM audio: PCM5102A DAC + TPA6132A2 amp
- Differential MEMS mic (Knowles SPM0687) on helmet pigtail

## Hardware

Main chips:
- U1: ESP32-S3-WROOM-1-N8 — main MCU, BT Radio #1 (phone A2DP)
- U2: BM62SPKS1MC2 — BT Radio #2 (radio HFP, e.g. BTECH UV-PRO)
- U3: BM62SPKS1MC2 — BT Radio #3 (expansion)
- U4: PCM5102A — I2S DAC, 112dB SNR
- U5: TPA6132A2 — headphone amp, 1Ω output impedance (IEM-safe)
- U6: IP5306 — LiPo charger + 5V boost
- U7: TPS2116 — hot-swap power mux (<1µs switchover)
- U8: AMS1117-3.3 — 3.3V LDO
- U9: MCP73831 — 400mAh keep-alive cell charger (100mA)
- MIC1: Knowles SPM0687LR5H-1 — differential MEMS, PDM output

Connectors:
- J1: USB-C (charge + ESP32 flash)
- J2: 5-pin magnetic pogo → helmet cable (IEM L/R + mic diff + GND)
- J3: 3.5mm TRRS wired radio (CTIA: Tip=SPK, R1=MIC, R2=PTT, Slv=GND)
- J4: 4-pin magnetic pogo → battery pack (BAT+, GND, VBUS, NTC)
- J5: JST-PH2 → 400mAh keep-alive cell (serviceable)
- J6: JST-GH2 → Knowles mic pigtail (serviceable)
- J7: 2.5mm mono → external PTT button

GPIO map (critical ones):
- IO1:  Wired radio PTT drive (open-drain, active low)
- IO2:  Wired radio PTT sense (active low, R10 pullup)
- IO3:  Battery NTC/DATA (ADC, R9 pullup to 3V3)
- IO4:  BM62-A RST (active low)
- IO5:  I2S BCK (shared: DAC + BM62 + mic)
- IO6:  I2S WS/LRCK
- IO7:  I2S DOUT → PCM5102A DIN
- IO8:  I2S DIN ← SPM0687 PDM DATA
- IO15: External PTT jack J7 (active low, R2 pullup)
- IO16: EC11 ENC_A
- IO17: EC11 ENC_B
- IO18: EC11 ENC_SW (mute)
- IO21: Wired radio mic enable (high = active)
- IO38: WS2812B data (R6 100Ω series)
- IO39: TPA6132 /SD (low = shutdown)
- IO40: Wired radio squelch / cable detect
- IO41: UART0 TX → BM62-A
- IO42: UART0 RX ← BM62-A
- IO43: UART1 TX → BM62-B
- IO44: UART1 RX ← BM62-B
- IO47: BM62-B RST (active low)
- IO48: BM62-A STATUS

## Firmware Structure

```
firmware/
├── main/
│   ├── main.c              App entry, task init — GPIO done, TODOs marked
│   ├── audio_mixer.c/h     Mixing + duck logic — skeleton complete, I2S TODO
│   ├── power_manager.h     Header done, .c stub
│   └── ptt_handler.h       Header done, .c stub
└── components/
    ├── bm62/               FULLY IMPLEMENTED — UART driver, event callbacks
    ├── wired_radio/        FULLY IMPLEMENTED — GPIO, state machine, monitor task
    ├── power_manager/      STUB — needs ADC reads for NTC + keep-alive voltage
    ├── ptt_handler/        STUB — needs GPIO ISR + routing logic
    └── ws2812/             STUB — needs RMT peripheral driver for NZR protocol
```

## What Needs Doing (priority order)

### 1. ws2812 component
Implement `ws2812.c` using ESP-IDF RMT peripheral (esp_rmt.h).
Single WS2812B-2020 on IO38. Brightness controlled via global scale.
Pattern animations: solid, slow_pulse (1Hz), fast_flash (4Hz).
Map led_state_t enum to colors per ARCHITECTURE.md LED table.

### 2. power_manager component
Implement `power_manager.c`:
- ADC read on IO3 via `esp_adc/adc_oneshot.h`
- R9 is 10k pullup to 3V3; NTC is 10k @ 25°C (B=3950K)
- Steinhart-Hart equation for temperature
- Voltage divider math for keep-alive cell estimation
- Monitor task at 1Hz, set event bits on threshold crossings

### 3. ptt_handler component
Implement `ptt_handler.c`:
- GPIO interrupt on IO15 (ext PTT jack J7), active low
- On press: call wired_radio_tx_start() if wired connected,
  else call bm62_hfp_open_audio(BM62_MODULE_A) if BT radio connected
- On release: call wired_radio_tx_end() / bm62_hfp_close_audio()
- Debounce: 20ms

### 4. audio_mixer.c I2S integration
The mixer task skeleton is in place. Need to:
- Initialize I2S TX channel to PCM5102A (IO5/6/7)
- Initialize I2S RX channel from BM62 / SPM0687 (IO8, PDM mode)
- Wire ESP32 A2DP audio callback into phone_buf ring buffer
- Wire BM62 I2S audio into radio_buf
- Actually write out_buf to i2s_channel_write()

### 5. main.c — wire everything together
The app_main has clear TODO comments. Initialize components in order,
pass event callbacks that route to audio_mixer_set_radio_active().

### 6. bt_a2dp component (not yet created)
Create `components/bt_a2dp/` using ESP-IDF classic BT A2DP sink stack.
Reference: `$IDF_PATH/examples/bluetooth/bluedroid/classic_bt/a2dp_sink/`
Deliver decoded PCM to audio_mixer via ring buffer.

## Build

```bash
cd firmware
idf.py set-target esp32s3
cp config/sdkconfig.defaults sdkconfig.defaults
idf.py build
idf.py -p /dev/ttyUSB0 flash monitor
```

## Testing Approach

Without hardware yet, you can:
1. `idf.py build` — verify compilation
2. Unit test audio_mixer duck logic with mock inputs (no I2S needed)
3. Test BM62 UART driver with a logic analyzer or UART loopback

## Key Design Decisions (don't change without discussion)

- TPS2116 hot-swap: hardware handles <1µs switchover, firmware just monitors
- Duck timeout: 5000ms after radio silence before restoring phone
- Duck attenuation: -20dB (not silence — radio audio still heard as cue)
- Sidetone: -20dB (user hears themselves slightly — prevents shouting)
- BM62 control: UART AT commands, not I2C (simpler, more reliable)
- Audio priority: wired radio > BT radio > phone (immutable)
- All wear items on connectors (JST, socketed encoder) — no hard-soldered cells

## Radio Compat Context

Primary radios: BTECH UV-PRO (both Steve and Brad)
- Can pair to BM62-A via BT HFP OR connect wired via K1 cable
- Wired preferred for reliability; BT as option when wired is inconvenient

David's Yaesu FTM-400XD in support Jeep — separate device, out of scope for now.

## Open Questions / Future Work

- OTA firmware updates (ESP32 supports this, BM62 also has OTA)
- BM62-B expansion use cases: unit-to-unit intercom between Steve and Brad?
- Battery pack PCB design (BQ25895 fast charge, see docs/BATTERY.md)
- Enclosure STLs (need PCB layout finalized first for exact dimensions)
- KiCad PCB layout (schematic complete in hardware/kicad/)
- iOS/Android config app (change duck timing, volume preset, pairing UI)
