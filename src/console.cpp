#include <strings.h>
#include "console.h"
#include "otaupdate.h"
#include "connectwifi.h"
#include "mqtt.h"
#include "errors.h"
#include "rotarySensor.h"
#include "buttonsensor.h"
#include "potSensor.h"
#include "pixels.h"
#include "connectwifi.h"
#include "settingsWebServer.h"
#include "HullOS.h"
#include "boot.h"
#include "utils.h"
#include <LittleFS.h>
#include "RFID.h"
#include "HullOSScript.h"

struct ConsoleSettings consoleSettings;

struct SettingItem echoSerialInput = {
	"Echo serial input",
	"echoserial",
	&consoleSettings.echoInput,
	ONOFF_INPUT_LENGTH,
	yesNo,
	setTrue,
	validateYesNo};

struct SettingItem autoSaveSettings = {
	"Auto save settings after change",
	"autosavesettings",
	&consoleSettings.autoSaveSettings,
	ONOFF_INPUT_LENGTH,
	yesNo,
	setTrue,
	validateYesNo};

struct SettingItem *consoleSettingItemPointers[] =
	{
		&echoSerialInput,
		&autoSaveSettings};

struct SettingItemCollection consoleSettingItems = {
	"Serial console",
	"Serial console configuration",
	consoleSettingItemPointers,
	sizeof(consoleSettingItemPointers) / sizeof(struct SettingItem *)};

void doHelp(char *commandLine);

char *skipCommand(char *commandLine)
{
	while ((*commandLine != ' ') && (*commandLine != 0))
	{
		commandLine++;
	}

	if (*commandLine == ' ')
	{
		commandLine++;
	}

	return commandLine;
}

void doShowSettings(char *commandLine)
{
	char *filterStart = skipCommand(commandLine);

	if (*filterStart != 0)
	{
		PrintSomeSettings(filterStart);
	}
	else
	{
		PrintAllSettings();
	}
	displayMessage("\nSettings complete\n");
}

void doDumpSettings(char *commandLine)
{

	char *filterStart = skipCommand(commandLine);

	if (*filterStart != 0)
	{
		DumpSomeSettings(filterStart);
	}
	else
	{
		DumpAllSettings();
	}
	displayMessage("\nDump complete\n");
}

#define CONSOLE_MESSAGE_SIZE 800

char consoleMessageBuffer[CONSOLE_MESSAGE_SIZE];

void doStartWebServer(char *commandLine)
{
#if defined(SETTINGS_WEB_SERVER)
	internalReboot(CONFIG_HOST_BOOT_NO_TIMEOUT_MODE);
#else
	displayMessage("The settings web server not installed on this box");
#endif
}

void doDumpStatus(char *commandLine)
{
	dumpSensorStatus();
	dumpProcessStatus();

#if defined(ARDUINO_ARCH_ESP8266)

	displayMessage("Heap: ");
	displayMessage("%u\n", ESP.getFreeHeap());

#endif

#if defined(ARDUINO_ARCH_ESP32)
	displayMessage("Heap: ");
	displayMessage("%u\n", ESP.getFreeHeap());
#endif
}

void doRestart(char *commandLine)
{
	displayMessage("Restarting...");
	saveSettings();
	delay(2000);
	internalReboot(COLD_BOOT_MODE);
}

void doClear(char *commandLine)
{
	displayMessage("Clearing settings and restarting...");
	resetSettings();
	saveSettings();
	internalReboot(COLD_BOOT_MODE);
}

void doSaveSettings(char *commandline)
{
	saveSettings();
	displayMessage("\nSettings saved");
}

#ifdef SENSOR_BUTTON
void doTestButtonSensor(char *commandline)
{
	buttonSensorTest();
}
#endif

#ifdef SENSOR_PIR
void doTestPIRSensor(char *commandline)
{
	pirSensorTest();
}
#endif

#ifdef SENSOR_ROTARY
void doTestRotarySensor(char *commandline)
{
	rotarySensorTest();
}
#endif

#ifdef SENSOR_POT
void doTestPotSensor(char *commandline)
{
	potSensorTest();
}
#endif

