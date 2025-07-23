# 🔥 Reaction Vessel Temperature Controller for Monkeypox Diagnostics

This repository contains the **hardware and software implementation** of a low-cost, portable LAMP-based diagnostic device for Monkeypox (Mpox) virus detection. The system is designed for rapid and reliable temperature control and real-time user interaction.

---

## 📁 Repository Structure

### `main.py`
- **Purpose:** Main controller for system logic.
- **Functions:**
  - Reads target temperature input.
  - Sends serial commands to the Arduino microcontroller.
  - Logs and visualizes temperature feedback.
- **Dependencies:** Python 3, `pyserial`, `matplotlib`, `tkinter`.

---

### `gui.py`
- **Purpose:** Graphical User Interface for starting tests and monitoring real-time temperature.
- **Features:**
  - Start/stop buttons.
  - Real-time temperature plotting.
  - Serial connection setup.

---

### `lamp_fw.ino`
- **Platform:** Arduino (AVR-based board).
- **Purpose:** Firmware controlling the heating element using a PID loop.
- **Features:**
  - Reads temperature from analog sensor (e.g., NTC thermistor).
  - Controls a MOSFET/relay to regulate heating.
  - Communicates with the Python GUI via Serial.

---

## ⚙️ How It Works

1. **GUI (Python)** sends target temperature to the Arduino.
2. **Arduino (lamp_fw.ino)** adjusts the heating based on PID control.
3. **GUI** plots live temperature data and allows user interaction.

---

## 🧪 Application

- Designed for **Loop-mediated Isothermal Amplification (LAMP)** assay at ~65°C.
- Ideal for **Monkeypox PoC diagnostics** in low-resource or mobile settings.

---

## 🔒 License & Usage

This repository is provided **only for portfolio and evaluation purposes.**  
**Do not reuse, distribute, or modify** any of the content without explicit written permission.  
All rights reserved © 2025 [Your Name]

---

## 🖼️ Demo & Visuals

![GUI Preview](Media/vid1.gif)
