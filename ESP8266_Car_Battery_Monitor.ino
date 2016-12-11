/*
SNEEZY BATTERY MONITOR OVER WIFI - USES ESP8266 BREAKOUT MODULE BOARDS VIA ARDUINO IDE 
VERSION 1A - Nov 2016
*/


#include <Vector.h>
#include <ESP8266WiFi.h>
#include "EIoTCloudRestApiV1.1.h"
#include <EEPROM.h>
#include <OneWire.h>
#include <DallasTemperature.h>

//#define DEBUG_PROG //Remark to disable serial DEBUG
/*
#ifdef DEBUG_PROG
  #define DEBUG_PRINTLN(x)  Serial.println(x)
  #define DEBUG_PRINT(x)    Serial.print(x)
#else
  #define DEBUG_PRINTLN(x) 
  #define DEBUG_PRINT(x)
#endif
*/
#ifdef DEBUG_PROG  //cool way to have debug serial ONLY when you want it, and replaced with effectively no statement when not needed. Supports basic formatting of vars.
#define DEBUG_PRINT(x,...) Serial.print(x, ##__VA_ARGS__)
#define DEBUG_PRINTLN(x,...) Serial.println(x, ##__VA_ARGS__)
#else
#define DEBUG_PRINT(x,...)
#define DEBUG_PRINTLN(x,...)
#endif

EIoTCloudRestApi eiotcloud;

//#define AP 1  //AP 1 = hotspot  //=home router

#ifdef AP 1
// change those lines
  #define AP_USERNAME "YOUR OWN WIFI NAME"
  #define AP_PASSWORD "YOUR PASSWORD"
  #define INSTANCE_ID "your cloud instance ID"
#else
// change those lines
  #define AP_USERNAME "YOUR OWN ALTERNATE WIFI NAME"
  #define AP_PASSWORD "YOUR ALTERNATE PASSWORD"
  #define INSTANCE_ID "your Eiot cloud instance ID number"
#endif

#define REPORT_INTERVAL 360   //Seconds before next report (multiplied by sleep_multiplier if not 0)
#define sleep_multiplier 10  //deep sleep period multiplier.

#define ONE_WIRE_BUS 2  // DS18B20 pin
OneWire oneWire(ONE_WIRE_BUS);
//DallasTemperature sensors(&oneWire);
DallasTemperature DS18B20(&oneWire);

#define ledPIN 2	// Also the serial data TX pin

#define CONFIG_START 0
#define CONFIG_VERSION "v01"

struct StoreStruct {
  // This is for mere detection if they are your settings
  char version[4];
  // The variables of your settings
  char token[41];
  uint moduleId;
  //bool tokenOk; // valid token  
} storage = {
  CONFIG_VERSION,
  // token
  "1234567890123456789012345678901234567890",
  // The default module 0 - invalid module
  0,
  //0 // not valid
};

uint32_t rtcData = 0;	//four byte bucket of RTC memory in position 0
uint8_t rtcByte0 = 0;    //var for first byte of that bucket

//float batt_voltsOLD;
//float tempOLD;

String moduleId = "";
String parameterId1 = "";
String parameterId2 = "";
float tempOld = 0;

//Function prototype references - C++ housekeeping for compiler
void Go_sleep();
void Multi_sleeps();
void loadConfig();
void saveConfig();