void doDumpListeners(char *commandline)
{
	displayMessage("\nSensor Listeners\n");
	printControllerListeners();
}

void doTestRFIDSensor(char *commandline)
{
	pollRFID();
}

void printCommandsJson(process *p)
{

	if (p->commands == NULL)
	{
		return;
	}

	struct CommandItemCollection *c = p->commands;

	if (c->noOfCommands > 0)
	{
		// have got some commands in the collection

		displayMessage("{\"name\":\"%s\",\"desc\":\"%s\",\n\"commands\":[", p->processName, c->description);

		for (int i = 0; i < c->noOfCommands; i++)
		{
			Command *com = c->commands[i];
			snprintf(consoleMessageBuffer, CONSOLE_MESSAGE_SIZE, "   ");
			appendCommandDescriptionToJson(com, consoleMessageBuffer, CONSOLE_MESSAGE_SIZE);
			displayMessage(consoleMessageBuffer);
			if (i < (c->noOfCommands - 1))
			{
				displayMessage(",");
			}
		}

		displayMessage("]}");
	}
}

void doShowRemoteCommandsJson(char *commandLine)
{
	struct process *procPtr = getAllProcessList();

	displayMessage("\n { \n\"processes\": [\n");

	while (procPtr != NULL)
	{
		if (procPtr->commands == NULL)
		{
			procPtr = procPtr->nextAllProcesses;
		}
		else
		{
			printCommandsJson(procPtr);
			procPtr = procPtr->nextAllProcesses;
			if (procPtr != NULL)
			{
				displayMessage(",");
			}
		}
	}

	displayMessage("]}");
}

void printCommandsText(process *p)
{

	if (p->commands == NULL)
	{
		return;
	}

	struct CommandItemCollection *c = p->commands;

	if (c->noOfCommands > 0)
	{
		// have got some commands in the collection

		displayMessage("Process:%s commands:%s\n", p->processName, c->description);

		for (int i = 0; i < c->noOfCommands; i++)
		{
			Command *com = c->commands[i];
			consoleMessageBuffer[0] = 0;
			appendCommandDescriptionToText(com, consoleMessageBuffer, CONSOLE_MESSAGE_SIZE);
			displayMessage(consoleMessageBuffer);
		}
	}
}

void doShowRemoteCommandsText(char *commandLine)
{
	displayMessage("\n");
	iterateThroughAllProcesses(printCommandsText);
}

void appendSensorDescriptionToJson(sensor *s, char *buffer, int bufferSize)
{
	appendFormattedString(buffer, bufferSize, "{\"name\":\"%s\",\"version\":\"%s\",\"triggers\":[",
						  s->sensorName, Version);

	for (int i = 0; i < s->noOfSensorListenerFunctions; i++)
	{
		if (i > 0)
		{
			appendFormattedString(buffer, bufferSize, ",");
		}

		sensorEventBinder *binder = &s->sensorListenerFunctions[i];

		appendFormattedString(buffer, bufferSize, "{\"name\":\"%s\"}", binder->listenerName);
	}

	appendFormattedString(buffer, bufferSize, "]}");
}

void printSensorTriggersJson(sensor *s)
{
	if (s->noOfSensorListenerFunctions == 0)
		return;

	snprintf(consoleMessageBuffer, CONSOLE_MESSAGE_SIZE, "Sensor: %s\n     ", s->sensorName);
	appendSensorDescriptionToJson(s, consoleMessageBuffer, CONSOLE_MESSAGE_SIZE);
	displayMessage("%s\n", consoleMessageBuffer);
}

void doShowSensorsJson(char *commandLine)
{
	displayMessage("\n");
	iterateThroughSensors(printSensorTriggersJson);
}

void appendSensorDescriptionToText(sensor *s, char *buffer, int bufferSize)
{
	appendFormattedString(buffer, bufferSize, "Sensor name %s\n",
						  s->sensorName);

	for (int i = 0; i < s->noOfSensorListenerFunctions; i++)
	{
		sensorEventBinder *binder = &s->sensorListenerFunctions[i];
		appendFormattedString(buffer, bufferSize, "   trigger:%s\n",
							  binder->listenerName);
	}
}

