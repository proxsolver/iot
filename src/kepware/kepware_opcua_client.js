/**
 * Kepware (KEPServerEX) OPC-UA Client Integration
 *
 * This module provides bi-directional communication between the IoT system
 * and Kepware KEPServerEX via OPC-UA protocol.
 *
 * Features:
 * - Connect to KEPServerEX as OPC-UA client
 * - Read/write tags from Kepware
 * - Subscribe to tag changes for real-time updates
 * - Push IoT sensor data to Kepware tags
 * - SCADA/HMI integration
 *
 * Technology: node-opcua
 */

const { OPCUAClient, MessageSecurityMode, SecurityPolicy, AttributeIds, makeBrowsePath } = require('node-opcua');
const { Kafka } = require('kafkajs'); // Optional: For industrial messaging

// ===========================================
// CONFIGURATION
// ===========================================

const KEPWARE_CONFIG = {
  // Kepware OPC-UA Server Endpoint
  endpoint: process.env.KEPWARE_ENDPOINT || 'opc.tcp://localhost:49320',

  // Security settings (adjust based on Kepware configuration)
  securityMode: MessageSecurityMode.None, // Use Sign or SignAndEncrypt for production
  securityPolicy: SecurityPolicy.None,

  // Connection settings
  connectionStrategy: {
    maxRetry: 3,
    initialDelay: 2000,
    maxDelay: 10000
  },

  // Session settings
  sessionTimeout: 60000,
  requestedSessionTimeout: 60000
};

// Tag mapping - IoT data to Kepware tags
// Format: Channel.Device.Tag (Kepware naming convention)
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

  // Control Tags (Read from Kepware, written by HMI/SCADA)
  systemEnable: 'Channel1.IoTGateway.Control_SystemEnable',
  alarmReset: 'Channel1.IoTGateway.Control_AlarmReset',
  updateInterval: 'Channel1.IoTGateway.Control_UpdateInterval'
};

// ===========================================
// KEPWARE OPC-UA CLIENT CLASS
// ===========================================

class KepwareOPCUAClient {
  constructor(config = KEPWARE_CONFIG) {
    this.config = config;
    this.client = null;
    this.session = null;
    this.subscription = null;
    this.monitoredItems = new Map();
    this.connected = false;
    this.reconnectAttempts = 0;

    // Callbacks for tag changes
    this.tagChangeCallbacks = new Map();
  }

  /**
   * Connect to Kepware OPC-UA Server
   */
  async connect() {
    try {
      console.log(`Connecting to Kepware at ${this.config.endpoint}...`);

      this.client = OPCUAClient.create({
        endpointMustExist: false,
        securityMode: this.config.securityMode,
        securityPolicy: this.config.securityPolicy,
        connectionStrategy: this.config.connectionStrategy
      });

      await this.client.connect(this.config.endpoint);
      console.log('Connected to Kepware OPC-UA Server');

      // Create session
      this.session = await this.client.createSession({
        timeout: this.config.sessionTimeout,
        sessionName: 'IoTGatewaySession'
      });

      console.log('OPC-UA Session created');
      this.connected = true;
      this.reconnectAttempts = 0;

      // Set up session keep-alive
      this.session.on('keepalive', () => {
        console.log('OPC-UA Session keepalive');
      });

      this.session.on('session_closed', () => {
        console.warn('OPC-UA Session closed');
        this.connected = false;
      });

      return true;
    } catch (error) {
      console.error('Failed to connect to Kepware:', error.message);
      this.connected = false;
      throw error;
    }
  }

  /**
   * Disconnect from Kepware
   */
  async disconnect() {
    try {
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
      console.log('Disconnected from Kepware');
    } catch (error) {
      console.error('Error disconnecting from Kepware:', error.message);
    }
  }

