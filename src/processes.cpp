#include <strings.h>

#include "debug.h"

#include "processes.h"
#include "pixels.h"
#include "utils.h"
#include "messages.h"
#include "settings.h"

struct process *activeProcessList = NULL;

struct process *allProcessList = NULL;

struct process *getAllProcessList()
{
	return allProcessList;
}

void addProcessToAllProcessList(struct process *newProcess)
{
	newProcess->nextAllProcesses = NULL;

	if (allProcessList == NULL)
	{
		allProcessList = newProcess;
	}
	else
	{
		process *addPos = allProcessList;

		while (addPos->nextAllProcesses != NULL)
		{
			addPos = addPos->nextAllProcesses;
		}
		addPos->nextAllProcesses = newProcess;
	}
}

void addProcessToActiveProcessList(struct process *newProcess)
{
	newProcess->nextActiveProcess = NULL;

	if (activeProcessList == NULL)
	{
		activeProcessList = newProcess;
	}
	else
	{
		process *addPos = activeProcessList;

		while (addPos->nextActiveProcess != NULL)
		{
			addPos = addPos->nextActiveProcess;
		}
		addPos->nextActiveProcess = newProcess;
	}
}

void buildActiveProcessListFromMask(int processMask)
{
	activeProcessList = NULL;

	struct process *processPtr = allProcessList;

	while (processPtr != NULL)
	{
		if ((processPtr->processStartMask & processMask) != 0)
		{
			addProcessToActiveProcessList(processPtr);
		}
		processPtr = processPtr->nextAllProcesses;
	}
}

struct process *findProcessByName(const char *name)
{
	struct process *procPtr = allProcessList;

	while (procPtr != NULL)
	{
		if (strcasecmp(procPtr->processName, name) == 0)
		{
			return procPtr;
		}
		procPtr = procPtr->nextAllProcesses;
	}
	return NULL;
}

struct process *findActiveProcessByName(const char *name)
{
	struct process *procPtr = activeProcessList;

	while (procPtr != NULL)
	{
		if (strcasecmp(procPtr->processName, name) == 0)
		{
			return procPtr;
		}
		procPtr = procPtr->nextActiveProcess;
	}
	return NULL;
}

struct process *findProcessSettingCollectionByName(const char *name)
{
	struct process *procPtr = allProcessList;

	while (procPtr != NULL)
	{
		if (procPtr->settingItems != NULL)
		{
			if (strcasecmp(procPtr->settingItems->collectionName, name) == 0)
			{
				return procPtr;
			}
		}
		procPtr = procPtr->nextAllProcesses;
	}
	return NULL;
}

#define PROCESS_STATUS_BUFFER_SIZE 300

char processStatusBuffer[PROCESS_STATUS_BUFFER_SIZE];

void startProcess(process *proc)
{
	displayMessage(F("   %s: "), proc->processName);
	proc->startProcess();
	proc->getStatusMessage(processStatusBuffer, PROCESS_STATUS_BUFFER_SIZE);
	displayMessage(F(" %s\n"), processStatusBuffer);
	proc->beingUpdated = true; // process only gets updated after it has started
}

struct process *startProcessByName(char *name)
{
	struct process *targetProcess = findProcessByName(name);

	if (targetProcess != NULL)
	{
		if (!targetProcess->beingUpdated)
		{
			targetProcess->startProcess();
			targetProcess->beingUpdated = true;
		}
	}

	return targetProcess;
}

void initialiseAllProcesses()
{
	//	messageLogf("Initialising processes\n\n");

	struct process *procPtr = allProcessList;

	while (procPtr != NULL)
	{
		// messageLogf("   initilising: %s\n", procPtr->processName);
		procPtr->totalTime = 0;
		procPtr->initProcess();
		DISPLAY_MEMORY_MONITOR(procPtr->processName);
		procPtr = procPtr->nextAllProcesses;
	}
}

