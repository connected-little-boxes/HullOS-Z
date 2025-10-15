#include <EEPROM.h>
#include <Arduino.h>
#include "string.h"
#include "errors.h"
#include "settings.h"
#include "debug.h"
#include "ArduinoJson-v5.13.2.h"
#include "utils.h"
#include "sensors.h"
#include "processes.h"
#include "controller.h"
#include "console.h"
#include "pixels.h"
#include "Motors.h"
#include "stepperDrive.h"
#include "distance.h"
#include "registration.h"
#include "HullOSCommands.h"
#include "HullOSVariables.h"
#include "HullOSScript.h"
#include "HullOS.h"
#include "RockStar.h"
#include "PythonIsh.h"
#include "otaupdate.h"
#include "utils.h"
#include "version.h"
#include "mqtt.h"

#ifdef PROCESS_REMOTE_ROBOT_DRIVE
#include "remoteRobotProcess.h"
#endif

// #define DIAGNOSTICS_ACTIVE
// #define PROGRAM_DEBUG
// #define HullOS_DEBUG

char HullOScodeRunningCode[HULLOS_PROGRAM_SIZE];
char *decodePos;
char *decodeLimit;

char HullOScodeCompileOutput[HULLOS_PROGRAM_SIZE];
char *compiledPos;
char *compiledLimit;

bool writeByteIntoHullOScodeCompileOutput(uint8_t byte, int pos)
{
    if (pos > HULLOS_PROGRAM_SIZE)
        return false;
    HullOScodeCompileOutput[pos] = byte;

    return true;
}

char HullOSOutputBuffer[HULLOS_PROGRAM_COMMAND_LENGTH];

int HullOSOutputBufferPos;

void resetHullOSOutputBuffer()
{
    HullOSOutputBufferPos = 0;
}

void HullOSProgramoutputFunction(char ch)
{

#ifdef HullOS_DEBUG

    displayMessage(F("Got a HullOS program ch %c\n"), ch);

    if (ch == STATEMENT_TERMINATOR)
    {
        displayMessage(F("Writing a line terminator\n"));
    }
    else
    {
        displayMessage(F("Writing a %c %d\n"), ch, ch);
    }

#endif

    if (HullOSOutputBufferPos >= HULLOS_PROGRAM_COMMAND_LENGTH - 2)
    {
        displayMessage(F("HullOS command too long - ignoring\n"));
        resetHullOSOutputBuffer();
        return;
    }

    if (ch == STATEMENT_TERMINATOR)
    {
        HullOSOutputBuffer[HullOSOutputBufferPos++] = STATEMENT_TERMINATOR;
        HullOSOutputBuffer[HullOSOutputBufferPos++] = 0;

#ifdef HullOS_DEBUG
        displayMessage(F("Got a statement to perform: %s\n"), HullOSOutputBuffer);
#endif
        hullOSActOnStatement(HullOSOutputBuffer, HullOSOutputBuffer + HullOSOutputBufferPos);
        resetHullOSOutputBuffer();
        return;
    }
    HullOSOutputBuffer[HullOSOutputBufferPos++] = ch;
}

void displayProgramState()
{
    switch (programState)
    {
    case PROGRAM_STOPPED:
        displayMessage(F("Program Stopped"));
        break;
    case PROGRAM_PAUSED:
        displayMessage(F("Program Stopped"));
        break;
    case PROGRAM_ACTIVE:
        displayMessage(F("Program Stopped"));
        break;
    case PROGRAM_AWAITING_DELAY_COMPLETION:
        displayMessage(F("Program Stopped"));
        break;
    case PROGRAM_AWAITING_MOVE_COMPLETION:
        displayMessage(F("Program Stopped"));
        break;
    }
};

ProgramState programState = PROGRAM_STOPPED;

InterpreterState interpreterState = EXECUTE_IMMEDIATELY;

bool storingProgram()
{
    return interpreterState == STORE_PROGRAM;
}

unsigned char diagnosticsOutputLevel = 0;

unsigned long delayEndTime;

///////////////////////////////////////////////////////////
/// remote filename
///////////////////////////////////////////////////////////

char HullOScommandsFilenameBuffer[REMOTE_FILENAME_BUFFER_SIZE];

bool getHullOSFileNameFromCode()
{

    if (*decodePos == STATEMENT_TERMINATOR)
    {
        return false;
    }

    int pos = 0;
    char *ch = decodePos;
    while (pos < REMOTE_FILENAME_BUFFER_SIZE - 1)
    {

        if (*ch == STATEMENT_TERMINATOR)
        {
            HullOScommandsFilenameBuffer[pos] = 0;
            return true;
        }

        HullOScommandsFilenameBuffer[pos] = *ch;
        ch++;
        pos++;
    }

    return false;
}

void clearHullOSFilename()
{
    HullOScommandsFilenameBuffer[0] = 0;
}

bool HullOSFilenameSet()
{
    return HullOScommandsFilenameBuffer[0] != 0;
}

///////////////////////////////////////////////////////////
/// Serial comms
///////////////////////////////////////////////////////////

int CharsAvailable()
{
    return Serial.available();
}

int (*decodeScriptChar)(char b);

// Current position in the program of the execution
int programCounter;

// Write position when downloading and storing program code
int programWriteBase;

// Write position for any incoming program code
int bufferWritePosition;

// Dumps the running program

void dumpRunningProgram()
{
    int progPos = 0;
    int lineNumber = 2;

    displayMessage(F("Program:\n1 : "));

    unsigned char b;

    while (true)
    {
        b = HullOScodeRunningCode[progPos++];

        if (b == STATEMENT_TERMINATOR)
            displayMessage(F("\n%d : "), lineNumber++);
        else
            displayMessage(F("%c"), b);

        if (b == PROGRAM_TERMINATOR)
        {
            displayMessage(F("\nProgram size: %d\n"), progPos);
            break;
        }

        if (progPos >= HULLOS_PROGRAM_SIZE)
        {
            displayMessage(F("\nProgram end\n"));
            break;
        }
    }
}

// Starts a program running

void startProgramExecution(bool clearVariablesBeforeRun)
{

#ifdef PROGRAM_DEBUG
    displayMessageWithNewline(F("Starting program execution"));
#endif

    if (clearVariablesBeforeRun)
    {
        clearVariables();
#ifdef PROCESS_PIXELS
        setAllLightsOff();
#endif
    }

    programCounter = 0;
    resetCommand();
    programState = PROGRAM_ACTIVE;
}

// RH - remote halt

void haltProgramExecution()
{
#ifdef PROGRAM_DEBUG
    displayMessage(F("Ending program execution at: "));
    displayMessageWithNewline(programCounter);
#endif

#ifdef PROCESS_MOTOR
    motorStop();
#endif
    programState = PROGRAM_STOPPED;
}

// RP - pause program

void pauseProgramExecution()
{
#ifdef PROGRAM_DEBUG
    displayMessage(F(".Pausing program execution at: "));
    displayMessageWithNewline(programCounter);
#endif

    programState = PROGRAM_PAUSED;

#ifdef DIAGNOSTICS_ACTIVE

    if (diagnosticsOutputLevel & STATEMENT_CONFIRMATION)
    {
        displayMessage(F("RPOK"));
    }

#endif
}

// RR - resume running program

void resumeProgramExecution()
{
#ifdef PROGRAM_DEBUG
    displayMessage(F(".Resuming program execution at: "));
    displayMessageWithNewline(programCounter);
#endif

    if (programState == PROGRAM_PAUSED)
    {
        // Can resume the program
        programState = PROGRAM_ACTIVE;

#ifdef DIAGNOSTICS_ACTIVE

        if (diagnosticsOutputLevel & STATEMENT_CONFIRMATION)
        {
            displayMessage(F("RROK"));
        }
#endif
    }
    else
    {
#ifdef DIAGNOSTICS_ACTIVE

        if (diagnosticsOutputLevel & STATEMENT_CONFIRMATION)
        {
            displayMessage(F("RRFail:"));
            displayProgramState();
        }
#endif
    }
}

lineStorageState lineStoreState;

void resetLineStorageState()
{
    lineStoreState = LINE_START;
}

void storeProgramByte(byte b)
{
    writeByteIntoHullOScodeCompileOutput(b, programWriteBase++);
}

void clearStoredProgram()
{
    writeByteIntoHullOScodeCompileOutput(PROGRAM_TERMINATOR, 0);
}

// Called to start the download of program code
// each byte that arrives down the serial port is now stored in program memory
//
void startDownloadingCode()
{
#ifdef PROGRAM_DEBUG
    displayMessageWithNewline(F(".Starting code download"));
#endif

    // clear the existing program so that
    // partially stored programs never get executed on power up

    clearStoredProgram();

    interpreterState = STORE_PROGRAM;

    programWriteBase = 0;

    resetLineStorageState();

#ifdef PROCESS_PIXELS
    startBusyPixel(128, 128, 128);
#endif

#ifdef DIAGNOSTICS_ACTIVE

    if (diagnosticsOutputLevel & STATEMENT_CONFIRMATION)
    {
        displayMessage(F("RMOK"));
    }

#endif
}

// #define STORE_RECEIVED_BYTE_DEBUG

void dumpProgram(char *start)
{
    while (true)
    {
        char ch = *start;
        if (ch == 0 || ch == PROGRAM_TERMINATOR)
        {
            break;
        }
        if (ch == STATEMENT_TERMINATOR)
        {
            displayMessage(F("\n"));
        }
        else
        {
            displayMessage(F("%c"), ch);
        }
        start++;
    }
}

void endProgramReceive(bool save)
{

#ifdef PROGRAM_DEBUG

    displayMessage(F("End Program Receive\n"));

    dumpProgram(HullOScodeCompileOutput);

#endif

#ifdef PROCESS_PIXELS
    stopBusyPixel();
#endif

    if (save)
    {

        if (HullOSFilenameSet())
        {
            displayMessage(F("Storing the program in:%s\n"), HullOScommandsFilenameBuffer);
            saveToFile(HullOScommandsFilenameBuffer, HullOScodeCompileOutput);
        }
        else
        {
            displayMessage(F("Storing the program in:%s\n"), RUNNING_PROGRAM_FILENAME);
            saveToFile(RUNNING_PROGRAM_FILENAME, HullOScodeCompileOutput);
        }
    }
    else
    {
        displayMessage(F("Program download aborted\n"));
    }

    // enable immediate command receipt

    interpreterState = EXECUTE_IMMEDIATELY;
}

