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

// This will help if your board is not of the same type as mine
#if (SSD1306_LCDHEIGHT != 64)
#error("Height incorrect, please fix Adafruit_SSD1306.h!");
#endif

DHT_Unified dht(DHTPIN, DHTTYPE);
Adafruit_SSD1306 display(OLED_RESET);

const char serverAddress[] = "tigoe.io";
String contentType = "application/json";
String postPath = "/data";
String temp;
String humid;
int port = 443;
unsigned long timeStamp = 0;
int period = 3600000; // every 1 hour

char ssid[] = SECRET_SSID;
char pass[] = SECRET_PASS;
String myMacAddress = SECRET_MAC;
String session = SECRET_SESH;
int statusCode;

WiFiSSLClient wifi;
HttpClient client = HttpClient(wifi, serverAddress, port);

void setup() {
  // put your setup code here, to run once:
  // Serial.begin(9600);
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C); // Initialize display
  display.clearDisplay(); // Clear display before each start
  delay(100);
  
  dht.begin();
  int connectionAttempt = 1;
  String message = "Connecting to " + String(ssid) + "; Attempt " + String(connectionAttempt);
  setStatusMessage(message,statusCode);
  
  while ( WiFi.status() != WL_CONNECTED) {
    // Serial.print("Attempting to connect to Network named: ");
    // Serial.println(ssid);                   // print the network name (SSID);

    // Connect to WPA/WPA2 network:
    WiFi.begin(ssid, pass);
    connectionAttempt++;
    message = "Connecting to " + String(ssid) + "; Attempt " + String(connectionAttempt);
    setStatusMessage(message,statusCode);
  }
  setStatusMessage("Connected",statusCode);
  // Serial.print("SSID: ");
  // Serial.println(WiFi.SSID());

  // print your WiFi shield's IP address:
  IPAddress ip = WiFi.localIP();
  // Serial.print("IP Address: ");
  // Serial.println(ip);
}

void loop() {
  // put your main code here, to run repeatedly
  
   collectReadings();
   displayReadings();
   checkForConnection();
   sendReadings();
   updateConnectionStatus();
   delay(1000);
}

void collectReadings(){
  sensors_event_t event;  
  dht.temperature().getEvent(&event);
  if (isnan(event.temperature)) {
    // Serial.print("Error w/ Temp.!");
  }
  else {
    // Serial.print("Temp.: ");
    // Serial.print(event.temperature);
    // Serial.print(" C");
    temp = String(event.temperature);
  }
  // Get humidity event and print its value.
  dht.humidity().getEvent(&event);

  if (isnan(event.relative_humidity)) {
    // Serial.print("Error w/ Humid.!");
  }
  else {
    // Serial.print("Humid.: ");
    // Serial.print(event.relative_humidity);
    // Serial.println("%");
    humid = String(event.temperature);
  }
}

void setStatusMessage(String message, int statusCode){
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(0,0);
  display.fillRect(0, 0, 128, 16, BLACK);
  display.println(message);
  String statusMessage;
  if (statusCode != NULL){
    statusMessage = "Status code: " + String(statusCode);
  } else {
    statusMessage = "Haven't sent data";
  }
  display.print(statusMessage); 
  display.display();
}

void showParameters(String temp, String humid, String desiredTemp){
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(0,16);
  display.fillRect(0, 16, 128, 64, BLACK);
  display.println(temp);
  display.println("");
  display.println(humid);
  display.println("");
  display.print(desiredTemp);
  display.display();
}

void setMainMessage(String message, int pixelH, int erase){
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(0,pixelH);
  if (erase){
    display.fillRect(0, 16, 128, 64, BLACK);
  }
  display.print(message);  
  display.display();
}

void displayReadings(){
  String tempMessage = "Temperature: " + temp + "C";  
  String humidMessage = "Humidity: " + humid + "%";
  int setTemp = map(analogRead(2), 1024, 0, 15, 30);
  String desiredTemp = "Set Temp.: " + String(setTemp) + "C";
  showParameters(tempMessage, humidMessage, desiredTemp);
}

void checkForConnection(){
  if (WiFi.status() != WL_CONNECTED){
    int connectionAttempt = 1;
    String message = "Connecting to " + String(ssid) + "; Attempt " + String(connectionAttempt);
    setStatusMessage(message,statusCode);
    setMainMessage("The connection has dropped, restoring the connection", 16, true);
  
    while ( WiFi.status() != WL_CONNECTED) {
      // Serial.print("Attempting to connect to Network named: ");
      // Serial.println(ssid);                   // print the network name (SSID);

      // Connect to WPA/WPA2 network:
      WiFi.begin(ssid, pass);
      connectionAttempt++;
      message = "Connecting to " + String(ssid) + "; Attempt = " + String(connectionAttempt);
      setStatusMessage(message, statusCode);
    }
  }
}

void sendReadings(){
  if (millis() - timeStamp > period){
    String dataJSON = "{\'temperature\':" + temp  + ",\'humidity\':" + humid + "}";
    String mac = "\"macAddress\":\""+myMacAddress+"\"";
    String sessionKey = "\"sessionKey\":\""+session+"\"";
    String data = "\"data\":\""+dataJSON+"\"";
    String postData = "{"+mac+","+sessionKey+","+data+"}";
    // Serial.print(postData);
    client.post(postPath,contentType,postData);
  
    statusCode = client.responseStatusCode();
    String response = client.responseBody();
    // Serial.println("");
    // Serial.print("Status code: ");
    // Serial.println(statusCode);
    // Serial.print("Response: ");
    // Serial.println(response);
    // Serial.println("");
    // Serial.println("");
    timeStamp = millis();
  }
}

void updateConnectionStatus(){
   setStatusMessage("Connected", statusCode);
}
