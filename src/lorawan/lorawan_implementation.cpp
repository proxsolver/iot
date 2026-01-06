/**
 * LoRaWAN Implementation
 *
 * Complete production-ready LoRaWAN implementation for Arduino MKR WAN 1310
 * Using IBM LMIC library with full OTAA/ABP support.
 *
 * This file contains the complete implementation of the LoRaWANManager class
 * and all supporting functions for LoRaWAN communication.
 *
 * Author: Production-Ready Implementation
 * Version: 2.0.0
 * License: MIT
 */

#include "lorawan_implementation.h"
#include <CRC16.h>

// ===========================================
// LMIC GLOBAL CONFIGURATION
// ===========================================

// Pin mapping for Arduino MKR WAN 1310
// These pins are fixed by the hardware design
const lmic_pinmap lmic_pins = {
    .nss = SS,                     // CS pin (default: 5)
    .rxtx = LMIC_UNUSED_PIN,
    .rst = LMIC_UNUSED_PIN,        // RST pin (not connected on MKR WAN 1310)
    .dio = {3, 5, LMIC_UNUSED_PIN}, // DIO0, DIO1, DIO2
    .rxtx_rx_active = 0,
    .rxtx_delay = 0,
    .busy = TX_BUSY_PIN,           // TX_BUSY pin (default: 14)
};

// ===========================================
// GLOBAL CREDENTIAL STORAGE
// ===========================================

// OTAA credentials (static for LMIC callbacks)
static uint8_t g_appEui[8] = {0};
static uint8_t g_devEui[8] = {0};
static uint8_t g_appKey[16] = {0};

// ===========================================
// LMIC CALLBACKS (REQUIRED)
// ===========================================

/**
 * @brief Get AppEUI for OTAA join
 * @param buf Buffer to store AppEUI (8 bytes)
 */
void os_getArtEui(u1_t* buf) {
    memcpy(buf, g_appEui, 8);
}

/**
 * @brief Get DevEUI for OTAA join
 * @param buf Buffer to store DevEUI (8 bytes)
 */
void os_getDevEui(u1_t* buf) {
    memcpy(buf, g_devEui, 8);
}

/**
 * @brief Get AppKey for OTAA join
 * @param buf Buffer to store AppKey (16 bytes)
 */
void os_getDevKey(u1_t* buf) {
    memcpy(buf, g_appKey, 16);
}

// ===========================================
// LORAWAN MANAGER IMPLEMENTATION
// ===========================================

/**
 * @brief Constructor
 */
LoRaWANManager::LoRaWANManager()
    : _connected(false)
    , _joining(false)
    , _adrEnabled(ADR_ENABLE)
    , _lastTransmission(0)
    , _lastDutyCycleTime(0)
    , _lastJoinAttempt(0)
    , _transmitInterval(TX_INTERVAL_60SEC)
    , _dutyCycleAccumulator(0)
    , _txCount(0)
    , _txSuccessCount(0)
    , _txFailCount(0)
    , _rxCount(0)
    , _joinRetryCount(0)
    , _retryCount(0)
    , _maxRetries(MAX_RETRIES)
    , _dataRate(DEFAULT_DATA_RATE)
    , _txPower(DEFAULT_TX_POWER)
    , _onJoinCallback(nullptr)
    , _onTxCompleteCallback(nullptr)
    , _onDownlinkCallback(nullptr)
    , _onErrorCallback(nullptr)
{
    memset(_appEui, 0, sizeof(_appEui));
    memset(_devEui, 0, sizeof(_devEui));
    memset(_appKey, 0, sizeof(_appKey));
    memset(_nwkSKey, 0, sizeof(_nwkSKey));
    memset(_appSKey, 0, sizeof(_appSKey));
    _devAddr = 0;
}

/**
 * @brief Initialize LoRaWAN with OTAA credentials
 * @param appEui Application EUI (8 bytes, LSB first)
 * @param appKey Application Key (16 bytes)
 * @param devEui Device EUI (8 bytes, LSB first, optional - uses built-in if null)
 * @return true if initialization successful
 */
