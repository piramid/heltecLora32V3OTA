#include "dimmer.h"
#include "config.h"
#include "oled_display.h"

static const int PWM_FREQ = 5000;      // 5 kHz
static const int PWM_RESOLUTION = 8;   // ความละเอียด 8 บิต (0-255)

// จำนวนครั้งที่อ่าน ADC ต่อรอบแล้วเอามาเฉลี่ย (oversampling) - ช่วยลด noise
// ของ ESP32 ADC ได้มาก โดยเฉพาะช่วงกลางของแรงดัน (ไม่ใช่แค่ปลายสุด 0V/3.3V)
static const int ADC_SAMPLES = 8;

// ค่ายิ่งเข้าใกล้ 1.0 ยิ่งกรองแรง (นิ่งขึ้นแต่ตอบสนองช้าลง), ยิ่งเข้าใกล้ 0 ยิ่งไวแต่กรองน้อยลง
static const float SMOOTHING_FACTOR = 0.9f;
static float smoothedValue = -1; // -1 = ยังไม่เคยอ่านค่า (บังคับให้ค่าแรกไม่ถูกกรอง)

// ต้องเปลี่ยนอย่างน้อยเท่านี้ % ถึงจะถือว่าเป็นการหมุนจริง ไม่ใช่ noise
static const int PERCENT_CHANGE_THRESHOLD = 1;

// threshold แยกต่างหากสำหรับ "ยกเลิก override" - ตั้งสูงกว่า threshold ปกติมาก
// เพราะ ESP32 ADC มี noise ค่อนข้างเยอะโดยเฉพาะช่วงกลาง (ไม่ใช่แค่ปลายสุด 0%/100%)
// ถึงจะ oversampling + smoothing แล้ว ก็ยังเผื่อ margin ไว้อีกชั้นกันหลุด override เอง
static const int OVERRIDE_CANCEL_THRESHOLD = 8;

int currentBrightness = 0;
int currentPotPercent = 0;
bool dimmerLedOn = false;

// โหมด override: ถ้ามีคนสั่งความสว่างจาก NETPIE จะค้างค่านั้นไว้
// จนกว่าจะมีคนหมุน VR จริงๆ (ค่าที่วัดได้เปลี่ยนไปจากตอนสั่ง remote) ถึงจะคืนการควบคุมให้ VR
static bool remoteOverride = false;
static int overrideBaselinePercent = 0;

// อ่าน ADC หลายครั้งแล้วเฉลี่ย ลด noise ก่อนส่งเข้า smoothing อีกชั้น
static int readPotAveraged() {
  long sum = 0;
  for (int i = 0; i < ADC_SAMPLES; i++) {
    sum += analogRead(POT_PIN);
  }
  return (int)(sum / ADC_SAMPLES);
}

static void applyBrightness(int percent, bool fromRemote) {
  percent = constrain(percent, 0, 100);
  int brightness = map(percent, 0, 100, 0, 255);

  ledcWrite(DIM_LED_PIN, brightness);

  currentPotPercent = percent;
  currentBrightness = brightness;
  dimmerLedOn = (brightness > 0);

  Serial.printf("%s set brightness to %d%% (PWM %d)\n",
                fromRemote ? "NETPIE" : "VR", percent, brightness);
  requestDisplayUpdate();

  // ไม่ส่งขึ้น NETPIE ทันทีตรงนี้แล้ว เพราะ widget แบบ onSlide ยิงคำสั่งรัวมาก
  // (หลายสิบครั้ง/วินาทีระหว่างลาก) ถ้าส่งยืนยันกลับทุกครั้งเสี่ยงโดน rate limit
  // ปล่อยให้ไปตามรอบ publish ปกติทุก 1 วินาทีแทน (netpie_mqtt.cpp) พอเพียงแล้ว
}

void dimmerSetup() {
  pinMode(POT_PIN, INPUT);

  // ผูก PWM เข้ากับขา LED หรี่ไฟ (Arduino-ESP32 core รุ่นใหม่ใช้ ledcAttach แบบนี้)
  ledcAttach(DIM_LED_PIN, PWM_FREQ, PWM_RESOLUTION);
}

// เรียกจาก NETPIE callback เมื่อมีคำสั่งตั้งความสว่างจากระยะไกล
void setDimmerBrightness(int percent) {
  applyBrightness(percent, true);

  remoteOverride = true;

  // จำ "ตำแหน่งจริงของ VR ตอนนี้" ไว้เทียบ (ไม่ใช่ค่าที่ NETPIE สั่งมา!)
  // ถ้าใช้ค่าที่ NETPIE สั่งมาเป็นตัวเทียบ พอ VR ยังอยู่ตำแหน่งเดิม (ไม่มีใครแตะ)
  // ผลต่างระหว่างตำแหน่ง VR จริงกับค่าที่ NETPIE สั่งจะเกิน threshold ทันที
  // ทำให้เข้าใจผิดว่า "มีคนหมุน VR แล้ว" และยกเลิก override ทันทีทั้งที่ไม่มีใครแตะ
  if (smoothedValue >= 0) {
    overrideBaselinePercent = map((int)smoothedValue, 0, 4095, 0, 100);
  } else {
    overrideBaselinePercent = map(readPotAveraged(), 0, 4095, 0, 100);
  }
}

void dimmerUpdate() {
  int rawValue = readPotAveraged();   // เฉลี่ยหลายครั้งก่อน ลด noise ของ ESP32 ADC

  // กรองสัญญาณด้วย exponential smoothing อีกชั้น แทนใช้ค่าที่เฉลี่ยแล้วตรงๆ
  if (smoothedValue < 0) {
    smoothedValue = rawValue;  // ค่าแรกสุด ใช้ตรงๆ ไปเลย
  } else {
    smoothedValue = (SMOOTHING_FACTOR * smoothedValue) + ((1.0f - SMOOTHING_FACTOR) * rawValue);
  }

  int potValue = (int)smoothedValue;
  int physicalPercent = map(potValue, 0, 4095, 0, 100);

  if (remoteOverride) {
    // ยังอยู่ในโหมดที่ NETPIE สั่งค้างไว้ - เช็คว่ามีคนหมุน VR จริงหรือยัง
    // ใช้ threshold ที่สูงกว่าปกติมาก กัน noise ของ ADC ทำให้ยกเลิก override เองโดยไม่ตั้งใจ
    if (abs(physicalPercent - overrideBaselinePercent) >= OVERRIDE_CANCEL_THRESHOLD) {
      remoteOverride = false; // มีคนหมุน VR แล้ว คืนการควบคุมให้ VR ตามปกติ
    } else {
      return; // ยังไม่มีใครแตะ VR - ไม่เขียนทับค่าที่ NETPIE สั่งไว้
    }
  }

  // โหมดปกติ: VR คุมความสว่างเอง
  if (abs(physicalPercent - currentPotPercent) >= PERCENT_CHANGE_THRESHOLD) {
    applyBrightness(physicalPercent, false);
  }
}