void storeReceivedByte(byte b)
{

#ifdef PROGRAM_DEBUG
    displayMessage(F("Storing:%d %c\n"), b, b);
#endif

    // ignore odd characters - except for CR

    if (b < 32 | b > 128)
    {
        if (b != STATEMENT_TERMINATOR)
            return;
    }

    switch (lineStoreState)
    {

    case LINE_START:
        // at the start of a line - look for an R command

        if (b == 'r' | b == 'R')
        {
            lineStoreState = GOT_R;
        }
        else
        {
            lineStoreState = STORING;
        }
        break;

    case GOT_R:
        // Last character was an R - is this an X or an A?

        switch (b)
        {
        case 'x':
        case 'X':
#ifdef PROGRAM_DEBUG
            displayMessageWithNewline(F("RX"));
#endif
            // put the terminator on the end

            storeProgramByte(PROGRAM_TERMINATOR);

            endProgramReceive(true);

#ifdef DIAGNOSTICS_ACTIVE

            if (diagnosticsOutputLevel & DUMP_DOWNLOADS)
            {
                dumpProgram(HullOScodeCompileOutput);
            }

#endif
            return;

        case 'A':
        case 'a':
#ifdef PROGRAM_DEBUG
            displayMessageWithNewline(F("RA"));
#endif
            storeProgramByte(PROGRAM_TERMINATOR);

            endProgramReceive(false);

            return;

        default:

            // Other commands don't affect remote download and can be handled as normal
            // write out the r
            storeProgramByte('r');

            // store everything which follows the command as normal
            lineStoreState = STORING;
            break;
        }

        break;

    case STORING:
        // break out- storing takes place next
        break;
    }

    if (lineStoreState == STORING)
    {
        // get here if we are storing or just got a line start

        // if we get here we store the byte
        storeProgramByte(b);

#ifdef DIAGNOSTICS_ACTIVE

        if (diagnosticsOutputLevel & ECHO_DOWNLOADS)
        {
            displayMessage(F("%c"), (char)b);
        }

#endif

        if (b == STATEMENT_TERMINATOR)
        {
            // Got a terminator, look for the command character

#ifdef DIAGNOSTICS_ACTIVE

            if (diagnosticsOutputLevel & ECHO_DOWNLOADS)
            {
                displayMessage(F("\n"));
            }

#endif
            lineStoreState = LINE_START;
            // look busy
#ifdef PROCESS_PIXELS
            updateBusyPixel();
#endif
        }
    }
}

void resetCommand()
{
#ifdef COMMAND_DEBUG
    displayMessageWithNewline(F(".**resetCommand"));
#endif
    resetHullOSOutputBuffer();
}

#ifdef COMMAND_DEBUG
#define MOVE_FORWARDS_DEBUG
#endif

#ifdef PROCESS_MOTOR

// Command MFddd,ttt - move distance ddd over time ttt (ttt expressed in "ticks" - tenths of a second)
// Return OK

void remoteMoveForwards()
{
    int forwardMoveDistance;

#ifdef MOVE_FORWARDS_DEBUG
    displayMessageWithNewline(F(".**moveForwards"));
#endif

    if (*decodePos == STATEMENT_TERMINATOR)
    {

#ifdef DIAGNOSTICS_ACTIVE

        if (diagnosticsOutputLevel & STATEMENT_CONFIRMATION)
        {
            displayMessageWithNewline(F("FAIL: no dist"));
        }

#endif

        return;
    }

    if (!getValue(&forwardMoveDistance))
    {
        return;
    }

    if (*decodePos == STATEMENT_TERMINATOR)
    {

#ifdef DIAGNOSTICS_ACTIVE

        if (diagnosticsOutputLevel & STATEMENT_CONFIRMATION)
        {
            displayMessageWithNewline(F("MFOK"));
        }

#endif

        fastMoveDistanceInMM(forwardMoveDistance, forwardMoveDistance);
        return;
    }

    decodePos++; // move past the separator

    if (*decodePos == STATEMENT_TERMINATOR)
    {

#ifdef DIAGNOSTICS_ACTIVE

        if (diagnosticsOutputLevel & STATEMENT_CONFIRMATION)
        {
            displayMessageWithNewline(F("FAIL: no time"));
        }

#endif
        return;
    }

    int forwardMoveTime;

    if (!getValue(&forwardMoveTime))
    {
        return;
    }

    int moveResult = timedMoveDistanceInMM(forwardMoveDistance, forwardMoveDistance, (float)forwardMoveTime / 10.0);

#ifdef DIAGNOSTICS_ACTIVE

    if (moveResult == 0)
    {

        if (diagnosticsOutputLevel & STATEMENT_CONFIRMATION)
        {
            displayMessageWithNewline(F("MFOK"));
        }
    }
    else
    {
        if (diagnosticsOutputLevel & STATEMENT_CONFIRMATION)
        {
            displayMessageWithNewline(F("MFFail"));
        }
    }

#endif
}

// Command MAradius,angle,time - move arc.
// radius - radius of the arc to move
// angle of the arc to move
// time - time for the move
//
// Return OK

// #define MOVE_ANGLE_DEBUG

void remoteMoveAngle()
{
#ifdef MOVE_ANGLE_DEBUG
    displayMessageWithNewline(F(".**moveAngle"));
#endif

    if (*decodePos == STATEMENT_TERMINATOR | decodePos == decodeLimit)
    {

#ifdef DIAGNOSTICS_ACTIVE

        if (diagnosticsOutputLevel & STATEMENT_CONFIRMATION)
        {
            displayMessageWithNewline(F("MAFail: no radius"));
        }

#endif

        return;
    }

    int radius;

    if (!getValue(&radius))
    {
        return;
    }

    if (*decodePos == STATEMENT_TERMINATOR | decodePos == decodeLimit)
    {

#ifdef DIAGNOSTICS_ACTIVE

        if (diagnosticsOutputLevel & STATEMENT_CONFIRMATION)
        {
            displayMessageWithNewline(F("MAFail: no angle"));
        }

#endif

        return;
    }

    decodePos++;

    if (*decodePos == STATEMENT_TERMINATOR | decodePos == decodeLimit)
    {

#ifdef DIAGNOSTICS_ACTIVE

        if (diagnosticsOutputLevel & STATEMENT_CONFIRMATION)
        {
            displayMessageWithNewline(F("MAFail: no angle"));
        }

#endif
        return;
    }

    int angle;

    if (!getValue(&angle))
    {
        return;
    }

    if (*decodePos == STATEMENT_TERMINATOR)
    {

#ifdef DIAGNOSTICS_ACTIVE

        if (diagnosticsOutputLevel & STATEMENT_CONFIRMATION)
        {
            displayMessage(F("MAOK"));
        }

#endif

        fastMoveArcRobot(radius, angle);
        return;
    }

    decodePos++;

    if (*decodePos == STATEMENT_TERMINATOR | decodePos == decodeLimit)
    {

#ifdef DIAGNOSTICS_ACTIVE

        if (diagnosticsOutputLevel & STATEMENT_CONFIRMATION)
        {
            displayMessageWithNewline(F("MAFail: no time"));
        }

#endif

        return;
    }

    int time;

    if (!getValue(&time))
    {
        return;
    }

#ifdef MOVE_ANGLE_DEBUG
    displayMessage(F("    radius: "));
    displayMessage(radius);
    displayMessage(F(" angle: "));
    displayMessage(angle);
    displayMessage(F(" time: "));
    displayMessageWithNewline(time);
#endif

    int reply = timedMoveArcRobot(radius, angle, time / 10.0);

#ifdef DIAGNOSTICS_ACTIVE

    if (reply == 0)
    {

        if (diagnosticsOutputLevel & STATEMENT_CONFIRMATION)
        {
            displayMessage(F("MAOK"));
        }
    }
    else
    {
        if (diagnosticsOutputLevel & STATEMENT_CONFIRMATION)
        {
            displayMessage(F("MAFail"));
        }
    }

#endif
}

// Command MMld,rd,time - move motors.
// ld - left distance
// rd - right distance
// time - time for the move
//
// Return OK

// #define MOVE_MOTORS_DEBUG

void remoteMoveMotors()
{
#ifdef MOVE_MOTORS_DEBUG
    displayMessageWithNewline(F(".**movemMotors"));
#endif

    if (*decodePos == STATEMENT_TERMINATOR | decodePos == decodeLimit)
    {

#ifdef DIAGNOSTICS_ACTIVE

        if (diagnosticsOutputLevel & STATEMENT_CONFIRMATION)
        {
            displayMessageWithNewline(F("MMFail: no left distance"));
        }

#endif

        return;
    }

    int leftDistance;

    if (!getValue(&leftDistance))
    {
        return;
    }

    if (*decodePos == STATEMENT_TERMINATOR | decodePos == decodeLimit)
    {

#ifdef DIAGNOSTICS_ACTIVE

        if (diagnosticsOutputLevel & STATEMENT_CONFIRMATION)
        {
            displayMessageWithNewline(F("MMFail: no right distance"));
        }

#endif

        return;
    }

    decodePos++;

    if (*decodePos == STATEMENT_TERMINATOR | decodePos == decodeLimit)
    {

#ifdef DIAGNOSTICS_ACTIVE

        if (diagnosticsOutputLevel & STATEMENT_CONFIRMATION)
        {
            displayMessageWithNewline(F("MMFail: no right distance"));
        }

#endif

        return;
    }

    int rightDistance;

    if (!getValue(&rightDistance))
    {
        return;
    }

    if (*decodePos == STATEMENT_TERMINATOR)
    {

#ifdef DIAGNOSTICS_ACTIVE

        if (diagnosticsOutputLevel & STATEMENT_CONFIRMATION)
        {
            displayMessage(F("MMOK"));
        }

#endif

        fastMoveDistanceInMM(leftDistance, rightDistance);
        return;
    }

    decodePos++;

    if (*decodePos == STATEMENT_TERMINATOR | decodePos == decodeLimit)
    {

#ifdef DIAGNOSTICS_ACTIVE

        if (diagnosticsOutputLevel & STATEMENT_CONFIRMATION)
        {
            displayMessageWithNewline(F("MMFail: no time"));
        }

#endif

        return;
    }

    int time;

    if (!getValue(&time))
    {
        return;
    }

#ifdef MOVE_MOTORS_DEBUG
    displayMessage(F("    ld: "));
    displayMessage(leftDistance);
    displayMessage(F(" rd: "));
    displayMessage(rightDistance);
    displayMessage(F(" time: "));
    displayMessageWithNewline(time);
#endif

    int reply = timedMoveDistanceInMM(leftDistance, rightDistance, time / 10.0);

#ifdef DIAGNOSTICS_ACTIVE

    if (reply == 0)
    {
        if (diagnosticsOutputLevel & STATEMENT_CONFIRMATION)
        {
            displayMessage(F("MMOK"));
        }
    }
    else
    {
        if (diagnosticsOutputLevel & STATEMENT_CONFIRMATION)
        {
            displayMessage(F("MMFail: %d\n"), reply);
        }
    }

#endif
}

