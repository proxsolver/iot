/**
 * Enhanced Kepware (KEPServerEX) OPC-UA Client Integration
 *
 * Complete production-ready OPC-UA client with:
 * - Automatic reconnection with exponential backoff
 * - Change detection and batch operations
 * - Tag subscription with data buffering
 * - Alarm management
 * - Control command processing
 * - Connection health monitoring
 * - Comprehensive error handling
 *
 * Technology: node-opcua v2.125.0
 */

const {
  OPCUAClient,
  MessageSecurityMode,
  SecurityPolicy,
  AttributeIds,
  makeBrowsePath,
  TimestampsToReturn,
  DataType,
  Variant
} = require('node-opcua');

const logger = require('../config/logger');

// ===========================================
// CONFIGURATION
// ===========================================

const KEPWARE_CONFIG = {
  // Kepware OPC-UA Server Endpoint
  endpoint: process.env.KEPWARE_ENDPOINT || 'opc.tcp://localhost:49320',

  // Security settings
  securityMode: MessageSecurityMode.None,
  securityPolicy: SecurityPolicy.None,

  // Connection settings
  connectionStrategy: {
    maxRetry: Infinity,
    initialDelay: 1000,
    maxDelay: 60000
  },

  // Session settings
  sessionTimeout: 60000,
  requestedSessionTimeout: 60000,

  // Reconnection settings
  autoReconnect: true,
  reconnectInterval: 5000,
  maxReconnectAttempts: 100,
  reconnectBackoffMultiplier: 1.5,

  // Subscription settings
  subscriptionPublishingInterval: 1000,
  subscriptionLifetimeCount: 100,
  subscriptionMaxKeepAliveCount: 10,
  subscriptionMaxNotificationsPerPublish: 100,

  // Data buffering
  enableDataBuffering: true,
  maxBufferSize: 10000,

  // Health monitoring
  healthCheckInterval: 30000
};

// Tag mapping - IoT data to Kepware tags
const TAG_MAPPING = {
  // Environmental Sensor Data
  temperature: 'Channel1.IoTGateway.NiclaSense_Temperature',
  humidity: 'Channel1.IoTGateway.NiclaSense_Humidity',
  pressure: 'Channel1.IoTGateway.NiclaSense_Pressure',
  gasResistance: 'Channel1.IoTGateway.NiclaSense_GasResistance',

  // Vision Detection Data
  camera1DetectedClass: 'Channel1.IoTGateway.Camera1_DetectedClass',
  camera1Confidence: 'Channel1.IoTGateway.Camera1_Confidence',
  camera1DetectionCount: 'Channel1.IoTGateway.Camera1_DetectionCount',

  camera2DetectedClass: 'Channel1.IoTGateway.Camera2_DetectedClass',
  camera2Confidence: 'Channel1.IoTGateway.Camera2_Confidence',
  camera2DetectionCount: 'Channel1.IoTGateway.Camera2_DetectionCount',

  // System Status
  gatewayStatus: 'Channel1.IoTGateway.System_Status',
  batteryLevel: 'Channel1.IoTGateway.System_BatteryLevel',
  signalStrength: 'Channel1.IoTGateway.System_SignalStrength',
  uptime: 'Channel1.IoTGateway.System_Uptime',

  // Alarm/Alert Status
  alertActive: 'Channel1.IoTGateway.Alarm_Active',
  alertType: 'Channel1.IoTGateway.Alarm_Type',
  alertSeverity: 'Channel1.IoTGateway.Alarm_Severity',

  // LoRaWAN Status
  loraConnected: 'Channel1.IoTGateway.LoRa_Connected',
  loraLastRSSI: 'Channel1.IoTGateway.LoRa_LastRSSI',
  loraLastSNR: 'Channel1.IoTGateway.LoRa_LastSNR',

  // Control Tags
  systemEnable: 'Channel1.IoTGateway.Control_SystemEnable',
  alarmReset: 'Channel1.IoTGateway.Control_AlarmReset',
  updateInterval: 'Channel1.IoTGateway.Control_UpdateInterval'
};

