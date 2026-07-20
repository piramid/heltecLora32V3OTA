/*
 * ============================================================
 *  TEMPLATE: Heltec WiFi LoRa32 V3 + ArduinoOTA + OLED status
 * ============================================================
 *  วิธีใช้เทมเพลตนี้:
 *  1. Copy ไฟล์นี้ไปตั้งชื่อโปรเจกต์ใหม่ (อย่าลืมเปลี่ยนชื่อโฟลเดอร์
 *     ให้ตรงกับชื่อไฟล์ .ino ด้วย - เป็นกฎบังคับของ Arduino)
 *  2. แก้ ssid / password ด้านล่าง
 *  3. เขียนโค้ดฟีเจอร์ของคุณเฉพาะใน 2 จุดที่มีคำว่า
 *     "===== YOUR CODE HERE =====" เท่านั้น
 *  4. ห้ามใช้ delay() ยาวๆ ใน loop() - ให้ใช้ millis() แทน
 *     (ดูตัวอย่างในฟังก์ชัน runMyFeature() ด้านล่าง)
 *     ถ้า loop() ค้างนานเกินไป ArduinoOTA.handle() จะไม่ถูกเรียก
 *     ทันเวลา และการอัปโหลดไร้สายจะหลุด/ค้างได้
 *
 *  Library ที่ต้องติดตั้ง (Sketch > Include Library > Manage Libraries):
 *  - ESP8266 and ESP32 OLED driver for SSD1306 displays (by ThingPulse)
 *  - ArduinoOTA -> มากับ ESP32 core อยู่แล้ว
 * ============================================================
 */

#include <Wire.h>
#include "SSD1306Wire.h"
#include "pins_arduino.h"

#include <WiFi.h>
#include <ESPmDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>

// ---------- ตั้งค่า WiFi (แก้ตรงนี้) ----------
const char* ssid     = "YOUR_WIFI_SSID";
const char* password = "YOUR_WIFI_PASSWORD";

// ---------- ตั้งชื่อโปรเจกต์ (จะขึ้นใน Tools > Port ตอนหา OTA) ----------
const char* OTA_HOSTNAME = "heltec-my-project";

// ---------- ขา LED ----------
#define LED_PIN 35   // ถ้าบอร์ดคุณไม่มี LED ตำแหน่งนี้ ให้ต่อ LED ภายนอกแล้วแก้เลขพิน

// ---------- จอ OLED ----------
SSD1306Wire display(0x3c, SDA_OLED, SCL_OLED);

bool otaInProgress = false;

/* ============================================================
 *  ===== OTA BOILERPLATE - ปกติไม่ต้องแก้อะไรตรงนี้ =====
 * ============================================================
 */

void VextON(void) {
  pinMode(Vext, OUTPUT);
  digitalWrite(Vext, LOW);
}

void displayReset(void) {
  pinMode(RST_OLED, OUTPUT);
  digitalWrite(RST_OLED, HIGH);
  delay(1);
  digitalWrite(RST_OLED, LOW);
  delay(1);
  digitalWrite(RST_OLED, HIGH);
  delay(1);
}

void showMessage(String line1, String line2 = "", String line3 = "") {
  display.clear();
  display.setTextAlignment(TEXT_ALIGN_LEFT);
  display.setFont(ArialMT_Plain_10);
  display.drawString(0, 0, line1);
  if (line2.length()) display.drawString(0, 16, line2);
  if (line3.length()) display.drawString(0, 32, line3);
  display.display();
}

void showProgress(unsigned int percent) {
  display.clear();
  display.setFont(ArialMT_Plain_10);
  display.setTextAlignment(TEXT_ALIGN_LEFT);
  display.drawString(0, 0, "Uploading firmware...");
  display.drawProgressBar(0, 32, 120, 10, percent);
  display.setTextAlignment(TEXT_ALIGN_CENTER);
  display.drawString(64, 44, String(percent) + " %");
  display.display();
}

// ---------- หน้าจอเครดิตตอนเปิดเครื่อง (แสดง 2 หน้าสลับกัน) ----------
void showSplashScreen() {
  // หน้าที่ 1: ชื่อผู้พัฒนา + อีเมล
  display.clear();
  display.setTextAlignment(TEXT_ALIGN_CENTER);
  display.setFont(ArialMT_Plain_16);
  display.drawString(64, 4, "Powered by");
  display.drawHorizontalLine(24, 24, 80);
  display.setFont(ArialMT_Plain_10);
  display.drawString(64, 32, "Mr. Pornchai Thong-in");
  display.drawString(64, 48, "pornchai.tom@gmail.com");
  display.display();
  delay(2200);

  // หน้าที่ 2: คณะ + วิทยาลัย
  display.clear();
  display.setTextAlignment(TEXT_ALIGN_CENTER);
  display.setFont(ArialMT_Plain_10);
  display.drawString(64, 10, "Computer Technology");
  display.drawString(64, 22, "Faculty");
  display.drawString(64, 40, "Mahasarakham Technical");
  display.drawString(64, 52, "College");
  display.display();
  delay(2200);
}

