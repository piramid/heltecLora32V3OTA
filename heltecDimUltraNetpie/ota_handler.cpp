#include "ota_handler.h"
#include "config.h"
#include "oled_display.h"

#include <WiFi.h>
#include <ESPmDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>

bool otaInProgress = false;

void connectWiFi() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  int dots = 0;
  while (WiFi.status() != WL_CONNECTED) {
    delay(400);
    digitalWrite(LED_PIN, !digitalRead(LED_PIN));
    String waitMsg = "Connecting";
    for (int i = 0; i < (dots % 4); i++) waitMsg += ".";
    showMessage("WiFi Setup", waitMsg);
    dots++;
  }

  digitalWrite(LED_PIN, LOW);
  showMessage("WiFi Connected!", WiFi.localIP().toString(), "Starting OTA...");
  Serial.println("IP address: " + WiFi.localIP().toString());
}

void setupOTA() {
  ArduinoOTA.setHostname(OTA_HOSTNAME);

  ArduinoOTA.onStart([]() {
    otaInProgress = true;
    String type = (ArduinoOTA.getCommand() == U_FLASH) ? "Sketch" : "Filesystem";
    showMessage("OTA Update Start", "Type: " + type);
    digitalWrite(LED_PIN, HIGH);
    Serial.println("Start updating " + type);
  });

  ArduinoOTA.onEnd([]() {
    otaInProgress = false;
    showMessage("Upload Success!", "Rebooting...");
    for (int i = 0; i < 6; i++) {
      digitalWrite(LED_PIN, !digitalRead(LED_PIN));
      delay(120);
    }
    digitalWrite(LED_PIN, HIGH);
    Serial.println("\nEnd");
  });

  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    unsigned int percent = (progress / (total / 100));
    showProgress(percent);
    digitalWrite(LED_PIN, !digitalRead(LED_PIN));
    Serial.printf("Progress: %u%%\r", percent);
  });

  ArduinoOTA.onError([](ota_error_t error) {
    otaInProgress = false;
    String errMsg;
    if (error == OTA_AUTH_ERROR) errMsg = "Auth Failed";
    else if (error == OTA_BEGIN_ERROR) errMsg = "Begin Failed";
    else if (error == OTA_CONNECT_ERROR) errMsg = "Connect Failed";
    else if (error == OTA_RECEIVE_ERROR) errMsg = "Receive Failed";
    else if (error == OTA_END_ERROR) errMsg = "End Failed";

    showMessage("OTA Error!", errMsg, "Check connection");
    for (int i = 0; i < 10; i++) {
      digitalWrite(LED_PIN, !digitalRead(LED_PIN));
      delay(80);
    }
    digitalWrite(LED_PIN, LOW);
    Serial.printf("Error[%u]: %s\n", error, errMsg.c_str());
  });

  ArduinoOTA.begin();
  Serial.println("OTA Ready");
}
