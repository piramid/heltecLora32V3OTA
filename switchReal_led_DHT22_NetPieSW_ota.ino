/*
 * ============================================================
 *  Heltec LoRa32 V3 - Switch controls LED + OLED status + DHT22
 *  + ส่งข้อมูลไปยัง NETPIE (NETPIE 2020, MQTT)
 *  (built from heltec_ota_template.ino - รองรับ OTA)
 * ============================================================
 *  การต่อวงจร:
 *  - สวิตช์ ขา 45  -> ต่อกับ GND เมื่อกด (ใช้ internal pull-up)
 *  - LED    ขา 46  -> ต่อผ่านตัวต้านทาน (220-330 โอห์ม) ไป GND
 *  - DHT22  ขา 7   -> ขา DATA ของเซนเซอร์ (ต้องมีตัวต้านทาน pull-up
 *                     10k โอห์ม คร่อมระหว่างขา DATA กับ VCC ถ้าโมดูล
 *                     ไม่มีตัวต้านทานนี้ในบอร์ดมาแล้ว)
 *                     (เคยลอง GPIO 19 - ชนกับ USB ในตัวชิป ทำให้ค้าง
 *                     บูตวนซ้ำ / เคยลอง GPIO 48 - จอ OLED ดับ น่าจะชน
 *                     กับ LED สีในตัวบอร์ด จึงย้ายมาใช้ GPIO 7 แทน)
 *
 *  พฤติกรรม:
 *  - กดสวิตช์ค้าง  -> LED ติด, จอขึ้น "LED ON" + วงกลมทึบ
 *  - ปล่อยสวิตช์   -> LED ดับ, จอขึ้น "LED OFF" + วงกลมเส้นขอบ
 *  - จอแสดงอุณหภูมิ/ความชื้นจาก DHT22 ด้านล่างวงกลม อัปเดตทุก ~2.5 วินาที
 *  - ส่งค่าอุณหภูมิ/ความชื้น/สถานะ LED ขึ้น NETPIE ทุก ~5 วินาที ผ่าน MQTT
 *  - กดปุ่มบน NETPIE (Freeboard) ส่งข้อความ "1"/"0" หรือ "on"/"off" ไปที่
 *    topic "@msg/led" เพื่อสั่งเปิด/ปิด LED จากระยะไกลได้ด้วย
 *
 *  ก่อนใช้งานต้องไปสร้าง Device บน https://netpie.io ก่อน แล้วนำ
 *  Client ID / Token / Secret ที่ได้มาใส่ในค่า NETPIE_* ด้านล่าง
 *
 *  Library ที่ต้องติดตั้ง (Sketch > Include Library > Manage Libraries):
 *  - ESP8266 and ESP32 OLED driver for SSD1306 displays (by ThingPulse)
 *  - DHT sensor library (by Adafruit)
 *  - Adafruit Unified Sensor (library ที่ DHT sensor library ต้องใช้ร่วม)
 *  - PubSubClient (by Nick O'Leary) -> สำหรับเชื่อมต่อ MQTT กับ NETPIE
 *  - ArduinoOTA -> มากับ ESP32 core อยู่แล้ว
 * ============================================================
 */

#include <Wire.h>
#include "SSD1306Wire.h"
#include "pins_arduino.h"
#include <DHT.h>
#include <PubSubClient.h>

#include <WiFi.h>
#include <ESPmDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>

// ---------- ตั้งค่า WiFi (แก้ตรงนี้) ----------
const char* ssid     = "ModelClassroom_1";
const char* password = "8888888888";

// ---------- ตั้งชื่อโปรเจกต์ (จะขึ้นใน Tools > Port ตอนหา OTA) ----------
const char* OTA_HOSTNAME = "heltec-switch-led-dht22-netpie";

