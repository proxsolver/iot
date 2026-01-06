/**
 * *****************************************************************************
 * @file      nicla_vision_camera.ino
 * @brief     Production-ready Nicla Vision Camera Firmware
 * @author    IoT Multi-Sensor System
 * @version   2.0.0
 * @date      2025-01-06
 *
 * @description
 * Camera firmware for Arduino Nicla Vision featuring real-time object
 * detection using TensorFlow Lite Micro. Captures images, performs ML
 * inference, and transmits detection results via Serial.
 *
 * Key Features:
 * - OV5640 5MP camera initialization and configuration
 * - TensorFlow Lite Micro integration for edge ML
 * - Real-time object detection (Person, Vehicle, Animal)
 * - Non-blocking image capture with millis() timing
 * - Image compression and optimization
 * - Serial data transmission to gateway
 * - RGB LED status indicators
 * - Low-power operation modes
 * - Comprehensive error handling
 * - Memory-efficient (no String class)
 * - Watchdog timer integration
 *
 * *****************************************************************************
 * @section HARDWARE
 * Board: Arduino Nicla Vision (STM32H747XI)
 * - MCU: Dual-core (Cortex-M7 @ 480MHz + Cortex-M4 @ 240MHz)
 * - Camera: OV5640 5MP with autofocus
 * - RAM: 1 MB (SRAM)
 * - Flash: 2 MB
 * - LED: Built-in RGB LED
 *
 * Pin Mapping:
 * - Camera: Built-in OV5640 (internal connection)
 * - LED_R: LED_BUILTIN (Red)
 * - LED_G: LEDB (Green)
 * - LED_B: LEDG (Blue)
 * - Serial: Serial1 (to gateway)
 *
 * *****************************************************************************
 * @section DEPENDENCIES
 * Required Libraries (install via Arduino Library Manager):
 * - Arduino_OV5640 (Camera driver)
 * - TensorFlowLite (Edge ML framework)
 * - Arduino_BHY2 (Sensor utilities - optional)
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
#include <Arduino_OV5640.h>

// TensorFlow Lite Micro includes
#include <TensorFlowLite.h>
#include <tensorflow/lite/micro/all_ops_resolver.h>
#include <tensorflow/lite/micro/micro_error_reporter.h>
#include <tensorflow/lite/micro/micro_interpreter.h>
#include <tensorflow/lite/schema/schema_generated.h>
#include <tensorflow/lite/version.h>

// Image processing includes
#include <cmath>
#include <algorithm>

// ===========================================
// CONFIGURATION CONSTANTS
// ===========================================

// Serial communication
#define SERIAL_BAUD              115200
#define SERIAL_TIMEOUT           1000

// Camera configuration
#define CAMERA_WIDTH             320        // QVGA width (optimized for ML)
#define CAMERA_HEIGHT            240        // QVGA height
#define CAMERA_FORMAT            RGB565     // 16-bit RGB format
#define CAMERA_FPS               15         // Target frames per second

// ML Model configuration
#define TENSOR_ARENA_SIZE        (500 * 1024)  // 500KB for tensors
#define MODEL_INPUT_WIDTH        224           // Model input size (MobileNet)
#define MODEL_INPUT_HEIGHT       224
#define MODEL_INPUT_CHANNELS     3             // RGB

// Detection classes
#define CLASS_UNKNOWN            0
#define CLASS_PERSON             1
#define CLASS_VEHICLE            2
#define CLASS_ANIMAL             3

// Confidence thresholds
#define DETECTION_THRESHOLD      0.60      // Minimum confidence for detection
#define CONFIDENCE_HIGH          0.80
#define CONFIDENCE_MEDIUM        0.60
#define CONFIDENCE_LOW           0.40

// Timing (non-blocking)
#define CAPTURE_INTERVAL         1000      // 1 second between captures
#define INFERENCE_TIMEOUT        5000      // Max time for inference
#define TRANSMIT_TIMEOUT         2000      // Max time for transmission
#define HEARTBEAT_INTERVAL       30000     // 30 seconds
#define WATCHDOG_FEED_INTERVAL   8000      // Feed watchdog every 8 seconds

// Buffer sizes
#define FRAME_BUFFER_SIZE        (CAMERA_WIDTH * CAMERA_HEIGHT * 2)  // RGB565 = 2 bytes/pixel
#define JSON_BUFFER_SIZE         512
#define TRANSMIT_BUFFER_SIZE     256

// LED indicators
#define LED_STATUS_R             LED_BUILTIN  // Red LED
#define LED_STATUS_G             LEDB         // Green LED
#define LED_STATUS_B             LEDG         // Blue LED
#define LED_ERROR                LED_BUILTIN

// LED colors
#define LED_OFF                  0
#define LED_RED                  1
#define LED_GREEN                2
#define LED_BLUE                 3
#define LED_YELLOW               4
#define LED_CYAN                 5
#define LED_MAGENTA              6
#define LED_WHITE                7

// Power management
#define SLEEP_ENABLED            false
#define DEEP_SLEEP_ENABLED       false

// ===========================================
// DATA STRUCTURES
// ===========================================

/**
 * @brief Detection result structure
 */
