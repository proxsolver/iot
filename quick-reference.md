# IoT Planning Agent - Quick Reference Guide

## Agent Capabilities at a Glance

This IoT Planning Agent is a specialized assistant for building Arduino-based IoT systems using:
- **Nicla Vision** (dual cameras for object detection)
- **Nicla Sense Me** (environmental sensing)
- **MKR WAN 1310** (LoRaWAN + WiFi communication)

### What This Agent Does

1. **Top-Down Project Planning**
   - Breaks complex IoT systems into manageable phases
   - Creates progressive learning paths
   - Identifies dependencies and prerequisites
   - Prevents overwhelm by focusing on one step at a time

2. **Technical Guidance**
   - Hardware compatibility analysis
   - Communication protocol selection
   - Power management strategies
   - Code structure and architecture

3. **Learning Support**
   - Explains concepts clearly for beginners
   - Provides code examples with detailed comments
   - Offers troubleshooting assistance
   - Suggests learning resources

4. **System Architecture**
   - Designs component integration
   - Creates data flow diagrams
   - Plans network topology
   - Optimizes for performance and reliability

---

## How to Use This Agent

### Starting a New Project

**Tell the agent**:
1. Your IoT project idea/vision
2. Available hardware or budget
3. Your skill level (beginner/intermediate/advanced)
4. Timeline requirements
5. Any specific goals or constraints

**Example prompt**:
```
I want to build a security system with 2 cameras for monitoring my property.
I have Nicla Vision, Nicla Sense Me, and MKR WAN 1310 boards.
I'm a beginner with basic C++ knowledge.
I need this working in 8 weeks.
```

### During Development

**Ask about specific challenges**:
- "How do I connect Nicla Vision to MKR WAN via Serial?"
- "My LoRa messages aren't reaching the gateway. Help?"
- "How can I optimize battery life?"
- "Explain JSON message formatting for sensor data"

### When Stuck

**Provide context**:
1. What you're trying to do
2. What you've already tried
3. Error messages or symptoms
4. Code snippet (if applicable)

**Example**:
```
I'm trying to read temperature from Nicla Sense Me but getting 0°C.
I'm using the Arduino_BHY2 library.
My code is: [paste code]
Error: No errors in Serial Monitor, just shows 0.00
```

---

## Project Structure Overview

### Complete System (10-week plan)

```
Multi-Sensor IoT Monitoring System
├─ Phase 1: Foundation (Week 1-2)
│  ├─ Environment setup
│  ├─ Individual board testing
│  └─ Basic sensor/vision/LoRa functionality
│
├─ Phase 2: Communication (Week 3-4)
│  ├─ Physical board connections
│  ├─ Serial communication protocol
│  └─ Data aggregation on MKR WAN
│
├─ Phase 3: LoRaWAN Integration (Week 5-6)
│  ├─ The Things Network setup
│  ├─ Long-range communication
│  └─ Cloud dashboard
│
├─ Phase 4: WiFi & OTA (Week 7)
│  ├─ WiFi connectivity
│  └─ Remote firmware updates
│
├─ Phase 5: Advanced Features (Week 8)
│  ├─ ML-based object detection
│  └─ Zone-based monitoring
│
└─ Phase 6: Testing & Optimization (Week 9-10)
   ├─ Comprehensive testing
   ├─ Performance optimization
   └─ Documentation
```

---

## Quick Start Commands

### For Complete Beginners
```
"Start from Phase 1. Guide me through setting up Arduino IDE for Nicla Vision, Nicla Sense Me, and MKR WAN 1310."
```

### For Specific Tasks
```
"Show me the wiring diagram for connecting Nicla Vision to MKR WAN 1310"
"Explain LoRaWAN and how to configure it for EU868 frequency"
"Help me implement object detection on Nicla Vision using EdgeML"
"How do I send JSON data from Nicla Sense Me via Serial?"
```

### For Troubleshooting
```
"My Nicla Vision camera won't initialize. Help me debug."
"LoRa join fails with error code. What does this mean?"
"How can I reduce power consumption to extend battery life?"
"OTA update failed. How do I recover my board?"
```

