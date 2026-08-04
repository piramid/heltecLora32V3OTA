#pragma once
#include <Arduino.h>

// ---------- ตั้งค่า WiFi (แก้ตรงนี้) ----------
extern const char* ssid;
extern const char* password;

// ---------- ตั้งชื่อโปรเจกต์ (จะขึ้นใน Tools > Port ตอนหา OTA) ----------
extern const char* OTA_HOSTNAME;

// ---------- ตั้งค่า NETPIE (แก้ตรงนี้ ใช้ค่าจาก Device บน netpie.io) ----------
extern const char* NETPIE_SERVER;
extern const int   NETPIE_PORT;
extern const char* NETPIE_CLIENTID;
extern const char* NETPIE_TOKEN;
extern const char* NETPIE_SECRET;

// ---------- ขาที่ใช้ ----------
#define LED_PIN 35        // LED สถานะ OTA ในตัวบอร์ด
#define SWITCH_PIN 45     // สวิตช์
#define USER_LED_PIN 46   // LED ที่ควบคุมด้วยสวิตช์/NETPIE/HC-SR04
#define DHTPIN 7          // ขา DATA ของ DHT22
#define DHTTYPE DHT22

#define DIM_LED_PIN 5     // LED หรี่ไฟ (ควบคุมด้วย PWM)
#define POT_PIN 4         // ขาเดือย (wiper) ของ VR 100K - ต้องเป็นขา ADC1
#define TRIG_PIN 47       // HC-SR04 TRIG
#define ECHO_PIN 48       // HC-SR04 ECHO (ต้องผ่าน voltage divider ลดจาก 5V เหลือ 3.3V ก่อน)
#define DISTANCE_THRESHOLD_CM 30.0
#define ULTRASONIC_LED_PIN 2   // LED ดวงที่ 3 - คุมด้วย HC-SR04 อย่างเดียว แยกจาก LED สวิตช์
                                // (เคยลอง GPIO 6 แล้วขาไม่จ่ายไฟออกเลยทั้งที่ LED ปกติ จึงย้ายมา GPIO 2)

// จอในตัวบอร์ดพัง จึงใช้จอ OLED ภายนอกแทน ต่อผ่าน I2C
#define EXT_OLED_SDA 41
#define EXT_OLED_SCL 42