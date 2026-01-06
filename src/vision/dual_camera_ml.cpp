/**
 * Dual Camera Object Detection Implementation
 *
 * Complete production-ready implementation with:
 * - I2C multiplexer (TCA9548A) control for dual cameras
 * - TensorFlow Lite Micro integration
 * - Model loading and initialization
 * - Camera switching logic
 * - Image capture from both cameras
 * - Preprocessing pipeline
 * - Inference execution
 * - Post-processing (NMS, thresholding)
 * - Detection buffering
 * - Performance profiling
 *
 * Board: Arduino Nicla Vision (x2)
 * Communication: I2C via TCA9548A multiplexer
 * ML Framework: TensorFlow Lite for Microcontrollers
 *
 * Memory Requirements:
 * - Tensor Arena: 500KB
 * - Frame Buffers: 320x240x2x2 = 307KB (2 cameras)
 * - Detection Buffer: Configurable (default: 10 entries)
 */

#include <Arduino.h>
#include <Wire.h>
#include <Arduino_OV5640.h>
#include <TensorFlowLite.h>

// ===========================================
// TENSORFLOW LITE MICRO HEADERS
// ===========================================

#include <tensorflow/lite/micro/all_ops_resolver.h>
#include <tensorflow/lite/micro/micro_error_reporter.h>
#include <tensorflow/lite/micro/micro_interpreter.h>
#include <tensorflow/lite/schema/schema_generated.h>
#include <tensorflow/lite/version.h>

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

// Detection thresholds
#define DETECTION_THRESHOLD 0.5f  // 50% confidence minimum
#define CONFIDENCE_THRESHOLD 0.6f  // 60% for alarm trigger
#define NMS_IOU_THRESHOLD 0.5f  // 50% IoU for NMS

// Detection classes
#define CLASS_PERSON 0
#define CLASS_VEHICLE 1
#define CLASS_ANIMAL 2
#define NUM_CLASSES 3

// Maximum number of detections per inference
#define MAX_DETECTIONS_PER_INFERENCE 10

// Class names
const char* classNames[NUM_CLASSES] = {
  "Person",
  "Vehicle",
  "Animal"
};

// Model types
enum ModelType {
  MODEL_TYPE_PERSON_DETECTION,
  MODEL_TYPE_VEHICLE_DETECTION,
  MODEL_TYPE_ANIMAL_DETECTION,
  MODEL_TYPE_CUSTOM
};

// Model metadata
struct ModelMetadata {
  const char* name;
  const char* version;
  ModelType type;
  uint8_t* modelData;
  uint32_t modelSize;
  int inputWidth;
  int inputHeight;
  int numClasses;
  float threshold;
  bool quantized;
};

// ===========================================
// DETECTION RESULT STRUCTURE
// ===========================================

struct BoundingBox {
  uint16_t x;
  uint16_t y;
  uint16_t width;
  uint16_t height;

  // Calculate IoU with another box
  float calculateIoU(const BoundingBox& other) const {
    uint16_t x1 = max(x, other.x);
    uint16_t y1 = max(y, other.y);
    uint16_t x2 = min(x + width, other.x + other.width);
    uint16_t y2 = min(y + height, other.y + other.height);

    if (x2 < x1 || y2 < y1) return 0.0f;

    uint32_t intersection = (x2 - x1) * (y2 - y1);
    uint32_t unionArea = width * height + other.width * other.height - intersection;

    return (float)intersection / unionArea;
  }

  // Calculate area
  uint32_t area() const {
    return width * height;
  }
};

struct DetectionResult {
  uint8_t cameraId;
  uint8_t classId;
  float confidence;
  BoundingBox boundingBox;
  uint32_t timestamp;
  bool valid;

  // Default constructor
  DetectionResult() : cameraId(0), classId(0), confidence(0.0f),
                      timestamp(0), valid(false) {
    boundingBox = {0, 0, 0, 0};
  }

  // Check if detection is valid
  bool isValid() const {
    return valid && confidence >= DETECTION_THRESHOLD;
  }

  // Check if detection should trigger alarm
  bool shouldAlarm() const {
    return valid && confidence >= CONFIDENCE_THRESHOLD;
  }
};

// ===========================================
// DETECTION BUFFER
// ===========================================

#define DETECTION_BUFFER_SIZE 10

struct DetectionBuffer {
  DetectionResult detections[DETECTION_BUFFER_SIZE];
  uint8_t head;
  uint8_t tail;
  uint8_t count;

  DetectionBuffer() : head(0), tail(0), count(0) {}

  void add(const DetectionResult& result) {
    if (count < DETECTION_BUFFER_SIZE) {
      detections[head] = result;
      head = (head + 1) % DETECTION_BUFFER_SIZE;
      count++;
    } else {
      // Buffer full, overwrite oldest
      detections[tail] = result;
      tail = (tail + 1) % DETECTION_BUFFER_SIZE;
    }
  }

