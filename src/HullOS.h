#pragma once

#define HULLOS_OK 1400
#define HULLOS_STOPPED 1401

#define HULLOS_PROGRAM_SIZE 100
#define HULLOS_LANGUAGE_NAME_SIZE 20
#define HULLOS_PROGRAM_COMMAND_LENGTH 20

struct HullOSSettings {
	bool hullosEnabled;
	unsigned char hullosLanguage[HULLOS_LANGUAGE_NAME_SIZE];
};

void hullosOff();

void hullosOn();

extern struct HullOSSettings hullosSettings;

extern struct SettingItemCollection hullosSettingItems;

extern struct process hullosProcess;

void HullOSStartPythonIsh(char *commandLine);
void HullOSStartRockstar(char *commandLine);

void sendMessageToHullOS(char *programText);