# Putting HullOS on a device

## PICO and PICO W

You can load HullOS onto a Raspberry Pi PICO or PICO-W by:

1. Download the image file for [PICOW](/.pio/build/rpipico/firmware.uf2) or [PICO2W](/.pio/build/rpipico2/firmware.uf2) and store it on your computer. 
1. Hold down the BOOTSEL button on your PICO. 
1. Plug the PICO into your PC.
1. Release the BOOTSEL button.
1. An external drive will open on your computer. 
1. Drag the image file onto the drive.
1. The PICO will reboot running HullOS

## ESP8266

You can find the bin file for the ESP8266 version [here](/.pio/build/d1_mini/firmware.bin). 

## ESP32 

You can find the bin file for the ESP32 version [here](/.pio/build/ESP32_DOIT/firmware.bin).

## Connecting to HullOS
You can communicate with HullOS via any terminal program. The baud rate is set to 115200. HullOS will echo commands as you type them. You can use the backspace command to back up through a command that you have entered. Press the enter key at the end of each command. The command "*IV" can be used to test a connection, it returns the version number of the software:
```
*IV
4.0.0.0
```

