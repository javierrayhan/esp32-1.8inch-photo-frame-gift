#include <Arduino.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7735.h>
#include <SPI.h>
#include <Wire.h>
#include <SD.h>
#include <WiFi.h>
#include <WebServer.h>
#include <EEPROM.h>
#include <NTPClient.h>
#include <HTTPClient.h>
#include <WiFiUdp.h>
#include "qrcode.h"
#include <FastLED.h>
#include <esp_sleep.h>

//USERNAME ID
const char* name = "USERNAME HERE";

//CLOCK 
uint8_t time_set = 25200; //Your country time based UTC

//Color
#define RGB565(r, g, b) (((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3))
#define LIGHT_GRAY RGB565(192, 192, 192)
#define MEDIUM_GRAY RGB565(128, 128, 128)
#define DARK_GRAY RGB565(64, 64, 64)

//font
// #include <Fonts/Font5x7Fixed.h> // Include modern font

//SET DEBUG
#define DEBUG 1
//Debugging state
#if DEBUG == 1
#define debug(x) Serial.print(x)
#define debugln(x) Serial.println(x)
#else
#define debug(x) 
#define debugln(x) 
#endif



//function declare
void setBrightness(int brightness);
void tftlcd(int set_rot, int size_text, int scx, int scy, String text_tft, int erase);
void displayBMP(const char *filename, int16_t x, int16_t y);
bool readBMPHeader(File &bmpFile, int &bmpWidth, int &bmpHeight, uint8_t &bmpDepth, uint32_t &bmpImageOffset);
void sdcardcheck();

//Web server and Wifi connection
void saveCredentials(const String &ssid, const String &password);
void loadCredentials();
void setupWiFi();
void handleRoot();
void handleSave();
void wifi_message();

void restarton();
void displayTime(String hour, String minute);
void displayWiFiPrompt(String ssid, String ipaddress);
void displayQRCode(const char *data, const int QR_SIZE, const int PADDING, const int TOP_PADDING ,  const int BOTTOM_PADDING);

//DATE
void gettime();
void getDate();
void getTimeDate();

//MENU
void handleButtonNavigation();
void showMenu();
void updateButtonStates();
void resetArray(int indexToSet);

//BRIGHNESS
void brighnessControll();
void drawBrighnessUI();

//SOUND
void soundCheck();
void soundsrawUI();
void drawOption(int x, int y, const char* text, bool selected);

//DOWNBAR
void downbar();

//ST7735
#define TFT_CS     33
#define TFT_RST    14   
#define TFT_DC     26  

//SD CARD
#define SD_CS      5

#define pinLED    0
#define chPWM     0
#define resPWM    8
#define freqPWM   2000

uint16_t dutyCycle = 0;

//EEPROM WIFI
#define EEPROM_SIZE 128
#define SSID_ADDRESS 0
#define PASSWORD_ADDRESS 32

WebServer server(80);

String ssid = "";
String password = "";

//ST7735
Adafruit_ST7735 tft = Adafruit_ST7735(TFT_CS, TFT_DC, TFT_RST);

const int buttonPin1 = 15; 
const int buttonPin2 = 13; 
const int buttonPin3 = 27; 
const int buzzer = 12;

int buttonState1 = HIGH;   
int buttonState2 = HIGH;
int buttonState3 = HIGH;

int lastButtonState1 = HIGH;
int lastButtonState2 = HIGH;  
int lastButtonState3 = HIGH;  

uint8_t page_status = 0;
uint8_t time_status = 0;
uint8_t blank = 1;
uint8_t blanktime = 1;
bool startstat = 1;
uint8_t time_downbar = 0;

//restart
unsigned long pressStartTime = 0; 

// Variabel status menu
uint8_t statusMenu = 5;
uint8_t inMenu = 0; 
uint8_t showmenu = 1;

//BRIGHNESS 
uint8_t brightnessValue = 60;
uint8_t statusBright = 1;

//status feature
int feature_array_stat[5] = {0, 0, 0, 0, 0};

//Sound state
uint8_t selectedOption = 0;
bool soundUI = true;

//LED
CRGB leds[1];

//Sleep
unsigned long startSleep = 0; 
uint8_t sleep_status;
// Define NTP Client to get time
const char* worldTimeAPI = "http://worldtimeapi.org/api/timezone/Asia/Jakarta";

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP);

unsigned long previousMillis = 0;
const long interval = 2000; // Interval 2 sec(2000 ms)


// Variables to save date and time
String formattedDate;
String dayStamp;
String timeStamp;

// Variables for current hour and minute
uint8_t  currentHour;
uint8_t currentMinute;

