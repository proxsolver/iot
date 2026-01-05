/**
 * MKR WAN Gateway Firmware
 *
 * This firmware runs on the Arduino MKR WAN 1310 to act as the central hub,
 * receiving data from Nicla Sense Me and managing dual Nicla Vision cameras
 * via I2C multiplexer, then transmitting data via LoRaWAN or WiFi.
 *
 * Board: Arduino MKR WAN 1310 (SAMD21)
 * Communication: Serial, I2C, LoRaWAN, WiFi
 *
 * Features:
 * - I2C multiplexer control for dual cameras
 * - Serial communication with sensor nodes
 * - Data aggregation and packet formatting
 * - LoRaWAN transmission with OTAA
 * - WiFi fallback for high-bandwidth data
 * - Non-blocking operation
 * - Efficient memory management
 */

#include <Arduino.h>
#include <Wire.h>
#include <ArduinoLoRaWAN.h>
#include <SPI.h>
#include <WiFiNINA.h>

// ===========================================
// CONFIGURATION
// ===========================================

// Serial ports
#define SERIAL_BAUD 115200
#define SERIAL_SENSOR Serial1  // Nicla Sense Me

// I2C configuration
#define I2C_SDA 11
#define I2C_SCL 12
#define TCA9548A_ADDR 0x70

// I2C Multiplexer channels
#define TCA_CHANNEL_VISION_1 0
#define TCA_CHANNEL_VISION_2 1

// LoRaWAN configuration (OTAA)
const char *appEui = "YOUR_APP_EUI";      // Application EUI
const char *appKey = "YOUR_APP_KEY";      // Application Key
const char *devEui = "YOUR_DEV_EUI";      // Device EUI (optional)

// WiFi configuration
const char *ssid = "YOUR_WIFI_SSID";
const char *pass = "YOUR_WIFI_PASSWORD";

// Timing (non-blocking)
unsigned long lastTransmitTime = 0;
const unsigned long LORAWAN_TRANSMIT_INTERVAL = 60000;  // 1 minute
const unsigned long WIFI_TRANSMIT_INTERVAL = 300000;    // 5 minutes

unsigned long lastSensorReadTime = 0;
const unsigned long SENSOR_READ_INTERVAL = 2000;        // 2 seconds

// LED indicators
#define LED_STATUS LED_BUILTIN
#define LED_LORA 5
#define LED_WIFI 6

// Data structures
struct SensorData {
  float temperature;
  float humidity;
  float pressure;
  float gasResistance;
  uint32_t timestamp;
  bool valid;
};

struct VisionData {
  uint8_t cameraId;
  uint16_t width;
  uint16_t height;
  uint8_t detectedClass;
  float confidence;
  uint32_t timestamp;
  bool valid;
};

SensorData sensorData = {0};
VisionData visionData[2] = {0};  // Two cameras

// LoRaWAN state
bool lorawanConnected = false;
bool wifiConnected = false;

// Data buffer for transmission
#define DATA_BUFFER_SIZE 512
char dataBuffer[DATA_BUFFER_SIZE];

// State machine
typedef enum {
  STATE_INIT,
  STATE_IDLE,
  STATE_READING_SENSORS,
  STATE_READING_CAMERAS,
  STATE_TRANSMITTING_LORAWAN,
  STATE_TRANSMITTING_WIFI,
  STATE_ERROR
} SystemState;

SystemState currentState = STATE_INIT;
char lastErrorMessage[128] = {0};

// ===========================================
// FUNCTION PROTOTYPES
// ===========================================

bool initializeI2C();
bool initializeLoRaWAN();
bool initializeWiFi();
void readSensorData();
void readCameraData(uint8_t cameraId);
void switchI2CChannel(uint8_t channel);
void transmitLoRaWAN();
void transmitWiFi();
void aggregateData();
void setStatusLED(uint8_t led, bool on);
void handleError(const char* message);
void printSystemInfo();
void processSerialData();

// ===========================================
// SETUP
// ===========================================

void setup() {
  // Initialize serial ports
  Serial.begin(SERIAL_BAUD);
  while (!Serial && millis() < 3000);

  SERIAL_SENSOR.begin(SERIAL_BAUD);

  // Initialize status LEDs
  pinMode(LED_STATUS, OUTPUT);
  pinMode(LED_LORA, OUTPUT);
  pinMode(LED_WIFI, OUTPUT);
  setStatusLED(LED_STATUS, false);
  setStatusLED(LED_LORA, false);
  setStatusLED(LED_WIFI, false);

  // Print system information
  printSystemInfo();

  // Initialize I2C
  Serial.println("Initializing I2C...");
  if (!initializeI2C()) {
    handleError("I2C initialization failed");
    return;
  }

  // Initialize LoRaWAN
  Serial.println("Initializing LoRaWAN...");
  if (!initializeLoRaWAN()) {
    Serial.println("  Warning: LoRaWAN initialization failed");
    setStatusLED(LED_LORA, false);
  } else {
    setStatusLED(LED_LORA, true);
  }

  // Initialize WiFi
  Serial.println("Initializing WiFi...");
  if (!initializeWiFi()) {
    Serial.println("  Warning: WiFi initialization failed");
    setStatusLED(LED_WIFI, false);
  } else {
    setStatusLED(LED_WIFI, true);
  }

  // Ready
  Serial.println("System ready");
  Serial.println("Starting main loop...");
  currentState = STATE_IDLE;
  setStatusLED(LED_STATUS, true);
}

