#ifdef PROCESS_MOTOR

#include "Motors.h"

#if STEPPER_MOTOR_DRIVE
#include "stepperDrive.h"
#endif

#if PROCESS_REMOTE_ROBOT_DRIVE
#include "remoteRobotProcess.h"
#endif

#include "utils.h"

#include <Arduino.h>
#include <math.h>

#include "settings.h"
#include "processes.h"
#include "errors.h"
#include "mqtt.h"
#include "Motors.h"

float turningCircle;
const int countsperrev = 512 * 8; // number of microsteps per full revolution

float leftStepsPerMM;
float rightStepsPerMM;
float leftWheelCircumference;
float rightWheelCircumference;

struct MotorSettings motorSettings;

struct SettingItem motorsOnOffSetting = {
    "Motors Active (yes or no)",
    "motors",
    &motorSettings.motorsActive,
    ONOFF_INPUT_LENGTH,
    yesNo,
    setFalse,
    validateYesNo};

void setDefaultWheelDiameter(void *dest)
{
  int *destInt = (int *)dest;
  *destInt = 69;
}

void dumpActiveWheelSettings()
{
  displayMessageWithNewline(F("Wheel settings"));
  displayMessageWithNewline(F("Left diameter: %d"), motorSettings.leftWheelDiameter);
  displayMessageWithNewline(F("Right diameter: %d"), motorSettings.rightWheelDiameter);
  displayMessageWithNewline(F("Wheel spacing: %d"), motorSettings.wheelSpacing);
}

void setActiveWheelSettings(int leftDiam, int rightDiam, int spacing)
{
  motorSettings.leftWheelDiameter = leftDiam;
  motorSettings.rightWheelDiameter = rightDiam;
  motorSettings.wheelSpacing = spacing;

  saveSettings();
}

struct SettingItem motorLeftWheelDiamSetting = {"Left wheel diameter",
                                                "motorleftwheeldiam",
                                                &motorSettings.leftWheelDiameter,
                                                NUMBER_INPUT_LENGTH,
                                                integerValue,
                                                setDefaultWheelDiameter,
                                                validateInt};

struct SettingItem motorRightWheelDiamSetting = {"Right wheel diameter",
                                                 "motorrightwheeldiam",
                                                 &motorSettings.rightWheelDiameter,
                                                 NUMBER_INPUT_LENGTH,
                                                 integerValue,
                                                 setDefaultWheelDiameter,
                                                 validateInt};

void setDefaultWheelSpacing(void *dest)
{
  int *destInt = (int *)dest;
  *destInt = 110;
}

struct SettingItem motorWheelSpacingSetting = {"Wheel spacing",
                                               "motorwheelspacing",
                                               &motorSettings.wheelSpacing,
                                               NUMBER_INPUT_LENGTH,
                                               integerValue,
                                               setDefaultWheelSpacing,
                                               validateInt};

#if STEPPER_MOTOR_DRIVE

void setDefaultLeftMotorPin1(void *dest)
{
  int *destInt = (int *)dest;
  *destInt = 15;
}

struct SettingItem motorLeftPin1 = {"Left motor pin 1",
                                    "leftmotorpin1",
                                    &motorSettings.lpin1,
                                    NUMBER_INPUT_LENGTH,
                                    integerValue,
                                    setDefaultLeftMotorPin1,
                                    validateInt};

void setDefaultLeftMotorPin2(void *dest)
{
  int *destInt = (int *)dest;
  *destInt = 14;
}

struct SettingItem motorLeftPin2 = {"Left motor pin 2",
                                    "leftmotorpin2",
                                    &motorSettings.lpin2,
                                    NUMBER_INPUT_LENGTH,
                                    integerValue,
                                    setDefaultLeftMotorPin2,
                                    validateInt};

void setDefaultLeftMotorPin3(void *dest)
{
  int *destInt = (int *)dest;
  *destInt = 13;
}

struct SettingItem motorLeftPin3 = {"Left motor pin 3",
                                    "leftmotorpin3",
                                    &motorSettings.lpin3,
                                    NUMBER_INPUT_LENGTH,
                                    integerValue,
                                    setDefaultLeftMotorPin3,
                                    validateInt};

void setDefaultLeftMotorPin4(void *dest)
{
  int *destInt = (int *)dest;
  *destInt = 12;
}

