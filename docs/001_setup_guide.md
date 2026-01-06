# Complete 10-Day Build Guide - IoT Multi-Sensor System

## Table of Contents
1. [Introduction](#introduction)
2. [Day-by-Day Build Plan](#day-by-day-build-plan)
3. [Complete Shopping List](#complete-shopping-list)
4. [Tool Requirements](#tool-requirements)
5. [Step-by-Step Assembly](#step-by-step-assembly)
6. [Testing Procedures](#testing-procedures)
7. [Troubleshooting After Each Step](#troubleshooting-after-each-step)
8. [Photo and Diagram Placeholders](#photo-and-diagram-placeholders)

---

## Introduction

This is a **comprehensive, production-ready guide** to building a complete IoT multi-sensor system from scratch. This system features:

- **Dual Camera Object Detection** using Arduino Nicla Vision with TinyML
- **Environmental Monitoring** with temperature, humidity, pressure, and gas sensing
- **Long-Range LoRaWAN Communication** up to 10km
- **WiFi Connectivity** for OTA updates
- **Real-time Dashboard** with data visualization
- **SCADA Integration** via Kepware OPC-UA

### Project Specifications

| Specification | Value |
|---------------|-------|
| **Total Cost** | ~$308 USD |
| **Build Time** | 10 days (2-3 hours/day) |
| **Difficulty** | Intermediate (requires basic electronics knowledge) |
| **Skills Required** | C++ programming, basic soldering, networking fundamentals |
| **Power Consumption** | 200-500mA typical |
| **LoRaWAN Range** | Up to 10km (line of sight) |
| **Detection Accuracy** | >80% with proper model training |

### Safety Warnings

**IMPORTANT: Read before starting**

- **Electrical Safety**: Always use a 5V power supply. Higher voltages will damage the boards.
- **ESD Protection**: Ground yourself before handling boards. Use anti-static mats when possible.
- **Battery Safety**: If using Li-Po batteries, use proper chargers and never leave charging unattended.
- **Heat Management**: Boards may get warm during operation. Ensure adequate ventilation.
- **Antenna Safety**: Always connect LoRa antenna before transmitting to prevent damage to the RF module.

---

## Day-by-Day Build Plan

### Day 1: Foundation and Planning

**Time Required**: 2-3 hours

**Objectives**:
- Unbox and inspect all components
- Set up development environment
- Test individual boards
- Plan physical layout

**Tasks**:
1. Unbox all components and verify against shopping list
2. Install Arduino IDE 2.0+ and required board packages
3. Install Node.js and backend dependencies
4. Test each Arduino board individually (upload blink sketch)
5. Create physical layout sketch for your deployment location
6. Label all boards (Vision #1, Vision #2, Sense Me, Gateway)

**Expected Outcome**:
- All components verified and working
- Development environment fully configured
- Clear plan for physical mounting

**Troubleshooting After Day 1**:
- If a board won't program: Check USB cable (use data cable, not charge-only)
- If Arduino IDE can't find board: Install drivers from Arduino website
- If board package won't install: Check internet connection, try again

### Day 2: Power Distribution and Basic Wiring

**Time Required**: 2-3 hours

**Objectives**:
- Build power distribution system
- Connect all boards to common power and ground
- Verify power consumption

**Tasks**:
1. Set up breadboard with power rails
2. Connect 5V supply to positive rail
3. Connect GND to negative rail
4. Install 1.5A fuse on positive rail for safety
5. Connect all VIN pins to 5V rail
6. Connect all GND pins to GND rail
7. **IMPORTANT**: Use multimeter to verify no shorts before powering
8. Measure current draw with all boards powered

**Wiring Diagram**:
```
Power Supply (5V 2A)
    |
    +--[Fuse 1.5A]--+
    |               |
   GND            +5V
    |               |
    +---+---+---+---+---+
    |   |   |   |   |   |
   GND GND GND GND GND GND
    |   |   |   |   |   |
    |   |   |   |   |   +--> MKR WAN VIN
    |   |   |   |   +------> Nicla V1 VIN
    |   |   |   +----------> Nicla V2 VIN
    |   |   +---------------> Nicla Sense VIN
    |   +-------------------> TCA9548A VCC
    +-----------------------> Common GND
```

**Expected Outcome**:
- All boards power on successfully
- Current draw 50-100mA at idle
- No unusual heat from boards or power supply
- All board LEDs illuminate

**Troubleshooting After Day 2**:
- If boards don't power on: Check fuse continuity, verify 5V at rails with multimeter
- If current draw >200mA at idle: Check for shorts, disconnect boards one at a time
- If board gets hot: Disconnect immediately, check for reversed polarity

### Day 3: I2C Multiplexer and Camera Integration

**Time Required**: 3-4 hours

**Objectives**:
- Install and configure TCA9548A I2C multiplexer
- Connect both Nicla Vision cameras via multiplexer
- Test I2C communication with all devices

**Tasks**:
1. Connect TCA9548A to MKR WAN 1310:
   - SDA → Pin 11 (SDA)
   - SCL → Pin 12 (SCL)
   - VCC → 5V
   - GND → GND
   - A0, A1, A2 → GND (sets address to 0x70)

2. Connect Nicla Vision #1 to Multiplexer Channel 0:
   - SDA → SD0
   - SCL → SC0
   - VIN → 5V
   - GND → GND

3. Connect Nicla Vision #2 to Multiplexer Channel 1:
   - SDA → SD1
   - SCL → SC1
   - VIN → 5V
   - GND → GND

4. Upload I2C scanner sketch to MKR WAN

5. Verify detection of:
   - 0x70 (TCA9548A)
   - 0x42 on channel 0 (Nicla Vision #1)
   - 0x42 on channel 1 (Nicla Vision #2)

**Complete I2C Wiring Table**:

| MKR WAN Pin | TCA9548A Pin | Nicla V1 | Nicla V2 | Nicla Sense |
|-------------|--------------|----------|----------|-------------|
| Pin 11 (SDA) | SDA | - | - | - |
| Pin 12 (SCL) | SCL | - | - | - |
| GND | A0, A1, A2 | GND | GND | GND |
| - | SD0 | SDA | - | - |
| - | SC0 | SCL | - | - |
| - | SD1 | - | SDA | - |
| - | SC1 | - | SCL | - |
| 5V | VCC | VIN | VIN | VIN |

**Expected Outcome**:
- I2C scanner detects all devices
- Both cameras accessible via multiplexer channels
- No I2C communication errors

**Troubleshooting After Day 3**:
- If only 0x70 detected: Check camera wiring, verify power to cameras
- If no devices detected: Check SDA/SCL connections, verify pull-up resistors
- If intermittent detection: Check for loose connections, try shorter wires

### Day 4: Sensor Integration and Serial Communication

**Time Required**: 2-3 hours

**Objectives**:
- Connect Nicla Sense Me via UART
- Configure serial communication
- Verify sensor data transmission

**Tasks**:
1. Connect Nicla Sense Me to MKR WAN via Serial1:
   - MKR TX1 (Pin 14) → Sense Me RX
   - MKR RX1 (Pin 15) → Sense Me TX
   - GND → GND (common ground)
   - VIN → 5V

2. Upload basic sensor sketch to Nicla Sense Me:
   ```cpp
   // This sketch reads all sensors and transmits via Serial
   #include <Arduino_BHY2.h>

   void setup() {
     Serial.begin(115200);
     BHY2.begin();
   }

   void loop() {
     BHY2.update();
     float temp = BHY2.getTemperature();
     float humid = BHY2.getHumidity();
     float press = BHY2.getPressure();

     Serial.print("TEMP:");
     Serial.print(temp);
     Serial.print("|HUMID:");
     Serial.print(humid);
     Serial.print("|PRESS:");
     Serial.println(press);
     delay(5000);
   }
   ```

3. Upload serial receiver sketch to MKR WAN

4. Verify data reception at 115200 baud

**UART Wiring Diagram**:
```
MKR WAN 1310              Nicla Sense Me
─────────────            ───────────────
Pin 14 (TX)   ─────────→ RX
Pin 15 (RX)   ←───────── TX
GND           ─────────→ GND
5V            ─────────→ VIN
```

**Expected Outcome**:
- Sensor data received every 5 seconds
- Temperature, humidity, pressure values displayed
- No garbled or missing data

**Troubleshooting After Day 4**:
- If no data received: Check TX/RX cross-connection, verify baud rate matches
- If garbled data: Check baud rate mismatch, verify ground connection
- If sensor values are 0 or NaN: Check BHY2 library installation, reset board

### Day 5: LoRaWAN Configuration and Testing

**Time Required**: 2-3 hours

**Objectives**:
- Register device on The Things Network
- Configure LoRaWAN credentials
- Test long-range communication

**Tasks**:
1. Create account on [The Things Network](https://console.thethingsnetwork.org/)

2. Create new application:
   - Go to Applications → Create Application
   - Name: "IoT-Multi-Sensor"
   - Handler: Select closest TTN server

3. Register end device:
   - Click "Register end device"
   - Select "Manually"
   - Frequency Plan: US915 (USA) or EU868 (Europe)
   - LoRaWAN version: 1.0.3
   - Copy the generated credentials

4. Update firmware with TTN credentials:
   ```cpp
   const char *appEui = "YOUR_APP_EUI";      // From TTN
   const char *appKey = "YOUR_APP_KEY";      // From TTN
   const char *devEui = "YOUR_DEV_EUI";      // From TTN
   ```

5. Connect LoRa antenna (CRITICAL: Do this before powering!)

6. Upload LoRaWAN sketch to MKR WAN

7. Monitor Serial Monitor for join messages

8. Verify uplink on TTN Console

**LoRaWAN Configuration Table**:

| Parameter | Value (US915) | Value (EU868) |
|-----------|---------------|---------------|
| Frequency | 915 MHz | 868 MHz |
| TX Power | 20 dBm | 14 dBm |
| Spreading Factor | SF7-SF10 | SF7-SF10 |
| Bandwidth | 125 kHz | 125 kHz |
| Duty Cycle | None | 1% limit |

**Expected Outcome**:
- Device joins TTN network within 2 minutes
- Uplink packets visible on TTN Console
- RSSI between -50 to -100 dBm (good signal)

**Troubleshooting After Day 5**:
- If join timeout: Verify antenna connected, check credentials, ensure correct frequency
- If no uplinks: Check device is activated on TTN, verify antenna connection
- If weak signal (< -100 dBm): Move closer to gateway, check antenna placement

### Day 6: Camera Object Detection Setup

**Time Required**: 3-4 hours

**Objectives**:
- Configure TensorFlow Lite Micro on Nicla Vision
- Load object detection model
- Test detection accuracy

**Tasks**:
1. Install TensorFlow Lite for Microcontrollers library

2. Download pre-trained model:
   - Person detection model
   - MobileNet SSD or COCO model
   - Place in `src/models/` directory

3. Upload object detection sketch to Nicla Vision #1:
   ```cpp
   #include <TensorFlowLite.h>
   #include <Arduino_OV5640.h>

   // Model setup
   const unsigned char* model = detect_model_data;
   tf lite::MicroInterpreter* interpreter;

   void setup() {
     Serial.begin(115200);
     Camera.begin(320, 240);
     setupModel();
   }

   void loop() {
     uint8_t* image = Camera.read();
     detectObjects(image);
     delay(1000);
   }
   ```

4. Upload same sketch to Nicla Vision #2

5. Test detection with:
   - Person standing in front of camera
   - Vehicle (toy car or real)
   - Animal (pet)

6. Measure detection latency and accuracy

**Expected Outcome**:
- Detection latency <2 seconds
- Accuracy >80% for person detection
- Serial output shows detected objects with confidence scores

**Troubleshooting After Day 6**:
- If model won't load: Verify model file format, check available memory
- If detection is slow: Reduce image resolution, optimize model
- If accuracy is low: Improve lighting, adjust detection threshold

### Day 7: Backend and Dashboard Setup

**Time Required**: 2-3 hours

**Objectives**:
- Install backend dependencies
- Set up SQLite database
- Launch real-time dashboard

**Tasks**:
1. Install Node.js dependencies:
   ```bash
   cd /private/var/folders/_f/lqvwt00n4hg98fxhkgg1cfjc0000gn/T/vibe-kanban/worktrees/17ff-/Ideation/src/backend
   npm install express body-parser sqlite3 ws opcua
   ```

2. Create database schema:
   ```bash
   node init_database.js
   ```

3. Start backend server:
   ```bash
   node data_pipeline.js
   ```

4. Verify server startup:
   ```
   Connected to SQLite database
   Database tables initialized
   IoT Data Pipeline server running on port 3000
   WebSocket server running on port 8080
   Dashboard: http://localhost:3000
   ```

5. Open dashboard in browser: http://localhost:3000

6. Verify real-time updates as sensor data arrives

**Dashboard Features**:
- Real-time sensor readings (temperature, humidity, pressure)
- 24-hour trend charts
- Detection feed with confidence scores
- System status indicators
- Historical data export

**Expected Outcome**:
- Dashboard loads successfully
- Sensor data updates in real-time
- Charts render correctly
- No console errors

**Troubleshooting After Day 7**:
- If server won't start: Check Node.js version (18+), verify all dependencies installed
- If dashboard blank: Check browser console for errors, verify backend running
- If no real-time updates: Verify WebSocket connection, check firewall settings

### Day 8: Alarm and Notification System

**Time Required**: 2-3 hours

**Objectives**:
- Configure local alarms (LED, buzzer)
- Set up email notifications
- Test alarm triggering

**Tasks**:
1. Connect alarm hardware:
   ```
   MKR WAN Pin 5 → Green LED → 220Ω → GND
   MKR WAN Pin 6 → Yellow LED → 220Ω → GND
   MKR WAN Pin 7 → Red LED → 220Ω → GND
   MKR WAN Pin 8 → Buzzer → GND
   ```

2. Upload alarm control sketch to MKR WAN

3. Configure alarm conditions:
   - Person detected: Red LED + buzzer
   - Temperature > 30°C: Yellow LED
   - Humidity > 80%: Yellow LED
   - System OK: Green LED

4. Set up email notifications in backend:
   ```javascript
   const nodemailer = require('nodemailer');

   const transporter = nodemailer.createTransport({
     service: 'gmail',
     auth: {
       user: 'your-email@gmail.com',
       pass: 'your-app-password'
     }
   });

   function sendAlert(message) {
     transporter.sendMail({
       from: 'iot-alerts@gmail.com',
       to: 'recipient@gmail.com',
       subject: 'IoT Alert',
       text: message
     });
   }
   ```

5. Test alarms:
   - Trigger person detection
   - Simulate high temperature
   - Verify email received

**Expected Outcome**:
- Alarms trigger within 2 seconds of detection
- LEDs indicate alarm type correctly
- Email notifications received within 30 seconds
- Buzzer can be silenced via dashboard

**Troubleshooting After Day 8**:
- If LEDs don't work: Check polarity (long leg = positive), verify resistor value
- If buzzer doesn't sound: Check buzzer type (active vs passive), verify pin number
- If email not received: Check app password, verify SMTP settings

### Day 9: WiFi and OTA Updates

**Time Required**: 2-3 hours

**Object objectives**:
- Configure WiFi connectivity
- Set up OTA update server
- Test remote firmware update

**Tasks**:
1. Configure WiFi credentials in firmware:
   ```cpp
   const char* ssid = "YOUR_WIFI_SSID";
   const char* password = "YOUR_WIFI_PASSWORD";
   ```

2. Install ArduinoOTA library on all boards

3. Upload OTA-enabled sketch:
   ```cpp
   #include <WiFiNINA.h>
   #include <ArduinoOTA.h>

   void setup() {
     WiFi.begin(ssid, password);
     while (WiFi.status() != WL_CONNECTED) {
       delay(500);
       Serial.print(".");
     }

     ArduinoOTA.begin("MKR-WAN-Gateway");
   }

   void loop() {
     ArduinoOTA.poll();
     // Your code here
   }
   ```

4. Set up OTA update server (Node.js):
   ```bash
   node ota_server.js
   ```

5. Test OTA update:
   - Make small code change
   - Upload new firmware via WiFi
   - Verify board updates and restarts

6. Document OTA update procedure

**Expected Outcome**:
- Boards connect to WiFi successfully
- OTA updates complete without physical access
- Update time <5 minutes for typical firmware
- No data loss during update

**Troubleshooting After Day 9**:
- If WiFi won't connect: Check SSID/password, verify 2.4GHz network
- If OTA fails: Check board is on same network, verify firewall settings
- If update corrupts firmware: Implement rollback mechanism

### Day 10: Integration, Testing, and Documentation

**Time Required**: 3-4 hours

**Objectives**:
- End-to-end system testing
- Performance benchmarking
- Documentation finalization

**Tasks**:
1. Run complete system test:
   ```
   ✓ Person detection → LoRa transmission → Dashboard update
   ✓ Sensor reading → LoRa transmission → Database storage
   ✓ Alarm trigger → Email notification → Buzzer activation
   ✓ WiFi OTA update → Board restart → Continued operation
   ```

2. Performance benchmarks:
   - Detection latency: <2 seconds
   - LoRa transmission success: >95%
   - System uptime: >95% over 1-hour test
   - Power consumption: 200-500mA typical

3. Create deployment documentation:
   - System configuration settings
   - Network credentials (encrypted)
   - Calibration data
   - Maintenance schedule

4. Create user manual:
   - Daily operation procedures
   - Troubleshooting quick reference
   - Contact information for support

5. Final system check:
   - Verify all components secured
   - Check all cable connections
   - Test alarm system
   - Verify data logging
   - Confirm backup procedures

**Acceptance Criteria**:
- [ ] All sensors reading accurately (±5% reference)
- [ ] Object detection accuracy >80%
- [ ] LoRaWAN transmission success >95%
- [ ] Dashboard updates in real-time (<1 second delay)
- [ ] Alarms trigger correctly
- [ ] OTA updates successful
- [ ] Documentation complete
- [ ] System ready for deployment

**Troubleshooting After Day 10**:
- If system unstable: Review logs, check for memory leaks
- If accuracy low: Recalibrate sensors, retrain ML model
- If documentation incomplete: Add missing sections

---

## Complete Shopping List

### Essential Components (Required)

#### Microcontrollers

| Component | Quantity | Unit Price | Total | Link |
|-----------|----------|------------|-------|------|
| Arduino MKR WAN 1310 | 1 | $44.95 | $44.95 | [Arduino Store](https://store.arduino.cc/products/arduino-mkr-wan-1310) |
| Arduino Nicla Vision | 2 | $69.95 | $139.90 | [Arduino Store](https://store.arduino.cc/products/arduino-nicla-vision) |
| Arduino Nicla Sense Me | 1 | $44.95 | $44.95 | [Arduino Store](https://store.arduino.cc/products/arduino-nicla-sense-me) |

#### Communication Modules

| Component | Quantity | Unit Price | Total | Link |
|-----------|----------|------------|-------|------|
| TCA9548A I2C Multiplexer | 1 | $2.95 | $2.95 | [Adafruit](https://www.adafruit.com/product/2717) |
| LoRaWAN Antenna (868/915MHz) | 1 | $9.95 | $9.95 | [Arduino Store](https://store.arduino.cc/products/antenna-dipole-antenna-lora) |
| WiFi Antenna (U.FL) | 1 | $4.95 | $4.95 | [Adafruit](https://www.adafruit.com/product/583) |

#### Power System

| Component | Quantity | Unit Price | Total | Link |
|-----------|----------|------------|-------|------|
| 5V 2A Power Supply | 1 | $9.95 | $9.95 | [Adafruit](https://www.adafruit.com/product/276) |
| DC Barrel Jack Adapter | 1 | $2.50 | $2.50 | [Adafruit](https://www.adafruit.com/product/368) |
| 1.5A Fuse Holder + Fuse | 1 | $3.00 | $3.00 | [Amazon](https://www.amazon.com) |

#### Wiring and Prototyping

| Component | Quantity | Unit Price | Total | Link |
|-----------|----------|------------|-------|------|
| Jumper Wires (M-M) | 1 pack (40) | $5.95 | $5.95 | [Adafruit](https://www.adafruit.com/product/758) |
| Jumper Wires (M-F) | 1 pack (40) | $5.95 | $5.95 | [Adafruit](https://www.adafruit.com/product/826) |
| Breadboard (830pt) | 1 | $9.95 | $9.95 | [Adafruit](https://www.adafruit.com/product/239) |

#### Indicators and Alarms

| Component | Quantity | Unit Price | Total | Link |
|-----------|----------|------------|-------|------|
| Red LED 5mm | 1 | $0.30 | $0.30 | [Adafruit](https://www.adafruit.com/product/297) |
| Green LED 5mm | 1 | $0.30 | $0.30 | [Adafruit](https://www.adafruit.com/product/298) |
| Yellow LED 5mm | 1 | $0.30 | $0.30 | [Adafruit](https://www.adafruit.com/product/300) |
| 220Ω Resistors | 10 | $0.10 | $1.00 | [Adafruit](https://www.adafruit.com/product/2784) |
| Active Buzzer 5V | 1 | $1.95 | $1.95 | [Adafruit](https://www.adafruit.com/product/1536) |

#### Enclosure and Mounting

| Component | Quantity | Unit Price | Total | Link |
|-----------|----------|------------|-------|------|
| Project Case IP65 | 1 | $19.95 | $19.95 | [Adafruit](https://www.adafruit.com/product/1921) |
| M3 Standoffs (10mm) | 8 | $0.25 | $2.00 | [Adafruit](https://www.adafruit.com/product/1230) |
| M3 Screws and Nuts | 1 set | $3.95 | $3.95 | [Amazon](https://www.amazon.com) |
| Thermal Paste | 1 tube | $4.95 | $4.95 | [Adafruit](https://www.adafruit.com/product/3530) |

**TOTAL ESSENTIAL**: $318.70

### Optional Accessories (Recommended)

| Component | Quantity | Unit Price | Total | Purpose |
|-----------|----------|------------|-------|---------|
| Li-Po Battery 2000mAh | 1 | $14.95 | $14.95 | Portable power |
| Solar Panel 6V 2W | 1 | $12.95 | $12.95 | Outdoor charging |
| TP4056 Charger Module | 1 | $1.95 | $1.95 | Battery charging |
| OLED Display 128x64 | 1 | $7.95 | $7.95 | Local status display |
| MicroSD Card Module | 1 | $4.95 | $4.95 | Local data logging |
| MicroSD Card 32GB | 1 | $8.95 | $8.95 | Data storage |
| DHT22 Temperature Sensor | 1 | $4.95 | $4.95 | Additional sensor |
| PIR Motion Sensor | 1 | $2.95 | $2.95 | Motion-triggered wake |

**TOTAL OPTIONAL**: $59.60

### Tools Required

| Tool | Quantity | Unit Price | Total | Link |
|------|----------|------------|-------|------|
| Multimeter | 1 | $19.95 | $19.95 | [Amazon](https://www.amazon.com) |
| Wire Strippers | 1 | $8.95 | $8.95 | [Amazon](https://www.amazon.com) |
| Soldering Iron | 1 | $14.95 | $14.95 | [Amazon](https://www.amazon.com) |
| USB-C Cables (3 pack) | 1 | $9.95 | $9.95 | [Amazon](https://www.amazon.com) |
| Small Screwdriver Set | 1 | $6.95 | $6.95 | [Amazon](https://www.amazon.com) |

**TOTAL TOOLS**: $60.75

### Grand Total: $439.05 (with all optional items and tools)

**Minimum Viable System**: $318.70 (essential components only)

---

## Tool Requirements

### Essential Tools (Must Have)

#### 1. Multimeter
**Purpose**: Voltage testing, continuity checking, current measurement

**Required Features**:
- DC voltage measurement (0-20V range)
- Resistance/continuity testing
- Current measurement (0-2A range)
- Auto-ranging recommended

**Recommended Models**:
- AstroAI AM33D ($19.95)
- INNOVA 3320 ($24.99)

#### 2. Wire Strippers
**Purpose**: Stripping insulation from jumper wires

**Required Features**:
- AWG 20-30 range
- Clean stripping without damaging wire

**Recommended**:
- Klein Tools D213-9 ($14.99)
- Irwin Vise-Grip ($8.95)

#### 3. USB-C Cables
**Purpose**: Programming Arduino boards

**Requirements**:
- USB-C to USB-A (for computer connection)
- Data cables, not charge-only
- At least 3 cables for multiple boards

**Recommended**:
- Anker PowerLine cables ($9.95 for 3-pack)

### Optional Tools (Nice to Have)

#### 4. Soldering Iron
**Purpose**: Permanent connections, custom cables

**Requirements**:
- Temperature adjustable (300-400°C)
- Fine tip for precision work
- Stand with sponge

**Recommended**:
- Weller WLC100 ($39.95)
- FX-888D ($99.95 - professional)

#### 5. Hot Glue Gun
**Purpose**: Securing wires, strain relief

**Requirements**:
- Low temperature mode (for plastics)
- Adjustable flow

**Recommended**:
- AdTech Hot Mue ($9.95)

#### 6. Helping Hands
**Purpose**: Holding wires during soldering

**Requirements**:
- Magnetic base
- Adjustable arms with alligator clips

**Recommended**:
- Neiko 01902A ($9.99)

#### 7. Oscilloscope (Advanced)
**Purpose**: Analyzing I2C signals, debugging communication

**Requirements**:
- 2 channels minimum
- 50MHz bandwidth minimum
- Digital storage recommended

**Recommended**:
- Rigol DS1054Z ($399 - advanced users)

### Software Tools

#### Required Software

1. **Arduino IDE 2.0+**
   - Download: https://www.arduino.cc/en/software
   - Install board packages via Board Manager

2. **Node.js 18+**
   - Download: https://nodejs.org/
   - Verify installation: `node --version`

3. **Git** (for version control)
   - Download: https://git-scm.com/
   - Verify installation: `git --version`

4. **Serial Terminal**
   - Arduino IDE Serial Monitor (built-in)
   - PuTTY (Windows)
   - screen or minicom (Linux/Mac)

#### Optional Software

1. **Fritzing** (circuit diagrams)
   - Download: https://fritzing.org/

2. **VS Code** (code editing)
   - Download: https://code.visualstudio.com/

3. **Wireshark** (network debugging)
   - Download: https://www.wireshark.org/

---

## Step-by-Step Assembly

### Phase 1: Component Preparation

#### Step 1.1: Inspect All Components

**Time**: 15 minutes

1. Remove all components from packaging
2. Verify against shopping list
3. Check for visible damage:
   - Bent pins on Arduino boards
   - Cracks in boards
   - Missing components
4. Label each board with tape:
   - "MKR WAN Gateway"
   - "Nicla Vision #1"
   - "Nicla Vision #2"
   - "Nicla Sense Me"

**Acceptance Criteria**:
- All components accounted for
- No visible damage
- All boards labeled

#### Step 1.2: Test Individual Boards

**Time**: 30 minutes

Test each board with blink sketch:

```cpp
// Basic blink test
void setup() {
  pinMode(LED_BUILTIN, OUTPUT);
}

void loop() {
  digitalWrite(LED_BUILTIN, HIGH);
  delay(1000);
  digitalWrite(LED_BUILTIN, LOW);
  delay(1000);
}
```

**Testing Procedure**:
1. Connect board via USB-C
2. Select correct board in Arduino IDE
3. Select correct port
4. Upload blink sketch
5. Verify LED blinks

**Acceptance Criteria**:
- All boards accept sketch upload
- All LEDs blink correctly
- No upload errors

### Phase 2: Power System Assembly

#### Step 2.1: Breadboard Layout

**Time**: 30 minutes

```
┌─────────────────────────────────────────────────┐
│  Power Rail (+)     5V  ◄────────────┐          │
│  ─────────────────────────────────────│─┐       │
│                                           │       │
│  Power Rail (-)     GND  ◄────────────┐ │       │
│  ─────────────────────────────────────│─│───┐   │
│                                         │   │   │
│  Component Area                          │   │   │
│  ┌─────────┐  ┌─────────┐  ┌─────────┐ │   │   │
│  │ MKR WAN │  │ Nicla V1│  │ Nicla V2│ │   │   │
│  └─────────┘  └─────────┘  └─────────┘ │   │   │
│                                         │   │   │
│  ┌─────────┐  ┌─────────┐              │   │   │
│  │NiclaSense│ │TCA9548A │              │   │   │
│  └─────────┘  └─────────┘              │   │   │
│                                         │   │   │
└─────────────────────────────────────────┴───┴───┘
```

#### Step 2.2: Install Power Distribution

**Time**: 45 minutes

1. Connect power supply to breadboard rails:
   - Positive (+) to red power rail
   - Negative (-) to blue power rail
   - DO NOT power on yet!

2. Install fuse holder:
   - Place in series with positive lead
   - Use 1.5A fast-blow fuse
   - Test fuse continuity with multimeter

3. Verify with multimeter:
   - Measure resistance between rails (should be infinite)
   - Check for shorts (VIN to GND on each board)
   - Verify fuse continuity

4. Power on and verify:
   - Measure 5V on positive rail
   - Measure 0V on negative rail
   - Check current (should be 0mA with no load)

**Acceptance Criteria**:
- 5V present on power rail
- No shorts detected
- Fuse installed correctly
- Current draw <50mA with no boards

### Phase 3: I2C Multiplexer Installation

#### Step 3.1: Mount TCA9548A

**Time**: 20 minutes

```
TCA9548A Pinout:
┌─────────────┐
│ VCC  SDA  A0│
│ GND  SCL  A1│
│ A2   SD0 SC0│
│ SD1  SD2 SC1│
│ SC2  SD3 SC3│
│ SD4  SD5 SC4│
│ SC5  SD6 SC5│
│ SC6  SD7 SC7│
│ RST  VCC  GND│
└─────────────┘
```

**Wiring**:
```
MKR WAN 1310    TCA9548A
─────────────   ─────────
Pin 11 (SDA)  → SDA
Pin 12 (SCL)  → SCL
GND           → A0
GND           → A1
GND           → A2
GND           → GND
5V            → VCC
```

**Acceptance Criteria**:
- All connections secure
- Address pins A0-A2 grounded
- Power and ground connected
- No solder bridges

#### Step 3.2: Test Multiplexer

**Time**: 15 minutes

Upload I2C scanner to MKR WAN:

```cpp
#include <Wire.h>

void setup() {
  Wire.begin();
  Serial.begin(115200);
  Serial.println("I2C Scanner");
}

void loop() {
  for (byte addr = 1; addr < 127; addr++) {
    Wire.beginTransmission(addr);
    if (Wire.endTransmission() == 0) {
      Serial.print("Found: 0x");
      Serial.println(addr, HEX);
    }
  }
  delay(5000);
}
```

**Expected Output**:
```
I2C Scanner
Found: 0x70
```

**Acceptance Criteria**:
- Multiplexer detected at 0x70
- No other devices yet (cameras not connected)
- Scanner runs without errors

### Phase 4: Camera Integration

#### Step 4.1: Connect Nicla Vision #1

**Time**: 30 minutes

**Wiring**:
```
TCA9548A         Nicla Vision #1
────────         ───────────────
SD0 (Pin 5)  →   SDA
SC0 (Pin 13) →   SCL
GND          →   GND
5V           →   VIN
```

**Test**:
- Run I2C scanner
- Should detect 0x70 (multiplexer)
- Should detect 0x42 on channel 0 (camera)

**Multiplexer Test Code**:
```cpp
#include <Wire.h>

#define TCA_ADDR 0x70

void tcaSelect(uint8_t channel) {
  Wire.beginTransmission(TCA_ADDR);
  Wire.write(1 << channel);
  Wire.endTransmission();
}

void setup() {
  Wire.begin();
  Serial.begin(115200);
}

void loop() {
  // Scan channel 0
  tcaSelect(0);
  for (byte addr = 1; addr < 127; addr++) {
    Wire.beginTransmission(addr);
    if (Wire.endTransmission() == 0) {
      Serial.print("Ch0 Found: 0x");
      Serial.println(addr, HEX);
    }
  }
  delay(2000);
}
```

#### Step 4.2: Connect Nicla Vision #2

**Time**: 30 minutes

**Wiring**:
```
TCA9548A         Nicla Vision #2
────────         ───────────────
SD1 (Pin 6)  →   SDA
SC1 (Pin 14) →   SCL
GND          →   GND
5V           →   VIN
```

**Test**:
- Both cameras detected on respective channels
- No address conflicts

**Acceptance Criteria**:
- Camera #1 detected on channel 0
- Camera #2 detected on channel 1
- Both cameras respond to I2C commands

### Phase 5: Sensor Integration

#### Step 5.1: Connect Nicla Sense Me

**Time**: 30 minutes

**Wiring**:
```
MKR WAN 1310       Nicla Sense Me
─────────────      ───────────────
Pin 14 (TX)  →     RX
Pin 15 (RX)  ←     TX
GND          →     GND
5V           →     VIN
```

**IMPORTANT**: Cross-connect TX to RX!

**Test Sketch for Nicla Sense Me**:
```cpp
#include <Arduino_BHY2.h>

void setup() {
  Serial.begin(115200);
  BHY2.begin();
}

void loop() {
  BHY2.update();
  float temp = BHY2.getTemperature();
  float humid = BHY2.getHumidity();
  float press = BHY2.getPressure();

  Serial.print("T:");
  Serial.print(temp);
  Serial.print("|H:");
  Serial.print(humid);
  Serial.print("|P:");
  Serial.println(press);
  delay(5000);
}
```

**Receiver Sketch for MKR WAN**:
```cpp
#define SERIAL_BAUD 115200

void setup() {
  Serial.begin(SERIAL_BAUD);
  Serial1.begin(SERIAL_BAUD);
}

void loop() {
  if (Serial1.available()) {
    String data = Serial1.readStringUntil('\n');
    Serial.print("Sense Me: ");
    Serial.println(data);
  }
}
```

**Acceptance Criteria**:
- Data received every 5 seconds
- Values are reasonable (T: 20-30°C, H: 40-70%, P: ~1000 hPa)
- No garbled data

### Phase 6: Antenna Installation

#### Step 6.1: LoRaWAN Antenna

**Time**: 10 minutes

**CRITICAL**: Always connect antenna BEFORE transmitting!

1. Locate MHF connector marked "LoRa" on MKR WAN
2. Align antenna connector
3. Push firmly until click
4. Gently tug to verify connection

**DO NOT**:
- Power on board without antenna
- Transmit without antenna (will damage RF module!)
- Use wrong frequency antenna

#### Step 6.2: WiFi Antenna

**Time**: 10 minutes

1. Locate U.FL connector marked "WiFi"
2. Align antenna connector
3. Push firmly until click
4. Verify connection

**Acceptance Criteria**:
- Both antennas securely attached
- No loose connections
- Correct frequency antennas

### Phase 7: Alarm System

#### Step 7.1: Install LEDs

**Time**: 20 minutes

**Wiring**:
```
MKR WAN Pin 5 → 220Ω → Green LED (+) → GND
MKR WAN Pin 6 → 220Ω → Yellow LED (+) → GND
MKR WAN Pin 7 → 220Ω → Red LED (+) → GND
```

**LED Orientation**:
- Long leg = positive (anode)
- Short leg = negative (cathode)
- Flat side on LED housing = negative

**Test Code**:
```cpp
#define GREEN_LED 5
#define YELLOW_LED 6
#define RED_LED 7

void setup() {
  pinMode(GREEN_LED, OUTPUT);
  pinMode(YELLOW_LED, OUTPUT);
  pinMode(RED_LED, OUTPUT);
}

void loop() {
  digitalWrite(GREEN_LED, HIGH);
  delay(1000);
  digitalWrite(GREEN_LED, LOW);

  digitalWrite(YELLOW_LED, HIGH);
  delay(1000);
  digitalWrite(YELLOW_LED, LOW);

  digitalWrite(RED_LED, HIGH);
  delay(1000);
  digitalWrite(RED_LED, LOW);
}
```

**Acceptance Criteria**:
- All LEDs illuminate
- Correct colors (green, yellow, red)
- No LEDs damaged

#### Step 7.2: Install Buzzer

**Time**: 10 minutes

**Wiring**:
```
MKR WAN Pin 8 → Buzzer (+) → GND
```

**Note**: Active buzzer has polarity (+ marked)

**Test Code**:
```cpp
#define BUZZER 8

void setup() {
  pinMode(BUZZER, OUTPUT);
}

void loop() {
  tone(BUZZER, 1000, 500);
  delay(1000);
}
```

**Acceptance Criteria**:
- Buzzer produces sound
- Volume is adequate
- No distortion

### Phase 8: Final Assembly

#### Step 8.1: Secure All Connections

**Time**: 30 minutes

1. Check all jumper wires:
   - Push firmly into breadboard
   - No loose connections
   - Label important wires

2. Organize cables:
   - Use cable ties
   - Separate power from signal
   - Keep antennas away from wires

3. Test all connections:
   - Gently tug each wire
   - Verify no intermittent connections
   - Re-seat any loose wires

#### Step 8.2: Mount in Enclosure

**Time**: 1 hour

1. Plan layout:
   - Leave space for airflow
   - Position antennas near openings
   - Allow access to USB ports

2. Install standoffs:
   - Use M3 standoffs
   - Secure with screws
   - Align boards

3. Mount boards:
   - Place on standoffs
   - Secure with nuts
   - Don't overtighten!

4. Route cables:
   - Use strain relief
   - Avoid sharp bends
   - Keep away from heat sources

5. Install thermal paste:
   - Apply to board voltage regulators
   - Thin layer only
   - Wipe excess

**Acceptance Criteria**:
- All boards secure
- No loose cables
- Good airflow
- Easy access to ports

---

## Testing Procedures

### Test 1: Power System Test

**Objective**: Verify power distribution and consumption

**Procedure**:
1. Set multimeter to DC current (2A range)
2. Connect in series with power supply
3. Power on system
4. Measure current at each state:

| State | Expected Current | Pass/Fail |
|-------|------------------|-----------|
| All boards idle | 50-100 mA | |
| One camera active | 150-250 mA | |
| Both cameras active | 300-450 mA | |
| LoRa transmitting | 400-650 mA | |

**Acceptance Criteria**:
- Idle current <100 mA
- Peak current <700 mA
- No overheating

**Troubleshooting**:
- If current >700 mA: Check for shorts, disconnect boards one at a time
- If current <50 mA: Check all boards powered, verify connections

### Test 2: I2C Communication Test

**Objective**: Verify all I2C devices accessible

**Procedure**:
1. Upload I2C scanner to MKR WAN
2. Scan all multiplexer channels
3. Verify detection:

| Device | Address | Channel | Expected | Pass/Fail |
|--------|---------|---------|----------|-----------|
| TCA9548A | 0x70 | - | Yes | |
| Nicla V1 | 0x42 | 0 | Yes | |
| Nicla V2 | 0x42 | 1 | Yes | |
| Nicla Sense | 0x28 | 2 | Yes | |

**Acceptance Criteria**:
- All devices detected
- No address conflicts
- Consistent detection

### Test 3: Sensor Accuracy Test

**Objective**: Verify sensor readings within specifications

**Procedure**:
1. Place reference thermometer/hygrometer near system
2. Let system stabilize for 10 minutes
3. Compare readings:

| Sensor | Reading | Reference | Tolerance | Pass/Fail |
|--------|---------|-----------|-----------|-----------|
| Temperature | ___.__ °C | ___.__ °C | ±0.5°C | |
| Humidity | ___.__ % | ___.__ % | ±3% | |
| Pressure | ___._ hPa | ___._ hPa | ±1 hPa | |

**Acceptance Criteria**:
- Temperature within ±0.5°C
- Humidity within ±3%
- Pressure within ±1 hPa

### Test 4: Camera Detection Test

**Objective**: Verify object detection accuracy and latency

**Procedure**:
1. Set up test objects:
   - Person (stand in front of camera)
   - Vehicle (toy car or real)
   - Animal (pet or picture)

2. Test detection:
   - Present object to camera
   - Record detection time
   - Record detection confidence

3. Repeat 10 times per object class

**Results Table**:

| Object | Trials | Detections | Accuracy | Avg Confidence | Avg Latency | Pass/Fail |
|--------|--------|------------|----------|----------------|-------------|-----------|
| Person | 10 | | >80% | | <2s | |
| Vehicle | 10 | | >80% | | <2s | |
| Animal | 10 | | >80% | | <2s | |

**Acceptance Criteria**:
- Detection accuracy >80%
- Confidence >60% for correct detections
- Latency <2 seconds

### Test 5: LoRaWAN Communication Test

**Objective**: Verify long-range communication

**Procedure**:
1. Register device on TTN
2. Place system at known distance from gateway
3. Send test packets
4. Measure success rate:

| Distance | Packets Sent | Packets Received | Success Rate | Pass/Fail |
|----------|--------------|------------------|--------------|-----------|
| 100m | 100 | | >95% | |
| 500m | 100 | | >95% | |
| 1km | 100 | | >95% | |
| 5km | 100 | | >90% | |

**TTN Console Checks**:
- Device shows as "Active"
- Uplink packets visible
- RSSI between -50 to -120 dBm
- SNR > -10 dB

**Acceptance Criteria**:
- 1km range with >95% success
- 5km range with >90% success
- RSSI > -120 dBm

### Test 6: Alarm System Test

**Objective**: Verify alarm triggering and notification

**Procedure**:
1. Trigger person detection
2. Observe alarm response:
   - Red LED illuminates
   - Buzzer sounds
   - Dashboard shows alarm
   - Email received (if configured)

3. Measure response time:

| Event | Detection Time | Alarm Time | Email Time | Total Latency | Pass/Fail |
|-------|---------------|------------|------------|---------------|-----------|
| Person detected | | | | <2s | |
| High temp | | | | <5s | |
| High humidity | | | | <5s | |

**Acceptance Criteria**:
- Alarm triggers within 2 seconds of detection
- Email received within 30 seconds
- Buzzer can be silenced

### Test 7: WiFi and OTA Test

**Objective**: Verify remote update capability

**Procedure**:
1. Connect to WiFi network
2. Verify board IP address
3. Initiate OTA update
4. Monitor progress:

| Step | Duration | Success | Pass/Fail |
|------|----------|---------|-----------|
| WiFi connect | <30s | | |
| Update download | <2min | | |
| Update flash | <3min | | |
| Board restart | <10s | | |

**Acceptance Criteria**:
- OTA update completes successfully
- Board restarts automatically
- No data loss during update

### Test 8: End-to-End System Test

**Objective**: Verify complete system operation

**Procedure**:
1. Start all components
2. Trigger detection event
3. Trace data flow:

| Step | Component | Status | Pass/Fail |
|------|-----------|--------|-----------|
| 1 | Camera captures image | | |
| 2 | Object detected | | |
| 3 | Detection sent to gateway | | |
| 4 | Gateway aggregates data | | |
| 5 | LoRa packet transmitted | | |
| 6 | Packet received by TTN | | |
| 7 | Backend processes data | | |
| 8 | Dashboard updates | | |
| 9 | Alarm triggered | | |
| 10 | Email sent | | |

**Acceptance Criteria**:
- All steps complete successfully
- End-to-end latency <5 seconds
- No data loss

### Test 9: 24-Hour Stability Test

**Objective**: Verify long-term reliability

**Procedure**:
1. Start system monitoring
2. Log all events for 24 hours
3. Record statistics:

| Metric | Value | Target | Pass/Fail |
|--------|-------|--------|-----------|
| Uptime | hours | >23h | |
| Detections | count | varies | |
| LoRa success rate | % | >95% | |
| Failed transmissions | count | <5% | |
| System restarts | count | 0 | |
| Errors logged | count | <10 | |

**Acceptance Criteria**:
- Uptime >95%
- LoRa success >95%
- No unexpected restarts

### Test 10: Power Consumption Test

**Objective**: Measure actual power consumption

**Procedure**:
1. Measure current at each state
2. Calculate daily energy consumption

| State | Current (mA) | Duty Cycle | Avg Current (mA) | Daily mAh |
|-------|--------------|------------|------------------|-----------|
| Sleep | 50 | 80% | 40 | 960 |
| Sensor read | 150 | 15% | 22.5 | 540 |
| Detection | 350 | 4% | 14 | 336 |
| LoRa TX | 500 | 1% | 5 | 120 |
| **Total** | | | **81.5** | **1956** |

**Battery Life Calculation**:
- 2000mAh battery / 1956 mAh/day = **1.02 days**

**Acceptance Criteria**:
- Average current <100 mA
- Daily consumption <2500 mAh
- Battery life >24 hours with 2000mAh

---

## Troubleshooting After Each Step

### After Day 1: Foundation

**Problem**: Board won't accept sketch upload

**Solutions**:
1. Check USB cable (use data cable, not charge-only)
2. Try different USB port
3. Press reset button before upload
4. Install Arduino drivers
5. Select correct board and port

**Problem**: Arduino IDE won't install board package

**Solutions**:
1. Check internet connection
2. Clear Arduino IDE cache
3. Restart Arduino IDE
4. Try installing manually

**Problem**: Board not detected by computer

**Solutions**:
1. Try different USB cable
2. Test on different computer
3. Check Device Manager (Windows) or lsusb (Linux)
4. Install Arduino drivers

---

### After Day 2: Power Distribution

**Problem**: Boards don't power on

**Solutions**:
1. Check fuse continuity with multimeter
2. Verify 5V at power rails
3. Check all VIN connections
4. Test power supply voltage
5. Verify power supply can provide 2A

**Problem**: High current draw (>500 mA at idle)

**Solutions**:
1. Disconnect boards one at a time
2. Check for short circuits
3. Measure individual board current
4. Verify 5V (not higher voltage)
5. Check for damaged boards

**Problem**: Board gets hot

**Solutions**:
1. Disconnect immediately
2. Check for reversed polarity
3. Verify input voltage is 5V
4. Check for short circuits
5. Test board individually

---

### After Day 3: I2C Multiplexer

**Problem**: I2C scanner finds no devices

**Solutions**:
1. Check SDA/SCL wiring
2. Verify pull-up resistors (4.7kΩ)
3. Check multiplexer address pins (A0-A2)
4. Test multiplexer alone
5. Try slower I2C speed (100kHz)

**Problem**: Only one camera detected

**Solutions**:
1. Check multiplexer channel wiring
2. Test cameras individually
3. Verify camera power connections
4. Try swapping cameras
5. Check camera I2C addresses

**Problem**: Intermittent I2C detection

**Solutions**:
1. Use shorter wires (<30cm)
2. Check for loose connections
3. Add pull-up resistors
4. Reduce I2C speed
5. Separate I2C wires from power lines

---

### After Day 4: Sensor Integration

**Problem**: No sensor data received

**Solutions**:
1. Check TX/RX cross-connection
2. Verify baud rate matches (115200)
3. Check common ground
4. Test sensors individually
5. Verify Serial1 initialized

**Problem**: Garbled serial data

**Solutions**:
1. Match baud rates on both ends
2. Check TX/RX connection (not TX-TX)
3. Verify ground connection
4. Try lower baud rate (57600)
5. Check for electrical noise

**Problem**: Sensor values are 0 or NaN

**Solutions**:
1. Verify BHY2 library installed
2. Reset sensor board
3. Check I2C connection
4. Update sensor firmware
5. Wait for sensor stabilization (30s)

---

### After Day 5: LoRaWAN

**Problem**: "Join timeout" error

**Solutions**:
1. Verify antenna connected
2. Check frequency region (US915/EU868)
3. Verify TTN credentials (AppEUI, AppKey)
4. Ensure device activated on TTN
5. Move closer to gateway

**Problem**: No uplinks on TTN Console

**Solutions**:
1. Check device status on TTN
2. Verify antenna connected
3. Check TX power setting
4. Try reset and rejoin
5. Check TTN gateway status

**Problem**: Weak signal (RSSI < -100 dBm)

**Solutions**:
1. Check antenna connection
2. Reposition antenna
3. Move closer to gateway
4. Increase TX power
5. Use outdoor antenna

---

### After Day 6: Camera Detection

**Problem**: Model won't load

**Solutions**:
1. Verify model file format (.tflite)
2. Check available memory
3. Reduce model size
4. Update TensorFlow Lite library
5. Re-convert model for microcontrollers

**Problem**: Detection is slow (>5 seconds)

**Solutions**:
1. Reduce image resolution
2. Use smaller model
3. Optimize model quantization
4. Check processor speed
5. Disable unnecessary features

**Problem**: Low detection accuracy

**Solutions**:
1. Improve lighting conditions
2. Adjust detection threshold
3. Train model with better data
4. Use higher resolution images
5. Calibrate camera

---

### After Day 7: Backend and Dashboard

**Problem**: Server won't start

**Solutions**:
1. Check Node.js version (18+)
2. Install missing dependencies
3. Check port 3000 availability
4. Review error logs
5. Verify database permissions

**Problem**: Dashboard is blank

**Solutions**:
1. Check browser console for errors
2. Verify backend is running
3. Check WebSocket connection
4. Clear browser cache
5. Try different browser

**Problem**: No real-time updates

**Solutions**:
1. Check WebSocket connection (port 8080)
2. Verify firewall settings
3. Check data pipeline running
4. Review browser console
5. Restart backend server

---

### After Day 8: Alarm System

**Problem**: LEDs don't illuminate

**Solutions**:
1. Check LED polarity (long leg = +)
2. Verify resistor value (220Ω)
3. Test LED individually
4. Check pin number in code
5. Verify voltage at LED

**Problem**: Buzzer doesn't sound

**Solutions**:
1. Check buzzer polarity
2. Verify buzzer type (active vs passive)
3. Test buzzer with 5V directly
4. Check pin number
5. Verify tone() function

**Problem**: Email not received

**Solutions**:
1. Check email credentials
2. Use app password (not regular password)
3. Verify SMTP settings
4. Check spam folder
5. Test email sending manually

---

### After Day 9: WiFi and OTA

**Problem**: WiFi won't connect

**Solutions**:
1. Check SSID and password
2. Verify 2.4GHz network (not 5GHz)
3. Check WiFi antenna
4. Move closer to router
5. Restart WiFi module

**Problem**: OTA update fails

**Solutions**:
1. Check board on same network
2. Verify firewall settings
3. Increase OTA timeout
4. Check available memory
5. Use smaller firmware size

**Problem**: Board won't restart after OTA

**Solutions**:
1. Verify upload completed successfully
2. Check for bootloader issues
3. Implement rollback mechanism
4. Reset board manually
5. Re-upload via USB

---

### After Day 10: Integration Testing

**Problem**: System unstable

**Solutions**:
1. Review system logs
2. Check for memory leaks
3. Reduce data rate
4. Optimize code
5. Add watchdog timer

**Problem**: Low detection accuracy

**Solutions**:
1. Recalibrate sensors
2. Retrain ML model
3. Improve lighting
4. Adjust thresholds
5. Update firmware

**Problem**: High power consumption

**Solutions**:
1. Implement sleep modes
2. Reduce duty cycle
3. Optimize transmission interval
4. Disable unused features
5. Check for unnecessary polling

---

## Photo and Diagram Placeholders

### Figure 1: Complete System Overview
```
[PHOTO: Assembled IoT multi-sensor system]
Caption: Complete system showing all four Arduino boards, I2C multiplexer,
power distribution, and alarm indicators mounted on breadboard.
File: docs/images/complete-system.jpg
```

### Figure 2: Power Distribution Wiring
```
[PHOTO: Close-up of power distribution]
Caption: 5V power supply connected to breadboard rails with 1.5A fuse
installed. All VIN pins connected to positive rail.
File: docs/images/power-distribution.jpg
```

### Figure 3: I2C Multiplexer Connection
```
[PHOTO: TCA9548A wiring]
Caption: TCA9548A I2C multiplexer connected to MKR WAN 1310 SDA/SCL pins.
Address pins A0-A2 grounded for address 0x70.
File: docs/images/i2c-multiplexer.jpg
```

### Figure 4: Dual Camera Setup
```
[PHOTO: Both Nicla Vision boards]
Caption: Two Nicla Vision boards connected to TCA9548A channels 0 and 1.
Both cameras accessible via multiplexer without address conflict.
File: docs/images/dual-cameras.jpg
```

### Figure 5: Complete Wiring Diagram
```
[DIAGRAM: Full system schematic]
Caption: Complete wiring diagram showing all connections between MKR WAN 1310,
TCA9548A, both Nicla Vision boards, Nicla Sense Me, LEDs, and buzzer.
File: docs/images/wiring-diagram.png
```

### Figure 6: Enclosure Layout
```
[PHOTO: System in enclosure]
Caption: System mounted in IP65 enclosure with antennas external,
LEDs visible through transparent window, and USB ports accessible.
File: docs/images/enclosure-layout.jpg
```

### Figure 7: Dashboard Interface
```
[SCREENSHOT: Web dashboard]
Caption: Real-time dashboard showing sensor readings, detection feed,
24-hour trend charts, and system status indicators.
File: docs/images/dashboard-screenshot.png
```

### Figure 8: Object Detection Output
```
[PHOTO: Camera detection in action]
Caption: Nicla Vision camera detecting person with bounding box and
confidence score displayed.
File: docs/images/object-detection.jpg
```

### Figure 9: LoRaWAN Antenna Installation
```
[PHOTO: Antenna connections]
Caption: LoRaWAN antenna connected to MHF connector (left) and WiFi antenna
connected to U.FL connector (right).
File: docs/images/antenna-installation.jpg
```

### Figure 10: TTN Console
```
[SCREENSHOT: The Things Network]
Caption: TTN console showing device status, live data feed, RSSI/SNR
metrics, and uplink/downlink packets.
File: docs/images/ttn-console.png
```

---

## Additional Resources

### Official Documentation
- [Arduino MKR WAN 1310](https://docs.arduino.cc/hardware/mkr-wan-1310)
- [Arduino Nicla Vision](https://docs.arduino.cc/hardware/nicla-vision)
- [Arduino Nicla Sense Me](https://docs.arduino.cc/hardware/nicla-sense-me)
- [The Things Network](https://www.thethingsnetwork.org/docs)
- [TensorFlow Lite Micro](https://www.tensorflow.org/lite/microcontrollers)

### Video Tutorials
- Arduino IoT Projects: [Arduino YouTube Channel](https://www.youtube.com/arduino)
- LoRaWAN Tutorial: [The Things Network YouTube](https://www.youtube.com/@thethingsnetwork)
- TinyML: [Edge Impulse](https://www.edgeimpulse.com/blog)

### Community Forums
- Arduino Forum: https://forum.arduino.cc/
- TTN Community: https://community.thethingsnetwork.org/
- Stack Overflow: https://stackoverflow.com/questions/tagged/arduino

---

## Version History

| Version | Date | Changes |
|---------|------|---------|
| 1.0.0 | 2025-01-06 | Initial comprehensive 10-day guide |

---

## Appendix A: Quick Reference Commands

### Arduino IDE Serial Monitor
```bash
# Open Serial Monitor
Ctrl+Shift+M

# Set baud rate
115200

# Copy Serial output
Ctrl+C

# Clear output
Click "Clear output" button
```

### Backend Server Commands
```bash
# Install dependencies
npm install

# Start backend server
node data_pipeline.js

# Start with Kepware integration
KEPWARE_ENABLED=true node data_pipeline.js

# Run tests
npm test

# Check logs
tail -f logs/system.log
```

### Git Commands
```bash
# Clone repository
git clone <repository-url>

# Check status
git status

# Commit changes
git add .
git commit -m "Description"

# Push to remote
git push origin main
```

---

## Appendix B: Pin Reference Cards

### MKR WAN 1310 Quick Reference
```
Power:
  VIN  - 5V input
  5V   - 5V output
  3.3V - 3.3V output
  GND  - Ground

I2C:
  Pin 11 (SDA)
  Pin 12 (SCL)

UART Serial1:
  Pin 14 (TX)
  Pin 15 (RX)

GPIO:
  Pin 2-7 (digital I/O)
  Pin 5-7 (PWM capable)

SPI (reserved for LoRa):
  Pin 8-11
```

### Nicla Vision Quick Reference
```
Power:
  VIN  - 3.7-5V input
  3.3V - 3.3V output
  GND  - Ground

I2C:
  Pin 0 (SDA)
  Pin 1 (SCL)

UART Serial1:
  Pin 14 (TX)
  Pin 15 (RX)

GPIO:
  Pin 2-12 (digital I/O)
  Pin D4-D7 (analog input)

Camera:
  OV5640 (5MP)
  MIPI CSI-2 interface
```

### Nicla Sense Me Quick Reference
```
Power:
  VIN  - 3.7-5V input
  3.3V - 3.3V output
  GND  - Ground

I2C:
  Pin 0 (SDA)
  Pin 1 (SCL)

UART Serial1:
  Pin 14 (TX)
  Pin 15 (RX)

GPIO:
  Pin 2-12 (digital I/O)
  Pin A0-A3 (analog input)

Sensors:
  BME688 (temp, humid, press, gas)
  BMI270 (IMU)
  BMM150 (magnetometer)
```

---

**End of 10-Day Build Guide**

**Last Updated**: 2025-01-06
**Version**: 1.0.0
**Status**: Production Ready

For questions or issues, please refer to the project repository or community forums.
