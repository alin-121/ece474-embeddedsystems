// File name: Final_Project_Interface
// Authors: Andrew Lin, Arjun Manu
// Date: 8/22/25
// Description: This file controls the interface of the security system which includes
//              the Liquid Crystal Display (LCD), the RFID scanner, and the Infrared 
//              remote. By using ESP-NOW, we are able to send data to and from each ESP
//              to turn on/off the backlight and interract with the sensors/bomb

// Libraries
#include <esp_now.h>
#include <esp_wifi.h>
#include <esp_pm.h>
#include <WiFi.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <IRremote.hpp>
#include <SPI.h>
#include <MFRC522.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/timers.h"

// Pins
#define IR_PIN 3
#define RESET 2
#define SDA 10

LiquidCrystal_I2C LCD = LiquidCrystal_I2C(0x27, 16, 2);
MFRC522 myRFID(SDA,RESET);

int col = 0;
int i = 0;
int password = 2005;
int tries = 0; 
int currentPassword = 0;
static constexpr uint32_t BACKLIGHT_IDLE_MS = 5000;


int arr[4] = {0, 0, 0, 0}; 
byte myID1[4] = {0x21, 0xD4, 0x7C, 0x00};
byte myID2[4] = {0xEF, 0xF6, 0xD4, 0x1E};
byte storedUID[4];
bool true1 = true;
bool true2 = true;
bool wasHome = false;


// Andrews Mac: fc:01:2c:db:f4:88
// My Mac:      fc:01:2c:db:ed:08

esp_now_peer_info_t peerInfo;
uint8_t broadcastAddress[] = {0xFC, 0x01, 0x2C, 0xDB, 0xF4, 0x88};

// struct for security
typedef struct {
  bool flag;   
  int  value;  
} hehe;

// Struct for sending messages
typedef struct struct_message {

  bool andrew_explode;
  int disable_andrew_sensor;

} struct_message;

// Struct for receiving messages
typedef struct struct_message1 {

    bool turn_on_lcd;
    int reset_logic_start;

} struct_message1;

struct_message myData;
struct_message1 andrewData;

// Handles
QueueHandle_t Numbers;
QueueHandle_t Security;
QueueHandle_t CheckRf;
QueueHandle_t Status;
QueueHandle_t ToSend;

TaskHandle_t TaskHandle_readRemote;
TaskHandle_t TaskHandle_testNumbers;
TaskHandle_t TaskHandle_getValue;
TaskHandle_t TaskHandle_checkPassword ;
TaskHandle_t TaskHandle_checkRFID;
TaskHandle_t TaskHandle_RFID;
TaskHandle_t TaskHandle_loopShim;
TaskHandle_t TaskHandle_Sender;
TaskHandle_t TaskHandle_Reciever;
TaskHandle_t TaskHandle_restart;
TaskHandle_t Scheduler1;
TaskHandle_t Scheduler2;

TimerHandle_t backlightTimer;


