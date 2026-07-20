/*
 * Heltec WiFi LoRa 32 V3 - ทดสอบ ArduinoOTA
 * แสดงสถานะบนจอ OLED ในตัว (ใช้ SSD1306Wire ตามโค้ดทดสอบที่ทำงานได้ดีอยู่แล้ว)
 * และไฟ LED บอกสถานะการอัปโหลด
 *
 * Library ที่ต้องติดตั้งก่อนใช้งาน (Sketch > Include Library > Manage Libraries):
 * - ESP8266 and ESP32 OLED driver for SSD1306 displays (by ThingPulse / Fabrice Weinberg)
 * - ArduinoOTA -> มากับ ESP32 core อยู่แล้ว ไม่ต้องติดตั้งเพิ่ม
 *
 * ขั้นตอนใช้งาน:
 * 1. แก้ ssid / password ด้านล่างให้ตรงกับ WiFi ของคุณ
 * 2. อัปโหลดครั้งแรกผ่านสาย USB ตามปกติ (จำเป็นสำหรับ OTA ครั้งถัดไป)
 * 3. เปิด Serial Monitor ดู IP ที่ได้ หรือดูจากจอ OLED
 * 4. ครั้งต่อไป เลือก Tools > Port > จะเห็น "heltec-lora32-v3 at [IP]" เป็น network port
 * 5. กด Upload ได้เลยโดยไม่ต้องเสียบสาย
 */

#include <Wire.h>
#include "SSD1306Wire.h"
#include "pins_arduino.h"   // ให้ SDA_OLED, SCL_OLED, RST_OLED, Vext มาจากไฟล์นี้ของบอร์ด Heltec

#include <WiFi.h>
#include <ESPmDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>

// ---------- ตั้งค่า WiFi (แก้ตรงนี้) ----------
const char* ssid     = "ModelClassroom_1";
const char* password = "8888888888";

// ---------- ขา LED ----------
#define LED_PIN 35   // LED ในตัวบอร์ด — ถ้าบอร์ดคุณไม่มีที่ขานี้ ให้ต่อ LED ภายนอกแล้วแก้เลขพิน

// ---------- จอ OLED (เหมือนโค้ดทดสอบที่ใช้งานได้ดี) ----------
SSD1306Wire display(0x3c, SDA_OLED, SCL_OLED);

bool otaInProgress = false;

/******************
 * VextON / displayReset
 * คัดลอกมาจากโค้ดทดสอบเดิม เพื่อเปิดไฟเลี้ยงและรีเซ็ตจอ OLED
 ******************/
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

// ---------- แสดงข้อความสูงสุด 3 บรรทัด ----------
void showMessage(String line1, String line2 = "", String line3 = "") {
  display.clear();
  display.setTextAlignment(TEXT_ALIGN_LEFT);
  display.setFont(ArialMT_Plain_10);
  display.drawString(0, 0, line1);
  if (line2.length()) display.drawString(0, 16, line2);
  if (line3.length()) display.drawString(0, 32, line3);
  display.display();
}

// ---------- แสดง progress bar ระหว่างอัปโหลด ----------
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

void setup() {
  Serial.begin(115200);

  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);

  // เปิดไฟเลี้ยง + รีเซ็ตจอ OLED ตามวิธีของ Heltec
  VextON();
  displayReset();

  display.init();
  display.flipScreenVertically();
  display.setFont(ArialMT_Plain_10);

  showMessage("Starting...", "Connecting WiFi");

  // ---------- เชื่อมต่อ WiFi ----------
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  int dots = 0;
  while (WiFi.status() != WL_CONNECTED) {
    delay(400);
    digitalWrite(LED_PIN, !digitalRead(LED_PIN)); // กระพริบระหว่างรอ WiFi
    String waitMsg = "Connecting";
    for (int i = 0; i < (dots % 4); i++) waitMsg += ".";
    showMessage("WiFi Setup", waitMsg);
    dots++;
  }

  digitalWrite(LED_PIN, LOW);
  showMessage("WiFi Connected!", WiFi.localIP().toString(), "Ready for OTA");
  Serial.println("IP address: " + WiFi.localIP().toString());

  // ---------- ตั้งค่า ArduinoOTA ----------
  ArduinoOTA.setHostname("heltec-lora32-v3");

  ArduinoOTA.onStart([]() {
    otaInProgress = true;
    String type = (ArduinoOTA.getCommand() == U_FLASH) ? "Sketch" : "Filesystem";
    showMessage("OTA Update Start", "Type: " + type);
    digitalWrite(LED_PIN, HIGH);   // LED ติดค้างตอนเริ่มอัปโหลด
    Serial.println("Start updating " + type);
  });

  ArduinoOTA.onEnd([]() {
    otaInProgress = false;
    showMessage("Upload Success!", "Rebooting...");
    // กระพริบ LED รัว ๆ เพื่อบอกว่าสำเร็จ
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
    digitalWrite(LED_PIN, !digitalRead(LED_PIN)); // กระพริบตามจังหวะรับข้อมูล
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
    // กระพริบ LED ถี่บอกว่า error
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

void loop() {
  ArduinoOTA.handle();

  // เมื่อไม่ได้อยู่ระหว่างอัปโหลด แสดงหน้าจอสถานะพร้อมใช้งานเป็นระยะ
  static unsigned long lastUpdate = 0;
  if (!otaInProgress && millis() - lastUpdate > 3000) {
    lastUpdate = millis();
    showMessage("Ready for OTA",
                 WiFi.localIP().toString(),
                 "LED: " + String(digitalRead(LED_PIN) ? "ON" : "OFF"));
  }
}
