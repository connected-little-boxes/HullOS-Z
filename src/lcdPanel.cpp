#ifdef PROCESS_LCD_PANEL

#include <Arduino.h>

#include "utils.h"
#include "settings.h"
#include "lcdPanel.h"
#include "processes.h"
#include "mqtt.h"
#include "errors.h"
#include "clock.h"

#include <Wire.h>
#include "HD44780_LCD_PCF8574.h"

struct LcdPanelSettings lcdPanelSettings;

HD44780LCD *myLCD = NULL;

char lcdPanelMessageBuffer[LCDMESSAGE_LENGTH];

struct SettingItem LCDenabledSetting = {
    "LCD panel enabled",
    "lcdon",
    &lcdPanelSettings.lcdPanelEnabled,
    ONOFF_INPUT_LENGTH,
    yesNo,
    setFalse,
    validateYesNo};

void setDefaultLCDDataPinNo(void *dest)
{
    int *destInt = (int *)dest;
#ifdef PICO
    *destInt = 4;
#else
    *destInt = 15;
#endif
}

void setDefaultLCDClockPinNo(void *dest)
{
    int *destInt = (int *)dest;
#ifdef PICO
    *destInt = 5;
#else
    *destInt = 16;
#endif
}

void setDefaultLCDwidth(void *dest)
{
    int *destInt = (int *)dest;
    *destInt = 16;
}

void setDefaultLCDheight(void *dest)
{
    int *destInt = (int *)dest;
    *destInt = 4;
}

struct SettingItem LcdPanelDataPinNo = {
    "LCD Panel Data Pin",
    "lcdPaneldatapin",
    &lcdPanelSettings.dataPin,
    NUMBER_INPUT_LENGTH,
    integerValue,
    setDefaultLCDDataPinNo,
    validateInt};

struct SettingItem LcdPanelClockPinNo = {
    "LCD Panel Clock Pin",
    "lcdPanelclockpin",
    &lcdPanelSettings.clockPin,
    NUMBER_INPUT_LENGTH,
    integerValue,
    setDefaultLCDClockPinNo,
    validateInt};

struct SettingItem LcdPanelWidth = {
    "LCD Panel width",
    "lcdPanelwidth",
    &lcdPanelSettings.width,
    NUMBER_INPUT_LENGTH,
    integerValue,
    setDefaultLCDwidth,
    validateInt};

struct SettingItem LcdPanelHeight = {
    "LCD Panel Clock Pin",
    "lcdPanelheight",
    &lcdPanelSettings.height,
    NUMBER_INPUT_LENGTH,
    integerValue,
    setDefaultLCDheight,
    validateInt};

struct SettingItem displayMessagesSetting = {
    "LcdPanel system messages",
    "lcdmessages",
    &lcdPanelSettings.printMessages,
    ONOFF_INPUT_LENGTH,
    yesNo,
    setFalse,
    validateYesNo};

struct SettingItem *lcdPanelSettingItemPointers[] =
    {
        &LCDenabledSetting,
        &LcdPanelWidth,
        &LcdPanelHeight,
        &LcdPanelClockPinNo,
        &LcdPanelDataPinNo,
        &displayMessagesSetting};

struct SettingItemCollection lcdPanelMessagesSettingItems = {
    "lcdPanelSettings",
    "LcdPanel setup",
    lcdPanelSettingItemPointers,
    sizeof(lcdPanelSettingItemPointers) / sizeof(struct SettingItem *)};

void lcdPanelMessagesOff()
{
    lcdPanelSettings.lcdPanelEnabled = false;
    saveSettings();
}

void lcdPanelMessagesOn()
{
    lcdPanelSettings.lcdPanelEnabled = true;
    saveSettings();
}

void displaySystemMessagesOn()
{
    lcdPanelSettings.printMessages = true;
    saveSettings();
}

void displaySystemMessagesOff()
{
    lcdPanelSettings.printMessages = false;
    saveSettings();
}