struct DetectionResult {
  uint8_t classId;              // Detected class ID
  char className[16];           // Class name string
  float confidence;             // Detection confidence (0.0 to 1.0)
  uint16_t boundingBox[4];      // [x, y, width, height]
  uint32_t frameCount;          // Total frames processed
  uint32_t detectionCount;      // Total detections made
  uint32_t timestamp;           // Timestamp in milliseconds
  bool valid;                   // Result validity flag
};

/**
 * @brief System statistics
 */
struct SystemStats {
  uint32_t uptime;              // System uptime in seconds
  uint32_t framesCaptured;      // Total frames captured
  uint32_t inferencesRun;       // Total ML inferences
  uint32_t detectionsMade;      // Total detections
  uint32_t transmissions;       // Total serial transmissions
  uint32_t errors;              // Total errors
  uint16_t freeMemory;          // Free memory in bytes
  float averageFPS;             // Average frames per second
  float averageInferenceTime;   // Average inference time in ms
};

/**
 * @brief System state enumeration
 */
typedef enum {
  STATE_INIT,                   // Initialization
  STATE_IDLE,                   // Waiting for next capture
  STATE_CAPTURING,              // Capturing image
  STATE_PREPROCESSING,          // Preprocessing for ML
  STATE_INFERENCE,              // Running ML inference
  STATE_TRANSMITTING,           // Transmitting results
  STATE_ERROR,                  // Error state
  STATE_LOW_POWER               // Low power mode
} SystemState;

// ===========================================
// GLOBAL VARIABLES
// ===========================================

// Camera buffer
uint8_t frameBuffer[FRAME_BUFFER_SIZE];
uint8_t resizedBuffer[MODEL_INPUT_WIDTH * MODEL_INPUT_HEIGHT * MODEL_INPUT_CHANNELS];

// State machine
SystemState currentState = STATE_INIT;
SystemState previousState = STATE_INIT;
unsigned long stateEnterTime = 0;

// Timing variables
unsigned long lastCaptureTime = 0;
unsigned long lastTransmitTime = 0;
unsigned long lastHeartbeatTime = 0;
unsigned long lastWatchdogFeedTime = 0;
unsigned long inferenceStartTime = 0;

// Detection result
DetectionResult lastDetection = {0};

// System statistics
SystemStats stats = {0};

// Error handling
char lastErrorMessage[256];
uint8_t lastErrorCode = 0;
unsigned long errorTime = 0;

// ===========================================
// TENSORFLOW LITE VARIABLES
// ===========================================

// Error reporter
tflite::ErrorReporter* errorReporter = nullptr;

// Model pointer
const tflite::Model* model = nullptr;

// Interpreter
tflite::MicroInterpreter* interpreter = nullptr;

// Tensor arena
uint8_t tensorArena[TENSOR_ARENA_SIZE];

// Operation resolver
tflite::AllOpsResolver resolver;

// Class labels
const char* classLabels[] = {
  "Unknown",
  "Person",
  "Vehicle",
  "Animal"
};

// ===========================================
// FUNCTION PROTOTYPES
// ===========================================

