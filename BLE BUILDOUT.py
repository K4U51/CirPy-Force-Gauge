#include <Adafruit_CircuitPlayground.h>
#include <Adafruit_BluefruitLE_SPI.h>
#include <BluefruitConfig.h>
#include <Adafruit_BLE.h>
#include <Adafruit_BLE_UART.h>

#define FACTORYRESET_ENABLE 0

Adafruit_BluefruitLE_SPI ble(BLUEFRUIT_SPI_CS, BLUEFRUIT_SPI_IRQ, BLUEFRUIT_SPI_RST);
Adafruit_BLE_UART uart(ble);

// Raw accelerometer readings
float x, y, z;

// Calibration offsets
float offsetX = 0, offsetY = 0, offsetZ = 0;

// Persistent state variables
float accelHold = 0;
float brakeHold = 0;
float bounceLevel = 0;
float leftRightIntensity = 0;

// Constants
const float HOLD_DECAY = 0.95;
const float BOUNCE_DECAY = 0.90;
const int ACCEL_THRESHOLD = 150;
const int BRAKE_THRESHOLD = -150;
const int BOUNCE_THRESHOLD = 250;

// Telemetry
unsigned long lastTelemetry = 0;
const unsigned long TELEMETRY_INTERVAL = 150; // ms

void setup() {
  CircuitPlayground.begin();
  CircuitPlayground.setBrightness(80);
  Serial.begin(115200);
  delay(1000);

  Serial.println(F("\n=== ForceGauge Visualizer with Auto-Calibration ==="));
  Serial.println(F("Calibrating... keep the device still."));

  // --- Auto-Calibration Routine ---
  float sumX = 0, sumY = 0, sumZ = 0;
  const int samples = 100;
  for (int i = 0; i < samples; i++) {
    sumX += CircuitPlayground.motionX();
    sumY += CircuitPlayground.motionY();
    sumZ += CircuitPlayground.motionZ();
    delay(10);
  }
  offsetX = sumX / samples;
  offsetY = sumY / samples;
  offsetZ = sumZ / samples;

  Serial.print(F("Offsets: X=")); Serial.print(offsetX);
  Serial.print(F(" Y=")); Serial.print(offsetY);
  Serial.print(F(" Z=")); Serial.println(offsetZ);
  Serial.println(F("Calibration complete.\n"));

  // --- BLE Initialization ---
  if (!ble.begin(VERBOSE_MODE)) {
    while (1) {
      Serial.println(F("Couldn't find Bluefruit. Check wiring or mode."));
      delay(1000);
    }
  }

  ble.echo(false);
  if (FACTORYRESET_ENABLE) ble.factoryReset();
  ble.setMode(BLUEFRUIT_MODE_DATA);
  Serial.println(F("BLE Ready"));
}

void loop() {
  // --- Read & Calibrate Accelerometer (convert to milli-G) ---
  x = (CircuitPlayground.motionX() - offsetX) * 100;
  y = (CircuitPlayground.motionY() - offsetY) * 100;
  z = (CircuitPlayground.motionZ() - offsetZ) * 100;

  // --- Turning Intensity (Y-axis) ---
  float turnMag = fabs(y);
  leftRightIntensity = constrain(turnMag / 300.0, 0.0, 1.0);

  // Gradient: Green → Yellow → Orange → Red
  uint8_t r = map(leftRightIntensity * 100, 0, 100, 0, 255);
  uint8_t g = map(leftRightIntensity * 100, 0, 100, 255, 100);
  uint8_t b = 0;

  // Light turning LEDs (0–4)
  for (int i = 0; i < 5; i++) {
    CircuitPlayground.setPixelColor(i, r, g, b);
  }

  // --- Acceleration / Braking (X-axis) ---
  if (x > ACCEL_THRESHOLD) accelHold = 1.0;
  else if (x < BRAKE_THRESHOLD) brakeHold = 1.0;

  accelHold *= HOLD_DECAY;
  brakeHold *= HOLD_DECAY;

  // Acceleration (Green fade)
  int accelR = 0;
  int accelG = (int)(255 * accelHold);
  int accelB = 0;

  // Braking (Red fade)
  int brakeR = (int)(255 * brakeHold);
  int brakeG = 0;
  int brakeB = 0;

  for (int i = 5; i <= 6; i++)
    CircuitPlayground.setPixelColor(i, accelR, accelG, accelB);
  for (int i = 8; i <= 9; i++)
    CircuitPlayground.setPixelColor(i, brakeR, brakeG, brakeB);

  // --- Bounce (Z-axis) ---
  if (fabs(z) > BOUNCE_THRESHOLD) bounceLevel = 1.0;
  bounceLevel *= BOUNCE_DECAY;

  int purpleR = (int)(150 * bounceLevel);
  int purpleG = 0;
  int purpleB = (int)(255 * bounceLevel);
  CircuitPlayground.setPixelColor(7, purpleR, purpleG, purpleB);

  // --- BLE Telemetry ---
  unsigned long now = millis();
  if (now - lastTelemetry > TELEMETRY_INTERVAL) {
    lastTelemetry = now;

    // Determine color-coded states
    const char* turnColor = (leftRightIntensity > 0.75) ? "RED" :
                            (leftRightIntensity > 0.5) ? "ORANGE" :
                            (leftRightIntensity > 0.25) ? "YELLOW" : "GREEN";

    const char* accelState = (accelHold > 0.2) ? "ACCEL=GREEN" : "ACCEL=OFF";
    const char* brakeState = (brakeHold > 0.2) ? "BRAKE=RED" : "BRAKE=OFF";
    const char* bounceState = (bounceLevel > 0.05) ? "BOUNCE=PURPLE" : "BOUNCE=OFF";

    char buffer[128];
    snprintf(buffer, sizeof(buffer),
             "X:%5.1f  Y:%5.1f  Z:%5.1f | TURN=%s | %s | %s | %s\r\n",
             x, y, z, turnColor, accelState, brakeState, bounceState);

    ble.print(buffer);
    Serial.print(buffer);
  }

  delay(20); // ~50Hz visual update rate
}
