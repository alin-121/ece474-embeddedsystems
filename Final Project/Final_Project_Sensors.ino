// Names: Final_Project_Sensors
// Authors: Andrew Lin, Arjun Manu
// Date: 8/22/25
// Description: This file contains code for the sensor and the 'bomb' of our project. By using ESP-NOW,
//              we are able to communicate between two different ESPs and send motion data / receive interface
//              data.  

// Sensors MAC address fc:01:2c:db:f4:88 
// User Interface MAC address fc:01:2c:db:ed:08

// Libraries
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <esp_now.h>
#include <WiFi.h>
#include <IRremote.hpp>
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>

#define BOMB_PIN 4
#define OUTSIDE_PIN 5
#define INSIDE_PIN 6
#define IR_PIN 15

// Task Handles
TaskHandle_t sendHandle;
TaskHandle_t insideSense;
TaskHandle_t outsideSense;
TaskHandle_t detonationCord;
TaskHandle_t readingRemote;
TaskHandle_t suspendOut;

TimerHandle_t bombTimer;

uint8_t broadcastAddress[] = {0xfc, 0x01, 0x2c, 0xdb, 0xed, 0x08};
volatile int bluetoothFlag = 1;
volatile bool outsideLatch = false;
volatile bool changeStatus = false;


// Struct to receive message
typedef struct struct_receiveMessage {
  bool explode;
  int correct;
} struct_receiveMessage;

struct_receiveMessage interfaceData;


// Struct to send message
typedef struct struct_sendMessage {
  bool wakeUp;
  int sleep;
} struct_sendMessage;

struct_sendMessage sensorData;

esp_now_peer_info_t peerInfo;

// Name: OnDataRecv
// Description: This function prints the received structs from the other ESP through
//              ESP-NOW and then sends a signal to turn off the LCD on the other ESP
//              if a certain value is received.
void OnDataRecv(const esp_now_recv_info_t* mac, const uint8_t* incomingData, int len) {
  memcpy(&interfaceData, incomingData, sizeof(interfaceData));
  Serial.print("Bytes received: ");
  Serial.println(len);
  Serial.print("Bool: ");
  Serial.println(interfaceData.explode);
  Serial.print("Int: ");
  Serial.println(interfaceData.correct);
  
  if(interfaceData.correct == 1 && changeStatus == false){
    bluetoothFlag = 0;
    changeStatus = true;
  }
  Serial.println();
}


// Name: OnDataSent
// Description: Makes sure the data is sent and prints if the last packet is sent as a sanity check
void OnDataSent(const wifi_tx_info_t *mac_addr, esp_now_send_status_t status) {
  Serial.print("\r\nLast Packet Send Status:\t");
  Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Delivery Success" : "Delivery Fail");
}

// Name: sendData
// Description: This function sets values of the struct we want to send to the other ESP
//              and then sends it.
void sendData(void *pvParameters){
  while(1){
    // Set values to send
  sensorData.sleep = bluetoothFlag;
  
  Serial.print("Sending wakeup: ");
  Serial.println(sensorData.wakeUp);

  Serial.print("Sending sleep: ");
  Serial.println(sensorData.sleep);
  
  // Send message via ESP-NOW
  esp_err_t result = esp_now_send(broadcastAddress, (uint8_t *) &sensorData, sizeof(sensorData));
   
  if (result == ESP_OK) {
    Serial.println("Sent with success");
  }
  else {
    Serial.println("Error sending the data");
  }
  vTaskDelay(pdMS_TO_TICKS(2000));
  } 
}

// Name: outsideSensor
// Description: Uses the infrared sensor to detect motion and sends a signal to 
//              turn on the LCD and stay latched onto that value so the LCD doesn't
//              turned off. It then turns off when the password/RFID is correct .
void outsideSensor(void *pvParameters){
  pinMode(OUTSIDE_PIN, INPUT_PULLDOWN);
  int last = LOW;
  while(1){
    int now = digitalRead(OUTSIDE_PIN);
    if(now == HIGH && last == LOW){
      if(!outsideLatch){
        outsideLatch = true;
        Serial.println("Someone is outside the door");
        sensorData.wakeUp = true;
        }
    } else if((outsideLatch && interfaceData.correct == 1)){
      outsideLatch = false;
      sensorData.wakeUp = false;
      interfaceData.correct = 0;
    } else {
      interfaceData.correct = 0;
    last = now;
    vTaskDelay(pdMS_TO_TICKS(20));
    }
  }
}

