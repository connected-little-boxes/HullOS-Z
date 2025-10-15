#if STEPPER_MOTOR_DRIVE

#include <Arduino.h>

float fastMoveSteps(long leftStepsToMove, long rightStepsToMove);
int timedMoveDistanceInMM(float leftMMs, float rightMMs, float timeToMoveInSeconds);

void initMotorHardware();

#endif