String str_currentMinute;
String str_currentHour;
String str_currentDate; 
String str_currentDay;   
String str_currentMonth;
String str_currentYear; 

unsigned long previousMillisTime = 0; 
const unsigned long intervalTime = 1500; 

unsigned long previousMillisDate = 0; 
const unsigned long intervalDate = 5000; 

unsigned long previousMillisDebugTime = 0; 
const unsigned long intervalDebugTime = 1500; 


String filenames[] = {"/1.bmp", "/2.bmp", "/3.bmp", "/4.bmp", "/5.bmp", "/6.bmp", "/7.bmp", "/8.bmp", "/9.bmp", "/10.bmp", "/11.bmp"};
const int fileCount = sizeof(filenames) / sizeof(filenames[0]);

void setup() {
  FastLED.addLeds<WS2812B, 25, GRB>(leds, 1);  
  FastLED.setBrightness(100);  
 

  pinMode(buttonPin1, INPUT_PULLUP); 
  pinMode(buttonPin2, INPUT_PULLUP); 
  pinMode(buttonPin3, INPUT_PULLUP); 
  // pinMode(buzzer, OUTPUT); 

  ledcSetup(1, 5000, 8);
  ledcAttachPin(buzzer, 1);
  int dutyCycle = (int)(0.900 * 255); 

  ledcAttachPin(pinLED, chPWM);
  ledcSetup(chPWM, freqPWM, resPWM);

  Serial.begin(115200);

  //Wire Begin
  Wire.begin();

  tft.initR(INITR_BLACKTAB);  
  tft.setRotation(2);        
  tft.fillScreen(ST77XX_BLACK);

  //EEPROM
  EEPROM.begin(EEPROM_SIZE); 
  
  sdcardcheck();

  // Config pin LED PWM
  ledcAttachPin(pinLED, chPWM);
  ledcSetup(chPWM, freqPWM, resPWM);

  leds[0] = CRGB::Blue; FastLED.show(); 

 // Set brigness 50%
  setBrightness(60); 

  // Show BMP
  displayBMP("/0BOOT.bmp", 0, 0);
  
  uint8_t sounds;
  EEPROM.get(64, sounds);

  if (sounds == 1) {
  // digitalWrite(buzzer, HIGH);
  ledcWrite(1, dutyCycle); leds[0] = CRGB::Cyan; FastLED.show(); 
  delay(100);
  // digitalWrite(buzzer, LOW);
  ledcWrite(1, 0); leds[0] = CRGB::Black; FastLED.show(); 
  delay(100);
  // digitalWrite(buzzer, HIGH);
  ledcWrite(1, dutyCycle); leds[0] = CRGB::Cyan; FastLED.show(); 
  delay(100);
  // digitalWrite(buzzer, LOW);
  ledcWrite(1, 0); leds[0] = CRGB::Black; FastLED.show();  
  delay(100);
  } 

  leds[0] = CRGB::Blue; FastLED.show(); 
  delay(200);
  //Wifi SetUp


  loadCredentials();

  if (ssid.length() > 0 && password.length() > 0) {
    setupWiFi();
    timeClient.begin();
    timeClient.setTimeOffset(time_set;); // GMT +7 (Indonesia Western Time)
  } else {
    WiFi.softAP("Ext Innovation");
    debugln("Started AP mode. IP Address: " + WiFi.softAPIP().toString());
  } 
  
  server.on("/", handleRoot);
  server.on("/save", HTTP_POST, handleSave);
  server.begin();

  debugln("HTTP server started");
  tft.fillScreen(ST77XX_BLACK);
}

