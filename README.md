# Smart Wearable Posture Correction System

**Student:** Gökhan Şendaş  
**Student No:** 20210808063  
**Course:** CSE 328 – Internet of Things  
**University:** Akdeniz University  
**GitHub:** https://github.com/Mythogenik/IOT-Term-Project

---

## 📌 Project Summary

This project is a wearable IoT device that monitors the user's posture in real-time. It detects slouching by comparing the angles of two MPU6050 sensors placed on the upper and lower back. When bad posture is detected for a sustained period, the system alerts the user through a vibration motor with increasing intensity. Real-time data is displayed on an OLED screen and transmitted to the Arduino Cloud dashboard, where the system can also be remotely controlled.

---

## 🔧 Hardware Components

| Component | Quantity | Description |
|---|---|---|
| ESP32 DOIT DevKit V1 | 1 | Main microcontroller with WiFi |
| MPU6050 GY-521 | 2 | Accelerometer/Gyroscope sensors |
| Arduino Vibration Motor Module | 1 | Haptic feedback actuator |
| 0.96" OLED Display (I2C, SSD1306) | 1 | Real-time status display |
| Push Button | 1 | Physical on/off toggle |
| TP4056 Charging Module | 1 | Li-ion battery charger |
| 18650 Li-ion Battery (1200mAh) | 1 | Portable power source |
| Tekli 18650 Pil Yuvası | 1 | Battery holder |
| 1N4007 Diode | 3 | Protection diodes |
| 1kΩ Resistor | 1 | Pull-down for button |
| Jumper Wires | - | Connections |
| Breadboard | 1 | Prototyping |

---

## 🔌 Wiring / Pin Connections

### MPU6050 #1 (Waist) — I2C Address: 0x68
| MPU6050 Pin | ESP32 Pin |
|---|---|
| VCC | 3.3V |
| GND | GND |
| SDA | GPIO 21 |
| SCL | GPIO 22 |
| AD0 | Not connected |

### MPU6050 #2 (Neck) — I2C Address: 0x69
| MPU6050 Pin | ESP32 Pin |
|---|---|
| VCC | 3.3V |
| GND | GND |
| SDA | GPIO 21 |
| SCL | GPIO 22 |
| AD0 | 3.3V (sets address to 0x69) |

### OLED Display (SSD1306)
| OLED Pin | ESP32 Pin |
|---|---|
| VCC | 3.3V |
| GND | GND |
| SDA | GPIO 21 |
| SCL | GPIO 22 |

### Vibration Motor Module
| Motor Pin | ESP32 Pin |
|---|---|
| VCC | 3.3V |
| GND | GND |
| IN | GPIO 5 |

### Push Button
| Button | ESP32 Pin |
|---|---|
| Leg 1 | GPIO 4 |
| Leg 2 | GND |
| 1kΩ resistor | Between GPIO 4 and 3.3V |

### Power (TP4056 + Battery)
| TP4056 Pin | Connection |
|---|---|
| BAT+ | 18650 Battery + |
| BAT- | 18650 Battery - |
| OUT+ | ESP32 VIN |
| OUT- | ESP32 GND |

---

## 📚 Software Libraries

| Library | Version | Purpose |
|---|---|---|
| MPU6050 by Electronic Cats | 1.4.4 | Sensor reading |
| Adafruit SSD1306 | 2.5.16 | OLED display |
| Adafruit GFX Library | 1.12.6 | Graphics |
| ArduinoIoTCloud | 2.9.0 | Cloud connectivity |
| Wire | Built-in | I2C communication |

---

## ⚙️ How It Works

### 1. Startup
- ESP32 boots and connects to WiFi
- Connects to Arduino IoT Cloud
- OLED shows "Smart Posture Monitor — Press button to start"

### 2. Calibration
- User presses the physical button (or toggles from dashboard)
- System reads both sensors for 2 seconds while user stands straight
- Calculates the offset between the two sensors
- OLED shows "Calibrated!" when done

