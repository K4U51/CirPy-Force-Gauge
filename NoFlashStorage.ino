#include <bluefruit.h>
#include <Adafruit_CircuitPlayground.h>

// Calibration
float calX = 0, calY = 0, calZ = 0;

// G-force data
int turningForce = 0, bouncingForce = 0, brakeAccelerateForce = 0;
float turnDecay = 0, bounceDecay = 0, brakeDecay = 0;
float decayFactor = 0.95;

// BLE thresholds
int turnThreshold = 150, brakeThreshold = 100, bounceThreshold = 200;

// Auto-tuning
const int autoWindow = 200;
const int learnPercentile = 90;
int turnSamples[autoWindow], brakeSamples[autoWindow], bounceSamples[autoWindow];
int sampleIndex = 0;
bool learningMode = false;

// Min/max tracking (RAM only now)
int maxTurn = 0, minTurn = 0, maxBrake = 0, minBrake = 0, maxBounce = 0, minBounce = 0;

BLEUart bleuart;

enum Orientation { FLAT, TILT90 };
Orientation currentOrientation = FLAT;

// --- Calibration ---
void calibrate() {
  Serial.println("Calibrating...");
  CircuitPlayground.setPixelColor(0, 0, 255, 0);
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
  CircuitPlayground.clearPixels();
  Serial.println("Calibration done.");
}

// --- EEPROM Helpers (REMOVED) ---
// loadStats() and saveStats() removed because flash storage unavailable

// --- BLE Command Parser ---
void parseBLECommand(String &cmd) {
  cmd.trim();
  if (cmd == "RESET") {
    minTurn = maxTurn = minBrake = maxBrake = minBounce = maxBounce = 0;
    Serial.println("Stats RESET.");
  }
  else if (cmd == "LEARN:ON") { learningMode = true; Serial.println("Learning ON"); }
  else if (cmd == "LEARN:OFF") { learningMode = false; Serial.println("Learning OFF"); }
  else if (cmd == "CAL") calibrate();
  else if (cmd.startsWith("T:")) turnThreshold = cmd.substring(2).toInt();
  else if (cmd.startsWith("B:")) brakeThreshold = cmd.substring(2).toInt();
  else if (cmd.startsWith("P:")) bounceThreshold = cmd.substring(2).toInt();
  else if (cmd.startsWith("D:")) decayFactor = constrain(cmd.substring(2).toFloat(), 0.80, 0.995);
  else if (cmd.startsWith("ORIENT:")) {
    int o = cmd.substring(7).toInt();
    currentOrientation = (o == 90 ? TILT90 : FLAT);
  }
}

void checkBLEInput() {
  static String buffer = "";
  while (bleuart.available()) {
    char c = bleuart.read();
    if (c == '\n') {
      parseBLECommand(buffer);
      buffer = "";
    } else {
      buffer += c;
    }
  }
}

// --- Gradient Utility ---
uint8_t gradientValue(float force, int thr, int mv = 255) {
  return uint8_t(min(abs(force) / float(thr), 1.0) * mv);
}

// --- Sample Tracking ---
void storeSample(int idx, int arr[], int val) {
  arr[idx % autoWindow] = abs(val);
}

int percentileValue(int arr[]) {
  int tmp[autoWindow];
  memcpy(tmp, arr, sizeof(tmp));
  sort(tmp, tmp + autoWindow);
  return tmp[(autoWindow * learnPercentile) / 100];
}

// --- Setup ---
void setup() {
  Serial.begin(9600);
  CircuitPlayground.begin();
  Bluefruit.begin();
  bleuart.begin();
  Bluefruit.setName("G-Force Sensor");
  Bluefruit.Advertising.addFlags(BLE_GAP_ADV_FLAGS_LE_ONLY_GENERAL_DISC_MODE);
  Bluefruit.Advertising.addTxPower();
  Bluefruit.Advertising.addName();
  Bluefruit.Advertising.addService(bleuart);
  Bluefruit.Advertising.restartOnDisconnect(true);
  Bluefruit.Advertising.setInterval(32, 244);
  Bluefruit.Advertising.setFastTimeout(30);
  Bluefruit.Advertising.start(0);

  calibrate();

  // No flash loadStats()
}

