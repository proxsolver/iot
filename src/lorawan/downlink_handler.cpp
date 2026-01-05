/**
 * LoRaWAN Downlink Handler
 *
 * Handles downlink messages from the LoRaWAN network server.
 * Supports commands for configuration, control, and status requests.
 */

#include <Arduino.h>
#include "lorawan_implementation.h"

// ===========================================
// DOWNLINK COMMAND DEFINITIONS
// ===========================================

// Command IDs
#define CMD_PING             0x00
#define CMD_SET_INTERVAL     0x01
#define CMD_SET_DATARATE     0x02
#define CMD_REBOOT           0x03
#define CMD_GET_STATUS       0x04
#define CMD_SET_LED          0x05
#define CMD_SET_ALARM        0x06
#define CMD_GET_BATTERY      0x07

// Response IDs
#define RESP_ACK             0x80
#define RESP_STATUS          0x81
#define RESP_BATTERY         0x82
#define RESP_ERROR           0xFF

// Downlink message structure
#pragma pack(push, 1)
struct DownlinkMessage {
  uint8_t commandId;
  uint8_t payload[16];  // Variable payload size
};

struct UplinkMessage {
  uint8_t responseId;
  uint8_t payload[16];
};
#pragma pack(pop)

// ===========================================
// GLOBAL VARIABLES
// ===========================================

// System configuration (modifiable via downlink)
struct SystemConfig {
  uint32_t transmitInterval;  // milliseconds
  uint8_t dataRate;
  uint8_t ledEnabled;
  uint8_t alarmEnabled;
} systemConfig = {
  60000,    // 60 seconds default
  DATA_RATE,
  1,        // LED enabled
  0         // Alarm disabled
};

// ===========================================
// DOWNLINK CALLBACK
// ===========================================

void handleDownlink(uint8_t* payload, size_t size, int rssi) {
  Serial.println("Downlink received!");
  Serial.print("  RSSI: ");
  Serial.println(rssi);
  Serial.print("  Size: ");
  Serial.println(size);

  if (size < 1) {
    Serial.println("  Invalid payload size");
    return;
  }

  DownlinkMessage* msg = (DownlinkMessage*)payload;

  switch (msg->commandId) {
    case CMD_PING:
      handlePing();
      break;

    case CMD_SET_INTERVAL:
      handleSetInterval(msg->payload);
      break;

    case CMD_SET_DATARATE:
      handleSetDataRate(msg->payload);
      break;

    case CMD_REBOOT:
      handleReboot();
      break;

    case CMD_GET_STATUS:
      handleGetStatus();
      break;

    case CMD_SET_LED:
      handleSetLED(msg->payload);
      break;

    case CMD_SET_ALARM:
      handleSetAlarm(msg->payload);
      break;

    case CMD_GET_BATTERY:
      handleGetBattery();
      break;

    default:
      Serial.print("  Unknown command: 0x");
      Serial.println(msg->commandId, HEX);
      sendErrorResponse(ERR_UNKNOWN_COMMAND);
      break;
  }
}

// ===========================================
// COMMAND HANDLERS
// ===========================================

void handlePing() {
  Serial.println("  Command: PING");
  sendAck();
}

void handleSetInterval(uint8_t* payload) {
  // Extract interval (4 bytes, little-endian)
  uint32_t interval = *((uint32_t*)payload);

  Serial.print("  Command: SET_INTERVAL (");
  Serial.print(interval);
  Serial.println("ms)");

  // Validate interval (min 10 seconds, max 1 hour)
  if (interval < 10000 || interval > 3600000) {
    Serial.println("  Invalid interval");
    sendErrorResponse(ERR_INVALID_PARAMETER);
    return;
  }

  systemConfig.transmitInterval = interval;
  Serial.println("  Interval updated");
  sendAck();
}

void handleSetDataRate(uint8_t* payload) {
  uint8_t dataRate = payload[0];

  Serial.print("  Command: SET_DATARATE (DR");
  Serial.print(dataRate);
  Serial.println(")");

  // Validate data rate (0-5 for US915)
  if (dataRate > 5) {
    Serial.println("  Invalid data rate");
    sendErrorResponse(ERR_INVALID_PARAMETER);
    return;
  }

  systemConfig.dataRate = dataRate;
  LoRaWAN.setDataRate(dataRate);
  Serial.println("  Data rate updated");
  sendAck();
}