struct SettingItem motorLeftPin4 = {"Left motor pin 4",
                                    "leftmotorpin4",
                                    &motorSettings.lpin4,
                                    NUMBER_INPUT_LENGTH,
                                    integerValue,
                                    setDefaultLeftMotorPin4,
                                    validateInt};

void setDefaultRightMotorPin1(void *dest)
{
  int *destInt = (int *)dest;
  *destInt = 8;
}

struct SettingItem motorRightPin1 = {"Right motor pin 1",
                                     "rightmotorpin1",
                                     &motorSettings.rpin1,
                                     NUMBER_INPUT_LENGTH,
                                     integerValue,
                                     setDefaultRightMotorPin1,
                                     validateInt};

void setDefaultRightMotorPin2(void *dest)
{
  int *destInt = (int *)dest;
  *destInt = 9;
}

struct SettingItem motorRightPin2 = {"Right motor pin 2",
                                     "rightmotorpin2",
                                     &motorSettings.rpin2,
                                     NUMBER_INPUT_LENGTH,
                                     integerValue,
                                     setDefaultRightMotorPin2,
                                     validateInt};

void setDefaultRightMotorPin3(void *dest)
{
  int *destInt = (int *)dest;
  *destInt = 10;
}

struct SettingItem motorRightPin3 = {"Right motor pin 3",
                                     "rightmotorpin3",
                                     &motorSettings.rpin3,
                                     NUMBER_INPUT_LENGTH,
                                     integerValue,
                                     setDefaultRightMotorPin3,
                                     validateInt};

void setDefaultRightMotorPin4(void *dest)
{
  int *destInt = (int *)dest;
  *destInt = 11;
}

struct SettingItem motorRightPin4 = {"Right motor pin 4",
                                     "rightmotorpin4",
                                     &motorSettings.rpin4,
                                     NUMBER_INPUT_LENGTH,
                                     integerValue,
                                     setDefaultRightMotorPin4,
                                     validateInt};

#endif

struct SettingItem *motorSettingItemPointers[] =
    {
        &motorsOnOffSetting,
        &motorLeftWheelDiamSetting,
        &motorRightWheelDiamSetting,
        &motorWheelSpacingSetting
#if STEPPER_MOTOR_DRIVE
        ,&motorLeftPin1,
        &motorLeftPin2,
        &motorLeftPin3,
        &motorLeftPin4,
        &motorRightPin1,
        &motorRightPin2,
        &motorRightPin3,
        &motorRightPin4
#endif
};

struct SettingItemCollection motorSettingItems = {
    "motor",
    "Motor and wheel configuration",
    motorSettingItemPointers,
    sizeof(motorSettingItemPointers) / sizeof(struct SettingItem *)};

void setupWheelSettings()
{
  leftWheelCircumference = PI * motorSettings.leftWheelDiameter;
  rightWheelCircumference = PI * motorSettings.rightWheelDiameter;
  turningCircle = motorSettings.wheelSpacing * PI;

  leftStepsPerMM = countsperrev / leftWheelCircumference;
  rightStepsPerMM = countsperrev / rightWheelCircumference;
}

float fastMoveSteps(long leftStepsToMove, long rightStepsToMove)
{

#ifdef DEBUG_FAST_MOVE_STEPS
  displayMessageWithNewline(F("fastMoveSteps"));
  Serial.print("    Left steps to move: ");
  Serial.print(leftStepsToMove);
  Serial.print(" Right steps to move: ");
  displayMessageWithNewline(rightStepsToMove);
#endif

  // work out how long it will take to move in seconds

  float timeForLeftMoveInSeconds;

  if (leftStepsToMove != 0)
  {
    timeForLeftMoveInSeconds = ((float)abs(leftStepsToMove) * (float)MIN_MOTOR_INTERRUPT_INTERVAL_MICROSECS) / 1000000.0;
  }
  else
  {
    timeForLeftMoveInSeconds = 0;
  }

  float timeForRightMoveInSeconds;

  if (rightStepsToMove != 0)
  {
    timeForRightMoveInSeconds = ((float)abs(rightStepsToMove) * (float)MIN_MOTOR_INTERRUPT_INTERVAL_MICROSECS) / 1000000.0;
  }
  else
  {
    timeForRightMoveInSeconds = 0;
  }

#ifdef DEBUG_FAST_MOVE_STEPS
  Serial.print("    Left time to move: ");
  Serial.print(timeForLeftMoveInSeconds);
  Serial.print(" Right time to move: ");
  displayMessageWithNewline(timeForRightMoveInSeconds);
#endif

  // Allow time for the slowest mover
  if (timeForLeftMoveInSeconds > timeForRightMoveInSeconds)
  {
    timedMoveSteps(leftStepsToMove, rightStepsToMove, timeForLeftMoveInSeconds);
    return timeForLeftMoveInSeconds;
  }
  else
  {
    timedMoveSteps(leftStepsToMove, rightStepsToMove, timeForRightMoveInSeconds);
    return timeForRightMoveInSeconds;
  }
}