void startProcesses()
{
	displayMessage(F("Starting processes\n"));

	struct process *procPtr = activeProcessList;

	while (procPtr != NULL)
	{
		if (procPtr->beingUpdated)
		{
			// only start processes that aren't active
			continue;
		}
		startProcess(procPtr);
		procPtr = procPtr->nextActiveProcess;
	}
}

void updateProcesses()
{
	struct process *procPtr = activeProcessList;

	while (procPtr != NULL)
	{
		unsigned long startMicros = micros();
		//		displayMessage(F("%s\n"), procPtr->processName);
		procPtr->udpateProcess();
		unsigned long runTime = ulongDiff(micros(), startMicros);
		if (messagesSettings.speedMessagesEnabled)
		{
			if (runTime > SLOW_PROCESS_TIME_MICROS)
			{
				displayMessage(F("  %s running slow: %ul\n"), procPtr->processName, runTime);
			}
		}
		procPtr->activeTime = runTime;
		procPtr->totalTime = procPtr->totalTime + runTime;
		DISPLAY_MEMORY_MONITOR(procPtr->processName);
		procPtr = procPtr->nextActiveProcess;
	}
}

void dumpProcessStatus()
{
	displayMessage(F("Processes\n"));

	struct process *procPtr = activeProcessList;

	while (procPtr != NULL)
	{
		if (procPtr->beingUpdated)
		{
			displayMessage(F("    %s:"), procPtr->processName);
			procPtr->getStatusMessage(processStatusBuffer, PROCESS_STATUS_BUFFER_SIZE);
			displayMessage(F("%s Active time: "), processStatusBuffer);
			displayMessage(F("%lu millisecs"), procPtr->activeTime / 1000);
			displayMessage(F(" Total time: "));
			displayMessage(F("%lu millisecs\n"), procPtr->totalTime / 1000);
		}
		procPtr = procPtr->nextActiveProcess;
	}
}

bool dumpProcessStatusFiltered(const char *name)
{

	struct process *procPtr = activeProcessList;

	while (procPtr != NULL)
	{
		if (procPtr->beingUpdated)
		{
			if (strcasecmp(procPtr->processName, name) == 0)
			{
				displayMessage(F("    %s:"), procPtr->processName);
				procPtr->getStatusMessage(processStatusBuffer, PROCESS_STATUS_BUFFER_SIZE);
				displayMessage(F("%s Active time: "), processStatusBuffer);
				displayMessage(F("%lu millisecs"), procPtr->activeTime / 1000);
				displayMessage(F(" Total time: "));
				displayMessage(F("%lu millisecs\n"), procPtr->totalTime / 1000);
				return true;
			}
		}
		procPtr = procPtr->nextActiveProcess;
	}
	return false;
}

int getProcessStatus(const char *name)
{
	struct process *procPtr = activeProcessList;

	while (procPtr != NULL)
	{
		if (procPtr->beingUpdated)
		{
			if (strcasecmp(procPtr->processName, name) == 0)
			{
				return procPtr->status;
			}
		}
		procPtr = procPtr->nextActiveProcess;
	}
	return -1;
}

void iterateThroughAllProcesses(void (*func)(process *p))
{
	struct process *procPtr = allProcessList;

	while (procPtr != NULL)
	{
		func(procPtr);
		procPtr = procPtr->nextAllProcesses;
	}
}

void iterateThroughActiveProcesses(void (*func)(process *p))
{
	struct process *procPtr = activeProcessList;

	while (procPtr != NULL)
	{
		func(procPtr);
		procPtr = procPtr->nextActiveProcess;
	}
}

void stopProcesses()
{
	displayMessage(F("Stopping processes\n"));

	struct process *procPtr = activeProcessList;

	while (procPtr != NULL)
	{
		displayMessage(F("   %s\n"), procPtr->processName);
		procPtr->stopProcess();
		procPtr->beingUpdated = false;
		procPtr = procPtr->nextAllProcesses;
	}
}