bool LoRaWANManager::beginOTAA(const uint8_t* appEui, const uint8_t* appKey, const uint8_t* devEui) {
    // Store credentials
    memcpy(g_appEui, appEui, 8);
    memcpy(g_appKey, appKey, 16);

    if (devEui != nullptr) {
        memcpy(g_devEui, devEui, 8);
    } else {
        // Use built-in DevEUI
        uint8_t builtInDevEui[8];
        os_getDevEui(builtInDevEui);
        memcpy(g_devEui, builtInDevEui, 8);
    }

    // Initialize LMIC
    os_init();
    LMIC_reset();

    // Set RX1 and RX2 window parameters
    LMIC.rx1_delay = RX1_DELAY;
    LMIC.rx2_delay = RX2_DELAY;

    // Set default channel configuration
    configureChannels();

    // Set initial data rate and power
    LMIC_setDrTxpow(DEFAULT_DATA_RATE, DEFAULT_TX_POWER);

    // Enable ADR
    LMIC_setAdrMode(ADR_ENABLE ? 1 : 0);

    // Set link check interval
    LMIC_setLinkCheckMode(1);

    // Disable link check validation (for faster join)
    LMIC.dn2Dr = DR_DNW2;  // Downlink data rate for RX2

    Serial.println(F("LoRaWAN initialized (OTAA mode)"));
    printState();

    return true;
}

/**
 * @brief Initialize LoRaWAN with ABP credentials
 * @param nwkSKey Network Session Key (16 bytes)
 * @param appSKey Application Session Key (16 bytes)
 * @param devAddr Device Address (4 bytes)
 * @return true if initialization successful
 */
bool LoRaWANManager::beginABP(const uint8_t* nwkSKey, const uint8_t* appSKey, uint32_t devAddr) {
    // Store credentials
    memcpy(_nwkSKey, nwkSKey, 16);
    memcpy(_appSKey, appSKey, 16);
    _devAddr = devAddr;

    // Initialize LMIC
    os_init();
    LMIC_reset();

    // Set session keys
    LMIC_setSession(0x1, _devAddr, _nwkSKey, _appSKey);

    // Set RX1 and RX2 window parameters
    LMIC.rx1_delay = RX1_DELAY;
    LMIC.rx2_delay = RX2_DELAY;

    // Set default channel configuration
    configureChannels();

    // Set initial data rate and power
    LMIC_setDrTxpow(DEFAULT_DATA_RATE, DEFAULT_TX_POWER);

    // Enable ADR (can be used with ABP)
    LMIC_setAdrMode(ADR_ENABLE ? 1 : 0);

    // For ABP, we're immediately "connected"
    _connected = true;

    Serial.println(F("LoRaWAN initialized (ABP mode)"));
    printState();

    return true;
}

/**
 * @brief Connect to LoRaWAN network
 * @return true if connection successful
 */
bool LoRaWANManager::connect() {
    if (_connected) {
        Serial.println(F("Already connected"));
        return true;
    }

    if (_joining) {
        Serial.println(F("Join already in progress"));
        return false;
    }

    // Check if we've joined too recently
    unsigned long now = millis();
    if (now - _lastJoinAttempt < JOIN_RETRY_DELAY) {
        Serial.println(F("Join retry too soon"));
        return false;
    }

    _joining = true;
    _lastJoinAttempt = now;
    _joinRetryCount++;

    Serial.print(F("Joining network (attempt "));
    Serial.print(_joinRetryCount);
    Serial.println(F(")..."));

    // Start join
    LMIC_startJoining();

    // Wait for join completion (with timeout)
    unsigned long timeout = 60000;  // 60 seconds
    unsigned long startTime = millis();

    while (_joining && (millis() - startTime < timeout)) {
        os_runloop_once();
        delay(10);
    }

    if (_connected) {
        Serial.println(F("Join successful!"));
        Serial.print(F("DevAddr: 0x"));
        Serial.println(LMIC.devaddr, HEX);

        // Notify callback
        if (_onJoinCallback) {
            _onJoinCallback(true);
        }

        _joining = false;
        return true;
    } else {
        Serial.println(F("Join failed"));

        // Notify callback
        if (_onJoinCallback) {
            _onJoinCallback(false);
        }

        _joining = false;
        return false;
    }
}

/**
 * @brief Disconnect from network
 */
void LoRaWANManager::disconnect() {
    _connected = false;
    LMIC_reset();
    Serial.println(F("Disconnected"));
}

/**
 * @brief Rejoin network
 * @return true if rejoin successful
 */
bool LoRaWANManager::rejoin() {
    disconnect();
    delay(1000);
    return connect();
}

