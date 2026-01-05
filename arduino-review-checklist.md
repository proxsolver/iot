# Arduino/IoT Code Review Quick Reference Checklist

## 🚨 CRITICAL ISSUES (Must Fix Before Deployment)

### Memory Safety
- [ ] **No String class usage** - Use char arrays, snprintf, or StringSumHelper
- [ ] **Buffer bounds checked** - All array accesses validated
- [ ] **No recursion** - Can cause stack overflow on MCUs
- [ ] **Dynamic allocation controlled** - No new/delete in continuous loops
- [ ] **Stack size safe** - Large arrays declared globally, not on stack
- [ ] **Memory within limits** - Check compilation report: RAM < 80%, Flash < 90%

### Hardware Safety
- [ ] **Pin conflicts resolved** - No pins used multiple times
- [ ] **Pin modes set correctly** - INPUT/OUTPUT/INPUT_PULLUP appropriate
- [ ] **No short circuits** - Output pins not connected directly to ground/VCC
- [ ] **Current limiting** - LEDs have resistors, outputs can handle load
- [ ] **Voltage levels compatible** - 3.3V vs 5V devices matched

### Communication Critical
- [ ] **LoRaWAN properly joined** - Join confirmed before sending
- [ ] **Credentials not hardcoded** - Use secure storage or provisioning
- [ ] **Timeouts implemented** - All I/O operations have timeouts
- [ ] **Error handling present** - Return values checked, failures handled

### Power Critical (Battery Devices)
- [ ] **Sleep mode implemented** - Deep sleep for battery-powered devices
- [ ] **No infinite loops** - No while(1) without yield() or sleep
- [ ] **Watchdog enabled** - For reliability and hang recovery

---

## ⚠️ HIGH PRIORITY (Should Fix)

### Performance
- [ ] **Non-blocking delays** - Replace delay() with millis() timers
- [ ] **Float operations minimized** - Use fixed-point on no-FPU MCUs
- [ ] **Efficient data types** - Use uint8_t, int16_t instead of int when possible
- [ ] **Sensor reads optimized** - Batch reads, don't poll continuously
- [ ] **LoRaWAN payload optimized** - Binary vs ASCII, compress data

### Code Robustness
- [ ] **All returns checked** - Every function that can fail has error handling
- [ ] **Graceful degradation** - System continues if one sensor fails
- [ ] **State machine for complexity** - No deeply nested if/else
- [ ] **Retry logic present** - Exponential backoff for failed operations
- [ ] **Input validation** - Sensor values checked for reasonable ranges

### LoRaWAN Best Practices
- [ ] **OTAA preferred** - Use OTAA unless ABP specifically needed
- [ ] **Duty cycle tracking** - Respect regional limits (EU: 1%)
- [ ] **ADR enabled** - Adaptive Data Rate for better network performance
- [ ] **Confirmed messages** - For critical data only
- [ ] **Session persistence** - Save session keys for quick rejoin

### Code Organization
- [ ] **Constants defined** - No magic numbers
- [ ] **Functions small** - Each function < 50 lines
- [ ] **Clear naming** - Descriptive variable and function names
- [ ] **Comments present** - Complex logic explained
- [ ] **Modular design** - Separate files for different functionality

---

## 📊 MEDIUM PRIORITY (Important)

### Maintainability
- [ ] **Configuration centralized** - All #defines at top of file or config.h
- [ ] **Hardware documentation** - Pin mapping, wiring guide
- [ ] **Dependencies listed** - All libraries with versions
- [ ] **Debug output optional** - Can be disabled for production
- [ ] **Version tracking** - Firmware version accessible

### Debugging Support
- [ ] **Error logging** - Errors logged with timestamps
- [ ] **Diagnostic mode** - Can output detailed system state
- [ ] **Performance monitoring** - Loop time, memory usage
- [ ] **LED indicators** - Status feedback (power, comm, errors)

### Testing Support
- [ ] **Unit testable** - Pure functions that can be tested separately
- [ ] **Mock hardware** - Can simulate sensors for testing
- [ ] **Test points defined** - Known-good states for validation

---

## 💡 LOW PRIORITY (Nice to Have)

### Optimizations
- [ ] **PROGMEM used** - Constant data stored in flash, not RAM (AVR)
- [ ] **Compiler optimizations** - -Os for size, -O3 for speed
- [ ] **Register variables** - For frequently accessed variables
- [ ] **Lookup tables** - For calculations instead of runtime math

### Advanced Features
- [ ] **OTA updates** - Over-the-air firmware update capability
- [ ] **Remote config** - Parameters adjustable via downlink
- [ ] **Statistics** - Packet success rate, error counts
- [ ] **Battery monitoring** - Voltage measurement, low-battery handling

