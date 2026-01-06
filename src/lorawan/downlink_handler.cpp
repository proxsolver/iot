/**
 * LoRaWAN Downlink Handler
 *
 * Complete implementation of downlink command handling for LoRaWAN network.
 * Supports remote configuration, control, and status queries.
 *
 * Features:
 * - Binary command parsing
 * - Command queue management
 * - Error handling and validation
 * - Response generation
 * - System configuration management
 *
 * Author: Production-Ready Implementation
 * Version: 2.0.0
 * License: MIT
 */

#include <Arduino.h>
#include "lorawan_implementation.h"

// ===========================================
// SYSTEM CONFIGURATION STORAGE
// ===========================================

/**
 * @brief System configuration structure
 *
 * Stores all modifiable system parameters that can be changed via downlink
 */
struct SystemConfig {
    uint32_t transmitInterval;      // Transmission interval (milliseconds)
    uint8_t dataRate;               // Data rate (DR0-DR5)
    int8_t txPower;                 // TX power (dBm)
    uint8_t ledEnabled;             // LED state (0=off, 1=on)
    uint8_t alarmEnabled;           // Alarm state (0=off, 1=on)
    uint8_t adrEnabled;             // ADR state (0=off, 1=on)
    uint8_t debugEnabled;           // Debug mode (0=off, 1=on)
    uint8_t motionDetectionEnabled; // Motion detection (0=off, 1=on)
    uint8_t objectDetectionEnabled; // Object detection (0=off, 1=on)
    uint16_t motionThreshold;       // Motion threshold
    uint16_t objectThreshold;       // Object detection threshold
} systemConfig;

/**
 * @brief Initialize system configuration with default values
 */
void initSystemConfig() {
    systemConfig.transmitInterval = TX_INTERVAL_60SEC;
    systemConfig.dataRate = DEFAULT_DATA_RATE;
    systemConfig.txPower = DEFAULT_TX_POWER;
    systemConfig.ledEnabled = 1;
    systemConfig.alarmEnabled = 0;
    systemConfig.adrEnabled = ADR_ENABLE ? 1 : 0;
    systemConfig.debugEnabled = 0;
    systemConfig.motionDetectionEnabled = 1;
    systemConfig.objectDetectionEnabled = 1;
    systemConfig.motionThreshold = 100;
    systemConfig.objectThreshold = 70;
}

// ===========================================
// COMMAND QUEUE MANAGEMENT
// ===========================================

#define COMMAND_QUEUE_SIZE 10

/**
 * @brief Command queue entry
 */
struct CommandQueueEntry {
    uint8_t commandId;
    uint8_t payload[16];
    uint8_t payloadLength;
    unsigned long timestamp;
};

CommandQueueEntry commandQueue[COMMAND_QUEUE_SIZE];
uint8_t commandQueueHead = 0;
uint8_t commandQueueTail = 0;
bool commandQueueOverflow = false;

/**
 * @brief Initialize command queue
 */
void initCommandQueue() {
    commandQueueHead = 0;
    commandQueueTail = 0;
    commandQueueOverflow = false;
    memset(commandQueue, 0, sizeof(commandQueue));
}

/**
 * @brief Add command to queue
 * @param commandId Command ID
 * @param payload Payload data
 * @param payloadLength Payload length
 * @return true if command added successfully
 */
bool enqueueCommand(uint8_t commandId, const uint8_t* payload, uint8_t payloadLength) {
    uint8_t nextTail = (commandQueueTail + 1) % COMMAND_QUEUE_SIZE;

    if (nextTail == commandQueueHead) {
        // Queue is full
        commandQueueOverflow = true;
        return false;
    }

    commandQueue[commandQueueTail].commandId = commandId;
    commandQueue[commandQueueTail].payloadLength = payloadLength;
    commandQueue[commandQueueTail].timestamp = millis();

    if (payloadLength > 0 && payloadLength <= 16) {
        memcpy(commandQueue[commandQueueTail].payload, payload, payloadLength);
    }

    commandQueueTail = nextTail;
    return true;
}

