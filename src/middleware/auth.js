/**
 * Authentication Middleware
 * JWT and API key authentication
 */

const jwt = require('jsonwebtoken');
const logger = require('../config/logger');
const db = require('../config/database');

const JWT_SECRET = process.env.JWT_SECRET || 'your-super-secret-jwt-key-change-this';
const JWT_EXPIRES_IN = process.env.JWT_EXPIRES_IN || '24h';
const API_KEY_HEADER = process.env.API_KEY_HEADER || 'x-api-key';

/**
 * Verify JWT token
 */
function verifyJWT(token) {
  try {
    return jwt.verify(token, JWT_SECRET);
  } catch (error) {
    return null;
  }
}

/**
 * Generate JWT token
 */
function generateToken(user) {
  return jwt.sign(
    {
      id: user.id,
      username: user.username,
      email: user.email,
      role: user.role
    },
    JWT_SECRET,
    { expiresIn: JWT_EXPIRES_IN }
  );
}

/**
 * JWT authentication middleware
 */
function authenticateJWT(req, res, next) {
  const authHeader = req.headers.authorization;

  if (!authHeader || !authHeader.startsWith('Bearer ')) {
    return res.status(401).json({ error: 'No token provided' });
  }

  const token = authHeader.substring(7);
  const decoded = verifyJWT(token);

  if (!decoded) {
    return res.status(401).json({ error: 'Invalid or expired token' });
  }

  req.user = decoded;
  next();
}

/**
 * API key authentication middleware
 */
function authenticateAPIKey(req, res, next) {
  const apiKey = req.headers[API_KEY_HEADER];

  if (!apiKey) {
    return res.status(401).json({ error: 'No API key provided' });
  }

  try {
    const user = db.queryOne(
      'SELECT id, username, email, role FROM users WHERE api_key = ? AND is_active = 1',
      [apiKey]
    );

    if (!user) {
      return res.status(401).json({ error: 'Invalid API key' });
    }

    req.user = user;
    next();
  } catch (error) {
    logger.error('Error authenticating API key:', error);
    return res.status(500).json({ error: 'Authentication error' });
  }
}

/**
 * Flexible authentication - accepts either JWT or API key
 */
function authenticate(req, res, next) {
  const authHeader = req.headers.authorization;
  const apiKey = req.headers[API_KEY_HEADER];

  // Try JWT first
  if (authHeader && authHeader.startsWith('Bearer ')) {
    const token = authHeader.substring(7);
    const decoded = verifyJWT(token);

    if (decoded) {
      req.user = decoded;
      return next();
    }
  }

  // Try API key
  if (apiKey) {
    try {
      const user = db.queryOne(
        'SELECT id, username, email, role FROM users WHERE api_key = ? AND is_active = 1',
        [apiKey]
      );

      if (user) {
        req.user = user;
        return next();
      }
    } catch (error) {
      logger.error('Error authenticating API key:', error);
    }
  }

  return res.status(401).json({ error: 'Authentication required' });
}

/**
 * Role-based authorization middleware factory
 */
function authorize(...roles) {
  return (req, res, next) => {
    if (!req.user) {
      return res.status(401).json({ error: 'Authentication required' });
    }

    if (!roles.includes(req.user.role)) {
      return res.status(403).json({ error: 'Insufficient permissions' });
    }

    next();
  };
}

/**
 * Optional authentication - attaches user if valid, but doesn't require it
 */
function optionalAuthenticate(req, res, next) {
  const authHeader = req.headers.authorization;
  const apiKey = req.headers[API_KEY_HEADER];

  // Try JWT
  if (authHeader && authHeader.startsWith('Bearer ')) {
    const token = authHeader.substring(7);
    const decoded = verifyJWT(token);
    if (decoded) {
      req.user = decoded;
    }
  }

  // Try API key
  if (!req.user && apiKey) {
    try {
      const user = db.queryOne(
        'SELECT id, username, email, role FROM users WHERE api_key = ? AND is_active = 1',
        [apiKey]
      );
      if (user) {
        req.user = user;
      }
    } catch (error) {
      // Ignore errors for optional auth
    }
  }

  next();
}

module.exports = {
  verifyJWT,
  generateToken,
  authenticateJWT,
  authenticateAPIKey,
  authenticate,
  authorize,
  optionalAuthenticate
};