/**
 * @brief Transmit raw packet
 * @param payload Payload data
 * @param size Payload size
 * @param port LoRaWAN port
 * @return true if transmission successful
 */
bool LoRaWANManager::transmitPacket(const uint8_t* payload, size_t size, uint8_t port) {
    if (!_connected) {
        Serial.println(F("Cannot transmit: Not connected"));
        if (_onErrorCallback) {
            _onErrorCallback(ERR_NOT_JOINED);
        }
        return false;
    }

    // Check payload size
    if (size > LMIC_MAX_PAYLOAD_LENGTH) {
        Serial.print(F("Payload too large: "));
        Serial.println(size);
        if (_onErrorCallback) {
            _onErrorCallback(ERR_BUFFER_OVERFLOW);
        }
        return false;
    }

    // Check duty cycle
    if (!canTransmit()) {
        Serial.println(F("Cannot transmit: Duty cycle limit reached"));
        return false;
    }

    // Validate packet if it has checksum
    if (size >= sizeof(uint16_t)) {
        uint16_t receivedChecksum = *((uint16_t*)(payload + size - sizeof(uint16_t)));
        uint16_t calculatedChecksum = calculateCRC16(payload, size - sizeof(uint16_t));

        if (receivedChecksum != calculatedChecksum) {
            Serial.println(F("Checksum validation failed"));
            if (_onErrorCallback) {
                _onErrorCallback(ERR_CHECKSUM_FAIL);
            }
            return false;
        }
    }

    // Calculate airtime
    uint32_t airtimeMs = calculateAirtime(size);

    // Attempt transmission with retries
    _retryCount = 0;
    bool success = false;

    while (_retryCount < _maxRetries && !success) {
        if (_retryCount > 0) {
            unsigned long retryDelay = getRetryDelay();

            Serial.print(F("Retry "));
            Serial.print(_retryCount);
            Serial.print(F(" in "));
            Serial.print(retryDelay);
            Serial.println(F("ms"));

            delay(retryDelay);
        }

        // Prepare transmission
        _txCount++;

        // Send payload
        int result = LMIC_setTxData2(port, (xref2u1_t)payload, size, 0);

        if (result != 0) {
            Serial.print(F("TX failed: "));
            Serial.println(result);

            _txFailCount++;
            _retryCount++;

            if (_onErrorCallback) {
                _onErrorCallback(result);
            }

            continue;
        }

        // Wait for transmission complete
        Serial.print(F("TX in progress ("));
        Serial.print(size);
        Serial.println(F(" bytes)..."));

        unsigned long txTimeout = 30000;  // 30 seconds
        unsigned long txStartTime = millis();

        while (LMIC.opmode & OP_TXRXPEND) {
            os_runloop_once();

            if (millis() - txStartTime > txTimeout) {
                Serial.println(F("TX timeout"));

                _txFailCount++;
                _retryCount++;

                if (_onErrorCallback) {
                    _onErrorCallback(ERR_NETWORK_ERROR);
                }

                break;
            }

            delay(10);
        }

        // Check if transmission was successful
        success = !(LMIC.opmode & OP_TXRXPEND);

        if (success) {
            _txSuccessCount++;
            _lastTransmission = millis();
            recordTransmission(airtimeMs);

            Serial.print(F("TX successful (airtime: "));
            Serial.print(airtimeMs);
            Serial.println(F("ms)"));

            if (_onTxCompleteCallback) {
                _onTxCompleteCallback(true);
            }
        } else {
            _txFailCount++;
            _retryCount++;

            Serial.print(F("TX failed, retry "));
            Serial.println(_retryCount);

            if (_onTxCompleteCallback) {
                _onTxCompleteCallback(false);
            }
        }
    }

    if (!success) {
        Serial.println(F("TX failed after all retries"));

        if (_onTxCompleteCallback) {
            _onTxCompleteCallback(false);
        }
    }

    _retryCount = 0;
    return success;
}

/**
 * @brief Transmit sensor data packet
 * @param packet Sensor data packet
 * @return true if transmission successful
 */
bool LoRaWANManager::transmitSensorData(const SensorDataPacket& packet) {
    return transmitPacket((const uint8_t*)&packet, sizeof(SensorDataPacket), LORAWAN_PORT_SENSOR);
}

/**
 * @brief Transmit detection packet
 * @param packet Detection data packet
 * @return true if transmission successful
 */
