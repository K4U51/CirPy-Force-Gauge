# CirPy-Force-Gauge
G-Force gauge project, improved upon the demands of the track and touge.

# 🛠️ What You’ll Need
  - Adafruit Circuit Playground (Classic or Express)
  - USB cable for programming
  -  Computer with Arduino IDE installed
  -  Double-sided tape or Velcro (for mounting)
        Optional: Enclosure or case for a more polished look
        
# 🔧 Step-by-Step Instructions

    Step 1: Understand the Concept
        The project uses the built-in accelerometer on the Circuit Playground to detect G-forces.
        10 RGB LEDs on the board visually represent:
        Turning (left/right)
        Acceleration/Braking
        Vertical bumps or road roughness
        
    Step 2: Set Up the Arduino IDE
        Install the Arduino IDE from arduino.cc.
        Add support for the Adafruit Circuit Playground board:
        Go to File > Preferences, and add this URL to the “Additional Board Manager URLs”:
                https://adafruit.github.io/arduino-board-index/package_adafruit_index.json
        Then go to Tools > Board > Board Manager, search for “Adafruit AVR Boards” and install it.
        
    Step 3: Install Required Libraries
        In the Arduino IDE, go to Sketch > Include Library > Manage Libraries.
        Install:
        Adafruit Circuit Playground
        Adafruit Sensor
        Adafruit NeoPixel
        
Step 4: Upload the Code
        Use the example code provided in the Instructables post or from Adafruit’s Circuit Playground examples.
        Connect the board via USB, select the correct board and port under Tools, and upload the sketch.

