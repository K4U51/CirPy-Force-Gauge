# CirPy-Force-Gauge
G-Force gauge project, improved upon the demands of the track and touge.

🛠️ What You’ll Need
    Adafruit Circuit Playground (Classic or Express)
    USB cable for programming
    Computer with Arduino IDE installed
    Double-sided tape or Velcro (for mounting)
        Optional: Enclosure or case for a more polished look
🔧 Step-by-Step Instructions
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

# Source
//https://www.instructables.com/Quick-20-G-force-Car-Gauge/

You will need a recent version of the Arduino IDE (1.6+) along with the Circuit Playground libraries, drivers and board definitions from Adafruit. Check on the Adafruit site for the most up-to-date instructions. (I’m using an early release “Developer Edition” of the board, so there is a constant stream of improvements.)

![image](https://github.com/user-attachments/assets/a915b4f7-371d-4ef7-8c82-be4e379bccdc)
![image](https://github.com/user-attachments/assets/d1acf736-ab26-4c8d-ab22-2e3cf09ecf3c)
![image](https://github.com/user-attachments/assets/b6fd6691-0879-4928-b041-5409f12cfd18)

The Arduino code is fairly simple. It's really just three series of if-then-else statements --- with one series for turning forces, one for braking and acceleration, and another for bouncing forces. The only tricky part is that we have to set an upper and lower limit for each test. For that we use the Boolean And symbol "&&" to create a range of values to test for.

    if (turningForce > -100 && turningForce < 100) {
        CircuitPlayground.setPixelColor(7, CircuitPlayground.colorWheel(170)); 
    } else if (....
Each series of if-thens tests to see if one of the accelerometer's axis reads within a certain value range. Then the code sets LEDs to indicate what range has been read.

Also, we must test the accelerometer for both the positive range and negative range. This allows us to determine whether the car is turning to the left or the right.

There are better ways to do the tasks (a for-loop that takes pairs of range values from an array), but I try to keep the code as simple as possible so absolute beginners can understand it.

There is also code that displays all three axes in the serial plotter of your Arduino IDE. Open the serial plotter from the Tools menu in the Arduino IDE. Move, tilt and twist the Circuit Playground board to see the results in the graph.


#Axis and Orientation

It’s called a triple-axis (or 3-axis) accelerometer because it senses acceleration when moved (1) forwards and backwards, (2) from left to right and (3) up or down. It can sense movements on all three axes simultaneously.

The axes are labeled X, Y and Z on the Circuit Playground board and the software library uses these same labels when you ask for a sensor reading with a request like:

      CircuitPlayground.motionZ(); 
The Cartesian coordinate system traditionally maps these X,Y,Z designations in a specific way and the Circuit Playground board adheres to this standard. The X and Y axes describe the flat plane of the board and the Z axis is the vertical or normal to the board’s face. This seems logical when the board is flat on a table.

But when you flip the board onto its edge, (like we do in this project), things can get a little confusing.

When flipped onto its side, the sensor’s Z axis is now measuring a horizontal forward/backward movement not the previous, more traditional up/down movement. The board and software do not care how the board is oriented. The axes stay the same as far as the board is concerned. That’s because the labels for X,Y,Z are “locally relative” to the sensor itself, but are fixed not relative to the world.

Think of it as your head and feet. Typically, your head will be higher than your feet. But if you hang upside down, your feet are now higher than your head. Yet we call them by the same names no matter what their relative position or orientation is to each other.

#How Does It Work

This type of accelerometer is basically a weight attached to one end of a spring. The other end of the spring is attached to a fixed surface. The spring flexes whenever the spring/weight is moved. The quicker it is moved, the more the spring is flexed. The amount of flexure is translated into voltage and sent to the Arduino. When at rest the voltage is near zero, when it flexes in one direction the voltage is positive, when flexed in the opposite direction the voltage is negative. Combine three of these spring/weight into a single sensor to measure all three axes.

Although it’s called an accelerometer, the sensor also measures the force of gravity. The weight is pulled down by the “acceleration of gravity.” So at least one axis will be affected by this downward force.

This means the sensor is also affected by the how many degrees the board is tilted. So it affected by whether you are headed up or down a hill. For a more accurate reading you will need additional sensors to create an "IMU." The amazing amandaghassaei and XenonJohn have great info on IMUs and how to use them.

The sensor is based on the LIS3DH and Adafruit has a separate board with just this sensor. You could use this board with your existing Arduino and get similar results. Adafruit’s tutorial explains a lot about the concepts behind the accelerometer, plus shows you how to do some fancy tricks like change the range of G-forces it measures, how to detect free-fall, taps and double taps.

IMPORTANT: Make sure your gauge is level when the car is at rest and on level ground. If you must mount the gauge at an angle you will need to adjust the default values of the affected axis.

Now, all you have to do is make a simple call to set individual pixels to specific colors - here, setting pixel #1 to blue (170 on the colorwheel):

    “CircuitPlayground.setPixelColor(1,CircuitPlayground.colorWheel(170));” 

Downloads
https://learn.adafruit.com/adafruit-circuit-playground-bluefruit/circuit-playground-bluefruit-circuitpython-libraries