void loop() {
  // tft.setFont(&Font5x7Fixed);
  buttonState1 = digitalRead(buttonPin1);
  buttonState2 = digitalRead(buttonPin2);
  buttonState3 = digitalRead(buttonPin3);
  
  //Sleep Mode
  if (buttonState1 == HIGH && buttonState2 == HIGH && buttonState3 == HIGH) {
    if (millis() - startSleep >= 60000 && sleep_status == 0) {
      esp_sleep_enable_ext0_wakeup(GPIO_NUM_15, 0); 

      debugln("Enter light sleep...");
      setBrightness(0);
      leds[0] = CRGB::Black; FastLED.show(); 
      tft.fillScreen(ST77XX_BLACK);
      esp_light_sleep_start();
    }
  } else {
    startSleep = millis();
    setBrightness(brightnessValue);
  }

  //Enter Menu
  if (buttonState3 == LOW && lastButtonState3 == HIGH && inMenu == 0) {
    inMenu = 1;
    debug(inMenu);
  }
  
  if(inMenu == 1){
    handleButtonNavigation();
  }
  
  //back
  if(statusMenu == 5 && feature_array_stat[4] == 1 && inMenu == 1){
    startstat = 1;
    time_downbar = 0;
    page_status = 0;
  }

  if(inMenu == 0 && sleep_status == 0){
    if(statusMenu == 5){

      if (startstat == 1){
        displayBMP(filenames[0].c_str(), 0, 0);

        startstat = 0;
        time_status = 0;
        downbar();
      }
        
      if (buttonState1 == LOW && lastButtonState1 == HIGH) { 
        page_status += 1;  
        tft.fillScreen(ST77XX_BLACK);
        if (page_status >= 10) { 
          page_status = 0;
        }

        displayBMP(filenames[page_status].c_str(), 0, 0);

        time_status = 0;
        downbar();
      }

      if (buttonState2 == LOW && lastButtonState2 == HIGH) { 
        page_status -= 1;  
        tft.fillScreen(ST77XX_BLACK);
        if (page_status <= 0) { 
          page_status = 0;
        }

        displayBMP(filenames[page_status].c_str(), 0, 0);

        time_status = 0;
        downbar();
      } 
    }

    if (statusMenu == 1 && feature_array_stat[0] == 1){
      if (blanktime == 1){
        tft.fillRect(0, 0, 128, 140, ST7735_BLACK);
        blanktime = 0;
      }
      wifi_message();
      if (blank == 1){
        unsigned long currentMillisTime = millis(); 
        if (currentMillisTime - previousMillisTime >= intervalTime) {
          previousMillisTime = currentMillisTime; 
          displayTime(str_currentHour, str_currentMinute);
          time_downbar = 1;
          downbar();
        }
        getTimeDate();
      }
    } else {
      blanktime = 1;
    }

    if(statusMenu == 2 && feature_array_stat[1] == 1){
      soundCheck();
    } 

    if(statusMenu == 3 && feature_array_stat[2] == 1){
      brighnessControll();
    }

    if(statusMenu == 4 && feature_array_stat[3] == 1){
      leds[0] = CRGB::Black;  
      FastLED.show(); 
      ESP.restart();
    }

  }



  // tftlcd(2, 1, 0, 0, messages[page_status], 0); 

  //Debug Button
  // debug("\n");
  // debug(buttonState1);
  // debug("\n");
  // debug(buttonState2);
  // debug("\n");
  // debug(buttonState3);
  // debug("\n");

  lastButtonState1 = buttonState1;
  lastButtonState2 = buttonState2;
  lastButtonState3 = buttonState3;

  server.handleClient();
  blank = 1;

}

// Fungsi untuk mengatur kecerahan
void setBrightness(int brightness) {
   // Brigness only 0-100
  if (brightness < 0) {
    brightness = 0;
  } else if (brightness > 100) {
    brightness = 100;
  }

  // Count PWM
  int per_bright = (int)(255 * (brightness / 100.0)); // Gunakan 100.0 untuk pembagian float
  ledcWrite(chPWM, per_bright); 
}


void tftlcd(int set_rot, int size_text, int scx, int scy, String text_tft, int erase){
  if (erase == 1){
    tft.fillScreen(ST77XX_BLACK);
  }
  tft.setRotation(set_rot);         // Atur orientasi layar
  tft.setTextColor(ST77XX_BLACK);
  tft.setTextSize(size_text);
  tft.setCursor(scx, scy);
  tft.println(text_tft);
}

// Read SD
void displayBMP(const char *filename, int16_t x, int16_t y) {
  File bmpFile;
  int bmpWidth, bmpHeight;
  uint8_t bmpDepth;
  uint32_t bmpImageOffset;

  bmpFile = SD.open(filename);
  if (!bmpFile) {
    debug("File Not Found: ");
    debugln(filename);
    return;
  }

  // Read header BMP
  if (readBMPHeader(bmpFile, bmpWidth, bmpHeight, bmpDepth, bmpImageOffset)) {
    if (bmpDepth == 24) {
      bmpFile.seek(bmpImageOffset);

      uint16_t pixel;
      for (int i = 0; i < bmpHeight; i++) {
        for (int j = 0; j < bmpWidth; j++) {
          uint8_t b = bmpFile.read();
          uint8_t g = bmpFile.read();
          uint8_t r = bmpFile.read();
          pixel = tft.color565(r, g, b);
          tft.drawPixel(x + j, y + (bmpHeight - 1 - i), pixel);
        }
      }
    }
  }
  bmpFile.close();
}

