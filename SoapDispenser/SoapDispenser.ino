#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>

/* define the characteristic and it's propeties */
BLECharacteristic customCharacteristic(
  BLEUUID((uint16_t)0x1A00),
  BLECharacteristic::PROPERTY_READ |
  BLECharacteristic::PROPERTY_NOTIFY
);

/* define the UUID that our custom service will use */
#define serviceID BLEUUID((uint16_t)0x1700)

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
String MAC = "010101010101010101010101";
String type = "01";

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
}

void loop() {
  // put your main code here, to run repeatedly:
  //Serial.println(total_int);
  if(change) {
    String temp = MAC + type + total_int;
    char buffer[temp.length()+1];
    temp.toCharArray(buffer,temp.length() + 1);
    customCharacteristic.setValue((char*)&buffer);
    customCharacteristic.notify();    
    change = !change;
  }
}

void dispensed() {
  total_int = total_int + 1;
  change = 1;
}