// ===========================================
// ENHANCED KEPWARE OPC-UA CLIENT CLASS
// ===========================================

class EnhancedKepwareOPCUAClient {
  constructor(config = KEPWARE_CONFIG) {
    this.config = config;
    this.client = null;
    this.session = null;
    this.subscription = null;
    this.monitoredItems = new Map();
    this.connected = false;
    this.connecting = false;

    // Reconnection state
    this.reconnectAttempts = 0;
    this.reconnectTimer = null;
    this.reconnectDelay = this.config.connectionStrategy.initialDelay;

    // Data buffering
    this.dataBuffer = [];
    this.maxBufferSize = this.config.maxBufferSize;

    // Node ID cache
    this.nodeIdCache = new Map();

    // Callbacks
    this.tagChangeCallbacks = new Map();
    this.onConnectionChange = null;
    this.onError = null;

    // Health monitoring
    this.healthCheckTimer = null;
    this.lastHealthCheck = null;
    this.connectionHealthy = true;

    // Statistics
    this.stats = {
      connectCount: 0,
      disconnectCount: 0,
      reconnectCount: 0,
      readCount: 0,
      writeCount: 0,
      errorCount: 0,
      lastReadTime: null,
      lastWriteTime: null,
      lastError: null
    };
  }

  /**
   * Connect to Kepware OPC-UA Server with automatic retry
   */
  async connect() {
    if (this.connected || this.connecting) {
      return true;
    }

    this.connecting = true;

    try {
      logger.info(`Connecting to Kepware at ${this.config.endpoint}...`);

      this.client = OPCUAClient.create({
        endpointMustExist: false,
        securityMode: this.config.securityMode,
        securityPolicy: this.config.securityPolicy,
        connectionStrategy: this.config.connectionStrategy
      });

      // Set up client event handlers
      this.client.on('backoff', (retry, delay) => {
        logger.info(`Connection retry ${retry}, next attempt in ${delay}ms`);
      });

      this.client.on('connection_reestablished', () => {
        logger.info('Connection reestablished');
      });

      await this.client.connect(this.config.endpoint);
      logger.info('Connected to Kepware OPC-UA Server');

      // Create session
      this.session = await this.client.createSession({
        timeout: this.config.sessionTimeout,
        sessionName: 'IoTGatewaySession'
      });

      logger.info('OPC-UA Session created');
      this.connected = true;
      this.connecting = false;
      this.reconnectAttempts = 0;
      this.reconnectDelay = this.config.connectionStrategy.initialDelay;

      this.stats.connectCount++;
      this.stats.lastConnectTime = new Date();

      // Set up session event handlers
      this.session.on('keepalive', () => {
        logger.debug('OPC-UA Session keepalive');
        this.connectionHealthy = true;
      });

      this.session.on('session_closed', (statusCode) => {
        logger.warn(`OPC-UA Session closed: ${statusCode?.toString()}`);
        this.handleDisconnection();
      });

      // Start health monitoring
      this.startHealthMonitoring();

      // Notify connection change
      if (this.onConnectionChange) {
        this.onConnectionChange(true);
      }

      // Process buffered data
      if (this.dataBuffer.length > 0) {
        logger.info(`Processing ${this.dataBuffer.length} buffered data points`);
        await this.processDataBuffer();
      }

      return true;
    } catch (error) {
      logger.error('Failed to connect to Kepware:', error.message);
      this.connecting = false;
      this.connected = false;

      this.stats.errorCount++;
      this.stats.lastError = {
        message: error.message,
        timestamp: new Date()
      };

      if (this.onError) {
        this.onError(error);
      }

      // Schedule reconnection
      if (this.config.autoReconnect) {
        this.scheduleReconnect();
      }

      throw error;
    }
  }

