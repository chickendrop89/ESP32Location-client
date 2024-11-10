/*
  A small project to determine location using beaconDB or similar API
  Copyright (C) 2024  chickendrop89

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
*/

#include <preferences.h>
#include <ArduinoJson.h>
#include <HTTPClient.h>

#include <WiFi.h>
#include <NetworkClientSecure.h>
#include <MycilaNTP.h>

#include <BLEDevice.h>
#include <BLEScan.h>
#include <BLEAdvertisedDevice.h>

DynamicJsonDocument doc(1024);
BLEScan* pBLEScan;

class BeaconCallback : public BLEAdvertisedDeviceCallbacks {
public:
  JsonArray beacons = doc.createNestedArray("bluetoothBeacons");
  int beaconCount = 0;

  void onResult(BLEAdvertisedDevice advertisedDevice) {
    // Limit colllected beacons to 3 to avoid creating too long JSON body
    if (beaconCount >= 3) {
      pBLEScan->stop();
      pBLEScan->clearResults();  // Clear scan results to release memory
      return;
    }

    BLEAddress ble_address = advertisedDevice.getAddress();
    String rawAddress = ble_address.toString().c_str();

    String macAddress = formatMACAddress(rawAddress);
    int signalStrength = advertisedDevice.getRSSI();

    JsonObject beacon = beacons.createNestedObject();
    beacon["macAddress"] = macAddress;
    beacon["signalStrength"] = signalStrength;

    beaconCount++;
  }
private:
  String formatMACAddress(String macAddress) {
    // Regular expression to insert colons after every two characters
    String regex = "(..)";
    String replacement = "$1:";

    // Replace matches with the replacement string
    macAddress.replace(regex, replacement);

    return macAddress;
  }
};

void scanBeacons() {
  pBLEScan = BLEDevice::getScan();

  pBLEScan->setAdvertisedDeviceCallbacks(new BeaconCallback());
  pBLEScan->setActiveScan(true);  // Active scan uses more power, but get results faster
  pBLEScan->setInterval(100);     // Interval between two consecutive scans
  pBLEScan->setWindow(50);        // Listen duration. Set to half of setInterval to lower power usage

  pBLEScan->start(BLUETOOTH_SCAN_TIME, false);
}

void scanNetworks() {
  // Pre-allocate this nested array
  JsonArray accessPoints = doc.createNestedArray("wifiAccessPoints");

  while (true) {
    int networks = WiFi.scanNetworks(false, false, false, WIFI_CHANNEL_SCAN_TIME);

    // Calculate AP frequency. Each channel is 5 MHz wide
    /* 
    int calculateFrequency(int channel) {
      return 2412 + (channel - 1) * 5;
    } 
    */

    if (networks == 0) {
      Serial.println(F("No networks found, trying again in 5s"));
      delay(5000);
    } else {
      for (int i = 0; i < networks; ++i) {
        // Networks can opt-out of scanning for privacy reasons. Don't include them.
        if (WiFi.SSID(i).indexOf("__nomap") == -1) {
          // Create a new object for each access point
          JsonObject accessPoint = accessPoints.createNestedObject();

          // const int frequency = calculateFrequency(WiFi.channel(i));

          accessPoint["ssid"] = WiFi.SSID(i);
          accessPoint["macAddress"] = WiFi.BSSIDstr(i);
          accessPoint["signalStrength"] = WiFi.RSSI(i);
          accessPoint["channel"] = WiFi.channel(i);
          // accessPoint["frequency"] = frequency;
        }
      }
      WiFi.scanDelete();  // Clear scan results to release memory
      break;              // Break out of the while loop
    }
  }
}

String parsedBody() {
  String json_body;

  serializeJson(doc, json_body);
  doc.clear();  // Clear, so we can reuse later

  return json_body;
}

String ichnaeaRequest(String input_json) {
  std::unique_ptr<NetworkClientSecure> client(new NetworkClientSecure);
  HTTPClient https;

  String content_length = String(input_json.length());

  // Set root certificate to trust
  client->setCACert(root_certificate);

  if (https.begin(*client, host, 443, endpoint, true)) {
    https.addHeader("User-Agent", user_agent);
    https.addHeader("Content-Length", content_length);
    https.addHeader("Content-Type", "application/json");

    int http_code = https.POST(input_json);

    if (http_code > 0) {
      if (http_code == HTTP_CODE_OK || http_code == HTTP_CODE_MOVED_PERMANENTLY) {
        String payload = https.getString();
        return payload;
      } else {
        String error = https.errorToString(http_code).c_str();
        return error;
      }
    } else {
      String error = https.errorToString(http_code).c_str();
      return error;
    }
    https.end();
  }
}

void printPayload(String input_payload) {
  DeserializationError error = deserializeJson(doc, input_payload);

  if (error) {
    Serial.print("Ichnaea deserialization failure: ");
    Serial.println(error.c_str());

    // For debugging purposes
    Serial.println("Input JSON:");
    Serial.println(input_payload);
  }

  // Extract latitude and longitude
  double latitude = doc["location"]["lat"];
  double longitude = doc["location"]["lng"];
  double accuracy = doc["accuracy"];

  Serial.println("");
  Serial.print("Latitude: ");
  Serial.println(latitude, 10);
  Serial.print("Longitude: ");
  Serial.println(longitude, 10);
  Serial.print("Accuracy: ");
  Serial.println(accuracy, 10);

  // Release memory once again
  doc.clear();
}

void connectToWiFi() {
  WiFi.begin(ssid, pass);

  while (WiFi.status() != WL_CONNECTED) {
    Serial.println(F("Waiting for a connection"));
    delay(5000);
  }

  Serial.print("Connected to: ");
  Serial.println(WiFi.SSID());
}

void waitForTimeSync() {
  struct tm timeinfo;

  while (!getLocalTime(&timeinfo)) {
    Serial.println(F("Waiting for time"));
    delay(1500);
  }

  Serial.println("Time synchronized");
}

void setup() {
  Serial.begin(115200);
  pinMode(BOOT_BUTTON_PIN, INPUT);

  // Initiate Wi-Fi
  WiFi.mode(WIFI_STA);
  connectToWiFi();

  // Initiate Bluetooth low-energy device
  BLEDevice::init(user_agent);

  // Synchronize time for a secure HTTP connection
  Mycila::NTP.setTimeZone(time_zone);
  Mycila::NTP.sync(ntp_server);
  waitForTimeSync();
}

void loop() {
  int button_state = digitalRead(BOOT_BUTTON_PIN);

  if (button_state == HIGH) {
    Serial.println("Scanning for wireless networks...");

    scanBeacons();
    scanNetworks();

    String payload = ichnaeaRequest(parsedBody());
    printPayload(payload);
  }
}
