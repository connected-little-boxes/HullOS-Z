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

int hullOSdecodeScriptLine(char *input)
{
    Serial.printf("Hullos decoding: %s\n", input);

    if(strcasecmp(input,"exit")==0){
        stopLanguageDecoding();
        return ERROR_OK;
    }

    // put a terminator on the end

    int len=strlen(input);
    input[len]=STATEMENT_TERMINATOR;
    len++;

    hullOSActOnStatement(input,input+len);

    return ERROR_OK;
}

void hullOSDecoderStart(){
    Serial.printf("Starting HullOS decoder");
}


struct LanguageHandler hullOSLanguage = {
    "HullOS",
    hullOSDecoderStart,
    hullOSdecodeScriptLine};

struct LanguageHandler *allLanguages[] =
    {
        &hullOSLanguage,
        &PythonIshLanguage,
        &RockstarLanguage};

struct LanguageHandler *currentLanguageHandler = NULL;

struct LanguageHandler *findLanguage(char *languageName)
{

    for (int i = 0; i < sizeof(allLanguages) / sizeof(struct LanguageHandler *); i++)
    {
        struct LanguageHandler *ptr = allLanguages[i];
        if (strncasecmp(ptr->hullosLanguage, languageName, HULLOS_LANGUAGE_NAME_SIZE) == 0)
        {
            currentLanguageHandler = ptr;
            return ptr;
        }
    }
    return NULL;
}

void stopLanguageDecoding()
{
    Serial.printf("Returning to CLB command prompt\n");
    currentLanguageHandler = NULL;
}

bool processLanguageLine(char *line)
{
    if (currentLanguageHandler != NULL)
    {
        int result = currentLanguageHandler->consoleInputHandler(line);
        if (result != ERROR_OK)
        {
            printError(result);
        }
        return true;
    }
    return false;
}

struct SettingItem hullosEnabled = {
    "HullOS enabled",
    "hullosactive",
    &hullosSettings.hullosEnabled,
    ONOFF_INPUT_LENGTH,
    yesNo,
    setFalse,
    validateYesNo};

struct SettingItem runProgramOnStart = {
    "HullOS run on start",
    "hullosrunprogramonstart",
    &hullosSettings.runProgramOnStart,
    ONOFF_INPUT_LENGTH,
    yesNo,
    setTrue,
    validateYesNo};

boolean validateHullOSLanguage(void *dest, const char *newValueStr)
{
    if (findLanguage((char *)newValueStr) != NULL)
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
    "Hullos language (Rockstar, PythonIsh or HullOS)",
    "hulloslanguage",
    &hullosSettings.hullosLanguage,
    HULLOS_LANGUAGE_NAME_SIZE,
    text,
    setDefaultHullOSLanguage,
    validateHullOSLanguage};

struct SettingItem *hullosSettingItemPointers[] = {
    &hullosEnabled,
    &runProgramOnStart,
    &hullosProgramSetting};

struct SettingItemCollection hullosSettingItems = {
    "hullos",
    "HullOs management",
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
    stopLanguageDecoding();
    hullosProcess.status = HULLOS_STOPPED;
}

void startHullOS()
{
    clearVariables();

    // read in the currently executing program

    if (hullosSettings.hullosEnabled)
    {
        Serial.printf("HullOS Enabled");
        if (loadRunningProgramFromFile())
        {
            Serial.printf("HullOS program loaded");
            if (hullosSettings.runProgramOnStart)
            {
                startProgramExecution();
            }
            hullosProcess.status = HULLOS_OK;
        }
        else
        {
            hullosProcess.status = BAD_STORED_PROGRAM;
        }
    }
}

void sendMessageToHullOS(char *programText)
{
    Serial.printf("Got a program via MQTT from the server: %s\n", programText);
}


bool HullOSStartLanguage(char *languageName)
{
    Serial.printf("Starting %s\n", languageName);

    LanguageHandler *handler = findLanguage(languageName);

    if (handler != NULL)
    {
        currentLanguageHandler = handler;
        handler->setup();
        resetCommand();
        resetSerialBuffer();
        return true;
    }
    else
    {
        return false;
    }
}

void updateHullOS()
{
    // If we receive serial data the program that is running
    // must stop.

    switch (hullosProcess.status)
    {
    case HULLOS_OK:
        updateRunningProgram();
        break;
    case HULLOS_STOPPED:
        break;
    case BAD_STORED_PROGRAM:
        break;
    }
}

void stopHullOS()
{
    hullosProcess.status = HULLOS_STOPPED;
}

bool hullosStatusOK()
{
    return hullosProcess.status == HULLOS_OK;
}

void hullosStatusMessage(char *buffer, int bufferLength)
{
    switch (hullosProcess.status)
    {
    case HULLOS_OK:
        updateRunningProgram();
        snprintf(buffer, bufferLength, "HullOS running");
        break;
    case HULLOS_STOPPED:
        snprintf(buffer, bufferLength, "HullOS stopped");
        break;
    case BAD_STORED_PROGRAM:
        snprintf(buffer, bufferLength, "HullOS Bad stored program");
        break;
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
    stopHullOS,
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