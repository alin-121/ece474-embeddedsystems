// File Name: Lab4Part1
// Authors: Arjun Manu, Andrew Lin
// Date: 8/9/25
// Description: This sketch implements a Shortest Remaining Time First (SRTF) system to 
//              schedule our tasks. All tasks' remaining times are reset after they have completed
//              and when then next task runs. This way, blink always preempts a task, and counting always
//              preempts the alphabet for every letter printed.

#include <Arduino.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define LED_PIN 6

LiquidCrystal_I2C lcd(0x27, 16, 2);

// Total time budgets
const TickType_t ledTotalTime = 500 / portTICK_PERIOD_MS;        // 500ms
const TickType_t counterTotalTime = 2000 / portTICK_PERIOD_MS;   // 2 sec
const TickType_t alphabetTotalTime = 13000 / portTICK_PERIOD_MS; // 13 sec

// Remaining times
volatile TickType_t remainingLedTime = ledTotalTime;
volatile TickType_t remainingCounterTime = counterTotalTime;
volatile TickType_t remainingAlphabetTime = alphabetTotalTime;

// Task handles
TaskHandle_t TaskBlinkHandle = NULL;
TaskHandle_t TaskCountHandle = NULL;
TaskHandle_t TaskAlphabetHandle = NULL;
TaskHandle_t SchedulerHandle = NULL;

// Internal state
int counter = 1;
char currentLetter = 'A';

// Name: ledTask
// Description: Blinks the LED and removes 500 ms from it's remaining time.
//              Then suspends itself for SRTF selection.
//              This task resets the other two tasks' remaining time if
//              they don't have any remaining time left. 
void ledTask(void *arg) {
  while (1) {
    vTaskDelay(10 / portTICK_PERIOD_MS);
    if (remainingCounterTime == 0) {
      remainingLedTime = 2000 / portTICK_PERIOD_MS;
    }
    if (remainingAlphabetTime == 0) {
      remainingAlphabetTime = 13000 / portTICK_PERIOD_MS;
    }

    digitalWrite(LED_PIN, HIGH);
    vTaskDelay(250 / portTICK_PERIOD_MS);
    digitalWrite(LED_PIN, LOW);
    vTaskDelay(250 / portTICK_PERIOD_MS);
    //Serial.println("blink");

    remainingLedTime -= 500 / portTICK_PERIOD_MS;
    
    vTaskSuspend(NULL); // run once, suspend
  }
}

// Name: counterTask
// Description: Prints a number counter onto the LCD and removes
//              1000 ms from its remaining time. Goes up to 20 and
//              then resets back down to 0. Then suspends itself for
//              SRTF selection.
//              This task resets the other two tasks' remaining time if
//              they don't have any remaining time left. 
void counterTask(void *arg) {
  static int count = 1;

  while (1) {
    if (remainingLedTime == 0) {
      remainingLedTime = ledTotalTime;
    }
    if (remainingAlphabetTime == 0) {
      remainingAlphabetTime = 13000 / portTICK_PERIOD_MS;
    }

    if (remainingCounterTime > 0) {
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Count: ");
      lcd.print(count);
      count++;
      //Serial.println(count);
      
      if(count > 20){
          count = 1;
      }

      vTaskDelay(1000 / portTICK_PERIOD_MS);
      remainingCounterTime -= 1000 / portTICK_PERIOD_MS;
    }

    vTaskSuspend(NULL);
  }
}

// Name: alphabetTask
// Description: Prints the alphabet to the serial monitor and removes 1000 ms. 
//              from its remaining time. Goes up from A-Z and then back down to A.
//              Then suspends itself for SRTF selection.
//              This task resets the other two tasks' remaining time if
//              they don't have any remaining time left. 
void alphabetTask(void *arg) {
  static char letter = 'A';

  while (1) {
    if (remainingLedTime == 0) {
      remainingLedTime = 500 / portTICK_PERIOD_MS;
    }
    if (remainingCounterTime == 0) {
      remainingCounterTime = 2000 / portTICK_PERIOD_MS;
    }

    if (remainingAlphabetTime > 0) {
      Serial.print(letter++);
      Serial.print(" ");
      if (letter > 'Z') letter = 'A';

      vTaskDelay(1000 / portTICK_PERIOD_MS);
      remainingAlphabetTime -= 1000 / portTICK_PERIOD_MS;
    }

    vTaskSuspend(NULL);
  }
}

// Name: scheduleTasks
// Description: This task is the main function that handles the logic for SRTF selection.
//              It checks the condition of each remaining time and then resumes it. 
void scheduleTasks(void *arg) {
  while (1) {
    // Pick the task with the shortest remaining time
    if (remainingLedTime > 0 &&
        (remainingLedTime <= remainingCounterTime || remainingCounterTime == 0) &&
        (remainingLedTime <= remainingAlphabetTime || remainingAlphabetTime == 0)) {
      vTaskResume(TaskBlinkHandle);
    } else if (remainingCounterTime > 0 &&
               (remainingCounterTime <= remainingAlphabetTime || remainingAlphabetTime == 0)) {
      vTaskResume(TaskCountHandle);

    } else if (remainingAlphabetTime > 0) {
      vTaskResume(TaskAlphabetHandle);
    }
    if(remainingAlphabetTime == 0){
      remainingAlphabetTime = alphabetTotalTime;
    }

    vTaskDelay(10 / portTICK_PERIOD_MS); // small delay between picks
  }
}

// Name: setup
// Description: Sets up the pins, the LCD, creates the tasks and pins them to their
//              respective cores.
void setup() {
  Serial.begin(115200);
  Wire.begin(); // adjust pins for your board
  lcd.init();
  lcd.backlight();
  pinMode(LED_PIN, OUTPUT);

  xTaskCreatePinnedToCore(ledTask, "LED", 2048, NULL, 1, &TaskBlinkHandle, 0);
  xTaskCreatePinnedToCore(counterTask, "Counter", 2048, NULL, 1, &TaskCountHandle, 0);
  xTaskCreatePinnedToCore(alphabetTask, "Alphabet", 2048, NULL, 1, &TaskAlphabetHandle, 0);
  xTaskCreatePinnedToCore(scheduleTasks, "Scheduler", 4096, NULL, 2, &SchedulerHandle, 0);
}

void loop() {}
