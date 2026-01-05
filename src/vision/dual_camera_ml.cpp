/**
 * Dual Camera Object Detection Implementation
 *
 * Manages dual Nicla Vision cameras with I2C multiplexer control,
 * TinyML model inference, and alarm triggering.
 *
 * Board: Arduino Nicla Vision (x2)
 * Communication: I2C via TCA9548A multiplexer
 * ML Framework: TensorFlow Lite for Microcontrollers
 */

#include <Arduino.h>
#include <Wire.h>
#include <Arduino_OV5640.h>
#include <TensorFlowLite.h>

// ===========================================
// I2C MULTIPLEXER CONFIGURATION
// ===========================================

#define TCA9548A_ADDR 0x70

// Camera channel assignments
#define CAMERA_1_CHANNEL 0
#define CAMERA_2_CHANNEL 1

// Camera IDs
#define CAMERA_1_ID 0
#define CAMERA_2_ID 1

// ===========================================
// CAMERA CONFIGURATION
// ===========================================

#define CAMERA_WIDTH 320
#define CAMERA_HEIGHT 240
#define CAMERA_FORMAT RGB565

#define FRAME_BUFFER_SIZE (CAMERA_WIDTH * CAMERA_HEIGHT * 2)  // RGB565

// ===========================================
// ML MODEL CONFIGURATION
// ===========================================

// Model input/output dimensions
#define MODEL_INPUT_WIDTH 96
#define MODEL_INPUT_HEIGHT 96
#define MODEL_INPUT_CHANNELS 3  // RGB

// Detection threshold
#define DETECTION_THRESHOLD 0.5f  // 50% confidence
#define CONFIDENCE_THRESHOLD 0.6f  // 60% for alarm

// Detection classes (example for person detection)
#define CLASS_PERSON 0
#define CLASS_VEHICLE 1
#define CLASS_ANIMAL 2
#define NUM_CLASSES 3

// Class names
const char* classNames[NUM_CLASSES] = {
  "Person",
  "Vehicle",
  "Animal"
};

// ===========================================
// ALARM CONFIGURATION
// ===========================================

// Alarm pins
#define ALARM_LED_PIN LED_BUILTIN
#define ALARM_BUZZER_PIN 2

// Alarm duration
#define ALARM_DURATION_MS 5000  // 5 seconds

// ===========================================
// DETECTION RESULT STRUCTURE
// ===========================================

struct DetectionResult {
  uint8_t cameraId;
  uint8_t classId;
  float confidence;
  uint16_t boundingBox[4];  // [x, y, width, height]
  uint32_t timestamp;
  bool valid;
};

// ===========================================
// GLOBAL VARIABLES
// ===========================================

// Frame buffers for both cameras
uint8_t frameBuffer1[FRAME_BUFFER_SIZE];
uint8_t frameBuffer2[FRAME_BUFFER_SIZE];

// Current active camera
uint8_t activeCamera = CAMERA_1_ID;

// Detection results
DetectionResult lastDetection1 = {0};
DetectionResult lastDetection2 = {0};

// Alarm state
bool alarmActive = false;
unsigned long alarmStartTime = 0;

// Detection statistics
uint32_t totalDetections = 0;
uint32_t detectionCounts[NUM_CLASSES] = {0};

// ===========================================
// TENSORFLOW LITE SETUP
// ===========================================

#include <tensorflow/lite/micro/all_ops_resolver.h>
#include <tensorflow/lite/micro/micro_error_reporter.h>
#include <tensorflow/lite/micro/micro_interpreter.h>
#include <tensorflow/lite/schema/schema_generated.h>

// TFLite globals
tflite::ErrorReporter* error_reporter = nullptr;
const tflite::Model* model = nullptr;
tflite::MicroInterpreter* interpreter = nullptr;

// Tensor arena
constexpr int kTensorArenaSize = 500 * 1024;
uint8_t tensor_arena[kTensorArenaSize];

// ===========================================
// FUNCTION PROTOTYPES
// ===========================================

// I2C Multiplexer control
bool initializeI2CMultiplexer();
void switchToCamera(uint8_t cameraId);
bool testCameraConnection(uint8_t cameraId);