// Command MWll,rr,ss - wheel configuration
// ll - diameter of left wheel
// rr - diameter of right wheel
// ss - spacing of wheels
//
// all dimensions in mm
#// Return OK

// #define CONFIG_WHEELS_DEBUG

void remoteConfigWheels()
{
#ifdef CONFIG_WHEELS_DEBUG
    displayMessageWithNewline(F(".**remoteConfigWheels"));
#endif

    if (*decodePos == STATEMENT_TERMINATOR | decodePos == decodeLimit)
    {

#ifdef DIAGNOSTICS_ACTIVE

        if (diagnosticsOutputLevel & STATEMENT_CONFIRMATION)
        {
            displayMessageWithNewline(F("MWFail: no left diameter"));
        }

#endif

        return;
    }

    int leftDiameter;

    if (!getValue(&leftDiameter))
    {
        return;
    }

    if (*decodePos == STATEMENT_TERMINATOR | decodePos == decodeLimit)
    {

#ifdef DIAGNOSTICS_ACTIVE

        if (diagnosticsOutputLevel & STATEMENT_CONFIRMATION)
        {
            displayMessageWithNewline(F("MWFail: no right diameter"));
        }

#endif

        return;
    }

    decodePos++;

    if (*decodePos == STATEMENT_TERMINATOR | decodePos == decodeLimit)
    {

#ifdef DIAGNOSTICS_ACTIVE

        if (diagnosticsOutputLevel & STATEMENT_CONFIRMATION)
        {
            displayMessageWithNewline(F("MWFail: no right diameter"));
        }

#endif

        return;
    }

    int rightDiameter;

    if (!getValue(&rightDiameter))
    {
        return;
    }

    if (*decodePos == STATEMENT_TERMINATOR)
    {

#ifdef DIAGNOSTICS_ACTIVE

        if (diagnosticsOutputLevel & STATEMENT_CONFIRMATION)
        {
            displayMessageWithNewline(F("MWFail: no wheel spacing"));
        }
#endif

        return;
    }

    decodePos++;

    if (*decodePos == STATEMENT_TERMINATOR | decodePos == decodeLimit)
    {

#ifdef DIAGNOSTICS_ACTIVE

        if (diagnosticsOutputLevel & STATEMENT_CONFIRMATION)
        {
            displayMessageWithNewline(F("MWFail: no wheel spacing"));
        }

#endif

        return;
    }

    int spacing;

    if (!getValue(&spacing))
    {
        return;
    }

#ifdef CONFIG_WHEELS_DEBUG
    displayMessage(F("    ld: "));
    displayMessage(leftDiameter);
    displayMessage(F(" rd: "));
    displayMessage(rightDiameter);
    displayMessage(F(" separation: "));
    displayMessageWithNewline(spacing);
#endif

    setActiveWheelSettings(leftDiameter, rightDiameter, spacing);

#ifdef DIAGNOSTICS_ACTIVE

    if (diagnosticsOutputLevel & STATEMENT_CONFIRMATION)
    {
        displayMessage(F("MWOK"));
    }
#endif
}

void remoteViewWheelConfig()
{
    dumpActiveWheelSettings();
}

#ifdef COMMAND_DEBUG
#define ROTATE_DEBUG
#endif

// Command MRddd,ttt - rotate distance in time ttt (ttt is given in "ticks", where a tick is a tenth of a second
// Command MR    - rotate previous distance, or 0 if no previous rotate
// Return OK

void remoteRotateRobot()
{
    int rotateAngle;

#ifdef ROTATE_DEBUG
    displayMessageWithNewline(F(".**rotateRobot"));
#endif

    if (*decodePos == STATEMENT_TERMINATOR)
    {

#ifdef DIAGNOSTICS_ACTIVE

        if (diagnosticsOutputLevel & STATEMENT_CONFIRMATION)
        {
            displayMessageWithNewline(F("MRFail: no angle"));
        }

#endif

        return;
    }

    rotateAngle;

    if (!getValue(&rotateAngle))
    {
        return;
    }

#ifdef ROTATE_DEBUG
    displayMessage(F(".  Rotating: "));
    displayMessageWithNewline(rotateAngle);
#endif

    if (*decodePos == STATEMENT_TERMINATOR)
    {

#ifdef DIAGNOSTICS_ACTIVE

        if (diagnosticsOutputLevel & STATEMENT_CONFIRMATION)
        {
            displayMessage(F("MROK"));
        }

#endif

        fastRotateRobot(rotateAngle);
        return;
    }

    decodePos++; // move past the separator

    if (*decodePos == STATEMENT_TERMINATOR)
    {

#ifdef DIAGNOSTICS_ACTIVE

        if (diagnosticsOutputLevel & STATEMENT_CONFIRMATION)
        {
            displayMessageWithNewline(F("MRFail: no time"));
        }
#endif
        return;
    }

    int rotateTimeInTicks;

    if (!getValue(&rotateTimeInTicks))
    {
        return;
    }

    int moveResult = timedRotateRobot(rotateAngle, rotateTimeInTicks / 10.0);

    if (moveResult == 0)
    {

#ifdef DIAGNOSTICS_ACTIVE

        if (diagnosticsOutputLevel & STATEMENT_CONFIRMATION)
        {
            displayMessage(F("MROK"));
        }
#endif
    }
    else
    {

#ifdef DIAGNOSTICS_ACTIVE

        if (diagnosticsOutputLevel & STATEMENT_CONFIRMATION)
        {
            displayMessageWithNewline(F("MRFail"));
        }

#endif
    }
}

#ifdef COMMAND_DEBUG
#define CHECK_MOVING_DEBUG
#endif

// Command MC - check if the robot is still moving
// Return "moving" or "stopped"

void checkMoving()
{
#ifdef CHECK_MOVING_DEBUG
    displayMessageWithNewline(F(".**CheckMoving: "));
#endif

    if (motorsMoving())
    {
#ifdef CHECK_MOVING_DEBUG
        displayMessageWithNewline(F(".  moving"));
#endif
        displayMessageWithNewline(F("MCMove"));
    }
    else
    {
#ifdef CHECK_MOVING_DEBUG
        displayMessageWithNewline(F(".  stopped"));
#endif
        displayMessageWithNewline(F("MCstopped"));
    }
}

#ifdef COMMAND_DEBUG
#define REMOTE_STOP_DEBUG
#endif

// Command MS - stops the robot
// Return OK
void remoteStopRobot()
{
#ifdef REMOTE_STOP_DEBUG
    displayMessageWithNewline(F(".**remoteStopRobot: "));
#endif

    motorStop();

#ifdef DIAGNOSTICS_ACTIVE

    if (diagnosticsOutputLevel & STATEMENT_CONFIRMATION)
    {
        displayMessageWithNewline(F("MSOK"));
    }

#endif
}

void remoteMoveControl()
{
    if (*decodePos == STATEMENT_TERMINATOR | decodePos == decodeLimit)
    {

#ifdef DIAGNOSTICS_ACTIVE

        displayMessageWithNewline(F("FAIL: missing move control command character"));

#endif

        return;
    }

#ifdef COMMAND_DEBUG
    displayMessageWithNewline(F(".**remoteMoveControl: "));
#endif

    char commandCh = *decodePos;

#ifdef COMMAND_DEBUG
    displayMessage(F(".  Move Command code : "));
    displayMessageWithNewline(commandCh);
#endif

    decodePos++;

    switch (commandCh)
    {
    case 'A':
    case 'a':
        remoteMoveAngle();
        break;
    case 'F':
    case 'f':
        remoteMoveForwards();
        break;
    case 'R':
    case 'r':
        remoteRotateRobot();
        break;
    case 'M':
    case 'm':
        remoteMoveMotors();
        break;
    case 'C':
    case 'c':
        checkMoving();
        break;
    case 'S':
    case 's':
        remoteStopRobot();
        break;
    case 'V':
    case 'v':
        remoteViewWheelConfig();
        break;
    case 'W':
    case 'w':
        remoteConfigWheels();
        break;
    }
}

#endif

#ifdef COMMAND_DEBUG
#define PIXEL_COLOUR_DEBUG
#endif

bool readColour(byte *r, byte *g, byte *b)
{
    int result;

    if (!getValue(&result))
    {
        return false;
    }

    *r = (byte)result;

#ifdef PIXEL_COLOUR_DEBUG
    displayMessage(F(".  Red: "));
    displayMessageWithNewline(*r);
#endif

    if (*decodePos == STATEMENT_TERMINATOR | decodePos == decodeLimit)
    {

#ifdef DIAGNOSTICS_ACTIVE

        if (diagnosticsOutputLevel & STATEMENT_CONFIRMATION)
        {
            displayMessageWithNewline(F("FAIL: missing colour values in readColor"));
        }

#endif
        return false;
    }

    decodePos++;

    if (*decodePos == STATEMENT_TERMINATOR | decodePos == decodeLimit)
    {

#ifdef DIAGNOSTICS_ACTIVE

        if (diagnosticsOutputLevel & STATEMENT_CONFIRMATION)
        {
            displayMessageWithNewline(F("FAIL: missing colours after red in readColor"));
        }

#endif

        return false;
    }

    if (!getValue(&result))
    {
        return false;
    }

    *g = (byte)result;

#ifdef PIXEL_COLOUR_DEBUG
    displayMessage(F(".  Green: "));
    displayMessageWithNewline(*g);
#endif

    decodePos++;

    if (*decodePos == STATEMENT_TERMINATOR | decodePos == decodeLimit)
    {

#ifdef DIAGNOSTICS_ACTIVE

        if (diagnosticsOutputLevel & STATEMENT_CONFIRMATION)
        {
            displayMessageWithNewline(F("FAIL: missing colours after green in readColor"));
        }

#endif

        return false;
    }

    if (!getValue(&result))
    {
        displayMessage(F("FAIL: missing colours after blue in readColor\n"));
        return false;
    }

    *b = (byte)result;

#ifdef PIXEL_COLOUR_DEBUG
    displayMessage(F(".  Blue: "));
    displayMessageWithNewline(*b);
#endif

    return true;
}

