# Pin Mapping and Configuration Reference

## Arduino MKR WAN 1310 Pin Mapping

### Board Layout and Pin Functions

```
                    ┌─────────────────────┐
                    │   Arduino MKR       │
                    │   WAN 1310          │
                    │                     │
         5V ────────┤ 1                14 ├─────────── VIN
         3.3V ──────┤ 2                13 ├─────────── GND
         ADC0 ──────┤ 3                12 ├─────────── A6/DAC
         ADC1 ──────┤ 4                11 ├─────────── A5
         ADC2 ──────┤ 5                10 ├─────────── A4
         ADC3 ──────┤ 6                 9 ├─────────── A3
         ADC4 ──────┤ 7                 8 ├─────────── A2
                    │                     │
         SDA ───────┤ 0                15 ├─────────── SCL
                    │                     │
         TX ────────┤ 14 (Serial1 TX)  16 ├─────────── RX (Serial1 RX)
         MOSI ──────┤ 8                17 ├─────────── MISO
         SCK ───────┤ 9                18 ├─────────── CS
                    │                     │
         PWM0 ──────┤ 4 (D4)          19 ├─────────── PWM1 (D5)
         PWM2 ──────┤ 6 (D6)          20 ├─────────── PWM3 (D7)
                    └─────────────────────┘
```

### Detailed Pin Reference

| Pin Number | Name | Function | Type | Notes |
|------------|------|----------|------|-------|
| 0 | SDA | I2C Data | Digital/IO | Default I2C SDA, can be GPIO |
| 1 | SCL | I2C Clock | Digital/IO | Default I2C SCL, can be GPIO |
| 2 | D2 | GPIO | Digital/IO | PWM capable |
| 3 | D3 | GPIO | Digital/IO | PWM capable |
| 4 | D4/A6 | GPIO/ADC | Digital/Analog | ADC, PWM capable |
| 5 | D5/A5 | GPIO/ADC | Digital/Analog | ADC, PWM capable |
| 6 | D6/A4 | GPIO/ADC | Digital/Analog | ADC, PWM capable |
| 7 | D7/A3 | GPIO/ADC | Digital/Analog | ADC, PWM capable |
| 8 | MOSI | SPI | Digital/IO | SPI MOSI |
| 9 | SCK | SPI | Digital/IO | SPI Clock |
| 10 | MISO | SPI | Digital/IO | SPI MISO |
| 11 | CS/ADC | SPI/ADC | Digital/Analog | SPI Chip Select or ADC |
| 12 | A2 | ADC | Analog | Analog input only |
| 13 | A1 | ADC | Analog | Analog input only |
| 14 | TX | UART | Digital/IO | Serial1 TX (can be GPIO) |
| 15 | RX | UART | Digital/IO | Serial1 RX (can be GPIO) |
| - | 5V | Power | Power | 5V output (from VIN) |
| - | 3.3V | Power | Power | 3.3V output (regulated) |
| - | GND | Ground | Power | Common ground |
| - | VIN | Power Input | Power | 5-6V input |
| - | RESET | Reset | Digital | Reset button (active low) |

### Reserved Pins
- **Pin 8-11**: SPI bus (reserved for LoRa module)
- **Pin 0-1**: I2C bus (can be used, but check for conflicts)
- **Pin 14-15**: UART (Serial1, often used for communication)

### Recommended Pin Assignments

**For I2C Master**:
- Use Pin 0 (SDA) and Pin 1 (SCL)
- Enable internal pull-ups or add external 4.7kΩ resistors

**For UART Communication**:
- Use Pin 14 (TX) and Pin 15 (RX) for Serial1
- Connect TX → RX (cross-connection to other device)

**For GPIO**:
- Available: Pin 2, 3, 4, 5, 6, 7 (if not using ADC)
- Interrupt-capable: All GPIO pins

**For PWM**:
- Pin 2, 3, 4, 5, 6, 7 (check specific PWM frequency)

## Arduino Nicla Vision Pin Mapping

### Board Layout and Pin Functions

