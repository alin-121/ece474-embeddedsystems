// File Name: Lab4Part2
// Authors: Arjun Manu, Andrew Lin
// Date: 8/9/25
// Description: Part II creates a scheduler with three tasks: a light detector for
//              data selection and simple moving average (sma) calculation, an LCD printer,
//              an alarm that flashes 3 times when the SMA is outside the desired range, and a
//              primer number calculator. Tasks are split onto different cores and use a binary
//              semaphore to synchronize data.
// Used ChatGPT to figure out Prime number calculator

// Libraries
#include <Arduino.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define PHOTO_PIN 5
#define LED_PIN 6

LiquidCrystal_I2C lcd(0x27, 16, 2);

// Initialize handles
SemaphoreHandle_t lightLevelSync;
TaskHandle_t TaskLightHandle;
TaskHandle_t TaskLCDHandle;
TaskHandle_t TaskAlarmHandle;
TaskHandle_t TaskPrimeHandle;

volatile int lightLevel = 0;
volatile float sma = 0;

// Name: setup
// Description: Sets up the serial monitor, the lcd, the semaphore, and the tasks (while
//              pinning them to their respective cores)
void setup(){
  Serial.begin(115200);
  Wire.begin(SDA, SCL);
  lcd.init();
  lcd.backlight();

  lightLevelSync = xSemaphoreCreateBinary();
  xSemaphoreGive(lightLevelSync);

  xTaskCreatePinnedToCore(lightDetector, "Light Detector Task", 8192, NULL, 1, &TaskLightHandle, 0);
  xTaskCreatePinnedToCore(LCD, "LCD Task", 4096, NULL, 1, &TaskLCDHandle, 0);
  xTaskCreatePinnedToCore(anomalyAlarm, "Anomaly Alarm Task", 1024, NULL, 1, &TaskAlarmHandle, 1);
  xTaskCreatePinnedToCore(primeCalculation, "Prime Calculation Task", 1024, NULL, 1, &TaskPrimeHandle, 1);
}

void loop(){}

// Name: lightDetector
// Description: This task first takes the binary semaphore and
//              reads the analog values from the photoResister pin
//              and then puts it into an array with size 5 (sma window size)
//              and finds the average of the numbers in the array. When index
//              reaches 4, reset back down to first index. 
//              This task then gives the binary semaphore.
void lightDetector(void *arg){
// ====================> TODO:
//          1. Initialize Variables
  const int windowSize = 5;
  int reading;
  int smaBuffer[windowSize] = {0};
  int smaIndex = 0;
  pinMode(PHOTO_PIN, INPUT);

  while(1){
    reading = analogRead(PHOTO_PIN);
    if(xSemaphoreTake(lightLevelSync, portMAX_DELAY)){
      smaBuffer[smaIndex] = lightLevel;
      smaIndex = (smaIndex + 1) % windowSize;

      int sum = 0;
      for(int i = 0; i < windowSize; i++){
        sum += smaBuffer[i];
      }

      sma = sum / (float)windowSize;
      lightLevel = reading;

      if(xSemaphoreGive(lightLevelSync) != pdTRUE){
        Serial.println("Could not give semaphore");
      }
    }
    vTaskDelay(pdMS_TO_TICKS(10)); 
  }
}


// Name: LCD
// Description: This task first takes the binary semaphore and
//              prints the lightlevel and SMA onto the LCD.
//              This task then gives the binary semaphore.
void LCD(void *arg){
  while(1){
    if(xSemaphoreTake(lightLevelSync, portMAX_DELAY)){
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Light: ");
      lcd.print(lightLevel);

      lcd.setCursor(0, 1);
      lcd.print("SMA: ");
      lcd.print((int)sma);
      xSemaphoreGive(lightLevelSync);
    }
    vTaskDelay(pdMS_TO_TICKS(10));
  }
}

// Name: anomalyAlarm
// Description: This task first takes a binary semaphore and
//              checks whether the SMA is outside of the desired
//              range. Then a for loop is used to blink the LED
//              three times and cools down for 2 seconds.
//              This task then gives the binary semaphore.
void anomalyAlarm(void *arg){
  pinMode(LED_PIN, OUTPUT);
  while(1){
    if(xSemaphoreTake(lightLevelSync, portMAX_DELAY)){
      if(sma < 300 || sma > 3800){
        for(int i = 0; i < 3; i++){
          digitalWrite(LED_PIN, HIGH);
          vTaskDelay(pdMS_TO_TICKS(100));
          digitalWrite(LED_PIN, LOW);
          vTaskDelay(pdMS_TO_TICKS(100));
        }
      }
      xSemaphoreGive(lightLevelSync);
    }
    vTaskDelay(pdMS_TO_TICKS(2000));
  }
}

// Name: primeCalculation
// Description: This just runs on its own and goes through 
//              a for loop from 2 to 5000 and checks whether
//              the number is a prime number and prints it to 
//              the serial monitor.
void primeCalculation(void *arg){
  while(1){
    for(int i = 2; i <= 5000; i++){
    bool prime = true;
    int j = 2;
    while(j * j <= i && prime){
      if(i % j == 0){
        prime = false;
      }
      j++;
    }

    if(prime){
      Serial.println(i);
    }
    vTaskDelay(pdMS_TO_TICKS(100));
    }
  }
}
