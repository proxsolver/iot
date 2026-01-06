# TinyML Dual Camera System - Quick Reference

## File Locations

```
/src/vision/
├── dual_camera_ml.cpp                    # Main implementation (1500+ lines)
├── vision_system_guide_complete.md       # Complete guide (500+ lines)
├── IMPLEMENTATION_SUMMARY.md             # This summary
└── QUICK_REFERENCE.md                    # This file
```

## Quick Start Checklist

### 1. Hardware Setup
- [ ] Connect TCA9548A I2C multiplexer (SDA, SCL, VCC, GND)
- [ ] Connect Camera 1 to Channel 0
- [ ] Connect Camera 2 to Channel 1
- [ ] Connect LED to D2 (with 330Ω resistor)
- [ ] Connect Buzzer to D3 (with 330Ω resistor)

### 2. Software Setup
- [ ] Install Arduino IDE 2.x or PlatformIO
- [ ] Install Arduino_OV5640 library
- [ ] Install TensorFlowLite library
- [ ] Copy dual_camera_ml.cpp to project

### 3. Model Training
- [ ] Train model using Edge Impulse OR TensorFlow
- [ ] Export as TFLite model
- [ ] Convert to quantized int8 format
- [ ] Generate C array: `xxd -i model.tflite > model_data.cc`

### 4. Integration
- [ ] Include model_data.cc in project
- [ ] Update model loading code with your model
- [ ] Adjust thresholds if needed

### 5. Deployment
- [ ] Upload firmware to Nicla Vision
- [ ] Open Serial Monitor (115200 baud)
- [ ] Verify initialization messages

## Key Configuration Parameters

```cpp
// Hardware
#define CAMERA_WIDTH 320
#define CAMERA_HEIGHT 240
#define TCA9548A_ADDR 0x70

// Model
#define MODEL_INPUT_WIDTH 96
#define MODEL_INPUT_HEIGHT 96
#define NUM_CLASSES 3

// Thresholds
#define DETECTION_THRESHOLD 0.5f    // 50% minimum
#define CONFIDENCE_THRESHOLD 0.6f   // 60% for alarm
#define NMS_IOU_THRESHOLD 0.5f      // 50% IoU overlap

// Memory
constexpr int kTensorArenaSize = 500 * 1024;  // 500KB
#define DETECTION_BUFFER_SIZE 10

// Timing
#define ALARM_DURATION_MS 5000
```

## API Reference

### Initialization
```cpp
bool initializeI2CMultiplexer();       // Init multiplexer
bool initializeBothCameras();          // Init both cameras
bool initializeMLModel();              // Init ML model
```

### Model Management
```cpp
bool loadPersonDetectionModel();       // Load person model
bool loadVehicleDetectionModel();      // Load vehicle model
bool loadAnimalDetectionModel();       // Load animal model
void switchModel(ModelType type);      // Switch models
```

### Image Processing
```cpp
bool captureImage(uint8_t cameraId, uint8_t* buffer);
void preprocessImage(uint8_t* src, int srcW, int srcH,
                     uint8_t* dst, int dstW, int dstH);
```

### Inference
```cpp
void runInference(uint8_t* imageData, DetectionResult* result);
void runInferenceMultiple(uint8_t* imageData,
                         DetectionResult* results,
                         uint8_t* numDetections);
```

### Detection Processing
```cpp
void processDetections();              // Process both cameras
void processCamera(uint8_t cameraId, uint8_t* buffer,
                   DetectionResult& result,
                   DetectionBuffer& buffer);
```

### Post-Processing
```cpp
void applyNonMaximumSuppression(DetectionResult* detections,
                                uint8_t* numDetections);
void filterByConfidence(DetectionResult* detections,
                        uint8_t* numDetections,
                        float threshold);
```

### Alarm System
```cpp
bool shouldTriggerAlarm(const DetectionResult& result);
void triggerAlarm();
void updateAlarm();                    // Call in loop()
```

### Utility
```cpp
void printDetectionResult(const DetectionResult& result);
void printStatistics();
void printMemoryUsage();
void resetStatistics();
```

## Data Structures

### DetectionResult
```cpp
struct DetectionResult {
  uint8_t cameraId;          // 0 or 1
  uint8_t classId;           // 0=Person, 1=Vehicle, 2=Animal
  float confidence;          // 0.0 to 1.0
  BoundingBox boundingBox;   // x, y, width, height
  uint32_t timestamp;        // millis()
  bool valid;                // true if detection

  bool isValid();            // Check if valid detection
  bool shouldAlarm();        // Check if should alarm
};
```