void setup() {
	
	pinMode(ledPIN, OUTPUT);
	digitalWrite(ledPIN, HIGH);  //LED off
    Serial.begin(115200);
    DEBUG_PRINTLN("Start...");

	Multi_sleeps();	//Check the multiple deep sleep cycle count 
	
	digitalWrite(ledPIN, LOW); //long LED flash to indicate a full code run through coming.
	delay(1000);  //Chucking in a short delay for the RF module to stabilise in case that's causing some lockups...
    
	EEPROM.begin(512);
    loadConfig();

//Dallas sensor addition by Sneezy
      DS18B20.begin();
      DS18B20.setResolution(9);
      DS18B20.setWaitForConversion(true);
//-------------------
        
    DEBUG_PRINTLN("Setup running");
	
//    eiotcloud.begin(AP_USERNAME, AP_PASSWORD);

    if (eiotcloud.begin(AP_USERNAME, AP_PASSWORD) == false)
{
    DEBUG_PRINTLN("WiFi not connected");    //MCS
	ESP.deepSleep(REPORT_INTERVAL * 1000000, WAKE_RF_DEFAULT); //go to deep sleep for the regular report interval (not multiplied)
	delay(1000); //Deep sleep might take some time to occur
}
    // if first time get new token and register new module
    // here hapend Plug and play logic to add module to Cloud
    if (storage.moduleId == 0)  //*** REMARK OUT ONCE to reset the moduleID and recreate in EasyIoT Cloud ***
    {
      // get new token - alternarive is to manually create token and store it in EEPROM
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
      DEBUG_PRINT("SetModuleType: ");
      DEBUG_PRINTLN(modtyperet);
      
      // set module name
      bool modname = eiotcloud.SetModuleName(moduleId, "S2000 monitor");
      DEBUG_PRINT("SetModuleName: ");
      DEBUG_PRINTLN(modname);

      // add image settings parameter
      String parameterImgId = eiotcloud.NewModuleParameter(moduleId, "Settings.Icon1");
      DEBUG_PRINT("parameterImgId: ");
      DEBUG_PRINTLN(parameterImgId);

      // set module image
      bool valueRet1 = eiotcloud.SetParameterValue(parameterImgId, "analog.png");
      DEBUG_PRINT("SetParameterValue: ");
      DEBUG_PRINTLN(valueRet1);
      
      // now add parameter to display battery voltage
      parameterId1 = eiotcloud.NewModuleParameter(moduleId, "Sensor.Parameter1");
      DEBUG_PRINT("parameterId1: ");
      DEBUG_PRINTLN(parameterId1);

      //set parameter description
      bool valueRet2 = eiotcloud.SetParameterDescription(parameterId1, "Voltage");
      DEBUG_PRINT("SetParameterDescription: ");
      DEBUG_PRINTLN(valueRet2);
      
      //set unit
      bool valueRet3 = eiotcloud.SetParameterUnit(parameterId1, "V");
      DEBUG_PRINT("SetParameterUnit: ");
      DEBUG_PRINTLN(valueRet3);

      //Set parameter LogToDatabase
      bool valueRet4 = eiotcloud.SetParameterLogToDatabase(parameterId1, true);
      DEBUG_PRINT("SetLogToDatabase: ");
      DEBUG_PRINTLN(valueRet4);

      //SetAvreageInterval
      bool valueRet5 = eiotcloud.SetParameterAverageInterval(parameterId1, "5");
      DEBUG_PRINT("SetAvreageInterval: ");
      DEBUG_PRINTLN(valueRet5);
      
      //Second sensor parameter ---------------------------------------
       // now add parameter to display temperature (if DS18B20 installed).
      parameterId2 = eiotcloud.NewModuleParameter(moduleId, "Sensor.Parameter2");
      DEBUG_PRINT("parameterId2: ");
      DEBUG_PRINTLN(parameterId2);

      //set parameter description
      bool valueRet6 = eiotcloud.SetParameterDescription(parameterId2, "Temperature");
      DEBUG_PRINT("SetParameterDescription: ");
      DEBUG_PRINTLN(valueRet2);
      
      //set unit
      // see http://meyerweb.com/eric/tools/dencoder/ how to encode °C 
	  bool valueRet7 = eiotcloud.SetParameterUnit(parameterId2, "%C2%B0C");
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

    // if something went wrong, wiat here
    if (storage.moduleId == 0)
      delay(1);

    // read module ID from storage
    moduleId = String(storage.moduleId);
    DEBUG_PRINT("ModuleID = ");
    DEBUG_PRINTLN(moduleId);
    // read token ID from storage
    eiotcloud.SetToken(storage.token);
        
    // read Sensor.Parameter1 ID from cloud
    parameterId1 = eiotcloud.GetModuleParameterByName(moduleId, "Sensor.Parameter1");
    DEBUG_PRINT("parameterId1: ");
    DEBUG_PRINTLN(parameterId1);

    parameterId2 = eiotcloud.GetModuleParameterByName(moduleId, "Sensor.Parameter2");
    DEBUG_PRINT("parameterId2: ");
    DEBUG_PRINTLN(parameterId2);

}


void loop() {
  float tempC;
  float batt_volts;
  float temp_var=0;     //holds cumulative reads for averaging
  unsigned int batt_raw;   // can I use 'int' or does it need to be a float to do maths with another float below ? (yep seems to work fine with float math)
    DEBUG_PRINTLN("Loop running");
  
//Read voltage with 10 times average.
   for (int i=0; i <= 9; i++){
   temp_var = temp_var + analogRead(A0);
   delay(10);
   } 

  batt_raw = temp_var/10;
   
  batt_volts = batt_raw * 0.0146; //scalling factor for 15V full scale with ADC range of 1V 
  
  DEBUG_PRINT("Battery volts: ");
  DEBUG_PRINTLN(batt_volts);  

//Read DS18B20 temperature with 10 times average.
  temp_var = 0;
   for (int i=0; i <= 9; i++){
   DS18B20.requestTemperatures();
//   delay(100);                    //Delay while sensor does temp sample and conversion in 9 bit resolution (could substitute Wait For Conversion if I understood how too).
   temp_var = temp_var + DS18B20.getTempCByIndex(0);   

   } 
  tempC = temp_var/10;    
 
    DEBUG_PRINT("Temperature: ");
    DEBUG_PRINTLN(tempC);

      eiotcloud.SetParameterValues("[{\"Id\": \""+parameterId1+"\", \"Value\": \""+String(batt_volts)+"\" },{\"Id\": \""+parameterId2+"\", \"Value\": \""+String(tempC)+"\" }]");

  Go_sleep();
}

void loadConfig() {
    DEBUG_PRINTLN("do loadConfig");
  // To make sure there are settings, and they are YOURS!
  // If nothing is found it will use the default settings.
  if (EEPROM.read(CONFIG_START + 0) == CONFIG_VERSION[0] &&
      EEPROM.read(CONFIG_START + 1) == CONFIG_VERSION[1] &&
      EEPROM.read(CONFIG_START + 2) == CONFIG_VERSION[2])
    for (unsigned int t=0; t<sizeof(storage); t++)
      *((char*)&storage + t) = EEPROM.read(CONFIG_START + t);
}

void saveConfig() {
    DEBUG_PRINTLN("do saveConfig");
  for (unsigned int t=0; t<sizeof(storage); t++)
    EEPROM.write(CONFIG_START + t, *((char*)&storage + t));

  EEPROM.commit();
}

void Multi_sleeps () {
//read the RTC static ram data
  ESP.rtcUserMemoryRead(0, (uint32_t*) &rtcData, sizeof(rtcData));       //read the variable out of RTC memory
 if((rtcData & 0xFF000000) !=  0xAC000000) // upper byte set check
{
   DEBUG_PRINTLN("this is the first time data is to be written!");
    rtcData = 0xAC000000; //+ rtcByte0;   //Upper byte set to 0xAC and lower (rtcByte0) is set to 0 initially also
     DEBUG_PRINT("rtcData_set_init: ");
     DEBUG_PRINTLN(rtcData,HEX);
     // write the new RTC static ram data
  ESP.rtcUserMemoryWrite(0, (uint32_t*) &rtcData, sizeof(rtcData));
//  delay(500); // allow the serial data to be pushed over ttl before reset
}
  else
{
       ++rtcData; // increment rtcData
}
  byte multi_sleep_count = 0;
  multi_sleep_count = rtcData & 0x000000FF; //isolate the current sleep cycle count
  if((multi_sleep_count > sleep_multiplier) || (multi_sleep_count == 1)) {        //if var 'sleep_multiplier' number of deep sleep cycles have passed run the whole program
      DEBUG_PRINTLN("Sleep_counter triggered");
      rtcData = 0xAC000000 + 1;  //Reset the sleep counter (+1 so an endless loop of triggering doesn't happen)
       ESP.rtcUserMemoryWrite(0, (uint32_t*) &rtcData, sizeof(rtcData));
        DEBUG_PRINT("rtcData_reset: ");
        DEBUG_PRINTLN(rtcData, HEX);
//        delay(500); // allow the serial data to be pushed over ttl before reset
      return;      //then go back to the Setup function and continue
}
  else
{
        ESP.rtcUserMemoryWrite(0, (uint32_t*) &rtcData, sizeof(rtcData));
        DEBUG_PRINT("rtcData_incremented: ");
        DEBUG_PRINTLN(rtcData, HEX);
//        delay(500); // allow the serial data to be pushed over ttl before reset
    Go_sleep();    //go back to sleep again
}
  }

void Go_sleep() {
//	uint16_t temp_var = 0
  float cnt = REPORT_INTERVAL * 1000000;  //Delay in units of second
      DEBUG_PRINT("Sleeping for ");
	  DEBUG_PRINT((REPORT_INTERVAL * sleep_multiplier) - (REPORT_INTERVAL * ((rtcData & 0x000000FF) - 1)));
	  DEBUG_PRINTLN(" seconds");
	  
  ESP.deepSleep(cnt, WAKE_RF_DEFAULT);
  delay(1000); //Deep sleep might take some time to occur
}

