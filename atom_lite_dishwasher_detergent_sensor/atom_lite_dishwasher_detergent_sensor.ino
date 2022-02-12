// atom-lite-dishwasher-detergent-sensor
// https://github.com/mugifly/atom-lite-dishwasher-detergent-sensor

#include <WiFi.h>
#include <HTTPClient.h>
#include <FS.h>
#include <SPIFFS.h>

// M5Atom v0.0.2 - https://github.com/m5stack/M5Atom
#include "M5Atom.h"

// HX711 Arduino Library v0.7.5 - https://github.com/bogde/HX711
#include "HX711.h"

// WiFiManager v2.0.5-beta - https://github.com/tzapu/WiFiManager
#include "WiFiManager.h"

// ArduinoJSON v6.18.2 - https://arduinojson.org/
#include "ArduinoJson.h"

// Configurations
#define LOADCELL_DOUT_PIN 25 //32
#define LOADCELL_SCK_PIN 21 //26
#define LOADCELL_CARIBRATION_KNOWN_WEIGHT_GRAM 4.8 // Weight of 100 yen (JPY)

#define LOADCELL_VALID_MINIMUM_THRESHOLD_WEIGHT_GRAM 10

#define DETERGENT_ONLY_FEW_LEFT_THRESHOLD_WEIGHT_GRAM 100
#define DETERGENT_EXTRA_USAGE_THRESHOLD_WEIGHT_GRAM 8
#define DETERGENT_MIN_USAGE_THRESHOLD_WEIGHT_GRAM 5
#define DETERGENT_USAGE_RESETTING_INTERVAL_MILISEC 5400000 // Reset the status after 1.5 hours passed after using detergent

#define INTEGRATION_HASSIO_STATUS_SENDING_MIN_INTERVAL_MILISEC 300000 // Send status after 5 min even if there is no update

// Loadcell sensor (scale)
HX711 loadcell;
float loadcellCaribratationDivider = 515.21;
float detergentBottleWeight = 0.0;
float rawWeight = 0.0;
float beforeRawWeight = 0.0;

// For Integration with IFTTT / Home Assistant (Hass.io)
WiFiManager wifiManager;
WiFiManagerParameter wifiManagerCustomField;
char integrationHassioHost[255] = "";
char integrationHassioToken[255] = "";
char integrationHassioEntityId[255] = "";
char integrationIFTTTEventName[255] = "";
char integrationIFTTTKey[255] = "";
unsigned long integrationHassioLastStatusSentAt = 0L;
unsigned long integrationIFTTTLastStatusSentAt = 0L;

// Status
enum Status {
  // Initialized
  INITIALIZED,
  // LED is turn off
  DETERGENT_NOT_USED,
  // LED is blinking white
  LOADCELL_RESETTED, // blinking 5 times
  // LED is blinking green
  DETERGENT_USUALLY_USED, // Once blinking
  DETERGENT_EXTRA_USED, // Twice blinking
  // LED is blinking yellow
  DETERGENT_USUALLY_USED_AND_ONLY_FEW_LEFT, // Once blinking
  DETERGENT_EXTRA_USED_AND_ONLY_FEW_LEFT, // Twice blinking
  // LED is blinking red
  MEASURED_WEIGHT_INVALID, // Fast blinking
};
Status status = INITIALIZED;
unsigned long statusUpdatedAt = 0L;

void setup() {
  M5.begin(true, false, true);

  Serial.begin(115200);

  // Load config for Loadcell (scale) and Integration
  SPIFFS.begin(true);
  loadConfig();

  // Initialize loadcell (scale)
  loadcell.begin(LOADCELL_DOUT_PIN, LOADCELL_SCK_PIN);
  loadcell.set_scale(loadcellCaribratationDivider);
  loadcell.tare();

  // Initialize Wi-Fi for Integration and Configuration
  WiFi.mode(WIFI_STA);
  wifiManager.setConfigPortalBlocking(false);
  wifiManager.setConfigPortalTimeout(600); // Auto close configportal after 10 minutes

  const char* custom_html =
    "<h2 style=\"margin: 0.2rem 0 0 0;\">Integration</h2><h3>Home Assistant</h3>URL of Home Assistant:<br/><input type=\"text\" name=\"hassio_host\" placeholder=\"https://example.com/\"><br/>Access Token:<br/><input type=\"text\" name=\"hassio_token\" placeholder=\"XXXXXXXXXXXXXXXXXXXXXX\"><br/>Entity ID (sensor.XXXXXX):<br/><input type=\"text\" name=\"hassio_entity_id\" placeholder=\"dishwasher_detergent\" value=\"dishwasher_detergent\"><br/><h3>IFTTT</h3>Webhook Event Name:<br/><input type=\"text\" name=\"ifttt_event_name\" value=\"detergent_updated\" placeholder=\"detergent_updated\"><br/>Webhooks Key:<br/><input type=\"text\" name=\"ifttt_key\" placeholder=\"XXXXXXXXXXXXXXXXXXXXXX\"><br/><hr/>";
  new (&wifiManagerCustomField) WiFiManagerParameter(custom_html);
  wifiManager.addParameter(&wifiManagerCustomField);
  wifiManager.setSaveParamsCallback(wifiManagerSaveParamCallback);

  bool res = wifiManager.autoConnect("dishwasher-detergent-sensor");
  if (!res) { // Failed to connect to configurated Wi-Fi access point. Instead, It will be works as self access point.
    Serial.println("Failed to connect to Wi-Fi. Starting self access point.");
    return;
  }

  // Now, connected to configured Wi-Fi access point.
  Serial.println("Connected to Wi-Fi.");
  M5.dis.drawpix(0, 0x0000f0); // LED is blinking blue
  delay(500);

}