// Name: setup
// Description: Initializes ESP-NOW, LCD, RFID, and the IR receiver, while also
//              pairing this ESP with another ESP. Also initializes queues, tasks, and
//              a timer.
void setup() {

  Serial.begin(115200);
 
  WiFi.mode(WIFI_STA);

  Wire.begin(8, 9);
  LCD.init();
  //LCD.backlight();
  LCD.setCursor(0, 0);
  LCD.print("Password: ");

  SPI.begin();      
  myRFID.PCD_Init();  

  IrReceiver.begin(IR_PIN);

  if (esp_now_init() != ESP_OK) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }

  esp_now_register_send_cb(OnDataSent);
  esp_now_register_recv_cb(OnDataRecv);
  
  memset(&peerInfo, 0, sizeof(peerInfo));
  memcpy(peerInfo.peer_addr, broadcastAddress, 6);
  peerInfo.channel = 0;  
  peerInfo.encrypt = false;
        
  if (esp_now_add_peer(&peerInfo) != ESP_OK){
    Serial.println("Failed to add peer");
    return;
  }

  Numbers  = xQueueCreate(16, sizeof(int));
  Security = xQueueCreate(16, sizeof(hehe));
  Status = xQueueCreate(16, sizeof(hehe));
  CheckRf  = xQueueCreate(16, sizeof(bool));
  ToSend = xQueueCreate(16, sizeof(struct_message));

  backlightTimer = xTimerCreate("bl", pdMS_TO_TICKS(BACKLIGHT_IDLE_MS), pdFALSE, NULL, backlightTimerCb);

  //xTaskCreatePinnedToCore(scheduleTasks1,   "Scheduler1", 1024, NULL, 10,    &Scheduler1, 0);
  //xTaskCreatePinnedToCore(scheduleTasks2,   "Scheduler1", 1024, NULL, 10,    &Scheduler2, 1);

  xTaskCreatePinnedToCore(readRemote,    "readRemote",    4096, NULL, 5,  &TaskHandle_readRemote,    0);
  xTaskCreatePinnedToCore(testNumbers,   "testNumbers",   4096, NULL, 1,  &TaskHandle_testNumbers,   0);
  xTaskCreatePinnedToCore(checkPassword, "checkPassword", 4096, NULL, 1,  &TaskHandle_checkPassword, 0);
  xTaskCreatePinnedToCore(Welcome,       "Welcome",       4096, NULL, 1,  &TaskHandle_restart,       0);

  xTaskCreatePinnedToCore(checkRFID,     "checkRFID",     4096, NULL, 2,  &TaskHandle_checkRFID,     1);
  xTaskCreatePinnedToCore(RFID,          "RFID",          4096, NULL, 3,  &TaskHandle_RFID,          1);

  xTaskCreatePinnedToCore(Send,    "Sender",        4096, NULL, 4,  &TaskHandle_Sender,        1);
  //xTaskCreatePinnedToCore(Recieve,    "Reciever",      4096, NULL, 5,  &TaskHandle_Reciever,      1);
}

// Name: backlightTimerCb
// Description: helper function to obtain the timer handle and
//              turn off the LCD backlight using a queue
void backlightTimerCb(TimerHandle_t){
  hehe m; m.flag = false; m.value = -1;
  xQueueSend(Status, &m, 0);
}

// Name: touchActivity
// Description: helper function that calls on a helper timer function to turn off the 
//              LCD backlight after 5 seconds whenever any activity is detected on the LCD
inline void touchActivity(){
  hehe m; m.flag = true; m.value = -1;
  xQueueSend(Status, &m, 0);
  if(backlightTimer){
    if(xTimerIsTimerActive(backlightTimer)) xTimerReset(backlightTimer, 0);
    else xTimerStart(backlightTimer, 0);
  }
}

// Name: Welcome
// Description: This function waits for a struct to be received and turns on the 
//              LCD backlight if the outside sensors form the other ESP detects motion.
//              if it does not detect motion, the LCD's backlight will turn. The LCD
//              will also restart whenever the state of the system is swapped from home
//              to away.  
void Welcome(void *pvParameters){
  hehe input;
  while(1){
   if (xQueueReceive(Status, &input, portMAX_DELAY) == pdTRUE) {
     if(input.flag == 1){
       LCD.backlight();
     }else{
      LCD.noBacklight();
     }

     if(input.value == 0 ){
      // suspend tasks 
      // reset global variables 
      vTaskSuspend(TaskHandle_readRemote);
      vTaskSuspend(TaskHandle_testNumbers);
      vTaskSuspend(TaskHandle_checkPassword);
      vTaskSuspend(TaskHandle_checkRFID);
      vTaskSuspend(TaskHandle_RFID);
      vTaskSuspend(TaskHandle_Sender);
      wasHome = true;
     }else if(input.value == 1 && wasHome){     
      wasHome = !wasHome;
      col = 0;
      i = 0;
      tries = 0; 
      for(int i = 0; i < 4 ; i++){
          arr[i] = 0;
      }
      currentPassword = 0;
      true1 = true;
      true2 = true;
      LCD.clear();
      LCD.setCursor(0, 0);
      LCD.print("Password: ");
      vTaskResume(TaskHandle_readRemote);
      vTaskResume(TaskHandle_testNumbers);
      vTaskResume(TaskHandle_checkPassword);
      vTaskResume(TaskHandle_checkRFID);
      vTaskResume(TaskHandle_RFID);  
      vTaskResume(TaskHandle_Sender);
     }
   }
  }
}