/**
 * @brief Get next command from queue
 * @param entry Command entry (output)
 * @return true if command retrieved successfully
 */
bool dequeueCommand(CommandQueueEntry* entry) {
    if (commandQueueHead == commandQueueTail) {
        // Queue is empty
        return false;
    }

    *entry = commandQueue[commandQueueHead];
    commandQueueHead = (commandQueueHead + 1) % COMMAND_QUEUE_SIZE;
    return true;
}

/**
 * @brief Process command queue
 */
void processCommandQueue() {
    CommandQueueEntry entry;

    while (dequeueCommand(&entry)) {
        executeCommand(entry.commandId, entry.payload, entry.payloadLength);
    }
}

// ===========================================
// DOWNLINK MESSAGE HANDLER
// ===========================================

/**
 * @brief Handle incoming downlink message
 * @param payload Payload data
 * @param size Payload size
 * @param rssi Signal strength (dBm)
 */
void handleDownlink(uint8_t* payload, size_t size, int rssi) {
    Serial.println(F(""));
    Serial.println(F("========== DOWNLINK RECEIVED =========="));
    Serial.print(F("Size: "));
    Serial.print(size);
    Serial.println(F(" bytes"));
    Serial.print(F("RSSI: "));
    Serial.print(rssi);
    Serial.println(F(" dBm"));

    // Validate minimum size
    if (size < 1) {
        Serial.println(F("ERROR: Invalid payload size"));
        sendErrorResponse(ERR_INVALID_PARAMETER);
        return;
    }

    // Print payload in hex
    Serial.print(F("Payload: "));
    for (size_t i = 0; i < size; i++) {
        if (payload[i] < 0x10) Serial.print(F("0"));
        Serial.print(payload[i], HEX);
        Serial.print(F(" "));
    }
    Serial.println(F(""));

    // Parse command
    uint8_t commandId = payload[0];

    // Validate command ID
    if (!isValidCommand(commandId)) {
        Serial.print(F("ERROR: Unknown command: 0x"));
        Serial.println(commandId, HEX);
        sendErrorResponse(ERR_UNKNOWN_COMMAND);
        return;
    }

    // Extract payload (if any)
    const uint8_t* commandPayload = nullptr;
    uint8_t commandPayloadLength = 0;

    if (size > 1) {
        commandPayload = payload + 1;
        commandPayloadLength = size - 1;
    }

    // Execute command
    if (systemConfig.debugEnabled) {
        Serial.println(F("Executing command..."));
    }

    executeCommand(commandId, commandPayload, commandPayloadLength);

    Serial.println(F("======================================"));
    Serial.println(F(""));
}

/**
 * @brief Validate command ID
 * @param commandId Command ID
 * @return true if command is valid
 */
bool isValidCommand(uint8_t commandId) {
    switch (commandId) {
        case CMD_PING:
        case CMD_SET_INTERVAL:
        case CMD_SET_DATARATE:
        case CMD_SET_TXPOWER:
        case CMD_REBOOT:
        case CMD_GET_STATUS:
        case CMD_SET_LED:
        case CMD_SET_ALARM:
        case CMD_GET_BATTERY:
        case CMD_SET_ADR:
        case CMD_CLEAR_STATS:
            return true;
        default:
            return false;
    }
}

/**
 * @brief Execute command
 * @param commandId Command ID
 * @param payload Payload data
 * @param payloadLength Payload length
 */
void executeCommand(uint8_t commandId, const uint8_t* payload, uint8_t payloadLength) {
    switch (commandId) {
        case CMD_PING:
            handlePing();
            break;

        case CMD_SET_INTERVAL:
            handleSetInterval(payload, payloadLength);
            break;

        case CMD_SET_DATARATE:
            handleSetDataRate(payload, payloadLength);
            break;

        case CMD_SET_TXPOWER:
            handleSetTxPower(payload, payloadLength);
            break;

        case CMD_REBOOT:
            handleReboot();
            break;

        case CMD_GET_STATUS:
            handleGetStatus();
            break;

        case CMD_SET_LED:
            handleSetLED(payload, payloadLength);
            break;

        case CMD_SET_ALARM:
            handleSetAlarm(payload, payloadLength);
            break;

        case CMD_GET_BATTERY:
            handleGetBattery();
            break;

        case CMD_SET_ADR:
            handleSetADR(payload, payloadLength);
            break;

        case CMD_CLEAR_STATS:
            handleClearStats();
            break;

        default:
            Serial.print(F("ERROR: Unhandled command: 0x"));
            Serial.println(commandId, HEX);
            sendErrorResponse(ERR_NOT_IMPLEMENTED);
            break;
    }
}

