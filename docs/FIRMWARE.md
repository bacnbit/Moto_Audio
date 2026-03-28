# Firmware Guide

## Prerequisites

- ESP-IDF v5.2 or later
- Python 3.8+
- USB-C cable (data capable, not charge-only)

## Install ESP-IDF

```bash
# macOS / Linux
mkdir -p ~/esp
cd ~/esp
git clone --recursive https://github.com/espressif/esp-idf.git
cd esp-idf
git checkout v5.2
./install.sh esp32s3
source ./export.sh

# Windows: use ESP-IDF Windows Installer from Espressif
```

## Clone and Build

```bash
git clone https://github.com/YOUR_ORG/moto-audio-unit.git
cd moto-audio-unit/firmware

# Configure (first time)
idf.py set-target esp32s3
idf.py menuconfig   # review config/sdkconfig.defaults first

# Build
idf.py build

# Flash (replace PORT with your serial port)
idf.py -p /dev/ttyUSB0 flash monitor
# macOS: /dev/cu.usbserial-XXXX
# Windows: COM3 (or whatever Device Manager shows)
```

## Flashing a New Board

The ESP32-S3-WROOM-1 module has a built-in USB-Serial/JTAG controller.
No external programmer needed — just USB-C directly to J1.

If the board doesn't auto-enter download mode:
1. Hold BOOT button (IO0, exposed on PCB pad)
2. Press and release RESET (EN pin)
3. Release BOOT
4. Run `idf.py flash`

## Firmware Components

```
firmware/
├── main/
│   ├── main.c              App entry, task init
│   ├── audio_mixer.c       Core mixing + ducking logic
│   ├── audio_mixer.h
│   ├── power_manager.c     TPS2116 state, keep-alive monitor
│   ├── power_manager.h
│   ├── ptt_handler.c       Wired + BT PTT unified handler
│   └── ptt_handler.h
├── components/
│   ├── bt_a2dp/            Phone A2DP sink (ESP32 native BT)
│   ├── bm62/               BM62 UART driver + HFP control
│   ├── pcm5102/            PCM5102A I2S DAC driver
│   ├── tpa6132/            TPA6132A2 amp control (SD pin)
│   ├── spm0687/            Knowles MEMS mic PDM driver
│   ├── ws2812/             Status LED driver
│   └── wired_radio/        TRRS wired radio interface
└── config/
    ├── sdkconfig.defaults  Default Kconfig values
    └── Kconfig.projbuild   Project config menu
```

## Key Configuration (menuconfig)

```
Moto Audio Unit Config
  ├── Audio
  │   ├── Duck attenuation dB       [20]
  │   ├── Duck fade in ms           [200]
  │   ├── Radio silence timeout ms  [5000]
  │   └── Sidetone level dB         [-20]
  ├── Bluetooth
  │   ├── Device name               [MotoAudio]
  │   ├── BM62-A UART port          [UART0]
  │   └── BM62-B UART port          [UART1]
  └── Hardware
      ├── Keep-alive low threshold mV [3400]
      └── LED brightness             [32]
```

## GPIO Reference

| GPIO | Direction | Function |
|------|-----------|----------|
| IO1  | OUT | Wired radio PTT drive (active low, open-drain) |
| IO2  | IN  | Wired radio PTT sense (active low, R10 pullup) |
| IO3  | IN  | Battery pack NTC/DATA (ADC, R9 pullup) |
| IO4  | OUT | BM62-A RST (active low) |
| IO5  | OUT | I2S BCK (shared DAC + mic) |
| IO6  | OUT | I2S WS / LRCK |
| IO7  | OUT | I2S DOUT → PCM5102A DIN |
| IO8  | IN  | I2S DIN ← SPM0687 DATA |
| IO15 | IN  | External PTT jack J7 (active low, R2 pullup) |
| IO16 | IN  | EC11 encoder A (R3 pullup) |
| IO17 | IN  | EC11 encoder B (R4 pullup) |
| IO18 | IN  | EC11 encoder SW / mute (R5 pullup) |
| IO21 | OUT | Wired radio mic enable (high = active) |
| IO38 | OUT | WS2812B data (R6 series) |
| IO39 | OUT | TPA6132A2 /SD (low = shutdown) |
| IO40 | IN  | Wired radio squelch detect |
| IO41 | OUT | UART0 TX → BM62-A RX |
| IO42 | IN  | UART0 RX ← BM62-A TX |
| IO43 | OUT | UART1 TX → BM62-B RX |
| IO44 | IN  | UART1 RX ← BM62-B TX |
| IO45 | OUT | BM62-B RST (active low) |
| IO47 | OUT | BM62-B RST (active low) |
| IO48 | IN  | BM62-A status |

## BM62 UART Protocol

The BM62 uses a simple command/event protocol at 115200 8N1.

Key commands sent from ESP32:
```
# Enter pairing mode
AT+AB BtStartPairing\r\n

# Connect to paired device
AT+AB BtConnect\r\n

# HFP answer call (open audio channel)
AT+AB BtHfpAnswer\r\n

# Set HFP volume
AT+AB SetHFPVolume 10\r\n
```

Key events received from BM62:
```
# Connected
+CONNECTED:<BD_ADDR>:<profile>

# HFP audio opened (radio TX detected here)
+HFP_AUDIO_CONNECTED

# HFP audio closed
+HFP_AUDIO_DISCONNECTED

# RSSI (battery monitoring etc)
+RSSI:<value>
```

## Audio Mixer Implementation Notes

The ESP32 receives two I2S streams:
- A2DP decoded PCM from phone (via ESP-IDF BT A2DP sink stack)
- HFP decoded PCM from BM62 (via I2S shared bus)
- PDM from Knowles MEMS (via I2S PDM mode)

Mixing is done in software in a FreeRTOS task at 48kHz 16-bit stereo.
The duck logic runs as a state machine:

```c
typedef enum {
    AUDIO_STATE_MUSIC,          // Phone audio only
    AUDIO_STATE_DUCKING,        // Fading phone down, radio fading up
    AUDIO_STATE_RADIO,          // Radio audio primary
    AUDIO_STATE_UNDUCKING,      // Radio ended, fading phone back
} audio_state_t;
```

Radio detect sources (any triggers duck):
1. `IO40` goes low (wired radio squelch opens)
2. BM62-A reports `+HFP_AUDIO_CONNECTED` (BT radio RX)
3. Audio level on HFP I2S channel exceeds threshold

## OTA Updates (future)

The ESP32-S3 supports OTA via Wi-Fi. The BM62 also has built-in
Wi-Fi capability that can be leveraged. OTA update endpoint TBD.
