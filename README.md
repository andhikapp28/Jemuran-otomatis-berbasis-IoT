# Detail Project #

I have designed an IoT-based automatic drying rack system that intelligently extends the rack during sunny weather and retracts it when rain is detected. The system utilizes an array of components, including the ESP32 microcontroller, LM393 IC, ULN2003 Driver Module, DHT11 Sensor, Raindrops Module MH-RD, and the 28BYJ-48 Stepper Motor actuator. It can be conveniently controlled through the Blynk.cloud website or the Blynk mobile application.

----
# Components
* ESP32 Microcontroller: Serving as the central processing unit, the ESP32 connects to the internet via Wi-Fi and orchestrates the interaction between sensors and actuators.
* DHT11 Sensor: This sensor gauges temperature and humidity levels. It effectively detects high temperatures indicative of sunny weather and monitors humidity for rain prediction.
* Raindrops Module MH-RD: The raindrops module identifies rain by measuring electrical resistance fluctuations caused by raindrops on its surface.
* ULN2003 Driver Module: Acting as an intermediary between the microcontroller and the 28BYJ-48 Stepper Motor, the ULN2003 module precisely controls motor rotation.
* 28BYJ-48 Stepper Motor: The heart of the actuation system, the stepper motor translates control signals into precise movements of the drying rack.
* Blynk.cloud and Blynk App: Blynk.cloud facilitates the creation of a user interface for remote control, which can be accessed through the Blynk app on smartphones.

----
# How It Works
* **Rain check (every 2s):** the system reads the Raindrops Module. If rain is detected and the rack is currently out, it automatically retracts.
* **Sensor upload & sunny check (every 30s):** temperature/humidity are read from the DHT11 and pushed to Firebase and the Blynk app. If no rain is detected and the temperature is at or above a configurable threshold (default 30°C), the rack automatically extends.
* **Manual control:** the rack can also be extended/retracted anytime from the Blynk app, overriding the automatic state.
* Firebase writes use `set` (not `push`), so each upload overwrites the previous value instead of growing the database indefinitely.

----
# Firmware
The sketch lives in [`src/JemuranIoT.ino`](src/JemuranIoT.ino). Before flashing, fill in your own:
* Wi-Fi `ssid` / `pass`
* Blynk `BLYNK_TEMPLATE_ID`, `BLYNK_DEVICE_NAME`, `BLYNK_AUTH_TOKEN` / `auth`
* Firebase `FIREBASE_HOST` / `FIREBASE_Authorization_key`

The sunny-weather threshold can be adjusted via the `SUNNY_TEMP_THRESHOLD` constant.

----
# Circuit
![picture alt](https://github.com/andhikapp28/Jemuran-otomatis-berbasis-IoT/blob/main/img/Wiring.png "")
