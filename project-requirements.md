# Multi-Sensor IoT Monitoring System - Project Requirements

## Project Overview

**System Name**: Intelligent Multi-Sensor Monitoring and Alert System

**Project Vision**: Build a comprehensive IoT system that combines vision-based detection, environmental monitoring, and long-range communication for security and environmental tracking applications.

## Core Objectives

### Primary Goals
1. Implement dual-camera vision system for object detection and alarming
2. Monitor environmental conditions (temperature, humidity) with real-time transmission
3. Establish LoRaWAN communication for long-range data transmission
4. Enable WiFi connectivity for over-the-air (OTA) system updates
5. Create centralized data reception and display system

### Success Criteria
- [ ] Both Nicla Vision boards detect predefined objects (person, vehicle, animal) with >80% accuracy
- [ ] Detection triggers local alarms (LED/buzzer) within 2 seconds
- [ ] Environmental data (temperature, humidity) transmitted every 5 minutes via LoRa
- [ ] LoRaWAN communication achieves 1km+ range in open area
- [ ] System receives OTA updates via WiFi without physical access
- [ ] Central hub displays real-time data from all sensors

## Hardware Components

### Vision Subsystem
**Component**: Arduino Nicla Vision (x2)

**Specifications**:
- Microcontroller: STM32H747 (dual-core: Cortex-M7 @ 480MHz + Cortex-M4 @ 240MHz)
- Camera: OmniVision OV5640, 2MP, 480x480 RGB565
- Storage: 16MB Flash, 512KB SRAM
- Connectivity: WiFi, Bluetooth LE
- Built-in components: RGB LED, microphone, IMU

**Capabilities Required**:
- Object detection using pre-trained models (person, vehicle, animal)
- Real-time image processing at 15-30 FPS
- Local alarm triggering (RGB LED patterns)
- Serial communication to transmit detection events
- Low-power sleep mode with interrupt-based wake

**Roles**:
- **Nicla Vision #1**: Primary zone monitoring (e.g., entrance, perimiter)
- **Nicla Vision #2**: Secondary zone monitoring (e.g., specific area, backup angle)

### Environmental Subsystem
**Component**: Arduino Nicla Sense Me

**Specifications**:
- Microcontroller: nRF52840 (64MHz, 1MB Flash, 256KB RAM)
- Sensors:
  - Temperature: BME688 (-40 to 85°C, ±0.5°C accuracy)
  - Humidity: BME688 (0-100% RH, ±2% accuracy)
  - Pressure: BME688 (300-1100 hPa)
  - Gas: BME688 (VOC detection)
  - Motion: BMM150 (9-axis IMU)
- Connectivity: Bluetooth LE, optimized for low power
- Battery support: Li-Po charging circuit

**Capabilities Required**:
- Read temperature, humidity, pressure at configurable intervals
- Implement BSEC algorithm for high accuracy readings
- Low-power operation (sleep between readings)
- Serial communication to transmit sensor data
- Motion-based wake-up capability (optional)

### Communication Subsystem
**Component**: Arduino MKR WAN 1310

**Specifications**:
- Microcontroller: SAMD21 Cortex-M0+ (48MHz, 256KB Flash, 32KB RAM)
- LoRa Module: Murata ABZ (LoRaWAN 1.0.3, EU868/US915)
- Connectivity: LoRaWAN, WiFi (through u-blox NINA-W10 module)
- Power: 5V DC or Li-Po battery
- Certifications: FCC, CE, IC, ANATEL

**Capabilities Required**:
- Receive data from both Nicla Vision boards via Serial
- Receive data from Nicla Sense Me via Serial
- Transmit aggregated data via LoRaWAN to gateway
- Connect to WiFi for OTA updates
- Implement proper LoRaWAN class (A/C for bidirectional)
- Error handling and data buffering
- Power management for battery operation

**Network Architecture**:
- **LoRaWAN**: For periodic sensor data transmission (long-range, low-power)
- **WiFi**: For OTA firmware updates and configuration (high-bandwidth, short-range)

## System Architecture

### Data Flow Architecture

```
┌─────────────────┐     Serial      ┌──────────────────┐
│  Nicla Vision 1 │ ──────────────> │                  │
│  (Zone A)       │   Detection     │                  │
│                 │   Events        │                  │
└─────────────────┘                 │                  │
                                   │                  │
┌─────────────────┐     Serial      │  MKR WAN 1310    │     LoRa     ┌──────────────┐
│  Nicla Vision 2 │ ──────────────> │  (Central Hub)   │ ───────────> │ LoRa Gateway │
│  (Zone B)       │   Detection     │                  │   Sensor     │              │
│                 │   Events        │                  │   Data       │   TTN/Local  │
└─────────────────┘                 │                  │              │              │
                                   │                  │              └──────────────┘
┌─────────────────┐     Serial      │                  │                     │
│ Nicla Sense Me  │ ──────────────> │                  │                     │
│ (Environment)   │   Sensor Data   │                  │                     │
└─────────────────┘                 └──────────────────┘                     │
                                                                                │
                                                                                ▼
                                                                        ┌──────────────┐
                                                                        │ Cloud/Local  │
                                                                        │ Dashboard    │
                                                                        └──────────────┘

WiFi (OTA Updates)
    │
    ▼
┌─────────────────┐
│  WiFi Router    │
└─────────────────┘
```

