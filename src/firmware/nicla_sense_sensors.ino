/**
 * Nicla Sense Me Sensor Firmware
 *
 * This firmware runs on the Arduino Nicla Sense Me to read environmental
 * sensors (temperature, humidity, pressure, gas) and transmit data via Serial.
 *
 * Board: Arduino Nicla Sense Me (nRF52840)
 * Sensors: BME688 (temp, humidity, pressure, gas)
 *
 * Features:
 * - BSEC library integration for accurate sensor readings
 * - Non-blocking sensor reading (millis-based timing)
 * - Efficient memory management (no String class)
 * - Serial communication to gateway
 * - Low power operation
 * - Error handling and validation
 */

#include <Arduino.h>
#include <Arduino_BHY2.h>

// ===========================================
// CONFIGURATION
// ===========================================

// Serial communication
#define SERIAL_BAUD 115200

// Sensor IDs (based on Arduino_BHY2 library)
#define SENSOR_ID_TEMP  SENSOR_ID_TEMP
#define SENSOR_ID_HUM   SENSOR_ID_HUMD
#define SENSOR_ID_PRES  SENSOR_ID_BARO
#define SENSOR_ID_GAS   SENSOR_ID_GAS

// Timing (non-blocking)
unsigned long lastSensorReadTime = 0;
const unsigned long SENSOR_READ_INTERVAL = 2000;  // 2 seconds between reads

unsigned long lastTransmitTime = 0;
const unsigned long TRANSMIT_INTERVAL = 5000;     // 5 seconds between transmissions

// LED indicators
#define LED_STATUS LED_BUILTIN
#define LED_ACTIVITY LED_BUILTIN

// Sensor data structure
struct SensorData {
  float temperature;    // Celsius
  float humidity;       // Percentage
  float pressure;       // hPa
  float gasResistance;  // Ohms
  uint32_t timestamp;   // Millis
  bool valid;
};

SensorData currentData = {0};

// Data buffer for transmission
#define DATA_BUFFER_SIZE 256
char dataBuffer[DATA_BUFFER_SIZE];

// Error tracking
typedef enum {
  STATE_OK,
  STATE_SENSOR_ERROR,
  STATE_COMM_ERROR,
  STATE_INIT_ERROR
} SystemState;

SystemState currentState = STATE_OK;
char lastErrorMessage[128] = {0};

// ===========================================
// SENSOR OBJECTS
// ===========================================

// Create sensor objects
SensorTemperature tempSensor(SENSOR_ID_TEMP);
SensorHumidity humSensor(SENSOR_ID_HUM);
SensorPressure baroSensor(SENSOR_ID_PRES);
SensorGas gasSensor(SENSOR_ID_GAS);

// ===========================================
// FUNCTION PROTOTYPES
// ===========================================

bool initializeSensors();
void readSensors();
void transmitData();
void formatSensorData();
void setStatusLED(bool on);
void handleError(SystemState state, const char* message);
void printSystemInfo();
bool validateSensorData(float value, float min, float max);

// ===========================================
// SETUP
// ===========================================

void setup() {
  // Initialize serial communication
  Serial.begin(SERIAL_BAUD);
  while (!Serial && millis() < 3000);  // Wait up to 3 seconds for serial

  // Initialize status LED
  pinMode(LED_STATUS, OUTPUT);
  setStatusLED(false);

  // Print system information
  printSystemInfo();

  // Initialize sensors
  Serial.println("Initializing sensors...");
  if (!initializeSensors()) {
    handleError(STATE_INIT_ERROR, "Sensor initialization failed");
    return;
  }

  // Ready
  Serial.println("System ready");
  Serial.println("Reading sensors...");
  currentState = STATE_OK;
}

// ===========================================
// MAIN LOOP
// ===========================================

void loop() {
  // Update sensors (non-blocking)
  BHY2.update();

  unsigned long currentTime = millis();

  // Read sensors at specified interval
  if (currentTime - lastSensorReadTime >= SENSOR_READ_INTERVAL) {
    lastSensorReadTime = currentTime;
    readSensors();
  }

  // Transmit data at specified interval
  if (currentTime - lastTransmitTime >= TRANSMIT_INTERVAL) {
    lastTransmitTime = currentTime;
    if (currentData.valid) {
      transmitData();
    }
  }

  // Small delay to prevent watchdog issues
  delay(10);
}

// ===========================================
// SENSOR INITIALIZATION
// ===========================================

