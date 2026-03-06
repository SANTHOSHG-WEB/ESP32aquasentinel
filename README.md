# 💧 Smart Water Management System - ESP32 Code

![Version](https://img.shields.io/badge/version-1.0.0-blue)
![ESP32](https://img.shields.io/badge/platform-ESP32-green)
![License](https://img.shields.io/badge/license-MIT-orange)
![Hackathon](https://img.shields.io/badge/hackathon-ready-purple)

<div align="center">
  <img src="https://img.shields.io/badge/IoT-Smart%20Water-blueviolet">
  <img src="https://img.shields.io/badge/Firebase-Realtime%20DB-yellow">
  <img src="https://img.shields.io/badge/IR%20Remote-Control-success">
  <br>
  <h3>🌊 Every Drop Counts - Smart Water Management for a Sustainable Future</h3>
</div>

---

## 📋 Table of Contents
- [Overview](#-overview)
- [Features](#-features)
- [Hardware Requirements](#-hardware-requirements)
- [Pin Connections](#-pin-connections)
- [Software Requirements](#-software-requirements)
- [Installation](#-installation)
- [Firebase Setup](#-firebase-setup)
- [Configuration](#-configuration)
- [Code Structure](#-code-structure)
- [Usage](#-usage)
- [IR Remote Control](#-ir-remote-control)
- [Automation Logic](#-automation-logic)
- [Troubleshooting](#-troubleshooting)
- [Contributing](#-contributing)
- [License](#-license)
- [Acknowledgments](#-acknowledgments)

---

## 🌟 Overview

This ESP32-based Smart Water Management System is a complete IoT solution for monitoring and controlling water usage in residential, agricultural, or industrial settings. The system collects data from multiple sensors, controls water pumps and valves automatically, and sends real-time data to Firebase for remote monitoring and control via a web dashboard.

### 🎯 Key Capabilities
- **Real-time water level monitoring** using ultrasonic sensor
- **Soil moisture-based automatic irrigation**
- **Rain detection** to prevent over-watering
- **Leak detection** using flow pattern analysis
- **Remote control** via IR remote and Firebase
- **Dual motor control** (tank filling + garden watering)
- **Gamification-ready** with points and achievements

---

## ✨ Features

### 📊 Sensor Monitoring
- ✅ **Water Tank Level** - Ultrasonic HC-SR04 (0-100%)
- ✅ **Soil Moisture** - Capacitive sensor (0-100%)
- ✅ **Rain Detection** - FC-37 rain sensor
- ✅ **Temperature & Humidity** - DHT11/DHT22
- ✅ **Flow Rate** - YF-S201 flow sensor (optional)

### 🎮 Control Systems
- ✅ **Automatic Tank Filling** - ON when <20%, OFF when >90%
- ✅ **Smart Irrigation** - Waters only when soil dry & not raining
- ✅ **Manual Override** - Via IR remote or Firebase
- ✅ **Emergency Stop** - Long press button or IR command

### 📱 Connectivity
- ✅ **WiFi Connection** - Connects to local network
- ✅ **Firebase Integration** - Real-time database sync
- ✅ **IR Remote Control** - Full control via any IR remote
- ✅ **Manual Button** - Physical control with emergency stop

### 🚨 Safety Features
- ✅ **Leak Detection** - Alerts on abnormal flow
- ✅ **Tank Low Alert** - Critical level notification
- ✅ **Rain Protection** - Prevents watering during rain
- ✅ **Auto Shutoff** - Garden motor max 30 minutes
- ✅ **Emergency Stop** - Kills all motors instantly

---

## 🔧 Hardware Requirements

### Essential Components
| Component | Quantity | Purpose |
|-----------|----------|---------|
| ESP32 Development Board | 1 | Main controller |
| HC-SR04 Ultrasonic Sensor | 1 | Water tank level |
| Soil Moisture Sensor | 1 | Garden soil monitoring |
| FC-37 Rain Sensor | 1 | Rain detection |
| DHT11/DHT22 | 1 | Temperature & humidity |
| 2-Channel Relay Module | 1 | Motor control (5V) |
| 12V DC Water Pump | 1 | Tank filling |
| 12V Solenoid Valve | 1 | Garden irrigation |
| Passive Buzzer | 1 | Alerts |
| IR Receiver (TSOP38238) | 1 | Remote control |
| Push Button | 1 | Manual control |
| LEDs (3 colors) | 3 | Status indicators |
| 220Ω Resistors | 3 | LED current limiting |
| 10kΩ Resistor | 1 | DHT11 pull-up |
| Breadboard & Jumper Wires | - | Connections |

### Optional Components
| Component | Purpose |
|-----------|---------|
| YF-S201 Flow Sensor | Water flow measurement |
| 16x2 I2C LCD | Local display |
| Power Supply (5V/12V) | System power |
| Waterproof Enclosure | Outdoor protection |

---

## 🔌 Pin Connections

### ESP32 Pin Mapping

| Component | ESP32 Pin | Connection |
|-----------|-----------|------------|
| **Ultrasonic TRIG** | GPIO 5 | Signal |
| **Ultrasonic ECHO** | GPIO 18 | Signal |
| **Soil Moisture** | GPIO 35 | Analog Input |
| **Rain Sensor** | GPIO 34 | Analog Input |
| **DHT11 Data** | GPIO 17 | Digital with 10k pull-up |
| **Flow Sensor** | GPIO 4 | Interrupt Pin |
| **Relay 1 (Tank)** | GPIO 25 | Control (LOW=ON) |
| **Relay 2 (Garden)** | GPIO 26 | Control (LOW=ON) |
| **Buzzer** | GPIO 27 | PWM capable |
| **IR Receiver** | GPIO 33 | Signal |
| **Push Button** | GPIO 32 | Input (pull-up) |
| **Green LED** | GPIO 13 | Status |
| **Yellow LED** | GPIO 14 | Status |
| **Red LED** | GPIO 15 | Status |

### Power Connections
| Component | Power | Ground |
|-----------|-------|--------|
| ESP32 | USB/5V | GND |
| Sensors | 3.3V/5V | GND |
| Relay Module | 5V | GND |
| Pumps/Valves | 12V | GND (separate) |

### 📝 Important Notes
- **Relay Logic**: LOW signal = Relay ON (motor runs)
- **Use common GND** for all components
- **Separate 12V supply** for pumps (do not use ESP32 power)
- **Add flyback diodes** across relay coils
- **Use pull-up resistors** for open-collector outputs

---

## 💻 Software Requirements

### Arduino IDE Setup
1. **Install Arduino IDE** (latest version)
2. **Add ESP32 Board Support:**
   - File → Preferences → Additional Board Manager URLs:
