#include <Arduino.h>
#include <WiFi.h>
#include <WiFiClient.h>
#include <WebServer.h>

#ifdef ARDUINO_ARCH_ESP32
#include <ESPmDNS.h>
#define MDNS_ID "esp32"
#endif

#ifdef PICO
#include <LEAmDNS.h>
#endif

#include "utils.h"
#include "settings.h"
#include "codeEditorProcess.h"
#include "codeEditorWebPage.h"
#include "processes.h"
#include "mqtt.h"
#include "errors.h"
#include "clock.h"
#include "console.h"
#include "statusled.h"
#include "console.h"
#include "pixels.h"
#include "settings.h"
#include "HullOS.h"

struct CodeEditorSettings codeEditorSettings;

void setDefaultCodeEditorDeviceName(void *dest)
{
    char *destStr = (char *)dest;

    char id_buffer[DEVICE_NAME_LENGTH];

    getProcID(id_buffer, DEVICE_NAME_LENGTH - 4);

    snprintf(destStr, DEVICE_NAME_LENGTH, "CLB-%s", id_buffer);
}

boolean validateCodeEditorDeviceName(void *dest, const char *newValueStr)
{
    return (validateString((char *)dest, newValueStr, DEVICE_NAME_LENGTH));
}

struct SettingItem codeEditorDeviceNameSetting = {
    "Code Editor device name",
    "codeeditordevicename",
    codeEditorSettings.codeEditorDeviceName,
    DEVICE_NAME_LENGTH,
    text,
    setDefaultCodeEditorDeviceName,
    validateCodeEditorDeviceName};

struct SettingItem codeEditorEnabledSetting = {
    "Code Editor enabled",
    "codeeditoron",
    &codeEditorSettings.codeEditorEnabled,
    ONOFF_INPUT_LENGTH,
    yesNo,
    setFalse,
    validateYesNo};

struct SettingItem *codeEditorSettingItemPointers[] =
    {
        &codeEditorDeviceNameSetting,
        &codeEditorEnabledSetting};

struct SettingItemCollection codeEditorMessagesSettingItems = {
    "codeeditorsettings",
    "Code Editor setup",
    codeEditorSettingItemPointers,
    sizeof(codeEditorSettingItemPointers) / sizeof(struct SettingItem *)};

// HTTP Server for code edit page

bool serverActive = false;

WebServer server(80);

void handleRoot()
{
    server.send(200, "text/html", webPage);
}

void handleNotFound()
{
    String message = "File Not Found\n\n";
    message += "URI: ";
    message += server.uri();
    message += "\nMethod: ";
    message += (server.method() == HTTP_GET) ? "GET" : "POST";
    message += "\nArguments: ";
    message += server.args();
    message += "\n";
    for (uint8_t i = 0; i < server.args(); i++)
    {
        message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
    }
    server.send(404, "text/plain", message);
}

//////////////////////////////////////////////////////////////////
/////// Program storage
//////////////////////////////////////////////////////////////////

char codeEditSource[CODE_EDIT_PROGRAM_SIZE];

void saveCode(){
    saveToFile(CODE_EDIT_PROGRAM_FILE_NAME,codeEditSource);
}

bool loadCodeFromFile(){
    return loadFromFile(CODE_EDIT_PROGRAM_FILE_NAME, codeEditSource,CODE_EDIT_PROGRAM_SIZE);
}

void sendByteToRobot(byte b)
{
#ifdef COMMAND_DEBUG
    Serial.print(F(".**processSerialByte: "));
    Serial.println((char)b);
#endif
}

bool sendStringToCodeBuffer(String str)
{
    int codeLength = str.length();

    if( codeLength > (CODE_EDIT_PROGRAM_SIZE-1)){
        return false;
    }

    for (int i = 0; i < codeLength; i++)
    {
        codeEditSource[i]=str[i];
    }
    return true;
}


void setupCodeEditorServer()
{

    alwaysDisplayMessage("Code editor starting\n");

    if (MDNS.begin(codeEditorSettings.codeEditorDeviceName))
    {
        alwaysDisplayMessage("MDNS responder started on %s\n", codeEditorSettings.codeEditorDeviceName);
    }

    server.on("/", handleRoot);

    server.on("/run", []()
              {
  alwaysDisplayMessage("Got a run request");
//  sendLineToRobot("*RS");
  handleRoot(); });

    server.on("/stop", []()
              {
//    Serial.println("Got a stop request");
//    sendLineToRobot("*RH");
    handleRoot(); });

    server.on("/save", []()
              {
    // code is the only argument
    String robotCode = server.arg(0); 
    sendStringToCodeBuffer(robotCode);
    Serial.printf("Source code received from web page: %s\n", codeEditSource);

    saveCode();
    handleRoot(); });

    server.onNotFound(handleNotFound);

    server.begin();
    serverActive = true;
}

void codeEditorMessagesOff()
{
    codeEditorSettings.codeEditorEnabled = false;
    saveSettings();
}

void codeEditorMessagesOn()
{
    codeEditorSettings.codeEditorEnabled = true;
    saveSettings();
}

void sendcodeEditorMessageToServer(char *messageText)
{
    char messageBuffer[HULLOS_PROGRAM_SIZE];
    char deviceNameBuffer[DEVICE_NAME_LENGTH];

    PrintSystemDetails(deviceNameBuffer, DEVICE_NAME_LENGTH);

    snprintf(messageBuffer, HULLOS_PROGRAM_SIZE,
             "{\"name\":\"%s\",\"from\":\"codeEditor\",\"message\":\"%s\"}",
             deviceNameBuffer,
             messageText);

    publishBufferToMQTTTopic(messageBuffer, CODE_EDITOR_TOPIC);
}

