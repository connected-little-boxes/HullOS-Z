## Building the HullOS software
The HullOS software is built using Platform.IO. You can edit the platformio.ini file in the repository to select the build version and deployment. At present the PICO code is deployed to the PICO debugger. You only need to do this if you want to change the assignment of the GPIO pins or add/change features or change the target platform. 

You can edit the Platform.ini file in this workspace to select the target device (ESP8266, ESP32 or PICOW). As supplied the software builds for the PICO W version 1. It will also build for ESP32 although the motor drive code has not been tested yet. 
