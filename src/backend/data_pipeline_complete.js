/**
 * Production-Ready IoT Data Pipeline Server
 *
 * Complete backend system with:
 * - RESTful API with all CRUD operations
 * - WebSocket server for real-time updates
 * - SQLite database with migrations
 * - Kepware OPC-UA integration
 * - MQTT broker/client
 * - Alert notification system
 * - Data export service
 * - Comprehensive logging
 * - Authentication and authorization
 * - Rate limiting and security
 * - Health monitoring
 */

const express = require('express');
const cors = require('cors');
const helmet = require('helmet');
const compression = require('compression');
const morgan = require('morgan');
const path = require('path');
const WebSocket = require('ws');

// Import configurations
const logger = require('./config/logger');
const db = require('./config/database');

// Import middleware
const {
  authenticate,
  optionalAuthenticate,
  authorize,
  generateToken
} = require('./middleware/auth');
const { validate, queryParamsSchema } = require('./middleware/validation');
const {
  errorHandler,
  notFoundHandler,
  asyncHandler,
  setupUnhandledRejectionHandler,
  setupUncaughtExceptionHandler
} = require('./middleware/errorHandler');
const {
  generalLimiter,
  dataIngestionLimiter,
  WebSocketRateLimiter
} = require('./middleware/rateLimiter');

// Import validation schemas
const {
  sensorDataSchema,
  detectionSchema,
  alertSchema,
  systemStatusSchema,
  controlCommandSchema,
  userCreateSchema,
  loginSchema,
  alertRuleSchema,
  exportRequestSchema
} = require('./middleware/validation');

// Import services
const { getMQTTService } = require('./services/mqttService');
const { getAlertService } = require('./services/alertService');
const { getExportService } = require('./services/exportService');

// Import Kepware integration
const {
  KepwareIntegrationManager,
  setupKepwareRoutes,
  INTEGRATION_CONFIG
} = require('../kepware/kepware_integration');

// ===========================================
// CONFIGURATION
// ===========================================

const PORT = process.env.PORT || 3000;
const WS_PORT = process.env.WS_PORT || 8080;
const NODE_ENV = process.env.NODE_ENV || 'production';

// ===========================================
// EXPRESS APP SETUP
// ===========================================

const app = express();

// Security middleware
app.use(helmet({
  contentSecurityPolicy: false, // Disable for dashboard
  crossOriginEmbedderPolicy: false
}));

// CORS configuration
app.use(cors({
  origin: process.env.CORS_ORIGIN || '*',
  credentials: true
}));

// Compression
app.use(compression());

// Body parsing
app.use(express.json({ limit: '10mb' }));
app.use(express.urlencoded({ extended: true, limit: '10mb' }));

// HTTP request logging
app.use(morgan('combined', { stream: logger.stream }));

// Static files
app.use(express.static(path.join(__dirname, '../dashboard')));

// Rate limiting
app.use('/api/', generalLimiter);

// ===========================================
// WEBSOCKET SERVER
// ===========================================

const wss = new WebSocket.Server({ port: WS_PORT });
const wsRateLimiter = new WebSocketRateLimiter(100, 100);
const wsClients = new Map();

// Generate client ID
function generateClientId() {
  return `client_${Date.now()}_${Math.random().toString(36).substr(2, 9)}`;
}

