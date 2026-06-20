# 🏠 ESP32 Voice & Gesture Controlled Home Automation

> A dual-mode smart home automation system using **ESP32**, **MPU-6050**, **Bluetooth**, and a **4-channel relay module** — controllable by hand gestures and voice commands without any internet dependency.

---

## 📌 Table of Contents

- [Overview](#-overview)
- [Demo](#-demo)
- [Features](#-features)
- [System Architecture](#-system-architecture)
- [Hardware Requirements](#-hardware-requirements)
- [Pin Configuration](#-pin-configuration)
- [Software & Libraries](#-software--libraries)
- [How It Works](#-how-it-works)
  - [Gesture Control](#gesture-control)
  - [Voice / Bluetooth Control](#voice--bluetooth-control)
  - [Timer Functionality](#timer-functionality)
  - [OLED Display](#oled-display)
- [Bluetooth Command Reference](#-bluetooth-command-reference)
- [Circuit Diagram](#-circuit-diagram)
- [Getting Started](#-getting-started)
- [Folder Structure](#-folder-structure)
- [Team](#-team)
- [License](#-license)

---

## 🔍 Overview

Traditional home automation relies on physical switches or cloud-dependent voice assistants. This project provides a **fully offline, dual-mode** solution:

| Mode | Technology | Input |
|------|-----------|-------|
| **Gesture** | MPU-6050 (Accelerometer + Gyroscope) | Tilt hand Left/Right to select, Tilt Up/Down to control |
| **Voice** | ESP32 Bluetooth + Android App | Text commands like `"light on"`, `"fan off 10"` |

Designed for **elderly users, differently-abled individuals**, and anyone who wants a hands-free, internet-free smart home experience.

---

## 🎥 Demo

**Hardware Setup**
>  <img width="1448" height="1086" alt="image" src="https://github.com/user-attachments/assets/27c0fc7a-5233-4092-af89-d5693fa4ab25" />

**Arduino Bluetooth Controller App**
>  <img width="400" height="400" alt="image" src="https://github.com/user-attachments/assets/5dc2ee83-d859-4408-bac8-bb74486ee948" />
https://play.google.com/store/apps/details?id=com.giristudio.hc05.bluetooth.arduino.control

---

## ✨ Features

- 🎤 **Voice Control** via Bluetooth (no internet needed)
- 🤚 **Gesture Control** using MPU-6050 tilt detection
- 💡 **Controls 4 Appliances**: Light, Fan, TV, Switch
- ⏱️ **Timer Support** — auto turn-off after N minutes
- 📺 **OLED Status Display** — real-time appliance status with highlight on selected device
- 🔋 **Active-Low Relay Support** — works with standard relay modules
- 🔁 **Debounced Gesture Detection** — prevents false triggers
- 💬 **Dual Command Mode** — single-character (`1`, `A`) and full text (`light on 5`)

---

## 🏗️ System Architecture

```
┌─────────────────────────────────────────────────────┐
│                    ESP32 Core                       │
│                                                     │
│   ┌──────────────┐      ┌───────────────────────┐   │
│   │ Bluetooth RX │      │ MPU-6050 Gesture Read │   │
│   │ (Voice Cmds) │      │ (Accel ax, ay values) │   │
│   └──────┬───────┘      └──────────┬────────────┘   │
│          │                         │                │
│   ┌──────▼─────────────────────────▼───────────┐    │
│   │          Command Processor / State Machine │    │
│   └──────────────────────┬─────────────────────┘    │
│                          │                          │
│   ┌──────────────────────▼──────────────────────┐   │
│   │     Relay Control (4-Channel, Active-Low)   │   │
│   │   PIN 25 - Light  |  PIN 26 - Fan           │   │
│   │   PIN 27 - TV     |  PIN 14 - Switch        │   │
│   └─────────────────────────────────────────────┘   │
│                                                     │
│   ┌─────────────────────────────────────────────┐   │
│   │       OLED SSD1306 (I2C @ 0x3C)             │   │
│   │    Real-time status with selection cursor   │   │
│   └─────────────────────────────────────────────┘   │
└─────────────────────────────────────────────────────┘
```

---

## 🔧 Hardware Requirements

| Component | Quantity | Purpose |
|-----------|----------|---------|
| **ESP32 Dev Board** | 1 | Main microcontroller |
| **MPU-6050** | 1 | Gesture detection (accel + gyro) |
| **SSD1306 OLED (128×64)** | 1 | Status display |
| **4-Channel Relay Module (Active-Low)** | 1 | Appliance switching |
| **Jumper Wires** | As needed | Connections |
| **Power Supply (5V/3.3V)** | 1 | Power for ESP32 & sensors |
| **Android Phone** | 1 | Bluetooth terminal app |

---

## 🔌 Pin Configuration

| ESP32 Pin | Connected To | Function |
|-----------|-------------|----------|
| GPIO 21 | MPU-6050 SDA / OLED SDA | I2C Data |
| GPIO 22 | MPU-6050 SCL / OLED SCL | I2C Clock |
| GPIO 25 | Relay 1 IN | Light control |
| GPIO 26 | Relay 2 IN | Fan control |
| GPIO 27 | Relay 3 IN | TV control |
| GPIO 14 | Relay 4 IN | Switch control |

> ⚠️ All relay pins are **ACTIVE-LOW** (LOW = ON, HIGH = OFF). Set `RELAY_ACTIVE_LOW true` in code.

---

## 📦 Software & Libraries

Install the following in **Arduino IDE** (via Library Manager or Board Manager):

| Library | Install From | Purpose |
|---------|-------------|---------|
| `BluetoothSerial` | ESP32 Arduino Core | Bluetooth communication |
| `Wire` | Built-in | I2C protocol |
| `MPU6050` | Library Manager (Electronic Cats / jrowberg) | IMU sensor |
| `Adafruit GFX` | Library Manager | Graphics base |
| `Adafruit SSD1306` | Library Manager | OLED display driver |

**Board Setup in Arduino IDE:**
1. Add ESP32 board URL: `https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json`
2. Install **esp32** board package by Espressif Systems
3. Select board: **ESP32 Dev Module**
4. Upload speed: **115200**

---

## ⚙️ How It Works

### Gesture Control

The MPU-6050 reads raw accelerometer values every loop cycle. Normalized `ax` and `ay` values determine gesture:

| Tilt Direction | Axis | Threshold | Action |
|---------------|------|-----------|--------|
| **Right** | ax > +1.2 | Positive X | Select next appliance |
| **Left** | ax < -1.2 | Negative X | Select previous appliance |
| **Up** | ay > +1.2 | Positive Y | Turn ON selected appliance |
| **Down** | ay < -1.2 | Negative Y | Turn OFF selected appliance |

A **500ms debounce** prevents accidental repeated triggers. The threshold (`1.2g`) is adjustable in code.

---

### Voice / Bluetooth Control

Connect to Bluetooth device named **`ESP32_Home`** using any serial Bluetooth terminal (e.g. *Serial Bluetooth Terminal* on Android).

Send text commands (followed by newline `\n`) or single characters.

---

### Timer Functionality

Append minutes to any ON command to auto-shutoff:

```
fan on 30      → Fan turns ON, auto OFF after 30 minutes
light on 10    → Light turns ON, auto OFF after 10 minutes
all on 60      → All devices ON, auto OFF after 60 minutes
```

---

### OLED Display
>  <img width="400" height="400" alt="image" src="https://github.com/user-attachments/assets/b4ad039b-78d6-4044-a435-26afa7d58a68" />
The 128×64 OLED shows 4 rows — one per appliance — with:
- An **icon** `[L]`, `[F]`, `[T]`, `[S]`
- Device **name** and **ON/OFF** status
- **Highlighted row** (inverted colors) = currently selected appliance for gesture control

---

## 📋 Bluetooth Command Reference

### Text Commands

| Command | Action |
|---------|--------|
| `light on` | Turn light ON |
| `light off` | Turn light OFF |
| `fan on` | Turn fan ON |
| `fan off` | Turn fan OFF |
| `tv on` | Turn TV ON |
| `tv off` | Turn TV OFF |
| `switch on` | Turn switch ON |
| `switch off` | Turn switch OFF |
| `all on` | Turn ALL devices ON |
| `all off` | Turn ALL devices OFF |
| `light on 15` | Light ON with 15-minute timer |
| `fan on 30` | Fan ON with 30-minute timer |
| `status` | Print all device states to serial |

### Single-Character Commands

| Char | Action |
|------|--------|
| `1` | Light ON |
| `A` / `a` | Light OFF |
| `2` | Fan ON |
| `B` / `b` | Fan OFF |
| `3` | TV ON |
| `C` / `c` | TV OFF |
| `4` | Switch ON |
| `D` / `d` | Switch OFF |
| `5` | ALL ON |
| `0` | ALL OFF |

---

## 📐 Circuit Diagram

> <img width="921" height="601" alt="image" src="https://github.com/user-attachments/assets/dd91b8d8-5c63-40c2-94a3-a467db88a7bc" />


Key connections:
- MPU-6050 and SSD1306 share I2C bus (SDA=21, SCL=22)
- Relay module powered by **5V**, signal pins from ESP32 GPIO
- Relay **COM** terminals connect to appliance live wire
- Relay **NO** (Normally Open) for default-off devices

---

## 🚀 Getting Started

### 1. Clone the Repository

```bash
git clone https://github.com/afnanjhadwale/ESP32-VoiceGesture-HomeAutomation.git
cd ESP32-VoiceGesture-HomeAutomation
```

### 2. Open in Arduino IDE

Open `src/main.ino` in Arduino IDE.

### 3. Install Libraries

Use **Sketch → Include Library → Manage Libraries** to install:
- `Adafruit SSD1306`
- `Adafruit GFX Library`
- `MPU6050` (by Electronic Cats)

### 4. Configure & Upload

- Select **ESP32 Dev Module** as board
- Select correct COM port
- Click **Upload**

### 5. Connect via Bluetooth

- Pair phone with `ESP32_Home`
- Open *Serial Bluetooth Terminal* app
- Type commands like `light on` and press send


---

## 👥 Team

| Name | USN |
|------|-----|
| **Afnan Jhadwale** | 1MV23EC013 |
| Abhishek Shivanand Rajapure | 1MV23EC008 |
| Sangamesh Somaling Nugganatti | 1MV23EC093 |
| S V Vikas | 1MV23EC089 |

**Guide:** Dr. Sheetal Belaldavar, Associate Professor, Dept. of ECE  
**Institution:** Sir M. Visvesvaraya Institute of Technology, Bengaluru  
**Affiliated to:** Visvesvaraya Technological University (VTU), Belagavi  
**Academic Year:** 2025–26

---

## 📄 License

This project is licensed under the [MIT License](LICENSE).  
Feel free to use, fork, and improve — attribution appreciated!

---

<p align="center">
  Made with ❤️ at Sir MVIT, Bengaluru &nbsp;|&nbsp; ECE Dept &nbsp;|&nbsp; Mini Project BEC586
</p>
