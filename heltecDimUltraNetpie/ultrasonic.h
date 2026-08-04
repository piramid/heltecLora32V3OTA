#pragma once
#include <Arduino.h>

extern float currentDistanceCM;   // ระยะล่าสุดที่วัดได้ (ซม.) -1 = ไม่พบวัตถุ/timeout
extern bool ultrasonicLedOn;      // สถานะ LED ดวงที่ 3 (คุมด้วย HC-SR04 อย่างเดียว)

void ultrasonicSetup();
void ultrasonicUpdate();   // เรียกทุกรอบ loop - วัดระยะเป็นช่วงๆ แล้วสั่ง LED ของตัวเองตามระยะ