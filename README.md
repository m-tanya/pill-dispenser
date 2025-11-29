
# **Automatic Pill Dispenser – IoT Project**

This project implements an **automatic pill dispenser** using the Heltec ESP32 V3, a servo-based dispensing mechanism, humidity monitoring, and buzzer/LED alerts. The system dispenses pills on a fixed schedule and provides real-time environmental safety warnings.

---

## ** Features**

### **1. Automatic Pill Dispensing**

* Pills are dispensed every 5 seconds (demo interval).
* Servo rotates in predefined angle segments: 0->60->120->180.
* Each dispense event activates:

  * **LED flash**
  * **Short buzzer beep**

### **2. Humidity & Temperature Monitoring (DHT11)**

* Constantly monitors environmental humidity.
* If **humidity > 70%**, the buzzer:

  * **Beeps three times** to warn that pills may degrade.

### **3. Real-Time Serial Output**

The ESP32 prints:

* Current segment + angle
* Temperature (°C)
* Humidity (%)
* Humidity alerts

Useful for debugging and live monitoring through **PlatformIO device monitor**.

### **4. Expandable Architecture**

Future additions:

* PIN-based activation using a capacitive touch sensor
* Azure IoT Hub integration for cloud storage and telemetry
* Enclosure for pill container

---

## **Hardware Used**

* Heltec WiFi LoRa 32 V3 (ESP32-S3)
* SG90 Micro-Servo Motor
* DHT11 Humidity/Temperature Sensor (4-pin)
* LED
* Buzzer (active)
* Resistors (10kΩ + 220Ω)
* Breadboard + jumper wires
* Capacitive Touch Sensor (Adafruit 1602 / TTP223) - Future Scope
---

## ** PlatformIO Commands**

**Upload firmware**

```
pio run --target upload
```

**Open serial monitor**

```
pio device monitor -b 9600
```

---

## ** Project Structure**

```
/src
  └── main.cpp        # Pill dispenser logic + DHT11 monitoring
/platformio.ini       # Board + library configuration
/README.md
```

---

## ** Status**

* Servo dispensing: Done
* LED + buzzer feedback: Done
* Humidity safety alert: Done
* PIN unlock: In Progress
* Azure IoT logging: In Progress
* Final enclosure: In Progress

---

## ** Author**

Munikoti Tanya
Ketan Kishor Sarda
---
.
