#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
#include <HCSR04.h>

const int trigPin = 9;
const int echoPin = 10;

long duration;
long distance;
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
String MAC = "010101010101010101010101";
String type = "85";

void setup() {
  // Set up code is here, it runs once
  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);
  // Set up Printing
  Serial.begin(9600);
  
  BLEDevice::init("Toilet_Paper_Disp"); // Name your BLE Device
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

  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);

  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);
  
  duration = pulseIn(echoPin, HIGH);
  distance = duration*0.034/2;

  Serial.print("Distance: ");
  Serial.println(distance);
  
}
