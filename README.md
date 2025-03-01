# ESP32Location-client
A small project to determine location using [beaconDB](https://beacondb.net) or similar API 

### How it should work:
- Upon press of a button, an ESP32 device scans for Wi-Fi networks and BLE beacons, parses necessary information, 
and sends the body to a GLS/MLS compatible server, such as [beaconDB](https://beacondb.net).
- Then the received location data is printed over the serial interface (latitude, longitude, accuracy).

### What if it doesn't work?
- If the serial output doesn't show any useful info (all zeroes) without any previous errors, then the [location might need to be mapped](https://github.com/mjaakko/NeoStumbler)

### Required hardware:
- An ESP32 (family) board with a button, defaulting to the `BOOT` button included on most devkits if not set

### Required libraries:
- **[ArduinoJson](https://docs.arduino.cc/libraries/arduinojson)**
- **[MycilaNTP](https://docs.arduino.cc/libraries/mycilantp)**