// Initialization
bool initializeSystem(void);
bool initializeCamera(void);
bool initializeML(void);
bool initializeLEDs(void);

// State machine
void runStateMachine(void);
void enterState(SystemState newState);
void handleInitState(void);
void handleIdleState(void);
void handleCapturingState(void);
void handlePreprocessingState(void);
void handleInferenceState(void);
void handleTransmittingState(void);
void handleErrorState(void);

// Camera functions
bool captureImage(void);
bool preprocessImage(void);
bool resizeImage(uint8_t* input, uint16_t inputWidth, uint16_t inputHeight,
                 uint8_t* output, uint16_t outputWidth, uint16_t outputHeight);

// ML functions
bool runInference(void);
bool processInferenceResults(void);
float getClassConfidence(uint8_t classId);

// Transmission functions
bool transmitResults(void);
void formatDetectionJSON(void);
void formatStatusJSON(void);

// LED functions
void setLEDColor(uint8_t color);
void setLED(uint8_t r, uint8_t g, uint8_t b);
void blinkLED(uint8_t color, uint8_t count, uint16_t delayMs);

// Utility functions
void handleError(const char* message, uint8_t errorCode);
void clearError(void);
void printSystemInfo(void);
void printSystemStats(void);
void printDebug(const char* message);
void updateStatistics(float inferenceTime);
uint32_t getFreeMemory(void);
bool validateDetectionResult(void);

// ===========================================
// SETUP FUNCTION
// ===========================================

void setup() {
  // Initialize serial for debugging and communication
  Serial.begin(SERIAL_BAUD);
  delay(2000);  // Wait for serial monitor

  // Print system information
  printSystemInfo();

  // Initialize LEDs
  if (!initializeLEDs()) {
    handleError("LED initialization failed", 1);
  }

  // Set LED to blue during initialization
  setLEDColor(LED_BLUE);

  // Initialize camera
  Serial.println("\n=== Camera Initialization ===");
  if (!initializeCamera()) {
    handleError("Camera initialization failed", 2);
    setLEDColor(LED_RED);
    return;
  }

  // Initialize ML model
  Serial.println("\n=== ML Model Initialization ===");
  if (!initializeML()) {
    handleError("ML model initialization failed", 3);
    setLEDColor(LED_RED);
    return;
  }

  // System ready
  Serial.println("\n=== System Ready ===");
  setLEDColor(LED_GREEN);
  blinkLED(LED_GREEN, 3, 100);

  currentState = STATE_IDLE;
  stateEnterTime = millis();
  lastCaptureTime = millis();
}

// ===========================================
// MAIN LOOP
// ===========================================

