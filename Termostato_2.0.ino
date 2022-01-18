#include <Arduino.h>
#include "DHT.h"
#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <AsyncElegantOTA.h>
#include "SPIFFS.h"
#include <Arduino_JSON.h>
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

//Screen
Adafruit_SSD1306 display(128, 64, &Wire, -1); //width, height, rest pin

//DHT & Relay
#define DHTPIN 4
#define RELAY 17
#define DHTTYPE DHT11

//Botones
#define BOTONENCENDIDO 25
#define SUBIRTEMPERATURA 26
#define BAJARTEMPERATURA 27

// Wifi credentials and hostname
const char* ssid = "BatcuevaDomo";
const char* password = "06022011";
const String hostname = "Termostato2";

//WebServer
AsyncWebServer server(80);
AsyncWebSocket ws("/ws");

//Timer variables
unsigned long lastTime = 0;
unsigned long timerDelay = 10000;

//Thermostat variables
DHT dht(DHTPIN, DHTTYPE);
int tempON = 24;
float tempAmbient;
boolean work = false;


void setup() {
  Serial.begin(115200);
  delay(200);
  initWiFi();
  SPIFFS.begin();
  initWebSocket();
  initScreen();
  dht.begin();

  //Botones
  pinMode(BOTONENCENDIDO, INPUT_PULLUP); 
  pinMode(SUBIRTEMPERATURA, INPUT_PULLUP);
  pinMode(BAJARTEMPERATURA, INPUT_PULLUP);

  //Relay
  pinMode(RELAY,OUTPUT);
  digitalWrite(RELAY, LOW);
  configServer();
}

void loop() {
  checkSensors();
  checkButtons();
  AsyncElegantOTA.loop();
  ws.cleanupClients();
}
void initWiFi() {
    Serial.print("Connecting to ");
    Serial.print(ssid);
    WiFi.setHostname(hostname.c_str());
   WiFi.begin(ssid, password);
   while (WiFi.status() != WL_CONNECTED) {
     delay(100);
     Serial.print(".");
   }
    Serial.println("");
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
    server.begin();
}

void initWebSocket(){
  ws.onEvent(onEvent);
  server.addHandler(&ws);
}

void initScreen(){
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  display.setTextColor(WHITE);
  display.clearDisplay();
  display.display();                                                                                       
}

void onEvent(AsyncWebSocket *server, AsyncWebSocketClient *client,AwsEventType type, void *arg, uint8_t *data, size_t len) {
  switch (type) {
    case WS_EVT_DATA:
      handleWebSocketMessage(arg, data, len);
    break;
    case WS_EVT_CONNECT:
    case WS_EVT_DISCONNECT:
    case WS_EVT_PONG:
    case WS_EVT_ERROR:
    break;
  }
}

void handleWebSocketMessage(void *arg, uint8_t *data, size_t len) {
  AwsFrameInfo *info = (AwsFrameInfo*)arg;
  if (info->final && info->index == 0 && info->len == len && info->opcode == WS_TEXT) {
    data[len] = 0;
    if (strcmp((char*)data, "states") == 0) {
      notifyClients(getDataReadings());
    }
    else{
      int botonpulsado = atoi((char*)data);
      modifySetUp(botonpulsado);
      notifyClients(getDataReadings());
    }
  }
}

void notifyClients(String state) {
  ws.textAll(state);
}

// Get Sensor Readings and return JSON object
String getDataReadings(){
  JSONVar readings;
  if (tempAmbient != dht.readTemperature()){
    updateScreen();
  }
  tempAmbient = dht.readTemperature();
  readings["working"] = String (work);
  readings["temperature"] = String(tempON);
  readings["ambient"] =  String(tempAmbient);
  String jsonString = JSON.stringify(readings);
  return jsonString;
}

void updateScreen(){
    display.clearDisplay();
    display.display();
    if (work){
      display.setTextSize(6);
      display.setCursor(0,0);
      display.println(String(tempON));
      display.setTextSize(2);
      display.setCursor(80,40);
      display.println(String(tempAmbient,1));
    }else{
      display.clearDisplay();
    }
    display.display();
}

void configServer(){
  // Web Server Root URL
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(SPIFFS, "/index.html", "text/html");
  });
  server.serveStatic("/", SPIFFS, "/");

  // Request for the latest sensor readings
  server.on("/readings", HTTP_GET, [](AsyncWebServerRequest *request){
    String json = getDataReadings();
    request->send(200, "application/json", json);
    json = String();
  });
  
  AsyncElegantOTA.begin(&server);
  server.begin();
}

void checkSensors(){
    if ((millis() - lastTime) > timerDelay) {
      
      lastTime = millis();
      if (work){
        if ((tempON > dht.readTemperature())&(digitalRead(RELAY)==LOW))
        {
          digitalWrite(RELAY, HIGH);
        }
        if ((tempON <= dht.readTemperature())&(digitalRead(RELAY)==HIGH))
        {
          digitalWrite(RELAY, LOW);
        }
        updateScreen(); 
        notifyClients(getDataReadings());
      }      
    }
}

void checkButtons(){
  if (digitalRead(BOTONENCENDIDO) == LOW)
  { 
    delay(300);   
    onOffThermostat();
    updateScreen();
    notifyClients(getDataReadings());
    
  }
  if (digitalRead(SUBIRTEMPERATURA) == LOW)
  {
    delay(300); 
    tempON++;
    updateScreen();
    notifyClients(getDataReadings());
    
  }
  if (digitalRead(BAJARTEMPERATURA) == LOW)
  {
    delay(300); 
    tempON--;
    updateScreen();
    notifyClients(getDataReadings());
    
  }
}

void modifySetUp(int botonpulsado){
  switch (botonpulsado) {
    case 1:
      onOffThermostat();
      break;
    case 2:
      tempON++;
      break;
    case 3:
      tempON--;
      break;
  }
  updateScreen();
}

void onOffThermostat(){
    work = !work;
    digitalWrite(RELAY,LOW);
}
