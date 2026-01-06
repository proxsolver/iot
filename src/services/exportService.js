/**
 * Export Service
 * Data export to CSV, JSON, XML formats
 */

const fs = require('fs');
const path = require('path');
const logger = require('../config/logger');
const db = require('../config/database');

class ExportService {
  constructor() {
    this.exportDir = process.env.EXPORT_DIR || path.join(__dirname, '../../exports');
    this.maxFileSize = this.parseSize(process.env.EXPORT_MAX_FILE_SIZE || '100MB');

    // Ensure export directory exists
    if (!fs.existsSync(this.exportDir)) {
      fs.mkdirSync(this.exportDir, { recursive: true });
    }
  }

  /**
   * Parse size string to bytes
   */
  parseSize(size) {
    const units = { B: 1, KB: 1024, MB: 1024 * 1024, GB: 1024 * 1024 * 1024 };
    const match = size.toString().match(/^(\d+(?:\.\d+)?)\s*([A-Z]{1,2})$/i);
    if (!match) return 100 * 1024 * 1024; // Default 100MB
    return parseFloat(match[1]) * (units[match[2].toUpperCase()] || 1);
  }

  /**
   * Get data from database
   */
  async getData(table, startTime, endTime, filters = {}) {
    try {
      let query = `SELECT * FROM ${table} WHERE timestamp >= ? AND timestamp <= ?`;
      const params = [startTime, endTime];

      // Apply filters
      if (filters.device_id) {
        query += ' AND device_id = ?';
        params.push(filters.device_id);
      }
      if (filters.camera_id) {
        query += ' AND camera_id = ?';
        params.push(filters.camera_id);
      }
      if (filters.class_name) {
        query += ' AND class_name = ?';
        params.push(filters.class_name);
      }
      if (filters.severity) {
        query += ' AND severity = ?';
        params.push(filters.severity);
      }

      query += ' ORDER BY timestamp ASC';

      // Check estimated size
      const countResult = db.queryOne(
        `SELECT COUNT(*) as count FROM ${table} WHERE timestamp >= ? AND timestamp <= ?`,
        [startTime, endTime]
      );

      const estimatedSize = countResult.count * 1024; // Rough estimate
      if (estimatedSize > this.maxFileSize) {
        throw new Error(`Export size exceeds maximum allowed size of ${this.maxFileSize} bytes`);
      }

      return db.queryAll(query, params);
    } catch (error) {
      logger.error('Error getting data for export:', error);
      throw error;
    }
  }

  /**
   * Export to CSV
   */
  async exportToCSV(data, filename) {
    const filepath = path.join(this.exportDir, filename);

    try {
      if (data.length === 0) {
        throw new Error('No data to export');
      }

      // Get headers
      const headers = Object.keys(data[0]);

      // Create CSV content
      const csvRows = [];
      csvRows.push(headers.join(','));

      for (const row of data) {
        const values = headers.map(header => {
          const value = row[header];
          // Escape quotes and wrap in quotes if contains comma
          if (value === null || value === undefined) {
            return '';
          }
          const stringValue = String(value);
          if (stringValue.includes(',') || stringValue.includes('"') || stringValue.includes('\n')) {
            return `"${stringValue.replace(/"/g, '""')}"`;
          }
          return stringValue;
        });
        csvRows.push(values.join(','));
      }

      const csvContent = csvRows.join('\n');
      fs.writeFileSync(filepath, csvContent, 'utf8');

      logger.info(`Exported ${data.length} records to CSV: ${filename}`);
      return filepath;
    } catch (error) {
      logger.error('Error exporting to CSV:', error);
      throw error;
    }
  }

  /**
   * Export to JSON
   */
  async exportToJSON(data, filename) {
    const filepath = path.join(this.exportDir, filename);

    try {
      if (data.length === 0) {
        throw new Error('No data to export');
      }

      const jsonContent = JSON.stringify(data, null, 2);
      fs.writeFileSync(filepath, jsonContent, 'utf8');

      logger.info(`Exported ${data.length} records to JSON: ${filename}`);
      return filepath;
    } catch (error) {
      logger.error('Error exporting to JSON:', error);
      throw error;
    }
  }

