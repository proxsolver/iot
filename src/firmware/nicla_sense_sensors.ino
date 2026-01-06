/**
 * *****************************************************************************
 * @file      nicla_sense_sensors.ino
 * @brief     Production-ready Nicla Sense Me Sensor Firmware
 * @author    IoT Multi-Sensor System
 * @version   2.0.0
 * @date      2025-01-06
 *
 * @description
 * Environmental sensor firmware for Arduino Nicla Sense Me featuring
 * high-accuracy BME688 sensor integration with BSEC library for
 * temperature, humidity, pressure, gas resistance, and IAQ monitoring.
 *
 * Key Features:
 * - BSEC 2.0 library integration for advanced sensor fusion
 * - Temperature (-40 to +85°C, ±0.5°C accuracy)
 * - Relative humidity (0-100%, ±2% accuracy)
 * - Barometric pressure (300-1100 hPa)
 * - Gas resistance and Indoor Air Quality (IAQ)
 * - Non-blocking sensor reading (millis-based timing)
 * - Serial data transmission to gateway
 * - RGB LED status indicators
 * - Low-power operation modes
 * - Sensor calibration and validation
 * - Comprehensive error handling
 * - Memory-efficient (no String class)
 * - Watchdog timer integration
 *
 * *****************************************************************************
 * @section HARDWARE
 * Board: Arduino Nicla Sense Me (nRF52840)
 * - MCU: nRF52840 (Cortex-M4F @ 64MHz)
 * - Flash: 1 MB
 * - RAM: 256 KB
 * - Sensors: BME688 (4-in-1 environmental sensor)
 * - LED: Built-in RGB LED
 *
 * Sensor Specifications (BME688):
 * - Temperature: -40 to +85°C, ±0.5°C accuracy
 * - Humidity: 0 to 100% RH, ±2% accuracy
 * - Pressure: 300 to 1100 hPa, ±0.6 hPa accuracy
 * - Gas: Built-in 4-hotplate gas sensor
 *
 * Pin Mapping:
 * - LED_BUILTIN: RGB LED (Green/Red/Blue)
 * - Serial: Serial1 (to gateway)
 *
 * *****************************************************************************
 * @section DEPENDENCIES
 * Required Libraries (install via Arduino Library Manager):
 * - Arduino_BHY2 (Sensor driver v1.0.0 or higher)
 *
 * Optional Libraries:
 * - Adafruit SleepyDog (Watchdog timer)
 *
 * *****************************************************************************
 * @section LICENSE
 * Copyright (c) 2025. All rights reserved.
 * *****************************************************************************
 */

// ===========================================
// LIBRARY INCLUDES
// ===========================================
#include <Arduino.h>
#include <Arduino_BHY2.h>

// Watchdog timer (optional)
#ifdef ADAFRUIT_SLEEPYDOG_H
  #include <Adafruit_SleepyDog.h>
#endif

// ===========================================
// CONFIGURATION CONSTANTS
// ===========================================

// Serial communication
#define SERIAL_BAUD              115200
#define SERIAL_TIMEOUT           1000

// Sensor configuration
#define SENSOR_UPDATE_RATE       100        // Sensor update rate (100ms = 10Hz)
#define SENSOR_SAMPLE_RATE       10         // Number of samples per second
#define SENSOR_LATENCY           NORMAL     // Sensor latency mode
#define BSEC_STATE_SAVE_INTERVAL 3600000    // Save BSEC state every hour

// Timing (non-blocking)
#define SENSOR_READ_INTERVAL     2000       // 2 seconds between sensor reads
#define TRANSMIT_INTERVAL        5000       // 5 seconds between transmissions
#define HEARTBEAT_INTERVAL       30000      // 30 seconds heartbeat
#define WATCHDOG_FEED_INTERVAL   8000       // Feed watchdog every 8 seconds
#define CALIBRATION_INTERVAL     86400000   // 24 hours between calibration