//BMP READ
bool readBMPHeader(File &bmpFile, int &bmpWidth, int &bmpHeight, uint8_t &bmpDepth, uint32_t &bmpImageOffset) {
  if (bmpFile.read() != 'B' || bmpFile.read() != 'M') {
    debugln("File BMP tidak valid.");
    return false;
  }

  bmpFile.seek(10);
  bmpImageOffset = bmpFile.read() | (bmpFile.read() << 8) | (bmpFile.read() << 16) | (bmpFile.read() << 24);

  bmpFile.seek(18);
  bmpWidth = bmpFile.read() | (bmpFile.read() << 8) | (bmpFile.read() << 16) | (bmpFile.read() << 24);

  bmpHeight = bmpFile.read() | (bmpFile.read() << 8) | (bmpFile.read() << 16) | (bmpFile.read() << 24);

  bmpFile.seek(28);
  bmpDepth = bmpFile.read();

  debug("BMP Width: "); debugln(bmpWidth);
  debug("BMP Height: "); debugln(bmpHeight);
  debug("BMP Depth: "); debugln(bmpDepth);

  return true;
}

void sdcardcheck(){
  if (!SD.begin(SD_CS)) {
    debugln("SD Card can not be found!");
    return;
  }
}

//WIFI CONFIG EEPROM
void saveCredentials(const String &ssid, const String &password) {
  EEPROM.writeString(SSID_ADDRESS, ssid);
  EEPROM.writeString(PASSWORD_ADDRESS, password);
  EEPROM.commit();
}

void loadCredentials() {
  ssid = EEPROM.readString(SSID_ADDRESS);
  password = EEPROM.readString(PASSWORD_ADDRESS);
}

void setupWiFi() {
  WiFi.begin(ssid.c_str(), password.c_str());
  debug("Connecting to WiFi");
  int retries = 10;
  while (WiFi.status() != WL_CONNECTED && retries-- > 0) {
    delay(500);
    debug(".");
  }
  if (WiFi.status() == WL_CONNECTED) {
    debugln("\nConnected to WiFi!");
    debugln("IP Address: " + WiFi.localIP().toString());

    leds[0] = CRGB::Green; 
    FastLED.show();  
  } else {
    debugln("\nFailed to connect to WiFi.");
    WiFi.softAP("Ext Innovation");
    debugln("Started AP mode. IP Address: " + WiFi.softAPIP().toString());

    leds[0] = CRGB::Red; 
    FastLED.show();  
  }
}

void handleRoot() {
  String page = R"rawliteral(
  <!DOCTYPE html>
  <html lang="en">
  <head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Connect to WiFi</title>
    <style>
      body {
        font-family: Arial, sans-serif;
        background-color: #121212;
        color: #f4f4f4;
        margin: 0;
        padding: 0;
        display: flex;
        justify-content: center;
        align-items: center;
        min-height: 100vh;
      }
      .container {
        width: 90%;
        max-width: 400px;
        background: #1e1e1e;
        padding: 20px;
        box-shadow: 0 4px 15px rgba(0, 0, 0, 0.3);
        border-radius: 10px;
      }
      h1 {
        font-size: 22px;
        margin-bottom: 20px;
        color: #C6E7FF;
      }
      form {
        display: flex;
        flex-direction: column;
      }
      label {
        text-align: left;
        margin-bottom: 8px;
        font-size: 14px;
        color: #ccc;
      }
      input[type="text"],
      input[type="password"] {
        padding: 12px;
        margin-bottom: 15px;
        border: 1px solid #333;
        border-radius: 6px;
        font-size: 14px;
        background-color: #2b2b2b;
        color: #f4f4f4;
      }
      input[type="text"]::placeholder,
      input[type="password"]::placeholder {
        color: #888;
      }
      input[type="submit"] {
        padding: 12px;
        background-color: #C6E7FF;;
        color: #fff;
        border: none;
        border-radius: 6px;
        font-size: 16px;
        cursor: pointer;
        transition: background-color 0.3s ease;
      }
      input[type="submit"]:hover {
        background-color: #C6E7FF;
      }
      .footer {
        font-size: 12px;
        margin-top: 20px;
        color: #888;
        text-align: center;
      }
      @media (max-width: 768px) {
        h1 {
          font-size: 20px;
        }
        input[type="text"],
        input[type="password"],
        input[type="submit"] {
          font-size: 14px;
          padding: 10px;
        }
      }
    </style>
  </head>
  <body>
    <div class="container">
      <h1>Connect to WiFi (EXT INNOVATION)</h1>
      <form action="/save" method="post">
        <label for="ssid">WiFi Name (SSID):</label>
        <input type="text" id="ssid" name="ssid" placeholder="Enter WiFi Name" required>

        <label for="password">WiFi Password:</label>
        <input type="password" id="password" name="password" placeholder="Enter WiFi Password" required>

        <input type="submit" value="Save">
      </form>
      <div class="footer">
        <p>Powered by ESP32 | &copy; 2024 EXT INNOVATION</p>
      </div>
    </div>
  </body>
  </html>
  )rawliteral";
  server.send(200, "text/html", page);
}