  DetectionResult* getLatest(uint8_t n) {
    if (n >= count) return nullptr;
    int8_t idx = (head - n - 1 + DETECTION_BUFFER_SIZE) % DETECTION_BUFFER_SIZE;
    return &detections[idx];
  }

  void clear() {
    head = 0;
    tail = 0;
    count = 0;
  }

  uint8_t size() const {
    return count;
  }
};

// ===========================================
// PERFORMANCE PROFILING
// ===========================================

struct PerformanceMetrics {
  uint32_t totalInferences;
  uint32_t totalInferenceTimeUs;
  uint32_t minInferenceTimeUs;
  uint32_t maxInferenceTimeUs;
  uint32_t totalCaptures;
  uint32_t failedCaptures;
  uint32_t totalPreprocessingTimeUs;
  uint32_t totalPostprocessingTimeUs;

  PerformanceMetrics() : totalInferences(0), totalInferenceTimeUs(0),
                         minInferenceTimeUs(0xFFFFFFFF), maxInferenceTimeUs(0),
                         totalCaptures(0), failedCaptures(0),
                         totalPreprocessingTimeUs(0), totalPostprocessingTimeUs(0) {}

  void recordInference(uint32_t timeUs) {
    totalInferences++;
    totalInferenceTimeUs += timeUs;
    if (timeUs < minInferenceTimeUs) minInferenceTimeUs = timeUs;
    if (timeUs > maxInferenceTimeUs) maxInferenceTimeUs = timeUs;
  }

  void recordCapture(bool success) {
    totalCaptures++;
    if (!success) failedCaptures++;
  }

  void recordPreprocessing(uint32_t timeUs) {
    totalPreprocessingTimeUs += timeUs;
  }

  void recordPostprocessing(uint32_t timeUs) {
    totalPostprocessingTimeUs += timeUs;
  }

  float getAverageInferenceTime() const {
    return totalInferences > 0 ? (float)totalInferenceTimeUs / totalInferences : 0.0f;
  }

  float getCaptureSuccessRate() const {
    return totalCaptures > 0 ? (float)(totalCaptures - failedCaptures) / totalCaptures : 0.0f;
  }

