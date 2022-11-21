/**
 * A BLE client example that is rich in capabilities.
 * There is a lot new capabilities implemented.
 * author unknown
 * updated by chegewara
 */

#include "BLEDevice.h"

//Libraries for Wifi
#include "WiFi.h"
#include "arduino_secrets.h"

const char* ssid = SECRET_SSID;
const char* password = SECRET_PASS;
int status = WL_IDLE_STATUS;

const char* host = "g5wdbckuah.execute-api.us-east-1.amazonaws.com";

//#include "BLEScan.h"

// The remote service we wish to connect to.
static BLEUUID serviceUUID("cd0f82b0-d59f-4c9b-8575-467ce047aa9e");

// The characteristic of the remote service we are interested in.
static BLEUUID    charUUID("4fafc201-36e1-4688-b7f5-c5c9c331914b");

static boolean doConnect = false;
static boolean connected = false;
static boolean doScan = false;
static BLERemoteCharacteristic* pRemoteCharacteristic;
static BLEAdvertisedDevice* myDevice;

//MAC Address to get data for
String mac_address[2] = {"44:17:93:e0:e5:36", "44:17:93:e0:6c:66"};
int connected_to[2] = {0,0};
String battery_lvl[2] = {"-1","-1"};
int num_data = 0;
unsigned long startBLEScan_time;
int mac_index = 0;

int active_mac_address(String cur_mac) {
  for(int i = 0; i < (sizeof(mac_address) / sizeof(mac_address[0])); i++) {
    if(cur_mac.equals(mac_address[i]) && connected_to[i] == 0) {
      return i;
    }
  }
  return -1;
}



static void notifyCallback(
  BLERemoteCharacteristic* pBLERemoteCharacteristic,
  uint8_t* pData,
  size_t length,
  bool isNotify) {
    Serial.print("Notify callback for characteristic ");
    Serial.print(pBLERemoteCharacteristic->getUUID().toString().c_str());
    Serial.print(" of data length ");
    Serial.println(length);
    Serial.print("data: ");
    Serial.println((char*)pData);
}

class MyClientCallback : public BLEClientCallbacks {
  void onConnect(BLEClient* pclient) {
  }

  void onDisconnect(BLEClient* pclient) {
    connected = false;
    Serial.println("onDisconnect");
  }
};

bool connectToServer() {
    Serial.print("Forming a connection to ");
    Serial.println(myDevice->getAddress().toString().c_str());
    
    BLEClient*  pClient  = BLEDevice::createClient();
    Serial.println(" - Created client");

    pClient->setClientCallbacks(new MyClientCallback());

    // Connect to the remove BLE Server.
    pClient->connect(myDevice);  // if you pass BLEAdvertisedDevice instead of address, it will be recognized type of peer device address (public or private)
    Serial.println(" - Connected to server");
    pClient->setMTU(517); //set client to request maximum MTU from server (default is 23 otherwise)
  
    // Obtain a reference to the service we are after in the remote BLE server.
    BLERemoteService* pRemoteService = pClient->getService(serviceUUID);
    if (pRemoteService == nullptr) {
      Serial.print("Failed to find our service UUID: ");
      Serial.println(serviceUUID.toString().c_str());
      pClient->disconnect();
      return false;
    }
    Serial.println(" - Found our service");


    // Obtain a reference to the characteristic in the service of the remote BLE server.
    pRemoteCharacteristic = pRemoteService->getCharacteristic(charUUID);
    if (pRemoteCharacteristic == nullptr) {
      Serial.print("Failed to find our characteristic UUID: ");
      Serial.println(charUUID.toString().c_str());
      pClient->disconnect();
      return false;
    }
    Serial.println(" - Found our characteristic");

    // Read the value of the characteristic.
    if(pRemoteCharacteristic->canRead()) {
      std::string value = pRemoteCharacteristic->readValue();
      Serial.print("The characteristic value was: ");
      Serial.println(value.c_str());
    }

    // if(pRemoteCharacteristic->canNotify())
    //   pRemoteCharacteristic->registerForNotify(notifyCallback);

    connected = true;
    return true;
}
/**
 * Scan for BLE servers and find the first one that advertises the service we are looking for.
 */
