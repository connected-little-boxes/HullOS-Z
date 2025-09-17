#pragma once

#ifdef PROCESS_PRINTER

#define PRINTER_OK 1300
#define PRINTER_OFF 1301

#define PRINTERMESSAGE_LENGTH 60
#define PRINTERMESSAGE_COMMAND_LENGTH 10

struct PrinterSettings
{
    bool printerEnabled;
    bool printMessages;
    int printerBaudRate;
    int dataPin;
};

void printMessage(char *messageText, char *option);

void printerprinterMessagesOff();

void printerprinterMessagesOn();

void printSystemMessagesOn();

void printSystemMessagesOff();

extern struct PrinterSettings printerSettings;

extern struct SettingItemCollection printerSettingItems;

extern struct process printerProcess;

#endif