# reSpeaker XVF3800 + XIAO ESP32S3 — XiaoZhi Build & Flash Guide

> Board/firmware: **respeaker-xvf3800** (`type`/`name`), XiaoZhi project version **2.4.2**
> This README documents how to **compile and push** this firmware to the
> Seeed **reSpeaker XVF3800 4-Mic Array with XIAO ESP32S3** device, and
> **what source files were changed to support it**.

---

## 1. TL;DR — build and flash

Open a PowerShell window and run:

```powershell
# ---- activate ESP-IDF v6.0.2 (one-time per terminal) ----
Set-ExecutionPolicy -Scope Process -ExecutionPolicy Bypass -Force
$env:IDF_TOOLS_PATH="D:\Espressif"
$env:IDF_PATH="D:\Espressif\frameworks\esp-idf-v6.0.2"
$env:TEMP="D:\Espressif\tmp"; $env:TMP="D:\Espressif\tmp"
$env:PIP_CACHE_DIR="D:\Espressif\pipcache"
$env:PATH="C:\Users\seeed\AppData\Local\Programs\Python\Python311;$env:PATH"
& "D:\Espressif\frameworks\esp-idf-v6.0.2\export.ps1"

# ---- compile (keep the wake word you want; see below) ----
cd D:\xiaozhi_9_2\xiaozhi-esp32-main\xiaozhi-esp32-main
python scripts/build.py respeaker-xvf3800 --name respeaker-xvf3800 --wake-word wn9_jarvis_tts

# ---- flash the merged image to the device ----
python -m esptool --chip esp32s3 -p COM10 -b 460800 write-flash 0x0 build\merged-binary.bin

# ---- watch the logs ----
idf.py -p COM10 monitor
```

- Device must be connected and shown as a COM port (its native USB-C / USB Serial‑JTAG).
- If the port is busy (`Access is denied`), close any running monitor first:
  ```powershell
  Get-CimInstance Win32_Process -Filter "Name='python.exe'" |
    Where-Object { $_.CommandLine -like '*esp-idf-v6.0.2*' -and $_.CommandLine -like '*COM10*' } |
    ForEach-Object { Stop-Process -Id $_.ProcessId -Force }
  ```
- Quit the monitor with `Ctrl+]`.

---

## 2. Hardware and prerequisites

