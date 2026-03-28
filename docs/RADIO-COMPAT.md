# Radio Compatibility

## Wired Interface (J3)

The wired radio jack uses a standard 3.5mm TRRS connector with CTIA pinout:

| Pin | Signal | Direction |
|-----|--------|-----------|
| Tip | RADIO_SPK | Radio → device (audio in) |
| Ring 1 | RADIO_MIC | Device → radio (audio out) |
| Ring 2 | PTT | Device drives low to TX; detects low for RX squelch |
| Sleeve | GND | Common ground |

The device detects cable insertion via SW_DETECT (J3 pin 5) → IO40.

## K1 Adapter Cable (included)

The included cable converts from the device's TRRS jack to the
Kenwood K1 2-pin connector used by most ham HTs:

```
Device side (TRRS 3.5mm plug)     Radio side (K1 connector)
─────────────────────────────     ──────────────────────────
Tip   (RADIO_SPK) ─────────────► 3.5mm plug Tip (SPK)
Ring1 (RADIO_MIC) ─────────────► 3.5mm plug Ring (MIC)
Ring2 (PTT)       ─────────────► 2.5mm plug Tip (PTT)
Sleeve (GND)      ─────────────► 3.5mm plug Sleeve (GND)
                                 2.5mm plug Sleeve (GND)
```

## Compatibility Table

### Native K1 (no adapter, use included cable)

| Radio | Notes |
|-------|-------|
| BTECH UV-PRO | Primary tested radio |
| BTECH UV-5X3, UV-5R series | All Baofeng/BTECH K1 models |
| Baofeng UV-5R, BF-F8HP, UV-82 | Ubiquitous budget HT |
| Kenwood TH-D74A, TH-F6A | High-end ham HTs |
| Kenwood TH-K20A, TH-K40A | VHF/UHF portables |
| Wouxun KG-UV8D, KG-UV9D | Popular Wouxun models |
| TYT MD-380, MD-9600 | DMR capable |
| Retevis RT3S, RT82 | DMR capable |
| AnyTone AT-D878UV | Popular DMR HT |
| BTECH GMRS-V1, GMRS-20V2 | GMRS (no license required) |

### Via off-shelf adapter cable (~$5-10 on Amazon)

| Radio | Adapter needed |
|-------|---------------|
| Yaesu FT-65, FT-70D, FT-60 | Yaesu → Kenwood adapter |
| Yaesu FT-1DR, FT-2DR | Yaesu → Kenwood adapter |
| Icom IC-T70A, IC-T10 | Icom → Kenwood adapter |
| Icom IC-V86, IC-U86 | Icom → Kenwood adapter |
| Motorola CLS1410, CLP1080 | Motorola → Kenwood adapter |

### Direct 3.5mm (use TRRS to TRRS cable)

| Radio | Notes |
|-------|-------|
| Yaesu FTM-400XD (front 3.5mm) | Use TRRS extension, check pinout |
| Radios with 3.5mm headset jacks | Verify CTIA vs OMTP pinout |

### Via custom cable (DIY, pinout in docs)

| Radio | Notes |
|-------|-------|
| Yaesu FTM-400XD (SP-10 port) | 6-pin mini-DIN → TRRS |
| Yaesu FT-991A | 6-pin mini-DIN → TRRS |
| Icom IC-7300 (ACC port) | 8-pin DIN → TRRS |

## Bluetooth Radio Interface

The BM62 (U2, BT Radio #2) pairs to any radio supporting:
- Bluetooth Classic HFP (Hands-Free Profile)

Tested:
- BTECH UV-PRO (HFP confirmed)

Should work (HFP certified):
- Any Bluetooth headset-compatible radio
- Most modern dual-band HTs with BT

BT radio audio quality note: HFP uses narrowband (8kHz) or wideband
(16kHz) codec. Ham radio audio is already narrowband FM, so there's
no perceptible quality difference vs wired. For music, always use
the phone A2DP path (ESP32 native BT).

## PTT Behavior

### Wired radio RX
When the radio receives a signal, the squelch opens. The radio's
speaker output (Ring2) goes active. The device detects this via
audio level monitoring on RADIO_SPK (IO40) and triggers ducking.

### Wired radio TX
To transmit: press the external PTT button (J7 2.5mm jack) or
a future firmware-controlled PTT. The device:
1. Opens the radio MIC path (RADIO_MIC line goes active)
2. Routes MEMS mic audio to RADIO_MIC
3. Pulls PTT line low (IO1, open-drain) to key the radio
4. Releases PTT line when button released

Note: The BS-PTT accessory (BTECH) connects directly to the UV-PRO
via Bluetooth and keys the radio independently. This works in parallel
with the wired path — both can be used simultaneously.

### BT radio RX (BM62)
BM62 receives audio from paired radio via HFP. When audio is present
on the HFP audio channel, the device detects this and triggers ducking.

### BT radio TX (BM62)
The BM62 can open the HFP mic channel and transmit voice to the paired
radio. PTT can be triggered by:
- External PTT button (J7) — fires AT command via UART to BM62
- Future: hardware button on device

## Making Custom Cables

TRRS pinout on device side (CTIA standard):

```
      ┌─ Tip:    RADIO_SPK (audio from radio)
3.5mm ├─ Ring1:  RADIO_MIC (audio to radio)
TRRS  ├─ Ring2:  PTT (active low, pull to GND to TX)
      └─ Sleeve: GND
```

For radios where PTT is active-high, add a small inverter circuit or
use firmware configuration (future) to invert PTT polarity.

For radios with separate SPK and MIC jacks (common on mobile rigs),
you'll need a splitter cable — contact us or see the community wiki.