  /**
   * Get NodeId from tag path (browse path to NodeId conversion)
   */
  async getNodeId(tagPath) {
    if (!this.session) {
      throw new Error('Not connected to Kepware');
    }

    try {
      const browsePath = makeBrowsePath('RootFolder', tagPath);
      const result = await this.session.translateBrowsePath(browsePath);

      if (result.statusCode.name === 'Good' && result.targets.length > 0) {
        return result.targets[0].targetId;
      }

      throw new Error(`Tag not found: ${tagPath}`);
    } catch (error) {
      console.error(`Error getting NodeId for ${tagPath}:`, error.message);
      throw error;
    }
  }

  /**
   * Read a single tag value from Kepware
   */
  async readTag(tagPath) {
    if (!this.connected || !this.session) {
      throw new Error('Not connected to Kepware');
    }

    try {
      const nodeId = await this.getNodeId(tagPath);
      const dataValue = await this.session.read({
        nodeId,
        attributeId: AttributeIds.Value
      });

      return {
        tag: tagPath,
        value: dataValue.value.value,
        quality: dataValue.statusCode.name,
        timestamp: dataValue.sourceTimestamp
      };
    } catch (error) {
      console.error(`Error reading tag ${tagPath}:`, error.message);
      throw error;
    }
  }

  /**
   * Read multiple tags at once
   */
  async readTags(tagPaths) {
    if (!this.connected || !this.session) {
      throw new Error('Not connected to Kepware');
    }

    try {
      const nodeIds = await Promise.all(
        tagPaths.map(tag => this.getNodeId(tag).catch(() => null))
      );

      const results = await this.session.read({
        nodesToRead: nodeIds
          .filter((id, index) => id !== null)
          .map(nodeId => ({ nodeId, attributeId: AttributeIds.Value }))
      });

      return results.map((result, index) => ({
        tag: tagPaths[index],
        value: result.value?.value,
        quality: result.statusCode?.name,
        timestamp: result.sourceTimestamp
      }));
    } catch (error) {
      console.error('Error reading tags:', error.message);
      throw error;
    }
  }

  /**
   * Write a value to a single tag
   */
  async writeTag(tagPath, value) {
    if (!this.connected || !this.session) {
      throw new Error('Not connected to Kepware');
    }

    try {
      const nodeId = await this.getNodeId(tagPath);

      const statusCode = await this.session.write({
        nodeId,
        attributeId: AttributeIds.Value,
        value: {
          value: { dataType: 'Variant', value }
        }
      });

      if (statusCode.isGood()) {
        console.log(`Wrote ${value} to ${tagPath}`);
        return { success: true, tag: tagPath, value };
      }

      throw new Error(`Write failed with status: ${statusCode.toString()}`);
    } catch (error) {
      console.error(`Error writing to tag ${tagPath}:`, error.message);
      throw error;
    }
  }