// --- Main Loop ---
void loop() {
  checkBLEInput();

  // Read acceleration
  float x = CircuitPlayground.motionX() - calX;
  float y = CircuitPlayground.motionY() - calY;
  float z = CircuitPlayground.motionZ() - calZ;

  // Orientation mapping
  turningForce = (currentOrientation == FLAT ? y : x) * 100;
  bouncingForce = (currentOrientation == FLAT ? x : z) * 100;
  brakeAccelerateForce = (currentOrientation == FLAT ? z : y) * 100;

  // Decay smoothing
  turnDecay = decayFactor * turnDecay + (1 - decayFactor) * turningForce;
  brakeDecay = decayFactor * brakeDecay + (1 - decayFactor) * brakeAccelerateForce;
  bounceDecay = decayFactor * bounceDecay + (1 - decayFactor) * bouncingForce;

  // Update min/max (RAM only)
  minTurn = min(minTurn, turningForce);
  maxTurn = max(maxTurn, turningForce);
  minBrake = min(minBrake, brakeAccelerateForce);
  maxBrake = max(maxBrake, brakeAccelerateForce);
  minBounce = min(minBounce, bouncingForce);
  maxBounce = max(maxBounce, bouncingForce);

  // No flash saveStats()

  // Auto-threshold learning
  if (learningMode) {
    storeSample(sampleIndex, turnSamples, turningForce);
    storeSample(sampleIndex, brakeSamples, brakeAccelerateForce);
    storeSample(sampleIndex, bounceSamples, bouncingForce);
    if (++sampleIndex >= autoWindow) {
      turnThreshold = percentileValue(turnSamples);
      brakeThreshold = percentileValue(brakeSamples);
      bounceThreshold = percentileValue(bounceSamples);
      sampleIndex = 0;
      Serial.println("Thresholds auto-tuned.");
    }
  }

  // BLE UART output for graph
  if (bleuart.isConnected()) {
    bleuart.print("GDATA,");
    bleuart.print(turningForce); bleuart.print(",");
    bleuart.print(brakeAccelerateForce); bleuart.print(",");
    bleuart.print(bouncingForce); bleuart.print(",");
    bleuart.print(turnThreshold); bleuart.print(",");
    bleuart.print(brakeThreshold); bleuart.print(",");
    bleuart.print(bounceThreshold);
    bleuart.println();
  }

  // NeoPixel Feedback
  CircuitPlayground.clearPixels();
  if (abs(bounceDecay) > bounceThreshold)
    CircuitPlayground.setPixelColor(2, 0, 0, gradientValue(bounceDecay, bounceThreshold));
  if (abs(brakeDecay) > brakeThreshold) {
    uint8_t g = gradientValue(brakeDecay, brakeThreshold);
    uint8_t r = gradientValue(-brakeDecay, brakeThreshold);
    for (int i : {0, 1, 3, 4}) CircuitPlayground.setPixelColor(i, r, g, 0);
  }
  if (abs(turnDecay) > turnThreshold) {
    uint8_t v = gradientValue(turnDecay, turnThreshold);
    if (turnDecay < 0) {
      CircuitPlayground.setPixelColor(5, v / 3, 0, 0);
      CircuitPlayground.setPixelColor(6, v / 2, 0, 0);
      CircuitPlayground.setPixelColor(7, v, 0, 0);
    } else {
      CircuitPlayground.setPixelColor(7, v, 0, 0);
      CircuitPlayground.setPixelColor(8, v / 2, 0, 0);
      CircuitPlayground.setPixelColor(9, v / 3, 0, 0);
    }
  }

  delay(100);
}
