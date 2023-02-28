#include "MAX30100_PulseOximeter.h"
#include <U8g2lib.h>
#include <Wire.h>

#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClient.h>

const char* ssid = "p";
const char* password = "21212121";

#define REPORTING_PERIOD_MS     500
U8G2_SSD1306_128X32_UNIVISION_F_HW_I2C u8g2(U8G2_R0);

PulseOximeter pox;

const int numReadings=15;
float filterweight=0.5;
uint32_t tsLastReport = 0;
uint32_t last_beat=0;
int readIndex=0;
int average_beat=0;
int average_SpO2=0;
bool calculation_complete=false;
bool calculating=false;
bool initialized=false;
byte beat=0;

// Callback (registered below) fired when a pulse is detected
void onBeatDetected()
{
  show_beat();
  last_beat=millis();
}

void send_data(int blood, int heart) {
  // Connect to WiFi network
    
Serial.begin(115200);
  WiFi.begin(ssid,password);

 while(WiFi.status() != WL_CONNECTED) { 
    delay(5000);
    Serial.print(".?");
  }
  Serial.println("");
  Serial.print("Connected to WiFi network with IP Address: ");
  Serial.println(WiFi.localIP());

  while (!Serial) continue;

 if(WiFi.status()== WL_CONNECTED){
      WiFiClient wifiClient;
      HTTPClient http;

      String name = "Mrx";
      int age = 27;
      String history = "none";
      String phone = "912601609";
   String serverName = "http://192.168.20.191:3000/upload_sensor_data";

  String serverPath = serverName + "?heart=" + heart   + "&blood=" + blood + 
"&username=" + name + "&age=" + age + "&history=" + history + "&phone="+ phone+"";

    // Your Domain name with URL path or IP address with path
    http.begin(wifiClient,serverPath.c_str());

    Serial.println(serverPath);
    
    int status = http.GET();

    Serial.println(status);
    Serial.println("====done==");

    //free the resource
http.end();

 }
  else {
    Serial.println("WiFi Disconnected");
  }
  //Send an HTTP POST request every 15 seconds
  delay(3000);  
}

void show_beat() 
{
  u8g2.setFont(u8g2_font_cursor_tf);
  u8g2.setCursor(8,10);
  if (beat==0) {
    u8g2.print("_");
    beat=1;
  } 
  else
  {
    u8g2.print("^");
    beat=0;
  }
  u8g2.sendBuffer();
}



void initial_display() 
{
  if (not initialized) 
  {
    u8g2.clearBuffer();
    show_beat();
    u8g2.setCursor(24,12);
    u8g2.setFont(u8g2_font_unifont_t_bengali);
    u8g2.print("Place Finger");  
    u8g2.setCursor(0,30);
    u8g2.print("On The Sensor..");
    u8g2.sendBuffer(); 
    initialized=true;
  }
}

void display_calculating(int j)
{
  if (not calculating) {
    u8g2.clearBuffer();
    calculating=true;
    initialized=false;
  }
  u8g2.clearBuffer();
  show_beat();
  u8g2.setCursor(24,12);
  u8g2.setFont(u8g2_font_unifont_t_bengali);
  u8g2.print("Measuring...."); 
  u8g2.setCursor(0,30);
  for (int i=0;i<=j;i++) {
    u8g2.print(".");
  }
  u8g2.sendBuffer();
}




void display_values()
{
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_unifont_t_bengali);
 
  u8g2.setCursor(65,12);  
  u8g2.print(average_beat);
  u8g2.print(" Bpm");
  u8g2.setCursor(0,30);
  u8g2.print("SpO2 ");
  u8g2.setCursor(65,30);  
  u8g2.print(average_SpO2);
  u8g2.print(" %"); 
  u8g2.sendBuffer();

  if(average_SpO2> 0 && average_beat>0){
    send_data(average_SpO2, average_beat);
  } 
}



void calculate_average(int beat, int SpO2) 
{
  if (readIndex==numReadings) {
    calculation_complete=true;
    calculating=false;
    initialized=false;
    readIndex=0;
    display_values();
    // send_data(average_SpO2, average_beat);
  }
  
  if (not calculation_complete and beat>30 and beat<220 and SpO2>50) {
    average_beat = filterweight * (beat) + (1 - filterweight ) * average_beat;
    average_SpO2 = filterweight * (SpO2) + (1 - filterweight ) * average_SpO2;
    readIndex++;
    display_calculating(readIndex);
    // send_data(average_SpO2, average_beat);
  }

  
}

void setup()
{
    Serial.begin(115200);
    Wire.begin(4,5);
    u8g2.begin();
    pox.begin();
    pox.setOnBeatDetectedCallback(onBeatDetected);
    initial_display();
}

void loop()
{
    // Make sure to call update as fast as possible
    pox.update();
    if ((millis() - tsLastReport > REPORTING_PERIOD_MS) and (not calculation_complete)) {
        calculate_average(pox.getHeartRate(),pox.getSpO2());
        tsLastReport = millis();
    }
    if ((millis()-last_beat>5000)) {
      calculation_complete=false;
      average_beat=0;
      average_SpO2=0;
      initial_display();
    }
}