---

## 🔍 Nicla Sensor Specific Checks

### Nicla Sense Me
- [ ] **BHY2 properly initialized** - Check return code
- [ ] **Sensor hub configured** - Virtual sensors enabled
- [ ] **Calibration loaded** - Magnetometer hard-iron, accelerometer offsets
- [ ] **Sample rate appropriate** - Not too fast (wastes power) or too slow
- [ ] **BME688 heater profile** - Gas sensor configured correctly

### Nicla Vision
- [ ] **Camera buffer managed** - Frame buffer fits in RAM
- [ ] **JPEG quality set** - Balance quality vs size
- [ ] **ML model size checked** - Fits in available memory
- [ ] **SD card handling** - Error checking, wear leveling
- [ ] **Power budget calculated** - Camera + WiFi + MCU current

---

## 📡 LoRaWAN Specific Checks

### Join Process
- [ ] **DevEUI unique** - Each device has unique ID
- [ ] **AppEUI correct** - Matches application server
- [ ] **AppKey secure** - Stored safely, not in source code
- [ ] **Join timeout set** - Don't wait forever
- [ ] **Join retry with backoff** - Exponential delay between attempts
- [ ] **Session saved** - Session keys stored for quick reconnect

### Transmission
- [ ] **Payload size minimal** - < 51 bytes for most DR
- [ ] **Data rate appropriate** - DR0-5 based on distance
- [ ] **TX power limited** - Respect local regulations
- [ ] **Channel mask set** - Use correct channels for region
- [ ] **Duty cycle tracked** - Enforce limits in software

### Downlink Handling
- [ ] **RX1/RX2 windows** - Correct timing for responses
- [ ] **Port filtering** - Only process expected ports
- [ ] **Command validation** - Check downlink commands before execution
- [ ] **Action confirmation** - Acknowledge configuration changes

---

## ⚡ Power Consumption Checklist

### Active Mode
- [ ] **CPU clock appropriate** - Not faster than needed
- [ ] **Unused peripherals off** - Turn off ADC, SPI, I2C when not used
- [ ] **Sensors sleep between reads** - Don't poll continuously
- [ ] **WiFi minimal** - Connect, send, disconnect quickly
- [ ] **No busy loops** - Use sleep instead of while waiting

### Sleep Mode
- [ ] **Deepest sleep used** - Deep sleep vs light sleep vs idle
- [ ] **Wake sources configured** - RTC, GPIO, external interrupt
- [ ] **State persisted** - Save volatile state before sleep
- [ ] **Fast wake supported** - Sensors reinitialize quickly
- [ ] **Sleep duration optimized** - Balance responsiveness vs power

### Measurement
- [ ] **Current measured** - Profile active and sleep current
- [ ] **Battery life calculated** - Estimate based on usage
- [ ] **Low-battery handling** - Graceful shutdown or reduced operation
- [ ] **Battery voltage monitored** - Regular voltage checks

### Power Budget Template
```
Device: [Name]
Battery: [mAh capacity]

Current Consumption:
- Active (CPU + Sensors): [mA] for [ms]
- Active + LoRaWAN TX: [mA] for [ms]
- Light Sleep: [µA]
- Deep Sleep: [µA]

Daily Cycle:
- Readings per day: [N]
- Transmissions per day: [N]
- Active time per day: [seconds]
- Sleep time per day: [seconds]

Battery Life Calculation:
- Daily charge = (Active_mA * Active_sec/3600) + (Sleep_mA * Sleep_sec/3600)
- Battery life = Battery_mAh / Daily_charge_mA = [days]
```

---

## 🔒 Security Checklist

### Credentials
- [ ] **No hardcoded secrets** - Keys, passwords in secure storage
- [ ] **Unique device keys** - Each device has unique credentials
- [ ] **Key rotation support** - Can update keys remotely
- [ ] **Secure provisioning** - Keys loaded securely during manufacturing

### Communication
- [ ] **Encryption enabled** - For sensitive data
- [ ] **Message integrity** - Checksums or HMAC
- [ ] **Replay protection** - Sequence numbers or timestamps
- [ ] **Secure boot** - Firmware signature validation

### Data Protection
- [ ] **Sensitive data minimized** - Only collect what's needed
- [ ] **Data sanitization** - Remove credentials from logs
- [ ] **Secure storage** - Flash encrypted if available

---

## 🧪 Testing Checklist

### Unit Testing
- [ ] **Sensor functions tested** - Validate reading logic
- [ ] **Communication tested** - Mock LoRaWAN/WiFi
- [ ] **Error paths tested** - Simulate failures
- [ ] **Edge cases tested** - Min/max values, boundary conditions