### BoundingBox
```cpp
struct BoundingBox {
  uint16_t x, y, width, height;

  float calculateIoU(const BoundingBox& other);
  uint32_t area();
};
```

### DetectionBuffer
```cpp
struct DetectionBuffer {
  void add(const DetectionResult& result);
  DetectionResult* getLatest(uint8_t n);
  void clear();
  uint8_t size();
};
```

### PerformanceMetrics
```cpp
struct PerformanceMetrics {
  void recordInference(uint32_t timeUs);
  void recordCapture(bool success);
  void print();

  float getAverageInferenceTime();
  float getCaptureSuccessRate();
};
```

## Common Tasks

### Change Detection Threshold
```cpp
// In dual_camera_ml.cpp, modify:
#define DETECTION_THRESHOLD 0.4f  // Lower for more sensitive
```

### Add New Class
```cpp
// 1. Update NUM_CLASSES
#define NUM_CLASSES 4

// 2. Add class name
const char* classNames[NUM_CLASSES] = {
  "Person", "Vehicle", "Animal", "Custom"
};

// 3. Retrain model with 4 classes
```

### Adjust Frame Rate
```cpp
// In loop(), change delay
void loop() {
  processDetections();
  delay(500);  // 2 FPS (was 1000ms = 1 FPS)
}
```

### Disable Alarm
```cpp
// Comment out alarm trigger
void processCamera(...) {
  // ...
  if (result.shouldAlarm()) {
    // triggerAlarm();  // Disabled
  }
}
```

### Monitor Performance
```cpp
// Call periodically
void loop() {
  static unsigned long lastStats = 0;

  if (millis() - lastStats >= 30000) {  // Every 30s
    printStatistics();
    printMemoryUsage();
    lastStats = millis();
  }

  processDetections();
}
```

## Troubleshooting Quick Fixes

### "Model version mismatch"
```bash
# Reinstall TensorFlow
pip install tensorflow==2.13.0
# Reconvert model
python convert_to_tflite.py
```

### "Failed to allocate tensors"
```cpp
// Increase tensor arena
constexpr int kTensorArenaSize = 600 * 1024;
```

### "Camera initialization failed"
```cpp
// Check I2C connection
void scanI2C() {
  for (byte addr = 1; addr < 127; addr++) {
    Wire.beginTransmission(addr);
    if (Wire.endTransmission() == 0) {
      Serial.print("Found at 0x");
      Serial.println(addr, HEX);
    }
  }
}
```

### Low accuracy
```cpp
// Lower threshold
#define DETECTION_THRESHOLD 0.4f
// Or retrain with more data
```

### Slow inference
```cpp
// Ensure quantized model
converter.inference_input_type = tf.int8;
// Or reduce input size
IMAGE_SIZE = (64, 64)  // Retrain model
```

## Memory Optimization Tips

1. **Reduce Input Size**: 96×96 → 64×64 (56% reduction)
2. **Single Buffer**: Process cameras sequentially
3. **Smaller Arena**: Monitor and adjust kTensorArenaSize
4. **Prune Model**: Remove unneeded layers/filters
5. **Quantize**: Always use int8 models

## Performance Targets

| Metric | Target | How to Measure |
|--------|--------|----------------|
| Inference Time | <200ms | `metrics.print()` |
| Frame Rate | 1-2 FPS | Count `processDetections()` calls |
| Memory Usage | <1MB | `printMemoryUsage()` |
| Detection Accuracy | >90% | Validation dataset |
| Capture Success Rate | >95% | `metrics.getCaptureSuccessRate()` |

## Serial Monitor Output

```
Initializing ML model...
Loading model...
  Name: Person Detection
  Version: 1.0.0
  Input dimensions: 1x96x96x3
  Input type: uint8
ML model initialized successfully

Camera 1: Person (85.3%) [0,0,96,96]
Inference time: 145.23 ms

Camera 2: No detection
Inference time: 142.87 ms
```

## Emergency Recovery

```cpp
// If system crashes, add watchdog
#include <Watchdog.h>

void setup() {
  Watchdog.enable(16000);  // 16 second timeout
}

void loop() {
  Watchdog.reset();
  // Your code here
}
```

## Support Links

- Full Guide: `vision_system_guide_complete.md`
- Summary: `IMPLEMENTATION_SUMMARY.md`
- TensorFlow Lite Micro: https://www.tensorflow.org/lite/microcontrollers
- Edge Impulse: https://docs.edgeimpulse.com

---

**Last Updated**: 2025-01-06
**Version**: 1.0.0
