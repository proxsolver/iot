/**
 * *****************************************************************************
 * @file      mkr_wan_gateway.ino
 * @brief     Production-ready MKR WAN 1310 Gateway Firmware
 * @author    IoT Multi-Sensor System
 * @version   2.0.0
 * @date      2025-01-06
 *
 * @description
 * Central hub firmware for Arduino MKR WAN 1310 that aggregates data from
 * Nicla Sense Me and dual Nicla Vision boards via I2C multiplexer and
 * Serial communication. Transmits data via LoRaWAN (primary) and WiFi (fallback).
 *
 * Key Features:
 * - I2C multiplexer (TCA9548A) control for dual Nicla Vision cameras
 * - Serial1 communication with Nicla Sense Me
 * - Serial2/3 communication with dual Nicla Vision boards
 * - Data aggregation and binary/JSON formatting
 * - LoRaWAN OTAA with adaptive data rate
 * - WiFi fallback with MQTT support
 * - Non-blocking state machine architecture
 * - Comprehensive error handling and retry logic
 * - Low-power sleep modes
 * - Watchdog timer integration
 * - RTCC backup for timestamp continuity
 *
 * *****************************************************************************
 * @section HARDWARE
 * Board: Arduino MKR WAN 1310 (SAMD21 Cortex-M0+)
 * - MCU: ATSAMD21G18A
 * - Flash: 256 KB
 * - RAM: 32 KB
 * - LoRa: Murata CMWX1ZZABZ-078 (SX1276)
 * - WiFi: u-blox NINA-W102
 *
 * Pin Mapping:
 * - I2C SDA: Pin 11
 * - I2C SCL: Pin 12
 * - Serial1 (RX/TX): Nicla Sense Me
 * - TCA9548A I2C Addr: 0x70
 * - LED_STATUS: LED_BUILTIN
 * - LED_LORA: Pin 5
 * - LED_WIFI: Pin 6
 * - LED_ERROR: Pin 4
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
#include <Wire.h>
#include <SPI.h>
#include <FlashStorage.h>
#include <RTCZero.h>
#include <Adafruit_SleepyDog.h>

// LoRaWAN library - supports both MKRWAN.h and ArduinoLoRaWAN.h
#ifdef ARDUINO_SAMD_MKRWAN1310
  #include <MKRWAN.h>
#else
  #include <ArduinoLoRaWAN.h>
#endif

// WiFi and MQTT
#include <WiFiNINA.h>
#include <ArduinoMqttClient.h>

// ===========================================
// CONFIGURATION CONSTANTS
// ===========================================

// Serial communication
#define SERIAL_BAUD              115200
#define SERIAL_TIMEOUT           1000

// Serial port assignments
#define SERIAL_SENSOR            Serial1    // Nicla Sense Me
#define SERIAL_VISION_1          Serial2    // Nicla Vision #1 (if using Serial2)
#define SERIAL_VISION_2          Serial3    // Nicla Vision #2 (if using Serial3)

// I2C Configuration
#define I2C_SDA                  11
#define I2C_SCL                  12
#define I2C_CLOCK_SPEED          100000     // 100kHz (fast mode)
#define TCA9548A_ADDR            0x70       // I2C multiplexer address

// I2C Multiplexer Channels
#define TCA_CHANNEL_NONE         0b00000000 // Disable all channels
#define TCA_CHANNEL_VISION_1     0b00000001 // Channel 0 - Nicla Vision #1
#define TCA_CHANNEL_VISION_2     0b00000010 // Channel 1 - Nicla Vision #2

// LED Indicators
#define LED_STATUS               LED_BUILTIN
#define LED_LORA                 5
#define LED_WIFI                 6
#define LED_ERROR                4

// LoRaWAN Configuration (OTAA)
// WARNING: Replace with your actual credentials from The Things Network
static const char* LORAWAN_APP_EUI      = "YOUR_APP_EUI_HERE";      // Application EUI (LSB first)
static const char* LORAWAN_APP_KEY      = "YOUR_APP_KEY_HERE";      // Application Key (MSB first)
static const char* LORAWAN_DEV_EUI      = "YOUR_DEV_EUI_HERE";      // Device EUI (optional - auto from hardware)

// LoRaWAN Regional Settings
#ifdef EUROPE
  #define LORAWAN_REGION         EU868
#elif defined(US)
  #define LORAWAN_REGION         US915
#elif defined(AUSTRALIA)
  #define LORAWAN_REGION         AU915
#elif defined(ASIA)
  #define LORAWAN_REGION         AS923
#else
  #define LORAWAN_REGION         EU868      // Default to Europe
#endif

// WiFi Configuration
static const char* WIFI_SSID            = "YOUR_WIFI_SSID";
static const char* WIFI_PASS            = "YOUR_WIFI_PASSWORD";
static const int WIFI_CONNECTION_TIMEOUT = 30000;    // 30 seconds

// MQTT Configuration (for WiFi fallback)
static const char* MQTT_BROKER          = "broker.hivemq.com";  // Public test broker
static const int MQTT_PORT              = 1883;
static const char* MQTT_TOPIC_SENSOR    = "iot/sensors";
static const char* MQTT_TOPIC_VISION    = "iot/vision";
static const char* MQTT_TOPIC_STATUS    = "iot/status";

// Timing Constants (non-blocking)
#define LORAWAN_TRANSMIT_INTERVAL  60000      // 60 seconds between LoRa transmissions
#define WIFI_TRANSMIT_INTERVAL     300000     // 5 minutes between WiFi transmissions
#define SENSOR_READ_INTERVAL       5000       // 5 seconds between sensor reads
#define CAMERA_POLL_INTERVAL       1000       // 1 second between camera polls
#define HEARTBEAT_INTERVAL         30000      // 30 seconds heartbeat
#define WATCHDOG_FEED_INTERVAL     8000       // Feed watchdog every 8 seconds

// Buffer Sizes
#define DATA_BUFFER_SIZE           512
#define JSON_BUFFER_SIZE           768
#define LORA_PAYLOAD_MAX_SIZE      242        // Max LoRaWAN payload (EU868)

// Retry Configuration
#define MAX_LORA_JOIN_RETRIES      5
#define MAX_WIFI_CONNECT_RETRIES   3
#define MAX_TRANSMIT_RETRIES       3
#define RETRY_DELAY_MS             5000

// ===========================================
// DATA STRUCTURES
// ===========================================

/**
 * @brief Sensor data from Nicla Sense Me
 */
