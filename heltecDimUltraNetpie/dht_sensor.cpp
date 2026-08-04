#include "dht_sensor.h"
#include "config.h"
#include "oled_display.h"
#include <DHT.h>

static DHT dht(DHTPIN, DHTTYPE);

float currentTemp = NAN;
float currentHumidity = NAN;

static unsigned long lastDHTRead = 0;
static const unsigned long DHT_INTERVAL = 2500; // อ่านได้ไม่ถี่กว่า ~2 วินาที

void dhtSetup() {
  dht.begin();
}

void dhtUpdate() {
  if (millis() - lastDHTRead <= DHT_INTERVAL) return;
  lastDHTRead = millis();

  float h = dht.readHumidity();
  float t = dht.readTemperature();

  if (isnan(h) || isnan(t)) {
    Serial.println("DHT22 read failed");
    currentTemp = NAN;
    currentHumidity = NAN;
  } else {
    currentTemp = t;
    currentHumidity = h;
    Serial.printf("Temp: %.1fC  Humidity: %.1f%%\n", t, h);
  }

  requestDisplayUpdate();
}