void printSensorTriggersText(sensor *s)
{
	if (s->noOfSensorListenerFunctions == 0)
		return;

	consoleMessageBuffer[0] = 0;

	appendSensorDescriptionToText(s, consoleMessageBuffer, CONSOLE_MESSAGE_SIZE);
	displayMessage("%s\n", consoleMessageBuffer);
}

void doShowSensorsText(char *commandLine)
{
	iterateThroughSensors(printSensorTriggersText);
}

void act_onJson_message(const char *json, void (*deliverResult)(char *resultText));

void showRemoteCommandResult(char *resultText)
{
	displayMessage(resultText);
}

void performRemoteCommand(char *commandLine)
{
	act_onJson_message(commandLine, showRemoteCommandResult);
}

void doClearAllListeners(char *commandLine)
{
	displayMessage("\nClearing all the listeners\n");
	clearAllListeners();
}

void doClearSensorListeners(char *commandLine)
{
	char *sensorName = skipCommand(commandLine);

	if (clearSensorNameListeners(sensorName))
	{
		displayMessage("Sensor listeners cleared");
	}
	else
	{
		displayMessage("Sensor not found");
	}
}

void doDumpStorage(char *commandLine)
{
	PrintStorage();
}

#if defined(WEMOSD1MINI) || defined(ESP32DOIT)

void doOTAUpdate(char *commandLine)
{
	if (needWifiConfigBootMode())
	{
		displayMessage("Can't perform OTA update as no WiFi networks have been configured");
		return;
	}

	requestOTAUpate();
}
#endif

void doColourDisplay(char *commandLine)
{
	displayMessage("Press space to step through each colour");
	displayMessage("Press the ESC key to exit");

	for (int i = 0; i < noOfColours; i++)
	{
		while (Serial.available())
		{
			int ch = Serial.read();
			if (ch == ESC_KEY)
			{
				displayMessage("Colour display ended");
				return;
			}
		}
		displayMessage("Colour:%s\n", colourNames[i].name);
		frame->fadeToColour(colourNames[i].col, 5);
		do
		{
			pixelProcess.udpateProcess();
			delay(20);
		} while (Serial.available() == 0);
	}
	displayMessage("Colour display finished");
}

void doDumpSprites(char *commandLine)
{
	frame->dump();
}

void dumpFilesInStores()
{
	File dir = fileOpen("/", "r");

	while (true)
	{

		File storeDir = dir.openNextFile();

		if (!storeDir)
		{
			// no more files in the folder
			break;
		}

		if (storeDir.isDirectory())
		{
			// Only dump the contents of directories
			displayMessage(" Store:%s\n", storeDir.name());
			while (true)
			{
				File storeFile = storeDir.openNextFile();

				if (!storeFile)
				{
					break;
				}

				displayMessage("   Command:%s\n", storeFile.name());

				String line = storeFile.readStringUntil(LINE_FEED);
				const char *lineChar = line.c_str();
				displayMessage("      %s\n", lineChar);
				storeFile.close();
			}
		}
	}
	dir.close();
}

