# Complete LoRaWAN Implementation Guide

## Table of Contents
1. [Overview](#overview)
2. [Hardware Requirements](#hardware-requirements)
3. [Software Requirements](#software-requirements)
4. [Network Server Setup](#network-server-setup)
5. [Device Registration](#device-registration)
6. [Configuration](#configuration)
7. [Data Packet Formats](#data-packet-formats)
8. [Downlink Commands](#downlink-commands)
9. [Frequency Plans](#frequency-plans)
10. [Troubleshooting](#troubleshooting)
11. [Best Practices](#best-practices)
12. [API Reference](#api-reference)

---

## Overview

This guide provides complete instructions for implementing production-ready LoRaWAN communication on the Arduino MKR WAN 1310 board using the IBM LMIC library.

### Key Features

- **Dual Authentication Modes**: OTAA (recommended) and ABP (fallback)
- **Binary Packet Encoding**: 18-byte optimized sensor packets
- **Duty Cycle Management**: Automatic 1% limit enforcement per ETSI/FCC regulations
- **Adaptive Data Rate (ADR)**: Automatic optimization of transmission parameters
- **Retry Logic**: Exponential backoff for reliable delivery
- **Downlink Handling**: Remote configuration and control
- **Multi-Region Support**: EU868, US915, KR920, AS923, AU915
- **CRC16 Validation**: Packet integrity checking
- **Command Queue**: Non-blocking downlink processing

---

## Hardware Requirements

### Required Components

1. **Arduino MKR WAN 1310**
   - SAMD21 Cortex-M0+ @ 32MHz
   - 256KB Flash, 32KB SRAM
   - Built-in Murata LoRa module (SX1276)

2. **LoRaWAN Antenna**
   - Frequency must match your region:
     - 868 MHz for EU868
     - 915 MHz for US915
     - 920 MHz for KR920
     - 923 MHz for AS923
   - SMA or IPEX connector
   - Gain: 2-5 dBi typical

3. **Power Supply**
   - USB cable for development
   - Li-Po battery (3.7V) for production
   - Solar panel + battery for remote deployment

4. **Sensors (Optional)**
   - BME680 (temperature, humidity, pressure, gas)
   - PIR motion sensor
   - Camera module (OV7670, etc.)

### Pin Mapping (MKR WAN 1310)

```
LoRa Module:
- SS   = Pin 5  (SPI Chip Select)
- SCK  = Pin 13 (SPI Clock)
- MOSI = Pin 12 (SPI MOSI)
- MISO = Pin 11 (SPI MISO)
- DIO0 = Pin 3  (Data Interrupt 0)
- DIO1 = Pin 5  (Data Interrupt 1)
- TX_BUSY = Pin 14 (Transmit Busy)
```

---

## Software Requirements

### Arduino Libraries

Install via Arduino Library Manager:

1. **IBM LMIC Framework**
   ```
   Author: IBM, Matthijs Kooijman
   Version: 3.0.99 or later
   ```

2. **ArduinoLoRaWAN** (Alternative)
   ```
   Author: Arduino
   Version: 1.0.0 or later
   ```

3. **CRC16 Library**
   ```
   Author: Rob Tillaart
   Version: 0.1.0 or later
   ```

### PlatformIO Alternative

If using PlatformIO, add to `platformio.ini`:

```ini
[env:mkrwan1310]
platform = atmelsam
board = mkrwan1310
framework = arduino
lib_deps =
    mcci-catena/arduino-lmic@^3.0.99
    robtillaart/CRC16@^0.1.0
build_flags =
    -DCFG_eu868    # Change for your region
```

---

## Network Server Setup

### Option 1: The Things Network (TTN)

#### 1. Create Account

1. Visit [https://www.thethingsnetwork.org/](https://www.thethingsnetwork.org/)
2. Click "Console" and sign up

#### 2. Create Application

1. Go to Console → Applications
2. Click "Create application"
3. Enter:
   - **Application ID**: `my-iot-gateway`
   - **Application Name**: `IoT Multi-Sensor Gateway`
   - **Description**: `Multi-sensor IoT gateway with LoRaWAN`

#### 3. Register Device

1. In your application, click "Register end device"
2. **Method**: "Manually"
3. **Frequency Plan**: Select your region
   - `Europe 863-870 MHz` for EU868
   - `United States 902-928 MHz` for US915
   - `South Korea 920-923 MHz` for KR920

4. **LoRaWAN Version**: `MAC V1.0.3`

5. **Join EUI**: Click "Generate" or enter manually

6. **DevEUI**: Click "Generate" or use device's built-in EUI

7. **AppKey**: Click "Generate"

8. **Click "Register end device"**

#### 4. Copy Credentials

After registration, you'll see:

```
Device EUI:  70B3D57ED006A5B8
App EUI:     70B3D57ED000A5B8
App Key:     4A7F1E9C3D2B8F6A5E7D9C1B3A8F4E2D
```

**Copy these values** - you'll need them for your Arduino code!

---

### Option 2: ChirpStack

#### 1. Install ChirpStack

**Using Docker** (recommended):

```bash
git clone https://github.com/chirpstack/chirpstack-docker.git
cd chirpstack-docker
cp .env.example .env
# Edit .env with your configuration
docker-compose up -d
```

Access at: `http://localhost:8080`

Default credentials:
- Username: `admin`
- Password: `admin`

#### 2. Create Gateway (Optional)

If you have your own gateway:

1. Go to "Gateways" → "Create"
2. Enter Gateway EUI (usually on gateway label)
3. Configure frequency plan

#### 3. Create Application

1. Go to "Applications" → "Create"
2. Enter:
   - **Name**: `IoT Multi-Sensor Gateway`
   - **Description**: `Multi-sensor IoT gateway`

#### 4. Register Device

1. In your application, go to "Devices" → "Create"
2. **Device EUI**: Enter or generate
3. **Device name**: `mkr-wan-1310-gateway`
4. **Device profile**: Select appropriate profile
   - `EU868` for Europe
   - `US915` for USA
   - `KR920` for South Korea
5. **Application EUI**: Enter your application EUI
6. **Application Key**: Generate a new key

#### 5. Note the Keys

Copy these for your Arduino:
```
Device EUI:  0102030405060708
Application EUI:  0807060504030201
Application Key: 01020304050607080102030405060708
```

---

### Option 3: Private Network Server

For production deployments, consider running your own server:

#### ChirpStack (Open Source)
- **Pros**: Full control, no external dependencies
- **Cons**: Requires infrastructure and maintenance

#### Helium (Decentralized)
- **Pros**: Community network, low cost
- **Cons**: Limited coverage in some areas

#### Loriot (Commercial)
- **Pros**: Managed service, high reliability
- **Cons**: Monthly fees

---

## Device Registration

### OTAA (Over-The-Air Activation) - **RECOMMENDED**

OTAA is the most secure and recommended method.

#### Credentials Needed

1. **AppEUI** (Application EUI) - 8 bytes, LSB first
2. **DevEUI** (Device EUI) - 8 bytes, LSB first
3. **AppKey** (Application Key) - 16 bytes

#### Example Code

```cpp
#include "lorawan_implementation.h"

// OTAA Credentials (from TTN/ChirpStack)
// Format: LSB first (reverse bytes from console)
const uint8_t appEui[8] = {0xB8, 0xA5, 0x06, 0x00, 0xD5, 0x7E, 0xB3, 0x70};
const uint8_t devEui[8] = {0xB8, 0xA5, 0x06, 0xD0, 0x7E, 0x57, 0xE0, 0x5D};
const uint8_t appKey[16] = {
    0x2D, 0x4E, 0xF8, 0x2A, 0x8B, 0x4F, 0xE2, 0x1D,
    0x3C, 0xA7, 0x96, 0x5F, 0xB4, 0x8E, 0x1D, 0x3A
};

LoRaWANManager lorawan;

void setup() {
    Serial.begin(115200);
    delay(3000);

    Serial.println("Initializing LoRaWAN...");

    // Initialize with OTAA
    if (!lorawan.beginOTAA(appEui, appKey, devEui)) {
        Serial.println("Failed to initialize LoRaWAN");
        while (1);
    }

    // Connect to network
    Serial.println("Connecting to LoRaWAN network...");
    if (!lorawan.connect()) {
        Serial.println("Failed to connect");
        while (1);
    }

    Serial.println("Connected!");
}

void loop() {
    lorawan.loop();
    // Your application code here
}
```

---

### ABP (Activation By Personalization) - FALLBACK

ABP is less secure but faster (no join procedure).

#### Credentials Needed

1. **DevAddr** (Device Address) - 4 bytes
2. **NwkSKey** (Network Session Key) - 16 bytes
3. **AppSKey** (Application Session Key) - 16 bytes

#### Example Code

```cpp
#include "lorawan_implementation.h"

// ABP Credentials
const uint8_t nwkSKey[16] = {0x01, 0x02, ...};  // From network server
const uint8_t appSKey[16] = {0x01, 0x02, ...};  // From network server
const uint32_t devAddr = 0x26011F4C;            // From network server

LoRaWANManager lorawan;

void setup() {
    Serial.begin(115200);

    Serial.println("Initializing LoRaWAN (ABP)...");

    // Initialize with ABP
    if (!lorawan.beginABP(nwkSKey, appSKey, devAddr)) {
        Serial.println("Failed to initialize");
        while (1);
    }

    Serial.println("Connected (ABP mode)");
}

void loop() {
    lorawan.loop();
}
```

---

## Configuration

### Region Configuration

Edit `lorawan_implementation.h`:

```cpp
// Uncomment your region:
#define CFG_eu868                   // Europe (868 MHz)
// #define CFG_us915                // USA (915 MHz)
// #define CFG_kr920                // South Korea (920 MHz)
// #define CFG_as923                // Asia (923 MHz)
// #define CFG_au915                // Australia (915 MHz)
```

### Data Rate Configuration

```cpp
// Default data rate
#define DEFAULT_DATA_RATE DR3     // Balance of speed and range

// Data rate options by region:
// EU868: DR0 (SF12) to DR5 (SF7)
// US915: DR0 (SF10) to DR4 (SF8)
// KR920: DR0 (SF12) to DR5 (SF7)
```

**Trade-offs**:
- Lower DR = Longer range, slower transmission
- Higher DR = Faster transmission, shorter range

### TX Power Configuration

```cpp
#define DEFAULT_TX_POWER 14        // 14 dBm EIRP

// Range: 0-20 dBm (depends on region)
// Higher power = longer range, more power consumption
```

### Duty Cycle Configuration

```cpp
#define DUTY_CYCLE_LIMIT 1.0f      // 1% duty cycle
#define DUTY_CYCLE_WINDOW 3600000  // 1 hour rolling window
```

**Important**: Duty cycle is a legal requirement!
- EU868: **Strict 1%** per channel (ETSI regulation)
- US915: 1% best practice (FCC has no hard limit)
- KR920: **Strict 1%** (Korean regulation)

---

## Data Packet Formats

### 1. Sensor Data Packet (18 bytes)

**Optimized binary format for efficiency**

```cpp
#pragma pack(push, 1)
typedef struct {
    uint16_t magic;                // 0xA5A5 (2 bytes) - Packet validation
    uint8_t type;                  // PACKET_TYPE_SENSOR (1 byte)
    uint32_t timestamp;            // Unix timestamp (4 bytes)
    int16_t temperature;           // Temperature x100 (2 bytes)
                                  // Example: 23.45°C = 2345
    uint16_t humidity;             // Humidity x100 (2 bytes)
                                  // Example: 65.00% = 6500
    uint16_t pressure;             // Pressure x10 (2 bytes)
                                  // Example: 1013.2 hPa = 10132
    uint16_t gasResistance;        // Gas resistance in Ohms (2 bytes)
    uint16_t iaq;                  // Indoor Air Quality (2 bytes)
    uint8_t status;                // Status flags (1 byte)
    uint8_t battery;               // Battery level 0-100 (1 byte)
    uint16_t checksum;             // CRC16 checksum (2 bytes)
} SensorDataPacket;
#pragma pack(pop)
```

**Status Flags**:
```
Bit 0 (0x01): STATUS_SENSOR_OK       - All sensors operational
Bit 1 (0x02): STATUS_MOTION_DETECT   - Motion detected
Bit 2 (0x04): STATUS_OBJECT_DETECT   - Object detected
Bit 3 (0x08): STATUS_ALARM_ACTIVE    - Alarm active
Bit 4 (0x10): STATUS_LOW_BATTERY     - Battery < 20%
Bit 5 (0x20): STATUS_SENSOR_ERROR    - Sensor error
Bit 6 (0x40): STATUS_NETWORK_ERROR   - Network error
Bit 7 (0x80): STATUS_MEMORY_ERROR    - Memory error
```

#### Example: Creating Sensor Packet

```cpp
SensorDataPacket createSensorPacket(
    float temperature,      // 23.45
    float humidity,         // 65.0
    float pressure,         // 1013.2
    float gasResistance,    // 45000.0
    uint16_t iaq,          // 150
    uint8_t status,        // 0x01 (sensor OK)
    uint8_t battery        // 85
);

// Usage:
SensorDataPacket packet = createSensorPacket(
    23.45f,    // Temperature
    65.0f,     // Humidity
    1013.2f,   // Pressure
    45000.0f,  // Gas resistance
    150,       // IAQ
    0x01,      // Status flags
    85         // Battery level
);

lorawan.transmitSensorData(packet);
```

---

### 2. Detection Event Packet (12 bytes)

**For motion/object detection events**

```cpp
#pragma pack(push, 1)
typedef struct {
    uint16_t magic;                // 0xA5A5 (2 bytes)
    uint8_t type;                  // PACKET_TYPE_DETECTION (1 byte)
    uint32_t timestamp;            // Unix timestamp (4 bytes)
    uint8_t detectionType;         // Detection type (1 byte)
    uint8_t confidence;            // Confidence 0-100 (1 byte)
    uint16_t duration;             // Duration in seconds (2 bytes)
    uint16_t checksum;             // CRC16 (2 bytes)
} DetectionDataPacket;
#pragma pack(pop)
```

**Detection Types**:
```
0x01: DETECTION_TYPE_MOTION   - Motion detected
0x02: DETECTION_TYPE_OBJECT   - Object detected
0x03: DETECTION_TYPE_PERSON   - Person detected
0x04: DETECTION_TYPE_VEHICLE  - Vehicle detected
0x05: DETECTION_TYPE_ANIMAL   - Animal detected
```

#### Example: Creating Detection Packet

```cpp
DetectionDataPacket packet = createDetectionPacket(
    DETECTION_TYPE_PERSON,    // Person detected
    85,                       // 85% confidence
    30                        // Detected for 30 seconds
);

lorawan.transmitDetection(packet);
```

---

### 3. Status Packet (15 bytes)

**Periodic status updates**

```cpp
#pragma pack(push, 1)
typedef struct {
    uint16_t magic;                // 0xA5A5 (2 bytes)
    uint8_t type;                  // PACKET_TYPE_STATUS (1 byte)
    uint32_t uptime;               // Uptime in seconds (4 bytes)
    uint32_t txCount;              // Total transmissions (4 bytes)
    uint32_t rxCount;              // Total downlinks (4 bytes)
    uint8_t dataRate;              // Current DR (1 byte)
    uint8_t battery;               // Battery level (1 byte)
    uint16_t checksum;             // CRC16 (2 bytes)
} StatusDataPacket;
#pragma pack(pop)
```

#### Example: Creating Status Packet

```cpp
StatusDataPacket packet = createStatusPacket(
    lorawan,   // LoRaWAN manager instance
    85         // Battery level
);

lorawan.transmitStatus(packet);
```

---

## Downlink Commands

### Command Format

All downlink commands follow this format:

```
[Command ID (1 byte)] [Payload (0-16 bytes)]
```

### Available Commands

#### 1. PING (0x00)

Test connectivity.

**Request**: `[0x00]`

**Response**: `[0x80]` (ACK)

**Example** (using TTN Console):
```
Downlink: 00
Expected Uplink: 80 (ACK)
```

---

#### 2. SET_INTERVAL (0x01)

Change transmission interval.

**Request**:
```
[0x01] [Interval LSB] [Byte 2] [Byte 3] [Interval MSB]
```

**Response**: `[0x80]` (ACK)

**Parameters**:
- Interval: 4 bytes, milliseconds, little-endian
- Range: 10,000 - 3,600,000 ms (10 seconds - 1 hour)

**Example**:
```
Set interval to 30 seconds:
Downlink: 01 10 27 00 00
(Uplink: 80 for ACK)

Set interval to 5 minutes:
Downlink: 01 20 4E 00 00
```

---

#### 3. SET_DATARATE (0x02)

Change data rate.

**Request**: `[0x02] [DR]`

**Response**: `[0x80]` (ACK)

**Parameters**:
- DR: 1 byte, data rate (0-5)

**Example**:
```
Set data rate to DR2:
Downlink: 02 02

Set data rate to DR5:
Downlink: 02 05
```

---

#### 4. SET_TXPOWER (0x03)

Change TX power.

**Request**: `[0x03] [Power]`

**Response**: `[0x80]` (ACK)

**Parameters**:
- Power: 1 byte, TX power in dBm (0-20)

**Example**:
```
Set TX power to 14 dBm:
Downlink: 03 0E

Set TX power to 20 dBm (max):
Downlink: 03 14
```

---

#### 5. REBOOT (0x04)

Reboot the device.

**Request**: `[0x04]`

**Response**: `[0x80]` (ACK), then device reboots

**Example**:
```
Downlink: 04
(Uplink: 80 for ACK)
(Device reboots after 1 second)
```

---

#### 6. GET_STATUS (0x05)

Request device status.

**Request**: `[0x05]`

**Response**:
```
[0x82] [Interval LSB...MSB] [DR] [Power] [ADR] [LED] [Alarm] [TX Count LSB...MSB] [RX Count LSB...MSB]
```

**Response Fields** (18 bytes):
- Response ID: 1 byte
- Interval: 4 bytes, milliseconds
- Data Rate: 1 byte
- TX Power: 1 byte
- ADR Enabled: 1 byte (0/1)
- LED Enabled: 1 byte (0/1)
- Alarm Enabled: 1 byte (0/1)
- TX Count: 4 bytes
- RX Count: 4 bytes

**Example**:
```
Downlink: 05
Uplink: 82 10 27 00 00 03 0E 01 01 00 00 00 01 00 00 00 00 00 05
```

---

#### 7. SET_LED (0x06)

Control built-in LED.

**Request**: `[0x06] [Enabled]`

**Response**: `[0x80]` (ACK)

**Parameters**:
- Enabled: 1 byte (0=off, 1=on)

**Example**:
```
Turn LED on:
Downlink: 06 01

Turn LED off:
Downlink: 06 00
```

---

#### 8. SET_ALARM (0x07)

Enable/disable alarm.

**Request**: `[0x07] [Enabled]`

**Response**: `[0x80]` (ACK)

**Parameters**:
- Enabled: 1 byte (0=off, 1=on)

**Example**:
```
Enable alarm:
Downlink: 07 01

Disable alarm:
Downlink: 07 00
```

---

#### 9. GET_BATTERY (0x08)

Get battery level.

**Request**: `[0x08]`

**Response**: `[0x83] [Percent] [Voltage]`

**Response Fields**:
- Response ID: 1 byte
- Percent: 1 byte (0-100)
- Voltage: 1 byte (voltage x 10, e.g., 42 = 4.2V)

**Example**:
```
Downlink: 08
Uplink: 83 55 2A
         │  │  └─ 4.2V (42 x 10)
         │  └──── 85%
         └─────── Battery response
```

---

#### 10. SET_ADR (0x09)

Enable/disable Adaptive Data Rate.

**Request**: `[0x09] [Enabled]`

**Response**: `[0x80]` (ACK)

**Parameters**:
- Enabled: 1 byte (0=off, 1=on)

**Example**:
```
Enable ADR:
Downlink: 09 01

Disable ADR:
Downlink: 09 00
```

---

#### 11. CLEAR_STATS (0x0A)

Clear transmission statistics.

**Request**: `[0x0A]`

**Response**: `[0x80]` (ACK)

**Example**:
```
Downlink: 0A
(Uplink: 80 for ACK)
(Statistics cleared)
```

---

### Error Responses

If a command fails, the device responds with:

```
[0xFF] [Error Code]
```

**Error Codes**:
- `0x01`: ERR_UNKNOWN_COMMAND - Unknown command ID
- `0x02`: ERR_INVALID_PARAMETER - Invalid parameter value
- `0x03`: ERR_NOT_IMPLEMENTED - Command not implemented
- `0x04`: ERR_BUFFER_OVERFLOW - Buffer overflow
- `0x05`: ERR_CHECKSUM_FAIL - Checksum validation failed
- `0x06`: ERR_NOT_JOINED - Device not joined to network

---

## Frequency Plans

### EU868 (Europe)

**Frequency Range**: 863-870 MHz

**Default Channels**:
```
Channel 0: 868.100 MHz (RX1)
Channel 1: 868.300 MHz
Channel 2: 868.500 MHz
```

**RX2 Window**: 869.525 MHz, DR0 (SF12)

**Duty Cycle**: **1% strict limit** (legal requirement)

**Data Rates**:
```
DR0: SF12, 125 kHz, 0.98 kbps
DR1: SF11, 125 kHz, 1.76 kbps
DR2: SF10, 125 kHz, 3.13 kbps
DR3: SF9,  125 kHz, 5.47 kbps
DR4: SF8,  125 kHz, 9.78 kbps
DR5: SF7,  125 kHz, 17.3 kbps
DR6: SF7,  250 kHz, 17.3 kbps (RFU)
```

**TX Power**: Max 14 dBm EIRP

---

### US915 (USA)

**Frequency Range**: 902-928 MHz

**Default Channels**: 64 channels (125 kHz each)

**Join Channels**: Channels 8-15 (916.8-917.0 MHz)

**RX2 Window**: 923.3 MHz, DR0 (SF10)

**Duty Cycle**: No hard limit, but 1% best practice

**Data Rates**:
```
DR0: SF10, 125 kHz, 0.98 kbps
DR1: SF9,  125 kHz, 1.76 kbps
DR2: SF8,  125 kHz, 3.13 kbps
DR3: SF7,  125 kHz, 5.47 kbps
DR4: SF8,  500 kHz, 12.5 kbps
```

**TX Power**: Max 20 dBm EIRP

---

### KR920 (South Korea)

**Frequency Range**: 920-923 MHz

**Default Channels**:
```
Channel 0: 921.900 MHz
Channel 1: 922.100 MHz
Channel 2: 922.300 MHz
```

**RX2 Window**: 921.900 MHz, DR0 (SF12)

**Duty Cycle**: **1% strict limit** (legal requirement)

**Data Rates**:
```
DR0: SF12, 125 kHz, 0.98 kbps
DR1: SF11, 125 kHz, 1.76 kbps
DR2: SF10, 125 kHz, 3.13 kbps
DR3: SF9,  125 kHz, 5.47 kbps
DR4: SF8,  125 kHz, 9.78 kbps
DR5: SF7,  125 kHz, 17.3 kbps
```

**TX Power**: Max 14 dBm EIRP

---

## Troubleshooting

### Problem: Cannot Join Network

**Symptoms**:
- "Join failed" message
- Multiple join attempts without success
- Timeout after 60 seconds

**Solutions**:

1. **Check Antenna**
   ```cpp
   // Ensure antenna is connected before transmitting
   // Never transmit without antenna!
   ```

2. **Verify Credentials**
   ```cpp
   // Check if credentials are in LSB format (reversed bytes)
   // TTN/ChirpStack display MSB, Arduino needs LSB

   // Convert from MSB to LSB:
   // "70B3D57ED006A5B8" (MSB)
   // becomes: {0xB8, 0xA5, 0x06, 0xD0, 0x7E, 0x57, 0xE0, 0x5D}
   ```

3. **Check Device Registration**
   - Verify device is registered on network server
   - Check device status (should be "active")

4. **Verify Frequency Plan**
   ```cpp
   // Ensure correct region is selected:
   #define CFG_eu868  // For Europe
   // #define CFG_us915  // For USA
   ```

5. **Check Gateway Coverage**
   - Use TTN Mapper to check coverage
   - Move closer to gateway for testing
   - Check gateway status (online/offline)

6. **Increase Join Attempts**
   ```cpp
   // Default: 5 attempts with 60s delay
   // You can modify in lorawan_implementation.h:
   #define JOIN_MAX_RETRIES 10
   #define JOIN_RETRY_DELAY 30000  // 30 seconds
   ```

---

### Problem: High Packet Loss (>50%)

**Symptoms**:
- Many "TX failed" messages
- Low success rate in statistics
- Missing data on network server

**Solutions**:

1. **Check Signal Strength**
   ```cpp
   // Check RSSI in Serial Monitor
   // Good: > -90 dBm
   // Fair: -90 to -110 dBm
   // Poor: < -110 dBm
   ```

2. **Reduce Data Rate**
   ```cpp
   // Use lower DR for longer range:
   lorawan.setDataRate(DR0);  // Longest range, slowest
   ```

3. **Increase TX Power**
   ```cpp
   // Use maximum TX power:
   lorawan.setTxPower(20);  // 20 dBm
   ```

4. **Check for Interference**
   ```cpp
   // Try different channels
   // Use spectrum analyzer if available
   // Move away from WiFi routers, microwaves, etc.
   ```

5. **Verify Duty Cycle**
   ```cpp
   // Check if duty cycle limit is causing delays:
   float usage = lorawan.getDutyCycleUsage();
   Serial.print("Duty cycle: ");
   Serial.print(usage);
   Serial.println("%");
   ```

6. **Adjust Retry Settings**
   ```cpp
   // Increase retry count:
   lorawan.setMaxRetries(10);
   ```

---

### Problem: Duty Cycle Limit Reached

**Symptoms**:
- "Duty cycle limit reached" error
- Cannot transmit even after waiting
- Duty cycle usage shows > 1%

**Solutions**:

1. **Reduce Transmission Frequency**
   ```cpp
   // Increase interval between transmissions:
   lorawan.setTransmitInterval(300000);  // 5 minutes
   ```

2. **Optimize Payload Size**
   ```cpp
   // Use binary format (18 bytes) instead of JSON (100+ bytes)
   // Already implemented in our packet structure
   ```

3. **Use Higher Data Rate**
   ```cpp
   // Higher DR = faster transmission = less airtime:
   lorawan.setDataRate(DR5);  // Fastest
   ```

4. **Wait for Duty Cycle Reset**
   ```cpp
   // Duty cycle resets after 1 hour
   // Or manually reset:
   lorawan.resetDutyCycle();
   ```

---

### Problem: Downlink Not Received

**Symptoms**:
- Downlink commands not working
- No "Downlink received" message
- Device doesn't respond to commands

**Solutions**:

1. **Check RX Windows**
   ```cpp
   // Ensure RX windows are configured:
   #define RX1_DELAY 1000  // 1 second
   #define RX2_DELAY 2000  // 2 seconds
   ```

2. **Verify Downlink is Scheduled**
   - On TTN Console: Go to device → "Messaging" → "Schedule downlink"
   - Downlink must be sent shortly after uplink (within RX windows)

3. **Check Downlink Port**
   ```cpp
   // Ensure using correct port:
   // Sensor data: Port 1
   // Status: Port 2
   // Commands: Port 3
   // Detection: Port 4
   ```

4. **Verify Downlink Format**
   ```
   // Downlink payload must be in hex:
   // Command: SET_INTERVAL to 30 seconds
   // Payload: 01 10 27 00 00
   ```

5. **Enable Debug Mode**
   ```cpp
   // Check if command is being received:
   systemConfig.debugEnabled = 1;
   ```

---

### Problem: Device Won't Boot

**Symptoms**:
- Device hangs at startup
- No Serial output
- LED not flashing

**Solutions**:

1. **Check Power Supply**
   - Ensure USB cable can supply enough current
   - Try external power supply
   - Check battery voltage

2. **Reduce Memory Usage**
   ```cpp
   // Reduce serial buffer size:
   Serial.setTxBufferSize(128);
   Serial.setRxBufferSize(128);
   ```

3. **Disable Debug Output**
   ```cpp
   // Comment out debug Serial.println() calls
   // Or use conditional compilation:
   #ifdef DEBUG
       Serial.println("Debug message");
   #endif
   ```

4. **Check for Stack Overflow**
   ```cpp
   // Increase stack size if using RTOS:
   // Or reduce local variable usage
   ```

---

## Best Practices

### 1. Always Use OTAA

**Why**:
- More secure (keys are never transmitted)
- Network assigns session keys
- Automatic channel selection
- Better for roaming

**Exception**: Use ABP only if:
- Join procedure takes too long
- Network doesn't support OTAA
- Testing/development

---

### 2. Enable Adaptive Data Rate (ADR)

**Why**:
- Network optimizes data rate
- Automatic power adjustment
- Better battery life
- More reliable transmissions

```cpp
// ADR is enabled by default:
lorawan.setAdrEnabled(true);
```

---

### 3. Use Binary Format

**Why**:
- Smaller payload = less airtime
- Less airtime = more messages within duty cycle
- Better battery life
- More reliable

**Example**:
```
Binary: 18 bytes
JSON:   {"temp":23.45,"hum":65.0,"pres":1013.2} = ~45 bytes

Savings: 60% reduction!
```

---

### 4. Implement Retry Logic

**Why**:
- LoRaWAN is unreliable by design
- Interference is common
- Gateways can be busy

**Already Implemented**:
```cpp
// Exponential backoff with max retries
// Attempt 1: Immediate
// Attempt 2: 1 second delay
// Attempt 3: 2 seconds delay
// Attempt 4: 4 seconds delay
// Max delay: 60 seconds
```

---

### 5. Monitor Duty Cycle

**Why**:
- Legal requirement (EU/KR)
- Prevents network congestion
- Fair access for all devices

```cpp
// Check duty cycle regularly:
if (lorawan.getDutyCycleUsage() > 0.8f) {
    // Reduce transmission frequency
    lorawan.setTransmitInterval(currentInterval * 2);
}
```

---

### 6. Implement Downlink Commands

**Why**:
- Remote configuration
- OTA updates
- Debugging
- Emergency control

**Best Commands to Implement**:
- SET_INTERVAL
- SET_DATARATE
- REBOOT
- GET_STATUS
- GET_BATTERY

---

### 7. Log Transmission Statistics

**Why**:
- Monitor success rate
- Detect issues early
- Optimize performance

```cpp
// Print statistics every hour:
if (millis() - lastStatsTime >= 3600000) {
    lorawan.printStatistics();
    lastStatsTime = millis();
}
```

---

### 8. Use Appropriate Data Rates

**Guidelines**:

| Scenario | Recommended DR |
|----------|---------------|
| Urban (short range) | DR4-DR5 |
| Suburban (medium range) | DR2-DR3 |
| Rural (long range) | DR0-DR1 |
| Remote (max range) | DR0 |

---

### 9. Implement Sleep Mode

**Why**:
- Massive battery savings
- LoRaWAN is bursty by nature
- Wake up only to transmit

```cpp
// Example sleep implementation:
void loop() {
    // Transmit data
    if (millis() - lastTransmit >= transmitInterval) {
        lorawan.transmitSensorData(packet);
        lastTransmit = millis();
    }

    // Enter sleep
    __WFI();  // Wait For Interrupt (low power mode)
}
```

---

### 10. Test Thoroughly

**Test Checklist**:
- [ ] Join network successfully
- [ ] Transmit sensor data
- [ ] Receive downlink commands
- [ ] Handle retries
- [ ] Duty cycle enforcement
- [ ] Battery consumption
- [ ] Range testing
- [ ] Interference testing
- [ ] Long-term stability (24+ hours)

---

## API Reference

### LoRaWANManager Class

#### Initialization

```cpp
bool beginOTAA(const uint8_t* appEui, const uint8_t* appKey, const uint8_t* devEui = nullptr);
bool beginABP(const uint8_t* nwkSKey, const uint8_t* appSKey, uint32_t devAddr);
```

#### Connection Management

```cpp
bool connect();
void disconnect();
bool isConnected() const;
bool isJoining() const;
bool rejoin();
```

#### Transmission

```cpp
bool transmitPacket(const uint8_t* payload, size_t size, uint8_t port = LORAWAN_PORT_SENSOR);
bool transmitSensorData(const SensorDataPacket& packet);
bool transmitDetection(const DetectionDataPacket& packet);
bool transmitStatus(const StatusDataPacket& packet);
```

#### Configuration

```cpp
void setDataRate(uint8_t dr);
void setTxPower(int8_t power);
void setTransmitInterval(unsigned long interval);
void setAdrEnabled(bool enabled);
void setMaxRetries(uint8_t maxRetries);
```

#### Getters

```cpp
uint8_t getDataRate() const;
int8_t getTxPower() const;
unsigned long getTransmitInterval() const;
bool isAdrEnabled() const;
float getDutyCycleUsage() const;
unsigned long getLastTransmission() const;
```

#### Statistics

```cpp
uint32_t getTxCount() const;
uint32_t getTxSuccessCount() const;
uint32_t getTxFailCount() const;
uint32_t getRxCount() const;
void resetStatistics();
void printStatistics();
```

#### Callbacks

```cpp
void setOnJoinCallback(OnJoinCallback callback);
void setOnTxCompleteCallback(OnTxCompleteCallback callback);
void setDownlinkCallback(OnDownlinkCallback callback);
void setOnErrorCallback(OnErrorCallback callback);
```

#### Main Loop

```cpp
void loop();  // Must be called in main loop
void onEvent(ev_t ev);  // LMIC event handler
```

---

### Utility Functions

#### Packet Creation

```cpp
SensorDataPacket createSensorPacket(
    float temperature,
    float humidity,
    float pressure,
    float gasResistance,
    uint16_t iaq,
    uint8_t status,
    uint8_t battery
);

DetectionDataPacket createDetectionPacket(
    uint8_t detectionType,
    uint8_t confidence,
    uint16_t duration
);

StatusDataPacket createStatusPacket(
    const LoRaWANManager& manager,
    uint8_t battery
);
```

#### Downlink Handler

```cpp
void setupDownlinkHandler();
void processDownlink();
void printSystemConfig();
```

---

## Complete Example

### Full Sketch Example

```cpp
#include "lorawan_implementation.h"
#include "downlink_handler.cpp"  // For downlink handling

// OTAA Credentials (replace with your own)
const uint8_t appEui[8] = {0xB8, 0xA5, 0x06, 0x00, 0xD5, 0x7E, 0xB3, 0x70};
const uint8_t devEui[8] = {0xB8, 0xA5, 0x06, 0xD0, 0x7E, 0x57, 0xE0, 0x5D};
const uint8_t appKey[16] = {
    0x2D, 0x4E, 0xF8, 0x2A, 0x8B, 0x4F, 0xE2, 0x1D,
    0x3C, 0xA7, 0x96, 0x5F, 0xB4, 0x8E, 0x1D, 0x3A
};

// Timing
unsigned long lastTransmit = 0;
unsigned long transmitInterval = 60000;  // 60 seconds

// Sensor data (mock values)
float temperature = 23.45f;
float humidity = 65.0f;
float pressure = 1013.2f;
float gasResistance = 45000.0f;
uint16_t iaq = 150;
uint8_t battery = 85;

void setup() {
    Serial.begin(115200);
    delay(3000);

    Serial.println("===========================================");
    Serial.println("IoT Multi-Sensor Gateway - LoRaWAN");
    Serial.println("===========================================");

    // Initialize LoRaWAN
    Serial.println("Initializing LoRaWAN...");
    if (!LoRaWAN.beginOTAA(appEui, appKey, devEui)) {
        Serial.println("ERROR: Failed to initialize LoRaWAN");
        while (1);
    }

    // Setup downlink handler
    setupDownlinkHandler();

    // Connect to network
    Serial.println("Connecting to LoRaWAN network...");
    if (!LoRaWAN.connect()) {
        Serial.println("ERROR: Failed to connect");
        Serial.println("Will retry in setup()");
    }

    // Set join callback
    LoRaWAN.setOnJoinCallback([](bool success) {
        if (success) {
            Serial.println("Join successful!");
            LoRaWAN.printState();
        } else {
            Serial.println("Join failed!");
        }
    });

    // Set TX complete callback
    LoRaWAN.setOnTxCompleteCallback([](bool success) {
        if (success) {
            Serial.println("Transmission successful");
        } else {
            Serial.println("Transmission failed");
        }
    });

    // Set error callback
    LoRaWAN.setOnErrorCallback([](uint8_t errorCode) {
        Serial.print("Error: ");
        Serial.println(errorCode);
    });

    Serial.println("Setup complete");
    Serial.println("===========================================");
}

void loop() {
    // Process LoRaWAN events
    LoRaWAN.loop();

    // Process downlink
    processDownlink();

    // Transmit sensor data periodically
    if (millis() - lastTransmit >= transmitInterval) {
        if (LoRaWAN.isConnected()) {
            Serial.println("-------------------------------------------");

            // Create sensor packet
            uint8_t status = STATUS_SENSOR_OK;
            SensorDataPacket packet = createSensorPacket(
                temperature,
                humidity,
                pressure,
                gasResistance,
                iaq,
                status,
                battery
            );

            // Transmit
            Serial.println("Transmitting sensor data...");
            bool success = LoRaWAN.transmitSensorData(packet);

            if (success) {
                Serial.println("Sensor data transmitted successfully");

                // Print statistics
                LoRaWAN.printStatistics();
            } else {
                Serial.println("Sensor data transmission failed");
            }

            Serial.println("-------------------------------------------");
        } else {
            Serial.println("Not connected, attempting to join...");
            LoRaWAN.connect();
        }

        lastTransmit = millis();
    }

    // Simulate sensor data changes
    temperature += 0.1f;
    if (temperature > 30.0f) temperature = 20.0f;
    battery -= 1;
    if (battery < 10) battery = 100;

    // Small delay
    delay(100);
}
```

---

## Conclusion

This guide provides a complete, production-ready LoRaWAN implementation for the Arduino MKR WAN 1310. With this implementation, you can:

- Connect to any LoRaWAN network server (TTN, ChirpStack, etc.)
- Transmit optimized binary sensor data
- Receive and process downlink commands
- Manage duty cycle compliance
- Handle errors and retries
- Monitor statistics and performance

For additional support or questions, refer to:
- [The Things Network documentation](https://www.thethingsnetwork.org/docs)
- [ChirpStack documentation](https://www.chirpstack.io/docs/)
- [LoRaWAN specification](https://lora-alliance.org/resource_hub/lorawan-specification-v1-0-3/)

---

**Version**: 2.0.0
**Last Updated**: 2025-01-06
**Author**: Production-Ready Implementation
**License**: MIT
