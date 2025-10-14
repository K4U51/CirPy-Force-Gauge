
FORCE GAUGE PROJECT

```============================
Hardware: Adafruit Circuit Playground Bluefruit
Features: Turning LEDs, Acceleration/Braking LEDs, Bounce LED, BLE UART, Smoothing, Startup animation

Modules:
1. TurningLED (Handles left/right turning with intensity)
2. AccelBrakeLED (Handles red/green acceleration and braking LEDs)
3. BounceLED (Handles purple bounce LED flashes)
4. ForceGauge (Main sketch)
5. ForceGauge.h / ForceGauge.cpp (Globals, config, and utility functions)
============================
```

```
/* ===================== LIBRARIES ===================== */
#include <bluefruit.h>
#include <Adafruit_CircuitPlayground.h>
#include <math.h>

/* ===================== BLE SETUP ===================== */
BLEUart bleuart;

/* ===================== GLOBAL VARIABLES ===================== */
// Calibration offsets
float offsetX = 0, offsetY = 0, offsetZ = 0;

// Smoothed accelerometer values
float smoothX = 0, smoothY = 0, smoothZ = 0;
const float SMOOTH_FACTOR = 0.15; // smoothing factor

// Accel/Brake hold
bool accelActive = false, brakeActive = false;
unsigned long accelTimer = 0, brakeTimer = 0;
const unsigned long HOLD_TIME = 500; // ms

// Bounce LED
float bounceFade = 0.0;
const float BOUNCE_THRESH = 1.0;
const float BOUNCE_FADE_SPEED = 0.05;

// Turning thresholds
const float TURN_THRESHOLDS[4] = {0.3, 0.7, 1.2, 1.8};

/* ===================== STARTUP ANIMATION ===================== */
void startupAnimation() {
  for (int i = 0; i < 10; i++) {
    CircuitPlayground.clearPixels();
    CircuitPlayground.setPixelColor(i, 0,0,255); // Blue loading LED
    CircuitPlayground.strip.show();
    delay(100);
  }
  CircuitPlayground.clearPixels();
  for (int i = 0; i < 10; i++)
    CircuitPlayground.setPixelColor(i, 255,255,255); // flash white
  delay(200);
  CircuitPlayground.clearPixels();
}

/* ===================== SENSOR CALIBRATION ===================== */
void calibrateSensors() {
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

/* ===================== TURNING LED LOGIC ===================== */
// Map turning intensity to color
uint32_t getTurnColor(float gForce) {
  if (gForce > TURN_THRESHOLDS[3]) return CircuitPlayground.strip.Color(255,0,0);      // Red
  else if (gForce > TURN_THRESHOLDS[2]) return CircuitPlayground.strip.Color(255,80,0); // Orange
  else if (gForce > TURN_THRESHOLDS[1]) return CircuitPlayground.strip.Color(255,150,0);// Yellow
  else if (gForce > TURN_THRESHOLDS[0]) return CircuitPlayground.strip.Color(0,255,0); // Green
  else return CircuitPlayground.strip.Color(0,0,0); // Off
}

// Apply turning LEDs based on X-axis input
void applyTurningLogic(float smoothX) {
  float turnForce = fabs(smoothX);
  uint32_t turnColor = getTurnColor(turnForce);

  if (turnForce > TURN_THRESHOLDS[0]) {
    if (smoothX > TURN_THRESHOLDS[0]) { // Right turn
      CircuitPlayground.setPixelColor(2, turnColor);
      CircuitPlayground.setPixelColor(1, turnColor);
      CircuitPlayground.setPixelColor(0, turnColor);
    } else if (smoothX < -TURN_THRESHOLDS[0]) { // Left turn
      CircuitPlayground.setPixelColor(2, turnColor);
      CircuitPlayground.setPixelColor(3, turnColor);
      CircuitPlayground.setPixelColor(4, turnColor);
    }
  }
}

/* ===================== ACCELERATION / BRAKING LOGIC ===================== */
void applyAccelBrakeLogic(float smoothZ) {
  unsigned long now = millis();

  // Detect acceleration or braking and start hold timer
  if (smoothZ > 0.15) {
    brakeActive = true; // Braking = GREEN
    brakeTimer = now + HOLD_TIME;
  } else if (smoothZ < -0.15) {
    accelActive = true; // Acceleration = RED
    accelTimer = now + HOLD_TIME;
  }

  // Hold LEDs if within timer
  if (brakeActive && now < brakeTimer) {
    CircuitPlayground.setPixelColor(5, 0, 255, 0);
    CircuitPlayground.setPixelColor(6, 0, 200, 0);
    CircuitPlayground.setPixelColor(8, 0, 150, 0);
    CircuitPlayground.setPixelColor(9, 0, 100, 0);
  } else if (now >= brakeTimer) brakeActive = false;

  if (accelActive && now < accelTimer) {
    CircuitPlayground.setPixelColor(5, 255, 0, 0);
    CircuitPlayground.setPixelColor(6, 200, 0, 0);
    CircuitPlayground.setPixelColor(8, 150, 0, 0);
    CircuitPlayground.setPixelColor(9, 100, 0, 0);
  } else if (now >= accelTimer) accelActive = false;
}

/* ===================== BOUNCE LED LOGIC ===================== */
void applyBounceLogic(float smoothY) {
  if (fabs(smoothY) > BOUNCE_THRESH) bounceFade = 1.0;

  if (bounceFade > 0.0) {
    int r = 180 * bounceFade;
    int g = 0;
    int b = 255 * bounceFade;
    CircuitPlayground.setPixelColor(7, r, g, b); // Purple flash
    bounceFade -= BOUNCE_FADE_SPEED;
  }
}

/* ===================== SETUP ===================== */
void setup() {
  Serial.begin(115200);
  CircuitPlayground.begin();

  // BLE
  Bluefruit.begin();
  bleuart.begin();
  Bluefruit.setTxPower(4);
  Bluefruit.setName("ForceGauge");
  Bluefruit.Advertising.addService(bleuart);
  Bluefruit.Advertising.start();

  startupAnimation();
  calibrateSensors();
}

/* ===================== MAIN LOOP ===================== */
void loop() {
  // Clear LEDs at start of each loop
  CircuitPlayground.clearPixels();

  // --- Read and smooth accelerometer ---
  float x = CircuitPlayground.motionX() - offsetX;
  float y = CircuitPlayground.motionY() - offsetY;
  float z = CircuitPlayground.motionZ() - offsetZ;

  smoothX = smoothX * (1 - SMOOTH_FACTOR) + x * SMOOTH_FACTOR;
  smoothY = smoothY * (1 - SMOOTH_FACTOR) + y * SMOOTH_FACTOR;
  smoothZ = smoothZ * (1 - SMOOTH_FACTOR) + z * SMOOTH_FACTOR;

  // --- Apply LED modules ---
  applyTurningLogic(smoothX);
  applyAccelBrakeLogic(smoothZ);
  applyBounceLogic(smoothY);

  // --- Update LEDs ---
  CircuitPlayground.strip.show();

  // --- BLE debug output ---
  bleuart.print("X: "); bleuart.print(smoothX);
  bleuart.print(", Y: "); bleuart.print(smoothY);
  bleuart.print(", Z: "); bleuart.println(smoothZ);

  delay(30); // Loop delay for responsiveness
}
