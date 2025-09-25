#include "utils.h"
#include "processes.h"
#include "settings.h"
#include "pixels.h"
#include "otaupdate.h"
#include "connectwifi.h"
#include "boot.h"
#include "statusled.h"

#if defined(ARDUINO_ARCH_ESP8266)
#include <ESP8266httpUpdate.h>
#endif

struct OtaUpdateSettings otaUpdateSettings;

struct SettingItem otaUpdateRequested = {
	"otaUpdateRequested (yes or no)",
	"otaupdaterequested",
	&otaUpdateSettings.otaUpdateRequested,
	ONOFF_INPUT_LENGTH,
	yesNo,
	setFalse, validateYesNo};

struct SettingItem *otaUpdateSettingItemPointers[] =
	{
		&otaUpdateRequested};

struct SettingItemCollection otaUpdateSettingItems = {
	"otaupdate",
	"Over The Air (OTA) update settings",
	otaUpdateSettingItemPointers,
	sizeof(otaUpdateSettingItemPointers) / sizeof(struct SettingItem *)};

void update_started()
{
	displayMessage(F("CALLBACK:  HTTP update process started"));
}

void update_finished()
{
	displayMessage(F("CALLBACK:  HTTP update process finished"));
	delay(1000);
	internalReboot(WARM_BOOT_MODE);
}

bool ota_update_init = true;

void update_progress(int cur, int total)
{
	statusLedToggle();
}

void update_error(int err)
{
	displayMessage(F("CALLBACK:  HTTP update fatal error code %d\n"), err);
	delay(1000);
	internalReboot(COLD_BOOT_MODE);
}

void performOTAUpdate()
{

	WiFiClient client;

	displayMessage(F("Boot OTA Update requested"));

	if (!syncWiFiConnect())
	{
		internalReboot(COLD_BOOT_MODE);
	}

	displayMessage(F("WiFi Connected"));

  initStatusLedHardware();
  statusLedOn();

#if defined(ARDUINO_ARCH_ESP8266)

	displayMessage(F("Performing OTA update from %s\n"), ESP8266_OTA_UPDATE_URL);

	//	ESPhttpUpdate.onStart(update_started);
	ESPhttpUpdate.onEnd(update_finished);
	ESPhttpUpdate.onProgress(update_progress);
	ESPhttpUpdate.onError(update_error);

	t_httpUpdate_return ret = ESPhttpUpdate.update(client, ESP8266_OTA_UPDATE_URL, Version);

	displayMessage(F("Update running.........."));

#endif

#if defined(ARDUINO_ARCH_ESP32)

	displayMessage(F("Performing OTA update from %s\n"), ESP32_OTA_UPDATE_URL);

	httpUpdate.onEnd(update_finished);
	httpUpdate.onError(update_error);
	httpUpdate.onProgress(update_progress);
	t_httpUpdate_return ret = httpUpdate.update(client, ESP32_OTA_UPDATE_URL);

#endif
}

void requestOTAUpate()
{
	displayMessage(F("OTA Update"));

	internalReboot(OTA_UPDATE_BOOT_MODE);
}