void iterateThroughProcessSettings(void (*func)(unsigned char *settings, int size))
{
	struct process *procPtr = allProcessList;

	while (procPtr != NULL)
	{
		// messageLogf("  Process %s\n", procPtr->processName);
		func(procPtr->settingsStoreBase,
			 procPtr->settingsStoreLength);
		procPtr = procPtr->nextAllProcesses;
	}
}

void iterateThroughProcessSettingCollections(void (*func)(SettingItemCollection *s))
{
	struct process *procPtr = allProcessList;

	while (procPtr != NULL)
	{
		if (procPtr->settingItems != NULL)
		{
			func(procPtr->settingItems);
		}
		procPtr = procPtr->nextAllProcesses;
	}
}

void iterateThroughProcessCommandCollections(void (*func)(CommandItemCollection *c))
{
	struct process *procPtr = allProcessList;

	while (procPtr != NULL)
	{
		if (procPtr->commands != NULL)
		{
			func(procPtr->commands);
		}
		procPtr = procPtr->nextAllProcesses;
	}
}

void iterateThroughProcessCommands(void (*func)(Command *c))
{
	struct process *procPtr = allProcessList;

	while (procPtr != NULL)
	{
		if (procPtr->commands != NULL)
		{
			for (int j = 0; j < procPtr->commands->noOfCommands; j++)
			{
				func(procPtr->commands->commands[j]);
			}
		}
		procPtr = procPtr->nextAllProcesses;
	}
}

Command *FindCommandInProcess(process *procPtr, const char *commandName)
{
	TRACELOG("Finding command:");
	TRACELOGLN(commandName);

	for (int i = 0; i < procPtr->commands->noOfCommands; i++)
	{
		TRACELOG("    checking:");
		TRACELOGLN(procPtr->commands->commands[i]->name);
		if (strcasecmp(procPtr->commands->commands[i]->name, commandName) == 0)
		{
			TRACELOGLN("    Found it!");
			return procPtr->commands->commands[i];
		}
	}
	TRACELOGLN("    Not found");
	return NULL;
}

Command *FindCommandByName(const char *processName, const char *name)
{
	struct process *procPtr = findProcessByName(processName);

	if (procPtr == NULL)
	{
		return NULL;
	}

	for (int i = 0; i < procPtr->commands->noOfCommands; i++)
	{
		if (strcasecmp(procPtr->commands->commands[i]->name, name) == 0)
		{
			return procPtr->commands->commands[i];
		}
	}
	return NULL;
}

void iterateThroughProcessSettings(void (*func)(SettingItem *s))
{
	struct process *procPtr = allProcessList;

	while (procPtr != NULL)
	{
		if (procPtr->settingItems != NULL)
		{
			for (int j = 0; j < procPtr->settingItems->noOfSettings; j++)
			{
				func(procPtr->settingItems->settings[j]);
			}
		}
		procPtr = procPtr->nextAllProcesses;
	}
}

void resetProcessesToDefaultSettings()
{
	struct process *procPtr = allProcessList;

	while (procPtr != NULL)
	{
		if (procPtr->settingItems != NULL)
		{
			for (int j = 0; j < procPtr->settingItems->noOfSettings; j++)
			{
				void *dest = procPtr->settingItems->settings[j]->value;
				procPtr->settingItems->settings[j]->setDefault(dest);
				displayMessage("Default set: ");
				printSetting(procPtr->settingItems->settings[j]);
			}
		}
		procPtr = procPtr->nextAllProcesses;
	}
}

SettingItem *FindProcesSettingByFormName(const char *settingName)
{
	struct process *procPtr = allProcessList;

	while (procPtr != NULL)
	{
		if (procPtr->settingItems != NULL)
		{
			SettingItemCollection *testItems = procPtr->settingItems;

			for (int j = 0; j < procPtr->settingItems->noOfSettings; j++)
			{
				SettingItem *testSetting = testItems->settings[j];
				if (matchSettingName(testSetting, settingName))
				{
					return testSetting;
				}
			}
		}
		procPtr = procPtr->nextAllProcesses;
	}
	return NULL;
}
