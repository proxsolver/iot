/**
 * Model Data Template
 *
 * This file is a template for integrating your trained TFLite models.
 * Replace the placeholder data with your actual model.
 *
 * Instructions:
 * 1. Train your model (see vision_system_guide_complete.md)
 * 2. Convert to TFLite format
 * 3. Convert to C array:
 *    - Linux/Mac: xxd -i model.tflite > model_data.cc
 *    - Windows: Use Python script (see guide)
 * 4. Copy the generated array below
 * 5. Update MODEL_SIZE and MODEL_VERSION
 */

#ifndef MODEL_DATA_H
#define MODEL_DATA_H

#include <stdint.h>

// ===========================================
// MODEL METADATA
// ===========================================

#define MODEL_NAME "Person Detection"
#define MODEL_VERSION "1.0.0"
#define MODEL_TYPE CLASSIFICATION  // or OBJECT_DETECTION
#define MODEL_INPUT_WIDTH 96
#define MODEL_INPUT_HEIGHT 96
#define MODEL_INPUT_CHANNELS 3
#define MODEL_NUM_CLASSES 3
#define MODEL_QUANTIZED true

// ===========================================
// MODEL DATA
// ===========================================

// This is a placeholder - replace with your actual model data
// Generated using: xxd -i your_model.tflite

const unsigned char g_model[] = {
  0x1c, 0x00, 0x00, 0x00, 0x54, 0x46, 0x4c, 0x33,  // TFL3 header
  // ... rest of your model data here ...
};

// Model size (automatically calculated)
const unsigned int g_model_len = sizeof(g_model);

// ===========================================
// CLASS LABELS
// ===========================================

const char* model_class_labels[] = {
  "Person",
  "Vehicle",
  "Animal"
};

#define MODEL_NUM_LABELS (sizeof(model_class_labels) / sizeof(model_class_labels[0]))

// ===========================================
// MODEL CONFIGURATION
// ===========================================

// Detection thresholds
#define MODEL_DETECTION_THRESHOLD 0.5f
#define MODEL_CONFIDENCE_THRESHOLD 0.6f

// Input normalization parameters (for float models)
#define MODEL_INPUT_MEAN 0.0f
#define MODEL_INPUT_STD 255.0f

// Output scaling parameters (for quantized models)
#define MODEL_OUTPUT_SCALE 1.0f
#define MODEL_OUTPUT_ZERO_POINT 0

// ===========================================
// OPTIONAL: Model-specific functions
// ===========================================

/**
 * Custom preprocessing (if needed)
 * Override default preprocessing if your model requires special handling
 */
void modelPreprocessInput(uint8_t* input, uint8_t* output) {
  // Default implementation: just copy
  // Customize if your model needs specific preprocessing

  int inputSize = MODEL_INPUT_WIDTH * MODEL_INPUT_HEIGHT * MODEL_INPUT_CHANNELS;
  memcpy(output, input, inputSize);

  // Example: Subtract mean
  // for (int i = 0; i < inputSize; i++) {
  //   output[i] = input[i] - 128;
  // }
}

/**
 * Custom postprocessing (if needed)
 * Override default postprocessing for model-specific output handling
 */
void modelPostprocessOutput(TfLiteTensor* output, DetectionResult* result) {
  // Default implementation: find max confidence
  // Customize for your model's output format

  float maxConfidence = -1.0f;
  int detectedClass = -1;

  int numOutputs = output->dims->data[output->dims->size - 1];

  for (int i = 0; i < min(numOutputs, MODEL_NUM_CLASSES); i++) {
    float confidence;

    if (output->type == kTfLiteUInt8) {
      // Quantized model
      confidence = (output->data.uint8[i] - MODEL_OUTPUT_ZERO_POINT) * MODEL_OUTPUT_SCALE;
    } else {
      // Float model
      confidence = output->data.f[i];
    }

    if (confidence > maxConfidence) {
      maxConfidence = confidence;
      detectedClass = i;
    }
  }

  result->classId = detectedClass;
  result->confidence = maxConfidence;
  result->valid = (maxConfidence >= MODEL_DETECTION_THRESHOLD);
}

/**
 * Extract bounding boxes (for object detection models)
 * Implement based on your model's output format
 */
void modelExtractBoundingBoxes(TfLiteTensor* output,
                                DetectionResult* results,
                                uint8_t* numDetections) {
  // TODO: Implement based on your model's output format
  // Examples:
  //
  // SSD MobileNet: [num_detections, 7]
  //   7 = [batch, class_id, confidence, x, y, width, height]
  //
  // YOLO: [grid_size, grid_size, num_anchors * (num_classes + 5)]
  //
  // For classification models, this is not needed

  *numDetections = 0;

  // Placeholder implementation
  // See vision_system_guide_complete.md for full examples
}

/**
 * Get model information
 */
void printModelInfo() {
  Serial.println("=== Model Information ===");
  Serial.print("Name: ");
  Serial.println(MODEL_NAME);
  Serial.print("Version: ");
  Serial.println(MODEL_VERSION);
  Serial.print("Type: ");
  Serial.println(MODEL_TYPE == CLASSIFICATION ? "Classification" : "Object Detection");
  Serial.print("Input: ");
  Serial.print(MODEL_INPUT_WIDTH);
  Serial.print("x");
  Serial.print(MODEL_INPUT_HEIGHT);
  Serial.print("x");
  Serial.println(MODEL_INPUT_CHANNELS);
  Serial.print("Classes: ");
  Serial.println(MODEL_NUM_CLASSES);
  Serial.print("Quantized: ");
  Serial.println(MODEL_QUANTIZED ? "Yes" : "No");
  Serial.print("Size: ");
  Serial.print(g_model_len / 1024);
  Serial.println(" KB");
  Serial.println("========================");

  Serial.println("Classes:");
  for (int i = 0; i < MODEL_NUM_LABELS; i++) {
    Serial.print("  ");
    Serial.print(i);
    Serial.print(": ");
    Serial.println(model_class_labels[i]);
  }
}

// ===========================================
// VALIDATION
// ===========================================

// Validate model header (TFLite format)
#define TFLITE_SCHEMA_VERSION 3

static bool validateModel() {
  // Check TFLite header
  if (g_model_len < 4) {
    Serial.println("ERROR: Model too small!");
    return false;
  }

  // Check TFLite version (first 4 bytes should be 0x1C 0x00 0x00 0x00 for version 3)
  uint32_t version = *((uint32_t*)g_model);

  if (version != TFLITE_SCHEMA_VERSION) {
    Serial.print("WARNING: Model version mismatch! Expected: ");
    Serial.print(TFLITE_SCHEMA_VERSION);
    Serial.print(", Got: ");
    Serial.println(version);
  }

  // Check model size
  if (g_model_len < 100) {
    Serial.println("ERROR: Model appears invalid (too small)!");
    return false;
  }

  Serial.println("Model validation passed");
  return true;
}

#endif  // MODEL_DATA_H