// Command PCrrr,ggg,bbb - set a coloured candle with the red, green
// and blue components as given
// Return OK

void remoteColouredCandle()
{

#ifdef PIXEL_COLOUR_DEBUG
    displayMessageWithNewline(F(".**remoteColouredCandle: "));
#endif

    byte r, g, b;

#ifdef DIAGNOSTICS_ACTIVE

    if (diagnosticsOutputLevel & STATEMENT_CONFIRMATION)
    {
        displayMessage(F("PC"));
    }

#endif

    if (readColour(&r, &g, &b))
    {
#ifdef PROCESS_PIXELS
        flickeringColouredLights(r, g, b, 10);
#endif
#ifdef DIAGNOSTICS_ACTIVE

        if (diagnosticsOutputLevel & STATEMENT_CONFIRMATION)
        {
            displayMessageWithNewline(F("OK"));
        }
#endif
    }
}

// Command PNname - set a coloured candle with the name as given
// Return OK

void remoteSetColorByName()
{
#ifdef PIXEL_COLOUR_DEBUG
    displayMessageWithNewline(F(".**remoteColouredName: "));
#endif

    byte r = 0, g = 0, b = 0;

#ifdef DIAGNOSTICS_ACTIVE

    if (diagnosticsOutputLevel & STATEMENT_CONFIRMATION)
    {
        displayMessage(F("PN"));
    }

#endif

    if (*decodePos == STATEMENT_TERMINATOR | decodePos == decodeLimit)
    {

#ifdef DIAGNOSTICS_ACTIVE

        if (diagnosticsOutputLevel & STATEMENT_CONFIRMATION)
        {
            displayMessageWithNewline(F("FAIL: missing colour in set colour by name"));
        }

#endif

        return;
    }

    char inputCh = toLowerCase(*decodePos);

#ifdef DIAGNOSTICS_ACTIVE

    if (diagnosticsOutputLevel & STATEMENT_CONFIRMATION)
    {
        displayMessage(F("  colour id:%c\n"), inputCh);
    }

#endif

    switch (inputCh)
    {
    case 'r':
        r = 255;
        break;
    case 'g':
        g = 255;
        break;
    case 'b':
        b = 255;
        break;
    case 'c':
        g = 255;
        b = 255;
        break;
    case 'm':
        r = 255;
        b = 255;
        break;
    case 'y':
        g = 255;
        r = 255;
        break;
    case 'w':
        r = 255;
        g = 255;
        b = 255;
        break;
    case 'k':
        break;
    default:

#ifdef DIAGNOSTICS_ACTIVE

        if (diagnosticsOutputLevel & STATEMENT_CONFIRMATION)
        {
            displayMessageWithNewline(F("FAIL: invalid colour in set colour by name"));
        }
#endif
        return;
    }

#ifdef PROCESS_PIXELS
    flickeringColouredLights(r, g, b, 10);
#endif

#ifdef DIAGNOSTICS_ACTIVE

    if (diagnosticsOutputLevel & STATEMENT_CONFIRMATION)
    {
        displayMessageWithNewline(F("OK"));
    }

#endif
}

// #define REMOTE_PIXEL_COLOR_FADE_DEBUG

void remoteFadeToColor()
{
#ifdef PIXEL_COLOUR_DEBUG
    displayMessageWithNewline(F(".**remoteFadeToColour: "));
#endif

    int result;

    if (!getValue(&result))
    {
        return;
    }

    byte no = (byte)result;

    if (no < 1)
        no = 1;
    if (no > 20)
        no = 20;

    no = 21 - no;

#ifdef DIAGNOSTICS_ACTIVE
    displayMessageWithNewline(F(" Speed: %d"), no);
#endif

    decodePos++;

#ifdef DIAGNOSTICS_ACTIVE

    if (diagnosticsOutputLevel & STATEMENT_CONFIRMATION)
    {
        displayMessage(F("PX"));
    }

#endif

    if (*decodePos == STATEMENT_TERMINATOR | decodePos == decodeLimit)
    {

#ifdef DIAGNOSTICS_ACTIVE

        if (diagnosticsOutputLevel & STATEMENT_CONFIRMATION)
        {
            displayMessageWithNewline(F("Fail: missing colours after speed"));
        }

#endif

        return;
    }

    byte r, g, b;

#ifdef DIAGNOSTICS_ACTIVE

    if (diagnosticsOutputLevel & STATEMENT_CONFIRMATION)
    {
        displayMessage(F("PX"));
    }

#endif

    if (readColour(&r, &g, &b))
    {
        transitionToColor(no, r, g, b);

#ifdef DIAGNOSTICS_ACTIVE

        if (diagnosticsOutputLevel & STATEMENT_CONFIRMATION)
        {
            displayMessageWithNewline(F("OK"));
        }

#endif
    }
}

// PFddd - set flicker speed to value given

void remoteSetFlickerSpeed()
{
#ifdef PIXEL_COLOUR_DEBUG
    displayMessageWithNewline(F(".**remoteSetFlickerSpeed: "));
#endif

    int result;

    if (!getValue(&result))
    {
        return;
    }

    byte no = (byte)result;

#ifdef PIXEL_COLOUR_DEBUG
    displayMessage(F(".  Setting: "));
    displayMessageWithNewline(no);
#endif

#ifdef PROCESS_PIXELS
    setFlickerUpdateSpeed(no);
#endif
#ifdef DIAGNOSTICS_ACTIVE

    if (diagnosticsOutputLevel & STATEMENT_CONFIRMATION)
    {
        displayMessageWithNewline(F("PFOK"));
    }

#endif
}

// PIppp,rrr,ggg,bbb
// Set individual pixel colour

void remoteSetIndividualPixel()
{

#ifdef PIXEL_COLOUR_DEBUG
    displayMessageWithNewline(F(".**remoteSetIndividualPixel: "));
#endif

    int result;

    if (!getValue(&result))
    {
        return;
    }

    byte no = (byte)result;

#ifdef PIXEL_COLOUR_DEBUG
    displayMessage(F(".  Setting: "));
    displayMessageWithNewline(no);
#endif

    decodePos++;

#ifdef DIAGNOSTICS_ACTIVE

    if (diagnosticsOutputLevel & STATEMENT_CONFIRMATION)
    {
        displayMessage(F("PI"));
    }

#endif

    if (*decodePos == STATEMENT_TERMINATOR | decodePos == decodeLimit)
    {

#ifdef DIAGNOSTICS_ACTIVE

        if (diagnosticsOutputLevel & STATEMENT_CONFIRMATION)
        {
            displayMessageWithNewline(F("Fail: missing colours after pixel"));
        }

#endif

        return;
    }

    byte r, g, b;

    if (readColour(&r, &g, &b))
    {
#ifdef PROCESS_PIXELS
        setLightColor(r, g, b);
#endif

#ifdef DIAGNOSTICS_ACTIVE

        if (diagnosticsOutputLevel & STATEMENT_CONFIRMATION)
        {
            displayMessageWithNewline(F("OK"));
        }

#endif
    }
}

void remoteSetPixelsOff()
{

#ifdef DIAGNOSTICS_ACTIVE

    if (diagnosticsOutputLevel & STATEMENT_CONFIRMATION)
    {
        displayMessageWithNewline(F("POOK"));
    }

#endif

#ifdef PROCESS_PIXELS
    setAllLightsOff();
#endif
}

void remoteSetRandomColors()
{

#ifdef DIAGNOSTICS_ACTIVE

    if (diagnosticsOutputLevel & STATEMENT_CONFIRMATION)
    {
        displayMessageWithNewline(F("PROK"));
    }

#endif

#ifdef PROCESS_PIXELS
    randomiseLights();
#endif
}

void remotePixelControl()
{
    if (*decodePos == STATEMENT_TERMINATOR | decodePos == decodeLimit)
    {

#ifdef DIAGNOSTICS_ACTIVE
        displayMessageWithNewline(F("FAIL: missing pixel control command character"));
#endif
        return;
    }

#ifdef PIXEL_COLOUR_DEBUG
    displayMessageWithNewline(F(".**remotePixelControl: "));
#endif

    char commandCh = *decodePos;

#ifdef PIXEL_COLOUR_DEBUG
    displayMessage(F(".  Pixel Command code : "));
    displayMessageWithNewline(commandCh);
#endif

    decodePos++;

    switch (commandCh)
    {
    case 'a':
    case 'A':
#ifdef PROCESS_PIXELS
        flickerOn();
#endif
        break;
    case 's':
    case 'S':
#ifdef PROCESS_PIXELS
        flickerOff();
#endif
        break;
    case 'i':
    case 'I':
        remoteSetIndividualPixel();
        break;
    case 'o':
    case 'O':
        remoteSetPixelsOff();
        break;
    case 'c':
    case 'C':
        remoteColouredCandle();
        break;
    case 'f':
    case 'F':
        remoteSetFlickerSpeed();
        break;
    case 'x':
    case 'X':
#ifdef PROCESS_PIXELS
        remoteFadeToColor();
#endif
        break;
    case 'r':
    case 'R':
        remoteSetRandomColors();
        break;
    case 'n':
    case 'N':
        remoteSetColorByName();
        break;
    }
}

// Command CDddd - delay time
// Command CD    - previous delay
// Return OK

#ifdef COMMAND_DEBUG
#define COMMAND_DELAY_DEBUG
#endif

void remoteDelay()
{
    int delayValueInTenthsIOfASecond;

#ifdef COMMAND_DELAY_DEBUG
    displayMessageWithNewline(F(".**remoteDelay"));
#endif

    if (*decodePos == STATEMENT_TERMINATOR)
    {

#ifdef DIAGNOSTICS_ACTIVE
        if (diagnosticsOutputLevel & STATEMENT_CONFIRMATION)
        {
            displayMessageWithNewline(F("CDFail: no delay"));
        }
#endif

        return;
    }

    if (!getValue(&delayValueInTenthsIOfASecond))
    {
        return;
    }

#ifdef COMMAND_DELAY_DEBUG
    displayMessage(F(".  Delaying: "));
    displayMessageWithNewline(delayValueInTenthsIOfASecond);
#endif

#ifdef DIAGNOSTICS_ACTIVE
    if (diagnosticsOutputLevel & STATEMENT_CONFIRMATION)
    {
        displayMessage(F("CDOK"));
    }
#endif

    delayEndTime = millis() + delayValueInTenthsIOfASecond * 100;

    programState = PROGRAM_AWAITING_DELAY_COMPLETION;
}