// ---------- ตั้งค่า NETPIE (แก้ตรงนี้ ใช้ค่าจาก Device บน netpie.io) ----------
const char* NETPIE_SERVER   = "mqtt.netpie.io";
const int   NETPIE_PORT     = 1883;
const char* NETPIE_CLIENTID = "d6fcf85e-63f0-42ae-b912-f83a36742ed4";
const char* NETPIE_TOKEN    = "GVDJNowNibNhbFEJtdQVNHbpcaxJqFXT";     // ใช้เป็น MQTT username
const char* NETPIE_SECRET   = "ESqQ4fpSihE3cucYSgBMhw9CtWQEgXrc";    // ใช้เป็น MQTT password

WiFiClient netpieWifiClient;
PubSubClient mqttClient(netpieWifiClient);

// ---------- ขา LED สถานะ OTA (ในตัวบอร์ด) ----------
#define LED_PIN 35   // ถ้าบอร์ดคุณไม่มี LED ตำแหน่งนี้ ให้ต่อ LED ภายนอกแล้วแก้เลขพิน

// ---------- จอ OLED ----------
SSD1306Wire display(0x3c, 41, 42);

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

// ---------- DHT22 ----------
#define DHTPIN 7
#define DHTTYPE DHT22
DHT dht(DHTPIN, DHTTYPE);

float currentTemp = NAN;
float currentHumidity = NAN;
unsigned long lastDHTRead = 0;
const unsigned long DHT_INTERVAL = 2500; // DHT22 อ่านได้ไม่ถี่กว่า ~2 วินาที

// ---------- NETPIE (MQTT) ----------
unsigned long lastMqttAttempt = 0;
const unsigned long MQTT_RETRY_INTERVAL = 5000; // พยายามเชื่อมต่อใหม่ทุก 5 วินาทีถ้าหลุด

unsigned long lastPublish = 0;
const unsigned long PUBLISH_INTERVAL = 5000; // ส่งข้อมูลขึ้น NETPIE ทุก 5 วินาที

// พยายามเชื่อมต่อ NETPIE (เรียกเป็นช่วงๆ ไม่เรียกทุกรอบ loop เพราะ
// mqttClient.connect() ค้างได้หลายวินาทีถ้าเชื่อมต่อไม่สำเร็จ)
void reconnectNetpie() {
  Serial.print("Connecting to NETPIE...");
  if (mqttClient.connect(NETPIE_CLIENTID, NETPIE_TOKEN, NETPIE_SECRET)) {
    Serial.println("connected!");
    mqttClient.subscribe("@msg/led");   // รอรับคำสั่งควบคุม LED จาก NETPIE
  } else {
    Serial.print("failed, rc=");
    Serial.println(mqttClient.state());
  }
}

// ทำงานเมื่อมีข้อความส่งมาจาก NETPIE (กดปุ่มบน Freeboard เป็นต้น)
// รับได้ทั้ง "1"/"0" หรือ "on"/"off" (ไม่สนตัวพิมพ์เล็ก-ใหญ่)
void mqttCallback(char* topic, byte* payload, unsigned int length) {
  String message;
  for (unsigned int i = 0; i < length; i++) {
    message += (char)payload[i];
  }
  message.trim();
  Serial.println("NETPIE message [" + String(topic) + "]: " + message);

  if (String(topic) == "@msg/led") {
    String cmd = message;
    cmd.toLowerCase();

    bool newState;
    if (cmd == "1" || cmd == "on") {
      newState = true;
    } else if (cmd == "0" || cmd == "off") {
      newState = false;
    } else {
      Serial.println("Unknown LED command, ignoring");
      return;
    }

    if (newState != ledIsOn) {
      ledIsOn = newState;
      digitalWrite(USER_LED_PIN, ledIsOn ? HIGH : LOW);
      displayNeedsUpdate = true;

      // ส่งสถานะล่าสุดกลับขึ้น NETPIE ทันที เพื่อให้ Shadow ตรงกับความจริง
      if (mqttClient.connected()) {
        publishToNetpie();
        lastPublish = millis();
      }
    }
  }
}

