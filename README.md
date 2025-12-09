# TABLE PET COMPANION WITH REALTIME WEATHER MONITORING

## Description
This project is an interactive **Table Pet Companion** powered by an **ESP32-S3** microcontroller. It features expressive animated eyes that react to touch and a real-time weather monitoring system. The device displays temperature and humidity data on an OLED screen and syncs this data to the **Blynk IoT platform** for remote monitoring. It also includes a built-in **WiFiManager** for easy network configuration without hardcoding credentials.

## Key Features
- **Animated Eyes:** Uses the `esp32-eyes` library to render realistic, animated eye expressions (Normal, Happy, Blink, etc.) on an OLED display.
- **Touch Interaction:**
  - **Single Tap:** Triggers a "Happy" expression.
  - **Double Tap:** Switches the display to "Weather Mode" to show sensor readings.
- **Real-Time Weather Monitoring:** Reads Temperature and Humidity from a **DHT11** sensor.
- **IoT Connectivity:** Seamlessly integrates with **Blynk** to visualize sensor data remotely on your smartphone.
- **Easy WiFi Setup:** Features a captive portal (via `WiFiManager`) to connect to WiFi without re-uploading code. If connection fails, it runs in offline mode.

## Hardware Requirements
- **Microcontroller:** ESP32-S3 (e.g., DevKitC-1)
- **Display:** 0.96" or 1.3" I2C OLED Display (SSD1306 driver)
- **Sensors:**
  - DHT11 Temperature & Humidity Sensor
  - Capacitive Touch Sensor (or use a built-in touch pin)
- **Wiring:**
  - **OLED:** SDA (GPIO 41), SCL (GPIO 42)
  - **DHT11:** Data Pin (GPIO 3)
  - **Touch:** Signal Pin (GPIO 7)
  - **WiFi Reset Button:** GPIO 0 (Boot button)

## Contributors
This project was developed by:
- **[MD TAUFIK KHAN](https://github.com/kmdtaufik)**
- **[MD SHAHNUR ISLAM BHUIYAN](https://github.com/shahnur07)**
- **[MD HABIBULLAH](https://github.com/Md-Habibullah-99)**

## Circuit Diagram
*(Place your circuit diagram here)*

> [!NOTE]
> Ensure your `secrets.h` file contains your valid `BLYNK_AUTH_TOKEN` before compiling.

## Installation & Setup
1. **Install Dependencies:**
   - PlatformIO (recommended) or Arduino IDE.
   - Libraries: `Blynk`, `Adafruit GFX`, `Adafruit SSD1306`, `DHT sensor library`, `U8g2`, `WiFiManager`, `esp32-eyes`.
2. **Upload Code:** Connect your ESP32-S3 and upload the firmware.
3. **WiFi Configuration:**
   - On first run, connect to the WiFi hotspot `ESP32-Setup` (Password: `12345678`).
   - Go to `192.168.4.1` in your browser and configure your home WiFi.
4. **Enjoy:** Your Table Pet is ready! Tap to interact.