```
                    ┌─────────────────────┐
                    │   Arduino Nicla     │
                    │   Vision            │
                    │                     │
         VIN ───────┤ USB               PWR ├─────────── 3.3V
         GND ───────┤ GND              13/3V3 ├───────── 3.3V
         SDA ───────┤ 0                 1   ├─────────── SCL
         TX ────────┤ 14 (Serial1 TX)   15   ├─────────── RX (Serial1 RX)
                    │                     │
         D2 ────────┤ 2                 12   ├─────────── D12
         D3 ────────┤ 3                 11   ├─────────── D11
         D4 ────────┤ 4                 10   ├─────────── D10
         D5 ────────┤ 5                  9   ├─────────── D9
         D6 ────────┤ 6                  8   ├─────────── D8
         D7 ────────┤ 7                  7   ├─────────── D7
                    │                     │
         A0 ────────┤ A0                 6   ├─────────── A6
         A1 ────────┤ A1                 5   ├─────────── A5
         A2 ────────┤ A2                 4   ├─────────── A4
         A3 ────────┤ A3                 3   ├─────────── A3
                    └─────────────────────┘
```

### Detailed Pin Reference

| Pin Number | Name | Function | Type | Notes |
|------------|------|----------|------|-------|
| 0 | SDA | I2C Data | Digital/IO | I2C SDA, GPIO when not using I2C |
| 1 | SCL | I2C Clock | Digital/IO | I2C SCL, GPIO when not using I2C |
| 2 | D2 | GPIO | Digital/IO | Interrupt-capable |
| 3 | D3 | GPIO | Digital/IO | Interrupt-capable |
| 4 | D4/A6 | GPIO/ADC | Digital/Analog | ADC or GPIO |
| 5 | D5/A5 | GPIO/ADC | Digital/Analog | ADC or GPIO |
| 6 | D6/A4 | GPIO/ADC | Digital/Analog | ADC or GPIO |
| 7 | D7/A3 | GPIO/ADC | Digital/Analog | ADC or GPIO |
| 8 | D8 | GPIO | Digital/IO | Interrupt-capable |
| 9 | D9 | GPIO | Digital/IO | Interrupt-capable |
| 10 | D10 | GPIO | Digital/IO | Interrupt-capable |
| 11 | D11 | GPIO | Digital/IO | Interrupt-capable |
| 12 | D12 | GPIO | Digital/IO | Interrupt-capable |
| 14 | TX | UART | Digital/IO | Serial1 TX |
| 15 | RX | UART | Digital/IO | Serial1 RX |
| A0 | A0 | ADC | Analog | Analog input |
| A1 | A1 | ADC | Analog | Analog input |
| A2 | A2 | ADC | Analog | Analog input |
| A3 | A3 | ADC | Analog | Analog input |
| - | VIN | Power Input | Power | 3.7V-5V input |
| - | 3.3V | Power | Power | 3.3V output |
| - | GND | Ground | Power | Common ground |

### Reserved Pins
- **Camera module**: Internal connections (not accessible)
- **Onboard sensors**: I2C (address 0x42 default)
- **USB**: Serial communication (USB CDC)

### Recommended Pin Assignments

**For I2C Slave**:
- Use Pin 0 (SDA) and Pin 1 (SCL)
- Default address: 0x42 (may be configurable)

**For UART Communication**:
- Use Pin 14 (TX) and Pin 15 (RX) for Serial1
- Connect TX → RX (cross-connection to other device)

**For GPIO**:
- Available: Pin 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12
- Interrupt-capable: All GPIO pins

**For Analog Input**:
- Available: A0, A1, A2, A3 (or D4-D7 in analog mode)

## Arduino Nicla Sense Me Pin Mapping

### Board Layout and Pin Functions

```
                    ┌─────────────────────┐
                    │   Arduino Nicla     │
                    │   Sense Me          │
                    │                     │
         VIN ───────┤ USB               PWR ├─────────── 3.3V
         GND ───────┤ GND              13/3V3 ├───────── 3.3V
         SDA ───────┤ 0                 1   ├─────────── SCL
         TX ────────┤ 14 (Serial1 TX)   15   ├─────────── RX (Serial1 RX)
                    │                     │
         D2 ────────┤ 2                 12   ├─────────── D12
         D3 ────────┤ 3                 11   ├─────────── D11
         D4 ────────┤ 4                 10   ├─────────── D10
         D5 ────────┤ 5                  9   ├─────────── D9
         D6 ────────┤ 6                  8   ├─────────── D8
         D7 ────────┤ 7                  7   ├─────────── D7
                    │                     │
         A0 ────────┤ A0                 6   ├─────────── A6
         A1 ────────┤ A1                 5   ├─────────── A5
         A2 ────────┤ A2                 4   ├─────────── A4
         A3 ────────┤ A3                 3   ├─────────── A3
                    └─────────────────────┘
```

