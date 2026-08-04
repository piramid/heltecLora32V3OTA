#include "netpie_mqtt.h"
#include "config.h"
#include "led_switch.h"
#include "dht_sensor.h"
#include "dimmer.h"
#include "ultrasonic.h"
#include <WiFi.h>

static WiFiClient netpieWifiClient;
PubSubClient mqttClient(netpieWifiClient);

static unsigned long lastMqttAttempt = 0;
static const unsigned long MQTT_RETRY_INTERVAL = 5000;

static unsigned long lastPublish = 0;
static const unsigned long PUBLISH_INTERVAL = 1000; // ส่งค่าที่มีอยู่แล้วซ้ำทุก 1 วินาที (ไม่ได้อ่านเซนเซอร์เพิ่ม จึงลดได้อย่างปลอดภัย)

// ทำงานเมื่อมีข้อความส่งมาจาก NETPIE (กดปุ่มบน Freeboard เป็นต้น)
// รับได้ทั้ง "1"/"0", "on"/"off", หรือ "toggle" (ไม่สนตัวพิมพ์เล็ก-ใหญ่)
static void mqttCallback(char* topic, byte* payload, unsigned int length) {
  String message;
  for (unsigned int i = 0; i < length; i++) {
    message += (char)payload[i];
  }
  message.trim();
  Serial.println("NETPIE message [" + String(topic) + "]: " + message);

  if (String(topic) == "@msg/led") {
    String cmd = message;
    cmd.toLowerCase();

    if (cmd == "1" || cmd == "on") {
      setLedState(true);
    } else if (cmd == "0" || cmd == "off") {
      setLedState(false);
    } else if (cmd == "toggle") {
      setLedState(!ledIsOn);
    } else {
      Serial.println("Unknown LED command, ignoring");
    }
  } else if (String(topic) == "@msg/dimmer") {
    // รับค่าความสว่างเป็นตัวเลข 0-100 (%) เช่นจาก Slider widget บน Freeboard
    int percent = message.toInt();
    if (percent == 0 && message != "0") {
      Serial.println("Invalid dimmer value, ignoring");
    } else {
      setDimmerBrightness(percent);
    }
  }
}

static void reconnectNetpie() {
  Serial.print("Connecting to NETPIE...");
  if (mqttClient.connect(NETPIE_CLIENTID, NETPIE_TOKEN, NETPIE_SECRET)) {
    Serial.println("connected!");
    mqttClient.subscribe("@msg/led");
    mqttClient.subscribe("@msg/dimmer");
  } else {
    Serial.print("failed, rc=");
    Serial.println(mqttClient.state());
  }
}

void netpieSetup() {
  mqttClient.setServer(NETPIE_SERVER, NETPIE_PORT);
  mqttClient.setCallback(mqttCallback);
}

void publishToNetpie() {
  String payload = "{\"data\":{";
  if (isnan(currentTemp) || isnan(currentHumidity)) {
    payload += "\"error\":\"sensor\"";
  } else {
    payload += "\"temp\":" + String(currentTemp, 1);
    payload += ",\"humid\":" + String(currentHumidity, 1);
  }
  payload += ",\"led\":" + String(ledIsOn ? 1 : 0);
  payload += ",\"led_ultrasonic\":" + String(ultrasonicLedOn ? 1 : 0);
  payload += ",\"led_dimmer\":" + String(dimmerLedOn ? 1 : 0);
  payload += ",\"vr_percent\":" + String(currentPotPercent);
  payload += ",\"distance_cm\":" + String(currentDistanceCM, 1);
  payload += "}}";

  mqttClient.publish("@shadow/data/update", payload.c_str());
  Serial.println("Published to NETPIE: " + payload);
}

void publishLedStateNow() {
  if (mqttClient.connected()) {
    publishToNetpie();
    lastPublish = millis();
  }
}

void netpieUpdate() {
  if (!mqttClient.connected()) {
    if (millis() - lastMqttAttempt > MQTT_RETRY_INTERVAL) {
      lastMqttAttempt = millis();
      reconnectNetpie();
    }
  } else {
    mqttClient.loop();

    if (millis() - lastPublish > PUBLISH_INTERVAL) {
      lastPublish = millis();
      publishToNetpie();
    }
  }
}