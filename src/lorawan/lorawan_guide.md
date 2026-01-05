# LoRaWAN Implementation Guide

## Overview

This document provides a complete guide for implementing LoRaWAN communication on the Arduino MKR WAN 1310 for the IoT multi-sensor system.

## Features

- **OTAA Authentication**: Secure over-the-air activation
- **Binary Data Format**: Efficient payload transmission
- **Duty Cycle Management**: 1% rule compliance (FCE/ETSI)
- **Exponential Backoff**: Automatic retry with increasing delays
- **Downlink Handling**: Remote command processing
- **Adaptive Data Rate**: Automatic optimization

## Hardware Requirements

- Arduino MKR WAN 1310
- LoRaWAN antenna (868MHz or 915MHz)
- LoRaWAN network server (e.g., The Things Network, ChirpStack)

## Quick Start

### 1. Hardware Setup

```cpp
// Connect LoRaWAN antenna to MKR WAN 1310
// Ensure proper antenna connection before transmitting
```

### 2. Network Registration

1. Register device on LoRaWAN network server
2. Obtain credentials:
   - AppEUI (Application EUI)
   - AppKey (Application Key)
   - DevEUI (Device EUI, optional - can be auto-generated)

### 3. Code Integration

```cpp
#include "lorawan_implementation.h"

// Credentials (replace with your own)
const char* appEui = "YOUR_APP_EUI";
const char* appKey = "YOUR_APP_KEY";
const char* devEui = "YOUR_DEV_EUI";

// Create LoRaWAN manager
LoRaWANManager lorawan;

void setup() {
  Serial.begin(115200);

  // Initialize LoRaWAN
  if (!lorawan.begin(appEui, appKey, devEui)) {
    Serial.println("LoRaWAN initialization failed");
    return;
  }

  // Connect to network
  if (!lorawan.connect()) {
    Serial.println("LoRaWAN connection failed");
    return;
  }

  Serial.println("LoRaWAN connected!");
}

void loop() {
  // Update LoRaWAN (process downlink)
  lorawan.update();

  // Create and transmit packet
  if (millis() - lastTransmit >= 60000) {  // Every minute
    LoRaWANPacket packet = createLoRaWANPacket(
      23.5f,    // Temperature
      65.0f,    // Humidity
      1013.2f,  // Pressure
      45000.0f, // Gas resistance
      0x01,     // Status flags
      85        // Battery level
    );

    lorawan.transmit(packet);
    lastTransmit = millis();
  }
}
```

## Data Format

### Uplink Packet Structure

```cpp
struct LoRaWANPacket {
  uint16_t header;        // 0xA5A5 (magic number)
  uint32_t timestamp;     // Unix timestamp
  int16_t temperature;    // x100 (e.g., 2345 = 23.45°C)
  uint16_t humidity;      // x100 (e.g., 6500 = 65.00%)
  uint16_t pressure;      // x10 (e.g., 10132 = 1013.2 hPa)
  uint16_t gasResistance; // Ohms
  uint8_t status;         // Status flags
  uint8_t battery;        // Battery level (0-100)
  uint16_t checksum;      // CRC16
};
```

Total: 18 bytes per packet

### Status Flags

| Bit | Flag | Description |
|-----|------|-------------|
| 0 | STATUS_SENSOR_OK | All sensors operational |
| 1 | STATUS_MOTION_DETECT | Motion detected |
| 2 | STATUS_OBJECT_DETECT | Object detected in camera |
| 3 | STATUS_ALARM_ACTIVE | Alarm is active |
| 4 | STATUS_LOW_BATTERY | Battery below threshold |

### Data Encoding

**Why use integer encoding instead of floats?**

- Efficiency: 2 bytes instead of 4+ bytes
- Bandwidth: Reduced airtime = more messages within duty cycle
- Compatibility: Better LoRaWAN compatibility

**Examples:**
- Temperature: 23.45°C → 2345 (int16)
- Humidity: 65.00% → 6500 (uint16)
- Pressure: 1013.2 hPa → 10132 (uint16)

## Duty Cycle Management

### What is Duty Cycle?

Duty cycle limits transmission time to prevent channel congestion and ensure fair access.

- **US (FCC)**: No hard limit, but best practice is 1%
- **EU (ETSI)**: Strict 1% duty cycle per channel
- **Calculation**: (TX time / window) × 100%

### Duty Cycle Tracking

```cpp
// Check if transmission is allowed
if (lorawan.canTransmit()) {
  // Transmit
  lorawan.transmit(packet);
} else {
  Serial.println("Duty cycle limit reached");
}

// Get current duty cycle usage
float usage = lorawan.getDutyCycleUsage();
Serial.print("Duty cycle: ");
Serial.print(usage, 2);
Serial.println("%");
```

### Duty Cycle Optimization

```cpp
// Strategies to minimize duty cycle:

1. Reduce transmission frequency
   - From 1 minute → 5 minutes
   - Saves 80% duty cycle

2. Use lower data rates
   - Higher DR = faster transmission = less airtime
   - But shorter range

3. Optimize payload size
   - Use binary format (not JSON)
   - 18 bytes vs 100+ bytes

4. Batch data
   - Send multiple readings in one packet
   - Reduces overhead
```

## Retry Logic

### Exponential Backoff

```cpp
// Retry delays:
// Attempt 1: Immediate
// Attempt 2: 1 second (1000ms × 2^0)
// Attempt 3: 2 seconds (1000ms × 2^1)
// Attempt 4: 4 seconds (1000ms × 2^2)
// Maximum: 60 seconds
```

