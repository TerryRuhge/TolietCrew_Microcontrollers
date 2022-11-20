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

int capdac = 0;
char result[100];

FDC1004 FDC;

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

const byte interruptPin = 13; //Pin Connected to Soap Dispensor motor
volatile int total_int = 0;
volatile boolean change = 0;
String MAC = "010101010101010101010101";
unsigned long lastmilis = 0;
unsigned long bouncemilis = 50;
String type = "85";

float full_dispenser = 13.03;
float last_measurement = 0;
float increment = 0;

void setup() {
  // Set up Printing
  Wire.begin();        //i2c begin
  Serial.begin(115200);
  // Set up interrupt Pin
  pinMode(interruptPin, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(interruptPin), dispensed, FALLING);

  //inital capacitance
  full_dispenser = 8.3;
  Serial.println("fulldispenser:");
  Serial.print(full_dispenser);
  Serial.println();
  
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
        
    Serial.println("Attempting Update");
    //String temp = MAC + type + String(total_int)
    delay(300);
    if(total_int % 2 == 0) {
      float new_measurement = get_avg_measurement(10);
      Serial.println("new_measurement:");
      Serial.print((new_measurement),4);
      Serial.println();
      if(new_measurement <= last_measurement - 0.001) {
        increment = last_measurement - new_measurement;
        Serial.println("increment:");
        Serial.print((increment),4);
        Serial.println();      
      }
      Serial.println("Battery level:");
      float level = round((new_measurement-7.38)/(full_dispenser-7.38));
      if( level > 100 )
        level = 100;
      if (level < 0)
        level = 0;
      Serial.println(round((new_measurement-7.49)/(full_dispenser-7.49)));
      Serial.println(level);
      String temp = type + String(total_int) + String(level);
      char buffer[temp.length()+1];
      temp.toCharArray(buffer,temp.length() + 1);
      customCharacteristic.setValue((char*)&buffer);
      customCharacteristic.notify();    
      Serial.println(total_int);
    }

    
    
    interrupts();
    change = !change;
  }

      
}

void dispensed() {
   
  if (change == 0) {
    noInterrupts(); 
    total_int = total_int + 1;
    change = 1;
  }  
}

float get_avg_measurement(int n) {
  int32_t avg = 0;
  for(int i = 0; i < n; i++) {
    FDC.configureMeasurementSingle(MEASURMENT, CHANNEL, capdac);
    FDC.triggerSingleMeasurement(MEASURMENT, FDC1004_100HZ);
    delay(15);
    uint16_t value[2];
    if (! FDC.readMeasurement(MEASURMENT, value)) {
      int16_t msb = (int16_t) value[0];
      int32_t capacitance = ((int32_t)457) * ((int32_t)msb); //in attofarads
      capacitance /= 1000;   //in femtofarads
      capacitance += ((int32_t)3028) * ((int32_t)capdac);

      //Serial.print((((float)capacitance/1000)),4);
      avg = avg + capacitance;


      if (msb > UPPER_BOUND) {
        if (capdac < FDC1004_CAPDAC_MAX)
          capdac++;
      }
    else if (msb < LOWER_BOUND) {
        if (capdac > 0)
          capdac--;
      }

    }
  }
  Serial.println((((float)(avg/n)/1000)),4);
  return (((float)(avg/n)/1000));
}
