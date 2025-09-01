# HullOS Python-ish 

HullOS programs can be written at two levels. 
At the high level you can use HullOS scripting commands in a language we called “Python-ish”. 
At the low level you can use HullOS "star" commands. see (HullOSRef.md) Each star command equates to a single low-level command that is interpreted when the device runs the program. A scripting command may be comprised of one or more "star" commands. A star (*) command provides access to low level programming features. Star commands are pre-ceded by the * character. They can be used at any point in a program. HullOS programs support the following programming constructions

* named variables and simple expressions
* conditional execution using if – else
* nested looping constructions
* direct access to hardware resources
* control over the robot stepper motors
* print messages to the serial port
* send messages over MQTT
## Immediate mode and program mode
The robot will accept individual commands at any time, even when a program is running. Some commands can be used to control program execution. These direct commands can also be used in scripts. It you want to enter PythonIsh programs you use the command **pythonish** to start a Pythonish session on the terminal:
```
pythonish
P>
```
The robot will now respond to PythonIsh commands. Note that you can 

## Set the colour of the pixel
You can set the colour of the pixel to one of a number of colours:
```
red
```
The above statement will turn the robot pixel red. The colours that can be used are:

```
red green blue cyan magenta yellow white black
```

A greater range of colours can be selected using the star commands. Take a look at the *PC command if you want more colours. 

## Make the robot angry or happy
The rate at which the pixels flash on the robot can be controlled by two program commands:

```
angry
```

This tells the robot to go into "angry" mode. The pixel will flash in an angry way.

```
happy
```

This tells the robot to go into "happy" mode. The pixel flashing will slow down as the robot becomes more relaxed. If you want to create even more subtle robot moods, take a look at the *PF command. 
## Move the robot
The **move** command can be used to start the robot moving, and to set a distance for the move. Move commands can also be configured to move in a particular time which, effectively, sets the speed of the motion.



### Starting moving 
```
move
```
The above statement will start the robot moving forwards. Note that the robot will continue moving until you give it a different movement command.
### Moving a distance
You make the robot move a particular distance by giving that distance after the **move** command:
```
move 100
```
The above statement will cause the robot to move 100 mm and then stop.

Note that the movement may not be very accurate, you can improve accuracy by using the ***MV** command to tell HullOS the size of your robot wheels and their spacing.
The **move** statement can be used to make the robot move backwards by using a negative value:
```
move -200
```
The above statement would move the robot backwards 200 mm. 
### Moving a distance in a given time
You can make the robot move a particular distance in a given time by adding an **intime** value:
```
move 100 intime 100
```
The value following the **intime** keyword is a number of tenths of a second that the move should take to complete. The above statement should cause the robot to take ten seconds to move 100 mm. 

Note that if you give a silly time, for example move 1000 millimetres in a one tenth of a second, the robot will not move. 

## Turn the robot
The **turn** command can be used to make the robot turn. It turns by moving one-wheel forwards and the other backwards, so that it rotates on the spot. If you want the robot to follow a curved trajectory, take a look at the **arc** command, which is next. 
### Starting turning
```
turn
```
This command would cause the robot to start turning clockwise. Note that the robot will continue turning until you give it a different movement command.
### Turning an angle
The turn command can be followed by the value of an angle which the robot is to turn. 
```
turn 90
```
This command would cause the robot to turn 90 degrees (a right angle) clockwise. You can turn anti-clockwise by giving a negative value to turn. 
### Turn a distance in a given time
You can make the robot turn a particular angle in a given time by adding an **intime** value:
```
turn 360 intime 600
```
The value following the **intime** keyword is a number of tenths of a second that the turn should take to complete. The above statement should cause the robot to take 60 seconds to perform a complete rotation. 

Note that if you give a silly time, for example turn 1000 degrees in a one tenth of a second, the robot will not turn at all. 
### Move over an arc
This statement allows the robot to move in an arc. This can be a bit hard to understand. Experiment with arcs to understand how they work. 
```
arc 0 angle 90
```

The first parameter is the **radius** of the arc, the second is the **angle** giving the angular distance around the arc.

 ![Robot with an radius value and an angle showing the point about which the rotation occurs](/images/robotrotation.png)