### For Architecture Decisions
```
"Compare Serial vs I2C for board-to-board communication"
"What's the optimal LoRa spreading factor for 2km range?"
"Should I use WiFi or LoRa for my sensor data transmission?"
"How do I design a system that works without internet access?"
```

---

## Key Concepts Reference

### Communication Protocols

**Serial (UART)**
- Use: Board-to-board communication
- Speed: 115200 baud (standard)
- Pros: Simple, reliable, point-to-point
- Cons: One device per port (unless using multiplexing)

**LoRaWAN**
- Use: Long-range data transmission (up to 10km)
- Speed: Very slow (0.3-50 kbps)
- Pros: Long range, low power, penetration
- Cons: Limited bandwidth, requires gateway
- Best for: Periodic sensor data, alerts

**WiFi**
- Use: High-bandwidth communication, OTA updates
- Speed: Fast (Mbps range)
- Pros: High bandwidth, infrastructure available
- Cons: Short range, high power consumption
- Best for: Firmware updates, configuration

### Power Management

**Active Mode**
- All sensors and processing running
- Current: 200-500mA depending on board
- Use: When actively monitoring/detecting

**Standby Mode**
- Main processor sleeping, sensors on
- Current: 5-50mA
- Use: Waiting for events, quick wake

**Deep Sleep Mode**
- Everything powered down except RTC
- Current: 0.1-5mA
- Use: Between periodic readings

**Optimization Strategies**:
- Reduce frame rate (vision)
- Increase sleep intervals
- Lower transmission power
- Use motion triggers instead of continuous operation

### Data Formats

**JSON (Human-readable)**
```json
{
  "source": "vision_1",
  "timestamp": 1234567890,
  "detection": {
    "class": "person",
    "confidence": 0.92,
    "bbox": [100, 150, 200, 300]
  }
}
```
- Use: Debugging, flexible data structures
- Size: Larger (100-500 bytes)
- Pros: Readable, flexible, well-supported
- Cons: High overhead, slower parsing

**CayenneLPP (Binary, efficient)**
```
[0x01, 0x67, 0x00, 0x4B, 0x00]  // Temperature 23.5°C
[0x02, 0x68, 0x01, 0x90]        // Humidity 65.2%
```
- Use: LoRaWAN transmission
- Size: Small (5-20 bytes typical)
- Pros: Efficient, standardized
- Cons: Not human-readable, limited types

---

## Component Pin Reference

### Nicla Vision
```
Pin    Function
───────┼────────────────────────────────
D0     GPIO / Serial1 RX
D1     GPIO / Serial1 TX
D2     GPIO
D3     GPIO / I2C SDA
D4     GPIO / I2C SCL
D5     GPIO / SPI MOSI
D6     GPIO / SPI MISO
D7     GPIO / SPI SCK
D8     GPIO / SPI CS
D9     GPIO
D10    GPIO
D11    GPIO
D12    GPIO
A0     Analog Input
A1     Analog Input
A2     Analog Input
A3     Analog Input
A4     Analog Input
A5     Analog Input

LEDs   LEDR, LEDG, LEDB (RGB LED - active LOW)
```

### Nicla Sense Me
```
Pin    Function
───────┼────────────────────────────────
D0     GPIO / Serial1 RX
D1     GPIO / Serial1 TX
D2     GPIO
D3     GPIO / I2C SDA
D4     GPIO / I2C SCL
D5     GPIO
D6     GPIO
D7     GPIO
D8     GPIO
D9     GPIO
D10    GPIO
D11    GPIO
D12    GPIO

LED    Built-in RGB LED
Battery JST connector for Li-Po
```

### MKR WAN 1310
```
Pin    Function
───────┼────────────────────────────────
0      UART1 RX (connect to Vision TX)
1      UART1 TX (connect to Vision RX)
2      I2C SDA
3      I2C SCL
4      Not available
5      Not available
6      Not available
7      Not available
8      UART2 TX (connect to Sense Me RX)
9      UART2 RX (connect to Sense Me TX)
10     Not available
11     Not available
12     Not available
13     Not available
14     A0
15     A1
16     A2
17     A3
18     A4
19     A5
A0     Analog Input
A1     Analog Input
A2     Analog Input
A3     Analog Input
A4     Analog Input
A5     Analog Input
A6     Analog Input (battery voltage)

Serial USB  (debug output to computer)
Serial1     pins 13 (RX), 14 (TX)
Serial2     pins 0 (RX), 1 (TX)
```