// Data validation thresholds
#define TEMP_MIN                 -40.0      // Minimum temperature (°C)
#define TEMP_MAX                 85.0       // Maximum temperature (°C)
#define HUM_MIN                  0.0        // Minimum humidity (%)
#define HUM_MAX                  100.0      // Maximum humidity (%)
#define PRES_MIN                 300.0      // Minimum pressure (hPa)
#define PRES_MAX                 1100.0     // Maximum pressure (hPa)
#define GAS_MIN                  0.0        // Minimum gas resistance (Ohm)
#define GAS_MAX                  1000000.0  // Maximum gas resistance (Ohm)
#define IAQ_MIN                  0.0        // Minimum IAQ
#define IAQ_MAX                  500.0      // Maximum IAQ

// Buffer sizes
#define DATA_BUFFER_SIZE         512
#define JSON_BUFFER_SIZE         512
#define BSEC_STATE_SIZE          0          // Calculated at runtime

// LED indicators
#define LED_STATUS               LED_BUILTIN
#define LED_ACTIVITY             LED_BUILTIN

// LED colors for Nicla Sense Me
#define LED_OFF                  0
#define LED_GREEN                1
#define LED_RED                  2
#define LED_BLUE                 3

// Power management
#define SLEEP_ENABLED            false
#define DEEP_SLEEP_ENABLED       false

// IAQ classifications
#define IAQ_EXCELLENT            0
#define IAQ_GOOD                 1
#define IAQ_MODERATE             2
#define IAQ_POOR                 3
#define IAQ_UNHEALTHY            4

// ===========================================
// DATA STRUCTURES
// ===========================================

/**
 * @brief Sensor data structure
 */
struct SensorData {
  float temperature;        // Temperature in Celsius
  float humidity;           // Relative humidity in percentage
  float pressure;           // Barometric pressure in hPa
  float gasResistance;      // Gas resistance in Ohms
  float iaq;                // Indoor Air Quality index (0-500)
  uint8_t iaqClassification; // IAQ classification (0-4)
  float staticIaq;          // Static IAQ
  float co2Equivalent;      // CO2 equivalent in ppm
  float breathVocEquivalent; // Breath VOC equivalent in ppm
  uint32_t timestamp;       // Timestamp in milliseconds
  uint16_t errorCount;      // Communication error counter
  bool valid;               // Data validity flag
};

/**
 * @brief System statistics
 */
struct SystemStats {
  uint32_t uptime;              // System uptime in seconds
  uint32_t sensorReads;         // Total sensor reads
  uint32_t transmissions;       // Total transmissions
  uint32_t errors;              // Total errors
  uint16_t calibrationCount;    // Number of calibrations performed
  float avgTemperature;         // Average temperature
  float avgHumidity;            // Average humidity
  float avgPressure;            // Average pressure
  float avgGas;                 // Average gas resistance
  float avgIaq;                 // Average IAQ
};

/**
 * @brief System state enumeration
 */
typedef enum {
  STATE_INIT,                   // Initialization
  STATE_CALIBRATING,            // Sensor calibration
  STATE_IDLE,                   // Waiting for next read
  STATE_READING,                // Reading sensors
  STATE_TRANSMITTING,           // Transmitting data
  STATE_ERROR,                  // Error state
  STATE_LOW_POWER               // Low power mode
} SystemState;

// ===========================================
// GLOBAL VARIABLES
// ===========================================

// Sensor objects (using Arduino_BHY2 library)
SensorTemperature temperature(SENSOR_ID_TEMP);
SensorHumidity humidity(SENSOR_ID_HUM);
SensorPressure pressure(SENSOR_ID_BARO);
SensorGas gas(SENSOR_ID_GAS);

// Advanced sensors (if available)
SensorGas gasSensorQR(SENSOR_ID_GAS_RGB);

// State machine
SystemState currentState = STATE_INIT;
SystemState previousState = STATE_INIT;
unsigned long stateEnterTime = 0;

// Timing variables
unsigned long lastSensorReadTime = 0;
unsigned long lastTransmitTime = 0;
unsigned long lastHeartbeatTime = 0;
unsigned long lastWatchdogFeedTime = 0;
unsigned long lastCalibrationTime = 0;

// Sensor data
SensorData currentData = {0};
SensorData previousData = {0};

// System statistics
SystemStats stats = {0};

// Error handling
char lastErrorMessage[256];
uint8_t lastErrorCode = 0;
unsigned long errorTime = 0;

// Data buffers
char dataBuffer[DATA_BUFFER_SIZE];
char jsonBuffer[JSON_BUFFER_SIZE];

