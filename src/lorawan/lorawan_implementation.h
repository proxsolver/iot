/**
 * LoRaWAN Implementation Header
 *
 * Complete LoRaWAN implementation for the Arduino MKR WAN 1310
 * with OTAA authentication, duty cycle management, and retry logic.
 */

#ifndef LORAWAN_IMPLEMENTATION_H
#define LORAWAN_IMPLEMENTATION_H

#include <ArduinoLoRaWAN.h>

// ===========================================
// CONFIGURATION
// ===========================================

// Region configuration
// Uncomment your region:
#define REGION US915     // USA
// #define REGION EU868  // Europe
// #define REGION AS923  // Asia
// #define REGION AU915  // Australia

// Data rate (DR) configuration
// Higher DR = faster transmission but shorter range
#define DATA_RATE DR3    // Balance between speed and range
#define TX_POWER 14      // TX power in dBm (max for US915)

// Duty cycle configuration
#define DUTY_CYCLE_LIMIT 1.0    // 1% duty cycle (FCC requirement)
#define DUTY_CYCLE_WINDOW 3600000  // 1 hour window

// Retry configuration
#define MAX_RETRIES 3
#define RETRY_DELAY_INITIAL 1000    // 1 second
#define RETRY_DELAY_MAX 60000       // 60 seconds

// Port configuration
#define LORAWAN_PORT 1

// ===========================================
// DATA STRUCTURES
// ===========================================

// LoRaWAN packet structure (binary format for efficiency)
#pragma pack(push, 1)
struct LoRaWANPacket {
  uint16_t header;        // Packet header (magic number + version)
  uint32_t timestamp;     // Unix timestamp
  int16_t temperature;    // Temperature x100 (e.g., 2345 = 23.45°C)
  uint16_t humidity;      // Humidity x100 (e.g., 6500 = 65.00%)
  uint16_t pressure;      // Pressure x10 (e.g., 10132 = 1013.2 hPa)
  uint16_t gasResistance; // Gas resistance in Ohms
  uint8_t status;         // Status flags
  uint8_t battery;        // Battery level (0-100)
  uint16_t checksum;      // CRC16 checksum
};
#pragma pack(pop)

// Packet status flags
#define STATUS_SENSOR_OK      0x01
#define STATUS_MOTION_DETECT  0x02
#define STATUS_OBJECT_DETECT  0x04
#define STATUS_ALARM_ACTIVE   0x08
#define STATUS_LOW_BATTERY    0x10

// ===========================================
// CLASS DEFINITIONS
// ===========================================

class LoRaWANManager {
private:
  bool _connected;
  bool _joining;
  unsigned long _lastTransmission;
  unsigned long _lastDutyCycleTime;
  float _dutyCycleAccumulator;
  uint8_t _retryCount;

  // OTAA credentials
  const char* _appEui;
  const char* _appKey;
  const char* _devEui;

  // Downlink callback
  void (*_downlinkCallback)(uint8_t* payload, size_t size, int rssi);

  // CRC16 calculation for packet validation
  uint16_t calculateCRC(const uint8_t* data, size_t length);

  // Exponential backoff for retries
  unsigned long getRetryDelay();

  // Duty cycle check
  bool canTransmit();
  void recordTransmission(uint32_t airtimeMs);

public:
  LoRaWANManager();

  // Initialization
  bool begin(const char* appEui, const char* appKey, const char* devEui = nullptr);

  // Connection management
  bool connect();
  void disconnect();
  bool isConnected() const { return _connected; }
  bool isConnecting() const { return _joining; }

  // Data transmission
  bool transmit(const LoRaWANPacket& packet);
  bool transmit(uint8_t* payload, size_t size);

  // Downlink handling
  void setDownlinkCallback(void (*callback)(uint8_t*, size_t, int));
  void processDownlink();

  // Utility functions
  void update();
  unsigned long getLastTransmission() const { return _lastTransmission; }
  float getDutyCycleUsage() const;
  void resetDutyCycle();

  // Configuration
  void setDataRate(uint8_t dr);
  void setTxPower(int8_t power);
};

// ===========================================
// IMPLEMENTATION
// ===========================================

LoRaWANManager::LoRaWANManager()
  : _connected(false)
  , _joining(false)
  , _lastTransmission(0)
  , _lastDutyCycleTime(0)
  , _dutyCycleAccumulator(0.0f)
  , _retryCount(0)
  , _appEui(nullptr)
  , _appKey(nullptr)
  , _devEui(nullptr)
  , _downlinkCallback(nullptr)
{
}

bool LoRaWANManager::begin(const char* appEui, const char* appKey, const char* devEui) {
  _appEui = appEui;
  _appKey = appKey;
  _devEui = devEui;

  // Initialize LoRaWAN module
#ifdef REGION
  if (!LoRaWAN.begin(REGION)) {
    return false;
  }
#else
  if (!LoRaWAN.begin()) {
    return false;
  }
#endif

  // Set data rate and TX power
  setDataRate(DATA_RATE);
  setTxPower(TX_POWER);

  // Enable adaptive data rate (ADR)
  LoRaWAN.setAdaptiveDataRate(true);

  return true;
}

bool LoRaWANManager::connect() {
  if (_connected || _joining) {
    return _connected;
  }

  _joining = true;

  // Join network using OTAA
  LoRaWAN.joinOTAA(_appEui, _appKey, _devEui);

  // Wait for connection with timeout
  unsigned long timeout = millis() + 60000;  // 60 second timeout
  while (!LoRaWAN.connected() && millis() < timeout) {
    delay(100);
  }

  _joining = false;
  _connected = LoRaWAN.connected();

  if (_connected) {
    Serial.println("LoRaWAN connected successfully");
  } else {
    Serial.println("LoRaWAN connection timeout");
  }

  return _connected;
}