### Detailed Pin Reference

| Pin Number | Name | Function | Type | Notes |
|------------|------|----------|------|-------|
| 0 | SDA | I2C Data | Digital/IO | I2C SDA, GPIO when not using I2C |
| 1 | SCL | I2C Clock | Digital/IO | I2C SCL, GPIO when not using I2C |
| 2 | D2 | GPIO | Digital/IO | Interrupt-capable |
| 3 | D3 | GPIO | Digital/IO | Interrupt-capable |
| 4 | D4/A6 | GPIO/ADC | Digital/Analog | ADC or GPIO |
| 5 | D5/A5 | GPIO/ADC | Digital/Analog | ADC or GPIO |
| 6 | D6/A4 | GPIO/ADC | Digital/Analog | ADC or GPIO |
| 7 | D7/A3 | GPIO/ADC | Digital/Analog | ADC or GPIO |
| 8 | D8 | GPIO | Digital/IO | Interrupt-capable |
| 9 | D9 | GPIO | Digital/IO | Interrupt-capable |
| 10 | D10 | GPIO | Digital/IO | Interrupt-capable |
| 11 | D11 | GPIO | Digital/IO | Interrupt-capable |
| 12 | D12 | GPIO | Digital/IO | Interrupt-capable |
| 14 | TX | UART | Digital/IO | Serial1 TX |
| 15 | RX | UART | Digital/IO | Serial1 RX |
| A0 | A0 | ADC | Analog | Analog input |
| A1 | A1 | ADC | Analog | Analog input |
| A2 | A2 | ADC | Analog | Analog input |
| A3 | A3 | ADC | Analog | Analog input |
| - | VIN | Power Input | Power | 3.7V-5V input |
| - | 3.3V | Power | Power | 3.3V output |
| - | GND | Ground | Power | Common ground |

### Reserved Pins
- **Onboard sensors**: I2C (address 0x28 default)
  - BSEC sensor (gas, air quality)
  - Temperature/humidity sensor
  - Pressure sensor
  - Motion sensor (accelerometer/gyroscope)
- **RGB LED**: Internal control

### Recommended Pin Assignments

**For I2C Slave**:
- Use Pin 0 (SDA) and Pin 1 (SCL)
- Default address: 0x28

**For UART Communication**:
- Use Pin 14 (TX) and Pin 15 (RX) for Serial1
- Connect TX → RX (cross-connection to other device)

**For GPIO**:
- Available: Pin 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12
- Interrupt-capable: All GPIO pins

**For Analog Input**:
- Available: A0, A1, A2, A3 (or D4-D7 in analog mode)

## I2C Bus Configuration

### I2C Address Summary

| Device | Default I2C Address | Configurable? | Notes |
|--------|---------------------|---------------|-------|
| Nicla Vision #1 | 0x42 | Check firmware | Both devices share address |
| Nicla Vision #2 | 0x42 | Check firmware | Must change or use multiplexer |
| Nicla Sense Me | 0x28 | Check firmware | Fixed or configurable |
| TCA9548A Multiplexer | 0x70-0x77 | Yes (via ADDR pins) | 3 address pins |
| OLED Display (SSD1306) | 0x3C or 0x3D | Yes (via resistor) | Check your module |
| RTC (DS3231) | 0x68 | No | Fixed address |

### I2C Pull-up Resistor Configuration

**Standard I2C Pull-up Values**:
- **Standard mode (100kHz)**: 4.7kΩ to 10kΩ
- **Fast mode (400kHz)**: 2.2kΩ to 4.7kΩ
- **Fast mode plus (1MHz)**: 1kΩ to 2.2kΩ

**Recommendation**: Use 4.7kΩ resistors for 100kHz-400kHz operation

