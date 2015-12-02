// wlan slovenija sensor node - JSON data push to nodewatcher
// this script reports: ambient ligting level with TSL2561 sensor, temperature for all attached 1Wire sensors and RSSI
// Prior to using this sketch you must register a node on https://nodes.wlan-si.net
// Node must be configured without IP allocation and platform, configured for Push data wihtout authentication
// Paste the UUID in the variable below
#include <ESP8266WiFi.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_TSL2561_U.h>

#define ONE_WIRE_BUS D6  // DS18B20: pin D6 / GPIO12
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature DS18B20(&oneWire);

// Light sensor definition, set address accordingly
Adafruit_TSL2561_Unified tsl = Adafruit_TSL2561_Unified(TSL2561_ADDR_LOW, 12345);

const String uuid   = "<UUID>";//insert UUID between quotation marks
const String host = "push.nodes.wlan-si.net";
const char* charHost = "push.nodes.wlan-si.net";
const String url = "/push/http/";

// WiFi network settings
const char* ssid     = "open.wlan-si.net";
const char* password = "";

// for debugging you can use requestbin and configure settings as below:
/*
const String host = "requestb.in";
const char* charHost = "requestb.in";
const String url = "/";
const String uuid   = "wmf4igwm";//insert UUID from requestbin
*/

const int httpPort = 80;

unsigned long push_time = 0;
const long push_interval = 60000; // default every 60s or on trigger
boolean push_trigger = LOW ;
int sensor_count=0;

/**************************************************************************/
void configureSensor(void)
{
  /* You can also manually set the gain or enable auto-gain support */
  // tsl.setGain(TSL2561_GAIN_1X);      /* No gain ... use in bright light to avoid sensor saturation */
  // tsl.setGain(TSL2561_GAIN_16X);     /* 16x gain ... use in low light to boost sensitivity */
  tsl.enableAutoRange(true);            /* Auto-gain ... switches automatically between 1x and 16x */
  
  /* Changing the integration time gives you better sensor resolution (402ms = 16-bit data) */
  // tsl.setIntegrationTime(TSL2561_INTEGRATIONTIME_13MS);      /* fast but low resolution */
  // tsl.setIntegrationTime(TSL2561_INTEGRATIONTIME_101MS);  /* medium resolution and speed   */
  tsl.setIntegrationTime(TSL2561_INTEGRATIONTIME_402MS);  /* 16-bit data but slowest conversions */
}


void setup() {
  Serial.begin(115200); // for debugging
   // Start up the 1wire library
  DS18B20.begin();

  // Start i2c ports
  Wire.begin(D1,D2);
  /* Setup the sensor gain and integration time */
  configureSensor();
  
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
  Serial.print("RSSI: ");
  Serial.println(WiFi.RSSI());

 

  // locate devices on the bus
  Serial.print("Locating devices...");
  Serial.print("Found ");
  sensor_count=DS18B20.getDeviceCount();
  Serial.print(sensor_count, DEC);
  Serial.println(" devices.");

  push_time=millis(); // push immediately first available values
}

void loop() {
    // push to nodewatcher
  if(push_time<millis() | push_trigger==HIGH){
    pushto_nodewatcher();
    push_trigger=LOW;
    push_time=millis()+push_interval;

  }
  
  delay(1000); // run every second
}

void pushto_nodewatcher(){
  // read sensors
  uint16_t broadband, ir;
  tsl.getLuminosity(&broadband, &ir);
  uint16_t light = tsl.calculateLux(broadband, ir);
  
  WiFiClient client;
  // preparing JSON data for nodewatcher manually, lighter then libraries
  //header
  String jsondata = "{\"sensors.generic\": {\"_meta\": {\"version\": 1";
  //################################################################
  // EDIT HERE with your sensor variables, copy/paste for more sensors
  // rssi
  jsondata +="},\"rssi\": {\"name\": \"WiFi signal strength\", \"unit\": \"dBm\", \"value\":";
  jsondata += String(WiFi.RSSI());
  // sensor1
  jsondata +="},\"light_1\": {\"name\": \"raw broadband amplitude\", \"unit\": \"\", \"value\":";
  jsondata += String(broadband);
  // sensor2
  jsondata +="},\"light_2\": {\"name\": \"raw ir amplitude\", \"unit\": \"\", \"value\":";
  jsondata += String(ir);
  // sensor2
  jsondata +="},\"light_3\": {\"name\": \"Ambient light\", \"unit\": \"lux\", \"value\":";
  jsondata += String(ir);

  Serial.print(broadband); Serial.print(" broad ");
  Serial.print(ir); Serial.print(" ir ");
  Serial.print(light); Serial.println(" lux");
  
  // multiple temperatures
  for(char i=0; i<sensor_count;i++){
    float temp;
    do {
      DS18B20.requestTemperatures(); 
      temp = DS18B20.getTempCByIndex(i);
      Serial.print("Temperature ");
      Serial.print(i,DEC);
      Serial.print(" ");
      Serial.println(temp);
    } while (temp == 85.0 || temp == (-127.0));
  
    jsondata +="},\"temp_sensor_";
    jsondata += String(i,DEC);
    jsondata += "\": {\"name\": \"Temperature ";
    jsondata += String(i,DEC);
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
    Serial.print(c);
  }
}