void sendMessageTocodeEditor(char *messageText)
{
    statusLedToggle();
    Serial.print((char)0x0d);
    Serial.print(messageText);
    Serial.print((char)0x0d);
    Serial.print((char)0x0a);
}

#define codeEditor_FLOAT_VALUE_OFFSET 0
#define codeEditor_MESSAGE_OFFSET (codeEditor_FLOAT_VALUE_OFFSET + sizeof(float))

boolean validatecodeEditorOptionString(void *dest, const char *newValueStr)
{
    return (validateString((char *)dest, newValueStr, CODE_EDITOR_MESSAGE_COMMAND_LENGTH));
}

boolean validatecodeEditorMessageString(void *dest, const char *newValueStr)
{
    return (validateString((char *)dest, newValueStr, CODE_EDITOR_MESSAGE_LENGTH));
}

struct CommandItem codeEditorMessageText = {
    "text",
    "codeEditor message text",
    codeEditor_MESSAGE_OFFSET,
    textCommand,
    validatecodeEditorMessageString,
    noDefaultAvailable};

struct CommandItem *codeEditorCommandItems[] =
    {
        &codeEditorMessageText};

int doSendcodeEditorMessage(char *destination, unsigned char *settingBase);

struct Command codeEditorMessageCommand{
    "Send",
    "Sends a message to the codeEditor",
    codeEditorCommandItems,
    sizeof(codeEditorCommandItems) / sizeof(struct CommandItem *),
    doSendcodeEditorMessage};

int doSendcodeEditorMessage(char *destination, unsigned char *settingBase)
{
    if (*destination != 0)
    {
        // we have a destination for the command. Build the string
        char buffer[JSON_BUFFER_SIZE];
        createJSONfromSettings("codeEditor", &codeEditorMessageCommand, destination, settingBase, buffer, JSON_BUFFER_SIZE);
        return publishCommandToRemoteDevice(buffer, destination);
    }

    if (codeEditorProcess.status != CODE_EDITOR_CONNECTED)
    {
        return JSON_MESSAGE_CODE_EDITOR_NOT_ENABLED;
    }

    char *message = (char *)(settingBase + codeEditor_MESSAGE_OFFSET);

    sendMessageTocodeEditor(message);

    TRACELOG("Sending: ");
    TRACELOGLN(buffer);

    return WORKED_OK;
}

struct Command *codeEditorCommandList[] = {
    &codeEditorMessageCommand};

struct CommandItemCollection codeEditorCommands =
    {
        "Control codeEditor",
        codeEditorCommandList,
        sizeof(codeEditorCommandList) / sizeof(struct Command *)};

unsigned long codeEditormillisAtLastScroll;

void initcodeEditorProcess()
{
    // Empty the edit file
    codeEditSource[0] = 0;
    // all the setup is performed in start
}

unsigned long millisOfLastCodeEditorUpdate;

void startcodeEditorProcess()
{
    loadCodeFromFile();

    Serial.printf("Source code: %s\n", codeEditSource);

    millisOfLastCodeEditorUpdate = millis();

    if (codeEditorSettings.codeEditorEnabled)
    {
        codeEditorProcess.status = CODE_EDITOR_WAITING_FOR_WIFI;
    }
    else
    {
        codeEditorProcess.status = CODE_EDITOR_OFF;
    }
}

#define MILLIS_BETWEEN_CODE_EDIT_UPDATES 20

void updatecodeEditorProcess()
{

	unsigned long currentMillis = millis();
	unsigned long millisSinceLastUpdate = ulongDiff(currentMillis, millisOfLastCodeEditorUpdate);

	if (millisSinceLastUpdate < MILLIS_BETWEEN_CODE_EDIT_UPDATES)
	{
        return;
	}

	millisOfLastCodeEditorUpdate = currentMillis;

    switch (codeEditorProcess.status)
    {
    case CODE_EDITOR_CONNECTED:
        server.handleClient();
#ifdef PICO
        MDNS.update();
#endif
        break;

    case CODE_EDITOR_WAITING_FOR_WIFI:
        if (WiFiProcessDescriptor.status == WIFI_OK)
        {
            setupCodeEditorServer();
            codeEditorProcess.status = CODE_EDITOR_CONNECTED;
        }

        break;

    case CODE_EDITOR_OFF:
        break;
    }
}

void stopcodeEditorProcess()
{
}

bool codeEditorStatusOK()
{
    return codeEditorProcess.status == CODE_EDITOR_CONNECTED;
}

void codeEditorStatusMessage(char *buffer, int bufferLength)
{
    switch (codeEditorProcess.status)
    {
    case CODE_EDITOR_CONNECTED:
        snprintf(buffer, bufferLength, "codeEditor connected");
        break;

    case CODE_EDITOR_WAITING_FOR_WIFI:
        snprintf(buffer, bufferLength, "codeEditor waiting for WiFi");
        break;

    case CODE_EDITOR_OFF:
        snprintf(buffer, bufferLength, "codeEditor off");
        break;
    }
}

#ifdef PROCESS_CODE_EDITOR

struct process codeEditorProcess = {
    "editor",
    initcodeEditorProcess,
    startcodeEditorProcess,
    updatecodeEditorProcess,
    stopcodeEditorProcess,
    codeEditorStatusOK,
    codeEditorStatusMessage,
    false,
    0,
    0,
    0,
    NULL,
    (unsigned char *)&codeEditorSettings, sizeof(codeEditorSettings), &codeEditorMessagesSettingItems,
    &codeEditorCommands,
    BOOT_PROCESS + ACTIVE_PROCESS + CONFIG_PROCESS + WIFI_CONFIG_PROCESS,
    NULL,
    NULL,
    NULL,
    NULL, // no command options
    0     // no command options
};

#endif