// ===========================================
// COMMAND HANDLERS
// ===========================================

/**
 * @brief Handle PING command
 *
 * Request: [0x00]
 * Response: [0x80] (ACK)
 */
void handlePing() {
    Serial.println(F("Command: PING"));

    sendAck();
}

/**
 * @brief Handle SET_INTERVAL command
 *
 * Request: [0x01, interval_lsb, interval, interval, interval_msb]
 *         interval: 4 bytes, milliseconds (little-endian)
 * Response: [0x80] (ACK)
 */
void handleSetInterval(const uint8_t* payload, uint8_t payloadLength) {
    Serial.println(F("Command: SET_INTERVAL"));

    // Validate payload length
    if (payloadLength != 4) {
        Serial.println(F("ERROR: Invalid payload length"));
        sendErrorResponse(ERR_INVALID_PARAMETER);
        return;
    }

    // Extract interval (little-endian)
    uint32_t interval = *((uint32_t*)payload);

    Serial.print(F("  Interval: "));
    Serial.print(interval / 1000);
    Serial.println(F(" seconds"));

    // Validate interval (min 10 seconds, max 1 hour)
    if (interval < TX_INTERVAL_MIN || interval > TX_INTERVAL_MAX) {
        Serial.print(F("ERROR: Interval out of range ["));
        Serial.print(TX_INTERVAL_MIN / 1000);
        Serial.print(F("-"));
        Serial.print(TX_INTERVAL_MAX / 1000);
        Serial.println(F("] seconds"));
        sendErrorResponse(ERR_INVALID_PARAMETER);
        return;
    }

    // Update system configuration
    systemConfig.transmitInterval = interval;
    LoRaWAN.setTransmitInterval(interval);

    Serial.println(F("  Interval updated successfully"));
    sendAck();
}

/**
 * @brief Handle SET_DATARATE command
 *
 * Request: [0x02, dr]
 *         dr: 1 byte, data rate (0-5)
 * Response: [0x80] (ACK)
 */
void handleSetDataRate(const uint8_t* payload, uint8_t payloadLength) {
    Serial.println(F("Command: SET_DATARATE"));

    // Validate payload length
    if (payloadLength != 1) {
        Serial.println(F("ERROR: Invalid payload length"));
        sendErrorResponse(ERR_INVALID_PARAMETER);
        return;
    }

    uint8_t dataRate = payload[0];

    Serial.print(F("  Data Rate: DR"));
    Serial.println(dataRate);

    // Validate data rate
    if (dataRate < MIN_DATA_RATE || dataRate > MAX_DATA_RATE) {
        Serial.print(F("ERROR: Data rate out of range [DR"));
        Serial.print(MIN_DATA_RATE);
        Serial.print(F("-DR"));
        Serial.print(MAX_DATA_RATE);
        Serial.println(F("]"));
        sendErrorResponse(ERR_INVALID_PARAMETER);
        return;
    }

    // Update system configuration
    systemConfig.dataRate = dataRate;
    LoRaWAN.setDataRate(dataRate);

    Serial.println(F("  Data rate updated successfully"));
    sendAck();
}

/**
 * @brief Handle SET_TXPOWER command
 *
 * Request: [0x03, power]
 *         power: 1 byte, TX power in dBm (0-20)
 * Response: [0x80] (ACK)
 */
