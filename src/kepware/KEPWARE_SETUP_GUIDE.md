# Kepware (KEPServerEX) OPC-UA Setup Guide

This guide explains how to integrate the IoT Multi-Sensor System with Kepware KEPServerEX for SCADA/HMI integration.

## Overview

The integration allows:
- Real-time sensor data push to Kepware tags
- Detection data from dual cameras
- System status monitoring
- Bidirectional communication (HMI/SCADA can control IoT system)
- LoRaWAN status monitoring

```
┌─────────────────────────────────────────────────────────────────────────┐
│                    Kepware Integration Architecture                    │
├─────────────────────────────────────────────────────────────────────────┤
│                                                                         │
│  ┌──────────────┐         ┌──────────────┐         ┌──────────────┐   │
│  │   Arduino    │         │  Data        │         │   Kepware    │   │
│  │   Sensors    │────────▶│  Pipeline    │────────▶│   KEPServer  │   │
│  │              │ Serial  │  (Node.js)   │ OPC-UA  │              │   │
│  └──────────────┘         └──────────────┘         └──────┬───────┘   │
│                                                           │            │
│                                                           │            │
│                                                           ▼            │
│                                                    ┌──────────────┐   │
│                                                    │   SCADA/     │   │
│                                                    │   HMI        │   │
│                                                    └──────────────┘   │
│                                                                         │
└─────────────────────────────────────────────────────────────────────────┘
```

## Prerequisites

1. **KEPServerEX** (v6 or later) installed and running
2. **OPC-UA Driver** enabled in Kepware
3. Node.js dependencies:
   ```bash
   npm install node-opcua
   ```

## Kepware Configuration

### Step 1: Create Channel

1. Open KEPServerEX
2. Right-click on "Project" → "New Channel"
3. Name: `Channel1`
4. Select "OPC UA" as the driver (or use any driver, we access via OPC-UA)

### Step 2: Create Device

1. Right-click on Channel1 → "New Device"
2. Name: `IoTGateway`
3. Configure device settings (default is fine)

### Step 3: Create Tag Groups

Create the following tag structure under IoTGateway:

#### Environmental Sensors
| Tag Name | Data Type | Description |
|----------|-----------|-------------|
| NiclaSense_Temperature | Float | Temperature in Celsius |
| NiclaSense_Humidity | Float | Humidity % |
| NiclaSense_Pressure | Float | Pressure in hPa |
| NiclaSense_GasResistance | Float | Gas resistance ohms |

#### Camera 1 (Vision)
| Tag Name | Data Type | Description |
|----------|-----------|-------------|
| Camera1_DetectedClass | String | Object class name |
| Camera1_Confidence | Integer | Confidence 0-100 |
| Camera1_DetectionCount | Integer | Total detections |

#### Camera 2 (Vision)
| Tag Name | Data Type | Description |
|----------|-----------|-------------|
| Camera2_DetectedClass | String | Object class name |
| Camera2_Confidence | Integer | Confidence 0-100 |
| Camera2_DetectionCount | Integer | Total detections |

#### System Status
| Tag Name | Data Type | Description |
|----------|-----------|-------------|
| System_Status | Boolean | Online status |
| System_BatteryLevel | Integer | Battery 0-100 |
| System_SignalStrength | Integer | RSSI value |
| System_Uptime | Integer | Uptime seconds |

#### Alarm
| Tag Name | Data Type | Description |
|----------|-----------|-------------|
| Alarm_Active | Boolean | Alarm active flag |
| Alarm_Type | Integer | Type: 1=Detection, 2=System, 3=Low Battery |
| Alarm_Severity | Integer | 1=Low, 2=Medium, 3=High, 4=Critical |

#### LoRaWAN
| Tag Name | Data Type | Description |
|----------|-----------|-------------|
| LoRa_Connected | Boolean | LoRaWAN connected |
| LoRa_LastRSSI | Integer | Last RSSI value |
| LoRa_LastSNR | Integer | Last SNR value |

#### Control Tags (Read from HMI/SCADA)
| Tag Name | Data Type | Description |
|----------|-----------|-------------|
| Control_SystemEnable | Boolean | Enable/disable system |
| Control_AlarmReset | Boolean | Reset alarm flag |
| Control_UpdateInterval | Integer | Data update interval (ms) |

### Step 4: Configure OPC-UA Server

1. In KEPServerEX, go to "Project" → "Properties"
2. Click on "OPC UA" → "Server Configuration"
3. Note the endpoint URL (default: `opc.tcp://localhost:49320`)
4. Configure security:
   - For testing: None
   - For production: Sign + SignAndEncrypt with certificates

## Environment Variables

Create a `.env` file in the project root:

```env
# Enable Kepware integration
KEPWARE_ENABLED=true

# Kepware OPC-UA endpoint
KEPWARE_ENDPOINT=opc.tcp://localhost:49320

# Security (for production)
# KEPWARE_SECURITY_MODE=Sign
# KEPWARE_SECURITY_POLICY=Basic256Sha256
```

## Usage

### Starting the Server

```bash
# Enable Kepware integration
export KEPWARE_ENABLED=true

# Start the data pipeline
node src/backend/data_pipeline.js
```

You should see:
```
Kepware integration is enabled, initializing...
Connecting to Kepware at opc.tcp://localhost:49320...
Connected to Kepware OPC-UA Server
OPC-UA Session created
✓ Kepware connection test successful
Control tag monitoring set up
Kepware integration started
  - Sensor data interval: 5000ms
  - Status interval: 10000ms
  - Control tag interval: 2000ms
```