### Component Communication Matrix

| From Component | To Component | Protocol | Data Type | Frequency | Priority |
|----------------|--------------|----------|-----------|-----------|----------|
| Nicla Vision 1 | MKR WAN 1310 | Serial (UART) | Detection event | On event | High (interrupt) |
| Nicla Vision 2 | MKR WAN 1310 | Serial (UART) | Detection event | On event | High (interrupt) |
| Nicla Sense Me | MKR WAN 1310 | Serial (UART) | Sensor readings | Every 5 min | Medium |
| MKR WAN 1310 | LoRa Gateway | LoRaWAN | Aggregated data | Every 5 min | Medium |
| WiFi Router | MKR WAN 1310 | WiFi (MQTT/HTTP) | OTA firmware | On demand | Low (background) |

### Power Management Architecture

**Power Sources**:
- Nicla Vision boards: USB power or Li-Po battery (1400mAh typical)
- Nicla Sense Me: Li-Po battery (500mAh typical)
- MKR WAN 1310: USB power or Li-Po battery (2000mAh typical)

**Power Optimization Strategies**:
1. **Nicla Vision**: Sleep when not detecting, wake on timer or motion
2. **Nicla Sense Me**: Deep sleep between sensor readings, wake on timer
3. **MKR WAN 1310**: Sleep between LoRa transmissions, receive interrupts

**Estimated Power Consumption**:
- **Active Mode (all boards)**: ~400-500mA total
- **Monitoring Mode (1 vision active, others sleeping)**: ~200-300mA
- **Sleep Mode (all sleeping)**: ~50-100mA total (with periodic wake)

**Battery Life Estimates** (2000mAh battery):
- Continuous monitoring: ~4-6 hours
- Duty-cycled (5 min wake): ~24-48 hours
- Optimized sleep (1 wake/hour): ~3-5 days

## Software Architecture

### Nicla Vision Firmware
**Platform**: Arduino IDE 2.0 + EdgeML/OpenMV

**Core Functions**:
- Camera initialization and configuration
- Object detection model loading and inference
- Serial communication protocol implementation
- LED alarm patterns
- Power management (sleep/wake)

**Libraries Required**:
- `Arduino_AdvancedFFT` (for motion detection)
- `Arduino_OV5640` (camera driver)
- `Arduino_TensorFlowLite` or `EdgeML` (ML inference)
- `Arduino_LowPower` (power management)

### Nicla Sense Me Firmware
**Platform**: Arduino IDE 2.0 + Arduino_BHY2

**Core Functions**:
- Sensor initialization (BME688 + IMU)
- BSEC algorithm integration
- Serial data transmission
- Low-power sleep configuration
- Data buffering

**Libraries Required**:
- `Arduino_BHY2` (sensor driver)
- `Arduino_LowPower` (power management)
- `Arduino_JSON` (data formatting)

### MKR WAN 1310 Firmware
**Platform**: Arduino IDE 2.0 + Mbed OS

**Core Functions**:
- Multi-port Serial communication (3 ports)
- Data aggregation and buffering
- LoRaWAN stack management
- WiFi connectivity for OTA
- MQTT/HTTP client for cloud integration

**Libraries Required**:
- `ArduinoLoRa` (LoRa communication)
- `MKRWAN` (LoRaWAN stack)
- `WiFiNINA` (WiFi connectivity)
- `ArduinoOTA` (OTA updates)
- `ArduinoJson` (JSON serialization)

## Communication Protocols

### Serial Protocol (UART)

**Baud Rate**: 115200 (all boards)

**Data Format**: JSON messages for flexibility

**Nicla Vision → MKR WAN**:
```json
{
  "source": "vision_1",
  "timestamp": 1234567890,
  "detection": {
    "class": "person",
    "confidence": 0.92,
    "bbox": [100, 150, 200, 300]
  },
  "alarm_triggered": true
}
```

**Nicla Sense Me → MKR WAN**:
```json
{
  "source": "sense_me",
  "timestamp": 1234567890,
  "sensors": {
    "temperature": 23.5,
    "humidity": 65.2,
    "pressure": 1013.25,
    "gas_resistance": 45000
  }
}
```

### LoRaWAN Protocol

**Configuration**:
- **Frequency**: EU868 or US915 (based on region)
- **Spreading Factor**: SF7 (fast) or SF10 (long range)
- **TX Power**: 14dBm (EU) or 20dBm (US)
- **Class**: A (default) or C (continuous receive for OTA)

**Payload Format** (CayenneLPP for efficiency):
```
{
  "detection_count": 2,
  "last_detection": "person",
  "temperature": 23.5,
  "humidity": 65.2,
  "battery_level": 85
}
```

### WiFi/OTA Protocol

**Configuration**:
- **Protocol**: MQTT over TLS or HTTPS
- **Authentication**: Token-based or certificate
- **Update Server**: Arduino Cloud or custom HTTP server
- **Update Mechanism:**
  1. Check for updates on WiFi connection
  2. Download firmware binary
  3. Verify integrity (checksum/signature)
  4. Apply update and reboot

