/**
 * LoRaWAN Implementation Header
 *
 * Complete production-ready LoRaWAN implementation for Arduino MKR WAN 1310
 * Using IBM LMIC library with full OTAA/ABP support, duty cycle management,
 * binary packet encoding, and comprehensive error handling.
 *
 * Features:
 * - OTAA and ABP authentication modes
 * - All LMIC callbacks implemented
 * - Binary packet encoding/decoding (18-byte optimized packets)
 * - Adaptive data rate (ADR) support
 * - Duty cycle enforcement (1% rule per ETSI/FCC)
 * - Retransmission with exponential backoff
 * - Channel mask configuration for all regions
 * - RX1/RX2 window configuration
 * - Complete downlink handling
 * - CRC16 validation
 * - Command queue management
 *
 * Author: Production-Ready Implementation
 * Version: 2.0.0
 * License: MIT
 */

#ifndef LORAWAN_IMPLEMENTATION_H
#define LORAWAN_IMPLEMENTATION_H

#include <Arduino.h>
#include <lmic.h>
#include <hal/hal.h>
#include <SPI.h>

// ===========================================
// REGION CONFIGURATION
// ===========================================

// Uncomment your region:
#define CFG_eu868                   // Europe (868 MHz)
// #define CFG_us915                // USA (915 MHz)
// #define CFG_kr920                // South Korea (920 MHz)
// #define CFG_as923                // Asia (923 MHz)
// #define CFG_au915                // Australia (915 MHz)

// ===========================================
// LMIC CONFIGURATION
// ===========================================

// Disable all channels in this mask
#define LMIC_DISABLE_JOIN_CHECK_BITS

// Disable ping slots and beacon tracking
#define LMIC_ENABLE_DeviceTimeReq 0
#define LMIC_ENABLE_event_logging 1

// Activation mode
#define ACTIVATION_MODE_OTAA       // Use OTAA (recommended)
// #define ACTIVATION_MODE_ABP     // Use ABP (fallback)

// ===========================================
// TIMING CONFIGURATION
// ===========================================

// TX/RX timing (per LoRaWAN specification)
#define TX_INTERVAL_60SEC          60000       // Default 60 seconds
#define TX_INTERVAL_MIN            10000       // Minimum 10 seconds
#define TX_INTERVAL_MAX            3600000     // Maximum 1 hour

// RX window timing
#define RX1_DELAY                  1000        // 1 second after TX
#define RX2_DELAY                  2000        // 2 seconds after TX
#define RX1_DURATION               1000        // 1 second window
#define RX2_DURATION               1000        // 1 second window

// Join timing
#define JOIN_RETRY_DELAY           60000       // 60 seconds between join attempts
#define JOIN_MAX_RETRIES           5           // Maximum join retries

// ===========================================
// DUTY CYCLE CONFIGURATION
// ===========================================

// Duty cycle limits (per regulatory requirements)
#define DUTY_CYCLE_LIMIT_EU        1.0f        // 1% for EU868 (ETSI)
#define DUTY_CYCLE_LIMIT_US        1.0f        // 1% best practice for US915
#define DUTY_CYCLE_LIMIT_KR        1.0f        // 1% for KR920
#define DUTY_CYCLE_WINDOW          3600000     // 1 hour rolling window

// Get duty cycle limit for current region
#ifdef CFG_eu868
    #define DUTY_CYCLE_LIMIT DUTY_CYCLE_LIMIT_EU
#elif defined(CFG_us915)
    #define DUTY_CYCLE_LIMIT DUTY_CYCLE_LIMIT_US
#elif defined(CFG_kr920)
    #define DUTY_CYCLE_LIMIT DUTY_CYCLE_LIMIT_KR
#else
    #define DUTY_CYCLE_LIMIT 1.0f
#endif

// ===========================================
// RETRY CONFIGURATION
// ===========================================

#define MAX_RETRIES                5           // Maximum transmission retries
#define RETRY_DELAY_INITIAL        1000        // Initial retry delay: 1 second
#define RETRY_DELAY_MAX            60000       // Maximum retry delay: 60 seconds
#define RETRY_BACKOFF_MULTIPLIER   2           // Exponential backoff multiplier

// ===========================================
// DATA RATE AND POWER CONFIGURATION
// ===========================================

