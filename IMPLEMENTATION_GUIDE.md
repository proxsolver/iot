# IoT Multi-Sensor System - Complete Implementation Guide

## Overview

This is a **production-ready**, **complete IoT multi-sensor system** with Kepware OPC-UA integration, featuring real-time data processing, WebSocket communications, MQTT messaging, alert notifications, and a comprehensive dashboard.

## Architecture

### Components Created

1. **Backend Server** (`src/backend/data_pipeline_complete.js`)
   - Complete RESTful API with all CRUD operations
   - WebSocket server for real-time updates
   - SQLite database with migrations
   - Authentication and authorization
   - Rate limiting and security
   - Error handling and logging

2. **Kepware Integration**
   - Enhanced OPC-UA client (`src/kepware/kepware_opcua_client_enhanced.js`)
   - Automatic reconnection with exponential backoff
   - Tag subscription and monitoring
   - Batch read/write operations
   - Data buffering and change detection
   - Health monitoring

3. **Middleware Services** (`src/middleware/`)
   - `auth.js` - JWT and API key authentication
   - `validation.js` - Request validation using Joi
   - `errorHandler.js` - Centralized error handling
   - `rateLimiter.js` - Rate limiting with configurable limits

4. **Business Services** (`src/services/`)
   - `mqttService.js` - MQTT broker and client
   - `alertService.js` - Alert notifications (email/webhook)
   - `exportService.js` - Data export (CSV/JSON/XML)

5. **Database** (`src/config/database.js`)
   - Better-sqlite3 for production performance
   - Automatic migrations
   - Connection pooling
   - Query helpers

6. **Logging** (`src/config/logger.js`)
   - Winston with daily rotating files
   - Multiple log levels
   - Structured logging

7. **Dashboard** (`src/dashboard/`)
   - Real-time WebSocket updates
   - Chart.js visualizations
   - Responsive design
   - Alert management

## Installation

### Prerequisites

- Node.js >= 14.0.0
- Kepware KEPServerEX (optional, for OPC-UA integration)
- MQTT broker (optional, built-in Aedes broker available)

### Setup Steps

1. **Install Dependencies**
   ```bash
   cd /path/to/iot-multi-sensor-system
   npm install
   ```

2. **Configure Environment**
   ```bash
   cp .env.example .env
   # Edit .env with your configuration
   ```

3. **Run Database Migrations**
   ```bash
   npm run migrate
   ```

4. **Start the Server**
   ```bash
   # Production mode
   npm start

   # Development mode (with auto-reload)
   npm run dev
   ```

5. **Access Dashboard**
   ```
   http://localhost:3000
   ```

## Configuration

### Environment Variables

```bash
# Server
PORT=3000
NODE_ENV=production

# Database
DB_PATH=./data/iot_data.db

# WebSocket
WS_PORT=8080

# Kepware OPC-UA
KEPWARE_ENABLED=true
KEPWARE_ENDPOINT=opc.tcp://localhost:49320

# MQTT
MQTT_ENABLED=true
MQTT_PORT=1883
MQTT_HOST=0.0.0.0

# Authentication
JWT_SECRET=your-super-secret-jwt-key
JWT_EXPIRES_IN=24h

# Rate Limiting
RATE_LIMIT_WINDOW_MS=900000  # 15 minutes
RATE_LIMIT_MAX_REQUESTS=100

# Logging
LOG_LEVEL=info
LOG_DIR=./logs

# Alerts
ALERT_EMAIL_ENABLED=false
ALERT_WEBHOOK_ENABLED=false
```

## API Documentation

### Sensor Data Endpoints

#### POST /api/sensor-data
Submit sensor readings from IoT devices.

**Request Body:**
```json
{
  "device_id": "nicla-sense-001",
  "temperature": 23.5,
  "humidity": 65.2,
  "pressure": 1013.25,
  "gas_resistance": 125000,
  "battery_level": 85,
  "signal_strength": -65,
  "timestamp": 1609459200
}
```

**Response:**
```json
{
  "success": true,
  "id": 123
}
```

#### GET /api/sensor-data
Retrieve sensor readings with filters.

**Query Parameters:**
- `limit` (default: 100, max: 1000)
- `device_id` - Filter by device
- `start_time` - Unix timestamp
- `end_time` - Unix timestamp

**Response:**
```json
[
  {
    "id": 123,
    "device_id": "nicla-sense-001",
    "temperature": 23.5,
    "humidity": 65.2,
    "pressure": 1013.25,
    "timestamp": 1609459200
  }
]
```

#### GET /api/sensor-data/latest
Get the most recent sensor reading.

#### GET /api/sensor-data/stats
Get sensor statistics for a time period.

**Query Parameters:**
- `device_id` - Filter by device
- `hours` (default: 24) - Time period in hours

### Detection Endpoints

#### POST /api/detections
Submit object detection data from cameras.

