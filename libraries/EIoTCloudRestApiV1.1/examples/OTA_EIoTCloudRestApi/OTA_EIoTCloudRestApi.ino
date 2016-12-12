#define _DEBUG
#ifdef _DEBUG
#define DEBUG_PRINTLN(x)  Serial.println(x)
#define DEBUG_PRINT(x)    Serial.print(x)
#else
#define DEBUG_PRINTLN(x) 
#define DEBUG_PRINT(x)
#endif

#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
//#include <WiFiUdp.h>
#include <ArduinoOTA.h>
#include <EEPROM.h>
#include <Vector.h>
//#include <EIoTCloudRestApiV1.0.h>
#include <EIoTCloudRestApiV1.1.h>

// +----------------------+---------+---------+
// | Library version      |  FLASH  |   SRAM  |
// +----------------------+---------+---------+
// | EIoTCloudRestApiV1.0 | 250,194 |  33,880 |
// | EIoTCloudRestApiV1.1 | 249,434 |  33,944 |
// +----------------------+---------+---------+

EIoTCloudRestApi eiotcloud;

const char* ssid = "AccessPoint";
const char* password = "";
const char* host = "OTA-EIoT";

#define INSTANCE_ID "57bccebdc943a0305d5aa4bb"
#define CONFIG_START 0
#define CONFIG_VERSION "v01"

struct eepromStorage {
	// This is for mere detection if they are your settings
	char version[4];
	// The variables of your settings
	char token[41];
	uint moduleId;
	char parameterId1[17];
	char parameterId2[17];
	//bool tokenOk; // valid token  
} storage = {
	CONFIG_VERSION,
	// token
	"1234567890123456789012345678901234567890",
	// The default module 0 - invalid module
	0,
	//0 // not valid
};

String moduleId = "";
String parameterId1 = "0";
String parameterId2 = "0";

WiFiServer TelnetServer(8266);

uint8_t _inc;
uint32_t _t0;
#define REPORT_INTERVAL 3000

bool EIoT_setup();
void EIoT_update(const uint8_t _inc = 0);

void setup()
{
	WiFi.hostname(host);
	//wifi_station_set_hostname(host);
	TelnetServer.begin();
#ifdef _DEBUG
	Serial.begin(115200);
#endif
	EEPROM.begin(512);
	loadConfig();
	printConfig();

	Serial.println("Booting");
	
	// try connecting to the access point
	if (eiotcloud.begin(ssid, password))
	{
		if (EIoT_setup() == false)
		{
			ESP.reset();
		}

		EIoT_update();

		ArduinoOTA.setHostname(host);
		ArduinoOTA.onStart([]() {});
		ArduinoOTA.onEnd([]() {});
		ArduinoOTA.onError([](ota_error_t error) { ESP.restart(); });

		/* setup the OTA server */
		ArduinoOTA.begin();
		Serial.println("Ready");

		_t0 = millis();
	}
	else
	{
		ESP.reset();
	}
}

void loop()
{
	ArduinoOTA.handle();
	yield();
	if (millis() - _t0 > REPORT_INTERVAL )
	{
		EIoT_update(_inc);
		_inc = (_inc == 9) ? 0 : _inc + 1;
		_t0 = millis();
	}
}

void EIoT_update(const uint8_t _inc)
{
	const float batt_volts = 5 + float(_inc) / 10.0F;
	const float tempC = 12 + float(_inc) / 10.0F;
	
	Vector<String> Id;
	Vector<String> Value;
	Id.push_back(parameterId1);
	Value.push_back(String(tempC));
	Id.push_back(parameterId2);
	Value.push_back(String(batt_volts));

	String updateString = "[{\"Id\": \"" + parameterId1 + "\", \"Value\": \"" + String(tempC) + "\" },{\"Id\": \"" + parameterId2 + "\", \"Value\": \"" + String(batt_volts) + "\" }]";
	DEBUG_PRINT("updateString : ");
	DEBUG_PRINTLN(updateString);

	eiotcloud.SetParameterValues(updateString);
	//eiotcloud.SetParameterValues(Id, Value);
}