// Camera initialization
bool initializeCamera(uint8_t cameraId);
bool initializeBothCameras();

// Image capture
bool captureImage(uint8_t cameraId, uint8_t* buffer);
void resizeImage(uint8_t* src, int srcWidth, int srcHeight,
                 uint8_t* dst, int dstWidth, int dstHeight);

// ML model initialization
bool initializeMLModel();
void runInference(uint8_t* imageData, DetectionResult* result);

// Detection processing
void processDetections();
bool shouldTriggerAlarm(const DetectionResult& result);
void triggerAlarm();
void updateAlarm();

// Utility functions
void printDetectionResult(const DetectionResult& result);
float getAverageConfidence();

// ===========================================
// I2C MULTIPLEXER FUNCTIONS
// ===========================================

bool initializeI2CMultiplexer() {
  Serial.println("Initializing I2C multiplexer...");

  Wire.begin();

  // Test connection to TCA9548A
  Wire.beginTransmission(TCA9548A_ADDR);
  uint8_t error = Wire.endTransmission();

  if (error != 0) {
    Serial.println("  ERROR: TCA9548A not found!");
    return false;
  }

  Serial.println("  TCA9548A detected at 0x70");

  // Test both camera channels
  if (!testCameraConnection(CAMERA_1_CHANNEL)) {
    Serial.println("  WARNING: Camera 1 not detected on channel 0");
  } else {
    Serial.println("  Camera 1 detected on channel 0");
  }

  if (!testCameraConnection(CAMERA_2_CHANNEL)) {
    Serial.println("  WARNING: Camera 2 not detected on channel 1");
  } else {
    Serial.println("  Camera 2 detected on channel 1");
  }

  return true;
}

void switchToCamera(uint8_t cameraId) {
  uint8_t channel = (cameraId == CAMERA_1_ID) ? CAMERA_1_CHANNEL : CAMERA_2_CHANNEL;

  // Switch multiplexer to specified channel
  Wire.beginTransmission(TCA9548A_ADDR);
  Wire.write(1 << channel);
  Wire.endTransmission();

  activeCamera = cameraId;

  // Small delay for I2C lines to settle
  delay(10);
}

bool testCameraConnection(uint8_t channel) {
  // Switch to channel
  Wire.beginTransmission(TCA9548A_ADDR);
  Wire.write(1 << channel);
  Wire.endTransmission();

  delay(10);

  // Try to communicate with camera (scan I2C bus)
  Wire.beginTransmission(0x3C);  // Typical camera I2C address
  uint8_t error = Wire.endTransmission();

  return (error == 0);
}

// ===========================================
// CAMERA FUNCTIONS
// ===========================================

bool initializeCamera(uint8_t cameraId) {
  Serial.print("  Initializing Camera ");
  Serial.println(cameraId + 1);

  // Switch to this camera
  switchToCamera(cameraId);

  // Initialize camera
  if (!Camera.begin(CAMERA_WIDTH, CAMERA_HEIGHT, CAMERA_FORMAT, 30)) {
    Serial.print("  ERROR: Camera ");
    Serial.print(cameraId + 1);
    Serial.println(" initialization failed!");
    return false;
  }

  // Configure camera
  Camera.setExposure(0);  // Auto exposure
  Camera.setGain(0);     // Auto gain
  Camera.setWhiteBalance(0);

  Serial.print("  Camera ");
  Serial.print(cameraId + 1);
  Serial.println(" initialized successfully");

  return true;
}

bool initializeBothCameras() {
  Serial.println("Initializing dual camera system...");

  if (!initializeCamera(CAMERA_1_ID)) {
    return false;
  }

  if (!initializeCamera(CAMERA_2_ID)) {
    return false;
  }

  Serial.println("Dual camera system ready");

  return true;
}

bool captureImage(uint8_t cameraId, uint8_t* buffer) {
  // Switch to camera
  switchToCamera(cameraId);

  // Capture frame
  Camera.readFrame(buffer);

  // Validate capture
  if (buffer == nullptr) {
    Serial.print("ERROR: Camera ");
    Serial.print(cameraId + 1);
    Serial.println(" capture failed!");
    return false;
  }

  return true;
}