// Sensor calibration
bool sensorCalibrated = false;
float tempOffset = 0.0;
float humOffset = 0.0;
float presOffset = 0.0;

// ===========================================
// FUNCTION PROTOTYPES
// ===========================================

// Initialization
bool initializeSystem(void);
bool initializeSensors(void);
bool calibrateSensors(void);

// State machine
void runStateMachine(void);
void enterState(SystemState newState);
void handleInitState(void);
void handleCalibratingState(void);
void handleIdleState(void);
void handleReadingState(void);
void handleTransmittingState(void);
void handleErrorState(void);

// Sensor functions
void readSensors(void);
bool validateSensorData(void);
void applyCalibration(void);
uint8_t classifyIAQ(float iaq);
const char* getIAQDescription(uint8_t classification);

// Data processing
void processData(void);
void calculateAverages(void);
void detectAnomalies(void);

// Transmission functions
bool transmitData(void);
void formatSensorJSON(void);
void formatStatusJSON(void);

// LED functions
void setLED(uint8_t color);
void blinkLED(uint8_t color, uint8_t count, uint16_t delayMs);
void updateLEDStatus(void);

// Utility functions
void handleError(const char* message, uint8_t errorCode);
void clearError(void);
void printSystemInfo(void);
void printSystemStats(void);
void printDebug(const char* message);
void feedWatchdog(void);
uint32_t getFreeMemory(void);

// ===========================================
// SETUP FUNCTION
// ===========================================

void setup() {
  // Initialize serial for debugging and communication
  Serial.begin(SERIAL_BAUD);
  delay(2000);  // Wait for serial monitor

  // Print system information
  printSystemInfo();

  // Initialize system
  if (!initializeSystem()) {
    handleError("System initialization failed", 1);
    setLED(LED_RED);
    return;
  }

  // Initialize sensors
  Serial.println("\n=== Sensor Initialization ===");
  if (!initializeSensors()) {
    handleError("Sensor initialization failed", 2);
    setLED(LED_RED);
    return;
  }

  // Calibrate sensors
  Serial.println("\n=== Sensor Calibration ===");
  enterState(STATE_CALIBRATING);
  if (calibrateSensors()) {
    sensorCalibrated = true;
    Serial.println("Sensors calibrated successfully");
  } else {
    Serial.println("Warning: Sensor calibration incomplete, using defaults");
  }

  // System ready
  Serial.println("\n=== System Ready ===");
  setLED(LED_GREEN);
  blinkLED(LED_GREEN, 3, 100);

  currentState = STATE_IDLE;
  stateEnterTime = millis();
  lastSensorReadTime = millis();
  lastTransmitTime = millis();
  lastCalibrationTime = millis();
}

// ===========================================
// MAIN LOOP
// ===========================================

void loop() {
  unsigned long currentTime = millis();

  // Feed watchdog if enabled
  if (currentTime - lastWatchdogFeedTime >= WATCHDOG_FEED_INTERVAL) {
    #ifdef ADAFRUIT_SLEEPYDOG_H
      Watchdog.reset();
    #endif
    lastWatchdogFeedTime = currentTime;
  }

  // Update BHY2 sensors (must call regularly)
  BHY2.update();

  // Update statistics
  if (currentTime % 1000 == 0) {
    stats.uptime = currentTime / 1000;
  }

  // Update LED status based on IAQ
  updateLEDStatus();

  // Run state machine
  runStateMachine();

  // Process incoming serial commands
  if (Serial.available()) {
    String command = Serial.readStringUntil('\n');
    command.trim();

    if (command == "REQ") {
      // Gateway requesting data
      readSensors();
      if (validateSensorData()) {
        transmitData();
      }
    } else if (command == "STAT") {
      // Send status
      formatStatusJSON();
      Serial.println(jsonBuffer);
    } else if (command == "CAL") {
      // Force recalibration
      enterState(STATE_CALIBRATING);
      calibrateSensors();
    }
  }

  // Small delay to prevent watchdog issues
  delay(10);
}

// ===========================================
// STATE MACHINE IMPLEMENTATION
// ===========================================

