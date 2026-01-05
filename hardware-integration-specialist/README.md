# Hardware Integration Specialist Agent

## Overview

This agent specializes in hardware integration for Arduino IoT systems, providing expert guidance on connecting, configuring, and troubleshooting multi-device Arduino deployments.

## Purpose

The Hardware Integration Specialist Agent helps you:
- **Plan and execute hardware integration** of multiple Arduino devices
- **Verify compatibility** between components before purchasing or connecting
- **Create proper wiring diagrams** with pin mappings and connections
- **Troubleshoot hardware issues** systematically
- **Optimize power distribution** and communication protocols
- **Select appropriate components** for your specific use case

## Capabilities

### 1. System Architecture Planning
- Analyze your hardware inventory and design optimal integration architecture
- Recommend master-slave vs. distributed configurations
- Plan communication protocols (I2C, SPI, UART, LoRaWAN)
- Design power distribution systems
- Plan for scalability and future expansion

### 2. Compatibility Analysis
- Voltage level compatibility verification
- Communication protocol matching
- Power consumption analysis and budgeting
- Pin availability and conflict detection
- Form factor and physical integration assessment

### 3. Wiring and Connection Design
- Detailed wiring diagrams for your specific setup
- Pin mapping tables for all devices
- I2C/SPI/UART bus configuration
- Power distribution schematics
- Signal integrity considerations

### 4. Component Selection
- Power supply sizing and recommendations
- Required additional components (resistors, capacitors, level shifters)
- Connector and wiring specifications
- Protection components (fuses, diodes)
- Optional peripherals (displays, alarms, sensors)

### 5. Troubleshooting Guidance
- Systematic fault diagnosis procedures
- Common issues and solutions
- Testing and verification procedures
- Debugging techniques for hardware problems
- Recovery procedures for common failures

## Documentation Structure

This agent provides comprehensive documentation in the following files:

### 1. Agent Capabilities
**File**: `/private/var/folders/_f/lqvwt00n4hg98fxhkgg1cfjc0000gn/T/vibe-kanban/worktrees/17ff-/Ideation/hardware-integration-specialist/agent-capabilities.md`

Detailed description of what this agent can do, including:
- Core capabilities and services
- Supported hardware platforms
- Integration phases
- Output formats
- Limitations and boundaries

### 2. Hardware Integration Checklist
**File**: `/private/var/folders/_f/lqvwt00n4hg98fxhkgg1cfjc0000gn/T/vibe-kanban/worktrees/17ff-/Ideation/hardware-integration-specialist/hardware-integration-checklist.md`

Comprehensive step-by-step checklist covering:
- Pre-integration planning (component inventory, power assessment, communication planning)
- Physical integration (power distribution, bus wiring, peripheral connections)
- Verification and testing (power-up, communication, functional tests)
- Optimization and finalization (performance tuning, documentation, safety)

### 3. Initial Guidance and Compatibility Analysis
**File**: `/private/var/folders/_f/lqvwt00n4hg98fxhkgg1cfjc0000gn/T/vibe-kanban/worktrees/17ff-/Ideation/hardware-integration-specialist/initial-guidance-compatibility-analysis.md`

Specific analysis for your system including:
- Voltage compatibility analysis
- Communication protocol compatibility
- Power budget calculations
- Recommended integration architectures (3 options)
- Critical integration considerations
- Component requirements list
- Testing strategy
- Common pitfalls and solutions

### 4. Pin Mapping and Configuration Reference
**File**: `/private/var/folders/_f/lqvwt00n4hg98fxhkgg1cfjc0000gn/T/vibe-kanban/worktrees/17ff-/Ideation/hardware-integration-specialist/pin-mapping-reference.md`

Complete pin mapping documentation for:
- Arduino MKR WAN 1310
- Arduino Nicla Vision
- Arduino Nicla Sense Me
- I2C bus configuration and addressing
- UART configuration
- SPI configuration (if needed)
- GPIO and interrupt assignments
- Power distribution mapping
- Complete system pin map

## Your Current System

Based on your hardware inventory:

