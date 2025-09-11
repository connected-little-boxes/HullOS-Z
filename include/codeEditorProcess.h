#pragma once


#define CODE_EDITOR_CONNECTED 6000
#define CODE_EDITOR_WAITING_FOR_WIFI 6001
#define CODE_EDITOR_OFF 6002

#define CODE_EDITOR_MESSAGE_LENGTH 1000
#define CODE_EDITOR_MESSAGE_COMMAND_LENGTH 10
#define CODE_EDIT_PROGRAM_SIZE 500
#define CODE_EDIT_PROGRAM_FILE_NAME "code.txt"


#define CODE_EDITOR_TOPIC "codeEditor"

struct CodeEditorSettings
{
	char codeEditorDeviceName[DEVICE_NAME_LENGTH];
    bool codeEditorEnabled;
};

void sendMessageTocodeEditor(char *messageText);

void codeEditorOff();

void codeEditorOn();

extern struct CodeEditorSettings codeEditorSettings;

extern struct SettingItemCollection codeEditorSettingItems;

extern struct process codeEditorProcess;
