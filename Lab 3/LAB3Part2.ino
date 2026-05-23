// File Name: Lab3Part2.ino
// Author: Arjun Manu, Andrew Lin
// Date: 7/22/2025
// Description: This Lab is a mockup of a priority based scheduler. Once an entire sequence of tasks has run,
//              Their priority is changed and the tasks are execuded in the order of their new priority.

// Version 1.0 no changes

// ********************* Libraries *********************//
#include "driver/gpio.h"            // For GPIO driver-level macros
#include "soc/io_mux_reg.h"         // For configuring the MUX (multiplexer)
#include "soc/gpio_reg.h"           // For direct GPIO register access
#include "soc/gpio_periph.h"        // For GPIO pin definitions
#include "soc/timer_group_reg.h"    // For Timing purposes
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

LiquidCrystal_I2C lcd(0x27, 16, 2);

// ********************* Constants *********************//
#define LED_PIN1 4
#define LED_PIN2 1
#define BUZZER_PIN 6
#define SDA_PIN 14
#define SCL_PIN 13

#define PWM_RES 10

#define N_MAX_TASKS 10
#define STATE_RUNNING 0
#define STATE_READY 1
#define STATE_WAITING 2
#define STATE_INACTIVE 3

#define TIMER_INCREMENT_MODE (1<<30)
#define TIMER_ENABLE (1<<31)
#define TIMER_DIVIDER_VALUE 80
#define ALL_TOGGLE_INTERVAL 1000000

// ********************* Global Variables *********************//
int current_index = 0; // to keep track of curent taks 
uint8_t temp; // for LCD display purposes 
volatile uint8_t number_counter = 0; // ticks of the counting task
int blink_counter = 0; // ticks of blinking task
uint8_t buzzer_counter = 0; // ticks of buzzer task
uint8_t alphabet_index = 0; // ticks of alphabet task


uint32_t last_toggle_time = 0;  // clock track of blinking task
uint32_t last_toggle_time_counter = 0; // clock track of the counting task
uint32_t last_toggle_time_buzzer = 0; // clock track of buzzer task
uint32_t last_toggle_time_letter = 0;  // clock track of alphabet task

int tracker[4] = {0, 0, 0, 0}; // array to keep track of executed tasks

void blink();
void counter();
void buzzer();
void alphabet();

// new function pionters
typedef void (*funcptr)();
funcptr blink_ptr = blink;
funcptr counter_ptr = counter;
funcptr buzzer_ptr = buzzer;
funcptr alphabet_ptr = alphabet; 

//struct for Task Control Block 
typedef struct TCBstruct{
  void (*fptr) (void);
  unsigned short int state;
  unsigned int delay;
  int priority;
  String toPrint;
} TCBstruct;

TCBstruct TaskList[N_MAX_TASKS];

void setup() {
  Serial.begin(115200);
  PIN_FUNC_SELECT(GPIO_PIN_MUX_REG[LED_PIN1],PIN_FUNC_GPIO);
  *((volatile uint32_t*) GPIO_ENABLE_REG) |= (1<<LED_PIN1);

  timer_setup();
  schedulerSetup();

  ledcAttach(BUZZER_PIN, 0, PWM_RES);
  ledcAttach(LED_PIN2, 0, PWM_RES);

  Wire.begin(SDA_PIN, SCL_PIN);
  lcd.init();
  delay(10);

}

void loop() {
   check();
   delay(10);

   scheduler();
   delay(10);
  *((volatile uint32_t *)TIMG_T0UPDATE_REG(0)) = 1;
}

// name: scheduler()
// description: Method helps in scheduling the tasks.
//.             It sets the higest priority task that is inactive to ready. and once it is ready the task is run in the next loop.
void scheduler(){
    if(TaskList[current_index].state == STATE_INACTIVE){
      start_task(current_index);
    }else if(TaskList[current_index].state == STATE_READY){
      executeTask(TaskList[current_index].fptr);
    }
    delay(2);
}


// name: check()
// description: This method helps in checking if all the tasks have been run yet and if it is time to change their piorities.
//.             If all the tasks have been run once in a cycle it sets their run status to not run and changes their priorities. 
void check() {
  bool checkFlag = true;
  for (int i = 0; i < 4; i++) {
    if (tracker[i] == 0) {
      checkFlag = false;
    }
  }
  if (checkFlag) {
    for (int i = 0; i < 4; i++) {
      tracker[i] = 0;
      TaskList[i].priority = TaskList[i].priority % 4 + 1;
    }
     current_index = getPriority();
  }
}

