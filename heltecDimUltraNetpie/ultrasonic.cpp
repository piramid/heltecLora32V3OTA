#include "ultrasonic.h"
#include "config.h"
#include "oled_display.h"

static unsigned long lastMeasure = 0;
static const unsigned long MEASURE_INTERVAL = 300; // วัดระยะทุก 300 มิลลิวินาที

float currentDistanceCM = -1;
bool ultrasonicLedOn = false;

void ultrasonicSetup() {
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  digitalWrite(TRIG_PIN, LOW);

  pinMode(ULTRASONIC_LED_PIN, OUTPUT);
  digitalWrite(ULTRASONIC_LED_PIN, LOW);
}

// คืนระยะเป็นเซนติเมตร, คืนค่า -1 ถ้าไม่พบวัตถุ/timeout
static float measureDistanceCM() {
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);

  // timeout 30000us (~30ms) เทียบเท่าระยะไกลสุดประมาณ 5 เมตร
  unsigned long duration = pulseIn(ECHO_PIN, HIGH, 30000);
  if (duration == 0) return -1;

  return duration * 0.0343f / 2.0f;
}

void ultrasonicUpdate() {
  if (millis() - lastMeasure < MEASURE_INTERVAL) return;
  lastMeasure = millis();

  currentDistanceCM = measureDistanceCM();

  if (currentDistanceCM > 0) {
    Serial.printf("Distance: %.1f cm\n", currentDistanceCM);
  } else {
    Serial.println("Distance: no object detected");
  }

  bool shouldBeOn = (currentDistanceCM > 0 && currentDistanceCM <= DISTANCE_THRESHOLD_CM);
  if (shouldBeOn != ultrasonicLedOn) {
    ultrasonicLedOn = shouldBeOn;
    digitalWrite(ULTRASONIC_LED_PIN, ultrasonicLedOn ? HIGH : LOW);
  }

  requestDisplayUpdate();  // อัปเดตจอทุกรอบที่วัดใหม่ (ไม่ว่า LED จะเปลี่ยนหรือไม่)
}