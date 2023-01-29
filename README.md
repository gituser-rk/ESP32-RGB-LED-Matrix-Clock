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



PCB was sourced from this design:
https://hackaday.io/project/28945-iot-rgb-led-matrix-controller-esp32


HUB75 DMA library used:
https://github.com/mrfaptastic/ESP32-HUB75-MatrixPanel-DMA

