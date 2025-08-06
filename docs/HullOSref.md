# HullOS Reference

A command is preceded by the * character to distinguish it from a setting value. Each command is comprised of two characters. The first is the command family, and the second is the command in that family. A command is then followed by an amount of extra data which varies from one command to another. Note that there is no need to put a space between the second command character and a data item. Note also that some data items are optional (indicated by [ ] in the documentation below).

# Information Commands

These commands display information to the output terminal. They all start with the character I.

## Read the Distance Sensor
### *ID	

Display current distance sensor reading in mm. This command does not accept any additional data.

```
*ID
135
```
Show the distance sensor reading. 

## Set the program execution diagnostic level
### *IM	Diagnostics-level	

This command is used to set the level of messaging that is produced by the robot when programs are running. The command is followed by a value that sets the messaging level. The message levels are set as bits in the messaging level value.

* **STATEMENT_CONFIRMATION 1** outputs a confirmation message after each statement. 
This will either be xxOK, where xx is the command, or xxFAIL: reason. 
Some commands, for example conditional jumps, will give additional information.
* **LINE_NUMBERS 2** displays the program position of each statement prior to execution
* **ECHO_DOWNLOADS 4** echoes each downloaded statement during a remote download
* **DUMP_DOWNLOADS 8** 	dumps a downloaded program before executing it

If the corresponding bitfield is set the program will output information as described. Note that these commands can result in significant traffic on the serial connection and are only intended to be used for debugging. 
When the robot is restarted the message level is set to 0 (i.e. all messages are turned off).
```
*IM9
```
This would set the program execution diagnostic level to 1 and cause downloads to be dumped when they have completed. 

## Print the currently executing program 
### *IP	

Print currently active program HullOS code. 
```
*IP
Hullos decoding: IP
Program:
1 : CLl1
2 : PNg
3 : VSd=@distance
4 : CFd<100,l3
5 : PNr
6 : MF-100
7 : CA
8 : CLl3
9 : CJl1
10 : CLl2
11 : Program size: 66
```
The current program is reading the distance sensor, turning the pixel red and moving away if the distance is less than 100mm. 
## Show program state and diagnostics level

### *IS

Shows the program status and diagnostic level as a two digit number. The first digit gives the execution state:

```
0	PROGRAM_STOPPED
1	PROGRAM_PAUSED
2	PROGRAM_ACTIVE
3	PROGRAM_AWAITING_DELAY_COMPLETION
4	PROGRAM_AWAITING_MOVE_COMPLETION
```
The second digit gives the diagnostic level as set above. 

```
*IS
20
```

## Show firmware version
### *IV	

Displays the current firmware version. 
```
*iv
4.0.0.1
```

# Movement Control Commands

These commands are associated with robot movement. They are not available if the **PROCESS_MOTOR** symbol has not been defined for the build. This is defined in the **PlatformIO.ini** file. 

## Move in an arc with radius, angle, and optional time
### *MA	Radius, angle[, time]

![Diagram showing how the robot rotation is expressed](/images/robotrotation.png)

The figure above shows how the robot moves. If the radius is positive the curve will be about a point which is to the right of the robot, and the robot will turn clockwise as it moves. If the radius is negative the curve will be about a point that is to the left of the robot, and the robot will turn counterclockwise.  The centre point of the arc is on a line drawn between the two wheels. If the angle is positive the robot will turn clockwise.
If the **STATEMENT_CONFIRMATION** flag is set the robot replies with:
```
MAOK
```

Note that this does not meant that the move command has been completed, rather that the robot has received and understood the command and has started moving. 

If the time requested is not possible because the robot cannot move that quickly the move will not take place. If the **STATEMENT_CONFIRMATION** flag is set the robot replies with an error message:

```
MAFail
```
As an example the following statement would cause the robot to traverse a complete circle and take a minute to perform this:

```
*MA100,360,600
```
The radius of the circle is 100mm, the distance around the circle is 360 degrees and the move will take 600 ticks (remember that a tick is a 10th of a second)
If the value of the radius is given as 0 the robot will rotate about its centre:

```
*MA0,90,600
```

This would cause the robot to rotate 90 degrees about its centre over 600 ticks.

## Check if motors are moving
### *MC

This command can be used to determine whether the robot has completed a requested move operation. If the motors are still moving the robot replies with: 

```
MCmove
```

If the motors are stopped the robot replies with: 

```
MCstopped
```

Note that these messages are sent irrespective of the state of the **STATEMENT_CONFIRMATION** flag.

```
*MC
MCstopped
```

## Move forward distance [d] with optional time [t] in tenths of a second

### *MF	Distance in mm[, time in tenths]	

The robot moves the number of steps given by the decimal value distance. If the number is negative the robot moves backwards that number of steps. If the robot is already moving this command will replace the existing one.  The command can be followed by a comma and an optional time value that give the number of ticks (tenths of a second) that the move will take to complete. If the time value (and the comma) are omitted the robot will move as fast as possible. 

```
*MF100
```

This would move the robot forwards 100 mm as quickly as possible. 

```
*MF150,100
```