// Command CLxxxx - program label
// Ignored at execution, specifies the destination of a branch
// Return OK

void declareLabel()
{
#ifdef COMMAND_DELAY_DEBUG
    displayMessageWithNewline(F(".**declareLabel"));
#endif

#ifdef DIAGNOSTICS_ACTIVE
    if (diagnosticsOutputLevel & STATEMENT_CONFIRMATION)
    {
        displayMessageWithNewline(F("CLOK"));
    }
#endif
}

int findNextStatement(int programPosition)
{

    while (true)
    {
        char ch = HullOScodeRunningCode[programPosition];

        if (ch == PROGRAM_TERMINATOR | programPosition == HULLOS_PROGRAM_SIZE)
            return -1;

        if (ch == STATEMENT_TERMINATOR)
        {
            programPosition++;
            if (programPosition == HULLOS_PROGRAM_SIZE)
                return -1;
            else
                return programPosition;
        }
        programPosition++;
    }
}

// Find a label in the program
// Returns the offset into the program where the label is declared
// The first parameter is the first character of the label
// (i.e. the character after the instruction code that specifies the destination)
// This might not always be the same command (it might be a branch or a subroutine call)
// The second parameter is the start position of the search in the program.
// This is always the start of a statement, and usually the start of the program, to allow
// branches up the code.

// #define FIND_LABEL_IN_PROGRAM_DEBUG

int findLabelInProgram(char *label, int programPosition)
{
    // Assume we are starting at the beginning of the program

    while (true)
    {
        // Spin down the statements

        int statementStart = programPosition;

        char programByte = HullOScodeRunningCode[programPosition++];

#ifdef FIND_LABEL_IN_PROGRAM_DEBUG
        displayMessage(F("Statement at: "));
        displayMessage(statementStart);
        displayMessage(F(" starting: "));
        displayMessageWithNewline(programByte);
#endif
        if (programByte != 'C' & programByte != 'c')
        {

#ifdef FIND_LABEL_IN_PROGRAM_DEBUG
            displayMessageWithNewline(F("Not a statement that starts with C"));
#endif

            programPosition = findNextStatement(programPosition);

#ifdef FIND_LABEL_IN_PROGRAM_DEBUG
            displayMessage(F("Spin to statement at: "));
            displayMessageWithNewline(programPosition);
#endif

            // Check to see if we have reached the end of the program in EEPROM
            if (programPosition == -1)
            {
                // Give up if the end of the code has been reached
                return -1;
            }
            else
            {
                // Check this statement
                continue;
            }
        }

        // If we get here we have found a C

        programByte = HullOScodeRunningCode[programPosition++];

#ifdef FIND_LABEL_IN_PROGRAM_DEBUG

        displayMessage(F("Second statement character: "));
        displayMessageWithNewline(programByte);

#endif

        // if we get here we have a control command - see if the command is a label
        if (programByte != 'L' & programByte != 'l')
        {

#ifdef FIND_LABEL_IN_PROGRAM_DEBUG
            displayMessageWithNewline(F("Not a loop command"));
#endif

            programPosition = findNextStatement(programPosition);
            if (programPosition == -1)
            {
                return -1;
            }
            else
            {
                continue;
            }
        }

        // if we get here we have a CL command

#ifdef FIND_LABEL_IN_PROGRAM_DEBUG
        displayMessageWithNewline(F("Got a CL command"));
#endif

        // Set start position for label comparison
        char *labelTest = label;

        // Now spin down the label looking for a match

        while (*labelTest != STATEMENT_TERMINATOR & programPosition < HULLOS_PROGRAM_SIZE)
        {
            programByte = HullOScodeRunningCode[programPosition];

#ifdef FIND_LABEL_IN_PROGRAM_DEBUG
            displayMessage(F("Destination byte: "));
            displayMessage(*labelTest);
            displayMessage(F(" Program byte: "));
            displayMessageWithNewline(programByte);
#endif

            if (*labelTest == programByte)
            {

#ifdef FIND_LABEL_IN_PROGRAM_DEBUG
                displayMessageWithNewline(F("Got a match"));
#endif
                // Move on to the next byte
                labelTest++;
                programPosition++;
            }
            else
            {
#ifdef FIND_LABEL_IN_PROGRAM_DEBUG
                displayMessageWithNewline(F("Fail"));
#endif
                break;
            }
        }

        // get here when we reach the end of the statement or we have a mismatch

        // Get the byte at the end of the destination statement

        programByte = HullOScodeRunningCode[programPosition];

        if (*labelTest == programByte)
        {
            // If the end of the label matches the end of the statement code we have a match
            // Note that this means that if the last line of the program is a label we can only
            // find this line if it has a statement terminator on the end.
            // Which is fine by me.

#ifdef FIND_LABEL_IN_PROGRAM_DEBUG
            displayMessageWithNewline(F("label match"));
#endif
            return statementStart;
        }
        else
        {
            programPosition = findNextStatement(programPosition);

#ifdef FIND_LABEL_IN_PROGRAM_DEBUG
            displayMessage(F("Spin to statement at: "));
            displayMessageWithNewline(programPosition);
#endif

            // Check to see if we have reached the end of the program in EEPROM
            if (programPosition == -1)
            {
                // Give up if the end of the code has been reached
                return -1;
            }
            else
            {
                // Check this statement
                continue;
            }
        }
    }
}

// #define JUMP_TO_LABEL_DEBUG

// Command CJxxxx - jump to label
// Jumps to the specified label
// Return CJOK if the label is found, error if not.

void jumpToLabel()
{
#ifdef JUMP_TO_LABEL_DEBUG
    displayMessageWithNewline(F(".**jump to label"));
#endif

    char *labelPos = decodePos;
    char *labelSearch = decodePos;

    int labelStatementPos = findLabelInProgram(decodePos, 0);

#ifdef JUMP_TO_LABEL_DEBUG
    displayMessage(F("Label statement pos: "));
    displayMessageWithNewline(labelStatementPos);
#endif

    if (labelStatementPos >= 0)
    {
        // the label has been found - jump to it
        programCounter = labelStatementPos;

#ifdef JUMP_TO_LABEL_DEBUG
        displayMessage(F("New Program Counter: "));
        displayMessageWithNewline(programCounter);
#endif

#ifdef DIAGNOSTICS_ACTIVE
        if (diagnosticsOutputLevel & STATEMENT_CONFIRMATION)
        {
            displayMessageWithNewline(F("CJOK"));
        }
#endif
    }
    else
    {
#ifdef DIAGNOSTICS_ACTIVE
        if (diagnosticsOutputLevel & STATEMENT_CONFIRMATION)
        {
            displayMessageWithNewline(F("CJFAIL: no dest"));
        }
#endif
    }
}

// #define JUMP_TO_LABEL_COIN_DEBUG

// Command CCxxxx - jump to label on a coin toss
// Jumps to the specified label
// Return CCOK if the label is found, error if not.

void jumpToLabelCoinToss()
{
#ifdef JUMP_TO_LABEL_COIN_DEBUG
    displayMessageWithNewline(F(".**jump to label coin toss"));
    send

#endif

        char *labelPos = decodePos;
    char *labelSearch = decodePos;

    int labelStatementPos = findLabelInProgram(decodePos, 0);

#ifdef JUMP_TO_LABEL_COIN_DEBUG
    displayMessage(F("  Label statement pos: "));
    displayMessageWithNewline(labelStatementPos);
#endif

    if (labelStatementPos >= 0)
    {
        // the label has been found - jump to it

        if (random(0, 2) == 0)
        {
            programCounter = labelStatementPos;
#ifdef DIAGNOSTICS_ACTIVE
            if (diagnosticsOutputLevel & STATEMENT_CONFIRMATION)
            {
                displayMessage(F("CCjump"));
            }
#endif
        }
        else
        {
#ifdef DIAGNOSTICS_ACTIVE
            if (diagnosticsOutputLevel & STATEMENT_CONFIRMATION)
            {
                displayMessage(F("CCcontinue"));
            }
#endif
        }

#ifdef JUMP_TO_LABEL_COIN_DEBUG
        displayMessage(F("New Program Counter: "));
        displayMessageWithNewline(programCounter);
#endif
    }
    else
    {
#ifdef DIAGNOSTICS_ACTIVE
        if (diagnosticsOutputLevel & STATEMENT_CONFIRMATION)
        {
            displayMessageWithNewline(F("CCFail: no dest"));
        }
#endif
    }
}

// Command CA - pause when motors active
// Return CAOK when the pause is started

void pauseWhenMotorsActive()
{
#ifdef PAUSE_MOTORS_ACTIVE_DEBUG
    displayMessageWithNewline(F(".**pause while the motors are active"));
#endif

    // Only wait for completion if the program is actually running

    if (programState == PROGRAM_ACTIVE)
        programState = PROGRAM_AWAITING_MOVE_COMPLETION;

#ifdef DIAGNOSTICS_ACTIVE
    if (diagnosticsOutputLevel & STATEMENT_CONFIRMATION)
    {
        displayMessageWithNewline(F("CAOK"));
    }
#endif
}

// Command Ccddd,ccc
// Jump to label if condition true

// #define COMMAND_MEASURE_DEBUG

