# Configuration

## Configuration via terminal
![Simpleterm Screenshot](/images/simpleterm.png)
You  connect a device to a serial port to configure the network settings. You can use the browser based terminal at [https://www.connectedlittleboxes.com/simpleterm](https://www.connectedlittleboxes.com/simpleterm). Enter commands into the command line underneath the output display. Then press the Send button to send the command to the box. These are the settings you will need to make:

```
wifissid1=
wifipwd1=
wifiactive=yes

mqtthost
mqttuser=
mqttpwd=
mqttport=1883
mqttsecure=no
mqttactive=yes

```
If you assemble the setting information into a text file you can paste all setting lines into the command input line. 

## Configuration via settings file

Edit the **defaults.hsec** file in the **src** folder to enter your Wi-Fi and MQTT credentials if you want to set a default configuration for all the boxes you build using PlatformIO. 
