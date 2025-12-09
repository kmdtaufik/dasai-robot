// Fixed: ESP32-S3 Eyes + Touch + DHT11 + Blynk IoT
// Fix: Explicitly defined Blynk Server to resolve "Connecting to 0.0.0.0" error

// #define BLYNK_TEMPLATE_ID ""
// #define BLYNK_TEMPLATE_NAME ""
// #define BLYNK_AUTH_TOKEN ""
#include "secrets.h"
#define BLYNK_PRINT Serial

#include "Face.h"
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Arduino.h>
#include <BlynkSimpleEsp32.h>
#include <DHT.h>
#include <WiFi.h>
#include <WiFiClient.h>
#include <Wire.h>

// ---- OLED ----
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// ---- DHT11 ----
#define DHTPIN 3
#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE);

// ---- TOUCH ----
#define TOUCH_PIN 7

// ---- WiFi & Blynk ----
char ssid[] = "CPL";
char pass[] = "12345678";
BlynkTimer timer;

// ---- BITMAPS (Icons) ----
const unsigned char PROGMEM icon_temp[] = {
    0x00, 0x00, 0x01, 0x80, 0x01, 0x80, 0x01, 0x80, 0x01, 0x80, 0x01,
    0x80, 0x01, 0x80, 0x01, 0x80, 0x05, 0x80, 0x07, 0x00, 0x0E, 0x70,
    0x0C, 0x30, 0x0C, 0x30, 0x0C, 0x30, 0x0E, 0x70, 0x07, 0xE0};

const unsigned char PROGMEM icon_humid[] = {
    0x00, 0x00, 0x00, 0x00, 0x01, 0x80, 0x03, 0xC0, 0x07, 0xE0, 0x0F,
    0xF0, 0x1F, 0xF8, 0x3F, 0xFC, 0x3F, 0xFC, 0x7F, 0xFE, 0x7F, 0xFE,
    0x7F, 0xFE, 0x3F, 0xFC, 0x1F, 0xF8, 0x07, 0xE0, 0x00, 0x00};

// ---- Modes ----
enum Mode { EYES_MODE, HAPPY_MODE, WEATHER_MODE };
Mode currentMode = EYES_MODE;

// ---- State Vars ----
unsigned long lastTouchTime = 0;
bool waitingForSecondTap = false;
const unsigned long doubleTapWindow = 400;
const unsigned long debounceMs = 200;
unsigned long lastTouchEvent = 0;
unsigned long modeStateStartTime = 0;
bool newModeEntered = false;

// ---- FACE (eyes) ----
Face *face;

// ---- Blynk Sensor Sending ----
void sendSensor() {
  float t = dht.readTemperature();
  float h = dht.readHumidity();

  if (isnan(t) || isnan(h)) {
    Serial.println("DHT Read Failed");
    return;
  }

  // Debug print to Serial
  Serial.print("Sending to Blynk -> T: ");
  Serial.print(t);
  Serial.print(" H: ");
  Serial.println(h);

  // Only send if connected to avoid lag
  if (Blynk.connected()) {
    Blynk.virtualWrite(V0, t);
    Blynk.virtualWrite(V1, h);
  }
}

// ------------- setup -------------
void setup() {
  Serial.begin(115200);

  // 1. Init Display
  Wire.begin(41, 42);
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println("SSD1306 init failed");
    while (1)
      delay(1000);
  }
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.println("Booting...");
  display.display();

  // 2. Manual Wi-Fi Connection
  WiFi.begin(ssid, pass);

  display.print("Wifi: ");
  display.println(ssid);
  display.display();

  int timeout = 0;
  // Wait up to 10 seconds
  while (WiFi.status() != WL_CONNECTED && timeout < 20) {
    delay(500);
    Serial.print(".");
    display.print(".");
    display.display();
    timeout++;
  }

  // 3. Connect to Blynk
  if (WiFi.status() == WL_CONNECTED) {
    display.println("");
    display.println("CONNECTED!");
    display.display();

    // FIX: Explicitly set server to "blynk.cloud"
    Blynk.config(BLYNK_AUTH_TOKEN, "blynk.cloud", 80);
    Blynk.connect();
  } else {
    display.println("");
    display.println("WIFI FAILED!");
    display.println("Offline Mode");
    display.display();
  }
  delay(1000);

  // 4. Init Hardware
  dht.begin();
  pinMode(TOUCH_PIN, INPUT);
  timer.setInterval(2000L, sendSensor);

  // 5. Init Face
  face = new Face(128, 64, 40);
  face->Expression.GoTo_Normal();
  face->RandomBehavior = true;
  face->RandomBlink = true;
  face->Blink.Timer.SetIntervalMillis(4000);

  display.clearDisplay();
  display.display();
}

// ------------- helper: show weather screen -------------
void drawWeatherScreen() {
  float t = dht.readTemperature();
  float h = dht.readHumidity();

  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);

  display.setTextSize(1);
  display.setCursor(28, 0);
  display.println("WEATHER");
  display.drawLine(0, 10, 128, 10, SSD1306_WHITE);

  if (isnan(t) || isnan(h)) {
    display.setCursor(20, 30);
    display.println("Sensor Error");
  } else {
    display.cp437(true);

    // Temp
    display.drawBitmap(10, 18, icon_temp, 16, 16, SSD1306_WHITE);
    display.setTextSize(2);
    display.setCursor(35, 18);
    display.print(t, 1);
    display.setTextSize(1);
    display.write(248);
    display.setTextSize(2);
    display.print("C");

    // Humidity
    display.drawBitmap(10, 42, icon_humid, 16, 16, SSD1306_WHITE);
    display.setTextSize(2);
    display.setCursor(35, 42);
    display.print(h, 0);
    display.print("%");
  }
  display.display();
}

// ------------- touch handling -------------
void handleTouch() {
  int t = digitalRead(TOUCH_PIN);

  if (t == HIGH) {
    unsigned long now = millis();
    if (now - lastTouchEvent < debounceMs)
      return;
    lastTouchEvent = now;

    if (currentMode != EYES_MODE) {
      currentMode = EYES_MODE;
      face->Expression.GoTo_Normal();
      waitingForSecondTap = false;
      return;
    }

    if (!waitingForSecondTap) {
      waitingForSecondTap = true;
      lastTouchTime = now;
    } else {
      if (now - lastTouchTime <= doubleTapWindow) {
        waitingForSecondTap = false;
        currentMode = WEATHER_MODE;
        newModeEntered = true;
        modeStateStartTime = millis();
      } else {
        lastTouchTime = now;
      }
    }
  }

  if (waitingForSecondTap && (millis() - lastTouchTime > doubleTapWindow)) {
    waitingForSecondTap = false;
    currentMode = HAPPY_MODE;
    newModeEntered = true;
    modeStateStartTime = millis();
  }
}

// ------------- main loop -------------
void loop() {
  // Only run Blynk if connected
  if (WiFi.status() == WL_CONNECTED) {
    Blynk.run();
  }

  timer.run();
  handleTouch();

  switch (currentMode) {
  case EYES_MODE:
    face->Update();
    break;

  case HAPPY_MODE:
    if (newModeEntered) {
      face->Expression.GoTo_Happy();
      newModeEntered = false;
    }
    face->Update();
    if (millis() - modeStateStartTime > 1600) {
      face->Expression.GoTo_Normal();
      currentMode = EYES_MODE;
    }
    break;

  case WEATHER_MODE:
    if (newModeEntered) {
      drawWeatherScreen();
      newModeEntered = false;
    }
    if (millis() - modeStateStartTime > 6000) {
      currentMode = EYES_MODE;
    }
    break;
  }
}
//team titan