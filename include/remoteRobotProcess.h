#if defined(PROCESS_REMOTE_ROBOT_DRIVE)

#pragma once

#include <Arduino.h>
#include "Motors.h"

#define ROBOT_CONNECTED 2000
#define ROBOT_OFF 2001

#define ROBOT_MESSAGE_LENGTH 60
#define ROBOT_MESSAGE_COMMAND_LENGTH 10

#define ROBOT_TOPIC "robot"

MoveFailReason timedMoveSteps(long leftStepsToMove, long rightStepsToMove, float timeToMoveInSeconds);

struct robotSettings
{
    bool robotEnabled;
    int robotBaudRate;
    int robotTXPin;
    int robotRXPin;
};

void sendMessageToRobot(char *messageText);

void sendStatementToRobot(char *statementText);

int getDistanceFromRobot();

void robotOff();

void robotOn();

extern struct robotSettings robotSettings;

extern struct SettingItemCollection robotSettingItems;

extern struct process robotProcess;

#endif