void displayLCDMessage(char *messageText, int lineNo, char *dateFormat)
{
    if (myLCD == NULL)
    {
        TRACELOG("LCD panel not initialized");
        return;
    }

    if(lineNo>lcdPanelSettings.height || lineNo<1){
        displayMessage("Line number invalid in LCD message display\n");
        return;
    }

    TRACELOG("Displaying message:");
    TRACELOG(messageText);
    TRACELOG(" with date format ");
    TRACELOGLN(dateFormat);
    TRACELOG(" on line ");
    TRACELOGLN(lineNo);

    char dateBuffer[15];
    bool dateSet = false;

    if (dateFormat != NULL)
    {
        TRACELOGLN("  Got date format");

        if (strcasecmp(dateFormat, "datestamp")==0)
        {
            TRACELOGLN("    Got datestamp");
            if (getDateAndTime(dateBuffer, 14))
            {
                TRACELOGLN("      printing datestamp");
                snprintf(lcdPanelMessageBuffer, LCDMESSAGE_LENGTH, "%s%s",dateBuffer, messageText);
                dateSet=true;
            }
        }

        if (strcasecmp(dateFormat, "date")==0)
        {
            TRACELOGLN("    Got date");
            if (getDate(dateBuffer, 14))
            {
                TRACELOGLN("      printing date");
                snprintf(lcdPanelMessageBuffer, LCDMESSAGE_LENGTH, "%s%s",dateBuffer, messageText);
                dateSet=true;
            }
        }

        if (strcasecmp(dateFormat, "time")==0)
        {
            TRACELOGLN("    Got time");
            if (getTime(dateBuffer, 14))
            {
                TRACELOGLN("      printing time");
                snprintf(lcdPanelMessageBuffer, LCDMESSAGE_LENGTH, "%s%s",dateBuffer, messageText);
                dateSet=true;
            }
        }
    }

    if(!dateSet) {
        Serial.println("default print");
        snprintf(lcdPanelMessageBuffer, LCDMESSAGE_LENGTH, "%s", messageText);
    }

    HD44780LCD::LCDLineNumber_e line = (HD44780LCD::LCDLineNumber_e)lineNo;

    myLCD->PCF8574_LCDClearLine(line);
    myLCD->PCF8574_LCDGOTO(line, 0);
    myLCD->PCF8574_LCDSendString(lcdPanelMessageBuffer);

    TRACELOGLN("Display complete");
}

#define LCD_FLOAT_VALUE_OFFSET 0
#define LCD_MESSAGE_OFFSET (LCD_FLOAT_VALUE_OFFSET + sizeof(float))
#define LCD_DATE_FORMAT_OFFSET (LCD_MESSAGE_OFFSET + LCDMESSAGE_LENGTH)
#define LCD_PRE_TEXT_OFFSET (LCD_DATE_FORMAT_OFFSET + LCDMESSAGE_COMMAND_LENGTH)
#define LCD_POST_TEXT_OFFSET (LCD_PRE_TEXT_OFFSET + LCDMESSAGE_COMMAND_LENGTH)
#define LCD_LINE_NUMBER_OFFSET (LCD_POST_TEXT_OFFSET + LCDMESSAGE_COMMAND_LENGTH)

boolean validateLcdPanelOptionString(void *dest, const char *newValueStr)
{
    return (validateString((char *)dest, newValueStr, LCDMESSAGE_COMMAND_LENGTH));
}

boolean validateLCDMessageString(void *dest, const char *newValueStr)
{
    return (validateString((char *)dest, newValueStr, LCDMESSAGE_LENGTH));
}

boolean validateLCDlineNumber(void *dest, const char *newValueStr)
{
    int value;

    if (sscanf(newValueStr, "%d", &value) == 1)
    {
        if ((value > 0) && (value <= lcdPanelSettings.height))
        {
            *(int *)dest = value;
            return true;
        }

        return false;
    }

    return false;
}

bool setDefaultLCDLine(void *dest)
{
    int *destInt = (int *)dest;
    *destInt = 1;
    return true;
}

struct CommandItem LcdPanelCommandDateFormat = {
    "dateformat",
    "lcdPanel date format(datestamp,time,date)",
    LCD_DATE_FORMAT_OFFSET,
    textCommand,
    validateLcdPanelOptionString,
    setDefaultEmptyString};

struct CommandItem LcdPanelMessageText = {
    "text",
    "lcdPanel message text",
    LCD_MESSAGE_OFFSET,
    textCommand,
    validateLCDMessageString,
    noDefaultAvailable};

struct CommandItem lcdPanelPreText = {
    "pre",
    "lcdPanel pre text",
    LCD_PRE_TEXT_OFFSET,
    textCommand,
    validateLCDMessageString,
    setDefaultEmptyString};

struct CommandItem lcdPanelPostText = {
    "post",
    "lcdPanel post text",
    LCD_POST_TEXT_OFFSET,
    textCommand,
    validateLCDMessageString,
    setDefaultEmptyString};

struct CommandItem lcdPanelLineNo = {
    "line",
    "lcdPanel line number",
    LCD_LINE_NUMBER_OFFSET,
    integerCommand,
    validateLCDlineNumber,
    setDefaultLCDLine};

struct CommandItem *DisplayTextCommandItems[] =
    {
        &LcdPanelMessageText,
        &LcdPanelCommandDateFormat,
        &lcdPanelPreText,
        &lcdPanelPostText,
        &lcdPanelLineNo
    };

int doDisplayText(char *destination, unsigned char *settingBase);