  /**
   * Write multiple tags at once
   */
  async writeTags(tagValuePairs) {
    const results = [];

    for (const { tag, value } of tagValuePairs) {
      try {
        const result = await this.writeTag(tag, value);
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
   * Subscribe to tag changes for real-time monitoring
   */
  async subscribeToTags(tagsToMonitor, callback) {
    if (!this.connected || !this.session) {
      throw new Error('Not connected to Kepware');
    }

    try {
      // Create subscription
      if (!this.subscription) {
        this.subscription = await this.session.createSubscription2({
          requestedPublishingInterval: 1000,
          requestedLifetimeCount: 100,
          requestedMaxKeepAliveCount: 10,
          maxNotificationsPerPublish: 100,
          publishingEnabled: true,
          priority: 10
        });

        console.log('Created OPC-UA subscription');
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
              value: dataValue.value.value,
              quality: dataValue.statusCode.name,
              timestamp: dataValue.sourceTimestamp
            };

            // Call the callback for this specific tag
            if (tagConfig.callback) {
              tagConfig.callback(changeData);
            }

            // Call the global callback
            if (callback) {
              callback(changeData);
            }

            // Store in monitored items map
            this.monitoredItems.set(tagConfig.tag, changeData);
          });

          monitoredItems.push({
            tag: tagConfig.tag,
            monitoredItem: item
          });

          console.log(`Subscribed to: ${tagConfig.tag}`);
        } catch (error) {
          console.error(`Failed to subscribe to ${tagConfig.tag}:`, error.message);
        }
      }

      return monitoredItems;
    } catch (error) {
      console.error('Error creating subscription:', error.message);
      throw error;
    }
  }

  /**
   * Push IoT sensor data to Kepware tags
   * This is the main method to send IoT data to Kepware
   */
  async pushSensorData(sensorData) {
    const tagValuePairs = [];

    // Environmental data
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

    // System status
    if (sensorData.batteryLevel !== undefined) {
      tagValuePairs.push({ tag: TAG_MAPPING.batteryLevel, value: sensorData.batteryLevel });
    }
    if (sensorData.signalStrength !== undefined) {
      tagValuePairs.push({ tag: TAG_MAPPING.signalStrength, value: sensorData.signalStrength });
    }
    if (sensorData.uptime !== undefined) {
      tagValuePairs.push({ tag: TAG_MAPPING.uptime, value: sensorData.uptime });
    }

    // Gateway status
    if (sensorData.status !== undefined) {
      // Convert status string to boolean
      const statusValue = sensorData.status === 'online' || sensorData.status === true;
      tagValuePairs.push({ tag: TAG_MAPPING.gatewayStatus, value: statusValue });
    }

    if (tagValuePairs.length === 0) {
      console.warn('No valid sensor data to push to Kepware');
      return [];
    }

    return await this.writeTags(tagValuePairs);
  }

  /**
   * Push detection data to Kepware tags
   */
  async pushDetectionData(detectionData) {
    const tagValuePairs = [];

    const cameraPrefix = detectionData.camera_id === 1 ? 'camera1' : 'camera2';

    // Detection class (convert to string for Kepware)
    if (detectionData.class_name !== undefined) {
      const classTag = TAG_MAPPING[`${cameraPrefix}DetectedClass`];
      // For string tags, Kepware needs proper configuration
      // We'll use numeric class_id as well
      tagValuePairs.push({ tag: classTag, value: detectionData.class_name });
    }

    // Confidence percentage
    if (detectionData.confidence !== undefined) {
      const confidenceTag = TAG_MAPPING[`${cameraPrefix}Confidence`];
      tagValuePairs.push({ tag: confidenceTag, value: Math.round(detectionData.confidence * 100) });
    }

    // Detection count increment
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

    // Alert active flag
    tagValuePairs.push({ tag: TAG_MAPPING.alertActive, value: true });

    // Alert type
    if (alertData.type) {
      // Convert type to number for Kepware
      const typeMap = { detection: 1, system: 2, low_battery: 3, connection: 4 };
      tagValuePairs.push({ tag: TAG_MAPPING.alertType, value: typeMap[alertData.type] || 0 });
    }

    // Alert severity
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
    await this.writeTag(TAG_MAPPING.alertActive, false);
    await this.writeTag(TAG_MAPPING.alertType, 0);
    await this.writeTag(TAG_MAPPING.alertSeverity, 0);
  }

  /**
   * Push LoRaWAN status to Kepware
   */
  async pushLoRaWANStatus(loraData) {
    const tagValuePairs = [];

    if (loraData.connected !== undefined) {
      tagValuePairs.push({ tag: TAG_MAPPING.loraConnected, value: loraData.connected });
    }
    if (loraData.rssi !== undefined) {
      tagValuePairs.push({ tag: TAG_MAPPING.loraLastRSSI, value: loraData.rssi });
    }
    if (loraData.snr !== undefined) {
      tagValuePairs.push({ tag: TAG_MAPPING.loraLastSNR, value: loraData.snr });
    }

    return await this.writeTags(tagValuePairs);
  }

  /**
   * Read control tags from Kepware (for receiving commands from HMI/SCADA)
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
      console.error('Error reading control tags:', error.message);
      return null;
    }
  }

  /**
   * Test connection to Kepware
   */
  async testConnection() {
    try {
      await this.connect();

      // Try to read a known tag
      const testTag = TAG_MAPPING.gatewayStatus;
      await this.readTag(testTag);

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
  KepwareOPCUAClient,
  TAG_MAPPING,
  KEPWARE_CONFIG
};