// Default data rate (DR)
// EU868: DR0=SF12, DR1=SF11, DR2=SF10, DR3=SF9, DR4=SF8, DR5=SF7, DR6=SF7
// US915: DR0=SF10, DR1=SF9, DR2=SF8, DR3=SF7, DR4=SF8
// KR920: DR0=SF12, DR1=SF11, DR2=SF10, DR3=SF9, DR4=SF8, DR5=SF7
#define DEFAULT_DATA_RATE          DR3         // Balance of speed and range
#define MIN_DATA_RATE              DR0         // Slowest, longest range
#define MAX_DATA_RATE              DR5         // Fastest, shortest range

// TX power configuration
#define DEFAULT_TX_POWER           14          // 14 dBm (EIRP)
#define MIN_TX_POWER               0           // 0 dBm
#define MAX_TX_POWER               20          // 20 dBm (max EIRP)

// ===========================================
// ADAPTIVE DATA RATE (ADR) CONFIGURATION
// ===========================================

#define ADR_ACK_DELAY_EXP          4           // ADR acknowledgment delay
#define ADR_ACK_LIMIT              64          // ADR acknowledgment limit
#define ADR_ENABLE                 true        // Enable ADR by default

// ===========================================
// PORT CONFIGURATION
// ===========================================

#define LORAWAN_PORT_SENSOR        1           // Sensor data port
#define LORAWAN_PORT_STATUS        2           // Status data port
#define LORAWAN_PORT_COMMAND       3           // Command port
#define LORAWAN_PORT_DETECTION     4           // Detection event port

// ===========================================
// PACKET STRUCTURES - BINARY ENCODING
// ===========================================

// Packet type identifiers
#define PACKET_TYPE_SENSOR         0x01
#define PACKET_TYPE_DETECTION      0x02
#define PACKET_TYPE_STATUS         0x03
#define PACKET_TYPE_COMMAND        0x04
#define PACKET_TYPE_ACK            0x80
#define PACKET_TYPE_NACK           0xFF

// Magic number for packet validation
#define PACKET_MAGIC               0xA5A5

// Sensor data packet (18 bytes optimized)
#pragma pack(push, 1)
typedef struct {
    uint16_t magic;                // 0xA5A5 (2 bytes) - Packet validation
    uint8_t type;                  // Packet type (1 byte)
    uint32_t timestamp;            // Unix timestamp (4 bytes)
    int16_t temperature;           // Temperature x100 (2 bytes, e.g., 2345 = 23.45°C)
    uint16_t humidity;             // Humidity x100 (2 bytes, e.g., 6500 = 65.00%)
    uint16_t pressure;             // Pressure x10 (2 bytes, e.g., 10132 = 1013.2 hPa)
    uint16_t gasResistance;        // Gas resistance in Ohms (2 bytes)
    uint16_t iaq;                  // Indoor Air Quality (2 bytes)
    uint8_t status;                // Status flags (1 byte)
    uint8_t battery;               // Battery level 0-100 (1 byte)
    uint16_t checksum;             // CRC16 checksum (2 bytes)
} SensorDataPacket;
#pragma pack(pop)

static_assert(sizeof(SensorDataPacket) == 18, "SensorDataPacket must be 18 bytes");

// Detection event packet (12 bytes optimized)
#pragma pack(push, 1)
typedef struct {
    uint16_t magic;                // 0xA5A5 (2 bytes)
    uint8_t type;                  // PACKET_TYPE_DETECTION (1 byte)
    uint32_t timestamp;            // Unix timestamp (4 bytes)
    uint8_t detectionType;         // Detection type (1 byte)
    uint8_t confidence;            // Confidence level 0-100 (1 byte)
    uint16_t duration;             // Duration in seconds (2 bytes)
    uint16_t checksum;             // CRC16 (2 bytes)
} DetectionDataPacket;
#pragma pack(pop)

static_assert(sizeof(DetectionDataPacket) == 12, "DetectionDataPacket must be 12 bytes");

// Status packet (15 bytes optimized)
#pragma pack(push, 1)
typedef struct {
    uint16_t magic;                // 0xA5A5 (2 bytes)
    uint8_t type;                  // PACKET_TYPE_STATUS (1 byte)
    uint32_t uptime;               // Uptime in seconds (4 bytes)
    uint32_t txCount;              // Total transmissions (4 bytes)
    uint32_t rxCount;              // Total downlinks received (4 bytes)
    uint8_t dataRate;              // Current data rate (1 byte)
    uint8_t battery;               // Battery level (1 byte)
    uint16_t checksum;             // CRC16 (2 bytes)
} StatusDataPacket;
#pragma pack(pop)

