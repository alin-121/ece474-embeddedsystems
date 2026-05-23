// File Name: Lab2Part4.ino
// Author: Arjun Manu, Andrew Lin
// Date: 7/9/2025
// Description: This Lab holds the code the play a buzzer sequence when the light fall below a certai treshold. 
//.             When the light becomes less brighter the photoresistor "detects it" and the buzzer plays a 3 sound sequence once
//.             Each sound having a frequency higher then the one before. The whole pattern is played even if the light returns 
//.             while the pattern is being played. 
//.             We use an FSM based logic to go through the sound sequence 


// Version 1.0 no changes


// ================ LIBRARIES  ==================
#include "driver/gpio.h"
#include "soc/io_mux_reg.h"
#include "soc/gpio_reg.h"
#include "soc/gpio_periph.h"
#include "soc/timer_group_reg.h"

// ================ GLOBAL VARIABES  ==================
#define PHOTO_PIN 1
#define BUZZER_PIN 20
#define PWM_RES 10
#define THRESHOLD 1700

#define SEGMENT_DURATION_TICKS 1000000 
#define TIMER_DIVIDER_VALUE 80
#define TIMER_INCREMENT_MODE (1 << 30)
#define TIMER_ENABLE (1 << 31)

int buzzerState = 0; // logic to go though an FSM based LOGIC
uint32_t stateStartTime = 0;
bool triggered = false;



void setup() {
  ledcAttach(BUZZER_PIN, 500, PWM_RES);
 
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
  uint32_t current_time = *((volatile uint32_t *)TIMG_T0LO_REG(0)); // tracking the current time from the register that keeps track of time 

  int lightLevel = analogRead(PHOTO_PIN);
  int duty = 512; // duty cycle set to 50% thoughout 

  // This condition checks if the threshold has been broken 
  // if light level falls below the threshold it starts the sequence of sounds using an FSM logic model
  if (!triggered && lightLevel < THRESHOLD) {
    triggered = true;
    buzzerState = 1;
    stateStartTime = current_time;
    ledcChangeFrequency(BUZZER_PIN, 500, PWM_RES);
    ledcWrite(BUZZER_PIN, duty);
  }

  // Transition from the first state of 500Hz to 1000Hz once the time peiod is covered 
  else if (buzzerState == 1 && (current_time - stateStartTime >= SEGMENT_DURATION_TICKS)) {
    buzzerState++;
    stateStartTime = current_time;
    ledcChangeFrequency(BUZZER_PIN, 1000, PWM_RES);
    ledcWrite(BUZZER_PIN, duty);
  }

  // Transition from the second state of 100Hz to 2000Hz once the time peiod is covered 
  else if (buzzerState == 2 && (current_time - stateStartTime >= SEGMENT_DURATION_TICKS)) {
    buzzerState++;
    stateStartTime = current_time;
    ledcChangeFrequency(BUZZER_PIN, 2000, PWM_RES);
    ledcWrite(BUZZER_PIN, duty);
  }

  // stoping the sound being played once the time peiod is covered 
  else if (buzzerState == 3 && (current_time - stateStartTime >= SEGMENT_DURATION_TICKS)) {
    buzzerState++;
    ledcWrite(BUZZER_PIN, 0);
  }

  // Resetting the FSM to the initial state after the 3rd state has been reached 
  else if (buzzerState == 4 && lightLevel >= THRESHOLD) {
    buzzerState = 0;
    triggered = false;
  }

  // resetting the timer register
  *((volatile uint32_t*)TIMG_T0UPDATE_REG(0)) = 1;
}