### 3. Posture Detection
- Continuously reads both MPU6050 sensors (averaged over 10 readings for stability)
- Calculates: `difference = abs((neckY - waistY) - offsetY)`
- Compares difference against thresholds:

| Difference | Status | Action |
|---|---|---|
| 0 – 199 | ✅ Good Posture | No vibration |
| 200 – 499 | ⚠️ Slightly Bad | Slow pulse (1000ms off) |
| 500 – 699 | 🔴 Bad Posture | Fast pulse (300ms off) |
| 700+ | 🚨 Critical | Constant vibration |

- Bad posture must persist for **3 seconds** before vibration starts
- Good posture must persist for **1 second** before vibration stops

### 4. Motor Noise Fix
- Motor is briefly stopped before each sensor reading
- Prevents electrical noise from motor affecting MPU6050 readings
- Motor restores state after reading

### 5. OLED Display
Shows real-time:
```
Smart Posture Monitor
─────────────────────
Neck:  [value]
Waist: [value]
Diff:  [value]

GOOD / BAD! / STOP!
```

### 6. Arduino Cloud Dashboard
- **System On/Off switch** → remotely toggle system
- **Neck Value** → live neck sensor reading
- **Waist Value** → live waist sensor reading
- **Posture Difference** → calculated difference
- **Posture Gauge** → visual gauge (0–3000)
- **Bad Posture Alert LED** → lights up when bad posture detected

### 7. Physical Button
- Press and release → toggles system ON (triggers calibration)
- Press and release again → toggles system OFF
- Uses debounce (300ms) to prevent false triggers

---

## 🏗️ Physical Assembly

- Sensors are mounted on the **back of a jacket/shirt**
- MPU6050 #2 (Neck) → below the neck, upper spine area
- MPU6050 #1 (Waist) → waist level, lower spine area
- ESP32 + TP4056 + Battery → mounted at waist level
- Wires run along the spine connecting components
- System is fully wireless when running on battery

---

## ☁️ Arduino Cloud Setup

1. Create a Thing called **"Posture Monitor"**
2. Add variables:
   - `systemSwitch` → Boolean, Read/Write
   - `neckValue` → Int, Read Only
   - `waistValue` → Int, Read Only
   - `postureDiff` → Int, Read Only
   - `badPosture` → Boolean, Read Only
3. Associate ESP32 device
4. Set WiFi credentials (SSID + Password)
5. Create Dashboard with widgets linked to variables

---

## 🔋 Battery Life

| Component | Current Draw |
|---|---|
| ESP32 + WiFi | ~150mA |
| 2x MPU6050 | ~10mA |
| OLED Display | ~20mA |
| Vibration Motor | ~80mA (when active) |
| **Total (idle)** | **~180mA** |
| **Total (vibrating)** | **~260mA** |

With 1200mAh battery:
- Idle: ~6.5 hours
- Active vibration: ~4.5 hours

---

## 🐛 Known Issues & Solutions

| Issue | Cause | Solution |
|---|---|---|
| Sensor readings fluctuate | Raw accelerometer noise | Average 10 readings |
| Values drop when motor vibrates | Electrical noise from motor | Stop motor briefly before reading |
| WiFi connection fails on first boot | ESP32 boot timing | Auto-retry built into ArduinoCloud |
| Button triggers multiple times | Contact bounce | 300ms debounce delay |

---

## 📁 File Structure

```
IOT-Term-Project/
├── Posture_Monitor/
│   ├── Posture_Monitor.ino    ← Main sketch
│   └── thingProperties.h     ← Auto-generated by Arduino Cloud
└── README.md
```

---

## 🎯 Project Checklist

- ✅ ESP32 microcontroller
- ✅ 2 different sensor types (2x MPU6050)
- ✅ OLED display
- ✅ Vibration motor actuator
- ✅ Arduino Cloud connected
- ✅ Dashboard monitors both sensors
- ✅ Dashboard controls the actuator (systemSwitch)
- ✅ Data processing (thresholds, alerts, calibration)
- ✅ Real-time posture status on OLED
- ✅ Fully wireless / battery powered