struct Command displayMessageCommand{
    "display",
    "Displays text",
    DisplayTextCommandItems,
    sizeof(DisplayTextCommandItems) / sizeof(struct CommandItem *),
    doDisplayText};

int doDisplayText(char *destination, unsigned char *settingBase)
{
    if (*destination != 0)
    {
        // we have a destination for the command. Build the string
        char buffer[JSON_BUFFER_SIZE];
        createJSONfromSettings("lcdPanel", &displayMessageCommand, destination, settingBase, buffer, JSON_BUFFER_SIZE);
        return publishCommandToRemoteDevice(buffer, destination);
    }

    if (lcdPanelProcess.status != LCD_OK)
    {
        return JSON_MESSAGE_LCD_NOT_ENABLED;
    }

    char *message = (char *)(settingBase + LCD_MESSAGE_OFFSET);
    char *post = (char *)(settingBase + LCD_POST_TEXT_OFFSET);
    char *pre = (char *)(settingBase + LCD_PRE_TEXT_OFFSET);
    char *dateFormat = (char *)(settingBase + LCD_DATE_FORMAT_OFFSET);
    int *lineNo = (int *)(settingBase + LCD_LINE_NUMBER_OFFSET);

    char buffer[MAX_MESSAGE_LENGTH];

    snprintf(buffer, MAX_MESSAGE_LENGTH, "%s%s%s", pre, message, post);
    displayLCDMessage(buffer, *lineNo, dateFormat);

    TRACELOG("Printing: ");
    TRACELOGLN(buffer);

    return WORKED_OK;
}

struct Command *LCDCommandList[] = {
    &displayMessageCommand};

struct CommandItemCollection LCDCommands =
    {
        "Control lcdPanel",
        LCDCommandList,
        sizeof(LCDCommandList) / sizeof(struct Command *)};

unsigned long LCDmillisAtLastScroll;

void initLcdPanel()
{
    // perform all the hardware startup before we start the WiFi running

    lcdPanelProcess.status = LCD_OFF;
}

void displayMessageOnStatusLcdPanel(int messageNumber, ledFlashBehaviour severity, char *messageText)
{
    displayMessage(messageText, 1, NULL);
}

// declare the lcd object for auto i2c address location

void startLcdPanel()
{
    // all the hardware is set up in the init function. We just display the default message here

    if (lcdPanelSettings.lcdPanelEnabled)
    {
#ifdef PICO
        Wire.setSDA(lcdPanelSettings.dataPin);
        Wire.setSCL(lcdPanelSettings.clockPin);
        Wire.begin();
#endif

#ifdef WEMOSD1MINI
        Wire.begin(lcdPanelSettings.dataPin, lcdPanelSettings.clockPin);
#endif

#ifdef ESP32DOIT
        Wire.begin(lcdPanelSettings.dataPin, lcdPanelSettings.clockPin);
#endif

        myLCD = new HD44780LCD(lcdPanelSettings.height, lcdPanelSettings.width, 0x27, &Wire);

        myLCD->PCF8574_LCDInit(myLCD->LCDCursorTypeOn);
        myLCD->PCF8574_LCDClearScreen();
        myLCD->PCF8574_LCDBackLightSet(true);
        myLCD->PCF8574_LCDGOTO(myLCD->LCDLineNumberOne, 0);

        myLCD->PCF8574_LCDSendString("LCD STARTED");
        lcdPanelProcess.status = LCD_OK;
    }

    if (lcdPanelSettings.printMessages)
    {
        bindMessageHandler(displayMessageOnStatusLcdPanel);
    }
}

void updateLcdPanel()
{
    if (lcdPanelProcess.status != LCD_OK)
    {
        return;
    }
}

void stopLcdPanel()
{
    lcdPanelProcess.status = LCD_OFF;
}

bool lcdPanelStatusOK()
{
    return lcdPanelProcess.status == LCD_OK;
}

void lcdPanelStatusMessage(char *buffer, int bufferLength)
{
    if (lcdPanelProcess.status == LCD_OFF)
    {
        snprintf(buffer, bufferLength, "LcdPanel off");
    }
    else
    {
        snprintf(buffer, bufferLength, "LcdPanel on");
    }
}

struct process lcdPanelProcess = {
    "lcdPanel",
    initLcdPanel,
    startLcdPanel,
    updateLcdPanel,
    stopLcdPanel,
    lcdPanelStatusOK,
    lcdPanelStatusMessage,
    false,
    0,
    0,
    0,
    NULL,
    (unsigned char *)&lcdPanelSettings, sizeof(LcdPanelSettings), &lcdPanelMessagesSettingItems,
    &LCDCommands,
    BOOT_PROCESS + ACTIVE_PROCESS + CONFIG_PROCESS + WIFI_CONFIG_PROCESS,
    NULL,
    NULL,
    NULL,
    NULL, // no command options
    0     // no command options
};

#endif