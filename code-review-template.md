# Arduino/IoT Code Review Request Template

Use this template when submitting code for review to the Arduino/IoT Review Agent. Copy the entire template and fill in the relevant sections.

---

## Project Information

**Project Name:** [Your project name]

**Hardware Platform:**
- [ ] Arduino Uno
- [ ] Arduino Nano
- [ ] Arduino Mega
- [ ] Nicla Sense Me
- [ ] Nicla Vision
- [ ] ESP32
- [ ] Other: [specify]

**Communication Method:**
- [ ] LoRaWAN (specify module: ___)
- [ ] WiFi
- [ ] Bluetooth
- [ ] Serial/USB
- [ ] None (local logging only)

**Power Source:**
- [ ] Battery (specify capacity: ___ mAh)
- [ ] USB/Mains powered
- [ ] Solar + Battery
- [ ] Other: [specify]

**Sensors Used:**
- List all sensors with model numbers and what they measure

**Deployment Context:**
- [ ] Indoor/Outdoor/Both
- [ ] Expected operating temperature range
- [ ] Expected battery life (if battery powered)
- [ ] Data transmission frequency

---

## Code Files

Please provide your code files. For each file, include the complete source.

### Main Sketch (.ino file)
```cpp
// Paste your main .ino file here

```

### Header Files (.h)
```cpp
// Paste any header files here

```

### Source Files (.cpp)
```cpp
// Paste any additional .cpp files here

```

### Configuration Files

**platformio.ini (if using PlatformIO):**
```ini
# Paste your platformio.ini here

```

**library.properties (if creating a library):**
```properties
# Paste your library.properties here

```

---

## Current Issues

**What problems are you experiencing?**
- [ ] Code doesn't compile
- [ ] Device crashes/reboots
- [ ] Sensors not working correctly
- [ ] Communication failures
- [ ] Battery drains too quickly
- [ ] Memory issues
- [ ] Other: [describe]

**How long does battery currently last?** [If applicable]

**Error messages or serial output:**
```
// Paste any error messages or debug output here

```

---

## Review Focus Areas

What would you like the review to focus on? (Check all that apply)

**Priority Areas:**
- [ ] Memory management (getting out of memory errors)
- [ ] Power optimization (battery life too short)
- [ ] Sensor integration (readings incorrect or unstable)
- [ ] LoRaWAN/WiFi communication (connection issues)
- [ ] Code quality and best practices
- [ ] Performance optimization
- [ ] Error handling and reliability
- [ ] Security concerns

**Specific Concerns:**
- List any specific concerns or questions about your code

---

## Additional Information

**Development Environment:**
- Arduino IDE version: [e.g., 1.8.19, 2.x]
- PlatformIO version: [if applicable]
- Board package version: [if known]

**Libraries Used:**
List all non-standard libraries with versions:
- Library name: version
- Library name: version

**Schematic/Wiring:**
```
// Describe or paste a simple wiring diagram
// Example:
// Sensor SDA -> Arduino A4 (SDA)
// Sensor SCL -> Arduino A5 (SCL)
// etc.
```

**Constraints:**
- Any size constraints (code size, RAM usage)
- Any timing constraints (must read sensor every X ms)
- Any regional restrictions (LoRaWAN frequency, duty cycle)

---

## Goals

**What are you trying to achieve?**
- Describe the main goal of your project

**What does success look like?**
- How will you know the code is working correctly?

**What are your metrics?**
- Expected battery life
- Expected data accuracy
- Expected transmission success rate

---

## Example Filled Template

### Project Information

**Project Name:** Environmental Monitor for Greenhouse

**Hardware Platform:**
- [x] Nicla Sense Me

**Communication Method:**
- [x] LoRaWAN (module: built-in SX1262)

**Power Source:**
- [x] Battery (specify capacity: 2000 mAh Li-Po)

**Sensors Used:**
- BME688: Temperature, Humidity, Pressure, Gas (Air Quality)
- Built-in RGB: Light intensity

**Deployment Context:**
- [x] Indoor
- Expected operating temperature: 10-40°C
- Expected battery life: 6+ months
- Data transmission frequency: Every 15 minutes

### Current Issues

**What problems are you experiencing?**
- [x] Battery drains too quickly
- [x] LoRaWAN join sometimes fails

**How long does battery currently last?**
Approximately 3 days with 2000mAh battery

**Error messages or serial output:**
```
Joining LoRaWAN... failed
Joining LoRaWAN... failed
Joining LoRaWAN... joined!
Sent: T=23.5°C, H=65.2%
```

### Review Focus Areas

**Priority Areas:**
- [x] Power optimization (battery life too short)
- [x] LoRaWAN/WiFi communication (connection issues)
- [x] Error handling and reliability

**Specific Concerns:**
- Is my sensor reading approach correct?
- Should I use deep sleep? How do I implement it?
- How do I handle LoRaWAN join failures better?
- Am I using too much memory?

### Goals

**What are you trying to achieve?**
Monitor greenhouse environmental conditions and send data to LoRaWAN gateway for remote monitoring and alerts.

**What does success look like?**
- Device sends temperature, humidity, air quality, and light every 15 minutes
- Battery lasts 6+ months
- Device recovers automatically from communication failures
- Data is accurate within sensor specifications

**What are your metrics?**
- Expected battery life: 180+ days
- Expected data accuracy: ±0.5°C, ±3% RH
- Expected transmission success rate: >95%

---

## Before Submitting

**Please ensure you have:**
- [ ] Included ALL code files needed to compile
- [ ] Removed or noted any sensitive credentials (API keys, passwords)
- [ ] Included library names and versions
- [ ] Described the hardware setup
- [ ] Specified what you want reviewed
- [ ] Provided current code status (working, partially working, not working)

**Code Review Best Practices:**
1. Be specific about your concerns
2. Provide context about your project goals
3. Include error messages and symptoms
4. Share what you've already tried
5. Ask questions, even if they seem basic

---

## What to Expect from Review

The Arduino/IoT Review Agent will provide:

1. **Critical Issues** - Must-fix problems that can cause crashes, memory issues, or hardware damage
2. **High Priority** - Important improvements for reliability, performance, and battery life
3. **Medium Priority** - Code quality improvements and maintainability
4. **Low Priority** - Optional optimizations and enhancements

For each issue, you'll receive:
- Location in code (file:line)
- Explanation of why it's a problem
- Recommended fix with code example
- Impact on your project (memory, power, reliability)

You'll also get:
- Memory usage analysis
- Power consumption estimate
- Battery life calculation (if applicable)
- Specific recommendations for your hardware

---

**Ready to submit?**
- Copy this template
- Fill in your information
- Include your code files
- Submit to the Arduino/IoT Review Agent

Good luck with your IoT project!