**Pull-up Installation**:
```
         3.3V
          │
          ├─────┐
          │     │
         4.7k   │
          │     │
          │    SDA ──────→ All devices
          │     │
         4.7k   │
          │     │
          │    SCL ──────→ All devices
         GND
```

**Note**: Many Arduino boards have internal pull-ups, but they may be too weak (20kΩ-50kΩ). External pull-ups are recommended for reliable operation.

### I2C Bus Topology

**Without Multiplexer** (if address conflict resolved):
```
Master (MKR WAN 1310)
  ├── SDA ────────────────────────────────┐
  ├── SCL ────────────────────────────────┤
  ├── 4.7kΩ pull-ups to 3.3V              │
  └── 4.7kΩ pull-ups to 3.3V              │
                                           │
         ┌─────────────────────────────────┼─────────────────┐
         │                                 │                 │
    ┌────▼─────┐                     ┌─────▼──────┐   ┌─────▼─────┐
    │ Nicla    │                     │ Nicla      │   │ Nicla     │
    │ Vision #1│                     │ Vision #2  │   │ Sense Me  │
    │ (0x42)   │                     │ (0x43?)    │   │ (0x28)    │
    └──────────┘                     └────────────┘   └───────────┘
```

**With TCA9548A Multiplexer** (recommended):
```
Master (MKR WAN 1310)
  ├── SDA ──────→ TCA9548A SDA
  ├── SCL ──────→ TCA9548A SCL
  └── 4.7kΩ pull-ups to 3.3V

TCA9548A Multiplexer (0x70)
  ├── Channel 0 Enable ──→ Channel 0 SDA/SCL ──→ Nicla Vision #1 (0x42)
  ├── Channel 1 Enable ──→ Channel 1 SDA/SCL ──→ Nicla Vision #2 (0x42)
  ├── Channel 2 Enable ──→ Channel 2 SDA/SCL ──→ Nicla Sense Me (0x28)
  └── Channel 3-7 ──→ Available for expansion
```

## UART Configuration

### UART Pin Mapping

**MKR WAN 1310**:
- TX: Pin 14 (Serial1 TX)
- RX: Pin 15 (Serial1 RX)

**Nicla Vision**:
- TX: Pin 14 (Serial1 TX)
- RX: Pin 15 (Serial1 RX)

**Nicla Sense Me**:
- TX: Pin 14 (Serial1 TX)
- RX: Pin 15 (Serial1 RX)

### UART Connection Example