---

## Common Wiring Connections

### Nicla Vision → MKR WAN 1310
```
Nicla Vision    →    MKR WAN 1310
─────────────────────────────────
D1 (TX)         →    0 (RX1)
D0 (RX)         →    1 (TX1)
GND             →    GND
VIN             →    5V (if powering from MKR WAN)
```

### Nicla Sense Me → MKR WAN 1310
```
Nicla Sense Me  →    MKR WAN 1310
─────────────────────────────────
D1 (TX)         →    9 (RX2)
D0 (RX)         →    8 (TX2)
GND             →    GND
```

### Power Distribution
```
Option 1: Individual USB power (development)
- Each board powered via USB-C cable

Option 2: Shared Li-Po battery (production)
- Large battery (2000-5000mAh) → 5V boost converter → all boards VIN
- Ensure common ground

Option 3: Solar charging (autonomous)
- Solar panel → charge controller → battery → 5V boost → boards
```

---

## Code Snippets Quick Reference

### Read Temperature (Nicla Sense Me)
```cpp
#include <Arduino_BHY2.h>

SensorTemperature temperature(SENSOR_ID_TEMP);

void setup() {
  Serial.begin(115200);
  BHY2.begin();
  temperature.begin();
}

void loop() {
  BHY2.update();
  Serial.print("Temperature: ");
  Serial.print(temperature.value());
  Serial.println(" °C");
  delay(5000);
}
```

### Send JSON via Serial
```cpp
#include <Arduino_JSON.h>

void sendDetection(String objectClass, float confidence) {
  JSONVar data;
  data["source"] = "vision_1";
  data["timestamp"] = millis();
  data["class"] = objectClass;
  data["confidence"] = confidence;

  Serial.println(JSON.stringify(data));
}

// Usage:
sendDetection("person", 0.92);
```

### Receive JSON on MKR WAN
```cpp
#include <Arduino_JSON.h>

void loop() {
  if (Serial1.available()) {
    String incoming = Serial1.readStringUntil('\n');
    JSONVar data = JSON.parse(incoming);

    if (JSON.typeof(data) == "undefined") {
      Serial.println("Parse error!");
      return;
    }

    String source = (const char*)data["source"];
    String objectClass = (const char*)data["class"];
    float confidence = (double)data["confidence"];

    Serial.print("Received from ");
    Serial.print(source);
    Serial.print(": ");
    Serial.print(objectClass);
    Serial.print(" (");
    Serial.print(confidence);
    Serial.println(")");
  }
}
```

### LoRa Transmission
```cpp
#include <MKRWAN.h>

LoRaModem modem;

void setup() {
  modem.begin(EU868);  // or US915
  modem.joinOTAA(appEui, appKey);
}

void sendLoRa(String message) {
  modem.beginPacket();
  modem.print(message);
  modem.endPacket();
}
```

---

## Troubleshooting Checklist

### Board Not Detected
- [ ] USB cable is data cable (not charge-only)
- [ ] Try different USB port
- [ ] Press reset button twice (enter bootloader mode)
- [ ] Install/update drivers (Windows: Zadig utility)
- [ ] Check device manager (Windows) or lsusb (Linux)

### Camera Initialization Failed
- [ ] Sufficient power (use powered USB hub)
- [ ] Camera ribbon cable properly seated
- [ ] Lower frame rate or resolution
- [ ] Reset board before retrying
- [ ] Check if camera is being used by another application

### LoRa Join Failing
- [ ] Antenna connected (REQUIRED!)
- [ ] AppEUI and AppKey correct (check TTN console)
- [ ] Frequency band matches region (EU868 vs US915)
- [ ] Gateway within range
- [ ] Try ABP activation if OTAA fails

### Serial Communication Issues
- [ ] TX connected to RX and vice versa
- [ ] Common ground connected
- [ ] Baud rate matches on both ends (115200)
- [ ] No other Serial devices interfering
- [ ] Check with logic analyzer or oscilloscope if available