void measureDistanceAndJump()
{

#ifdef COMMAND_MEASURE_DEBUG
    displayMessageWithNewline(F(".**measure distance and jump to label"));
#endif

    int distance;

    if (!getValue(&distance))
    {
        return;
    }

#ifdef COMMAND_MEASURE_DEBUG
    displayMessage(F(".  Distance: "));
    displayMessageWithNewline(distance);
#endif

#ifdef DIAGNOSTICS_ACTIVE
    if (diagnosticsOutputLevel & STATEMENT_CONFIRMATION)
    {
        displayMessage(F("CM"));
    }
#endif

    if (*decodePos == STATEMENT_TERMINATOR | decodePos == decodeLimit)
    {
#ifdef DIAGNOSTICS_ACTIVE
        if (diagnosticsOutputLevel & STATEMENT_CONFIRMATION)
        {
            displayMessageWithNewline(F("FAIL: missing dest"));
        }
#endif
        return;
    }

    decodePos++;

    if (*decodePos == STATEMENT_TERMINATOR | decodePos == decodeLimit)
    {
#ifdef DIAGNOSTICS_ACTIVE
        if (diagnosticsOutputLevel & STATEMENT_CONFIRMATION)
        {
            displayMessageWithNewline(F("FAIL: missing dest"));
        }
#endif
        return;
    }

    int labelStatementPos = findLabelInProgram(decodePos, 0);

#ifdef COMMAND_MEASURE_DEBUG
    displayMessage(F("Label statement pos: "));
    displayMessageWithNewline(labelStatementPos);
#endif

    if (labelStatementPos < 0)
    {
#ifdef DIAGNOSTICS_ACTIVE
        displayMessageWithNewline(F("FAIL: label not found"));
#endif
        return;
    }

    int measuredDistance = getDistanceValueInt();

#ifdef COMMAND_MEASURE_DEBUG
    displayMessage(F("Measured Distance: "));
    displayMessageWithNewline(measuredDistance);
#endif

    if (measuredDistance < distance)
    {
#ifdef COMMAND_MEASURE_DEBUG
        displayMessageWithNewline(F("Distance smaller - taking jump"));
#endif
        // the label has been found - jump to it
        programCounter = labelStatementPos;

#ifdef DIAGNOSTICS_ACTIVE
        if (diagnosticsOutputLevel & STATEMENT_CONFIRMATION)
        {
            displayMessageWithNewline(F("jump"));
        }
#endif
    }
    else
    {
#ifdef COMMAND_MEASURE_DEBUG
        displayMessageWithNewline(F("Distance larger - continuing"));
#endif
#ifdef DIAGNOSTICS_ACTIVE
        if (diagnosticsOutputLevel & STATEMENT_CONFIRMATION)
        {
            displayMessageWithNewline(F("continue"));
        }
#endif
    }

    // otherwise do nothing
}

// #define COMPARE_CONDITION_DEBUG

void compareAndJump(bool jumpIfTrue)
{

#ifdef COMPARE_CONDITION_DEBUG
    displayMessageWithNewline(F(".**test condition and jump to label"));
#endif

    bool result;

    if (!testCondition(&result))
    {
        return;
    }

#ifdef DIAGNOSTICS_ACTIVE
    if (diagnosticsOutputLevel & STATEMENT_CONFIRMATION)
    {
        if (jumpIfTrue)
            displayMessage(F("CC"));
        else
            displayMessage(F("CN"));
    }
#endif

    if (*decodePos == STATEMENT_TERMINATOR | decodePos == decodeLimit)
    {
#ifdef DIAGNOSTICS_ACTIVE
        if (diagnosticsOutputLevel & STATEMENT_CONFIRMATION)
        {
            displayMessageWithNewline(F("FAIL: missing dest"));
        }
#endif
        return;
    }

    decodePos++;

    if (*decodePos == STATEMENT_TERMINATOR | decodePos == decodeLimit)
    {
#ifdef DIAGNOSTICS_ACTIVE
        if (diagnosticsOutputLevel & STATEMENT_CONFIRMATION)
        {
            displayMessageWithNewline(F("FAIL: missing dest"));
        }
#endif
        return;
    }

    int labelStatementPos = findLabelInProgram(decodePos, 0);

#ifdef COMPARE_CONDITION_DEBUG
    displayMessage(F("Label statement pos: "));
    displayMessageWithNewline(labelStatementPos);
#endif

    if (labelStatementPos < 0)
    {
#ifdef DIAGNOSTICS_ACTIVE
        displayMessageWithNewline(F("FAIL: label not found"));
#endif
        return;
    }

    if (result == jumpIfTrue)
    {
#ifdef COMPARE_CONDITION_DEBUG
        displayMessageWithNewline(F("Condition true - taking jump"));
#endif
        // the label has been found - jump to it
        programCounter = labelStatementPos;

#ifdef DIAGNOSTICS_ACTIVE
        if (diagnosticsOutputLevel & STATEMENT_CONFIRMATION)
        {
            displayMessageWithNewline(F("jump"));
        }
#endif
    }
    else
    {
#ifdef COMPARE_CONDITION_DEBUG
        displayMessageWithNewline(F("condition failed - continuing"));
#endif
#ifdef DIAGNOSTICS_ACTIVE
        if (diagnosticsOutputLevel & STATEMENT_CONFIRMATION)
        {
            displayMessageWithNewline(F("continue"));
        }
#endif
    }

    // otherwise do nothing
}


#if defined(PROCESS_MOTOR) || defined(PROCESS_REMOTE_ROBOT_DRIVE)

// Command CIccc
// Jump to label if the motors are not running

// #define JUMP_MOTORS_INACTIVE_DEBUG

void jumpWhenMotorsInactive()
{

#ifdef JUMP_MOTORS_INACTIVE_DEBUG
    displayMessageWithNewline(F(".**jump to label if motors inactive"));
#endif

#ifdef DIAGNOSTICS_ACTIVE
    if (diagnosticsOutputLevel & STATEMENT_CONFIRMATION)
    {
        displayMessage(F("CI"));
    }
#endif

    if (*decodePos == STATEMENT_TERMINATOR | decodePos == decodeLimit)
    {
#ifdef DIAGNOSTICS_ACTIVE
        if (diagnosticsOutputLevel & STATEMENT_CONFIRMATION)
        {
            displayMessageWithNewline(F("FAIL: missing dest"));
        }
#endif
        return;
    }

    int labelStatementPos = findLabelInProgram(decodePos, 0);

#ifdef JUMP_MOTORS_INACTIVE_DEBUG
    displayMessage(F("Label statement pos: "));
    displayMessageWithNewline(labelStatementPos);
#endif

    if (labelStatementPos < 0)
    {
#ifdef DIAGNOSTICS_ACTIVE
        if (diagnosticsOutputLevel & STATEMENT_CONFIRMATION)
        {
            displayMessageWithNewline(F("FAIL: label not found"));
        }
#endif
        return;
    }

    if (!motorsMoving())
    {
#ifdef JUMP_MOTORS_INACTIVE_DEBUG
        displayMessageWithNewline(F("Motors inactive - taking jump"));
#endif
        // the label has been found - jump to it
        programCounter = labelStatementPos;
#ifdef DIAGNOSTICS_ACTIVE
        if (diagnosticsOutputLevel & STATEMENT_CONFIRMATION)
        {
            displayMessageWithNewline(F("jump"));
        }
#endif
    }
    else
    {
#ifdef DIAGNOSTICS_ACTIVE
        if (diagnosticsOutputLevel & STATEMENT_CONFIRMATION)
        {
            displayMessageWithNewline(F("continue"));
        }
#endif
#ifdef JUMP_MOTORS_INACTIVE_DEBUG
        displayMessageWithNewline(F("Motors running - continuing"));
#endif
    }

    // otherwise do nothing
}

#endif

void programControl()
{
    if (*decodePos == STATEMENT_TERMINATOR | decodePos == decodeLimit)
    {
#ifdef DIAGNOSTICS_ACTIVE
        displayMessageWithNewline(F("FAIL: missing program control command character"));
#endif
        return;
    }

#ifdef COMMAND_DEBUG
    displayMessageWithNewline(F(".**remoteProgramControl: "));
#endif

    char commandCh = *decodePos;

#ifdef COMMAND_DEBUG
    displayMessage(F(".   Program command : "));
    displayMessageWithNewline(commandCh);
#endif

    decodePos++;

    switch (commandCh)
    {

#if defined(PROCESS_MOTOR) || defined(PROCESS_REMOTE_ROBOT_DRIVE)

    case 'I':
    case 'i':
        jumpWhenMotorsInactive();
        break;
    case 'A':
    case 'a':
        pauseWhenMotorsActive();
        break;
#endif

    case 'D':
    case 'd':
        remoteDelay();
        break;
    case 'L':
    case 'l':
        declareLabel();
        break;
    case 'J':
    case 'j':
        jumpToLabel();
        break;
    case 'M':
    case 'm':
        measureDistanceAndJump();
        break;
    case 'C':
    case 'c':
        jumpToLabelCoinToss();
        break;
    case 'T':
    case 't':
        compareAndJump(true);
        break;
    case 'F':
    case 'f':
        compareAndJump(false);
        break;
    }
}

// #define REMOTE_DOWNLOAD_DEBUG

// RM - start remote download

void remoteDownloadCommand()
{

#ifdef REMOTE_DOWNLOAD_DEBUG
    displayMessageWithNewline(F(".**remote download"));
#endif

#ifdef PROGRAM_DEBUG
    displayMessage(F("Remote download command decode pos:%s\n"), decodePos);
#endif
    if (getHullOSFileNameFromCode())
    {
        displayMessage(F("Storing in file:%s\n"), HullOScommandsFilenameBuffer);
    }
    else
    {
        displayMessage(F("Storing in %s\n"), RUNNING_PROGRAM_FILENAME);
        clearHullOSFilename();
    }
    startDownloadingCode();
}

void startProgramCommand(bool clearVariablesBeforeRun)
{

    //    displayMessage(F("Start program command decode pos:%s\n"), decodePos);

    if (getHullOSFileNameFromCode())
    {
        displayMessage(F("Got filename:%s\n"), HullOScommandsFilenameBuffer);
        if (loadFromFile(HullOScommandsFilenameBuffer, HullOScodeRunningCode, HULLOS_PROGRAM_SIZE))
        {
            dumpRunningProgram();
        }
    }
    else
    {
        displayMessage(F("Starting default program:%s\n"), RUNNING_PROGRAM_FILENAME);
        if (loadFromFile(RUNNING_PROGRAM_FILENAME, HullOScodeRunningCode, HULLOS_PROGRAM_SIZE))
        {
            dumpRunningProgram();
        }
    }

    startProgramExecution(clearVariablesBeforeRun);

#ifdef DIAGNOSTICS_ACTIVE
    if (diagnosticsOutputLevel & STATEMENT_CONFIRMATION)
    {
        displayMessageWithNewline(F("RSOK"));
    }
#endif
}

void haltProgramExecutionCommand()
{
    haltProgramExecution();

#ifdef DIAGNOSTICS_ACTIVE
    if (diagnosticsOutputLevel & STATEMENT_CONFIRMATION)
    {
        displayMessageWithNewline(F("RHOK"));
    }
#endif
}

void clearProgramStoreCommand()
{

    clearStoredProgram();

#ifdef DIAGNOSTICS_ACTIVE
    if (diagnosticsOutputLevel & STATEMENT_CONFIRMATION)
    {
        displayMessageWithNewline(F("RCOK"));
    }
#endif
}

