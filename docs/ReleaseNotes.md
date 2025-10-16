# HullOS-Z Release notes

Soon to be a major motion picture.

# Version 4.0.1.5

## Refined the communication for ESP8266 based devices with an Arduino based motor controller. 

* Added a cache for motor states which records the intention of a motor command
* Added a message timestamping so that status replies from the motor controller are ignored if the request was sent after a motor command has been issued. 
* Refactored the motor driver code so that there is a single motor.c which contains the motor pulse calculation logic and then remoteRobotProcess and stepperDrive files for the two different motor drive options. The process based driver calculates move distances and detects when an invalid move has been requested. The stepper driver generates waveforms on the stepper. TODO: remote motor control works but occasionally drops messages. Needs further investigation. The issue doesn't affect devices where the motor control is integral. 
* Integrated all the motor functions so that they now work on the remote motor controller. This includes the motor stop commands.

# Version 4.0.1.6

## Improving MQTT reload behaviour:

* changed timeouts and reset behaviour to improve the response

## Performing system commands in PythonIsh programs

* system commands (prefixed by the '!' character) can now be stored in PythonIsh programs. They can also be issued in immediate mode at the console. 

# Version 4.0.1.7

## Excess status information

* removed setting display from the status command. This was added by in error when filters were added to the status command.

## Serial comms issues for ESP8266 based robots
* changed default external robot controller baud rate to 19200
* added a remote controller data test during startup - surfaces a message but doesn't raise an error

## Move 0 bug

* fixed move 0 bug on PICO and ESP32. The move 0 command will now stop the motors. A move of 0 length on either motor will stop that motor rather than leave it running at the previous speed. 

## Tidied up PICO motor control

* removed the motor code that used the second core as a motor driver. This is no longer needed as the PICO timer routines now work correctly. 

# Version 4.0.1.8

## Robot Motor Controller

* added a remote terminal connection to the internal robot co-processor. Use the command robot to connect to the Arduino motor driver. Only used on the ESP8266 version of the code with the Arduino motor controller. 

## Settings management

* added the display of settings when clearing to defaults


