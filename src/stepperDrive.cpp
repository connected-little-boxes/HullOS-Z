#if STEPPER_MOTOR_DRIVE

#include <Arduino.h>
#include "Motors.h"
#include "stepperDrive.h"

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

  if (leftStepsToMove != 0)
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

  if (rightStepsToMove != 0)
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

  if (leftStepsToMove != 0)
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

  if (rightStepsToMove != 0)
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

#ifdef PICO_CORE_MOTOR

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

unsigned long lastMicros;

volatile unsigned long leftIntervalBetweenSteps;
volatile unsigned long rightIntervalBetweenSteps;

volatile unsigned long leftTimeOfLastStep;
volatile unsigned long rightTimeOfLastStep;

volatile unsigned long leftTimeOfNextStep;
volatile unsigned long rightTimeOfNextStep;

volatile unsigned long currentMicros;
volatile unsigned long leftTimeSinceLastStep;
volatile unsigned long rightTimeSinceLastStep;

inline void startMotor(unsigned long stepLimit, unsigned long microSecsPerPulse, bool forward,
                       volatile unsigned long *motorStepLimit, volatile unsigned long *motorPulseInterval,
                       volatile int *motorDelta, volatile int *motorPos)
{
  // If we are not moving - set the delta to zero and return

  if (stepLimit == 0)
  {
    motorDelta = 0;
    return;
  }

  *motorStepLimit = stepLimit;

  *motorPulseInterval = microSecsPerPulse;

  if (forward)
  {
    *motorDelta = 1;
  }
  else
  {
    *motorDelta = -1;
  }

  *motorPos = 0;
}

void startMotorsCore1(
    unsigned long leftSteps, unsigned long rightSteps,
    unsigned long leftMicroSecsPerPulse, unsigned long rightMicroSecsPerPulse,
    bool leftForward, bool rightForward)
{
  // Set up the counters for the left and right motor moves

  lastMicros = micros();

  leftStepCounter = 0;
  rightStepCounter = 0;

  startMotor(leftSteps, leftMicroSecsPerPulse, leftForward,
             &leftNumberOfStepsToMove, &leftIntervalBetweenSteps, &leftMotorWaveformDelta, &leftMotorWaveformPos);

  startMotor(rightSteps, rightMicroSecsPerPulse, rightForward,
             &rightNumberOfStepsToMove, &rightIntervalBetweenSteps, &rightMotorWaveformDelta, &rightMotorWaveformPos);

  return;
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

  if (leftStepsToMove != 0)
  {
    leftInterruptIntervalInMicroSeconds = (long)((timeToMoveInSeconds / (double)abs(leftStepsToMove)) * 1000000L + 0.5);
  }
  else
  {
    leftInterruptIntervalInMicroSeconds = MIN_MOTOR_INTERRUPT_INTERVAL_MICROSECS;
  }

  long rightInterruptIntervalInMicroseconds;

  if (rightStepsToMove != 0)
  {
    rightInterruptIntervalInMicroseconds = (long)((timeToMoveInSeconds / (double)abs(rightStepsToMove)) * 1000000L + 0.5);
  }
  else
  {
    rightInterruptIntervalInMicroseconds = MIN_MOTOR_INTERRUPT_INTERVAL_MICROSECS;
  }

#ifdef DEBUG_TIMED_MOVE
  Serial.print("    Left interval in microseconds: ");
  Serial.print(leftInterruptIntervalInMicroSeconds);
  Serial.print(" Right interval in microseconds: ");
  displayMessageWithNewline(rightInterruptIntervalInMicroseconds);
#endif

  // There's a minium gap allowed between intervals. This is set by the top speed of the motors
  // Presently set at 1000 microseconds

  if (leftInterruptIntervalInMicroSeconds < MIN_MOTOR_INTERRUPT_INTERVAL_MICROSECS & rightInterruptIntervalInMicroseconds < MIN_MOTOR_INTERRUPT_INTERVAL_MICROSECS)
  {
    return Left_And_Right_Distance_Too_Large;
  }

  if (leftInterruptIntervalInMicroSeconds<MIN_MOTOR_INTERRUPT_INTERVAL_MICROSECS & rightInterruptIntervalInMicroseconds> MIN_MOTOR_INTERRUPT_INTERVAL_MICROSECS)
  {
    return Left_Distance_Too_Large;
  }

  if (rightInterruptIntervalInMicroseconds<MIN_MOTOR_INTERRUPT_INTERVAL_MICROSECS & leftInterruptIntervalInMicroSeconds> MIN_MOTOR_INTERRUPT_INTERVAL_MICROSECS)
  {
    return Right_Distance_Too_Large;
  }

  // If we get here we can move at this speed.

  startMotorsCore1(abs(leftStepsToMove), abs(rightStepsToMove),
                   leftInterruptIntervalInMicroSeconds, rightInterruptIntervalInMicroseconds,
                   leftStepsToMove > 0, rightStepsToMove > 0);

  return Move_OK;
}

void updateMotorsCore1()
{
  unsigned long currentMicros = micros();

  if (leftMotorWaveformDelta != 0)
  {
    // left is moving - see if it is time to do a step
    leftTimeSinceLastStep = ulongDiff(currentMicros, leftTimeOfLastStep);
    if (leftTimeSinceLastStep >= leftIntervalBetweenSteps)
    {
      if (++leftStepCounter > leftNumberOfStepsToMove)
      {
        leftMotorWaveformDelta = 0;
        setLeft(0);
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
      leftTimeOfLastStep = currentMicros - (leftTimeSinceLastStep - leftIntervalBetweenSteps);
      leftTimeOfNextStep = currentMicros + leftIntervalBetweenSteps;
    }
  }
  if (rightMotorWaveformDelta != 0)
  {
    rightTimeSinceLastStep = ulongDiff(currentMicros, rightTimeOfLastStep);

    if (rightTimeSinceLastStep >= rightIntervalBetweenSteps)
    {
      if (++rightStepCounter > rightNumberOfStepsToMove)
      {
        rightMotorWaveformDelta = 0;
        setRight(0);
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
      rightTimeOfLastStep = currentMicros - (rightTimeSinceLastStep - rightIntervalBetweenSteps);
      rightTimeOfNextStep = currentMicros + rightIntervalBetweenSteps;
    }
  }
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

void motorStop()
{
  setRight(0);
  rightMotorWaveformDelta = 0;
  rightStepCounter = 0;
  setLeft(0);
  leftMotorWaveformDelta = 0;
  leftStepCounter = 0;
}


#endif