// Name: Send
// Description: Sends struct data to the communicating ESP 
void Send(void *pvParameters){
  struct_message sending;
  while(1){
    vTaskDelay(pdMS_TO_TICKS(500));
     if (xQueueReceive(ToSend, &sending, pdMS_TO_TICKS(1000)) == pdTRUE) {
      esp_err_t result = esp_now_send(broadcastAddress, (uint8_t *) &sending, sizeof(sending));
      if (result == ESP_OK) {
        Serial.println("Sent with success");
      }
      else {
        Serial.println("Error sending the data");
      }
      
    }
  }
}

// Name: OnDataSent
// Description: Helper method to make sure data is sent and prints if the last packet is sent
void OnDataSent(const wifi_tx_info_t *mac_addr, esp_now_send_status_t status) {
  Serial.print("\r\nLast Packet Send Status:\t");
  Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Delivery Success" : "Delivery Fail");
}

// Name: OnDataRecv
// Description: Helper mehod to receive struct data and print the values received while also
//              putting said values into an auxillary struct to send to other tasks.
void OnDataRecv(const esp_now_recv_info_t* mac, const uint8_t *incomingData, int len) {
  memcpy(&andrewData, incomingData, sizeof(andrewData));
  Serial.print("Bytes received: ");
  Serial.println(len);

  hehe m2;

  Serial.print("turn_on_lcd: ");
  Serial.println(andrewData.turn_on_lcd);

  Serial.print("reset_logic_start: ");
  Serial.println(andrewData.reset_logic_start);

  m2.flag = andrewData.turn_on_lcd;
  m2.value = andrewData.reset_logic_start;
  xQueueSend(Status , &m2, portMAX_DELAY);

  Serial.println();
}

// **************************************************** REMOTE TASKS ***************************************************************

// Name: readRemote
// Description: this task decodes the signal from the IR remote and obtains a number 
//              to send through a queue to the testNumbers function
void readRemote(void *pvParameters){
  while(1){
    if(IrReceiver.decode()){
      //Serial.println(IR.decodedIRData.decodedRawData,DEC);
      uint32_t in = IrReceiver.decodedIRData.decodedRawData;
      IrReceiver.resume();
      //Serial.println((int)raw);

      int get =  getValue(in);
      xQueueSend(Numbers, &get, portMAX_DELAY);
    }
    vTaskDelay(pdMS_TO_TICKS(100));
  }
}

// Name: testNumbers
// Description: this task obtains the decoded numbers and prints them onto the LCD 
//              and the tries increment up to 4 if incorrect answer
void testNumbers(void *pvParameters) {
  int input;
  hehe m;
  while(1){
    if (xQueueReceive(Numbers, &input, portMAX_DELAY) == pdTRUE) {
      touchActivity();
      if(input == -5){
        touchActivity();
        tries++;
        m.flag = false;
        m.value = input;
        xQueueSend(Security, &m, portMAX_DELAY);
      }else if (input != -1 ){
          if(input == -2){
            LCD.setCursor(col, 1);
            LCD.print(" ");
            col--;
            i--;
          }else{
            col = i % 4; 
            LCD.setCursor(col, 1);   
            if(col == 0 && i != 0){
                for (int j = 0; j < 4; j++){
                    LCD.setCursor(j, 1);
                    LCD.print(" ");
                }
              LCD.setCursor(0, 1); 
          }
          LCD.print(input);
          arr[col] = input;
          i++;   
        }
      }
    }
   vTaskDelay(pdMS_TO_TICKS(100));
  }
}