// ===========================================
// MAIN LOOP
// ===========================================

void loop() {
  unsigned long currentTime = millis();

  // State machine
  switch (currentState) {
    case STATE_IDLE:
      // Check if it's time to read sensors
      if (currentTime - lastSensorReadTime >= SENSOR_READ_INTERVAL) {
        lastSensorReadTime = currentTime;
        currentState = STATE_READING_SENSORS;
      }
      // Check if it's time to transmit via LoRaWAN
      else if (lorawanConnected &&
               currentTime - lastTransmitTime >= LORAWAN_TRANSMIT_INTERVAL) {
        lastTransmitTime = currentTime;
        currentState = STATE_TRANSMITTING_LORAWAN;
      }
      break;

    case STATE_READING_SENSORS:
      readSensorData();
      currentState = STATE_IDLE;
      break;

    case STATE_READING_CAMERAS:
      // Read from both cameras
      readCameraData(0);
      readCameraData(1);
      currentState = STATE_IDLE;
      break;

    case STATE_TRANSMITTING_LORAWAN:
      if (lorawanConnected) {
        transmitLoRaWAN();
      }
      currentState = STATE_IDLE;
      break;

    case STATE_TRANSMITTING_WIFI:
      if (wifiConnected) {
        transmitWiFi();
      }
      currentState = STATE_IDLE;
      break;

    case STATE_ERROR:
      // Blink status LED
      setStatusLED(LED_STATUS, currentTime % 1000 < 500);
      break;

    default:
      currentState = STATE_IDLE;
      break;
  }

  // Process incoming serial data from sensors
  processSerialData();

  // Maintain WiFi connection
  if (wifiConnected) {
    WiFi maintaining();
  }

  // Small delay
  delay(10);
}

// ===========================================
// I2C INITIALIZATION
// ===========================================

bool initializeI2C() {
  // Initialize I2C with custom pins
  Wire.begin(I2C_SDA, I2C_SCL);

  // Test I2C connection to multiplexer
  Wire.beginTransmission(TCA9548A_ADDR);
  uint8_t error = Wire.endTransmission();

  if (error != 0) {
    Serial.print("  TCA9548A not found at address 0x");
    Serial.println(TCA9548A_ADDR, HEX);
    return false;
  }

  Serial.println("  TCA9548A I2C multiplexer detected");
  Serial.println("  I2C initialized successfully");

  return true;
}

// ===========================================
// LORAWAN INITIALIZATION
// ===========================================

bool initializeLoRaWAN() {
  // Initialize LoRaWAN
  if (!LoRaWAN.begin(US915)) {  // Change to EU868 for Europe
    Serial.println("  Failed to start LoRaWAN");
    return false;
  }

  // Set OTAA parameters
  LoRaWAN.joinOTAA(appEui, appKey, devEui);

  // Wait for connection
  Serial.print("  Connecting to LoRaWAN");
  int retries = 0;
  while (!LoRaWAN.connected() && retries < 10) {
    Serial.print(".");
    delay(1000);
    retries++;
  }
  Serial.println();

  if (LoRaWAN.connected()) {
    Serial.println("  Connected to LoRaWAN");
    lorawanConnected = true;
    return true;
  } else {
    Serial.println("  LoRaWAN connection timeout");
    return false;
  }
}

// ===========================================
// WIFI INITIALIZATION
// ===========================================

bool initializeWiFi() {
  // Check for WiFi module
  if (WiFi.status() == WL_NO_MODULE) {
    Serial.println("  WiFi module not found");
    return false;
  }

  // Attempt to connect
  Serial.print("  Connecting to WiFi");
  int retries = 0;
  while (WiFi.begin(ssid, pass) != WL_CONNECTED && retries < 10) {
    Serial.print(".");
    delay(1000);
    retries++;
  }
  Serial.println();

  if (WiFi.status() == WL_CONNECTED) {
    Serial.print("  WiFi connected, IP: ");
    Serial.println(WiFi.localIP());
    wifiConnected = true;
    return true;
  } else {
    Serial.println("  WiFi connection timeout");
    return false;
  }
}

// ===========================================
// SENSOR DATA READING
// ===========================================

void readSensorData() {
  // Serial data is processed asynchronously in processSerialData()
  // This function is a placeholder for any active sensor polling

  Serial.println("Reading sensor data from Serial1...");

  // Request data from sensor
  SERIAL_SENSOR.println("REQ");

  // Wait for response
  unsigned long timeout = millis() + 1000;
  while (!SERIAL_SENSOR.available() && millis() < timeout) {
    delay(10);
  }

  if (SERIAL_SENSOR.available()) {
    processSerialData();
  }
}

// ===========================================
// CAMERA DATA READING
// ===========================================

