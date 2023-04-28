// ------------------------------------------------------------
// Depot Handler
// Janick Wäspi 
// 12.04.2023
// Metrohm AG
// 
// handles the interaction between the server and the depot
// ------------------------------------------------------------
#include <ArduinoJson.h>
#include <EthHttpClient.h>
#include <Ethernet.h>
#include <SPI.h>
#include "HX711.h"

// MUX wiring
const int A0_Pin = 2;
const int A1_Pin = 16;
const int A2_Pin = 0;

// HX711 wiring
const int DOUT_Pin = 4;
const int SCK_Pin = 5;

// global variables
static int amountOfBoxes = 0;
static int depotId = 0;

byte mac[] = {0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED};    // MAC address
IPAddress server(10, 0, 2, 109);                      // server IP address 
EthernetClient client;                                // Ethernet client
EthHttpClient boxes(server, client, "/api");    // rest api
HX711 scale;                                          // Hx711 scale

// ------------------------------------------------------------
// initializes the ethernet connection and ethernet shield
// builds a connection with the rest API
// checks for possible errors
// initializes MUX Pins
// ------------------------------------------------------------
void SetupDepot(){

  Ethernet.init(15);      // Pin 15 for ESP8266

  // start the Ethernet connection:
  Serial.println("Initialize Ethernet with DHCP:");
  if (Ethernet.begin(mac) == 0) {
    Serial.println("Failed to configure Ethernet using DHCP");
    // check for Ethernet hardware
    while (Ethernet.hardwareStatus() == EthernetNoHardware){
      Serial.println("Ethernet shield was not found");
      delay(1000);
    }
    // check if ethernet cable is connected
    while (Ethernet.linkStatus() == LinkOFF) {
      Serial.println("Ethernet cable is not connected.");
      delay(1000);
    }
  } else {
    Serial.print("  DHCP assigned IP ");
    Serial.println(Ethernet.localIP());
  }
  delay(1000);          // give the Ethernet shield time to initialize

  // set MUX Pins as outputs
  pinMode(A0_Pin, OUTPUT);
  pinMode(A1_Pin, OUTPUT);
  pinMode(A2_Pin, OUTPUT);

  // set Hx711 I2C Pins
  scale.begin(DOUT_Pin, SCK_Pin);
  delay(100);
}
// ------------------------------------------------------------
// detects how many boxes are avaiable
// creates n amount of box objects
// ------------------------------------------------------------
void InitializeBoxes(){

  // Convert mac byte array to string
  char macStr[18];
  sprintf(macStr, "%02X:%02X:%02X:%02X:%02X:%02X", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
  String macAddress = String(macStr);

  
  String jsonBody = "{\"mac\": \""  + macAddress + "\"}";   // define json body
  String res = boxes.Post("/depots/", jsonBody);            // create a new depot Page  

  // extract response body
  int start = res.indexOf("{");                   
  int end = res.lastIndexOf("}");
  String body = res.substring(start, end+1);
  
  DynamicJsonDocument doc(1024);                  // create a json object
  deserializeJson(doc, body);                     // Parse the response as a JSON object
  depotId = doc["id"];                            // Extract the ID value from the JSON object

  
  delay(1000);                                        // wait 1s
  boxes.Delete("/boxes/" + String(depotId), "{}");    // deletes existing boxes
  // loop through all MUX addresses
  for(int i = 0; i < 8; i++){
    // set MUX address
    digitalWrite(A0_Pin, i & 0x01);
    digitalWrite(A1_Pin, i & 0x02);
    digitalWrite(A2_Pin, i & 0x04);
    delay(500);
    // check if scale is ready to measure
    if(scale.is_ready()){
      long val = scale.read();  // read ADC value  
      Serial.println(val);
      // check if a box is connected
      if(val > 1000){
        amountOfBoxes++;
        boxes.Post("/boxes/" + String(depotId), "{}");  // create box object
      }
    }
  }
  Serial.print("Detected Boxes:");
  Serial.println(amountOfBoxes);
  Serial.println();
}
// ------------------------------------------------------------
// updates the weight of all boxes
// ------------------------------------------------------------
void UpdateBoxes(){
  // loops through every box
  for(int i = 0; i < amountOfBoxes; i++){
    // set MUX address
    digitalWrite(A0_Pin, i & 0x01);
    digitalWrite(A1_Pin, i & 0x02);
    digitalWrite(A2_Pin, i & 0x04);
    delay(500);
    // check if scale is ready to measure
    if(scale.is_ready()){
      long val = scale.read();            // read ADC value
      String jsonBody = "{\"weight\": "  + String(val) + "}";
      boxes.Put("/boxes/ESP/" + String(depotId) + "/" + String(i + 1), jsonBody);
    }
  }
}
