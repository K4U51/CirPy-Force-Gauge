//Initializes BLE UART
//Calibrates on boot
//Sends live G-force over BLE
//Visualizes G-force on NeoPixels with color and brightness
//Tracks peak G-force

#include <Adafruit_CircuitPlayground.h>
#include <Adafruit_LIS3DH.h>
#include <Adafruit_Sensor.h>
#include <bluefruit.h>

// BLE UART object
BLEUart bleuart;

float x_offset = 0, y_offset = 0, z_offset = 0;
float peakForce = 0;
unsigned long peakTime = 0;
bool calibrating = true;

const int calibrationSamples = 100;
float brightness = 1.0;

void calibrate() {
  float x_sum = 0, y_sum = 0, z_sum = 0;
  for (int i = 0; i < calibrationSamples; i++) {
    sensors_event_t event;
    CircuitPlayground.motionSensor.getEvent(&event);
    x_sum += event.acceleration.x;
    y_sum += event.acceleration.y;
    z_sum += event.acceleration.z;
    delay(10);
  }
  x_offset = x_sum / calibrationSamples;
  y_offset = y_sum / calibrationSamples;
  z_offset = z_sum / calibrationSamples;
  calibrating = false;
  Serial.println("Calibration complete.");
}

void setup() {
  Serial.begin(115200);
  while (!Serial);

  CircuitPlayground.begin();

  // BLE Init
  Bluefruit.begin();
  Bluefruit.setTxPower(4);  // max power
  Bluefruit.setName("GForceGauge");

  bleuart.begin();

  Bluefruit.Advertising.addFlags(BLE_GAP_ADV_FLAGS_LE_ONLY_GENERAL_DISC_MODE);
  Bluefruit.Advertising.addTxPower();
  Bluefruit.Advertising.addService(bleuart);
  Bluefruit.Advertising.start();

  calibrate();
}

void loop() {
  sensors_event_t event;
  CircuitPlayground.motionSensor.getEvent(&event);

  float x = event.acceleration.x - x_offset;
  float y = event.acceleration.y - y_offset;
  float z = event.acceleration.z - z_offset;

  float force = sqrt(x * x + y * y + z * z);

  // BLE Send
  if (bleuart.connected()) {
    bleuart.print("G: ");
    bleuart.println(force, 2);
  }

  // Peak detection
  if (force > peakForce) {
    peakForce = force;
    peakTime = millis();
  }

  // Visual indicator with color gradient
  float gNormalized = constrain((force - 1.0) / 4.0, 0.0, 1.0);  // normalize 1G-5G
  int hue = map(gNormalized * 100, 0, 100, 85, 0);  // Green to Red
  int brightnessValue = map(gNormalized * 100, 0, 100, 10, 255);

  uint32_t color = CircuitPlayground.strip.ColorHSV(hue * 182, 255, brightnessValue);

  for (int i = 0; i < 10; i++) {
    CircuitPlayground.setPixelColor(i, color);
  }

  delay(50);
}