void handleSave() {
  if (server.hasArg("ssid") && server.hasArg("password")) {
    ssid = server.arg("ssid");
    password = server.arg("password");
    saveCredentials(ssid, password);

    String message = "SSID and Password saved! Restarting device...";
    server.send(200, "text/plain", message);

    delay(2000);
    ESP.restart();
  } else {
    server.send(400, "text/plain", "Invalid input");
  }
}

//GET TIME 
void gettime(){
  //With NTP
  
  timeClient.forceUpdate();
  formattedDate = timeClient.getFormattedTime();
  debugln(formattedDate);

  int splitT_time = formattedDate.indexOf("T");
  dayStamp = formattedDate.substring(0, splitT_time );

  // Extract time
  timeStamp = formattedDate.substring(splitT_time  + 1, formattedDate.length() - 1);
  // debug("HOUR: ");
  // debugln(timeStamp);

  // Extract hour and minute
  int colonIndex = timeStamp.indexOf(":");
  currentHour = timeStamp.substring(0, colonIndex).toInt();
  currentMinute = timeStamp.substring(colonIndex + 1, colonIndex + 3).toInt();
  

  //WithOUT NTP

  // HTTPClient http;
  // http.begin("http://worldtimeapi.org/api/timezone/Asia/Jakarta"); // API endpoint
  // int httpCode = http.GET(); // Mengirimkan GET request
  
  // if (httpCode == 200) { // Jika request berhasil
  //   String payload = http.getString(); // Menyimpan response JSON
    
  //   // Menemukan dan mengambil field "datetime"
  //   int dateTimeStart = payload.indexOf("\"datetime\":\"") + 12;
  //   int dateTimeEnd = payload.indexOf("\",", dateTimeStart);
  //   String datetime = payload.substring(dateTimeStart, dateTimeEnd); // Format "2024-12-02T12:34:56"
    
  //   // Memisahkan waktu (setelah huruf 'T')
  //   int splitT = datetime.indexOf("T");
  //   String timeStamp = datetime.substring(splitT + 1); // Waktu dalam format "HH:MM:SS"
    
  //   // Mengambil jam dan menit
  //   int colonIndex = timeStamp.indexOf(":");
  //   currentHour = timeStamp.substring(0, colonIndex).toInt();               // Jam
  //   currentMinute = timeStamp.substring(colonIndex + 1, colonIndex + 3).toInt(); 
  // } else {
  //   Serial.println("Gagal mengambil waktu. HTTP Code: " + String(httpCode));
  // }
  // http.end(); 
  
  str_currentHour = (currentHour < 10 ? "0" : "") + String(currentHour);
  str_currentMinute = (currentMinute < 10 ? "0" : "") + String(currentMinute);

}

//Wifi message
void wifi_message(){
  tft.setTextColor(ST77XX_WHITE); 
  if (WiFi.status() != WL_CONNECTED && blank == 1){
    WiFi.softAP("Ext Innovation");
    debugln("Started AP mode. IP Address: " + WiFi.softAPIP().toString());
  }
  while (WiFi.status() != WL_CONNECTED) {
    buttonState3 = digitalRead(buttonPin3);

    if (blank == 1){
      tft.fillScreen(ST77XX_BLACK);
    }

    if (buttonState3 == LOW){
      restarton();
    }    

    String ipaddress = WiFi.softAPIP().toString();
    tft.setCursor((tft.width() - 116) / 2, 5); 
    tft.print("Connect WiFi named:");

    tft.setCursor((tft.width() -88) / 2, 15); 
    tft.print("Ext Innovation");

    tft.setCursor((tft.width()-95) / 2, 25); 
    tft.print("Then, scan this:");

    delay(100);
    displayQRCode(ipaddress.c_str(), 3, 10, 50, 10);

    blank = 0;
    server.handleClient();
    
    lastButtonState3 = buttonState3;
  }

  if (WiFi.status() == WL_CONNECTED){
    if (blank == 0){
      tft.fillScreen(ST77XX_BLACK);
    } else {
      blank =1;
    }
  }


}

//Restart function
void restarton() {
  if (digitalRead(buttonPin3) == LOW) { 
    if (!buttonPressed) {
      pressStartTime = millis(); 
      buttonPressed = true;
    }

    if (millis() - pressStartTime >= 3000) {
      debugln("Button held for 3 seconds. Restarting...");
      leds[0] = CRGB::Black;  
      FastLED.show();  
      ESP.restart();
    }
  } else {
   
    buttonPressed = false;
  }

  // pressStartTime = millis(); // Catat waktu mulai ditekan
  // if (millis() - pressStartTime >= 3000) {
  //   debugln("Button held for 3 seconds. Restarting...");
  //   delay(100); // Sedikit delay untuk memastikan pesan terkirim
  //   ESP.restart();
  // }
}