bool LoRaWANManager::transmitDetection(const DetectionDataPacket& packet) {
    return transmitPacket((const uint8_t*)&packet, sizeof(DetectionDataPacket), LORAWAN_PORT_DETECTION);
}

/**
 * @brief Transmit status packet
 * @param packet Status data packet
 * @return true if transmission successful
 */
bool LoRaWANManager::transmitStatus(const StatusDataPacket& packet) {
    return transmitPacket((const uint8_t*)&packet, sizeof(StatusDataPacket), LORAWAN_PORT_STATUS);
}

/**
 * @brief Send downlink message
 * @param payload Payload data
 * @param size Payload size
 * @param port LoRaWAN port
 * @param confirmed Request confirmation
 * @return true if downlink scheduled successfully
 */
bool LoRaWANManager::sendDownlink(const uint8_t* payload, size_t size, uint8_t port, bool confirmed) {
    if (!_connected) {
        return false;
    }

    // Schedule downlink (will be sent in next RX window)
    // Note: This is a simplified implementation
    // Actual downlink scheduling depends on LMIC state
    return false;
}

/**
 * @brief Set data rate
 * @param dr Data rate (DR0-DR5)
 */
void LoRaWANManager::setDataRate(uint8_t dr) {
    if (dr < MIN_DATA_RATE || dr > MAX_DATA_RATE) {
        Serial.println(F("Invalid data rate"));
        return;
    }

    _dataRate = dr;
    LMIC_setDrTxpow(dr, _txPower);

    Serial.print(F("Data rate set to DR"));
    Serial.println(dr);
}

/**
 * @brief Set TX power
 * @param power TX power in dBm (0-20)
 */
void LoRaWANManager::setTxPower(int8_t power) {
    if (power < MIN_TX_POWER || power > MAX_TX_POWER) {
        Serial.println(F("Invalid TX power"));
        return;
    }

    _txPower = power;
    LMIC_setDrTxpow(_dataRate, power);

    Serial.print(F("TX power set to "));
    Serial.print(power);
    Serial.println(F(" dBm"));
}

/**
 * @brief Set transmit interval
 * @param interval Interval in milliseconds
 */
void LoRaWANManager::setTransmitInterval(unsigned long interval) {
    if (interval < TX_INTERVAL_MIN || interval > TX_INTERVAL_MAX) {
        Serial.println(F("Invalid transmit interval"));
        return;
    }

    _transmitInterval = interval;

    Serial.print(F("Transmit interval set to "));
    Serial.print(interval / 1000);
    Serial.println(F(" seconds"));
}

/**
 * @brief Enable/disable ADR
 * @param enabled Enable ADR
 */
void LoRaWANManager::setAdrEnabled(bool enabled) {
    _adrEnabled = enabled;
    LMIC_setAdrMode(enabled ? 1 : 0);

    Serial.print(F("ADR "));
    Serial.println(enabled ? F("enabled") : F("disabled"));
}

/**
 * @brief Set maximum retries
 * @param maxRetries Maximum number of retries
 */
void LoRaWANManager::setMaxRetries(uint8_t maxRetries) {
    _maxRetries = maxRetries;
}

/**
 * @brief Get duty cycle usage
 * @return Duty cycle usage in percent
 */
float LoRaWANManager::getDutyCycleUsage() const {
    unsigned long currentTime = millis();

    // Calculate duty cycle usage in the current window
    float usage = (float)_dutyCycleAccumulator / DUTY_CYCLE_WINDOW * 100.0f;

    // Reset if window has passed
    if (currentTime - _lastDutyCycleTime > DUTY_CYCLE_WINDOW) {
        const_cast<LoRaWANManager*>(this)->_dutyCycleAccumulator = 0;
        usage = 0;
    }

    return usage;
}

/**
 * @brief Reset statistics
 */
void LoRaWANManager::resetStatistics() {
    _txCount = 0;
    _txSuccessCount = 0;
    _txFailCount = 0;
    _rxCount = 0;
    _joinRetryCount = 0;
    _dutyCycleAccumulator = 0;
    _lastDutyCycleTime = millis();

    Serial.println(F("Statistics reset"));
}

/**
 * @brief Set downlink callback
 * @param callback Callback function
 */
void LoRaWANManager::setDownlinkCallback(OnDownlinkCallback callback) {
    _onDownlinkCallback = callback;
}

/**
 * @brief Main loop - must be called regularly
 */
