# IoT Multi-Sensor System with Kepware Integration

A complete IoT multi-sensor system featuring dual camera object detection, environmental monitoring, LoRaWAN communication, and SCADA integration via Kepware OPC-UA.

## System Overview

```
┌─────────────────────────────────────────────────────────────────────────────────┐
│                          Complete IoT System Architecture                       │
├─────────────────────────────────────────────────────────────────────────────────┤
│                                                                                 │
│  ┌──────────────┐    ┌──────────────┐    ┌──────────────┐    ┌──────────────┐  │
│  │   Nicla     │    │   Nicla     │    │   Nicla     │    │   MKR WAN    │  │
│  │   Vision 1  │    │   Vision 2  │    │   Sense Me  │    │   1310       │  │
│  │              │    │              │    │              │    │              │  │
│  │  • Camera   │    │  • Camera   │    │  • Temp     │    │  • LoRaWAN  │  │
│  │  • TinyML   │    │  • TinyML   │    │  • Humid     │    │  • WiFi     │  │
│  │  • Object   │    │  • Object   │    │  • Pressure  │    │  • Gateway  │  │
│  │    Detect   │    │    Detect   │    │  • Gas       │    │              │  │
│  └──────┬───────┘    └──────┬───────┘    └──────┬───────┘    └──────┬───────┘  │
│         │                   │                   │                   │          │
│         └───────────────────┴───────────────────┴───────────────────┘          │
│                                 Serial/I2C                                        │
│                                                                                 │
│  ┌──────────────────────────────────────────────────────────────────────────┐  │
│  │                     Data Pipeline (Node.js)                              │  │
│  │  • SQLite Database  • REST API  • WebSocket  • Real-time Dashboard      │  │
│  └───────────────────────────────────────┬──────────────────────────────────┘  │
│                                          │                                     │
│                                          │ OPC-UA                              │
│                                          ▼                                     │
│  ┌──────────────────────────────────────────────────────────────────────────┐  │
│  │                      Kepware KEPServerEX                                │  │
│  │  • Sensor Tags  • Detection Tags  • Alarm Tags  • Control Tags          │  │
│  └───────────────────────────────────────┬──────────────────────────────────┘  │
│                                          │                                     │
│                                          ▼                                     │
│  ┌──────────────────────────────────────────────────────────────────────────┐  │
│  │                          SCADA / HMI                                     │  │
│  │  • Ignition  • InduSoft  • Wonderware  • FactoryTalk                     │  │
│  └──────────────────────────────────────────────────────────────────────────┘  │
│                                                                                 │
└─────────────────────────────────────────────────────────────────────────────────┘
```

## Hardware Components

| Component | Qty | Purpose |
|-----------|-----|---------|
| Arduino Nicla Vision | 2 | Dual camera object detection with TinyML |
| Arduino Nicla Sense Me | 1 | Environmental sensing (temp, humidity, pressure, gas) |
| Arduino MKR WAN 1310 | 1 | LoRaWAN + WiFi gateway |
| TCA9548A I2C Multiplexer | 1 | Enable dual cameras on single I2C bus |
| 5V 2A Power Supply | 1 | Power for all components |

**Total Cost:** ~$308 USD

## Software Components

### Embedded Firmware (Arduino)
- `src/firmware/mkr_wan_gateway.ino` - Central hub with I2C multiplexer control
- `src/firmware/nicla_vision_camera.ino` - Camera initialization and ML inference
- `src/firmware/nicla_sense_sensors.ino` - Environmental sensor reading

### Backend (Node.js)
- `src/backend/data_pipeline.js` - Main data pipeline server
- `src/kepware/kepware_opcua_client.js` - OPC-UA client for Kepware
- `src/kepware/kepware_integration.js` - Integration manager

### Communication
- `src/lorawan/lorawan_implementation.h` - LoRaWAN OTAA/ABP implementation
- `src/lorawan/downlink_handler.cpp` - Downlink command processing

### Vision/ML
- `src/vision/dual_camera_ml.cpp` - Dual camera control with TinyML

### Dashboard
- `src/dashboard/index.html` - Real-time monitoring dashboard
- `src/dashboard/styles.css` - Dark theme styling
- `src/dashboard/dashboard.js` - Interactive logic

## Features

### 1. Dual Camera Object Detection
- Simultaneous monitoring from 2 cameras
- TensorFlow Lite Micro for on-device ML
- Object classes: Person, Vehicle, Animal
- Real-time detection in ~150ms
- Automatic alarm triggering

### 2. Environmental Monitoring
- Temperature (±0.5°C accuracy)
- Humidity (±2% accuracy)
- Barometric pressure
- Gas resistance (air quality)

### 3. LoRaWAN Communication
- OTAA authentication
- Range up to 10km
- Binary packet format (5x efficiency vs JSON)
- Duty cycle management (1% rule)
- Adaptive data rate

