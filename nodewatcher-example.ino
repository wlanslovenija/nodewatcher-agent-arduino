// wlan slovenija sensor node - JSON data push to nodewatcher
// this script reports: ambient ligting level with TSL2561 sensor, temperature for all attached 1Wire sensors and RSSI
// Prior to using this sketch you must register a node on https://nodes.wlan-si.net
// Node must be configured without IP allocation and platform, configured for Push data wihtout authentication
// Paste the UUID in the variable below
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <Wire.h>

//general defines
#define LED_BLUE 16

// all variables associated with 1wire
#define ONE_WIRE_BUS D6  // DS18B20: pin D6 on nodemcu / GPIO12
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature DS18B20(&oneWire);
String str_addr[15]; // array of addresses, increase size if there are more sensors
int sensor_count=0;

// configuration for pushing to wlan slovenija servers
const String uuid   = "<your UUID>";//insert UUID between quotation marks
const String host = "push.nodes.wlan-si.net";
const char* charHost = "push.nodes.wlan-si.net";
const String url = "/push/http/";
const int httpPort = 80;
unsigned long push_time = 0; // storing time
const long push_interval = 5000; // default every 60s or on trigger
boolean push_trigger = LOW ; // such taht you can push on trigger also
 
// WiFi network settings
const char* ssid     = "open.wlan-si.net";
const char* password = "";

// webserver configuration, so you can manually check/read sent data on unit IP
ESP8266WebServer server(80);

// variable for storing json constructed output
String jsondata = "";

// called every time you access the module IP address
void handleRoot() {
  server.send(200, "text/plain", jsondata);
}

void setup() {
  // Debug uart interface
  Serial.begin(115200);

  // Pin setup
  pinMode(LED_BLUE,OUTPUT);
  digitalWrite(LED_BLUE,HIGH);

  delay(10); // delay for everything to settle
  
  // connect to WiFi 
  WiFi.begin(ssid, password);
  // wait for connection to be established
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  //set led to on
  digitalWrite(LED_BLUE,HIGH);

  Serial.println("");
  Serial.print("WiFi connected to:");
  Serial.println(ssid);  
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
  Serial.print("RSSI: ");
  Serial.println(WiFi.RSSI());

  // Start up the 1wire library
  DS18B20.begin();
  //configure resolution
  DS18B20.setResolution(TEMP_12_BIT);

  delay(10);
  
  // Locate devices on the bus
  Serial.print("Locating devices...");
  Serial.print("Found ");
  sensor_count=DS18B20.getDeviceCount();
  Serial.print(sensor_count, DEC);
  Serial.println(" devices.");

  // finds an address at a given index on the bus
  for(char i=0;i<sensor_count;i++){
      DeviceAddress address; //storing address for processing
      DS18B20.getAddress(address, i);
      //make address printable
      char str_buffer[17];//address length plus one
      char* ptr=&str_buffer[0];
      for(char j=0;j<8;j++){
        // print address to ASCII array
        sprintf(ptr,("%02x"), address[j]);
        ptr=ptr+2;   
    }
    //make sure buffer is null terminated
    str_buffer[16]=0x00;
    str_addr[i] = String(str_buffer);
    Serial.println(str_addr[i]);    
  }

  // Webserver
  server.on("/", handleRoot);
  server.begin();
  Serial.println("HTTP server started");

  // Set time such we push immediately
  push_time=millis(); // push immediately first available values
}


void pushto_nodewatcher(){
  // read sensors
  uint16_t broadband, ir, light;
  
  WiFiClient client;
  // preparing JSON data for nodewatcher manually, lighter then libraries
  //header
  jsondata = "{\"sensors.generic\": {\"_meta\": {\"version\": 1";
  //################################################################
  // EDIT HERE with your sensor variables, copy/paste for more sensors
  // rssi
  jsondata +="},\"rssi\": {\"name\": \"WiFi signal strength\", \"unit\": \"dBm\", \"value\":";
  jsondata += String(WiFi.RSSI());

  // multiple temperatures
  for(char i=0; i<sensor_count;i++){
    float temp;
    do {
      DS18B20.requestTemperatures(); 
      temp = DS18B20.getTempCByIndex(i);
      Serial.print("Temperature ");
      Serial.print(i,DEC);
      Serial.print(" (");
      Serial.print(str_addr[i]);
      Serial.print(") ");
      Serial.println(temp);
    } while (temp == 85.0 || temp == (-127.0));
  
    jsondata +="},\"temp_sensor_";
    jsondata += String(i,DEC);
    jsondata += "\": {\"name\": \"Temperature ";
    jsondata += String(i,DEC);
    jsondata += " (";
    jsondata += str_addr[i]; //prints 1wire address
    jsondata += ")";
    jsondata += "\", \"unit\": \"C\", \"value\":";
    jsondata += String(temp);
  }

  //################################################################
  //footer
  jsondata += "}}}\r\n";

  // communication with nodewatcher
  if (client.connect(charHost, httpPort)) {
    client.print(String("POST ") + url + uuid + " HTTP/1.1\r\n" + "Host: " + host + 
    "\r\n" + "User-Agent: Arduino/1.0\r\n" + "Content-Type: text/plain\r\n" + 
    "Content-Length: " + jsondata.length() + "\r\n" + "Connection: close\r\n\r\n");
    client.println(jsondata);
  } else {
    //Serial.println("connection failed");
  }
    if (client.available()) {
    char c = client.read();
    // this does not give any useful output yet
  }
}


void loop() {
  // push to nodewatcher
  if(push_time<millis() | push_trigger==HIGH){
    pushto_nodewatcher();
    push_trigger=LOW;
    push_time=millis()+push_interval;
  }

  if(WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi Down");
    digitalWrite(LED_BLUE,HIGH);
    delay(500);
  }
  server.handleClient();
}