void LoRaWANManager::disconnect() {
  _connected = false;
  LoRaWAN.disconnect();
}

void LoRaWANManager::setDataRate(uint8_t dr) {
  LoRaWAN.setDataRate(dr);
}

void LoRaWANManager::setTxPower(int8_t power) {
  LoRaWAN.setTxPower(power);
}

bool LoRaWANManager::transmit(const LoRaWANPacket& packet) {
  return transmit((uint8_t*)&packet, sizeof(LoRaWANPacket));
}

bool LoRaWANManager::transmit(uint8_t* payload, size_t size) {
  if (!_connected) {
    Serial.println("Cannot transmit: Not connected");
    return false;
  }

  // Check duty cycle
  if (!canTransmit()) {
    Serial.println("Cannot transmit: Duty cycle limit reached");
    return false;
  }

  // Calculate airtime (simplified calculation)
  // Actual airtime depends on data rate, payload size, and region
  uint32_t airtimeMs = (size * 100) / (DATA_RATE + 1);  // Rough estimate

  // Attempt transmission with retries
  _retryCount = 0;
  bool success = false;

  while (_retryCount < MAX_RETRIES && !success) {
    if (_retryCount > 0) {
      unsigned long retryDelay = getRetryDelay();
      Serial.print("Retrying in ");
      Serial.print(retryDelay);
      Serial.println("ms");
      delay(retryDelay);
    }

    // Send payload
    LoRaWAN.sendPayload(LORAWAN_PORT, payload, size);

    // Wait for confirmation
    unsigned long txTimeout = millis() + 5000;
    while (!LoRaWAN.txComplete() && millis() < txTimeout) {
      delay(10);
    }

    success = LoRaWAN.txComplete();

    if (!success) {
      _retryCount++;
      Serial.print("Transmission failed, retry ");
      Serial.println(_retryCount);
    }
  }

  if (success) {
    _lastTransmission = millis();
    recordTransmission(airtimeMs);
    _retryCount = 0;
    Serial.println("Transmission successful");
  } else {
    Serial.println("Transmission failed after all retries");
  }

  return success;
}

void LoRaWANManager::setDownlinkCallback(void (*callback)(uint8_t*, size_t, int)) {
  _downlinkCallback = callback;
}

void LoRaWANManager::processDownlink() {
  if (_downlinkCallback && LoRaWAN.rxAvailable()) {
    uint8_t payload[256];
    size_t size = LoRaWAN.parsePacket();
    int rssi = LoRaWAN.rxSignalStrength();

    if (size > 0 && size <= sizeof(payload)) {
      LoRaWAN.readPayload(payload, size);
      _downlinkCallback(payload, size, rssi);
    }
  }
}

void LoRaWANManager::update() {
  // Process downlink messages
  processDownlink();

  // Maintain connection
  if (!_connected && !_joining) {
    connect();
  }
}

float LoRaWANManager::getDutyCycleUsage() const {
  unsigned long currentTime = millis();
  unsigned long windowStart = currentTime - DUTY_CYCLE_WINDOW;

  // Calculate duty cycle usage in the current window
  float usage = _dutyCycleAccumulator;

  // Reset if window has passed
  if (currentTime - _lastDutyCycleTime > DUTY_CYCLE_WINDOW) {
    const_cast<LoRaWANManager*>(this)->_dutyCycleAccumulator = 0;
    usage = 0;
  }

  return usage;
}

void LoRaWANManager::resetDutyCycle() {
  _dutyCycleAccumulator = 0;
  _lastDutyCycleTime = millis();
}

bool LoRaWANManager::canTransmit() {
  return getDutyCycleUsage() < DUTY_CYCLE_LIMIT;
}

void LoRaWANManager::recordTransmission(uint32_t airtimeMs) {
  _dutyCycleAccumulator += (float)airtimeMs / DUTY_CYCLE_WINDOW * 100.0f;
  _lastDutyCycleTime = millis();
}

unsigned long LoRaWANManager::getRetryDelay() {
  // Exponential backoff: delay = initial * (2 ^ retryCount)
  unsigned long delay = RETRY_DELAY_INITIAL * (1UL << _retryCount);
  return min(delay, RETRY_DELAY_MAX);
}

uint16_t LoRaWANManager::calculateCRC(const uint8_t* data, size_t length) {
  uint16_t crc = 0xFFFF;

  for (size_t i = 0; i < length; i++) {
    crc ^= data[i];
    for (int j = 0; j < 8; j++) {
      if (crc & 0x0001) {
        crc = (crc >> 1) ^ 0xA001;
      } else {
        crc >>= 1;
      }
    }
  }

  return crc;
}

// ===========================================
// UTILITY FUNCTIONS
// ===========================================

// Create a LoRaWAN packet from sensor data
LoRaWANPacket createLoRaWANPacket(
  float temperature,
  float humidity,
  float pressure,
  float gasResistance,
  uint8_t status,
  uint8_t battery
) {
  LoRaWANPacket packet;

  packet.header = 0xA5A5;  // Magic number
  packet.timestamp = millis() / 1000;
  packet.temperature = (int16_t)(temperature * 100);
  packet.humidity = (uint16_t)(humidity * 100);
  packet.pressure = (uint16_t)(pressure * 10);
  packet.gasResistance = (uint16_t)gasResistance;
  packet.status = status;
  packet.battery = battery;

  // Calculate checksum
  LoRaWANManager manager;
  packet.checksum = manager.calculateCRC(
    (uint8_t*)&packet,
    sizeof(LoRaWANPacket) - sizeof(uint16_t)
  );

  return packet;
}

#endif // LORAWAN_IMPLEMENTATION_H
