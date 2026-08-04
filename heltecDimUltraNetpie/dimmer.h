#pragma once
#include <Arduino.h>

extern int currentBrightness;   // ค่า PWM ปัจจุบัน 0-255
extern int currentPotPercent;   // ค่าความสว่างปัจจุบัน แปลงเป็น 0-100%
extern bool dimmerLedOn;        // true ถ้าความสว่าง > 0

void dimmerSetup();
void dimmerUpdate();                    // เรียกทุกรอบ loop - อ่าน VR แล้วปรับความสว่าง LED หรี่ไฟทันที
void setDimmerBrightness(int percent);  // ตั้งความสว่างจากระยะไกล (0-100) เช่นจาก NETPIE