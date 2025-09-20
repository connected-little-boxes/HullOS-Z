#pragma once

#ifdef PROCESS_LCD_PANEL

#define LCD_OK 1400
#define LCD_OFF 1401

#define LCDMESSAGE_LENGTH 60
#define LCDMESSAGE_COMMAND_LENGTH 10

struct LcdPanelSettings
{
    bool lcdPanelEnabled;
    bool printMessages;
    int dataPin;
    int clockPin;
    int width;
    int height;
};

void displayMessage(char *messageText, char *option);

void displaySystemMessagesOn();

void displaySystemMessagesOff();

extern struct LcdPanelSettings lcdPanelSettings;

extern struct SettingItemCollection lcdPanelSettingItems;

extern struct process lcdPanelProcess;

#endif