void handleSetTxPower(const uint8_t* payload, uint8_t payloadLength) {
    Serial.println(F("Command: SET_TXPOWER"));

    // Validate payload length
    if (payloadLength != 1) {
        Serial.println(F("ERROR: Invalid payload length"));
        sendErrorResponse(ERR_INVALID_PARAMETER);
        return;
    }

    int8_t txPower = (int8_t)payload[0];

    Serial.print(F("  TX Power: "));
    Serial.print(txPower);
    Serial.println(F(" dBm"));

    // Validate TX power
    if (txPower < MIN_TX_POWER || txPower > MAX_TX_POWER) {
        Serial.print(F("ERROR: TX power out of range ["));
        Serial.print(MIN_TX_POWER);
        Serial.print(F("-"));
        Serial.print(MAX_TX_POWER);
        Serial.println(F("] dBm"));
        sendErrorResponse(ERR_INVALID_PARAMETER);
        return;
    }

    // Update system configuration
    systemConfig.txPower = txPower;
    LoRaWAN.setTxPower(txPower);

    Serial.println(F("  TX power updated successfully"));
    sendAck();
}

/**
 * @brief Handle REBOOT command
 *
 * Request: [0x04]
 * Response: [0x80] (ACK)
 * Note: Device reboots after sending ACK
 */
void handleReboot() {
    Serial.println(F("Command: REBOOT"));

    // Send ACK first
    sendAck();

    // Delay to allow transmission
    delay(1000);

    // Reboot
    Serial.println(F("Rebooting..."));

    NVIC_SystemReset();
}

/**
 * @brief Handle GET_STATUS command
 *
 * Request: [0x05]
 * Response: [0x82, interval_lsb, ..., interval_msb, dr, power, adr, tx_count_lsb, ..., tx_count_msb, rx_count_lsb, ..., rx_count_msb]
 */
void handleGetStatus() {
    Serial.println(F("Command: GET_STATUS"));

    sendStatus();
}

/**
 * @brief Handle SET_LED command
 *
 * Request: [0x06, enabled]
 *         enabled: 1 byte (0=off, 1=on)
 * Response: [0x80] (ACK)
 */
void handleSetLED(const uint8_t* payload, uint8_t payloadLength) {
    Serial.println(F("Command: SET_LED"));

    // Validate payload length
    if (payloadLength != 1) {
        Serial.println(F("ERROR: Invalid payload length"));
        sendErrorResponse(ERR_INVALID_PARAMETER);
        return;
    }

    uint8_t enabled = payload[0];

    Serial.print(F("  LED: "));
    Serial.println(enabled ? F("ON") : F("OFF"));

    // Update system configuration
    systemConfig.ledEnabled = enabled;
    digitalWrite(LED_BUILTIN, enabled ? HIGH : LOW);

    Serial.println(F("  LED updated successfully"));
    sendAck();
}

/**
 * @brief Handle SET_ALARM command
 *
 * Request: [0x07, enabled]
 *         enabled: 1 byte (0=off, 1=on)
 * Response: [0x80] (ACK)
 */
void handleSetAlarm(const uint8_t* payload, uint8_t payloadLength) {
    Serial.println(F("Command: SET_ALARM"));

    // Validate payload length
    if (payloadLength != 1) {
        Serial.println(F("ERROR: Invalid payload length"));
        sendErrorResponse(ERR_INVALID_PARAMETER);
        return;
    }

    uint8_t enabled = payload[0];

    Serial.print(F("  Alarm: "));
    Serial.println(enabled ? F("ON") : F("OFF")));

    // Update system configuration
    systemConfig.alarmEnabled = enabled;

    Serial.println(F("  Alarm updated successfully"));
    sendAck();
}

/**
 * @brief Handle GET_BATTERY command
 *
 * Request: [0x08]
 * Response: [0x83, percent, voltage]
 *          percent: 1 byte (0-100)
 *          voltage: 1 byte (voltage x 10, e.g., 42 = 4.2V)
 */
void handleGetBattery() {
    Serial.println(F("Command: GET_BATTERY"));

    sendBatteryLevel();
}