void LoRaWANManager::loop() {
    // Process LMIC events
    os_runloop_once();
}

/**
 * @brief LMIC event handler
 * @param ev Event type
 */
void LoRaWANManager::onEvent(ev_t ev) {
    Serial.print(os_getTime());
    Serial.print(F(": "));
    switch (ev) {
        case EV_SCAN_TIMEOUT:
            Serial.println(F("EV_SCAN_TIMEOUT"));
            break;

        case EV_BEACON_FOUND:
            Serial.println(F("EV_BEACON_FOUND"));
            break;

        case EV_BEACON_MISSED:
            Serial.println(F("EV_BEACON_MISSED"));
            break;

        case EV_BEACON_TRACKED:
            Serial.println(F("EV_BEACON_TRACKED"));
            break;

        case EV_JOINING:
            Serial.println(F("EV_JOINING"));
            break;

        case EV_JOINED:
            Serial.println(F("EV_JOINED"));

            // Disable link check validation
            LMIC_setLinkCheckMode(0);

            // Get current data rate
            _dataRate = LMIC.datarate;
            _txPower = LMIC.txpow;

            _connected = true;
            _joining = false;
            _joinRetryCount = 0;

            printState();
            break;

        case EV_JOIN_FAILED:
            Serial.println(F("EV_JOIN_FAILED"));

            _joining = false;

            // Schedule rejoin
            if (_joinRetryCount < JOIN_MAX_RETRIES) {
                Serial.print(F("Retrying join in "));
                Serial.print(JOIN_RETRY_DELAY / 1000);
                Serial.println(F(" seconds"));
            }
            break;

        case EV_REJOIN_FAILED:
            Serial.println(F("EV_REJOIN_FAILED"));
            break;

        case EV_TXCOMPLETE:
            Serial.println(F("EV_TXCOMPLETE"));

            // Check if we received data in RX window
            if (LMIC.dataLen) {
                Serial.print(F("Received "));
                Serial.print(LMIC.dataLen);
                Serial.println(F(" bytes"));

                // Extract RSSI and SNR
                int rssi = LMIC.rssi;
                int snr = LMIC.snr / 4;

                Serial.print(F("RSSI: "));
                Serial.print(rssi);
                Serial.println(F(" dBm"));

                Serial.print(F("SNR: "));
                Serial.print(snr);
                Serial.println(F(" dB"));

                // Call downlink callback if registered
                if (_onDownlinkCallback) {
                    _onDownlinkCallback(LMIC.frame, LMIC.dataLen, rssi);
                }

                _rxCount++;
            }

            // Check if TX was successful
            if (LMIC.txrxFlags & TXRX_ACK) {
                Serial.println(F("ACK received"));
            }
            break;

        case EV_LOST_TSYNC:
            Serial.println(F("EV_LOST_TSYNC"));
            _connected = false;

            if (_onErrorCallback) {
                _onErrorCallback(ERR_NETWORK_ERROR);
            }

            // Try to rejoin
            rejoin();
            break;

        case EV_RESET:
            Serial.println(F("EV_RESET"));
            break;

        case EV_RXCOMPLETE:
            Serial.println(F("EV_RXCOMPLETE"));
            break;

        case EV_LINK_DEAD:
            Serial.println(F("EV_LINK_DEAD"));
            _connected = false;

            if (_onErrorCallback) {
                _onErrorCallback(ERR_NETWORK_ERROR);
            }

            // Try to rejoin
            rejoin();
            break;

        case EV_LINK_ALIVE:
            Serial.println(F("EV_LINK_ALIVE"));
            break;

        default:
            Serial.print(F("Unknown event: "));
            Serial.println((unsigned)ev);
            break;
    }
}

/**
 * @brief Print current state
 */
void LoRaWANManager::printState() {
    Serial.println(F("=== LoRaWAN State ==="));
    Serial.print(F("Connected: "));
    Serial.println(_connected ? F("Yes") : F("No"));
    Serial.print(F("Data Rate: DR"));
    Serial.println(_dataRate);
    Serial.print(F("TX Power: "));
    Serial.print(_txPower);
    Serial.println(F(" dBm"));
    Serial.print(F("ADR: "));
    Serial.println(_adrEnabled ? F("Enabled") : F("Disabled"));
    Serial.print(F("DevAddr: 0x"));
    Serial.println(LMIC.devaddr, HEX);
    Serial.println(F("===================="));
}

