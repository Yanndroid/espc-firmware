# ESP Clock

<img align="right" src="https://github.com/Yanndroid/espc-hardware/raw/main/readme-res/clock.jpg" alt="ESP Clock" width="30%">

ESPC stands for [ESP](https://www.espressif.com/) Clock and is a digital IoT clock. It's connected to WiFi, automatically syncs time, shows weather information, and most importantly, has RGB! At its core, there is an ESP8266, and the display is made of WS2812B LEDs.  
It's a project I've been working on over the past few years, and it has allowed me to learn a lot about the C language and PCB designs.

This project consists of multiple repositories:

- espc-firmware
- [espc-hardware](https://github.com/Yanndroid/espc-hardware)
- [espc-python-client](https://github.com/Yanndroid/espc-python-client)
- [espc-dev-tools](https://github.com/Yanndroid/espc-dev-tools)

# ESP Clock Firmware

This repository contains the firmware source code for the ESPC. It's written in C using the [ESP8266_RTOS_SDK](https://github.com/espressif/ESP8266_RTOS_SDK).

### Features

- RTOS
- Own WS2812B driver
- Automatic time synchronization
- Local weather (temperature and weather icon)
- NVS storage
- COAP server for setting up, configuring and controlling the ESPC
- OTA updates with signature verification