void deleteFileInStore(char *deleteName)
{
	displayMessage("Deleting file:%s\n", deleteName);

	File dir = fileOpen("/", "r");

	char fullDeleteFileName[STORE_FILENAME_LENGTH];

	// set the delete filename to empty
	fullDeleteFileName[0] = 0;

	// spin until we have a file to delete
	while (fullDeleteFileName[0] == 0)
	{
		File storeDir = dir.openNextFile();

		if (!storeDir)
		{
			// no more files in the folder
			break;
		}

		if (storeDir.isDirectory())
		{
			const char *storeName = storeDir.name();

			displayMessage("   Searching store:%s\n", storeName);

			while (fullDeleteFileName[0] == 0)
			{
				File storeFile = storeDir.openNextFile();

				if (!storeFile)
				{
					break;
				}

				const char *filename = (const char *)storeFile.name();
				char compareFileName[STORE_FILENAME_LENGTH];

				// on the ESP8266 and the PICO LittleFS the dir name function just delivers the name of the file in the folder(test)
				// on the ESP32 LittleFS it delivers the file path to the file (\start\test)

#if defined(ARDUINO_ARCH_ESP32)
				buildStoreFilename(compareFileName, STORE_FILENAME_LENGTH, storeName, deleteName);
#endif

#if defined(ARDUINO_ARCH_ESP8266) || defined(PICO)
				strcpy(compareFileName, deleteName);
#endif

				displayMessage("       Checking:%s for %s\n", filename, compareFileName);

				if (strcasecmp(compareFileName, filename) == 0)
				{
#if defined(ARDUINO_ARCH_ESP32)
					strcpy(fullDeleteFileName, compareFileName);
#endif

#if defined(ARDUINO_ARCH_ESP8266) || defined(PICO)
					buildStoreFilename(fullDeleteFileName, STORE_FILENAME_LENGTH, storeName, filename);
#endif
				}

				storeFile.close();
			}
		}
	}

	dir.close();

	if (fullDeleteFileName[0] != 0)
	{
		displayMessage("\nRemoving:%s", fullDeleteFileName);
		if (LittleFS.remove(fullDeleteFileName))
		{
			displayMessage("\n   done\n");
		}
		else
		{
			displayMessage("\n   failed\n");
		};
	}
	else
	{
		displayMessage("\nFile:%s not found", deleteName);
	}
}

void doDumpStores(char *commandLine)
{
	displayMessage("\nStored commands\n");
	dumpFilesInStores();
	displayMessage("\nend of stored commands\n");
}

void doDeleteCommand(char *commandLine)
{
	char *filename = skipCommand(commandLine);
	deleteFileInStore(filename);
}

#ifdef PICO

void doFirmwareUpgradeReset(char *commandLine)
{
	displayMessage("Booting into USB drive mode for firmware update...");
	saveSettings();
	delay(2000);
	reset_usb_boot(1, 0);
}

#endif

#ifdef PROCESS_HULLOS

void doPythonIshBegin (char *commandLine)
{
	HullOSStartLanguage("PythonIsh");
	startCompiling();
	endCommand();
}

void doStartPythonIsh(char *commandLine)
{
	HullOSStartLanguage("PythonIsh");
}

void doStartRockstar(char *commandLine)
{
	HullOSStartLanguage(commandLine);
}

void doStartHullOS(char *commandLine)
{
	HullOSStartLanguage(commandLine);
}

#endif

struct consoleCommand userCommands[] =
	{
#ifdef PROCESS_HULLOS
		{"begin", "begin receiving a PythonIsh program", doPythonIshBegin},
#endif
#ifdef SENSOR_BUTTON
		{"buttontest", "test the button sensor", doTestButtonSensor},
#endif
		{"clearalllisteners", "clear all the command listeners", doClearAllListeners},
		{"clear", "clear all settings and restart the device", doClear},
		{"clearsensorlisteners", "clear the command listeners for a sensor", doClearSensorListeners},
#ifdef PROCESS_PIXELS
		{"colours", "step through all the colours", doColourDisplay},
#endif
		{"commands", "show all the remote commands", doShowRemoteCommandsText},
		{"commandsjson", "show all the remote commands in json", doShowRemoteCommandsJson},
		{"deletecommand", "delete the named command", doDeleteCommand},
		{"dump", "dump all the setting values", doDumpSettings},
		{"help", "show all the commands", doHelp},
#ifdef PROCESS_HULLOS
		{"hullos", "start the HullOS language interpreter", doStartHullOS},
#endif
#ifdef SETTINGS_WEB_SERVER
		{"host", "start the configuration web host", doStartWebServer},
#endif
		{"listeners", "list the command listeners", doDumpListeners},
		{"help", "show all the commands", doHelp},
#if defined(WEMOSD1MINI) || defined(ESP32DOIT)
		{"otaupdate", "start an over-the-air firmware update", doOTAUpdate},
#endif
#ifdef SENSOR_PIR
		{"pirtest", "test the PIR sensor", doTestPIRSensor},
#endif
#ifdef SENSOR_POT
		{"pottest", "test the pot sensor", doTestPotSensor},
#endif
#ifdef PROCESS_HULLOS
		{"pythonish", "open the PythonIsh programming interface", doStartPythonIsh},
#endif

#ifdef SENSOR_RFID
		{"rfidtest", "test the RFID sensor", doTestRFIDSensor},
#endif

#ifdef PROCESS_HULLOS
		{"rockstar", "open the Rockstar programming interface", doStartRockstar},
#endif

#ifdef SENSOR_ROTARY
		{"rotarytest", "test the rotary sensor", doTestRotarySensor},
#endif
		{"restart", "restart the device", doRestart},
		{"save", "save all the setting values", doSaveSettings},
		{"sensors", "list all the sensor triggers", doShowSensorsText},
		{"sensorsjson", "list all the sensor triggers in json", doShowSensorsJson},
		{"settings", "show all the setting values", doShowSettings},
#ifdef SENSOR_RFID
		{"setdrinksresetcard", "setup the drinks reset card", doRFIDSetupDrinksResetCard},
#endif

#ifdef PROCESS_PIXELS
		{"sprites", "dump sprite data", doDumpSprites},
#endif
		{"status", "show the sensor status", doDumpStatus},
		{"stores", "dump all the command stores", doDumpStores},
		{"storage", "show the storage use of sensors and processes", doDumpStorage},
#ifdef PICO
		{"upgrade", "resets the PICO into firmware update mode", doFirmwareUpgradeReset},
#endif
};

