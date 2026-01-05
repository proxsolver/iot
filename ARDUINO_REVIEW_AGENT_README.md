# Arduino/IoT Code Review Agent

A specialized code review agent designed for Arduino and embedded IoT projects, with expertise in memory-constrained microcontrollers, sensor integration, LoRaWAN communication, and power optimization.

## 📚 Documentation Contents

This agent specification includes comprehensive documentation for reviewing Arduino and IoT code:

1. **[arduino-iot-review-agent.md](./arduino-iot-review-agent.md)** - Complete agent specification and review criteria
2. **[arduino-code-examples.md](./arduino-code-examples.md)** - Before/after code examples with detailed explanations
3. **[arduino-review-checklist.md](./arduino-review-checklist.md)** - Quick reference checklist for code reviews
4. **[code-review-template.md](./code-review-template.md)** - Template for submitting code for review

## 🎯 What This Agent Does

The Arduino/IoT Code Review Agent specializes in:

### 1. Memory Management Analysis
- Detects memory leaks, heap fragmentation, and buffer overflows
- Analyzes stack usage and identifies stack overflow risks
- Optimizes RAM and Flash usage for microcontroller constraints
- Validates String class usage (major source of memory issues)

### 2. Sensor Integration Review
- Validates Arduino Nicla series sensors (Sense Me, Vision)
- Checks sensor initialization, calibration, and data reading
- Reviews timing and synchronization for multiple sensors
- Analyzes sensor fusion and data processing

### 3. Communication Protocol Validation
- **LoRaWAN**: OTAA/ABP join procedures, duty cycle compliance, payload optimization
- **WiFi**: Connection management, retry logic, error handling
- **Serial/I2C/SPI**: Protocol validation and error recovery
- Ensures robust communication in real-world conditions

### 4. Power Optimization
- Identifies power-wasting patterns
- Implements sleep modes (deep sleep, light sleep)
- Optimizes sensor reading frequency
- Calculates battery life estimates
- Suggests hardware-specific optimizations

### 5. Code Quality Assessment
- Arduino and embedded C++ best practices
- Non-blocking code patterns
- Error handling robustness
- Watchdog timer implementation
- Code organization and maintainability

## 🚀 Quick Start

### For Users New to IoT Development

1. **Read the Quick Checklist** ([arduino-review-checklist.md](./arduino-review-checklist.md))
   - Start with the critical issues section
   - Understand common pitfalls before coding

2. **Review the Examples** ([arduino-code-examples.md](./arduino-code-examples.md))
   - See common mistakes and their fixes
   - Learn production-ready patterns
   - Copy good practices into your code

3. **Prepare Your Code for Review**
   - Use the [code-review-template.md](./code-review-template.md)
   - Include all relevant files
   - Describe your hardware and goals

4. **Submit for Review**
   - Share your code with specific concerns
   - Ask questions about anything unclear
   - Iterate on feedback

### For Experienced Developers

- Reference the full [agent specification](./arduino-iot-review-agent.md) for detailed review criteria
- Use the [checklist](./arduino-review-checklist.md) for self-review before submission
- Review [examples](./arduino-code-examples.md) for optimization patterns

## 🎓 Learning Path

### Beginner (New to Arduino/IoT)

**Start Here:**
1. Read "Common Pitfalls" in the main agent document
2. Review the "Before/After" examples to understand issues
3. Use the checklist when writing your first code

**Focus Areas:**
- Memory safety (avoid String class)
- Non-blocking timing (millis() instead of delay())
- Basic error handling
- Sensor reading validation

### Intermediate (Some Experience, Deploying Projects)

**Focus Areas:**
- LoRaWAN best practices
- Power optimization basics
- Sleep mode implementation
- Communication reliability
- Code organization

**Common Goals:**
- Get projects working reliably
- Improve battery life
- Handle communication failures
- Structure code for maintenance

### Advanced (Production Deployments)

**Focus Areas:**
- Advanced power optimization
- Security implementation
- Over-the-air updates
- Remote monitoring
- Fleet management

**Common Goals:**
- Deploy hundreds of devices
- Achieve months of battery life
- Remote configuration and updates
- Production-grade reliability

## 📋 Hardware Specializations

### Arduino Nicla Sense Me
- BHI260AP sensor hub
- BME688 environmental sensor (temp, humidity, pressure, gas)
- BMM150 magnetometer
- Color/light sensor
- Integrated LoRaWAN (SX1262)

**Specific Review Areas:**
- Sensor fusion configuration
- Gas sensor heater profiles
- Calibration data management
- Low-power sensor operation