**Request Body:**
```json
{
  "camera_id": 1,
  "class_id": 0,
  "class_name": "person",
  "confidence": 0.95,
  "bounding_box": {
    "x": 100,
    "y": 150,
    "width": 200,
    "height": 300
  },
  "timestamp": 1609459200
}
```

#### GET /api/detections
Retrieve detection history.

**Query Parameters:**
- `limit` (default: 100)
- `camera_id` - Filter by camera
- `class_name` - Filter by class
- `start_time` - Start timestamp
- `end_time` - End timestamp

### Alert Endpoints

#### POST /api/alerts
Create a new alert (requires authentication).

**Request Body:**
```json
{
  "alert_type": "threshold",
  "severity": "high",
  "message": "Temperature exceeded threshold",
  "data": {
    "device_id": "nicla-sense-001",
    "temperature": 45.5
  }
}
```

#### GET /api/alerts
Retrieve alert history.

**Query Parameters:**
- `limit` (default: 50)
- `acknowledged` - true/false
- `severity` - low/medium/high/critical

#### PUT /api/alerts/:id/acknowledge
Acknowledge an alert.

### System Status Endpoints

#### POST /api/status
Submit system status updates.

#### GET /api/status?device_id=gateway
Get current system status.

### Control Command Endpoints

#### POST /api/control/command
Send a control command to a device (requires authentication).

**Request Body:**
```json
{
  "command_type": "system_enable",
  "target_device": "gateway",
  "parameters": {
    "enabled": true
  }
}
```

#### GET /api/control/commands
Get command history.

### Data Export Endpoints

#### POST /api/export
Create a data export (requires authentication).

**Request Body:**
```json
{
  "export_type": "csv",
  "table": "sensor_readings",
  "start_time": 1609459200,
  "end_time": 1609545600
}
```

**Response:**
```json
{
  "success": true,
  "filepath": "/exports/sensor_readings_2024-01-01.csv",
  "filename": "sensor_readings_2024-01-01.csv",
  "recordCount": 1440,
  "size": 123456
}
```

### Kepware Integration Endpoints

#### GET /api/kepware/status
Get Kepware connection status.

#### POST /api/kepware/connect
Connect to Kepware server.

#### POST /api/kepware/disconnect
Disconnect from Kepware.

#### GET /api/kepware/tags
Read all Kepware tags.

#### POST /api/kepware/write
Write to a specific tag.

**Request Body:**
```json
{
  "tag": "Channel1.IoTGateway.Control_SystemEnable",
  "value": true
}
```

### Authentication Endpoints

#### POST /api/auth/login
Authenticate and get JWT token.

**Request Body:**
```json
{
  "username": "admin",
  "password": "password"
}
```

**Response:**
```json
{
  "success": true,
  "token": "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9...",
  "user": {
    "id": 1,
    "username": "admin",
    "email": "admin@example.com",
    "role": "admin"
  }
}
```

## WebSocket API

### Connection

Connect to: `ws://localhost:8080`

### Message Types

#### Server -> Client

**sensor_data**
```json
{
  "type": "sensor_data",
  "data": {
    "device_id": "nicla-sense-001",
    "temperature": 23.5,
    "humidity": 65.2,
    "pressure": 1013.25,
    "timestamp": 1609459200
  },
  "timestamp": 1609459200000
}
```

**detection**
```json
{
  "type": "detection",
  "data": {
    "camera_id": 1,
    "class_name": "person",
    "confidence": 0.95
  },
  "timestamp": 1609459200000
}
```

**alert**
```json
{
  "type": "alert",
  "data": {
    "alert_type": "threshold",
    "severity": "high",
    "message": "Temperature exceeded threshold"
  },
  "timestamp": 1609459200000
}
```

#### Client -> Server

**subscribe**
```json
{
  "type": "subscribe",
  "channel": "sensor"
}
```

Available channels: `sensor`, `detection`, `alert`, `status`, `control`

**unsubscribe**
```json
{
  "type": "unsubscribe",
  "channel": "sensor"
}
```

**ping**
```json
{
  "type": "ping"
}
```

## Kepware Tag Mapping

| IoT Data | Kepware Tag |
|----------|-------------|
| Temperature | Channel1.IoTGateway.NiclaSense_Temperature |
| Humidity | Channel1.IoTGateway.NiclaSense_Humidity |
| Pressure | Channel1.IoTGateway.NiclaSense_Pressure |
| Gas Resistance | Channel1.IoTGateway.NiclaSense_GasResistance |
| Battery Level | Channel1.IoTGateway.System_BatteryLevel |
| Signal Strength | Channel1.IoTGateway.System_SignalStrength |
| System Status | Channel1.IoTGateway.System_Status |

## MQTT Topics

### Published Topics

- `iot/sensor/{device_id}` - Sensor data
- `iot/detection/camera{1,2}` - Detection data
- `iot/alert/{severity}` - Alerts
- `iot/status/{device_id}` - System status