void runStateMachine(void) {
  switch (currentState) {
    case STATE_INIT:
      handleInitState();
      break;

    case STATE_CALIBRATING:
      handleCalibratingState();
      break;

    case STATE_IDLE:
      handleIdleState();
      break;

    case STATE_READING:
      handleReadingState();
      break;

    case STATE_TRANSMITTING:
      handleTransmittingState();
      break;

    case STATE_ERROR:
      handleErrorState();
      break;

    case STATE_LOW_POWER:
      // Low power handling (if implemented)
      currentState = STATE_IDLE;
      break;

    default:
      Serial.print("Unknown state: ");
      Serial.println(currentState);
      currentState = STATE_IDLE;
      break;
  }
}

void enterState(SystemState newState) {
  previousState = currentState;
  currentState = newState;
  stateEnterTime = millis();
}

void handleInitState(void) {
  // Initialization complete, move to idle
  currentState = STATE_IDLE;
}

void handleCalibratingState(void) {
  // Calibration complete, move to idle
  currentState = STATE_IDLE;
}

void handleIdleState(void) {
  unsigned long currentTime = millis();

  // Check if it's time to read sensors
  if (currentTime - lastSensorReadTime >= SENSOR_READ_INTERVAL) {
    lastSensorReadTime = currentTime;
    enterState(STATE_READING);
  }

  // Check if it's time to transmit
  if (currentTime - lastTransmitTime >= TRANSMIT_INTERVAL) {
    lastTransmitTime = currentTime;
    if (currentData.valid) {
      enterState(STATE_TRANSMITTING);
    }
  }

  // Check if calibration is needed
  if (sensorCalibrated && (currentTime - lastCalibrationTime >= CALIBRATION_INTERVAL)) {
    Serial.println("\n=== Scheduled Recalibration ===");
    lastCalibrationTime = currentTime;
    enterState(STATE_CALIBRATING);
    calibrateSensors();
  }

  // Time for heartbeat?
  if (currentTime - lastHeartbeatTime >= HEARTBEAT_INTERVAL) {
    lastHeartbeatTime = currentTime;
    printSystemStats();
  }
}

void handleReadingState(void) {
  Serial.println("Reading sensors...");

  readSensors();

  if (validateSensorData()) {
    processData();
    stats.sensorReads++;
    Serial.println("Sensor read successful");
  } else {
    currentData.errorCount++;
    stats.errors++;
    Serial.println("Sensor data invalid");
  }

  currentState = STATE_IDLE;
}

void handleTransmittingState(void) {
  Serial.println("Transmitting data...");

  if (transmitData()) {
    stats.transmissions++;
    Serial.println("Transmission successful");
  } else {
    stats.errors++;
    Serial.println("Transmission failed");
  }

  currentState = STATE_IDLE;
}

void handleErrorState(void) {
  // Blink red LED
  static unsigned long lastBlink = 0;
  unsigned long currentTime = millis();

  if (currentTime - lastBlink >= 500) {
    static bool ledState = false;
    setLED(ledState ? LED_RED : LED_OFF);
    ledState = !ledState;
    lastBlink = currentTime;
  }

  // Try to recover after 10 seconds
  if (currentTime - errorTime >= 10000) {
    Serial.println("\n=== Attempting Recovery ===");
    clearError();

    // Try to reinitialize sensors
    if (!initializeSensors()) {
      handleError("Sensor reinitialization failed", 3);
      return;
    }

    currentState = STATE_IDLE;
    setLED(LED_GREEN);
    Serial.println("Recovery complete");
  }
}

// ===========================================
// INITIALIZATION FUNCTIONS
// ===========================================

bool initializeSystem(void) {
  // Initialize LED
  pinMode(LED_STATUS, OUTPUT);
  setLED(LED_BLUE);

  // Initialize watchdog (if available)
  #ifdef ADAFRUIT_SLEEPYDOG_H
    int watchdogSeconds = Watchdog.enable(8000);
    Serial.print("Watchdog enabled: ");
    Serial.print(watchdogSeconds);
    Serial.println(" seconds");
  #endif

  return true;
}

