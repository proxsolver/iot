/**
 * Validation Middleware
 * Request validation using Joi schemas
 */

const Joi = require('joi');
const logger = require('../config/logger');

/**
 * Validation middleware factory
 */
function validate(schema, property = 'body') {
  return (req, res, next) => {
    const { error, value } = schema.validate(req[property], {
      abortEarly: false,
      stripUnknown: true
    });

    if (error) {
      const errors = error.details.map(detail => ({
        field: detail.path.join('.'),
        message: detail.message
      }));

      logger.warn('Validation failed:', { errors });

      return res.status(400).json({
        error: 'Validation failed',
        details: errors
      });
    }

    // Replace request property with sanitized value
    req[property] = value;
    next();
  };
}

/**
 * Sensor data validation schema
 */
const sensorDataSchema = Joi.object({
  device_id: Joi.string().required().min(1).max(100),
  temperature: Joi.number().required().min(-50).max(150),
  humidity: Joi.number().required().min(0).max(100),
  pressure: Joi.number().required().min(800).max(1200),
  gas_resistance: Joi.number().optional().min(0).max(1000000),
  timestamp: Joi.number().optional().integer().min(0),
  battery_level: Joi.number().optional().min(0).max(100),
  signal_strength: Joi.number().optional().min(-120).max(0),
  uptime: Joi.number().optional().integer().min(0)
});

/**
 * Detection data validation schema
 */
const detectionSchema = Joi.object({
  camera_id: Joi.number().integer().required().min(1).max(10),
  class_id: Joi.number().integer().required().min(0).max(1000),
  class_name: Joi.string().required().min(1).max(100),
  confidence: Joi.number().required().min(0).max(1),
  bounding_box: Joi.object({
    x: Joi.number().min(0).required(),
    y: Joi.number().min(0).required(),
    width: Joi.number().min(0).required(),
    height: Joi.number().min(0).required()
  }).optional(),
  image_path: Joi.string().optional().max(500),
  timestamp: Joi.number().optional().integer().min(0)
});

/**
 * Alert creation validation schema
 */
const alertSchema = Joi.object({
  alert_type: Joi.string().required().valid('detection', 'system', 'low_battery', 'connection', 'threshold'),
  severity: Joi.string().required().valid('low', 'medium', 'high', 'critical'),
  message: Joi.string().required().min(1).max(1000),
  data: Joi.object().optional()
});

/**
 * System status validation schema
 */
const systemStatusSchema = Joi.object({
  device_id: Joi.string().required().min(1).max(100),
  status: Joi.string().required().valid('online', 'offline', 'error', 'maintenance'),
  battery_level: Joi.number().optional().min(0).max(100),
  signal_strength: Joi.number().optional().min(-120).max(0),
  uptime: Joi.number().optional().integer().min(0),
  memory_usage: Joi.number().optional().integer().min(0),
  cpu_usage: Joi.number().optional().min(0).max(100)
});

/**
 * Control command validation schema
 */
const controlCommandSchema = Joi.object({
  command_type: Joi.string().required().valid(
    'system_enable',
    'system_disable',
    'alarm_reset',
    'update_config',
    'restart_device',
    'calibrate_sensor',
    'start_recording',
    'stop_recording'
  ),
  target_device: Joi.string().optional().max(100),
  parameters: Joi.object().optional()
});

/**
 * User creation validation schema
 */
const userCreateSchema = Joi.object({
  username: Joi.string().required().alphanum().min(3).max(30),
  email: Joi.string().required().email(),
  password: Joi.string().required().min(8).pattern(
    /^(?=.*[a-z])(?=.*[A-Z])(?=.*\d)(?=.*[@$!%*?&])[A-Za-z\d@$!%*?&]/
  ),
  role: Joi.string().optional().valid('admin', 'user', 'viewer')
});

/**
 * User update validation schema
 */
const userUpdateSchema = Joi.object({
  username: Joi.string().optional().alphanum().min(3).max(30),
  email: Joi.string().optional().email(),
  password: Joi.string().optional().min(8).pattern(
    /^(?=.*[a-z])(?=.*[A-Z])(?=.*\d)(?=.*[@$!%*?&])[A-Za-z\d@$!%*?&]/
  ),
  role: Joi.string().optional().valid('admin', 'user', 'viewer'),
  is_active: Joi.boolean().optional()
});

/**
 * Login validation schema
 */
const loginSchema = Joi.object({
  username: Joi.string().required(),
  password: Joi.string().required()
});

/**
 * Alert rule creation schema
 */
const alertRuleSchema = Joi.object({
  name: Joi.string().required().min(1).max(100),
  condition_type: Joi.string().required().valid(
    'threshold',
    'rate_of_change',
    'offline',
    'detection_count'
  ),
  sensor_type: Joi.string().when('condition_type', {
    is: Joi.valid('threshold', 'rate_of_change'),
    then: Joi.required(),
    otherwise: Joi.optional()
  }),
  threshold_min: Joi.number().optional(),
  threshold_max: Joi.number().optional(),
  severity: Joi.string().optional().valid('low', 'medium', 'high', 'critical'),
  enabled: Joi.boolean().optional(),
  notification_channels: Joi.array().items(
    Joi.string().valid('email', 'webhook', 'websocket')
  ).optional(),
  cooldown_period: Joi.number().optional().integer().min(0).max(86400)
});

/**
 * Query parameter validation schemas
 */
const queryParamsSchema = Joi.object({
  limit: Joi.number().integer().min(1).max(1000).optional(),
  offset: Joi.number().integer().min(0).optional(),
  start_time: Joi.number().integer().min(0).optional(),
  end_time: Joi.number().integer().min(0).optional(),
  device_id: Joi.string().optional(),
  camera_id: Joi.number().integer().optional(),
  class_name: Joi.string().optional(),
  severity: Joi.string().optional().valid('low', 'medium', 'high', 'critical'),
  acknowledged: Joi.boolean().optional(),
  hours: Joi.number().integer().min(1).max(8760).optional()
});

/**
 * Export request validation schema
 */
const exportRequestSchema = Joi.object({
  export_type: Joi.string().required().valid('csv', 'json', 'xml'),
  table: Joi.string().required().valid(
    'sensor_readings',
    'detections',
    'alerts',
    'system_status'
  ),
  start_time: Joi.number().integer().min(0).required(),
  end_time: Joi.number().integer().min(0).required(),
  filters: Joi.object().optional()
});

module.exports = {
  validate,
  sensorDataSchema,
  detectionSchema,
  alertSchema,
  systemStatusSchema,
  controlCommandSchema,
  userCreateSchema,
  userUpdateSchema,
  loginSchema,
  alertRuleSchema,
  queryParamsSchema,
  exportRequestSchema
};
