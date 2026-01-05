/**
 * Nicla Vision Camera Firmware
 *
 * This firmware runs on the Arduino Nicla Vision boards to initialize the camera,
 * capture images, and perform object detection using TinyML.
 *
 * Board: Arduino Nicla Vision (STM32H747)
 * Camera: OV5640 5MP
 *
 * Features:
 * - Camera initialization and configuration
 * - Non-blocking image capture (millis-based timing)
 * - Efficient memory management (no String class)
 * - Serial communication to gateway
 * - LED status indicators
 * - Error handling and validation
 */

#include <Arduino.h>
#include <Arduino_OV5640.h>
#include <TensorFlowLite.h>
#include <tensorflow/lite/micro/all_ops_resolver.h>
#include <tensorflow/lite/micro/micro_error_reporter.h>
#include <tensorflow/lite/micro/micro_interpreter.h>
#include <tensorflow/lite/schema/schema_generated.h>
#include <tensorflow/lite/version.h>

// ===========================================
// CONFIGURATION
// ===========================================

// Serial communication
#define SERIAL_BAUD 115200

// Camera configuration
#define CAMERA_WIDTH 320
#define CAMERA_HEIGHT 240
#define CAMERA_FORMAT RGB565

// Timing (non-blocking)
unsigned long lastCaptureTime = 0;
const unsigned long CAPTURE_INTERVAL = 5000;  // 5 seconds between captures

// LED pins
#define LED_STATUS LED_BUILTIN
#define LED_ERROR LEDR

// Buffer for image data (use char array instead of String)
#define FRAME_BUFFER_SIZE (CAMERA_WIDTH * CAMERA_HEIGHT * 2)  // RGB565 = 2 bytes per pixel
uint8_t frameBuffer[FRAME_BUFFER_SIZE];

// Status tracking
typedef enum {
  STATE_IDLE,
  STATE_CAPTURING,
  STATE_PROCESSING,
  STATE_TRANSMITTING,
  STATE_ERROR
} SystemState;

SystemState currentState = STATE_IDLE;
char lastErrorMessage[128] = {0};

// ===========================================
// TENSORFLOW LITE SETUP
// ===========================================

// Global TFLite variables
tflite::ErrorReporter* error_reporter = nullptr;
const tflite::Model* model = nullptr;
tflite::MicroInterpreter* interpreter = nullptr;

// Create an area of memory to use for input, output, and intermediate arrays.
// The size depends on the model you're using.
constexpr int kTensorArenaSize = 500 * 1024;
uint8_t tensor_arena[kTensorArenaSize];

// ===========================================
// FUNCTION PROTOTYPES
// ===========================================

bool initializeCamera();
bool initializeML();
void captureImage();
void processImage();
void transmitImage();
void setStatusLED(bool on);
void setErrorLED(bool on);
void handleError(const char* message);
void printSystemInfo();

// ===========================================
// SETUP
// ===========================================

void setup() {
  // Initialize serial communication
  Serial.begin(SERIAL_BAUD);
  while (!Serial && millis() < 3000);  // Wait up to 3 seconds for serial

  // Initialize status LEDs
  pinMode(LED_STATUS, OUTPUT);
  pinMode(LED_ERROR, OUTPUT);
  setStatusLED(false);
  setErrorLED(false);

  // Print system information
  printSystemInfo();

  // Initialize camera
  Serial.println("Initializing camera...");
  if (!initializeCamera()) {
    handleError("Camera initialization failed");
    return;
  }

  // Initialize ML model
  Serial.println("Initializing ML model...");
  if (!initializeML()) {
    handleError("ML initialization failed");
    return;
  }

  // Ready
  Serial.println("System ready");
  setStatusLED(true);
  currentState = STATE_IDLE;
}

// ===========================================
// MAIN LOOP
// ===========================================

void loop() {
  // Non-blocking timing check
  unsigned long currentTime = millis();

  switch (currentState) {
    case STATE_IDLE:
      // Check if it's time to capture a new image
      if (currentTime - lastCaptureTime >= CAPTURE_INTERVAL) {
        lastCaptureTime = currentTime;
        currentState = STATE_CAPTURING;
        Serial.println("Starting image capture...");
      }
      break;

    case STATE_CAPTURING:
      captureImage();
      if (currentState != STATE_ERROR) {
        currentState = STATE_PROCESSING;
      }
      break;

    case STATE_PROCESSING:
      processImage();
      if (currentState != STATE_ERROR) {
        currentState = STATE_TRANSMITTING;
      }
      break;

    case STATE_TRANSMITTING:
      transmitImage();
      currentState = STATE_IDLE;
      break;

    case STATE_ERROR:
      // Blink error LED
      setErrorLED(currentTime % 1000 < 500);
      // Stay in error state until reset
      break;

    default:
      currentState = STATE_IDLE;
      break;
  }

  // Small delay to prevent watchdog issues
  delay(10);
}

// ===========================================
// CAMERA INITIALIZATION
// ===========================================

bool initializeCamera() {
  Serial.println("  Camera: OV5640");

  // Initialize the camera
  if (!Camera.begin(CAMERA_WIDTH, CAMERA_HEIGHT, CAMERA_FORMAT, 30)) {
    Serial.println("  Failed to initialize camera!");
    return false;
  }

  // Configure camera settings
  Camera.setExposure(0);  // Auto exposure
  Camera.setGain(0);     // Auto gain
  Camera.setWhiteBalance(0);  // Auto white balance

  Serial.print("  Resolution: ");
  Serial.print(CAMERA_WIDTH);
  Serial.print("x");
  Serial.println(CAMERA_HEIGHT);
  Serial.println("  Camera initialized successfully");

  return true;
}