  /**
   * Disconnect from Kepware
   */
  async disconnect() {
    try {
      this.stopHealthMonitoring();
      this.clearReconnectTimer();

      if (this.subscription) {
        await this.subscription.terminate();
        this.subscription = null;
      }

      if (this.session) {
        await this.session.close();
        this.session = null;
      }

      if (this.client) {
        await this.client.disconnect();
        this.client = null;
      }

      this.connected = false;
      this.stats.disconnectCount++;

      logger.info('Disconnected from Kepware');
    } catch (error) {
      logger.error('Error disconnecting from Kepware:', error.message);
    }
  }

  /**
   * Handle unexpected disconnection
   */
  async handleDisconnection() {
    this.connected = false;
    this.connectionHealthy = false;
    this.stats.disconnectCount++;

    logger.warn('Kepware connection lost');

    if (this.onConnectionChange) {
      this.onConnectionChange(false);
    }

    if (this.config.autoReconnect) {
      this.scheduleReconnect();
    }
  }

  /**
   * Schedule reconnection attempt
   */
  scheduleReconnect() {
    this.clearReconnectTimer();

    if (this.reconnectAttempts >= this.config.maxReconnectAttempts) {
      logger.error('Max reconnection attempts reached');
      return;
    }

    this.reconnectAttempts++;
    const delay = Math.min(
      this.reconnectDelay * Math.pow(this.config.reconnectBackoffMultiplier, this.reconnectAttempts - 1),
      this.config.connectionStrategy.maxDelay
    );

    logger.info(`Scheduling reconnection attempt ${this.reconnectAttempts} in ${delay}ms`);

    this.reconnectTimer = setTimeout(async () => {
      try {
        await this.connect();
        this.stats.reconnectCount++;
      } catch (error) {
        logger.error(`Reconnection attempt ${this.reconnectAttempts} failed:`, error.message);
        this.scheduleReconnect();
      }
    }, delay);
  }

  /**
   * Clear reconnection timer
   */
  clearReconnectTimer() {
    if (this.reconnectTimer) {
      clearTimeout(this.reconnectTimer);
      this.reconnectTimer = null;
    }
  }

  /**
   * Start health monitoring
   */
  startHealthMonitoring() {
    this.stopHealthMonitoring();

    this.healthCheckTimer = setInterval(async () => {
      try {
        if (!this.session || !this.connected) {
          this.connectionHealthy = false;
          return;
        }

        // Read a simple tag to verify connection
        await this.session.read({
          nodeId: 'i=2253', // Server status
          attributeId: AttributeIds.Value
        });

        this.lastHealthCheck = new Date();
        this.connectionHealthy = true;
      } catch (error) {
        logger.error('Health check failed:', error.message);
        this.connectionHealthy = false;

        if (this.connected) {
          this.handleDisconnection();
        }
      }
    }, this.config.healthCheckInterval);
  }

  /**
   * Stop health monitoring
   */
  stopHealthMonitoring() {
    if (this.healthCheckTimer) {
      clearInterval(this.healthCheckTimer);
      this.healthCheckTimer = null;
    }
  }

  /**
   * Get NodeId from tag path with caching
   */
  async getNodeId(tagPath) {
    if (!this.session) {
      throw new Error('Not connected to Kepware');
    }

    // Check cache first
    if (this.nodeIdCache.has(tagPath)) {
      return this.nodeIdCache.get(tagPath);
    }

    try {
      const browsePath = makeBrowsePath('RootFolder', tagPath);
      const result = await this.session.translateBrowsePath(browsePath);

      if (result.statusCode.name === 'Good' && result.targets.length > 0) {
        const nodeId = result.targets[0].targetId;
        this.nodeIdCache.set(tagPath, nodeId);
        return nodeId;
      }

      throw new Error(`Tag not found: ${tagPath}`);
    } catch (error) {
      logger.error(`Error getting NodeId for ${tagPath}:`, error.message);
      throw error;
    }
  }