void doHelp(char *commandLine)
{
	displayMessage("\n\nConnected Little Boxes\nDevice Version %s\n\nThese are all the available commands.\n\n",
						 Version);

	int noOfCommands = sizeof(userCommands) / sizeof(struct consoleCommand);

	for (int i = 0; i < noOfCommands; i++)
	{
		displayMessage("    %s - %s\n", userCommands[i].name, userCommands[i].commandDescription);
	}

	displayMessage("\nYou can view the value of any setting just by typing the setting name, for example:\n\n"
						 "    mqttdevicename\n\n"
						 "- would show you the MQTT device name.\n"
						 "You can assign a new value to a setting, for example:\n\n"
						 "     mqttdevicename=Rob\n\n"
						 "- would set the name of the mqttdevicename to Rob.\n\n"
						 "To see a list of all the setting names use the command settings.\n"
						 "This displays all the settings, their values and names.\n"
						 "To see a dump of settings (which can be restored to the device later) use dump.\n"
						 "The dump and settings can be followed by a filter string to match setting names\n\n"
						 "   dump pix\n\n"
						 "- would dump all the settings that contain the string pix\n\n"
						 "If you enter a JSON string this will be interpreted as a remote command.\n"
						 "See the remote command documentation for more details of this.\n");
}

boolean findCommandName(consoleCommand *com, char *name)
{
	int commandNameLength = strlen(com->name);

	for (int i = 0; i < commandNameLength; i++)
	{
		if (tolower(name[i]) != tolower(com->name[i]))
			return false;
	}

	// reached the end of the name, character that follows should be zero (end of the string)
	// or a space delimiter to the next part of the command

	if (name[commandNameLength] == 0)
		return true;

	if (name[commandNameLength] == ' ')
		return true;

	return false;
}

struct consoleCommand *findCommand(char *commandLine, consoleCommand *commands, int noOfCommands)
{
	for (int i = 0; i < noOfCommands; i++)
	{
		if (findCommandName(&commands[i], commandLine))
		{
			return &commands[i];
		}
	}
	return NULL;
}