//Clock Ui
void displayTime(String hour, String minute) {
  
  tft.fillRect(0, 0, 128, 160, ST77XX_BLACK);


  tft.fillRoundRect(10, 48, 108, 45, 15, ST77XX_WHITE); 

  tft.setTextColor(ST77XX_BLACK); 
  tft.setTextSize(3);             

  tft.setCursor(22, 60);           
  tft.print(hour);

  tft.setCursor(56, 60);           
  tft.print(":");

  tft.setCursor(72, 60);         
  tft.print(minute);

 
  tft.setTextSize(1);
  tft.setCursor(27, 130);           // Posisi untuk teks tambahan
  tft.print("Current Time");
} 

void displayQRCode(const char *data, const int QR_SIZE, const int PADDING, const int TOP_PADDING ,  const int BOTTOM_PADDING) {
  // const int QR_SIZE = 3; // Ukuran piksel setiap modul QR (lebih kecil dari sebelumnya)
  // const int PADDING = 10; // Margin di sekitar QR code
  // const int TOP_PADDING = 50; // Padding 100px di bagian atas (jarak antara atas dan QR code)
  // const int BOTTOM_PADDING = 10; // Padding lebih kecil di bagian bawah (ubah sesuai kebutuhan)

  QRCode qrcode;
  uint8_t qrcodeData[qrcode_getBufferSize(3)]; // Buffer untuk versi 3

  // Generate QR Code
  qrcode_initText(&qrcode, qrcodeData, 3, ECC_LOW, data);

  // Hitung posisi awal untuk memusatkan QR code
  int qrWidth = qrcode.size * QR_SIZE;
  int startX = (tft.width() - qrWidth) / 2;
  int startY = TOP_PADDING; // Mengatur padding atas menjadi 100px

  // Gambar QR Code pada layar
  for (int y = 0; y < qrcode.size; y++) {
    for (int x = 0; x < qrcode.size; x++) {
      // Tentukan warna piksel (hitam atau putih)
      uint16_t color = qrcode_getModule(&qrcode, x, y) ? ST77XX_BLACK : ST77XX_WHITE;

      // Gambar modul QR sebagai kotak kecil
      tft.fillRect(startX + x * QR_SIZE, startY + y * QR_SIZE, QR_SIZE, QR_SIZE, color);
    }
  }
}

void getDate(){
// Extract Date
  HTTPClient http;
  http.begin("http://worldtimeapi.org/api/timezone/Asia/Jakarta"); 
  int httpCode = http.GET(); 
  
  if (httpCode == 200) { // Jika sukses (200 OK)
    String payload = http.getString(); // Ambil response JSON
    
    // Cari dan ambil field "datetime"
    int dateTimeStart = payload.indexOf("\"datetime\":\"") + 12;
    int dateTimeEnd = payload.indexOf("\",", dateTimeStart);
    String datetime = payload.substring(dateTimeStart, dateTimeEnd); // Format "2024-12-02T12:34:56"

    // Ambil bagian sebelum 'T' sebagai tanggal
    int splitT = datetime.indexOf("T");
    str_currentDate = datetime.substring(0, splitT); // Format "YYYY-MM-DD"

    // Pecah tanggal menjadi tahun, bulan, hari
    str_currentYear = str_currentDate.substring(0, 4);
    str_currentMonth = str_currentDate.substring(5, 7);
    str_currentDay = str_currentDate.substring(8, 10);
  } else {
    debugln("Gagal mengambil waktu. HTTP Code: " + String(httpCode));
  } 
  http.end();
} 

void getTimeDate(){
  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis >= interval) {
    previousMillis = currentMillis;
    gettime();
  }

  unsigned long currentMillisDate = millis();
  if (currentMillisDate - previousMillisDate >= intervalDate ) {
    previousMillisDate = currentMillisDate; // Perbarui waktu terakhir
    getDate(); // Ambil data waktu dari API
  }

  // Print current hour and minute
  unsigned long currentMillisDebugTime = millis();
  if (currentMillisDebugTime - previousMillisDebugTime >= intervalDebugTime ) {
    previousMillisDebugTime = currentMillisDebugTime; // Perbarui waktu terakhir
    debugln("");
    debug("Current Hour: ");
    debugln(currentHour);
    debug("Current Minute: ");
    debugln(currentMinute);
    debug("Current Year: ");
    debugln(str_currentYear);
    debug("Current Month: ");
    debugln(str_currentMonth);
    debug("Current Day: ");
    debugln(str_currentDay);
    debugln("");
  }
}

