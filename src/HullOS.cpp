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
#include "PythonIsh.h"
#include "messages.h"

struct HullOSSettings hullosSettings;

int hullOSdecodeScriptLine(char *input)
{
    //    displayMessage(F("Hullos decoding: %s\n"), input);

    if (strcasecmp(input, "exit") == 0)
    {
        stopLanguageDecoding();
        return ERROR_OK;
    }

    // put a terminator on the end

    int len = strlen(input);
    input[len] = STATEMENT_TERMINATOR;
    len++;

    hullOSActOnStatement(input, input + len);

    return ERROR_OK;
}

void HullOSStartProgramOnReset();

void hullOSDecoderStart()
{
    displayMessage(F("Starting HullOS decoder"));
    HullOSStartProgramOnReset();
}

struct LanguageHandler hullOSLanguage = {
    "HullOS",
    hullOSDecoderStart,
    hullOSdecodeScriptLine,
    HullOSShowPrompt,
    '*'
};

void HullOSShowPrompt()
{
    if (storingProgram())
    {
        displayMessage(F("H*>"));
    }
    else
    {
        displayMessage(F("H>"));
    }
}

struct LanguageHandler *allLanguages[] =
    {
        &PythonIshLanguage,
#ifdef LANGUAGE_ROCKSTAR
        &RockstarLanguage,
#endif
        &hullOSLanguage
    };

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
    displayMessage(F("Returning to CLB command prompt\n"));
    currentLanguageHandler = NULL;
}

bool displayLanguagePrompt()
{
    if (currentLanguageHandler == NULL)
    {
        return false;
    }

    currentLanguageHandler->displayPrompt();

    return true;
}

bool processLanguageLine(char *line)
{
    if (currentLanguageHandler == NULL)
    {
        return false;
    }

    if (strcasecmp(line, "Exit") == 0)
    {
        displayMessage(F("%s session ended\n"), currentLanguageHandler->hullosLanguage);
        stopLanguageDecoding();
        return true;
    }

    int result = currentLanguageHandler->consoleInputHandler(line);

    if (result != ERROR_OK)
    {
        printError(result);
    }

    return true;
}

struct SettingItem hullosEnabled = {
    "HullOS enabled",
    "hullosactive",
    &hullosSettings.hullosEnabled,
    ONOFF_INPUT_LENGTH,
    yesNo,
    setTrue,
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
    hullosProcess.status = HULLOS_DEVICE_STARTED;

    clearVariables();
}

char *programTextPos;

#define PROGRAM_TEXT_LINE_BUFFER_SIZE 100
char programTextLineBuffer[PROGRAM_TEXT_LINE_BUFFER_SIZE + 1];

bool getProgramTextLine()
{

    // return false if we are at the end of the string

    if (!*programTextPos)
    {
        return false;
    }

    int chCount = 0;

    while (true)
    {

        char ch = *programTextPos;
        //        displayMessage(F("Got a: %d %c\n"), ch, ch);

        if (ch == 0)
        {
            // end of the input string
            if (chCount == 0)
            {
                return false;
            }
            programTextLineBuffer[chCount] = 0;
            return true;
        }

        if (ch == STATEMENT_TERMINATOR)
        {
            //            displayMessage(F("Reached the end of the string\n"));
            programTextLineBuffer[chCount] = 0;
            programTextPos++;
            return true;
        }

        if (ch < ' ')
        {
            programTextPos++;
            continue;
        }

        programTextLineBuffer[chCount] = ch;

        chCount++;
        programTextPos++;
    }
}

void sendMessageToHullOS(char *programText)
{
    displayMessage(F("Got command via MQTT from the server: %s\n"), programText);

    if (programText[0] == '*')
    {
        displayMessage(F("Performing HullOS command\n "));
        hullOSdecodeScriptLine(programText + 1);
        return;
    }

    displayMessage(F("Processing a PythonIsh program\n"));

    pythonIshdecodeScriptLine("begin");

    programTextPos = programText;

    while (getProgramTextLine())
    {
        displayMessage(F("   got a line:%s\n"), programTextLineBuffer);
        pythonIshdecodeScriptLine(programTextLineBuffer);
    }

    pythonIshdecodeScriptLine("end");

    pythonIshdecodeScriptLine("save \"active.txt\"");

    pythonIshdecodeScriptLine("load \"active.txt\"");
}

bool HullOSStartLanguage(char *languageName)
{
    displayMessage(F("Starting %s\n"), languageName);

    LanguageHandler *handler = findLanguage(languageName);

    if (handler != NULL)
    {
        currentLanguageHandler = handler;
        handler->setup();
        resetCommand();
        return true;
    }
    else
    {
        return false;
    }
}

void HullOSStartProgramOnReset()
{
    if (hullosSettings.hullosEnabled)
    {
        displayMessage(F("HullOS Enabled\n"));

        if (loadFromFile(RUNNING_PROGRAM_FILENAME, HullOScodeRunningCode, HULLOS_PROGRAM_SIZE))
        {
            displayMessage(F("HullOS program loaded\n"));
            dumpProgram(HullOScodeRunningCode);
            if (hullosSettings.runProgramOnStart)
            {
                displayMessage(F("Starting execution\n"));
                startProgramExecution(true);
            }
        }
    }

    hullosProcess.status = HULLOS_OK;
}

void updateHullOS()
{
    // If we receive serial data the program that is running
    // must stop.

    switch (hullosProcess.status)
    {
    case HULLOS_DEVICE_STARTED:
        HullOSStartProgramOnReset();
        break;
    case HULLOS_OK:
        updateRunningProgram();
        break;
    case HULLOS_STOPPED:
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
        programStatus(buffer, bufferLength);
        break;
    case HULLOS_STOPPED:
        snprintf(buffer, bufferLength, "HullOS stopped");
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
        startProgramExecution(true);
        commandPerformed = true;
    }

    if (strcasecmp(command, "no") == 0)
    {
        displayMessageWithNewline(F("Doing stop"));
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