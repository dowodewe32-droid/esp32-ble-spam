# ESP32 BLE Spam Web Controller

ESP32 firmware for BLE spam with web-based control interface.

## Features

- **Web UI Control** - Control BLE spam from any browser
- **25 Device Payloads** - Apple, Samsung, Android, Windows devices
- **Random MAC** - Toggle random MAC address
- **Adjustable Interval** - 100-5000ms advertising interval
- **WiFi Connected** - Auto-connect to GMpro

## Hardware

- ESP32 Devkit V1 (4MB Flash)
- USB Cable

## WiFi Configuration

- **SSID:** GMpro
- **Password:** Sangkur87

(Fallback: AP mode "BLE-Spam" / password "12345678")

## Quick Start

### Option 1: GitHub Actions Build

1. Fork this repository
2. Go to Actions tab
3. Click "Build ESP32 Firmware"
4. Download artifacts

### Option 2: Download Pre-built

Download latest firmware from [Releases](../../releases)

### Option 3: Compile Locally

```bash
pio run
pio run --target upload
```

## Flashing

### Using esptool.py

```bash
pip install esptool

esptool.py --chip esp32 --port /dev/ttyUSB0 --baud 115200 write_flash \
  0x1000 bootloader.bin \
  0x8000 partitions.bin \
  0x10000 esp32-ble-spam.bin
```

### Using ESP32 Flash Download Tool (Windows)

| File | Offset |
|------|--------|
| bootloader.bin | 0x1000 |
| partitions.bin | 0x8000 |
| esp32-ble-spam.bin | 0x10000 |

## Usage

1. Connect ESP32 to USB
2. Open Serial Monitor (115200 baud)
3. Wait for WiFi connection
4. Note IP address (e.g., `192.168.1.100`)
5. Open browser to `http://192.168.1.100`
6. Select device and click **START SPAM**

## Web Interface

- **START SPAM** - Begin BLE advertising
- **STOP SPAM** - Stop BLE advertising
- **Device Select** - Choose target device
- **Interval** - Advertising interval (ms)
- **Random MAC** - Toggle MAC randomization

## Device Payloads

| ID | Device |
|----|--------|
| 0-14 | Apple (AirPods, iPhone, Watch, etc.) |
| 15-19 | Samsung (Buds, Galaxy, etc.) |
| 20-24 | Android/Windows devices |

## LED Indicator

- **Solid** - Idle
- **Blinking** - Spam active

## License

WTFPL - Do What The Fuck You Want To Public License

**For authorized security testing only.**