  /**
   * Read a single tag value
   */
  async readTag(tagPath) {
    if (!this.connected || !this.session) {
      if (this.config.enableDataBuffering) {
        logger.debug(`Buffering read for ${tagPath} (not connected)`);
        return { tag: tagPath, value: null, buffered: true };
      }
      throw new Error('Not connected to Kepware');
    }

    try {
      const nodeId = await this.getNodeId(tagPath);
      const dataValue = await this.session.read({
        nodeId,
        attributeId: AttributeIds.Value
      });

      this.stats.readCount++;
      this.stats.lastReadTime = new Date();

      return {
        tag: tagPath,
        value: dataValue.value?.value,
        statusCode: dataValue.statusCode?.name,
        quality: dataValue.statusCode?.name,
        timestamp: dataValue.sourceTimestamp,
        serverTimestamp: dataValue.serverTimestamp
      };
    } catch (error) {
      this.stats.errorCount++;
      logger.error(`Error reading tag ${tagPath}:`, error.message);
      throw error;
    }
  }

  /**
   * Batch read multiple tags
   */
  async readTags(tagPaths) {
    if (!this.connected || !this.session) {
      throw new Error('Not connected to Kepware');
    }

    try {
      const nodesToRead = [];

      for (const tagPath of tagPaths) {
        try {
          const nodeId = await this.getNodeId(tagPath);
          nodesToRead.push({
            nodeId,
            attributeId: AttributeIds.Value
          });
        } catch (error) {
          logger.error(`Error getting NodeId for ${tagPath}:`, error.message);
          nodesToRead.push(null);
        }
      }

      const results = await this.session.read(nodesToRead);

      this.stats.readCount += tagPaths.length;
      this.stats.lastReadTime = new Date();

      return results.map((result, index) => ({
        tag: tagPaths[index],
        value: result.value?.value,
        statusCode: result.statusCode?.name,
        quality: result.statusCode?.name,
        timestamp: result.sourceTimestamp,
        serverTimestamp: result.serverTimestamp
      }));
    } catch (error) {
      this.stats.errorCount++;
      logger.error('Error reading tags:', error.message);
      throw error;
    }
  }

  /**
   * Write a value to a single tag
   */
  async writeTag(tagPath, value, dataType = null) {
    if (!this.connected || !this.session) {
      if (this.config.enableDataBuffering) {
        this.bufferData({ type: 'write', tag: tagPath, value, dataType });
        return { success: true, tag: tagPath, value, buffered: true };
      }
      throw new Error('Not connected to Kepware');
    }

    try {
      const nodeId = await this.getNodeId(tagPath);

      const statusCode = await this.session.write({
        nodeId,
        attributeId: AttributeIds.Value,
        value: {
          value: new Variant({
            dataType: dataType || this.inferDataType(value),
            value: value
          })
        }
      });

      this.stats.writeCount++;
      this.stats.lastWriteTime = new Date();

      if (statusCode.isGood()) {
        logger.debug(`Wrote ${value} to ${tagPath}`);
        return { success: true, tag: tagPath, value };
      }

      throw new Error(`Write failed with status: ${statusCode.toString()}`);
    } catch (error) {
      this.stats.errorCount++;
      logger.error(`Error writing to tag ${tagPath}:`, error.message);
      throw error;
    }
  }

  /**
   * Batch write multiple tags
   */
  async writeTags(tagValuePairs) {
    const results = [];

    for (const { tag, value, dataType } of tagValuePairs) {
      try {
        const result = await this.writeTag(tag, value, dataType);
        results.push(result);
      } catch (error) {
        results.push({
          success: false,
          tag,
          error: error.message
        });
      }
    }

    return results;
  }

