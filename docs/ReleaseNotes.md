# HullOS-Z Release notes

Soon to be a major motion picture.

# Version 4.0.1.5

Refined the communication for ESP8266 based devices with an Arduino based motor controller. 

* Added a cache for motor states which records the intention of a motor command
* Added a message timestamping so that status replies from the motor controller are ignored if the request was sent after a motor command has been issued. 
* Refactored the motor driver code so that there is a single motor.c which contains the motor pulse calculation logic and then remoteRobotProcess and stepperDrive files for the two different motor drive options. The process based driver calculates move distances and detects when an invalid move has been requested. The stepper driver generates waveforms on the stepper. TODO: remote motor control works but occasionally drops messages. Needs further investigation. The issue doesn't affect devices where the motor control is integral. 
* Integrated all the motor functions so that they now work on the remote motor controller. This includes the motor stop commands.