This would move the robot 150 mm and take 10 seconds to complete (remember that a “tick” is a tenth of a second). 
The robot starts moving as soon as the command is received. If the move can be performed and the **STATEMENT_CONFIRMATION** flag is set the robot replies with:

```
MFOK
```

Note that this does not meant that the move command has been completed, rather that the robot has received and understood the command and has started moving. If the time requested is not possible because the robot cannot move that quickly (for example move 100 mm in 1 tick) the move will not take place. If the **STATEMENT_CONFIRMATION** flag is set the robot replies with an error message:

```
MFFail
```

## Move motors: left and right distances, and optional time
### *MM	Left distance, right distance[, time]	
This statement provides direct control of the speed and distance moved of each individual wheel.  It is followed by three integer values that give the distance to be moved for each motor and the time to be taken for the move. When the motor has moved the specified distance it stops. The distance values can be signed, in which case the motor will run in reverse. 

If the **STATEMENT_CONFIRMATION** flag is set the robot replies with:

```
MMOK
```

Note that this does not meant that the move command has been completed, rather that the robot has received and understood the command and has started moving.  If the turn could not be performed a fail message is generated followed by a value which indicates the reason for the failure.

```
MMFail: d
```

The value of d gives the reason why the move could not take place:
* 1	Left_Distance_Too_Large
* 2	Right_Distance_Too_Large
* 3	Left_And_Right_Distance_Too_Large

## Rotate robot a certain angle, with optional time
### *MR	Angle[, time]	

The robot rotates clockwise the given number of degrees given by the value angle (there are 360 degrees in a circle). If the number is negative the robot rotates anticlockwise that number of steps. If the robot is already moving this command will replace the existing one. If the **STATEMENT_CONFIRMATION** flag is set the robot replies with:

```
MROK
```

Note that this does not meant that the move command has been completed, rather that the robot has received and understood the command and has started moving. If the time is omitted the robot will perform the rotation as quickly as possible. 
If the time requested is not possible because the robot cannot rotate that quickly (for example rotate 200 mm in 1 tick) the move will not take place. If the **STATEMENT_CONFIRMATION** flag is set the robot replies with an error message:

```
*MRFail
```

## Stop motors
### *MS

Stops any current move behaviour.  
If the **STATEMENT_CONFIRMATION** flag is set the robot replies with:

```
MSOK
```

## View current wheel configuration
### *MV

This allows you to view the current settings of the wheel configuration values. 

```
*mv
Wheel settings
Left diameter: 69
Right diameter: 69
Wheel spacing: 110
```
All dimensions are given in millimeters. 

## 	Configure wheels: left/right diameters, wheel spacing (mm)
## *MW	Left diameter, right diameter, spacing (mm)

This command can be used to configure the dimensions and spacing of the wheels fitted to the robot. The values are used to calculate all the robot movement. The values are stored in EEPROM inside the robot. The very first time the robot is turned on the dimensions are set to their default values:

* Wheel diameter	69mm
* Wheel spacing	110mm

These are the dimensions based on the stl files for the robot chassis. If the **STATEMENT_CONFIRMATION** flag is set the robot replies with:

```
MWOK
```

Note that these values are not validated in any way, so if you put silly dimensions you may find that silly things happen. And quite right too.

# Pixel Control Commands

These commands control the pixel display. Note that if a foreground program is running this may overwrite any changes made from the console. 

## Set flicker speed
### *PF	Flicker speed	

Sets the flicker update speed for the pixels. The larger the number, the faster the pixels will change colour. The speed is given in the range 1 to 20. A speed of 1 is very gentle, a speed of 20 is manic. When the program starts the speed is set to 8. Values outside the range 1-20 are clamped.  If the **STATEMENT_CONFIRMATION** flag is set the robot replies with:

```
PFOK
```

## Set candle color by name (r/g/b/c/m/y/w/k)
### *PN	Color name (r/g/b/c/m/y/w/k)	

Sets the pixel display to show a flickering candle of the given colour. This sets the colour of all the pixels in the display. The required colour is specified by a single character:

* R – red
* G – green
* B – blue
* Y – yellow
* M - magenta
* C – cyan
* W – white
* K - black

If the **STATEMENT_CONFIRMATION** flag is set the robot replies with:

```
PCNOK
```

If any of the values are missing an appropriate message is displayed, for example: 

```
PC
```
would generate the error:

```
FAIL: mising colour in set colour by name
```

Note that this message is only output if the **STATEMENT_CONFIRMATION** flag is set. If the colour selection character is invalid the following message is displayed. 

```
FAIL: invalid colour in set colour by name
```

Note that these messages are only output if the **STATEMENT_CONFIRMATION** flag is set.

# Program Control Commands

These commands are performed when the robot is running a program sequence.  They can be entered directly into the robot, but they won’t do much.

## Pause when the motors are active
## *CA

The program pauses while the motors are active. This allows a program to wait until a movement has completed. If the STATEMENT_CONFIRMATION flag is set the robot replies with:

```
CAOK
```

Note the reply is sent when the command has been received not when the robot has completed the pause.

