/* Name: SoapDispenser.ino                      */
/* Author: Terry Ruhge                          */
/* Sources:                                     */
/* Last updated: 11/19/2022                     */
/* Descrption:                                  */

//Libraries for BLE service for ESP32
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>

//Libraries for Break Capacitance
#include <Wire.h>
#include <Protocentral_FDC1004.h>



/* Definitions for Breakout Capacitance Service */
#define UPPER_BOUND  0X4000                 // max readout capacitance
#define LOWER_BOUND  (-1 * UPPER_BOUND)     // min readout capacitance
#define CHANNEL 1                           // channel to be read
#define MEASURMENT 0                        // measurment channel

/* service UUID for entire Bluetooth TolietCrew service */
#define serviceID BLEUUID("cd0f82b0-d59f-4c9b-8575-467ce047aa9e")

/* Defines seconds for deepsleep function */
#define uS_TO_S_FACTOR 1000000
#define TIME_TO_SLEEP 5 //Time to sleep in seconds
#define CONNECTION_ATTEMPT_TIME_OUT 300000 //Time to attempt connection in milliseconds
#define CONNECTION_TIME_OUT 300000 //Time to be connected without disconnect in milliseconds



/* This function handles server callbacks */
bool deviceConnected = false;
class ServerCallbacks: public BLEServerCallbacks {
    void onConnect(BLEServer* MyServer) {
      deviceConnected = true;
    };
    void onDisconnect(BLEServer* MyServer) {
      deviceConnected = false;
    }
};

/* This function is for testing the wakeup reasons */
void print_wakeup_reason(){
  esp_sleep_wakeup_cause_t  wakeup_reason;

  wakeup_reason = esp_sleep_get_wakeup_cause();

  switch(wakeup_reason) {
    case ESP_SLEEP_WAKEUP_EXT0 : Serial.println("Wakeup caused by external signal using RTC_IO"); break;
    case ESP_SLEEP_WAKEUP_EXT1 : Serial.println("Wakeup caused by external signal using RTC_CNTL"); break;
    case ESP_SLEEP_WAKEUP_TIMER : Serial.println("Wakeup caused by timer"); break;
    case ESP_SLEEP_WAKEUP_TOUCHPAD : Serial.println("Wakeup caused by touchpad"); break;
    case ESP_SLEEP_WAKEUP_ULP : Serial.println("Wakeup caused by ULP program"); break;
    default : Serial.printf("Wakeup was not cause by deep sleep: %d\n",wakeup_reason); break;
  }  
}

/* UUID characteristics & properties for Soap Dispenser */
BLECharacteristic customCharacteristic(
  BLEUUID("4fafc201-36e1-4688-b7f5-c5c9c331914b"),
  BLECharacteristic::PROPERTY_READ |
  BLECharacteristic::PROPERTY_NOTIFY
);



//Variables for counter/interrupt of soap dispenser activations
const byte interruptPin = 2; //Pin Connected to Soap Dispensor motor
volatile int total_int = 0;
volatile boolean change = 1;

//Variables for BLE
String output = "";

//Variables for deepsleep
RTC_DATA_ATTR int bootCount = 0;

//Variables for breakout capacitance
FDC1004 FDC;
int capdac = 0;
char result[100];

float fullcapacitance = 0;
float emptyCapacitance = 7.03;

void setup() {
  // Set up Capacitance Breakout
  Wire.begin(); // i2c begin
  
  // Set up Interrupt for counter
  //pinMode(interruptPin, INPUT_PULLUP);
  //attachInterrupt(digitalPinToInterrupt(interruptPin), dispensed, FALLING);

  // Set up Printing
  Serial.begin(115200);

  //Take some time to open up the Serial Monitor
  delay(1000); 

  //setup full and empty capacitance
  if(bootCount == 0) {
    fullcapacitance = get_avg_cap_read(10) + 0.03;
  }  
  
  ++bootCount;  
  Serial.println("Boot number: " + String(bootCount));   

  print_wakeup_reason(); 

  send_update();
   
  esp_sleep_enable_timer_wakeup(TIME_TO_SLEEP * uS_TO_S_FACTOR);
  Serial.println("Setup ESP32 to sleep for every " + String(TIME_TO_SLEEP) +
  " Seconds");

  Serial.println("Going to sleep now");
  delay(1000);
  Serial.flush(); 
  esp_deep_sleep_start();
  Serial.println("This will never be printed");
}

void loop() {

}

//For initial drop in capacitance
void detectChange() {
    
}

void send_update() {
  //BLE Service
  
  int soap_lvl = get_level();
  Serial.printf("\nSaop Level: %d", soap_lvl);
  Serial.print("%\n");
  ble_send(soap_lvl);
}