// Name: insideSensor
// Description: this function uses an infrared sensor for the inside of our system. If 
//              the system is set on away and movement is sensed, this function will send 
//              a notification to the bomb trigger or if the other ESP sends a signal it
//              will also notify the bombtrigger function. Otherwise the sensor is disabled
//              when the password/rfid is correct.
void insideSensor(void *pvParameters){
  pinMode(INSIDE_PIN, INPUT_PULLDOWN);
  while(1){
    if(interfaceData.explode == true || bluetoothFlag == 1){
      if(digitalRead(INSIDE_PIN) == HIGH || interfaceData.explode == 1){
        Serial.println("Someone is robbing you");
        //Send signal to bomb
        xTaskNotifyGive(detonationCord);
      }
      vTaskDelay(pdMS_TO_TICKS(1000));
    } else if(interfaceData.correct == 1){
      bluetoothFlag = 0;
    }
  }
}

// Name: bombTrigger
// Description: This function receives a notification from the insideSensor
//              function and then begins a timer for 5 seconds to the bombTimerCallback
//              function.
void bombTrigger(void *pvParameters){
  pinMode(BOMB_PIN, OUTPUT);
  while(1){
    if(ulTaskNotifyTake(pdTRUE, portMAX_DELAY)){
      
      xTimerStart(bombTimer, 0);
    }
  }
}

// Name: bombTimerCallback
// Description: This function uses the freeRTOS timer to cause the 
//              bomb to explode
void bombTimerCallback(TimerHandle_t xTimer) {
  digitalWrite(BOMB_PIN, HIGH);
}

// Name: setStatus
// Description: This function is a helper function that has conditionals with the
//              decoded IR signals to tell it to set the state of the system (home/away)
void setStatus(uint32_t x){
  if(x == -1153106176){ // home
    bluetoothFlag = 0;
    changeStatus = true;
  } else if(x == -1136394496){ // away
    bluetoothFlag = 1;
    changeStatus = false;
  }
}

// Name: readRemote
// Description: This function decodes and reads the signals from the IR remote and calls on setStatus to 
//              complete do a task.
void readRemote(void *pvParameters){
  while(1){
    if(IrReceiver.decode()){
      //Serial.println(IR.decodedIRData.decodedRawData,DEC);
      uint32_t in = IrReceiver.decodedIRData.decodedRawData;
      IrReceiver.resume();
      //Serial.println((int)raw);

      setStatus(in);
    }
    vTaskDelay(pdMS_TO_TICKS(100));
  }
}


// Name: setup
// Description: Initialized WiFi for ESP-NOW, the Infrared Receiver, ESP-NOW sending and receiving data,
//              ESP-NOW peer register, task creation and a timer 
void setup() {
  // Initialize Serial Monitor
  Serial.begin(115200);
  
  // Set device as a Wi-Fi Station
  WiFi.mode(WIFI_STA);

  IrReceiver.begin(IR_PIN);

  // Init ESP-NOW
  if (esp_now_init() != ESP_OK) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }
  
  // Once ESPNow is successfully Init, we will register for recv CB to
  // get recv packer info
  esp_now_register_recv_cb(OnDataRecv);

  esp_now_register_send_cb(OnDataSent);
  
  // Register peer
  memcpy(peerInfo.peer_addr, broadcastAddress, 6);
  peerInfo.channel = 0;  
  peerInfo.encrypt = false;
  
  // Add peer        
  if (esp_now_add_peer(&peerInfo) != ESP_OK){
    Serial.println("Failed to add peer");
    return;
  }

  xTaskCreatePinnedToCore(sendData, "SendData", 4096, NULL, 1, &sendHandle, 0);
  xTaskCreatePinnedToCore(outsideSensor, "OutsideSensor", 4096, NULL, 1, &outsideSense, 0);
  xTaskCreatePinnedToCore(insideSensor, "InsideSensor", 4096, NULL, 1, &insideSense, 1);
  xTaskCreatePinnedToCore(bombTrigger, "BombTrigger", 4096, NULL, 1, &detonationCord, 1);
  xTaskCreate(readRemote, "ReadRemote", 1024, NULL, 1, &readingRemote);

  bombTimer = xTimerCreate("BombTimer", pdMS_TO_TICKS(5000), pdFALSE, (void*)0, bombTimerCallback);
}
 
void loop() {
}