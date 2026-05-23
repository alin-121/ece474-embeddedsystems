// File Name: Lab2Part2.ino
// Author: Arjun Manu, Andrew Lin
// Date: 7/9/2025
// Description: This file holds the code that makes an LED Blink and a particlat interval of time 
//              This is acheived by using a clock which is configured usign the direct the direct register access 
//.             And the output from the GPIO PIN is also done by Accessig the GPIO register and modifying it. 


// Version 1.0 no changes


// ================ Libraries  ==================

#include "driver/gpio.h"
#include "soc/io_mux_reg.h"
#include "soc/gpio_reg.h"
#include "soc/gpio_periph.h"
#include "soc/timer_group_reg.h"

// Defining GPIO pin number
#define GPIO_PIN 5

// Defining toggle interval in timer ticks (e.g., 1 second)
#define LED_TOGGLE_INTERVAL 1000000

// Defining TIMER_DIVIDER_VALUE, TIMER_INCREMENT_MODE and TIMER_ENABLE macros
#define TIMER_INCREMENT_MODE (1<<30)
#define TIMER_ENABLE (1<<31)
#define TIMER_DIVIDER_VALUE 80

void setup() {
  Serial.begin(115200);

  // Setting GPIO_PIN function to GPIO using MUX macro
  PIN_FUNC_SELECT(GPIO_PIN_MUX_REG[GPIO_PIN],PIN_FUNC_GPIO);

  // Enabling GPIO_PIN as output
  *((volatile uint32_t*) GPIO_ENABLE_REG) ^= (1<<GPIO_PIN);

  // Configuring timer
  // Applying a clock divider
  uint32_t timer_config = (TIMER_DIVIDER_VALUE << 13);

  // Setting increment mode and enable timer
  timer_config |= TIMER_ENABLE;
  timer_config |= TIMER_INCREMENT_MODE;

  // Writing config to timer register
   *((volatile uint32_t *)TIMG_T0CONFIG_REG(0)) |= timer_config;

  // Triggering a timer update to load settings
   *((volatile uint32_t *)TIMG_T0UPDATE_REG(0)) = 1;
}

void loop() {
  // Track last toggle time
  static uint32_t last_toggle_time = 0;

  // Reading current timer value
  uint32_t current_time = 0;
  current_time = *((volatile uint32_t *)TIMG_T0LO_REG(0));

  // Checking if toggle interval has passed
  if ((current_time - last_toggle_time) >= LED_TOGGLE_INTERVAL) {
     //Reading current GPIO output state
     uint32_t gpio_out = 0;
     gpio_out = *((volatile uint32_t *)GPIO_OUT_REG);

    // Toggling GPIO_PIN using XOR
     *((volatile uint32_t *)GPIO_OUT_REG) = gpio_out ^  (1<<GPIO_PIN) ;

    // Updating last_toggle_time
    last_toggle_time = current_time;
  }

  // Refreshing timer counter value
  *((volatile uint32_t *)TIMG_T0UPDATE_REG(0)) = 1;
}