// WebSocket connection handler
wss.on('connection', (ws, req) => {
  const clientId = generateClientId();
  const clientIp = req.socket.remoteAddress;

  // Check rate limit
  if (!wsRateLimiter.canConnect(clientIp)) {
    logger.warn(`WebSocket connection rejected (rate limit): ${clientIp}`);
    ws.close(1008, 'Rate limit exceeded');
    return;
  }

  logger.info(`WebSocket client connected: ${clientId} from ${clientIp}`);

  wsClients.set(clientId, {
    ws,
    ip: clientIp,
    connectedAt: new Date(),
    subscribedChannels: new Set()
  });

  // Send welcome message
  ws.send(JSON.stringify({
    type: 'connected',
    clientId,
    timestamp: Date.now()
  }));

  // Handle incoming messages
  ws.on('message', (message) => {
    try {
      const data = JSON.parse(message);
      handleWebSocketMessage(clientId, data);
    } catch (error) {
      logger.error(`Error handling WebSocket message from ${clientId}:`, error);
    }
  });

  // Handle disconnect
  ws.on('close', () => {
    logger.info(`WebSocket client disconnected: ${clientId}`);
    wsRateLimiter.removeConnection(clientIp, ws);
    wsClients.delete(clientId);
  });

  // Handle errors
  ws.on('error', (error) => {
    logger.error(`WebSocket error for ${clientId}:`, error);
  });

  // Register with rate limiter
  wsRateLimiter.addConnection(clientIp, ws);
});

// Handle WebSocket messages
function handleWebSocketMessage(clientId, data) {
  const client = wsClients.get(clientId);
  if (!client) return;

  switch (data.type) {
    case 'subscribe':
      // Subscribe to channel
      if (data.channel) {
        client.subscribedChannels.add(data.channel);
        logger.debug(`Client ${clientId} subscribed to ${data.channel}`);
      }
      break;

    case 'unsubscribe':
      // Unsubscribe from channel
      if (data.channel) {
        client.subscribedChannels.delete(data.channel);
        logger.debug(`Client ${clientId} unsubscribed from ${data.channel}`);
      }
      break;

    case 'ping':
      // Respond to ping
      client.ws.send(JSON.stringify({ type: 'pong', timestamp: Date.now() }));
      break;

    default:
      logger.warn(`Unknown WebSocket message type: ${data.type}`);
  }
}

// Broadcast to all WebSocket clients
function broadcast(type, data, channel = null) {
  const message = JSON.stringify({ type, data, timestamp: Date.now() });

  wss.clients.forEach((ws) => {
    if (ws.readyState === WebSocket.OPEN) {
      // If channel specified, only send to subscribed clients
      if (channel) {
        const client = [...wsClients.values()].find(c => c.ws === ws);
        if (client && client.subscribedChannels.has(channel)) {
          ws.send(message);
        }
      } else {
        ws.send(message);
      }
    }
  });
}

// ===========================================
// SERVICE INITIALIZATION
// ===========================================

let mqttService = null;
let alertService = null;
let exportService = null;
let kepwareManager = null;

function initializeServices() {
  // Initialize MQTT service
  mqttService = getMQTTService();
  if (mqttService.enabled) {
    mqttService.startBroker();
    logger.info('MQTT service initialized');
  }

  // Initialize alert service
  alertService = getAlertService();
  logger.info('Alert service initialized');

  // Initialize export service
  exportService = getExportService();
  logger.info('Export service initialized');

  // Initialize Kepware integration
  if (INTEGRATION_CONFIG.enabled) {
    kepwareManager = new KepwareIntegrationManager();

    kepwareManager.onControlCommand = (command) => {
      logger.info('Control command received from Kepware:', command);
      broadcast('kepware_command', command);
    };

    kepwareManager.onConnectionChange = (connected) => {
      logger.info(`Kepware connection state: ${connected ? 'CONNECTED' : 'DISCONNECTED'}`);
      broadcast('kepware_status', { connected });
    };

    kepwareManager.initialize().then((result) => {
      if (result) {
        kepwareManager.start();
      }
    }).catch((error) => {
      logger.error('Failed to initialize Kepware:', error);
    });
  }
}

// ===========================================
// API ENDPOINTS - HEALTH
// ===========================================

app.get('/health', (req, res) => {
  const health = {
    status: 'healthy',
    uptime: process.uptime(),
    timestamp: Date.now(),
    memory: process.memoryUsage(),
    websocket: {
      port: WS_PORT,
      clients: wsClients.size
    },
    mqtt: mqttService ? mqttService.getStats() : { enabled: false },
    kepware: kepwareManager ? kepwareManager.getStatus() : { enabled: false },
    database: {
      connected: db.getDatabase() !== null
    }
  };

  res.json(health);
});

