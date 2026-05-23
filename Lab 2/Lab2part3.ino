// File Name: Lab2Part3.ino
// Author: Arjun Manu, Andrew Lin
// Date: 7/9/2025
// Description: This file holds the code that takes input from a photo resistor and Brightens a led based on the amount of light 
//.             the photoresistor recieves. In this case, lesser the light, the greater the brightness of the LED


// Version 1.0 no changes


// ================ GLOBAL VARIABLES  ==================

const int LED_PIN = 19;
const int PHOTO_RES_PIN = 1;

void setup() {
  ledcAttach(LED_PIN, 500, 11); //setting frequency of the LED to 500hz and the resolution to 11 bits
}

void loop() {
  int x = analogRead(PHOTO_RES_PIN); // light recieved by the resistor
  int y = 2047 - x; // the duty cycle is written based on the light recieved by the Photoresistor, 
  ledcWrite(LED_PIN, y); 
}
