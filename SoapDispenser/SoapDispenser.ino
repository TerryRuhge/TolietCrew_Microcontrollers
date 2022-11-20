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

/* UUID characteristics & properties for Soap Dispenser */
BLECharacteristic customCharacteristic(
  BLEUUID("a1efe119-fe3e-4d94-a542-fcb2fb4bb6f5"),
  BLECharacteristic::PROPERTY_READ |
  BLECharacteristic::PROPERTY_NOTIFY
);

/* service UUID for entire Bluetooth TolietCrew service */
#define serviceID BLEUUID("cd0f82b0-d59f-4c9b-8575-467ce047aa9e")

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

const byte interruptPin = 2; //Pin Connected to Soap Dispensor motor
volatile int total_int = 0;
volatile boolean change = 1;

void setup() {
  // Set up Capacitance Breakout
  Wire.begin(); // i2c begin
  
  // Set up Interrupt for counter
  pinMode(interruptPin, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(interruptPin), dispensed, FALLING);

  // Set up Printing
  Serial.begin(115200);
  

  //BLE Service
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
}

void loop() {
  // put your main code here, to run repeatedly:
  //Serial.println(total_int);
  if(change) {
    Serial.println("Attempting Update");
    //String temp = MAC + type + String(total_int);
    String temp = type + String(total_int);
    char buffer[temp.length()+1];
    temp.toCharArray(buffer,temp.length() + 1);
    customCharacteristic.setValue((char*)&buffer);
    customCharacteristic.notify();    
    Serial.println(total_int);

    FDC.configureMeasurementSingle(MEASURMENT, CHANNEL, capdac);
    Serial.println(MEASURMENT);
    FDC.triggerSingleMeasurement(MEASURMENT, FDC1004_100HZ);
    Serial.println(MEASURMENT);
    Serial.println(MEASURMENT);

    //wait for completion
    delay(15);
    uint16_t value[2];
    if (! FDC.readMeasurement(MEASURMENT, value)) {
      int16_t msb = (int16_t) value[0];
      int32_t capacitance = ((int32_t)457) * ((int32_t)msb); //in attofarads
      capacitance /= 1000;   //in femtofarads
      capacitance += ((int32_t)3028) * ((int32_t)capdac);

      Serial.print((((float)capacitance/1000)),4);
      Serial.print("  pf \n");

      if (msb > UPPER_BOUND) {
        if (capdac < FDC1004_CAPDAC_MAX)
          capdac++;
      }
      else if (msb < LOWER_BOUND) {
        if (capdac > 0)
          capdac--;
      }
      change = !change;
    }
  }
}

void dispensed() {
  if (change == 0) {
    total_int = total_int + 1;
    change = 1;
  }  
}