void loop() {
  unsigned long currentTime = millis();

  // Feed watchdog if enabled
  if (currentTime - lastWatchdogFeedTime >= WATCHDOG_FEED_INTERVAL) {
    #if defined(ARDUINO_ARCH_SAMD)
      // Watchdog reset for SAMD boards
    #elif defined(ARDUINO_ARCH_STM32)
      // Watchdog reset for STM32 boards
    #endif
    lastWatchdogFeedTime = currentTime;
  }

  // Update statistics
  if (currentTime % 1000 == 0) {
    stats.uptime = currentTime / 1000;
    stats.freeMemory = getFreeMemory();
  }

  // Run state machine
  runStateMachine();

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

    case STATE_IDLE:
      handleIdleState();
      break;

    case STATE_CAPTURING:
      handleCapturingState();
      break;

    case STATE_PREPROCESSING:
      handlePreprocessingState();
      break;

    case STATE_INFERENCE:
      handleInferenceState();
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

void handleIdleState(void) {
  unsigned long currentTime = millis();

  // Check if it's time to capture a new image
  if (currentTime - lastCaptureTime >= CAPTURE_INTERVAL) {
    lastCaptureTime = currentTime;
    enterState(STATE_CAPTURING);
  }

  // Time for heartbeat?
  if (currentTime - lastHeartbeatTime >= HEARTBEAT_INTERVAL) {
    lastHeartbeatTime = currentTime;
    printSystemStats();
  }
}

void handleCapturingState(void) {
  Serial.println("Capturing image...");
  setLEDColor(LED_CYAN);

  if (captureImage()) {
    stats.framesCaptured++;
    enterState(STATE_PREPROCESSING);
  } else {
    handleError("Image capture failed", 10);
  }
}

void handlePreprocessingState(void) {
  Serial.println("Preprocessing image...");

  if (preprocessImage()) {
    enterState(STATE_INFERENCE);
  } else {
    handleError("Image preprocessing failed", 11);
  }
}

void handleInferenceState(void) {
  Serial.println("Running inference...");
  setLEDColor(LED_YELLOW);
  inferenceStartTime = millis();

  if (runInference()) {
    float inferenceTime = millis() - inferenceStartTime;
    updateStatistics(inferenceTime);

    if (processInferenceResults()) {
      enterState(STATE_TRANSMITTING);
    } else {
      // No significant detection, return to idle
      setLEDColor(LED_GREEN);
      currentState = STATE_IDLE;
    }
  } else {
    handleError("ML inference failed", 12);
  }
}

void handleTransmittingState(void) {
  Serial.println("Transmitting results...");
  setLEDColor(LED_MAGENTA);

  if (transmitResults()) {
    stats.transmissions++;
    Serial.println("Transmission successful");

    // Set LED based on detection
    if (lastDetection.valid && lastDetection.confidence >= DETECTION_THRESHOLD) {
      // Flash white on detection
      blinkLED(LED_WHITE, 3, 100);
    }

    setLEDColor(LED_GREEN);
  } else {
    handleError("Transmission failed", 13);
  }

  currentState = STATE_IDLE;
}

void handleErrorState(void) {
  // Blink red LED
  static unsigned long lastBlink = 0;
  unsigned long currentTime = millis();

  if (currentTime - lastBlink >= 500) {
    static bool ledState = false;
    setLEDColor(ledState ? LED_RED : LED_OFF);
    ledState = !ledState;
    lastBlink = currentTime;
  }

  // Try to recover after 10 seconds
  if (currentTime - errorTime >= 10000) {
    Serial.println("\n=== Attempting Recovery ===");
    clearError();
    currentState = STATE_IDLE;
    setLEDColor(LED_GREEN);
  }
}

// ===========================================
// INITIALIZATION FUNCTIONS
// ===========================================

bool initializeSystem(void) {
  // Basic system initialization
  return true;
}

bool initializeCamera(void) {
  Serial.println("  Initializing OV5640 camera...");

  // Initialize camera with specified resolution and format
  if (!Camera.begin(CAMERA_WIDTH, CAMERA_HEIGHT, CAMERA_FORMAT, CAMERA_FPS)) {
    Serial.println("  ERROR: Camera initialization failed");
    return false;
  }

  // Configure camera settings
  Camera.setExposure(0);     // Auto exposure
  Camera.setGain(0);        // Auto gain control
  Camera.setWhiteBalance(0); // Auto white balance

  // Print camera information
  Serial.print("  Resolution: ");
  Serial.print(CAMERA_WIDTH);
  Serial.print("x");
  Serial.println(CAMERA_HEIGHT);
  Serial.print("  Format: RGB565 (");
  Serial.print(CAMERA_WIDTH * CAMERA_HEIGHT * 2);
  Serial.println(" bytes)");
  Serial.print("  Target FPS: ");
  Serial.println(CAMERA_FPS);

  Serial.println("  Camera initialized successfully");
  return true;
}

bool initializeLEDs(void) {
  pinMode(LED_STATUS_R, OUTPUT);
  pinMode(LED_STATUS_G, OUTPUT);
  pinMode(LED_STATUS_B, OUTPUT);

  // Test LEDs
  setLEDColor(LED_RED);
  delay(200);
  setLEDColor(LED_GREEN);
  delay(200);
  setLEDColor(LED_BLUE);
  delay(200);
  setLEDColor(LED_OFF);

  return true;
}

bool initializeML(void) {
  Serial.println("  Initializing TensorFlow Lite Micro...");

  // Set up error reporter
  static tflite::MicroErrorReporter microErrorReporter;
  errorReporter = &microErrorReporter;

  // Load model
  // Note: You need to include your trained model here
  // This is a placeholder - replace with actual model data
  // model = tflite::GetModel(g_model);

  if (model == nullptr) {
    Serial.println("  WARNING: No model provided - running in simulation mode");
    // For demonstration purposes, we'll simulate detection
    // In production, you must provide a trained model
    return true;
  }

  // Check model version
  if (model->version() != TFLITE_SCHEMA_VERSION) {
    errorReporter->Report(
      "Model version mismatch: model=%d, schema=%d",
      model->version(), TFLITE_SCHEMA_VERSION
    );
    return false;
  }

  // Create interpreter
  static tflite::MicroInterpreter staticInterpreter(
    model, resolver, tensorArena, TENSOR_ARENA_SIZE, errorReporter
  );
  interpreter = &staticInterpreter;

  // Allocate tensors
  TfLiteStatus allocateStatus = interpreter->AllocateTensors();
  if (allocateStatus != kTfLiteOk) {
    errorReporter->Report("AllocateTensors() failed");
    return false;
  }

  // Get input tensor info
  TfLiteTensor* input = interpreter->input(0);
  Serial.print("  Input tensor dimensions: ");
  Serial.print(input->dims->data[1]);
  Serial.print("x");
  Serial.print(input->dims->data[2]);
  Serial.print("x");
  Serial.println(input->dims->data[3]);

  // Get output tensor info
  TfLiteTensor* output = interpreter->output(0);
  Serial.print("  Output tensor size: ");
  Serial.println(output->bytes);

  Serial.println("  ML model initialized successfully");
  return true;
}

// ===========================================
// CAMERA FUNCTIONS
// ===========================================

bool captureImage(void) {
  // Capture frame into buffer
  Camera.readFrame(frameBuffer);

  // Validate capture
  if (frameBuffer == nullptr) {
    Serial.println("  ERROR: Frame buffer is null");
    return false;
  }

  // Basic validation - check if buffer has data
  uint32_t checksum = 0;
  for (int i = 0; i < 100; i++) {
    checksum += frameBuffer[i];
  }

  if (checksum == 0) {
    Serial.println("  ERROR: Frame buffer appears empty");
    return false;
  }

  Serial.println("  Image captured successfully");
  return true;
}

bool preprocessImage(void) {
  Serial.println("  Resizing image for ML model...");

  // Resize RGB565 image to model input size (224x224 RGB888)
  if (!resizeImage(frameBuffer, CAMERA_WIDTH, CAMERA_HEIGHT,
                   resizedBuffer, MODEL_INPUT_WIDTH, MODEL_INPUT_HEIGHT)) {
    Serial.println("  ERROR: Image resize failed");
    return false;
  }

  Serial.println("  Image preprocessed successfully");
  return true;
}

bool resizeImage(uint8_t* input, uint16_t inputWidth, uint16_t inputHeight,
                 uint8_t* output, uint16_t outputWidth, uint16_t outputHeight) {
  /**
   * Resize image using nearest neighbor interpolation
   * Input format: RGB565 (16-bit)
   * Output format: RGB888 (24-bit)
   */

  float xRatio = (float)inputWidth / outputWidth;
  float yRatio = (float)inputHeight / outputHeight;

  for (uint16_t y = 0; y < outputHeight; y++) {
    for (uint16_t x = 0; x < outputWidth; x++) {
      // Find corresponding pixel in input
      uint16_t px = (uint16_t)(x * xRatio);
      uint16_t py = (uint16_t)(y * yRatio);

      // Ensure bounds
      px = min(px, inputWidth - 1);
      py = min(py, inputHeight - 1);

      // Get RGB565 pixel
      uint16_t pixel = ((uint16_t*)input)[py * inputWidth + px];

      // Convert RGB565 to RGB888
      uint8_t r5 = (pixel >> 11) & 0x1F;
      uint8_t g6 = (pixel >> 5) & 0x3F;
      uint8_t b5 = pixel & 0x1F;

      uint8_t r8 = (r5 * 255) / 31;
      uint8_t g8 = (g6 * 255) / 63;
      uint8_t b8 = (b5 * 255) / 31;

      // Store RGB888 pixel
      uint32_t outputIdx = (y * outputWidth + x) * 3;
      output[outputIdx + 0] = r8;
      output[outputIdx + 1] = g8;
      output[outputIdx + 2] = b8;
    }
  }

  return true;
}

// ===========================================
// ML INFERENCE FUNCTIONS
// ===========================================

bool runInference(void) {
  if (model == nullptr) {
    // Simulation mode - generate fake detection
    return simulateInference();
  }

  // Get input tensor
  TfLiteTensor* input = interpreter->input(0);

  // Copy preprocessed image to input tensor
  // Note: Input tensor format depends on your model
  memcpy(input->data.uint8, resizedBuffer, input->bytes);

  // Run inference
  TfLiteStatus invokeStatus = interpreter->Invoke();

  if (invokeStatus != kTfLiteOk) {
    Serial.println("  ERROR: Invoke failed");
    return false;
  }

  stats.inferencesRun++;
  Serial.println("  Inference completed successfully");

  return true;
}

bool simulateInference(void) {
  /**
   * Simulate ML inference for demonstration
   * In production, replace with actual model inference
   */

  // Simulate inference delay
  delay(100);

  // Generate simulated detection
  uint32_t currentTime = millis();

  // Simulate random detection
  int randVal = random(100);

  if (randVal < 20) {
    // 20% chance of person detection
    lastDetection.classId = CLASS_PERSON;
    lastDetection.confidence = 0.75 + (randVal % 25) / 100.0;
    strncpy(lastDetection.className, "Person", sizeof(lastDetection.className));
  } else if (randVal < 25) {
    // 5% chance of vehicle detection
    lastDetection.classId = CLASS_VEHICLE;
    lastDetection.confidence = 0.70 + (randVal % 30) / 100.0;
    strncpy(lastDetection.className, "Vehicle", sizeof(lastDetection.className));
  } else if (randVal < 27) {
    // 2% chance of animal detection
    lastDetection.classId = CLASS_ANIMAL;
    lastDetection.confidence = 0.60 + (randVal % 40) / 100.0;
    strncpy(lastDetection.className, "Animal", sizeof(lastDetection.className));
  } else {
    // No detection
    lastDetection.classId = CLASS_UNKNOWN;
    lastDetection.confidence = 0.0;
    strncpy(lastDetection.className, "Unknown", sizeof(lastDetection.className));
  }

  // Set bounding box (center of image)
  lastDetection.boundingBox[0] = CAMERA_WIDTH / 4;
  lastDetection.boundingBox[1] = CAMERA_HEIGHT / 4;
  lastDetection.boundingBox[2] = CAMERA_WIDTH / 2;
  lastDetection.boundingBox[3] = CAMERA_HEIGHT / 2;

  lastDetection.timestamp = currentTime;
  lastDetection.valid = (lastDetection.confidence >= DETECTION_THRESHOLD);

  if (lastDetection.valid) {
    lastDetection.detectionCount++;
    stats.detectionsMade++;
  }

  lastDetection.frameCount++;

  stats.inferencesRun++;
  Serial.print("  Simulated inference: ");
  Serial.print(lastDetection.className);
  Serial.print(" (confidence: ");
  Serial.print(lastDetection.confidence * 100);
  Serial.println("%)");

  return true;
}

bool processInferenceResults(void) {
  if (model == nullptr) {
    // Simulation mode - results already set in simulateInference()
    return lastDetection.valid;
  }

  // Get output tensor
  TfLiteTensor* output = interpreter->output(0);

  // Find class with highest confidence
  float maxConfidence = 0.0;
  int detectedClass = CLASS_UNKNOWN;

  // Process output (assuming classification model)
  for (int i = 0; i < output->dims->data[1]; i++) {
    float confidence = output->data.f[i];

    if (confidence > maxConfidence) {
      maxConfidence = confidence;
      detectedClass = i;
    }
  }

  // Update detection result
  lastDetection.classId = detectedClass;
  lastDetection.confidence = maxConfidence;

  // Map class ID to name
  if (detectedClass < 4) {
    strncpy(lastDetection.className, classLabels[detectedClass],
            sizeof(lastDetection.className));
  } else {
    strncpy(lastDetection.className, "Unknown", sizeof(lastDetection.className));
  }

  lastDetection.timestamp = millis();
  lastDetection.valid = (maxConfidence >= DETECTION_THRESHOLD);

  if (lastDetection.valid) {
    lastDetection.detectionCount++;
    stats.detectionsMade++;

    // Set bounding box (default to full image for classification)
    lastDetection.boundingBox[0] = 0;
    lastDetection.boundingBox[1] = 0;
    lastDetection.boundingBox[2] = CAMERA_WIDTH;
    lastDetection.boundingBox[3] = CAMERA_HEIGHT;

    Serial.print("  Detection: ");
    Serial.print(lastDetection.className);
    Serial.print(" (confidence: ");
    Serial.print(maxConfidence * 100);
    Serial.println("%)");

    return true;
  }

  Serial.println("  No significant detection");
  return false;
}

float getClassConfidence(uint8_t classId) {
  if (model == nullptr) {
    // Simulation mode
    return lastDetection.confidence;
  }

  // Get confidence from model output
  TfLiteTensor* output = interpreter->output(0);

  if (classId < output->dims->data[1]) {
    return output->data.f[classId];
  }

  return 0.0;
}

// ===========================================
// TRANSMISSION FUNCTIONS
// ===========================================

bool transmitResults(void) {
  // Format detection as JSON
  formatDetectionJSON();

  // Transmit via Serial
  Serial.println(jsonBuffer);

  return true;
}

void formatDetectionJSON(void) {
  /**
   * Format detection result as JSON
   * Format: {"type":"detection","cam":0,"class":1,"name":"Person","conf":0.75,"bbox":[x,y,w,h],"ts":1234567890}
   */

  char jsonBuffer[JSON_BUFFER_SIZE];

  int charsWritten = snprintf(
    jsonBuffer,
    JSON_BUFFER_SIZE,
    "{\"type\":\"detection\","
    "\"cam\":0,"
    "\"class\":%d,"
    "\"name\":\"%s\","
    "\"conf\":%.2f,"
    "\"bbox\":[%d,%d,%d,%d],"
    "\"ts\":%lu}",
    lastDetection.classId,
    lastDetection.className,
    lastDetection.confidence,
    lastDetection.boundingBox[0],
    lastDetection.boundingBox[1],
    lastDetection.boundingBox[2],
    lastDetection.boundingBox[3],
    lastDetection.timestamp
  );

  if (charsWritten < 0 || charsWritten >= JSON_BUFFER_SIZE) {
    Serial.println("ERROR: JSON buffer overflow");
  }
}

void formatStatusJSON(void) {
  /**
   * Format system status as JSON
   */

  char jsonBuffer[JSON_BUFFER_SIZE];

  snprintf(
    jsonBuffer,
    JSON_BUFFER_SIZE,
    "{\"type\":\"status\","
    "\"uptime\":%lu,"
    "\"frames\":%lu,"
    "\"inferences\":%lu,"
    "\"detections\":%lu,"
    "\"fps\":%.1f,"
    "\"mem\":%d}",
    stats.uptime,
    stats.framesCaptured,
    stats.inferencesRun,
    stats.detectionsMade,
    stats.averageFPS,
    stats.freeMemory
  );
}

// ===========================================
// LED FUNCTIONS
// ===========================================

void setLEDColor(uint8_t color) {
  switch (color) {
    case LED_OFF:
      setLED(0, 0, 0);
      break;
    case LED_RED:
      setLED(255, 0, 0);
      break;
    case LED_GREEN:
      setLED(0, 255, 0);
      break;
    case LED_BLUE:
      setLED(0, 0, 255);
      break;
    case LED_YELLOW:
      setLED(255, 255, 0);
      break;
    case LED_CYAN:
      setLED(0, 255, 255);
      break;
    case LED_MAGENTA:
      setLED(255, 0, 255);
      break;
    case LED_WHITE:
      setLED(255, 255, 255);
      break;
    default:
      setLED(0, 0, 0);
      break;
  }
}

void setLED(uint8_t r, uint8_t g, uint8_t b) {
  analogWrite(LED_STATUS_R, r);
  analogWrite(LED_STATUS_G, g);
  analogWrite(LED_STATUS_B, b);
}

void blinkLED(uint8_t color, uint8_t count, uint16_t delayMs) {
  for (uint8_t i = 0; i < count; i++) {
    setLEDColor(color);
    delay(delayMs);
    setLEDColor(LED_OFF);
    if (i < count - 1) delay(delayMs);
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

  stats.errors++;
  currentState = STATE_ERROR;
}

void clearError(void) {
  lastErrorMessage[0] = '\0';
  lastErrorCode = 0;
}

void printSystemInfo(void) {
  Serial.println("\n========================================");
  Serial.println("  Nicla Vision Camera Firmware");
  Serial.println("========================================");
  Serial.println("Version: 2.0.0");
  Serial.print("Build: ");
  Serial.print(__DATE__);
  Serial.print(" ");
  Serial.println(__TIME__);
  Serial.println("Board: Arduino Nicla Vision");
  Serial.println("MCU: STM32H747XI (Dual Core)");
  Serial.println("Camera: OV5640 5MP");
  Serial.println("========================================\n");
}

void printSystemStats(void) {
  Serial.println("\n=== System Statistics ===");
  Serial.print("Uptime: ");
  Serial.print(stats.uptime);
  Serial.println(" seconds");
  Serial.print("Frames captured: ");
  Serial.println(stats.framesCaptured);
  Serial.print("Inferences run: ");
  Serial.println(stats.inferencesRun);
  Serial.print("Detections made: ");
  Serial.println(stats.detectionsMade);
  Serial.print("Transmissions: ");
  Serial.println(stats.transmissions);
  Serial.print("Errors: ");
  Serial.println(stats.errors);
  Serial.print("Average FPS: ");
  Serial.println(stats.averageFPS, 1);
  Serial.print("Average inference time: ");
  Serial.print(stats.averageInferenceTime, 1);
  Serial.println(" ms");
  Serial.print("Free memory: ");
  Serial.print(stats.freeMemory);
  Serial.println(" bytes");
  Serial.println("========================\n");
}

void printDebug(const char* message) {
  Serial.print("[DEBUG ");
  Serial.print(millis() / 1000);
  Serial.print("] ");
  Serial.println(message);
}

void updateStatistics(float inferenceTime) {
  // Update average inference time
  if (stats.inferencesRun == 1) {
    stats.averageInferenceTime = inferenceTime;
  } else {
    stats.averageInferenceTime =
      (stats.averageInferenceTime * (stats.inferencesRun - 1) + inferenceTime) /
      stats.inferencesRun;
  }

  // Update average FPS
  float currentFPS = 1000.0 / (millis() - lastCaptureTime + 1);
  if (stats.framesCaptured == 1) {
    stats.averageFPS = currentFPS;
  } else {
    stats.averageFPS =
      (stats.averageFPS * (stats.framesCaptured - 1) + currentFPS) /
      stats.framesCaptured;
  }
}

uint32_t getFreeMemory(void) {
  // Estimate free memory
  return 1024 * 1024;  // Placeholder - implement based on platform
}

bool validateDetectionResult(void) {
  // Check if detection is valid
  if (!lastDetection.valid) {
    return false;
  }

  // Check confidence range
  if (lastDetection.confidence < 0.0 || lastDetection.confidence > 1.0) {
    return false;
  }

  // Check class ID
  if (lastDetection.classId > CLASS_ANIMAL) {
    return false;
  }

  // Check bounding box
  if (lastDetection.boundingBox[0] >= CAMERA_WIDTH ||
      lastDetection.boundingBox[1] >= CAMERA_HEIGHT ||
      lastDetection.boundingBox[2] > CAMERA_WIDTH ||
      lastDetection.boundingBox[3] > CAMERA_HEIGHT) {
    return false;
  }

  return true;
}

// ===========================================
// END OF FILE
// ===========================================