void loadConfig() {
  Serial.println("loadConfig");

  File file = SPIFFS.open("/config.json", "r");
  if (!file) {
    Serial.println("Error: config.json not found");
  }

  size_t fileSize = file.size();
  std::unique_ptr<char[]> buffer(new char[fileSize]);
  file.readBytes(buffer.get(), fileSize);

  DynamicJsonDocument json(1024);
  if (deserializeJson(json, buffer.get())) {
    Serial.println("Error: Could not parse JSON");
    return;
  }

  if (json["loadcell_caribratation_divider"]) {
    float loadcell_caribratation_divider = json["loadcell_caribratation_divider"];
    loadcellCaribratationDivider = loadcell_caribratation_divider;
    Serial.println("loadcell_caribratation_divider = " + String(loadcell_caribratation_divider));
  }

  strcpy(integrationHassioHost, json["hassio_host"]);
  strcpy(integrationHassioToken, json["hassio_token"]);
  strcpy(integrationHassioEntityId, json["hassio_entity_id"]);
  strcpy(integrationIFTTTEventName, json["ifttt_event_name"]);
  strcpy(integrationIFTTTKey, json["ifttt_key"]);

  Serial.println("hassio_host = " + String(integrationHassioHost));
  Serial.println("hassio_token = " + String(integrationHassioToken));
  Serial.println("hassio_entity_id = " + String(integrationHassioEntityId));
  Serial.println("ifttt_event_name = " + String(integrationIFTTTEventName));
  Serial.println("ifttt_key = " + String(integrationIFTTTKey));

  file.close();
}

void saveConfig() {
  Serial.println("saveConfig - Saving config to SPIFFS...");
  DynamicJsonDocument json(1024);

  json["loadcell_caribratation_divider"] = loadcellCaribratationDivider;

  json["hassio_host"] = integrationHassioHost;
  json["hassio_token"] = integrationHassioToken;
  json["hassio_entity_id"] = integrationHassioEntityId;
  json["ifttt_event_name"] = integrationIFTTTEventName;
  json["ifttt_key"] = integrationIFTTTKey;

  File file = SPIFFS.open("/config.json", "w");
  if (!file) {
    Serial.println("Error: Could not open config.json");
    return;
  }
  serializeJson(json, Serial);
  serializeJson(json, file);
  file.close();

  Serial.println("saveConfig - Config saved");

  loadConfig();

}

String getWifiManagerParam(String name) {
  String value;
  if (wifiManager.server->hasArg(name)) {
    value = wifiManager.server->arg(name);
    value.trim();
  }
  return value;
}

void wifiManagerSaveParamCallback() {
  Serial.println("wifiManagerSaveParamCallback");
  strcpy(integrationHassioHost, getWifiManagerParam("hassio_host").c_str());
  strcpy(integrationHassioToken, getWifiManagerParam("hassio_token").c_str());
  strcpy(integrationHassioEntityId, getWifiManagerParam("hassio_entity_id").c_str());
  strcpy(integrationIFTTTEventName, getWifiManagerParam("ifttt_event_name").c_str());
  strcpy(integrationIFTTTKey, getWifiManagerParam("ifttt_key").c_str());
  saveConfig();
}

void calibrateLoadcell() {
  Serial.println("calibrateLoadcell - Resetting...");
  for (int i = 0; i < 5; i++) {
    M5.dis.drawpix(0, 0x00f0f0);
    delay(100);
    M5.dis.drawpix(0, 0x000000);
    delay(100);
  }
  delay(1000);
  loadcell.set_scale();
  loadcell.tare();
  delay(3000);

  Serial.println("calibrateLoadcell - Please place a known weight...");
  for (int i = 0; i < 5; i++) {
    M5.dis.drawpix(0, 0x00f0f0);
    delay(100);
    M5.dis.drawpix(0, 0x000000);
    delay(100);
  }
  delay(3000);
  rawWeight = loadcell.get_units(10);
  loadcellCaribratationDivider = rawWeight / LOADCELL_CARIBRATION_KNOWN_WEIGHT_GRAM;
  Serial.println("calibrateLoadcell - Divider = " + String(loadcellCaribratationDivider));
  loadcell.set_scale(loadcellCaribratationDivider);
  rawWeight = loadcell.get_units(10);
  beforeRawWeight = rawWeight;
  Serial.println(rawWeight);

  saveConfig();

  status = LOADCELL_RESETTED;
}

