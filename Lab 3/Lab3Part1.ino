// File Name: Lab3Part1.ino
// Author: Arjun Manu, Andrew Lin
// Date: 7/22/2025
// Description: This Lab holds the code for the progam in which you can type in a group of charagters into the Serial monitor input and 
//              the input given will be printed out onto the LCD display 

// Version 1.0 no changes

#include <Wire.h>
#include <LiquidCrystal_I2C.h>
LiquidCrystal_I2C lcd(0x27, 16, 2); // Initialize the LCD

#define LCD_ADDR 0x27 
#define LCD_FIRST 0x80
#define LCD_CLEAR 0x01

String input = "";
uint8_t temp;
int SDA_pin = 14;
int SCL_pin = 13; 

void setup() {
 Serial.begin(115200);
 Wire.begin(SDA_pin, SCL_pin);
 lcd.init();
 delay(2);
}


void loop() {
 // Writing the code that takes Serial input and displays it to the LCD
 if(Serial.available()){
  display_command(0x01);
  display_command(0x80);
  if (Serial.available()) {
    // Read the full string from Serial (waits for timeout or newline)
    String input = Serial.readStringUntil('\n');

    // Loop through each character in the string and display it
    for (int i = 0; i < input.length(); i++) {
      char c = input.charAt(i);
      display_data(c);
    }
  }
 }
}


// Name: display_data
// Description: Sends a command to a LCD module via I2C to write the characters onto the LCD
void display_data(char c){
  Wire.beginTransmission(0x27); // Start communication at address 0x27
  temp = (c & 0b11110000) | 0b00001101; // Get high nibble
  Wire.write(temp); // Send high nibble to LCD
  Wire.write(0b00001001);
  temp = ((c & 0b00001111) << 4) | 0b00001101; // Get low nibble
  Wire.write(temp); // Send low nibble to LCD
  Wire.write(0b00001001);
  Wire.endTransmission(); // End communication
  delay(10);
}


// Name: display_command
// Description: Sends a command to a LCD module via I2C to set up the display
void display_command(uint8_t c){
  Wire.beginTransmission(0x27); // Start communication at address 0x27
  temp = (c & 0b11110000) | 0b00001100; // Get high nibble
  Wire.write(temp); // Send high nibble to LCD
  Wire.write(0b00001000); // latch
  temp = ((c & 0b00001111) << 4) | 0b00001100; // Get low nibble
  Wire.write(temp); // Send low nibble to LCD
  Wire.write(0b00001000);
  Wire.endTransmission(); // End communication
  delay(10);
}