app.get('/api/health', (req, res) => {
  res.json({
    status: 'ok',
    timestamp: Date.now(),
    services: {
      mqtt: mqttService?.getStats(),
      kepware: kepwareManager?.getStatus()
    }
  });
});

// ===========================================
// API ENDPOINTS - SENSOR DATA
// ===========================================

// POST /api/sensor-data - Receive sensor data
app.post('/api/sensor-data',
  dataIngestionLimiter,
  validate(sensorDataSchema),
  asyncHandler(async (req, res) => {
    const { device_id, temperature, humidity, pressure, gas_resistance, timestamp, battery_level, signal_strength, uptime } = req.body;

    const finalTimestamp = timestamp || Math.floor(Date.now() / 1000);

    // Insert into database
    const result = db.execute(
      `INSERT INTO sensor_readings
       (device_id, temperature, humidity, pressure, gas_resistance, timestamp)
       VALUES (?, ?, ?, ?, ?, ?)`,
      [device_id, temperature, humidity, pressure, gas_resistance, finalTimestamp]
    );

    const sensorData = {
      id: result.lastInsertRowid,
      device_id,
      temperature,
      humidity,
      pressure,
      gas_resistance,
      timestamp: finalTimestamp
    };

    // Broadcast to WebSocket clients
    broadcast('sensor_data', sensorData, 'sensor');

    // Publish to MQTT
    if (mqttService && mqttService.enabled) {
      const mqttClient = mqttService.createClient(`sensor_${device_id}`);
      if (mqttClient) {
        mqttService.publishSensorData(mqttClient, sensorData);
      }
    }

    // Check alert rules
    await alertService.checkAlertRules(sensorData);

    // Push to Kepware
    if (kepwareManager && kepwareManager.isConnected) {
      kepwareManager.updateSensorData(sensorData);
    }

    res.status(201).json({ success: true, id: result.lastInsertRowid });
  })
);

// GET /api/sensor-data - Get sensor readings
app.get('/api/sensor-data',
  optionalAuthenticate,
  asyncHandler(async (req, res) => {
    const { limit = 100, device_id, start_time, end_time } = req.query;

    let query = 'SELECT * FROM sensor_readings WHERE 1=1';
    const params = [];

    if (device_id) {
      query += ' AND device_id = ?';
      params.push(device_id);
    }

    if (start_time) {
      query += ' AND timestamp >= ?';
      params.push(start_time);
    }

    if (end_time) {
      query += ' AND timestamp <= ?';
      params.push(end_time);
    }

    query += ' ORDER BY timestamp DESC LIMIT ?';
    params.push(Math.min(limit, 1000));

    const rows = db.queryAll(query, params);
    res.json(rows);
  })
);

// GET /api/sensor-data/latest - Get latest sensor reading
app.get('/api/sensor-data/latest',
  optionalAuthenticate,
  asyncHandler(async (req, res) => {
    const { device_id } = req.query;

    let query = 'SELECT * FROM sensor_readings';
    const params = [];

    if (device_id) {
      query += ' WHERE device_id = ?';
      params.push(device_id);
    }

    query += ' ORDER BY timestamp DESC LIMIT 1';

    const row = db.queryOne(query, params);
    res.json(row || null);
  })
);

// GET /api/sensor-data/stats - Get sensor statistics
app.get('/api/sensor-data/stats',
  optionalAuthenticate,
  asyncHandler(async (req, res) => {
    const { device_id, hours = 24 } = req.query;

    const startTime = Math.floor(Date.now() / 1000) - (hours * 3600);

    let query = `
      SELECT
        AVG(temperature) as avg_temp,
        MIN(temperature) as min_temp,
        MAX(temperature) as max_temp,
        AVG(humidity) as avg_hum,
        MIN(humidity) as min_hum,
        MAX(humidity) as max_hum,
        AVG(pressure) as avg_pres,
        MIN(pressure) as min_pres,
        MAX(pressure) as max_pres,
        COUNT(*) as count
      FROM sensor_readings
      WHERE timestamp >= ?
    `;
    const params = [startTime];

    if (device_id) {
      query += ' AND device_id = ?';
      params.push(device_id);
    }

    const row = db.queryOne(query, params);
    res.json(row);
  })
);

