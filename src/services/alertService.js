/**
 * Alert Service
 * Alert notification system with email and webhook support
 */

const nodemailer = require('nodemailer');
const https = require('https');
const http = require('http');
const { URL } = require('url');
const logger = require('../config/logger');
const db = require('../config/database');

class AlertService {
  constructor() {
    this.emailEnabled = process.env.ALERT_EMAIL_ENABLED === 'true';
    this.webhookEnabled = process.env.ALERT_WEBHOOK_ENABLED === 'true';
    this.emailConfig = {
      smtp: process.env.ALERT_EMAIL_SMTP,
      from: process.env.ALERT_EMAIL_FROM,
      to: process.env.ALERT_EMAIL_TO
    };
    this.webhookUrl = process.env.ALERT_WEBHOOK_URL;
    this.emailTransporter = null;
    this.alertCooldowns = new Map();
  }

  /**
   * Initialize email transporter
   */
  initializeEmail() {
    if (!this.emailEnabled) {
      logger.info('Email alerts are disabled');
      return;
    }

    try {
      this.emailTransporter = nodemailer.createTransporter(this.emailConfig.smtp);
      logger.info('Email transporter initialized');
    } catch (error) {
      logger.error('Failed to initialize email transporter:', error);
    }
  }

  /**
   * Send email alert
   */
  async sendEmail(alert) {
    if (!this.emailEnabled || !this.emailTransporter) {
      return { success: false, message: 'Email not enabled' };
    }

    try {
      const mailOptions = {
        from: this.emailConfig.from,
        to: this.emailConfig.to,
        subject: `[${alert.severity.toUpperCase()}] ${alert.alert_type} Alert`,
        text: `
Alert Type: ${alert.alert_type}
Severity: ${alert.severity}
Message: ${alert.message}
Time: ${new Date(alert.timestamp * 1000).toISOString()}

Data: ${JSON.stringify(alert.data, null, 2)}
        `,
        html: `
          <div style="font-family: Arial, sans-serif;">
            <h2 style="color: ${this.getSeverityColor(alert.severity)}">
              ${alert.severity.toUpperCase()} - ${alert.alert_type}
            </h2>
            <p><strong>Message:</strong> ${alert.message}</p>
            <p><strong>Time:</strong> ${new Date(alert.timestamp * 1000).toISOString()}</p>
            <pre style="background: #f4f4f4; padding: 10px; border-radius: 4px;">
${JSON.stringify(alert.data, null, 2)}
            </pre>
          </div>
        `
      };

      await this.emailTransporter.sendMail(mailOptions);
      logger.info(`Email alert sent: ${alert.alert_type}`);
      return { success: true };
    } catch (error) {
      logger.error('Failed to send email alert:', error);
      return { success: false, error: error.message };
    }
  }

  /**
   * Send webhook alert
   */
  async sendWebhook(alert) {
    if (!this.webhookEnabled || !this.webhookUrl) {
      return { success: false, message: 'Webhook not enabled' };
    }

    return new Promise((resolve) => {
      try {
        const url = new URL(this.webhookUrl);
        const isHttps = url.protocol === 'https:';
        const client = isHttps ? https : http;

        const payload = JSON.stringify({
          text: `Alert: ${alert.alert_type}`,
          blocks: [
            {
              type: 'section',
              text: {
                type: 'mrkdwn',
                text: `*${alert.severity.toUpperCase()} - ${alert.alert_type}*\n${alert.message}`
              }
            },
            {
              type: 'section',
              fields: [
                { type: 'mrkdwn', text: `*Severity:*\n${alert.severity}` },
                { type: 'mrkdwn', text: `*Time:*\n${new Date(alert.timestamp * 1000).toISOString()}` }
              ]
            }
          ]
        });

        const options = {
          hostname: url.hostname,
          port: url.port || (isHttps ? 443 : 80),
          path: url.pathname + url.search,
          method: 'POST',
          headers: {
            'Content-Type': 'application/json',
            'Content-Length': Buffer.byteLength(payload)
          }
        };

        const req = client.request(options, (res) => {
          let data = '';
          res.on('data', (chunk) => { data += chunk; });
          res.on('end', () => {
            if (res.statusCode >= 200 && res.statusCode < 300) {
              logger.info(`Webhook alert sent: ${alert.alert_type}`);
              resolve({ success: true, statusCode: res.statusCode });
            } else {
              logger.error(`Webhook failed with status ${res.statusCode}: ${data}`);
              resolve({ success: false, statusCode: res.statusCode, error: data });
            }
          });
        });

        req.on('error', (error) => {
          logger.error('Webhook request error:', error);
          resolve({ success: false, error: error.message });
        });

        req.write(payload);
        req.end();
      } catch (error) {
        logger.error('Failed to send webhook alert:', error);
        resolve({ success: false, error: error.message });
      }
    });
  }