// ส่งค่าล่าสุดขึ้น NETPIE Shadow (JSON เดียว รวมทุกค่า)
void publishToNetpie() {
  String payload = "{\"data\":{";
  if (isnan(currentTemp) || isnan(currentHumidity)) {
    payload += "\"error\":\"sensor\"";
  } else {
    payload += "\"temp\":" + String(currentTemp, 1);
    payload += ",\"humid\":" + String(currentHumidity, 1);
  }
  payload += ",\"led\":" + String(ledIsOn ? 1 : 0);
  payload += "}}";

  mqttClient.publish("@shadow/data/update", payload.c_str());
  Serial.println("Published to NETPIE: " + payload);
}

// วาดข้อความ LED ON/OFF พร้อมวงกลมทึบ/วงกลมเส้นขอบ และค่าจาก DHT22 ด้านล่าง
void drawLedStatus(bool isOn) {
  display.clear();

  display.setTextAlignment(TEXT_ALIGN_CENTER);
  display.setFont(ArialMT_Plain_16);
  display.drawString(64, 0, isOn ? "LED ON" : "LED OFF");

  // วงกลมกลางจอ: ทึบเมื่อ ON, เส้นขอบเมื่อ OFF
  int centerX = 64;
  int centerY = 32;
  int radius = 14;

  if (isOn) {
    display.fillCircle(centerX, centerY, radius);
  } else {
    display.drawCircle(centerX, centerY, radius);
  }

  // แสดงค่าอุณหภูมิ/ความชื้นด้านล่างสุด
  display.setFont(ArialMT_Plain_10);
  if (isnan(currentTemp) || isnan(currentHumidity)) {
    display.drawString(64, 52, "Sensor Error");
  } else {
    String line = String(currentTemp, 1) + "C   " + String(currentHumidity, 1) + "%RH";
    display.drawString(64, 52, line);
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
  dht.begin();

  VextON();
  displayReset();
  display.init();
  display.flipScreenVertically();
  display.setFont(ArialMT_Plain_10);

  showSplashScreen();

  showMessage("Starting...", "Connecting WiFi");

  connectWiFi();
  setupOTA();

  mqttClient.setServer(NETPIE_SERVER, NETPIE_PORT);
  mqttClient.setCallback(mqttCallback);

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

          // ส่งสถานะ LED ขึ้น NETPIE ทันที ไม่ต้องรอรอบเวลาปกติ
          if (mqttClient.connected()) {
            publishToNetpie();
            lastPublish = millis(); // เลื่อนรอบถัดไปออกไป กันส่งซ้ำติดกัน
          }
        }
      }
    }

    lastRawReading = reading;

    // ---------- อ่าน DHT22 แบบไม่บล็อก (อ่านเป็นช่วงๆ ไม่ใช่ทุกรอบ) ----------
    if (millis() - lastDHTRead > DHT_INTERVAL) {
      lastDHTRead = millis();

      float h = dht.readHumidity();
      float t = dht.readTemperature();

      if (isnan(h) || isnan(t)) {
        Serial.println("DHT22 read failed");
        currentTemp = NAN;
        currentHumidity = NAN;
      } else {
        currentTemp = t;
        currentHumidity = h;
        Serial.printf("Temp: %.1fC  Humidity: %.1f%%\n", t, h);
      }
      displayNeedsUpdate = true;
    }

    if (displayNeedsUpdate) {
      drawLedStatus(ledIsOn);
      displayNeedsUpdate = false;
    }

    // ---------- ดูแลการเชื่อมต่อ NETPIE (ไม่บล็อกทุกรอบ) ----------
    if (!mqttClient.connected()) {
      if (millis() - lastMqttAttempt > MQTT_RETRY_INTERVAL) {
        lastMqttAttempt = millis();
        reconnectNetpie();
      }
    } else {
      mqttClient.loop();   // ต้องเรียกทุกรอบเมื่อเชื่อมต่ออยู่ เพื่อรักษาสถานะ MQTT

      if (millis() - lastPublish > PUBLISH_INTERVAL) {
        lastPublish = millis();
        publishToNetpie();
      }
    }
  }
  /* ============================================================
   *  จบส่วน YOUR CODE HERE (2/2)
   * ============================================================
   */
}