### API Endpoints

#### Get Kepware Status
```bash
GET /api/kepware/status
```

Response:
```json
{
  "enabled": true,
  "connected": true,
  "latestSensorUpdate": 1704451200000,
  "latestStatusUpdate": 1704451260000
}
```

#### Read All Tags
```bash
GET /api/kepware/tags
```

#### Write to Tag
```bash
POST /api/kepware/write
Content-Type: application/json

{
  "tag": "Channel1.IoTGateway.System_Status",
  "value": true
}
```

#### Read Control Tags
```bash
GET /api/kepware/control
```

Response:
```json
{
  "systemEnabled": true,
  "alarmReset": false,
  "updateInterval": 5000
}
```

#### Clear Alarm
```bash
POST /api/kepware/alert/clear
```

## SCADA/HMI Integration Examples

### Ignition SCADA

1. Create new OPC-UA connection
2. Endpoint: `opc.tcp://localhost:49320`
3. Browse to tags: `Channel1 → IoTGateway`
4. Add tags to your project

### InduSoft Web Studio

1. Create new OPC-UA driver
2. Configure endpoint: `opc.tcp://localhost:49320`
3. Import tags using the tag browser

### Wonderware System Platform

1. Configure OPC-UA DA Server object
2. Add Kepware as a data source
3. Create tags mirroring Kepware structure

### FactoryTalk View SE

1. Create new OPC-UA topic
2. Configure server endpoint
3. Design HMI screens with tag bindings

## Testing

### Manual Tag Testing

```javascript
const { KepwareOPCUAClient } = require('./src/kepware/kepware_opcua_client');

async function test() {
  const client = new KepwareOPCUAClient();

  await client.connect();

  // Read a tag
  const value = await client.readTag('Channel1.IoTGateway.NiclaSense_Temperature');
  console.log('Temperature:', value);

  // Write a tag
  await client.writeTag('Channel1.IoTGateway.System_Status', true);

  await client.disconnect();
}

test();
```

### Monitor Control Commands

The data pipeline broadcasts control commands via WebSocket:

```javascript
const ws = new WebSocket('ws://localhost:8080');

ws.onmessage = (event) => {
  const data = JSON.parse(event.data);
  if (data.type === 'kepware_command') {
    console.log('Control command:', data.data);
    // Handle command from HMI/SCADA
  }
};
```

## Troubleshooting

### Connection Refused

- Verify Kepware is running
- Check OPC-UA server is enabled
- Verify endpoint URL and port

### Tag Not Found

- Verify tag path matches Kepware structure
- Check tag is enabled in Kepware
- Use tag browser to find correct path

### Security Errors

- For testing: use MessageSecurityMode.None
- For production: import certificates and configure properly

### High CPU Usage

- Increase data push intervals
- Reduce number of monitored tags
- Use data change filters in Kepware

## Advanced Configuration

### Custom Tag Mapping

Edit `src/kepware/kepware_opcua_client.js`:

```javascript
const TAG_MAPPING = {
  // Your custom tag paths
  customSensor: 'Channel1.CustomFolder.SensorValue'
};
```

### Batch Writing

For efficient bulk updates:

```javascript
const tagValuePairs = [
  { tag: 'Channel1.IoTGateway.Tag1', value: 10 },
  { tag: 'Channel1.IoTGateway.Tag2', value: 20 },
  { tag: 'Channel1.IoTGateway.Tag3', value: 30 }
];

await client.writeTags(tagValuePairs);
```

### Subscription Filtering

Monitor only specific tags for changes:

```javascript
await client.subscribeToTags([
  {
    tag: 'Channel1.IoTGateway.Control_AlarmReset',
    samplingInterval: 500,
    callback: (data) => console.log('Alarm reset:', data.value)
  }
]);
```

## Security Best Practices

1. **Production Environment**
   - Enable encryption (Sign + SignAndEncrypt)
   - Use certificates
   - Implement user authentication
   - Restrict IP access

2. **Network Security**
   - Use VPN for remote access
   - Firewall rules for port 49320
   - Network segmentation

3. **Application Security**
   - Validate all tag values
   - Implement rate limiting
   - Log all write operations

## Data Flow Diagram

```
┌────────────────────────────────────────────────────────────────────────┐
│                        Data Flow Sequence                              │
├────────────────────────────────────────────────────────────────────────┤
│                                                                        │
│  1. Sensor Data → Arduino                                             │
│     │                                                                 │
│  2. Serial Communication → MKR WAN 1310                               │
│     │                                                                 │
│  3. HTTP POST → /api/sensor-data                                      │
│     │                                                                 │
│  4. Data Pipeline                                                     │
│     ├─▶ Store in SQLite                                              │
│     ├─▶ Broadcast WebSocket                                          │
│     └─▶ Push to Kepware (opc.tcp://localhost:49320)                   │
│              │                                                         │
│  5. Kepware Tags Updated                                             │
│              │                                                         │
│  6. SCADA/HMI Polls Tags                                             │
│              │                                                         │
│  7. HMI Display Updated                                              │
│                                                                        │
└────────────────────────────────────────────────────────────────────────┘
```

## Support

For issues or questions:
1. Check Kepware logs: KEPServerEX → Administration → Event Log
2. Check data pipeline console output
3. Verify OPC-UA connection with UAExpert client
4. Review tag configuration in Kepware