//MENU
void handleButtonNavigation() {
  if (buttonState3 == LOW && lastButtonState3 == HIGH && showmenu == 0) {
    inMenu = 0;  //Exit Menu and OK
    showmenu = 1;
    statusBright = 1;
    soundUI = true;
    
    debug(inMenu);
  }

  if(inMenu == 1 && showmenu == 1){
    showMenu();
    downbar(); 
    showmenu = 0;
  }

  // Up Menu
  if (buttonState1 == LOW && lastButtonState1 == HIGH) {
    if (statusMenu > 1) {
      statusMenu--;  // Choose Menu Up
      showMenu();  // Show Menu
      downbar(); 
    }
    delay(100);  
  }

  // Down Menu
  if (buttonState2 == LOW && lastButtonState2 == HIGH) {
    if (statusMenu < 5) { // 5 menu 
      statusMenu++;  // choose down
      showMenu();  // show menu
      downbar(); 
    }
    delay(100);  
  }
}

// Fungsi untuk menampilkan menu di layar TFT
void showMenu() {
  tft.fillScreen(ST77XX_BLACK);  // Set background putih
  tft.setTextSize(1);            // Ukuran teks lebih kecil
  tft.setTextColor(ST77XX_WHITE); // Teks berwarna hitam

  // Gambar border rounded di sekitar menu
  // tft.drawRoundRect(10, 11, 108, 128, 10, ST77XX_WHITE); 

  // Menampilkan opsi menu dengan penyorotan sesuai statusMenu
  for (int i = 1; i <= 5; i++) { 
    int yPosition = 13 + (i - 1) * 25;  
  
    int paddingLeft = 15; 
    int textWidth = 108 - 2 * paddingLeft;

    if (i == statusMenu) {
      tft.fillRoundRect(12, yPosition+1, 104 , 20, 10, ST77XX_WHITE); 
      tft.setTextColor(ST77XX_BLACK);  
    } else {
      tft.fillRoundRect(12, yPosition+1, 104 , 20, 10, DARK_GRAY);  
      tft.setTextColor(ST77XX_WHITE);  
    }

    String menuText;
    switch (i) {
      case 1:
        menuText = "Clock";  // Opsi 1
        break;
      case 2:
        menuText = "Sound";  // Opsi 2
        break;
      case 3:
        menuText = "Brightness";  // Opsi 3 
        break;
      case 4:
        menuText = "Restart";  // Opsi 4
        break;
      case 5:
        menuText = "Home";  // Opsi 5
        break;
      default:
        menuText = "Unknown";  // Default teks untuk opsi yang tidak dikenal
        break;
    }

    // Menampilkan teks dengan padding
    tft.setCursor(20, yPosition + 7);  // Kursor dimulai dari 20 untuk padding kiri
    tft.print(menuText);    // Menampilkan opsi menu yang sesuai dengan teks

    debug("Menu Option ");
    debug(i);
    debug(": ");
    if (i == statusMenu) {
      debugln("Selected");
    } else {
      debugln("Not Selected");
    } 
  }

  // Print statusMenu setelah menu diperbarui
  debug("Current Menu Status: ");
  debugln(statusMenu);
  resetArray(statusMenu-1);  
}

//index feature
void resetArray(int indexToSet) {
  for (int i = 0; i < 5; i++) {
    feature_array_stat[i] = (i == indexToSet) ? 1 : 0;
  } 

  debug("[");
  for (int i = 0; i < 5; i++) {
    debug(feature_array_stat[i]);
    if (i < 4) {  // Tambahkan koma kecuali elemen terakhir
      debug(", ");
    }
  }
  debugln("]"); // Tutup array dengan tanda kurung
}

void downbar(){
  tft.fillRoundRect(4, 144, 120, 12, 4, ST77XX_WHITE); 
  if (WiFi.status() != WL_CONNECTED){
    tft.fillCircle(9, 149, 3, ST77XX_RED); 
    leds[0] = CRGB::Red;  
    FastLED.show();  
  } else if (WiFi.status() ==   WL_CONNECTED){
    tft.fillCircle(9, 149, 3, ST77XX_GREEN); 
    leds[0] = CRGB::Green;  
    FastLED.show();  
  }

  tftlcd(2, 1, 15, 147, name, 0); 

  if (statusMenu == 1 && feature_array_stat[0] == 1 && inMenu == 0){
    tftlcd(2, 1, 92, 147, "Clock", 0);
  } else if (statusMenu == 2 && feature_array_stat[1] == 1 && inMenu == 0){
    tftlcd(2, 1, 92, 147, "Sound", 0);
  } else if (statusMenu == 3 && feature_array_stat[2] == 1 && inMenu == 0){
    tftlcd(2, 1, 86, 147, "Bright", 0);
  }

  if(inMenu == 0 && time_downbar == 0  && statusMenu >= 3){
    tftlcd(2, 1, 115, 147, String(page_status + 1), 0);
  } else if (inMenu == 1){
    tftlcd(2, 1, 96, 147, "Menu", 0);
  }
  
}

