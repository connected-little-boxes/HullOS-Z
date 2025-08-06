# Building a Robot

![Robot picture](/images/Pixelbot-small.jpg)

You can use the HullOS software to control a small robot which is moved by small stepper motors. 
## Battery power
The robot can be powered using 3 or 4 AA sized cells. If you use 4 cells you should make sure that the battery voltage does not exceed 6.5 volts (some batteries give out more than 1.5 volts when brand new) otherwise the PICO may not run. If you use rechargeable batteries you can use 4 with no problem as each battery only produces 1.2 volts. 
## Building the hardware
![Fritzing circuit](/images/Hullpixelbot%20Breadboard.png)
You can build the circuit on a breadboard as shown above. The signal pins for the devices connected to the robot are as follows. If you want to change these they are set in the respective ".h" files for the different devices.

These are the robot motor coil connections. At present the motors are solely controlled from HullOS and PythonIsh. They can't be bound to sensor based triggers. These are the setting items for the robot motors. The diameter and spacing dimensions are in mm. 

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