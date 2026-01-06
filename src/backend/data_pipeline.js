/**
 * Data Pipeline for IoT Multi-Sensor System
 *
 * Handles data aggregation, validation, storage, and API endpoints
 * for the complete IoT system including sensors, cameras, and LoRaWAN.
 *
 * Technology: Node.js with Express
 * Database: SQLite (can be upgraded to PostgreSQL/MySQL)
 * Message Queue: Redis (optional)
 */

const express = require('express');
const bodyParser = require('body-parser');
const sqlite3 = require('sqlite3').verbose();
const WebSocket = require('ws');
const path = require('path');

// Kepware Integration
const {
  KepwareIntegrationManager,
  setupKepwareRoutes,
  INTEGRATION_CONFIG
} = require('../kepware/kepware_integration');

// ===========================================
// CONFIGURATION
// ===========================================

const PORT = process.env.PORT || 3000;
const DB_PATH = path.join(__dirname, 'iot_data.db');

// Kepware Integration Manager
let kepwareManager = null;

// ===========================================
// DATABASE SETUP
// ===========================================

const db = new sqlite3.Database(DB_PATH, (err) => {
  if (err) {
    console.error('Error opening database:', err);
  } else {
    console.log('Connected to SQLite database');
    initializeDatabase();
  }
});

function initializeDatabase() {
  // Create sensor readings table
  db.run(`
    CREATE TABLE IF NOT EXISTS sensor_readings (
      id INTEGER PRIMARY KEY AUTOINCREMENT,
      device_id TEXT NOT NULL,
      temperature REAL,
      humidity REAL,
      pressure REAL,
      gas_resistance REAL,
      timestamp INTEGER NOT NULL,
      received_at INTEGER DEFAULT (strftime('%s', 'now'))
    )
  `);

  // Create detections table
  db.run(`
    CREATE TABLE IF NOT EXISTS detections (
      id INTEGER PRIMARY KEY AUTOINCREMENT,
      camera_id INTEGER NOT NULL,
      class_id INTEGER NOT NULL,
      class_name TEXT NOT NULL,
      confidence REAL NOT NULL,
      bounding_box TEXT,
      timestamp INTEGER NOT NULL,
      received_at INTEGER DEFAULT (strftime('%s', 'now'))
    )
  `);

  // Create alerts table
  db.run(`
    CREATE TABLE IF NOT EXISTS alerts (
      id INTEGER PRIMARY KEY AUTOINCREMENT,
      alert_type TEXT NOT NULL,
      severity TEXT NOT NULL,
      message TEXT NOT NULL,
      data TEXT,
      acknowledged BOOLEAN DEFAULT 0,
      timestamp INTEGER NOT NULL,
      received_at INTEGER DEFAULT (strftime('%s', 'now'))
    )
  `);

  // Create system status table
  db.run(`
    CREATE TABLE IF NOT EXISTS system_status (
      id INTEGER PRIMARY KEY AUTOINCREMENT,
      device_id TEXT NOT NULL,
      status TEXT NOT NULL,
      battery_level INTEGER,
      signal_strength INTEGER,
      uptime INTEGER,
      timestamp INTEGER NOT NULL
    )
  `);

  // Create indexes for performance
  db.run('CREATE INDEX IF NOT EXISTS idx_sensor_timestamp ON sensor_readings(timestamp)');
  db.run('CREATE INDEX IF NOT EXISTS idx_detection_timestamp ON detections(timestamp)');
  db.run('CREATE INDEX IF NOT EXISTS idx_alert_timestamp ON alerts(timestamp)');

  console.log('Database tables initialized');
}

// ===========================================
// EXPRESS APP SETUP
// ===========================================

const app = express();
app.use(bodyParser.json());
app.use(bodyParser.urlencoded({ extended: true }));
app.use(express.static('public'));

// WebSocket server for real-time updates
const wss = new WebSocket.Server({ port: 8080 });

// Broadcast data to all connected WebSocket clients
function broadcast(type, data) {
  wss.clients.forEach((client) => {
    if (client.readyState === WebSocket.OPEN) {
      client.send(JSON.stringify({ type, data }));
    }
  });
}

// ===========================================
// API ENDPOINTS - SENSOR DATA
// ===========================================