The figure above shows how this works. If the radius is positive the curve will be about a point which is to the right of the robot, and the robot will turn clockwise as it moves. If the radius is negative the curve will be about a point that is to the left of the robot, and the robot will turn counterclockwise.  The centre point of the arc is on a line drawn between the two wheels. If the angle is positive the robot will turn clockwise. 

In the statement above the radius is 0, i.e. the centre of the arc is the point midway between the wheels. This means that the robot would rotate about its axis. 
### Move over an arc in a given time
The **arc** command can be followed with an **intime** value to give the number of tenths of a second it should take for the robot to complete the arc.
```
arc 1000 angle 360 intime 600
```
This statement would cause the robot to describe a complete circle of 1000mm radius, and take a minute to do this. 

### Waiting for motion to complete

If you want your pythonish program to wait until the motion has completed before executing the next program statement use the command *mwait* for example:-
```
begin
println "moving"
move 1000
mwait
println "stopped"
end
```

To get on with something else whilst a motion is taking place use the built-in variable @moving. For example:-
```
begin
move 1000
while @moving==1
 if @distance < 100
   red
   move -50
   turn 90
   move 50
 else
   green
end
```

## Make a sound
The robot can be fitted with a small speaker that can produce quite annoying sounds. You can use the **sound** command to make these:

```
sound 1000
```
The value following the sound command is the frequency of the sound to be made, in Hz. It is best not to make sounds lower than 100 Hz or higher than about 5000 Hz, although you are free to experiment. The sound quality is not great, but I quite like it. Sounds play for half a second. 
### Play a note
If you want to play a note in a tune you can follow the **sound** frequency with a **duration**. The duration is given in thousandths of a second:
```
sound 2000 duration 1000
```
This statement would play a note of 2000 Hz for a second. 
### Play a note and wait
The **sound** command will start a note playing. It will not wait for the note to finish. This can make it hard to play tunes, because you want your program to wait for a note to finish before playing the next one. You can add a **wait** command to the end of a sound command to tell the program to wait for this note to finish playing before the next statement in a program is performed.
```
sound 1500 duration 500 wait
```
This statement would play a note of 1500 Hz for half a second. It would then move on to the next statement in the program. 
## Delay
The delay statement is frequently used in programs to, er, delay them. The delay statement is followed by the size of the delay, in tenths of a second.
```
delay 5
```
The above statement would cause a robot program to delay for half a second. Note that any robot movement or sound playback would continue. 

## Printing values
Your programs can print information back to the serial connection. Of course, printed messages will only mean anything if something is actually connected to the serial port. If you use a terminal program to connect to your robot you will see your printed messages displayed in the output window. 
### Print a string
The print command will print a string of text to the serial port.
```
print "hello"
```
The text to be printed is enclosed in double quotes, as shown above. If you print something else it will be appended to the line that is being printed.

A **print** statement can also print the value of an expression.
```
print x
```
This command would print the contents of the variable **x**. 
### Print and take a new line
If you want to print a statement and take a new line after it you can use the **println** command:
```
println "This is followed by a new line"
```
The **println** statement works in exactly the same way as **print**, except that it prints a new line after it has finished printing. 
## Values and variables
A HullOS program can store and manipulate numeric values by means of variables. Each variable can hold a singed integer value. A variable has a name which is expressed as a letter, followed by a sequence of letters and numbers. You create a variable and set a value to it at the same time:
```
go=50
```
This would create a variable called **go** which holds the value **50**. Variables can be used everywhere we have seen a number in previous examples. In other words:
```
move go
```
..would move the robot 50 mm.
Variables can be used in simple expressions:
```
go = go + 10
```
This would increase the value in **go** by **10**. Note that expressions must be very simple and take the form of two operands (in the case above these would be the values of go and 10) and one operator (in the case above this would be the operator +). If you want to do more complicated things with values you'll have to spread the calculation over several statements. 
The following arithmetic operators are available.

