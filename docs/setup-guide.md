# Complete Setup Guide - IoT Multi-Sensor System

## Table of Contents
1. [Introduction](#introduction)
2. [Hardware Setup](#hardware-setup)
3. [Software Installation](#software-installation)
4. [Firmware Configuration](#firmware-configuration)
5. [Backend & Dashboard Setup](#backend--dashboard-setup)
6. [Testing & Troubleshooting](#testing--troubleshooting)
7. [Deployment](#deployment)

## Introduction

This guide walks you through setting up a complete IoT multi-sensor system with dual camera object detection, environmental sensing, and long-range LoRaWAN communication.

**Estimated Setup Time**: 4-6 hours
**Difficulty**: Intermediate
**Total Cost**: ~$308 USD

## Hardware Setup

### Tools Required
- Soldering iron (optional, for permanent connections)
- Wire strippers
- Multimeter (recommended)
- USB-C cables (for programming)
- Breadboard and jumper wires

### Step 1: Gather Components

Verify you have all components:
- [ ] Arduino MKR WAN 1310
- [ ] 2x Arduino Nicla Vision
- [ ] Arduino Nicla Sense Me
- [ ] TCA9548A I2C Multiplexer
- [ ] 5V 2A Power Supply
- [ ] LoRaWAN Antenna (868MHz or 915MHz)
- [ ] WiFi Antenna (U.FL connector)
- [ ] Jumper wires (M-M)
- [ ] Breadboard
- [ ] LEDs and buzzer (optional)

### Step 2: Power Distribution Wiring

**WARNING**: Always connect power last to prevent accidental shorts.

1. Connect all VIN pins to 5V:
   - MKR WAN 1310 VIN → 5V+
   - Nicla Vision #1 VIN → 5V+
   - Nicla Vision #2 VIN → 5V+
   - Nicla Sense Me VIN → 5V+
   - TCA9548A VCC → 5V+

2. Connect all GND pins to common ground:
   - All GND pins → GND

3. **IMPORTANT**: Verify connections with multimeter before powering on

### Step 3: I2C Bus Wiring

Connect the I2C multiplexer to MKR WAN 1310:

```
MKR WAN 1310          TCA9548A
Pin 11 (SDA)    →     SDA
Pin 12 (SCL)    →     SCL
GND             →     A0
GND             →     A1
GND             →     A2
```

### Step 4: Camera I2C Connections

Connect cameras to multiplexer channels:

```
TCA9548A           Nicla Vision #1
SC0               →    SDA
SD0               →    SCL
GND               →    GND
5V                →    VIN

TCA9548A           Nicla Vision #2
SC1               →    SDA
SD1               →    SCL
GND               →    GND
5V                →    VIN
```

### Step 5: Sensor Serial Connection

Connect Nicla Sense Me to MKR WAN:

```
MKR WAN 1310       Nicla Sense Me
TX1 (Pin 14)  →    RX
RX1 (Pin 13)  →    TX
GND           →    GND
5V            →    VIN
```

### Step 6: Antenna Connections

- LoRaWAN antenna → MHF connector (marked "LoRa")
- WiFi antenna → U.FL connector (marked "WiFi")

### Step 7: LED/Buzzer (Optional)

Connect alarm indicators:

```
MKR WAN 1310
Pin 5 → LED (220Ω resistor) → GND
Pin 6 → Buzzer → GND
```

## Software Installation

### Step 1: Install Arduino IDE

1. Download Arduino IDE 2.0+ from [arduino.cc](https://www.arduino.cc/en/software)
2. Install for your operating system
3. Launch Arduino IDE

### Step 2: Install Board Definitions

1. Open Arduino IDE
2. Go to **Tools** → **Board** → **Boards Manager**
3. Search and install:
   - "Arduino SAMD Boards" (for MKR WAN 1310)
   - "Arduino mbed OS Boards" (for Nicla Vision/Sense)

### Step 3: Install Libraries

Install these libraries via **Tools** → **Manage Libraries**:

**Required Libraries:**
- Arduino_OV5640 (Nicla Vision camera)
- Arduino_BHY2 (Nicla Sense sensors)
- Arduino_LoRaWAN_Network (MKR WAN LoRaWAN)
- WiFiNINA (MKR WAN WiFi)
- TensorFlowLite for Microcontrollers (ML)
- Wire (I2C)

**Installation Steps:**
1. Open Library Manager
2. Search for each library name
3. Click "Install"
4. Accept dependencies

### Step 4: Install Node.js (for Dashboard)

1. Download Node.js 18+ from [nodejs.org](https://nodejs.org/)
2. Install using default settings
3. Verify installation:
   ```bash
   node --version  # Should show v18.x.x or higher
   npm --version   # Should show 9.x.x or higher
   ```

## Firmware Configuration

### Step 1: Configure LoRaWAN

1. **Register Device on The Things Network (TTN):**
   - Go to [console.thethingsnetwork.org](https://console.thethingsnetwork.org/)
   - Create account (if needed)
   - Create new application
   - Add new device (end device)
   - Copy credentials:
     - AppEUI
     - AppKey
     - DevEUI

2. **Update Firmware:**
   Open `src/firmware/mkr_wan_gateway.ino` and update:

   ```cpp
   const char *appEui = "YOUR_APP_EUI";      // From TTN
   const char *appKey = "YOUR_APP_KEY";      // From TTN
   const char *devEui = "YOUR_DEV_EUI";      // From TTN
   ```

### Step 2: Configure WiFi

Update WiFi credentials in firmware:

```cpp
const char *ssid = "YOUR_WIFI_SSID";
const char *pass = "YOUR_WIFI_PASSWORD";
```

### Step 3: Set Frequency Region

In `src/lorawan/lorawan_implementation.h`, uncomment your region:

```cpp
#define REGION US915     // USA
// #define REGION EU868  // Europe
// #define REGION AS923  // Asia
```

### Step 4: Upload Firmware

**Upload to MKR WAN 1310:**
1. Connect USB-C cable
2. Select board: **Tools** → **Board** → **Arduino MKR WAN 1310**
3. Select port: **Tools** → **Port** → (your board)
4. Open `src/firmware/mkr_wan_gateway.ino`
5. Click Upload button

**Upload to Nicla Vision #1:**
1. Connect to camera board
2. Select board: **Tools** → **Board** → **Arduino Nicla Vision**
3. Open `src/firmware/nicla_vision_camera.ino`
4. Upload

**Upload to Nicla Vision #2:**
1. Connect to second camera
2. Same process as camera #1

**Upload to Nicla Sense Me:**
1. Connect to sensor board
2. Select board: **Tools** → **Board** → **Arduino Nicla Sense Me**
3. Open `src/firmware/nicla_sense_sensors.ino`
4. Upload

## Backend & Dashboard Setup

### Step 1: Install Dependencies

```bash
cd src/backend
npm install express body-parser sqlite3 ws
```

### Step 2: Update Dashboard Configuration

In `src/dashboard/dashboard.js`, verify URLs:

```javascript
const WS_URL = 'ws://localhost:8080';
const API_URL = 'http://localhost:3000/api';
```

### Step 3: Start Backend Server

```bash
node data_pipeline.js
```

Expected output:
```
Connected to SQLite database
Database tables initialized
IoT Data Pipeline server running on port 3000
WebSocket server running on port 8080
Dashboard: http://localhost:3000
```

### Step 4: Open Dashboard

1. Open web browser
2. Navigate to `http://localhost:3000`
3. Should see dashboard with "Connecting..." status

## Testing & Troubleshooting

### Test 1: Hardware Power

**Check:** All boards power on

```bash
Expected:
- LEDs on boards illuminate
- No unusual heat
- Current draw ~50mA at idle

Troubleshooting:
- Check power supply connections
- Verify 5V output with multimeter
- Check for shorts (VIN to GND)
```

### Test 2: I2C Communication

**Check:** I2C scanner detects devices

```cpp
// Run I2C scanner on MKR WAN
#include <Wire.h>

void setup() {
  Serial.begin(115200);
  Wire.begin();

  for (uint8_t addr = 1; addr < 127; addr++) {
    Wire.beginTransmission(addr);
    if (Wire.endTransmission() == 0) {
      Serial.print("Found: 0x");
      Serial.println(addr, HEX);
    }
  }
}

void loop() {}
```

**Expected:**
- 0x70 (TCA9548A)

### Test 3: Sensor Reading

**Check:** Nicla Sense Me transmits data

**Expected Serial Output:**
```
Sensor readings:
  Temperature: 23.5 C
  Humidity: 65.0 %
  Pressure: 1013.2 hPa
  Gas Resistance: 45000 Ohm
```

### Test 4: Camera Capture

**Check:** Camera captures images

**Expected Serial Output:**
```
Camera: OV5640
Resolution: 320x240
Camera initialized successfully
Starting image capture...
Image captured successfully
```

### Test 5: LoRaWAN Connection

**Check:** Device joins network

**Expected Serial Output:**
```
Connecting to LoRaWAN.....
Connected to LoRaWAN
```

**On TTN Console:**
- Device should show as "Active"
- See uplink packets in "Live Data"

### Test 6: Dashboard Connection

**Check:** WebSocket connects

**Expected:**
- Connection status: "Connected" (green)
- Last update shows recent timestamp
- Sensor values populate

## Troubleshooting Guide

### Issue: Camera Not Detected

**Symptoms:**
- I2C scanner doesn't find camera
- "Camera initialization failed" error

**Solutions:**
1. Check TCA9548A wiring (SDA, SCL)
2. Verify multiplexer address (A0-A2 to GND = 0x70)
3. Test camera on direct I2C (bypass multiplexer)
4. Check camera power (5V required)
5. Try slower I2C speed (100kHz instead of 400kHz)

### Issue: LoRaWAN Won't Connect

**Symptoms:**
- "LoRaWAN connection timeout"
- No uplinks on TTN console

**Solutions:**
1. Verify antenna is connected
2. Check frequency region (US915 vs EU868)
3. Verify credentials (AppEUI, AppKey)
4. Check device is activated on TTN
5. Try resetting device and rejoining
6. Move closer to gateway (if using private gateway)

### Issue: Dashboard Not Updating

**Symptoms:**
- "Disconnected" status
- No sensor values

**Solutions:**
1. Check backend server is running
2. Verify WebSocket connection (port 8080)
3. Check browser console for errors
4. Restart backend server
5. Clear browser cache and reload

### Issue: High Power Consumption

**Symptoms:**
- Current >500mA at idle
- Boards getting hot

**Solutions:**
1. Check for short circuits
2. Verify 5V supply (not higher)
3. Disconnect and measure individual board current
4. Enable sleep modes in firmware
5. Reduce transmission frequency

### Issue: Object Detection Not Working

**Symptoms:**
- No detections reported
- Confidence always 0%

**Solutions:**
1. Verify ML model is included
2. Check image capture is working
3. Improve lighting conditions
4. Adjust detection thresholds
5. Train model with better data

## Deployment

### Indoor Deployment

1. **Location:**
   - Near power outlet
   - WiFi coverage area
   - Central monitoring point

2. **Mounting:**
   - Use project case or enclosure
   - Ensure ventilation
   - Secure wiring with cable ties

3. **Power:**
   - Use reliable 5V 2A supply
   - Consider UPS backup
   - Label power cable

### Outdoor Deployment

1. **Weather Protection:**
   - Use IP65-rated enclosure
   - Seal cable entries
   - Add desiccant packets

2. **Power:**
   - Consider solar panel + battery
   - Size battery for 5-7 days autonomy
   - Use power-efficient settings

3. **Antenna Placement:**
   - Mount antennas outside enclosure
   - Use waterproof connectors
   - Position for best LoRaWAN reception

4. **Camera Positioning:**
   - Avoid direct sunlight
   - Use weather-resistant housing
   - Consider sun shield

### Remote Monitoring

1. **Network Setup:**
   - Configure port forwarding (if needed)
   - Use VPN for secure access
   - Set up dynamic DNS

2. **Alerts:**
   - Configure email notifications
   - Set up SMS alerts (critical)
   - Test alarm system

3. **Maintenance:**
   - Schedule monthly inspections
   - Clean camera lenses
   - Check antenna connections
   - Review system logs

## Next Steps

After successful setup:

1. **Calibration:**
   - Calibrate sensors against known references
   - Adjust detection thresholds
   - Test alarm system

2. **Optimization:**
   - Fine-tune transmission intervals
   - Optimize power consumption
   - Update ML model with custom data

3. **Integration:**
   - Connect to cloud services (AWS IoT, Azure)
   - Set up data analytics
   - Create custom alerts

4. **Documentation:**
   - Document your specific setup
   - Save configuration files
   - Create maintenance schedule

## Support Resources

- **Documentation**: See `docs/` folder
- **Wiring Diagram**: `src/hardware/wiring_diagram.md`
- **Component Specs**: `src/hardware/components.md`
- **Troubleshooting**: Each firmware file has troubleshooting section

---

**Last Updated**: January 2025
**Version**: 1.0.0
**Status**: Production Ready