void measureRemainingAmount() {
  unsigned long now = millis();

  rawWeight = loadcell.get_units(10);
  if (10 <= abs(beforeRawWeight - rawWeight)) { // weight is still unstable
    // Wait until the weight stablizes
    beforeRawWeight = rawWeight;
    return;
  }
  beforeRawWeight = rawWeight;

  if (abs(detergentBottleWeight - rawWeight) <= 1.5) { // bottle weight is almost not changed
    switch (status) {
      case DETERGENT_USUALLY_USED:
      case DETERGENT_EXTRA_USED:
      case DETERGENT_USUALLY_USED_AND_ONLY_FEW_LEFT:
      case DETERGENT_EXTRA_USED_AND_ONLY_FEW_LEFT:
        if (DETERGENT_USAGE_RESETTING_INTERVAL_MILISEC <= now - statusUpdatedAt) {
          // Reset status to "detergent is not used"
          status = DETERGENT_NOT_USED;
          statusUpdatedAt = now;
          // Send status to integration
          sendStatusToHomeAssistant(true);
          sendStatusToIFTTT();
        }
        return;
        break;
    };
  }

  Status newStatus = status;
  if (rawWeight <= LOADCELL_VALID_MINIMUM_THRESHOLD_WEIGHT_GRAM) {
    newStatus = MEASURED_WEIGHT_INVALID;
  } else if (rawWeight <= detergentBottleWeight - DETERGENT_EXTRA_USAGE_THRESHOLD_WEIGHT_GRAM) { // a lot of used
    if (rawWeight <= DETERGENT_ONLY_FEW_LEFT_THRESHOLD_WEIGHT_GRAM) {
      newStatus = DETERGENT_EXTRA_USED_AND_ONLY_FEW_LEFT;
    } else {
      newStatus = DETERGENT_EXTRA_USED;
    }
  } else if (rawWeight <= detergentBottleWeight - DETERGENT_MIN_USAGE_THRESHOLD_WEIGHT_GRAM) { // usually used
    if (rawWeight <= DETERGENT_ONLY_FEW_LEFT_THRESHOLD_WEIGHT_GRAM) {
      newStatus = DETERGENT_USUALLY_USED_AND_ONLY_FEW_LEFT;
    } else {
      newStatus = DETERGENT_USUALLY_USED;
    }
  } else { // Not used or replenished
    newStatus = DETERGENT_NOT_USED;
  }

  if (newStatus != MEASURED_WEIGHT_INVALID) {
    detergentBottleWeight = rawWeight;
  }

  bool statusChanged = (status != newStatus);
  status = newStatus;
  if (statusChanged) statusUpdatedAt = now;

  // Send status to integration
  sendStatusToHomeAssistant(statusChanged);
  if (statusChanged) {
    sendStatusToIFTTT();
  }
}

void sendStatusToHomeAssistant(bool statusChanged) {
  if (WiFi.status() != WL_CONNECTED || strlen(integrationHassioHost) <= 0 || strlen(integrationHassioToken) <= 0 || strlen(integrationHassioEntityId) <= 0) {
    Serial.println("sendStatusToHomeAssistant - Canceled");
    return;
  }

  unsigned long now = millis();
  if (!statusChanged && now - integrationHassioLastStatusSentAt <= INTEGRATION_HASSIO_STATUS_SENDING_MIN_INTERVAL_MILISEC) {
    return;
  }

  String baseUrl = integrationHassioHost;
  String token = integrationHassioToken;
  String entityId = integrationHassioEntityId;
  String statusText = getIntegrationStatusText();

  Serial.println("sendStatusToHomeAssistant - Requesting...");
  integrationHassioLastStatusSentAt = now;

  if (baseUrl.charAt(baseUrl.length() - 1) != '/') {
    baseUrl += "/";
  }
  String url = baseUrl + "api/states/sensor." + entityId;
  Serial.println(url);

  char postBody[255];
  sprintf(postBody, "{\"state\": \"%s\", \"attributes\": {\"rawWeight\": %f, \"bottleWeight\": %f, \"loadcellCaribratationDivider\": %f, \"time\": %u, \"statusUpdatedAt\": %u}}", statusText.c_str(), rawWeight, detergentBottleWeight, loadcellCaribratationDivider, millis(), statusUpdatedAt);
  Serial.println(postBody);

  HTTPClient http;
  http.begin(url);
  http.addHeader("Authorization", "Bearer " + String(token));
  http.addHeader("Content-Type", "application/json");
  http.addHeader("Content-Length", String(strlen(postBody)));
  int httpCode = http.POST(postBody);
  Serial.println("sendStatusToHomeAssistant - Result = " + String(httpCode));
  http.end();
}