// DELETE /api/sensor-data - Delete old sensor data
app.delete('/api/sensor-data',
  authenticate,
  authorize('admin'),
  asyncHandler(async (req, res) => {
    const { before_timestamp } = req.query;

    if (!before_timestamp) {
      return res.status(400).json({ error: 'before_timestamp parameter required' });
    }

    const result = db.execute(
      'DELETE FROM sensor_readings WHERE timestamp < ?',
      [before_timestamp]
    );

    res.json({ success: true, deleted: result.changes });
  })
);

// ===========================================
// API ENDPOINTS - DETECTIONS
// ===========================================

// POST /api/detections - Receive detection data
app.post('/api/detections',
  dataIngestionLimiter,
  validate(detectionSchema),
  asyncHandler(async (req, res) => {
    const { camera_id, class_id, class_name, confidence, bounding_box, image_path, timestamp } = req.body;

    const finalTimestamp = timestamp || Math.floor(Date.now() / 1000);

    const result = db.execute(
      `INSERT INTO detections
       (camera_id, class_id, class_name, confidence, bounding_box, image_path, timestamp)
       VALUES (?, ?, ?, ?, ?, ?, ?)`,
      [camera_id, class_id, class_name, confidence, JSON.stringify(bounding_box), image_path, finalTimestamp]
    );

    const detectionData = {
      id: result.lastInsertRowid,
      camera_id,
      class_id,
      class_name,
      confidence,
      bounding_box,
      image_path,
      timestamp: finalTimestamp
    };

    // Broadcast to WebSocket clients
    broadcast('detection', detectionData, 'detection');

    // Publish to MQTT
    if (mqttService && mqttService.enabled) {
      const mqttClient = mqttService.createClient(`detection_camera${camera_id}`);
      if (mqttClient) {
        mqttService.publishDetection(mqttClient, detectionData);
      }
    }

    // Create alert for high-confidence detections
    if (confidence > 0.8) {
      await alertService.createAlert('detection', confidence > 0.9 ? 'high' : 'medium',
        `High confidence ${class_name} detected on camera ${camera_id}`,
        { camera_id, confidence, bounding_box }
      );
    }

    // Push to Kepware
    if (kepwareManager && kepwareManager.isConnected) {
      kepwareManager.pushDetection(detectionData);
    }

    res.status(201).json({ success: true, id: result.lastInsertRowid });
  })
);

// GET /api/detections - Get detections
app.get('/api/detections',
  optionalAuthenticate,
  asyncHandler(async (req, res) => {
    const { limit = 100, camera_id, class_name, start_time, end_time } = req.query;

    let query = 'SELECT * FROM detections WHERE 1=1';
    const params = [];

    if (camera_id) {
      query += ' AND camera_id = ?';
      params.push(camera_id);
    }

    if (class_name) {
      query += ' AND class_name = ?';
      params.push(class_name);
    }

    if (start_time) {
      query += ' AND timestamp >= ?';
      params.push(start_time);
    }

    if (end_time) {
      query += ' AND timestamp <= ?';
      params.push(end_time);
    }

    query += ' ORDER BY timestamp DESC LIMIT ?';
    params.push(Math.min(limit, 1000));

    const rows = db.queryAll(query, params);

    // Parse bounding_box JSON
    rows.forEach(row => {
      try {
        row.bounding_box = JSON.parse(row.bounding_box || '{}');
      } catch (e) {
        row.bounding_box = {};
      }
    });

    res.json(rows);
  })
);