void setupOTA() {
  ArduinoOTA.setHostname(OTA_HOSTNAME);

  ArduinoOTA.onStart([]() {
    otaInProgress = true;
    String type = (ArduinoOTA.getCommand() == U_FLASH) ? "Sketch" : "Filesystem";
    showMessage("OTA Update Start", "Type: " + type);
    digitalWrite(LED_PIN, HIGH);
    Serial.println("Start updating " + type);
  });

  ArduinoOTA.onEnd([]() {
    otaInProgress = false;
    showMessage("Upload Success!", "Rebooting...");
    for (int i = 0; i < 6; i++) {
      digitalWrite(LED_PIN, !digitalRead(LED_PIN));
      delay(120);
    }
    digitalWrite(LED_PIN, HIGH);
    Serial.println("\nEnd");
  });

  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    unsigned int percent = (progress / (total / 100));
    showProgress(percent);
    digitalWrite(LED_PIN, !digitalRead(LED_PIN));
    Serial.printf("Progress: %u%%\r", percent);
  });

  ArduinoOTA.onError([](ota_error_t error) {
    otaInProgress = false;
    String errMsg;
    if (error == OTA_AUTH_ERROR) errMsg = "Auth Failed";
    else if (error == OTA_BEGIN_ERROR) errMsg = "Begin Failed";
    else if (error == OTA_CONNECT_ERROR) errMsg = "Connect Failed";
    else if (error == OTA_RECEIVE_ERROR) errMsg = "Receive Failed";
    else if (error == OTA_END_ERROR) errMsg = "End Failed";

    showMessage("OTA Error!", errMsg, "Check connection");
    for (int i = 0; i < 10; i++) {
      digitalWrite(LED_PIN, !digitalRead(LED_PIN));
      delay(80);
    }
    digitalWrite(LED_PIN, LOW);
    Serial.printf("Error[%u]: %s\n", error, errMsg.c_str());
  });

  ArduinoOTA.begin();
  Serial.println("OTA Ready");
}

void connectWiFi() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  int dots = 0;
  while (WiFi.status() != WL_CONNECTED) {
    delay(400);
    digitalWrite(LED_PIN, !digitalRead(LED_PIN));
    String waitMsg = "Connecting";
    for (int i = 0; i < (dots % 4); i++) waitMsg += ".";
    showMessage("WiFi Setup", waitMsg);
    dots++;
  }

  digitalWrite(LED_PIN, LOW);
  showMessage("WiFi Connected!", WiFi.localIP().toString(), "Starting OTA...");
  Serial.println("IP address: " + WiFi.localIP().toString());
}

/* ============================================================
 *  ===== YOUR CODE HERE (1/2): ตัวแปร / setup อะไรก็ได้ =====
 * ============================================================
 *  ตัวอย่าง: ตัวแปรของเซนเซอร์, ตัวแปรนับเวลา ฯลฯ
 */

unsigned long lastFeatureRun = 0;
const unsigned long FEATURE_INTERVAL = 5000; // ทำงานทุก 5 วินาที (ปรับได้)

// เขียนฟังก์ชันฟีเจอร์ของคุณตรงนี้ - ห้ามใส่ delay() ยาวๆ ข้างใน
void runMyFeature() {
  // ตัวอย่าง: อ่านเซนเซอร์ / ส่งข้อมูล / ประมวลผลอะไรก็ได้
  Serial.println("Feature running...");
}

/* ============================================================
 *  จบส่วน YOUR CODE HERE (1/2)
 * ============================================================
 */

void setup() {
  Serial.begin(115200);

  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);

  VextON();
  displayReset();
  display.init();
  display.flipScreenVertically();
  display.setFont(ArialMT_Plain_10);

  showSplashScreen();

  showMessage("Starting...", "Connecting WiFi");

  connectWiFi();
  setupOTA();

  showMessage("Ready!", WiFi.localIP().toString());

  // ===== YOUR CODE HERE: เพิ่มโค้ด setup ฟีเจอร์ของคุณตรงนี้ (ถ้ามี) =====
}

void loop() {
  // ต้องเรียกทุกรอบ ห้ามข้าม ห้ามอยู่หลัง delay() ยาวๆ
  ArduinoOTA.handle();

  /* ============================================================
   *  ===== YOUR CODE HERE (2/2): โค้ด loop ของคุณ =====
   * ============================================================
   *  ใช้ millis() แทน delay() เพื่อไม่ให้บล็อก ArduinoOTA.handle()
   */
  if (!otaInProgress) {
    unsigned long now = millis();
    if (now - lastFeatureRun > FEATURE_INTERVAL) {
      lastFeatureRun = now;
      runMyFeature();
    }
  }
  /* ============================================================
   *  จบส่วน YOUR CODE HERE (2/2)
   * ============================================================
   */

  // หน้าจอสถานะพร้อมใช้งาน (แสดงเมื่อไม่ได้อัปโหลดอยู่)
  static unsigned long lastStatusUpdate = 0;
  if (!otaInProgress && millis() - lastStatusUpdate > 3000) {
    lastStatusUpdate = millis();
    showMessage("Ready for OTA",
                 WiFi.localIP().toString(),
                 "LED: " + String(digitalRead(LED_PIN) ? "ON" : "OFF"));
  }
}