// name: getpriority()
// description: this method helps in getting the next taks with the highest priority that is supposed to run
// return: returns an int value that represents the index of the next highest priority task to be run in the TaskList[N_MAX_TASKS]
int getPriority(){
  int currind = 1000;
  int currprio = 1000;
  for(int i = 0; TaskList[i].fptr != NULL; i++){
    if(TaskList[i].priority < currprio && tracker[i] == 0){
      currprio = TaskList[i].priority;
      currind = i;
    }
  }
 return currind;
}

// name: schedulerSetup()
// description: this method helps setup up and initialize the TaskList[N_MAX_TASKS] array to setup the tasks and their states 
void schedulerSetup(){
  int j = 0;
  TaskList[j].fptr = blink;
  TaskList[j].state = STATE_INACTIVE;
  TaskList[j].delay = 0;
  TaskList[j].priority = 1;
  TaskList[j].toPrint = "Task Blink:";
  j++;

  TaskList[j].fptr = counter;
  TaskList[j].state = STATE_INACTIVE;
  TaskList[j].delay = 0;
  TaskList[j].priority = 2;
  TaskList[j].toPrint = "Task Counter: ";
  j++;

  TaskList[j].fptr = buzzer;
  TaskList[j].state = STATE_INACTIVE;
  TaskList[j].delay = 0;
  TaskList[j].priority = 3;
  TaskList[j].toPrint = "Task Buzzer: ";
  j++;

  TaskList[j].fptr = alphabet;
  TaskList[j].state = STATE_INACTIVE;
  TaskList[j].delay = 0;
  TaskList[j].priority = 4;
  TaskList[j].toPrint = "Task Alphabets: ";
  j++;

  TaskList[j].fptr = NULL;
}

// name: blink()
// description: This method helps blink an LED 8 time a 1 second intervals. Once the whole task is executed it Changes
//              its state to inactive changes the curent_index global variable to the next task that has to run. It also changes 
//              tick counter global varible to 0.
void blink() {
  uint32_t currentTime = 0;
  currentTime = *((volatile uint32_t *)TIMG_T0LO_REG(0));


  if(blink_counter >= 16){
    blink_counter = 0;
    Serial.print(TaskList[current_index].toPrint);
    Serial.println(TaskList[current_index].priority);
    halt_me();
    current_index = getPriority();
  }

  if((currentTime-last_toggle_time >= ALL_TOGGLE_INTERVAL)){
    uint32_t gpio_out = 0;
    gpio_out = *((volatile uint32_t *)GPIO_OUT_REG);
    toggleLED(LED_PIN1);
    last_toggle_time = currentTime;
    blink_counter++;
  }
}

// name: counter()
// description: This method helps display a count on a LCD disply from 1 to 10. Once the whole task is executed it Changes
//              its state to inactive changes the curent_index global variable to the next task that has to run. It also changes 
//.             Its tick counter to 0. This method takes the help of display_command() display_data() methos to display data onto the LCD
void counter() {
  uint32_t currentTime = 0;
  currentTime = *((volatile uint32_t *)TIMG_T0LO_REG(0));

  if(number_counter > 8 && (currentTime - last_toggle_time_counter >= ALL_TOGGLE_INTERVAL)){
    last_toggle_time_counter = currentTime;
    display_command(0x01);
    display_command(0x80);
    String input4 = "Count:";
    for (int i = 0; i < input4.length(); i++) {
        char x4 = input4.charAt(i);
        display_data(x4); 
    }
    String input2 = "10";
    for (int i = 0; i < input2.length(); i++) {
        char xy = input2.charAt(i);
        display_data(xy); 
    }
    Serial.print(TaskList[current_index].toPrint);
    Serial.println(TaskList[current_index].priority);
    number_counter = 0;
    halt_me();
    current_index = getPriority();
  }
  if((currentTime - last_toggle_time_counter >= ALL_TOGGLE_INTERVAL)){
      display_command(0x01);
      display_command(0x80);
      String input = "Count:";
      for (int i = 0; i < input.length(); i++) {
        char x = input.charAt(i);
        display_data(x); 
      }
      last_toggle_time_counter = currentTime;
      display_data('1' + number_counter);
      number_counter++;
  }
}