* Addition +
* Subtraction -
* Multiplication *
* Division /
* Modulus %
## System variables 
HullOS provides a number of system variables that give access to values returned by the robot sensors. A system variable has a name which begins with the @ character. 
### Read the distance sensor value
The **@distance** variable returns the current reading of the distance sensor as an integer value in mm. You can use it anywhere you can use a variable, although you can't assign values to it. That would be silly.
```
println @distance
```
This statement would print the current reading.
### Detect motor movement
The **@moving** variable returns **1** if the motors are moving and **0** if they are not. You can use this to determine whether or not a given movement accept has been accepted.
```
println @moving
```
This statement would print 1 if the motors are moving and 0 if they are not. 
### Get random numbers
The **@random** variable returns a random number between 1 and 12. You can use this to give your robot random behaviours.
```
move @random*10
```
This statement make the robot move a random distance between 10 and 120 mm. 
# HullOS Scripting
Individual commands can be sent to robots at any time, but you can also enter a program into a device controlled by HullOS. HullOS programs are expressed as a series of statements. Each statement must be given on a single line. A HullOS script is enclosed in the keywords **begin** and **end**. Programs can contain comments which are expressed using the # character at the start of the line:
```
begin
# This is an empty program
end
```
The case of a keyword is ignored. 
```
BEGIN
# This is a shouty empty program
END
```
## Conditional Statements
A program can make decisions using an **if** construction. The **if** construction is followed by a Boolean expression which can be either **true** or **false**. If the condition is **true** the statement following the **if** condition is performed. The conditional statement can be followed by an **else** element which is obeyed if the statement is false
```
begin
if @distance < 100
  red
else
  green
end
```
The program above would turn the pixel red if the distance sensor was reading a distance of less than 100, otherwise the pixel would be turned green.
The language uses "Python style" indenting to indicate which statements are controlled by a condition. 
```
begin
if @distance < 100
  sound 2000
  red
else
  sound 1000
  green
end
```
In the statements above the sound and the colour selection statements would be controlled by conditions. Conditions can be nested inside each other. 
```
begin
silent=1
if @distance < 100
  if silent==0
    sound 2000
  red
else
  if silent==0
    sound 1000
  green
end
```
This program uses a variable called **silent** to turn the sounds off. If the value of **silent** is 1 the sound is not played. 
You don't need to provide the else if your program doesn't need it. 
## Logical operators
The following logical conditions are available.
* greater than >
* less than <
* greater than or equal to >=
* less than or equal to <=
* equal to ==
* not equal to !=

As with arithmetic expressions, a logical expression can only be comprised of two operands and one logical operator. 
## Indent statements in conditions
The indenting for a given block level must be consistent within that block. In other words, if you indent the first statement after the if with two extra spaces, you must indent all the statements controlled by that if with two extra spaces as well. This means that the following statement is invalid:
```
begin
if @distance < 100
  sound 2000
   red
else
  sound 1000
  green
end
```
The highlighted red statement is indented too far. This will cause the program to be rejected by the robot and an error to be displayed. 
```
Line:  4 Error: 34    red
```
The error gives the line number and the number of the error that was detected. You can find a table of error numbers and their explanation at the end of this document. 
You also use indenting when creating loops, as in the next section.
## Creating loops
A loop is a piece of code that is repeated multiple times. There are two looping constructions, **forever** and **while**. 
### Loop code forever
A loop which continues forever will never stop. The **forever** statement is followed by the block of statements to be repeated indefinitely. 
```
begin
forever
  red
  delay 5
  blue
  delay 5
end
```
The above program would flash the pixel red and blue, with a half second delay between each flash. Note that you can place statements after the loop, but these will never be obeyed.
### Loop on condition
The **while** statement is used to repeat a block of statements while a condition is true:
```
begin
while @distance>99
  green
magenta
end
```
The above program would display green until the distance value falls below 101. The it would display magenta.
### Break out of loops
A program can use the **break** statement to break out of a loop. When a program breaks out of a loop the program continues running at the statement following the last statement of the loop.
```
begin
green
forever
  if @distance<100
    break
red
end
```
The above program uses the **break** statement to break out of the forever loop when the distance sensor reading drops below 100. You can break out of forever or while loops. A break will only break out of one enclosing loop.
### Continue loops
The **continue** statement will cause a loop to return to the top of the loop and continue from there. If the loop is a **while** loop the loop condition will be tested. 
```
begin
forever
  if @distance<100
    red
    continue
  sound 1000
  blue
red
end
```
The **continue** statement will cause the loop to be restarted, meaning that the sound is only made when the distance value is less than 100. 
## Entering Immediate Commands
You can enter a PythonIsh program manually from a serial terminal connected to the serial port on the robot by pressing cntrl-C to get the following prompt:-
```
P>
```

From there you can enter pythonish commands.

To exit just enter something like *IP which will list the current program.
