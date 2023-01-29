# ESP32-RGB-LED-Matrix-Clock
ESP32 based RGB-LED-Matrix WiFi Clock with MQTT sourced room and outside temperature display and date. Automatic brightness control based on LDR

# Features:
- displays Time HH:MM:SS
- displays date
- displays 2 temperatures from MQTT server: i.e. room and outside
- Brightness control with LDR
- NTP timesource
- timezone changable
- firmware updatable over the air (HTTP webserver)
- 5V/1A USB powersupply
- power consumtion average is about 1.3W


PCB was sourced from this design:
https://hackaday.io/project/28945-iot-rgb-led-matrix-controller-esp32


HUB75 DMA library used:
https://github.com/mrfaptastic/ESP32-HUB75-MatrixPanel-DMA

Wall mounted:
![Pic1](pics/20230129_115709710_iOS.jpg)

With acrylic front plate (30% light transfer):
![Pic1](pics/20230129_120152541_iOS.jpg)

Backside:
![Pic1](pics/20230129_120210674_iOS.jpg)

LDR detail:
![Pic1](pics/20230129_120223002_iOS.jpg)
Acrylic front plate is mounted with black PVC tape :-)