// POST /api/sensor-data - Receive sensor data from gateway
app.post('/api/sensor-data', (req, res) => {
  const { device_id, temperature, humidity, pressure, gas_resistance, timestamp } = req.body;

  // Validate data
  if (!device_id || !temperature || !humidity || !pressure) {
    return res.status(400).json({ error: 'Missing required fields' });
  }

  // Insert into database
  const stmt = db.prepare(`
    INSERT INTO sensor_readings
    (device_id, temperature, humidity, pressure, gas_resistance, timestamp)
    VALUES (?, ?, ?, ?, ?, ?)
  `);

  stmt.run([device_id, temperature, humidity, pressure, gas_resistance, timestamp], function(err) {
    if (err) {
      console.error('Error inserting sensor data:', err);
      return res.status(500).json({ error: 'Database error' });
    }

    // Broadcast to WebSocket clients
    broadcast('sensor_data', {
      id: this.lastID,
      device_id,
      temperature,
      humidity,
      pressure,
      gas_resistance,
      timestamp
    });

    // Push to Kepware
    if (kepwareManager && kepwareManager.isConnected) {
      kepwareManager.updateSensorData({
        device_id,
        temperature,
        humidity,
        pressure,
        gas_resistance,
        timestamp
      });
    }

    res.status(201).json({ success: true, id: this.lastID });
  });

  stmt.finalize();
});

// GET /api/sensor-data - Get sensor readings
app.get('/api/sensor-data', (req, res) => {
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
  params.push(limit);

  db.all(query, params, (err, rows) => {
    if (err) {
      return res.status(500).json({ error: err.message });
    }
    res.json(rows);
  });
});

// GET /api/sensor-data/latest - Get latest sensor reading
app.get('/api/sensor-data/latest', (req, res) => {
  const { device_id } = req.query;

  let query = 'SELECT * FROM sensor_readings';
  if (device_id) {
    query += ' WHERE device_id = ?';
  }
  query += ' ORDER BY timestamp DESC LIMIT 1';

  db.get(query, device_id ? [device_id] : [], (err, row) => {
    if (err) {
      return res.status(500).json({ error: err.message });
    }
    res.json(row);
  });
});

// GET /api/sensor-data/stats - Get sensor statistics
app.get('/api/sensor-data/stats', (req, res) => {
  const { device_id, hours = 24 } = req.query;

  const startTime = Date.now() / 1000 - (hours * 3600);

  const query = `
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
    ${device_id ? 'AND device_id = ?' : ''}
  `;

  const params = device_id ? [startTime, device_id] : [startTime];

  db.get(query, params, (err, row) => {
    if (err) {
      return res.status(500).json({ error: err.message });
    }
    res.json(row);
  });
});

// ===========================================
// API ENDPOINTS - DETECTIONS
// ===========================================

// POST /api/detections - Receive detection data
app.post('/api/detections', (req, res) => {
  const { camera_id, class_id, class_name, confidence, bounding_box, timestamp } = req.body;

  if (!camera_id || class_id === undefined || !class_name || !confidence) {
    return res.status(400).json({ error: 'Missing required fields' });
  }

  const stmt = db.prepare(`
    INSERT INTO detections
    (camera_id, class_id, class_name, confidence, bounding_box, timestamp)
    VALUES (?, ?, ?, ?, ?, ?)
  `);

  stmt.run([camera_id, class_id, class_name, confidence, JSON.stringify(bounding_box), timestamp], function(err) {
    if (err) {
      console.error('Error inserting detection:', err);
      return res.status(500).json({ error: 'Database error' });
    }

    // Broadcast to WebSocket clients
    broadcast('detection', {
      id: this.lastID,
      camera_id,
      class_id,
      class_name,
      confidence,
      bounding_box,
      timestamp
    });

    // Create alert for high-confidence detections
    if (confidence > 0.8) {
      createAlert('detection', 'high', `High confidence ${class_name} detected`, {
        camera_id,
        confidence
      });
    }

    // Push detection to Kepware
    if (kepwareManager && kepwareManager.isConnected) {
      kepwareManager.pushDetection({
        camera_id,
        class_id,
        class_name,
        confidence,
        timestamp
      });
    }

    res.status(201).json({ success: true, id: this.lastID });
  });

  stmt.finalize();
});

// GET /api/detections - Get detections
app.get('/api/detections', (req, res) => {
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
  params.push(limit);

  db.all(query, params, (err, rows) => {
    if (err) {
      return res.status(500).json({ error: err.message });
    }

    // Parse bounding_box JSON
    rows.forEach(row => {
      row.bounding_box = JSON.parse(row.bounding_box || '{}');
    });

    res.json(rows);
  });
});

// GET /api/detections/stats - Get detection statistics
app.get('/api/detections/stats', (req, res) => {
  const { hours = 24 } = req.query;

  const startTime = Date.now() / 1000 - (hours * 3600);

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

  db.all(query, [startTime], (err, rows) => {
    if (err) {
      return res.status(500).json({ error: err.message });
    }
    res.json(rows);
  });
});

// ===========================================
// API ENDPOINTS - ALERTS
// ===========================================

