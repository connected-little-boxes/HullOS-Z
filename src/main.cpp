#define sillyX

#ifdef silly

/*!
 @file     HelloWorldSTM32.ino
 @author   Gavin Lyons
 @brief
    Hello World for HD44780_LCD_PCF8574 arduino library  for STM32 "blue pill"
 @note Allows testing of both I2C ports 1 and 2 on the STM32 "blue pill" board
*/

#include <Wire.h>

// Section: Included library
#include "HD44780_LCD_PCF8574.h"

HD44780LCD myLCD(4, 20, 0x27, &Wire);

// Section: Setup

void setup()
{
  delay(300);
  Wire.setSDA(4);
  Wire.setSCL(5);
  Wire.begin();

  delay(100);
  myLCD.PCF8574_LCDInit(myLCD.LCDCursorTypeOn);
  myLCD.PCF8574_LCDClearScreen();
  myLCD.PCF8574_LCDBackLightSet(true);
  myLCD.PCF8574_LCDGOTO(myLCD.LCDLineNumberOne, 0);
}

// Section: Main Loop

void loop()
{
  char testString[] = "Hello World";
  myLCD.PCF8574_LCDSendString(testString);
  myLCD.PCF8574_LCDSendChar('!'); // Display a single character
  myLCD.PCF8574_LCDGOTO(myLCD.LCDLineNumberTwo, 0);
  myLCD.PCF8574_LCDSendString(testString);
  myLCD.PCF8574_LCDGOTO(myLCD.LCDLineNumberThree, 0);
  myLCD.PCF8574_LCDSendString(testString);
  myLCD.PCF8574_LCDGOTO(myLCD.LCDLineNumberFour, 0);
  myLCD.PCF8574_LCDSendString(testString);
  while (true)
  {
  };
}

// EOF
#else

#include <Arduino.h>
#include <SPI.h>
#include <Wire.h>
#include "LittleFS.h"

#include "debug.h"
#include "utils.h"

#if defined(PROCESS_PIXELS)
#include "pixels.h"
#endif

#include "settings.h"
#include "processes.h"
#include "sensors.h"
#include "pirSensor.h"
#include "buttonsensor.h"
#include "inputswitch.h"
#include "controller.h"
#include "statusled.h"
#include "messages.h"
#include "console.h"
#include "connectwifi.h"
#include "mqtt.h"
#include "servoproc.h"
#include "clock.h"
#include "registration.h"
#include "rotarySensor.h"
#include "potSensor.h"
#include "settingsWebServer.h"
#include "MAX7219Messages.h"
#include "printer.h"
#include "BME280Sensor.h"
#include "HullOS.h"
#include "boot.h"
#include "otaupdate.h"
#include "outpin.h"
#include "RFID.h"
#include "Motors.h"
#include "distance.h"
#include "codeEditorProcess.h"
#include "lcdPanel.h"
#include "robotProcess.h"

#ifdef PROCESS_MOTOR

#ifdef PICO_CORE_MOTOR

void setup1()
{
}

void loop1()
{
  updateMotorsCore1();
}

#endif

#endif

// This function will be different for each build of the device.

void populateProcessList()
{
#if defined(PROCESS_CONSOLE)
  addProcessToAllProcessList(&consoleProcessDescriptor);
#endif
#if defined(PROCESS_CONTROLLER)
  addProcessToAllProcessList(&controllerProcess);
#endif
#ifdef PROCESS_HULLOS
  addProcessToAllProcessList(&hullosProcess);
#endif
#if defined(PROCESS_INPUT_SWITCH)
  addProcessToAllProcessList(&inputSwitchProcess);
#endif
#ifdef PROCESS_MAX7219
  addProcessToAllProcessList(&max7219MessagesProcess);
#endif
#if defined(PROCESS_MESSAGES)
  addProcessToAllProcessList(&messagesProcess);
#endif
#ifdef PROCESS_OUTPIN
  addProcessToAllProcessList(&outPinProcess);
#endif
#if defined(PROCESS_MQTT)
  addProcessToAllProcessList(&MQTTProcessDescriptor);
#endif
#if defined(PROCESS_PIXELS)
  addProcessToAllProcessList(&pixelProcess);
#endif
#ifdef PROCESS_PRINTER
  addProcessToAllProcessList(&printerProcess);
#endif
#if defined(PROCESS_REGISTRATION)
  addProcessToAllProcessList(&RegistrationProcess);
#endif
#ifdef PROCESS_SERVO
  addProcessToAllProcessList(&ServoProcess);
#endif
#if defined(PROCESS_STATUS_LED)
  addProcessToAllProcessList(&statusLedProcess);
#endif
#if defined(PROCESS_WIFI)
  addProcessToAllProcessList(&WiFiProcessDescriptor);
#endif
#if defined(PROCESS_MOTOR)
  addProcessToAllProcessList(&motorProcessDescriptor);
#endif
#if defined(PROCESS_LCD_PANEL)
  addProcessToAllProcessList(&lcdPanelProcess);
#endif
#if defined(PROCESS_CODE_EDITOR)
  addProcessToAllProcessList(&codeEditorProcess);
#endif
#if defined(PROCESS_ROBOT)
  addProcessToAllProcessList(&robotProcess);
#endif
}

