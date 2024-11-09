#include <ArduinoJson.h>
#include <HTTPClient.h>

#include <WiFi.h>
#include <NetworkClientSecure.h>
#include <MycilaNTP.h>

#include <BLEDevice.h>
#include <BLEScan.h>
#include <BLEAdvertisedDevice.h>

BLEScan* pBLEScan;

// Network credentials
const char* ssid = "";  // Network SSID
const char* pass = "";  // Network password

// GLS/MLS-compatible host. Change the root certificate if modified!
const char* host = "beacondb.net";
const char* endpoint = "/v1/geolocate";
const char* user_agent = "ESP32Location/0.1";

// IANA timezone format
const char* time_zone = "Europe/Prague";

// ISRG Root X1 certificate for beacondb.net
// Valid until 4th June of 2035
const char* root_certificate = R"=====(
-----BEGIN CERTIFICATE-----
MIIFazCCA1OgAwIBAgIRAIIQz7DSQONZRGPgu2OCiwAwDQYJKoZIhvcNAQELBQAw
TzELMAkGA1UEBhMCVVMxKTAnBgNVBAoTIEludGVybmV0IFNlY3VyaXR5IFJlc2Vh
cmNoIEdyb3VwMRUwEwYDVQQDEwxJU1JHIFJvb3QgWDEwHhcNMTUwNjA0MTEwNDM4
WhcNMzUwNjA0MTEwNDM4WjBPMQswCQYDVQQGEwJVUzEpMCcGA1UEChMgSW50ZXJu
ZXQgU2VjdXJpdHkgUmVzZWFyY2ggR3JvdXAxFTATBgNVBAMTDElTUkcgUm9vdCBY
MTCCAiIwDQYJKoZIhvcNAQEBBQADggIPADCCAgoCggIBAK3oJHP0FDfzm54rVygc
h77ct984kIxuPOZXoHj3dcKi/vVqbvYATyjb3miGbESTtrFj/RQSa78f0uoxmyF+
0TM8ukj13Xnfs7j/EvEhmkvBioZxaUpmZmyPfjxwv60pIgbz5MDmgK7iS4+3mX6U
A5/TR5d8mUgjU+g4rk8Kb4Mu0UlXjIB0ttov0DiNewNwIRt18jA8+o+u3dpjq+sW
T8KOEUt+zwvo/7V3LvSye0rgTBIlDHCNAymg4VMk7BPZ7hm/ELNKjD+Jo2FR3qyH
B5T0Y3HsLuJvW5iB4YlcNHlsdu87kGJ55tukmi8mxdAQ4Q7e2RCOFvu396j3x+UC
B5iPNgiV5+I3lg02dZ77DnKxHZu8A/lJBdiB3QW0KtZB6awBdpUKD9jf1b0SHzUv
KBds0pjBqAlkd25HN7rOrFleaJ1/ctaJxQZBKT5ZPt0m9STJEadao0xAH0ahmbWn
OlFuhjuefXKnEgV4We0+UXgVCwOPjdAvBbI+e0ocS3MFEvzG6uBQE3xDk3SzynTn
jh8BCNAw1FtxNrQHusEwMFxIt4I7mKZ9YIqioymCzLq9gwQbooMDQaHWBfEbwrbw
qHyGO0aoSCqI3Haadr8faqU9GY/rOPNk3sgrDQoo//fb4hVC1CLQJ13hef4Y53CI
rU7m2Ys6xt0nUW7/vGT1M0NPAgMBAAGjQjBAMA4GA1UdDwEB/wQEAwIBBjAPBgNV
HRMBAf8EBTADAQH/MB0GA1UdDgQWBBR5tFnme7bl5AFzgAiIyBpY9umbbjANBgkq
hkiG9w0BAQsFAAOCAgEAVR9YqbyyqFDQDLHYGmkgJykIrGF1XIpu+ILlaS/V9lZL
ubhzEFnTIZd+50xx+7LSYK05qAvqFyFWhfFQDlnrzuBZ6brJFe+GnY+EgPbk6ZGQ
3BebYhtF8GaV0nxvwuo77x/Py9auJ/GpsMiu/X1+mvoiBOv/2X/qkSsisRcOj/KK
NFtY2PwByVS5uCbMiogziUwthDyC3+6WVwW6LLv3xLfHTjuCvjHIInNzktHCgKQ5
ORAzI4JMPJ+GslWYHb4phowim57iaztXOoJwTdwJx4nLCgdNbOhdjsnvzqvHu7Ur
TkXWStAmzOVyyghqpZXjFaH3pO3JLF+l+/+sKAIuvtd7u+Nxe5AW0wdeRlN8NwdC
jNPElpzVmbUq4JUagEiuTDkHzsxHpFKVK7q4+63SM1N95R1NbdWhscdCb+ZAJzVc
oyi3B43njTOQ5yOf+1CceWxG1bQVs5ZufpsMljq4Ui0/1lvh+wjChP4kqKOJ2qxq
4RgqsahDYVvTH9w7jXbyLeiNdd8XM2w9U/t7y0Ff/9yi0GE44Za4rF2LN9d11TPA
mRGunUHBcnWEvgJBQl9nJEiU0Zsnvgc/ubhPgXRR4Xq37Z0j4r7g1SgEEzwxA57d
emyPxgcYxn/eR44/KJ4EBs+lVDR3veyJm+kXQ99b21/+jh5Xos1AnX5iItreGCc=
-----END CERTIFICATE-----
)=====";

// Prepare JSONDocument
DynamicJsonDocument doc(1024);

const int bluetooth_scan_time = 5;           // In seconds
const int wifi_scan_time_per_channel = 500;  // In milliseconds. wifiScanTimePerChannel * 13

// The 'BOOT' button on our board according to the pin reference
const int boot_button_pin = 0;


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

  pBLEScan->start(bluetooth_scan_time, false);
}

void scanNetworks() {
  // Pre-allocate this nested array
  JsonArray accessPoints = doc.createNestedArray("wifiAccessPoints");

  while (true) {
    int networks = WiFi.scanNetworks(false, false, false, wifi_scan_time_per_channel);

    // Calculate AP frequency. Each channel is 5 MHz wide
    /* 
    int calculateFrequency(int channel) {
      return 2412 + (channel - 1) * 5;
    } 
    */

    if (networks == 0) {
      Serial.println("No networks found, trying again in 5s");
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
  pinMode(boot_button_pin, INPUT);

  // Initiate Wi-Fi
  WiFi.mode(WIFI_STA);
  connectToWiFi();

  // Initiate Bluetooth low-energy device
  BLEDevice::init(user_agent);

  // Synchronize time for a secure HTTP connection
  Mycila::NTP.setTimeZone(time_zone);
  Mycila::NTP.sync("pool.ntp.org");
  waitForTimeSync();
}

void loop() {
  int button_state = digitalRead(boot_button_pin);

  if (button_state == HIGH) {
    Serial.println("Scanning for wireless networks...");

    scanBeacons();
    scanNetworks();

    String payload = ichnaeaRequest(parsedBody());
    printPayload(payload);
  }
}
