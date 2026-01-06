# Deployment Guide - IoT Multi-Sensor System

## Table of Contents
1. [Deployment Overview](#deployment-overview)
2. [Indoor Deployment](#indoor-deployment)
3. [Outdoor Deployment](#outdoor-deployment)
4. [Network Configuration](#network-configuration)
5. [Power Options](#power-options)
6. [Maintenance Procedures](#maintenance-procedures)
7. [Scaling to Multiple Units](#scaling-to-multiple-units)
8. [Security Considerations](#security-considerations)
9. [Troubleshooting Deployment Issues](#troubleshooting-deployment-issues)

---

## Deployment Overview

### Pre-Deployment Checklist

Before deploying your IoT multi-sensor system, ensure you have completed:

- [ ] **System Testing**: All components tested individually and integrated
- [ ] **24-Hour Stability Test**: System ran for 24 hours without failures
- [ ] **Calibration**: Sensors calibrated against reference instruments
- [ ] **Configuration**: WiFi, LoRaWAN, and cloud credentials configured
- [ ] **Documentation**: System settings documented and backed up
- [ ] **Spare Parts**: Fuses, jumper wires, and basic tools on hand
- [ ] **Access Plan**: Physical access for maintenance arranged

### Deployment Scenarios

| Scenario | Environment | Protection Required | Power Source | Network |
|----------|-------------|---------------------|--------------|---------|
| **Home Monitoring** | Indoor | Basic enclosure | Mains power | WiFi |
| **Industrial** | Indoor/Outdoor | IP65 enclosure, ESD protection | Mains + UPS | WiFi/LAN |
| **Agricultural** | Outdoor | IP67 enclosure, weatherproofing | Solar + battery | LoRaWAN only |
| **Remote Monitoring** | Outdoor | IP67 enclosure, tamper-proof | Solar + large battery | LoRaWAN + satellite |

### Site Survey Requirements

Before final deployment, conduct a site survey:

1. **Power Availability**:
   - Nearest power outlet location
   - Voltage stability
   - Backup power options
   - Cable routing requirements

2. **Network Coverage**:
   - WiFi signal strength at deployment location
   - LoRaWAN gateway proximity
   - Cellular coverage (if using backup)
   - Network interference sources

3. **Environmental Conditions**:
   - Temperature range (min/max)
   - Humidity levels
   - Exposure to elements (rain, sun, wind)
   - Dust/debris presence
   - Vibration sources

4. **Physical Constraints**:
   - Mounting surface type
   - Available space
   - Accessibility for maintenance
   - Security requirements

---

## Indoor Deployment

### Location Selection

**Ideal Locations**:
- Central monitoring point
- Near power outlet
- Within WiFi coverage
- Away from direct sunlight
- Good ventilation
- Minimal dust/debris

**Avoid**:
- Near heat sources (radiators, heaters)
- Direct sunlight (causes overheating)
- High humidity areas (bathrooms, kitchens)
- High traffic areas (accidental damage risk)
- Behind metal objects (blocks WiFi/LoRa)

### Mounting Options

#### Option 1: Desk/Shelf Mounting

**Best for**: Home monitoring, temporary installations

**Requirements**:
- Flat, stable surface
- Elevated position (for better camera coverage)
- Near power outlet
- Good airflow

**Installation Steps**:
1. Place system on stable surface
2. Ensure ventilation space (5cm minimum around boards)
3. Route cables neatly
4. Secure with non-slip mat
5. Label cables for easy identification

**Pros**:
- Easy installation
- Good ventilation
- Easy access for maintenance

**Cons**:
- Visible cables
- Accidental damage risk
- Limited placement options

#### Option 2: Wall Mounting

**Best for**: Permanent installations, security monitoring

**Requirements**:
- Wall mount bracket or shelf
- Drills and screws
- Cable routing channels
- Level tool

**Installation Steps**:
1. Select mounting height (camera angle optimal at 2-3m)
2. Mark screw locations
3. Drill pilot holes
4. Install wall anchors
5. Mount bracket
6. Route cables through channels
7. Connect power and network

**Pros**:
- Professional appearance
- Better camera angle
- Protection from accidental damage

**Cons**:
- Permanent installation
- Harder to access for maintenance
- Requires drilling

#### Option 3: Ceiling Mounting

**Best for**: Security monitoring, wide area coverage

**Requirements**:
- Ceiling mount bracket
- Stable ceiling structure
- Cable routing through ceiling
- Professional installation recommended

**Installation Steps**:
1. Locate ceiling joists
2. Install mount bracket to joists
3. Route power cables through ceiling
4. Hang enclosure from bracket
5. Adjust camera angle
6. Test weight capacity

**Pros**:
- Maximum coverage area
- Tamper-resistant
- Cables hidden

**Cons**:
- Difficult installation
- Hard to access for maintenance
- Requires professional help

### Indoor Enclosure Recommendations

**Basic Enclosure** (IP30 - dust protection):
- Material: ABS plastic
- Ventilation: Passive vents
- Mounting: Flanges for screws
- Cost: $10-20

**Recommended Enclosure**:
- Material: ABS or polycarbonate
- IP Rating: IP54 (dust and splash protection)
- Ventilation: Fan or passive vents
- Features: Transparent window for LEDs, cable glands
- Cost: $30-50

**Industrial Enclosure**:
- Material: Metal or polycarbonate
- IP Rating: IP65 (dust-tight, water jet protection)
- Ventilation: Forced air fan
- Features: Lockable door, mounting plate, cable management
- Cost: $80-150

### Indoor Cable Management

**Best Practices**:
1. Use cable trays or channels
2. Label all cables at both ends
3. Keep power cables separate from signal cables
4. Use strain relief at connectors
5. Leave service loop (extra cable) for maintenance
6. Secure cables with zip ties or velcro
7. Avoid sharp bends (<3x cable diameter)

---

## Outdoor Deployment

### Weather Protection Requirements

**Environmental Threats**:
- Rain and moisture
- Temperature extremes
- UV radiation
- Dust and debris
- Insects and rodents
- Tampering/vandalism

**Protection Strategies**:

#### 1. Moisture Protection

**IP65 Enclosure** (minimum for outdoor):
- Dust-tight
- Protection against water jets
- Sealed doors with gaskets
- Cable glands for all penetrations

**IP67 Enclosure** (recommended for harsh environments):
- Dust-tight
- Protection against immersion (15cm-1m depth)
- Additional sealing requirements
- Desiccant packets inside

**Moisture Control Tips**:
- Use silicone sealant on all cable entries
- Add desiccant packets (replace monthly)
- Install breather membrane for pressure equalization
- Position enclosure with door facing away from rain
- Slope roof for water runoff

#### 2. Temperature Management

**Heat Mitigation** (for hot climates):
- Light-colored enclosure (reflects sunlight)
- Sun shield above enclosure
- Forced air ventilation (fan + vent)
- Thermal insulation between boards and enclosure
- Temperature-triggered fan (turns on at 35°C)

**Cold Protection** (for cold climates):
- Insulated enclosure (foam or fiberglass)
- Heater element (thermostatically controlled)
- Heat sink on voltage regulators
- Low-temperature-rated components
- Battery insulation

**Temperature Specifications**:
| Component | Min Temp | Max Temp | Notes |
|------------|----------|----------|-------|
| MKR WAN 1310 | -40°C | +85°C | Industrial rated |
| Nicla Vision | -20°C | +70°C | Condensation risk |
| Nicla Sense Me | -40°C | +85°C | Sensor accuracy affected |
| LoRa module | -40°C | +85°C | Frequency drift possible |

#### 3. UV Protection

**UV Damage Prevention**:
- Use UV-stabilized enclosure materials
- Paint exterior with UV-resistant paint
- Install sun shield or hood
- Position in shaded area if possible
- UV-resistant cable jackets

**Materials to Avoid**:
- Clear plastics (yellow and become brittle)
- Standard PVC cables
- Non-UV-stabilized ABS

**UV-Resistant Materials**:
- Polycarbonate (natural UV resistance)
- Fiberglass
- Powder-coated aluminum
- Stainless steel hardware

### Outdoor Power Options

#### Option 1: Mains Power (Recommended)

**Best for**: Fixed installations with power available

**Requirements**:
- Weatherproof outlet (IP65)
- GFCI protection
- Conduit for cable routing
- Professional installation recommended

**Installation Steps**:
1. Install weatherproof outlet near enclosure
2. Run power cable through conduit
3. Install inline fuse holder
4. Use waterproof connectors
5. Test GFCI functionality
6. Label circuit breaker

**Safety Notes**:
- Use proper grounding
- Install drip loops on cables
- Use UV-resistant cable jackets
- Follow local electrical codes

#### Option 2: Solar + Battery

**Best for**: Remote locations, no mains power available

**Sizing Calculations**:

**System Power Consumption**:
- Average current: 81.5 mA (from testing guide)
- Daily consumption: 1956 mAh
- Voltage: 5V
- Daily energy: 9.78 Wh

**Solar Panel Sizing**:
| Location | Peak Sun Hours | Required Panel Size | Recommended Panel |
|----------|----------------|---------------------|-------------------|
| Sunny (Arizona) | 6-7 hours | 2W | 6V 3W panel |
| Moderate (California) | 4-5 hours | 3W | 6V 5W panel |
| Cloudy (Seattle) | 2-3 hours | 6W | 6V 10W panel |

**Battery Sizing**:
| Autonomy Days | Battery Capacity (at 5V) | Recommended Battery |
|---------------|--------------------------|-------------------|
| 1 day | 2000 mAh | 18650 Li-Ion (1 cell) |
| 3 days | 6000 mAh | 18650 Li-Ion (2 cells) |
| 5 days | 10000 mAh | Li-Po 10000mAh |
| 7 days | 14000 mAh | LiFePO4 12V 10Ah |

**Solar Installation Steps**:
1. Mount solar panel at optimal angle (latitude + 15°)
2. Orient south (northern hemisphere)
3. Connect to solar charge controller
4. Connect battery to charge controller
5. Connect load (IoT system) to controller
6. Install weatherproof enclosure for battery

**Battery Safety**:
- Use LiFePO4 for outdoor (safer chemistry)
- Install battery in separate sealed compartment
- Use battery management system (BMS)
- Temperature monitoring (don't charge below 0°C)
- Vent for gas release (if using lead-acid)

#### Option 3: PoE (Power over Ethernet)

**Best for**: Networked installations, remote from power outlets

**Requirements**:
- PoE injector or switch
- CAT5e/CAT6 outdoor cable
- PoE splitter (5V output)
- Weatherproof cable connections

**Installation Steps**:
1. Install PoE injector near network switch
2. Run outdoor CAT5e/CAT6 cable to enclosure
3. Install waterproof RJ45 connectors
4. Connect to PoE splitter in enclosure
5. Power IoT system from splitter
6. Test voltage under load

**Benefits**:
- Single cable for power and data
- Long cable runs (up to 100m)
- Backup power possible (UPS at injector)

### Outdoor Antenna Placement

#### LoRaWAN Antenna

**Optimal Placement**:
- Outdoor, elevated position
- Away from metal objects
- Line of sight to gateway
- Vertical orientation (most LoRa antennas)

**Mounting Options**:
1. **Pole Mount** (recommended)
   - 1-2m mast
   - Clear area around antenna
   - Lightning protection recommended

2. **Wall Mount**
   - Exterior wall of building
   - Away from corners (multipath interference)
   - Use antenna bracket

3. **Enclosure Mount** (not recommended)
   - Antenna inside enclosure (poor signal)
   - Only for short-range applications

**Lightning Protection**:
- Install lightning arrestor on antenna cable
- Ground antenna mast
- Use shielded antenna cable
- Surge protector on power line
- Disconnect during storms if possible

#### WiFi Antenna

**Placement Guidelines**:
- Near WiFi source
- Away from LoRa antenna (minimum 30cm separation)
- Avoid metal obstructions
- Orient for best signal

**Antenna Options**:
- Standard dipole (included with boards): Short range
- High-gain dipole (5dBi): Medium range
- Outdoor omni-directional: Long range

**Installation**:
- Use U.FL to RP-SMA pigtail
- Mount antenna outside enclosure
- Use waterproof connector
- Keep cable short (<1m) to reduce loss

### Outdoor Camera Placement

**Considerations**:
- Avoid direct sunlight (causes washout and overheating)
- Use sun shield or hood
- Position for desired coverage area
- Protect from rain (eave or overhang)
- Consider night vision lighting

**Mounting Height**:
- Minimum: 2.5m (out of reach)
- Optimal: 3-4m (good angle, tamper-resistant)
- Maximum: 6m (difficult to access/maintain)

**Camera Housing**:
- Use weatherproof camera enclosure
- Silica gel packets for moisture
- Anti-fog coating on lens
- Sun hood above lens

---

## Network Configuration

### WiFi Configuration

#### 2.4GHz vs 5GHz

**Use 2.4GHz** for IoT systems:
- Better range
- Better wall penetration
- Lower power consumption
- All Arduino boards support 2.4GHz only

**Avoid 5GHz**:
- Shorter range
- Poor wall penetration
- Not supported by Arduino boards

#### WiFi Security

**Recommended Configuration**:
- Security mode: WPA2-AES
- Password: Minimum 12 characters, mixed case + numbers
- Network isolation: IoT devices on separate VLAN
- MAC filtering: Optional but not sufficient alone
- Disable WPS (vulnerability)

**Example Configuration**:
```cpp
const char* ssid = "IoT-Network";
const char* password = "SecureP@ssw0rd123";

// Optional: Static IP configuration
IPAddress local_IP(192, 168, 1, 100);
IPAddress gateway(192, 168, 1, 1);
IPAddress subnet(255, 255, 255, 0);
WiFi.config(local_IP, gateway, subnet);
```

#### WiFi Troubleshooting

**Poor Signal Strength**:
- Move closer to router
- Use WiFi antenna with higher gain
- Add WiFi repeater/range extender
- Reduce interference (microwaves, Bluetooth)

**Intermittent Connection**:
- Check for interference sources
- Change WiFi channel (use 1, 6, or 11)
- Update WiFi firmware
- Check power supply stability

**Cannot Connect**:
- Verify SSID and password
- Check 2.4GHz network enabled on router
- Restart WiFi router
- Verify antenna connected

### LoRaWAN Configuration

#### Frequency Selection

| Region | Frequency | Channels | Notes |
|--------|-----------|----------|-------|
| US | 915 MHz | US915 | 64 uplink, 8 downlink |
| EU | 868 MHz | EU868 | 3 channels, 1% duty cycle |
| AU | 915 MHz | AU915 | Similar to US915 |
| AS | 923 MHz | AS923 | Specific to Asian countries |

**Important**: Use correct frequency for your region!

#### Antenna Configuration

**Antenna Types**:
- Dipole (included): Omnidirectional, 2dBi gain
- Monopole: Omnidirectional, 3dBi gain
- Yagi: Directional, 10dBi+ gain
- Panel: Directional, 8-12dBi gain

**Gain vs Coverage**:
| Antenna Gain | Horizontal Pattern | Range | Use Case |
|--------------|-------------------|-------|----------|
| 2dBi (standard) | 360° | 1-2km | General purpose |
| 5dBi | 360° | 3-5km | Open areas |
| 10dBi Yagi | 30° | 10km+ | Point-to-point |

**Antenna Mounting Height**:
- Ground level: 100m-500m range
- 2m elevation: 1-2km range
- 10m elevation: 5-10km range
- Tower (30m+): 15km+ range

#### Gateway Selection

**Public Gateway** (The Things Network):
- Free to use
- Coverage dependent on community gateways
- No infrastructure cost
- Data available via TTN API

**Private Gateway**:
- Full control
- Guaranteed coverage
- Infrastructure cost ($100-500)
- Requires maintenance

**Gateway Recommendations**:
- The Things Network Indoor Gateway (~€150)
- Laird RG1xx (~$200)
- MultiTech Conduit (~$300)
- RAK2245 Pi HAT + Raspberry Pi (~$100)

### Port Forwarding (Remote Access)

**When to Use**: Remote dashboard access, OTA updates

**Router Configuration**:
1. Assign static IP to backend server
2. Forward external port to internal port:
   - External: 3000 → Internal: 3000 (HTTP)
   - External: 8080 → Internal: 8080 (WebSocket)
3. Use HTTPS (reverse proxy) for security
4. Implement authentication

**Security Warning**: Public internet access requires strong security!

### VPN Configuration (Recommended)

**Why Use VPN**:
- Secure remote access
- No port forwarding required
- Encrypted traffic
- Access to local network resources

**VPN Options**:
- WireGuard (recommended, fast)
- OpenVPN (widely supported)
- Tailscale (easy setup)
- ZeroTier (peer-to-peer)

**Example WireGuard Setup**:
1. Install WireGuard on backend server
2. Generate server/Client keys
3. Configure server:
   ```
   [Interface]
   PrivateKey = <server-private-key>
   Address = 10.0.0.1/24
   ListenPort = 51820

   [Peer]
   PublicKey = <client-public-key>
   AllowedIPs = 10.0.0.2/32
   ```
4. Configure client (laptop/phone)
5. Connect and access dashboard at 10.0.0.1:3000

---

## Power Options

### Power Budget Review

| Component | Standby | Active | Peak | Duty Cycle |
|-----------|---------|--------|------|------------|
| MKR WAN 1310 | 5 mA | 30 mA | 150 mA | 100% |
| Nicla Vision #1 | 30 mA | 150 mA | 250 mA | 5% |
| Nicla Vision #2 | 30 mA | 150 mA | 250 mA | 5% |
| Nicla Sense Me | 5 mA | 30 mA | 50 mA | 15% |
| **Total Average** | | | | **~360 mA** |

### Option 1: Mains Power (Recommended for Indoor)

**Requirements**:
- 5V 2A power supply minimum
- Stable voltage (±5%)
- Low ripple (<100mV)
- Overcurrent protection

**Recommended Power Supplies**:
- Mean Well GS25A05-P1J (5V 5A, industrial grade)
- Cincon EFB0505S (5V 1A, compact)
- Adafruit 276 (5V 2A, hobbyist)

**Installation**:
1. Use proper gauge wire (18AWG for 2A)
2. Include fuse or circuit breaker
3. Provide strain relief
4. Label power cables
5. Ground enclosure if metal

**Backup Power**:
- UPS (Uninterruptible Power Supply)
- Recommended: APC BE600M1 (600VA, ~$80)
- Runtime: 30-60 minutes at this load

### Option 2: Battery Power (Portable/Outdoor)

**Battery Chemistry Comparison**:

| Chemistry | Energy Density | Safety | Cost | Temp Range | Best For |
|-----------|----------------|--------|------|------------|----------|
| Li-Ion (18650) | High | Moderate | Moderate | 0-45°C | General use |
| Li-Po | High | Poor | Low | 0-45°C | Lightweight |
| LiFePO4 | Medium | Excellent | High | -20-60°C | Outdoor |
| Lead-Acid | Low | Good | Low | -20-50°C | Large systems |

**LiFePO4 Recommended for Outdoor**:
- Safe chemistry (no thermal runaway)
- 2000+ cycles (5+ years life)
- Wide temperature range
- Built-in BMS available

**Battery Capacity Calculator**:
```
Daily consumption = 1956 mAh
Desired autonomy = X days
Battery capacity = Daily consumption × (Autonomy + 0.2 buffer)

Example for 3 days:
1956 mAh × 3.2 = 6259 mAh (minimum 6,800 mAh battery)
```

**Battery Management**:
- Use BMS (Battery Management System)
- Monitor cell voltages
- Temperature monitoring
- Low voltage cutoff (3.0V per cell)
- Balance charging

### Option 3: Solar + Battery (Remote/Outdoor)

**Solar Panel Selection**:

| Panel Size | Power | Daily Production (5 sun hours) | Best For |
|------------|-------|------------------------------|----------|
| 3W | 6V 3W | 12 Wh | Low power, sunny climates |
| 5W | 6V 5W | 20 Wh | Typical installation |
| 10W | 6V 10W | 40 Wh | Cloudy climates |

**Charge Controller**:
- PWM controller (cheap, less efficient): 70-80% efficiency
- MPPT controller (expensive, efficient): 90-95% efficiency

**Recommended**: MPPT for small systems, worth the extra cost

**Solar Sizing Example**:
```
Daily consumption: 1956 mAh × 3.7V = 7.2 Wh
Location: Seattle (3 peak sun hours)
Required panel: 7.2Wh / 0.9 (MPPT) / 3 hours = 2.7W
Add 50% margin: 2.7W × 1.5 = 4W minimum
Select: 6V 5W panel
```

**Installation Tips**:
- Tilt panel at latitude angle
- Orient south (northern hemisphere)
- Avoid shading (even partial shading causes big losses)
- Clean panel regularly
- Check mounting stability

### Power Monitoring

**Measure Power Consumption**:
```cpp
// Inline power monitor example
void setup() {
  Serial.begin(115200);
}

void loop() {
  float voltage = readVCC();
  float current = readCurrent();
  float power = voltage * current;

  Serial.print("Power: ");
  Serial.print(power);
  Serial.println(" mW");

  delay(60000); // Every minute
}
```

**Battery Monitoring**:
```cpp
// Simple voltage monitor
void checkBattery() {
  float batteryVoltage = analogRead(A0) * (5.0 / 1024.0) * 2.0; // With divider

  if (batteryVoltage < 3.3) {
    sendAlert("Low battery!");
    enterLowPowerMode();
  }
}
```

**Power Conservation Tips**:
- Use sleep modes between readings
- Reduce transmission frequency
- Lower WiFi transmission power
- Disable unused peripherals
- Use adaptive duty cycling

---

## Maintenance Procedures

### Daily Checks (Automated)

**System Monitoring Script**:
```javascript
// Check system health every hour
setInterval(async () => {
  // Check data reception
  const lastData = await getLastDataTime();
  if (Date.now() - lastData > 3600000) {
    sendAlert('No data received in 1 hour!');
  }

  // Check battery level
  const battery = await getBatteryLevel();
  if (battery < 20) {
    sendAlert('Battery low!');
  }

  // Check storage space
  const storage = await getStorageUsage();
  if (storage > 90) {
    sendAlert('Storage almost full!');
  }
}, 3600000); // Every hour
```

### Weekly Maintenance

**Visual Inspection**:
1. Check enclosure for damage
2. Verify antenna connections secure
3. Check cable integrity
4. Look for water ingress or condensation
5. Verify LED indicators normal

**Performance Checks**:
1. Review system logs for errors
2. Check data reception rate
3. Verify LoRaWAN success rate >95%
4. Test alarm system
5. Check battery trend

**Documentation**:
1. Log maintenance activities
2. Record any anomalies
3. Update calibration if needed
4. Backup database
5. Check firmware updates

### Monthly Maintenance

**Deep Cleaning**:
1. Clean camera lenses (compressed air)
2. Check for dust/debris in enclosure
3. Verify ventilation clear
4. Inspect all cable connections
5. Replace desiccant packets

**Sensor Calibration**:
1. Compare readings to reference sensors
2. Calibrate temperature if drift >0.5°C
3. Calibrate humidity if drift >3%
4. Record calibration values

**Battery Maintenance** (if applicable):
1. Check battery voltage under load
2. Inspect for swelling/damage
3. Clean battery terminals
4. Test backup power (UPS)
5. Cycle battery if needed

**Software Updates**:
1. Check for firmware updates
2. Update ML model with new data
3. Review and update security credentials
4. Update backend dependencies
5. Test updates on backup system first

### Quarterly Maintenance

**Comprehensive Testing**:
1. Full system diagnostic
2. Performance benchmarking
3. Range testing for LoRaWAN
4. Stress testing (high detection rate)
5. Backup and restore testing

**Hardware Inspection**:
1. Check for corrosion
2. Verify thermal paste condition
3. Inspect solder joints (if accessible)
4. Test all sensors individually
5. Check power supply output

**Documentation Review**:
1. Update network diagrams
2. Review and update procedures
3. Update contact information
4. Review security settings
5. Update disaster recovery plan

### Annual Maintenance

**Major Overhaul**:
1. Replace batteries if degraded
2. Update all firmware/software
3. Recalibrate all sensors
4. Replace worn cables
5. Clean/replace fans if present

**System Audit**:
1. Review security protocols
2. Audit access logs
3. Update disaster recovery plan
4. Review insurance coverage
5. Budget for replacements

---

## Scaling to Multiple Units

### Multi-Unit Architecture

**Centralized Monitoring**:
```
Multiple IoT Units → LoRaWAN Gateway → TTN → Central Server → Dashboard
```

**Benefits**:
- Single dashboard for all units
- Centralized data storage
- Unified alerting system
- Easier maintenance
- Cost-effective scaling

### Device Identification

**Unique Device IDs**:
```cpp
// Set unique ID for each unit
const char* deviceId = "IOT-UNIT-001";
const char* deviceLocation = "Building A - Floor 1";

void sendData() {
  // Include device ID in all transmissions
  String payload = "{\"id\":\"" + String(deviceId) + "\",";
  payload += "\"location\":\"" + String(deviceLocation) + "\",";
  payload += "\"temperature\":" + String(temperature) + "}";
  LoRa.send(payload.c_str());
}
```

### Network Architecture

**Single Gateway** (small deployments, <10 units):
- All units connect to one gateway
- Gateway connects to TTN
- Cost-effective
- Limited by gateway capacity

**Multiple Gateways** (medium deployments, 10-100 units):
- Each gateway covers area
- Gateways connect to TTN
- Better coverage
- Higher cost

**Private Network** (large deployments, 100+ units):
- Private LoRaWAN network server
- Full control
- No reliance on TTN
- Significant infrastructure cost

### Data Management

**Database Schema** (multi-unit):
```sql
CREATE TABLE devices (
  device_id TEXT PRIMARY KEY,
  location TEXT,
  install_date DATE,
  firmware_version TEXT,
  last_seen TIMESTAMP
);

CREATE TABLE sensor_data (
  timestamp TIMESTAMP,
  device_id TEXT,
  sensor_type TEXT,
  value REAL,
  FOREIGN KEY (device_id) REFERENCES devices(device_id)
);

CREATE TABLE alerts (
  alert_id INTEGER PRIMARY KEY,
  device_id TEXT,
  alert_type TEXT,
  severity TEXT,
  timestamp TIMESTAMP,
  acknowledged BOOLEAN,
  FOREIGN KEY (device_id) REFERENCES devices(device_id)
);
```

### Maintenance Scaling

**Remote Diagnostics**:
```cpp
// Remote diagnostic command via LoRaWAN downlink
void handleDownlink(byte* payload, unsigned int length) {
  if (payload[0] == CMD_DIAGNOSTIC) {
    sendSystemStatus();
    sendBatteryStatus();
    sendSignalStrength();
    sendUptime();
    sendErrorCount();
  }
}
```

**OTA Updates for Multiple Units**:
1. Stage firmware update on server
2. Update units sequentially
3. Monitor each unit's update
4. Rollback if failure detected
5. Update remaining units

### Cost Analysis

**Per-Unit Cost** (with scaling):

| Units | Hardware | Installation | Monthly (TTN) | Annual Total |
|-------|----------|--------------|---------------|--------------|
| 1 | $318 | $50 | Free | $368 |
| 10 | $3,180 | $300 | Free | $3,480 |
| 50 | $15,900 | $1,000 | Free | $16,900 |
| 100 | $31,800 | $1,500 | $100 | $34,100 |

**Economies of Scale**:
- Bulk hardware discounts (10-20%)
- Shared infrastructure (network, gateway)
- Automated maintenance
- Centralized monitoring

---

## Security Considerations

### Physical Security

**Indoor**:
- Lockable enclosure
- Tamper-evident seals
- Secure mounting
- Restricted access

**Outdoor**:
- Heavy-duty enclosure (metal)
- Tamper-proof screws
- Alarm on enclosure opening
- Outdoor-rated locks

### Network Security

**WiFi Security**:
- WPA2-AES encryption minimum
- Strong passwords (12+ characters)
- Separate VLAN for IoT
- Disable WPS

**LoRaWAN Security**:
- AES-128 encryption (built-in)
- Unique AppKey per device
- Rotate keys periodically
- Monitor for unauthorized devices

**Backend Security**:
- HTTPS/TLS for all connections
- Strong authentication (MFA)
- Regular security updates
- Input validation
- SQL injection prevention

### Data Security

**Encryption at Rest**:
```javascript
// Encrypt sensitive data in database
const crypto = require('crypto');
const algorithm = 'aes-256-cbc';

function encrypt(text) {
  const key = Buffer.from(process.env.ENCRYPTION_KEY, 'hex');
  const iv = crypto.randomBytes(16);
  const cipher = crypto.createCipheriv(algorithm, key, iv);
  let encrypted = cipher.update(text, 'utf8', 'hex');
  encrypted += cipher.final('hex');
  return iv.toString('hex') + ':' + encrypted;
}
```

**Secure Communication**:
- Use HTTPS (TLS 1.3)
- Certificate pinning for critical systems
- VPN for remote access
- No cleartext passwords in code

**Access Control**:
- Role-based access control (RBAC)
- Audit logging
- Regular access reviews
- MFA for admin access

### Backup and Recovery

**Backup Strategy**:
1. **Daily**: Automated database backup
2. **Weekly**: Full system configuration backup
3. **Monthly**: Archive to offsite storage

**Backup Script**:
```bash
#!/bin/bash
# Daily backup script
DATE=$(date +%Y%m%d)
BACKUP_DIR="/backups"

# Backup database
sqlite3 iot_system.db ".backup ${BACKUP_DIR}/db_${DATE}.db"

# Backup configuration files
tar -czf ${BACKUP_DIR}/config_${DATE}.tar.gz /etc/iot-system/

# Keep last 30 days
find ${BACKUP_DIR} -mtime +30 -delete
```

**Disaster Recovery**:
1. Document recovery procedures
2. Test recovery monthly
3. Maintain spare hardware
4. Offsite backup storage
5. Recovery time objective (RTO): <4 hours
6. Recovery point objective (RPO): <24 hours

---

## Troubleshooting Deployment Issues

### Power-Related Issues

**Symptom**: System resets randomly

**Diagnosis**:
1. Measure voltage under load
2. Check for voltage drops (<4.5V causes reset)
3. Inspect power supply capacity

**Solutions**:
- Upgrade to higher current power supply
- Add decoupling capacitors (100µF near each board)
- Check for loose connections
- Verify power quality

**Symptom**: Battery drains quickly

**Diagnosis**:
1. Measure actual current draw
2. Check for parasitic loads
3. Verify battery capacity

**Solutions**:
- Implement deeper sleep modes
- Reduce transmission frequency
- Replace degraded battery
- Check for short circuits

### Network-Related Issues

**Symptom**: WiFi drops intermittently

**Diagnosis**:
1. Check signal strength (should be >-70 dBm)
2. Scan for interference
3. Review router logs

**Solutions**:
- Move closer to router
- Add WiFi extender
- Change WiFi channel
- Update router firmware

**Symptom**: LoRaWAN won't join

**Diagnosis**:
1. Verify antenna connected
2. Check frequency settings
3. Verify TTN credentials

**Solutions**:
- Check antenna connection
- Verify correct frequency region
- Re-register device on TTN
- Check gateway coverage

### Environmental Issues

**Symptom**: Overheating in summer

**Diagnosis**:
1. Measure internal temperature
2. Check ambient temperature
3. Inspect ventilation

**Solutions**:
- Add sun shield
- Increase ventilation (fan or vents)
- Use heat-resistant materials
- Add thermal insulation

**Symptom**: Condensation inside enclosure

**Diagnosis**:
1. Check for water ingress
2. Monitor humidity levels
3. Inspect seals

**Solutions**:
- Improve sealing
- Add desiccant packets
- Install breather membrane
- Add slight heating element

### Detection Issues

**Symptom**: False detections

**Diagnosis**:
1. Review detection logs
2. Check camera angle
3. Assess lighting conditions

**Solutions**:
- Adjust detection threshold
- Improve lighting
- Recalibrate camera
- Update ML model

**Symptom**: Missed detections

**Diagnosis**:
1. Test detection accuracy
2. Check camera focus
3. Verify model loaded correctly

**Solutions**:
- Lower detection threshold
- Improve camera positioning
- Retrain ML model
- Check camera cable connection

### Maintenance Issues

**Symptom**: Difficulty accessing unit for maintenance

**Diagnosis**:
1. Review mounting height
2. Check enclosure location
3. Assess accessibility

**Solutions**:
- Install at accessible height
- Use quick-release mounts
- Provide access platform
- Consider remote diagnostics

---

## Appendix A: Deployment Checklist

### Pre-Deployment

- [ ] Site survey completed
- [ ] Power requirements determined
- [ ] Network coverage verified
- [ ] Environmental conditions assessed
- [ ] Mounting location selected
- [ ] Permits/permissions obtained
- [ ] Spare parts procured

### Equipment Checklist

- [ ] IoT system assembled and tested
- [ ] Enclosure (appropriate IP rating)
- [ ] Power supply and cables
- [ ] Antennas and mounting hardware
- [ ] Mounting bracket or stand
- [ ] Weatherproof connectors (outdoor)
- [ ] Tools (drill, screwdriver, etc.)
- [ ] Cable management supplies
- [ ] Labels and marker

### Deployment Day

- [ ] Transport equipment to site
- [ ] Mount enclosure securely
- [ ] Route and connect power cables
- [ ] Install antennas
- [ ] Connect all components
- [ ] Power on system
- [ ] Verify startup sequence
- [ ] Test all functionality
- [ ] Document installation
- [ ] Train local staff (if applicable)

### Post-Deployment

- [ ] Monitor system for 24 hours
- [ ] Calibrate sensors
- [ ] Configure alerts
- [ ] Test alarm system
- [ ] Document any issues
- [ ] Schedule maintenance
- [ ] Hand over to operations

---

## Appendix B: Regulatory Compliance

### FCC (United States)

- FCC Part 15 (unintentional radiator)
- LoRaWAN certification required
- Antenna restrictions
- RF exposure limits

### CE (Europe)

- CE marking required
- EMC Directive compliance
- R&TTE Directive
- RoHS compliance

### Other Regions

Check local regulations for:
- RF spectrum usage
- Environmental compliance
- Electrical safety
- Data privacy (GDPR, etc.)

---

**End of Deployment Guide**

**Last Updated**: 2025-01-06
**Version**: 1.0.0
**Status**: Production Ready
