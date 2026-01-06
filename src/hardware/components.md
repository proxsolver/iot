# Component Specifications - IoT Multi-Sensor System

## Bill of Materials (BOM)

### Microcontrollers

| Component | Quantity | Specs | Purpose | Link |
|-----------|----------|-------|---------|------|
| Arduino MKR WAN 1310 | 1 | SAMD21 Cortex-M0+ 32-bit, 256KB Flash, 32KB SRAM, LoRaWAN 868/915MHz, WiFi | Central hub, LoRaWAN gateway | [Link](https://store.arduino.cc/products/arduino-mkr-wan-1310) |
| Arduino Nicla Vision | 2 | STM32H747, 2MB Flash, 1MB SRAM, 5MP camera, ML acceleration, BLE | Dual camera object detection | [Link](https://store.arduino.cc/products/arduino-nicla-vision) |
| Arduino Nicla Sense Me | 1 | nRF52840, 1MB Flash, 256KB SRAM, BSEC library, BLE | Environmental sensors | [Link](https://store.arduino.cc/products/arduino-nicla-sense-me) |

### Communication Modules

| Component | Quantity | Specs | Purpose | Link |
|-----------|----------|-------|---------|------|
| TCA9548A I2C Multiplexer | 1 | 8-channel, 3.3V/5V, I2C 400kHz | Dual camera I2C switching | [Link](https://www.ti.com/product/TCA9548A) |
| LoRaWAN Antenna | 1 | 868MHz or 915MHz, 2dBi gain | LoRaWAN communication | [Link](https://store.arduino.cc/products/antenna-dipole-antenna-lora) |
| WiFi Antenna | 1 | 2.4GHz, U.FL connector | High-bandwidth data | [Link](https://store.arduino.cc/products/antenna-u.fl) |

### Power System

| Component | Quantity | Specs | Purpose | Link |
|-----------|----------|-------|---------|------|
| 5V 2A Power Supply | 1 | DC 5V, 2A output, USB-C or barrel jack | Main power source | [Link](https://www.adafruit.com/product/276) |
| Jumper Wires (M-M) | 20 | 22 AWG, various colors | Internal connections | [Link](https://www.adafruit.com/product/758) |
| Breadboard | 1 | 830 tie points | Prototyping | [Link](https://www.adafruit.com/product/239) |

### Indicators & Alert System

| Component | Quantity | Specs | Purpose | Link |
|-----------|----------|-------|---------|------|
| LED (Red) | 1 | 5mm, 20mA | Alarm indicator | [Link](https://www.adafruit.com/product/297) |
| LED (Green) | 1 | 5mm, 20mA | Status indicator | [Link](https://www.adafruit.com/product/298) |
| Resistor 220Ω | 2 | 1/4W, 5% | LED current limiting | [Link](https://www.adafruit.com/product/2784) |
| Buzzer | 1 | 5V, active buzzer | Audio alarm | [Link](https://www.adafruit.com/product/1536) |

### Enclosure & Mounting

| Component | Quantity | Specs | Purpose | Link |
|-----------|----------|-------|---------|------|
| Project Case | 1 | 200x150x80mm, IP65 | Weather protection | [Link](https://www.adafruit.com/product/1921) |
| M3 Standoffs | 8 | 10mm height, brass | Board mounting | [Link](https://www.adafruit.com/product/1230) |
| Thermal Paste | 1 | 5g tube | Heat dissipation | [Link](https://www.adafruit.com/product/3530) |

## Sensor Specifications

### Arduino Nicla Vision

**Microcontroller:**
- Processor: Dual-core STM32H747
- Core: Cortex-M7 @ 480MHz + Cortex-M4 @ 240MHz
- Memory: 2MB Flash, 1MB SRAM
- Clock: 32.768kHz crystal

**Camera:**
- Sensor: 5MP OV5640
- Resolution: 2592x1944 max
- Frame rate: 30fps @ 720p
- Interface: MIPI CSI-2

**Machine Learning:**
- Hardware acceleration: Ethos-U55
- Framework: TensorFlow Lite for Microcontrollers
- Max model size: 4MB

**Connectivity:**
- BLE 5.2
- WiFi: N/A
- Interfaces: I2C, SPI, UART, USB

**Power:**
- Voltage: 3.3V or 5V (via VIN)
- Current: 150mA typical, 250mA peak
- Sleep mode: < 5mA

### Arduino Nicla Sense Me

**Microcontroller:**
- Processor: nRF52840
- Core: Cortex-M4F @ 64MHz
- Memory: 1MB Flash, 256KB SRAM

**Environmental Sensors:**
- Temperature: BME688 (-40°C to +85°C, ±0.5°C)
- Humidity: BME688 (0-100%, ±3%)
- Pressure: BME688 (300-1100hPa, ±1hPa)
- Gas: BME688 (VOC detection)
- Motion: BMI270 (3-axis accelerometer, 3-axis gyroscope)
- Light: Color sensor (RGB)
- Microphone: MP34DT05 (PDM)

**Connectivity:**
- BLE 5.2
- WiFi: N/A
- Interfaces: I2C, SPI, UART, USB

**Power:**
- Voltage: 3.3V or 5V (via VIN)
- Current: 20mA typical, 50mA peak
- Sleep mode: < 2mA

### Arduino MKR WAN 1310

**Microcontroller:**
- Processor: Microchip SAMD21
- Core: Cortex-M0+ @ 32MHz
- Memory: 256KB Flash, 32KB SRAM

**LoRaWAN Module:**
- Module: Murata Type ABZ
- Frequency: 868MHz (EU) or 915MHz (US)
- Range: Up to 10km (line of sight)
- Data rate: 0.3kbps to 50kbps

**WiFi Module:**
- Module: u-blox NINA-W102
- Standard: 802.11b/g/n
- Frequency: 2.4GHz

**Connectivity:**
- Interfaces: I2C, SPI, UART, USB, 22x GPIO

**Power:**
- Voltage: 3.7V - 5V (via VIN)
- Current: 30mA typical, 150mA peak (LoRa TX)
- Sleep mode: < 5mA

## TCA9548A I2C Multiplexer Specifications

**Electrical Characteristics:**
- Supply Voltage: 2.7V - 5.5V
- Supply Current: < 1mA (active)
- I2C Clock Frequency: Up to 400kHz (Fast Mode)
- I2C Address: 0x70 (default), configurable via A0-A2

**Channel Characteristics:**
- Channels: 8 independent bidirectional channels
- Voltage Translation: Supports mixed voltage levels
- Switching Time: < 1µs
- Channel Isolation: > 100MΩ

**Package:**
- Type: TSSOP-28 or SOIC-28
- Operating Temperature: -40°C to +85°C

**Pin Configuration:**
```
Pin 1-2: A0-A2 (Address select)
Pin 3: SDA (I2C data)
Pin 4: SCL (I2C clock)
Pin 5-12: SC0-SC7 (Channel SCL output)
Pin 13-20: SD0-SD7 (Channel SDA output)
Pin 21-24: A2-A0 (Address select - alternate)
Pin 25: GND
Pin 26: VCC
Pin 27: RESET (Active low)
Pin 28: VCC
```

## Total System Specifications

**Computational Capacity:**
- Total Processing Power: 576MHz + 240MHz + 64MHz + 32MHz = 912MHz
- Total RAM: 1MB + 256KB + 32KB = 1.3MB
- Total Flash: 2MB + 1MB + 256KB = 3.3MB

**Sensing Capabilities:**
- Visual: 2x 5MP cameras
- Temperature: -40°C to +85°C (±0.5°C)
- Humidity: 0-100% (±3%)
- Pressure: 300-1100hPa (±1hPa)
- Gas/Air Quality: VOC index
- Motion: 3-axis accelerometer + gyroscope
- Light: RGB color sensing
- Audio: PDM microphone

**Communication:**
- LoRaWAN: Up to 10km range
- WiFi: 802.11b/g/n
- BLE: 5.2 (2 devices)
- Total Bandwidth: WiFi (high-speed) + LoRa (long-range)

**Power Consumption:**
- Idle: ~50mA
- Normal Operation: ~355mA
- Peak: ~710mA
- Recommended Supply: 5V 2A

**Environmental Rating:**
- Operating Temperature: -20°C to +60°C (with enclosure)
- Humidity: 10-90% (non-condensing)
- IP Rating: IP65 (with proper enclosure)

## Cost Estimate (USD)

| Component | Unit Price | Quantity | Total |
|-----------|------------|----------|-------|
| MKR WAN 1310 | $45 | 1 | $45 |
| Nicla Vision | $70 | 2 | $140 |
| Nicla Sense Me | $45 | 1 | $45 |
| TCA9548A | $3 | 1 | $3 |
| Power Supply | $15 | 1 | $15 |
| Antennas (2x) | $10 | 2 | $20 |
| Indicators | $5 | - | $5 |
| Enclosure | $20 | 1 | $20 |
| Misc (wires, standoffs) | $15 | - | $15 |
| **Total** | | | **$308** |

## Ordering Notes

1. **Frequency Selection**: Choose 868MHz or 915MHz LoRaWAN based on region
2. **Power Supply**: Ensure 5V 2A minimum with appropriate connector
3. **Antennas**: Verify connector type (U.FL or SMA) matches boards
4. **Enclosure**: Consider IP rating for outdoor deployment
5. **Spare Parts**: Order extra jumper wires and standoffs

## Vendor Recommendations

**Primary Sources:**
- Arduino Store (Official boards)
- Adafruit (Accessories, power supply)
- DigiKey (TCA9548A, components)
- Mouser (Enclosures, hardware)

**Alternative Sources:**
- Amazon (Basic components, cables)
- eBay (Enclosures, standoffs)
- AliExpress (Cost savings, longer shipping)
