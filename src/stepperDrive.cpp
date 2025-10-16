#if STEPPER_MOTOR_DRIVE

#include <Arduino.h>
#include "Motors.h"
#include "stepperDrive.h"
#include "utils.h"

const char motorWaveformLookup[8] = {0b01000, 0b01100, 0b00100, 0b00110, 0b00010, 0b00011, 0b00001, 0b01001};

volatile int leftMotorWaveformPos = 0;
volatile int leftMotorWaveformDelta = 0;

volatile int rightMotorWaveformPos = 0;
volatile int rightMotorWaveformDelta = 0;

volatile unsigned long leftStepCounter = 0;
volatile unsigned long leftNumberOfStepsToMove = 0;

volatile unsigned long rightStepCounter = 0;
volatile unsigned long rightNumberOfStepsToMove = 0;

inline void setLeft(byte bits)
{
  digitalWrite(motorSettings.lpin1, bits & 1);
  digitalWrite(motorSettings.lpin2, bits & 2);
  digitalWrite(motorSettings.lpin3, bits & 4);
  digitalWrite(motorSettings.lpin4, bits & 8);
}

inline void setRight(byte bits)
{
  digitalWrite(motorSettings.rpin1, bits & 1);
  digitalWrite(motorSettings.rpin2, bits & 2);
  digitalWrite(motorSettings.rpin3, bits & 4);
  digitalWrite(motorSettings.rpin4, bits & 8);
}

volatile bool motorsRunning = false;

#ifdef ARDUINO_ARCH_ESP32

hw_timer_t *leftTimer = NULL;
hw_timer_t *rightTimer = NULL;

void rightStop()
{
  rightMotorWaveformDelta = 0;
  setRight(0);
  timerAlarmDisable(rightTimer);
}

void leftStop()
{
  leftMotorWaveformDelta = 0;
  setLeft(0);
  timerAlarmDisable(leftTimer);
}

void motorStop()
{
  rightStop();
  leftStop();
}

void IRAM_ATTR onLeft()
{
  // If we are not moving, don't do anything
  if (leftMotorWaveformDelta != 0)
  {
    if (++leftStepCounter > leftNumberOfStepsToMove)
    {
      leftMotorWaveformDelta = 0;
      setLeft(0);
      timerAlarmDisable(leftTimer);
    }
    else
    {
      // Move the motor one step
      setLeft(motorWaveformLookup[leftMotorWaveformPos]);

      // Update and wrap the waveform position
      leftMotorWaveformPos += leftMotorWaveformDelta;
      if (leftMotorWaveformPos == 8)
        leftMotorWaveformPos = 0;
      if (leftMotorWaveformPos < 0)
        leftMotorWaveformPos = 7;
    }
  }
}

void ARDUINO_ISR_ATTR onRight()
{
  // If we are not moving, don't do anything
  if (rightMotorWaveformDelta != 0)
  {
    if (++rightStepCounter > rightNumberOfStepsToMove)
    {
      rightMotorWaveformDelta = 0;
      setRight(0);
      timerAlarmDisable(rightTimer);
    }
    else
    {
      // Move the motor one step

      setRight(motorWaveformLookup[rightMotorWaveformPos]);

      // Update and wrap the waveform position
      rightMotorWaveformPos += rightMotorWaveformDelta;
      if (rightMotorWaveformPos == 8)
        rightMotorWaveformPos = 0;
      if (rightMotorWaveformPos < 0)
        rightMotorWaveformPos = 7;
    }
  }
}

void initMotorHardware()
{
  pinMode(motorSettings.lpin1, OUTPUT);
  pinMode(motorSettings.lpin2, OUTPUT);
  pinMode(motorSettings.lpin3, OUTPUT);
  pinMode(motorSettings.lpin4, OUTPUT);

  pinMode(motorSettings.rpin1, OUTPUT);
  pinMode(motorSettings.rpin2, OUTPUT);
  pinMode(motorSettings.rpin3, OUTPUT);
  pinMode(motorSettings.rpin4, OUTPUT);

  leftTimer = timerBegin(0, 80, true);
  timerAttachInterrupt(leftTimer, &onLeft, true);

  rightTimer = timerBegin(1, 80, true);
  timerAttachInterrupt(rightTimer, &onRight, true);
}