int timedMoveDistanceInMM(float leftMMs, float rightMMs, float timeToMoveInSeconds)
{

#ifdef TIMED_MOVE_MM_DEBUG
  displayMessageWithNewline(F("timedMoveDistanceInMM"));
  Serial.print("    Left mms to move: ");
  Serial.print(leftMMs);
  Serial.print(" Right mms to move: ");
  Serial.print(rightMMs);
  Serial.print(" Time to move in seconds: ");
  displayMessageWithNewline(timeToMoveInSeconds);
#endif

  // Do this because rounding errors can lead to different numbers for
  // positive and negative moves of the same length

  long leftSteps = (long)((abs(leftMMs) * leftStepsPerMM) + 0.5);
  long rightSteps = (long)((abs(rightMMs) * rightStepsPerMM) + 0.5);
  if (leftMMs < 0)
  {
    leftSteps *= -1;
  }

  if (rightMMs < 0)
  {
    rightSteps *= -1;
  }

#ifdef TIMED_MOVE_MM_DEBUG
  Serial.print("    Left steps to move: ");
  Serial.print(leftSteps);
  Serial.print(" Right steps to move: ");
  displayMessageWithNewline(rightSteps);
#endif

  return timedMoveSteps(leftSteps, rightSteps, timeToMoveInSeconds);
}

int fastMoveDistanceInMM(float leftMMs, float rightMMs)
{

#ifdef FAST_MOVE_MM_DEBUG
  displayMessageWithNewline(F("fastMoveDistanceInMM"));
  Serial.print("    Left mms to move: ");
  Serial.print(leftMMs);
  Serial.print(" Right mms to move: ");
  displayMessageWithNewline(rightMMs);
  Serial.print("    Left mms per step: ");
  Serial.print(leftStepsPerMM);
  Serial.print(" Right mms per step: ");
  displayMessageWithNewline(rightStepsPerMM);

#endif

  long leftSteps = (long)((abs(leftMMs) * leftStepsPerMM) + 0.5);
  long rightSteps = (long)((abs(rightMMs) * rightStepsPerMM) + 0.5);

  if (leftMMs < 0)
  {
    leftSteps *= -1;
  }

  if (rightMMs < 0)
  {
    rightSteps *= -1;
  }

#ifdef FAST_MOVE_MM_DEBUG
  Serial.print("    Left steps to move: ");
  Serial.print(leftSteps);
  Serial.print(" Right steps to move: ");
  displayMessageWithNewline(rightSteps);
#endif

  int result = fastMoveSteps(leftSteps, rightSteps);

  return result;
}


// #define DEBUG_FAST_ROTATE

void fastRotateRobot(float angle)
{
  float noOfTurns = angle / 360.0;
  float distanceToRotate = noOfTurns * turningCircle;

  fastMoveDistanceInMM(distanceToRotate, -distanceToRotate);

#ifdef DEBUG_FAST_ROTATE
  Serial.print(". angle: ");
  Serial.print(angle);
  Serial.print(" noOfTurns: ");
  Serial.print(noOfTurns);
  Serial.print(" distanceToRotate: ");
  displayMessageWithNewline(distanceToRotate);
#endif
}

int timedRotateRobot(float angle, float timeToMoveInSeconds)
{
  float noOfTurns = angle / 360.0;
  float distanceToRotate = noOfTurns * turningCircle;

#ifdef DEBUG_TIMED_ROTATE
  Serial.print(". angle: ");
  Serial.print(angle);
  Serial.print(" noOfTurns: ");
  Serial.print(noOfTurns);
  Serial.print(" time: ");
  Serial.print(timeToMoveInSeconds);
  Serial.print(" distanceToRotate: ");
  displayMessageWithNewline(distanceToRotate);
#endif

  return timedMoveDistanceInMM(distanceToRotate, -distanceToRotate, timeToMoveInSeconds);
}