**Point-to-Point Connection** (MKR WAN 1310 ↔ Nicla Vision #1):
```
MKR WAN 1310              Nicla Vision #1
─────────────            ───────────────
Pin 14 (TX)   ─────────→ RX (Pin 15)
Pin 15 (RX)   ←───────── TX (Pin 14)
GND           ─────────→ GND
```

**Important**: Always cross-connect TX to RX and RX to TX

### Recommended UART Configuration

**Baud Rate Options**:
- 9600: Slow but reliable over long distances
- 57600: Good balance of speed and reliability
- 115200: Maximum speed, good for short distances
- Custom: Adjust based on your requirements

**Code Example**:
```cpp
// Master device
Serial1.begin(115200);

// Slave device
Serial1.begin(115200);
```

### Multi-Device UART Configuration

**Option 1: Daisy Chain** (not recommended for more than 2 devices):
```
Device A (TX/RX) ←→ Device B (TX/RX) ←→ Device C (TX/RX)
```

**Option 2: Star Configuration** (requires multiple UART ports):
```
         Device A (Master)
              ↑ TX/RX
              ├─────→ Device B
              ├─────→ Device C
              └─────→ Device D
```

**Option 3: Single Master, Multiple Slaves** (using I2C instead):
- Recommended to use I2C for multi-device communication
- UART reserved for high-speed or point-to-point communication

## SPI Configuration (if needed)

### SPI Pin Mapping

**MKR WAN 1310** (reserved for LoRa module):
- MOSI: Pin 8
- MISO: Pin 10
- SCK: Pin 9
- CS: Pin 11

**Nicla Vision**:
- MOSI: Available (check pinout)
- MISO: Available (check pinout)
- SCK: Available (check pinout)
- CS: Any GPIO pin

**Nicla Sense Me**:
- MOSI: Available (check pinout)
- MISO: Available (check pinout)
- SCK: Available (check pinout)
- CS: Any GPIO pin

### SPI Configuration Recommendations

**Use SPI when**:
- High-speed data transfer required (>1 Mbps)
- Transferring large amounts of data (e.g., camera frames)
- Lower latency needed than I2C

**Avoid SPI when**:
- More than 2-3 devices on bus (complexity increases)
- Simple control/status messages sufficient
- I2C adequate for your data rate

## GPIO and Interrupt Configuration

### Interrupt-Capable Pins

All devices support interrupts on most GPIO pins:
- **MKR WAN 1310**: All digital pins
- **Nicla Vision**: All GPIO pins
- **Nicla Sense Me**: All GPIO pins

### Recommended Interrupt Pin Assignments

**MKR WAN 1310**:
- Pin 2: Data ready interrupt from Nicla Vision #1
- Pin 3: Data ready interrupt from Nicla Vision #2
- Pin 4: Alert interrupt from Nicla Sense Me

**Example Code**:
```cpp
const int VISION1_INT = 2;
const int VISION2_INT = 3;
const int SENSE_INT = 4;

void setup() {
  pinMode(VISION1_INT, INPUT_PULLUP);
  pinMode(VISION2_INT, INPUT_PULLUP);
  pinMode(SENSE_INT, INPUT_PULLUP);

  attachInterrupt(digitalPinToInterrupt(VISION1_INT), vision1ISR, FALLING);
  attachInterrupt(digitalPinToInterrupt(VISION2_INT), vision2ISR, FALLING);
  attachInterrupt(digitalPinToInterrupt(SENSE_INT), senseISR, FALLING);
}

void vision1ISR() {
  // Handle Nicla Vision #1 interrupt
}

void vision2ISR() {
  // Handle Nicla Vision #2 interrupt
}

void senseISR() {
  // Handle Nicla Sense Me interrupt
}
```

## Power Distribution Pin Mapping

### Power Supply Connection

**Recommended Power Distribution**:
```
5V Power Supply (2A)
    │
    ├── [Fuse: 1.5A-2A]
    │
    ├─────────────────────────────────────────┐
    │                                         │
   GND                                      +5V
    │                                         │
    └───┬───┬───┬───┬───┐                    └───┬───┬───┬───┐
        │   │   │   │   │                       │   │   │   │
       GND  GND  GND  GND  GND                5V  5V  5V  5V
        │   │   │   │   │                       │   │   │   │
        │   │   │   │   └─→ MKR WAN 1310 VIN   │   │   │   │
        │   │   │   └─────→ Nicla Vision #1 VIN│   │   │   │
        │   │   └─────────→ Nicla Vision #2 VIN│   │   │   │
        │   └─────────────→ Nicla Sense Me VIN │   │   │   │
        └─────────────────→ [Common Ground]    └───┴───┴───┘
```

### Power Consumption Reference

| Device | Standby | Active | Peak | Notes |
|--------|---------|--------|------|-------|
| Nicla Vision | 30 mA | 150 mA | 250 mA | Camera + ML processing |
| Nicla Sense Me | 5 mA | 30 mA | 50 mA | All sensors active |
| MKR WAN 1310 | 5 mA | 30 mA | 150 mA | LoRa TX at max power |

## Complete System Pin Map

### Master Controller: MKR WAN 1310

```
Assigned Functions:
├─ Power
│  ├─ VIN: 5V input
│  ├─ 5V: Output (not used)
│  ├─ 3.3V: Output (not used)
│  └─ GND: Common ground
│
├─ I2C Bus (Master)
│  ├─ Pin 0 (SDA): I2C SDA
│  └─ Pin 1 (SCL): I2C SCL
│
├─ UART Communication
│  ├─ Pin 14 (TX): Serial1 TX
│  └─ Pin 15 (RX): Serial1 RX
│
├─ Interrupt Inputs
│  ├─ Pin 2: Interrupt from Nicla Vision #1
│  ├─ Pin 3: Interrupt from Nicla Vision #2
│  └─ Pin 4: Interrupt from Nicla Sense Me
│
├─ Status LEDs
│  ├─ Pin 5: System status LED (green)
│  ├─ Pin 6: Communication status LED (yellow)
│  └─ Pin 7: Error status LED (red)
│
└─ Control Outputs
   ├─ Pin 10: Power enable for peripherals
   └─ Pin 11: Buzzer control
```

### Slave Device: Nicla Vision #1

```
Assigned Functions:
├─ Power
│  ├─ VIN: 5V input
│  ├─ 3.3V: Output (not used)
│  └─ GND: Common ground
│
├─ I2C Bus (Slave)
│  ├─ Pin 0 (SDA): I2C SDA
│  └─ Pin 1 (SCL): I2C SCL
│  └─ Address: 0x42
│
├─ Status Indicator
│  └─ Pin 2: Status LED (active)
│
└─ Interrupt Output
   └─ Pin 3: Data ready interrupt to master
```

### Slave Device: Nicla Vision #2

```
Assigned Functions:
├─ Power
│  ├─ VIN: 5V input
│  ├─ 3.3V: Output (not used)
│  └─ GND: Common ground
│
├─ I2C Bus (Slave - via multiplexer)
│  ├─ Pin 0 (SDA): I2C SDA (to multiplexer ch 1)
│  └─ Pin 1 (SCL): I2C SCL (to multiplexer ch 1)
│  └─ Address: 0x42 (on separate bus)
│
├─ Status Indicator
│  └─ Pin 2: Status LED (active)
│
└─ Interrupt Output
   └─ Pin 3: Data ready interrupt to master
```

### Slave Device: Nicla Sense Me

```
Assigned Functions:
├─ Power
│  ├─ VIN: 5V input
│  ├─ 3.3V: Output (not used)
│  └─ GND: Common ground
│
├─ I2C Bus (Slave - via multiplexer)
│  ├─ Pin 0 (SDA): I2C SDA (to multiplexer ch 2)
│  └─ Pin 1 (SCL): I2C SCL (to multiplexer ch 2)
│  └─ Address: 0x28
│
├─ Status Indicator
│  └─ Pin 2: Status LED (active)
│
└─ Interrupt Output
   └─ Pin 3: Data ready interrupt to master
```

## Wiring Color Code Standard

**Recommended Wire Colors**:
- **Red**: 5V power
- **Black**: Ground (GND)
- **White**: 3.3V power
- **Blue**: I2C SDA
- **Yellow**: I2C SCL
- **Green**: UART TX
- **Orange**: UART RX
- **Purple**: GPIO signals
- **Gray**: Interrupt lines

**Example Label**:
- "RED-5V-MAIN": Main 5V power line
- "BLU-SDA-MSTR": I2C SDA from master
- "GRN-TX-MKR": TX line from MKR WAN 1310

## Testing and Verification

### Pin Continuity Testing

**Use multimeter to verify**:
1. All GND pins connected (0-1Ω resistance)
2. Power connections complete (5V present at all VIN pins)
3. I2C connections: SDA connected to SDA, SCL to SCL
4. UART connections: TX → RX cross-connections

### I2C Bus Scanning

**Use I2C scanner sketch**:
```cpp
#include <Wire.h>

void setup() {
  Wire.begin();
  Serial.begin(9600);
  Serial.println("I2C Scanner");
}

void loop() {
  for (byte addr = 1; addr < 127; addr++) {
    Wire.beginTransmission(addr);
    if (Wire.endTransmission() == 0) {
      Serial.print("Found device at 0x");
      Serial.println(addr, HEX);
    }
  }
  delay(5000);
}
```

**Expected Results**:
- Without multiplexer: 0x28, 0x42 (only one Nicla Vision)
- With multiplexer: 0x70 (multiplexer), plus devices on each channel

## Notes

1. Always verify pin assignments with official documentation
2. Some pins may have alternate functions not listed here
3. Test connections with multimeter before powering on
4. Use current-limiting resistors for LEDs (220Ω-1kΩ)
5. Add decoupling capacitors (100nF) near each device's power pins
6. Keep I2C bus wires short (<30cm) for reliable operation
7. Use twisted pair for I2C lines if longer runs needed
8. Label all connections for easier debugging
