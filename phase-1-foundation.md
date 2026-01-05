# Phase 1: Foundation and Single-Board Basics

## Phase Overview

**Duration**: 2 weeks (14 days)
**Difficulty Level**: Beginner
**Prerequisites**: Basic C/C++ knowledge, Arduino IDE installed

**Phase Goal**: Establish development environment, understand individual board capabilities, and implement basic functionality on each board separately.

## Learning Objectives

By the end of this phase, you will be able to:
- Set up Arduino IDE for Nicla Vision, Nicla Sense Me, and MKR WAN 1310
- Write and upload firmware to each board
- Read sensor data from Nicla Sense Me
- Capture images and run basic vision processing on Nicla Vision
- Configure LoRa communication on MKR WAN 1310
- Debug code using Serial Monitor
- Implement basic power management

---

## Week 1: Environment Setup and Nicla Sense Me

### Day 1-2: Arduino IDE Setup and Board Configuration

#### Task 1.1: Install Arduino IDE 2.0
**Steps**:
1. Download Arduino IDE 2.0 from [arduino.cc](https://www.arduino.cc/en/software)
2. Install for your operating system (Windows/Mac/Linux)
3. Launch Arduino IDE and verify installation

**Success Criteria**:
- [ ] Arduino IDE opens without errors
- [ ] Version 2.0+ confirmed in Help → About

#### Task 1.2: Add Board Support Packages

**For Nicla Vision**:
```
Tools → Board → Boards Manager → Search "Arduino mbed OS Nano Boards" → Install
```

**For Nicla Sense Me**:
```
Tools → Board → Boards Manager → Search "Arduino nRF528x Boards (Mbed OS)" → Install
```

**For MKR WAN 1310**:
```
Tools → Board → Boards Manager → Search "Arduino SAMD Boards" → Install
```

**Success Criteria**:
- [ ] All three board packages installed
- [ ] Board selection menu shows all three boards

#### Task 1.3: Install Essential Libraries

Open Library Manager (Sketch → Include Library → Manage Libraries) and install:

**For All Boards**:
- `Arduino_JSON` by Benoit Blanchon (latest version)
- `Arduino_LowPower` by Arduino (latest version)

**For Nicla Vision**:
- `Arduino_AdvancedFFT` (for motion detection)
- `TensorFlowLite` (for ML models - install later when needed)

**For Nicla Sense Me**:
- `Arduino_BHY2` by Arduino (for sensors)

**For MKR WAN 1310**:
- `MKRWAN` by Arduino (for LoRaWAN)
- `WiFiNINA` by Arduino (for WiFi)

**Success Criteria**:
- [ ] All libraries installed successfully
- [ ] Library examples accessible in File → Examples

### Day 3-5: Nicla Sense Me - Environmental Sensing

#### Task 1.4: Your First Sketch - Temperature Reading

**Hardware Setup**:
- Connect Nicla Sense Me via USB-C cable
- Verify green LED indicates power

**Code**: Create file `nicla_sense_basics.ino`

```cpp
#include <Arduino_BHY2.h>

// Sensor objects
SensorTemperature temperature(SENSOR_ID_TEMP);
SensorHumidity humidity(SENSOR_ID_HUM);
SensorPressure pressure(SENSOR_ID_BARO);

void setup() {
  Serial.begin(115200);

  // Wait for Serial Monitor to open
  while (!Serial);

  Serial.println("Nicla Sense Me - Environmental Sensing");
  Serial.println("========================================");

  // Initialize sensors
  BHY2.begin();
  temperature.begin();
  humidity.begin();
  pressure.begin();

  Serial.println("Sensors initialized!");
  Serial.println();
}

void loop() {
  // Update sensor values
  BHY2.update();

  // Read and display temperature
  float tempC = temperature.value();
  float tempF = (tempC * 9/5) + 32;

  // Read and display humidity
  float hum = humidity.value();

  // Read and display pressure
  float pres = pressure.value();

  // Print results
  Serial.println("────────────────────────────────────────");
  Serial.print("Temperature: ");
  Serial.print(tempC);
  Serial.print(" °C (");
  Serial.print(tempF);
  Serial.println(" °F)");

  Serial.print("Humidity: ");
  Serial.print(hum);
  Serial.println(" %");

  Serial.print("Pressure: ");
  Serial.print(pres);
  Serial.println(" hPa");
  Serial.println("────────────────────────────────────────");
  Serial.println();

  // Wait 5 seconds between readings
  delay(5000);
}
```

**Success Criteria**:
- [ ] Code compiles without errors
- [ ] Code uploads to Nicla Sense Me
- [ ] Serial Monitor shows temperature, humidity, pressure
- [ ] Values are reasonable (e.g., temperature 20-30°C, humidity 30-70%)

#### Task 1.5: Understanding Sensor Data

**Exercise**: Monitor sensors for 5 minutes and observe:
1. How much does temperature vary when you hold the board?
2. What happens to humidity when you breathe on the sensor?
3. How does pressure change with altitude (if you have multi-story building)?

**Create a data log** in your notebook:
```
Time (min) | Temp (°C) | Humidity (%) | Pressure (hPa) | Notes
-----------|-----------|--------------|----------------|-------------------
0          | 23.5      | 55.2         | 1013.25        | Room temp
1          | 23.6      | 55.5         | 1013.20        | Stable
2          | 25.1      | 62.0         | 1013.18        | Held in hand
```

**Success Criteria**:
- [ ] Data log completed with 10+ entries
- [ ] Can explain variations in sensor readings
- [ ] Understand sensor refresh rate and response time

#### Task 1.6: Basic JSON Formatting

**Code Enhancement**: Modify previous sketch to output JSON

```cpp
#include <Arduino_BHY2.h>
#include <Arduino_JSON.h>

SensorTemperature temperature(SENSOR_ID_TEMP);
SensorHumidity humidity(SENSOR_ID_HUM);
SensorPressure pressure(SENSOR_ID_BARO);

void setup() {
  Serial.begin(115200);
  while (!Serial);

  BHY2.begin();
  temperature.begin();
  humidity.begin();
  pressure.begin();

  Serial.println("Nicla Sense Me - JSON Output");
}

void loop() {
  BHY2.update();

  // Create JSON object
  JSONVar sensorData;

  sensorData["source"] = "nicla_sense_me";
  sensorData["timestamp"] = millis();

  JSONVar sensors;
  sensors["temperature"] = temperature.value();
  sensors["humidity"] = humidity.value();
  sensors["pressure"] = pressure.value();

  sensorData["sensors"] = sensors;

  // Print JSON
  Serial.println(JSON.stringify(sensorData));

  delay(5000);
}
```

**Success Criteria**:
- [ ] JSON output displays correctly in Serial Monitor
- [ ] JSON validates using online JSON parser
- [ ] Understand JSON structure for future communication

### Day 6-7: Power Management Basics

#### Task 1.7: Implement Low-Power Sleep

**Code**: Create `nicla_sense_sleep.ino`

```cpp
#include <Arduino_BHY2.h>
#include <Arduino_LowPower.h>

SensorTemperature temperature(SENSOR_ID_TEMP);
SensorHumidity humidity(SENSOR_ID_HUM);

const unsigned long UPDATE_INTERVAL = 300000; // 5 minutes in ms
unsigned long lastUpdate = 0;

volatile bool alarmFlag = false;

void alarmCallback() {
  alarmFlag = true;
  // When waking from sleep, we don't do anything here
  // The loop() will handle the sensor reading
}

void setup() {
  Serial.begin(115200);

  // Don't wait for Serial to allow battery operation
  delay(2000);

  BHY2.begin();
  temperature.begin();
  humidity.begin();

  Serial.println("Nicla Sense Me - Low Power Mode");
  Serial.println("Sensors will update every 5 minutes");
  Serial.println("Board will sleep between updates");
  Serial.println();

  // Configure RTC alarm for wake-up
  LowPower.attachInterruptWakeup(RTC_ALARM_WAKEUP, alarmCallback);
}

void loop() {
  // Read sensors
  BHY2.update();

  float temp = temperature.value();
  float hum = humidity.value();

  Serial.println("────────────────────────────────────────");
  Serial.print("Wake time: ");
  Serial.println(millis());
  Serial.print("Temperature: ");
  Serial.print(temp);
  Serial.println(" °C");
  Serial.print("Humidity: ");
  Serial.print(hum);
  Serial.println(" %");
  Serial.println("────────────────────────────────────────");

  // Flush serial before sleeping
  Serial.flush();

  // Sleep for UPDATE_INTERVAL milliseconds
  // Note: Arduino LowPower library uses different API
  // This is a simplified example
  delay(UPDATE_INTERVAL);

  // In production, use:
  // LowPower.sleep(UPDATE_INTERVAL);
}
```

**Success Criteria**:
- [ ] Code implements sleep mode
- [ ] Current consumption decreases in sleep (measure if possible)
- [ ] Board wakes up reliably and takes measurements

---

## Week 2: Nicla Vision and MKR WAN 1310 Basics

### Day 8-10: Nicla Vision - Camera and Object Detection

#### Task 1.8: Camera Setup and Image Capture

**Hardware Setup**:
- Connect Nicla Vision via USB-C cable
- Ensure camera lens is clean

**Code**: Create `nicla_vision_camera.ino`

```cpp
#include "Arduino_OV5640.h"

OV5640 camera;

void setup() {
  Serial.begin(115200);
  while (!Serial);

  Serial.println("Nicla Vision - Camera Test");

  // Initialize camera
  if (!camera.begin(OV5640_COLOR_YUV422, OV5640_RESOLUTION_480x480, OV5640_FIX_30FPS)) {
    Serial.println("Camera init failed!");
    while (1);
  }

  Serial.println("Camera initialized successfully!");
  Serial.println("Resolution: 480x480");
  Serial.println("Format: RGB565");
  Serial.println();
}

void loop() {
  Serial.println("Capturing image...");

  // Capture frame
  if (camera.captureFrame()) {
    Serial.print("Image captured! Frame size: ");
    Serial.print(camera.frameSize());
    Serial.println(" bytes");

    // Note: Can't easily send image over Serial for now
    // In next phase, we'll process it on-board
  } else {
    Serial.println("Capture failed!");
  }

  delay(5000);
}
```

**Success Criteria**:
- [ ] Camera initializes without errors
- [ ] Frame captures successfully
- [ ] Frame size is reasonable (expect ~450KB for 480x480 RGB565)

#### Task 1.9: Simple Motion Detection

**Concept**: Use frame differencing to detect motion

**Code**: Create `nicla_vision_motion.ino` (simplified version)

```cpp
#include "Arduino_OV5640.h"

OV5640 camera;
uint8_t* previousFrame = nullptr;

void setup() {
  Serial.begin(115200);
  while (!Serial);

  if (!camera.begin(OV5640_COLOR_RGB565, OV5640_RESOLUTION_480x480, OV5640_FIX_15FPS)) {
    Serial.println("Camera init failed!");
    while (1);
  }

  Serial.println("Motion Detection System");
  Serial.println("Monitoring for movement...");
}

void loop() {
  if (camera.captureFrame()) {
    uint8_t* currentFrame = camera.getFrame();
    size_t frameSize = camera.frameSize();

    if (previousFrame != nullptr) {
      // Calculate differences between frames
      int diffCount = 0;
      const int threshold = 30; // Pixel difference threshold

      // Sample every 10th pixel for speed
      for (size_t i = 0; i < frameSize; i += 10 * 2) { // *2 for RGB565
        int r1 = (currentFrame[i] & 0xF8);
        int r2 = (previousFrame[i] & 0xF8);

        if (abs(r1 - r2) > threshold) {
          diffCount++;
        }
      }

      // If more than 1% of sampled pixels changed, trigger alarm
      if (diffCount > (frameSize / 10 / 100)) {
        Serial.println("⚠️ MOTION DETECTED!");
        // In next phase: trigger LED alarm
        delay(2000); // Cooldown
      }
    }

    // Save current frame for next comparison
    if (previousFrame != nullptr) {
      free(previousFrame);
    }
    previousFrame = (uint8_t*)malloc(frameSize);
    memcpy(previousFrame, currentFrame, frameSize);
  }

  delay(100);
}
```

**Success Criteria**:
- [ ] Motion detection triggers when you wave hand
- [ ] No false alarms when scene is static
- [ ] Understanding of frame differencing concept

#### Task 1.10: LED Alarm System

**Code**: Enhance with LED feedback

```cpp
// Add to previous sketch

void setup() {
  // ... existing setup ...

  // Initialize RGB LED
  pinMode(LEDR, OUTPUT);
  pinMode(LEDG, OUTPUT);
  pinMode(LEDB, OUTPUT);

  // Turn off LED
  digitalWrite(LEDR, HIGH);
  digitalWrite(LEDG, HIGH);
  digitalWrite(LEDB, HIGH);
}

void triggerAlarm() {
  Serial.println("⚠️ MOTION DETECTED! ALARM ACTIVATED");

  // Flash red LED
  for (int i = 0; i < 5; i++) {
    digitalWrite(LEDR, LOW);
    digitalWrite(LEDG, HIGH);
    digitalWrite(LEDB, HIGH);
    delay(200);

    digitalWrite(LEDR, HIGH);
    delay(200);
  }

  // Return to green (safe)
  digitalWrite(LEDR, HIGH);
  digitalWrite(LEDG, LOW);
  digitalWrite(LEDB, HIGH);
}
```

**Success Criteria**:
- [ ] LED flashes red on motion detection
- [ ] LED shows green when monitoring
- [ ] Alarm timing is appropriate

### Day 11-14: MKR WAN 1310 - LoRa Communication

#### Task 1.11: LoRa Configuration

**Hardware Setup**:
- Connect MKR WAN 1310 via USB-C
- Connect LoRa antenna (SMA connector) - **CRITICAL**

**Code**: Create `mkr_wan_lora_test.ino`

```cpp
#include <MKRWAN.h>

LoRaModem modem;

void setup() {
  Serial.begin(115200);
  while (!Serial);

  Serial.println("MKR WAN 1310 - LoRa Test");
  Serial.println("=========================");

  // Initialize modem
  if (!modem.begin(EU868)) {  // Change to US915 for US
    Serial.println("Failed to start module");
    while (1);
  }

  Serial.print("Your module version is: ");
  Serial.println(modem.version());
  Serial.print("Your device EUI is: ");
  Serial.println(modem.deviceEUI());

  // Configure LoRaWAN parameters
  Serial.println("Setting parameters");

  // Set spread factor (7 = fastest, 12 = longest range)
  modem.dataRate(7);

  // Set TX power
  modem.setPort(10);

  Serial.println("LoRa module configured!");
  Serial.println();
}

void loop() {
  Serial.println("Attempting to join LoRaWAN network...");

  // Join network (OTAA - Over the Air Activation)
  // You need to register on The Things Network first
  if (modem.joinOTAA(/*appEui*/, /*appKey*/)) {
    Serial.println("Successfully joined network!");

    // Send test message
    modem.beginPacket();
    modem.print("Hello from MKR WAN 1310!");
    modem.endPacket();

    Serial.println("Message sent!");
  } else {
    Serial.println("Join failed. Check credentials.");
  }

  delay(60000); // Try again every minute
}
```

**Prerequisites**:
1. Register on [The Things Network](https://www.thethingsnetwork.org/)
2. Create application and device
3. Copy AppEUI and AppKey

**Success Criteria**:
- [ ] LoRa module initializes
- [ ] Device EUI displayed
- [ ] Successfully joins TTN network
- [ ] Message appears in TTN console

#### Task 1.12: Serial Communication Basics

**Concept**: Practice multi-port Serial communication

**Code**: Create `mkr_wan_serial_test.ino`

```cpp
// MKR WAN 1310 has multiple Serial ports:
// Serial - USB (to computer)
// Serial1 - pins 13 (RX) and 14 (TX)

void setup() {
  // Initialize both serial ports
  Serial.begin(115200);
  Serial1.begin(115200);

  while (!Serial && millis() < 5000);  // Wait up to 5 seconds

  Serial.println("MKR WAN 1310 - Serial Test");
  Serial.println("===========================");
  Serial.println("Serial1 configured on pins 13 (RX) and 14 (TX)");
  Serial.println("Listening for incoming data...");
  Serial.println();
}

void loop() {
  // Check if data received on Serial1 (from another board)
  if (Serial1.available()) {
    String incomingData = Serial1.readStringUntil('\n');

    Serial.print("Received: ");
    Serial.println(incomingData);

    // Echo back acknowledgment
    Serial1.println("ACK: " + incomingData);
  }

  // Check for commands from Serial Monitor
  if (Serial.available()) {
    String command = Serial.readStringUntil('\n');

    Serial.print("Sending command: ");
    Serial.println(command);

    // Forward to Serial1
    Serial1.println(command);
  }
}
```

**Success Criteria**:
- [ ] Serial1 initialized successfully
- [ ] Can send/receive data on both ports
- [ ] Understanding of multi-serial architecture

---

## Phase 1 Completion Checklist

### Knowledge Assessment
- [ ] Can explain difference between Serial and Serial1
- [ ] Understand JSON data format
- [ ] Can describe LoRaWAN network architecture
- [ ] Know how to read sensor data
- [ ] Understand basic image processing concepts
- [ ] Can explain power management strategies

### Practical Skills
- [ ] Successfully compiled and uploaded code to all three boards
- [ ] Used Serial Monitor for debugging
- [ ] Read temperature, humidity, and pressure from Nicla Sense Me
- [ ] Captured images with Nicla Vision
- [ ] Implemented motion detection on Nicla Vision
- [ ] Configured LoRaWAN on MKR WAN 1310
- [ ] Sent LoRa message to The Things Network

### Deliverables
- [ ] Working code for all three boards
- [ ] Sensor data log (minimum 20 entries)
- [ ] LoRaWAN device registered on TTN
- [ ] Documentation of challenges and solutions
- [ ] Photos/videos of working system

### Before Moving to Phase 2
**Ensure**:
1. All boards can run independently
2. You understand the capabilities and limitations of each board
3. Serial Monitor debugging is comfortable
4. Basic code structure (setup/loop) is clear
5. You have all necessary cables and power supplies

---

## Common Issues and Solutions

### Issue 1: "Board not found" error
**Solution**:
- Check USB cable (use data cable, not charge-only)
- Try different USB port
- Install drivers if needed (especially on Windows)
- Reset board by pressing reset button twice quickly

### Issue 2: Camera fails to initialize
**Solution**:
- Ensure sufficient power (use powered USB hub if needed)
- Check camera ribbon cable is properly seated
- Try lowering frame rate or resolution
- Reset board before attempting again

### Issue 3: LoRa join fails
**Solution**:
- Verify antenna is connected (required!)
- Check AppEUI and AppKey are correct (copy carefully)
- Ensure frequency band matches region (EU868 vs US915)
- Check TTN application/device configuration
- Try ABP activation if OTAA fails

### Issue 4: Code won't compile
**Solution**:
- Check all libraries are installed
- Verify board is selected correctly in Tools menu
- Update Arduino IDE to latest version
- Check for typos in code (especially bracket matching)
- Try compiling example code first

### Issue 5: Serial Monitor shows garbage characters
**Solution**:
- Verify baud rate matches in code and Serial Monitor
- Check both ends are using same baud rate
- Try resetting board after opening Serial Monitor

---

## Next Phase Preview

**Phase 2**: Multi-Board Communication
- Connect Nicla Vision → MKR WAN via Serial
- Connect Nicla Sense Me → MKR WAN via Serial
- Implement data aggregation on MKR WAN
- Create unified message format
- Test complete data pipeline

**Prerequisites for Phase 2**:
- Complete all Phase 1 tasks
- Comfortable with Serial communication
- Understanding of JSON format
- All boards tested individually

---

## Resources

### Official Documentation
- [Arduino Nicla Vision](https://docs.arduino.cc/hardware/nicla-vision)
- [Arduino Nicla Sense Me](https://docs.arduino.cc/hardware/nicla-sense-me)
- [Arduino MKR WAN 1310](https://docs.arduino.cc/hardware/mkr-wan-1310)
- [The Things Network](https://www.thethingsnetwork.org/docs/)

### Learning Resources
- [Arduino Language Reference](https://www.arduino.cc/reference/en/)
- [C++ Tutorial](https://www.learncpp.com/)
- [JSON Guide](https://www.json.org/json-en.html)
- [LoRaWAN explained](https://www.lorawan.io/)

### Community Forums
- [Arduino Forum](https://forum.arduino.cc/)
- [The Things Network Community](https://community.thethingsnetwork.org/)
- [Stack Overflow - Arduino Tag](https://stackoverflow.com/questions/tagged/arduino)
