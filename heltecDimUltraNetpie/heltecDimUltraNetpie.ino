/*
 * ============================================================
 *  Heltec LoRa32 V3 - Switch controls LED + OLED status + DHT22
 *  + ส่งข้อมูลไปยัง NETPIE (NETPIE 2020, MQTT)
 * ============================================================
 *  ไฟล์ถูกแยกเป็นไฟล์ย่อยตามฟีเจอร์ (ดูแท็บด้านบนใน Arduino IDE):
 *    - config.h / config.cpp     : ตั้งค่า WiFi, NETPIE, เลขพินทั้งหมด
 *    - oled_display.h / .cpp     : ทุกอย่างเกี่ยวกับจอ OLED
 *    - ota_handler.h / .cpp      : WiFi + ArduinoOTA
 *    - dht_sensor.h / .cpp       : อ่านค่า DHT22
 *    - netpie_mqtt.h / .cpp      : เชื่อมต่อ/ส่ง-รับข้อมูล NETPIE
 *    - led_switch.h / .cpp       : อ่านสวิตช์ + ควบคุม LED ตัวเดิม (setLedState)
 *    - dimmer.h / .cpp           : VR 100K ควบคุมความสว่าง LED หรี่ไฟด้วย PWM
 *    - ultrasonic.h / .cpp       : HC-SR04 วัดระยะ ควบคุม LED ตัวเดิมผ่าน setLedState
 *
 *  การต่อขาที่เพิ่มใหม่:
 *  - LED หรี่ไฟ (ตัวใหม่) -> GPIO 5  (ควบคุมด้วย PWM ตาม VR)
 *  - VR 100K ขาเดือย      -> GPIO 4  (ADC1)
 *  - HC-SR04 TRIG         -> GPIO 15
 *  - HC-SR04 ECHO         -> GPIO 16 (ต้องผ่าน voltage divider ลดจาก 5V เหลือ 3.3V)
 *  - ถ้ามีวัตถุอยู่ใกล้ไม่เกิน 30 ซม. -> LED ตัวเดิม (ขา 46) ติด
 *    ถ้าไกลกว่านั้น/ไม่พบวัตถุ -> LED ตัวเดิมดับ
 *
 *  จอ OLED สลับ 2 หน้าอัตโนมัติทุก 3 วินาที:
 *  - หน้า 1: สถานะ LED ตัวเดิม (ข้อความ + วงกลม) + อุณหภูมิ/ความชื้น
 *  - หน้า 2: วงกลม gauge หมุนตาม VR (เข็ม + เปอร์เซ็นต์) + ระยะจาก HC-SR04 (ซม.)
 *
 *  ทุกค่า (VR %, ระยะ, อุณหภูมิ, ความชื้น, สถานะ LED) จะถูก:
 *  - พิมพ์ออกทาง Serial Monitor (115200 baud)
 *  - วาดบนจอ OLED ตามหน้าที่เกี่ยวข้อง
 *  - ส่งขึ้น NETPIE Shadow ทุก 5 วินาที (หรือทันทีเมื่อ LED เปลี่ยนสถานะ)
 *
 *  ไฟล์นี้ (.ino หลัก) เหลือแค่ setup()/loop() ที่เรียกใช้แต่ละโมดูล
 *  ทำให้เห็นภาพรวมการทำงานได้ในหน้าเดียว โดยรายละเอียดแยกไปแต่ละไฟล์
 *
 *  ก่อนใช้งานต้องแก้ค่าใน config.cpp (WiFi, NETPIE Client ID/Token/Secret)
 *
 *  Library ที่ต้องติดตั้ง (Sketch > Include Library > Manage Libraries):
 *  - ESP8266 and ESP32 OLED driver for SSD1306 displays (by ThingPulse)
 *  - DHT sensor library (by Adafruit)
 *  - Adafruit Unified Sensor
 *  - PubSubClient (by Nick O'Leary)
 *  - ArduinoOTA -> มากับ ESP32 core อยู่แล้ว
 * ============================================================
 */

#include <Wire.h>
#include <WiFi.h>
#include <ArduinoOTA.h>

#include "config.h"
#include "oled_display.h"
#include "ota_handler.h"
#include "dht_sensor.h"
#include "netpie_mqtt.h"
#include "led_switch.h"
#include "dimmer.h"
#include "ultrasonic.h"

void setup() {
  Serial.begin(115200);

  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);

  switchSetup();
  dhtSetup();
  dimmerSetup();
  ultrasonicSetup();

  oledSetup();
  showSplashScreen();
  showMessage("Starting...", "Connecting WiFi");

  connectWiFi();
  setupOTA();
  netpieSetup();

  showMessage("Ready!", WiFi.localIP().toString());
  delay(1000);
}

void loop() {
  // ต้องเรียกทุกรอบ ห้ามข้าม ห้ามอยู่หลัง delay() ยาวๆ
  ArduinoOTA.handle();

  if (!otaInProgress) {
    switchUpdate();
    dhtUpdate();
    dimmerUpdate();
    ultrasonicUpdate();
    updateDisplayIfNeeded(ledIsOn);
    netpieUpdate();
  }
}
