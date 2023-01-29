#include <Adafruit_GFX.h>    // Core graphics library for custom fonts
#include "FreeSans10pt7b.h"
#include "DejaVuSans9pt7b.h"

#include "time.h"
#include <WiFi.h>
#include <PubSubClient.h>

//forOTA:
#include <Arduino.h>
#include <WiFiClient.h>
#include <WebServer.h>
#include <ESPmDNS.h>
#include <Update.h>
#include "otaweb.h"
#include <HTTPClient.h>
//end OTA

const char* mqtt_server = "172.16.1.91";
const char* mqttUser = "your mqtt user";
const char* mqttPass = "your mqtt password";
const char* mqttTopic_A = "home/outside/temperature";
const char* mqttTopic_I = "home/bath/temperature";
const char* host = "ledclock1"; //network hostname, also MQTT client ID
const char * ssid="YOUR WIFI SSID";
const char * wifipw="YOUR WIFI KEY";
const int analogPin  = 36; //pin where the LDR for brightness control is connected to
// light sensor cabeling: +3.3V --10K-- AnalogPIN --LDR-- GND
const int numReadings  = 100; //brightness average over numReadings count


int ResetCounter =0;
bool I_msgArrived = false;
bool A_msgArrived = false;
int readings [numReadings];
int readIndex  = 0;
long total  = 0;

char A_message[10]; //buffer for outside temperature
char I_message[10]; //buffer for bathroom temperature

#include <ESP32-HUB75-MatrixPanel-I2S-DMA.h>
#define R1 5
#define G1 17
#define BL1 18
#define R2 19
#define G2 16
#define BL2 25
#define CH_A 26
#define CH_B 4
#define CH_C 27
#define CH_D 2
#define CH_E -1 // assign to any available pin if using two panels or 64x64 panels with 1/32 scan i.e. "32"
#define CLK 14
#define LAT 15
#define OE 13

#define PANEL_RES_X 64      // Number of pixels wide of each INDIVIDUAL panel module. 
#define PANEL_RES_Y 32     // Number of pixels tall of each INDIVIDUAL panel module.
#define PANEL_CHAIN 1      // Total number of panels chained one to another
 
//MatrixPanel_I2S_DMA dma_display;
MatrixPanel_I2S_DMA *dma_display = nullptr;

uint16_t myBLACK = dma_display->color565(0, 0, 0);
uint16_t myWHITE = dma_display->color565(255, 255, 255);
uint16_t myRED = dma_display->color565(255, 0, 0);
uint16_t myGREEN = dma_display->color565(0, 255, 0);
uint16_t myBLUE = dma_display->color565(0, 0, 255);

int analogVal  = 0;
int analogAvg = 0;
int mappedAvg = 100; //start brightness of display

//Begin OTA
char bu[100];
String msg;
String GET_Request(const char* server) {
  HTTPClient http;    
  http.begin(server);
  int httpResponseCode = http.GET();
  
  String payload = "{}"; 
  
//  if (httpResponseCode>0) {
  if (httpResponseCode==200) {
    //Serial.print("HTTP Response code: ");
    //Serial.println(httpResponseCode);
    payload = http.getString();
  }
  else {
    Serial.print("Error code: ");
    Serial.println(httpResponseCode);
    msg=httpResponseCode; 
    msg+="!!";   
    msg.toCharArray(bu,100);
  }
  http.end();
  return payload;
}
//End OTA


long smooth() { /* function smooth */
  long average;
  total = total - readings[readIndex];
  readings[readIndex] = analogRead(analogPin);
  total = total + readings[readIndex];
  readIndex = readIndex + 1;
  if (readIndex >= numReadings) {
    readIndex = 0;
  }
  average = total / numReadings;
  return average;
}

