// File Name: Lab2Part2B.ino.  (Library access toggle)
// Author: Arjun Manu, Andrew Lin
// Date: 7/9/2025
// Description: This file holds the code that tests the runtime/ time it takes for a LED to blink using 
//              A Library Function 
//.             For this experiment tho we dont have to see the physical LED blink just measure how fast 
//.             we can change from high to low 


// Version 1.0 no changes


// ================ Global Variables ==================
const int LED1 = 21;

void setup() {
  Serial.begin(115200); // setting Serial monitor baud rate to see the output
  pinMode(LED1, OUTPUT);
}

void loop() {
  unsigned long begin = micros(); // tracking time at start
  for (int i = 0; i < 1000; i++) {
     digitalWrite(LED1, HIGH); // these 2 lines blink the LED 1000 times using a library function
     digitalWrite(LED1, LOW);
  }
  unsigned long stop = micros(); // tracking time at end of loop

  Serial.print("i took this much time to run ");
  Serial.println(stop-begin); // performing a start time - end time to find total time taken
  delay(1000);
}
