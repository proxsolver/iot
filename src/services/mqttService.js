/**
 * MQTT Service
 * MQTT broker and client for IoT messaging
 */

const mqtt = require('mqtt');
const Aedes = require('aedes');
const { createServer } = require('net');
const logger = require('../config/logger');

class MQTTService {
  constructor(config = {}) {
    this.enabled = config.enabled === 'false' ? false : true;
    this.brokerPort = config.port || 1883;
    this.brokerHost = config.host || '0.0.0.0';
    this.broker = null;
    this.server = null;
    this.clients = new Map();
    this.subscriptions = new Map();
  }

  /**
   * Start MQTT broker
   */
  startBroker() {
    if (!this.enabled) {
      logger.info('MQTT broker is disabled');
      return;
    }

    try {
      // Create Aedes broker instance
      this.broker = new Aedes({
        id: 'iot-mqtt-broker',
        heartbeatInterval: 60000
      });

      // Create TCP server
      this.server = createServer(this.broker.handle);

      // Start listening
      this.server.listen(this.brokerPort, this.brokerHost, () => {
        logger.info(`MQTT broker listening on ${this.brokerHost}:${this.brokerPort}`);
      });

      // Broker event handlers
      this.broker.on('client', (client) => {
        logger.info(`MQTT client connected: ${client.id}`);
        this.clients.set(client.id, {
          client,
          connected: true,
          connectedAt: new Date()
        });
      });

      this.broker.on('clientDisconnect', (client) => {
        logger.info(`MQTT client disconnected: ${client.id}`);
        const clientInfo = this.clients.get(client.id);
        if (clientInfo) {
          clientInfo.connected = false;
          clientInfo.disconnectedAt = new Date();
        }
      });

      this.broker.on('subscribe', (subscriptions, client) => {
        logger.debug(`MQTT client ${client.id} subscribed to:`, subscriptions);
        this.subscriptions.set(client.id, subscriptions);
      });

      this.broker.on('unsubscribe', (subscriptions, client) => {
        logger.debug(`MQTT client ${client.id} unsubscribed from:`, subscriptions);
      });

      this.broker.on('publish', (packet, client) => {
        if (client) {
          logger.debug(`MQTT message published by ${client.id} to ${packet.topic}`);
        }
      });

      return true;
    } catch (error) {
      logger.error('Failed to start MQTT broker:', error);
      return false;
    }
  }

  /**
   * Stop MQTT broker
   */
  stopBroker() {
    return new Promise((resolve) => {
      if (this.server) {
        this.server.close(() => {
          logger.info('MQTT broker stopped');
          resolve();
        });
      } else {
        resolve();
      }
    });
  }

  /**
   * Create MQTT client
   */
  createClient(clientId, options = {}) {
    if (!this.enabled) {
      logger.warn('MQTT is disabled, cannot create client');
      return null;
    }

    try {
      const connectUrl = `mqtt://${this.brokerHost}:${this.brokerPort}`;
      const client = mqtt.connect(connectUrl, {
        clientId: clientId,
        clean: true,
        connectTimeout: 4000,
        reconnectPeriod: 1000,
        ...options
      });

      client.on('connect', () => {
        logger.info(`MQTT client ${clientId} connected`);
      });

      client.on('error', (error) => {
        logger.error(`MQTT client ${clientId} error:`, error);
      });

      client.on('close', () => {
        logger.info(`MQTT client ${clientId} connection closed`);
      });

      client.on('reconnect', () => {
        logger.info(`MQTT client ${clientId} reconnecting...`);
      });

      return client;
    } catch (error) {
      logger.error(`Failed to create MQTT client ${clientId}:`, error);
      return null;
    }
  }

  /**
   * Publish message to topic
   */
  publish(client, topic, message, options = {}) {
    if (!client || !client.connected) {
      logger.error('MQTT client not connected');
      return false;
    }

    try {
      const payload = typeof message === 'object' ? JSON.stringify(message) : message;
      client.publish(topic, payload, { qos: 0, retain: false, ...options }, (error) => {
        if (error) {
          logger.error(`Failed to publish to ${topic}:`, error);
        } else {
          logger.debug(`Published to ${topic}:`, payload);
        }
      });
      return true;
    } catch (error) {
      logger.error(`Error publishing to ${topic}:`, error);
      return false;
    }
  }

  /**
   * Subscribe to topic
   */
  subscribe(client, topic, callback) {
    if (!client || !client.connected) {
      logger.error('MQTT client not connected');
      return false;
    }

    try {
      client.subscribe(topic, { qos: 0 }, (error) => {
        if (error) {
          logger.error(`Failed to subscribe to ${topic}:`, error);
        } else {
          logger.info(`Subscribed to ${topic}`);
        }
      });

      client.on('message', (receivedTopic, message) => {
        if (receivedTopic === topic) {
          try {
            const payload = JSON.parse(message.toString());
            callback(topic, payload);
          } catch (error) {
            callback(topic, message.toString());
          }
        }
      });

      return true;
    } catch (error) {
      logger.error(`Error subscribing to ${topic}:`, error);
      return false;
    }
  }

  /**
   * Publish sensor data
   */
  publishSensorData(client, sensorData) {
    const topic = `iot/sensor/${sensorData.device_id}`;
    return this.publish(client, topic, sensorData);
  }

  /**
   * Publish detection data
   */
  publishDetection(client, detectionData) {
    const topic = `iot/detection/camera${detectionData.camera_id}`;
    return this.publish(client, topic, detectionData);
  }

  /**
   * Publish alert
   */
  publishAlert(client, alertData) {
    const topic = `iot/alert/${alertData.severity}`;
    return this.publish(client, topic, alertData);
  }

  /**
   * Publish system status
   */
  publishSystemStatus(client, statusData) {
    const topic = `iot/status/${statusData.device_id}`;
    return this.publish(client, topic, statusData);
  }

  /**
   * Get broker statistics
   */
  getStats() {
    const connectedClients = Array.from(this.clients.values()).filter(c => c.connected);
    return {
      enabled: this.enabled,
      host: this.brokerHost,
      port: this.brokerPort,
      connectedClients: connectedClients.length,
      totalClients: this.clients.size,
      subscriptions: this.subscriptions.size
    };
  }
}

// Singleton instance
let mqttService = null;

/**
 * Get MQTT service instance
 */
function getMQTTService() {
  if (!mqttService) {
    mqttService = new MQTTService({
      enabled: process.env.MQTT_ENABLED,
      port: process.env.MQTT_PORT,
      host: process.env.MQTT_HOST
    });
  }
  return mqttService;
}

module.exports = {
  MQTTService,
  getMQTTService
};
