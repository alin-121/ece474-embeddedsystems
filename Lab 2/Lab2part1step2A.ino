// File Name: Lab2Part2A.ino (direct register access)
// Author: Arjun Manu, Andrew Lin
// Date: 7/9/2025
// Description: This file holds the code that tests the runtime/ time it takes for a LED to blihnk using 
//              register access
//.             For this experiment tho we dont have to see the physical LED blink just measure how fast 
//.             we can change from high to low 

// Version 1.0 no changes


// ================ Libraries ==================
#include "driver/gpio.h"
#include "soc/io_mux_reg.h"
#include "soc/gpio_reg.h"
#include "soc/gpio_periph.h"

// ================ Global Variables ==================
#define GPIO_PIN 21

void setup() {
  Serial.begin(115200);// setting Serial monitor baud rate to see the output

  // Setting GPIO_PIN function to GPIO using MUX macro
  PIN_FUNC_SELECT(GPIO_PIN_MUX_REG[GPIO_PIN],PIN_FUNC_GPIO);

  // Enabling GPIO_PIN as output
  volatile uint32_t * enable = (volatile uint32_t*) GPIO_ENABLE_REG;
  *enable ^= (1<<GPIO_PIN);

}

void loop() {

  // creating a new pointer to point to the same location as the pointer pointing to the 
  // GPIO_OUT_REG register
  volatile uint32_t * reg = (volatile uint32_t*) GPIO_OUT_REG;
  

  unsigned long begin = micros();// tracking time at start
  for (int i = 0; i < 1000; i++) {
      *reg ^= (1<<GPIO_PIN);// these 2 lines blink the LED 1000 times using a library function
      *reg ^= (1<<GPIO_PIN);
  }
  unsigned long stop = micros();// tracking time at end of loop


  Serial.print("i took this much time to run: ");
  Serial.println(stop-begin);// performing a start time - end time to find total time taken


  delay(1000);
}
