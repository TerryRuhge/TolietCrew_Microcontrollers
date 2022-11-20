
#include <WiFiNINA.h>

#include "arduino_secrets.h"

char ssid[] = SECRET_SSID;        // your network SSID (name)
char pass[] = SECRET_PASS;    // your network password (use for WPA, or use as key for WEP)
int status = WL_IDLE_STATUS;     // the Wifi radio's status

byte mac[6];

char server[] = "g5wdbckuah.execute-api.us-east-1.amazonaws.com";
String url = "/Prod";
//const char fingerprint[] PROGEM = "64 60 D8 6B 33 A2 62 1C 31 F9 1F 5A C8 F4 07 6F 9B A6 81 F6";
//IPAddress server(108,138,159,23); //Ip of above address
WiFiSSLClient client;

void printData() {
  Serial.println("Board Information:");
  // print your board's IP address:
  IPAddress ip = WiFi.localIP();
  Serial.print("IP Address: ");
  Serial.println(ip);

  Serial.println();
  Serial.println("Network Information:");
  Serial.print("SSID: ");
  Serial.println(WiFi.SSID());

  // print the received signal strength:
  long rssi = WiFi.RSSI();
  Serial.print("signal strength (RSSI):");
  Serial.println(rssi);
}


void setup() {
  //Initialize serial and wait for port to open:
  Serial.begin(9600);
  while (!Serial);

  // attempt to connect to Wifi network:
  while (status != WL_CONNECTED) {
    Serial.print("Attempting to connect to network: ");
    WiFi.macAddress(mac)
    Serial.println(String(mac));
    Serial.println(ssid);
    // Connect to WPA/WPA2 network:
    status = WiFi.begin(ssid, pass);

    // wait 10 seconds for connection:
    delay(10000);
  }

  // you're connected now, so print out the data:
  Serial.println("You're connected to the network");

  Serial.println("----------------------------------------");
  printData();
  Serial.println("----------------------------------------");

  Serial.println("\nStarting connection to server...");
  // if you get a connection, report back via serial:
  if (client.connectSSL(server, 443)) {
    Serial.println("connected to server");
    // Make a HTTP request:
    client.println(String("GET ") + url + "/dispensers?" + " HTTP/1.1");
    client.println("Host: g5wdbckuah.execute-api.us-east-1.amazonaws.com");
    client.println("Connection: close");
    client.println();
  }

}

void loop() {
  // check the network connection once every 10 seconds:
 delay(10000);
 Serial.println("----------------------------------------");
  while(client.available()) {
    char c = client.read();
    Serial.write(c);     
  }

  if (!client.connected()) {
    Serial.println();
    Serial.println("disconnecting from server.");
    client.stop();

    // do nothing forevermore:
    while (true);
  }
}