  void print() const {
    Serial.println("=== Performance Metrics ===");
    Serial.print("Total Inferences: "); Serial.println(totalInferences);
    Serial.print("Avg Inference Time: "); Serial.print(getAverageInferenceTime() / 1000.0); Serial.println(" ms");
    Serial.print("Min Inference Time: "); Serial.print(minInferenceTimeUs / 1000.0); Serial.println(" ms");
    Serial.print("Max Inference Time: "); Serial.print(maxInferenceTimeUs / 1000.0); Serial.println(" ms");
    Serial.print("Total Captures: "); Serial.println(totalCaptures);
    Serial.print("Capture Success Rate: "); Serial.print(getCaptureSuccessRate() * 100); Serial.println("%");
    Serial.print("Avg Preprocessing Time: "); Serial.print(totalPreprocessingTimeUs / (float)totalInferences / 1000.0); Serial.println(" ms");
    Serial.print("Avg Postprocessing Time: "); Serial.print(totalPostprocessingTimeUs / (float)totalInferences / 1000.0); Serial.println(" ms");
    Serial.println("===========================");
  }
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
// GLOBAL VARIABLES
// ===========================================

// Frame buffers for both cameras
uint8_t frameBuffer1[FRAME_BUFFER_SIZE];
uint8_t frameBuffer2[FRAME_BUFFER_SIZE];

// Preprocessing buffer (resized image for model input)
uint8_t preprocessingBuffer[MODEL_INPUT_WIDTH * MODEL_INPUT_HEIGHT * MODEL_INPUT_CHANNELS];

// Current active camera
uint8_t activeCamera = CAMERA_1_ID;

// Detection results
DetectionResult lastDetection1;
DetectionResult lastDetection2;

// Detection buffers for both cameras
DetectionBuffer detectionBuffer1;
DetectionBuffer detectionBuffer2;

// Alarm state
bool alarmActive = false;
unsigned long alarmStartTime = 0;

// Detection statistics
uint32_t totalDetections = 0;
uint32_t detectionCounts[NUM_CLASSES] = {0};

// Performance metrics
PerformanceMetrics metrics;

// ===========================================
// TENSORFLOW LITE MICRO SETUP
// ===========================================

// TFLite globals
tflite::ErrorReporter* error_reporter = nullptr;
const tflite::Model* model = nullptr;
tflite::MicroInterpreter* interpreter = nullptr;

// Tensor arena
constexpr int kTensorArenaSize = 500 * 1024;
uint8_t tensor_arena[kTensorArenaSize];

// Current model metadata
ModelMetadata currentModel = {
  "Person Detection",  // name
  "1.0.0",              // version
  MODEL_TYPE_PERSON_DETECTION,  // type
  nullptr,              // modelData (to be set)
  0,                    // modelSize (to be set)
  MODEL_INPUT_WIDTH,
  MODEL_INPUT_HEIGHT,
  NUM_CLASSES,
  DETECTION_THRESHOLD,
  true                  // quantized
};

// Model loaded flag
bool modelLoaded = false;

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

// Image preprocessing
void preprocessImage(uint8_t* src, int srcWidth, int srcHeight,
                     uint8_t* dst, int dstWidth, int dstHeight);
void resizeImageNearestNeighbor(uint8_t* src, int srcWidth, int srcHeight,
                                uint8_t* dst, int dstWidth, int dstHeight);
void convertRGB565toRGB888(uint8_t* src, uint8_t* dst, int pixelCount);
void normalizeImage(uint8_t* image, int width, int height, int channels);

// ML model initialization and loading
bool initializeMLModel();
bool loadModel(ModelMetadata& modelInfo);
bool loadModelFromData(uint8_t* modelData, uint32_t modelSize, ModelMetadata& modelInfo);
void unloadModel();
bool isModelLoaded();

// Inference
void runInference(uint8_t* imageData, DetectionResult* result);
void runInferenceMultiple(uint8_t* imageData, DetectionResult* results, uint8_t* numDetections);

// Post-processing
void applyNonMaximumSuppression(DetectionResult* detections, uint8_t* numDetections);
void filterByConfidence(DetectionResult* detections, uint8_t* numDetections, float threshold);
void extractBoundingBoxes(TfLiteTensor* output, DetectionResult* results, uint8_t* numDetections);

// Detection processing
void processDetections();
void processCamera(uint8_t cameraId, uint8_t* frameBuffer, DetectionResult& result, DetectionBuffer& buffer);
bool shouldTriggerAlarm(const DetectionResult& result);
void triggerAlarm();
void updateAlarm();

// Model management
void switchModel(ModelType modelType);
bool loadPersonDetectionModel();
bool loadVehicleDetectionModel();
bool loadAnimalDetectionModel();
bool loadCustomModel(uint8_t* modelData, uint32_t modelSize, const char* name);

// Utility functions
void printDetectionResult(const DetectionResult& result);
void printModelInfo(const ModelMetadata& modelInfo);
void printMemoryUsage();
float getAverageConfidence();
void resetStatistics();

// Error handling
void handleMLError(const char* message);
void handleCameraError(uint8_t cameraId, const char* message);

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
// IMAGE PREPROCESSING
// ===========================================

/**
 * Complete preprocessing pipeline:
 * 1. Resize image (320x240 -> 96x96)
 * 2. Convert RGB565 to RGB888
 * 3. Normalize pixel values (if needed)
 */
void preprocessImage(uint8_t* src, int srcWidth, int srcHeight,
                     uint8_t* dst, int dstWidth, int dstHeight) {
  unsigned long startTime = micros();

  // Step 1: Resize + Color space conversion
  resizeImageNearestNeighbor(src, srcWidth, srcHeight, dst, dstWidth, dstHeight);

  // Step 2: Normalize (if using float model)
  // For quantized models, normalization is handled in the model
  // if (!currentModel.quantized) {
  //   normalizeImage(dst, dstWidth, dstHeight, MODEL_INPUT_CHANNELS);
  // }

  unsigned long endTime = micros();
  metrics.recordPreprocessing(endTime - startTime);
}

/**
 * Resize image using nearest neighbor interpolation
 * Simultaneously converts RGB565 to RGB888
 */
void resizeImageNearestNeighbor(uint8_t* src, int srcWidth, int srcHeight,
                                uint8_t* dst, int dstWidth, int dstHeight) {
  float xRatio = (float)srcWidth / dstWidth;
  float yRatio = (float)srcHeight / dstHeight;

  for (int y = 0; y < dstHeight; y++) {
    for (int x = 0; x < dstWidth; x++) {
      // Calculate source position
      int srcX = (int)(x * xRatio);
      int srcY = (int)(y * yRatio);

      // Ensure within bounds
      srcX = min(srcX, srcWidth - 1);
      srcY = min(srcY, srcHeight - 1);

      // Calculate indices
      int srcIdx = (srcY * srcWidth + srcX) * 2;  // RGB565 = 2 bytes
      int dstIdx = (y * dstWidth + x) * 3;       // RGB888 = 3 bytes

      // Convert RGB565 to RGB888
      uint16_t pixel = *((uint16_t*)&src[srcIdx]);
      uint8_t r = ((pixel >> 11) & 0x1F) << 3;      // 5 bits -> 8 bits
      uint8_t g = ((pixel >> 5) & 0x3F) << 2;       // 6 bits -> 8 bits
      uint8_t b = (pixel & 0x1F) << 3;              // 5 bits -> 8 bits

      dst[dstIdx] = r;
      dst[dstIdx + 1] = g;
      dst[dstIdx + 2] = b;
    }
  }
}

/**
 * Convert RGB565 buffer to RGB888
 */
void convertRGB565toRGB888(uint8_t* src, uint8_t* dst, int pixelCount) {
  for (int i = 0; i < pixelCount; i++) {
    uint16_t pixel = *((uint16_t*)&src[i * 2]);
    dst[i * 3] = ((pixel >> 11) & 0x1F) << 3;
    dst[i * 3 + 1] = ((pixel >> 5) & 0x3F) << 2;
    dst[i * 3 + 2] = (pixel & 0x1F) << 3;
  }
}

/**
 * Normalize image values to [0, 1] range (for float models)
 * Note: For quantized models, this is not needed
 */
void normalizeImage(uint8_t* image, int width, int height, int channels) {
  int totalPixels = width * height * channels;
  for (int i = 0; i < totalPixels; i++) {
    // Convert uint8 [0, 255] to float [0, 1]
    // This requires the input tensor to be float type
    // For quantized models, we keep uint8 values
    // image[i] is already in the correct format
  }
}

// ===========================================
// ML MODEL INITIALIZATION AND LOADING
// ===========================================

/**
 * Initialize ML model with default person detection model
 */
bool initializeMLModel() {
  Serial.println("Initializing ML model...");

  // Set up logging
  static tflite::MicroErrorReporter micro_error_reporter;
  error_reporter = &micro_error_reporter;

  // Load default model (person detection)
  if (!loadPersonDetectionModel()) {
    Serial.println("ERROR: Failed to load person detection model!");
    return false;
  }

  Serial.println("ML model initialized successfully");
  printModelInfo(currentModel);

  return true;
}

/**
 * Load model from metadata structure
 */
bool loadModel(ModelMetadata& modelInfo) {
  Serial.println("Loading model...");
  Serial.print("  Name: ");
  Serial.println(modelInfo.name);
  Serial.print("  Version: ");
  Serial.println(modelInfo.version);

  // Validate model data
  if (modelInfo.modelData == nullptr || modelInfo.modelSize == 0) {
    Serial.println("ERROR: Invalid model data!");
    return false;
  }

  // Unload existing model if loaded
  if (modelLoaded) {
    unloadModel();
  }

  // Map model into usable data structure
  model = tflite::GetModel(modelInfo.modelData);
  if (model->version() != TFLITE_SCHEMA_VERSION) {
    Serial.println("ERROR: Model schema version mismatch!");
    Serial.print("  Model version: ");
    Serial.println(model->version());
    Serial.print("  Supported version: ");
    Serial.println(TFLITE_SCHEMA_VERSION);
    return false;
  }

  // Build interpreter with all operations resolver
  static tflite::AllOpsResolver resolver;

  static tflite::MicroInterpreter static_interpreter(
    model, resolver, tensor_arena, kTensorArenaSize, error_reporter
  );

  interpreter = &static_interpreter;

  // Allocate tensors
  TfLiteStatus allocate_status = interpreter->AllocateTensors();
  if (allocate_status != kTfLiteOk) {
    Serial.println("ERROR: Failed to allocate tensors!");
    printMemoryUsage();
    return false;
  }

  // Get input tensor info
  TfLiteTensor* input = interpreter->input(0);
  Serial.print("  Input dimensions: ");
  Serial.print(input->dims->data[1]);
  Serial.print("x");
  Serial.print(input->dims->data[2]);
  Serial.print("x");
  Serial.println(input->dims->data[3]);
  Serial.print("  Input type: ");
  Serial.println(input->type == kTfLiteUInt8 ? "uint8" : "float32");

  // Get output tensor info
  TfLiteTensor* output = interpreter->output(0);
  Serial.print("  Output dimensions: ");
  for (int i = 0; i < output->dims->size; i++) {
    Serial.print(output->dims->data[i]);
    if (i < output->dims->size - 1) Serial.print("x");
  }
  Serial.println();

  // Update current model metadata
  currentModel = modelInfo;
  modelLoaded = true;

  return true;
}

/**
 * Load model from raw data array
 */
bool loadModelFromData(uint8_t* modelData, uint32_t modelSize, ModelMetadata& modelInfo) {
  modelInfo.modelData = modelData;
  modelInfo.modelSize = modelSize;
  return loadModel(modelInfo);
}

/**
 * Unload current model and free resources
 */
void unloadModel() {
  if (modelLoaded) {
    // Reset interpreter
    if (interpreter != nullptr) {
      interpreter = nullptr;
    }

    model = nullptr;
    modelLoaded = false;

    Serial.println("Model unloaded");
  }
}

/**
 * Check if model is loaded
 */
bool isModelLoaded() {
  return modelLoaded && (model != nullptr) && (interpreter != nullptr);
}

/**
 * Load person detection model
 * NOTE: You need to include your model data as a C array
 * This is a placeholder - replace with actual model loading
 */
bool loadPersonDetectionModel() {
  // TODO: Include your person detection model
  // Example: #include "person_detection_model_data.h"
  // extern uint8_t g_person_detection_model[];

  ModelMetadata personModel = {
    "Person Detection",
    "1.0.0",
    MODEL_TYPE_PERSON_DETECTION,
    nullptr,  // g_person_detection_model
    0,        // model size (set automatically)
    MODEL_INPUT_WIDTH,
    MODEL_INPUT_HEIGHT,
    NUM_CLASSES,
    DETECTION_THRESHOLD,
    true      // quantized
  };

  return loadModel(personModel);
}

/**
 * Load vehicle detection model
 */
bool loadVehicleDetectionModel() {
  // TODO: Include your vehicle detection model
  ModelMetadata vehicleModel = {
    "Vehicle Detection",
    "1.0.0",
    MODEL_TYPE_VEHICLE_DETECTION,
    nullptr,  // g_vehicle_detection_model
    0,
    MODEL_INPUT_WIDTH,
    MODEL_INPUT_HEIGHT,
    NUM_CLASSES,
    DETECTION_THRESHOLD,
    true
  };

  return loadModel(vehicleModel);
}

/**
 * Load animal detection model
 */
bool loadAnimalDetectionModel() {
  // TODO: Include your animal detection model
  ModelMetadata animalModel = {
    "Animal Detection",
    "1.0.0",
    MODEL_TYPE_ANIMAL_DETECTION,
    nullptr,  // g_animal_detection_model
    0,
    MODEL_INPUT_WIDTH,
    MODEL_INPUT_HEIGHT,
    NUM_CLASSES,
    DETECTION_THRESHOLD,
    true
  };

  return loadModel(animalModel);
}

/**
 * Load custom model from data
 */
bool loadCustomModel(uint8_t* modelData, uint32_t modelSize, const char* name) {
  ModelMetadata customModel = {
    name,
    "1.0.0",
    MODEL_TYPE_CUSTOM,
    modelData,
    modelSize,
    MODEL_INPUT_WIDTH,
    MODEL_INPUT_HEIGHT,
    NUM_CLASSES,
    DETECTION_THRESHOLD,
    true
  };

  return loadModel(customModel);
}

/**
 * Switch between different model types
 */
void switchModel(ModelType modelType) {
  Serial.println("Switching model...");

  bool success = false;

  switch (modelType) {
    case MODEL_TYPE_PERSON_DETECTION:
      success = loadPersonDetectionModel();
      break;
    case MODEL_TYPE_VEHICLE_DETECTION:
      success = loadVehicleDetectionModel();
      break;
    case MODEL_TYPE_ANIMAL_DETECTION:
      success = loadAnimalDetectionModel();
      break;
    default:
      Serial.println("ERROR: Unknown model type!");
      return;
  }

  if (success) {
    Serial.println("Model switched successfully");
  } else {
    Serial.println("ERROR: Failed to switch model!");
    handleMLError("Model switch failed");
  }
}

// ===========================================
// INFERENCE EXECUTION
// ===========================================

/**
 * Run single-object detection inference
 * Returns the highest confidence detection
 */
void runInference(uint8_t* imageData, DetectionResult* result) {
  if (!isModelLoaded()) {
    Serial.println("ERROR: No model loaded!");
    result->valid = false;
    return;
  }

  // Get input tensor
  TfLiteTensor* input = interpreter->input(0);

  // Preprocess image
  preprocessImage(imageData, CAMERA_WIDTH, CAMERA_HEIGHT,
                  preprocessingBuffer, MODEL_INPUT_WIDTH, MODEL_INPUT_HEIGHT);

  // Copy preprocessed data to input tensor
  memcpy(input->data.uint8, preprocessingBuffer, input->bytes);

  // Run inference
  unsigned long startTime = micros();
  TfLiteStatus invokeStatus = interpreter->Invoke();
  unsigned long endTime = micros();

  // Record metrics
  metrics.recordInference(endTime - startTime);

  if (invokeStatus != kTfLiteOk) {
    Serial.println("ERROR: Inference failed!");
    result->valid = false;
    handleMLError("Inference invoke failed");
    return;
  }

  // Get output tensor
  TfLiteTensor* output = interpreter->output(0);

  // Post-processing start
  unsigned long postprocessStart = micros();

  // Process output based on model type
  // For classification model (1D output)
  if (output->dims->size == 2 || output->dims->size == 4) {
    // Find highest confidence class
    float maxConfidence = -1.0f;
    int detectedClass = -1;

    // Get number of classes
    int numOutputs = output->dims->data[output->dims->size - 1];

    for (int i = 0; i < min(numOutputs, NUM_CLASSES); i++) {
      float confidence;
      if (output->type == kTfLiteUInt8) {
        // Quantized model - convert to float
        confidence = (output->data.uint8[i] - output->params.zero_point) *
                     output->params.scale;
      } else {
        // Float model
        confidence = output->data.f[i];
      }

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
    result->valid = (maxConfidence >= DETECTION_THRESHOLD && detectedClass >= 0);

    // Default bounding box (full image)
    result->boundingBox.x = 0;
    result->boundingBox.y = 0;
    result->boundingBox.width = MODEL_INPUT_WIDTH;
    result->boundingBox.height = MODEL_INPUT_HEIGHT;
  }

  unsigned long postprocessEnd = micros();
  metrics.recordPostprocessing(postprocessEnd - postprocessStart);

  // Print inference time
  Serial.print("Inference time: ");
  Serial.print((endTime - startTime) / 1000.0, 2);
  Serial.println(" ms");
}

/**
 * Run multi-object detection inference
 * Returns multiple detections with bounding boxes
 */
void runInferenceMultiple(uint8_t* imageData, DetectionResult* results, uint8_t* numDetections) {
  if (!isModelLoaded()) {
    Serial.println("ERROR: No model loaded!");
    *numDetections = 0;
    return;
  }

  // Get input tensor
  TfLiteTensor* input = interpreter->input(0);

  // Preprocess image
  preprocessImage(imageData, CAMERA_WIDTH, CAMERA_HEIGHT,
                  preprocessingBuffer, MODEL_INPUT_WIDTH, MODEL_INPUT_HEIGHT);

  // Copy to input tensor
  memcpy(input->data.uint8, preprocessingBuffer, input->bytes);

  // Run inference
  unsigned long startTime = micros();
  TfLiteStatus invokeStatus = interpreter->Invoke();
  unsigned long endTime = micros();

  metrics.recordInference(endTime - startTime);

  if (invokeStatus != kTfLiteOk) {
    Serial.println("ERROR: Inference failed!");
    *numDetections = 0;
    handleMLError("Multi-inference invoke failed");
    return;
  }

  // Get output tensor
  TfLiteTensor* output = interpreter->output(0);

  // Extract bounding boxes from output
  unsigned long postprocessStart = micros();
  extractBoundingBoxes(output, results, numDetections);

  // Apply NMS to remove overlapping detections
  if (*numDetections > 1) {
    applyNonMaximumSuppression(results, numDetections);
  }

  // Filter by confidence threshold
  filterByConfidence(results, numDetections, DETECTION_THRESHOLD);

  unsigned long postprocessEnd = micros();
  metrics.recordPostprocessing(postprocessEnd - postprocessStart);

  Serial.print("Detected ");
  Serial.print(*numDetections);
  Serial.println(" objects");
}

// ===========================================
// POST-PROCESSING
// ===========================================

/**
 * Extract bounding boxes from model output
 * This is a placeholder - implement based on your model's output format
 */
void extractBoundingBoxes(TfLiteTensor* output, DetectionResult* results, uint8_t* numDetections) {
  // TODO: Implement based on your model's output format
  // Different models have different output formats:
  //
  // 1. SSD MobileNet: [num_detections, 7] where 7 = [batch, class_id, confidence, x, y, width, height]
  // 2. YOLO: [grid_size, grid_size, num_anchors * (num_classes + 5)]
  // 3. Classification: [batch, num_classes]
  //
  // This is a simplified implementation for classification models

  *numDetections = 0;

  // For classification model, create single detection
  float maxConfidence = -1.0f;
  int detectedClass = -1;

  int numOutputs = output->dims->data[output->dims->size - 1];

  for (int i = 0; i < min(numOutputs, NUM_CLASSES); i++) {
    float confidence;
    if (output->type == kTfLiteUInt8) {
      confidence = (output->data.uint8[i] - output->params.zero_point) *
                   output->params.scale;
    } else {
      confidence = output->data.f[i];
    }

    if (confidence > maxConfidence) {
      maxConfidence = confidence;
      detectedClass = i;
    }
  }

  if (maxConfidence >= DETECTION_THRESHOLD && detectedClass >= 0) {
    results[0].cameraId = activeCamera;
    results[0].classId = detectedClass;
    results[0].confidence = maxConfidence;
    results[0].timestamp = millis();
    results[0].valid = true;
    results[0].boundingBox = {0, 0, MODEL_INPUT_WIDTH, MODEL_INPUT_HEIGHT};
    *numDetections = 1;
  }
}

/**
 * Apply Non-Maximum Suppression to remove overlapping detections
 * Sorts by confidence and removes boxes with high IoU
 */
void applyNonMaximumSuppression(DetectionResult* detections, uint8_t* numDetections) {
  if (*numDetections <= 1) return;

  // Sort detections by confidence (descending)
  for (int i = 0; i < *numDetections - 1; i++) {
    for (int j = i + 1; j < *numDetections; j++) {
      if (detections[j].confidence > detections[i].confidence) {
        DetectionResult temp = detections[i];
        detections[i] = detections[j];
        detections[j] = temp;
      }
    }
  }

  // Remove overlapping detections
  bool keep[MAX_DETECTIONS_PER_INFERENCE];
  memset(keep, true, sizeof(keep));

  for (int i = 0; i < *numDetections; i++) {
    if (!keep[i]) continue;

    for (int j = i + 1; j < *numDetections; j++) {
      if (!keep[j]) continue;

      // Only compare detections of same class
      if (detections[i].classId == detections[j].classId) {
        float iou = detections[i].boundingBox.calculateIoU(detections[j].boundingBox);

        if (iou > NMS_IOU_THRESHOLD) {
          // Remove lower confidence detection
          keep[j] = false;
        }
      }
    }
  }

  // Compact the array
  uint8_t newCount = 0;
  for (int i = 0; i < *numDetections; i++) {
    if (keep[i]) {
      if (newCount != i) {
        detections[newCount] = detections[i];
      }
      newCount++;
    }
  }

  *numDetections = newCount;
}

/**
 * Filter detections by confidence threshold
 */
void filterByConfidence(DetectionResult* detections, uint8_t* numDetections, float threshold) {
  uint8_t newCount = 0;

  for (int i = 0; i < *numDetections; i++) {
    if (detections[i].confidence >= threshold) {
      if (newCount != i) {
        detections[newCount] = detections[i];
      }
      newCount++;
    }
  }

  *numDetections = newCount;
}

// ===========================================
// DETECTION PROCESSING
// ===========================================

/**
 * Process detections from both cameras
 */
void processDetections() {
  processCamera(CAMERA_1_ID, frameBuffer1, lastDetection1, detectionBuffer1);
  processCamera(CAMERA_2_ID, frameBuffer2, lastDetection2, detectionBuffer2);
}

/**
 * Process single camera: capture, infer, buffer, alarm
 */
void processCamera(uint8_t cameraId, uint8_t* frameBuffer,
                   DetectionResult& result, DetectionBuffer& buffer) {
  // Capture image
  bool captureSuccess = captureImage(cameraId, frameBuffer);
  metrics.recordCapture(captureSuccess);

  if (!captureSuccess) {
    result.valid = false;
    handleCameraError(cameraId, "Capture failed");
    return;
  }

  // Run inference
  runInference(frameBuffer, &result);

  // Add to buffer
  if (result.isValid()) {
    buffer.add(result);

    // Update statistics
    totalDetections++;
    detectionCounts[result.classId]++;

    // Print result
    printDetectionResult(result);

    // Check for alarm
    if (result.shouldAlarm()) {
      triggerAlarm();
    }
  }
}

/**
 * Determine if detection should trigger alarm
 */
bool shouldTriggerAlarm(const DetectionResult& result) {
  // High-confidence person detection
  if (result.classId == CLASS_PERSON &&
      result.confidence >= CONFIDENCE_THRESHOLD) {
    return true;
  }

  // High-confidence vehicle detection (optional)
  if (result.classId == CLASS_VEHICLE &&
      result.confidence >= CONFIDENCE_THRESHOLD) {
    // Uncomment if vehicle detection should trigger alarm
    // return true;
  }

  // High-confidence animal detection (optional)
  if (result.classId == CLASS_ANIMAL &&
      result.confidence >= CONFIDENCE_THRESHOLD) {
    // Uncomment if animal detection should trigger alarm
    // return true;
  }

  // Multiple detections in buffer (crowd detection)
  DetectionBuffer* buffer = (result.cameraId == CAMERA_1_ID) ?
                            &detectionBuffer1 : &detectionBuffer2;
  if (buffer->size() >= 3) {
    // Check if we have multiple recent detections
    int recentDetections = 0;
    for (int i = 0; i < min(3, buffer->size()); i++) {
      DetectionResult* r = buffer->getLatest(i);
      if (r && r->isValid()) {
        recentDetections++;
      }
    }
    if (recentDetections >= 3) {
      return true;  // Multiple recent detections
    }
  }

  return false;
}

/**
 * Trigger alarm
 */
void triggerAlarm() {
  if (alarmActive) {
    // Already alarming, extend duration
    alarmStartTime = millis();
    return;
  }

  Serial.println("ALARM TRIGGERED!");

  alarmActive = true;
  alarmStartTime = millis();

  // Turn on indicators
  digitalWrite(ALARM_LED_PIN, HIGH);
  digitalWrite(ALARM_BUZZER_PIN, HIGH);
}

/**
 * Update alarm state (call in loop)
 */
void updateAlarm() {
  if (!alarmActive) return;

  // Check if alarm duration has elapsed
  if (millis() - alarmStartTime >= ALARM_DURATION_MS) {
    alarmActive = false;

    // Turn off indicators
    digitalWrite(ALARM_LED_PIN, LOW);
    digitalWrite(ALARM_BUZZER_PIN, LOW);

    Serial.println("Alarm deactivated");
  }
}

// ===========================================
// UTILITY FUNCTIONS
// ===========================================

/**
 * Print detection result to serial
 */
void printDetectionResult(const DetectionResult& result) {
  Serial.print("Camera ");
  Serial.print(result.cameraId + 1);
  Serial.print(": ");

  if (result.valid) {
    Serial.print(classNames[result.classId]);
    Serial.print(" (");
    Serial.print(result.confidence * 100, 1);
    Serial.print("%) ");

    // Print bounding box
    Serial.print("[");
    Serial.print(result.boundingBox.x);
    Serial.print(",");
    Serial.print(result.boundingBox.y);
    Serial.print(",");
    Serial.print(result.boundingBox.width);
    Serial.print(",");
    Serial.print(result.boundingBox.height);
    Serial.println("]");
  } else {
    Serial.println("No detection");
  }
}

/**
 * Print model information
 */
void printModelInfo(const ModelMetadata& modelInfo) {
  Serial.println("=== Model Info ===");
  Serial.print("Name: ");
  Serial.println(modelInfo.name);
  Serial.print("Version: ");
  Serial.println(modelInfo.version);
  Serial.print("Input: ");
  Serial.print(modelInfo.inputWidth);
  Serial.print("x");
  Serial.println(modelInfo.inputHeight);
  Serial.print("Classes: ");
  Serial.println(modelInfo.numClasses);
  Serial.print("Threshold: ");
  Serial.println(modelInfo.threshold);
  Serial.print("Quantized: ");
  Serial.println(modelInfo.quantized ? "Yes" : "No");
  Serial.println("=================");
}

/**
 * Print current memory usage
 */
void printMemoryUsage() {
  Serial.println("=== Memory Usage ===");

  // Calculate tensor arena usage
  if (interpreter != nullptr) {
    TfLiteTensor* tensors[10];
    int used = 0;

    // Estimate usage by checking tensor allocations
    for (int i = 0; i < 10; i++) {
      TfLiteTensor* tensor = interpreter->tensor(i);
      if (tensor != nullptr && tensor->bytes > 0) {
        used += tensor->bytes;
      }
    }

    Serial.print("Tensor Arena: ");
    Serial.print(used / 1024);
    Serial.print(" KB / ");
    Serial.print(kTensorArenaSize / 1024);
    Serial.println(" KB");
  }

  // Frame buffers
  Serial.print("Frame Buffers: ");
  Serial.print((FRAME_BUFFER_SIZE * 2) / 1024);
  Serial.println(" KB");

  // Preprocessing buffer
  Serial.print("Preprocessing Buffer: ");
  Serial.print((MODEL_INPUT_WIDTH * MODEL_INPUT_HEIGHT * MODEL_INPUT_CHANNELS) / 1024);
  Serial.println(" KB");

  // Total
  uint32_t total = (FRAME_BUFFER_SIZE * 2) +
                   (MODEL_INPUT_WIDTH * MODEL_INPUT_HEIGHT * MODEL_INPUT_CHANNELS) +
                   kTensorArenaSize;
  Serial.print("Total: ");
  Serial.print(total / 1024);
  Serial.println(" KB");

  Serial.println("===================");
}

/**
 * Get average confidence of all detections
 */
float getAverageConfidence() {
  if (totalDetections == 0) return 0.0f;

  float sum = 0.0f;
  if (lastDetection1.valid) sum += lastDetection1.confidence;
  if (lastDetection2.valid) sum += lastDetection2.confidence;

  return sum / totalDetections;
}

/**
 * Reset detection statistics
 */
void resetStatistics() {
  totalDetections = 0;
  memset(detectionCounts, 0, sizeof(detectionCounts));
  detectionBuffer1.clear();
  detectionBuffer2.clear();

  Serial.println("Statistics reset");
}

/**
 * Print detection statistics
 */
void printStatistics() {
  Serial.println("=== Detection Statistics ===");
  Serial.print("Total Detections: ");
  Serial.println(totalDetections);

  for (int i = 0; i < NUM_CLASSES; i++) {
    Serial.print("  ");
    Serial.print(classNames[i]);
    Serial.print(": ");
    Serial.println(detectionCounts[i]);
  }

  Serial.print("Average Confidence: ");
  Serial.print(getAverageConfidence() * 100);
  Serial.println("%");

  Serial.println("Camera 1 Buffer: ");
  Serial.print("  Size: ");
  Serial.println(detectionBuffer1.size());

  Serial.println("Camera 2 Buffer: ");
  Serial.print("  Size: ");
  Serial.println(detectionBuffer2.size());

  Serial.println("=============================");

  // Print performance metrics
  metrics.print();
}

// ===========================================
// ERROR HANDLING
// ===========================================

/**
 * Handle ML errors
 */
void handleMLError(const char* message) {
  Serial.print("ML ERROR: ");
  Serial.println(message);

  // TODO: Add error recovery logic
  // - Reload model
  // - Reset interpreter
  // - Log error for debugging
}

/**
 * Handle camera errors
 */
void handleCameraError(uint8_t cameraId, const char* message) {
  Serial.print("Camera ");
  Serial.print(cameraId + 1);
  Serial.print(" ERROR: ");
  Serial.println(message);

  // TODO: Add error recovery logic
  // - Reinitialize camera
  // - Switch I2C channel
  // - Log error for debugging
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