### 4. SCADA Integration (NEW!)
- **OPC-UA connectivity** to Kepware KEPServerEX
- **Real-time tag updates** for all sensor data
- **Bidirectional communication** (HMI/SCADA can control IoT system)
- **Alarm integration** with SCADA systems
- **Compatible with major HMI/SCADA**: Ignition, InduSoft, Wonderware, FactoryTalk

### 5. Web Dashboard
- Real-time sensor data visualization
- 24-hour trend charts
- Detection feed with confidence scores
- System status monitoring
- Responsive dark theme design

## Quick Start

### Prerequisites

```bash
# Node.js dependencies
npm install

# Arduino IDE (for firmware)
# - Install Arduino SAMD boards (v1.8.13+)
# - Install WiFiNINA, LoRa, MKRWAN libraries
```

### Backend Setup

```bash
# Start the data pipeline
npm start

# Or with Kepware integration
KEPWARE_ENABLED=true npm start

# Test Kepware connection
npm test
```

### Dashboard Access

Open browser to: `http://localhost:3000`

## Kepware Integration

### Configuration

Create tags in Kepware following the structure in `src/kepware/KEPWARE_SETUP_GUIDE.md`:

```
Channel1
└── IoTGateway
    ├── NiclaSense_Temperature
    ├── NiclaSense_Humidity
    ├── NiclaSense_Pressure
    ├── Camera1_DetectedClass
    ├── Camera1_Confidence
    ├── Camera2_DetectedClass
    ├── Camera2_Confidence
    ├── Alarm_Active
    ├── Alarm_Severity
    └── Control_SystemEnable (read from HMI)
```

### API Endpoints

| Endpoint | Method | Description |
|----------|--------|-------------|
| `/api/kepware/status` | GET | Connection status |
| `/api/kepware/connect` | POST | Connect to Kepware |
| `/api/kepware/tags` | GET | Read all tags |
| `/api/kepware/write` | POST | Write to tag |
| `/api/kepware/control` | GET | Read control tags |
| `/api/kepware/alert/clear` | POST | Clear alarm |

### Supported SCADA Systems

- **Ignition SCADA** - OPC-UA native support
- **InduSoft Web Studio** - OPC-UA driver
- **Wonderware System Platform** - OPC-UA DA Server
- **FactoryTalk View SE** - OPC-UA topic
- **WinCC Unified** - OPC-UA connectivity

## Project Structure

```
src/
├── hardware/          # Wiring diagrams, components, power budget
├── firmware/          # Arduino sketches (.ino)
├── lorawan/           # LoRaWAN implementation
├── vision/            # ML and camera control
├── backend/           # Data pipeline server
├── dashboard/         # Web UI (HTML/CSS/JS)
├── kepware/           # OPC-UA integration
│   ├── kepware_opcua_client.js
│   ├── kepware_integration.js
│   ├── KEPWARE_SETUP_GUIDE.md
│   └── test_connection.js
└── docs/              # Setup guide, blog post
```

## Documentation

- [Hardware Setup Guide](src/hardware/wiring_diagram.md)
- [Kepware Integration Guide](src/kepware/KEPWARE_SETUP_GUIDE.md)
- [Firmware Guide](src/firmware/README.md)
- [LoRaWAN Guide](src/lorawan/lorawan_guide.md)
- [Setup Tutorial](docs/setup-guide.md)
- [Blog Post](docs/blog-post.md)

## Performance Specifications

| Metric | Value |
|--------|-------|
| Detection latency | <2 seconds |
| Detection accuracy | >80% |
| Environmental accuracy | ±0.5°C, ±2% RH |
| LoRa transmission success | >95% |
| System uptime | >95% over 7 days |
| Power consumption (active) | 200-500mA |
| Power consumption (sleep) | 5-50mA |
| Battery life (5min duty) | >24 hours |

## Agent Ecosystem

This project includes 5 specialized agents for development:

1. **IoT Planning Agent** - Top-down project guidance
2. **Visual Documentation Agent** - Diagram and guide generation
3. **SEO Blog Agent** - Content optimization for technical blogs
4. **Code Review Agent** - Arduino/IoT code review
5. **Hardware Integration Agent** - Wiring and compatibility guidance

See `IOT_PLANNING_README.md` for details.

## Contributing

Contributions welcome! Please:
1. Fork the repository
2. Create a feature branch
3. Make your changes
4. Submit a pull request

## License

MIT License - Free to use and modify

## Acknowledgments

- Arduino for the Nicla series boards
- Kepware for OPC-UA connectivity
- TensorFlow Lite for Microcontrollers
- The open-source IoT community

---

**Version:** 1.1.0 (with Kepware Integration)
**Last Updated:** 2025-01-05
**Status:** Production Ready
