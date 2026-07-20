/*
 * ============================================================
 *  Heltec LoRa32 V3 - Switch controls LED + OLED status
 *  (built from heltec_ota_template.ino - รองรับ OTA)
 * ============================================================
 *  การต่อวงจร:
 *  - สวิตช์ ขา 45  -> ต่อกับ GND เมื่อกด (ใช้ internal pull-up)
 *  - LED    ขา 46  -> ต่อผ่านตัวต้านทาน (220-330 โอห์ม) ไป GND
 *
 *  พฤติกรรม:
 *  - กดสวิตช์ค้าง  -> LED ติด, จอขึ้น "LED ON" + วงกลมทึบ
 *  - ปล่อยสวิตช์   -> LED ดับ, จอขึ้น "LED OFF" + วงกลมเส้นขอบ
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
const char* ssid     = "ModelClassroom_1";
const char* password = "8888888888";

// ---------- ตั้งชื่อโปรเจกต์ (จะขึ้นใน Tools > Port ตอนหา OTA) ----------
const char* OTA_HOSTNAME = "heltec-switch-led";

// ---------- ขา LED สถานะ OTA (ในตัวบอร์ด) ----------
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
 *  ===== YOUR CODE HERE (1/2): ตัวแปร / ฟังก์ชันของฟีเจอร์ =====
 * ============================================================
 */

#define SWITCH_PIN 45
#define USER_LED_PIN 46

// สถานะปัจจุบันที่ใช้เทียบว่ามีการเปลี่ยนแปลงหรือไม่ (กันจอกระพริบซ้ำ)
bool ledIsOn = false;
bool displayNeedsUpdate = true;   // true = บังคับวาดจอครั้งแรกตอนบูต

// ---------- debounce แบบไม่บล็อกโปรแกรม (ไม่ใช้ delay) ----------
int lastRawReading = HIGH;
int stableSwitchState = HIGH;
unsigned long lastDebounceTime = 0;
const unsigned long DEBOUNCE_DELAY = 50; // มิลลิวินาที

// วาดข้อความ LED ON/OFF พร้อมวงกลมทึบ/วงกลมเส้นขอบ
void drawLedStatus(bool isOn) {
  display.clear();

  display.setTextAlignment(TEXT_ALIGN_CENTER);
  display.setFont(ArialMT_Plain_16);
  display.drawString(64, 4, isOn ? "LED ON" : "LED OFF");

  // วงกลมกลางจอ: ทึบเมื่อ ON, เส้นขอบเมื่อ OFF
  int centerX = 64;
  int centerY = 42;
  int radius = 18;

  if (isOn) {
    display.fillCircle(centerX, centerY, radius);
  } else {
    display.drawCircle(centerX, centerY, radius);
  }

  display.display();
}

/* ============================================================
 *  จบส่วน YOUR CODE HERE (1/2)
 * ============================================================
 */

void setup() {
  Serial.begin(115200);

  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);

  // ===== YOUR CODE HERE: ตั้งค่าขาสวิตช์และ LED ของฟีเจอร์ =====
  pinMode(SWITCH_PIN, INPUT_PULLUP);   // กดแล้วขาลง GND = LOW
  pinMode(USER_LED_PIN, OUTPUT);
  digitalWrite(USER_LED_PIN, LOW);

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
  delay(1000);
}

void loop() {
  // ต้องเรียกทุกรอบ ห้ามข้าม ห้ามอยู่หลัง delay() ยาวๆ
  ArduinoOTA.handle();

  /* ============================================================
   *  ===== YOUR CODE HERE (2/2): อ่านสวิตช์ + คุม LED + อัปเดตจอ =====
   * ============================================================
   *  เช็คทุกรอบของ loop (ไม่ใช้ delay) เพื่อให้ตอบสนองไว
   *  และไม่ไปบล็อกการทำงานของ ArduinoOTA.handle()
   */
  if (!otaInProgress) {
    int reading = digitalRead(SWITCH_PIN);

    // ถ้าค่าที่อ่านได้เปลี่ยนจากรอบก่อน ให้เริ่มจับเวลา debounce ใหม่
    if (reading != lastRawReading) {
      lastDebounceTime = millis();
    }

    // ถ้าค่าคงที่นานพอ (ไม่กระเด้ง) ถือว่าเป็นค่าจริง
    if ((millis() - lastDebounceTime) > DEBOUNCE_DELAY) {
      if (reading != stableSwitchState) {
        stableSwitchState = reading;

        bool pressed = (stableSwitchState == LOW); // active-low (internal pull-up)

        if (pressed != ledIsOn) {
          ledIsOn = pressed;
          digitalWrite(USER_LED_PIN, ledIsOn ? HIGH : LOW);
          displayNeedsUpdate = true;
        }
      }
    }

    lastRawReading = reading;

    if (displayNeedsUpdate) {
      drawLedStatus(ledIsOn);
      displayNeedsUpdate = false;
    }
  }
  /* ============================================================
   *  จบส่วน YOUR CODE HERE (2/2)
   * ============================================================
   */
}