/**
 * @brief Handle SET_ADR command
 *
 * Request: [0x09, enabled]
 *         enabled: 1 byte (0=off, 1=on)
 * Response: [0x80] (ACK)
 */
void handleSetADR(const uint8_t* payload, uint8_t payloadLength) {
    Serial.println(F("Command: SET_ADR"));

    // Validate payload length
    if (payloadLength != 1) {
        Serial.println(F("ERROR: Invalid payload length"));
        sendErrorResponse(ERR_INVALID_PARAMETER);
        return;
    }

    uint8_t enabled = payload[0];

    Serial.print(F("  ADR: "));
    Serial.println(enabled ? F("Enabled") : F("Disabled")));

    // Update system configuration
    systemConfig.adrEnabled = enabled;
    LoRaWAN.setAdrEnabled(enabled != 0);

    Serial.println(F("  ADR updated successfully"));
    sendAck();
}

/**
 * @brief Handle CLEAR_STATS command
 *
 * Request: [0x0A]
 * Response: [0x80] (ACK)
 */
void handleClearStats() {
    Serial.println(F("Command: CLEAR_STATS"));

    LoRaWAN.resetStatistics();

    Serial.println(F("  Statistics cleared successfully"));
    sendAck();
}

// ===========================================
// RESPONSE SENDERS
// ===========================================

/**
 * @brief Send ACK response
 */
void sendAck() {
    uint8_t response[1];
    response[0] = RESP_ACK;

    LMIC_setTxData2(LORAWAN_PORT_COMMAND, response, sizeof(response), 0);

    Serial.println(F("  Response: ACK sent"));
}

/**
 * @brief Send NACK response
 */
void sendNack() {
    uint8_t response[1];
    response[0] = RESP_NACK;

    LMIC_setTxData2(LORAWAN_PORT_COMMAND, response, sizeof(response), 0);

    Serial.println(F("  Response: NACK sent"));
}

/**
 * @brief Send status response
 */
void sendStatus() {
    uint8_t response[18];

    response[0] = RESP_STATUS;

    // Pack interval (4 bytes, little-endian)
    response[1] = systemConfig.transmitInterval & 0xFF;
    response[2] = (systemConfig.transmitInterval >> 8) & 0xFF;
    response[3] = (systemConfig.transmitInterval >> 16) & 0xFF;
    response[4] = (systemConfig.transmitInterval >> 24) & 0xFF;

    // Pack data rate
    response[5] = systemConfig.dataRate;

    // Pack TX power
    response[6] = (uint8_t)systemConfig.txPower;

    // Pack ADR enabled
    response[7] = systemConfig.adrEnabled;

    // Pack LED state
    response[8] = systemConfig.ledEnabled;

    // Pack alarm state
    response[9] = systemConfig.alarmEnabled;

    // Pack TX count (4 bytes, little-endian)
    uint32_t txCount = LoRaWAN.getTxCount();
    response[10] = txCount & 0xFF;
    response[11] = (txCount >> 8) & 0xFF;
    response[12] = (txCount >> 16) & 0xFF;
    response[13] = (txCount >> 24) & 0xFF;

    // Pack RX count (4 bytes, little-endian)
    uint32_t rxCount = LoRaWAN.getRxCount();
    response[14] = rxCount & 0xFF;
    response[15] = (rxCount >> 8) & 0xFF;
    response[16] = (rxCount >> 16) & 0xFF;
    response[17] = (rxCount >> 24) & 0xFF;

    LMIC_setTxData2(LORAWAN_PORT_COMMAND, response, sizeof(response), 0);

    Serial.println(F("  Response: STATUS sent"));
}

/**
 * @brief Send battery level response
 */