### Components
- **2x Arduino Nicla Vision**: AI-enabled cameras with edge ML capabilities
- **1x Arduino Nicla Sense Me**: Multi-sensor environmental monitoring
- **1x Arduino MKR WAN 1310**: LoRaWAN long-range communication

### System Characteristics
- **Voltage**: All devices operate at 3.3V logic levels (fully compatible)
- **Communication**: I2C, UART, SPI available on all devices
- **Power Consumption**:
  - Standby: ~45 mA total
  - Typical operation: ~360 mA total
  - Peak: ~650 mA total
- **Recommended Power Supply**: 5V, 2A minimum

### Key Integration Challenges
1. **I2C Address Conflict**: Both Nicla Vision devices use address 0x42
   - **Solution**: Use TCA9548A I2C multiplexer
2. **Power Requirements**: Peak current exceeds USB port capacity
   - **Solution**: Use dedicated 5V 2A power supply
3. **Communication Architecture**: Need to coordinate multiple devices
   - **Solution**: Use MKR WAN 1310 as master gateway

## Quick Start Guide

### Step 1: Read the Initial Guidance
Start with `/private/var/folders/_f/lqvwt00n4hg98fxhkgg1cfjc0000gn/T/vibe-kanban/worktrees/17ff-/Ideation/hardware-integration-specialist/initial-guidance-compatibility-analysis.md`

This file provides:
- Complete compatibility analysis
- Recommended integration architecture
- Component requirements
- Initial wiring diagrams

### Step 2: Review the Pin Mapping
Reference `/private/var/folders/_f/lqvwt00n4hg98fxhkgg1cfjc0000gn/T/vibe-kanban/worktrees/17ff-/Ideation/hardware-integration-specialist/pin-mapping-reference.md`

This file provides:
- Detailed pin assignments for each device
- I2C addressing and configuration
- UART connection diagrams
- Complete system pin map

### Step 3: Use the Integration Checklist
Follow `/private/var/folders/_f/lqvwt00n4hg98fxhkgg1cfjc0000gn/T/vibe-kanban/worktrees/17ff-/Ideation/hardware-integration-specialist/hardware-integration-checklist.md`

This file provides:
- Step-by-step integration procedures
- Verification tests at each phase
- Safety checks
- Documentation templates

### Step 4: Procure Components
Based on the analysis, you'll need:

**Essential**:
- 5V 2A power supply
- TCA9548A I2C multiplexer module
- Jumper wires (20+ male-to-male, 10+ male-to-female)
- 4.7kΩ resistors (2-4 pieces for I2C pull-ups)
- 1.5-2A fuse and holder
- Mounting plate and standoffs

**Recommended**:
- Status LEDs and resistors
- Power switch
- I2C OLED display (optional)
- Buzzer for alerts (optional)

### Step 5: Start with Minimal Setup
Begin by connecting:
1. MKR WAN 1310 + 1 Nicla Vision
2. Establish basic communication
3. Test LoRaWAN connectivity
4. Verify power consumption

### Step 6: Expand Gradually
After minimal setup works:
1. Add I2C multiplexer
2. Add second Nicla Vision
3. Add Nicla Sense Me
4. Add status indicators and peripherals
5. Test under full load

## Common Scenarios

### Scenario 1: "I want to add a new sensor to my system"
**Agent Response**:
- Analyze sensor specifications (voltage, communication protocol, power requirements)
- Check compatibility with existing system
- Recommend integration method (I2C, SPI, UART, analog)
- Provide wiring diagram
- Update pin mapping if needed

### Scenario 2: "My I2C scanner only detects one device"
**Agent Response**:
- Check for address conflicts (both Nicla Vision at 0x42)
- Verify wiring connections (SDA, SCL, GND)
- Check pull-up resistors
- Recommend I2C multiplexer solution
- Provide troubleshooting steps

### Scenario 3: "Device resets randomly when processing images"
**Agent Response**:
- Analyze power consumption during processing
- Check for voltage drops under load
- Verify power supply capacity
- Recommend adding decoupling capacitors
- Suggest power supply upgrade if needed

### Scenario 4: "LoRaWAN signal is weak or unreliable"
**Agent Response**:
- Check antenna connection and type
- Verify antenna placement (away from metal)
- Check LoRaWAN parameters (frequency, power, data rate)
- Test range and signal strength
- Recommend antenna positioning or upgrade