  /**
   * Subscribe to tag changes
   */
  async subscribeToTags(tagsToMonitor, callback) {
    if (!this.connected || !this.session) {
      throw new Error('Not connected to Kepware');
    }

    try {
      if (!this.subscription) {
        this.subscription = await this.session.createSubscription2({
          requestedPublishingInterval: this.config.subscriptionPublishingInterval,
          requestedLifetimeCount: this.config.subscriptionLifetimeCount,
          requestedMaxKeepAliveCount: this.config.subscriptionMaxKeepAliveCount,
          maxNotificationsPerPublish: this.config.subscriptionMaxNotificationsPerPublish,
          publishingEnabled: true,
          priority: 10
        });

        logger.info('Created OPC-UA subscription');
      }

      const monitoredItems = [];

      for (const tagConfig of tagsToMonitor) {
        try {
          const nodeId = await this.getNodeId(tagConfig.tag);
          const item = await this.subscription.monitor({
            nodeId,
            attributeId: AttributeIds.Value
          },
          {
            samplingInterval: tagConfig.samplingInterval || 1000,
            discardOldest: true,
            queueSize: 10
          },
          TimestampsToReturn.Both
          );

          item.on('changed', (dataValue) => {
            const changeData = {
              tag: tagConfig.tag,
              value: dataValue.value?.value,
              statusCode: dataValue.statusCode?.name,
              timestamp: dataValue.sourceTimestamp,
              serverTimestamp: dataValue.serverTimestamp
            };

            // Call callbacks
            if (tagConfig.callback) {
              tagConfig.callback(changeData);
            }
            if (callback) {
              callback(changeData);
            }

            this.monitoredItems.set(tagConfig.tag, changeData);
          });

          monitoredItems.push({
            tag: tagConfig.tag,
            monitoredItem: item
          });

          logger.info(`Subscribed to: ${tagConfig.tag}`);
        } catch (error) {
          logger.error(`Failed to subscribe to ${tagConfig.tag}:`, error.message);
        }
      }

      return monitoredItems;
    } catch (error) {
      logger.error('Error creating subscription:', error.message);
      throw error;
    }
  }

  /**
   * Buffer data for later processing
   */
  bufferData(data) {
    if (this.dataBuffer.length >= this.maxBufferSize) {
      this.dataBuffer.shift(); // Remove oldest
    }
    this.dataBuffer.push(data);
  }

  /**
   * Process buffered data
   */
  async processDataBuffer() {
    const buffer = [...this.dataBuffer];
    this.dataBuffer = [];

    for (const data of buffer) {
      try {
        if (data.type === 'write') {
          await this.writeTag(data.tag, data.value, data.dataType);
        }
      } catch (error) {
        logger.error('Error processing buffered data:', error.message);
      }
    }
  }

  /**
   * Infer data type from value
   */
  inferDataType(value) {
    if (typeof value === 'boolean') return DataType.Boolean;
    if (typeof value === 'number') {
      return Number.isInteger(value) ? DataType.Int32 : DataType.Double;
    }
    if (typeof value === 'string') return DataType.String;
    return DataType.Variant;
  }

  /**
   * Push IoT sensor data to Kepware
   */
  async pushSensorData(sensorData) {
    const tagValuePairs = [];

    if (sensorData.temperature !== undefined) {
      tagValuePairs.push({ tag: TAG_MAPPING.temperature, value: sensorData.temperature });
    }
    if (sensorData.humidity !== undefined) {
      tagValuePairs.push({ tag: TAG_MAPPING.humidity, value: sensorData.humidity });
    }
    if (sensorData.pressure !== undefined) {
      tagValuePairs.push({ tag: TAG_MAPPING.pressure, value: sensorData.pressure });
    }
    if (sensorData.gasResistance !== undefined) {
      tagValuePairs.push({ tag: TAG_MAPPING.gasResistance, value: sensorData.gasResistance });
    }
    if (sensorData.batteryLevel !== undefined) {
      tagValuePairs.push({ tag: TAG_MAPPING.batteryLevel, value: sensorData.batteryLevel });
    }
    if (sensorData.signalStrength !== undefined) {
      tagValuePairs.push({ tag: TAG_MAPPING.signalStrength, value: sensorData.signalStrength });
    }
    if (sensorData.uptime !== undefined) {
      tagValuePairs.push({ tag: TAG_MAPPING.uptime, value: sensorData.uptime });
    }
    if (sensorData.status !== undefined) {
      const statusValue = sensorData.status === 'online' || sensorData.status === true;
      tagValuePairs.push({ tag: TAG_MAPPING.gatewayStatus, value: statusValue });
    }

    if (tagValuePairs.length === 0) {
      logger.warn('No valid sensor data to push to Kepware');
      return [];
    }

    return await this.writeTags(tagValuePairs);
  }