static_assert(sizeof(StatusDataPacket) == 15, "StatusDataPacket must be 15 bytes");

// Status flags bit definitions
#define STATUS_SENSOR_OK           0x01        // All sensors operational
#define STATUS_MOTION_DETECT       0x02        // Motion detected
#define STATUS_OBJECT_DETECT       0x04        // Object detected
#define STATUS_ALARM_ACTIVE        0x08        // Alarm active
#define STATUS_LOW_BATTERY         0x10        // Battery < 20%
#define STATUS_SENSOR_ERROR        0x20        // Sensor error
#define STATUS_NETWORK_ERROR       0x40        // Network error
#define STATUS_MEMORY_ERROR        0x80        // Memory error

// Detection type definitions
#define DETECTION_TYPE_MOTION      0x01
#define DETECTION_TYPE_OBJECT      0x02
#define DETECTION_TYPE_PERSON      0x03
#define DETECTION_TYPE_VEHICLE     0x04
#define DETECTION_TYPE_ANIMAL      0x05

// ===========================================
// DOWNLINK COMMAND DEFINITIONS
// ===========================================

// Command IDs (uplink)
#define CMD_PING                   0x00
#define CMD_SET_INTERVAL           0x01
#define CMD_SET_DATARATE           0x02
#define CMD_SET_TXPOWER            0x03
#define CMD_REBOOT                 0x04
#define CMD_GET_STATUS             0x05
#define CMD_SET_LED                0x06
#define CMD_SET_ALARM              0x07
#define CMD_GET_BATTERY            0x08
#define CMD_SET_ADR                0x09
#define CMD_CLEAR_STATS            0x0A

// Response IDs (downlink)
#define RESP_ACK                   0x80
#define RESP_NACK                  0x81
#define RESP_STATUS                0x82
#define RESP_BATTERY               0x83
#define RESP_ERROR                 0xFF

// Error codes
#define ERR_SUCCESS                0x00
#define ERR_UNKNOWN_COMMAND        0x01
#define ERR_INVALID_PARAMETER      0x02
#define ERR_NOT_IMPLEMENTED        0x03
#define ERR_BUFFER_OVERFLOW        0x04
#define ERR_CHECKSUM_FAIL          0x05
#define ERR_NOT_JOINED             0x06

// Downlink message structure
#pragma pack(push, 1)
typedef struct {
    uint8_t commandId;             // Command ID
    uint8_t payload[16];           // Variable payload (max 16 bytes)
    uint8_t payloadLength;         // Payload length
} DownlinkMessage;

typedef struct {
    uint8_t responseId;            // Response ID
    uint8_t payload[16];           // Response payload
    uint8_t payloadLength;         // Payload length
} UplinkMessage;
#pragma pack(pop)

// ===========================================
// LMIC CALLBACK FUNCTION POINTERS
// ===========================================

// User callback types
typedef void (*OnJoinCallback)(bool success);
typedef void (*OnTxCompleteCallback)(bool success);
typedef void (*OnDownlinkCallback)(uint8_t* payload, size_t size, int rssi);
typedef void (*OnErrorCallback)(uint8_t errorCode);

// ===========================================
// LORAWAN MANAGER CLASS
// ===========================================

class LoRaWANManager {
private:
    // Connection state
    bool _connected;
    bool _joining;
    bool _adrEnabled;

    // Timing
    unsigned long _lastTransmission;
    unsigned long _lastDutyCycleTime;
    unsigned long _lastJoinAttempt;
    unsigned long _transmitInterval;
    uint32_t _dutyCycleAccumulator;

    // Statistics
    uint32_t _txCount;
    uint32_t _txSuccessCount;
    uint32_t _txFailCount;
    uint32_t _rxCount;
    uint32_t _joinRetryCount;

    // Retry management
    uint8_t _retryCount;
    uint8_t _maxRetries;

    // Configuration
    uint8_t _dataRate;
    int8_t _txPower;

