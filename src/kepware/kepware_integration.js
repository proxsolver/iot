/**
 * Kepware Integration Manager
 *
 * This module integrates Kepware OPC-UA client with the main data pipeline.
 * It automatically pushes IoT sensor data to Kepware tags for SCADA/HMI consumption.
 *
 * Usage:
 *   const kepwInt = new KepwareIntegrationManager();
 *   await kepwInt.initialize();
 *   kepwInt.start(); // Start automatic data push
 */

const { KepwareOPCUAClient, TAG_MAPPING } = require('./kepware_opcua_client');

// ===========================================
// CONFIGURATION
// ===========================================

const INTEGRATION_CONFIG = {
  enabled: process.env.KEPWARE_ENABLED === 'true',
  autoReconnect: true,
  reconnectInterval: 30000, // 30 seconds

  // Data push intervals
  sensorDataInterval: 5000,    // Push sensor data every 5 seconds
  statusInterval: 10000,       // Push status every 10 seconds
  controlTagInterval: 2000,    // Read control tags every 2 seconds

  // Retry settings
  maxRetries: 3,
  retryDelay: 2000
};

// ===========================================
// KEPWARE INTEGRATION MANAGER CLASS
// ===========================================

class KepwareIntegrationManager {
  constructor(config = INTEGRATION_CONFIG) {
    this.config = config;
    this.client = new KepwareOPCUAClient();
    this.enabled = config.enabled;

    // Intervals
    this.sensorInterval = null;
    this.statusInterval = null;
    this.controlInterval = null;

    // Latest data cache
    this.latestSensorData = null;
    this.latestDetectionData = null;
    this.latestStatusData = null;

    // Event callbacks
    this.onControlCommand = null;
    this.onConnectionChange = null;

    // Connection state
    this.isConnected = false;
  }

  /**
   * Initialize the Kepware integration
   */
  async initialize() {
    if (!this.enabled) {
      console.log('Kepware integration is disabled');
      return false;
    }

    try {
      console.log('Initializing Kepware integration...');

      // Test connection
      const testResult = await this.client.testConnection();

      if (testResult.success) {
        console.log('✓ Kepware connection test successful');
        this.isConnected = true;

        // Connect for real
        await this.client.connect();

        // Notify connection change
        if (this.onConnectionChange) {
          this.onConnectionChange(true);
        }

        // Set up control tag monitoring
        await this.setupControlTagMonitoring();

        return true;
      } else {
        console.error('✗ Kepware connection test failed:', testResult.message);
        return false;
      }
    } catch (error) {
      console.error('Failed to initialize Kepware integration:', error.message);

      // Schedule reconnection attempt
      if (this.config.autoReconnect) {
        console.log(`Scheduling reconnection in ${this.config.reconnectInterval}ms`);
        setTimeout(() => this.initialize(), this.config.reconnectInterval);
      }

      return false;
    }
  }

  /**
   * Set up monitoring of control tags from HMI/SCADA
   */
  async setupControlTagMonitoring() {
    const tagsToMonitor = [
      {
        tag: TAG_MAPPING.systemEnable,
        samplingInterval: 1000,
        callback: (data) => this.handleControlTagChange('systemEnable', data)
      },
      {
        tag: TAG_MAPPING.alarmReset,
        samplingInterval: 1000,
        callback: (data) => this.handleControlTagChange('alarmReset', data)
      },
      {
        tag: TAG_MAPPING.updateInterval,
        samplingInterval: 1000,
        callback: (data) => this.handleControlTagChange('updateInterval', data)
      }
    ];

    await this.client.subscribeToTags(tagsToMonitor);
    console.log('Control tag monitoring set up');
  }

  /**
   * Handle control tag changes from HMI/SCADA
   */
  handleControlTagChange(tagName, data) {
    console.log(`Control tag changed: ${tagName} = ${data.value}`);

    if (this.onControlCommand) {
      this.onControlCommand({
        tag: tagName,
        value: data.value,
        timestamp: data.timestamp
      });
    }

    // Handle alarm reset command
    if (tagName === 'alarmReset' && data.value === true) {
      this.client.clearAlert();
      console.log('Alarm cleared via Kepware command');
    }
  }

  /**
   * Start the integration (begin pushing data)
   */
  start() {
    if (!this.enabled || !this.isConnected) {
      console.warn('Cannot start: Kepware integration not ready');
      return;
    }

    console.log('Starting Kepware integration...');

    // Start sensor data push interval
    this.sensorInterval = setInterval(async () => {
      if (this.latestSensorData) {
        try {
          await this.client.pushSensorData(this.latestSensorData);
        } catch (error) {
          console.error('Error pushing sensor data:', error.message);
        }
      }
    }, this.config.sensorDataInterval);

    // Start status push interval
    this.statusInterval = setInterval(async () => {
      if (this.latestStatusData) {
        try {
          await this.client.pushLoRaWANStatus(this.latestStatusData);
        } catch (error) {
          console.error('Error pushing status:', error.message);
        }
      }
    }, this.config.statusInterval);

    // Start control tag read interval
    this.controlInterval = setInterval(async () => {
      try {
        const controlData = await this.client.readControlTags();
        if (controlData && this.onControlCommand) {
          this.onControlCommand({ source: 'poll', data: controlData });
        }
      } catch (error) {
        console.error('Error reading control tags:', error.message);
      }
    }, this.config.controlTagInterval);

    console.log('Kepware integration started');
    console.log(`  - Sensor data interval: ${this.config.sensorDataInterval}ms`);
    console.log(`  - Status interval: ${this.config.statusInterval}ms`);
    console.log(`  - Control tag interval: ${this.config.controlTagInterval}ms`);
  }