MoveFailReason timedMoveSteps(long leftStepsToMove, long rightStepsToMove, float timeToMoveInSeconds)
{
#ifdef DEBUG_TIMED_MOVE
  displayMessageWithNewline(F("timedMoveSteps"));
  Serial.print("    Left steps to move: ");
  Serial.print(leftStepsToMove);
  Serial.print(" Right steps to move: ");
  Serial.print(rightStepsToMove);
  Serial.print(" Time to move in seconds: ");
  displayMessageWithNewline(timeToMoveInSeconds);
#endif

  long leftInterruptIntervalInMicroSeconds;
  long absLeftStepsToMove = abs(leftStepsToMove);

  if (leftStepsToMove == 0)
  {
    leftStop();
  }
  else
  {
#ifdef DEBUG_TIMED_MOVE
    displayMessageWithNewline(F("    Moving left steps.."));
#endif
    leftInterruptIntervalInMicroSeconds = (long)((timeToMoveInSeconds / (double)absLeftStepsToMove) * 1000000L + 0.5);
    timerAlarmWrite(leftTimer, leftInterruptIntervalInMicroSeconds, true);
    leftNumberOfStepsToMove = absLeftStepsToMove;
    leftStepCounter = 0;
    if (leftStepsToMove > 0)
    {
      leftMotorWaveformDelta = 1;
    }
    else
    {
      leftMotorWaveformDelta = -1;
    }
    timerAlarmEnable(leftTimer);
  }

  long rightInterruptIntervalInMicroseconds;
  long absRightStepsToMove = abs(rightStepsToMove);

  if (rightStepsToMove == 0)
  {
    rightStop();
  }
  else
  {
#ifdef DEBUG_TIMED_MOVE
    displayMessageWithNewline(F("    Moving right steps.."));
#endif
    rightInterruptIntervalInMicroseconds = (long)((timeToMoveInSeconds / (double)absRightStepsToMove) * 1000000L + 0.5);
    timerAlarmWrite(rightTimer, rightInterruptIntervalInMicroseconds, true);
    rightNumberOfStepsToMove = absRightStepsToMove;
    rightStepCounter = 0;
    if (rightStepsToMove > 0)
    {
      rightMotorWaveformDelta = -1;
    }
    else
    {
      rightMotorWaveformDelta = 1;
    }
    timerAlarmEnable(rightTimer);
  }

#ifdef DEBUG_TIMED_MOVE
  Serial.print("    Left interval in microseconds: ");
  Serial.print(leftInterruptIntervalInMicroSeconds);
  Serial.print(" Right interval in microseconds: ");
  displayMessageWithNewline(rightInterruptIntervalInMicroseconds);
#endif

  return Move_OK;
}

#endif

#ifdef PICO_TIMER_MOTOR

void initMotorHardware()
{
  pinMode(motorSettings.lpin1, OUTPUT);
  pinMode(motorSettings.lpin2, OUTPUT);
  pinMode(motorSettings.lpin3, OUTPUT);
  pinMode(motorSettings.lpin4, OUTPUT);

  pinMode(motorSettings.rpin1, OUTPUT);
  pinMode(motorSettings.rpin2, OUTPUT);
  pinMode(motorSettings.rpin3, OUTPUT);
  pinMode(motorSettings.rpin4, OUTPUT);
}

struct repeating_timer leftTimer;
struct repeating_timer rightTimer;

bool onLeft(struct repeating_timer *t)
{
  // If we are not moving, don't do anything
  if (leftMotorWaveformDelta != 0)
  {
    if (++leftStepCounter > leftNumberOfStepsToMove)
    {
      leftMotorWaveformDelta = 0;
      setLeft(0);
      cancel_repeating_timer(&leftTimer);
    }
    else
    {
      // Move the motor one step
      setLeft(motorWaveformLookup[leftMotorWaveformPos]);

      // Update and wrap the waveform position
      leftMotorWaveformPos += leftMotorWaveformDelta;
      if (leftMotorWaveformPos == 8)
        leftMotorWaveformPos = 0;
      if (leftMotorWaveformPos < 0)
        leftMotorWaveformPos = 7;
    }
  }
  return true;
}