void populateSensorList()
{
#ifdef SENSOR_BME280
  addSensorToAllSensorsList(&bme280Sensor);
  addSensorToActiveSensorsList(&bme280Sensor);
#endif
#ifdef SENSOR_BUTTON
  addSensorToAllSensorsList(&buttonSensor);
  addSensorToActiveSensorsList(&buttonSensor);
#endif
#ifdef SENSOR_CLOCK
  addSensorToAllSensorsList(&clockSensor);
  addSensorToActiveSensorsList(&clockSensor);
#endif
#ifdef SENSOR_POT
  addSensorToAllSensorsList(&potSensor);
  addSensorToActiveSensorsList(&potSensor);
#endif
#ifdef SENSOR_PIR
  addSensorToAllSensorsList(&pirSensor);
  addSensorToActiveSensorsList(&pirSensor);
#endif
#ifdef SENSOR_RFID
  addSensorToAllSensorsList(&RFIDSensor);
  addSensorToActiveSensorsList(&RFIDSensor);
#endif
#ifdef SENSOR_ROTARY
  addSensorToAllSensorsList(&rotarySensor);
  addSensorToActiveSensorsList(&rotarySensor);
#endif
#ifdef SENSOR_DISTANCE
  addSensorToAllSensorsList(&Distance);
  addSensorToActiveSensorsList(&Distance);
#endif
}

void displayControlMessage(int messageNumber, ledFlashBehaviour severity, char *messageText)
{
  char buffer[20];

  ledFlashBehaviourToString(severity, buffer, 20);

  displayMessage(F("%s: %d %s\n"), buffer, messageNumber, messageText);
}

unsigned long heapPrintTime = 0;

void startDevice()
{

#if defined(PICO_USE_UART)
  Serial1.begin(115200);
  delay(100);
  while (1)
  {
    Serial1.print(".");
    delay(200);
  }
#else
  Serial.begin(115200);
#endif

#ifdef STARTUP_WAIT

  for (int i = 0; i < 10; i++)
  {
    delay(1000);
    displayMessageWithNewline(F("%d"), 10 - i);
    if (Serial.available())
    {
      break;
    }
  }

#endif

  Serial.printf("\n\nStarting\n\n");

  char deviceNameBuffer[DEVICE_NAME_LENGTH];
  PrintSystemDetails(deviceNameBuffer, DEVICE_NAME_LENGTH);
  Serial.printf("%s\n", deviceNameBuffer);
  Serial.printf("Connected Little Boxes Device\n");
  Serial.printf("Powered by HULLOS-X\n");
  Serial.printf("www.connectedlittleboxes.com\n");
  Serial.printf("Version %s build date: %s %s\n", Version, __DATE__, __TIME__);

#ifdef DEBUG
  messageLogf("**** Debug output enabled");
#endif

  START_MEMORY_MONITOR();

  populateProcessList();

  DISPLAY_MEMORY_MONITOR("Populate process list");

  populateSensorList();

  DISPLAY_MEMORY_MONITOR("Populate sensor list");

  Serial.printf("Setup settings complete\n");

  // set the parameter to true to force a setting request
  // useful if one of the settings as broken the boot

  SettingsSetupStatus status = setupSettings(false);

  switch (status)
  {
  case SETTINGS_SETUP_OK:
    Serial.printf("Settings loaded OK\n");
    break;
  case SETTINGS_RESET_TO_DEFAULTS:
    Serial.printf("Settings reset to defaults\n");
    break;
  case SETTINGS_FILE_SYSTEM_FAIL:
    Serial.printf("Settings file system fail\n");
    break;
  default:
    Serial.printf("Invalid setupSettings return\n");
  }

  // get the boot mode
  getBootMode();

  if (bootMode == COLD_BOOT_MODE)
  {

#if defined(SETTINGS_WEB_SERVER)
    // first power up
    if (needWifiConfigBootMode())
    {
      // start hosting the config website and request a timeout
      startHostingConfigWebsite(true);
    }
  }
  else
  {

    if (bootMode == CONFIG_HOST_BOOT_NO_TIMEOUT_MODE)
    {
      startHostingConfigWebsite(false);
    }

    if (bootMode == CONFIG_HOST_TIMEOUT_BOOT_MODE)
    {
      startHostingConfigWebsite(true);
    }
#endif

#if defined(WEMOSD1MINI) || defined(ESP32DOIT)
    if (bootMode == OTA_UPDATE_BOOT_MODE)
    {
      performOTAUpdate();
    }
#endif
  }

  startstatusLedFlash(1000);

  initialiseAllProcesses();

  DISPLAY_MEMORY_MONITOR("Initialise all processes\n");

  buildActiveProcessListFromMask(BOOT_PROCESS);

  startProcesses();

  DISPLAY_MEMORY_MONITOR("Initialise all processes\n");

  bindMessageHandler(displayControlMessage);

  startSensors();

  DISPLAY_MEMORY_MONITOR("Start all sensors\n");

  delay(1000); // show the status for a while

  displayMessage(F("Start complete\n\nType help and press enter for help\n\n"));
}

#if defined(ARDUINO_ARCH_ESP8266) || defined(ARDUINO_ARCH_ESP32)
uint32_t oldHeap = 0;

#define HEAP_PRINT_INTERVAL 500

void heapMonitor()
{
  unsigned long m = millis();
  if ((m - heapPrintTime) > HEAP_PRINT_INTERVAL)
  {
    uint32_t newHeap = ESP.getFreeHeap();
    if (newHeap != oldHeap)
    {
      displayMessage(F("Heap: "));
      displayMessage(F("%lu"), newHeap);
      oldHeap = newHeap;
    }
    heapPrintTime = millis();
  }
}
#endif

void setup()
{
  delay(3000);
  startDevice();
}

void loop()
{
  updateSensors();
  updateProcesses();
  delay(5);
  //  DISPLAY_MEMORY_MONITOR("System");
}

#endif