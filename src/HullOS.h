#pragma once

#define HULLOS_OK 1400
#define HULLOS_STOPPED 1401
#define HULLOS_DEVICE_STARTED 1402

#define HULLOS_PROGRAM_SIZE 2000
#define HULLOS_LANGUAGE_NAME_SIZE 20
#define HULLOS_PROGRAM_COMMAND_LENGTH 120


struct HullOSSettings {
	bool hullosEnabled;
	bool runProgramOnStart;
	unsigned char hullosLanguage[HULLOS_LANGUAGE_NAME_SIZE];
};

struct LanguageHandler {
	char hullosLanguage[HULLOS_LANGUAGE_NAME_SIZE];
	void (*setup)(void);
	int (*consoleInputHandler)(char *);
	void (*displayPrompt)();
};

void HullOSShowPrompt();

extern struct LanguageHandler hullOSLanguage;

bool processLanguageLine(char * line);
void stopLanguageDecoding();
bool displayLanguagePrompt();


void hullosOff();

void hullosOn();

extern struct HullOSSettings hullosSettings;

extern struct SettingItemCollection hullosSettingItems;

extern struct process hullosProcess;

bool HullOSStartLanguage(char * languageName);

void sendMessageToHullOS(char *programText);

int hullOSdecodeScriptLine(char *input);