  /**
   * Export to XML
   */
  async exportToXML(data, filename, rootElement = 'records') {
    const filepath = path.join(this.exportDir, filename);

    try {
      if (data.length === 0) {
        throw new Error('No data to export');
      }

      let xmlContent = '<?xml version="1.0" encoding="UTF-8"?>\n';
      xmlContent += `<${rootElement}>\n`;

      for (const row of data) {
        xmlContent += '  <record>\n';
        for (const [key, value] of Object.entries(row)) {
          if (value !== null && value !== undefined) {
            const escapedValue = String(value)
              .replace(/&/g, '&amp;')
              .replace(/</g, '&lt;')
              .replace(/>/g, '&gt;')
              .replace(/"/g, '&quot;')
              .replace(/'/g, '&apos;');
            xmlContent += `    <${key}>${escapedValue}</${key}>\n`;
          }
        }
        xmlContent += '  </record>\n';
      }

      xmlContent += `</${rootElement}>`;
      fs.writeFileSync(filepath, xmlContent, 'utf8');

      logger.info(`Exported ${data.length} records to XML: ${filename}`);
      return filepath;
    } catch (error) {
      logger.error('Error exporting to XML:', error);
      throw error;
    }
  }

  /**
   * Create export and log to database
   */
  async createExport(exportType, table, startTime, endTime, filters = {}) {
    const startTimeMs = Date.now();
    const timestamp = new Date().toISOString().replace(/[:.]/g, '-');
    const filename = `${table}_${timestamp}.${exportType}`;

    try {
      // Get data
      const data = await this.getData(table, startTime, endTime, filters);

      // Export based on type
      let filepath;
      switch (exportType.toLowerCase()) {
        case 'csv':
          filepath = await this.exportToCSV(data, filename);
          break;
        case 'json':
          filepath = await this.exportToJSON(data, filename);
          break;
        case 'xml':
          filepath = await this.exportToXML(data, filename);
          break;
        default:
          throw new Error(`Unsupported export type: ${exportType}`);
      }

      const endTimeMs = Date.now();

      // Log to database
      db.execute(
        `INSERT INTO export_logs (export_type, file_path, record_count, start_time, end_time, status)
         VALUES (?, ?, ?, ?, ?, ?)`,
        [
          exportType,
          filepath,
          data.length,
          Math.floor(startTimeMs / 1000),
          Math.floor(endTimeMs / 1000),
          'completed'
        ]
      );

      return {
        success: true,
        filepath,
        filename,
        recordCount: data.length,
        exportType,
        size: fs.statSync(filepath).size
      };
    } catch (error) {
      // Log failed export
      db.execute(
        `INSERT INTO export_logs (export_type, file_path, record_count, start_time, end_time, status)
         VALUES (?, ?, ?, ?, ?, ?)`,
        [
          exportType,
          filename,
          0,
          Math.floor(startTimeMs / 1000),
          Math.floor(Date.now() / 1000),
          'failed'
        ]
      );

      logger.error('Export failed:', error);
      throw error;
    }
  }

  /**
   * Get export logs
   */
  getExportLogs(limit = 50) {
    try {
      return db.queryAll(
        `SELECT * FROM export_logs ORDER BY created_at DESC LIMIT ?`,
        [limit]
      );
    } catch (error) {
      logger.error('Error getting export logs:', error);
      throw error;
    }
  }

  /**
   * Delete export file
   */
  deleteExportFile(filepath) {
    try {
      if (fs.existsSync(filepath)) {
        fs.unlinkSync(filepath);
        logger.info(`Deleted export file: ${filepath}`);
        return true;
      }
      return false;
    } catch (error) {
      logger.error('Error deleting export file:', error);
      return false;
    }
  }

  /**
   * Clean up old exports
   */
  cleanupOldExports(maxAge = 7 * 24 * 60 * 60 * 1000) { // 7 days
    try {
      const files = fs.readdirSync(this.exportDir);
      const now = Date.now();

      for (const file of files) {
        const filepath = path.join(this.exportDir, file);
        const stats = fs.statSync(filepath);

        if (now - stats.mtimeMs > maxAge) {
          this.deleteExportFile(filepath);
        }
      }

      logger.info('Cleanup of old exports completed');
    } catch (error) {
      logger.error('Error cleaning up old exports:', error);
    }
  }
}

// Singleton instance
let exportService = null;

/**
 * Get export service instance
 */
function getExportService() {
  if (!exportService) {
    exportService = new ExportService();
  }
  return exportService;
}

module.exports = {
  ExportService,
  getExportService
};