// GET /api/detections/stats - Get detection statistics
app.get('/api/detections/stats',
  optionalAuthenticate,
  asyncHandler(async (req, res) => {
    const { hours = 24 } = req.query;

    const startTime = Math.floor(Date.now() / 1000) - (hours * 3600);

    const query = `
      SELECT
        class_name,
        COUNT(*) as count,
        AVG(confidence) as avg_confidence,
        MAX(confidence) as max_confidence
      FROM detections
      WHERE timestamp >= ?
      GROUP BY class_name
      ORDER BY count DESC
    `;

    const rows = db.queryAll(query, [startTime]);
    res.json(rows);
  })
);

// ===========================================
// API ENDPOINTS - ALERTS
// ===========================================

// POST /api/alerts - Create alert
app.post('/api/alerts',
  authenticate,
  validate(alertSchema),
  asyncHandler(async (req, res) => {
    const { alert_type, severity, message, data } = req.body;

    const result = db.execute(
      `INSERT INTO alerts (alert_type, severity, message, data, timestamp)
       VALUES (?, ?, ?, ?, ?)`,
      [alert_type, severity, message, JSON.stringify(data), Math.floor(Date.now() / 1000)]
    );

    const alert = {
      id: result.lastInsertRowid,
      alert_type,
      severity,
      message,
      data,
      timestamp: Math.floor(Date.now() / 1000)
    };

    // Broadcast
    broadcast('alert', alert, 'alert');

    // Process notifications
    await alertService.processAlert(alert);

    res.status(201).json({ success: true, id: result.lastInsertRowid });
  })
);

// GET /api/alerts - Get alerts
app.get('/api/alerts',
  optionalAuthenticate,
  asyncHandler(async (req, res) => {
    const { limit = 50, acknowledged, severity } = req.query;

    let query = 'SELECT * FROM alerts WHERE 1=1';
    const params = [];

    if (acknowledged !== undefined) {
      query += ' AND acknowledged = ?';
      params.push(acknowledged === 'true' ? 1 : 0);
    }

    if (severity) {
      query += ' AND severity = ?';
      params.push(severity);
    }

    query += ' ORDER BY timestamp DESC LIMIT ?';
    params.push(Math.min(limit, 1000));

    const rows = db.queryAll(query, params);

    // Parse data JSON
    rows.forEach(row => {
      try {
        row.data = JSON.parse(row.data || '{}');
      } catch (e) {
        row.data = {};
      }
    });

    res.json(rows);
  })
);

// PUT /api/alerts/:id/acknowledge - Acknowledge alert
app.put('/api/alerts/:id/acknowledge',
  authenticate,
  asyncHandler(async (req, res) => {
    const { id } = req.params;
    const username = req.user.username;

    const result = db.execute(
      'UPDATE alerts SET acknowledged = 1, acknowledged_by = ?, acknowledged_at = ? WHERE id = ?',
      [username, Math.floor(Date.now() / 1000), id]
    );

    if (result.changes === 0) {
      return res.status(404).json({ error: 'Alert not found' });
    }

    res.json({ success: true });
  })
);

// ===========================================
// API ENDPOINTS - SYSTEM STATUS
// ===========================================

// POST /api/status - Receive system status
app.post('/api/status',
  validate(systemStatusSchema),
  asyncHandler(async (req, res) => {
    const { device_id, status, battery_level, signal_strength, uptime, memory_usage, cpu_usage } = req.body;

    const result = db.execute(
      `INSERT INTO system_status
       (device_id, status, battery_level, signal_strength, uptime, memory_usage, cpu_usage, timestamp)
       VALUES (?, ?, ?, ?, ?, ?, ?, ?)`,
      [device_id, status, battery_level, signal_strength, uptime, memory_usage, cpu_usage, Math.floor(Date.now() / 1000)]
    );

    const statusData = {
      device_id,
      status,
      battery_level,
      signal_strength,
      uptime,
      timestamp: Math.floor(Date.now() / 1000)
    };

    broadcast('status', statusData, 'status');

    if (mqttService && mqttService.enabled) {
      const mqttClient = mqttService.createClient(`status_${device_id}`);
      if (mqttClient) {
        mqttService.publishSystemStatus(mqttClient, statusData);
      }
    }

    if (kepwareManager && kepwareManager.isConnected) {
      kepwareManager.updateStatus(statusData);
    }

    res.status(201).json({ success: true });
  })
);