// name: alphabet()
// description: This method helps display the Alphabets  A - Z on a LCD display. Once the whole task is executed it Changes
//              its state to inactive changes the curent_index global variable to the next task that has to run. It also changes 
//.             Its tick counter to 0.
//              This method takes the help of display_command() display_data() methos to display data onto the LCD
void alphabet() {
    uint32_t currentTime = 0;
    currentTime = *((volatile uint32_t *)TIMG_T0LO_REG(0));

  if(alphabet_index > 25){
    Serial.println("");
    Serial.print(TaskList[current_index].toPrint);
    Serial.println(TaskList[current_index].priority);
    alphabet_index =0;
    halt_me();
    current_index = getPriority();
  }
  if((currentTime - last_toggle_time_letter >= ALL_TOGGLE_INTERVAL)){
    last_toggle_time_letter = currentTime;
    display_command(0x01);
    display_command(0x80);
    char c = 'A' + alphabet_index;
    Serial.print(c);
    Serial.print(", ");
    display_data(c);
    alphabet_index++;
  }
}


// Name: buzzer()
// Description: Task to play 10 notes on a buzzer that also has a LED to reflect each note
//              Once the whole task is executed it Changes its state to inactive changes the curent_index global variable to the next 
//              task that has to run. It also changes its tick counter to 0.
void buzzer() {
  uint32_t currentTime = 0;
  currentTime = *((volatile uint32_t *)TIMG_T0LO_REG(0));
  uint32_t notes[10] = {100, 250, 400, 550, 670, 880, 1000, 1200, 1400, 2000};
  int duty = 512;

  if(buzzer_counter > 9){
    Serial.print(TaskList[current_index].toPrint);
    Serial.println(TaskList[current_index].priority);
    //ledcChangeFrequency(BUZZER_PIN,0, PWM_RES);
    //ledcChangeFrequency(LED_PIN2, 0, PWM_RES);
    ledcWrite(BUZZER_PIN, 0);
    ledcWrite(LED_PIN2, 0);
    buzzer_counter = 0 ;
    halt_me();
    current_index = getPriority();
  }
 
  if((currentTime - last_toggle_time_buzzer >= ALL_TOGGLE_INTERVAL)){
      last_toggle_time_buzzer = currentTime;
      ledcDetach(BUZZER_PIN);
      ledcDetach(LED_PIN2);
      //ledcChangeFrequency(BUZZER_PIN, notes[buzzer_counter], PWM_RES);
      ledcAttach(BUZZER_PIN, notes[buzzer_counter], PWM_RES);
      ledcWrite(BUZZER_PIN, duty);
      //ledcChangeFrequency(LED_PIN2, notes[buzzer_counter], PWM_RES);
      ledcAttach(LED_PIN2, notes[buzzer_counter], PWM_RES);
      ledcWrite(LED_PIN2, notes[buzzer_counter]/4 );
      buzzer_counter++;
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
// Description: Sends a command to an LCD module via I2C
void display_command(uint8_t c){
  Wire.beginTransmission(0x27); // Start communication at address 0x27
  temp = (c & 0b11110000) | 0b00001100; // Get high nibble
  Wire.write(temp); // Send high nibble to LCD
  Wire.write(0b00001000);
  temp = ((c & 0b00001111) << 4) | 0b00001100; // Get low nibble
  Wire.write(temp); // Send low nibble to LCD
  Wire.write(0b00001000);
  Wire.endTransmission(); // End communication
  delay(10);
}

// name: toggleLED()
// description: function to toggle LED using Register access
void toggleLED(int pin){
  *((volatile uint32_t*) GPIO_OUT_REG) ^= (1<<pin);
}

// name: timer_setup()
// description: function to set up internal ESp timer using Register access
void timer_setup(){
  uint32_t timer_config = (TIMER_DIVIDER_VALUE << 13);

  timer_config |= TIMER_ENABLE;
  timer_config |= TIMER_INCREMENT_MODE;

  *((volatile uint32_t *)TIMG_T0CONFIG_REG(0)) |= timer_config;

  *((volatile uint32_t *)TIMG_T0UPDATE_REG(0)) = 1;
}

// function to change 
void start_task(int task_id){
  TaskList[task_id].state = STATE_READY;
}

// function to pause current task
void halt_me(){
  TaskList[current_index].state = STATE_INACTIVE;
  tracker[current_index] += 1;
}

// function to execute task
void executeTask(funcptr functionPTR){
  functionPTR();
}

