#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <NTPClient.h>
#include <WiFiUdp.h>

#include <Wire.h>


#ifndef STASSID
#define STASSID "tickle_me2"
#define STAPSK  "elmo1985"
#endif


#define DEVICE_1 B0100000
#define DEVICE_2 B0100001  

const char* ssid = STASSID;
const char* password = STAPSK;
const long utcOffsetInSeconds = 3600;


char daysOfTheWeek[7][12] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};
bool PCFOutput = true;

unsigned long previousMillis = 0;    
const long interval = 10000;

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", utcOffsetInSeconds);

ESP8266WebServer server(80);



void handleRoot() {
  
  if(PCFOutput)
   IOexpanderWrite(DEVICE_1, 0x00 );
  else
   IOexpanderWrite(DEVICE_1, 0x01 );
  PCFOutput = !PCFOutput;
   Serial.println(PCFOutput);
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
 
  
  Serial.begin(57600);
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  Serial.println("");

  // Wait for connection
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  
  
  
  Serial.println("");
  Serial.print("Connected to ");
  Serial.println(ssid);
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  if (MDNS.begin("esp8266")) {
    Serial.println("MDNS responder started");
  }
  Serial.println("Start Scanning I2C");
  

  timeClient.begin();
  server.on("/", handleRoot);

  server.on("/inline", []() {
    server.send(200, "text/plain", "this works as well");
  });

  server.onNotFound(handleNotFound);

  server.begin();
  Serial.println("HTTP server started");
  
  Wire.begin(2, 13); //GPIO2 and 13
  IOexpanderWrite(DEVICE_1, 0x0F);
}

void IOexpanderWrite(byte address, byte _data ) 
{
 byte error;
 Wire.beginTransmission(address);
 Wire.write(_data);
 error = Wire.endTransmission(); 
 if(error == 0)
  Serial.println("transmission, no error");
 else
  Serial.println("Transmission error"+error);
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
  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis >= interval) {
      previousMillis = currentMillis;
      timeClient.update();
    }
}
