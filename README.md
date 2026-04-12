# 🛡️ ESP32 Privacy Shield Pro

**Auto-lock your PC when you walk away. Configure everything from a beautiful web dashboard.**

A smart ultrasonic-based privacy device that automatically locks your computer (Win+L) via BLE when you leave your desk, and supports hand-wave gestures for quick manual locking — all configurable through a modern web interface.

![Platform](https://img.shields.io/badge/Platform-ESP32-blue)
![License](https://img.shields.io/badge/License-MIT-green)
![Version](https://img.shields.io/badge/Version-2.0.0-cyan)

---

## ✨ Features

- **Auto-Lock Detection** — Ultrasonic sensor detects when you leave your desk and locks your PC
- **Hand-Wave Gesture** — Quick-lock by waving your hand in front of the sensor
- **BLE Keyboard** — Sends Win+L keystroke to lock Windows/macOS/Linux
- **Web Dashboard** — Beautiful dark-themed configuration UI, accessible from any browser
- **WiFi Setup** — Captive portal for easy first-time WiFi configuration
- **Persistent Settings** — All configuration survives reboots (stored in NVS)
- **Real-Time Monitoring** — Live distance readings and system state in the dashboard

## 🔧 Configurable Parameters

| Setting | Default | Range | Description |
|---------|---------|-------|-------------|
| Lock Distance | 50 cm | 10–200 cm | Triggers auto-lock when exceeded |
| Wake Distance | 30 cm | 5–100 cm | Detects user return |
| Lock Delay | 3000 ms | 1–10 sec | Grace period before locking |
| Hand Near | 5 cm | 1–20 cm | Hand-in gesture threshold |
| Hand Far | 10 cm | 5–30 cm | Hand-out gesture threshold |
| Waves Required | 2 | 1–5 | Hand waves for quick-lock |

## 🚀 Quick Start

1. **Install libraries** — See [Setup Guide](docs/README.md#installing-libraries)
2. **Upload firmware** — Flash `firmware/privacy_shield_pro.ino` to ESP32
3. **Upload web files** — Use LittleFS upload for the `firmware/data/` folder
4. **Connect WiFi** — Join the `PrivacyShield-Setup` AP and configure your WiFi
5. **Open dashboard** — Navigate to the ESP32's IP address in any browser

📖 **Full instructions:** [docs/README.md](docs/README.md)

## 📁 Project Structure

```
├── firmware/
│   ├── privacy_shield_pro.ino   # ESP32 firmware
│   └── data/                    # Web dashboard (LittleFS)
│       ├── index.html
│       ├── style.css
│       └── app.js
├── docs/
│   └── README.md                # Detailed setup guide
└── README.md                    # This file
```

## 🔌 Hardware

| Component | Pin |
|-----------|-----|
| Ultrasonic Trigger | GPIO 5 |
| Ultrasonic Echo | GPIO 18 |
| Red LED | GPIO 19 |
| Green LED | GPIO 21 |
| Buzzer | GPIO 22 |

## 📡 API Endpoints

| Endpoint | Method | Description |
|----------|--------|-------------|
| `/api/settings` | GET | Get current settings |
| `/api/settings` | POST | Update settings |
| `/api/status` | GET | Live sensor data |
| `/api/reset` | POST | Reset to defaults |

---

**Built with ❤️ using ESP32, BLE, and modern web technologies.**