void fastMoveArcRobot(float radius, float angle)
{
  float noOfTurns = angle / 360.0;
  float absRadius = abs(radius);

  float leftDistanceToMove = noOfTurns * ((absRadius + (motorSettings.wheelSpacing / 2.0)) * 2.0 * PI);
  float rightDistanceToMove = noOfTurns * ((absRadius - (motorSettings.wheelSpacing / 2.0)) * 2.0 * PI);

#ifdef DEBUG_FAST_ARC
  displayMessageWithNewline(F("fastMoveArcRobot"));
  Serial.print(" radius: ");
  Serial.print(radius);
  Serial.print(" angle: ");
  Serial.print(angle);
  Serial.print(" leftDistanceToMove: ");
  Serial.print(leftDistanceToMove);
  Serial.print(" rightDistanceToMove: ");
  displayMessageWithNewline(rightDistanceToMove);
#endif

  if (radius >= 0)
  {
    fastMoveDistanceInMM(leftDistanceToMove, rightDistanceToMove);
  }
  else
  {
    fastMoveDistanceInMM(rightDistanceToMove, leftDistanceToMove);
  }
}

int timedMoveArcRobot(float radius, float angle, float timeToMoveInSeconds)
{
  float noOfTurns = angle / 360.0;
  float absRadius = abs(radius);

  float leftDistanceToMove = noOfTurns * ((absRadius + (motorSettings.wheelSpacing / 2.0)) * 2.0 * PI);
  float rightDistanceToMove = noOfTurns * ((absRadius - (motorSettings.wheelSpacing / 2.0)) * 2.0 * PI);

#ifdef DEBUG_TIMED_ARC
  displayMessageWithNewline(F("timedMoveArcRobot"));
  Serial.print(" radius: ");
  Serial.print(radius);
  Serial.print(" angle: ");
  Serial.print(angle);
  Serial.print(" time: ");
  Serial.print(timeToMoveInSeconds);
  Serial.print(" leftDistanceToMove: ");
  Serial.print(leftDistanceToMove);
  Serial.print(" rightDistanceToMove: ");
  displayMessageWithNewline(rightDistanceToMove);
#endif

  if (radius >= 0)
  {
    return timedMoveDistanceInMM(leftDistanceToMove, rightDistanceToMove, timeToMoveInSeconds);
  }
  else
  {
    return timedMoveDistanceInMM(rightDistanceToMove, leftDistanceToMove, timeToMoveInSeconds);
  }
}

void initMotors()
{
  if (motorSettings.motorsActive)
  {
    setupWheelSettings();
#ifdef STEPPER_MOTOR_DRIVE
    initMotorHardware();
#endif
  }
  else
  {
    motorProcessDescriptor.status = MOTORS_OFF;
  }
}

void startMotors()
{
  if (motorSettings.motorsActive)
  {
    motorProcessDescriptor.status = MOTORS_OK;
  }
}

void updateMotors()
{
  //  updateMotorsCore1();
}

void stopMotors()
{
  if (motorProcessDescriptor.status == MOTORS_OK)
  {
    motorStop();
  }

  //  cancel_repeating_timer(&leftTimer);
  //  cancel_repeating_timer(&rightTimer);
  motorProcessDescriptor.status = MOTORS_OFF;
}

bool motorStatusOK()
{
  return motorProcessDescriptor.status == MOTORS_OK;
}

void motorStatusMessage(char *buffer, int bufferLength)
{
  switch (motorProcessDescriptor.status)
  {
  case MOTORS_OFF:
    snprintf(buffer, bufferLength, "Motors off");
    break;
  case MOTORS_OK:
    snprintf(buffer, bufferLength, "Motors OK Moving: %d", motorsMoving());
    break;
  default:
    snprintf(buffer, bufferLength, "Motor status invalid");
    break;
  }
}

boolean validateMoveDistance(void *dest, const char *newValueStr)
{
  float value;

  if (!validateFloat(&value, newValueStr))
  {
    return false;
  }

  if (value < -10000 || value > 10000)
  {
    return false;
  }

  *(float *)dest = value;
  return true;
}

