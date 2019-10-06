#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <Wire.h>

//define access point
#ifndef STASSID
#define STASSID "tickle_me2"
#define STAPSK  "elmo1985"
#endif

//define i2c address for the different nixies
#define nixie_s1 B0100000
#define nixie_s2 B0100001

#define nixie_m1 B0100010
#define nixie_m2 B0100011

#define nixie_h1 B0100100
#define nixie_h2 B0100101

//define i2c io ports
#define I2C_SDA 2
#define I2C_SCL 13

//global constants
const char* ssid = STASSID;
const char* password = STAPSK;
const long utcOffsetInSeconds = 3600;
bool PCFOutput = true;

unsigned long previousMillis = 0;    
const long interval = 10000;

//global variables
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", utcOffsetInSeconds);

ESP8266WebServer server(80);

void handleAbout() {
  String message = "{'madeby':'sergekeyser@gmail.com'}";
  server.send(200, "application/json", message );
}

void handleRoot() {
  String message = "{'time':'" + timeClient.getFormattedTime() + "'}";
  server.send(200, "application/json", message );
}

void handleNotFound() {
  String message = "File Not Found\n\n";
  message += "URI: ";
  message += server.uri();
  message += "\nMethod: ";
  message += (server.method() == HTTP_GET) ? "GET" : "POST";
  message += "\nArguments: ";
  message += server.args();
  message += "\n";
  for (uint8_t i = 0; i < server.args(); i++) {
    message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
  }
  server.send(404, "text/plain", message);
}

void setup(void) {
  
  //start serial
  Serial.begin(57600);
  
  //start wifi
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  Serial.println("");

  //wait for connection
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
    
  Serial.print("Connected to ");
  Serial.print(ssid);
  Serial.print("IP address: ");
  Serial.print(WiFi.localIP());
  
  //start mdns protocol
  if (MDNS.begin("nixie")) {
    Serial.println("MDNS responder started");
  }

  //define available endpoints
  server.on("/", handleRoot);
  server.on("/about", handleAbout);

  //define what to do if the requested endpoint is not defined in the above list
  server.onNotFound(handleNotFound);

  //start http server
  server.begin();
  
  //start the NTP client
  timeClient.begin();
  Serial.println("NTP Service started");
    
  //initialize the i2c service
  Wire.begin(I2C_SDA, I2C_SCL); 
}

void IOexpanderWrite(byte address, byte _data ) 
{
 byte error;
 Wire.beginTransmission(address);
 Wire.write(_data);
 error = Wire.endTransmission(); 
}

byte IOexpanderRead(int address) 
{
 byte _data;
 Wire.requestFrom(address, 1);
 if(Wire.available()) {
   _data = Wire.read();
 }
 return _data;
}

void loop(void) {
  server.handleClient();
  MDNS.update();
  //updates every 60 seconds (even though it is called more often)
  timeClient.update();
}
