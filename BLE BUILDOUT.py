#include <bluefruit.h>
#include <Adafruit_CircuitPlayground.h>

// Calibration variables
float calX = 0, calY = 0, calZ = 0;

// Raw forces
float rawTurning = 0;
float rawBounce = 0;
float rawBrakeAccel = 0;

// Smoothed forces (EMA smoothing)
float smoothTurning = 0;
float smoothBounce = 0;
float smoothBrakeAccel = 0;
const float smoothingFactor = 0.1;  // Smaller = smoother, 0.1 = decent smoothing

// Thresholds
int turnThreshold = 100;
int brakeThreshold = 50;
int bounceThreshold = 150;

BLEUart bleuart;

// Helper to map force to 0-255 brightness with clamp
uint8_t mapForceToBrightness(float force, float threshold) {
  float val = fabs(force) / threshold;
  if (val > 1.0) val = 1.0;
  return (uint8_t)(val * 255);
}

void calibrate() {
  Serial.println("Calibrating accelerometer...");
  float sumX = 0, sumY = 0, sumZ = 0;
  for (int i = 0; i < 50; i++) {
    sumX += CircuitPlayground.motionX();
    sumY += CircuitPlayground.motionY();
    sumZ += CircuitPlayground.motionZ();
    delay(20);
  }
  calX = sumX / 50;
  calY = sumY / 50;
  calZ = sumZ / 50;
  Serial.println("Calibration done.");
}

void setup() {
  Serial.begin(9600);
  CircuitPlayground.begin();

  Bluefruit.begin();
  Bluefruit.setName("G-Force Sensor");
  bleuart.begin();

  Bluefruit.Advertising.addFlags(BLE_GAP_ADV_FLAGS_LE_ONLY_GENERAL_DISC_MODE);
  Bluefruit.Advertising.addTxPower();
  Bluefruit.Advertising.addName();
  Bluefruit.Advertising.addService(bleuart);
  Bluefruit.Advertising.restartOnDisconnect(true);
  Bluefruit.Advertising.setInterval(32, 244);
  Bluefruit.Advertising.setFastTimeout(30);
  Bluefruit.Advertising.start(0);

  calibrate();
}

void sendBLEData() {
  if (Bluefruit.connected()) {
    bleuart.print("Turning: "); bleuart.print(smoothTurning, 2);
    bleuart.print(", Bounce: "); bleuart.print(smoothBounce, 2);
    bleuart.print(", BrakeAccel: "); bleuart.print(smoothBrakeAccel, 2);
    bleuart.println();
  }
}

void loop() {
  // Read raw accelerometer data (correct for calibration)
  rawTurning = (CircuitPlayground.motionX() - calX) * 100;
  rawBounce = (CircuitPlayground.motionY() - calY) * 100;
  rawBrakeAccel = (CircuitPlayground.motionZ() - calZ) * 100;

  // Smooth with EMA filter
  smoothTurning = smoothingFactor * rawTurning + (1 - smoothingFactor) * smoothTurning;
  smoothBounce = smoothingFactor * rawBounce + (1 - smoothingFactor) * smoothBounce;
  smoothBrakeAccel = smoothingFactor * rawBrakeAccel + (1 - smoothingFactor) * smoothBrakeAccel;

  // Send live data over BLE
  sendBLEData();

  // Clear previous LED states
  CircuitPlayground.clearPixels();

  // ====== TURNING LEFT and RIGHT ======
  // Left turn LEDs: 0,1
  // Right turn LEDs: 3,4
  // Center LED #2 lights dim blue if no turn
  // Blue color (low intensity center, brighter outside)

  uint8_t turnBrightnessOuter = mapForceToBrightness(smoothTurning, turnThreshold);
  uint8_t turnBrightnessCenter = turnBrightnessOuter / 3;

  if (smoothTurning < -turnThreshold) {
    // Left turn (negative)
    CircuitPlayground.setPixelColor(0, 0, 0, turnBrightnessOuter); // bright blue outer left
    CircuitPlayground.setPixelColor(1, 0, 0, turnBrightnessOuter / 2); // mid blue
  } else if (smoothTurning > turnThreshold) {
    // Right turn (positive)
    CircuitPlayground.setPixelColor(4, 0, 0, turnBrightnessOuter); // bright blue outer right
    CircuitPlayground.setPixelColor(3, 0, 0, turnBrightnessOuter / 2); // mid blue
  } else {
    // No turn: center LED #2 dim blue
    CircuitPlayground.setPixelColor(2, 0, 0, 50);
  }

  // ====== ACCELERATION and BRAKING ======
  // LEDs: 5,6,7,8,9
  // Acceleration: red (negative z)
  // Braking: green (positive z)
  // Fade with outside LEDs brighter (5 and 9 outer, 6 and 8 mid, 7 center)

  uint8_t accelBrightness = (smoothBrakeAccel < -brakeThreshold) ? mapForceToBrightness(-smoothBrakeAccel, brakeThreshold) : 0;
  uint8_t brakeBrightness = (smoothBrakeAccel > brakeThreshold) ? mapForceToBrightness(smoothBrakeAccel, brakeThreshold) : 0;

  if (accelBrightness > 0) {
    CircuitPlayground.setPixelColor(5, accelBrightness, 0, 0);          // outer left - bright red
    CircuitPlayground.setPixelColor(6, accelBrightness / 2, 0, 0);      // mid left
    // LED 7 is reserved for bounce now, so skip here
    CircuitPlayground.setPixelColor(8, accelBrightness / 2, 0, 0);      // mid right
    CircuitPlayground.setPixelColor(9, accelBrightness, 0, 0);          // outer right - bright red
  } else if (brakeBrightness > 0) {
    CircuitPlayground.setPixelColor(5, 0, brakeBrightness, 0);          // outer left - bright green
    CircuitPlayground.setPixelColor(6, 0, brakeBrightness / 2, 0);      // mid left
    // LED 7 reserved for bounce
    CircuitPlayground.setPixelColor(8, 0, brakeBrightness / 2, 0);      // mid right
    CircuitPlayground.setPixelColor(9, 0, brakeBrightness, 0);          // outer right - bright green
  }

  // ====== BOUNCE ======
  // LED 7 is used for bounce, purple color (red + blue)
  // Intensity based on bounce magnitude, off if below threshold

  if (fabs(smoothBounce) > bounceThreshold) {
    uint8_t bounceBrightness = mapForceToBrightness(smoothBounce, bounceThreshold);
    CircuitPlayground.setPixelColor(7, bounceBrightness / 2, 0, bounceBrightness / 2);  // Purple, dimmed a bit
  } else {
    // Ensure LED 7 is off if no bounce
    CircuitPlayground.setPixelColor(7, 0, 0, 0);
  }

  delay(50);
}
