#include <Arduino.h>
#include "utils.h"
#include "settings.h"
#include "processes.h"
#include "HullOSCommands.h"
#include "HullOSVariables.h"
#include "HullOSScript.h"
#include "HullOS.h"
#include "PythonIsh.h"
#include "RockStar.h"
#include "errors.h"
#include "mqtt.h"
#include "Motors.h"
#include "console.h"

struct HullOSSettings hullosSettings;

struct SettingItem hullosEnabled = {
    "HullOS enabled",
    "hullosactive",
    &hullosSettings.hullosEnabled,
    ONOFF_INPUT_LENGTH,
    yesNo,
    setFalse,
    validateYesNo};

boolean validateHullOSLanguage(void *dest, const char *newValueStr)
{
    if (strncasecmp(newValueStr, "Rockstar", HULLOS_LANGUAGE_NAME_SIZE) ||
        strncasecmp(newValueStr, "PythonIsh", HULLOS_LANGUAGE_NAME_SIZE))
    {
        strcpy((char *)dest, newValueStr);
        return true;
    }
    return false;
}

void setDefaultHullOSLanguage(void *dest)
{
    strcpy((char *)dest, "PythonIsh");
}

struct SettingItem hullosProgramSetting = {
    "Hullos language (Rockstar or PythonIsh)",
    "hulloslanguage",
    &hullosSettings.hullosLanguage,
    HULLOS_LANGUAGE_NAME_SIZE,
    text,
    setDefaultHullOSLanguage,
    validateHullOSLanguage};

struct SettingItem *hullosSettingItemPointers[] = {
    &hullosEnabled,
    &hullosProgramSetting};

struct SettingItemCollection hullosSettingItems = {
    "hullos",
    "HullOs active",
    hullosSettingItemPointers,
    sizeof(hullosSettingItemPointers) / sizeof(struct SettingItem *)};

void hullosOff()
{
    hullosProcess.status = HULLOS_STOPPED;
}

void hullosOn()
{
    hullosProcess.status = HULLOS_OK;
    clearVariables();
}

void initHullOS()
{
    hullosProcess.status = HULLOS_STOPPED;
}

void startHullOS()
{
    if (hullosSettings.hullosEnabled)
    {
        hullosProcess.status = HULLOS_OK;
        clearVariables();
    }
}

void sendMessageToHullOS(char *programText)
{

}

void HullOSStartPythonIsh(char *commandLine)
{
    Serial.printf("Starting PythonIsh: %s\n", commandLine);

    setProgramOutputFunction(interpretSerialByte);
    //   setProgramOutputFunction(storeReceivedByte);

    setProgramDecodeFunction(pythonIshdecodeScriptChar);

    consoleProcessDescriptor.status = CONSOLE_OFF;
    programState = EXECUTE_IMMEDIATELY;
    resetCommand();
    resetSerialBuffer();
}

void HullOSStartRockstar(char *commandLine)
{
    Serial.printf("Starting Rockstar: %s\n", commandLine);
}

void sendChars(){
    while (CharsAvailable())
    {
        byte b = GetRawCh();
        HullOSProgramInputFunction(b);
    }
}

void updateHullOS()
{
    // If we recieve serial data the program that is running
    // must stop.

    if (hullosProcess.status == HULLOS_STOPPED)
    {
        return;
    }

    // If we recieve serial data the program that is running
    // must stop.
    switch (programState)
    {
	case EXECUTE_IMMEDIATELY:
        sendChars();
        break;
	case STORE_PROGRAM:
        sendChars();
        break;
    case PROGRAM_STOPPED:
    case PROGRAM_PAUSED:
        break;
    case PROGRAM_ACTIVE:
        exeuteProgramStatement();
        break;
    case PROGRAM_AWAITING_MOVE_COMPLETION:
        if (!motorsMoving())
        {
            programState = PROGRAM_ACTIVE;
        }
        break;
    case PROGRAM_AWAITING_DELAY_COMPLETION:
        if (millis() > delayEndTime)
        {
            programState = PROGRAM_ACTIVE;
        }
        break;
    }
}

void stophullos()
{
    hullosProcess.status = HULLOS_STOPPED;
}

bool hullosStatusOK()
{
    return hullosProcess.status == HULLOS_OK;
}

void hullosStatusMessage(char *buffer, int bufferLength)
{
    if (hullosProcess.status == HULLOS_STOPPED)
    {
        snprintf(buffer, bufferLength, "HullOS stopped");
    }
    else
    {
        snprintf(buffer, bufferLength, "HullOS enabled");
    }
}

#define HULLOS_FLOAT_VALUE_OFFSET 0
#define HULLOS_PROGRAM_CONTROL_OFFSET (HULLOS_FLOAT_VALUE_OFFSET + HULLOS_LANGUAGE_NAME_SIZE + 1)

boolean validateHullOSStateString(void *dest, const char *newValueStr)
{
    return (validateString((char *)dest, newValueStr, HULLOS_PROGRAM_COMMAND_LENGTH));
}

struct CommandItem hullOSRunningcommand = {
    "running",
    "control state of executing HullOS program (run or stop)",
    HULLOS_PROGRAM_CONTROL_OFFSET,
    textCommand,
    validateHullOSStateString,
    noDefaultAvailable};

struct CommandItem *setstateItems[] =
    {
        &hullOSRunningcommand};

int doSetState(char *destination, unsigned char *settingBase);

struct Command setHullosStateCommand{
    "state",
    "Sets the state of the program",
    setstateItems,
    sizeof(setstateItems) / sizeof(struct CommandItem *),
    doSetState};

int doSetState(char *destination, unsigned char *settingBase)
{
    if (*destination != 0)
    {
        // we have a destination for the command. Build the string
        char buffer[JSON_BUFFER_SIZE];
        createJSONfromSettings("hullos", &setHullosStateCommand, destination, settingBase, buffer, JSON_BUFFER_SIZE);
        return publishCommandToRemoteDevice(buffer, destination);
    }

    char *command = (char *)(settingBase + HULLOS_PROGRAM_CONTROL_OFFSET);

    bool commandPerformed = false;

    if (strcasecmp(command, "yes") == 0)
    {
        startProgramExecution();
        commandPerformed = true;
    }

    if (strcasecmp(command, "no") == 0)
    {
        Serial.println("Doing stop");
        commandPerformed = true;
    }

    if (!commandPerformed)
    {
        return JSON_MESSAGE_INVALID_RUN_STATE_IN_HULLOS_COMMAND;
    }

    return WORKED_OK;
}

struct Command *hullOSCommandList[] = {
    &setHullosStateCommand};

struct CommandItemCollection hullOSCommands =
    {
        "Control the HullOS programs on the device",
        hullOSCommandList,
        sizeof(hullOSCommandList) / sizeof(struct Command *)};

#ifdef PROCESS_HULLOS

struct process hullosProcess = {
    "hullos",
    initHullOS,
    startHullOS,
    updateHullOS,
    stophullos,
    hullosStatusOK,
    hullosStatusMessage,
    false,
    0,
    0,
    0,
    NULL,
    (unsigned char *)&hullosSettings, sizeof(HullOSSettings), &hullosSettingItems,
    &hullOSCommands,
    BOOT_PROCESS + ACTIVE_PROCESS + CONFIG_PROCESS + WIFI_CONFIG_PROCESS,
    NULL,
    NULL,
    NULL,
    NULL, // no command options
    0     // no command options
};
#endif