void readCameraData(uint8_t cameraId) {
  Serial.print("Reading camera ");
  Serial.println(cameraId);

  // Switch I2C multiplexer to specified camera
  switchI2CChannel(cameraId);

  // Request image from camera
  // This is a placeholder - implement actual I2C communication
  // with the Nicla Vision board

  // For now, mark as invalid
  visionData[cameraId].valid = false;
}

// ===========================================
// I2C CHANNEL SWITCHING
// ===========================================

void switchI2CChannel(uint8_t channel) {
  if (channel > 7) {
    Serial.println("Invalid channel");
    return;
  }

  Wire.beginTransmission(TCA9548A_ADDR);
  Wire.write(1 << channel);
  Wire.endTransmission();

  Serial.print("Switched to I2C channel ");
  Serial.println(channel);
}

// ===========================================
// LORAWAN TRANSMISSION
// ===========================================

void transmitLoRaWAN() {
  Serial.println("Transmitting via LoRaWAN...");

  // Aggregate data into binary format
  aggregateData();

  // Create LoRaWAN payload (binary for efficiency)
  // Format: [temp(2), hum(2), pres(2), gas(2), status(1)]
  uint8_t payload[9];

  // Convert floats to uint16_t (scale by 100)
  int16_t temp = (int16_t)(sensorData.temperature * 100);
  uint16_t hum = (uint16_t)(sensorData.humidity * 100);
  uint16_t pres = (uint16_t)(sensorData.pressure * 10);
  uint16_t gas = (uint16_t)(sensorData.gasResistance);

  // Pack data
  payload[0] = (temp >> 8) & 0xFF;
  payload[1] = temp & 0xFF;
  payload[2] = (hum >> 8) & 0xFF;
  payload[3] = hum & 0xFF;
  payload[4] = (pres >> 8) & 0xFF;
  payload[5] = pres & 0xFF;
  payload[6] = (gas >> 8) & 0xFF;
  payload[7] = gas & 0xFF;
  payload[8] = sensorData.valid ? 0x01 : 0x00;

  // Send via LoRaWAN
  LoRaWAN.sendPayload(payload, sizeof(payload));

  Serial.println("LoRaWAN transmission complete");
}

// ===========================================
// WIFI TRANSMISSION
// ===========================================

void transmitWiFi() {
  Serial.println("Transmitting via WiFi...");

  // This would typically send data to a server via HTTP/MQTT
  // For now, just print the data

  Serial.println("WiFi payload:");
  Serial.println(dataBuffer);
}

// ===========================================
// DATA AGGREGATION
// ===========================================

void aggregateData() {
  // Format all data into JSON-like structure
  // Using snprintf instead of String for efficiency

  int charsWritten = snprintf(
    dataBuffer,
    DATA_BUFFER_SIZE,
    "{\"temp\":%.2f,\"hum\":%.2f,\"pres\":%.2f,\"gas\":%.0f,\"ts\":%lu}",
    sensorData.temperature,
    sensorData.humidity,
    sensorData.pressure,
    sensorData.gasResistance,
    sensorData.timestamp
  );

  if (charsWritten < 0 || charsWritten >= DATA_BUFFER_SIZE) {
    handleError("Data buffer overflow");
    return;
  }
}

// ===========================================
// SERIAL DATA PROCESSING
// ===========================================

void processSerialData() {
  // Process incoming data from sensor node
  while (SERIAL_SENSOR.available()) {
    char c = SERIAL_SENSOR.read();

    // Look for data packet format: "SENS:temp,hum,pres,gas,timestamp"
    static char buffer[128];
    static int index = 0;

    if (c == '\n') {
      buffer[index] = '\0';

      // Parse data
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
        sensorData.timestamp = strtoul(ptr, NULL, 10);
        sensorData.valid = true;

        Serial.println("Sensor data received:");
        Serial.print("  Temperature: ");
        Serial.println(sensorData.temperature);
        Serial.print("  Humidity: ");
        Serial.println(sensorData.humidity);
        Serial.print("  Pressure: ");
        Serial.println(sensorData.pressure);
        Serial.print("  Gas: ");
        Serial.println(sensorData.gasResistance);
      }

      index = 0;
    } else if (index < sizeof(buffer) - 1) {
      buffer[index++] = c;
    }
  }
}

// ===========================================
// UTILITY FUNCTIONS
// ===========================================

void setStatusLED(uint8_t led, bool on) {
  digitalWrite(led, on ? HIGH : LOW);
}

void handleError(const char* message) {
  currentState = STATE_ERROR;
  strncpy(lastErrorMessage, message, sizeof(lastErrorMessage) - 1);
  lastErrorMessage[sizeof(lastErrorMessage) - 1] = '\0';

  Serial.print("ERROR: ");
  Serial.println(lastErrorMessage);
}

void printSystemInfo() {
  Serial.println("==================================");
  Serial.println("MKR WAN Gateway Firmware");
  Serial.println("==================================");
  Serial.print("Version: 1.0.0");
  Serial.print("Build: ");
  Serial.print(__DATE__);
  Serial.print(" ");
  Serial.println(__TIME__);
  Serial.println("==================================");
}
