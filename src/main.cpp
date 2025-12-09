// Fixed: ESP32-S3 Eyes + WiFiManager + Working Channel 11
// Using confirmed working WiFi settings

#include "secrets.h" 
// Ensure your secrets.h has BLYNK_AUTH_TOKEN defined

#define BLYNK_PRINT Serial

// --- FIX FOR "T_R" ERROR ---
#define T_R WM_T_R_GUARD 
#include <WiFiManager.h>
#undef T_R 
// ---------------------------

#include "Face.h"
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <BlynkSimpleEsp32.h>
#include <DHT.h>

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

// ---- WiFi Reset Button ----
#define RESET_WIFI_PIN 0

BlynkTimer timer;

// ---- BITMAPS ----
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

unsigned long lastTouchTime = 0;
bool waitingForSecondTap = false;
const unsigned long doubleTapWindow = 400;
const unsigned long debounceMs = 200;
unsigned long lastTouchEvent = 0;
unsigned long modeStateStartTime = 0;
bool newModeEntered = false;

Face *face;

void sendSensor() {
  float t = dht.readTemperature();
  float h = dht.readHumidity();

  if (isnan(t) || isnan(h)) return;

  if (Blynk.connected()) {
    Blynk.virtualWrite(V0, t);
    Blynk.virtualWrite(V1, h);
  }
}

// ------------- setup -------------
void setup() {
  Serial.begin(115200);
  delay(500);
  
  Serial.println("\n\n=== ESP32 Starting ===");

  // Check if reset button is pressed
  pinMode(RESET_WIFI_PIN, INPUT_PULLUP);
  bool resetWiFi = (digitalRead(RESET_WIFI_PIN) == LOW);

  // 1. Init Display
  Wire.begin(41, 42);
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println("SSD1306 allocation failed");
    for(;;);
  }
  
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.println("Starting...");
  display.display();

  // Handle WiFi Reset
  if (resetWiFi) {
    Serial.println("Reset button pressed - clearing WiFi");
    display.clearDisplay();
    display.setCursor(0, 0);
    display.println("Resetting WiFi...");
    display.display();
    
    WiFiManager wm;
    wm.resetSettings();
    delay(2000);
    ESP.restart();
  }

  // 2. WiFiManager with CHANNEL 11
  WiFiManager wm;
  
  // CRITICAL: Set WiFi channel to 11 (the one that works!)
  wm.setWiFiAPChannel(11);
  
  wm.setDebugOutput(true);
  wm.setConfigPortalTimeout(300); // 5 minutes

  Serial.println("=== Starting WiFi Portal ===");
  Serial.println("Network: ESP32-Setup");
  Serial.println("Password: 12345678");
  Serial.println("Channel: 11");
  Serial.println("IP: 192.168.4.1");

  // Display info
  display.clearDisplay();
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.println("WiFi Setup");
  display.println("");
  display.println("Connect to:");
  display.setTextSize(2);
  display.println("ESP32-Setup");
  display.setTextSize(1);
  display.println("");
  display.println("Pass: 12345678");
  display.println("Then: 192.168.4.1");
  display.display();

  // autoConnect with password on channel 11
  Serial.println("Calling autoConnect...");
  bool res = wm.autoConnect("ESP32-Setup", "12345678"); 
  Serial.print("autoConnect returned: ");
  Serial.println(res ? "SUCCESS" : "FAILED");

  if(!res) {
    Serial.println("WiFi timeout - running offline");
    display.clearDisplay();
    display.setCursor(0,0);
    display.setTextSize(1);
    display.println("WiFi Timeout");
    display.println("");
    display.println("Running Offline");
    display.println("");
    display.println("To retry:");
    display.println("Hold GPIO0 & reset");
    display.display();
  } 
  else {
    Serial.println("=== WiFi Connected Successfully! ===");
    Serial.print("SSID: ");
    Serial.println(WiFi.SSID());
    Serial.print("IP Address: ");
    Serial.println(WiFi.localIP());
    Serial.print("Signal Strength: ");
    Serial.print(WiFi.RSSI());
    Serial.println(" dBm");
    
    display.clearDisplay();
    display.setCursor(0,0);
    display.setTextSize(1);
    display.println("WiFi Connected!");
    display.println("");
    display.print("SSID: ");
    display.println(WiFi.SSID());
    display.print("IP: ");
    display.println(WiFi.localIP());
    display.println("");
    display.println("Connecting Blynk...");
    display.display();
    delay(1000);
    
    // Connect to Blynk
    Serial.println("Connecting to Blynk...");
    Blynk.config(BLYNK_AUTH_TOKEN, "blynk.cloud", 80);
    
    // Try connecting with timeout
    unsigned long blynkStart = millis();
    Blynk.connect();
    
    // Wait up to 10 seconds for Blynk
    while (!Blynk.connected() && millis() - blynkStart < 10000) {
      delay(100);
    }
    
    if (Blynk.connected()) {
      Serial.println("Blynk connected successfully!");
      display.clearDisplay();
      display.setCursor(0,0);
      display.println("All Connected!");
      display.println("");
      display.println("WiFi: OK");
      display.println("Blynk: OK");
      display.display();
    } else {
      Serial.println("Blynk connection failed");
      display.clearDisplay();
      display.setCursor(0,0);
      display.println("WiFi: OK");
      display.println("Blynk: FAILED");
      display.println("");
      display.println("Check token");
      display.display();
    }
  }
  delay(3000);

  // 3. Init Hardware
  Serial.println("Initializing hardware...");
  dht.begin();
  pinMode(TOUCH_PIN, INPUT);
  timer.setInterval(2000L, sendSensor);

  // 4. Init Face
  Serial.println("Initializing face...");
  face = new Face(128, 64, 40);
  face->Expression.GoTo_Normal();
  face->RandomBehavior = true;
  face->RandomBlink = true;
  face->Blink.Timer.SetIntervalMillis(4000);

  display.clearDisplay();
  display.display();
  
  Serial.println("=== Ready! ===");
}

// ------------- Weather Screen -------------
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

// ------------- Touch -------------
void handleTouch() {
  int t = digitalRead(TOUCH_PIN);
  if (t == HIGH) {
    unsigned long now = millis();
    if (now - lastTouchEvent < debounceMs) return;
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

void loop() {
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