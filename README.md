# Cat Feeder ESP8266 Project

This project is designed to control a servo motor for a cat feeder using an ESP8266 module. It connects to WiFi, retrieves the current time from an NTP server, and interacts with a ThingSpeak channel to check and update the timestamp of the last feeding.

## Components

- ESP8266 module
- Servo motor
- EEPROM of the ESP8266 module
- NTPClient for time synchronization
- ThingSpeak for data logging

## Features

- Connects to a predefined WiFi network using stored credentials from EEPROM.
- Sets up an access point if no credentials are found or if they are invalid.
- Fetches the current timestamp from a ThingSpeak channel and updates it periodically.
- Controls a servo motor to dispense food.
- Provides a web interface to save new WiFi credentials.

## Libraries Used

- Servo.h
- EEPROM.h
- ESP8266WiFi.h
- ESP8266WebServer.h
- WiFiClient.h
- ArduinoJson.h
- NTPClient.h
- WiFiUdp.h

## Setup and Usage

1. **Initial Setup:**
   - Power up the ESP8266.
   - If no valid WiFi credentials are stored, it will start an access point with the SSID `CatFeeder` and password `catfeeder123`.

2. **Access Point:**
   - Connect to the `CatFeeder` WiFi network.
   - Open the mobile app.
   - Enter your WiFi SSID and password in the provided form and submit.

3. **Normal Operation:**
   - The ESP8266 will connect to the specified WiFi network.
   - It will synchronize time using the NTP server `pool.ntp.org`.
   - It will periodically check and update the ThingSpeak channel with the current timestamp.
   - The servo motor will be activated based on the ThingSpeak timestamp.
