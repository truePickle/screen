#include <WiFi.h>
#include <string> 
#include "SPI.h"
#include "TFT_eSPI.h"
#include <HTTPClient.h>
#include "cJSON.h"
#include <PNGdec.h>

int screenWidth = 240;
int screenHeight = 320;
TFT_eSPI tft = TFT_eSPI();
PNG png;

String password = "ejlh7298";
String ssid = "Brockphone";
WiFiClient client;
HTTPClient http;
String serverPath ="http://spotify.noah.nat.selfnet.de";
int linecount =0;

const int buttonPin = 17;
volatile bool buttonLongPressed = false;
volatile bool buttonShortPressed = false;
const unsigned long shortPressMax = 1000;// 1s


struct response{
  int code;
  String payload;
};

void displayText(String msg){
  // Setting text o screen
  tft.setCursor(0, 0);
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_WHITE);  tft.setTextSize(2);
  tft.println(msg);
}

void pngDraw(PNGDRAW *pDraw) {
    uint16_t lineBuffer[screenWidth];
    png.getLineAsRGB565(pDraw, lineBuffer, PNG_RGB565_BIG_ENDIAN, 0xffffffff);
    tft.pushImage(0, pDraw->y, pDraw->iWidth, 1, lineBuffer);
}/* PNGDraw() */

void displayImage(uint8_t *png_data, int length){
  //reset screen to black
  tft.fillScreen(TFT_BLACK);
  int16_t rc = png.openRAM((uint8_t *)png_data, sizeof(uint8_t) * length, pngDraw);
  if (rc == PNG_SUCCESS) {
    tft.startWrite();
    rc = png.decode(NULL, 0);
    tft.endWrite();
  }
  else{
    displayText("Error Loading RAM :");
    Serial.println(rc);
  }
  return;
}


response sendGETRequest(const String &path){
  //Check if Wifi connected
  if(WiFi.status()!= WL_CONNECTED){
    return response{1,"Wifi not connected"};
  }
  // get server path
  Serial.println(path);
  http.begin(path);
  int responseCode = http.GET();
  String payload = http.getString();
  Serial.println("Got payload");
  //client errors wont return response
  if(responseCode < 0){
    Serial.printf("My number is: %d\n", responseCode);
    return response{1,"Http Client Error"};
  }
  else if(responseCode == 500){
    // Internal Server Error
    Serial.println("Parsing Error JSON Code:");
    Serial.println(responseCode);
    Serial.println("Payload:");
    Serial.println(payload);
    const cJSON* root = cJSON_Parse(payload.c_str());
    const int cd = cJSON_GetObjectItem(root, "code")->valueint;
    const String msg = cJSON_GetObjectItem(root, "message")->valuestring;
    Serial.println("Got error");
    return response{cd, msg};
  }
  else if (responseCode != 200){
    // other http error codes
    return response{responseCode,payload};

  }
  Serial.println("Got image");
  return response{0, payload};
}

int displayLogin(){
  Serial.println("Getting Code");
  if(WiFi.status()==WL_CONNECTED){
    Serial.println("Wifi is connected");
    const String path = serverPath +"/code";
    response res = sendGETRequest(path);
    Serial.println("Finished /code Request");

    if(res.code !=0){
      displayText(res.payload);
      return res.code;
    }
    //Cast string to uint and the draw array

    std::vector<uint8_t> buffer(res.payload.begin(), res.payload.end());
    uint8_t* pic = buffer.data();
    
    displayImage(pic, res.payload.length());
    return 0;
  }
  return 1;
}

int displayPlaying(){
  Serial.println("Getting Playing");
  if(WiFi.status()==WL_CONNECTED){
    Serial.println("Wifi is connected");
    const String path = serverPath +"/playing";
    response res = sendGETRequest(path);
    Serial.println("Finished /playing Request");

    if(res.code !=0){
      displayText(res.payload);
      return res.code;
    }
    //Cast string to uint and the draw array
    Serial.printf("Length of payload: %d\n", res.payload.length());
    std::vector<uint8_t> buffer(res.payload.begin(), res.payload.end());
    uint8_t* pic = buffer.data();
    
    displayImage(pic, res.payload.length());
    return 0;
  }
  return 1;
}

int displayJam(){
  Serial.println("Getting Jam");
  if(WiFi.status()==WL_CONNECTED){
    Serial.println("Wifi is connected");
    const String path = serverPath +"/jam";
    response res = sendGETRequest(path);
    Serial.println("Finished /jam Request");

    if(res.code !=0){
      displayText(res.payload);
      return res.code;
    }
    //Cast string to uint and the draw array

    std::vector<uint8_t> buffer(res.payload.begin(), res.payload.end());
    uint8_t* pic = buffer.data();
    
    displayImage(pic, res.payload.length());
    return 0;
  }
  return 1;
}

//Button Handling
void buttonISR() {
  int buttonState = digitalRead(buttonPin);
  int pressStartTime = millis();
  while(digitalRead(buttonPin) == LOW){
    //wait until button is stop being pressed+
    if(millis() - pressStartTime>shortPressMax){
      //Button was pressed longer than shortPressMax
      buttonLongPressed = true;
      return;
    }
  }
  if(millis() - pressStartTime< 10){
    //Press was to short so doesnt count
    return;
  }
  //Otherwise short press
  buttonShortPressed = true;
  return;
}

void setup() {
  Serial.begin(9600);
  while (!Serial);

  Serial.println(""); Serial.println("");
  Serial.println("Starting Application");
  //Screen init
  tft.init();
  tft.fillScreen(TFT_BLACK);
  tft.setRotation(2);
  tft.setCursor(0, 0);
  tft.setTextColor(TFT_WHITE);  tft.setTextSize(2);

  // Wifi init
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  tft.println("Welcome to the \n Spotify Deco Screen\n Trying to Connect to WiFi");
  while (WiFi.status()!=WL_CONNECTED){
    Serial.print(".");
    delay(5000);
  }
  Serial.println("IP:"); Serial.println(WiFi.localIP());

  pinMode(buttonPin, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(buttonPin), buttonISR, FALLING);

  displayLogin();
  delay(10000);
}

void loop() {
  Serial.println("Looping");
  if (buttonLongPressed) {
    Serial.println("Long Button Press detected");
    displayLogin();
    buttonLongPressed = false;
    delay(10000);
  }
  else if(buttonShortPressed){
    Serial.println("Short Button Press detected");
    displayJam();
    buttonShortPressed = false;
    delay(10000);
  }
  displayPlaying();
  delay(5000);
}