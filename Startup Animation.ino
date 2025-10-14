#include <bluefruit.h>
#include <Adafruit_CircuitPlayground.h>
#include <math.h>

// BLE UART setup
BLEUart bleuart;

// Calibration offsets
float offsetX = 0, offsetY = 0, offsetZ = 0;

// Smoothing and persistence
float smoothX = 0, smoothY = 0, smoothZ = 0;
const float SMOOTH_FACTOR = 0.15;

// Accel/brake hold logic
bool accelActive = false, brakeActive = false;
unsigned long accelTimer = 0, brakeTimer = 0;
const unsigned long HOLD_TIME = 500; // ms

// Bounce tracking
float bounceFade = 0.0;
const float bounceFadeSpeed = 0.05;

// Utility color wheel
uint32_t colorWheel(byte pos) {
  pos = 255 - pos;
  if (pos < 85) {
    return CircuitPlayground.strip.Color(255 - pos * 3, 0, pos * 3);
  } else if (pos < 170) {
    pos -= 85;
    return CircuitPlayground.strip.Color(0, pos * 3, 255 - pos * 3);
  } else {
    pos -= 170;
    return CircuitPlayground.strip.Color(pos * 3, 255 - pos * 3, 0);
  }
}

// Get color by intensity (Green → Yellow → Orange → Red)
uint32_t getIntensityColor(float gForce) {
  if (gForce > 1.8) return CircuitPlayground.strip.Color(255, 0, 0);       // Red
  else if (gForce > 1.2) return CircuitPlayground.strip.Color(255, 80, 0); // Orange
  else if (gForce > 0.7) return CircuitPlayground.strip.Color(255, 150, 0);// Yellow
  else if (gForce > 0.3) return CircuitPlayground.strip.Color(0, 255, 0);  // Green
  else return CircuitPlayground.strip.Color(0, 0, 0);
}

// === Startup animation ===
void startupAnimation() {
  for (int i = 0; i < 10; i++) {
    CircuitPlayground.clearPixels();
    CircuitPlayground.setPixelColor(i, 0, 0, 255); // Blue loading LED
    CircuitPlayground.strip.show();
    delay(100);
  }
  CircuitPlayground.clearPixels();
  for (int i = 0; i < 10; i++) CircuitPlayground.setPixelColor(i, 255, 255, 255);
  delay(200);
  CircuitPlayground.clearPixels();
}

void setup() {
  Serial.begin(115200);
  CircuitPlayground.begin();
  Bluefruit.begin();
  bleuart.begin();
  Bluefruit.setTxPower(4);
  Bluefruit.setName("ForceGauge");
  Bluefruit.Advertising.addService(bleuart);
  Bluefruit.Advertising.start();

  startupAnimation();

  // Calibrate at rest
  for (int i = 0; i < 100; i++) {
    offsetX += CircuitPlayground.motionX();
    offsetY += CircuitPlayground.motionY();
    offsetZ += CircuitPlayground.motionZ();
    delay(10);
  }
  offsetX /= 100;
  offsetY /= 100;
  offsetZ /= 100;
}

void loop() {
  // Read accelerometer and apply calibration
  float x = CircuitPlayground.motionX() - offsetX;
  float y = CircuitPlayground.motionY() - offsetY;
  float z = CircuitPlayground.motionZ() - offsetZ;

  // Smooth data
  smoothX = smoothX * (1 - SMOOTH_FACTOR) + x * SMOOTH_FACTOR;
  smoothY = smoothY * (1 - SMOOTH_FACTOR) + y * SMOOTH_FACTOR;
  smoothZ = smoothZ * (1 - SMOOTH_FACTOR) + z * SMOOTH_FACTOR;

  // Clear LEDs
  CircuitPlayground.clearPixels();

  // === Turning logic (left/right) ===
  float turnForce = fabs(smoothX);
  uint32_t turnColor = getIntensityColor(turnForce);

  if (turnForce > 0.3) {
    if (smoothX > 0.3) { // Right turn
      CircuitPlayground.setPixelColor(2, turnColor);
      CircuitPlayground.setPixelColor(1, turnColor);
      CircuitPlayground.setPixelColor(0, turnColor);
    } else if (smoothX < -0.3) { // Left turn
      CircuitPlayground.setPixelColor(2, turnColor);
      CircuitPlayground.setPixelColor(3, turnColor);
      CircuitPlayground.setPixelColor(4, turnColor);
    }
  }

  // === Acceleration / Braking logic (swapped colors) ===
  unsigned long now = millis();
  if (smoothZ > 0.15) {
    // Braking (positive Z) → GREEN
    brakeActive = true;
    brakeTimer = now + HOLD_TIME;
  } else if (smoothZ < -0.15) {
    // Acceleration (negative Z) → RED
    accelActive = true;
    accelTimer = now + HOLD_TIME;
  }

  // Apply braking (green hold)
  if (brakeActive && now < brakeTimer) {
    CircuitPlayground.setPixelColor(5, 0, 255, 0);
    CircuitPlayground.setPixelColor(6, 0, 200, 0);
    CircuitPlayground.setPixelColor(8, 0, 150, 0);
    CircuitPlayground.setPixelColor(9, 0, 100, 0);
  } else if (now >= brakeTimer) brakeActive = false;

  // Apply acceleration (red hold)
  if (accelActive && now < accelTimer) {
    CircuitPlayground.setPixelColor(5, 255, 0, 0);
    CircuitPlayground.setPixelColor(6, 200, 0, 0);
    CircuitPlayground.setPixelColor(8, 150, 0, 0);
    CircuitPlayground.setPixelColor(9, 100, 0, 0);
  } else if (now >= accelTimer) accelActive = false;

  // === Bounce logic (purple flash fade) ===
  if (fabs(smoothY) > 1.0) bounceFade = 1.0;

  if (bounceFade > 0.0) {
    int r = 180 * bounceFade;
    int g = 0;
    int b = 255 * bounceFade;
    CircuitPlayground.setPixelColor(7, r, g, b);
    bounceFade -= bounceFadeSpeed;
  }

  CircuitPlayground.strip.show();

  // BLE debug stream
  bleuart.print("X: "); bleuart.print(smoothX);
  bleuart.print(", Y: "); bleuart.print(smoothY);
  bleuart.print(", Z: "); bleuart.println(smoothZ);

  delay(30);
}