// ===========================================
// IMAGE RESIZING (NEAREST NEIGHBOR)
// ===========================================

void resizeImage(uint8_t* src, int srcWidth, int srcHeight,
                 uint8_t* dst, int dstWidth, int dstHeight) {
  float xRatio = (float)srcWidth / dstWidth;
  float yRatio = (float)srcHeight / dstHeight;

  for (int y = 0; y < dstHeight; y++) {
    for (int x = 0; x < dstWidth; x++) {
      int srcX = (int)(x * xRatio);
      int srcY = (int)(y * yRatio);
      int srcIdx = (srcY * srcWidth + srcX) * 2;  // RGB565
      int dstIdx = (y * dstWidth + x) * 3;       // RGB888

      // Convert RGB565 to RGB888
      uint16_t pixel = *((uint16_t*)&src[srcIdx]);
      uint8_t r = ((pixel >> 11) & 0x1F) << 3;
      uint8_t g = ((pixel >> 5) & 0x3F) << 2;
      uint8_t b = (pixel & 0x1F) << 3;

      dst[dstIdx] = r;
      dst[dstIdx + 1] = g;
      dst[dstIdx + 2] = b;
    }
  }
}

// ===========================================
// ML MODEL INITIALIZATION
// ===========================================

bool initializeMLModel() {
  Serial.println("Initializing ML model...");

  // Set up logging
  static tflite::MicroErrorReporter micro_error_reporter;
  error_reporter = &micro_error_reporter;

  // Load model (you need to include your model file)
  // This is a placeholder - replace with your actual model
  // model = tflite::GetModel(g_model);

  if (model->version() != TFLITE_SCHEMA_VERSION) {
    Serial.println("ERROR: Model schema version mismatch!");
    return false;
  }

  // Build interpreter
  static tflite::AllOpsResolver resolver;
  static tflite::MicroInterpreter static_interpreter(
    model, resolver, tensor_arena, kTensorArenaSize, error_reporter
  );

  interpreter = &static_interpreter;

  // Allocate tensors
  if (interpreter->AllocateTensors() != kTfLiteOk) {
    Serial.println("ERROR: Failed to allocate tensors!");
    return false;
  }

  // Get input tensor info
  TfLiteTensor* input = interpreter->input(0);
  Serial.print("  Input tensor size: ");
  Serial.print(input->dims->data[1]);
  Serial.print("x");
  Serial.println(input->dims->data[2]);

  Serial.println("ML model initialized successfully");

  return true;
}

// ===========================================
// INFERENCE
// ===========================================

void runInference(uint8_t* imageData, DetectionResult* result) {
  // Get input tensor
  TfLiteTensor* input = interpreter->input(0);

  // Copy and preprocess image data
  // Resize image to model input size
  uint8_t resizedImage[MODEL_INPUT_WIDTH * MODEL_INPUT_HEIGHT * MODEL_INPUT_CHANNELS];
  resizeImage(imageData, CAMERA_WIDTH, CAMERA_HEIGHT,
              resizedImage, MODEL_INPUT_WIDTH, MODEL_INPUT_HEIGHT);

  // Copy to input tensor
  memcpy(input->data.uint8, resizedImage, input->bytes);

  // Run inference
  unsigned long startTime = micros();
  TfLiteStatus invokeStatus = interpreter->Invoke();
  unsigned long endTime = micros();

  if (invokeStatus != kTfLiteOk) {
    Serial.println("ERROR: Inference failed!");
    result->valid = false;
    return;
  }

  // Get output tensor
  TfLiteTensor* output = interpreter->output(0);

  // Find highest confidence class
  float maxConfidence = 0.0f;
  int detectedClass = -1;

  for (int i = 0; i < NUM_CLASSES; i++) {
    float confidence = output->data.f[i];
    if (confidence > maxConfidence) {
      maxConfidence = confidence;
      detectedClass = i;
    }
  }

  // Populate result
  result->cameraId = activeCamera;
  result->classId = detectedClass;
  result->confidence = maxConfidence;
  result->timestamp = millis();
  result->valid = (maxConfidence >= DETECTION_THRESHOLD);

  // Print inference time
  Serial.print("Inference time: ");
  Serial.print((endTime - startTime) / 1000.0);
  Serial.println("ms");
}