  /**
   * Push detection data to Kepware
   */
  async pushDetectionData(detectionData) {
    const cameraPrefix = detectionData.camera_id === 1 ? 'camera1' : 'camera2';
    const tagValuePairs = [];

    if (detectionData.class_name !== undefined) {
      const classTag = TAG_MAPPING[`${cameraPrefix}DetectedClass`];
      tagValuePairs.push({ tag: classTag, value: detectionData.class_name });
    }

    if (detectionData.confidence !== undefined) {
      const confidenceTag = TAG_MAPPING[`${cameraPrefix}Confidence`];
      tagValuePairs.push({ tag: confidenceTag, value: Math.round(detectionData.confidence * 100) });
    }

    if (detectionData.incrementCount) {
      const countTag = TAG_MAPPING[`${cameraPrefix}DetectionCount`];
      const currentCount = await this.readTag(countTag).catch(() => ({ value: 0 }));
      tagValuePairs.push({ tag: countTag, value: (currentCount.value || 0) + 1 });
    }

    return await this.writeTags(tagValuePairs);
  }

  /**
   * Push alert data to Kepware
   */
  async pushAlertData(alertData) {
    const tagValuePairs = [];

    tagValuePairs.push({ tag: TAG_MAPPING.alertActive, value: true });

    if (alertData.type) {
      const typeMap = { detection: 1, system: 2, low_battery: 3, connection: 4 };
      tagValuePairs.push({ tag: TAG_MAPPING.alertType, value: typeMap[alertData.type] || 0 });
    }

    if (alertData.severity) {
      const severityMap = { low: 1, medium: 2, high: 3, critical: 4 };
      tagValuePairs.push({ tag: TAG_MAPPING.alertSeverity, value: severityMap[alertData.severity] || 0 });
    }

    return await this.writeTags(tagValuePairs);
  }

  /**
   * Clear alert in Kepware
   */
  async clearAlert() {
    await this.writeTags([
      { tag: TAG_MAPPING.alertActive, value: false },
      { tag: TAG_MAPPING.alertType, value: 0 },
      { tag: TAG_MAPPING.alertSeverity, value: 0 }
    ]);
  }

  /**
   * Read control tags
   */
  async readControlTags() {
    const controlTags = [
      TAG_MAPPING.systemEnable,
      TAG_MAPPING.alarmReset,
      TAG_MAPPING.updateInterval
    ];

    try {
      const values = await this.readTags(controlTags);

      return {
        systemEnabled: values[0]?.value,
        alarmReset: values[1]?.value,
        updateInterval: values[2]?.value
      };
    } catch (error) {
      logger.error('Error reading control tags:', error.message);
      return null;
    }
  }

  /**
   * Get statistics
   */
  getStatistics() {
    return {
      ...this.stats,
      connected: this.connected,
      connectionHealthy: this.connectionHealthy,
      bufferedDataCount: this.dataBuffer.length,
      monitoredItemsCount: this.monitoredItems.size,
      nodeIdCacheSize: this.nodeIdCache.size,
      lastHealthCheck: this.lastHealthCheck
    };
  }

  /**
   * Test connection
   */
  async testConnection() {
    try {
      await this.connect();
      await this.readTag(TAG_MAPPING.gatewayStatus);
      await this.disconnect();
      return { success: true, message: 'Connection successful' };
    } catch (error) {
      return { success: false, message: error.message };
    }
  }
}

// ===========================================
// EXPORTS
// ===========================================

module.exports = {
  EnhancedKepwareOPCUAClient,
  TAG_MAPPING,
  KEPWARE_CONFIG
};
