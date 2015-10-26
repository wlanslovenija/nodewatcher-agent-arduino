// wlan slovenija sensor node - JSON data push to nodewatcher
// Prior to using this sketch you must register a node on https://nodes.wlan-si.net
// Node must be configured without IP allocation and platform, configured for Push data wihtout authentication
// Paste the UUID in the variable below
#include <ESP8266WiFi.h>


const String uuid   = "";//insert UUID between quotation marks

const char* ssid     = "open.wlan-si.net";
const char* password = "";
const String host = "push.nodes.wlan-si.net";
const char* charHost = "push.nodes.wlan-si.net";
const String url = "/push/http/";
const int httpPort = 80;

unsigned long push_time = 0;
const long push_interval = 60000; // default every 60s or on trigger
boolean push_trigger = LOW ;
//sensor variables for publishing
float sensor1 = 0;
float sensor2 = 0;

float prev_sensor1 = 0;

void setup() {
  Serial.begin(115200); // for debugging
  
  delay(10); // delay for everything to settle
  // connecto to WiFi 
  WiFi.begin(ssid, password);
  // wait for connection to be established
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    // Serial.print(".");
  }

  Serial.println("");
  Serial.print("WiFi connected to:");
  Serial.println(ssid);  
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  push_time=millis(); // push immediately first available values
}

void loop() {
  // first read sensors
  prev_sensor1=sensor1; // store previous measurement
  sensor1=analogRead(A0);
  sensor2=digitalRead(1);
  // perform any calculations on variables

  //hysteresys trigger and immediate push
  if(sensor1<50 & prev_sensor1>60){
    push_trigger=HIGH;
  }
  
  // push to nodewatcher
  if(push_time<millis() | push_trigger==HIGH){
    pushto_nodewatcher();
    push_trigger=LOW;
    push_time=millis()+push_interval;
  }
  
  delay(1000); // run every second
}

void pushto_nodewatcher(){
  WiFiClient client;
  // preparing JSON data for nodewatcher manually, lighter then libraries
  //header
  String jsondata = "{\"sensors.generic\": {\"_meta\": {\"version\": 1";
  //################################################################
  // EDIT HERE with your sensor variables, copy/paste for more sensors
  // sensor1
  jsondata +="},\"my_sensor_1\": {\"name\": \"My Sensor Voltage 1\", \"unit\": \"V\", \"value\":";
  jsondata += String(sensor1);
  // sensor2
  jsondata +="},\"my_sensor_2\": {\"name\": \"My Sensor Voltage 2\", \"unit\": \"V\", \"value\":";
  jsondata += String(sensor2);
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
}