bool ble_send(int soap_lvl) {
  BLEDevice::init("Soap_Dispenser"); // Soap Dispenser Service
  BLEServer *MyServer = BLEDevice::createServer();  //Create the BLE Server
  MyServer->setCallbacks(new ServerCallbacks());  // Set the function that handles server callbacks
  BLEService *customService = MyServer->createService(serviceID); // Create the BLE Service
  customService->addCharacteristic(&customCharacteristic);  // Create a BLE Characteristic
  customCharacteristic.addDescriptor(new BLE2902());  // Create a BLE Descriptor
  MyServer->getAdvertising()->addServiceUUID(serviceID);  // Configure Advertising
  customService->start(); // Start the service  
  MyServer->getAdvertising()->start();  // Start the server/advertising
  Serial.println("Waiting for a client to connect....");


  bool connected = 0;
  bool client_disconnected = 0;
  unsigned long attemptStart = millis();  
  unsigned long connectionStart; 
  //Loop until connection was connected and disconnected
  while(!client_disconnected)  {
    //Client Connected
    if(deviceConnected) {
      //First Time Connection
      if(!connected) {
        Serial.println("Connected to Client");
        connectionStart = millis();
        Serial.println("Attempting Update");
      }
      
      //update output of BLE
      output = String(soap_lvl);
      char buffer[output.length()+1];
      output.toCharArray(buffer,output.length() + 1);
      customCharacteristic.setValue((char*)&buffer);
      customCharacteristic.notify();    
      //Serial.println("Update Complete");
      connected = 1;
    }
    
    //If hasn't been connected to and been longer than desired time end now;
    if(!connected && millis() - attemptStart > CONNECTION_ATTEMPT_TIME_OUT) {
      Serial.println("Connection took too long ending attempt");
      break;
    }

    //Exit loop once device has been disconnected by client
    if(connected && !deviceConnected) {
      Serial.println("Disconnected by client");
      connected = 0;
      client_disconnected = 1;      
    }
    //Exit loop once device has been disconnected after not being disconnected from client for too long
    if(connected && millis() - connectionStart > CONNECTION_TIME_OUT) {
      Serial.println("Disconnected by Timeout");
      connected = 0;
      client_disconnected = 1;    
    }    
  }
  //Reset Value (Shouldn't matter since we are going into deep sleep)
  client_disconnected = 0;
  return 0;
}

float get_one_cap_read() {
  //configure Sensor
  FDC.configureMeasurementSingle(MEASURMENT, CHANNEL, capdac);
  Serial.println(MEASURMENT);
  FDC.triggerSingleMeasurement(MEASURMENT, FDC1004_100HZ);

  //wait for Configuration
  delay(15);
  uint16_t value[2];

  //If valid read then  
  if (! FDC.readMeasurement(MEASURMENT, value)) {
    //Convert to capacitance value
    int16_t msb = (int16_t) value[0];
    int32_t capacitance = ((int32_t)457) * ((int32_t)msb); //in attofarads
    capacitance /= 1000;   //in femtofarads
    capacitance += ((int32_t)3028) * ((int32_t)capdac);

    //Print for Testing
    Serial.print((((float)capacitance/1000)),4);
    Serial.print("  pf \n");

//Update Balance    
    if (msb > UPPER_BOUND) {
      if (capdac < FDC1004_CAPDAC_MAX)
        capdac++;
    }
    else if (msb < LOWER_BOUND) {
      if (capdac > 0)
        capdac--;
    }
    return ((float)capacitance)/1000;         
  }
  //Measurement Could not be taken
  return -1;
}

float get_avg_cap_read(int n) {
  //make sure n is an acceptable value
  if(n > 1) {
    float avg_value = 0;
    float max_value = emptyCapacitance / 2;
    float min_value = fullcapacitance * 2;
    //collect and average single measurements
    for(int i = 0; i < n; i++) {
      float current_cap_value = get_one_cap_read();
      //Make sure range of values is acceptable      
      if (min_value > current_cap_value)
        min_value = current_cap_value;
      if (max_value < current_cap_value)
        max_value = current_cap_value;
      avg_value = avg_value + current_cap_value/n;        
    }
    if(max_value - min_value > 0.15) {
      //get next average to be less than current to prevent darastic recurrsion
      return get_avg_cap_read(n - 1);    
    } else {
      return avg_value;      
    }
  }
  return get_one_cap_read();
}

int get_level() {
  //converts current capacitance level to battery level
   int soap_lvl = 0;
    float current_cap = get_avg_cap_read(10);
    if (current_cap != -1) {
      soap_lvl = round(((current_cap-emptyCapacitance)/(fullcapacitance-emptyCapacitance)) * 100);
      //make sure level is witihin and includes (0-100)
      if(soap_lvl > 100) {
        soap_lvl = 100;
      } else if(soap_lvl < 0) {
        soap_lvl = 0;
      }
      return soap_lvl;
    }  
  return -1;  
}