void drawText()
{
  //workaround for custom fonts:
  dma_display->fillScreen(dma_display->color444(0, 0, 0)); //custom fonts does'nt use the second color
  //date
  dma_display->setTextSize(1);     // size 1 == 8 pixels high
  dma_display->setTextWrap(false); // Don't wrap at end of line - will do ourselves
  dma_display->setTextColor(dma_display->color444(15,9,0),dma_display->color444(0,0,0)); // orange,schwarz
  dma_display->setFont(&DejaVuSans9pt7b);
  dma_display->setCursor(0, 7);    
  struct tm timeinfo;
  char dateStringBuff[11]; //11 chars should be enough
  char lumin[7];
  if(!getLocalTime(&timeinfo))
  {
    Serial.println("Failed to obtain time");
    sprintf(dateStringBuff, "- - -");
   //return;
  }
  else{strftime(dateStringBuff, sizeof(dateStringBuff), "%a %d %b", &timeinfo);}
  // # remove ! from the above if comperator and uncomment the following 2 lines to display 
  // # the brightness measurments instead of the date, see map() funktion in loop:
  //else{itoa(analogAvg, lumin, 10);}
  //dma_display->println(lumin);
  dma_display->println(dateStringBuff);
  
  int delta = 14;

  //Time
  dma_display->setFont(&FreeSans10pt7b);
  dma_display->setTextColor(dma_display->color444(15,15,15),dma_display->color444(0,0,0));
  dma_display->setCursor(3, 8+delta);    // start at top left, with 8 pixel of spacing
  char timeStringBuff[9]; //50 chars should be enough
  strftime(timeStringBuff, sizeof(timeStringBuff), "%H:%M", &timeinfo);
  //dma_display->print(" ");
  dma_display->println(timeStringBuff);
  dma_display->setTextSize(1);     // size 1 == 8 pixels high (buildin font)
//seconds:
  dma_display->setFont(&DejaVuSans9pt7b);
  dma_display->setTextColor(dma_display->color444(9,9,9),dma_display->color444(0,0,0));
  dma_display->setCursor(52, 23); 
  char secStringBuff[3]; //3 chars should be enough
  strftime(secStringBuff, sizeof(secStringBuff), "%S", &timeinfo);
  dma_display->println(secStringBuff);  
  dma_display->setFont();
  
  //temperature 
  delta = 8;
  dma_display->setFont(&DejaVuSans9pt7b);
  dma_display->setTextColor(dma_display->color444(10,6,0)); // orange, little darker
  dma_display->setCursor(0, 24+delta);   
  String aString("A");
  dma_display->print(aString);
  aString = ":";
  dma_display->setCursor(5, 24+delta);
  dma_display->print(aString); 
  dma_display->setTextColor(dma_display->color444(15,9,0),dma_display->color444(0,0,0)); // orange,black
  char bufTA [10];
  //convert char buffer to float, round to one decimal and convert back to char buffer
  if(!A_msgArrived)
  {
    //no MQTT connection
    sprintf(bufTA, "- - -");
  }
  else{dtostrf (atof(A_message), 3, 1, bufTA);}
  dma_display->setCursor(9, 24+delta);
  dma_display->print(bufTA);
  dma_display->setCursor(35, 24+delta);
  dma_display->setTextColor(dma_display->color444(10,6,0)); // orange, little darker
  aString = "I";
  dma_display->print(aString);
  aString = ":";
  dma_display->setCursor(36, 24+delta);
  dma_display->print(aString); 
  dma_display->setTextColor(dma_display->color444(15,9,0),dma_display->color444(0,0,0)); // orange,black
  char bufTI [10];
  //convert char buffer to float, round to one decimal and convert back to char buffer
  if(!I_msgArrived)
  {
    //no MQTT connection
    sprintf(bufTI, "- - -");
  }
  else{dtostrf (atof(I_message), 3, 1, bufTI);}
  dma_display->setCursor(40, 24+delta);
  dma_display->print(bufTI);
  dma_display->setFont();
}

void setTimezone(String timezone){
  Serial.printf("  Setting Timezone to %s\n",timezone.c_str());
  setenv("TZ",timezone.c_str(),1);  //  Now adjust the TZ.  Clock settings are adjusted to show the new local time
  tzset();
}

void initTime(String timezone){
  struct tm timeinfo;

  Serial.println("Setting up time");
  configTime(0, 0, "pool.ntp.org");    // First connect to NTP server, with 0 TZ offset
  if(!getLocalTime(&timeinfo)){
    Serial.println("  Failed to obtain time");
    return;
  }
  Serial.println("  Got the time from NTP");
  // Now we can set the real timezone
  setTimezone(timezone);
}