// GET /api/status - Get system status
app.get('/api/status',
  optionalAuthenticate,
  asyncHandler(async (req, res) => {
    const { device_id } = req.query;

    const query = `
      SELECT * FROM system_status
      WHERE device_id = ?
      ORDER BY timestamp DESC
      LIMIT 1
    `;

    const row = db.queryOne(query, [device_id || 'gateway']);
    res.json(row || null);
  })
);

// ===========================================
// API ENDPOINTS - CONTROL COMMANDS
// ===========================================

// POST /api/control/command - Send control command
app.post('/api/control/command',
  authenticate,
  validate(controlCommandSchema),
  asyncHandler(async (req, res) => {
    const { command_type, target_device, parameters } = req.body;

    const result = db.execute(
      `INSERT INTO control_commands (command_type, target_device, parameters, status)
       VALUES (?, ?, ?, ?)`,
      [command_type, target_device, JSON.stringify(parameters), 'pending']
    );

    const command = {
      id: result.lastInsertRowid,
      command_type,
      target_device,
      parameters,
      status: 'pending'
    };

    broadcast('control_command', command, 'control');

    res.status(201).json({ success: true, id: result.lastInsertRowid });
  })
);

// GET /api/control/commands - Get control commands
app.get('/api/control/commands',
  authenticate,
  asyncHandler(async (req, res) => {
    const { limit = 50, status } = req.query;

    let query = 'SELECT * FROM control_commands WHERE 1=1';
    const params = [];

    if (status) {
      query += ' AND status = ?';
      params.push(status);
    }

    query += ' ORDER BY created_at DESC LIMIT ?';
    params.push(Math.min(limit, 1000));

    const rows = db.queryAll(query, params);

    // Parse parameters JSON
    rows.forEach(row => {
      try {
        row.parameters = JSON.parse(row.parameters || '{}');
      } catch (e) {
        row.parameters = {};
      }
    });

    res.json(rows);
  })
);

// PUT /api/control/command/:id - Update command status
app.put('/api/control/command/:id',
  authenticate,
  asyncHandler(async (req, res) => {
    const { id } = req.params;
    const { status, result, acknowledged_by } = req.body;

    const updates = ['status = ?', 'executed_at = ?'];
    const params = [status, Math.floor(Date.now() / 1000)];

    if (result !== undefined) {
      updates.push('result = ?');
      params.push(JSON.stringify(result));
    }

    if (acknowledged_by !== undefined) {
      updates.push('acknowledged_by = ?');
      params.push(acknowledged_by);
    }

    params.push(id);

    const dbResult = db.execute(
      `UPDATE control_commands SET ${updates.join(', ')} WHERE id = ?`,
      params
    );

    if (dbResult.changes === 0) {
      return res.status(404).json({ error: 'Command not found' });
    }

    res.json({ success: true });
  })
);

// ===========================================
// API ENDPOINTS - ALERT RULES
// ===========================================

// POST /api/alert-rules - Create alert rule
app.post('/api/alert-rules',
  authenticate,
  authorize('admin'),
  validate(alertRuleSchema),
  asyncHandler(async (req, res) => {
    const { name, condition_type, sensor_type, threshold_min, threshold_max, severity, enabled, notification_channels, cooldown_period } = req.body;

    const result = db.execute(
      `INSERT INTO alert_rules
       (name, condition_type, sensor_type, threshold_min, threshold_max, severity, enabled, notification_channels, cooldown_period, created_by)
       VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?)`,
      [
        name,
        condition_type,
        sensor_type,
        threshold_min,
        threshold_max,
        severity || 'medium',
        enabled !== undefined ? enabled : 1,
        JSON.stringify(notification_channels || []),
        cooldown_period || 300,
        req.user.username
      ]
    );

    res.status(201).json({ success: true, id: result.lastInsertRowid });
  })
);

