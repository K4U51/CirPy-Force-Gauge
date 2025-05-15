#include <bluefruit.h>
#include <Adafruit_CircuitPlayground.h>

// Calibration
float calX = 0, calY = 0, calZ = 0;

// G-force data
int turningForce = 0;
int bouncingForce = 0;
int brakeAccelerateForce = 0;

// Decayed peak values
float turnDecay = 0, brakeDecay = 0, bounceDecay = 0;
float decayFactor = 0.95;

// BLE thresholds
int turnThreshold = 150;
int brakeThreshold = 100;
int bounceThreshold = 200;

BLEUart bleuart;

void calibrate() {
  Serial.println("Calibrating...");
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

void checkBLEInput() {
  while (bleuart.available()) {
    String command = bleuart.readStringUntil('\n');
    command.trim();
    if (command.startsWith("T:")) turnThreshold = command.substring(2).toInt();
    if (command.startsWith("B:")) brakeThreshold = command.substring(2).toInt();
    if (command.startsWith("P:")) bounceThreshold = command.substring(2).toInt();
    if (command.startsWith("D:")) {
      decayFactor = command.substring(2).toFloat();
      decayFactor = constrain(decayFactor, 0.80, 0.995);
    }
  }
}

uint8_t gradientValue(float force, int threshold, int maxVal = 255) {
  float norm = min(abs(force) / float(threshold), 1.0);
  return uint8_t(norm * maxVal);
}

void loop() {
  checkBLEInput();

  float x = CircuitPlayground.motionX() - calX;
  float y = CircuitPlayground.motionY() - calY;
  float z = CircuitPlayground.motionZ() - calZ;

  turningForce = y * 100;
  bouncingForce = x * 100;
  brakeAccelerateForce = z * 100;

  // Update decays
  turnDecay = abs(turningForce) > abs(turnDecay) ? turningForce : turnDecay * decayFactor;
  brakeDecay = abs(brakeAccelerateForce) > abs(brakeDecay) ? brakeAccelerateForce : brakeDecay * decayFactor;
  bounceDecay = abs(bouncingForce) > abs(bounceDecay) ? bouncingForce : bounceDecay * decayFactor;

  // BLE Output
  if (bleuart.isConnected()) {
    bleuart.print("Turn: "); bleuart.print(turningForce);
    bleuart.print(", Brake: "); bleuart.print(brakeAccelerateForce);
    bleuart.print(", Bounce: "); bleuart.print(bouncingForce);
    bleuart.print(" | Decay -> T: "); bleuart.print(turnDecay);
    bleuart.print(" B: "); bleuart.print(brakeDecay);
    bleuart.print(" P: "); bleuart.print(bounceDecay);
    bleuart.print(" | D: "); bleuart.println(decayFactor, 3);
  }

  // Clear and update LEDs
  CircuitPlayground.clearPixels();

  // Bounce LED (center bottom = LED 2)
  if (abs(bounceDecay) > bounceThreshold) {
    uint8_t bVal = gradientValue(bounceDecay, bounceThreshold);
    CircuitPlayground.setPixelColor(2, 0, 0, bVal);
  }

  // Brake / Accelerate LEDs (bottom: 0,1,3,4)
  if (abs(brakeDecay) > brakeThreshold) {
    uint8_t gVal = gradientValue(brakeDecay, brakeThreshold);
    uint8_t rVal = gradientValue(-brakeDecay, brakeThreshold);
    for (int i : {0, 1, 3, 4}) {
      CircuitPlayground.setPixelColor(i, rVal, gVal, 0);
    }
  }

  // Turning LEDs (top: left 5,6,7; right 7,8,9)
  if (abs(turnDecay) > turnThreshold) {
    uint8_t val = gradientValue(turnDecay, turnThreshold);
    if (turnDecay < 0) {
      // Left Turn
      CircuitPlayground.setPixelColor(5, val / 3, 0, 0);
      CircuitPlayground.setPixelColor(6, val / 2, 0, 0);
      CircuitPlayground.setPixelColor(7, val, 0, 0);
    } else {
      // Right Turn
      CircuitPlayground.setPixelColor(7, val, 0, 0);
      CircuitPlayground.setPixelColor(8, val / 2, 0, 0);
      CircuitPlayground.setPixelColor(9, val / 3, 0, 0);
    }
  }

  delay(100);
}