void runProgramFromFileCommand(bool clearVariablesBeforeRun)
{

    displayMessage(F("Running program from file\n"));

    if (getHullOSFileNameFromCode())
    {
        displayMessage(F("Got filename:%s\n"), HullOScommandsFilenameBuffer);
    }
    else
    {
        displayMessage(F("No filename supplied to run\n"));
        clearHullOSFilename();
        return;
    }

    bool result = loadFromFile(
        HullOScommandsFilenameBuffer,
        HullOScodeRunningCode,
        HULLOS_PROGRAM_SIZE);

    if (!result)
    {
        displayMessage(F("File read failed\n"));
        clearHullOSFilename();
        return;
    }

    displayMessage(F("Running program in %s\n"), HullOScommandsFilenameBuffer);
    dumpRunningProgram();
    startProgramExecution(clearVariablesBeforeRun);
}

void saveCompiledProgramToFileCommand()
{
    displayMessage(F("Save compiled program into a file\n"));

    if (getHullOSFileNameFromCode())
    {
        displayMessage(F("Got save filename:%s\n"), HullOScommandsFilenameBuffer);
    }
    else
    {
        displayMessage(F("No filename supplied to save\n"));
        clearHullOSFilename();
        return;
    }

    saveToFile(HullOScommandsFilenameBuffer, HullOScodeCompileOutput);
}

void dumpFileCommand()
{
    displayMessage(F("Dump named file\n"));

    if (getHullOSFileNameFromCode())
    {
        displayMessage(F("  Got dump filename:%s\n"), HullOScommandsFilenameBuffer);
    }
    else
    {
        displayMessage(F("  No filename supplied to dump\n"));
        clearHullOSFilename();
        return;
    }

    printFileContents(HullOScommandsFilenameBuffer);
}

void listFilesCommand()
{
    displayMessage(F("List all files\n"));

    listLittleFSContents();
}

void removeFileCommand()
{
    displayMessage(F("Remove named file\n"));

    if (getHullOSFileNameFromCode())
    {
        displayMessage(F("  Got filename:%s\n"), HullOScommandsFilenameBuffer);
    }
    else
    {
        displayMessage(F("  No filename supplied to remove\n"));
        clearHullOSFilename();
        return;
    }

    removeFile(HullOScommandsFilenameBuffer);
}

char messageBuffer[MQTT_TEXT_BUFFER_SIZE + 1];

void transmitMQTTmessage()
{
    displayMessage(F("Transmit MQTT message\n"));

    int messageBufferPos = 0;

    while (*decodePos != STATEMENT_TERMINATOR & decodePos != decodeLimit)
    {
        messageBuffer[messageBufferPos] = *decodePos;
        decodePos++;
        messageBufferPos++;
        if (messageBufferPos == MQTT_TEXT_BUFFER_SIZE)
        {
            break;
        }
    }

    messageBuffer[messageBufferPos] = 0;

    publishBufferToMQTT(messageBuffer);
}

void transmitMQTTvalue()
{
    displayMessage(F("Transmit MQTT value\n"));

    int valueToTransmit;

    if (getValue(&valueToTransmit))
    {
        snprintf(messageBuffer, MQTT_TEXT_BUFFER_SIZE, "%d", valueToTransmit);
        publishBufferToMQTT(messageBuffer);
    }
}

void remoteManagement()
{
    if (*decodePos == STATEMENT_TERMINATOR | decodePos == decodeLimit)
    {
#ifdef DIAGNOSTICS_ACTIVE
        displayMessageWithNewline(F("FAIL: missing remote control command character"));
#endif
        return;
    }

#ifdef COMMAND_DEBUG
    displayMessageWithNewline(F(".**remoteProgramDownload: "));
#endif

    char commandCh = *decodePos;

#ifdef COMMAND_DEBUG
    displayMessage(F(".   Download command : "));
    displayMessageWithNewline(commandCh);
#endif

    decodePos++;

    switch (commandCh)
    {
    case 'C':
    case 'c':
        clearProgramStoreCommand();
        break;
    case 'D':
    case 'd':
        dumpFileCommand();
        break;
    case 'E':
    case 'e':
        runProgramFromFileCommand(false);
        break;
    case 'F':
    case 'f':
        runProgramFromFileCommand(true);
        break;
    case 'H':
    case 'h':
        haltProgramExecutionCommand();
        break;
    case 'k':
    case 'K':
        removeFileCommand();
        break;
    case 'l':
    case 'L':
        listFilesCommand();
        break;
    case 'M':
    case 'm':
        remoteDownloadCommand();
        break;
    case 'P':
    case 'p':
        pauseProgramExecution();
        break;
    case 'S':
    case 's':
        startProgramCommand(true);
        break;
    case 'R':
    case 'r':
        resumeProgramExecution();
        break;
    case 't':
    case 'T':
        transmitMQTTmessage();
        break;
    case 'V':
    case 'v':
        transmitMQTTvalue();
        break;
    case 'W':
    case 'w':
        saveCompiledProgramToFileCommand();
        break;
    }
}

// IV - information display version

void displayVersion()
{
#ifdef DIAGNOSTICS_ACTIVE
    if (diagnosticsOutputLevel & STATEMENT_CONFIRMATION)
    {
        displayMessageWithNewline(F("IVOK"));
    }
#endif

    displayMessageWithNewline(Version);
}

void displayDistance()
{
#ifdef DIAGNOSTICS_ACTIVE
    if (diagnosticsOutputLevel & STATEMENT_CONFIRMATION)
    {
        displayMessageWithNewline(F("IDOK"));
    }
#endif
    displayMessageWithNewline(F("%d"), getDistanceValueInt());
}

void printStatus()
{
#ifdef DIAGNOSTICS_ACTIVE
    if (diagnosticsOutputLevel & STATEMENT_CONFIRMATION)
    {
        displayMessageWithNewline(F("ISOK"));
    }
#endif
    displayMessage(F("state:%d diagnostics:%d\n"), (int)programState, diagnosticsOutputLevel);
}

// IMddd - set the debugging diagnostics level

// #define SET_MESSAGING_DEBUG

void setMessaging()
{

#ifdef SET_MESSAGING_DEBUG
    displayMessageWithNewline(F(".**informationlevelset: "));
#endif
    int result;

    if (!getValue(&result))
    {
        return;
    }

    byte no = (byte)result;

#ifdef SET_MESSAGING_DEBUG
    displayMessage(F(".  Setting: "));
    displayMessageWithNewline(no);
#endif

    diagnosticsOutputLevel = no;

#ifdef DIAGNOSTICS_ACTIVE
    if (diagnosticsOutputLevel & STATEMENT_CONFIRMATION)
    {
        displayMessageWithNewline(F("IMOK"));
    }
#endif
}

void printProgram()
{
    dumpRunningProgram();

#ifdef DIAGNOSTICS_ACTIVE
    if (diagnosticsOutputLevel & STATEMENT_CONFIRMATION)
    {
        displayMessageWithNewline(F("IPOK"));
    }
#endif
}

void information()
{
    if (*decodePos == STATEMENT_TERMINATOR | decodePos == decodeLimit)
    {
#ifdef DIAGNOSTICS_ACTIVE
        displayMessageWithNewline(F("FAIL: missing information command character"));
#endif
        return;
    }

#ifdef COMMAND_DEBUG
    displayMessageWithNewline(F(".**remoteProgramDownload: "));
#endif

    char commandCh = *decodePos;

#ifdef COMMAND_DEBUG
    displayMessage(F(".   Download command : "));
    displayMessageWithNewline(commandCh);
#endif

    decodePos++;

    switch (commandCh)
    {
    case 'V':
    case 'v':
        displayVersion();
        break;
    case 'D':
    case 'd':
        displayDistance();
        break;
    case 'S':
    case 's':
        printStatus();
        break;
    case 'M':
    case 'm':
        setMessaging();
        break;
    case 'P':
    case 'p':
        printProgram();
        break;
    }
}

void doClearVariables()
{
    clearVariables();

    if (diagnosticsOutputLevel & STATEMENT_CONFIRMATION)
    {
        displayMessage(F("VCOK"));
    }
}

void variableManagement()
{
    if (*decodePos == STATEMENT_TERMINATOR | decodePos == decodeLimit)
    {
#ifdef DIAGNOSTICS_ACTIVE
        displayMessageWithNewline(F("FAIL: missing variable command character"));
#endif
        return;
    }

#ifdef COMMAND_DEBUG
    displayMessageWithNewline(F(".**variable management: "));
#endif

    char commandCh = *decodePos;

#ifdef COMMAND_DEBUG
    displayMessage(F(".   Download command : "));
    displayMessageWithNewline(commandCh);
#endif

    decodePos++;

    switch (commandCh)
    {
    case 'C':
    case 'c':
        doClearVariables();
        break;

    case 'S':
    case 's':
        setVariable();
        break;

    case 'V':
    case 'v':
        viewVariable();
        break;
    }
}

// Command STfreq,time - play tone with freq for given time
// waits for tone to complete playing if wait is true
// Return OK

void doTone()
{
    int frequency;

#ifdef PLAY_TONE_DEBUG
    displayMessageWithNewline(F(".**play tone"));
#endif

    if (*decodePos == STATEMENT_TERMINATOR)
    {
#ifdef DIAGNOSTICS_ACTIVE
        if (diagnosticsOutputLevel & STATEMENT_CONFIRMATION)
        {
            displayMessageWithNewline(F("FAIL: no tone frequency"));
        }
#endif
        return;
    }

    if (!getValue(&frequency))
    {
        return;
    }

    if (*decodePos == STATEMENT_TERMINATOR)
    {
#ifdef DIAGNOSTICS_ACTIVE
        if (diagnosticsOutputLevel & STATEMENT_CONFIRMATION)
        {
            displayMessageWithNewline(F("FAIL: no tone frequency"));
        }
#endif
        return;
    }

    decodePos++; // move past the separator

    if (*decodePos == STATEMENT_TERMINATOR)
    {
#ifdef DIAGNOSTICS_ACTIVE
        if (diagnosticsOutputLevel & STATEMENT_CONFIRMATION)
        {
            displayMessageWithNewline(F("FAIL: no tone duration"));
        }
#endif
        return;
    }

    int duration;

    if (!getValue(&duration))
    {
        return;
    }

    if (*decodePos == STATEMENT_TERMINATOR)
    {
#ifdef DIAGNOSTICS_ACTIVE
        if (diagnosticsOutputLevel & STATEMENT_CONFIRMATION)
        {
            displayMessageWithNewline(F("FAIL: no wait "));
        }
#endif
        return;
    }

    decodePos++;

    switch (*decodePos)
    {
    case 'W':
    case 'w':
        delayEndTime = millis() + duration;
        programState = PROGRAM_AWAITING_DELAY_COMPLETION;
        // Note that this code intentionally falls through the end of the case
    case 'n':
    case 'N':
//        playTone(frequency, duration);
#ifdef DIAGNOSTICS_ACTIVE
        if (diagnosticsOutputLevel & STATEMENT_CONFIRMATION)
        {
            displayMessageWithNewline(F("SOUNDOK"));
        }
#endif
        break;
    default:
        break;

#ifdef DIAGNOSTICS_ACTIVE
        if (diagnosticsOutputLevel & STATEMENT_CONFIRMATION)
        {
            displayMessageWithNewline(F("FAIL: no wait "));
        }
#endif
    }
}

