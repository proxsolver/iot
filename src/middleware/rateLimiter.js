/**
 * Rate Limiting Middleware
 * Request rate limiting using express-rate-limit
 */

const rateLimit = require('express-rate-limit');
const logger = require('../config/logger');

/**
 * General API rate limiter
 */
const generalLimiter = rateLimit({
  windowMs: parseInt(process.env.RATE_LIMIT_WINDOW_MS) || 15 * 60 * 1000, // 15 minutes
  max: parseInt(process.env.RATE_LIMIT_MAX_REQUESTS) || 100,
  message: 'Too many requests from this IP, please try again later.',
  standardHeaders: true,
  legacyHeaders: false,
  handler: (req, res) => {
    logger.warn('Rate limit exceeded:', {
      ip: req.ip,
      path: req.path,
      method: req.method
    });
    res.status(429).json({
      error: 'Too many requests',
      message: 'Rate limit exceeded. Please try again later.',
      retryAfter: Math.ceil(
        (parseInt(process.env.RATE_LIMIT_WINDOW_MS) || 15 * 60 * 1000) / 1000
      )
    });
  }
});

/**
 * Strict rate limiter for sensitive endpoints (login, password reset, etc.)
 */
const strictLimiter = rateLimit({
  windowMs: 15 * 60 * 1000, // 15 minutes
  max: 5,
  message: 'Too many attempts, please try again later.',
  skipSuccessfulRequests: true,
  handler: (req, res) => {
    logger.warn('Strict rate limit exceeded:', {
      ip: req.ip,
      path: req.path,
      method: req.method
    });
    res.status(429).json({
      error: 'Too many attempts',
      message: 'Rate limit exceeded. Please try again later.',
      retryAfter: 900
    });
  }
});

/**
 * Data ingestion rate limiter (higher limit for sensor data)
 */
const dataIngestionLimiter = rateLimit({
  windowMs: 60 * 1000, // 1 minute
  max: 1000, // Allow up to 1000 requests per minute
  message: 'Data ingestion rate limit exceeded.',
  handler: (req, res) => {
    logger.warn('Data ingestion rate limit exceeded:', {
      ip: req.ip,
      path: req.path,
      device_id: req.body.device_id
    });
    res.status(429).json({
      error: 'Rate limit exceeded',
      message: 'Too many data points submitted. Please slow down.'
    });
  }
});

/**
 * WebSocket limiter (custom implementation)
 */
class WebSocketRateLimiter {
  constructor(maxConnections = 100, maxMessagesPerMinute = 100) {
    this.connections = new Map();
    this.maxConnections = maxConnections;
    this.maxMessagesPerMinute = maxMessagesPerMinute;
  }

  /**
   * Check if client can connect
   */
  canConnect(ip) {
    const connectionCount = this.connections.get(ip) || 0;
    return connectionCount < this.maxConnections;
  }

  /**
   * Register connection
   */
  addConnection(ip, ws) {
    const connections = this.connections.get(ip) || [];
    connections.push(ws);
    this.connections.set(ip, connections);

    // Remove connection on close
    ws.on('close', () => {
      this.removeConnection(ip, ws);
    });
  }

  /**
   * Remove connection
   */
  removeConnection(ip, ws) {
    const connections = this.connections.get(ip) || [];
    const index = connections.indexOf(ws);
    if (index > -1) {
      connections.splice(index, 1);
    }
    if (connections.length === 0) {
      this.connections.delete(ip);
    } else {
      this.connections.set(ip, connections);
    }
  }

  /**
   * Check if client can send message
   */
  canSendMessage(ip) {
    const connections = this.connections.get(ip) || [];
    return connections.length > 0 && connections.length < this.maxConnections;
  }
}

module.exports = {
  generalLimiter,
  strictLimiter,
  dataIngestionLimiter,
  WebSocketRateLimiter
};