boolean validateMoveTime(void *dest, const char *newValueStr)
{
  float value;

  if (!validateFloat(&value, newValueStr))
  {
    return false;
  }

  if (value < 0 || value > 10000)
  {
    return false;
  }

  *(float *)dest = value;
  return true;
}

#define MOTOR_FLOAT_VALUE_OFFSET 0
#define MOTOR_LEFT_DISTANCE_OFFSET (MOTOR_FLOAT_VALUE_OFFSET + sizeof(float))
#define MOTOR_RIGHT_DISTANCE_OFFSET (MOTOR_LEFT_DISTANCE_OFFSET + sizeof(float))
#define MOTOR_MOVE_TIME_OFFSET (MOTOR_RIGHT_DISTANCE_OFFSET + sizeof(float))

struct CommandItem motorLeftCommandItem = {
    "left",
    "left motor move distance mm",
    MOTOR_LEFT_DISTANCE_OFFSET,
    floatCommand,
    validateMoveDistance,
    noDefaultAvailable};

struct CommandItem motorRightCommandItem = {
    "right",
    "right motor move distance mm",
    MOTOR_RIGHT_DISTANCE_OFFSET,
    floatCommand,
    validateMoveDistance,
    noDefaultAvailable};

struct CommandItem motorMoveTimeCommandItem = {
    "time",
    "time for the move to take (secs)",
    MOTOR_MOVE_TIME_OFFSET,
    floatCommand,
    validateMoveTime,
    setDefaultFloatZero};

struct CommandItem *motorMoveItems[] =
    {
        &motorLeftCommandItem,
        &motorRightCommandItem,
        &motorMoveTimeCommandItem};

int doMotorLeftRightIntime(char *destination, unsigned char *settingBase);

struct Command motorLeftRightIntimeCommand{
    "move",
    "Moves the left and right sides a distance in a given time",
    motorMoveItems,
    sizeof(motorMoveItems) / sizeof(struct CommandItem *),
    doMotorLeftRightIntime};

int doMotorLeftRightIntime(char *destination, unsigned char *settingBase)
{
  if (*destination != 0)
  {
    // we have a destination for the command. Build the string
    char buffer[JSON_BUFFER_SIZE];
    createJSONfromSettings("motors", &motorLeftRightIntimeCommand, destination, settingBase, buffer, JSON_BUFFER_SIZE);
    return publishCommandToRemoteDevice(buffer, destination);
  }

  if (motorProcessDescriptor.status != MOTORS_OK)
  {
    return JSON_MESSAGE_MOTORS_NOT_ENABLED;
  }

  float left = (float)getUnalignedFloat(settingBase + MOTOR_LEFT_DISTANCE_OFFSET);
  float right = (float)getUnalignedFloat(settingBase + MOTOR_RIGHT_DISTANCE_OFFSET);
  float time = (float)getUnalignedFloat(settingBase + MOTOR_MOVE_TIME_OFFSET);

  if (time > 0)
  {
    if (timedMoveDistanceInMM(left, right, time) == Move_OK)
    {
      return WORKED_OK;
    }
    else
    {
      return JSON_MESSAGE_INVALID_MOVE_VALUES;
    }
  }
  else
  {
    if (fastMoveDistanceInMM(left, right) == Move_OK)
    {
      return WORKED_OK;
    }
    else
    {
      return JSON_MESSAGE_INVALID_MOVE_VALUES;
    }
  }
}

struct Command *motorCommandList[] = {
    &motorLeftRightIntimeCommand};

struct CommandItemCollection motorCommands =
    {
        "Control the pixels on the device",
        motorCommandList,
        sizeof(motorCommandList) / sizeof(struct Command *)};

struct process motorProcessDescriptor = {
    "motors",
    initMotors,
    startMotors,
    updateMotors,
    stopMotors,
    motorStatusOK,
    motorStatusMessage,
    false,
    0,
    0,
    0,
    NULL,
    (unsigned char *)&motorSettings, sizeof(motorSettings), &motorSettingItems,
    &motorCommands,
    BOOT_PROCESS + ACTIVE_PROCESS + CONFIG_PROCESS + WIFI_CONFIG_PROCESS,
    NULL,
    NULL,
    NULL};

#endif