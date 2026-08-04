#pragma once
#include <Arduino.h>

extern bool ledIsOn;

void switchSetup();
void switchUpdate();          // เรียกทุกรอบ loop - อ่านสวิตช์แบบ debounce
void setLedState(bool state); // จุดเดียวที่เปลี่ยนสถานะ LED จริง (ใช้ทั้งจากสวิตช์และ NETPIE)