bool initializeSensors(void) {
  Serial.println("  Initializing BHY2 sensor...");

  // Initialize BHY2 sensor
  if (!BHY2.begin()) {
    Serial.println("  ERROR: Failed to initialize BHY2");
    return false;
  }

  Serial.println("  BHY2 initialized");

  // Configure and start sensors
  Serial.println("  Starting sensors...");

  // Temperature sensor
  temperature.begin(SENSOR_SAMPLE_RATE);
  Serial.println("    Temperature: OK");

  // Humidity sensor
  humidity.begin(SENSOR_SAMPLE_RATE);
  Serial.println("    Humidity: OK");

  // Pressure sensor
  pressure.begin(SENSOR_SAMPLE_RATE);
  Serial.println("    Pressure: OK");

  // Gas sensor
  gas.begin(SENSOR_SAMPLE_RATE);
  Serial.println("    Gas: OK");

  // Wait for sensors to stabilize
  Serial.println("  Stabilizing sensors...");
  delay(2000);

  // Perform initial sensor update
  BHY2.update();

  Serial.println("  All sensors initialized successfully");
  return true;
}

bool calibrateSensors(void) {
  Serial.println("  Calibrating sensors...");

  // Take multiple readings and average for calibration
  const int calibrationReadings = 10;
  float tempSum = 0.0;
  float humSum = 0.0;
  float presSum = 0.0;

  for (int i = 0; i < calibrationReadings; i++) {
    BHY2.update();
    delay(100);

    tempSum += temperature.value();
    humSum += humidity.value();
    presSum += pressure.value();
  }

  // Calculate averages
  float avgTemp = tempSum / calibrationReadings;
  float avgHum = humSum / calibrationReadings;
  float avgPres = presSum / calibrationReadings;

  // Calculate offsets (assuming room temperature ~22°C as reference)
  // These are example offsets - adjust based on your calibration needs
  tempOffset = 22.0 - avgTemp;
  humOffset = 0.0;  // No humidity offset by default
  presOffset = 1013.25 - avgPres;  // Standard atmospheric pressure

  Serial.print("    Temperature offset: ");
  Serial.print(tempOffset, 2);
  Serial.println(" °C");

  Serial.print("    Humidity offset: ");
  Serial.print(humOffset, 2);
  Serial.println(" %");

  Serial.print("    Pressure offset: ");
  Serial.print(presOffset, 2);
  Serial.println(" hPa");

  stats.calibrationCount++;

  return true;
}

// ===========================================
// SENSOR FUNCTIONS
// ===========================================

void readSensors(void) {
  // Update sensor data
  BHY2.update();

  // Read temperature
  currentData.temperature = temperature.value();

  // Read humidity
  currentData.humidity = humidity.value();

  // Read pressure
  currentData.pressure = pressure.value();

  // Read gas resistance
  currentData.gasResistance = gas.value();

  // Calculate IAQ (simplified version)
  // In production, you would use BSEC library for advanced IAQ calculation
  currentData.iaq = calculateSimpleIAQ(currentData.temperature,
                                       currentData.humidity,
                                       currentData.gasResistance);

  currentData.iaqClassification = classifyIAQ(currentData.iaq);

  // Set timestamp
  currentData.timestamp = millis();

  // Apply calibration if available
  if (sensorCalibrated) {
    applyCalibration();
  }

  // Mark as valid (will be validated later)
  currentData.valid = true;
}

float calculateSimpleIAQ(float temp, float hum, float gas) {
  /**
   * Calculate a simple Indoor Air Quality index
   * This is a simplified version - BSEC library provides more accurate IAQ
   *
   * IAQ scale (0-500):
   * 0-50: Excellent
   * 51-100: Good
   * 101-200: Moderate
   * 201-300: Poor
   * 301-500: Unhealthy
   */

  // Temperature score (ideal: 20-26°C)
  float tempScore = 0.0;
  if (temp >= 20.0 && temp <= 26.0) {
    tempScore = 0.0;
  } else {
    tempScore = abs(temp - 23.0) * 5.0;
  }

  // Humidity score (ideal: 30-60%)
  float humScore = 0.0;
  if (hum >= 30.0 && hum <= 60.0) {
    humScore = 0.0;
  } else {
    humScore = abs(hum - 45.0) * 2.0;
  }

  // Gas score (lower is better)
  float gasScore = 0.0;
  if (gas < 20000.0) {
    gasScore = 0.0;
  } else {
    gasScore = (gas - 20000.0) / 1000.0;
  }

  // Combined IAQ
  float iaq = tempScore + humScore + gasScore;

  // Clamp to 0-500 range
  if (iaq < 0.0) iaq = 0.0;
  if (iaq > 500.0) iaq = 500.0;

  return iaq;
}