void printLocalTime(){
  struct tm timeinfo;
  if(!getLocalTime(&timeinfo)){
    Serial.println("Failed to obtain time 1");
    return;
  }
  Serial.println(&timeinfo, "%A, %B %d %Y %H:%M:%S");
}

void  startWifi(){
  WiFi.setHostname(host);
  WiFi.begin(ssid, wifipw);
  Serial.println("Connecting Wifi");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(500);
  }
  Serial.print("Wifi RSSI=");
  Serial.println(WiFi.RSSI());
  Serial.print("Wifi IP=");
  Serial.println(WiFi.localIP());

}

void setTime(int yr, int month, int mday, int hr, int minute, int sec, int isDst){
  struct tm tm;

  tm.tm_year = yr - 1900;   // Set date
  tm.tm_mon = month-1;
  tm.tm_mday = mday;
  tm.tm_hour = hr;      // Set time
  tm.tm_min = minute;
  tm.tm_sec = sec;
  tm.tm_isdst = isDst;  // 1 or 0
  time_t t = mktime(&tm);
  Serial.printf("Setting time: %s", asctime(&tm));
  struct timeval now = { .tv_sec = t };
  settimeofday(&now, NULL);
}

void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  if (strcmp(topic,mqttTopic_A)==0){
    A_msgArrived = true;
    Serial.print("]\n ");
    int i=0;
    for (i;i<length;i++) {
      A_message[i]=char(payload[i]);
    }
    A_message[i]='\0';
  }
  if (strcmp(topic,mqttTopic_I)==0) {
    I_msgArrived = true;
    Serial.print("]\n ");
    int i=0;
    for (i;i<length;i++) {
      I_message[i]=char(payload[i]);
    }
    I_message[i]='\0';
  }  
}
WiFiClient espClient;
PubSubClient mqttClient(espClient);

void reconnect() {
  // Loop until we're reconnected
  while (!mqttClient.connected()) {
    WiFi.begin(ssid, wifipw); 
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    delay(100);
    //if (mqttClient.connect(clientId.c_str()))  {
    if (mqttClient.connect(host,mqttUser,mqttPass))  {
      Serial.println("connected");
      // Once connected, publish an announcement...
      // ... and resubscribe
      
      mqttClient.subscribe(mqttTopic_A);
      mqttClient.subscribe(mqttTopic_I);
    } else {

        ResetCounter++;
        Serial.println("failed");
        Serial.print("MQTT state: ");
        Serial.println(mqttClient.state());
        Serial.println(" try again in 5 seconds");
 
        Serial.println(ResetCounter);
        drawText();
        if (ResetCounter >=5)
        {
            //ESP.restart();
            mqttClient.disconnect();
            startWifi();
            mqttClient.setServer(mqtt_server, 1883); 
            mqttClient.setCallback(callback);
            if (mqttClient.connect(host,mqttUser,mqttPass)) {
              // connection succeeded
              Serial.println("Connected ");
              boolean ra= mqttClient.subscribe(mqttTopic_A);
              boolean rb= mqttClient.subscribe(mqttTopic_I);    
              Serial.println("subscribe ");
              if(ra)Serial.println(mqttTopic_A);
              if(rb)Serial.println(mqttTopic_I);
              Serial.print("MQTT state: ");
              Serial.println(mqttClient.state());

            } 
            else {
              ESP.restart();
            }
             
            ResetCounter =0;
        }
        
        // Wait 5 seconds before retrying (update display between)
        delay(1000);
        drawText();
        delay(1000);
        drawText();
        delay(1000);
        drawText();
        delay(1000);
        drawText();
        delay(1000);
        drawText();
      }
   }
}



