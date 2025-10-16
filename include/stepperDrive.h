#if STEPPER_MOTOR_DRIVE

#include <Arduino.h>

int timedMoveDistanceInMM(float leftMMs, float rightMMs, float timeToMoveInSeconds);

void initMotorHardware();

#endif