  /**
   * Stop the integration
   */
  stop() {
    console.log('Stopping Kepware integration...');

    if (this.sensorInterval) {
      clearInterval(this.sensorInterval);
      this.sensorInterval = null;
    }

    if (this.statusInterval) {
      clearInterval(this.statusInterval);
      this.statusInterval = null;
    }

    if (this.controlInterval) {
      clearInterval(this.controlInterval);
      this.controlInterval = null;
    }

    console.log('Kepware integration stopped');
  }

  /**
   * Update sensor data (called by data pipeline)
   */
  updateSensorData(sensorData) {
    this.latestSensorData = sensorData;
  }

  /**
   * Push detection data immediately
   */
  async pushDetection(detectionData) {
    if (!this.enabled || !this.isConnected) {
      return;
    }

    try {
      await this.client.pushDetectionData(detectionData);

      // Also push alert if high confidence
      if (detectionData.confidence > 0.8) {
        await this.client.pushAlertData({
          type: 'detection',
          severity: detectionData.confidence > 0.9 ? 'high' : 'medium',
          class: detectionData.class_name
        });
      }
    } catch (error) {
      console.error('Error pushing detection:', error.message);
    }
  }

  /**
   * Update system status
   */
  updateStatus(statusData) {
    this.latestStatusData = statusData;
  }

  /**
   * Push alert immediately
   */
  async pushAlert(alertData) {
    if (!this.enabled || !this.isConnected) {
      return;
    }

    try {
      await this.client.pushAlertData(alertData);
    } catch (error) {
      console.error('Error pushing alert:', error.message);
    }
  }

  /**
   * Clear alert
   */
  async clearAlert() {
    if (!this.enabled || !this.isConnected) {
      return;
    }

    try {
      await this.client.clearAlert();
    } catch (error) {
      console.error('Error clearing alert:', error.message);
    }
  }

  /**
   * Set callback for control commands
   */
  onControlCommand(callback) {
    this.onControlCommand = callback;
  }

  /**
   * Set callback for connection state changes
   */
  onConnectionChange(callback) {
    this.onConnectionChange = callback;
  }

  /**
   * Shutdown the integration
   */
  async shutdown() {
    console.log('Shutting down Kepware integration...');
    this.stop();
    await this.client.disconnect();
    this.isConnected = false;
    console.log('Kepware integration shutdown complete');
  }

  /**
   * Get connection status
   */
  getStatus() {
    return {
      enabled: this.enabled,
      connected: this.isConnected,
      latestSensorUpdate: this.latestSensorData?.timestamp,
      latestStatusUpdate: this.latestStatusData?.timestamp
    };
  }
}

// ===========================================
// EXPRESS ROUTE INTEGRATION
// ===========================================

/**
 * Add Kepware-related routes to Express app
 */
function setupKepwareRoutes(app, kepwManager) {
  // GET /api/kepware/status - Get Kepware connection status
  app.get('/api/kepware/status', (req, res) => {
    res.json(kepwManager.getStatus());
  });

  // POST /api/kepware/connect - Connect to Kepware
  app.post('/api/kepware/connect', async (req, res) => {
    try {
      const result = await kepwManager.initialize();
      if (result) {
        kepwManager.start();
      }
      res.json({ success: result });
    } catch (error) {
      res.status(500).json({ error: error.message });
    }
  });

  // POST /api/kepware/disconnect - Disconnect from Kepware
  app.post('/api/kepware/disconnect', async (req, res) => {
    try {
      await kepwManager.shutdown();
      res.json({ success: true });
    } catch (error) {
      res.status(500).json({ error: error.message });
    }
  });

  // GET /api/kepware/tags - Read all Kepware tags
  app.get('/api/kepware/tags', async (req, res) => {
    try {
      const tags = await kepwManager.client.readTags(Object.values(TAG_MAPPING));
      res.json(tags);
    } catch (error) {
      res.status(500).json({ error: error.message });
    }
  });

  // POST /api/kepware/write - Write to a specific tag
  app.post('/api/kepware/write', async (req, res) => {
    const { tag, value } = req.body;

    if (!tag || value === undefined) {
      return res.status(400).json({ error: 'Tag and value required' });
    }

    try {
      const result = await kepwManager.client.writeTag(tag, value);
      res.json(result);
    } catch (error) {
      res.status(500).json({ error: error.message });
    }
  });

  // POST /api/kepware/alert/clear - Clear active alert
  app.post('/api/kepware/alert/clear', async (req, res) => {
    try {
      await kepwManager.client.clearAlert();
      res.json({ success: true });
    } catch (error) {
      res.status(500).json({ error: error.message });
    }
  });

  // GET /api/kepware/control - Read control tags
  app.get('/api/kepware/control', async (req, res) => {
    try {
      const controlData = await kepwManager.client.readControlTags();
      res.json(controlData);
    } catch (error) {
      res.status(500).json({ error: error.message });
    }
  });
}

// ===========================================
// EXPORTS
// ===========================================

module.exports = {
  KepwareIntegrationManager,
  setupKepwareRoutes,
  INTEGRATION_CONFIG,
  TAG_MAPPING
};