// GET /api/alert-rules - Get alert rules
app.get('/api/alert-rules',
  authenticate,
  asyncHandler(async (req, res) => {
    const rows = db.queryAll('SELECT * FROM alert_rules ORDER BY created_at DESC');

    // Parse notification_channels JSON
    rows.forEach(row => {
      try {
        row.notification_channels = JSON.parse(row.notification_channels || '[]');
      } catch (e) {
        row.notification_channels = [];
      }
    });

    res.json(rows);
  })
);

// PUT /api/alert-rules/:id - Update alert rule
app.put('/api/alert-rules/:id',
  authenticate,
  authorize('admin'),
  asyncHandler(async (req, res) => {
    const { id } = req.params;
    const updates = [];
    const params = [];

    const allowedFields = ['name', 'condition_type', 'sensor_type', 'threshold_min', 'threshold_max', 'severity', 'enabled', 'notification_channels', 'cooldown_period'];

    for (const field of allowedFields) {
      if (req.body[field] !== undefined) {
        updates.push(`${field} = ?`);
        params.push(field === 'notification_channels' ? JSON.stringify(req.body[field]) : req.body[field]);
      }
    }

    if (updates.length === 0) {
      return res.status(400).json({ error: 'No fields to update' });
    }

    params.push(id);

    const result = db.execute(
      `UPDATE alert_rules SET ${updates.join(', ')} WHERE id = ?`,
      params
    );

    if (result.changes === 0) {
      return res.status(404).json({ error: 'Alert rule not found' });
    }

    res.json({ success: true });
  })
);

// DELETE /api/alert-rules/:id - Delete alert rule
app.delete('/api/alert-rules/:id',
  authenticate,
  authorize('admin'),
  asyncHandler(async (req, res) => {
    const { id } = req.params;

    const result = db.execute('DELETE FROM alert_rules WHERE id = ?', [id]);

    if (result.changes === 0) {
      return res.status(404).json({ error: 'Alert rule not found' });
    }

    res.json({ success: true });
  })
);

// ===========================================
// API ENDPOINTS - DATA EXPORT
// ===========================================

// POST /api/export - Create data export
app.post('/api/export',
  authenticate,
  validate(exportRequestSchema),
  asyncHandler(async (req, res) => {
    const { export_type, table, start_time, end_time, filters } = req.body;

    try {
      const result = await exportService.createExport(export_type, table, start_time, end_time, filters);

      res.json(result);
    } catch (error) {
      res.status(500).json({ error: error.message });
    }
  })
);

// GET /api/export/logs - Get export logs
app.get('/api/export/logs',
  authenticate,
  authorize('admin'),
  asyncHandler(async (req, res) => {
    const { limit = 50 } = req.query;

    const logs = exportService.getExportLogs(Math.min(limit, 1000));
    res.json(logs);
  })
);

// ===========================================
// API ENDPOINTS - USERS (for authentication)
// ===========================================

// POST /api/users - Create user (admin only)
app.post('/api/users',
  authenticate,
  authorize('admin'),
  validate(userCreateSchema),
  asyncHandler(async (req, res) => {
    const { username, email, password, role } = req.body;

    // Check if user exists
    const existingUser = db.queryOne(
      'SELECT id FROM users WHERE username = ? OR email = ?',
      [username, email]
    );

    if (existingUser) {
      return res.status(400).json({ error: 'Username or email already exists' });
    }

    // Hash password (use bcrypt in production)
    const password_hash = password; // In production, use bcrypt.hash()

    const result = db.execute(
      'INSERT INTO users (username, email, password_hash, role) VALUES (?, ?, ?, ?)',
      [username, email, password_hash, role || 'user']
    );

    res.status(201).json({ success: true, id: result.lastInsertRowid });
  })
);