void setup() {
  HUB75_I2S_CFG::i2s_pins _pins={R1, G1, BL1, R2, G2, BL2, CH_A, CH_B, CH_C, CH_D, CH_E, LAT, OE, CLK};
  HUB75_I2S_CFG mxconfig(
                          64,   // width
                          32,   // height
                           1,   // chain length
                       _pins   // pin mapping
  
  );

  // Display Setup
  dma_display = new MatrixPanel_I2S_DMA(mxconfig);
  dma_display->begin();
  dma_display->clearScreen();
  dma_display->setBrightness8(30); //0-255
  // fill the screen with 'black'
  dma_display->fillScreen(dma_display->color444(0, 0, 0));
  Serial.begin(115200);
  Serial.setDebugOutput(true);
  dma_display->setTextColor(dma_display->color444(15,15,15),dma_display->color444(0,0,0)); // white, black
  dma_display->setCursor(5, 0);
  String wString("WiFi ...");
  dma_display->print(wString);
  startWifi();
  delay(1000);
  dma_display->setCursor(5, 12);
  String tString("Time ...");
  dma_display->print(tString);
  initTime("CET-1CEST,M3.5.0,M10.5.0/3");   // Set for Europe/Berlin
  printLocalTime();
  delay(1000);
  dma_display->setCursor(5, 24);
  String mString("MQTT ...");
  dma_display->print(mString);
  delay(1000);
  mqttClient.setServer(mqtt_server, 1883);
  mqttClient.setCallback(callback);
  if (mqttClient.connect(host,mqttUser,mqttPass)) {
    // connection succeeded
    Serial.println("Connected ");
    boolean ra= mqttClient.subscribe(mqttTopic_A);
    boolean rb= mqttClient.subscribe(mqttTopic_I);    
    Serial.println("subscribe ");
    if(ra)Serial.println(mqttTopic_A);
    if(rb)Serial.println(mqttTopic_I);
  } 
  else {
    // connection failed
    // mqttClient.state() will provide more information
    // on why it failed.
    Serial.println("Connection failed ");
  }
  dma_display->fillScreen(dma_display->color444(0, 0, 0)); //clearscreen
  //light sensor (LDR) init
  pinMode(analogPin, INPUT);
  //beginOTA
  Serial.printf("beginOTA setup\n");
  /*use mdns for host name resolution*/
    if (!MDNS.begin(host)) { //http://host.local
      Serial.println("Error setting up MDNS responder!");
      while (1) {
        delay(1000);
      }
    }
    Serial.println("mDNS responder started");
    /*return index page which is stored in serverIndex */
    server.on("/", HTTP_GET, []() {
      server.sendHeader("Connection", "close");
      server.send(200, "text/html", loginIndex);
    });
    server.on("/serverIndex", HTTP_GET, []() {
      server.sendHeader("Connection", "close");
      server.send(200, "text/html", serverIndex);
    });
    /*handling uploading firmware file */
    server.on("/update", HTTP_POST, []() {
      server.sendHeader("Connection", "close");
      server.send(200, "text/plain", (Update.hasError()) ? "FAIL" : "OK");
      ESP.restart();
    }, []() {
      HTTPUpload& upload = server.upload();
      if (upload.status == UPLOAD_FILE_START) {
        Serial.printf("Update: %s\n", upload.filename.c_str());
        if (!Update.begin(UPDATE_SIZE_UNKNOWN)) { //start with max available size
          Update.printError(Serial);
        }
      } else if (upload.status == UPLOAD_FILE_WRITE) {
        /* flashing firmware to ESP*/
        if (Update.write(upload.buf, upload.currentSize) != upload.currentSize) {
          Update.printError(Serial);
        }
      } else if (upload.status == UPLOAD_FILE_END) {
        if (Update.end(true)) { //true to set the size to the current progress
          Serial.printf("Update Success: %u\nRebooting...\n", upload.totalSize);
        } else {
          Update.printError(Serial);
        }
      }
    });
    server.begin();
    Serial.printf("starting OTA web server\n");
    //endOTA  
}



void loop() {

  //begin OTA
    server.handleClient();
    delay(1);
  //end OTA

    mqttClient.loop();
  analogAvg = smooth();
  // map ambient light (analog) value to display brightness:
  mappedAvg = map(analogAvg,4200,1100,15,120);
  dma_display->setBrightness8(mappedAvg); //0-255

  //display routine
  if (!mqttClient.connected()) {  
    A_msgArrived = false;
    I_msgArrived = false;
    drawText();
    Serial.print("MQTT state: ");
    Serial.println(mqttClient.state());
    reconnect();  
  }
  drawText();
  delay(20); 

  
}
