## Building the HullOS software
The HullOS software is built using Platform.IO. You can edit the platformio.ini file in the repository to select the build version and deployment. At present the PICO code is deployed to the PICO debugger. You only need to do this if you want to change the assignment of the GPIO pins or add/change features or change the target platform. 

You can edit the Platform.ini file in this workspace to select the target device (ESP8266, ESP32 or PICOW). As supplied the software builds for all platforms. To build for just one platform edit **platformIO.ini** and remove the semi-colon comment to select the platform. 

```
[platformio]
;default_envs = rpipico
;default_envs = ESP32_DOIT
default_envs = d1_mini
;default_envs = rpipico2
```
If you are using a Raspberry Pi to perform the build you may need to manually install the inteHex library for the ESP32:
```
# 1) Find PlatformIO�s Python (usually here):
~/.platformio/penv/bin/python -V

# 2) Install intelhex into that exact interpreter:
~/.platformio/penv/bin/python -m pip install --upgrade pip
~/.platformio/penv/bin/python -m pip install intelhex

# 3) Sanity-check:
~/.platformio/penv/bin/python -c "import intelhex, sys; print('intelhex', intelhex.__version__, 'on', sys.executable)"

```
