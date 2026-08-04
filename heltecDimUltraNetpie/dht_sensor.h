#pragma once
#include <Arduino.h>

extern float currentTemp;
extern float currentHumidity;

void dhtSetup();
void dhtUpdate();   // เรียกทุกรอบ loop - ภายในหน่วงเวลาอ่านเองอยู่แล้ว
