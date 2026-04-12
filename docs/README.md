# ESP32 Privacy Shield Pro — Setup Guide

## 📋 Table of Contents
1. [Prerequisites](#prerequisites)
2. [Installing Libraries](#installing-libraries)
3. [Project Setup](#project-setup)
4. [Uploading Firmware](#uploading-firmware)
5. [Uploading Web Files (LittleFS)](#uploading-web-files)
6. [First-Time WiFi Setup](#first-time-wifi-setup)
7. [Using the Dashboard](#using-the-dashboard)
8. [Troubleshooting](#troubleshooting)
9. [API Reference](#api-reference)

---

## Prerequisites

- **Hardware:** ESP32 Dev Module (4MB flash)
- **Software:** Arduino IDE 2.x (recommended) or Arduino IDE 1.8.x
- **ESP32 Board Core:** v2.0.0 or later

### Install ESP32 Board Core
1. Open Arduino IDE → **File** → **Preferences**
2. Add to "Additional Board Manager URLs":
   ```
   https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json
   ```
3. Go to **Tools** → **Board** → **Boards Manager**
4. Search `esp32` → Install **"ESP32 by Espressif Systems"** (v2.0+)
5. Select board: **Tools** → **Board** → **ESP32 Dev Module**

---

## Installing Libraries

### Method 1: Arduino Library Manager (Recommended)

Open Arduino IDE → **Sketch** → **Include Library** → **Manage Libraries...**

| Library | Author | Search Term |
|---------|--------|-------------|
| **ArduinoJson** | Benoit Blanchon | `ArduinoJson` |
| **WiFiManager** | tzapu | `WiFiManager` (select the one by "tzapu") |

### Method 2: Manual ZIP Install (for GitHub-only libraries)

These libraries are NOT in the Library Manager. Download as ZIP and install:

#### ESP32-BLE-Keyboard
1. Go to: https://github.com/T-vK/ESP32-BLE-Keyboard
2. Click **Code** → **Download ZIP**
3. Arduino IDE → **Sketch** → **Include Library** → **Add .ZIP Library...**
4. Select the downloaded ZIP file

#### ESPAsyncWebServer
1. Go to: https://github.com/me-no-dev/ESPAsyncWebServer
2. Click **Code** → **Download ZIP**
3. Arduino IDE → **Sketch** → **Include Library** → **Add .ZIP Library...**

#### AsyncTCP (dependency of ESPAsyncWebServer)
1. Go to: https://github.com/me-no-dev/AsyncTCP
2. Click **Code** → **Download ZIP**
3. Arduino IDE → **Sketch** → **Include Library** → **Add .ZIP Library...**

### Summary — All Required Libraries

| # | Library | Source | Install Method |
|---|---------|--------|---------------|
| 1 | ESP32-BLE-Keyboard | GitHub | ZIP |
| 2 | ESPAsyncWebServer | GitHub | ZIP |
| 3 | AsyncTCP | GitHub | ZIP |
| 4 | ArduinoJson | Library Manager | Search |
| 5 | WiFiManager (tzapu) | Library Manager | Search |
| 6 | LittleFS | Built-in (ESP32 core) | Pre-installed |
| 7 | Preferences | Built-in (ESP32 core) | Pre-installed |

---

## Project Setup

### Folder Structure
```
esp32-privacy-shield/
├── firmware/
│   ├── privacy_shield_pro.ino    ← Main sketch
│   └── data/                     ← Web files (uploaded to LittleFS)
│       ├── index.html
│       ├── style.css
│       └── app.js
├── docs/
│   └── README.md                 ← This file
└── README.md
```

### Arduino IDE Settings
| Setting | Value |
|---------|-------|
| Board | ESP32 Dev Module |
| Upload Speed | 921600 |
| Flash Size | 4MB (32Mb) |
| Partition Scheme | **Default 4MB with spiffs (1.2MB APP/1.5MB SPIFFS)** |
| Flash Mode | QIO |
| Core Debug Level | None |

> **Important:** The partition scheme must include a SPIFFS/LittleFS partition. The "Default 4MB with spiffs" scheme works perfectly.

---

## Uploading Firmware

1. Connect ESP32 via USB
2. Open `firmware/privacy_shield_pro.ino` in Arduino IDE
3. Select correct **Port** under **Tools** → **Port**
4. Click **Upload** (→ button)
5. Wait for "Done uploading"

---

## Uploading Web Files

The `data/` folder contains the web dashboard files. These must be uploaded separately to the ESP32's LittleFS filesystem.

### Install LittleFS Upload Plugin

#### For Arduino IDE 2.x:
1. Download the plugin from: https://github.com/earlephilhower/arduino-littlefs-upload
2. Follow the installation instructions in the repository README
3. After installing, restart Arduino IDE
4. Use **Ctrl+Shift+P** → type "Upload LittleFS" → select the command

#### For Arduino IDE 1.8.x:
1. Download "ESP32 LittleFS Data Upload" plugin
2. Create a `tools/` folder in your Arduino sketchbook directory
3. Extract the plugin into the `tools/` folder
4. Restart Arduino IDE
5. Go to **Tools** → **ESP32 Sketch Data Upload**

### Upload Steps
1. Make sure the `data/` folder is inside the sketch folder (`firmware/data/`)
2. Close Serial Monitor (it blocks the upload port)
3. Run the LittleFS upload tool
4. Wait for "LittleFS Image Uploaded"

---

## First-Time WiFi Setup

On first boot (or if saved WiFi is unavailable), the ESP32 creates a WiFi Access Point:

1. **Look for WiFi network:** `PrivacyShield-Setup`
2. **Password:** `shield123`
3. **Connect** from your phone or laptop
4. A **captive portal** will automatically open (or navigate to `192.168.4.1`)
5. Click **"Configure WiFi"**
6. Select your home WiFi network
7. Enter your WiFi password
8. Click **Save**
9. ESP32 will reboot and connect to your WiFi

### Finding the ESP32's IP Address
After WiFi connection, the IP is printed to Serial Monitor:
```
[WiFi] Connected!
[WiFi] IP Address: 192.168.1.XXX
```

Open this IP in any browser to access the dashboard!

---

## Using the Dashboard

### Opening the Dashboard
1. Ensure your phone/laptop is on the **same WiFi** as the ESP32
2. Open a browser and navigate to `http://<ESP32_IP>`
3. The dashboard loads with current settings

### Configuring Settings
- **Sliders:** Drag to adjust proximity thresholds and lock delay
- **Stepper:** Use +/- buttons to set waves required
- **Save to Device:** Sends settings to ESP32 and saves to non-volatile storage
- **Reset Defaults:** Restores all values to factory defaults
- **Export Config:** Downloads settings as a JSON file

### Live Status
The bottom status card shows real-time data:
- **Current Distance:** Live ultrasonic sensor reading
- **System State:** Present / Away / Locking
- **BLE Status:** Whether a computer is paired via Bluetooth

### Keyboard Shortcut
- **Ctrl+S** — Quick save settings

---

## Troubleshooting

### "LittleFS mount failed"
- Make sure you uploaded the `data/` folder files using the LittleFS upload tool
- Check that `Partition Scheme` is set to "Default 4MB with spiffs"

### Dashboard not loading
- Verify ESP32 is connected to WiFi (check Serial Monitor for IP)
- Ensure your device is on the same network
- Try accessing `http://<IP>/api/settings` directly to test the API

### WiFiManager portal not appearing
- Hold the ESP32's RESET button for 5 seconds
- Or upload a sketch that calls `wm.resetSettings()` to clear saved WiFi

### BLE Keyboard not working
- Only one device can connect via BLE at a time
- Go to your PC's Bluetooth settings → Remove "Privacy Shield Pro" → Re-pair

### Settings not persisting after reboot
- Verify the `saveSettings()` function is being called (check Serial Monitor)
- Try manually calling `POST /api/reset` to reinitialize NVS

---

## API Reference

All endpoints return JSON. Base URL: `http://<ESP32_IP>`

### `GET /api/settings`
Returns current configuration.

**Response:**
```json
{
  "distThresholdLock": 50,
  "distThresholdAwake": 30,
  "lockDelay": 3000,
  "gestureNear": 5,
  "gestureFar": 10,
  "wavesRequired": 2
}
```

### `POST /api/settings`
Update configuration. All fields are optional.

**Request Body:**
```json
{
  "distThresholdLock": 60,
  "lockDelay": 5000
}
```

**Response:**
```json
{ "status": "ok" }
```

### `GET /api/status`
Returns live sensor data.

**Response:**
```json
{
  "distance": 42.5,
  "state": 0,
  "bleConnected": true
}
```
State values: `0` = Present, `1` = Away, `2` = Locking

### `POST /api/reset`
Reset all settings to factory defaults.

**Response:**
```json
{ "status": "reset" }
```

---

## Hardware Connections

| Component | ESP32 Pin |
|-----------|-----------|
| Ultrasonic TRIG | GPIO 5 |
| Ultrasonic ECHO | GPIO 18 |
| Red LED | GPIO 19 |
| Green LED | GPIO 21 |
| Buzzer | GPIO 22 |
