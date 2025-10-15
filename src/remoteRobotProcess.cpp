#if defined(PROCESS_REMOTE_ROBOT_DRIVE)

#include <Arduino.h>

#include "utils.h"
#include "settings.h"
#include "remoteRobotProcess.h"
#include "HullOSScript.h"
#include "processes.h"
#include "mqtt.h"
#include "errors.h"
#include "clock.h"
#include "console.h"
#include "statusled.h"
#include "console.h"

#if defined(ARDUINO_ARCH_ESP8266)

#include "SoftwareSerial.h"

EspSoftwareSerial::UART robotSwSer;

#endif

#define ROBOT_MESSAGE_BUFFER_SIZE 256
#define ROBOT_MESSAGE_INTERVAL_MSECS 50

struct robotSettings robotSettings;

void setDefaultrobotTXpinNo(void *dest)
{
    int *destInt = (int *)dest;
    *destInt = 15;
}

struct SettingItem robotTXpinNo = {
    "robot TX Pin",
    "robotTXPin",
    &robotSettings.robotTXPin,
    NUMBER_INPUT_LENGTH,
    integerValue,
    setDefaultrobotTXpinNo,
    validateInt};

void setDefaultrobotRXpinNo(void *dest)
{
    int *destInt = (int *)dest;
    *destInt = 13;
}

struct SettingItem robotRXpinNo = {
    "robot RX Pin",
    "robotRXPin",
    &robotSettings.robotRXPin,
    NUMBER_INPUT_LENGTH,
    integerValue,
    setDefaultrobotRXpinNo,
    validateInt};

void setDefaultrobotBaudRate(void *dest)
{
    int *destInt = (int *)dest;
    *destInt = 1200;
}

struct SettingItem robotBaudRate = {
    "Robot baud rate",
    "robotbaud",
    &robotSettings.robotBaudRate,
    NUMBER_INPUT_LENGTH,
    integerValue,
    setDefaultrobotBaudRate,
    validateInt};

struct SettingItem robotEnabledSetting = {
    "Robot enabled",
    "roboton",
    &robotSettings.robotEnabled,
    ONOFF_INPUT_LENGTH,
    yesNo,
    setFalse,
    validateYesNo};

struct SettingItem *robotSettingItemPointers[] =
    {
        &robotEnabledSetting,
        &robotBaudRate,
        &robotTXpinNo,
        &robotRXpinNo};

struct SettingItemCollection robotMessagesSettingItems = {
    "robotSettings",
    "robot setup",
    robotSettingItemPointers,
    sizeof(robotSettingItemPointers) / sizeof(struct SettingItem *)};

void robotMessagesOff()
{
    robotSettings.robotEnabled = false;
    saveSettings();
}

void robotMessagesOn()
{
    robotSettings.robotEnabled = true;
    saveSettings();
}

void sendRobotMessageToServer(char *messageText)
{
    char messageBuffer[ROBOT_MESSAGE_BUFFER_SIZE];
    char deviceNameBuffer[DEVICE_NAME_LENGTH];

    PrintSystemDetails(deviceNameBuffer, DEVICE_NAME_LENGTH);

    snprintf(messageBuffer, ROBOT_MESSAGE_BUFFER_SIZE,
             "{\"name\":\"%s\",\"from\":\"robot\",\"message\":\"%s\"}",
             deviceNameBuffer,
             messageText);

    publishBufferToMQTTTopic(messageBuffer, ROBOT_TOPIC);
}

void sendMessageToRobot(char *messageText)
{
    // displayMessage("Sending robot message: %s\n", messageText);

#if defined(ARDUINO_ARCH_ESP8266)
    robotSwSer.print((char)0x0d);
    robotSwSer.print(messageText);
    robotSwSer.print((char)0x0d);
    robotSwSer.print((char)0x0a);
    robotSwSer.flush();
#else
    // TODO: Add the serial transmission for other devices
    displayMessage("**** Message sending to robots is not yet implemented for this platform.");
#endif
}

#define STATEMENT_BUFFER_SIZE 100
char buffer[STATEMENT_BUFFER_SIZE];

void sendStatementToRobot(char *statementText)
{
    int bpos = 0;

    buffer[bpos++] = '*';

    while (*statementText != STATEMENT_TERMINATOR)
    {
        buffer[bpos++] = *statementText++;
        if (bpos == STATEMENT_BUFFER_SIZE - 1)
        {
            break;
        }
    }
    buffer[bpos] = 0;
    sendMessageToRobot(buffer);
}

#define robot_FLOAT_VALUE_OFFSET 0
#define robot_MESSAGE_OFFSET (robot_FLOAT_VALUE_OFFSET + sizeof(float))

boolean validaterobotOptionString(void *dest, const char *newValueStr)
{
    return (validateString((char *)dest, newValueStr, ROBOT_MESSAGE_COMMAND_LENGTH));
}

