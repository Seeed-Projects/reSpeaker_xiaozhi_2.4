# reSpeaker XVF3800 (XIAO ESP32S3 host)

XiaoZhi board for the Seeed XIAO ESP32S3 wired to a **reSpeaker XVF3800 USB
4-Mic Array**. The XMOS XVF3800 performs far-field pickup, beamforming, and
acoustic echo cancellation and exchanges clean PCM with the ESP32 over a
full-duplex I2S link, so the ESP32 needs **no codec chip and no device-side
AEC**.

## Hardware

- Host MCU: Seeed XIAO ESP32S3 (8 MB flash, octal PSRAM)
- Mic array / DSP: reSpeaker XVF3800 with the dedicated **I2S firmware** flashed
  (see https://wiki.seeedstudio.com/respeaker_xvf3800_xiao_getting_started/)
- Audio: I2S STD full-duplex, ESP32 is I2S master, 16 kHz in == 16 kHz out,
  32-bit slots (mono, left)
- Control: I2C used only to probe the XVF3800 at `0x2C` during startup (logs
  "detected"/"not detected"); no register writes

### Wiring (XIAO ESP32S3 <-> reSpeaker)

| XIAO GPIO | Breakout | Signal |
|---|---|---|
| 8 | D9 | I2S BCLK |
| 7 | D8 | I2S WS / LRCK |
| 44 | D7 | I2S DOUT (ESP32 -> XVF3800 playback / AEC ref) |
| 43 | D6 | I2S DIN (XVF3800 -> ESP32 processed mic) |
| 5 | D4 | I2C SDA (probe only) |
| 6 | D5 | I2C SCL (probe only) |
| 0 | - | BOOT button (short: talk/wake toggle; long: Wi-Fi config) |

> **Important:** GPIO43/44 are the default UART0 console pins. This board forces
> the console to **USB Serial/JTAG** (see `config.json`), which is also the
> flash/monitor interface on the XIAO's native USB-C port. Do not remove that
> sdkconfig override or the firmware will not boot correctly while I2S is
> active.

## Build

Requires ESP-IDF v6.0.x (see `docs/esp-idf-6-migration.md`):

```bash
python scripts/build.py respeaker-xvf3800 --name respeaker-xvf3800
```

Output: `build/respeaker-xvf3800.bin` / `build/merged-binary.bin`.

## Flash

1. Connect the XIAO ESP32S3 via its USB-C port.
2. If needed, put the chip into download mode (hold BOOT, plug in, release).
3. Flash the merged binary at offset 0:

```bash
python -m esptool --chip esp32s3 -p COMx write_flash 0x0 build/merged-binary.bin
```

or, with the ESP-IDF environment active:

```bash
idf.py -p COMx flash
```

## Notes

- No display: notifications are silent and setup uses the `Xiaozhi-xxxxxx`
  hotspot at `http://192.168.4.1`.
- Wake word / VAD run on the ESP32's own AFE (16 kHz, PSRAM required) on top of
  the XVF3800-cleaned stream; the XVF3800's own AEC does the echo cancellation.
- Volume is software-controlled (see the XiaoZhi audio volume setting); the
  XVF3800 control I2C register volume is not used.