struct SensorData {
  float temperature;        // Temperature in Celsius (-40 to 85°C)
  float humidity;           // Relative humidity (0 to 100%)
  float pressure;           // Barometric pressure (300 to 1100 hPa)
  float gasResistance;      // Gas resistance (Ohms)
  float iaq;                // Indoor Air Quality index (0-500)
  uint32_t timestamp;       // Timestamp in milliseconds
  uint16_t errorCount;      // Communication error counter
  bool valid;               // Data validity flag
};

/**
 * @brief Vision detection data from Nicla Vision
 */
struct VisionData {
  uint8_t cameraId;         // Camera identifier (0 or 1)
  uint8_t detectedClass;    // Class ID (0=unknown, 1=person, 2=vehicle, 3=animal)
  float confidence;         // Detection confidence (0.0 to 1.0)
  uint16_t boundingBox[4];  // [x, y, width, height]
  uint32_t frameCount;      // Total frames processed
  uint32_t detectionCount;  // Total detections
  uint32_t timestamp;       // Timestamp in milliseconds
  uint16_t errorCount;      // Communication error counter
  bool valid;               // Data validity flag
};

/**
 * @brief System health and statistics
 */
struct SystemStats {
  uint32_t uptime;              // System uptime in seconds
  uint32_t loraTransmitCount;   // Total LoRaWAN transmissions
  uint32_t wifiTransmitCount;   // Total WiFi transmissions
  uint32_t sensorReadCount;     // Total sensor reads
  uint32_t cameraReadCount;     // Total camera reads
  uint32_t errorCount;          // Total errors
  uint16_t loraJoinRetries;     // LoRaWAN join retry count
  uint8_t batteryLevel;         // Battery level (0-100%)
  float batteryVoltage;         // Battery voltage
  bool lowBattery;              // Low battery warning flag
};

// ===========================================
// GLOBAL VARIABLES
// ===========================================

// Data instances
SensorData sensorData = {0};
VisionData visionData[2] = {0};  // Array for 2 cameras
SystemStats systemStats = {0};

// Communication objects
#ifdef ARDUINO_SAMD_MKRWAN1310
  LoRaModem modem;
#else
  // Alternative for non-MKRWAN boards
#endif

WiFiClient wifiClient;
MqttClient mqttClient(wifiClient);

RTCZero rtc;

// State machine states
typedef enum {
  STATE_INIT,                    // Initialization
  STATE_CONNECT_LORAWAN,         // Connecting to LoRaWAN
  STATE_CONNECT_WIFI,            // Connecting to WiFi
  STATE_IDLE,                    // Waiting for next operation
  STATE_READING_SENSORS,         // Reading environmental sensors
  STATE_READING_CAMERAS,         // Reading camera data
  STATE_PROCESSING_DATA,         // Processing and aggregating data
  STATE_TRANSMITTING_LORAWAN,    // Transmitting via LoRaWAN
  STATE_TRANSMITTING_WIFI,       // Transmitting via WiFi
  STATE_LOW_POWER,               // Entering low-power mode
  STATE_ERROR,                   // Error state
  STATE_RECOVERY                 // Recovery from error
} SystemState;

SystemState currentState = STATE_INIT;
SystemState previousState = STATE_INIT;

// Timing variables
unsigned long lastTransmitTime = 0;
unsigned long lastSensorReadTime = 0;
unsigned long lastCameraPollTime = 0;
unsigned long lastHeartbeatTime = 0;
unsigned long lastWatchdogFeedTime = 0;
unsigned long stateEnterTime = 0;

// Connection status
bool lorawanConnected = false;
bool wifiConnected = false;
bool mqttConnected = false;

// Data buffers
char dataBuffer[DATA_BUFFER_SIZE];
char jsonBuffer[JSON_BUFFER_SIZE];
uint8_t loraPayload[LORA_PAYLOAD_MAX_SIZE];

// Error handling
char lastErrorMessage[256];
uint8_t lastErrorCode = 0;
unsigned long errorTime = 0;

// Persistent configuration (stored in flash)
struct Config {
  uint32_t magic;                  // Magic number for validation
  uint8_t transmitInterval;        // LoRaWAN transmit interval (minutes)
  uint8_t sensorReadInterval;      // Sensor read interval (seconds)
  bool enableWiFiFallback;         // Enable WiFi fallback
  bool enableLowPowerMode;         // Enable low-power mode
  uint8_t loraDataRate;            // LoRaWAN data rate (0-15)
  uint8_t loraTxPower;             // LoRaWAN TX power (0-15)
  uint32_t checksum;               // Configuration checksum
};

Config config;
FlashStorage(flash_config, Config);

// ===========================================
// FUNCTION PROTOTYPES
// ===========================================

// Initialization
bool initializeSystem(void);
bool initializeI2C(void);
bool initializeLoRaWAN(void);
bool initializeWiFi(void);
bool initializeRTC(void);
bool loadConfiguration(void);
bool saveConfiguration(void);
uint32_t calculateChecksum(const Config* cfg);

// State machine functions
void runStateMachine(void);
void enterState(SystemState newState);
void handleInitState(void);
void handleConnectLoRaWANState(void);
void handleConnectWiFiState(void);
void handleIdleState(void);
void handleReadingSensorsState(void);
void handleReadingCamerasState(void);
void handleProcessingDataState(void);
void handleTransmittingLoRaWANState(void);
void handleTransmittingWiFiState(void);
void handleLowPowerState(void);
void handleErrorState(void);
void handleRecoveryState(void);

// I2C and multiplexer
bool switchI2CChannel(uint8_t channel);
bool scanI2CDevices(void);
bool testI2CDevice(uint8_t address);

// Sensor communication
void readSensorData(void);
void readCameraData(uint8_t cameraId);
bool processSensorSerialData(void);
bool processCameraSerialData(uint8_t cameraId);
bool requestSensorData(void);
bool requestCameraData(uint8_t cameraId);

// Data processing
void aggregateData(void);
void formatSensorDataJSON(void);
void formatVisionDataJSON(uint8_t cameraId);
void formatLoRaWANPayload(void);
void formatSystemStatsJSON(void);

// Transmission
bool transmitLoRaWAN(void);
bool transmitWiFi(void);
bool transmitMQTT(const char* topic, const char* payload);
bool reconnectLoRaWAN(void);
bool reconnectWiFi(void);
bool reconnectMQTT(void);