boolean validaterobotMessageString(void *dest, const char *newValueStr)
{
    return (validateString((char *)dest, newValueStr, ROBOT_MESSAGE_LENGTH));
}

struct CommandItem robotMessageText = {
    "text",
    "robot message text",
    robot_MESSAGE_OFFSET,
    textCommand,
    validaterobotMessageString,
    noDefaultAvailable};

struct CommandItem *RobotCommandItems[] =
    {
        &robotMessageText};

int doSendRobotMessage(char *destination, unsigned char *settingBase);

struct Command robotMessageCommand{
    "Send",
    "Sends a message to the robot",
    RobotCommandItems,
    sizeof(RobotCommandItems) / sizeof(struct CommandItem *),
    doSendRobotMessage};

int doSendRobotMessage(char *destination, unsigned char *settingBase)
{
    if (*destination != 0)
    {
        // we have a destination for the command. Build the string
        char buffer[JSON_BUFFER_SIZE];
        createJSONfromSettings("robot", &robotMessageCommand, destination, settingBase, buffer, JSON_BUFFER_SIZE);
        return publishCommandToRemoteDevice(buffer, destination);
    }

    if (robotProcess.status != ROBOT_CONNECTED)
    {
        return JSON_MESSAGE_ROBOT_NOT_ENABLED;
    }

    char *message = (char *)(settingBase + robot_MESSAGE_OFFSET);

    sendMessageToRobot(message);

    TRACELOG("Sending: ");
    TRACELOGLN(buffer);

    return WORKED_OK;
}

struct Command *robotCommandList[] = {
    &robotMessageCommand};

struct CommandItemCollection robotCommands =
    {
        "Control robot",
        robotCommandList,
        sizeof(robotCommandList) / sizeof(struct Command *)};

enum RobotCommsState
{
    robotIdle,
    robotRequestActive
};

int robotDistanceValue = 0;

RobotCommsState robotCommsState;

unsigned long timeOfLastRobotMessage = 0;

#define ROBOT_MESSAGE_INTERVAL_MILLIS 5
#define ROBOT_MESSAGE_REPLY_TIMEOUT_MILLIS 100

struct RobotMessage
{
    void (*sender)();
    void (*reader)(char *);
};

void readRobotDistanceSendRequest()
{
    sendMessageToRobot("*id");
}

void readRobotDistanceProcessResponse(char *buffer)
{
    // displayMessage("Read Robot Distance called: %s\n", buffer);

    int receivedDistanceValue;

    if (sscanf(buffer, "%d", &receivedDistanceValue) == 1)
    {
        // displayMessage("Got Robot Distance:%d\n", receivedDistanceValue);
        robotDistanceValue = receivedDistanceValue;
    }
}

RobotMessage readRobotDistanceMessage = {
    readRobotDistanceSendRequest,
    readRobotDistanceProcessResponse};

volatile bool remoteRobotMoving = false;

unsigned long readMovingRequestLastSent;
unsigned long moveRobotRequestLastSent;

// Called when the robot is moved 
// Set the state of the remoteRobotMoving flag to reflect the command and 
// record the time of the last robot move request to invalidate move status messages
// requested before the move was performed. 

MoveFailReason timedMoveSteps(long leftStepsToMove, long rightStepsToMove, float timeToMoveInSeconds){

    if((leftStepsToMove==0)&&(rightStepsToMove==0)){
        // stopping robot
         remoteRobotMoving = false;
    }
    else {
        // robot is moving
         remoteRobotMoving = true;
    }

    moveRobotRequestLastSent = millis();

    return Move_OK;
}

void motorStop()
{
    sendMessageToRobot("*MM0,0");
    remoteRobotMoving = false;
    moveRobotRequestLastSent = millis();
}

int motorsMoving()
{
    return remoteRobotMoving;
}

void readRobotMotorSendRequest()
{
    readMovingRequestLastSent = millis();
    sendMessageToRobot("*MC");
}

void readRobotMotorProcessResponse(char *buffer)
{
    if (moveRobotRequestLastSent > readMovingRequestLastSent)
    {
        // move request was sent after last read moving request
        // ignore this incoming value as it may be out of date
        return;
    }
    if (strcasecmp(buffer, "MCstopped") == 0)
    {
        remoteRobotMoving = false;
    }
    if (strcasecmp(buffer, "MCmove") == 0)
    {
        remoteRobotMoving = true;
    }
}

RobotMessage readRobotMotorMessage = {readRobotMotorSendRequest,
                                      readRobotMotorProcessResponse};

struct RobotMessage *robotMessages[] = {
    &readRobotDistanceMessage,
    &readRobotMotorMessage};

#define NO_OF_ROBOT_MESSAGES sizeof(robotMessages) / sizeof(struct robotMessage *)

int currentRobotMessageNumber = 0;

void initRobotMessageSending()
{
    currentRobotMessageNumber = 0;
    timeOfLastRobotMessage = millis();
    robotCommsState = robotIdle;
}

