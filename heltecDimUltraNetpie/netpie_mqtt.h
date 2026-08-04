#pragma once
#include <Arduino.h>
#include <PubSubClient.h>

extern PubSubClient mqttClient;

void netpieSetup();
void netpieUpdate();        // เรียกทุกรอบ loop - จัดการ reconnect + publish ให้เอง
void publishToNetpie();     // ส่งค่าล่าสุดทั้งหมดขึ้น NETPIE Shadow
void publishLedStateNow();  // ส่งทุกค่าล่าสุดขึ้น NETPIE ทันที (ไม่ต้องรอรอบเวลาปกติ) ใช้ตอน LED/dimmer เปลี่ยนสถานะ