// POST /api/auth/login - Login
app.post('/api/auth/login',
  validate(loginSchema),
  asyncHandler(async (req, res) => {
    const { username, password } = req.body;

    const user = db.queryOne(
      'SELECT * FROM users WHERE username = ? AND is_active = 1',
      [username]
    );

    if (!user || user.password_hash !== password) {
      return res.status(401).json({ error: 'Invalid credentials' });
    }

    // Update last login
    db.execute(
      'UPDATE users SET last_login = ? WHERE id = ?',
      [Math.floor(Date.now() / 1000), user.id]
    );

    const token = generateToken(user);

    res.json({
      success: true,
      token,
      user: {
        id: user.id,
        username: user.username,
        email: user.email,
        role: user.role
      }
    });
  })
);

// ===========================================
// API ENDPOINTS - KEPWARE INTEGRATION
// ===========================================

// Setup Kepware routes
setupKepwareRoutes(app, {
  getStatus: () => kepwareManager ? kepwareManager.getStatus() : { enabled: false, connected: false },
  initialize: async () => {
    if (!kepwareManager) {
      kepwareManager = new KepwareIntegrationManager();
    }
    const result = await kepwareManager.initialize();
    if (result) {
      kepwareManager.start();
    }
    return result;
  },
  shutdown: async () => {
    if (kepwareManager) {
      await kepwareManager.shutdown();
    }
  },
  pushDetection: async (data) => {
    if (kepwareManager) {
      await kepwareManager.pushDetection(data);
    }
  },
  clearAlert: async () => {
    if (kepwareManager) {
      await kepwareManager.clearAlert();
    }
  },
  readControlTags: async () => {
    if (kepwareManager && kepwareManager.client) {
      return await kepwareManager.client.readControlTags();
    }
    return null;
  },
  client: null
});

// ===========================================
// SERVE DASHBOARD
// ===========================================

app.get('/', (req, res) => {
  res.sendFile(path.join(__dirname, '../dashboard/index.html'));
});

// ===========================================
// ERROR HANDLING
// ===========================================

app.use(notFoundHandler);
app.use(errorHandler);

// ===========================================
// START SERVER
// ===========================================

async function startServer() {
  try {
    // Initialize database
    logger.info('Initializing database...');
    db.initializeDatabase();
    db.migrate();

    // Initialize services
    logger.info('Initializing services...');
    initializeServices();

    // Start HTTP server
    app.listen(PORT, () => {
      logger.info(`IoT Data Pipeline server running on port ${PORT}`);
      logger.info(`WebSocket server running on port ${WS_PORT}`);
      logger.info(`Dashboard: http://localhost:${PORT}`);
      logger.info(`Environment: ${NODE_ENV}`);
      logger.info(`Kepware: ${kepwareManager && kepwareManager.isConnected ? 'CONNECTED' : 'DISCONNECTED'}`);
      logger.info(`MQTT: ${mqttService && mqttService.enabled ? 'ENABLED' : 'DISABLED'}`);
    });
  } catch (error) {
    logger.error('Failed to start server:', error);
    process.exit(1);
  }
}

// Setup error handlers
setupUnhandledRejectionHandler();
setupUncaughtExceptionHandler();

// Start server
startServer();

// Graceful shutdown
process.on('SIGINT', async () => {
  logger.info('Shutting down gracefully...');

  // Stop MQTT broker
  if (mqttService) {
    await mqttService.stopBroker();
  }

  // Shutdown Kepware
  if (kepwareManager) {
    await kepwareManager.shutdown();
  }

  // Close database
  db.closeDatabase();

  // Close WebSocket server
  wss.close(() => {
    logger.info('WebSocket server closed');
  });

  process.exit(0);
});

process.on('SIGTERM', async () => {
  logger.info('Received SIGTERM, shutting down...');
  await mqttService?.stopBroker();
  await kepwareManager?.shutdown();
  db.closeDatabase();
  wss.close();
  process.exit(0);
});

module.exports = app;