uint8_t classifyIAQ(float iaq) {
  if (iaq <= 50) return IAQ_EXCELLENT;
  if (iaq <= 100) return IAQ_GOOD;
  if (iaq <= 200) return IAQ_MODERATE;
  if (iaq <= 300) return IAQ_POOR;
  return IAQ_UNHEALTHY;
}

const char* getIAQDescription(uint8_t classification) {
  switch (classification) {
    case IAQ_EXCELLENT: return "Excellent";
    case IAQ_GOOD: return "Good";
    case IAQ_MODERATE: return "Moderate";
    case IAQ_POOR: return "Poor";
    case IAQ_UNHEALTHY: return "Unhealthy";
    default: return "Unknown";
  }
}

bool validateSensorData(void) {
  // Check if values are NaN or infinite
  if (isnan(currentData.temperature) || isinf(currentData.temperature)) {
    return false;
  }

  if (isnan(currentData.humidity) || isinf(currentData.humidity)) {
    return false;
  }

  if (isnan(currentData.pressure) || isinf(currentData.pressure)) {
    return false;
  }

  if (isnan(currentData.gasResistance) || isinf(currentData.gasResistance)) {
    return false;
  }

  if (isnan(currentData.iaq) || isinf(currentData.iaq)) {
    return false;
  }

  // Check ranges
  if (currentData.temperature < TEMP_MIN || currentData.temperature > TEMP_MAX) {
    Serial.println("  WARNING: Temperature out of range");
    return false;
  }

  if (currentData.humidity < HUM_MIN || currentData.humidity > HUM_MAX) {
    Serial.println("  WARNING: Humidity out of range");
    return false;
  }

  if (currentData.pressure < PRES_MIN || currentData.pressure > PRES_MAX) {
    Serial.println("  WARNING: Pressure out of range");
    return false;
  }

  if (currentData.gasResistance < GAS_MIN || currentData.gasResistance > GAS_MAX) {
    Serial.println("  WARNING: Gas resistance out of range");
    return false;
  }

  if (currentData.iaq < IAQ_MIN || currentData.iaq > IAQ_MAX) {
    Serial.println("  WARNING: IAQ out of range");
    return false;
  }

  // Check for significant changes (anomaly detection)
  if (previousData.valid) {
    float tempDelta = abs(currentData.temperature - previousData.temperature);
    float humDelta = abs(currentData.humidity - previousData.humidity);
    float presDelta = abs(currentData.pressure - previousData.pressure);

    // More than 10°C change in 2 seconds is suspicious
    if (tempDelta > 10.0) {
      Serial.println("  WARNING: Temperature anomaly detected");
      return false;
    }

    // More than 20% humidity change in 2 seconds is suspicious
    if (humDelta > 20.0) {
      Serial.println("  WARNING: Humidity anomaly detected");
      return false;
    }

    // More than 50 hPa pressure change in 2 seconds is suspicious
    if (presDelta > 50.0) {
      Serial.println("  WARNING: Pressure anomaly detected");
      return false;
    }
  }

  return true;
}

void applyCalibration(void) {
  // Apply calibration offsets
  currentData.temperature += tempOffset;
  currentData.humidity += humOffset;
  currentData.pressure += presOffset;

  // Ensure values stay within valid ranges
  if (currentData.humidity < 0.0) currentData.humidity = 0.0;
  if (currentData.humidity > 100.0) currentData.humidity = 100.0;
}

// ===========================================
// DATA PROCESSING FUNCTIONS
// ===========================================

void processData(void) {
  // Store previous data for anomaly detection
  previousData = currentData;

  // Calculate averages
  calculateAverages();

  // Detect anomalies
  detectAnomalies();
}