void handleReboot() {
  Serial.println("  Command: REBOOT");
  sendAck();

  // Delay to allow transmission
  delay(1000);

  // Reboot
  NVIC_SystemReset();
}

void handleGetStatus() {
  Serial.println("  Command: GET_STATUS");
  sendStatus();
}

void handleSetLED(uint8_t* payload) {
  uint8_t enabled = payload[0];

  Serial.print("  Command: SET_LED (");
  Serial.print(enabled ? "ON" : "OFF");
  Serial.println(")");

  systemConfig.ledEnabled = enabled;
  digitalWrite(LED_BUILTIN, enabled ? HIGH : LOW);
  Serial.println("  LED status updated");
  sendAck();
}

void handleSetAlarm(uint8_t* payload) {
  uint8_t enabled = payload[0];

  Serial.print("  Command: SET_ALARM (");
  Serial.print(enabled ? "ON" : "OFF");
  Serial.println(")");

  systemConfig.alarmEnabled = enabled;
  Serial.println("  Alarm status updated");
  sendAck();
}

void handleGetBattery() {
  Serial.println("  Command: GET_BATTERY");
  sendBatteryLevel();
}

// ===========================================
// RESPONSE SENDERS
// ===========================================

void sendAck() {
  UplinkMessage msg;
  msg.responseId = RESP_ACK;
  memset(msg.payload, 0, sizeof(msg.payload));

  LoRaWAN.sendPayload(LORAWAN_PORT, (uint8_t*)&msg, 1);
  Serial.println("  ACK sent");
}

void sendStatus() {
  UplinkMessage msg;
  msg.responseId = RESP_STATUS;

  // Pack status information
  msg.payload[0] = systemConfig.transmitInterval & 0xFF;
  msg.payload[1] = (systemConfig.transmitInterval >> 8) & 0xFF;
  msg.payload[2] = (systemConfig.transmitInterval >> 16) & 0xFF;
  msg.payload[3] = (systemConfig.transmitInterval >> 24) & 0xFF;
  msg.payload[4] = systemConfig.dataRate;
  msg.payload[5] = systemConfig.ledEnabled;
  msg.payload[6] = systemConfig.alarmEnabled;
  msg.payload[7] = LoRaWAN.currentDataRate();

  LoRaWAN.sendPayload(LORAWAN_PORT, (uint8_t*)&msg, 9);
  Serial.println("  Status sent");
}

void sendBatteryLevel() {
  UplinkMessage msg;
  msg.responseId = RESP_BATTERY;

  // Read battery voltage (MKR WAN 1310 has analog pin A0 for battery)
  int batteryRaw = analogRead(A0);
  float batteryVoltage = batteryRaw * (3.3f / 1023.0f) * 2.0f;  // Voltage divider
  uint8_t batteryPercent = constrain((batteryVoltage - 3.0f) / (4.2f - 3.0f) * 100, 0, 100);

  msg.payload[0] = batteryPercent;
  msg.payload[1] = (uint8_t)(batteryVoltage * 10);

  LoRaWAN.sendPayload(LORAWAN_PORT, (uint8_t*)&msg, 3);
  Serial.println("  Battery level sent");
}

void sendErrorResponse(uint8_t errorCode) {
  UplinkMessage msg;
  msg.responseId = RESP_ERROR;
  msg.payload[0] = errorCode;

  LoRaWAN.sendPayload(LORAWAN_PORT, (uint8_t*)&msg, 2);
  Serial.println("  Error response sent");
}

// ===========================================
// ERROR CODES
// ===========================================

#define ERR_UNKNOWN_COMMAND      0x01
#define ERR_INVALID_PARAMETER    0x02
#define ERR_NOT_IMPLEMENTED      0x03

// ===========================================
// UTILITY FUNCTIONS
// ===========================================

void setupDownlinkHandler() {
  // Set downlink callback
  LoRaWAN.setDownlinkCallback(handleDownlink);
  Serial.println("Downlink handler registered");
}

void processDownlink() {
  // Called in main loop to process downlink messages
  LoRaWAN.processDownlink();
}