// Name: checkPassword
// Description: this task checks whether the inputted numbers from the IR remote
//              is correct. After the tries increases to 4, send a signal to 
//              the other ESP to trigger the bomb. If the password is correct, 
//              send a signal to the other ESP to turn off sensors.
void checkPassword(void *pvParameters){
  hehe dequeue;
  struct_message sendTo;
  while(1){
    if (xQueueReceive(Security, &dequeue, portMAX_DELAY) == pdTRUE) {
      int  input = dequeue.value;
      bool accepted1  = dequeue.flag;

      if(input == -5){
        for(int i = 0; i < 4 ; i++){
          currentPassword = currentPassword * 10 + arr[i];
        }

        if(currentPassword == password || accepted1 ){
          LCD.clear();
          LCD.setCursor(0, 0);
          LCD.print("Welcome LinManu! ");
          sendTo.disable_andrew_sensor = 1;
          sendTo.andrew_explode = false;
        }else{
          if(tries >= 4){
            LCD.clear();
            LCD.print("You Are Cooked");
            myData.andrew_explode = true;
            sendTo.disable_andrew_sensor = 0;
            sendTo.andrew_explode = true;
          }else{
            LCD.clear();
            LCD.setCursor(0, 0);
            LCD.print("Invalid Entry");
            delay(1000);
            LCD.clear();
            LCD.setCursor(0, 0);
            LCD.print("Try Again");
            delay(1000);
            LCD.clear();
            LCD.setCursor(0, 0);
            LCD.print("Password: ");
            sendTo.disable_andrew_sensor = 0;
            sendTo.andrew_explode = false;
            
          }
        }
          col = 0; 
          i = 0;
          currentPassword = 0;
      }
      xQueueSend(ToSend , &sendTo, portMAX_DELAY);
    }
    
  }
}

// Name: getValue
// Description: Helper method to return integers from the decoded IR signals
int getValue(uint32_t x){
    if (x == -434503936) {         // 0
        return 0;
    } else if (x == -1169817856) { // 1
        return 1;
    } else if (x == -1186529536) { // 2
        return 2;
    } else if (x == -1203241216) { // 3
        return 3;
    } else if (x == -1153106176) { // 4
        return 4;
    } else if (x == -1086259456) { // 5
        return 5;
    } else if (x == -1136394496) { // 6
        return 6;
    } else if (x == -133693696) {  // 7
        return 7;
    } else if (x == -367657216) {  // 8
        return 8;
    } else if (x == -167117056) {  // 9
        return 9;
    } else if (x == -150405376) {  // Backspace
        return -2;
    } else if (x == -484638976) {  // Backspace
        return -5;
    } else {
        return -1; // unknown
    }
}

// **************************************************** RFID TASKS ***************************************************************

// Name: RFID
// Description: this task detects the RFID input and sends data to checkRFID
//              to get the RFID checked.
void RFID(void *arg){
  bool checkit;
  while(1){
    if (myRFID.PICC_IsNewCardPresent() && myRFID.PICC_ReadCardSerial()) {
      for (byte i = 0; i < myRFID.uid.size; i++) {
         storedUID[i] = myRFID.uid.uidByte[i];
      }
    myRFID.PICC_HaltA();
    checkit = true;
    xQueueSend(CheckRf, &checkit, portMAX_DELAY);
    Serial.println("Detected");
    }
    vTaskDelay(pdMS_TO_TICKS(100));
    checkit = false;
  }
}

// Names: checkRFID
// Description: this task checks whether the RFID matches the accepted identification.
void checkRFID(void *arg){
  bool checkit;
  hehe m;
  bool accepted = false;
  while(1){
    if (xQueueReceive(CheckRf, &checkit, portMAX_DELAY) == pdTRUE) {
      if(checkit){
        for (byte i = 0; i < myRFID.uid.size; i++) {
          if(storedUID[i] != myID1[i]){
            true1 = false;
          }
          if(storedUID[i] != myID2[i]){
            true2 = false;
          }
        }

        if(true1 || true2){
          Serial.println("Accepted");
          accepted = true;
        }else{
          accepted = false;
        }

        m.flag = accepted;
        m.value = -5;
        xQueueSend(Security, &m, portMAX_DELAY);
      }
    }
    vTaskDelay(pdMS_TO_TICKS(100));
  }
}

void loop() {                 
  // put your main code here, to run repeatedly:
}
