#include <RTCZero.h>
#include <ArduinoLowPower.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <WiFiNINA.h>
#include <ArduinoHttpClient.h>
#include <Adafruit_Sensor.h>
#include <DHT.h>
#include <DHT_U.h>

#include "secrets.h"

#define DHTPIN 2  
#define DHTTYPE DHT22
#define OLED_RESET 4

// Wake-Up Interrupt pin
const int manualPin = 5;
bool displayOn = true;

DHT_Unified dht(DHTPIN, DHTTYPE);
Adafruit_SSD1306 display(OLED_RESET);

const char serverAddress[] = "tigoe.io";
String contentType = "application/json";
String postPath = "/data";
String temp;
String humid;
int port = 443;
int period = 30000; // every 30s

char ssid[] = SECRET_SSID;
char pass[] = SECRET_PASS;
String myMacAddress = SECRET_MAC;
String session = SECRET_SESH;
String statusCode = "none";

WiFiSSLClient wifi;
HttpClient client = HttpClient(wifi, serverAddress, port);

void setup() {
     Serial.begin(9600);
  // Start Display and DHT sensor
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C); // Initialize display
  display.clearDisplay(); // Clear display before each start
  delay(100);
  dht.begin();

  // Initiated First Scree
  int connectionAttempt = 1;
  String message = "Connecting to " + String(ssid) + "; Attempt " + String(connectionAttempt);
  setStatusMessage(message,statusCode);

  // Wait until connected
  while ( WiFi.status() != WL_CONNECTED) {
    WiFi.begin(ssid, pass);
    connectionAttempt++;
    message = "Connecting to " + String(ssid) + "; Attempt " + String(connectionAttempt);
    setStatusMessage(message,statusCode);
  }
  setStatusMessage("Connected",statusCode);
  delay(3000);
  display.fillRect(0, 0, 128, 64, BLACK);
  display.display();
  LowPower.attachInterruptWakeup(manualPin, turnOnDisplay, CHANGE); 
}

void loop() {
   collectReadings();
   if (displayOn) {
     displayReadings();
   }
   establishConnection();
   sendReadings();
   if (displayOn){
    delay(10000);
    display.fillRect(0, 0, 128, 64, BLACK);
    display.display();
    displayOn = false;
   }
   delay(5000);
   LowPower.sleep(period);
}

void collectReadings(){
  sensors_event_t event;  
  dht.temperature().getEvent(&event);
  if (isnan(event.temperature)) {
    temp = "n/a";
  }
  else {
    temp = String(event.temperature);
  }
  dht.humidity().getEvent(&event);

  if (isnan(event.relative_humidity)) {
    humid =  "n/a";
  }
  else {
    humid = String(event.temperature);
  }
}

void setStatusMessage(String message, String statusCode){
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(0,0);
  display.fillRect(0, 0, 128, 16, BLACK);
  display.println(message);
  String statusMessage;
  if (statusCode != NULL){
    statusMessage = "Status code: " + statusCode;
  } else {
    statusMessage = "Haven't sent data";
  }
  display.print(statusMessage); 
  display.display();
}

void showParameters(String temp, String humid){
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(0,16);
  display.fillRect(0, 16, 128, 64, BLACK);
  display.println(temp);
  display.println("");
  display.println(humid);
  display.display();
}

void displayReadings(){
  String tempMessage = "Temperature: " + temp + "C";  
  String humidMessage = "Humidity: " + humid + "%";
  showParameters(tempMessage, humidMessage);
}

void establishConnection(){
  if (WiFi.status() != WL_CONNECTED){
    Serial.println("Establishing connection");
    int connectionAttempt = 1;
    String message = "Connecting to " + String(ssid) + "; Attempt " + String(connectionAttempt);
    if (displayOn){
      setStatusMessage(message,statusCode);
    }
  
    while ( WiFi.status() != WL_CONNECTED) {
      WiFi.begin(ssid, pass);
      connectionAttempt++;
      if (displayOn){
        message = "Connecting to " + String(ssid) + "; Attempt = " + String(connectionAttempt);
        setStatusMessage(message, statusCode);
      }
    }

    if (displayOn){
      setStatusMessage("Connected",statusCode);
    }
    
  } else {
    if (displayOn){
      setStatusMessage("Connected",statusCode);
    }
  }
}

void sendReadings(){
    Serial.println("sending data");
    String dataJSON = "{\'temperature\':" + temp  + ",\'humidity\':" + humid + "}";
    String mac = "\"macAddress\":\""+myMacAddress+"\"";
    String sessionKey = "\"sessionKey\":\""+session+"\"";
    String data = "\"data\":\""+dataJSON+"\"";
    String postData = "{"+mac+","+sessionKey+","+data+"}";
    client.post(postPath,contentType,postData);
  
    statusCode = String(client.responseStatusCode());
    String response = client.responseBody();
    Serial.print("status code: ");
   Serial.println(statusCode);
}

void turnOnDisplay(){
  displayOn = true;
}