void sendNextRobotMessage()
{
    RobotMessage *currentMessage = robotMessages[currentRobotMessageNumber];
    currentMessage->sender();
}

void performValueUpdate(char *buffer)
{
    RobotMessage *currentMessage = robotMessages[currentRobotMessageNumber];
    void (*reader)(char *) = currentMessage->reader;
    reader(buffer);
    currentRobotMessageNumber++;
    if (currentRobotMessageNumber == NO_OF_ROBOT_MESSAGES)
    {
        currentRobotMessageNumber = 0;
    }
}

#define ROBOT_BUFFER_SIZE 129
#define ROBOT_BUFFER_LIMIT ROBOT_BUFFER_SIZE - 1

char robotReceiveBuffer[ROBOT_BUFFER_SIZE];

int robotReceiveBufferPos = 0;

void reset_robot_buffer()
{
    robotReceiveBufferPos = 0;
}

void checkRobotSendMessage()
{
    unsigned long now = millis();

    if (ulongDiff(now, timeOfLastRobotMessage) > ROBOT_MESSAGE_INTERVAL_MILLIS)
    {
        sendNextRobotMessage();
        timeOfLastRobotMessage = now;
        robotCommsState = robotRequestActive;
    }
}

void checkRobotReceivedReply()
{
    unsigned long now = millis();

    if (ulongDiff(now, timeOfLastRobotMessage) > ROBOT_MESSAGE_REPLY_TIMEOUT_MILLIS)
    {
        sendNextRobotMessage();
        timeOfLastRobotMessage = now;
        reset_robot_buffer();
        robotCommsState = robotRequestActive;
    }
}


void processMessageFromRobot(char *buffer)
{
    // displayMessage("Robot Message: %s\n", buffer);
    performValueUpdate(buffer);
    robotCommsState = robotIdle;
}

void initRobotProcess()
{
    remoteRobotMoving=false;

    initRobotMessageSending();
}
void startRobotProcess()
{
    reset_robot_buffer();

    if (robotSettings.robotEnabled)
    {
        robotProcess.status = ROBOT_CONNECTED;

#if defined(ARDUINO_ARCH_ESP8266)

        robotSwSer.begin(
            robotSettings.robotBaudRate,
            EspSoftwareSerial::SWSERIAL_8N1,
            robotSettings.robotRXPin,
            robotSettings.robotTXPin);
        robotSwSer.enableIntTx(true);

        // stop any program that might be running in the robot

        sendMessageToRobot("*rh");

#else

        // TODO: add the serial port opening for the other devices
        displayMessage("**** Serial port opening for robots is not yet implemented for this platform.");

#endif
    }
    else
    {
        robotProcess.status = ROBOT_OFF;
    }
}

void bufferRobotSerialChar(char ch)
{
    if (ch == '\n' || ch == '\r' || ch == 0)
    {
        if (robotReceiveBufferPos > 0)
        {
            robotReceiveBuffer[robotReceiveBufferPos] = 0;
            processMessageFromRobot(robotReceiveBuffer);
            reset_robot_buffer();
        }
        return;
    }

    if (robotReceiveBufferPos < ROBOT_BUFFER_SIZE)
    {
        robotReceiveBuffer[robotReceiveBufferPos] = ch;
        robotReceiveBufferPos++;
    }
}

void checkRobotBuffer()
{

#if defined(ARDUINO_ARCH_ESP8266)

    while (robotSwSer.available())
    {
        bufferRobotSerialChar(robotSwSer.read());
    }
#endif
}

int getDistanceFromRobot()
{
    return robotDistanceValue;
}

void updateRobotProcess()
{
    if (robotProcess.status != ROBOT_CONNECTED)
    {
        return;
    }

    switch (robotCommsState)
    {
    case robotIdle:
        checkRobotSendMessage();
        break;
    case robotRequestActive:
        checkRobotReceivedReply();
        break;
    }

    checkRobotBuffer();
}

void stopRobotProcess()
{
    robotProcess.status = ROBOT_OFF;
}

bool robotStatusOK()
{
    return robotProcess.status == ROBOT_CONNECTED;
}

void robotStatusMessage(char *buffer, int bufferLength)
{
    if (robotProcess.status == ROBOT_OFF)
    {
        snprintf(buffer, bufferLength, "robot off");
    }
    else
    {
        snprintf(buffer, bufferLength, "robot on");
    }
}

struct process robotProcess = {
    "robot",
    initRobotProcess,
    startRobotProcess,
    updateRobotProcess,
    stopRobotProcess,
    robotStatusOK,
    robotStatusMessage,
    false,
    0,
    0,
    0,
    NULL,
    (unsigned char *)&robotSettings, sizeof(robotSettings), &robotMessagesSettingItems,
    &robotCommands,
    BOOT_PROCESS + ACTIVE_PROCESS + CONFIG_PROCESS + WIFI_CONFIG_PROCESS,
    NULL,
    NULL,
    NULL,
    NULL, // no command options
    0     // no command options
};

#endif