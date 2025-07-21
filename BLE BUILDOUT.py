// Initializes BLE UART
// Calibrates on boot
// Sends live G-force (X, Y, Z, Total) over BLE for graphing
// Visualizes G-force on NeoPixels with color and brightness
// Tracks peak G-force

#include <bluefruit.h>
#include <Adafruit_CircuitPlayground.h>

// BLE UART setup
BLEUart bleuart;

// Calibration and smoothing
float offsetX = 0, offsetY = 0, offsetZ = 0;
float peakForce = 0;
unsigned long peakTimestamp = 0;

const int numPixels = 10;
const int calibrationSamples = 100;
const int smoothingSamples = 5;

// G-force thresholds for NeoPixel color zones
const float gThresholdGreen = 1.5;
const float gThresholdYellow = 2.5;
const float gThresholdRed = 3.5;

void setup() {
  Serial.begin(115200);
  while (!Serial);  // Wait for Serial Monitor

  // Initialize Circuit Playground Bluefruit
  CircuitPlayground.begin();
  CircuitPlayground.clearPixels();

  // Initialize BLE
  Bluefruit.begin();
  Bluefruit.setTxPower(4);  // Max transmission power
  Bluefruit.setName("GForceGauge");
  bleuart.begin();

  Bluefruit.Advertising.addFlags(BLE_GAP_ADV_FLAGS_LE_ONLY_GENERAL_DISC_MODE);
  Bluefruit.Advertising.addTxPower();
  Bluefruit.Advertising.addService(bleuart);
  Bluefruit.Advertising.start();

  // Calibrate accelerometer
  Serial.println("Calibrating accelerometer...");
  calibrate();
}

void loop() {
  // Smoothed and calibrated sensor readings
  float x = getSmoothedReading('x') - offsetX;
  float y = getSmoothedReading('y') - offsetY;
  float z = getSmoothedReading('z') - offsetZ;

  float force = sqrt(x * x + y * y + z * z);

  // Log to Serial
  Serial.print("G-Force: ");
  Serial.print(force, 3);
  Serial.print(" | X: ");
  Serial.print(x, 3);
  Serial.print(" Y: ");
  Serial.print(y, 3);
  Serial.print(" Z: ");
  Serial.println(z, 3);

  // Send data over BLE in graph-friendly format: X,Y,Z,Total
  if (Bluefruit.connected()) {
    bleuart.print(x, 3);
    bleuart.print(",");
    bleuart.print(y, 3);
    bleuart.print(",");
    bleuart.print(z, 3);
    bleuart.print(",");
    bleuart.println(force, 3);
  }

  // Update peak force tracking
  if (force > peakForce) {
    peakForce = force;
    peakTimestamp = millis();
  }

  // Update NeoPixels based on force intensity
  updatePixels(force);
  delay(100);
}

void calibrate() {
  float sumX = 0, sumY = 0, sumZ = 0;

  for (int i = 0; i < calibrationSamples; i++) {
    sumX += CircuitPlayground.motionX();
    sumY += CircuitPlayground.motionY();
    sumZ += CircuitPlayground.motionZ();
    delay(10);
  }

  offsetX = sumX / calibrationSamples;
  offsetY = sumY / calibrationSamples;
  offsetZ = sumZ / calibrationSamples;

  Serial.print("Calibration complete. Offsets - X: ");
  Serial.print(offsetX, 3);
  Serial.print(" Y: ");
  Serial.print(offsetY, 3);
  Serial.print(" Z: ");
  Serial.println(offsetZ, 3);
}

float getSmoothedReading(char axis) {
  float sum = 0;
  for (int i = 0; i < smoothingSamples; i++) {
    switch (axis) {
      case 'x': sum += CircuitPlayground.motionX(); break;
      case 'y': sum += CircuitPlayground.motionY(); break;
      case 'z': sum += CircuitPlayground.motionZ(); break;
    }
    delay(5);
  }
  return sum / smoothingSamples;
}

void updatePixels(float force) {
  uint32_t color;

  if (force < gThresholdGreen) {
    color = CircuitPlayground.strip.Color(0, 255, 0);  // Green
  } else if (force < gThresholdYellow) {
    color = CircuitPlayground.strip.Color(255, 255, 0);  // Yellow
  } else if (force < gThresholdRed) {
    color = CircuitPlayground.strip.Color(255, 165, 0);  // Orange
  } else {
    color = CircuitPlayground.strip.Color(255, 0, 0);  // Red
  }

  for (int i = 0; i < numPixels; i++) {
    CircuitPlayground.setPixelColor(i, color);
  }

  CircuitPlayground.strip.show();
}
