# ESP32Location-client
A small project to determine location using [beaconDB](https://beacondb.net) or similar API 

**This is my first ever thing i have done with ESPs and C++ in general,
and as my other hardware hasn't shipped yet, i resorted to doing this 😛**

### How it should work:
- An ESP32 device scans for Wi-Fi networks and BLE beacons, parses necessary information, 
and sends the body to a GLS/MLS compatible server, such as [beaconDB](https://beacondb.net).
- Then the received location data is printed over the serial interface.

### What if it doesn't work?
- If the serial output doesn't show any useful info (all zeroes), then the [location needs to be mapped](https://github.com/mjaakko/NeoStumbler)

### Required libraries:
- **[ArduinoJson](https://docs.arduino.cc/libraries/arduinojson)**
- **[MycilaNTP](https://docs.arduino.cc/libraries/mycilantp)**