### Integration Testing
- [ ] **Full cycle tested** - Sensor → Process → Transmit
- [ ] **Recovery tested** - Power loss, communication failure
- [ ] **Long-run tested** - Run for 24+ hours to check stability
- [ ] **Field tested** - Test in actual deployment environment

### Hardware Testing
- [ ] **Power profiled** - Measure current in all states
- [ ] **Temperature tested** - Verify at min/max operating temps
- [ ] **Vibration tested** - For mobile applications
- [ ] **EMC tested** - Verify no interference issues

---

## 📋 Review Process

### Initial Review (15-30 minutes)
1. Check all CRITICAL items
2. Review memory usage
3. Verify basic functionality
4. Identify major issues

### Detailed Review (1-2 hours)
1. Check HIGH priority items
2. Analyze code quality
3. Review error handling
4. Check power optimization
5. Validate communication logic

### Deep Review (2-4 hours)
1. Check MEDIUM priority items
2. Review testing coverage
3. Analyze security
4. Review documentation
5. Suggest optimizations

---

## 📊 Common Metrics

### Memory Limits by Board
```
Arduino Uno:      2KB RAM,   32KB Flash
Arduino Nano:     2KB RAM,   32KB Flash
Arduino Mega:     8KB RAM,   256KB Flash
Nicla Sense Me:   64KB RAM,  256KB Flash
Nicla Vision:     512KB RAM, 2MB Flash
ESP32:            520KB RAM, 4MB Flash
```

### Typical Current Consumption
```
Deep Sleep:       1-10 µA
Light Sleep:      100-500 µA
Active (CPU):     5-15 mA
Active + Sensors: 15-30 mA
LoRaWAN TX:       100-150 mA (peak)
WiFi TX:          200-300 mA (peak)
```

### LoRaWAN Duty Cycle Limits
```
EU868:            1% (36s/hour transmit max)
US915:            No limit, but fair use expected
AS923:            1%
AU915:            No limit
```

---

## 🎯 Quick Decision Tree

**Is this a battery-powered device?**
- YES → Sleep mode critical, check power optimization
- NO → Focus on performance and responsiveness

**Does it use LoRaWAN?**
- YES → Check duty cycle, join process, payload size
- NO → Check WiFi/BLE connection management

**Are sensors involved?**
- YES → Check initialization, validation, calibration
- NO → Focus on communication and logic

**Is code size > 80% of Flash?**
- YES → Optimize, PROGMEM, reduce features
- NO → Good, room for growth

**Is RAM usage > 70%?**
- YES → Critical: reduce buffers, eliminate String class
- NO → Good margin for stack

---

## 🔧 Common Fixes

### Replace String with Char Array
```cpp
// Before
String msg = "Value: " + String(value);

// After
char msg[32];
snprintf(msg, sizeof(msg), "Value: %d", value);
```

### Replace delay() with millis()
```cpp
// Before
void loop() {
    doSomething();
    delay(1000);
}

// After
unsigned long lastTime = 0;
void loop() {
    if (millis() - lastTime >= 1000) {
        doSomething();
        lastTime = millis();
    }
}
```

### Add Error Checking
```cpp
// Before
int value = sensor.read();
process(value);

// After
int value = sensor.read();
if (value == ERROR_VALUE) {
    handleError();
    return;
}
process(value);
```

### Optimize LoRaWAN Payload
```cpp
// Before: ~20 bytes
String payload = "T:" + String(temp) + ",H:" + String(hum);

// After: 4 bytes
uint8_t payload[4];
payload[0] = (int8_t)temp;
payload[1] = (uint8_t)hum;
```

---

## 📚 Additional Resources

### Learning Materials
- [Arduino Official Tutorials](https://www.arduino.cc/en/Tutorial/)
- [LoRaWAN for Developers](https://www.lorawandeveloper.com/)
- [Embedded Systems Best Practices](https://www.embedded.com/)

### Reference
- [SAMD21 Datasheet](https://www.microchip.com/wwwproducts/en/ATSAMD21J18)
- [SX1262 Datasheet (LoRa)](https://www.semtech.com/products/wireless-rf/lora-transceivers/sx1262)
- [BME688 Datasheet](https://www.bosch-sensortec.com/products/environmental-sensors/gas-sensors-bme688/)

### Tools
- [PlatformIO](https://platformio.org/) - IDE and build system
- [Arduino IDE](https://www.arduino.cc/en/software) - Official IDE
- [The Things Network](https://www.thethingsnetwork.org/) - LoRaWAN network
- [Cayenne](https://mydevices.com/cayenne/) - IoT dashboard

---

Last Updated: January 2025
Version: 1.0

This checklist complements the main Arduino/IoT Review Agent documentation.
Use both together for comprehensive code reviews.