### Arduino Nicla Vision
- Camera interface (OV5640)
- ML inference engine (Edge Impulse)
- SD card storage
- Built-in WiFi/BLE

**Specific Review Areas:**
- Frame buffer memory management
- ML model optimization
- Image data handling
- Storage wear leveling

### LoRaWAN Devices
- OTAA/ABP activation
- Duty cycle compliance
- Regional parameter adherence
- Adaptive Data Rate (ADR)
- Downlink handling

## 🔍 Review Process

### What Gets Reviewed

**Code Quality:**
- Memory safety and efficiency
- Hardware interface correctness
- Communication protocol implementation
- Error handling robustness
- Code organization and maintainability

**Performance:**
- Power consumption
- Memory usage
- Timing correctness
- Data throughput

**Reliability:**
- Error recovery
- Watchdog implementation
- Communication retry logic
- State machine correctness

### What You Get

1. **Categorized Issues**
   - Critical: Must fix before deployment
   - High: Should fix for reliability
   - Medium: Important for quality
   - Low: Nice to have improvements

2. **Specific Recommendations**
   - Exact code locations (file:line)
   - Why each issue matters
   - How to fix it with code examples
   - Impact on your project

3. **Analysis & Metrics**
   - Memory usage breakdown
   - Power consumption estimate
   - Battery life calculation
   - Communication performance

## 💡 Common Issues We Solve

### Memory Issues
- "My device crashes after a few hours"
- "String class causing heap fragmentation"
- "Not enough RAM for my project"
- "Random restarts or lockups"

### Power Issues
- "Battery only lasts a few days"
- "How do I implement deep sleep?"
- "Device drains battery even when idle"
- "Need longer battery life"

### Communication Issues
- "LoRaWAN join keeps failing"
- "Can't connect to WiFi"
- "Sensor data not getting through"
- "How do I handle downlink messages?"

### Sensor Issues
- "Sensor readings are wrong"
- "How do I calibrate sensors?"
- "Multiple sensors interfere with each other"
- "Sensor initialization fails randomly"

## 📊 Typical Results

### Memory Optimization
- Typical reduction: 30-50% RAM usage
- Eliminate heap fragmentation issues
- Prevent stack overflows
- Enable more features in same hardware

### Power Optimization
- Typical improvement: 5-10x battery life
- From days to months of operation
- Proper sleep mode implementation
- Adaptive power management

### Reliability Improvements
- Reduce communication failures by 90%+
- Automatic error recovery
- Graceful degradation
- Production-grade robustness

## 🛠️ Tools and Resources

### Supported Development Environments
- Arduino IDE (1.x and 2.x)
- PlatformIO
- Arduino CLI

### Supported Hardware
- Arduino Uno, Nano, Mega (AVR)
- Arduino MKR series (SAMD)
- Arduino Nicla series (SAMD)
- ESP32/ESP8266
- Other ARM Cortex-M0/M4 boards

### Supported Libraries
- Arduino LoRaWAN
- Arduino_BHY2 (Nicla sensors)
- Arduino LowPower
- Standard Arduino libraries
- Most common sensor libraries

## 🔗 Additional Resources

### Official Documentation
- [Arduino Language Reference](https://www.arduino.cc/reference/en/)
- [LoRaWAN Specification](https://lora-alliance.org/resource_hub/lorawan-specification-v1-0-4/)
- [The Things Network Docs](https://www.thethingsnetwork.org/docs/)

### Learning Resources
- [Arduino Official Tutorials](https://www.arduino.cc/en/Tutorial/)
- [Embedded Systems Best Practices](https://www.embedded.com/)
- [IoT Development Guide](https://www.mdpi.com/2079-9292/10/10/1102)

### Hardware Reference
- [Nicla Sense Me Docs](https://docs.arduino.cc/hardware/nicla-sense-me)
- [Nicla Vision Docs](https://docs.arduino.cc/hardware/nicla-vision)
- [SAMD21 Datasheet](https://www.microchip.com/wwwproducts/en/ATSAMD21J18)

## 📞 Getting Started with a Review

To have your Arduino/IoT code reviewed:

1. **Prepare your submission** using [code-review-template.md](./code-review-template.md)
2. **Include all relevant code files** (.ino, .cpp, .h files)
3. **Describe your hardware** and what you're trying to achieve
4. **Specify your concerns** (memory, power, reliability, etc.)
5. **Submit for review**

You'll receive:
- Detailed analysis of your code
- Categorized issues by priority
- Specific code fixes
- Optimization suggestions
- Performance estimates

---

**Last Updated:** January 2025
**Version:** 1.0
**Maintained For:** Arduino and IoT developers building production-grade embedded systems