## Delay
## *CDddd
The program pauses for the number of ticks given by the decimal value ddd. The number of ticks can be omitted:

```
CD
```

The program repeats the previous delay. If there was no previous delay the program does not delay. The delay starts as soon as the command is received. A tick is a tenth of a second. 
If the **STATEMENT_CONFIRMATION** flag is set the robot replies with:

```
CDOK
```

Note the reply is sent when the command has been received not when the robot has completed the delay. Ongoing move commands will continue to complete, and the robot will respond to other direct commands.

## Label
### *CLcccc
This statement declares a label which can be used as the destination of a jump instruction. The label can be any number of characters and will be terminated by the end of the statement. If the **STATEMENT_CONFIRMATION** flag is set the robot replies with:
```
CLOK
```
When a jump is performed the interpreter searches for the first occurrence of the specified label. If the same label is declared more than once the first label will be the one that is used. 

```
*CLL100
```

The statement above creates a label called L100.

## Jump
### *CJcccc

This statement causes execution of the program to continue from the given label. The label can be any number of characters and will be terminated by the end of the statement. The program must contain the label requested, or the program will stop. 

```
*CJL100
```

The above statement would transfer program execution to label L100.

If the **STATEMENT_CONFIRMATION** flag is set the console displays the following messages:

If the label is found and the jump performed:
```
CJOK
```

If the label is missing from the program:
```
CJFAIL: no dest
```

## Coin toss jump
### *CCcccc

This statement causes execution of the program to continue from the given label fifty percent of the time. The rest of the time the program continues. The label can be any number of characters and will be terminated by the end of the statement. The program must contain the label requested, or the program will stop. If the **STATEMENT_CONFIRMATION** flag is set the following messages are displayed:

If the label is found and the jump performed:

```
CCjump
```

If the label is found and the jump not performed:
```
CCcontinue
```

If the label is missing from the program:

```
CCFAIL: no dest
```

## Jump when motors inactive
## *CIccc

The program will jump to the given label if the motors are inactive.  The robot replies with:

*CIOK
Measure Distance: *CM
*CMddd,cccc
This statement causes execution of the program to continue from the given label if the distance sensor reading is less than the given distance value. The value is given in centimetres. 
The label can be any number of characters and will be terminated by the end of the statement. The program must contain the label requested, or the program will stop. If the distance measured is greater than the given value, the program continues at the next statement.
If the STATEMENT_CONFIRMATION flag is set the robot replies with the following messages:
If the label is not present in the instruction the robot replies with:
CMFail: missing dest
If the label is found the robot replies with:
CMFail: label not found
If the label is found and the distance is less the robot replies with:
CMjump
If the label is found and the distance is greater the robot replies with:
CMcontinue
Compare Condition: *CC
*CCeeee,cccc
This statement causes execution of the program to continue from the given label if the given expression to True. 
The label can be any number of characters and will be terminated by the end of the statement. The program must contain the label requested, or the program will stop. If the given expression is false the program continues at the next statement.
If the STATEMENT_CONFIRMATION flag is set the robot replies with the following messages:
If the label is not present in the instruction the robot replies with:
CCFail: missing dest
If the label is found the robot replies with:
CCFail: label not found
If the label is found and the distance is less the robot replies with:
CCjump
If the label is found and the distance is greater the robot replies with:
CCcontinue
Compare Condition not true: *CN
*CFeeee,cccc
This statement causes execution of the program to continue from the given label if the given expression evaluates to True. 
The label can be any number of characters and will be terminated by the end of the statement. The program must contain the label requested, or the program will stop. If the given expression is false the program continues at the next statement.
If the STATEMENT_CONFIRMATION flag is set the robot replies with the following messages:
If the label is not present in the instruction the robot replies with:
CCFail: missing dest
If the label is found the robot replies with:
CCFail: label not found
If the label is found and the distance is less the robot replies with:
CCjump
If the label is found and the distance is greater the robot replies with:
CCcontinue




Command	Command Data	Description
*CA	None	Pause while motors are active
*CC	Label name	Jump to label with 50% chance (coin toss)
*CD	Delay time (tenths of second)	Delay in tenths of seconds
*CF	Condition, Label name	Test condition, jump if false
*CI	Label name	Jump to label if motors are inactive
*CJ	Label name	Jump to label
*CL	Label name	Label (no action, for jump targets)
*CM	Distance, Label name	Measure distance, jump if below threshold
*CT	Condition, Label name	Test condition, jump if true
Remote Management Commands
Command	Command Data	Description
*RC	None	Clear stored program
*RH	None	Halt execution
*RM	None	Begin remote code download
*RP	None	Pause execution
*RR	None	Resume execution
*RS	None	Start program execution
Sound Commands
Command	Command Data	Description
*ST	Frequency, duration, wait (W/N)	Play tone: frequency, duration, wait[n/w]
Variable Management Commands
Command	Command Data	Description
*VC	None	Clear all variables
*VS	Variable name, value	Set a variable
*VV	Variable name	View a variable
Write Output Commands
Command	Command Data	Description
*WL	None	Print newline
*WT	Text	Print text
*WV	Variable name	Print variable value