bool EIoT_setup()
{
	// if first time get new token and register new module
	// here hapend Plug and play logic to add module to Cloud
	if (storage.moduleId == 0)  //*** set to != 0 to reset the moduleID and recreate in EasyIoT Cloud ***
	{
		// get new token - alternative is to manually create token and store it in EEPROM
		String token = eiotcloud.TokenNew(INSTANCE_ID);
		DEBUG_PRINT("Token: ");
		DEBUG_PRINTLN(token);
		eiotcloud.SetToken(token);
		
		// remember token
		token.toCharArray(storage.token, 41);
	
		// add new module and configure it
		moduleId = eiotcloud.ModuleNew();
		DEBUG_PRINT("ModuleId: ");
		DEBUG_PRINTLN(moduleId);
		storage.moduleId = moduleId.toInt();

		// set module type
		bool modtyperet = eiotcloud.SetModuleType(moduleId, "MT_GENERIC");
		DEBUG_PRINT("SetModulType: ");
		DEBUG_PRINTLN(modtyperet);

		// set module name
		bool modname = eiotcloud.SetModuleName(moduleId, "Temp and Voltage monitor");
		DEBUG_PRINT("SetModulName: ");
		DEBUG_PRINTLN(modname);

		// add image settings parameter
		String parameterImgId = eiotcloud.NewModuleParameter(moduleId, "Settings.Icon1");
		DEBUG_PRINT("parameterImgId: ");
		DEBUG_PRINTLN(parameterImgId);

		// set module image
		bool valueRet1 = eiotcloud.SetParameterValue(parameterImgId, "analog.png");
		DEBUG_PRINT("SetParameterValue: ");
		DEBUG_PRINTLN(valueRet1);

		// now add parameter to display temperature
		parameterId1 = eiotcloud.NewModuleParameter(moduleId, "Sensor.Parameter1");
		DEBUG_PRINT("parameterId1: ");
		DEBUG_PRINTLN(parameterId1);

		//set parameter description
		bool valueRet2 = eiotcloud.SetParameterDescription(storage.parameterId1, "Temperature");
		DEBUG_PRINT("SetParameterDescription: ");
		DEBUG_PRINTLN(valueRet2);

		//set unit
		// see http://meyerweb.com/eric/tools/dencoder/ how to encode �C 
		bool valueRet3 = eiotcloud.SetParameterUnit(storage.parameterId1, "%C2%B0C");
		DEBUG_PRINT("SetParameterUnit: ");
		DEBUG_PRINTLN(valueRet3);

		//Set parameter LogToDatabase
		bool valueRet4 = eiotcloud.SetParameterLogToDatabase(storage.parameterId1, true);
		DEBUG_PRINT("SetLogToDatabase: ");
		DEBUG_PRINTLN(valueRet4);

		//SetAvreageInterval
		bool valueRet5 = eiotcloud.SetParameterAverageInterval(storage.parameterId1, "5");
		DEBUG_PRINT("SetAvreageInterval: ");
		DEBUG_PRINTLN(valueRet5);

		//Second sensor parameter ---------------------------------------
		// now add parameter to display voltage
		parameterId2 = eiotcloud.NewModuleParameter(moduleId, "Sensor.Parameter2");
		DEBUG_PRINT("parameterId2: ");
		DEBUG_PRINTLN(parameterId2);

		//set parameter description
		bool valueRet6 = eiotcloud.SetParameterDescription(parameterId2, "Voltage");
		DEBUG_PRINT("SetParameterDescription: ");
		DEBUG_PRINTLN(valueRet6);

		//set unit
		bool valueRet7 = eiotcloud.SetParameterUnit(parameterId2, "V");
		DEBUG_PRINT("SetParameterUnit: ");
		DEBUG_PRINTLN(valueRet7);

		//Set parameter LogToDatabase
		bool valueRet8 = eiotcloud.SetParameterLogToDatabase(parameterId2, true);
		DEBUG_PRINT("SetLogToDatabase: ");
		DEBUG_PRINTLN(valueRet8);

		//SetAvreageInterval
		bool valueRet9 = eiotcloud.SetParameterAverageInterval(parameterId2, "5");
		DEBUG_PRINT("SetAvreageInterval: ");
		DEBUG_PRINTLN(valueRet9);
		
		// save configuration
		saveConfig();
	}

	// if something went wrong, wait here
	if (storage.moduleId == 0)
	{
		DEBUG_PRINT("Error : Cannot connect to Easy IoT cloud");
		return false;
	}

	// read module ID from storage
	moduleId = String(storage.moduleId);
	DEBUG_PRINT("ModuleID = ");
	DEBUG_PRINTLN(moduleId);

	// read token ID from storage
	eiotcloud.SetToken(storage.token);

	// read Sensor.Parameter1 ID from cloud
	parameterId1 = eiotcloud.GetModuleParameterByName(moduleId, "Sensor.Parameter1");
	parameterId2 = eiotcloud.GetModuleParameterByName(moduleId, "Sensor.Parameter2");

	return true;
}
void printConfig()
{
	const String version = String(storage.version);
	const String token = String(storage.token);
	//const String paramId1 = String(storage.parameterId1);
	//const String paramId2 = String(storage.parameterId2);

	DEBUG_PRINTLN("EEPROM storage");
	DEBUG_PRINT("version      : "); DEBUG_PRINTLN(version.c_str());
	DEBUG_PRINT("token        : "); DEBUG_PRINTLN(token.c_str());
	DEBUG_PRINT("moduleId     : "); DEBUG_PRINTLN(storage.moduleId);
	//DEBUG_PRINT("parameterId1 : "); DEBUG_PRINTLN(paramId1.c_str());
	//DEBUG_PRINT("parameterId2 : "); DEBUG_PRINTLN(paramId2.c_str());
}

void loadConfig()
{
	// To make sure there are settings, and they are YOURS!
	// If nothing is found it will use the default settings.
	if (EEPROM.read(CONFIG_START + 0) == CONFIG_VERSION[0] &&
		EEPROM.read(CONFIG_START + 1) == CONFIG_VERSION[1] &&
		EEPROM.read(CONFIG_START + 2) == CONFIG_VERSION[2])
	{
		for (unsigned int t = 0; t < sizeof(storage); t++)
		{
			*((char*)&storage + t) = EEPROM.read(CONFIG_START + t);
		}
	}

	moduleId = String(storage.moduleId);
	//parameterId1 = String(storage.parameterId1);
	//parameterId2 = String(storage.parameterId2);
}


void saveConfig()
{
	storage.moduleId = moduleId.toInt();
	//parameterId1.toCharArray(storage.parameterId1, 17);
	//parameterId2.toCharArray(storage.parameterId2, 17);

	for (unsigned int t = 0; t < sizeof(storage); t++)
	{
		EEPROM.write(CONFIG_START + t, *((char*)&storage + t));
	}
	EEPROM.commit();
}