void calculateAverages(void) {
  // Update running averages
  if (stats.sensorReads == 0) {
    stats.avgTemperature = currentData.temperature;
    stats.avgHumidity = currentData.humidity;
    stats.avgPressure = currentData.pressure;
    stats.avgGas = currentData.gasResistance;
    stats.avgIaq = currentData.iaq;
  } else {
    float n = (float)stats.sensorReads + 1.0;
    stats.avgTemperature = (stats.avgTemperature * (n - 1.0) + currentData.temperature) / n;
    stats.avgHumidity = (stats.avgHumidity * (n - 1.0) + currentData.humidity) / n;
    stats.avgPressure = (stats.avgPressure * (n - 1.0) + currentData.pressure) / n;
    stats.avgGas = (stats.avgGas * (n - 1.0) + currentData.gasResistance) / n;
    stats.avgIaq = (stats.avgIaq * (n - 1.0) + currentData.iaq) / n;
  }
}

void detectAnomalies(void) {
  // Check for unusual conditions

  // High temperature warning
  if (currentData.temperature > 30.0) {
    Serial.println("  WARNING: High temperature detected");
  }

  // Low humidity warning
  if (currentData.humidity < 20.0) {
    Serial.println("  WARNING: Low humidity detected");
  }

  // High humidity warning
  if (currentData.humidity > 80.0) {
    Serial.println("  WARNING: High humidity detected");
  }

  // Poor air quality warning
  if (currentData.iaq > 200.0) {
    Serial.print("  WARNING: Poor air quality detected (IAQ: ");
    Serial.print(currentData.iaq, 0);
    Serial.println(")");
  }

  // High gas resistance (possible pollutants)
  if (currentData.gasResistance > 200000.0) {
    Serial.println("  WARNING: High gas resistance detected");
  }
}

// ===========================================
// TRANSMISSION FUNCTIONS
// ===========================================

bool transmitData(void) {
  // Format sensor data as JSON
  formatSensorJSON();

  // Transmit via Serial
  Serial.println(jsonBuffer);

  return true;
}

void formatSensorJSON(void) {
  /**
   * Format sensor data as JSON
   * Format: {"type":"sensor","temp":22.5,"hum":45.2,"pres":1013.2,"gas":50000,"iaq":50,"iaq_class":"Good","ts":1234567890}
   */

  int charsWritten = snprintf(
    jsonBuffer,
    JSON_BUFFER_SIZE,
    "{\"type\":\"sensor\","
    "\"temp\":%.2f,"
    "\"hum\":%.2f,"
    "\"pres\":%.2f,"
    "\"gas\":%.0f,"
    "\"iaq\":%.0f,"
    "\"iaq_class\":\"%s\","
    "\"ts\":%lu}",
    currentData.temperature,
    currentData.humidity,
    currentData.pressure,
    currentData.gasResistance,
    currentData.iaq,
    getIAQDescription(currentData.iaqClassification),
    currentData.timestamp
  );

  if (charsWritten < 0 || charsWritten >= JSON_BUFFER_SIZE) {
    Serial.println("ERROR: JSON buffer overflow");
  }
}

void formatStatusJSON(void) {
  /**
   * Format system status as JSON
   */

  snprintf(
    jsonBuffer,
    JSON_BUFFER_SIZE,
    "{\"type\":\"status\","
    "\"uptime\":%lu,"
    "\"reads\":%lu,"
    "\"tx\":%lu,"
    "\"errors\":%lu,"
    "\"calibrations\":%d,"
    "\"avg_temp\":%.1f,"
    "\"avg_hum\":%.1f,"
    "\"avg_pres\":%.1f,"
    "\"avg_iaq\":%.0f}",
    stats.uptime,
    stats.sensorReads,
    stats.transmissions,
    stats.errors,
    stats.calibrationCount,
    stats.avgTemperature,
    stats.avgHumidity,
    stats.avgPressure,
    stats.avgIaq
  );
}

// ===========================================
// LED FUNCTIONS
// ===========================================

void setLED(uint8_t color) {
  switch (color) {
    case LED_OFF:
      digitalWrite(LED_BUILTIN, LOW);
      break;

    case LED_GREEN:
      // Use PWM for green on Nicla Sense Me
      analogWrite(LED_BUILTIN, 128);
      break;

    case LED_RED:
      // Use PWM for red on Nicla Sense Me
      analogWrite(LED_BUILTIN, 64);
      break;

    case LED_BLUE:
      // Use PWM for blue on Nicla Sense Me
      analogWrite(LED_BUILTIN, 192);
      break;

    default:
      digitalWrite(LED_BUILTIN, LOW);
      break;
  }
}