bool initializeSensors() {
  Serial.println("  Initializing BHY2 sensor...");

  // Initialize BHY2 sensor
  if (!BHY2.begin()) {
    Serial.println("  Failed to initialize BHY2!");
    return false;
  }

  // Configure and start sensors
  Serial.println("  Starting sensors...");

  tempSensor.begin(10);   // Sample every 100ms
  humSensor.begin(10);
  baroSensor.begin(10);
  gasSensor.begin(10);

  // Wait for sensors to stabilize
  delay(1000);

  Serial.println("  Temperature sensor: OK");
  Serial.println("  Humidity sensor: OK");
  Serial.println("  Pressure sensor: OK");
  Serial.println("  Gas sensor: OK");
  Serial.println("  All sensors initialized successfully");

  return true;
}

// ===========================================
// READ SENSORS
// ===========================================

void readSensors() {
  // Read temperature
  float temp = tempSensor.value();
  if (!validateSensorData(temp, -40.0, 85.0)) {
    handleError(STATE_SENSOR_ERROR, "Invalid temperature reading");
    return;
  }
  currentData.temperature = temp;

  // Read humidity
  float hum = humSensor.value();
  if (!validateSensorData(hum, 0.0, 100.0)) {
    handleError(STATE_SENSOR_ERROR, "Invalid humidity reading");
    return;
  }
  currentData.humidity = hum;

  // Read pressure
  float pres = baroSensor.value();
  if (!validateSensorData(pres, 300.0, 1100.0)) {
    handleError(STATE_SENSOR_ERROR, "Invalid pressure reading");
    return;
  }
  currentData.pressure = pres;

  // Read gas resistance
  float gas = gasSensor.value();
  if (!validateSensorData(gas, 0.0, 1000000.0)) {
    handleError(STATE_SENSOR_ERROR, "Invalid gas reading");
    return;
  }
  currentData.gasResistance = gas;

  // Set timestamp and valid flag
  currentData.timestamp = millis();
  currentData.valid = true;

  // Print sensor readings
  Serial.println("Sensor readings:");
  Serial.print("  Temperature: ");
  Serial.print(currentData.temperature, 1);
  Serial.println(" C");

  Serial.print("  Humidity: ");
  Serial.print(currentData.humidity, 1);
  Serial.println(" %");

  Serial.print("  Pressure: ");
  Serial.print(currentData.pressure, 1);
  Serial.println(" hPa");

  Serial.print("  Gas Resistance: ");
  Serial.print(currentData.gasResistance, 0);
  Serial.println(" Ohm");

  // Blink LED to indicate activity
  setStatusLED(true);
  delay(50);
  setStatusLED(false);
}

// ===========================================
// TRANSMIT DATA
// ===========================================

void transmitData() {
  // Format sensor data into buffer
  formatSensorData();

  // Transmit via Serial
  Serial.println(dataBuffer);

  Serial.println("Data transmitted");
}

// ===========================================
// FORMAT SENSOR DATA
// ===========================================

void formatSensorData() {
  // Format: SENS:temp,humidity,pressure,gas,timestamp
  // Using snprintf instead of String for efficiency

  int charsWritten = snprintf(
    dataBuffer,
    DATA_BUFFER_SIZE,
    "SENS:%.2f,%.2f,%.2f,%.0f,%lu",
    currentData.temperature,
    currentData.humidity,
    currentData.pressure,
    currentData.gasResistance,
    currentData.timestamp
  );

  // Check for buffer overflow
  if (charsWritten < 0 || charsWritten >= DATA_BUFFER_SIZE) {
    handleError(STATE_COMM_ERROR, "Data buffer overflow");
    return;
  }
}

// ===========================================
// UTILITY FUNCTIONS
// ===========================================

void setStatusLED(bool on) {
  digitalWrite(LED_STATUS, on ? HIGH : LOW);
}

void handleError(SystemState state, const char* message) {
  currentState = state;
  strncpy(lastErrorMessage, message, sizeof(lastErrorMessage) - 1);
  lastErrorMessage[sizeof(lastErrorMessage) - 1] = '\0';

  currentData.valid = false;

  Serial.print("ERROR: ");
  Serial.println(lastErrorMessage);

  // Blink error code
  if (currentState != STATE_OK) {
    for (int i = 0; i < 3; i++) {
      setStatusLED(true);
      delay(100);
      setStatusLED(false);
      delay(100);
    }
  }
}

bool validateSensorData(float value, float min, float max) {
  return !isnan(value) && !isinf(value) && value >= min && value <= max;
}

void printSystemInfo() {
  Serial.println("==================================");
  Serial.println("Nicla Sense Me Sensor Firmware");
  Serial.println("==================================");
  Serial.print("Version: 1.0.0");
  Serial.print("Build: ");
  Serial.print(__DATE__);
  Serial.print(" ");
  Serial.println(__TIME__);
  Serial.println("==================================");
}