// ===========================================
// ML INITIALIZATION
// ===========================================

bool initializeML() {
  Serial.println("  TensorFlow Lite for Microcontrollers");

  // Set up logging
  static tflite::MicroErrorReporter micro_error_reporter;
  error_reporter = &micro_error_reporter;

  // Map the model into a usable data structure
  // Note: You need to include your model data here
  // model = tflite::GetModel(g_model);

  if (model->version() != TFLITE_SCHEMA_VERSION) {
    error_reporter->Report("Model provided is schema version %d not equal "
                           "to supported version %d.",
                           model->version(), TFLITE_SCHEMA_VERSION);
    return false;
  }

  // Pull in only the operation implementations we need
  static tflite::AllOpsResolver resolver;

  // Build an interpreter to run the model
  static tflite::MicroInterpreter static_interpreter(
      model, resolver, tensor_arena, kTensorArenaSize, error_reporter);
  interpreter = &static_interpreter;

  // Allocate tensor_arena from the tensor_arena
  TfLiteStatus allocate_status = interpreter->AllocateTensors();
  if (allocate_status != kTfLiteOk) {
    error_reporter->Report("AllocateTensors() failed");
    return false;
  }

  // Get information about the memory area to use for the model's input
  TfLiteTensor* input = interpreter->input(0);

  Serial.print("  Input tensor size: ");
  Serial.println(input->bytes);
  Serial.println("  ML model initialized successfully");

  return true;
}

// ===========================================
// IMAGE CAPTURE
// ===========================================

void captureImage() {
  Serial.println("  Capturing image...");

  // Capture frame into buffer
  Camera.readFrame(frameBuffer);

  // Validate capture
  if (frameBuffer == nullptr || frameBuffer[0] == 0) {
    handleError("Image capture failed - empty buffer");
    return;
  }

  Serial.println("  Image captured successfully");
}

// ===========================================
// IMAGE PROCESSING (ML INFERENCE)
// ===========================================

void processImage() {
  Serial.println("  Running ML inference...");

  // Get input tensor from model
  TfLiteTensor* input = interpreter->input(0);

  // Copy image data to input tensor
  // Note: You may need to preprocess the image (resize, normalize, etc.)
  memcpy(input->data.uint8, frameBuffer, input->bytes);

  // Run inference
  TfLiteStatus invoke_status = interpreter->Invoke();
  if (invoke_status != kTfLiteOk) {
    handleError("Invoke failed");
    return;
  }

  // Get output tensor
  TfLiteTensor* output = interpreter->output(0);

  // Process results
  // This is a placeholder - implement based on your model
  float max_confidence = 0.0;
  int detected_class = -1;

  for (int i = 0; i < output->dims->data[1]; i++) {
    float confidence = output->data.f[i];
    if (confidence > max_confidence) {
      max_confidence = confidence;
      detected_class = i;
    }
  }

  Serial.print("  Detected class: ");
  Serial.print(detected_class);
  Serial.print(" (confidence: ");
  Serial.print(max_confidence * 100);
  Serial.println("%)");
}

// ===========================================
// DATA TRANSMISSION
// ===========================================

void transmitImage() {
  Serial.println("  Transmitting data...");

  // Send data header
  Serial.print("IMG:");
  Serial.print(CAMERA_WIDTH);
  Serial.print(",");
  Serial.print(CAMERA_HEIGHT);
  Serial.print(",");

  // Send image data in chunks to avoid buffer overflow
  const int CHUNK_SIZE = 128;
  int totalBytes = CAMERA_WIDTH * CAMERA_HEIGHT * 2;

  for (int i = 0; i < totalBytes; i += CHUNK_SIZE) {
    int chunkSize = min(CHUNK_SIZE, totalBytes - i);

    // Send base64 encoded chunk (simplified - use proper base64 library)
    for (int j = 0; j < chunkSize; j++) {
      Serial.write(frameBuffer[i + j]);
    }

    // Small delay between chunks
    delay(5);
  }

  Serial.println();  // End marker
  Serial.println("  Data transmitted successfully");
}

// ===========================================
// UTILITY FUNCTIONS
// ===========================================

void setStatusLED(bool on) {
  digitalWrite(LED_STATUS, on ? HIGH : LOW);
}

void setErrorLED(bool on) {
  digitalWrite(LED_ERROR, on ? HIGH : LOW);
}

void handleError(const char* message) {
  currentState = STATE_ERROR;
  strncpy(lastErrorMessage, message, sizeof(lastErrorMessage) - 1);
  lastErrorMessage[sizeof(lastErrorMessage) - 1] = '\0';

  Serial.print("ERROR: ");
  Serial.println(lastErrorMessage);
  setErrorLED(true);
}

void printSystemInfo() {
  Serial.println("==================================");
  Serial.println("Nicla Vision Camera Firmware");
  Serial.println("==================================");
  Serial.print("Version: 1.0.0");
  Serial.print("Build: ");
  Serial.print(__DATE__);
  Serial.print(" ");
  Serial.println(__TIME__);
  Serial.println("==================================");
}