void brighnessControll() {
  if (statusBright==1){
    drawBrighnessUI();
    downbar();
    statusBright = 0;
  }

  // Tombol menambah nilai
  if (digitalRead(buttonPin1) == LOW) {
    brightnessValue = min(brightnessValue + 5, 100); // Maksimal nilai 100
    setBrightness(brightnessValue);
    drawBrighnessUI();
    downbar();
    delay(150); // Debounce delay
  }
  
  // Tombol mengurangi nilai
  if (digitalRead(buttonPin2) == LOW) {
    brightnessValue = max(brightnessValue - 5, 5); // Minimal nilai 0
    setBrightness(brightnessValue);
    drawBrighnessUI();
    downbar();
    statusBright = 1;
    delay(150); // Debounce delay
  }
}

void drawBrighnessUI() {
  // Bersihkan layar
  tft.fillScreen(ST77XX_BLACK);
  
  // Gambar bar vertikal
  int barHeight = map(brightnessValue, 0, 100, 0, 100); // Hitung tinggi bar
  int barY = 120 - barHeight; // Hitung posisi Y dari atas ke bawah
  tft.fillRect(20, barY, 20, barHeight, ST77XX_WHITE); // Bar terisi (vertikal)
  tft.drawRect(20, 20, 20, 100, ST77XX_WHITE); // Bingkai bar (tetap)

  // Tampilkan nilai sebagai persentase di sebelah kanan, tengah layar
  tft.setTextColor(ST77XX_WHITE);
  tft.setTextSize(3);

  tft.setCursor(55, 60);
  tft.print(brightnessValue);
  tft.print("%");
  
  tft.setTextSize(1);
  tft.setCursor(55, 95);
  tft.print(F("Up (+)"));

  tft.setCursor(55, 105);
  tft.print(F("Down (-)"));
}


void soundCheck() {
  // Cek apakah UI perlu diupdate
  if (soundUI == true) {
    EEPROM.get(64, selectedOption); // Ambil pengaturan dari EEPROM
    debugln(selectedOption);
    soundsrawUI(); // Tampilkan UI awal
    downbar();     // Update bagian bawah
    soundUI = false;
  }

  // Simulasi tombol untuk memilih opsi
  if (buttonState1 == LOW && lastButtonState1 == HIGH) { // Tombol "ON" ditekan
    selectedOption = 1; // Set opsi ke "ON"
    soundsrawUI();      // Update UI
    downbar();
    EEPROM.put(64, selectedOption); // Simpan ke EEPROM
    EEPROM.commit();
    delay(100); // Tambahkan debounce
  }

  if (buttonState2 == LOW && lastButtonState2 == HIGH) { // Tombol "OFF" ditekan
    selectedOption = 0; // Set opsi ke "OFF"
    soundsrawUI();      // Update UI
    downbar();
    EEPROM.put(64, selectedOption); // Simpan ke EEPROM
    EEPROM.commit();
    delay(100); // Tambahkan debounce
  }

  // Update status tombol
  lastButtonState1 = buttonState1;
  lastButtonState2 = buttonState2;
}

void soundsrawUI() {
  tft.fillScreen(ST77XX_BLACK);

  tft.setTextSize(1);
  tft.setTextColor(ST77XX_WHITE);
  tft.setCursor(22, 20);
  tft.println("Sound Setting");

  //Draw "ON" and "OFF"
  drawOption(35, 60, "ON ", selectedOption == 1);  // Pilih "ON" jika selectedOption == 1
  drawOption(75, 60, "OFF", selectedOption == 0); // Pilih "OFF" jika selectedOption == 0
}

void drawOption(int x, int y, const char* text, bool selected) {
  if (selected) {
    tft.fillRoundRect(x - 8, y - 5, 30, 15, 10, ST77XX_WHITE); // Latar putih untuk opsi aktif
    tft.setTextColor(ST77XX_BLACK);
  } else {
    tft.drawRoundRect(x - 8, y - 5, 30, 15, 10, ST77XX_WHITE); // Garis putih untuk opsi tidak aktif
    tft.setTextColor(ST77XX_WHITE);
  }
  tft.setCursor(x, y);
  tft.setTextSize(1);
  tft.print(text);
}
