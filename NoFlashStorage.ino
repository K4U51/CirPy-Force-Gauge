#include <Adafruit_CircuitPlayground.h>

float offsetX = 0, offsetY = 0, offsetZ = 0;
float peakForce = 0;
int peakIndex = 0;

const int numPixels = 10;
const float gThresholdGreen = 1.5;
const float gThresholdYellow = 2.5;
const float gThresholdRed = 3.5;

void setup() {
  Serial.begin(9600);
  CircuitPlayground.begin();

  // Calibrate on startup
  calibrate();
}

void loop() {
  float x = CircuitPlayground.motionX() - offsetX;
  float y = CircuitPlayground.motionY() - offsetY;
  float z = CircuitPlayground.motionZ() - offsetZ;

  float force = sqrt(x * x + y * y + z * z);

  Serial.print("G-Force: ");
  Serial.println(force);

  // Update peak force
  if (force > peakForce) {
    peakForce = force;
    peakIndex = millis();
  }

  // Set NeoPixels based on G-force
  for (int i = 0; i < numPixels; i++) {
    if (force < gThresholdGreen) {
      CircuitPlayground.setPixelColor(i, 0, 255, 0); // Green
    } else if (force < gThresholdYellow) {
      CircuitPlayground.setPixelColor(i, 255, 255, 0); // Yellow
    } else if (force < gThresholdRed) {
      CircuitPlayground.setPixelColor(i, 255, 165, 0); // Orange
    } else {
      CircuitPlayground.setPixelColor(i, 255, 0, 0); // Red
    }
  }

  delay(100);
}

void calibrate() {
  const int samples = 100;
  float sumX = 0, sumY = 0, sumZ = 0;

  for (int i = 0; i < samples; i++) {
    sumX += CircuitPlayground.motionX();
    sumY += CircuitPlayground.motionY();
    sumZ += CircuitPlayground.motionZ();
    delay(10);
  }

  offsetX = sumX / samples;
  offsetY = sumY / samples;
  offsetZ = sumZ / samples;

  Serial.print("Calibration complete. Offsets - X: ");
  Serial.print(offsetX);
  Serial.print(", Y: ");
  Serial.print(offsetY);
  Serial.print(", Z: ");
  Serial.println(offsetZ);
}
