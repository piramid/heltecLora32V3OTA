#include "oled_display.h"
#include "SSD1306Wire.h"
#include "pins_arduino.h"
#include "config.h"         // ต้องใช้ EXT_OLED_SDA/EXT_OLED_SCL
#include "dht_sensor.h"     // ต้องใช้ currentTemp/currentHumidity ตอนวาดสถานะ LED
#include "dimmer.h"         // ต้องใช้ currentPotPercent วาด gauge
#include "ultrasonic.h"     // ต้องใช้ currentDistanceCM
#include <math.h>

// จอในตัวบอร์ดพัง เปลี่ยนมาใช้จอ OLED ภายนอกที่ขา 41/42 แทน
SSD1306Wire display(0x3c, EXT_OLED_SDA, EXT_OLED_SCL);

static bool displayNeedsUpdate = true;   // true = บังคับวาดจอครั้งแรกตอนบูต

static void VextON() {
  pinMode(Vext, OUTPUT);
  digitalWrite(Vext, LOW);
}

void oledSetup() {
  VextON();   // เผื่อมีอุปกรณ์อื่นในตัวบอร์ดที่ยังพึ่งไฟเส้นนี้อยู่
  display.init();
  display.flipScreenVertically();
  display.setFont(ArialMT_Plain_10);
}

void showMessage(String line1, String line2, String line3) {
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

void requestDisplayUpdate() {
  displayNeedsUpdate = true;
}

// วาดทุกอย่างในหน้าเดียว: สถานะ LED สวิตช์ + LED ultrasonic (แถวบน),
// VR gauge พร้อมเข็มและเปอร์เซ็นต์ (ซ้ายล่าง), ระยะ/อุณหภูมิ/ความชื้น (ขวาล่าง)
static void drawMainPage(bool switchLedOn) {
  display.clear();

  // ----- แถวบน: สถานะ LED ทั้งสองดวง -----
  display.setTextAlignment(TEXT_ALIGN_LEFT);
  display.setFont(ArialMT_Plain_10);

  // LED สวิตช์ (ซ้าย)
  if (switchLedOn) display.fillCircle(6, 6, 4);
  else             display.drawCircle(6, 6, 4);
  display.drawString(14, 1, "SW");

  // LED ultrasonic (ขวา)
  if (ultrasonicLedOn) display.fillCircle(70, 6, 4);
  else                 display.drawCircle(70, 6, 4);
  display.drawString(78, 1, "US");

  // ----- ซ้ายล่าง: VR gauge -----
  int cx = 24;
  int cy = 36;
  int radius = 16;

  display.drawCircle(cx, cy, radius);

  // 0% ชี้ขึ้นตรง (12 นาฬิกา) แล้วหมุนตามเข็มนาฬิกาไปจนครบ 100% = กลับมาที่เดิม
  float angleRad = (currentPotPercent / 100.0f) * 2 * PI - (PI / 2);
  int nx = cx + (int)((radius - 3) * cos(angleRad));
  int ny = cy + (int)((radius - 3) * sin(angleRad));
  display.drawLine(cx, cy, nx, ny);
  display.fillCircle(cx, cy, 2); // จุดหมุนตรงกลาง

  display.setTextAlignment(TEXT_ALIGN_CENTER);
  display.drawString(cx, 55, String(currentPotPercent) + "%");

  // ----- ขวาล่าง: ระยะ / อุณหภูมิ / ความชื้น -----
  display.setTextAlignment(TEXT_ALIGN_LEFT);

  String distLine = (currentDistanceCM > 0) ? String(currentDistanceCM, 1) + "cm" : "---";
  display.drawString(54, 20, "D: " + distLine);

  if (isnan(currentTemp) || isnan(currentHumidity)) {
    display.drawString(54, 34, "Sensor Error");
  } else {
    display.drawString(54, 34, "T: " + String(currentTemp, 1) + "C");
    display.drawString(54, 48, "H: " + String(currentHumidity, 1) + "%");
  }

  display.display();
}

void updateDisplayIfNeeded(bool ledIsOn) {
  if (!displayNeedsUpdate) return;
  drawMainPage(ledIsOn);
  displayNeedsUpdate = false;
}