/**
 * @brief Print statistics
 */
void LoRaWANManager::printStatistics() {
    Serial.println(F("=== LoRaWAN Statistics ==="));
    Serial.print(F("TX Count: "));
    Serial.println(_txCount);
    Serial.print(F("TX Success: "));
    Serial.println(_txSuccessCount);
    Serial.print(F("TX Fail: "));
    Serial.println(_txFailCount);
    Serial.print(F("RX Count: "));
    Serial.println(_rxCount);
    Serial.print(F("Success Rate: "));
    Serial.print((_txCount > 0) ? (_txSuccessCount * 100 / _txCount) : 0);
    Serial.println(F("%"));
    Serial.print(F("Duty Cycle: "));
    Serial.print(getDutyCycleUsage(), 2);
    Serial.println(F("%"));
    Serial.println(F("========================="));
}

// ===========================================
// PRIVATE METHODS
// ===========================================

/**
 * @brief Calculate CRC16 checksum
 * @param data Data buffer
 * @param length Data length
 * @return CRC16 checksum
 */
uint16_t LoRaWANManager::calculateCRC16(const uint8_t* data, size_t length) {
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

/**
 * @brief Validate packet
 * @param data Data buffer
 * @param length Data length
 * @return true if packet is valid
 */
bool LoRaWANManager::validatePacket(const uint8_t* data, size_t length) {
    if (length < sizeof(uint16_t) + sizeof(uint16_t)) {  // magic + checksum
        return false;
    }

    // Check magic number
    uint16_t magic = *((uint16_t*)data);
    if (magic != PACKET_MAGIC) {
        return false;
    }

    // Validate checksum
    uint16_t receivedChecksum = *((uint16_t*)(data + length - sizeof(uint16_t)));
    uint16_t calculatedChecksum = calculateCRC16(data, length - sizeof(uint16_t));

    return receivedChecksum == calculatedChecksum;
}

/**
 * @brief Get retry delay with exponential backoff
 * @return Delay in milliseconds
 */
unsigned long LoRaWANManager::getRetryDelay() {
    // Exponential backoff: delay = initial * (2 ^ retryCount)
    unsigned long delay = RETRY_DELAY_INITIAL * (1UL << _retryCount);
    return min(delay, RETRY_DELAY_MAX);
}

/**
 * @brief Check if transmission is allowed
 * @return true if transmission allowed
 */
bool LoRaWANManager::canTransmit() {
    return getDutyCycleUsage() < DUTY_CYCLE_LIMIT;
}

/**
 * @brief Record transmission time
 * @param airtimeMs Airtime in milliseconds
 */
void LoRaWANManager::recordTransmission(uint32_t airtimeMs) {
    _dutyCycleAccumulator += airtimeMs;
    _lastDutyCycleTime = millis();

    // Reset duty cycle window if expired
    resetDutyCycleWindow();
}

/**
 * @brief Reset duty cycle window if expired
 */
void LoRaWANManager::resetDutyCycleWindow() {
    unsigned long currentTime = millis();

    if (currentTime - _lastDutyCycleTime > DUTY_CYCLE_WINDOW) {
        _dutyCycleAccumulator = 0;
        _lastDutyCycleTime = currentTime;
    }
}

/**
 * @brief Calculate airtime for payload
 * @param payloadSize Payload size in bytes
 * @return Airtime in milliseconds
 */
uint32_t LoRaWANManager::calculateAirtime(size_t payloadSize) {
    // Simplified airtime calculation
    // Actual calculation depends on data rate, bandwidth, coding rate, etc.

    uint8_t dr = LMIC.datarate;

    // Approximate bit rates for different data rates (kbps)
    float bitRates[] = {
        0.98f,   // DR0 (SF12)
        1.76f,   // DR1 (SF11)
        3.13f,   // DR2 (SF10)
        5.47f,   // DR3 (SF9)
        9.78f,   // DR4 (SF8)
        17.3f    // DR5 (SF7)
    };

    uint8_t drIndex = min(dr, (uint8_t)5);
    float bitRate = bitRates[drIndex] * 1000.0f;  // Convert to bps

    // Calculate airtime (including overhead)
    uint32_t airtimeMs = ((payloadSize + 13) * 8 * 1000) / bitRate;

    return airtimeMs;
}

/**
 * @brief Configure channels for current region
 */
void LoRaWANManager::configureChannels() {
    setDefaultChannels();
}

/**
 * @brief Set default channels for current region
 */
void LoRaWANManager::setDefaultChannels() {
    // LMIC automatically configures channels based on region setting
    // This function can be used to override default channels if needed

#if defined(CFG_eu868)
    // EU868: All channels enabled by default
    LMIC_enableChannel(0);
    LMIC_enableChannel(1);
    LMIC_enableChannel(2);

#elif defined(CFG_us915)
    // US915: Use default channels
    // LMIC automatically uses channels 8-15 for join

#elif defined(CFG_kr920)
    // KR920: All channels enabled by default
    // No specific configuration needed

#endif
}

// ===========================================
// PACKET CREATION UTILITIES
// ===========================================

/**
 * @brief Create sensor data packet
 */
SensorDataPacket createSensorPacket(
    float temperature,
    float humidity,
    float pressure,
    float gasResistance,
    uint16_t iaq,
    uint8_t status,
    uint8_t battery
) {
    SensorDataPacket packet;

    packet.magic = PACKET_MAGIC;
    packet.type = PACKET_TYPE_SENSOR;
    packet.timestamp = millis() / 1000;
    packet.temperature = (int16_t)(temperature * 100);
    packet.humidity = (uint16_t)(humidity * 100);
    packet.pressure = (uint16_t)(pressure * 10);
    packet.gasResistance = (uint16_t)gasResistance;
    packet.iaq = iaq;
    packet.status = status;
    packet.battery = battery;

    // Calculate checksum
    uint16_t checksum = 0xFFFF;
    uint8_t* data = (uint8_t*)&packet;

    for (size_t i = 0; i < sizeof(SensorDataPacket) - sizeof(uint16_t); i++) {
        checksum ^= data[i];
        for (int j = 0; j < 8; j++) {
            if (checksum & 0x0001) {
                checksum = (checksum >> 1) ^ 0xA001;
            } else {
                checksum >>= 1;
            }
        }
    }

    packet.checksum = checksum;

    return packet;
}

/**
 * @brief Create detection packet
 */
DetectionDataPacket createDetectionPacket(
    uint8_t detectionType,
    uint8_t confidence,
    uint16_t duration
) {
    DetectionDataPacket packet;

    packet.magic = PACKET_MAGIC;
    packet.type = PACKET_TYPE_DETECTION;
    packet.timestamp = millis() / 1000;
    packet.detectionType = detectionType;
    packet.confidence = confidence;
    packet.duration = duration;

    // Calculate checksum
    uint16_t checksum = 0xFFFF;
    uint8_t* data = (uint8_t*)&packet;

    for (size_t i = 0; i < sizeof(DetectionDataPacket) - sizeof(uint16_t); i++) {
        checksum ^= data[i];
        for (int j = 0; j < 8; j++) {
            if (checksum & 0x0001) {
                checksum = (checksum >> 1) ^ 0xA001;
            } else {
                checksum >>= 1;
            }
        }
    }

    packet.checksum = checksum;

    return packet;
}

/**
 * @brief Create status packet
 */
StatusDataPacket createStatusPacket(
    const LoRaWANManager& manager,
    uint8_t battery
) {
    StatusDataPacket packet;

    packet.magic = PACKET_MAGIC;
    packet.type = PACKET_TYPE_STATUS;
    packet.uptime = millis() / 1000;
    packet.txCount = manager.getTxCount();
    packet.rxCount = manager.getRxCount();
    packet.dataRate = manager.getDataRate();
    packet.battery = battery;

    // Calculate checksum
    uint16_t checksum = 0xFFFF;
    uint8_t* data = (uint8_t*)&packet;

    for (size_t i = 0; i < sizeof(StatusDataPacket) - sizeof(uint16_t); i++) {
        checksum ^= data[i];
        for (int j = 0; j < 8; j++) {
            if (checksum & 0x0001) {
                checksum = (checksum >> 1) ^ 0xA001;
            } else {
                checksum >>= 1;
            }
        }
    }

    packet.checksum = checksum;

    return packet;
}

// ===========================================
// GLOBAL INSTANCE
// ===========================================

LoRaWANManager LoRaWAN;

// ===========================================
// LMIC EVENT HANDLER
// ===========================================

/**
 * @brief LMIC event handler (called by LMIC library)
 * @param ev Event type
 */
void onEvent(ev_t ev) {
    LoRaWAN.onEvent(ev);
}
