#pragma once

#include <Arduino.h>

#if defined(PICO_USE_UART)
#include <stdio.h>
#include <pico/stdlib.h>
#endif

#include "utils.h"
#include "settings.h"
#include "messages.h"
#include "processes.h"
#include <stdarg.h>
#include <stdio.h>
#include <string.h>


struct MessagesSettings messagesSettings;

struct SettingItem messagesEnabled = {
    "Messages enabled",
    "messagesactive",
    &messagesSettings.messagesEnabled,
    ONOFF_INPUT_LENGTH,
    yesNo,
    setTrue,
    validateYesNo};

struct SettingItem speedMessagesEnabled = {
    "Speed messages enabled",
    "speedmessagesactive",
    &messagesSettings.speedMessagesEnabled,
    ONOFF_INPUT_LENGTH,
    yesNo,
    setFalse,
    validateYesNo};


    struct SettingItem *messagesSettingItemPointers[] =
    {
        &messagesEnabled,
        &speedMessagesEnabled
    };

struct SettingItemCollection messagesSettingItems = {
    "messages",
    "Enable/disable for message output",
    messagesSettingItemPointers,
    sizeof(messagesSettingItemPointers) / sizeof(struct SettingItem *)};

void messagesOff()
{
    messagesSettings.messagesEnabled = false;
    saveSettings();
}

void messagesOn()
{
    messagesSettings.messagesEnabled = true;
    saveSettings();
}

#ifndef MSG_FMT_BUF
#define MSG_FMT_BUF 256
#endif

#ifndef MSG_FLASH_FMT_BUF
#define MSG_FLASH_FMT_BUF 160
#endif

static void vprintFromRamFmt(const char* fmt, va_list ap, bool newline) {
  char out[MSG_FMT_BUF];
  vsnprintf(out, sizeof(out), fmt ? fmt : "", ap);
  if (newline) Serial.println(out);
  else         Serial.print(out);
}

static void vprintFromFlashFmt(const __FlashStringHelper* ffmt, va_list ap, bool newline) {
  char fmt[MSG_FLASH_FMT_BUF];
  // Copy flash-resident format into RAM; strncpy_P works across cores that support F()
  strncpy_P(fmt, reinterpret_cast<PGM_P>(ffmt), sizeof(fmt) - 1);
  fmt[sizeof(fmt) - 1] = '\0';

  char out[MSG_FMT_BUF];
  vsnprintf(out, sizeof(out), fmt, ap);
  if (newline) Serial.println(out);
  else         Serial.print(out);
}

// -------- printf-style (flash) --------
void displayMessage(const __FlashStringHelper* fmt, ...) {
  va_list ap; va_start(ap, fmt);
  vprintFromFlashFmt(fmt, ap, /*newline*/false);
  va_end(ap);
}

void displayMessageWithNewline(const __FlashStringHelper* fmt, ...) {
  va_list ap; va_start(ap, fmt);
  vprintFromFlashFmt(fmt, ap, /*newline*/true);
  va_end(ap);
}

// -------- printf-style (RAM) --------
void displayMessage(const char* fmt, ...) {
  va_list ap; va_start(ap, fmt);
  vprintFromRamFmt(fmt, ap, /*newline*/false);
  va_end(ap);
}

void displayMessageWithNewline(const char* fmt, ...) {
  va_list ap; va_start(ap, fmt);
  vprintFromRamFmt(fmt, ap, /*newline*/true);
  va_end(ap);
}

// -------- String convenience (no formatting) --------
void displayMessage(const String& s) {
  Serial.print(s);
}

void displayMessageWithNewline(const String& s) {
  Serial.println(s);
}


// enough room for four message handlers

void (*messageHandlerList[])(int messageNumber, ledFlashBehaviour severity, char *messageText) = {NULL, NULL, NULL, NULL};

int noOfMessageHandlers = sizeof(messageHandlerList) / sizeof(int (*)(int, char *));

bool bindMessageHandler(void (*newHandler)(int messageNumber, ledFlashBehaviour severity, char *messageText))
{
    for (int i = 0; i < noOfMessageHandlers; i++)
        if (messageHandlerList[i] == NULL)
        {
            messageHandlerList[i] = newHandler;
            return true;
        }
    return false;
}

void ledFlashBehaviourToString(ledFlashBehaviour severity, char *dest, int length)
{
    switch (severity)
    {
    case ledFlashOn:
        snprintf(dest, length, "Starting");
        break;

    case ledFlashNormalState:
        snprintf(dest, length, "OK");
        break;

    case ledFlashConfigState:
        snprintf(dest, length, "Config");
        break;

    case ledFlashAlertState:
        snprintf(dest, length, "Alert");
        break;

    default:
        snprintf(dest, length, "Unknown");
        break;
    }
}

void hardwareDisplayMessage(int messageNumber, ledFlashBehaviour severity, char *messageText)
{
    for (int i = 0; i < noOfMessageHandlers; i++)
    {
        if (messageHandlerList[i] != NULL)
        {
            messageHandlerList[i](messageNumber, severity, messageText);
        }
    }
}

void initMessages()
{
    messagesProcess.status = MESSAGES_STOPPED;
}

void startMessages()
{
    if (messagesSettings.messagesEnabled)
    {
        messagesProcess.status = MESSAGES_OK;
    }
    else{
        Serial.printf("Message displays disabled. Use the command messagesactive=yes to turn messages on,\n");
    }
}

void updateMessages()
{
}

void stopmessages()
{
    messagesProcess.status = MESSAGES_STOPPED;
}

bool messagesStatusOK()
{
    return messagesProcess.status == MESSAGES_OK;
}

void messagesStatusMessage(char *buffer, int bufferLength)
{
    if (messagesProcess.status == MESSAGES_STOPPED)
    {
        snprintf(buffer, bufferLength, "Messages stopped");
    }
    else
    {
        snprintf(buffer, bufferLength, "Messages enabled");
    }
}

struct process messagesProcess = {
    "messages",
    initMessages,
    startMessages,
    updateMessages,
    stopmessages,
    messagesStatusOK,
    messagesStatusMessage,
    false,
    0,
    0,
    0,
    NULL,
    (unsigned char *)&messagesSettings, sizeof(MessagesSettings), &messagesSettingItems,
    NULL,
    BOOT_PROCESS + ACTIVE_PROCESS + CONFIG_PROCESS + WIFI_CONFIG_PROCESS,
    NULL,
    NULL,
    NULL,
    NULL, // no command options
    0     // no command options
};