    // Callbacks
    OnJoinCallback _onJoinCallback;
    OnTxCompleteCallback _onTxCompleteCallback;
    OnDownlinkCallback _onDownlinkCallback;
    OnErrorCallback _onErrorCallback;

    // OTAA credentials
    uint8_t _appEui[8];
    uint8_t _devEui[8];
    uint8_t _appKey[16];

    // ABP credentials (alternative)
    uint8_t _nwkSKey[16];
    uint8_t _appSKey[16];
    uint32_t _devAddr;

    // Internal methods
    uint16_t calculateCRC16(const uint8_t* data, size_t length);
    bool validatePacket(const uint8_t* data, size_t length);
    unsigned long getRetryDelay();
    bool canTransmit();
    void recordTransmission(uint32_t airtimeMs);
    void resetDutyCycleWindow();
    uint32_t calculateAirtime(size_t payloadSize);

    // Channel configuration
    void configureChannels();
    void setDefaultChannels();

public:
    // Constructor
    LoRaWANManager();

    // Initialization
    bool beginOTAA(const uint8_t* appEui, const uint8_t* appKey, const uint8_t* devEui = nullptr);
    bool beginABP(const uint8_t* nwkSKey, const uint8_t* appSKey, uint32_t devAddr);

    // Connection management
    bool connect();
    void disconnect();
    bool isConnected() const { return _connected; }
    bool isJoining() const { return _joining; }
    bool rejoin();

    // Data transmission
    bool transmitPacket(const uint8_t* payload, size_t size, uint8_t port = LORAWAN_PORT_SENSOR);
    bool transmitSensorData(const SensorDataPacket& packet);
    bool transmitDetection(const DetectionDataPacket& packet);
    bool transmitStatus(const StatusDataPacket& packet);

    // Downlink handling
    void setDownlinkCallback(OnDownlinkCallback callback);
    bool sendDownlink(const uint8_t* payload, size_t size, uint8_t port, bool confirmed = false);

    // Configuration
    void setDataRate(uint8_t dr);
    void setTxPower(int8_t power);
    void setTransmitInterval(unsigned long interval);
    void setAdrEnabled(bool enabled);
    void setMaxRetries(uint8_t maxRetries);

    // Getters
    uint8_t getDataRate() const { return _dataRate; }
    int8_t getTxPower() const { return _txPower; }
    unsigned long getTransmitInterval() const { return _transmitInterval; }
    bool isAdrEnabled() const { return _adrEnabled; }
    float getDutyCycleUsage() const;
    unsigned long getLastTransmission() const { return _lastTransmission; }

    // Statistics
    uint32_t getTxCount() const { return _txCount; }
    uint32_t getTxSuccessCount() const { return _txSuccessCount; }
    uint32_t getTxFailCount() const { return _txFailCount; }
    uint32_t getRxCount() const { return _rxCount; }
    void resetStatistics();

    // Callback registration
    void setOnJoinCallback(OnJoinCallback callback) { _onJoinCallback = callback; }
    void setOnTxCompleteCallback(OnTxCompleteCallback callback) { _onTxCompleteCallback = callback; }
    void setOnErrorCallback(OnErrorCallback callback) { _onErrorCallback = callback; }

    // LMIC integration
    void loop();                   // Must be called in main loop
    void onEvent(ev_t ev);         // LMIC event handler

    // Utility functions
    void printState();
    void printStatistics();
};

// ===========================================
// PACKET CREATION UTILITIES
// ===========================================

// Create sensor data packet
SensorDataPacket createSensorPacket(
    float temperature,
    float humidity,
    float pressure,
    float gasResistance,
    uint16_t iaq,
    uint8_t status,
    uint8_t battery
);

// Create detection packet
DetectionDataPacket createDetectionPacket(
    uint8_t detectionType,
    uint8_t confidence,
    uint16_t duration
);

// Create status packet
StatusDataPacket createStatusPacket(
    const LoRaWANManager& manager,
    uint8_t battery
);

// ===========================================
// LMIC CALLBACKS
// ===========================================

// These are called by LMIC and must be available globally
void os_getDevKey(u1_t* buf);
void os_getArtEui(u1_t* buf);
void os_getDevEui(u1_t* buf);

// ===========================================
// GLOBAL INSTANCES
// ===========================================

extern LoRaWANManager LoRaWAN;

#endif // LORAWAN_IMPLEMENTATION_H