void sendStatusToIFTTT() {
  if (WiFi.status() != WL_CONNECTED || strlen(integrationIFTTTKey) <= 0 || strlen(integrationIFTTTEventName) <= 0) {
    Serial.println("sendStatusToIFTTT - Canceled");
    return;
  }

  String eventName = integrationIFTTTEventName;
  String key = integrationIFTTTKey;
  String statusText = getIntegrationStatusText();

  Serial.println("sendStatusToIFTTT - Requesting...");
  integrationIFTTTLastStatusSentAt = millis();

  String url = "https://maker.ifttt.com/trigger/" + eventName + "/with/key/" + key;
  Serial.println(url);

  char postBody[255];
  sprintf(postBody, "{\"value1\": \"%s\", \"value2\": %f, \"value3\": %u}", statusText.c_str(), detergentBottleWeight, statusUpdatedAt);
  Serial.println(postBody);

  HTTPClient http;
  http.begin(url);
  http.addHeader("Content-Type", "application/json");
  http.addHeader("Content-Length", String(strlen(postBody)));
  int httpCode = http.POST(postBody);
  Serial.println("sendStatusToIFTTT - Result = " + String(httpCode));
  http.end();
}

String getIntegrationStatusText() {
  String statusText = "UNKNOWN";
  switch (status) {
    case LOADCELL_RESETTED:
      statusText = "RESETTED";
      break;
    case DETERGENT_NOT_USED:
      statusText = "NOT_USED";
      break;
    case DETERGENT_USUALLY_USED:
      statusText = "USUALLY_USED";
      break;
    case DETERGENT_EXTRA_USED:
      statusText = "EXTRA_USED";
      break;
    case DETERGENT_USUALLY_USED_AND_ONLY_FEW_LEFT:
      statusText = "USUALLY_USED_AND_ONLY_FEW_LEFT";
      break;
    case DETERGENT_EXTRA_USED_AND_ONLY_FEW_LEFT:
      statusText = "EXTRA_USED_AND_ONLY_FEW_LEFT";
      break;
    case MEASURED_WEIGHT_INVALID:
      statusText = "INVALID";
      break;
    default:
      statusText = "UNKNOWN";
      break;
  };
  return statusText;
}

void showStatus() {

  int color = 0x000000;
  int numOfBlinks = 0;

  switch (status) {
    case LOADCELL_RESETTED:
      color = 0xf0f0f0; // white
      numOfBlinks = 5;
    case DETERGENT_NOT_USED:
      // LED is turn off
      break;
    case DETERGENT_USUALLY_USED:
      color = 0xf00000; // green
      numOfBlinks = 1;
      break;
    case DETERGENT_EXTRA_USED:
      color = 0xf00000; // green
      numOfBlinks = 2;
      break;
    case DETERGENT_USUALLY_USED_AND_ONLY_FEW_LEFT:
      color = 0xf0f000; // yellow
      numOfBlinks = 1;
      break;
    case DETERGENT_EXTRA_USED_AND_ONLY_FEW_LEFT:
      color = 0xf0f000; // yellow
      numOfBlinks = 2;
      break;
    case MEASURED_WEIGHT_INVALID:
      color = 0x00f000; // red
      numOfBlinks = -1; // fast blinking
      break;
  };

  if (color == 0x000000) {
    // LED is turn off
    M5.dis.drawpix(0, color);
    return;
  }

  if (numOfBlinks == -1) {
    // LED is fast blinking
    M5.dis.drawpix(0, color);
    delay(50);
    M5.dis.drawpix(0, 0x000000);
    delay(50);
    return;
  }

  for (int count = 0; count < numOfBlinks; count++) {
    // LED is fast blinking
    M5.dis.drawpix(0, color);
    delay(400);
    M5.dis.drawpix(0, 0x000000);
    delay(100);
  }
  delay(500);

}

void resetWiFiSettings() {
  M5.dis.drawpix(0, 0x0000f0); // LED is lights blue
  delay(2000);
  wifiManager.resetSettings();
  ESP.restart();
}

void loop() {
  M5.update();
  wifiManager.process();

  if (M5.Btn.wasReleasefor(8000)) {
    resetWiFiSettings();
  } else if (M5.Btn.wasReleasefor(1000)) {
    calibrateLoadcell();
  } else {
    measureRemainingAmount();
  }

  // Show status using LED
  showStatus();
}
