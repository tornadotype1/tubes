#include <Wire.h>
#include <MAX30100_PulseOximeter.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include "time.h"

// Konfigurasi hardware
#define BUZZER_PIN 4
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1

// WiFi credentials
const char* ssid = "althaf";
const char* password = "mahardani";

// Server Flask
const char* serverUrl = "https://heartmonitor.vercel.app/api/bpm";
const char* oledStatusUrl = "https://heartmonitor.vercel.app/status/oled";

// Pulse oximeter
PulseOximeter pox;
uint32_t tsLastReport = 0;
uint32_t tsLastSend = 0;
uint32_t tsLastCheckOled = 0;

// OLED display
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
bool oledOn = true;

// Fungsi untuk mengatur waktu menggunakan NTP
void setupTime() {
  configTime(7 * 3600, 0, "pool.ntp.org", "time.nist.gov");  // UTC+7
  Serial.print("Waiting for time sync");
  while (time(nullptr) < 100000) {
    Serial.print(".");
    delay(500);
  }
  Serial.println("Time synced!");
}

void setup() {
  Serial.begin(115200);
  pinMode(BUZZER_PIN, OUTPUT);
  digitalWrite(BUZZER_PIN, LOW);

  // Inisialisasi OLED
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println("OLED failed");
    while (1);
  }
  display.clearDisplay();
  display.display();

  // Koneksi WiFi
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(0, 0);
  display.println("Connecting WiFi...");
  display.display();

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    display.print(".");
    display.display();
  }

  display.clearDisplay();
  display.setCursor(0, 0);
  display.println("WiFi connected!");
  display.println(WiFi.localIP());
  display.display();
  delay(2000);

  // Waktu
  setupTime();

  // Inisialisasi sensor MAX30100
  if (!pox.begin()) {
    display.clearDisplay();
    display.println("MAX30100 ERROR");
    display.display();
    while (1);
  }
  pox.setIRLedCurrent(MAX30100_LED_CURR_4_4MA);
  pox.setOnBeatDetectedCallback(onBeatDetected);
}

void loop() {
  pox.update();

  // Update OLED tampilan tiap 200ms
  if (millis() - tsLastReport > 200 && oledOn) {
    updateDisplay();
    tsLastReport = millis();
  }

  // Kirim BPM ke server tiap 2 detik
  if (millis() - tsLastSend > 2000) {
    sendToServer();
    tsLastSend = millis();
  }

  // Cek status OLED tiap 3 detik
  if (millis() - tsLastCheckOled > 3000) {
    checkOledStatus();
    tsLastCheckOled = millis();
  }
}

void onBeatDetected() {
  Serial.println("Beat!");
  digitalWrite(BUZZER_PIN, HIGH);
  delay(50);
  digitalWrite(BUZZER_PIN, LOW);
}

void updateDisplay() {
  display.clearDisplay();

  // Tampilkan waktu
  time_t now = time(nullptr);
  struct tm* timeinfo = localtime(&now);
  char timeStr[9]; // "HH:MM:SS"
  strftime(timeStr, sizeof(timeStr), "%H:%M:%S", timeinfo);

  display.setTextSize(1);
  display.setCursor(0, 0);
  display.print(timeStr);

  // Tampilkan BPM
  display.setTextSize(2);
  display.setCursor(0, 20);
  display.print("BPM: ");
  display.println(pox.getHeartRate());

  // Tampilkan status
  display.setTextSize(1);
  display.setCursor(0, 50);
  if (pox.getHeartRate() > 0) {
    display.print("Detecting...");
  } else {
    display.print("Place finger on sensor");
  }

  display.display();
}

void sendToServer() {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    http.begin(serverUrl);
    http.addHeader("Content-Type", "application/json");

    String payload = "{\"bpm\":" + String(pox.getHeartRate()) + "}";
    int httpCode = http.POST(payload);

    if (httpCode > 0) {
      String response = http.getString();
      Serial.println("Server response: " + response);
    } else {
      Serial.println("HTTP POST failed, error: " + String(httpCode));
    }

    http.end();
  } else {
    Serial.println("WiFi disconnected");
  }
}

void checkOledStatus() {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    http.begin(oledStatusUrl);
    int httpCode = http.GET();

    if (httpCode == 200) {
      String payload = http.getString();
      Serial.println("OLED status response: " + payload);

      StaticJsonDocument<200> doc;
      DeserializationError error = deserializeJson(doc, payload);

      if (!error) {
        bool status = doc["oled_on"];
        oledOn = status;

        if (oledOn) {
          display.ssd1306_command(SSD1306_DISPLAYON);
          Serial.println("OLED turned ON");
        } else {
          display.ssd1306_command(SSD1306_DISPLAYOFF);
          Serial.println("OLED turned OFF");
        }
      } else {
        Serial.println("JSON parse error");
      }
    } else {
      Serial.println("Failed to get OLED status");
    }

    http.end();
  }
}
