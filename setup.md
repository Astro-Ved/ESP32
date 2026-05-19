# CrowpanelTV Firmware Setup Guide

Welcome to the CrowpanelTV firmware project! This guide will help you compile, flash, and configure your ESP32-based Elecrow 2.4" Crowpanel display to run the TV-like GUI with media playback features.

## Hardware Configuration

This firmware utilizes the dual-core architecture of the ESP32.
- **Core 1** manages the User Interface, Touch inputs, and Wi-Fi Scanning.
- **Core 0** handles intensive I/O operations including MJPEG decoding, RGB565 rendering, and I2S DAC Audio Streaming.

### SPI Bus Segregation
To ensure maximum performance and stability, the firmware splits the hardware SPI buses:
- **Display & Touch:** Uses the hardware **HSPI** bus.
- **SD Card Reader:** Uses the hardware **VSPI** bus.

### Default Pinout
*The Display & Touch pins are fixed at compile time. The SD and Audio pins can be changed dynamically at runtime.*

#### Fixed Pins (TFT_eSPI / HSPI)
| Component | Pin |
| :--- | :--- |
| TFT MISO | 12 |
| TFT MOSI | 13 |
| TFT SCLK | 14 |
| TFT CS | 15 |
| TFT DC | 2 |
| TFT RST | -1 (Tied to EN) |
| TFT Backlight | 27 |
| Touch CS | 33 |

#### Dynamic Pins (VSPI & I2S)
| Component | Default Pin |
| :--- | :--- |
| SD MISO | 19 |
| SD MOSI | 23 |
| SD SCLK | 18 |
| SD CS | 5 |
| I2S BCLK | 26 |
| I2S LRC | 25 |
| I2S DOUT | 22 |

*(Note: While I2S pins can be configured, the firmware is currently set up to output audio using the ESP32's Internal DAC which is hardwired to pins 25 and 26).*

---

## Software Prerequisites

This project uses `arduino-cli` to guarantee reproducible builds without modifying your global Arduino IDE library source files.

You must be on a Linux/macOS environment (or WSL on Windows). `arduino-cli` is downloaded and managed automatically by the build script.

---

## Build Instructions

1. Clone or download this repository.
2. Open a terminal and navigate to the project root.
3. Make the build script executable (if it isn't already):
   ```bash
   chmod +x build.sh
   ```
4. Run the build script:
   ```bash
   ./build.sh
   ```

The script will automatically:
- Download `arduino-cli` locally.
- Install the `esp32:esp32` board core.
- Install required dependencies (`TFT_eSPI` and `JPEGDEC`).
- Compile the code while dynamically injecting the required `TFT_eSPI` hardware configs.
- Output the generated `.bin` files into the `build/` directory.

### Flashing the Firmware
You can flash the generated `.bin` file directly using `esptool.py` or the Arduino IDE.
Alternatively, to flash via the command line using `arduino-cli` directly, run:

```bash
./bin/arduino-cli upload -p /dev/ttyUSB0 --fqbn esp32:esp32:esp32 -i build/CrowpanelTV.ino.bin
```
*(Replace `/dev/ttyUSB0` with your ESP32's actual COM port).*

---

## Serial Configuration CLI

The CrowpanelTV firmware allows you to dynamically change the SD Card and I2S pins without recompiling the code. The configuration is saved to the ESP32's internal Non-Volatile Storage (NVS).

1. Connect your ESP32 to your PC and open a Serial Monitor (baud rate **115200**).
2. Use the following commands:

**View current pin mapping:**
```text
print_map
```

**Change a specific pin:**
```text
set_pin <pin_name> <gpio_number>
```
*Valid pin names are: `sd_miso`, `sd_mosi`, `sd_sclk`, `sd_cs`, `i2s_bclk`, `i2s_lrc`, `i2s_dout`.*

*Example: Change the SD Card Chip Select to pin 4:*
```text
set_pin sd_cs 4
```

**Reboot the device (to apply changes):**
```text
reboot
```

---

## Media Files
To use the media player, place a `.mjpeg` or `.rgb` (RGB565 raw format) file named `video.mjpeg` or `image.rgb` on the root of a FAT32-formatted SD card. Navigate to the "Media" tab on the device, and playback will commence automatically.