void remoteSoundPlay()
{
    if (*decodePos == STATEMENT_TERMINATOR | decodePos == decodeLimit)
    {
#ifdef DIAGNOSTICS_ACTIVE
        displayMessageWithNewline(F("FAIL: missing sound command character"));
#endif
        return;
    }

#ifdef COMMAND_DEBUG
    displayMessageWithNewline(F(".**sound playback: "));
#endif

    char commandCh = *decodePos;

#ifdef COMMAND_DEBUG
    displayMessage(F(".   Download command : "));
    displayMessageWithNewline(commandCh);
#endif

    decodePos++;

    switch (commandCh)
    {
    case 'T':
    case 't':
        doTone();
        break;
    }
}

void doRemoteWriteText()
{
    while (*decodePos != STATEMENT_TERMINATOR & decodePos != decodeLimit)
    {
        displayMessage(F("%c"), *decodePos);
        decodePos++;
    }
}

void doRemoteWriteLine()
{
    displayMessage(F("\n"));
}

void doRemotePrintValue()
{
    int valueToPrint;

    if (getValue(&valueToPrint))
    {
        displayMessage(F("%d"), valueToPrint);
    }
}

void remoteWriteOutput()
{
    if (*decodePos == STATEMENT_TERMINATOR | decodePos == decodeLimit)
    {
#ifdef DIAGNOSTICS_ACTIVE
        displayMessageWithNewline(F("FAIL: missing write output command character"));
#endif
        return;
    }

#ifdef COMMAND_DEBUG
    displayMessageWithNewline(F(".**write output: "));
#endif

    char commandCh = *decodePos;

#ifdef COMMAND_DEBUG
    displayMessage(F(".   Download command : "));
    displayMessageWithNewline(commandCh);
#endif

    decodePos++;

    switch (commandCh)
    {
    case 'T':
    case 't':
        doRemoteWriteText();
        break;

    case 'L':
    case 'l':
        doRemoteWriteLine();
        break;

    case 'V':
    case 'v':
        doRemotePrintValue();
        break;
    }
}

void absorbCommandResult(char *resultText)
{
    displayMessage(resultText);
}

#define CONSOLE_COMMAND_BUFFER_SIZE 200

void buildAndExecuteConsoleCommand(){
    char buffer[CONSOLE_COMMAND_BUFFER_SIZE];
    int bpos = 0;

    while (*decodePos != STATEMENT_TERMINATOR && decodePos != decodeLimit && bpos<CONSOLE_COMMAND_BUFFER_SIZE-1 )
    {
        buffer[bpos] = *decodePos;
        bpos++;
        decodePos++;
    }

    buffer[bpos]=0;

    actOnConsoleCommandText(buffer);
}

void hullOSExecuteStatement(char *commandDecodePos, char *comandDecodeLimit)
{
    decodePos = commandDecodePos;
    decodeLimit = comandDecodeLimit;

    //    *decodeLimit = 0;

#ifdef COMMAND_DEBUG

    dumpRunningProgram();

    displayMessage(F(".**processCommand:"));
    char *dump_pos = commandDecodePos;
    while (dump_pos != comandDecodeLimit)
    {
        displayMessage(F("%c"), *dump_pos);
        dump_pos++;
    }
    displayMessage(F("\n"));
#endif

    char commandCh = *decodePos;

#ifdef COMMAND_DEBUG
    displayMessage(F(".  Command code : "));
    displayMessageWithNewline(commandCh);
#endif

    decodePos++;

    switch (commandCh)
    {

    case '!':
        buildAndExecuteConsoleCommand();
        break;

    case '{':
        // It's a JSON formatted command
        act_onJson_message(commandDecodePos, absorbCommandResult);
        break;
    case '#':
        // Ignore comments
        break;
    case 'I':
    case 'i':
        information();
        break;
    case 'M':
    case 'm':
#ifdef PROCESS_REMOTE_ROBOT_DRIVE
        sendStatementToRobot(decodePos - 1);
#endif
#ifdef PROCESS_MOTOR
        remoteMoveControl();
#endif
        break;
    case 'P':
    case 'p':
#ifdef PROCESS_REMOTE_ROBOT_DRIVE
        sendStatementToRobot(decodePos - 1);
#else
        remotePixelControl();
#endif
        break;
    case 'C':
    case 'c':
        programControl();
        break;
    case 'R':
    case 'r':
        remoteManagement();
        break;
    case 'V':
    case 'v':
        variableManagement();
        break;
    case 's':
    case 'S':
#ifdef PROCESS_REMOTE_ROBOT_DRIVE
        sendStatementToRobot(decodePos - 1);
#else
        remoteSoundPlay();
#endif
        break;
    case 'w':
    case 'W':
        remoteWriteOutput();
        break;

    default:
#ifdef COMMAND_DEBUG
        displayMessageWithNewline(F(".  Invalid command : "));
#endif
#ifdef DIAGNOSTICS_ACTIVE
        if (diagnosticsOutputLevel & STATEMENT_CONFIRMATION)
        {
            displayMessage(F("Invalid Command:%c code:%d\n"), commandCh, (int)commandCh);
        }
#endif
        break;
    }
}

void hullOSStoreStatement(char *commandDecodePos, char *comandDecodeLimit)
{
    while (commandDecodePos != comandDecodeLimit)
    {
        storeReceivedByte(*commandDecodePos);
        commandDecodePos++;
    }
}

void hullOSActOnStatement(char *commandDecodePos, char *comandDecodeLimit)
{
    switch (interpreterState)
    {
    case EXECUTE_IMMEDIATELY:
        // This might change the interpreter state to STORE_PROGRAM
        hullOSExecuteStatement(commandDecodePos, comandDecodeLimit);
        break;

    case STORE_PROGRAM:
        // This might change the interpreter state to EXECUTE_IMMEDIATELY
        hullOSStoreStatement(commandDecodePos, comandDecodeLimit);
        break;
    default:
        displayMessageWithNewline(F("Invalid interpreter state"));
        break;
    }
}

void setupHullOSReceiver()
{
#ifdef COMMAND_DEBUG
    displayMessageWithNewline(F(".**setupHullOSReceiver"));
#endif
    resetCommand();
    resetSerialBuffer();
}

// Executes the statement at the current program counter

bool executeProgramStatement()
{
    char programByte;

#ifdef PROGRAM_DEBUG
    displayMessageWithNewline(F(".Executing statement"));
#endif

#ifdef DIAGNOSTICS_ACTIVE
    if (diagnosticsOutputLevel & LINE_NUMBERS)
    {
        displayMessageWithNewline(F("Offset:%d"), (int)programCounter);
    }
#endif

    char *statementStart = HullOScodeRunningCode + programCounter;
    int statementLength = 0;

    while (true)
    {
        programByte = HullOScodeRunningCode[programCounter++];

#ifdef PROGRAM_DEBUG
        displayMessage(F(".    program byte: "));
        displayMessageWithNewline(programByte);
#endif

        if (programCounter >= HULLOS_PROGRAM_SIZE || programByte == PROGRAM_TERMINATOR)
        {
            if (statementLength > 0)
            {
                hullOSExecuteStatement(statementStart, statementStart + statementLength);
            }
            haltProgramExecution();
            return false;
        }

        if (programByte == STATEMENT_TERMINATOR)
        {
            hullOSExecuteStatement(statementStart, statementStart + statementLength);
            return true;
        }
    }
}

void programStatus(char *buffer, int bufferLength)
{
    switch (programState)
    {
    case PROGRAM_STOPPED:
        snprintf(buffer, bufferLength, "HullOS Program stopped");
    case PROGRAM_PAUSED:
        snprintf(buffer, bufferLength, "HullOS Program paused");
        break;
    case PROGRAM_ACTIVE:
        snprintf(buffer, bufferLength, "HullOS Program active");
        break;
    case PROGRAM_AWAITING_MOVE_COMPLETION:
        snprintf(buffer, bufferLength, "HullOS Awaiting move completion");
        break;
    case PROGRAM_AWAITING_DELAY_COMPLETION:
        snprintf(buffer, bufferLength, "HullOS Awaiting delay completion");
        break;
    default:
        snprintf(buffer, bufferLength, "HullOS Invalid program state");
        break;
    }
}

void updateRunningProgram()
{
    // If we receive serial data the program that is running
    // must stop.
    switch (programState)
    {
    case PROGRAM_STOPPED:
    case PROGRAM_PAUSED:
        break;
    case PROGRAM_ACTIVE:
        executeProgramStatement();
        break;
#if defined(PROCESS_MOTOR) || defined(PROCESS_REMOTE_ROBOT_DRIVE)
    case PROGRAM_AWAITING_MOVE_COMPLETION:
        if (!motorsMoving())
        {
            programState = PROGRAM_ACTIVE;
        }
        break;
#endif
    case PROGRAM_AWAITING_DELAY_COMPLETION:
        if (millis() > delayEndTime)
        {
            programState = PROGRAM_ACTIVE;
        }
        break;
    }
}

#ifdef TEST_PROGRAM

// const char SAMPLE_CODE[] = { "PC255,0,0\rCD5\rCLtop\rPC0,0,255\rCD5\rPC0,255,255\rCD5\rPC255,0,255\rCD5\rCJtop\r" };
const char SAMPLE_CODE[] = {"CLtop\rCM10,close\rPC255,0,0\rCJtop\rCLclose\rPC0,0,255\rCJtop\r"};

void loadTestProgram(int offset)
{
    int inPos = 0;
    int outPos = offset;

    int len = strlen_P(SAMPLE_CODE);
    int i;
    char myChar;

    for (i = 0; i < len; i++)
    {
        myChar = pgm_read_byte_near(SAMPLE_CODE + i);
        EEPROM.write(outPos++, myChar);
    }

    EEPROM.write(outPos, 0);
}

#endif