## Security Features

1. **Authentication**
   - JWT tokens for API access
   - API key authentication
   - Role-based access control (admin, user, viewer)

2. **Rate Limiting**
   - Configurable per-IP limits
   - Stricter limits for sensitive endpoints
   - WebSocket connection limits

3. **Input Validation**
   - Joi schema validation
   - SQL injection prevention
   - XSS protection

4. **Security Headers**
   - Helmet.js for HTTP headers
   - CORS configuration
   - Compression

## Performance Features

1. **Database**
   - Better-sqlite3 for synchronous operations
   - WAL mode for concurrency
   - Indexed queries
   - Prepared statements

2. **Caching**
   - NodeId caching for OPC-UA
   - Data buffering during disconnection
   - Chart data optimization

3. **Error Recovery**
   - Automatic reconnection
   - Exponential backoff
   - Graceful degradation

## Monitoring & Logging

### Log Files

- `logs/error-YYYY-MM-DD.log` - Error logs
- `logs/combined-YYYY-MM-DD.log` - All logs
- `logs/application-YYYY-MM-DD.log` - Application logs

### Health Check

```
GET /health
```

Returns system status, including:
- Uptime
- Memory usage
- WebSocket clients
- MQTT status
- Kepware status
- Database status

## Testing

### Test Kepware Connection

```bash
npm test
```

### Load Testing

Use tools like Apache Bench or Artillery to test endpoints:

```bash
# Test sensor data endpoint
ab -n 1000 -c 10 -p sensor_data.json -T application/json http://localhost:3000/api/sensor-data
```

## Deployment

### Production Checklist

1. Set `NODE_ENV=production`
2. Configure strong `JWT_SECRET`
3. Enable HTTPS with reverse proxy (nginx/Apache)
4. Set up process manager (PM2/systemd)
5. Configure log rotation
6. Set up database backups
7. Configure firewall rules
8. Enable CORS for specific origins only
9. Set up monitoring (Prometheus/Grafana)
10. Configure email/webhook alerts

### PM2 Configuration

```javascript
// ecosystem.config.js
module.exports = {
  apps: [{
    name: 'iot-backend',
    script: './src/backend/data_pipeline_complete.js',
    instances: 1,
    autorestart: true,
    watch: false,
    max_memory_restart: '1G',
    env: {
      NODE_ENV: 'production',
      PORT: 3000
    }
  }]
};
```

```bash
pm2 start ecosystem.config.js
pm2 save
pm2 startup
```

## Troubleshooting

### Kepware Connection Issues

1. Verify Kepware is running: Check KEPServerEX application
2. Check endpoint: `opc.tcp://localhost:49320`
3. Security settings: Ensure security mode matches Kepware config
4. Firewall: Ensure port 49320 is open

### Database Issues

1. Check file permissions on `data/iot_data.db`
2. Ensure disk space is available
3. Check SQLite version compatibility

### WebSocket Connection Issues

1. Verify port 8080 is not blocked
2. Check browser console for errors
3. Ensure WebSocket URL is correct

## File Structure

```
iot-multi-sensor-system/
├── src/
│   ├── backend/
│   │   └── data_pipeline_complete.js     # Main server
│   ├── kepware/
│   │   ├── kepware_opcua_client.js       # Original OPC-UA client
│   │   ├── kepware_opcua_client_enhanced.js  # Enhanced client
│   │   └── kepware_integration.js        # Integration manager
│   ├── middleware/
│   │   ├── auth.js                       # Authentication
│   │   ├── validation.js                 # Request validation
│   │   ├── errorHandler.js               # Error handling
│   │   └── rateLimiter.js                # Rate limiting
│   ├── services/
│   │   ├── mqttService.js                # MQTT broker/client
│   │   ├── alertService.js               # Alert notifications
│   │   └── exportService.js              # Data export
│   ├── config/
│   │   ├── database.js                   # Database configuration
│   │   └── logger.js                     # Logging configuration
│   └── dashboard/
│       ├── index.html                    # Dashboard UI
│       ├── dashboard.js                  # Dashboard logic
│       ├── styles.css                    # Dashboard styles
│       └── chart.js                      # Chart.js integration
├── data/                                 # Database files
├── logs/                                 # Log files
├── exports/                              # Export files
├── .env.example                          # Environment template
├── package.json                          # Dependencies
└── README.md                             # This file
```

## Dependencies

Key production dependencies:

- **express** - Web framework
- **better-sqlite3** - Database
- **node-opcua** - OPC-UA client
- **ws** - WebSocket server
- **mqtt** - MQTT client
- **aedes** - MQTT broker
- **winston** - Logging
- **joi** - Validation
- **jsonwebtoken** - Authentication
- **helmet** - Security headers
- **cors** - CORS handling

## License

MIT

## Support

For issues and questions, please refer to the project repository or documentation.