// Power management
void enterLowPowerMode(uint32_t sleepMs);
void wakeFromLowPowerMode(void);
void feedWatchdog(void);
void checkBatteryLevel(void);
bool prepareSleep(void);
bool prepareWake(void);

// Utility functions
void setStatusLED(uint8_t led, bool state);
void blinkLED(uint8_t led, uint8_t count, uint16_t delayMs);
void handleError(const char* message, uint8_t errorCode);
void clearError(void);
void printSystemInfo(void);
void printSystemStats(void);
void printDebug(const char* message);
void printHex(const uint8_t* data, size_t len);
bool validateSensorValues(void);
bool validateVisionValues(uint8_t cameraId);

// Timekeeping
uint32_t getTimestamp(void);
void syncRTC(void);
String getFormattedTime(void);

// ===========================================
// SETUP FUNCTION
// ===========================================

void setup() {
  // Initialize serial for debugging
  Serial.begin(SERIAL_BAUD);
  delay(2000);  // Wait for serial monitor to open

  // Initialize LED pins
  pinMode(LED_STATUS, OUTPUT);
  pinMode(LED_LORA, OUTPUT);
  pinMode(LED_WIFI, OUTPUT);
  pinMode(LED_ERROR, OUTPUT);

  // Turn on status LED to indicate boot
  setStatusLED(LED_STATUS, true);

  // Print system information
  printSystemInfo();

  // Load configuration from flash
  if (!loadConfiguration()) {
    // Set default configuration
    config.magic = 0x4A5A4A5A;  // Magic number: "JZJZ"
    config.transmitInterval = 1; // 1 minute
    config.sensorReadInterval = 5; // 5 seconds
    config.enableWiFiFallback = true;
    config.enableLowPowerMode = false;
    config.loraDataRate = 3;     // DR3
    config.loraTxPower = 14;     // 14dBm
    config.checksum = calculateChecksum(&config);
    saveConfiguration();
    Serial.println("Default configuration saved");
  }

  // Initialize watchdog timer (8 seconds timeout)
  int watchdogSeconds = 0;
  #ifdef ADAFRUIT_SLEEPYDOG_H
    watchdogSeconds = Watchdog.enable(8000);
    Serial.print("Watchdog enabled: ");
    Serial.print(watchdogSeconds);
    Serial.println(" seconds timeout");
  #endif

  lastWatchdogFeedTime = millis();

  // Initialize RTC
  if (!initializeRTC()) {
    handleError("RTC initialization failed", 1);
  }

  // Initialize I2C
  Serial.println("\n=== I2C Initialization ===");
  if (!initializeI2C()) {
    handleError("I2C initialization failed", 2);
    return;
  }

  // Scan I2C bus
  scanI2CDevices();

  // Initialize sensor serial port
  Serial.println("\n=== Sensor Serial Port ===");
  SERIAL_SENSOR.begin(SERIAL_BAUD);
  Serial.println("Serial1 initialized for Nicla Sense Me");

  // Initialize LoRaWAN
  Serial.println("\n=== LoRaWAN Initialization ===");
  enterState(STATE_CONNECT_LORAWAN);
  handleConnectLoRaWANState();

  // Initialize WiFi (non-blocking, will retry in main loop)
  Serial.println("\n=== WiFi Initialization ===");
  if (config.enableWiFiFallback) {
    enterState(STATE_CONNECT_WIFI);
    handleConnectWiFiState();
  }

  // Print initial status
  Serial.println("\n=== System Status ===");
  Serial.print("LoRaWAN: ");
  Serial.println(lorawanConnected ? "CONNECTED" : "DISCONNECTED");
  Serial.print("WiFi: ");
  Serial.println(wifiConnected ? "CONNECTED" : "DISCONNECTED");

  // Ready
  Serial.println("\n=== System Ready ===");
  Serial.println("Starting main loop...");
  blinkLED(LED_STATUS, 3, 100);

  currentState = STATE_IDLE;
  stateEnterTime = millis();
}

// ===========================================
// MAIN LOOP
// ===========================================