### High Power Consumption
- [ ] Reduce frame rate on vision boards
- [ ] Increase sleep intervals
- [ ] Lower transmission power (LoRa)
- [ ] Disable unused peripherals
- [ ] Use sleep instead of delay()

### WiFi OTA Failures
- [ ] Stable WiFi connection
- [ ] Sufficient flash space for new firmware
- [ ] Verify firmware checksum
- [ ] Don't update during critical operations
- [ ] Implement rollback mechanism

---

## Learning Path Recommendations

### Absolute Beginner (No Arduino Experience)
**Start here**: Arduino built-in examples
1. File → Examples → 01.Basics → Blink
2. File → Examples → 01.Basics → AnalogReadSerial
3. File → Examples → 04.Communication → SerialEvent
4. Then proceed to Phase 1

**Time investment**: 2-3 days for basics

### Some Programming Experience (C/C++, Python, etc.)
**Start here**: Direct to Phase 1
- Install Arduino IDE
- Follow Phase 1 tasks sequentially
- Reference Arduino Language Guide as needed

**Time investment**: 1-2 weeks for Phase 1

### Experienced with Arduino
**Start here**: Phase 2 or 3
- Review board-specific libraries
- Skip basic setup, focus on integration
- Customize phases to your needs

**Time investment**: 1-2 weeks for integration phases

---

## Resources Reference

### Official Documentation
- [Arduino Nicla Vision](https://docs.arduino.cc/hardware/nicla-vision)
- [Arduino Nicla Sense Me](https://docs.arduino.cc/hardware/nicla-sense-me)
- [Arduino MKR WAN 1310](https://docs.arduino.cc/hardware/mkr-wan-1310)
- [The Things Network](https://www.thethingsnetwork.org/docs/)

### Libraries
- `Arduino_BHY2` - Nicla Sense Me sensors
- `Arduino_OV5640` - Nicla Vision camera
- `MKRWAN` - LoRaWAN communication
- `WiFiNINA` - WiFi connectivity
- `Arduino_JSON` - JSON parsing
- `Arduino_LowPower` - Power management

### Community
- [Arduino Forum](https://forum.arduino.cc/)
- [The Things Network Community](https://community.thethingsnetwork.org/)
- [Stack Overflow - Arduino](https://stackoverflow.com/questions/tagged/arduino)
- [Arduino Project Hub](https://create.arduino.cc/projecthub)

### Tools
- Arduino IDE 2.0: [arduino.cc](https://www.arduino.cc/en/software)
- MQTT Explorer: [mqtt-explorer.com](https://mqtt-explorer.com/)
- JSON Validator: [jsonlint.com](https://jsonlint.com/)
- TTN Console: [console.thethingsnetwork.org](https://console.thethingsnetwork.org/)

---

## Getting Help

When asking for help, provide:

1. **Context**
   - What phase/task are you working on?
   - What hardware are you using?
   - What's your goal?

2. **What You've Tried**
   - Code you've written
   - Steps you've taken
   - Documentation you've referenced

3. **Problem Details**
   - Error messages (exact text)
   - Unexpected behavior
   - Expected vs actual results

4. **Environment**
   - Arduino IDE version
   - Operating system
   - Library versions

### Example Good Question:
```
I'm in Phase 2, trying to connect Nicla Vision to MKR WAN 1310 via Serial.
Wired: Vision D1(TX) → MKR 0(RX), Vision D0(RX) → MKR 1(TX), GND→GND
Using Arduino IDE 2.1.0 on macOS.

Vision sends: {"source":"vision_1","class":"person","confidence":0.92}
MKR receives: Nothing in Serial Monitor

Code (simplified):
void loop() {
  if (Serial1.available()) {
    Serial.println("Data available!");
    String data = Serial1.readStringUntil('\n');
    Serial.println(data);
  }
}

Expected: Print JSON to Serial Monitor
Actual: "Data available!" never prints
```

---

## Project Customization

This agent can help adapt the project for:
- Different hardware (Portenta, other Arduinos)
- Different sensors (gas, air quality, UV)
- Different communication (BLE, WiFi-only, cellular)
- Different use cases (agriculture, industrial, home)
- Different scale (1 board to 100+ boards)

Just describe your requirements and the agent will create a customized plan!
