# Complete Vision System Guide for Arduino Nicla Vision Dual Camera TinyML

## Table of Contents

1. [System Overview](#system-overview)
2. [Hardware Requirements](#hardware-requirements)
3. [Software Requirements](#software-requirements)
4. [Model Training Guide](#model-training-guide)
5. [Model Conversion to TFLite Micro](#model-conversion-to-tflite-micro)
6. [Deployment Steps](#deployment-steps)
7. [Performance Optimization](#performance-optimization)
8. [Memory Usage Analysis](#memory-usage-analysis)
9. [Troubleshooting](#troubleshooting)
10. [Advanced Features](#advanced-features)

---

## System Overview

### Architecture

```
┌─────────────────────────────────────────────────────────┐
│         Arduino MKR WAN 1310 (Gateway)                  │
│                                                         │
│  ┌──────────────────────────────────────────────────┐  │
│  │        TCA9548A I2C Multiplexer (0x70)          │  │
│  │                                                   │  │
│  │  Channel 0    Channel 1                          │  │
│  │     │              │                               │  │
│  │     ▼              ▼                               │  │
│  │  ┌──────────┐  ┌──────────┐                       │  │
│  │  │  Camera  │  │  Camera  │                       │  │
│  │  │    #1    │  │    #2    │                       │  │
│  │  │ Nicla    │  │ Nicla    │                       │  │
│  │  │ Vision   │  │ Vision   │                       │  │
│  │  └──────────┘  └──────────┘                       │  │
│  │       │              │                              │  │
│  │       └──────┬───────┘                              │  │
│  │              ▼                                      │  │
│  │     TensorFlow Lite Micro                          │  │
│  │     Object Detection                              │  │
│  │                                                   │  │
│  │  ┌─────────────────────────────────────────┐     │  │
│  │  │  Detection Results                      │     │  │
│  │  │  - Class ID                             │     │  │
│  │  │  - Confidence                           │     │  │
│  │  │  - Bounding Box                         │     │  │
│  │  │  - Timestamp                            │     │  │
│  │  └─────────────────────────────────────────┘     │  │
│  │              │                                      │  │
│  └──────────────┼──────────────────────────────────────┘  │
│                 │                                         │
│                 ▼                                         │
│         ┌─────────────┐                                   │
│         │  LoRaWAN    │                                   │
│         │  WiFi       │                                   │
│         │  Serial     │                                   │
│         └─────────────┘                                   │
└─────────────────────────────────────────────────────────┘
```

### Key Features

- **Dual Camera System**: Two Arduino Nicla Vision boards
- **I2C Multiplexing**: TCA9548A for independent camera control
- **TinyML Inference**: TensorFlow Lite Micro for on-device ML
- **Real-time Detection**: <200ms inference time per camera
- **Multi-class Detection**: Person, Vehicle, Animal detection
- **Detection Buffering**: Store recent detection history
- **NMS Support**: Non-Maximum Suppression for overlapping detections
- **Performance Profiling**: Built-in metrics tracking
- **Alarm System**: Configurable threshold-based alerts

---

## Hardware Requirements

### Components

| Component | Quantity | Specifications | Purpose |
|-----------|----------|----------------|---------|
| Arduino MKR WAN 1310 | 1 | STM32L0, 32KB RAM | Gateway/controller |
| Arduino Nicla Vision | 2 | STM32H747, 1MB RAM, OV5640 5MP | Camera + ML processing |
| TCA9548A I2C Multiplexer | 1 | 8-channel, 3.3V/5V | Dual camera control |
| LED | 1 | Red, 5V | Alarm indicator |
| Buzzer | 1 | 5V, Active | Audio alert |
| Resistors | 2 | 330Ω | LED/buzzer current limit |
| Jumper wires | - | Various | Connections |

### Wiring Diagram

```
MKR WAN 1310                    TCA9548A
─────────────                  ┌─────────┐
SDA (Pin 11) ────────────────► │ SDA     │
SCL (Pin 12) ────────────────► │ SCL     │
                              │         │
                           5V │ VCC     │
                          GND │ GND     │
                              │         │
                 Channel 0 ───► │ C0      │─────► Nicla Vision #1 (SDA, SCL)
                 Channel 1 ───► │ C1      │─────► Nicla Vision #2 (SDA, SCL)
                              └─────────┘

Alarm System:
MKR WAN 1310
─────────────
D2 ──[330Ω]────► Buzzer (+)
GND ───────────► Buzzer (-)

LED_BUILTIN ────► LED (+)
GND ───────────► LED (-)
```

### Power Requirements

- **MKR WAN 1310**: ~40mA (normal), ~100mA (LoRa transmission)
- **Nicla Vision**: ~200mA (camera on), ~300mA (inference)
- **TCA9548A**: ~5mA
- **LED + Buzzer**: ~20mA
- **Total Peak Current**: ~750mA

**Recommendation**: Use 5V 2A power supply with adequate decoupling capacitors.

---

## Software Requirements

### Development Environment

1. **Arduino IDE 2.x** or **PlatformIO**
2. **Arduino Boards**: Arduino mbed OS Nicla Vision boards
3. **Required Libraries**:
   - `Arduino_OV5640` (Camera driver)
   - `TensorFlowLite` (ML framework)
   - `Wire` (I2C communication - built-in)

### Installing Libraries

#### Arduino IDE

```bash
# In Arduino IDE, install these libraries:
# 1. Arduino_OV5640
Tools -> Manage Libraries -> Search "Arduino_OV5640" -> Install

# 2. TensorFlowLite
Tools -> Manage Libraries -> Search "TensorFlowLite" -> Install
```

#### PlatformIO

```ini
; platformio.ini
[env:nicla_vision]
platform = ststm32
board = nicla_vision
framework = arduino
lib_deps =
    arduino-libraries/Arduino_OV5640
    tensorflow/tensorflowlite
```

---

## Model Training Guide

### Option 1: Edge Impulse (Recommended for Beginners)

Edge Impulse provides a no-code/low-code platform for training ML models for edge devices.

#### Step 1: Create Edge Impulse Project

1. Go to [https://studio.edgeimpulse.com](https://studio.edgeimpulse.com)
2. Create account and new project
3. Select "Image classification" or "Object detection"

#### Step 2: Collect Training Data

**Option A: Use Existing Datasets**
- Download from [Edge Impulse Data](https://www.edgeimpulse.com/datasets)
- COCO dataset for person/vehicle detection
- Open Images dataset

**Option B: Capture Custom Data**

```python
# Use Edge Impulse CLI to capture data
edge-impulse-data-forwarder

# Or manually capture images
# 1. Connect camera to computer
# 2. Capture images for each class
# 3. Organize: dataset/person/*.jpg, dataset/vehicle/*.jpg
```

**Data Requirements**:
- Minimum 100 images per class (500+ recommended)
- Image size: 96x96 to 320x240 (will be resized)
- Include varied: lighting, angles, backgrounds
- Split: 80% training, 20% testing

#### Step 3: Train Model

1. **Create Impulse**:
   - Input: Image data
   - Processing: Image (resize to 96x96)
   - Learning: Transfer Learning (MobileNet) or Object Detection

2. **Configure Features**:
   - Image dimensions: 96x96 (recommended)
   - Color depth: RGB
   - Extra features: None

3. **Configure Model**:
   - Model type: Transfer Learning (Recommended)
   - Base model: MobileNetV2 (96x96, 0.35)
   - Number of training cycles: 50-100
   - Learning rate: 0.0005 (default)
   - Data augmentation: Enabled

4. **Start Training**:
   - Click "Start training"
   - Wait 10-30 minutes
   - Monitor accuracy (>90% target)

5. **Evaluate Model**:
   - Check confusion matrix
   - Review accuracy per class
   - Test with unseen data

#### Step 4: Export for Arduino

1. Click **Deployment** in Edge Impulse
2. Select **Arduino Library**
3. Configure:
   - Optimized for: **Arduino**
   - Quantization: **Int8** (recommended)
   - Output format: **C++ library**
4. Download and extract ZIP file
5. Copy to Arduino `libraries/` folder

#### Step 5: Integrate Model

```cpp
// In your Arduino sketch
#include <tensorflow/lite/micro/all_ops_resolver.h>
#include <tensorflow/lite/micro/micro_interpreter.h>
#include <tensorflow/lite/schema/schema_generated.h>

// Include your exported model
#include "ei_model_model.h"  // From Edge Impulse

// Model loading
const tflite::Model* model = tflite::GetModel(g_model);
```

---

### Option 2: TensorFlow (Advanced)

For full control over model architecture and training process.

#### Step 1: Setup Python Environment

```bash
# Create virtual environment
python -m venv tflite_env
source tflite_env/bin/activate  # On Windows: tflite_env\Scripts\activate

# Install dependencies
pip install tensorflow==2.13.0
pip install numpy pillow matplotlib
pip install tensorflow-lite  # For conversion
```

#### Step 2: Prepare Dataset

```python
# prepare_dataset.py
import tensorflow as tf
from tensorflow.keras.preprocessing import image_dataset_from_directory
import pathlib

# Configuration
DATASET_DIR = pathlib.Path("dataset")
IMAGE_SIZE = (96, 96)
BATCH_SIZE = 32
NUM_CLASSES = 3  # Person, Vehicle, Animal

# Load dataset
train_ds = image_dataset_from_directory(
    DATASET_DIR,
    validation_split=0.2,
    subset="training",
    seed=123,
    image_size=IMAGE_SIZE,
    batch_size=BATCH_SIZE
)

val_ds = image_dataset_from_directory(
    DATASET_DIR,
    validation_split=0.2,
    subset="validation",
    seed=123,
    image_size=IMAGE_SIZE,
    batch_size=BATCH_SIZE
)

# Class names
class_names = train_ds.class_names
print(f"Classes: {class_names}")

# Prefetch for performance
AUTOTUNE = tf.data.AUTOTUNE
train_ds = train_ds.prefetch(buffer_size=AUTOTUNE)
val_ds = val_ds.prefetch(buffer_size=AUTOTUNE)
```

#### Step 3: Define Model Architecture

**Option A: Simple CNN (for classification)**

```python
# model_simple_cnn.py
import tensorflow as tf

def create_simple_cnn(input_shape, num_classes):
    """Simple CNN for image classification"""

    model = tf.keras.Sequential([
        # Input layer
        tf.keras.layers.Input(shape=input_shape),

        # Block 1
        tf.keras.layers.Conv2D(32, 3, activation='relu', padding='same'),
        tf.keras.layers.BatchNormalization(),
        tf.keras.layers.MaxPooling2D(2),

        # Block 2
        tf.keras.layers.Conv2D(64, 3, activation='relu', padding='same'),
        tf.keras.layers.BatchNormalization(),
        tf.keras.layers.MaxPooling2D(2),

        # Block 3
        tf.keras.layers.Conv2D(128, 3, activation='relu', padding='same'),
        tf.keras.layers.BatchNormalization(),
        tf.keras.layers.MaxPooling2D(2),

        # Classifier
        tf.keras.layers.GlobalAveragePooling2D(),
        tf.keras.layers.Dense(256, activation='relu'),
        tf.keras.layers.Dropout(0.5),
        tf.keras.layers.Dense(num_classes, activation='softmax')
    ])

    return model

# Create model
model = create_simple_cnn(
    input_shape=(96, 96, 3),
    num_classes=NUM_CLASSES
)

# Compile
model.compile(
    optimizer='adam',
    loss='sparse_categorical_crossentropy',
    metrics=['accuracy']
)

model.summary()
```

**Option B: MobileNetV2 (transfer learning)**

```python
# model_mobilenet.py
def create_mobilenet(input_shape, num_classes):
    """MobileNetV2 with transfer learning"""

    # Load pre-trained MobileNetV2
    base_model = tf.keras.applications.MobileNetV2(
        input_shape=input_shape,
        include_top=False,
        weights='imagenet'
    )

    # Freeze base model
    base_model.trainable = False

    # Add custom head
    inputs = tf.keras.Input(shape=input_shape)
    x = tf.keras.applications.mobilenet_v2.preprocess_input(inputs)
    x = base_model(x, training=False)
    x = tf.keras.layers.GlobalAveragePooling2D()(x)
    x = tf.keras.layers.Dropout(0.2)(x)
    outputs = tf.keras.layers.Dense(num_classes, activation='softmax')(x)

    model = tf.keras.Model(inputs, outputs)

    return model

model = create_mobilenet((96, 96, 3), NUM_CLASSES)
model.compile(
    optimizer=tf.keras.optimizers.Adam(1e-3),
    loss='sparse_categorical_crossentropy',
    metrics=['accuracy']
)
```

**Option C: SSD MobileNet (for object detection)**

```python
# model_ssd.py
def create_ssd_model(num_classes):
    """SSD MobileNet for object detection"""

    # Use TensorFlow Object Detection API
    # Install: pip install tensorflow-object-detection-api

    # This requires more complex setup
    # See: https://github.com/tensorflow/models/blob/master/research/object_detection/g3doc/tf1_detection_zoo.md

    pass  # Implementation requires TF OD API
```

#### Step 4: Train Model

```python
# train.py
import tensorflow as tf
from tensorflow.keras.callbacks import EarlyStopping, ModelCheckpoint

# Callbacks
early_stopping = EarlyStopping(
    monitor='val_loss',
    patience=10,
    restore_best_weights=True
)

checkpoint = ModelCheckpoint(
    'best_model.h5',
    monitor='val_accuracy',
    save_best_only=True
)

# Train
history = model.fit(
    train_ds,
    validation_data=val_ds,
    epochs=100,
    callbacks=[early_stopping, checkpoint]
)

# Plot training history
import matplotlib.pyplot as plt

plt.figure(figsize=(12, 4))

plt.subplot(1, 2, 1)
plt.plot(history.history['accuracy'], label='Training')
plt.plot(history.history['val_accuracy'], label='Validation')
plt.title('Accuracy')
plt.legend()

plt.subplot(1, 2, 2)
plt.plot(history.history['loss'], label='Training')
plt.plot(history.history['val_loss'], label='Validation')
plt.title('Loss')
plt.legend()

plt.savefig('training_history.png')
plt.show()

# Evaluate
test_loss, test_acc = model.evaluate(val_ds)
print(f"Test accuracy: {test_acc:.2%}")
```

#### Step 5: Test Model

```python
# test_model.py
import numpy as np
from PIL import Image

# Load test image
def load_and_preprocess(image_path):
    img = Image.open(image_path).convert('RGB')
    img = img.resize((96, 96))
    img_array = np.array(img) / 255.0
    return np.expand_dims(img_array, axis=0)

# Predict
image_path = "test_image.jpg"
image = load_and_preprocess(image_path)
predictions = model.predict(image)

# Get top prediction
predicted_class = np.argmax(predictions[0])
confidence = predictions[0][predicted_class]

print(f"Predicted: {class_names[predicted_class]}")
print(f"Confidence: {confidence:.2%}")
```

---

## Model Conversion to TFLite Micro

### Step 1: Convert to TFLite

```python
# convert_to_tflite.py
import tensorflow as tf

# Load trained model
model = tf.keras.models.load_model('best_model.h5')

# Convert to TensorFlow Lite
converter = tf.lite.TFLiteConverter.from_keras_model(model)

# Optimization: Quantization
converter.optimizations = [tf.lite.Optimize.DEFAULT]

# For quantized model (recommended for microcontrollers)
def representative_dataset():
    for i in range(100):
        data = np.random.rand(1, 96, 96, 3).astype(np.float32)
        yield [data]

converter.target_spec.supported_ops = [tf.lite.OpsSet.TFLITE_BUILTINS_INT8]
converter.inference_input_type = tf.uint8  # or tf.int8
converter.inference_output_type = tf.uint8  # or tf.int8
converter.representative_dataset = representative_dataset

# Convert
tflite_model = converter.convert()

# Save
with open('model_quantized.tflite', 'wb') as f:
    f.write(tflite_model)

print("Model converted successfully!")
print(f"Model size: {len(tflite_model) / 1024:.1f} KB")
```

### Step 2: Convert to C Array

**Linux/macOS:**

```bash
xxd -i model_quantized.tflite > model_data.cc
```

**Windows (PowerShell):**

```powershell
# Use Python instead
python -c "
import sys
with open('model_quantized.tflite', 'rb') as f:
    data = f.read()
print('const unsigned char g_model[] = {')
print(','.join(f'0x{b:02x}' for b in data))
print('};')
print(f'const unsigned int g_model_len = {len(data)};')
" > model_data.cc
```

### Step 3: Verify Model

```python
# verify_model.py
import tensorflow as tf
import numpy as np

# Load TFLite model
interpreter = tf.lite.Interpreter(model_path='model_quantized.tflite')
interpreter.allocate_tensors()

# Get details
input_details = interpreter.get_input_details()
output_details = interpreter.get_output_details()

print("Input shape:", input_details[0]['shape'])
print("Input type:", input_details[0]['dtype'])
print("Output shape:", output_details[0]['shape'])
print("Output type:", output_details[0]['dtype'])

# Test inference
test_input = np.random.randint(0, 256, input_details[0]['shape'], dtype=np.uint8)
interpreter.set_tensor(input_details[0]['index'], test_input)
interpreter.invoke()
output = interpreter.get_tensor(output_details[0]['index'])

print("Output:", output)
```

---

## Deployment Steps

### Step 1: Prepare Arduino Project

1. **Copy Model Data**:
   ```bash
   # Copy model_data.cc to your project
   cp model_data.cc ~/Arduino/your_project/src/
   ```

2. **Update Model Include**:
   ```cpp
   // In your sketch
   #include "model_data.cc"

   // Global model
   const tflite::Model* model = nullptr;
   ```

3. **Update Model Loading**:
   ```cpp
   bool initializeMLModel() {
     // Load model from data
     model = tflite::GetModel(g_model);

     if (model->version() != TFLITE_SCHEMA_VERSION) {
       Serial.println("Model version mismatch!");
       return false;
     }

     // Create interpreter
     static tflite::AllOpsResolver resolver;
     static tflite::MicroInterpreter static_interpreter(
       model, resolver, tensor_arena, kTensorArenaSize, error_reporter
     );

     interpreter = &static_interpreter;

     // Allocate tensors
     if (interpreter->AllocateTensors() != kTfLiteOk) {
       Serial.println("Failed to allocate tensors!");
       return false;
     }

     return true;
   }
   ```

### Step 2: Configure Hardware

1. **Connect I2C Multiplexer**:
   - SDA: MKR WAN pin 11
   - SCL: MKR WAN pin 12
   - VCC: 5V
   - GND: GND

2. **Connect Cameras**:
   - Camera 1: TCA9548A Channel 0
   - Camera 2: TCA9548A Channel 1

3. **Connect Alarm**:
   - LED: D2 (with 330Ω resistor)
   - Buzzer: D3 (with 330Ω resistor)

### Step 3: Upload Firmware

```bash
# Using Arduino CLI
arduino-cli compile --fqbn arduino:mbed_nicla:nicla_vision dual_camera_ml.cpp
arduino-cli upload -p /dev/ttyACM0 --fqbn arduino:mbed_nicla:nicla_vision

# Or use Arduino IDE 2.x
# Select Board: Tools -> Board -> "Arduino Nicla Vision"
# Select Port: Tools -> Port -> /dev/ttyACM0
# Click Upload
```

### Step 4: Monitor Output

```bash
# Open serial monitor
arduino-cli monitor -p /dev/ttyACM0 -c baudrate=115200

# Expected output:
# Initializing ML model...
# Loading model...
#   Name: Person Detection
#   Version: 1.0.0
#   Input dimensions: 1x96x96x3
#   Input type: uint8
# Model initialized successfully
# Camera 1: Person (85.3%) [0,0,96,96]
```

---

## Performance Optimization

### 1. Model Size Reduction

**Reduce Model Dimensions**:
```python
# Smaller input size
IMAGE_SIZE = (64, 64)  # Instead of (96, 96)

# Fewer filters
model = tf.keras.Sequential([
    tf.keras.layers.Conv2D(16, 3),  # Instead of 32
    tf.keras.layers.Conv2D(32, 3),  # Instead of 64
    # ...
])
```

**Prune Model**:
```python
import tensorflow_model_optimization as tfmot

# Apply pruning
prune_low_magnitude = tfmot.sparsity.keras.prune_low_magnitude

model = prune_low_magnitude(
    model,
    pruning_schedule=tfmot.sparsity.keras.PolynomialDecay(
        initial_sparsity=0.0,
        final_sparsity=0.5,
        begin_step=0,
        end_step=1000
    )
)

# Train and convert
model.compile(optimizer='adam', loss='sparse_categorical_crossentropy')
model.fit(train_ds, epochs=50)

# Strip wrappers for conversion
model_for_export = tfmot.sparsity.keras.strip_pruning(model)
```

### 2. Inference Speed Optimization

**Use Quantized Models**:
- Int8 quantization: 2-4x faster than float
- Reduces memory bandwidth
- Lower power consumption

**Optimize Preprocessing**:
```cpp
// Use fixed-point arithmetic instead of float
void resizeImageFixedPoint(uint8_t* src, uint8_t* dst) {
    // Use int16_t instead of float for calculations
    int16_t xRatio = ((int32_t)srcWidth << 8) / dstWidth;  // Fixed-point 8.8

    for (int y = 0; y < dstHeight; y++) {
        for (int x = 0; x < dstWidth; x++) {
            int16_t srcX = ((int32_t)x * xRatio) >> 8;
            // ...
        }
    }
}
```

**Enable Hardware Acceleration** (if available):
```cpp
// For STM32H7 with ART Accelerator
// Enable I-Cache
SCB_EnableICache();

// Enable D-Cache
SCB_EnableDCache();
```

### 3. Memory Optimization

**Reduce Tensor Arena**:
```cpp
// Monitor actual usage and adjust
constexpr int kTensorArenaSize = 300 * 1024;  // Instead of 500KB

// Check if allocation succeeds
if (interpreter->AllocateTensors() != kTfLiteOk) {
  Serial.println("Tensor arena too small!");
}
```

**Reuse Buffers**:
```cpp
// Don't allocate per inference
// Use static/global buffers
static uint8_t preprocessingBuffer[96 * 96 * 3];

void runInference(uint8_t* image, DetectionResult* result) {
  // Reuse preprocessingBuffer
  preprocessImage(image, preprocessingBuffer);
  // ...
}
```

**Memory Pooling**:
```cpp
// Allocate all memory at startup
class MemoryPool {
public:
  static uint8_t* allocate(size_t size) {
    static uint8_t pool[1024 * 100];  // 100KB pool
    static size_t offset = 0;

    if (offset + size > sizeof(pool)) return nullptr;
    uint8_t* ptr = &pool[offset];
    offset += size;
    return ptr;
  }
};
```

### 4. Frame Rate Optimization

**Adaptive Frame Rate**:
```cpp
// Reduce FPS when no activity detected
uint32_t getFrameInterval() {
  if (detectionBuffer1.size() == 0 && detectionBuffer2.size() == 0) {
    return 5000;  // 0.2 FPS when idle
  } else {
    return 1000;  // 1 FPS when active
  }
}

void loop() {
  static unsigned long lastCapture = 0;
  uint32_t interval = getFrameInterval();

  if (millis() - lastCapture >= interval) {
    processDetections();
    lastCapture = millis();
  }
}
```

**Skip Frames**:
```cpp
// Process every Nth frame
#define FRAME_SKIP 2
static uint32_t frameCount = 0;

void loop() {
  frameCount++;

  if (frameCount % FRAME_SKIP == 0) {
    processDetections();
  }
}
```

---

## Memory Usage Analysis

### Memory Breakdown

```
┌─────────────────────────────────────────┐
│ Total RAM: 1 MB (Nicla Vision)         │
├─────────────────────────────────────────┤
│                                         │
│ 1. Tensor Arena:        500 KB         │
│    - Model weights:     ~250 KB        │
│    - Intermediate:      ~250 KB        │
│                                         │
│ 2. Frame Buffers:        307 KB        │
│    - Camera 1:           153 KB        │
│    - Camera 2:           153 KB        │
│    (320x240x2 bytes each)              │
│                                         │
│ 3. Preprocessing Buffer:   27 KB       │
│    - 96x96x3 RGB                        │
│                                         │
│ 4. Detection Buffers:     ~2 KB        │
│    - 10 detections * 2 cameras         │
│                                         │
│ 5. Stack/Heap:           ~50 KB        │
│    - Runtime variables                 │
│                                         │
│ 6. System:              ~100 KB        │
│    - Arduino framework                  │
│    - Libraries                          │
│                                         │
│ Total Used:            ~986 KB         │
│ Remaining:              ~34 KB         │
└─────────────────────────────────────────┘
```

### Memory Optimization Strategies

1. **Reduce Input Resolution**:
   - 96x96 → 64x64: Saves 56% in model size
   - Trade-off: Lower accuracy

2. **Smaller Model**:
   - MobileNetV2 0.35 → 0.25: Saves 30%
   - Fewer layers/filters

3. **External PSRAM**:
   - Add external PSRAM chip (if supported)
   - Offload frame buffers

4. **Single Buffer Approach**:
   - Process one camera at a time
   - Saves 153 KB

### Monitoring Memory

```cpp
void printMemoryUsage() {
  extern "C" char* sbrk(int incr);
  char top;
  char* heapEnd = sbrk(0);

  Serial.println("=== Memory Usage ===");
  Serial.print("Stack: ");
  Serial.println((int)&top - (int)heapEnd);
  Serial.print("Heap: ");
  Serial.println((int)heapEnd - (int)__brkval);
  Serial.print("Free: ");
  Serial.println(freeMemory());
}
```

---

## Troubleshooting

### Problem: "Model version mismatch"

**Cause**: TensorFlow version incompatibility

**Solution**:
```bash
# Ensure matching versions
pip install tensorflow==2.13.0
pip install arduino-tflite-runtime

# Reconvert model with correct version
python convert_to_tflite.py
```

### Problem: "Failed to allocate tensors"

**Cause**: Tensor arena too small for model

**Solution**:
```cpp
// Increase tensor arena size
constexpr int kTensorArenaSize = 600 * 1024;  // Increase to 600KB

// Or reduce model size
// - Use smaller input dimensions
// - Fewer layers/filters
```

### Problem: "Camera initialization failed"

**Cause**: I2C multiplexer not detected or wiring issue

**Solution**:
```cpp
// Test I2C connection
#include <Wire.h>

void scanI2C() {
  Serial.println("Scanning I2C bus...");

  for (byte addr = 1; addr < 127; addr++) {
    Wire.beginTransmission(addr);
    byte error = Wire.endTransmission();

    if (error == 0) {
      Serial.print("Device found at 0x");
      Serial.println(addr, HEX);
    }
  }
}

// In setup()
scanI2C();
// Should see TCA9548A at 0x70
```

**Wiring Checklist**:
- ✓ SDA connected to correct pin
- ✓ SCL connected to correct pin
- ✓ VCC connected to 5V (not 3.3V)
- ✓ GND connected
- ✓ Pull-up resistors present (4.7kΩ on SDA/SCL)

### Problem: "Low detection accuracy"

**Cause**: Model under-trained or poor data quality

**Solution**:

1. **Check Training Data**:
   - Minimum 500 images per class
   - Diverse lighting/angles
   - Balanced classes

2. **Retrain with More Data**:
   ```python
   # Use data augmentation
   train_datagen = tf.keras.preprocessing.image.ImageDataGenerator(
       rotation_range=20,
       width_shift_range=0.2,
       height_shift_range=0.2,
       horizontal_flip=True,
       zoom_range=0.2
   )
   ```

3. **Adjust Thresholds**:
   ```cpp
   #define DETECTION_THRESHOLD 0.4f  // Lower from 0.5
   #define CONFIDENCE_THRESHOLD 0.5f  // Lower from 0.6
   ```

4. **Improve Lighting**:
   - Add LED illumination
   - Avoid backlight
   - Consistent exposure

### Problem: "Slow inference (>500ms)"

**Cause**: Model too large or unoptimized

**Solution**:

1. **Use Quantized Model**:
   ```python
   # Ensure int8 quantization
   converter.target_spec.supported_types = [tf.int8]
   converter.inference_input_type = tf.int8
   ```

2. **Reduce Input Size**:
   ```python
   # Retrain with smaller input
   IMAGE_SIZE = (64, 64)  # Instead of (96, 96)
   ```

3. **Enable Caching**:
   ```cpp
   // Enable I-Cache on STM32H7
   SCB_EnableICache();
   SCB_EnableDCache();
   ```

### Problem: "Random crashes/reboots"

**Cause**: Stack overflow or memory corruption

**Solution**:

1. **Increase Stack Size**:
   - In platformio.ini: `build_flags = -DSTACK_SIZE=8192`

2. **Check for Memory Leaks**:
   ```cpp
   // Avoid dynamic allocation
   // Use static/global buffers instead

   // Bad:
   void process() {
       uint8_t* buffer = new uint8_t[1024];  // Leak!
       // ...
       delete[] buffer;  // Might forget
   }

   // Good:
   static uint8_t buffer[1024];
   void process() {
       // Use buffer
   }
   ```

3. **Monitor Free Memory**:
   ```cpp
   if (freeMemory() < 1024) {
       Serial.println("Low memory warning!");
       // Take action
   }
   ```

---

## Advanced Features

### 1. Multi-Model Support

Switch between different models based on requirements:

```cpp
// Load different models
void setup() {
  // Default: Person detection
  loadPersonDetectionModel();
}

void loop() {
  // Switch to vehicle detection at night
  if (hour() >= 18 || hour() <= 6) {
    switchModel(MODEL_TYPE_VEHICLE_DETECTION);
  }

  processDetections();
}
```

### 2. Custom Model Loading

Load models from external storage (SD card):

```cpp
#include <SD.h>

bool loadModelFromSD(const char* filename) {
  File modelFile = SD.open(filename);

  if (!modelFile) {
    Serial.println("Failed to open model file!");
    return false;
  }

  uint32_t modelSize = modelFile.size();
  uint8_t* modelBuffer = (uint8_t*)malloc(modelSize);

  modelFile.read(modelBuffer, modelSize);
  modelFile.close();

  ModelMetadata customModel = {
    "Custom Model",
    "1.0.0",
    MODEL_TYPE_CUSTOM,
    modelBuffer,
    modelSize,
    96, 96, NUM_CLASSES,
    DETECTION_THRESHOLD,
    true
  };

  return loadModel(customModel);
}
```

### 3. OTA Model Updates

Update models over-the-air:

```cpp
// Receive model via WiFi/LoRaWAN
bool updateModelOTA() {
  // Connect to server
  WiFiClient client;
  if (!client.connect("server.com", 80)) {
    return false;
  }

  // Download model
  client.println("GET /models/person_v2.tflite HTTP/1.0");
  // ... read response ...

  // Save to filesystem/SD card
  // Load new model
  loadModelFromSD("/models/person_v2.tflite");

  Serial.println("Model updated successfully!");
  return true;
}
```

### 4. Detection Tracking

Track objects across frames:

```cpp
struct TrackedObject {
  uint8_t id;
  uint8_t classId;
  BoundingBox bbox;
  uint32_t lastSeen;
  int frameCount;
};

TrackedObject trackedObjects[10];
uint8_t numTracked = 0;

void updateTracking(const DetectionResult& detection) {
  // Find matching tracked object
  for (int i = 0; i < numTracked; i++) {
    float iou = trackedObjects[i].bbox.calculateIoU(detection.boundingBox);

    if (iou > 0.5 &&
        trackedObjects[i].classId == detection.classId) {
      // Update existing track
      trackedObjects[i].bbox = detection.boundingBox;
      trackedObjects[i].lastSeen = detection.timestamp;
      trackedObjects[i].frameCount++;
      return;
    }
  }

  // Create new track
  if (numTracked < 10) {
    trackedObjects[numTracked].id = numTracked;
    trackedObjects[numTracked].classId = detection.classId;
    trackedObjects[numTracked].bbox = detection.boundingBox;
    trackedObjects[numTracked].lastSeen = detection.timestamp;
    trackedObjects[numTracked].frameCount = 1;
    numTracked++;
  }
}
```

### 5. Adaptive Thresholds

Adjust detection thresholds based on conditions:

```cpp
float getAdaptiveThreshold() {
  // Higher threshold during day (more false positives)
  // Lower threshold during night (more false negatives)

  int hour = (millis() / 3600000) % 24;

  if (hour >= 6 && hour <= 18) {
    return 0.6f;  // Day
  } else {
    return 0.4f;  // Night
  }
}

void processDetections() {
  float threshold = getAdaptiveThreshold();

  DetectionResult result;
  runInference(frameBuffer1, &result);

  if (result.confidence >= threshold) {
    // Valid detection
  }
}
```

### 6. Detection Logging

Log detections to SD card for analysis:

```cpp
#include <SD.h>

File logFile;

void setupLogging() {
  SD.begin();
  logFile = SD.open("detections.csv", FILE_WRITE);
  logFile.println("timestamp,camera_id,class,confidence,x,y,width,height");
}

void logDetection(const DetectionResult& result) {
  logFile.print(result.timestamp);
  logFile.print(",");
  logFile.print(result.cameraId);
  logFile.print(",");
  logFile.print(classNames[result.classId]);
  logFile.print(",");
  logFile.print(result.confidence);
  logFile.print(",");
  logFile.print(result.boundingBox.x);
  logFile.print(",");
  logFile.print(result.boundingBox.y);
  logFile.print(",");
  logFile.print(result.boundingBox.width);
  logFile.print(",");
  logFile.println(result.boundingBox.height);

  logFile.flush();
}
```

---

## Summary

This guide provides a complete workflow for:

1. **Training** ML models for object detection
2. **Converting** models to TFLite Micro format
3. **Deploying** to Arduino Nicla Vision
4. **Optimizing** for performance and memory
5. **Troubleshooting** common issues

### Key Takeaways

- **Start with Edge Impulse** for rapid prototyping
- **Use quantized models** (int8) for best performance
- **Monitor memory usage** carefully (500KB tensor arena)
- **Optimize inference** to <200ms per camera
- **Test thoroughly** before deployment

### Resources

- [TensorFlow Lite Micro](https://www.tensorflow.org/lite/microcontrollers)
- [Edge Impulse Documentation](https://docs.edgeimpulse.com)
- [Arduino TensorFlow Lite](https://github.com/tensorflow/tflite-micro-arduino-examples)
- [TinyML Book](https://www.oreilly.com/library/view/tinyml/9781492052036/)

---

**Next Steps**: Integrate with your IoT system and deploy!

For questions or issues, refer to the troubleshooting section or consult the TensorFlow Lite Micro documentation.