// ===========================================
// DETECTION PROCESSING
// ===========================================

void processDetections() {
  // Capture from camera 1
  if (captureImage(CAMERA_1_ID, frameBuffer1)) {
    runInference(frameBuffer1, &lastDetection1);

    if (lastDetection1.valid) {
      totalDetections++;
      detectionCounts[lastDetection1.classId]++;
      printDetectionResult(lastDetection1);

      // Check for alarm
      if (shouldTriggerAlarm(lastDetection1)) {
        triggerAlarm();
      }
    }
  }

  // Capture from camera 2
  if (captureImage(CAMERA_2_ID, frameBuffer2)) {
    runInference(frameBuffer2, &lastDetection2);

    if (lastDetection2.valid) {
      totalDetections++;
      detectionCounts[lastDetection2.classId]++;
      printDetectionResult(lastDetection2);

      // Check for alarm
      if (shouldTriggerAlarm(lastDetection2)) {
        triggerAlarm();
      }
    }
  }
}

bool shouldTriggerAlarm(const DetectionResult& result) {
  // Trigger alarm for high-confidence person detection
  if (result.classId == CLASS_PERSON &&
      result.confidence >= CONFIDENCE_THRESHOLD) {
    return true;
  }

  // Add other alarm conditions here

  return false;
}

void triggerAlarm() {
  Serial.println("ALARM TRIGGERED!");

  alarmActive = true;
  alarmStartTime = millis();

  // Turn on LED
  digitalWrite(ALARM_LED_PIN, HIGH);

  // Turn on buzzer
  digitalWrite(ALARM_BUZZER_PIN, HIGH);
}

void updateAlarm() {
  if (!alarmActive) return;

  // Check if alarm duration has elapsed
  if (millis() - alarmStartTime >= ALARM_DURATION_MS) {
    alarmActive = false;

    // Turn off LED
    digitalWrite(ALARM_LED_PIN, LOW);

    // Turn off buzzer
    digitalWrite(ALARM_BUZZER_PIN, LOW);

    Serial.println("Alarm deactivated");
  }
}

// ===========================================
// UTILITY FUNCTIONS
// ===========================================

void printDetectionResult(const DetectionResult& result) {
  Serial.print("Camera ");
  Serial.print(result.cameraId + 1);
  Serial.print(": ");
  Serial.print(classNames[result.classId]);
  Serial.print(" (");
  Serial.print(result.confidence * 100);
  Serial.println("%)");
}

float getAverageConfidence() {
  if (totalDetections == 0) return 0.0f;

  float sum = 0.0f;
  if (lastDetection1.valid) sum += lastDetection1.confidence;
  if (lastDetection2.valid) sum += lastDetection2.confidence;

  return sum / totalDetections;
}

// ===========================================
// MAIN SETUP AND LOOP
// ===========================================

void setup() {
  Serial.begin(115200);
  while (!Serial && millis() < 3000);

  // Initialize alarm pins
  pinMode(ALARM_LED_PIN, OUTPUT);
  pinMode(ALARM_BUZZER_PIN, OUTPUT);
  digitalWrite(ALARM_LED_PIN, LOW);
  digitalWrite(ALARM_BUZZER_PIN, LOW);

  // Initialize I2C multiplexer
  if (!initializeI2CMultiplexer()) {
    Serial.println("ERROR: I2C multiplexer initialization failed!");
    while (1);  // Halt
  }

  // Initialize cameras
  if (!initializeBothCameras()) {
    Serial.println("ERROR: Camera initialization failed!");
    while (1);  // Halt
  }

  // Initialize ML model
  if (!initializeMLModel()) {
    Serial.println("ERROR: ML model initialization failed!");
    while (1);  // Halt
  }

  Serial.println("==================================");
  Serial.println("Dual Camera Object Detection");
  Serial.println("Ready to detect objects!");
  Serial.println("==================================");
}

void loop() {
  // Update alarm state
  updateAlarm();

  // Process detections
  processDetections();

  // Small delay
  delay(1000);
}
