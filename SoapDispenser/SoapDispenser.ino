#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>

#include <Wire.h>
#include <Protocentral_FDC1004.h>

#define UPPER_BOUND  0X4000                 // max readout capacitance
#define LOWER_BOUND  (-1 * UPPER_BOUND)
#define CHANNEL 1                          // channel to be read
#define MEASURMENT 0                       // measurment channel

/* define the characteristic and it's propeties */
BLECharacteristic customCharacteristic(
  BLEUUID("19b10001-e8f2-537e-4f6c-d104768a1214"),
  BLECharacteristic::PROPERTY_READ |
  BLECharacteristic::PROPERTY_NOTIFY
);

/* define the UUID that our custom service will use */
#define serviceID BLEUUID("19b10001-e8f2-537e-4f6c-d104768a1237")

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
  // Set up Printing
  Serial.begin(115200);
  // Set up interrupt Pin
  pinMode(interruptPin, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(interruptPin), dispensed, FALLING);
  
  BLEDevice::init("Soap_Disp"); // Name your BLE Device
  BLEServer *MyServer = BLEDevice::createServer();  //Create the BLE Server
  MyServer->setCallbacks(new ServerCallbacks());  // Set the function that handles server callbacks
  BLEService *customService = MyServer->createService(serviceID); // Create the BLE Service
  customService->addCharacteristic(&customCharacteristic);  // Create a BLE Characteristic
  customCharacteristic.addDescriptor(new BLE2902());  // Create a BLE Descriptor
  MyServer->getAdvertising()->addServiceUUID(serviceID);  // Configure Advertising
  customService->start(); // Start the service  
  MyServer->getAdvertising()->start();  // Start the server/advertising

  Serial.println("Waiting for a client to connect....");

  Wire.begin();        //i2c begin
  Serial.begin(115200); // serial baud rate
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