| Item | Detail |
|---|---|
| Host MCU | Seeed XIAO ESP32S3 (8 MB flash, octal PSRAM) — part of the reSpeaker‑XIAO device |
| Mic array / DSP | reSpeaker XVF3800 4‑Mic Array running the **I2S firmware** (2‑ch, 32‑bit, 16 kHz) |
| Speaker | reSpeaker **JST speaker connector** (5 W amp) or **3.5 mm jack** (active speaker/headphones) |
| Audio link | I2S STD full duplex, ESP32 is **master**, **16 kHz stereo 32‑bit** (matches Seeed's reference) |
| Control | I2C used only to probe the XVF3800 at `0x2C` (boot log) |
| Toolchain | **ESP-IDF v6.0.2** (see ESP-IDF Tools setup in `docs/esp-idf-6-migration.md`) |

The XMOS firmware on the reSpeaker itself must be an `i2s` DFU image
(`respeaker_xvf3800_i2s_dfu_firmware_v1.0.x.bin`). It is flashed to the XMOS
(not the ESP32) with `dfu-util` — see Seeed's reSpeaker getting-started guide.
This step is independent of the files below.

Wiring (embedded on the reSpeaker‑XIAO board):

| XIAO GPIO | Signal | | XIAO GPIO | Signal |
|---|---|---|---|---|
| 8 | I2S BCLK | | 44 | I2S DOUT (ESP32 → XVF playback / AEC ref) |
| 7 | I2S WS / LRCK | | 43 | I2S DIN (XVF → ESP32 processed mic) |
| 5 | I2C SDA (probe) | | 6 | I2C SCL (probe) |

> GPIO43/44 are the default UART0 console pins. The build forces the console to
> **USB Serial/JTAG** so these pins are free for I2S.

---

## 3. Build options (language / wake word)

Wake words are a *per-build* option — the repo default is `nihaoxiaozhi`
(你好小智), so **include `--wake-word` on every build** you want to keep:

```powershell
# list valid model ids (English examples: jarvis, alexa, computer, heywillow, ...)
python scripts/build.py --list-wake-words

python scripts/build.py respeaker-xvf3800 --name respeaker-xvf3800 --wake-word wn9_jarvis_tts
```

Language is independent:

```powershell
python scripts/build.py respeaker-xvf3800 --name respeaker-xvf3800 --language en-US
```

After a successful build you get:

- `build/xiaozhi.bin` + `build/merged-binary.bin` (flash this one at `0x0`)
- Reported board: `SKU=respeaker-xvf3800`, app version 2.4.2

---

## 4. First run / network setup

1. Boot the device. Log should show:
   `Board: UUID=... SKU=respeaker-xvf3800`,
   `RespeakerXvf3800: reSpeaker XVF3800 detected at 0x2C`,
   `RespeakerCodec: Stereo 32-bit duplex channels created (16000 Hz)`.
2. It starts a hotspot `Xiaozhi-XXXXXX` (Web server on `http://192.168.4.1`).
3. Phone → join that hotspot → enter your 2.4 GHz Wi-Fi.
4. After it reconnects, say the wake word (e.g. **"Jarvis"**) and register the
   6‑digit code on your XiaoZhi server (e.g. https://xiaozhi.me).

### No playback / no sound?

- Confirm the speaker is attached (JST connector or 3.5 mm with active
  speaker/headphones) and that output is not muted.
- The I2S link is **stereo** by design — the previous mono setting produced
  silence on this hardware (fixed in `respeaker_xvf3800_audio_codec.cc`).
- Next hardware/software checks if still silent: XVF3800 amp enable (X0D31,
  active low), AIC3104 output gain, or re-flash the XMOS I2S firmware.

---

## 5. Changed files (emphasis)

Everything needed to support this device lives under **one new board
directory** plus **two build-wiring edits**. **No core code was modified** —
`main/application.*`, `main/audio/*`, `main/protocols/*`, and no other
board's files were touched.

### 5.1 New board files — `main/boards/respeaker-xvf3800/`

| File | Purpose |
|---|---|
| **`config.h`** | All pin/settings macros: 16 kHz rates, I2S GPIO 8/7/44/43, I2C probe 5/6 + `XVF3800_I2C_ADDR 0x2C`, boot button GPIO0, no display/LED |
| **`respeaker_xvf3800_board.cc`** | Board class `RespeakerXvf3800Board : WifiBoard`: probes XVF3800 at boot, boot-button chat/wifi-config, `DECLARE_BOARD` |
| **`respeaker_xvf3800_audio_codec.h`** | Board-local stereo codec declaration |
| **`respeaker_xvf3800_audio_codec.cc`** | **Audio fix**: full-duplex **16 kHz stereo 32-bit** I2S (matches the XVF3800). TX duplicates each mono sample to L+R slots (playback/AEC ref), RX takes left slot (Channel 0). Fixes silent playback caused by the default mono codec |
| **`config.json`** | New OTA identity: `"type": "respeaker-xvf3800"`, `"target": "esp32s3"`, build variant `respeaker-xvf3800`, sdkconfig: 8 MB flash, `partitions/v2/8m.csv`, console USB Serial/JTAG |
| **`README.md`** | Board-local notes (this same content, board-scoped) |

### 5.2 Build-wiring edits (only two existing files touched)

| File | Where | What changed |
|---|---|---|
| **`main/Kconfig.projbuild`** | inside `choice BOARD_TYPE` | Added entry `BOARD_TYPE_RESPEAKER_XVF3800` → `bool "Seeed XIAO ESP32S3 + reSpeaker XVF3800"` with `depends on IDF_TARGET_ESP32S3` (label kept sorted) |
| **`main/CMakeLists.txt`** | board branch chain | Added `elseif(CONFIG_BOARD_TYPE_RESPEAKER_XVF3800)  set(BOARD_DIR "respeaker-xvf3800")` so board sources auto-glob and `config.json` supplies the identity |

### 5.3 Not changed (on purpose)

- `main/main.cc`, `main/application.*`, all of `main/audio/`, `main/protocols/`,
  `scripts/build.py`, partitions, and every other board directory are untouched.
- The XMOS I2S firmware on the reSpeaker is flashed with `dfu-util` and is not
  part of this repository.

---

## 6. Notes / troubleshooting

- **Wake word resets**: rebuilds without `--wake-word` revert to the default
  `nihaoxiaozhi` — pass `--wake-word wn9_jarvis_tts` each time.
- **COM port busy**: only one monitor/flash process may hold the port (see §1).
- **Monitor shows only the wrapper version** (`v1.0.3`) for `idf.py --version`;
  check the real IDF with `python "$env:IDF_PATH\tools\idf.py" --version`.
- Re-flash is idempotent; full-image `merged-binary.bin` at offset `0x0`
  programs bootloader + partitions + app + assets in one step.
