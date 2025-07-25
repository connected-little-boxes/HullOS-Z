# HULLOS-Z
Latest Connected Little Boxes software with embeded PythonIsh. This is a snapshot of a work in progress. You can build it for the Raspberry Pi PICO (just edit the platform.ini file). It presently doesn't build for any of the other platforms. 

Edit the defaults.hsec file in the src folder to enter your Wi-Fi and MQTT credentials if you want to set a default configuration for all the boxes you build. Otherwise you can use the interactive commands or the web configuration tool to set up your box. 

You can find documentation for the Connected Little Boxes platform [here](https://github.com/connected-little-boxes/box-doc).

![Robot picture](images/Pixelbot-small.jpg)

This is the Raspberry Pi PICO version of the Hull Pixelbot. It can also be built for an ESP32 (although at present this is a work in progress). The control software is called HullOS. 

* You can find the Arduino Uno version of the pixelbot [here](https://github.com/HullPixelbot/HullOS). 

* You can find all the Hull Pixelbot repositories on GitHub [here](http://hullpixelbot.com/). 

* You can get the documentation for Python-is (the language used to program the Pixelbot) [here](https://www.hullpixelbot.com/HullOS%20Python-ish%20Specification.pdf),

* You can get the 3D printable and laser cut chassis designs [here](https://github.com/HullPixelbot/Hardware).

* You can find the code for the Connected Little Boxes device which can provide network connectiivty to an Arduino powered Pixelbot [here](https://github.com/connected-little-boxes/HULLOS-X)

## Putting HullOS on a PICO or PICO-W
You can load HullOS onto a Raspberry Pi PICO or PICO-W by:

1. Download the image file from here [PICO-W](/binaries/PICO-HullOS-Z.uf2) and store it on your computer. 
1. Hold down the BOOTSEL button on your PICO. 
1. Plug the PICO into your PC.
1. Release the BOOTSEL button.
1. An external drive will open on your computer. 
1. Drag the image file onto the drive.
1. The PICO will reboot running HullOS

## Connecting to HullOS
You can communicate with HullOS via any terminal program. The baud rate is set to 115200. HullOS will not echo commands as you type them. Press the enter key at the end of each command. The command "*IV" can be used to test a connection, it returns the version number of the software:
```
*IV
4.0.0.0
```
## Battery power
The robot can be powered using 3 or 4 AA sized cells. If you use 4 cells you should make sure that the battery voltage does not exceed 6.5 volts (some batteries give out more than 1.5 volts when brand new) otherwise the PICO may not run. If you use rechargeable batteries you can use 4 with no problem as each battery only produces 1.2 volts. 
## Building the hardware
![Fritzing circuit](images/Hullpixelbot%20Breadboard.png)
You can build the circuit on a breadboard as shown above. The signal pins for the devices connected to the robot are as follows. If you want to change these they are set in the respective ".h" files for the different devices.

| Signal | PICO GPIO | ESP32 GPIO |
|--------|-----------|------------|
| Distance trigger | 17  | xx |
| Distance Echo | 16 | xx |
| Left motor 1| 15| 13 |
| Left motor 2| 14| 12 |
| Left motor 3| 13| 14 |
| Left motor 4| 12| 27 |
| Right motor 1| 11| 26 |
| Right motor 2| 10| 25 |
| Right motor 3| 9| 33 |
| Right motor 4| 8| 32 |
| Neopixel | 6 | 21 |

## Building the HullOS software
The HullOS software is built using Platform.IO. You can edit the platformio.ini file in the repository to select the build version and deployment. At present the PICO code is deployed to the PICO debugger. You only need to do this if you want to change the assignment of the GPIO pins or add/change features or deploy to the ESP32 platform. There are two program source folders, one for PICO and the other for PICO-W. 

## Settings

The device manages setting values for the internal devices.

### Sensors
Values returned by sensors can be bound to output values so that, for example, temperature values can be displayed.

### bme280Sensor

A single BME280 sensor can be connected to the I2C connection fo the device. This has not been tested on the PICO yet. 
```
bme280sensorfitted=no
bme280tempchangetoxmit=0.500000
bme280tempbasenorm=10.000000
bme280templimitnorm=30.000000
bme280presschangetoxmit=10.000000
bme280pressbasenorm=0.000000
bme280presslimitnorm=100.000000
bme280humidchangetoxmit=2.000000
bme280humidbasenorm=0.000000
bme280humidlimitnorm=100.000000
bme280envnoOfAverages=25
```
###Push Button Switch
A device can trigger actions based on the state of a button connected to an input pin. 
```
buttoninputpin=14
buttoninputgroundpin=16
pushbuttonfitted=no
```
### clock
The clock value is only available when the device is connected to a network. The clock can generate alarm events which can trigger actions.
```
timezone=Europe/London
alarm1hour=7
alarm1min=0
alarm1enabled=no
alarm1timematch=yes
alarm2hour=12
alarm2min=0
alarm2enabled=no
alarm2timematch=yes
alarm3hour=19
alarm3min=0
alarm3enabled=no
alarm3timematch=yes
timer1=30
timer1enabled=no
timer1singleshot=yes
timer2=30
timer2enabled=no
timer2singleshot=yes
```

### potSensor
The potentiometer can generate events and values when turned. 
```
potsensordatapin=0
potsensorfitted=no
potsensormillisbetween=100
potsensordeadzone=10
```

### pirSensor
The pir sensor will generate events when it is triggered and cleared. 
```
pirsensorfitted=no
pirsensorinputpin=4
piractivehigh=yes
```

### Distance
The distance sensor provides a distance value used by the robot. It can also be used to trigger actions. 
```
distancedeadzonesize=10
distancereadinginterval=100
distancefitted=yes
distancetriggerpin=17
distancereplypin=16
```
### Processes

Processes control actions and devices. They can consume data but don't produce it. 
### Serial console
This controls the settings of the serial console. If **echoserial** is set to "yes" the console will echo commands entered via the serial connection. If **autosavesettings** is set to "yes" the setting information will be saved whenever a value is changed. This might slow the user interface down a bit and wear out the EEPROM. If you set this to "no" you will have to use the **save** command to save settings after you have changed them. 
```
echoserial=yes
autosavesettings=yes
```

### Controller
The controller takes sensor inputs and triggers process actions. You can disable this behaviour by setting to "no". 
```
controlleractive=yes
```

### hullos
These settings control the behaviour of the HullOS command interpreter. If the interpreter is active it can be made to run the active HullOS program (in the file active.txt) when the device starts. This also sets the default high level langauge (this setting is not used at the moment).
```
hullosactive=yes
hullosrunprogramonstart=yes
hulloslanguage=PythonIsh
```

### messages
These settings controll what messages are displayed. If **speedmessagesactive** is set to yes the system will display messages when a process is running slow. 
```
messagesactive=yes
speedmessagesactive=no
```

### OutPin
You can bind an output pin to a sensor or button input. These settings control the output behaviour. 
```
outpin=16
outpinactivehigh=yes
outpinactive=no
outpininitialhigh=no
```

### MQTT
These are the MQTT settings for a device. Note that the **mqttdevicename** setting value is generated automatically from the device ID. 
```
mqttdevicename=CLB-E661385283457925
mqttactive=yes
mqtthost=mqtt.server.com
mqttport=1883
mqttsecure=no
mqttuser=mqtt user
mqttpwd=*****
mqttpre=lb
mqttpub=data
mqttsub=command
mqttreport=report
mqttsecsperupdate=360
mqttsecsperretry=10
```

### pixel
These are the settings for the neopixel connections
```
pixelcontrolpin=6
noofxpixels=12
noofypixels=1
pixelconfig=1
pixelbrightness=1.000000
pixelname=
```

### printerSettings
You can connect a serial printer. This has not been tested on a PICO yet. 
```
printeron=no
printerbaud=19200
printerdatapin=16
```

### Device Registration
This allows a friendly name to be set when the device is registered with the server. The name is not used at the moment. 
```
friendlyName=
```

### WiFi
These are the Wi-Fi settings for the device. You can enter 5 different settings and the device will look for them. Note that some services (for example clock and MQTT) will not start until a Wi-Fi connection has been established. The present PICO version of the code makes the connection when the device powers up. This will be moved into a background behaviour. 
```
wifiactive=yes
wifissid1=SSID
wifipwd1=*****
wifissid2=
wifipwd2=*****
wifissid3=
wifipwd3=*****
wifissid4=
wifipwd4=*****
wifissid5=
wifipwd5=*****
```

### motor
These are the robot motor coil connections. At present the motors are solely controlled from HullOS and PythonIsh. They can't be bound to sensor based triggers. 
```
motors=yes
motorleftwheeldiam=69
motorrightwheeldiam=69
motorwheelspacing=110
leftmotorpin1=15
leftmotorpin2=14
leftmotorpin3=13
leftmotorpin4=12
rightmotorpin1=8
rightmotorpin2=9
rightmotorpin3=10
rightmotorpin4=11
```

### codeeditorsettings
The PICO can host a program editor for the PythonIsh language. If you enable the editor by setting **codeeditordevicename** to "yes" the device will host a website at http://clb-e661385283457925.local/ (in the case of the device name below). 
Note that if you have both the code editor and the MQTT client running at the same time you might find console response gets a bit sluggish.

```
codeeditordevicename=CLB-E661385283457925
codeeditoron=no
```

Rob Miles


