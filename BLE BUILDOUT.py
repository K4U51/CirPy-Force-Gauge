//Initializes BLE UART
//Calibrates on boot
//Sends live G-force over BLE
//Visualizes G-force on NeoPixels with color and brightness
//Tracks peak G-force

#include <bluefruit.h>
#include <Adafruit_CircuitPlayground_Bluefruit.h>  // Correct library for CPB Bluefruit
#include <Adafruit_Sensor.h>  // Needed for sensors_event_t

BLEUart bleuart;

float x_offset = 0, y_offset = 0, z_offset = 0;
float peakForce = 0;
unsigned long peakTime = 0;

const int calibrationSamples = 100;

void calibrate() {
  float x_sum = 0, y_sum = 0, z_sum = 0;
  Serial.println("Calibrating accelerometer, keep device still...");
  for (int i = 0; i < calibrationSamples; i++) {
    sensors_event_t event;
    CircuitPlayground.motionSensor.getEvent(&event);  // Read accelerometer event
    x_sum += event.acceleration.x;
    y_sum += event.acceleration.y;
    z_sum += event.acceleration.z;
    delay(10);
  }
  x_offset = x_sum / calibrationSamples;
  y_offset = y_sum / calibrationSamples;
  z_offset = z_sum / calibrationSamples;

  Serial.print("Calibration complete: ");
  Serial.print(x_offset, 4); Serial.print(", ");
  Serial.print(y_offset, 4); Serial.print(", ");
  Serial.println(z_offset, 4);
}

void setup() {
  Serial.begin(115200);
  while (!Serial) delay(10);

  CircuitPlayground.begin();

  Bluefruit.begin();
  Bluefruit.setTxPower(4);
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

  float fx = event.acceleration.x - x_offset;
  float fy = event.acceleration.y - y_offset;
  float fz = event.acceleration.z - z_offset;

  float force = sqrt(fx * fx + fy * fy + fz * fz);

  if (Bluefruit.connected()) {
    bleuart.print("G: ");
    bleuart.println(force, 2);
  }

  // Normalize force roughly between 1G and 5G (9.80665 m/s² per G)
  float gNormalized = constrain((force - 9.80665) / (9.80665 * 4), 0.0, 1.0);
  int hue = map(gNormalized * 100, 0, 100, 85, 0);  // Green to red
  int brightnessValue = map(gNormalized * 100, 0, 100, 10, 255);

  uint32_t color = CircuitPlayground.strip.ColorHSV(hue * 182, 255, brightnessValue);

  for (int i = 0; i < CircuitPlayground.strip.numPixels(); i++) {
    Circu