int performCommand(char *commandLine, consoleCommand *commands, int noOfCommands)
{
	displayMessage("Got command: %s\n", commandLine);

	if (commandLine[0] == '{')
	{
		// treat the command as JSON
		performRemoteCommand(commandLine);
		return WORKED_OK;
	}

	// Look for a command with that name

	consoleCommand *comm = findCommand(commandLine, commands, noOfCommands);

	if (comm != NULL)
	{
		comm->processLine(commandLine);
		return WORKED_OK;
	}

	// Look for a setting with that name

	processSettingCommandResult result;

	result = processSettingCommand(commandLine);

	switch (result)
	{
	case displayedOK:
		displayMessage("setting displayed OK\n");
		return true;
	case setOK:
		displayMessage("setting set OK\n");
		if (consoleSettings.autoSaveSettings)
		{
			saveSettings();
		}
		return WORKED_OK;
	case settingNotFound:
		displayMessage("setting not found\n");
		return COMMAND_SETTING_NOT_FOUND;
	case settingValueInvalid:
		displayMessage("setting value invalid\n");
		return COMMAND_SETTING_VALUE_INVALID;
	}

	return COMMAND_NO_COMMAND_FOUND;
}

void showHelp()
{
	doHelp("help");
}

#define SERIAL_BUFFER_LIMIT SERIAL_BUFFER_SIZE - 1

char serialReceiveBuffer[SERIAL_BUFFER_SIZE];

int serialReceiveBufferPos = 0;

void reset_serial_buffer()
{
	serialReceiveBufferPos = 0;
}

int actOnConsoleCommandText(char *buffer)
{
	return performCommand(buffer, userCommands, sizeof(userCommands) / sizeof(struct consoleCommand));
}

void bufferSerialChar(char ch)
{
	if (consoleSettings.echoInput)
	{
		if (ch == BACKSPACE_CHAR)
		{
			if (serialReceiveBufferPos > 0)
			{
				Serial.print(ch);
				serialReceiveBufferPos--;
				Serial.print(' ');
				Serial.print(ch);
			}
			return;
		}

		if (ch != LINE_FEED)
		{
			Serial.print(ch);
		}
	}

	if (ch == LINE_FEED || ch == '\r' || ch == 0)
	{
		if (serialReceiveBufferPos > 0)
		{
			serialReceiveBuffer[serialReceiveBufferPos] = 0;

			displayMessage("\n\r");

			if (serialReceiveBuffer[0] == '*')
			{
				hullOSdecodeScriptLine(serialReceiveBuffer + 1);
			}
			else
			{
				if (!processLanguageLine(serialReceiveBuffer))
				{
					actOnConsoleCommandText(serialReceiveBuffer);
				}
			}

			reset_serial_buffer();
			displayLanguagePrompt();
		}

		return;
	}

	if (serialReceiveBufferPos < SERIAL_BUFFER_SIZE)
	{
		serialReceiveBuffer[serialReceiveBufferPos] = ch;
		serialReceiveBufferPos++;
	}
}

void checkSerialBuffer()
{
	while (Serial.available())
	{
		bufferSerialChar(Serial.read());
	}
}

void sendMessageToConsole(char * message)
{
	displayMessage("Acting on received command\n");
	while(*message){
		bufferSerialChar(*message);
		message++;
	}
	bufferSerialChar('\r');
	displayMessage("Received command complete\n");
}

void initConsole()
{
	consoleProcessDescriptor.status = CONSOLE_OFF;
}

void startConsole()
{
	consoleProcessDescriptor.status = CONSOLE_OK;
}

void updateConsole()
{
	if (consoleProcessDescriptor.status == CONSOLE_OK)
	{
		checkSerialBuffer();
	}
}

void stopConsole()
{
	consoleProcessDescriptor.status = CONSOLE_OFF;
}

bool consoleStatusOK()
{
	return consoleProcessDescriptor.status == CONSOLE_OK;
}

void consoleStatusMessage(char *buffer, int bufferLength)
{
	switch (consoleProcessDescriptor.status)
	{
	case CONSOLE_OK:
		snprintf(buffer, bufferLength, "Console OK");
		break;
	case CONSOLE_OFF:
		snprintf(buffer, bufferLength, "Console OFF");
		break;
	default:
		snprintf(buffer, bufferLength, "Console status invalid");
		break;
	}
}

boolean validateConsoleCommandString(void *dest, const char *newValueStr)
{
	displayMessage("  Validate console command %s\n", newValueStr);
	return (validateString((char *)dest, newValueStr, CONSOLE_COMMAND_SIZE));
}

/************************************************************************
 *
 * Command offsets */