bool onRight(struct repeating_timer *t)
{
  // If we are not moving, don't do anything
  if (rightMotorWaveformDelta != 0)
  {
    if (++rightStepCounter > rightNumberOfStepsToMove)
    {
      rightMotorWaveformDelta = 0;
      setRight(0);
      cancel_repeating_timer(&rightTimer);
    }
    else
    {
      // Move the motor one step
      setRight(motorWaveformLookup[rightMotorWaveformPos]);

      // Update and wrap the waveform position
      rightMotorWaveformPos += rightMotorWaveformDelta;
      if (rightMotorWaveformPos == 8)
        rightMotorWaveformPos = 0;
      if (rightMotorWaveformPos < 0)
        rightMotorWaveformPos = 7;
    }
  }
  return true;
}

void rightStop()
{
  setRight(0);
  rightMotorWaveformDelta = 0;
  rightStepCounter = 0;
}

void leftStop()
{
  setLeft(0);
  leftMotorWaveformDelta = 0;
  leftStepCounter = 0;
}

void motorStop()
{
  rightStop();
  leftStop();
}

MoveFailReason timedMoveSteps(long leftStepsToMove, long rightStepsToMove, float timeToMoveInSeconds)
{
#ifdef DEBUG_TIMED_MOVE
  displayMessageWithNewline(F("timedMoveSteps"));
  Serial.print("    Left steps to move: ");
  Serial.print(leftStepsToMove);
  Serial.print(" Right steps to move: ");
  Serial.print(rightStepsToMove);
  Serial.print(" Time to move in seconds: ");
  displayMessageWithNewline(timeToMoveInSeconds);
#endif

  long leftInterruptIntervalInMicroSeconds;
  long absLeftStepsToMove = abs(leftStepsToMove);

  if (leftStepsToMove == 0)
  {
    leftStop();
  }
  else
  {
    cancel_repeating_timer(&leftTimer);

#ifdef DEBUG_TIMED_MOVE
    displayMessageWithNewline(F("    Moving left steps.."));
#endif
    leftInterruptIntervalInMicroSeconds = (long)((timeToMoveInSeconds / (double)absLeftStepsToMove) * 1000000L + 0.5);
    leftNumberOfStepsToMove = absLeftStepsToMove;
    leftStepCounter = 0;
    if (leftStepsToMove > 0)
    {
      leftMotorWaveformDelta = 1;
    }
    else
    {
      leftMotorWaveformDelta = -1;
    }
    add_repeating_timer_us(leftInterruptIntervalInMicroSeconds, onLeft, NULL, &leftTimer);
  }

  long rightInterruptIntervalInMicroseconds;
  long absRightStepsToMove = abs(rightStepsToMove);

  if (rightStepsToMove == 0)
  {
    rightStop();
  }
  else
  {
#ifdef DEBUG_TIMED_MOVE
    displayMessageWithNewline(F("    Moving right steps.."));
#endif
    cancel_repeating_timer(&rightTimer);

    rightInterruptIntervalInMicroseconds = (long)((timeToMoveInSeconds / (double)absRightStepsToMove) * 1000000L + 0.5);
    rightNumberOfStepsToMove = absRightStepsToMove;
    rightStepCounter = 0;
    if (rightStepsToMove > 0)
    {
      rightMotorWaveformDelta = 1;
    }
    else
    {
      rightMotorWaveformDelta = -1;
    }
    add_repeating_timer_us(rightInterruptIntervalInMicroseconds, onRight, NULL, &rightTimer);
  }

#ifdef DEBUG_TIMED_MOVE
  Serial.print("    Left interval in microseconds: ");
  Serial.print(leftInterruptIntervalInMicroSeconds);
  Serial.print(" Right interval in microseconds: ");
  displayMessageWithNewline(rightInterruptIntervalInMicroseconds);
#endif

  return Move_OK;
}

#endif

int motorsMoving()
{
  if (rightMotorWaveformDelta != 0)
    return 1;
  if (leftMotorWaveformDelta != 0)
    return 1;
  return 0;
}

#endif