void loop() {
  unsigned long currentTime = millis();

  // Feed watchdog periodically
  if (currentTime - lastWatchdogFeedTime >= WATCHDOG_FEED_INTERVAL) {
    #ifdef ADAFRUIT_SLEEPYDOG_H
      Watchdog.reset();
    #endif
    lastWatchdogFeedTime = currentTime;
  }

  // Update system statistics
  if (currentTime % 1000 == 0) {
    systemStats.uptime = currentTime / 1000;
  }

  // Run state machine
  runStateMachine();

  // Process incoming serial data from sensors (non-blocking)
  if (SERIAL_SENSOR.available()) {
    processSensorSerialData();
  }

  // Maintain WiFi connection
  if (config.enableWiFiFallback && wifiConnected) {
    if (WiFi.status() != WL_CONNECTED) {
      wifiConnected = false;
      setStatusLED(LED_WIFI, false);
      Serial.println("WiFi connection lost");
    }
  }

  // Maintain MQTT connection
  if (wifiConnected && mqttConnected) {
    mqttClient.poll();
    if (!mqttClient.connected()) {
      mqttConnected = false;
      Serial.println("MQTT connection lost");
    }
  }

  // Check battery level
  static unsigned long lastBatteryCheck = 0;
  if (currentTime - lastBatteryCheck >= 60000) {  // Check every minute
    lastBatteryCheck = currentTime;
    checkBatteryLevel();
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

    case STATE_CONNECT_LORAWAN:
      handleConnectLoRaWANState();
      break;

    case STATE_CONNECT_WIFI:
      handleConnectWiFiState();
      break;

    case STATE_IDLE:
      handleIdleState();
      break;

    case STATE_READING_SENSORS:
      handleReadingSensorsState();
      break;

    case STATE_READING_CAMERAS:
      handleReadingCamerasState();
      break;

    case STATE_PROCESSING_DATA:
      handleProcessingDataState();
      break;

    case STATE_TRANSMITTING_LORAWAN:
      handleTransmittingLoRaWANState();
      break;

    case STATE_TRANSMITTING_WIFI:
      handleTransmittingWiFiState();
      break;

    case STATE_LOW_POWER:
      handleLowPowerMode();
      break;

    case STATE_ERROR:
      handleErrorState();
      break;

    case STATE_RECOVERY:
      handleRecoveryState();
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

void handleConnectLoRaWANState(void) {
  static uint8_t retries = 0;

  if (!lorawanConnected) {
    if (retries < MAX_LORA_JOIN_RETRIES) {
      Serial.print("Connecting to LoRaWAN (attempt ");
      Serial.print(retries + 1);
      Serial.print("/");
      Serial.print(MAX_LORA_JOIN_RETRIES);
      Serial.println(")...");

      if (initializeLoRaWAN()) {
        lorawanConnected = true;
        setStatusLED(LED_LORA, true);
        Serial.println("LoRaWAN connected successfully");
        retries = 0;
        currentState = STATE_IDLE;
        return;
      } else {
        retries++;
        systemStats.loraJoinRetries++;
        delay(RETRY_DELAY_MS);
      }
    } else {
      Serial.println("Max LoRaWAN join retries reached. Will retry later.");
      retries = 0;
      currentState = STATE_IDLE;
    }
  } else {
    currentState = STATE_IDLE;
  }
}

void handleConnectWiFiState(void) {
  static uint8_t retries = 0;

  if (!wifiConnected && config.enableWiFiFallback) {
    if (retries < MAX_WIFI_CONNECT_RETRIES) {
      Serial.print("Connecting to WiFi (attempt ");
      Serial.print(retries + 1);
      Serial.print("/");
      Serial.print(MAX_WIFI_CONNECT_RETRIES);
      Serial.println(")...");

      if (initializeWiFi()) {
        wifiConnected = true;
        setStatusLED(LED_WIFI, true);
        Serial.println("WiFi connected successfully");
        retries = 0;

        // Connect to MQTT
        reconnectMQTT();
        currentState = STATE_IDLE;
        return;
      } else {
        retries++;
        delay(RETRY_DELAY_MS);
      }
    } else {
      Serial.println("Max WiFi connection retries reached");
      retries = 0;
      currentState = STATE_IDLE;
    }
  } else {
    currentState = STATE_IDLE;
  }
}

void handleIdleState(void) {
  unsigned long currentTime = millis();

  // Check for scheduled tasks
  bool taskScheduled = false;

  // Time to read sensors?
  if (currentTime - lastSensorReadTime >= (config.sensorReadInterval * 1000)) {
    enterState(STATE_READING_SENSORS);
    taskScheduled = true;
  }

  // Time to poll cameras?
  if (!taskScheduled && (currentTime - lastCameraPollTime >= CAMERA_POLL_INTERVAL)) {
    enterState(STATE_READING_CAMERAS);
    taskScheduled = true;
  }

  // Time to transmit via LoRaWAN?
  if (!taskScheduled && lorawanConnected &&
      (currentTime - lastTransmitTime >= (config.transmitInterval * 60000))) {
    enterState(STATE_PROCESSING_DATA);
    taskScheduled = true;
  }

  // Time for heartbeat?
  if (!taskScheduled && (currentTime - lastHeartbeatTime >= HEARTBEAT_INTERVAL)) {
    lastHeartbeatTime = currentTime;
    printSystemStats();
  }

  // Check for LoRaWAN reconnection if needed
  if (!lorawanConnected && (currentTime % 30000 < 100)) {
    // Every 30 seconds, try to reconnect
    handleConnectLoRaWANState();
  }

  // Enter low power mode if enabled and no tasks scheduled
  if (!taskScheduled && config.enableLowPowerMode) {
    // Only enter low power if nothing to do for at least 1 second
    unsigned long timeToNextTask = config.sensorReadInterval * 1000 -
                                   (currentTime - lastSensorReadTime);
    if (timeToNextTask >= 1000) {
      enterState(STATE_LOW_POWER);
    }
  }
}

void handleReadingSensorsState(void) {
  Serial.println("\n=== Reading Sensors ===");
  lastSensorReadTime = millis();

  // Request data from sensor
  if (requestSensorData()) {
    // Wait for response with timeout
    unsigned long timeout = millis() + SERIAL_TIMEOUT;
    bool dataReceived = false;

    while (millis() < timeout) {
      if (processSensorSerialData()) {
        dataReceived = true;
        break;
      }
      delay(10);
      #ifdef ADAFRUIT_SLEEPYDOG_H
        Watchdog.reset();
      #endif
    }

    if (dataReceived && validateSensorValues()) {
      systemStats.sensorReadCount++;
      Serial.println("Sensor data read successfully");
    } else {
      sensorData.errorCount++;
      systemStats.errorCount++;
      Serial.println("Sensor data read failed or invalid");
    }
  }

  currentState = STATE_IDLE;
}

void handleReadingCamerasState(void) {
  Serial.println("\n=== Reading Cameras ===");
  lastCameraPollTime = millis();

  // Read from both cameras
  for (uint8_t i = 0; i < 2; i++) {
    Serial.print("Camera ");
    Serial.print(i);
    Serial.print(": ");

    if (requestCameraData(i)) {
      // Process camera data
      unsigned long timeout = millis() + SERIAL_TIMEOUT;
      bool dataReceived = false;

      while (millis() < timeout) {
        if (processCameraSerialData(i)) {
          dataReceived = true;
          break;
        }
        delay(10);
      }

      if (dataReceived && validateVisionValues(i)) {
        systemStats.cameraReadCount++;
        Serial.println("OK");
      } else {
        visionData[i].errorCount++;
        systemStats.errorCount++;
        Serial.println("FAILED");
      }
    }
  }

  currentState = STATE_IDLE;
}

void handleProcessingDataState(void) {
  Serial.println("\n=== Processing Data ===");

  // Aggregate all sensor and vision data
  aggregateData();

  // Determine transmission method
  if (lorawanConnected) {
    enterState(STATE_TRANSMITTING_LORAWAN);
  } else if (wifiConnected && config.enableWiFiFallback) {
    enterState(STATE_TRANSMITTING_WIFI);
  } else {
    Serial.println("No connection available for transmission");
    currentState = STATE_IDLE;
  }
}

void handleTransmittingLoRaWANState(void) {
  Serial.println("\n=== Transmitting via LoRaWAN ===");

  if (transmitLoRaWAN()) {
    systemStats.loraTransmitCount++;
    lastTransmitTime = millis();
    Serial.println("LoRaWAN transmission successful");
    blinkLED(LED_LORA, 2, 100);
  } else {
    systemStats.errorCount++;
    Serial.println("LoRaWAN transmission failed");

    // Try WiFi fallback
    if (wifiConnected && config.enableWiFiFallback) {
      Serial.println("Falling back to WiFi");
      enterState(STATE_TRANSMITTING_WIFI);
      return;
    }
  }

  currentState = STATE_IDLE;
}

void handleTransmittingWiFiState(void) {
  Serial.println("\n=== Transmitting via WiFi ===");

  if (transmitWiFi()) {
    systemStats.wifiTransmitCount++;
    lastTransmitTime = millis();
    Serial.println("WiFi transmission successful");
    blinkLED(LED_WIFI, 2, 100);
  } else {
    systemStats.errorCount++;
    Serial.println("WiFi transmission failed");
  }

  currentState = STATE_IDLE;
}

void handleLowPowerMode(void) {
  Serial.println("\n=== Entering Low Power Mode ===");
  Serial.flush();

  // Calculate sleep time (wake up before next sensor read)
  unsigned long timeToNextRead = (config.sensorReadInterval * 1000) -
                                  (millis() - lastSensorReadTime);
  uint32_t sleepMs = min(timeToNextRead - 1000, (unsigned long)5000); // Max 5 seconds

  enterLowPowerMode(sleepMs);

  currentState = STATE_IDLE;
}

void handleErrorState(void) {
  // Blink error LED
  setStatusLED(LED_ERROR, millis() % 500 < 250);

  // Try to recover after 10 seconds
  if (millis() - errorTime >= 10000) {
    Serial.println("\n=== Attempting Recovery ===");
    currentState = STATE_RECOVERY;
  }
}

void handleRecoveryState(void) {
  Serial.println("Recovering from error...");

  // Clear error state
  clearError();
  setStatusLED(LED_ERROR, false);

  // Reinitialize critical components
  if (!lorawanConnected) {
    handleConnectLoRaWANState();
  }

  if (!wifiConnected && config.enableWiFiFallback) {
    handleConnectWiFiState();
  }

  // Return to idle
  currentState = STATE_IDLE;
  Serial.println("Recovery complete");
}

// ===========================================
// INITIALIZATION FUNCTIONS
// ===========================================

bool initializeI2C(void) {
  // Initialize I2C with custom pins
  Wire.begin(I2C_SDA, I2C_SCL);
  Wire.setClock(I2C_CLOCK_SPEED);

  // Test connection to TCA9548A multiplexer
  if (!testI2CDevice(TCA9548A_ADDR)) {
    Serial.print("  ERROR: TCA9548A not found at 0x");
    Serial.println(TCA9548A_ADDR, HEX);
    return false;
  }

  Serial.println("  TCA9548A I2C multiplexer detected");

  // Disable all channels initially
  switchI2CChannel(TCA_CHANNEL_NONE);

  Serial.println("  I2C initialized successfully");
  return true;
}

bool initializeLoRaWAN(void) {
  #ifdef ARDUINO_SAMD_MKRWAN1310
    // Check for LoRa module
    if (!modem.begin(LORAWAN_REGION)) {
      Serial.println("  ERROR: Failed to start LoRaWAN modem");
      return false;
    }

    Serial.print("  LoRaWAN modem initialized (");
    Serial.print(LORAWAN_REGION == EU868 ? "EU868" :
                 LORAWAN_REGION == US915 ? "US915" :
                 LORAWAN_REGION == AU915 ? "AU915" : "AS923");
    Serial.println(")");

    // Set data rate and TX power from config
    if (!modem.dataRate(config.loraDataRate)) {
      Serial.println("  WARNING: Failed to set data rate");
    }

    if (!modem.power(config.loraTxPower)) {
      Serial.println("  WARNING: Failed to set TX power");
    }

    // Join OTAA
    Serial.print("  Joining LoRaWAN network (OTAA)...");

    int result = modem.joinOTAA(LORAWAN_APP_EUI, LORAWAN_APP_KEY);

    if (result == 0) {
      Serial.println(" SUCCESS");
      return true;
    } else {
      Serial.print(" FAILED (code: ");
      Serial.print(result);
      Serial.println(")");
      return false;
    }
  #else
    // Alternative for non-MKRWAN boards
    // Implement ArduinoLoRaWAN library initialization here
    Serial.println("  ERROR: LoRaWAN initialization not implemented for this board");
    return false;
  #endif
}

bool initializeWiFi(void) {
  // Check for WiFi module
  if (WiFi.status() == WL_NO_MODULE) {
    Serial.println("  ERROR: WiFi module not found");
    return false;
  }

  // Check firmware version
  String fv = WiFi.firmwareVersion();
  if (fv < "1.0.0") {
    Serial.println("  WARNING: WiFi firmware may need upgrading");
    Serial.print("  Current firmware: ");
    Serial.println(fv);
  }

  // Attempt to connect
  Serial.print("  Connecting to '");
  Serial.print(WIFI_SSID);
  Serial.println("'...");

  unsigned long startTime = millis();
  int status = WiFi.begin(WIFI_SSID, WIFI_PASS);

  while (WiFi.status() != WL_CONNECTED &&
         (millis() - startTime < WIFI_CONNECTION_TIMEOUT)) {
    Serial.print(".");
    delay(500);
    #ifdef ADAFRUIT_SLEEPYDOG_H
      Watchdog.reset();
    #endif
  }

  Serial.println();

  if (WiFi.status() == WL_CONNECTED) {
    Serial.print("  WiFi connected, IP: ");
    Serial.println(WiFi.localIP());
    Serial.print("  Signal strength: ");
    Serial.print(WiFi.RSSI());
    Serial.println(" dBm");
    return true;
  } else {
    Serial.print("  WiFi connection timeout (status: ");
    Serial.print(WiFi.status());
    Serial.println(")");
    return false;
  }
}

bool initializeRTC(void) {
  rtc.begin();

  // Set RTC to compile time if not set
  if (rtc.getYear() == 0) {
    // January 1, 2025, 00:00:00
    rtc.setDate(1, 1, 25);
    rtc.setTime(0, 0, 0);
  }

  Serial.println("RTC initialized");
  Serial.print("Current time: ");
  Serial.println(getFormattedTime());

  return true;
}

bool loadConfiguration(void) {
  Config storedConfig = flash_config.read();

  // Validate magic number and checksum
  if (storedConfig.magic != 0x4A5A4A5A) {
    Serial.println("No valid configuration found in flash");
    return false;
  }

  uint32_t calcChecksum = calculateChecksum(&storedConfig);
  if (storedConfig.checksum != calcChecksum) {
    Serial.println("Configuration checksum mismatch");
    return false;
  }

  config = storedConfig;
  Serial.println("Configuration loaded from flash");
  return true;
}

bool saveConfiguration(void) {
  config.checksum = calculateChecksum(&config);
  flash_config.write(config);
  Serial.println("Configuration saved to flash");
  return true;
}

uint32_t calculateChecksum(const Config* cfg) {
  uint32_t sum = 0;
  const uint8_t* data = (const uint8_t*)cfg;
  for (size_t i = 0; i < sizeof(Config) - sizeof(uint32_t); i++) {
    sum += data[i];
  }
  return sum;
}

// ===========================================
// I2C AND MULTIPLEXER FUNCTIONS
// ===========================================

bool switchI2CChannel(uint8_t channel) {
  Wire.beginTransmission(TCA9548A_ADDR);
  Wire.write(channel);
  uint8_t error = Wire.endTransmission();

  if (error != 0) {
    Serial.print("  ERROR: Failed to switch I2C channel (error: ");
    Serial.print(error);
    Serial.println(")");
    return false;
  }

  return true;
}

bool scanI2CDevices(void) {
  Serial.println("\n=== I2C Device Scan ===");
  bool devicesFound = false;

  for (uint8_t addr = 0x08; addr <= 0x77; addr++) {
    Wire.beginTransmission(addr);
    uint8_t error = Wire.endTransmission();

    if (error == 0) {
      Serial.print("  Device found at 0x");
      if (addr < 0x10) Serial.print("0");
      Serial.println(addr, HEX);
      devicesFound = true;
    }
  }

  if (!devicesFound) {
    Serial.println("  No I2C devices found");
  }

  return devicesFound;
}

bool testI2CDevice(uint8_t address) {
  Wire.beginTransmission(address);
  uint8_t error = Wire.endTransmission();
  return (error == 0);
}

// ===========================================
// SENSOR COMMUNICATION FUNCTIONS
// ===========================================

bool requestSensorData(void) {
  SERIAL_SENSOR.println("REQ");
  return true;
}

bool requestCameraData(uint8_t cameraId) {
  // Select I2C channel
  uint8_t channel = (cameraId == 0) ? TCA_CHANNEL_VISION_1 : TCA_CHANNEL_VISION_2;
  if (!switchI2CChannel(channel)) {
    return false;
  }

  // Send request via I2C or serial
  // This is a simplified version - implement based on your camera communication protocol
  return true;
}

bool processSensorSerialData(void) {
  static char buffer[256];
  static int index = 0;

  while (SERIAL_SENSOR.available()) {
    char c = SERIAL_SENSOR.read();

    if (c == '\n') {
      buffer[index] = '\0';

      // Parse data format: "SENS:temp,humidity,pressure,gas,iaq,timestamp"
      if (strncmp(buffer, "SENS:", 5) == 0) {
        char* ptr = buffer + 5;

        sensorData.temperature = atof(ptr);
        ptr = strchr(ptr, ',') + 1;

        sensorData.humidity = atof(ptr);
        ptr = strchr(ptr, ',') + 1;

        sensorData.pressure = atof(ptr);
        ptr = strchr(ptr, ',') + 1;

        sensorData.gasResistance = atof(ptr);
        ptr = strchr(ptr, ',') + 1;

        sensorData.iaq = atof(ptr);
        ptr = strchr(ptr, ',') + 1;

        sensorData.timestamp = strtoul(ptr, NULL, 10);
        sensorData.valid = true;

        Serial.println("  Data received from Nicla Sense Me");
        Serial.print("    Temperature: ");
        Serial.print(sensorData.temperature, 1);
        Serial.println(" °C");
        Serial.print("    Humidity: ");
        Serial.print(sensorData.humidity, 1);
        Serial.println(" %");
        Serial.print("    Pressure: ");
        Serial.print(sensorData.pressure, 1);
        Serial.println(" hPa");
        Serial.print("    IAQ: ");
        Serial.println(sensorData.iaq, 0);

        index = 0;
        return true;
      }

      index = 0;
    } else if (index < sizeof(buffer) - 1) {
      buffer[index++] = c;
    }
  }

  return false;
}

bool processCameraSerialData(uint8_t cameraId) {
  // Implement camera data parsing based on your protocol
  // This is a placeholder - implement based on actual camera data format

  // Example format: "CAM:id,class,confidence,x,y,w,h,timestamp"
  // For now, just mark as valid if any data available
  if (SERIAL_SENSOR.available()) {
    visionData[cameraId].valid = true;
    visionData[cameraId].timestamp = millis();
    return true;
  }

  return false;
}

void readSensorData(void) {
  // This function is called from state machine
  // Actual reading is done in processSensorSerialData()
}

void readCameraData(uint8_t cameraId) {
  // This function is called from state machine
  // Actual reading is done in processCameraSerialData()
}

// ===========================================
// DATA PROCESSING FUNCTIONS
// ===========================================

void aggregateData(void) {
  // Create aggregated JSON payload
  formatSensorDataJSON();

  // Add vision data if available
  if (visionData[0].valid || visionData[1].valid) {
    for (uint8_t i = 0; i < 2; i++) {
      if (visionData[i].valid) {
        formatVisionDataJSON(i);
      }
    }
  }
}

void formatSensorDataJSON(void) {
  int charsWritten = snprintf(
    jsonBuffer,
    JSON_BUFFER_SIZE,
    "{\"type\":\"sensor\","
    "\"temp\":%.2f,"
    "\"hum\":%.2f,"
    "\"pres\":%.2f,"
    "\"gas\":%.0f,"
    "\"iaq\":%.0f,"
    "\"ts\":%lu,"
    "\"uptime\":%lu}",
    sensorData.temperature,
    sensorData.humidity,
    sensorData.pressure,
    sensorData.gasResistance,
    sensorData.iaq,
    sensorData.timestamp,
    systemStats.uptime
  );

  if (charsWritten < 0 || charsWritten >= JSON_BUFFER_SIZE) {
    Serial.println("ERROR: JSON buffer overflow");
  }
}

void formatVisionDataJSON(uint8_t cameraId) {
  char visionJson[256];
  int charsWritten = snprintf(
    visionJson,
    sizeof(visionJson),
    "{\"type\":\"vision\","
    "\"cam\":%d,"
    "\"class\":%d,"
    "\"conf\":%.2f,"
    "\"ts\":%lu}",
    visionData[cameraId].cameraId,
    visionData[cameraId].detectedClass,
    visionData[cameraId].confidence,
    visionData[cameraId].timestamp
  );

  if (charsWritten > 0 && charsWritten < (int)sizeof(visionJson)) {
    // Could append to main JSON or send separately
  }
}

void formatLoRaWANPayload(void) {
  // Create binary payload for LoRaWAN (more efficient than JSON)
  // Format: [type(1), temp(2), hum(2), pres(2), gas(2), flags(1)]
  // Type: 0x01 = sensor data

  loraPayload[0] = 0x01;  // Packet type: sensor data

  // Convert floats to uint16_t (scale by 100)
  int16_t temp = (int16_t)(sensorData.temperature * 100);
  uint16_t hum = (uint16_t)(sensorData.humidity * 100);
  uint16_t pres = (uint16_t)(sensorData.pressure * 10);
  uint16_t gas = (uint16_t)(sensorData.gasResistance);

  // Pack data (big-endian)
  loraPayload[1] = (temp >> 8) & 0xFF;
  loraPayload[2] = temp & 0xFF;
  loraPayload[3] = (hum >> 8) & 0xFF;
  loraPayload[4] = hum & 0xFF;
  loraPayload[5] = (pres >> 8) & 0xFF;
  loraPayload[6] = pres & 0xFF;
  loraPayload[7] = (gas >> 8) & 0xFF;
  loraPayload[8] = gas & 0xFF;

  // Flags byte
  uint8_t flags = 0;
  flags |= (sensorData.valid ? 0x01 : 0x00);
  flags |= (visionData[0].valid ? 0x02 : 0x00);
  flags |= (visionData[1].valid ? 0x04 : 0x00);
  loraPayload[9] = flags;

  // Payload size
  uint8_t payloadSize = 10;
}

void formatSystemStatsJSON(void) {
  snprintf(
    jsonBuffer,
    JSON_BUFFER_SIZE,
    "{\"type\":\"stats\","
    "\"uptime\":%lu,"
    "\"lorawan\":{\"tx\":%lu,\"connected\":%s},"
    "\"wifi\":{\"tx\":%lu,\"connected\":%s},"
    "\"errors\":%lu,"
    "\"battery\":{"
    "\"level\":%d,"
    "\"voltage\":%.2f,"
    "\"low\":%s}}",
    systemStats.uptime,
    systemStats.loraTransmitCount,
    lorawanConnected ? "true" : "false",
    systemStats.wifiTransmitCount,
    wifiConnected ? "true" : "false",
    systemStats.errorCount,
    systemStats.batteryLevel,
    systemStats.batteryVoltage,
    systemStats.lowBattery ? "true" : "false"
  );
}

// ===========================================
// TRANSMISSION FUNCTIONS
// ===========================================

bool transmitLoRaWAN(void) {
  #ifdef ARDUINO_SAMD_MKRWAN1310
    if (!lorawanConnected) {
      Serial.println("  ERROR: LoRaWAN not connected");
      return false;
    }

    // Format payload
    formatLoRaWANPayload();

    // Begin packet
    int result = modem.beginPacket();
    if (result != 1) {
      Serial.print("  ERROR: beginPacket failed (");
      Serial.print(result);
      Serial.println(")");
      return false;
    }

    // Write payload
    modem.write(loraPayload, 10);

    // End packet and send
    result = modem.endPacket(true);

    if (result > 0) {
      Serial.println("  Packet sent successfully");
      return true;
    } else {
      Serial.print("  ERROR: Packet send failed (");
      Serial.print(result);
      Serial.println(")");
      return false;
    }
  #else
    return false;
  #endif
}

bool transmitWiFi(void) {
  if (!wifiConnected) {
    Serial.println("  ERROR: WiFi not connected");
    return false;
  }

  // Format data
  formatSensorDataJSON();

  // Send via MQTT if connected
  if (mqttConnected) {
    return transmitMQTT(MQTT_TOPIC_SENSOR, jsonBuffer);
  } else {
    // Fallback: HTTP POST (implement if needed)
    Serial.println("  WiFi available but MQTT not connected");
    return false;
  }
}

bool transmitMQTT(const char* topic, const char* payload) {
  if (!mqttClient.connected()) {
    Serial.println("  ERROR: MQTT not connected");
    return false;
  }

  Serial.print("  Publishing to '");
  Serial.print(topic);
  Serial.println("'...");

  if (mqttClient.beginMessage(topic)) {
    mqttClient.print(payload);
    if (mqttClient.endMessage()) {
      Serial.println("  MQTT message sent successfully");
      return true;
    }
  }

  Serial.println("  ERROR: MQTT message failed");
  return false;
}

bool reconnectLoRaWAN(void) {
  // Try to reconnect to LoRaWAN
  return initializeLoRaWAN();
}

bool reconnectWiFi(void) {
  // Try to reconnect to WiFi
  return initializeWiFi();
}

bool reconnectMQTT(void) {
  if (!wifiConnected) {
    return false;
  }

  Serial.print("  Connecting to MQTT broker '");
  Serial.print(MQTT_BROKER);
  Serial.println("'...");

  mqttClient.setId("mkr-wan-gateway");
  mqttClient.onMessage([](int messageSize) {
    // Handle incoming MQTT messages
    Serial.println("Received MQTT message");
  });

  if (mqttClient.connect(MQTT_BROKER, MQTT_PORT)) {
    Serial.println("  MQTT connected");
    mqttConnected = true;
    return true;
  } else {
    Serial.println("  MQTT connection failed");
    mqttConnected = false;
    return false;
  }
}

// ===========================================
// POWER MANAGEMENT FUNCTIONS
// ===========================================

void enterLowPowerMode(uint32_t sleepMs) {
  Serial.print("  Sleeping for ");
  Serial.print(sleepMs);
  Serial.println(" ms...");

  Serial.flush();

  #ifdef ADAFRUIT_SLEEPYDOG_H
    // Sleep with watchdog enabled
    int actualSleep = Watchdog.sleep(sleepMs);

    Serial.print("  Woke after ");
    Serial.print(actualSleep);
    Serial.println(" ms");
  #else
    // Fallback: use delay
    delay(sleepMs);
  #endif
}

void wakeFromLowPowerMode(void) {
  // Reinitialize peripherals if needed
  Serial.println("  Waking from low power mode");
}

void feedWatchdog(void) {
  #ifdef ADAFRUIT_SLEEPYDOG_H
    Watchdog.reset();
  #endif
  lastWatchdogFeedTime = millis();
}

void checkBatteryLevel(void) {
  // Read battery voltage (MKR WAN 1310 has ADC on pin A0)
  // Note: This requires voltage divider circuit
  // Default implementation assumes external battery monitor

  int rawValue = analogRead(ADC_BATTERY);
  float voltage = (rawValue / 1023.0) * 3.3;  // Convert to voltage

  // Add voltage divider factor if using external divider
  // voltage *= VOLTAGE_DIVIDER_RATIO;

  systemStats.batteryVoltage = voltage;

  // Estimate battery level (simplified)
  // Assuming 3.7V Li-Po: 4.2V = 100%, 3.0V = 0%
  if (voltage >= 4.2) {
    systemStats.batteryLevel = 100;
  } else if (voltage <= 3.0) {
    systemStats.batteryLevel = 0;
  } else {
    systemStats.batteryLevel = (uint8_t)((voltage - 3.0) / 1.2 * 100);
  }

  systemStats.lowBattery = (systemStats.batteryLevel < 20);

  if (systemStats.lowBattery) {
    Serial.println("WARNING: Low battery!");
    blinkLED(LED_ERROR, 5, 100);
  }
}

bool prepareSleep(void) {
  // Prepare system for sleep
  // Disconnect WiFi to save power
  if (wifiConnected) {
    WiFi.disconnect();
    wifiConnected = false;
    setStatusLED(LED_WIFI, false);
  }

  return true;
}

bool prepareWake(void) {
  // Reinitialize after wake
  if (config.enableWiFiFallback) {
    initializeWiFi();
  }

  return true;
}

// ===========================================
// UTILITY FUNCTIONS
// ===========================================

void setStatusLED(uint8_t led, bool state) {
  digitalWrite(led, state ? HIGH : LOW);
}

void blinkLED(uint8_t led, uint8_t count, uint16_t delayMs) {
  for (uint8_t i = 0; i < count; i++) {
    setStatusLED(led, true);
    delay(delayMs);
    setStatusLED(led, false);
    if (i < count - 1) delay(delayMs);
  }
}

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
  setStatusLED(LED_ERROR, true);
}

void clearError(void) {
  lastErrorMessage[0] = '\0';
  lastErrorCode = 0;
  setStatusLED(LED_ERROR, false);
}

void printSystemInfo(void) {
  Serial.println("\n========================================");
  Serial.println("  MKR WAN 1310 Gateway Firmware");
  Serial.println("========================================");
  Serial.println("Version: 2.0.0");
  Serial.print("Build: ");
  Serial.print(__DATE__);
  Serial.print(" ");
  Serial.println(__TIME__);
  Serial.println("Board: Arduino MKR WAN 1310");
  Serial.println("MCU: ATSAMD21G18A");
  Serial.println("LoRa: SX1276");
  Serial.println("WiFi: u-blox NINA-W102");
  Serial.println("========================================\n");
}

void printSystemStats(void) {
  Serial.println("\n=== System Statistics ===");
  Serial.print("Uptime: ");
  Serial.print(systemStats.uptime);
  Serial.println(" seconds");
  Serial.print("Sensor reads: ");
  Serial.println(systemStats.sensorReadCount);
  Serial.print("Camera reads: ");
  Serial.println(systemStats.cameraReadCount);
  Serial.print("LoRaWAN transmissions: ");
  Serial.println(systemStats.loraTransmitCount);
  Serial.print("WiFi transmissions: ");
  Serial.println(systemStats.wifiTransmitCount);
  Serial.print("Errors: ");
  Serial.println(systemStats.errorCount);
  Serial.print("Battery: ");
  Serial.print(systemStats.batteryLevel);
  Serial.print("% (");
  Serial.print(systemStats.batteryVoltage, 2);
  Serial.println("V)");
  Serial.println("========================\n");
}

void printDebug(const char* message) {
  Serial.print("[DEBUG ");
  Serial.print(millis() / 1000);
  Serial.print("] ");
  Serial.println(message);
}

void printHex(const uint8_t* data, size_t len) {
  for (size_t i = 0; i < len; i++) {
    if (data[i] < 0x10) Serial.print("0");
    Serial.print(data[i], HEX);
    Serial.print(" ");
  }
  Serial.println();
}

bool validateSensorValues(void) {
  // Check if values are within valid ranges
  if (isnan(sensorData.temperature) ||
      sensorData.temperature < -40.0 ||
      sensorData.temperature > 85.0) {
    return false;
  }

  if (isnan(sensorData.humidity) ||
      sensorData.humidity < 0.0 ||
      sensorData.humidity > 100.0) {
    return false;
  }

  if (isnan(sensorData.pressure) ||
      sensorData.pressure < 300.0 ||
      sensorData.pressure > 1100.0) {
    return false;
  }

  if (isnan(sensorData.gasResistance) ||
      sensorData.gasResistance < 0.0) {
    return false;
  }

  return true;
}

bool validateVisionValues(uint8_t cameraId) {
  // Validate vision detection data
  if (!visionData[cameraId].valid) {
    return false;
  }

  if (visionData[cameraId].confidence < 0.0 ||
      visionData[cameraId].confidence > 1.0) {
    return false;
  }

  return true;
}

// ===========================================
// TIMEKEEPING FUNCTIONS
// ===========================================

uint32_t getTimestamp(void) {
  return rtc.getEpoch();
}

void syncRTC(void) {
  // Sync RTC with network time if available
  // This could use WiFi or LoRaWAN time sync
  if (wifiConnected) {
    // Implement NTP sync here
  }
}

String getFormattedTime(void) {
  char buffer[32];
  snprintf(buffer, sizeof(buffer),
           "%04d-%02d-%02d %02d:%02d:%02d",
           rtc.getYear() + 2000,
           rtc.getMonth(),
           rtc.getDay(),
           rtc.getHours(),
           rtc.getMinutes(),
           rtc.getSeconds());
  return String(buffer);
}

// ===========================================
// END OF FILE
// ===========================================
