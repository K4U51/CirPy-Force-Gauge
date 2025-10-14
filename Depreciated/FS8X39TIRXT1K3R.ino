// This code is based on the libraries and code from the fine folks at https://www.adafruit.com/products/3000
// It is intended for use with the Circuit Playground board and implements a basic 3-axis G-Force dial that
// displays turns, braking and bouncing forces using the board's built-in accelerometer and RGB LEDs

#include <Adafruit_CircuitPlayground.h>

//Create some variables to hold the accelerometer readings
int turningForce = 0;
int bouncingForce = 0;
int brakeAccelerateForce = 0;

void setup() {
  //while (!Serial);
  Serial.begin(9600);

  CircuitPlayground.begin(); //start-up the full Circuit-Playground library
}


void loop() {
  // If you change the orientation of the board during mounting, this is where you change which axis maps to which G-force
  // (multiplied by 100 to convert to easy integer format (7.69 * 100 = 769)
  turningForce = CircuitPlayground.motionX() * 100;
  bouncingForce = (CircuitPlayground.motionY() * 100) + 900; //add back to "zero out" gravity on the vertical axis
  brakeAccelerateForce = CircuitPlayground.motionZ() * 100;

  //------------------ SET UP GRAPHING TO AVOID JITTERY PLOTS -----------------------
  //
  Serial.print(2000);                //top limit
  Serial.print(",    ");             //seperator
  Serial.print(turningForce);        //the first variable for plotting
  Serial.print(",    ");             //seperator
  Serial.print(bouncingForce);       //the second variable for plotting
  Serial.print(",    ");             //seperator
  Serial.print(brakeAccelerateForce);//the third variable for plotting including line break
  Serial.print(",    ");             //seperator
  Serial.println(-2000);             //lower limit

  // there is no "smoothing" function in this code, so add a small delay to avoid strobing the LEDs
  delay(100);

  //turn off all the pixels (LEDs) each time through the loop
  CircuitPlayground.clearPixels();



  // =================== Display the forces exerted during Left/Right turns ===========================
  //                     (Change the <> values to fine tune the sensitivity)


  if (turningForce > -100 && turningForce < 100) {                         // if the car is going straight ahead the reading should be near zero
    CircuitPlayground.setPixelColor(7, CircuitPlayground.colorWheel(170)); // so turn the top row center-line pixel blue

    // if the car is turning RIGHT---------------------------------------
  } else if (turningForce > -150 && turningForce < -100) {                 //very gentle turn
    CircuitPlayground.setPixelColor(7, CircuitPlayground.colorWheel(170)); //
    CircuitPlayground.setPixelColor(6, CircuitPlayground.colorWheel(170));
  } else if (turningForce > -200 && turningForce < -150) {                 //gentle turn
    CircuitPlayground.setPixelColor(7, CircuitPlayground.colorWheel(170)); //
    CircuitPlayground.setPixelColor(6, CircuitPlayground.colorWheel(170)); //
    CircuitPlayground.setPixelColor(5, CircuitPlayground.colorWheel(170));
  } else if (turningForce > -250 && turningForce < -200) {                 //turn
    CircuitPlayground.setPixelColor(7, CircuitPlayground.colorWheel(170)); //
    CircuitPlayground.setPixelColor(6, CircuitPlayground.colorWheel(170)); //
    CircuitPlayground.setPixelColor(5, CircuitPlayground.colorWheel(230));
  } else if (turningForce > -300 && turningForce < -250) {                 //strong turn
    CircuitPlayground.setPixelColor(7, CircuitPlayground.colorWheel(170)); //
    CircuitPlayground.setPixelColor(6, CircuitPlayground.colorWheel(200)); //
    CircuitPlayground.setPixelColor(5, CircuitPlayground.colorWheel(250));
  } else if (turningForce > -350 && turningForce < -300) {                 //Really Strong Turn
    CircuitPlayground.setPixelColor(7, CircuitPlayground.colorWheel(190)); //
    CircuitPlayground.setPixelColor(6, CircuitPlayground.colorWheel(245)); //
    CircuitPlayground.setPixelColor(5, CircuitPlayground.colorWheel(255));
  } else if (turningForce < -350) {                                        //Really Really Strong Turn
    CircuitPlayground.setPixelColor(7, CircuitPlayground.colorWheel(230)); //
    CircuitPlayground.setPixelColor(6, CircuitPlayground.colorWheel(245));
    CircuitPlayground.setPixelColor(5, CircuitPlayground.colorWheel(255));


    // if the car is turning LEFT----------------------------------------
  } else if (turningForce > 100 && turningForce < 150) {
    CircuitPlayground.setPixelColor(7, CircuitPlayground.colorWheel(170));
    CircuitPlayground.setPixelColor(8, CircuitPlayground.colorWheel(170));
  } else if (turningForce > 150 && turningForce < 200) {
    CircuitPlayground.setPixelColor(7, CircuitPlayground.colorWheel(170));
    CircuitPlayground.setPixelColor(8, CircuitPlayground.colorWheel(170));
    CircuitPlayground.setPixelColor(9, CircuitPlayground.colorWheel(170));
  } else if (turningForce > 200 && turningForce < 250) {
    CircuitPlayground.setPixelColor(7, CircuitPlayground.colorWheel(170));
    CircuitPlayground.setPixelColor(8, CircuitPlayground.colorWheel(170));
    CircuitPlayground.setPixelColor(9, CircuitPlayground.colorWheel(230));
  } else if (turningForce > 250 && turningForce < 300) {
    CircuitPlayground.setPixelColor(7, CircuitPlayground.colorWheel(170));
    CircuitPlayground.setPixelColor(8, CircuitPlayground.colorWheel(200));
    CircuitPlayground.setPixelColor(9, CircuitPlayground.colorWheel(250));
  } else if (turningForce > 300 && turningForce < 350) {
    CircuitPlayground.setPixelColor(7, CircuitPlayground.colorWheel(190));
    CircuitPlayground.setPixelColor(8, CircuitPlayground.colorWheel(245));
    CircuitPlayground.setPixelColor(9, CircuitPlayground.colorWheel(255));
  } else if (turningForce > 350) {
    CircuitPlayground.setPixelColor(7, CircuitPlayground.colorWheel(230));
    CircuitPlayground.setPixelColor(8, CircuitPlayground.colorWheel(245));
    CircuitPlayground.setPixelColor(9, CircuitPlayground.colorWheel(255));
  }

  // ==================== Display the forces exerted during Braking and Acceleration ================
  if (brakeAccelerateForce > -50 && brakeAccelerateForce < 50) {
    //    CircuitPlayground.setPixelColor(0, CircuitPlayground.colorWheel(170));
    //    CircuitPlayground.setPixelColor(4, CircuitPlayground.colorWheel(170));

  // if car is accelerating (with damping for bounces)-----------------
  } else if (brakeAccelerateForce > -100 && brakeAccelerateForce < -50 && bouncingForce < 200 && bouncingForce > -200) {
    CircuitPlayground.setPixelColor(0, CircuitPlayground.colorWheel(85));
    CircuitPlayground.setPixelColor(4, CircuitPlayground.colorWheel(85));
  } else if (brakeAccelerateForce < -100 && bouncingForce < 200 && bouncingForce > -200) {
    CircuitPlayground.setPixelColor(0, CircuitPlayground.colorWheel(85));
    CircuitPlayground.setPixelColor(1, CircuitPlayground.colorWheel(85));
    CircuitPlayground.setPixelColor(3, CircuitPlayground.colorWheel(85));
    CircuitPlayground.setPixelColor(4, CircuitPlayground.colorWheel(85));
    
  //if car is braking--------------------------------------------------
  } else if (brakeAccelerateForce > 50 && brakeAccelerateForce < 150) {
    CircuitPlayground.setPixelColor(0, CircuitPlayground.colorWheel(0));
    CircuitPlayground.setPixelColor(4, CircuitPlayground.colorWheel(0));
  } else if (brakeAccelerateForce > 150) {
    CircuitPlayground.setPixelColor(0, CircuitPlayground.colorWheel(0));
    CircuitPlayground.setPixelColor(1, CircuitPlayground.colorWheel(0));
    CircuitPlayground.setPixelColor(3, CircuitPlayground.colorWheel(0));
    CircuitPlayground.setPixelColor(4, CircuitPlayground.colorWheel(0));
  }



  // ============================ Display the forces from Bouncing and Potholes ==============================
  if (bouncingForce > -150 && bouncingForce < 150) {
    CircuitPlayground.setPixelColor(2, CircuitPlayground.colorWheel(90));

  } else if ((bouncingForce > 150 && bouncingForce < 300) || (bouncingForce > -300 && bouncingForce < -150)) {
    CircuitPlayground.setPixelColor(2, CircuitPlayground.colorWheel(70));
  } else if ((bouncingForce > 300 && bouncingForce < 400) || (bouncingForce > -400 && bouncingForce < -300)) {
    CircuitPlayground.setPixelColor(2, CircuitPlayground.colorWheel(25));
  } else if ((bouncingForce > 400 && bouncingForce < 500) || (bouncingForce > -500 && bouncingForce < -400)) {
    CircuitPlayground.setPixelColor(2, CircuitPlayground.colorWheel(20));
  } else if ((bouncingForce > 500 && bouncingForce < 600) || (bouncingForce > -600 && bouncingForce < -500)) {
    CircuitPlayground.setPixelColor(2, CircuitPlayground.colorWheel(15));
  } else if ((bouncingForce > 600 && bouncingForce < 700) || (bouncingForce > -700 && bouncingForce < -600)) {
    CircuitPlayground.setPixelColor(2, CircuitPlayground.colorWheel(10));
  } else if ((bouncingForce > 700 && bouncingForce < 800) || (bouncingForce > -800 && bouncingForce < -700)) {
    CircuitPlayground.setPixelColor(2, CircuitPlayground.colorWheel(5));
  } else if ((bouncingForce > 800 && bouncingForce < 900) || (bouncingForce > -900 && bouncingForce < -800)) {
    CircuitPlayground.setPixelColor(2, CircuitPlayground.colorWheel(3));
  } else if ((bouncingForce > 900 && bouncingForce < 1000) || (bouncingForce > -1000 && bouncingForce < -900)) {
    CircuitPlayground.setPixelColor(2, CircuitPlayground.colorWheel(0));

  } else if (bouncingForce > 1000 ||  bouncingForce < -1000) {
    CircuitPlayground.setPixelColor(1, CircuitPlayground.colorWheel(220));
    CircuitPlayground.setPixelColor(2, CircuitPlayground.colorWheel(220));
    CircuitPlayground.setPixelColor(3, CircuitPlayground.colorWheel(220));
    CircuitPlayground.setPixelColor(6, CircuitPlayground.colorWheel(220));
    CircuitPlayground.setPixelColor(7, CircuitPlayground.colorWheel(220));
    CircuitPlayground.setPixelColor(8, CircuitPlayground.colorWheel(220));
  }



}//////////////////////////////////////// END OF MAIN LOOP /////////////////////////////////////////////