### Configuration

```cpp
#define MAX_RETRIES 3
#define RETRY_DELAY_INITIAL 1000    // 1 second
#define RETRY_DELAY_MAX 60000       // 60 seconds
```

### Monitoring Retries

```cpp
// The library automatically retries on failure
// Monitor success rate:

int successCount = 0;
int failCount = 0;

if (lorawan.transmit(packet)) {
  successCount++;
} else {
  failCount++;
  Serial.println("Transmission failed after retries");
}

float successRate = (float)successCount / (successCount + failCount) * 100;
```

## Downlink Commands

### Supported Commands

| Command ID | Command | Payload | Response |
|------------|---------|---------|----------|
| 0x00 | PING | None | ACK |
| 0x01 | SET_INTERVAL | uint32_t (ms) | ACK |
| 0x02 | SET_DATARATE | uint8_t (DR) | ACK |
| 0x03 | REBOOT | None | ACK |
| 0x04 | GET_STATUS | None | Status |
| 0x05 | SET_LED | uint8_t (0/1) | ACK |
| 0x06 | SET_ALARM | uint8_t (0/1) | ACK |
| 0x07 | GET_BATTERY | None | Battery |

### Example: Set Transmission Interval

**Downlink payload:**
```
[0x01, 0x10, 0x27, 0x00, 0x00]  // 10 seconds (10000ms)
```

**Response:**
```
[0x80]  // ACK
```

### Example: Get Status

**Downlink payload:**
```
[0x04]  // GET_STATUS
```

**Response:**
```
[0x81, 0x10, 0x27, 0x00, 0x00, 0x03, 0x01, 0x00, 0x03]
  │     │───────────────────────│ │──│ │──│ │──│
  │     │   Interval (10000ms)   │ DR│ LED│Alm│ Curr DR
  │                           │ 3  │ ON │OFF│   3
Status
```

## Network Server Configuration

### The Things Network (TTN)

```yaml
# Console configuration

Frequency:
  - US915 (USA)
  - EU868 (Europe)

Data Rate (DR):
  - DR0 (slowest, longest range)
  - DR1
  - DR2
  - DR3 (default)
  - DR4
  - DR5 (fastest, shortest range)

Adaptive Data Rate (ADR):
  - Enabled (recommended)
  - Network optimizes data rate

RX1 Delay:
  - 1 second (default)

RX2 Frequency:
  - US915: 923.3 MHz
  - EU868: 869.525 MHz
```

### ChirpStack

```yaml
# ChirpStack configuration

General:
  Device Status: Enabled
  MAC Version: 1.0.3
  PHY Version: 1.0.3revA

RX Window:
  RX1 Delay: 1s
  RX2 Frequency: 923.3MHz
  RX2 Data Rate: DR0

ADR:
  Algorithm: Default
  Margin: 10dB
```

## Troubleshooting

### No Connection

```
Problem: Cannot connect to LoRaWAN network

Solutions:
1. Verify antenna is connected
2. Check credentials (AppEUI, AppKey)
3. Verify device is registered on network server
4. Check frequency region (US915 vs EU868)
5. Ensure gateway is in range
6. Try increasing TX power
```

### High Failure Rate

```
Problem: High packet loss (> 50%)

Solutions:
1. Check signal strength (RSSI)
2. Reduce data rate for longer range
3. Increase TX power
4. Check for interference
5. Verify duty cycle compliance
6. Add more gateways (if deploying own)
```

### Duty Cycle Limit

```
Problem: "Duty cycle limit reached" error

Solutions:
1. Reduce transmission frequency
2. Optimize payload size
3. Use multiple channels (network dependent)
4. Wait for duty cycle reset (1 hour window)
5. Consider higher data rates
```

### Downlink Not Received

```
Problem: Downlink commands not working

Solutions:
1. Verify RX windows are configured
2. Check network server downlink queue
3. Ensure sufficient time between uplinks
4. Verify downlink payload format
5. Check serial output for errors
```

## Performance Optimization

### Throughput vs Range

| Data Rate | Bit Rate | Range | Best For |
|-----------|----------|-------|----------|
| DR0 | 0.98 kbps | 15km | Remote sensors |
| DR1 | 1.76 kbps | 12km | Outdoor |
| DR2 | 3.13 kbps | 10km | Typical |
| DR3 | 5.47 kbps | 8km | Default |
| DR4 | 9.78 kbps | 6km | Urban |
| DR5 | 17.3 kbps | 4km | Dense urban |

### Payload Size vs Airtime

| Payload (bytes) | Airtime (ms) @ DR3 |
|-----------------|-------------------|
| 10              | 51                |
| 18 (our packet) | 69                |
| 51 (max)        | 146               |

**Optimization**: Our 18-byte packet uses 69ms airtime @ DR3
- At 1% duty cycle: Can transmit ~145 times/hour
- At 5% duty cycle: Can transmit ~725 times/hour

## Best Practices

1. **Always use OTAA** (more secure than ABP)
2. **Enable ADR** for automatic optimization
3. **Use binary format** (not JSON) for efficiency
4. **Monitor duty cycle** to prevent limit violations
5. **Implement downlink** for remote management
6. **Use retry logic** for reliability
7. **Log transmission success** rate
8. **Optimize transmission interval** based on use case
9. **Test with different** data rates
10. **Respect duty cycle limits** (legal requirement)

## Integration with Main Firmware

See `src/firmware/mkr_wan_gateway.ino` for complete integration example.