  /**
   * Check if alert is in cooldown
   */
  isInCooldown(alertType, deviceId, cooldownPeriod = 300) {
    const key = `${alertType}:${deviceId}`;
    const lastSent = this.alertCooldowns.get(key);
    const now = Date.now() / 1000;

    if (lastSent && (now - lastSent) < cooldownPeriod) {
      return true;
    }

    return false;
  }

  /**
   * Set alert cooldown
   */
  setCooldown(alertType, deviceId) {
    const key = `${alertType}:${deviceId}`;
    this.alertCooldowns.set(key, Date.now() / 1000);
  }

  /**
   * Process alert and send notifications
   */
  async processAlert(alert) {
    try {
      // Check cooldown
      const cooldownPeriod = alert.data?.cooldown_period || 300;
      if (this.isInCooldown(alert.alert_type, alert.data?.device_id, cooldownPeriod)) {
        logger.debug(`Alert in cooldown: ${alert.alert_type}`);
        return { success: true, skipped: true, reason: 'cooldown' };
      }

      // Send notifications
      const results = {
        email: await this.sendEmail(alert),
        webhook: await this.sendWebhook(alert)
      };

      // Set cooldown if any notification was successful
      if (results.email.success || results.webhook.success) {
        this.setCooldown(alert.alert_type, alert.data?.device_id);
      }

      // Log alert to database
      this.logAlertToDatabase(alert, results);

      return results;
    } catch (error) {
      logger.error('Error processing alert:', error);
      return { success: false, error: error.message };
    }
  }

  /**
   * Log alert to database
   */
  logAlertToDatabase(alert, notificationResults) {
    try {
      db.execute(
        `INSERT INTO alerts (alert_type, severity, message, data, timestamp)
         VALUES (?, ?, ?, ?, ?)`,
        [
          alert.alert_type,
          alert.severity,
          alert.message,
          JSON.stringify({ ...alert.data, notificationResults }),
          alert.timestamp
        ]
      );
    } catch (error) {
      logger.error('Failed to log alert to database:', error);
    }
  }

  /**
   * Get severity color for HTML
   */
  getSeverityColor(severity) {
    const colors = {
      low: '#28a745',
      medium: '#ffc107',
      high: '#fd7e14',
      critical: '#dc3545'
    };
    return colors[severity] || '#6c757d';
  }

  /**
   * Create and process alert
   */
  async createAlert(alertType, severity, message, data = {}) {
    const alert = {
      alert_type: alertType,
      severity,
      message,
      data,
      timestamp: Math.floor(Date.now() / 1000)
    };

    return await this.processAlert(alert);
  }

  /**
   * Check alert rules and trigger if conditions met
   */
  async checkAlertRules(sensorData) {
    try {
      const rules = db.queryAll(
        'SELECT * FROM alert_rules WHERE enabled = 1'
      );

      for (const rule of rules) {
        const shouldTrigger = await this.evaluateRule(rule, sensorData);

        if (shouldTrigger) {
          await this.createAlert(
            'threshold',
            rule.severity,
            `Alert rule triggered: ${rule.name}`,
            {
              rule_id: rule.id,
              rule_name: rule.name,
              device_id: sensorData.device_id,
              sensor_type: rule.sensor_type,
              value: sensorData[rule.sensor_type]
            }
          );
        }
      }
    } catch (error) {
      logger.error('Error checking alert rules:', error);
    }
  }

  /**
   * Evaluate alert rule
   */
  async evaluateRule(rule, sensorData) {
    const value = sensorData[rule.sensor_type];

    if (value === undefined || value === null) {
      return false;
    }

    switch (rule.condition_type) {
      case 'threshold':
        if (rule.threshold_min !== null && value < rule.threshold_min) {
          return true;
        }
        if (rule.threshold_max !== null && value > rule.threshold_max) {
          return true;
        }
        break;

      case 'rate_of_change':
        // Implement rate of change logic
        break;

      case 'offline':
        if (sensorData.status === 'offline') {
          return true;
        }
        break;
    }

    return false;
  }
}

// Singleton instance
let alertService = null;

/**
 * Get alert service instance
 */
function getAlertService() {
  if (!alertService) {
    alertService = new AlertService();
    alertService.initializeEmail();
  }
  return alertService;
}

module.exports = {
  AlertService,
  getAlertService
};
