#include "led_switch.h"
#include "config.h"
#include "oled_display.h"
#include "netpie_mqtt.h"
#include "ota_handler.h"

bool ledIsOn = false;

// ---------- debounce แบบไม่บล็อกโปรแกรม (ไม่ใช้ delay) ----------
static int lastRawReading = HIGH;
static int stableSwitchState = HIGH;
static unsigned long lastDebounceTime = 0;
static const unsigned long DEBOUNCE_DELAY = 50; // มิลลิวินาที

void switchSetup() {
  pinMode(SWITCH_PIN, INPUT_PULLUP);   // กดแล้วขาลง GND = LOW
  pinMode(USER_LED_PIN, OUTPUT);
  digitalWrite(USER_LED_PIN, LOW);
}

// จุดเดียวที่เปลี่ยนสถานะ LED จริง - ทั้งสวิตช์จริงและคำสั่งจาก NETPIE
// เรียกฟังก์ชันนี้ร่วมกัน กันโค้ดซ้ำซ้อนและพฤติกรรมไม่ตรงกัน
void setLedState(bool newState) {
  if (newState == ledIsOn) return;

  ledIsOn = newState;
  digitalWrite(USER_LED_PIN, ledIsOn ? HIGH : LOW);
  requestDisplayUpdate();
  publishLedStateNow();
}

void switchUpdate() {
  int reading = digitalRead(SWITCH_PIN);

  if (reading != lastRawReading) {
    lastDebounceTime = millis();
  }

  if ((millis() - lastDebounceTime) > DEBOUNCE_DELAY) {
    if (reading != stableSwitchState) {
      stableSwitchState = reading;
      bool pressed = (stableSwitchState == LOW); // active-low (internal pull-up)
      setLedState(pressed);
    }
  }

  lastRawReading = reading;
}
