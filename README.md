# 🏠 ESP32 Voice & Gesture Controlled Home Automation

> A dual-mode smart home automation system using **ESP32**, **MPU6050**, **Bluetooth**, **FreeRTOS**, **OLED Display**, and a **4-channel relay module**. Control appliances using voice commands or hand-tilt gestures without internet dependency.

<p align="center">
  <img src="https://github.com/user-attachments/assets/9a41fd10-cecf-4eb7-81df-b55f02f2c08a" alt="ESP32 Voice and Gesture Home Automation Prototype" width="850">
</p>

---

## 📌 Table of Contents

* [Overview](#-overview)
* [Features](#-features)
* [System Architecture](#-system-architecture)
* [Hardware Requirements](#-hardware-requirements)
* [Pin Configuration](#-pin-configuration)
* [Software and Libraries](#-software-and-libraries)
* [How It Works](#-how-it-works)

  * [Voice Control](#voice-control)
  * [Gesture Control](#gesture-control)
  * [Timer Functionality](#timer-functionality)
  * [OLED Status Display](#oled-status-display)
* [Bluetooth Command Reference](#-bluetooth-command-reference)
* [Circuit Diagram](#-circuit-diagram)
* [Getting Started](#-getting-started)
* [Results](#-results)
* [Future Scope](#-future-scope)
* [Author](#-author)
* [License](#-license)

---

## 🔍 Overview

Traditional home automation mainly depends on physical switches, remote controls, or internet-based voice assistants. This project provides a simple and low-cost alternative by combining **Bluetooth voice control** and **MPU6050-based gesture control**.

The user can control four appliances:

| Appliance                | Relay Channel | Example Command |
| ------------------------ | ------------: | --------------- |
| Light                    |       Relay 1 | `light on`      |
| Fan                      |       Relay 2 | `fan on`        |
| TV                       |       Relay 3 | `tv on`         |
| Switch / Additional Load |       Relay 4 | `switch on`     |

The ESP32 receives voice commands from a Bluetooth mobile application and gesture inputs from the MPU6050 sensor. It processes the input, controls the relay module, and updates the OLED display with the current appliance status.

---

## ✨ Features

* 🎤 Bluetooth voice control without internet
* 🤚 Hand-tilt gesture control using MPU6050
* 💡 Controls four appliances independently
* ⏱️ Timer-based appliance control
* 📺 OLED display for real-time appliance status
* 🔁 FreeRTOS-based multitasking
* 🔌 4-channel relay-based appliance switching
* 📱 Supports text commands and Bluetooth app buttons
* ♿ Useful for elderly users and differently-abled users
* 🌐 Future-ready design for Wi-Fi and IoT integration

---

## 🏗️ System Architecture

<p align="center">
  <img src="https://github.com/user-attachments/assets/32de0e30-69b8-44d6-8e9b-1d8481e50dc7" alt="Block Diagram" width="850">
</p>

The ESP32 acts as the main controller. It receives voice commands through Bluetooth and gesture data from the MPU6050 sensor. FreeRTOS helps the ESP32 handle voice input, gesture reading, timer checking, relay control, and OLED updating smoothly.

The relay module controls the connected appliances, while the OLED display shows appliance selection and ON or OFF status.

---

## 🔧 Hardware Requirements

| Component                           |    Quantity | Purpose                              |
| ----------------------------------- | ----------: | ------------------------------------ |
| ESP32 Development Board             |           1 | Main controller                      |
| MPU6050 Accelerometer and Gyroscope |           1 | Hand-tilt gesture detection          |
| OLED Display 0.96 inch              |           1 | Appliance status display             |
| 4-Channel Relay Module              |           1 | Controls four appliances             |
| Bulb and Holder                     |           1 | Light load demonstration             |
| DC Fan                              |           1 | Fan load demonstration               |
| Switch Sockets                      |           2 | TV and additional load demonstration |
| 5 V Power Supply                    |           1 | Power supply                         |
| Jumper Wires                        | As required | Component connections                |
| Android Phone                       |           1 | Bluetooth voice and button control   |

---

## 🔌 Pin Configuration

| ESP32 Pin | Connected To             | Function       |
| --------: | ------------------------ | -------------- |
|   GPIO 21 | MPU6050 SDA and OLED SDA | I2C data line  |
|   GPIO 22 | MPU6050 SCL and OLED SCL | I2C clock line |
|   GPIO 25 | Relay IN1                | Light control  |
|   GPIO 26 | Relay IN2                | Fan control    |
|   GPIO 27 | Relay IN3                | TV control     |
|   GPIO 14 | Relay IN4                | Switch control |
|     3.3 V | MPU6050 VCC              | Sensor power   |
|       GND | All modules              | Common ground  |

> ⚠️ Most 4-channel relay modules are active-LOW. In that case, `LOW` turns the relay ON and `HIGH` turns the relay OFF.

---

## 📦 Software and Libraries

The firmware is developed using **Arduino IDE**.

| Software / Library   | Purpose                      |
| -------------------- | ---------------------------- |
| Arduino IDE          | Code development and upload  |
| ESP32 Board Package  | ESP32 board support          |
| BluetoothSerial      | Bluetooth communication      |
| Wire                 | I2C communication            |
| Adafruit GFX Library | OLED graphics support        |
| Adafruit SSD1306     | OLED display driver          |
| MPU6050 Library      | MPU6050 sensor communication |
| FreeRTOS             | Parallel task management     |

### Arduino IDE Board Setup

1. Install Arduino IDE.
2. Open **File → Preferences**.
3. Add the ESP32 board URL:

```text
https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json
```

4. Open **Tools → Board → Boards Manager**.
5. Search for `ESP32` and install the package by Espressif Systems.
6. Select **ESP32 Dev Module**.
7. Select the correct COM port.
8. Upload the code.

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
<p align="center">
  <img src="https://github.com/user-attachments/assets/b4ad039b-78d6-4044-a435-26afa7d58a68" alt="OLED Appliance Status Display" width="400">
</p>

The 128×64 OLED shows 4 rows — one per appliance — with:
- An **icon** `[L]`, `[F]`, `[T]`, `[S]`
- Device **name** and **ON/OFF** status
- **Highlighted row** (inverted colors) = currently selected appliance for gesture control

---


## 📋 Bluetooth Command Reference

### Text Commands

| Command       | Action                               |
| ------------- | ------------------------------------ |
| `light on`    | Turn light ON                        |
| `light off`   | Turn light OFF                       |
| `light on 5`  | Turn light ON for 5 minutes          |
| `fan on`      | Turn fan ON                          |
| `fan off`     | Turn fan OFF                         |
| `fan on 5`    | Turn fan ON for 5 minutes            |
| `tv on`       | Turn TV ON                           |
| `tv off`      | Turn TV OFF                          |
| `tv on 5`     | Turn TV ON for 5 minutes             |
| `switch on`   | Turn switch ON                       |
| `switch off`  | Turn switch OFF                      |
| `switch on 5` | Turn switch ON for 5 minutes         |
| `all on`      | Turn all appliances ON               |
| `all off`     | Turn all appliances OFF              |
| `all on 5`    | Turn all appliances ON for 5 minutes |
| `status`      | Show current appliance status        |

### Single-Character Commands

| Command    | Action             |
| ---------- | ------------------ |
| `1`        | Light ON           |
| `A` or `a` | Light OFF          |
| `2`        | Fan ON             |
| `B` or `b` | Fan OFF            |
| `3`        | TV ON              |
| `C` or `c` | TV OFF             |
| `4`        | Switch ON          |
| `D` or `d` | Switch OFF         |
| `5`        | All appliances ON  |
| `0`        | All appliances OFF |

---

## 📐 Circuit Diagram

<p align="center">
  <img src="https://github.com/user-attachments/assets/68666590-73b3-4cdd-80b3-9323c9912ad7" alt="Circuit Diagram" width="900">
</p>

The MPU6050 sensor and OLED display share the I2C communication lines. The ESP32 controls the four relay inputs through GPIO pins. The relay module acts as an electrical isolation interface between the ESP32 control circuit and the connected appliance loads.

---

## 🔄 System Flowchart

<p align="center">
  <img src="https://github.com/user-attachments/assets/dbba3646-a2cb-4b0a-8fcd-4ac5d97d649d" alt="System Flowchart" width="300">
</p>

The system initializes the ESP32, Bluetooth, MPU6050 sensor, OLED display, relay module, and FreeRTOS tasks. It then continuously reads voice and gesture inputs, processes the command, updates the relay output, and displays the latest appliance status on the OLED.

---

## 🚀 Getting Started

### 1. Clone the Repository

```bash
git clone https://github.com/afnanjhadwale/ESP32-Voice-Gesture-Home-Automation.git
cd ESP32-Voice-Gesture-Home-Automation
```

### 2. Open the Code

Open this file in Arduino IDE:

```text
src/ESP32_Voice_Gesture_Home_Automation.ino
```

### 3. Install Required Libraries

Install the following libraries from Arduino IDE Library Manager:

* Adafruit GFX Library
* Adafruit SSD1306
* MPU6050
* ESP32 Board Package

### 4. Upload the Code

1. Connect ESP32 using USB cable.
2. Select **ESP32 Dev Module**.
3. Select the correct COM port.
4. Click Upload.

### 5. Connect Through Bluetooth

1. Turn ON Bluetooth on your Android phone.
2. Pair with `ESP32_Home`.
3. Open a Bluetooth serial control application.
4. Send commands such as `light on` or `fan on 5`.

---

## 🧪 Results

The prototype successfully controls the bulb, fan, TV load, and additional switch load using Bluetooth voice commands and MPU6050 gesture inputs. The OLED display updates appliance status in real time. Timer commands automatically turn OFF appliances after the selected duration.

---

## 👤 Author

**Afnan Jhadwale**

Electronics and Communication Engineering

Sir M. Visvesvaraya Institute of Technology, Bengaluru

---

## 📄 License

This project is licensed under the [MIT License](LICENSE).


<p align="center">
  Built using ESP32, MPU6050, FreeRTOS, Bluetooth, OLED, and Relay Control
</p>