class MyAdvertisedDeviceCallbacks: public BLEAdvertisedDeviceCallbacks {
 /**
   * Called for each advertising BLE server.
   */
  void onResult(BLEAdvertisedDevice advertisedDevice) {
    Serial.print("BLE Advertised Device found: ");
    Serial.println(advertisedDevice.toString().c_str());

    // We have found a device, let us now see if it contains the service we are looking for.
    //String test = 
    mac_index = active_mac_address(String(advertisedDevice.getAddress().toString().c_str()));
    Serial.println("MAC Address: " +  String(advertisedDevice.getAddress().toString().c_str()));
    Serial.printf("Does mac match:%d\n", mac_index);   
    if (advertisedDevice.haveServiceUUID() && mac_index != -1) {
      connected_to[mac_index] = 0;
      BLEDevice::getScan()->stop();
      myDevice = new BLEAdvertisedDevice(advertisedDevice);
      doConnect = true;
      doScan = true;


      //If there are no more mac address set doScan to false
      if((sizeof(mac_address) / sizeof(mac_address[0])) == num_data) {
        doScan = false;
      }

    } // Found our server
  } // onResult
}; // MyAdvertisedDeviceCallbacks










void setup() {
  Serial.begin(115200);
  
  //Get Function for mac addresses
  
  
  
  
  
  
  Serial.println("Starting Arduino BLE Client application...");
  BLEDevice::init("");

  // Retrieve a Scanner and set the callback we want to use to be informed when we
  // have detected a new device.  Specify that we want active scanning and start the
  // scan to run for 5 seconds.
  BLEScan* pBLEScan = BLEDevice::getScan();
  pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
  pBLEScan->setInterval(1349);
  pBLEScan->setWindow(449);
  pBLEScan->setActiveScan(true);
  pBLEScan->start(5, false);

  startBLEScan_time = millis();   

  //Search through BLE signals
  collectData();

  //Put Function to update data
  sendData();



  //ESP.restart();
} // End of setup.


// This is the Arduino main loop function.
void loop() {

} // End of loop



void sendData() {
  connectWiFi();
  for(int i = 0; i < (sizeof(mac_address) / sizeof(mac_address[0])); i++) {
    Serial.println("Updating #: " + String(i));
    updateSensor(mac_address[i], battery_lvl[i]);
  }
}

void updateSensor(String macAddress, String battery_level) {
    delay(5000);

    Serial.print("connecting to ");
    Serial.println(host);

    // Use WiFiClient class to create TCP connections
    WiFiClient client;
    const int httpPort = 80;
    if (!client.connect(host, httpPort)) {
        Serial.println("connection failed");
        return;
    }

    // We now create a URI for the request
    String url = "/Prod/dispensers?";

    Serial.print("Requesting URL: ");
    Serial.println(url);

    // This will send the request to the server
    client.print(String("GET ") + url + " HTTP/1.1\r\n" +
                 "Host: " + host + "\r\n" +
                 "Connection: close\r\n\r\n");
    unsigned long timeout = millis();

    while (client.available() == 0) {
        if (millis() - timeout > 5000) {
            Serial.println(">>> Client Timeout !");
            client.stop();
            return;
        }
    }

    // Read all the lines of the reply from server and print them to Serial
    while(client.available()) {
        String line = client.readStringUntil('\r');
        Serial.print(line);
    }

    Serial.println();
    Serial.println("closing connection");
}

void connectWiFi() {
    WiFi.begin(ssid, password);

    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }

    Serial.println("");
    Serial.println("WiFi connected");
    Serial.println("IP address: ");
    Serial.println(WiFi.localIP());  
}



void collectData() {
  //Attempt bluetooth connection
  while(millis() - startBLEScan_time <= 1000 * 60 * 5) { 
    // If the flag "doConnect" is true then we have scanned for and found the desired
    // BLE Server with which we wish to connect.  Now we connect to it.  Once we are 
    // connected we set the connected flag to be true.
    if (doConnect == true) {
      //Reset timer for each scan just in case.
      startBLEScan_time = millis();      
      if (connectToServer()) {
        Serial.println("We are now connected to the BLE Server.");
      } else {
        Serial.println("We have failed to connect to the server; there is nothin more we will do.");
      }
      doConnect = false;
    }

    // If we are connected to a peer BLE Server, read the characteristic.
    if (connected) {
      battery_lvl[mac_index] = pRemoteCharacteristic->readValue().c_str();
      Serial.println("Read the data: " + battery_lvl[mac_index]);
      
      num_data++;
      Serial.println("So far we found: " + String(num_data));
      delay(5000);
      
      if(num_data == 2) {
        Serial.println("Found all devices and collected all data.");
        BLEDevice::getScan()->stop();
        esp_bt_controller_disable();
        esp_bt_controller_deinit();
        btStop();
        doConnect = false;
        connected = false;
        break;
      }
      BLEDevice::getScan()->start(0);  
    }else if(doScan){
      BLEDevice::getScan()->start(0);  // this is just example to start scan after disconnect, most likely there is better way to do it in arduino
    } else {
      BLEDevice::getScan()->stop();
      esp_bt_controller_disable();
      esp_bt_controller_deinit();
      btStop();
      doConnect = false;
      connected = false;
      break;
    }
    
    delay(1000); // Delay a second between loops.
  }
}
