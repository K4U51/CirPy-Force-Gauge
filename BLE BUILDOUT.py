#include <Adafruit_CircuitPlayground.h>
#include <bluefruit.h>

BLEUart bleuart;

float offsetX = 0, offsetY = 0, offsetZ = 0;
float smoothing = 0.1;
float totalG = 0;

void setup() {
  Serial.begin(115200);
  CircuitPlayground.begin();
  Bluefruit.begin();
  Bluefruit.setTxPower(4);
  Bluefruit.setName("CPB Force Gauge");
  bleuart.begin();
  Bluefruit.Advertising.addFlags(BLE_GAP_ADV_FLAGS_LE_ONLY_GENERAL_DISC_MODE);
  Bluefruit.Advertising.addTxPower();
  Bluefruit.Advertising.addService(bleuart);
  Bluefruit.Advertising.start();

  // Calibrate
  delay(1000);
  offsetX = CircuitPlayground.motionX();
  offsetY = CircuitPlayground.motionY();
  offsetZ = CircuitPlayground.motionZ();
}

void loop() {
  float x = CircuitPlayground.motionX() - offsetX;
  float y = CircuitPlayground.motionY() - offsetY;
  float z = CircuitPlayground.motionZ() - offsetZ;

  float turning = x;
  float bounce = y + 9.8; // offset gravity
  float accel = z;

  totalG = sqrt(x*x + y*y + z*z);

  sendBLE(turning, bounce, accel, totalG);
  visualizeTurning(turning);
  visualizeAccelBraking(accel);
  visualizeBounce(bounce);

  delay(50);
}

void sendBLE(float x, float y, float z, float t) {
  String data = String(x, 2) + "," + String(y, 2) + "," + String(z, 2) + "," + String(t, 2);
  bleuart.print(data + "\n");
}

void visualizeTurning(float turning) {
  int strength = min(abs(turning * 2), 3); // max 3 LEDs
  uint32_t baseColor = CircuitPlayground.colorWheel(160); // blueish base

  // Clear turning LEDs first
  CircuitPlayground.setPixelColor(0, 0);
  CircuitPlayground.setPixelColor(1, 0);
  CircuitPlayground.setPixelColor(2, 0);
  CircuitPlayground.setPixelColor(3, 0);
  CircuitPlayground.setPixelColor(4, 0);

  if (turning > 1.0) { // Right turn: 2,1,0
    for (int i = 0; i < strength; i++) {
      int led = 2 - i;  // 2,1,0
      float scale = 1.0 - (i * 0.3);
      CircuitPlayground.setPixelColor(led, dimColor(baseColor, scale));
    }
  } else if (turning < -1.0) { // Left turn: 2,3,4
    for (int i = 0; i < strength; i++) {
      int led = 2 + i; // 2,3,4
      float scale = 1.0 - (i * 0.3);
      CircuitPlayground.setPixelColor(led, dimColor(baseColor, scale));
    }
  }
}

void visualizeAccelBraking(float accel) {
  uint32_t accelColor = CircuitPlayground.colorWheel(85);  // Red
  uint32_t brakeColor = CircuitPlayground.colorWheel(0);   // Green

  // Clear LEDs 5,6,8,9
  CircuitPlayground.setPixelColor(5, 0);
  CircuitPlayground.setPixelColor(6, 0);
  CircuitPlayground.setPixelColor(8, 0);
  CircuitPlayground.setPixelColor(9, 0);

  if (accel > 1.0) { // Acceleration
    CircuitPlayground.setPixelColor(5, accelColor);
    CircuitPlayground.setPixelColor(6, accelColor);
  } else if (accel < -1.0) { // Braking
    CircuitPlayground.setPixelColor(8, brakeColor);
    CircuitPlayground.setPixelColor(9, brakeColor);
  }
}

void visualizeBounce(float bounce) {
  // LED 7 for bounce
  if (abs(bounce) > 3.0) {
    CircuitPlayground.setPixelColor(7, 255, 255, 0); // Yellow
  } else {
    CircuitPlayground.setPixelColor(7, 0);
  }
}

// Helper to dim a color by a scale factor (0.0 to 1.0)
uint32_t dimColor(uint32_t color, float scale) {
  uint8_t r = (uint8_t)(((color >> 16) & 0xFF) * scale);
  uint8_t g = (uint8_t)(((color >> 8) & 0xFF) * scale);
  uint8_t b = (uint8_t)((color & 0xFF) * scale);
  return CircuitPlayground.strip.Color(r, g, b);
}