void sendBatteryLevel() {
    uint8_t response[3];

    response[0] = RESP_BATTERY;

    // Read battery voltage
    // MKR WAN 1310: Battery is connected to ADC_BATTERY (ADC0)
    // Voltage divider: 2.0 (actual voltage = ADC * 2.0)
    int batteryRaw = analogRead(ADC_BATTERY);
    float batteryVoltage = (batteryRaw * 3.3f / 1023.0f) * 2.0f;

    // Calculate battery percentage (assuming 3.0V - 4.2V range)
    uint8_t batteryPercent = constrain(
        (batteryVoltage - 3.0f) / (4.2f - 3.0f) * 100,
        0,
        100
    );

    response[1] = batteryPercent;
    response[2] = (uint8_t)(batteryVoltage * 10);

    LMIC_setTxData2(LORAWAN_PORT_COMMAND, response, sizeof(response), 0);

    Serial.print(F("  Battery: "));
    Serial.print(batteryPercent);
    Serial.print(F("% ("));
    Serial.print(batteryVoltage, 2);
    Serial.println(F("V)"));

    Serial.println(F("  Response: BATTERY sent"));
}

/**
 * @brief Send error response
 * @param errorCode Error code
 */
void sendErrorResponse(uint8_t errorCode) {
    uint8_t response[2];

    response[0] = RESP_ERROR;
    response[1] = errorCode;

    LMIC_setTxData2(LORAWAN_PORT_COMMAND, response, sizeof(response), 0);

    Serial.print(F("  Response: ERROR ("));
    Serial.print(errorCode);
    Serial.println(F(") sent"));

    // Print error description
    const char* errorDescription;
    switch (errorCode) {
        case ERR_UNKNOWN_COMMAND:
            errorDescription = "Unknown command";
            break;
        case ERR_INVALID_PARAMETER:
            errorDescription = "Invalid parameter";
            break;
        case ERR_NOT_IMPLEMENTED:
            errorDescription = "Not implemented";
            break;
        case ERR_BUFFER_OVERFLOW:
            errorDescription = "Buffer overflow";
            break;
        case ERR_CHECKSUM_FAIL:
            errorDescription = "Checksum failed";
            break;
        case ERR_NOT_JOINED:
            errorDescription = "Not joined";
            break;
        default:
            errorDescription = "Unknown error";
            break;
    }

    Serial.print(F("  Description: "));
    Serial.println(errorDescription);
}

// ===========================================
// SETUP AND UTILITIES
// ===========================================

/**
 * @brief Setup downlink handler
 */
void setupDownlinkHandler() {
    // Initialize system configuration
    initSystemConfig();

    // Initialize command queue
    initCommandQueue();

    // Set downlink callback
    LoRaWAN.setDownlinkCallback(handleDownlink);

    Serial.println(F("Downlink handler initialized"));
}

/**
 * @brief Process downlink messages
 *
 * This function should be called in the main loop
 */
void processDownlink() {
    // Process command queue
    processCommandQueue();

    // LMIC event processing is handled by LoRaWAN.loop()
}

/**
 * @brief Print system configuration
 */
void printSystemConfig() {
    Serial.println(F("========== SYSTEM CONFIGURATION =========="));
    Serial.print(F("Transmit Interval: "));
    Serial.print(systemConfig.transmitInterval / 1000);
    Serial.println(F(" seconds"));
    Serial.print(F("Data Rate: DR"));
    Serial.println(systemConfig.dataRate);
    Serial.print(F("TX Power: "));
    Serial.print(systemConfig.txPower);
    Serial.println(F(" dBm"));
    Serial.print(F("LED: "));
    Serial.println(systemConfig.ledEnabled ? F("ON") : F("OFF"));
    Serial.print(F("Alarm: "));
    Serial.println(systemConfig.alarmEnabled ? F("ON") : F("OFF"));
    Serial.print(F("ADR: "));
    Serial.println(systemConfig.adrEnabled ? F("Enabled") : F("Disabled"));
    Serial.print(F("Debug: "));
    Serial.println(systemConfig.debugEnabled ? F("Enabled") : F("Disabled"));
    Serial.print(F("Motion Detection: "));
    Serial.println(systemConfig.motionDetectionEnabled ? F("Enabled") : F("Disabled"));
    Serial.print(F("Object Detection: "));
    Serial.println(systemConfig.objectDetectionEnabled ? F("Enabled") : F("Disabled"));
    Serial.println(F("========================================"));
}