## Integration Points

### Hardware Integration
1. **Physical Mounting**:
   - Nicla Vision boards mounted at strategic angles
   - Nicla Sense Me placed in representative location
   - MKR WAN 1310 centrally located for antenna access

2. **Wiring Connections**:
   - Serial TX/RX connections between boards
   - Common ground reference
   - Independent power supplies or shared with sufficient capacity
   - LoRa antenna properly connected (SMA connector)

3. **Antenna Placement**:
   - LoRa antenna: External, elevated position
   - WiFi antenna: Internal PCB antenna (ensure clearance)
   - Keep antennas separated to avoid interference

### Software Integration
1. **Time Synchronization**:
   - All boards maintain synchronized clocks
   - Use NTP via WiFi or LoRa downlink messages

2. **Data Coordination**:
   - Timestamps in UTC for consistency
   - Message sequence numbers for ordering
   - Acknowledgment protocol for critical messages

3. **Error Handling**:
   - Retry logic for failed transmissions
   - Watchdog timers for system recovery
   - Graceful degradation (e.g., continue local operation if LoRa fails)

## Testing and Validation Requirements

### Component-Level Testing
- [ ] Nicla Vision: Test object detection with various lighting conditions
- [ ] Nicla Vision: Verify alarm LED activation and timing
- [ ] Nicla Sense Me: Validate sensor accuracy against reference
- [ ] MKR WAN 1310: Test Serial communication with all connected boards
- [ ] MKR WAN 1310: Verify LoRa transmission to gateway
- [ ] MKR WAN 1310: Test OTA update process

### Integration Testing
- [ ] Test data flow from Nicla Vision → MKR WAN → LoRa Gateway
- [ ] Test data flow from Nicla Sense Me → MKR WAN → LoRa Gateway
- [ ] Verify message aggregation and buffering
- [ ] Test system behavior under high load (simulated multiple detections)
- [ ] Test WiFi OTA update while LoRa operational
- [ ] Test power consumption in all modes

### System-Level Testing
- [ ] End-to-end test: Detection → LoRa → Dashboard
- [ ] Range testing: Verify 1km+ LoRa communication
- [ ] Battery life testing: Monitor drain over 24 hours
- [ ] Recovery testing: Simulate failures and verify restart
- [ ] Environmental testing: Outdoor conditions (rain, sun, temp extremes)

## Constraints and Limitations

### Technical Constraints
- **LoRa Bandwidth**: Limited to ~50 bytes per message, restricts image transmission
- **Vision Processing**: On-device ML limits model complexity and accuracy
- **Power**: Battery life requires duty cycling, not continuous operation
- **Range**: LoRa range affected by terrain, buildings, interference
- **WiFi**: Short range, requires infrastructure access

### Resource Constraints
- **Memory**: Limited RAM on all boards, requires efficient code
- **Storage**: Limited flash space for ML models and firmware
- **Processing**: Edge ML inference slower than cloud-based alternatives

### Knowledge Requirements
- **C++ Programming**: For Arduino firmware development
- **Electronics Basics**: Understanding of serial communication, power, grounding
- **Networking**: LoRaWAN concepts, WiFi, MQTT/HTTP
- **ML Basics**: Understanding of object detection, model training/deployment

### Timeline Considerations
- **Beginner**: 8-12 weeks for full implementation
- **Intermediate**: 4-6 weeks for full implementation
- **Advanced**: 2-3 weeks for full implementation

## Success Metrics

### Functional Metrics
- Object detection accuracy: >80% on test set
- Detection latency: <2 seconds from event to alarm
- Environmental accuracy: ±0.5°C temperature, ±2% humidity
- LoRa transmission success: >95% packets delivered
- OTA update success: 100% (verified by checksum)
- System uptime: >95% over 7-day test period

### Performance Metrics
- Battery life: >24 hours with 5-minute duty cycle
- LoRa range: >1km in open terrain
- WiFi OTA update: <5 minutes for 500KB firmware
- Memory usage: <80% RAM, <90% Flash
- Boot time: <10 seconds from power to operational

### Learning Metrics
- Complete Phase 1 (single board): Week 1-2
- Complete Phase 2 (multi-board communication): Week 3-4
- Complete Phase 3 (full integration): Week 5-6
- Complete Phase 4 (testing and optimization): Week 7-8

## Project Deliverables

### Documentation
1. System architecture diagrams
2. Wiring schematics and pin maps
3. Code documentation and comments
4. Setup and configuration guide
5. Troubleshooting guide
6. User manual for operation

### Software
1. Nicla Vision firmware (with ML models)
2. Nicla Sense Me firmware
3. MKR WAN 1310 firmware
4. LoRaWAN integration scripts
5. Dashboard code (web-based)

### Hardware
1. Assembled and tested system
2. 3D printed mounting brackets (optional)
3. Custom wiring harnesses
4. Enclosure (weatherproof if outdoor)

### Testing
1. Component test results
2. Integration test reports
3. Performance benchmarks
4. Power consumption measurements
5. Range test results