# Source & Proof of Concept - //https://www.instructables.com/Quick-20-G-force-Car-Gauge/
![image](https://github.com/user-attachments/assets/a915b4f7-371d-4ef7-8c82-be4e379bccdc)
![image](https://github.com/user-attachments/assets/d1acf736-ab26-4c8d-ab22-2e3cf09ecf3c)
![image](https://github.com/user-attachments/assets/b6fd6691-0879-4928-b041-5409f12cfd18)

The Arduino code is fairly simple. It's really just three series of if-then-else statements --- with one series for turning forces, one for braking and acceleration, and another for bouncing forces. The only tricky part is that we have to set an upper and lower limit for each test. For that we use the Boolean And symbol "&&" to create a range of values to test for.

    if (turningForce > -100 && turningForce < 100) {
        CircuitPlayground.setPixelColor(7, CircuitPlayground.colorWheel(170)); 
    } else if (....
Each series of if-thens tests to see if one of the accelerometer's axis reads within a certain value range. Then the code sets LEDs to indicate what range has been read.

Also, we must test the accelerometer for both the positive range and negative range. This allows us to determine whether the car is turning to the left or the right.

    Note: There are better ways to do the tasks (a for-loop that takes pairs of range values from an array), but I try to keep the code as simple as possible so absolute beginners can understand it.

    Note: There is also code that displays all three axes in the serial plotter of your Arduino IDE. Open the serial plotter from the Tools menu in the Arduino IDE. Move, tilt and twist the Circuit Playground board to see the results in the graph.


# Axis and Orientation
The axes are labeled X, Y and Z on the Circuit Playground board and the software library uses these same labels when you ask for a sensor reading with a request like:

      CircuitPlayground.motionZ(); 
The Cartesian coordinate system traditionally maps these X,Y,Z designations in a specific way and the Circuit Playground board adheres to this standard. The X and Y axes describe the flat plane of the board and the Z axis is the vertical or normal to the board’s face. This seems logical when the board is flat on a table.

🧭 Understanding the 3-Axis Accelerometer
      The accelerometer detects motion in three directions:
      
          X-axis: Left ↔ Right
          Y-axis: Forward ↔ Backward
          Z-axis: Up ↕ Down
            These axes are fixed relative to the board, not the world.
            
          So:
              If the board is flat on a table, Z is vertical.
              If the board is on its side (like in this project), Z might now point forward/backward.
            
🧠 Local vs. Global Orientation

      The board always reports motion based on its own orientation, not the room or car.
      Think of it like your head and feet: even if you’re upside down, your head is still your head.

# How Does It Work
🌀 How an Accelerometer Works (Simplified)
    Basic Principle: It’s like a weight on a spring. When the device moves, the spring flexes.
       
        Voltage Output:
        At rest → voltage ≈ 0
        Move one way → positive voltage
        Move the other way → negative voltage
        3D Sensing: Three of these spring-weight systems measure motion in X, Y, and Z axes.
        
🌍 Gravity & Tilt Effects
       
        The sensor also detects gravity, since gravity pulls on the weight.
        This means it’s sensitive to tilt (e.g., going uphill or downhill).
        For more accurate motion tracking, you’d need an IMU (Inertial Measurement Unit), which combines multiple sensors.
        
🔧 Practical Notes
      
        The sensor used is the LIS3DH, also available as a standalone board from Adafruit.
        Adafruit’s tutorials explain how to:
        Adjust G-force sensitivity
        Detect free-fall
        Recognize taps and double taps
        
⚠️ Important Setup Tip
       
        Mount the gauge level when the car is on flat ground.
        If mounted at an angle, you’ll need to calibrate the sensor by adjusting the default axis values.

Now, all you have to do is make a simple call to set individual pixels to specific colors - here, setting pixel #1 to blue (170 on the colorwheel):

    “CircuitPlayground.setPixelColor(1,CircuitPlayground.colorWheel(170));” 

# Codex

    #include <Adafruit_CircuitPlayground.h>
    #include <bluefruit.h>
    
    BLEUart bleuart; // BLE UART object
    
    // Calibration and filtering
    float offsetX = 0, offsetY = 0, offsetZ = 0;
    float smoothX = 0, smoothY = 0, smoothZ = 0;
    
    // Impact detection
    unsigned long impactHoldTime = 1000;
    unsigned long lastImpactTime = 0;
    bool impactDetected = false;
    
    // BLE OTA Config
    int gThresholdSlider = 25; // Default from slider (0–100 scale)
    float mappedGThreshold = 2.0; // Mapped threshold from slider to 0–4G
    
    // Battery reading pin
    #define VBATPIN A6
    
    void setup() {
      Serial.begin(115200);
      CircuitPlayground.begin();
      delay(1000);
    
      calibrateAccelerometer();
    
      // BLE setup
      Bluefruit.begin();
      Bluefruit.setTxPower(4);
      Bluefruit.setName("GForceGauge");
    
      bleuart.begin();
      Bluefruit.Periph.setConnectCallback(connect_callback);
      Bluefruit.Periph.setDisconnectCallback(disconnect_callback);
    
      // OTA control via Bluefruit app sliders
      Bluefruit.Central.setConnectCallback(central_connect_callback);
    
      Bluefruit.Advertising.addFlags(BLE_GAP_ADV_FLAGS_LE_ONLY_GENERAL_DISC_MODE);
      Bluefruit.Advertising.addTxPower();
      Bluefruit.Advertising.addService(bleuart);
      Bluefruit.Advertising.start();
    }
    
    void loop() {
      // Read + filter G-force
      float rawX = CircuitPlayground.motionX() - offsetX;
      float rawY = CircuitPlayground.motionY() - offsetY;
      float rawZ = CircuitPlayground.motionZ() - offsetZ;
    
      smoothX = 0.8 * smoothX + 0.2 * rawX;
      smoothY = 0.8 * smoothY + 0.2 * rawY;
      smoothZ = 0.8 * smoothZ + 0.2 * rawZ;
    
      float totalG = sqrt(smoothX * smoothX + smoothY * smoothY + smoothZ * smoothZ);
    
      CircuitPlayground.clearPixels();
    
      // Map BLE slider to dynamic G-threshold
      mappedGThreshold = map(gThresholdSlider, 0, 100, 10, 40) / 10.0; // 1.0–4.0G
    
      // Turning logic (X)
      renderForceLED(smoothX, 3, 1, 2, 3, 7, 8, 9); // center LED, right 3, left 3
    
      // Accel/Braking logic (Y)
      renderForceLED(smoothY, 4, 4, 4, 4, 5, 5, 5); // green front accel / red rear brake
    
      // Z-axis bump detection
      if (abs(smoothZ) > mappedGThreshold) {
        lastImpactTime = millis();
        impactDetected = true;
      }
    
      if (impactDetected && millis() - lastImpactTime < impactHoldTime) {
        for (int i = 0; i < 10; i++) {
          CircuitPlayground.setPixelColor(i, 128, 0, 128); // Purple bump flash
        }
      } else {
        impactDetected = false;
      }
    
      // BLE stream
      if (Bluefruit.connected()) {
        char packet[128];
        float battery = analogRead(VBATPIN) * 2 * 3.3 / 1023.0;
        snprintf(packet, sizeof(packet), "X: %.2f, Y: %.2f, Z: %.2f, Bat: %.2fV\n", smoothX, smoothY, smoothZ, battery);
        bleuart.print(packet);
      }
    
      delay(100);
    }
    
    void calibrateAccelerometer() {
      float sumX = 0, sumY = 0, sumZ = 0;
      for (int i = 0; i < 25; i++) {
        sumX += CircuitPlayground.motionX();
        sumY += CircuitPlayground.motionY();
        sumZ += CircuitPlayground.motionZ();
        delay(40);
      }
      offsetX = sumX / 25;
      offsetY = sumY / 25;
      offsetZ = sumZ / 25;
    }
    
    void renderForceLED(float force, int centerLED, int softLED, int medLED, int hardLED, int softNegLED, int medNegLED, int hardNegLED) {
      float absForce = abs(force);
    
      if (force > 0) {
        int hue = map(absForce * 10, 0, 40, 16000, 65535); // green to red
        uint32_t color = CircuitPlayground.strip.ColorHSV(hue);
        if (absForce > mappedGThreshold * 0.75) {
          CircuitPlayground.setPixelColor(hardLED, color);
        } else if (absForce > mappedGThreshold * 0.5) {
          CircuitPlayground.setPixelColor(medLED, color);
        } else if (absForce > mappedGThreshold * 0.25) {
          CircuitPlayground.setPixelColor(softLED, color);
        } else {
          CircuitPlayground.setPixelColor(centerLED, color);
        }
      } else if (force < 0) {
        int hue = map(absForce * 10, 0, 40, 16000, 65535);
        uint32_t color = CircuitPlayground.strip.ColorHSV(hue);
        if (absForce > mappedGThreshold * 0.75) {
          CircuitPlayground.setPixelColor(hardNegLED, color);
        } else if (absForce > mappedGThreshold * 0.5) {
          CircuitPlayground.setPixelColor(medNegLED, color);
        } else if (absForce > mappedGThreshold * 0.25) {
          CircuitPlayground.setPixelColor(softNegLED, color);
        } else {
          CircuitPlayground.setPixelColor(centerLED, color);
        }
      }
    }
    
    // BLE callbacks
    void connect_callback(uint16_t conn_handle) {
      Serial.println("BLE connected");
    }
    
    void disconnect_callback(uint16_t conn_handle, uint8_t reason) {
      Serial.println("BLE disconnected");
    }
    
    void central_connect_callback(uint16_t conn_handle) {
      Serial.println("Central connected");
    
      // Example: update threshold from Bluefruit Connect app slider
      bleuart.println("Send a value 0–100 to set sensitivity");
    }
    
    // Optional: allow OTA config from slider
    void serialEvent() {
      if (bleuart.available()) {
        String input = bleuart.readStringUntil('\n');
        int val = input.toInt();
        if (val >= 0 && val <= 100) {
          gThresholdSlider = val;
        }
      }
    }
    