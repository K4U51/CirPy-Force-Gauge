#include <bluefruit.h>
#include <Adafruit_CircuitPlayground.h>

// BLE UART setup
BLEUart bleuart;

// Calibration
float offsetX = 0, offsetY = 0, offsetZ = 0;

void setup() {
  Serial.begin(115200);
  CircuitPlayground.begin();
  Bluefruit.begin();
  bleuart.begin();
  Bluefruit.setTxPower(4);
  Bluefruit.setName("ForceGauge");
  Bluefruit.Advertising.addFlags(BLE_GAP_ADV_FLAGS_LE_ONLY_GENERAL_DISC_MODE);
  Bluefruit.Advertising.addTxPower();
  Bluefruit.Advertising.addService(bleuart);
  Bluefruit.Advertising.start();

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
  // Read and calibrate accelerometer
  float x = CircuitPlayground.motionX() - offsetX;
  float y = CircuitPlayground.motionY() - offsetY + 900;  // offset gravity
  float z = CircuitPlayground.motionZ() - offsetZ;

  float total = sqrt(x * x + y * y + z * z);

  // Send over BLE UART
  bleuart.print("X: "); bleuart.print(x);
  bleuart.print(", Y: "); bleuart.print(y);
  bleuart.print(", Z: "); bleuart.print(z);
  bleuart.print(", Total: "); bleuart.println(total);

  // Serial plotter debug
  Serial.print(x); Serial.print(",");
  Serial.print(y); Serial.print(",");
  Serial.print(z); Serial.print(",");
  Serial.println(total);

  // Clear all LEDs
  for (int i = 0; i < 10; i++) CircuitPlayground.setPixelColor(i, 0);

  // --- Turning Logic (X axis) ---
  int turnForce = abs(x);
  if (turnForce > 200) {
    uint32_t color = colorWheel(map(turnForce, 200, 1500, 160, 0)); // Blue to Cyan
    CircuitPlayground.setPixelColor(2, color); // Center LED always lit during turning
    if (x > 200) {
      CircuitPlayground.setPixelColor(1, color);
      CircuitPlayground.setPixelColor(0, color);
    } else if (x < -200) {
      CircuitPlayground.setPixelColor(3, color);
      CircuitPlayground.setPixelColor(4, color);
    }
  }

  // --- Bouncing Logic (Y axis) ---
  if (y > 250) {
    uint32_t bounceColor = colorWheel(map(y, 250, 1500, 60, 0)); // Yellow to Red
    CircuitPlayground.setPixelColor(7, bounceColor);
  }

  // --- Acceleration/Braking (Z axis) ---
  if (z > 150) {
    // Braking – Green
    CircuitPlayground.setPixelColor(5, 0, 255, 0);
    CircuitPlayground.setPixelColor(6, 0, 200, 0);
    CircuitPlayground.setPixelColor(8, 0, 150, 0);
    CircuitPlayground.setPixelColor(9, 0, 100, 0);
  } else if (z < -150) {
    // Acceleration – Red
    CircuitPlayground.setPixelColor(5, 255, 0, 0);
    CircuitPlayground.setPixelColor(6, 200, 0, 0);
    CircuitPlayground.setPixelColor(8, 150, 0, 0);
    CircuitPlayground.setPixelColor(9, 100, 0, 0);
  }

  delay(50); // adjust for responsiveness
}

// Color wheel helper
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
