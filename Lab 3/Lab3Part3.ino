// File Name: Lab3Part3_Manu_Lin
// Author: Arjun Manu, Andrew Lin
// Date: 7/24/25
// Description: Part 3 introduces us to utilizing interrupts. We are using a timer interrupt(not really an interrupt functionally) to 
//              increment a counter and print it to an LCD, a bluetooth app to send a signal to the ESP32 to print a message to an LCD,
//              and a button to print a message to the LCD.

#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

LiquidCrystal_I2C lcd(0x27, 16, 2);

#define SDA_PIN 14
#define SCL_PIN 13
#define BUTTON_PIN 37

uint8_t temp;
volatile bool BLEMessage = false;
volatile bool buttonMessage = false;
volatile int counter = 0;
hw_timer_t* timer = NULL;

#define SERVICE_UUID        "9d3727a9-000a-45a7-8ded-5a8eb657a6e1"
#define CHARACTERISTIC_UUID "6cea4e99-69bd-4e93-8c15-93e271b11dc3"

class MyCallbacks: public BLECharacteristicCallbacks {
   void onWrite(BLECharacteristic *pCharacteristic) {
     // =========> TODO: This callback function will be invoked when signal is
     // 		     received over BLE. Implement the necessary functionality that
     //		     will trigger the message to the LCD.
     BLEMessage = true;
   }
};

// Name: timerISR
// Descriipton: Timer ISR that increments counter
void IRAM_ATTR timerISR(){
    counter++;
}

// Name: buttonISR
// Description: ISR for button
void IRAM_ATTR buttonISR(){
  buttonMessage = true;
}

void setup() {
  Serial.begin(115200);
 BLEDevice::init("MyESP32");
 BLEServer *pServer = BLEDevice::createServer();
 BLEService *pService = pServer->createService(SERVICE_UUID);
 BLECharacteristic *pCharacteristic = pService->createCharacteristic(
                                        CHARACTERISTIC_UUID,
                                        BLECharacteristic::PROPERTY_READ |
                                        BLECharacteristic::PROPERTY_WRITE
                                      );


 pCharacteristic->setCallbacks(new MyCallbacks());
 pService->start();
 BLEAdvertising *pAdvertising = pServer->getAdvertising();
 pAdvertising->start();


   // Initializing LCD display
   Wire.begin(SDA_PIN, SCL_PIN);
   lcd.init();
   // Creating a timer, attaching an interrupt, setting an alarm which will
   //                  update the counter every second.
   //
   timer = timerBegin(100000);
   timerAttachInterrupt(timer, &timerISR);
   timerAlarm(timer, 100000, true, 0);

    // Setting button pin as input and attaching an interrupt
    pinMode(BUTTON_PIN, INPUT_PULLUP);
    attachInterrupt(digitalPinToInterrupt(BUTTON_PIN), buttonISR, FALLING);
}


void loop() {
 //  Printing out an incrementing counter to the LCD.
 //        If a signal has been received over BLE, print out “New
 //        Message!” on the LCD.
 //        If the button has been pressed, print out "Button Pressed"
 //        on the LCD.

  if(BLEMessage){
    BLEMessage = false;
    Serial.println("New Message!");
    display_command(0xC0);

    String BLEInput = "New Message!";
    for(int i = 0; i < BLEInput.length(); i++){
      char c = BLEInput.charAt(i);
      display_data(c);
    }
    timerStop(timer);
    delay(2000);
    timerStart(timer);

  } else if(buttonMessage){
    buttonMessage = false;
    Serial.println("Button Pressed");
    display_command(0xC0);

    String buttonInput = "Button Pressed";
    for(int i = 0; i < buttonInput.length(); i++){
      char c = buttonInput.charAt(i);
      display_data(c);
    }
    timerStop(timer);
    delay(2000);
    timerStart(timer);

  } else {
    display_command(0x01);
    display_command(0x80);

    String counterNumber = String(counter);
    for(int i = 0; i < counterNumber.length(); i++){
      char c = counterNumber.charAt(i);
      display_data(c);
    }
    delay(1000);
    Serial.println(counter);
  }
}

// Name: display_data
// Description: Sends data to display to an LCD module via I2C
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
