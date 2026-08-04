#pragma once
#include <Arduino.h>

void oledSetup();
void showMessage(String line1, String line2 = "", String line3 = "");
void showProgress(unsigned int percent);
void showSplashScreen();

// เรียกจากโมดูลอื่นเมื่อสถานะเปลี่ยน (เช่น LED เปลี่ยน หรืออ่าน DHT ใหม่ได้)
void requestDisplayUpdate();

// เรียกทุกรอบ loop - จะวาดจอใหม่ก็ต่อเมื่อมีคน requestDisplayUpdate() ไว้เท่านั้น
void updateDisplayIfNeeded(bool ledIsOn);