function createAlert(type, severity, message, data = {}) {
  const stmt = db.prepare(`
    INSERT INTO alerts (alert_type, severity, message, data, timestamp)
    VALUES (?, ?, ?, ?, ?)
  `);

  stmt.run([type, severity, message, JSON.stringify(data), Math.floor(Date.now() / 1000)], function(err) {
    if (err) {
      console.error('Error creating alert:', err);
      return;
    }

    broadcast('alert', {
      id: this.lastID,
      type,
      severity,
      message,
      data,
      timestamp: Math.floor(Date.now() / 1000)
    });
  });

  stmt.finalize();
}

// GET /api/alerts - Get alerts
app.get('/api/alerts', (req, res) => {
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
  params.push(limit);

  db.all(query, params, (err, rows) => {
    if (err) {
      return res.status(500).json({ error: err.message });
    }

    rows.forEach(row => {
      row.data = JSON.parse(row.data || '{}');
    });

    res.json(rows);
  });
});

// PUT /api/alerts/:id/acknowledge - Acknowledge alert
app.put('/api/alerts/:id/acknowledge', (req, res) => {
  const { id } = req.params;

  db.run('UPDATE alerts SET acknowledged = 1 WHERE id = ?', [id], function(err) {
    if (err) {
      return res.status(500).json({ error: err.message });
    }

    if (this.changes === 0) {
      return res.status(404).json({ error: 'Alert not found' });
    }

    res.json({ success: true });
  });
});

// ===========================================
// API ENDPOINTS - SYSTEM STATUS
// ===========================================

// POST /api/status - Receive system status
app.post('/api/status', (req, res) => {
  const { device_id, status, battery_level, signal_strength, uptime } = req.body;

  const stmt = db.prepare(`
    INSERT INTO system_status
    (device_id, status, battery_level, signal_strength, uptime, timestamp)
    VALUES (?, ?, ?, ?, ?, ?)
  `);

  stmt.run([device_id, status, battery_level, signal_strength, uptime, Math.floor(Date.now() / 1000)], function(err) {
    if (err) {
      console.error('Error inserting status:', err);
      return res.status(500).json({ error: 'Database error' });
    }

    broadcast('status', {
      device_id,
      status,
      battery_level,
      signal_strength,
      uptime,
      timestamp: Math.floor(Date.now() / 1000)
    });

    res.status(201).json({ success: true });
  });

  stmt.finalize();
});

// GET /api/status - Get system status
app.get('/api/status', (req, res) => {
  const { device_id } = req.query;

  const query = `
    SELECT * FROM system_status
    WHERE device_id = ?
    ORDER BY timestamp DESC
    LIMIT 1
  `;

  db.get(query, [device_id || 'gateway'], (err, row) => {
    if (err) {
      return res.status(500).json({ error: err.message });
    }
    res.json(row);
  });
});

// ===========================================
// SERVE DASHBOARD
// ===========================================

app.get('/', (req, res) => {
  res.sendFile(path.join(__dirname, '../dashboard/index.html'));
});

// ===========================================
// KEPWARE ROUTES
// ===========================================

// Setup Kepware integration routes
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
  client: null // Will be set when manager initializes
});

// ===========================================
// ERROR HANDLING
// ===========================================

app.use((err, req, res, next) => {
  console.error('Error:', err);
  res.status(500).json({ error: 'Internal server error' });
});

// ===========================================
// START SERVER
// ===========================================

async function startServer() {
  // Initialize Kepware if enabled
  if (INTEGRATION_CONFIG.enabled) {
    console.log('Kepware integration is enabled, initializing...');
    kepwareManager = new KepwareIntegrationManager();

    // Set up control command callback
    kepwareManager.onControlCommand = (command) => {
      console.log('Control command received from Kepware:', command);
      broadcast('kepware_command', command);
    };

    // Set up connection state callback
    kepwareManager.onConnectionChange = (connected) => {
      console.log(`Kepware connection state: ${connected ? 'CONNECTED' : 'DISCONNECTED'}`);
      broadcast('kepware_status', { connected });
    };

    // Initialize connection (will retry on failure)
    await kepwareManager.initialize();

    if (kepwareManager.isConnected) {
      kepwareManager.start();
    }
  } else {
    console.log('Kepware integration is disabled. Set KEPWARE_ENABLED=true to enable.');
  }

  // Start HTTP server
  app.listen(PORT, () => {
    console.log(`IoT Data Pipeline server running on port ${PORT}`);
    console.log(`WebSocket server running on port 8080`);
    console.log(`Dashboard: http://localhost:${PORT}`);
    console.log(`Kepware: ${kepwareManager && kepwareManager.isConnected ? 'CONNECTED' : 'DISCONNECTED'}`);
  });
}

startServer();

// Graceful shutdown
process.on('SIGINT', async () => {
  console.log('\nShutting down gracefully...');

  // Shutdown Kepware connection
  if (kepwareManager) {
    await kepwareManager.shutdown();
  }

  db.close((err) => {
    if (err) {
      console.error('Error closing database:', err);
    }
    process.exit(0);
  });
});