### Scenario 5: "I need to reduce power consumption for battery operation"
**Agent Response**:
- Analyze current power consumption patterns
- Identify high-power operations (camera processing, LoRa TX)
- Recommend sleep modes and duty cycling
- Suggest power optimization strategies
- Calculate battery life improvements

## Troubleshooting Quick Reference

| Symptom | Likely Cause | Quick Fix |
|---------|--------------|-----------|
| Only one I2C device detected | Address conflict | Use I2C multiplexer |
| Random device resets | Insufficient power | Upgrade power supply to 2A |
| Garbled UART data | Wrong wiring or baud rate | Check TX→RX connection, verify baud |
| Weak LoRa signal | Antenna issue | Check antenna connection, positioning |
| Device not responding | No common ground | Verify all GND pins connected |
| High power consumption | No sleep mode | Implement power management |
| I2C communication errors | Missing pull-ups | Add 4.7kΩ resistors |
| Intermittent operation | Loose connections | Reseat all connectors |

## How to Use This Agent

### For Planning Phase
1. Provide your complete hardware inventory
2. Describe your system requirements and constraints
3. Ask for recommended architecture and components

**Example Query**:
```
I have 2x Nicla Vision, 1x Nicla Sense Me, and 1x MKR WAN 1310.
I need to capture images from both cameras, add sensor data,
and transmit via LoRaWAN. What's the best architecture?
```

### For Integration Phase
1. Reference the documentation files
2. Follow the integration checklist
3. Ask specific questions about wiring or configuration

**Example Query**:
```
How do I connect the TCA9548A multiplexer to my MKR WAN 1310
and both Nicla Vision devices? Show me the exact wiring.
```

### For Troubleshooting Phase
1. Describe the symptom you're experiencing
2. Provide details about your current setup
3. Ask for diagnostic steps

**Example Query**:
```
My MKR WAN 1310 only detects one Nicla Vision on the I2C bus.
I've checked the wiring and connections seem correct.
What else should I check?
```

### For Optimization Phase
1. Describe your current system performance
2. Specify what you want to optimize (power, speed, reliability)
3. Ask for recommendations

**Example Query**:
```
My system consumes 400mA typically. I need to run on battery
for at least 8 hours. What can I do to reduce power consumption?
```

## Technical Support

This agent provides guidance based on:
- Official Arduino documentation and datasheets
- Standard electronics design principles
- Best practices for IoT deployments
- Common integration patterns and solutions

**Limitations**:
- Does not replace professional electrical engineering for safety-critical systems
- Assumes standard operating conditions
- Always verify recommendations with official documentation
- Test thoroughly before deployment

## Additional Resources

### Official Documentation
- Arduino MKR WAN 1310: https://docs.arduino.cc/hardware/mkr-wan-1310
- Arduino Nicla Vision: https://docs.arduino.cc/hardware/nicla-vision
- Arduino Nicla Sense Me: https://docs.arduino.cc/hardware/nicla-sense-me

### Recommended Components
- TCA9548A I2C Multiplexer: Adafruit, SparkFun, Amazon
- 5V 2A Power Supply: Adafruit, SparkFun, Amazon
- Jumper Wires: Adafruit, SparkFun, Amazon
- Assorted Resistors: Local electronics store, online retailers

### Learning Resources
- Arduino I2C Communication: Arduino Wire library documentation
- LoRaWAN: The Things Network documentation
- IoT Hardware Design: Various online tutorials and courses

## Version History

- **v1.0** (2026-01-05): Initial agent creation with comprehensive documentation
  - Agent capabilities definition
  - Hardware integration checklist
  - Initial guidance and compatibility analysis
  - Complete pin mapping reference

## Feedback and Improvements

To improve this agent's capabilities:
1. Report any inaccuracies in documentation
2. Suggest additional hardware platforms to support
3. Share successful integration configurations
4. Request additional capabilities or features

---

**Last Updated**: 2026-01-05
**Agent Version**: 1.0
**Supported Hardware**: Arduino MKR series, Nicla series, expandable to other Arduino platforms