#define CONSOLE_FLOAT_VALUE_OFFSET 0
#define CONSOLE_REPORT_TEXT_OFFSET (CONSOLE_FLOAT_VALUE_OFFSET + sizeof(float))
#define CONSOLE_JSON_ATTR_OFFSET (CONSOLE_REPORT_TEXT_OFFSET + CONSOLE_MAX_MESSAGE_LENGTH)
#define CONSOLE_PRE_TEXT_OFFSET (CONSOLE_JSON_ATTR_OFFSET + CONSOLE_MAX_OPTION_LENGTH)
#define CONSOLE_POST_TEXT_OFFSET (CONSOLE_PRE_TEXT_OFFSET + CONSOLE_PRE_MESSAGE_LENGTH)
#define CONSOLE_COMMAND_OFFSET (CONSOLE_REPORT_TEXT_OFFSET)

/************************************************************************
 *
 * Command option validation */

boolean validateConsoleOptionString(void *dest, const char *newValueStr)
{
	return (validateString((char *)dest, newValueStr, CONSOLE_MAX_OPTION_LENGTH));
}

boolean validateConsoleReportString(void *dest, const char *newValueStr)
{
	return (validateString((char *)dest, newValueStr, CONSOLE_MAX_MESSAGE_LENGTH));
}

boolean validateConsolePreString(void *dest, const char *newValueStr)
{
	return (validateString((char *)dest, newValueStr, CONSOLE_PRE_MESSAGE_LENGTH));
}

boolean validateConsolePostString(void *dest, const char *newValueStr)
{
	return (validateString((char *)dest, newValueStr, CONSOLE_POST_MESSAGE_LENGTH));
}

/************************************************************************
 *
 * Command items */

struct CommandItem ConsolefloatValueItem = {
	"value",
	"value",
	CONSOLE_FLOAT_VALUE_OFFSET,
	floatCommand,
	validateFloat,
	noDefaultAvailable};

struct CommandItem ConsoleReportText = {
	"text",
	"console message text",
	CONSOLE_REPORT_TEXT_OFFSET,
	textCommand,
	validateConsoleReportString,
	noDefaultAvailable};

struct CommandItem JSONAttribute = {
	"attr",
	"json attribute for value",
	CONSOLE_JSON_ATTR_OFFSET,
	textCommand,
	validateConsoleOptionString,
	noDefaultAvailable};

struct CommandItem ConsolePreText = {
	"pre",
	"console pre text",
	CONSOLE_PRE_TEXT_OFFSET,
	textCommand,
	validateConsolePreString,
	setDefaultEmptyString};

struct CommandItem ConsolePostText = {
	"post",
	"console post text",
	CONSOLE_POST_TEXT_OFFSET,
	textCommand,
	validateConsolePostString,
	setDefaultEmptyString};

/************************************************************************
 *
 * Display message command */

struct CommandItem *ConsoledisplayMessageCommandItems[] =
	{
		&ConsoleReportText,
		&ConsolePreText,
		&ConsolePostText};

int doSetConsoleReport(char *destination, unsigned char *settingBase);

struct Command consoleReportText{
	"reporttext",
	"Reports a text message",
	ConsoledisplayMessageCommandItems,
	sizeof(ConsoledisplayMessageCommandItems) / sizeof(struct CommandItem *),
	doSetConsoleReport};

int doSetConsoleReport(char *destination, unsigned char *settingBase)
{
	TRACELOGLN("Doing console text report");

	if (*destination != 0)
	{
		// we have a destination for the command. Build the string
		char buffer[JSON_BUFFER_SIZE];
		createJSONfromSettings("console", &consoleReportText, destination, settingBase, buffer, JSON_BUFFER_SIZE);
		return publishCommandToRemoteDevice(buffer, destination);
	}

	char *message = (char *)(settingBase + CONSOLE_REPORT_TEXT_OFFSET);
	char *post = (char *)(settingBase + CONSOLE_POST_TEXT_OFFSET);
	char *pre = (char *)(settingBase + CONSOLE_PRE_TEXT_OFFSET);

	char buffer[MAX_MESSAGE_LENGTH];

	snprintf(buffer, MAX_MESSAGE_LENGTH, "%s%s%s", pre, message, post);

	displayMessage(buffer);

	TRACELOGLN("Done console display");
	return WORKED_OK;
}

