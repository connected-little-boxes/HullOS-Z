#pragma once
#include <Arduino.h>

#define MESSAGES_OK 400
#define MESSAGES_STOPPED 401

struct MessagesSettings {
	bool messagesEnabled;
	bool speedMessagesEnabled;
};

#pragma once
#include <Arduino.h>

// printf-style variants (use these for literals and F("..."))
void displayMessage(const __FlashStringHelper* fmt, ...);
void displayMessage(const char* fmt, ...);

// convenience overload for Arduino String (no formatting)
void displayMessage(const String& s);

void displayMessageWithNewline(const __FlashStringHelper* fmt, ...);
void displayMessageWithNewline(const char* fmt, ...);
void displayMessageWithNewline(const String& s);

enum ledFlashBehaviour { ledFlashOn, ledFlashNormalState, ledFlashStartingState,
	ledFlashConfigState, ledFlashAlertState};

void hardwareDisplayMessage(int messageNumber, ledFlashBehaviour flashBehaviour, char * messageText);

void ledFlashBehaviourToString(ledFlashBehaviour severity, char * dest, int length);

bool bindMessageHandler(void(*newHandler)(int messageNumber, ledFlashBehaviour severity, char* messageText));

void messagesOff();

void messagesOn();

void messagesStatusMessage(struct process * inputSwitchProcess, char * buffer, int bufferLength);

extern struct MessagesSettings messagesSettings;

extern struct SettingItemCollection messagesSettingItems;

extern struct process messagesProcess;