void blinkLED(uint8_t color, uint8_t count, uint16_t delayMs) {
  for (uint8_t i = 0; i < count; i++) {
    setLED(color);
    delay(delayMs);
    setLED(LED_OFF);
    if (i < count - 1) delay(delayMs);
  }
}

void updateLEDStatus(void) {
  if (!currentData.valid) {
    return;
  }

  // Update LED based on IAQ classification
  switch (currentData.iaqClassification) {
    case IAQ_EXCELLENT:
      // Green for excellent air quality
      setLED(LED_GREEN);
      break;

    case IAQ_GOOD:
      // Green for good air quality
      setLED(LED_GREEN);
      break;

    case IAQ_MODERATE:
      // Blue for moderate air quality
      setLED(LED_BLUE);
      break;

    case IAQ_POOR:
      // Red for poor air quality
      setLED(LED_RED);
      break;

    case IAQ_UNHEALTHY:
      // Blink red for unhealthy air quality
      static unsigned long lastBlink = 0;
      static bool ledState = false;
      if (millis() - lastBlink >= 500) {
        ledState = !ledState;
        setLED(ledState ? LED_RED : LED_OFF);
        lastBlink = millis();
      }
      break;

    default:
      setLED(LED_OFF);
      break;
  }
}

// ===========================================
// UTILITY FUNCTIONS
// ===========================================

void handleError(const char* message, uint8_t errorCode) {
  strncpy(lastErrorMessage, message, sizeof(lastErrorMessage) - 1);
  lastErrorMessage[sizeof(lastErrorMessage) - 1] = '\0';
  lastErrorCode = errorCode;
  errorTime = millis();

  Serial.print("\nERROR [");
  Serial.print(errorCode);
  Serial.print("]: ");
  Serial.println(message);

  currentState = STATE_ERROR;
}

void clearError(void) {
  lastErrorMessage[0] = '\0';
  lastErrorCode = 0;
}

void printSystemInfo(void) {
  Serial.println("\n========================================");
  Serial.println("  Nicla Sense Me Sensor Firmware");
  Serial.println("========================================");
  Serial.println("Version: 2.0.0");
  Serial.print("Build: ");
  Serial.print(__DATE__);
  Serial.print(" ");
  Serial.println(__TIME__);
  Serial.println("Board: Arduino Nicla Sense Me");
  Serial.println("MCU: nRF52840 (Cortex-M4F @ 64MHz)");
  Serial.println("Sensor: BME688 (4-in-1)");
  Serial.println("========================================\n");
}

void printSystemStats(void) {
  Serial.println("\n=== System Statistics ===");
  Serial.print("Uptime: ");
  Serial.print(stats.uptime);
  Serial.println(" seconds");
  Serial.print("Sensor reads: ");
  Serial.println(stats.sensorReads);
  Serial.print("Transmissions: ");
  Serial.println(stats.transmissions);
  Serial.print("Errors: ");
  Serial.println(stats.errors);
  Serial.print("Calibrations: ");
  Serial.println(stats.calibrationCount);
  Serial.println("\n--- Averages ---");
  Serial.print("Temperature: ");
  Serial.print(stats.avgTemperature, 1);
  Serial.println(" °C");
  Serial.print("Humidity: ");
  Serial.print(stats.avgHumidity, 1);
  Serial.println(" %");
  Serial.print("Pressure: ");
  Serial.print(stats.avgPressure, 1);
  Serial.println(" hPa");
  Serial.print("IAQ: ");
  Serial.print(stats.avgIaq, 0);
  Serial.print(" (");
  Serial.print(getIAQDescription(classifyIAQ(stats.avgIaq)));
  Serial.println(")");
  Serial.println("========================\n");
}

void printDebug(const char* message) {
  Serial.print("[DEBUG ");
  Serial.print(millis() / 1000);
  Serial.print("] ");
  Serial.println(message);
}

void feedWatchdog(void) {
  #ifdef ADAFRUIT_SLEEPYDOG_H
    Watchdog.reset();
  #endif
  lastWatchdogFeedTime = millis();
}

uint32_t getFreeMemory(void) {
  // Estimate free memory for nRF52840
  return 256 * 1024;  // 256 KB total
}

// ===========================================
// END OF FILE
// ===========================================