/************************************************************************
 *
 * Display JSON value command */

struct CommandItem *ConsoleJSONsendValueCommandItems[] =
	{
		&ConsoleReportText,
		&JSONAttribute};

int doShowJSONvalue(char *destination, unsigned char *settingBase);

struct Command consoleReportJSONvalue{
	"reportjson",
	"report a message as a JSON object",
	ConsoleJSONsendValueCommandItems,
	sizeof(ConsoleJSONsendValueCommandItems) / sizeof(struct CommandItem *),
	doShowJSONvalue};

int doShowJSONvalue(char *destination, unsigned char *settingBase)
{
	if (*destination != 0)
	{
		// we have a destination for the command. Build the string
		char buffer[JSON_BUFFER_SIZE];
		createJSONfromSettings("console", &consoleReportJSONvalue, destination, settingBase, buffer, JSON_BUFFER_SIZE);
		return publishCommandToRemoteDevice(buffer, destination);
	}

	char *message = (char *)(settingBase + CONSOLE_REPORT_TEXT_OFFSET);
	char *attributeName = (char *)(settingBase + CONSOLE_JSON_ATTR_OFFSET);

	char buffer[MAX_MESSAGE_LENGTH];

	if (message[0] == '{')
	{
		// the message is already formatted as JSON, just send it
		snprintf(buffer, MAX_MESSAGE_LENGTH, "{\"%s\":%s}", attributeName, message);
	}
	else
	{
		// single value - wrap in quotes
		snprintf(buffer, MAX_MESSAGE_LENGTH, "{\"%s\":\"%s\"}", attributeName, message);
	}

	displayMessage(buffer);

	return WORKED_OK;
}

struct CommandItem consoleCommandName = {
	"cmd",
	"console command text",
	CONSOLE_COMMAND_OFFSET,
	textCommand,
	validateConsoleCommandString,
	noDefaultAvailable};

struct CommandItem *consoleCommandItems[] =
	{
		&consoleCommandName};

int doRemoteConsoleCommand(char *destination, unsigned char *settingBase);

struct Command performConsoleCommand{
	"remote",
	"Perform a remote console command",
	consoleCommandItems,
	sizeof(consoleCommandItems) / sizeof(struct CommandItem *),
	doRemoteConsoleCommand};

int doRemoteConsoleCommand(char *destination, unsigned char *settingBase)
{
	if (*destination != 0)
	{
		// we have a destination for the command. Build the string
		char buffer[JSON_BUFFER_SIZE];
		createJSONfromSettings("console", &performConsoleCommand, destination, settingBase, buffer, JSON_BUFFER_SIZE);
		return publishCommandToRemoteDevice(buffer, destination);
	}

	char *command = (char *)(settingBase + CONSOLE_COMMAND_OFFSET);

	displayMessage("Performing remote command: %s\n", command);

	if (performCommand(command, userCommands, sizeof(userCommands) / sizeof(struct consoleCommand)))
	{
		return WORKED_OK;
	}

	return JSON_MESSAGE_INVALID_CONSOLE_COMMAND;
}

struct Command *consoleCommandList[] = {
	&consoleReportText,
	&consoleReportJSONvalue,
	&performConsoleCommand};

struct CommandItemCollection consoleCommands =
	{
		"Perform console commands on a remote device",
		consoleCommandList,
		sizeof(consoleCommandList) / sizeof(struct Command *)};

struct process consoleProcessDescriptor = {
	"console",
	initConsole,
	startConsole,
	updateConsole,
	stopConsole,
	consoleStatusOK,
	consoleStatusMessage,
	false,
	0,
	0,
	0,
	NULL,
	(unsigned char *)&consoleSettings, sizeof(consoleSettings), &consoleSettingItems,
	&consoleCommands,
	BOOT_PROCESS + ACTIVE_PROCESS + CONFIG_PROCESS + WIFI_CONFIG_PROCESS,
	NULL,
	NULL,
	NULL};
