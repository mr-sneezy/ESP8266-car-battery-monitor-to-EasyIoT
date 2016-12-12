/*
Created by Igor Jarc
See http://iot-playground.com for details
Please use community forum on website do not contact author directly


This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
version 2 as published by the Free Software Foundation.

Changelog
----------------
10.02.2016 - first version
04.03.2016 - added SetParameterValues
22.08.2016 - set max number of wifi connection attempts inside wifiConnect().
Consolidate cpp code into the header so that EIoTCloudRestApiConfig.h is not required.
Code cleanup: @micooke
*/
#ifndef EIoTCloudRestApi_h
#define EIoTCloudRestApi_h

#include <ESP8266WiFi.h>

#ifdef _DEBUG
#define debug(x,...) Serial.print(x, ##__VA_ARGS__)
#define debugln(x,...) Serial.println(x, ##__VA_ARGS__)
#else
#define debug(x,...)
#define debugln(x,...)
#endif

#define EIOT_CLOUD_ADDRESS     "cloud.iot-playground.com"
#define EIOT_CLOUD_PORT        40404

//#define EIOT_CLOUD_ADDRESS     "192.168.88.110"
//#define EIOT_CLOUD_PORT        12345

class EIoTCloudRestApi
{
private:
	const char* _ssid;
	const char* _password;
	String		_token;
	uint8_t _wifi_max_attempts;
	uint32_t _wifi_interval;

public:
	EIoTCloudRestApi(const uint8_t wifi_max_attempts = 5, const uint32_t wifi_interval = 1000) : _wifi_max_attempts(wifi_max_attempts), _wifi_interval(wifi_interval) {}
	bool begin(const char* ssid, const char* password, String token);
	bool begin(const char* ssid, const char* password) { return begin(ssid, password, ""); }

	String TokenNew(String instance);
	bool TokenList(String instance, int *, String**);
	void SetToken(String);
	String GetToken();

	String ModuleNew();
	bool SetModuleType(String id, String moduleType);
	bool SetModuleName(String id, String name);
	String NewModuleParameter(String id);
	String NewModuleParameter(String id, String name);

	String GetModuleParameterByName(String id, String parameterName);

	bool SetParameterName(String parameterId, String name);
	String GetParameterName(String parameterId);
	bool SetParameterDescription(String parameterId, String description);
	String GetParameterDescription(String parameterId);
	bool SetParameterUnit(String parameterId, String unit);
	String GetParameterUnit(String parameterId);
	bool SetParameterUINotification(String parameterId, bool uiNotification);
	String GetParameterUINotification(String parameterId);
	bool SetParameterLogToDatabase(String parameterId, bool logToDatabase);
	String GetParameterLogToDatabase(String parameterId);
	bool SetParameterAverageInterval(String parameterId, String avgInterval);
	String GetParameterAverageInterval(String parameterId);
	bool SetParameterChartSteps(String parameterId, bool chartSteps);
	String GetParameterChartSteps(String parameterId);
	bool SetParameterValue(String parameterId, String value);
	String GetParameterValue(String parameterId);

	bool SetParameterValues(String values);

private:
	String parseId(WiFiClient* client);
	bool parseResponse(WiFiClient* client);
	bool setParameterProperty(String parameterId, String property, String value);
	bool setParameterProperty(String parameterId, String property, bool value);

	String getParameterProperty(String parameterId, String property);
	String parseParameter(WiFiClient* client, String property);

protected:
	bool wifiConnect();
};

bool EIoTCloudRestApi::begin(const char* ssid, const char* password, String token)
{
	_ssid = ssid;
	_password = password;
	_token = token;

	return wifiConnect();
}


String EIoTCloudRestApi::TokenNew(String instance)
{
	String ret = "";

	WiFiClient client;

	while (!client.connect(EIOT_CLOUD_ADDRESS, EIOT_CLOUD_PORT))
	{
		debug(F("connection failed"));
		wifiConnect();
	}

	debugln(F("\r\nGET /RestApi/v1.0/Token/New"));

	client.print(String("POST /RestApi/v1.0/Token/New HTTP/1.1\r\n") +
		"Host: " + String(EIOT_CLOUD_ADDRESS) + "\r\n" +
		"EIOT-Instance: " + String(instance) + "\r\n" +
		"Connection: close\r\n" +
		"Content-Length: 0\r\n" +
		"\r\n");

	while (!client.available());
	//delay(300);
	String line = "";
	while (client.available()) {
		line += client.readStringUntil('\r');
	}

	debug(line.c_str());

	int pos = 0;

	while ((pos = line.indexOf('{', pos)) != -1)
	{
		if (line.substring(pos, pos + 10) == "{\"Token\":\"")
		{
			int ix1 = line.indexOf("\"}", pos + 11);

			if (ix1 != -1)
			{
				ret = line.substring(pos + 10, ix1);
				debug(F("\r\n"));
				debugln(ret.c_str());
			}
			break;
		}
		else
			pos++;
	}

	_token = ret;

	return ret;
}

// list all tokens
bool EIoTCloudRestApi::TokenList(String instance, int *ptrTokenCnt, String** ptrArr)
{
	WiFiClient client;

	while (!client.connect(EIOT_CLOUD_ADDRESS, EIOT_CLOUD_PORT))
	{
		debug(F("connection failed"));
		wifiConnect();
	}

	debug(F("GET TokenList: "));

	client.print(String("GET /RestApi/v1.0/TokenList HTTP/1.1\r\n") +
		"Host: " + String(EIOT_CLOUD_ADDRESS) + "\r\n" +
		"EIOT-Instance: " + String(instance) + "\r\n" +
		"Connection: close\r\n" +
		"Content-Length: 0\r\n" +
		"\r\n");

	while (!client.available());
	//delay(300);
	String line = "";
	while (client.available())
	{
		line += client.readStringUntil('\r');
	}

	int ix = line.indexOf('[');

	if (ix != -1)
		line = line.substring(ix);

	int tokenCnt = 0;
	int pos = 0;

	while ((pos = line.indexOf('T', pos)) != -1)
	{
		if (line.substring(pos, pos + 8) == "Token\":\"")
		{
			tokenCnt++;
		}
		pos++;
	}
	debug(F("tokenCnt : "));
	debugln(tokenCnt);

	String tokens[tokenCnt];

	*ptrTokenCnt = tokenCnt;
	ptrArr = (String **)&tokens;

	pos = 0;
	int ix1;
	int i = 0;
	while ((pos = line.indexOf('T', pos)) != -1)
	{
		if (line.substring(pos, pos + 8) == "Token\":\"")
		{
			line = line.substring(pos + 8);
			ix1 = line.indexOf("\"}");

			debugln(line.substring(0, ix1));
			
			tokens[i] = line.substring(0, ix1);
			i++;
			pos = 0;
		}
		else
			pos++;
	}
	return true;
}

void EIoTCloudRestApi::SetToken(String token)
{
	_token = token;
}

String EIoTCloudRestApi::GetToken()
{
	return _token;
}

String EIoTCloudRestApi::ModuleNew()
{
	WiFiClient client;

	while (!client.connect(EIOT_CLOUD_ADDRESS, EIOT_CLOUD_PORT))
	{
		debug(F("connection failed"));
		wifiConnect();
	}

	debug(F("\r\nGET /RestApi/v1.0/Module/New\r\n"));

	client.print(String("POST /RestApi/v1.0/Module/New HTTP/1.1\r\n") +
		"Host: " + String(EIOT_CLOUD_ADDRESS) + "\r\n" +
		"EIOT-AuthToken: " + String(_token) + "\r\n" +
		"Connection: close\r\n" +
		"Content-Length: 0\r\n" +
		"\r\n");

	return parseId(&client);
}

bool EIoTCloudRestApi::SetModuleType(String id, String moduleType)
{
	WiFiClient client;

	while (!client.connect(EIOT_CLOUD_ADDRESS, EIOT_CLOUD_PORT))
	{
		debug(F("connection failed"));
		wifiConnect();
	}

	String url = "POST /RestApi/v1.0/Module/" + id + "/Type/" + moduleType;
	debug(F("\r\n"));
	debugln(url.c_str());

	client.print(String(url + " HTTP/1.1\r\n") +
		"Host: " + String(EIOT_CLOUD_ADDRESS) + "\r\n" +
		"EIOT-AuthToken: " + String(_token) + "\r\n" +
		"Connection: close\r\n" +
		"Content-Length: 0\r\n" +
		"\r\n");

	return parseResponse(&client);
}

bool EIoTCloudRestApi::SetModuleName(String id, String name)
{
	WiFiClient client;

	while (!client.connect(EIOT_CLOUD_ADDRESS, EIOT_CLOUD_PORT))
	{
		debug(F("connection failed"));
		wifiConnect();
	}

	name.replace(" ", "%20");

	String url = "POST /RestApi/v1.0/Module/" + id + "/Name/" + name;
	debug(F("\r\n"));
	debugln(url.c_str());

	client.print(String(url + " HTTP/1.1\r\n") +
		"Host: " + String(EIOT_CLOUD_ADDRESS) + "\r\n" +
		"EIOT-AuthToken: " + String(_token) + "\r\n" +
		"Connection: close\r\n" +
		"Content-Length: 0\r\n" +
		"\r\n");

	return parseResponse(&client);
}

String EIoTCloudRestApi::NewModuleParameter(String id)
{
	WiFiClient client;

	while (!client.connect(EIOT_CLOUD_ADDRESS, EIOT_CLOUD_PORT))
	{
		debug(F("connection failed"));
		wifiConnect();
	}

	String url = "POST /RestApi/v1.0/Module/" + id + "/NewParameter";
	debug(F("\r\n"));
	debugln(url.c_str());

	client.print(String(url + " HTTP/1.1\r\n") +
		"Host: " + String(EIOT_CLOUD_ADDRESS) + "\r\n" +
		"EIOT-AuthToken: " + String(_token) + "\r\n" +
		"Connection: close\r\n" +
		"Content-Length: 0\r\n" +
		"\r\n");

	return parseId(&client);
}

String EIoTCloudRestApi::NewModuleParameter(String id, String name)
{
	WiFiClient client;

	while (!client.connect(EIOT_CLOUD_ADDRESS, EIOT_CLOUD_PORT))
	{
		debug(F("connection failed"));
		wifiConnect();
	}

	String url = "POST /RestApi/v1.0/Module/" + id + "/NewParameter/" + name;
	debug(F("\r\n"));
	debugln(url.c_str());

	client.print(String(url + " HTTP/1.1\r\n") +
		"Host: " + String(EIOT_CLOUD_ADDRESS) + "\r\n" +
		"EIOT-AuthToken: " + String(_token) + "\r\n" +
		"Connection: close\r\n" +
		"Content-Length: 0\r\n" +
		"\r\n");

	return parseId(&client);
}

String EIoTCloudRestApi::GetModuleParameterByName(String id, String parameterName)
{
	WiFiClient client;

	while (!client.connect(EIOT_CLOUD_ADDRESS, EIOT_CLOUD_PORT))
	{
		debug(F("connection failed"));
		wifiConnect();
	}

	String url = "GET /RestApi/v1.0/Module/" + id + "/ParameterByName/" + parameterName;
	debug(F("\r\n"));
	debugln(url.c_str());

	client.print(String(url + " HTTP/1.1\r\n") +
		"Host: " + String(EIOT_CLOUD_ADDRESS) + "\r\n" +
		"EIOT-AuthToken: " + String(_token) + "\r\n" +
		"Connection: close\r\n" +
		"Content-Length: 0\r\n" +
		"\r\n");

	return parseId(&client);
}

bool EIoTCloudRestApi::SetParameterValue(String parameterId, String value)
{
	return setParameterProperty(parameterId, "Value", value);
}

String EIoTCloudRestApi::GetParameterValue(String parameterId)
{
	return getParameterProperty(parameterId, "Value");
}

bool EIoTCloudRestApi::SetParameterName(String parameterId, String name)
{
	name.replace(" ", "%20");
	return setParameterProperty(parameterId, "Name", name);
}

String EIoTCloudRestApi::GetParameterName(String parameterId)
{
	String name = getParameterProperty(parameterId, "Name");

	name.replace("%20", " ");
	return name;
}

bool EIoTCloudRestApi::SetParameterDescription(String parameterId, String description)
{
	description.replace(" ", "%20");
	return setParameterProperty(parameterId, "Description", description);
}

String EIoTCloudRestApi::GetParameterDescription(String parameterId)
{
	String description = getParameterProperty(parameterId, "Description");
	description.replace("%20", " ");
	return description;
}

bool EIoTCloudRestApi::SetParameterUnit(String parameterId, String unit)
{
	return setParameterProperty(parameterId, "Unit", unit);
}

String EIoTCloudRestApi::GetParameterUnit(String parameterId)
{
	return getParameterProperty(parameterId, "Unit");
}

bool EIoTCloudRestApi::SetParameterUINotification(String parameterId, bool uiNotification)
{
	return setParameterProperty(parameterId, "UINotification", uiNotification);
}

String EIoTCloudRestApi::GetParameterUINotification(String parameterId)
{
	return getParameterProperty(parameterId, "UINotification");
}

bool EIoTCloudRestApi::SetParameterLogToDatabase(String parameterId, bool logToDatabase)
{
	return setParameterProperty(parameterId, "LogToDatabase", logToDatabase);
}

String EIoTCloudRestApi::GetParameterLogToDatabase(String parameterId)
{
	return getParameterProperty(parameterId, "LogToDatabase");
}


bool EIoTCloudRestApi::SetParameterAverageInterval(String parameterId, String avgInterval)
{
	return setParameterProperty(parameterId, "AverageInterval", avgInterval);
}

String EIoTCloudRestApi::GetParameterAverageInterval(String parameterId)
{
	return getParameterProperty(parameterId, "AverageInterval");
}

bool EIoTCloudRestApi::SetParameterChartSteps(String parameterId, bool chartSteps)
{
	return setParameterProperty(parameterId, "ChartSteps", chartSteps);
}

String EIoTCloudRestApi::GetParameterChartSteps(String parameterId)
{
	return getParameterProperty(parameterId, "ChartSteps");
}

String EIoTCloudRestApi::getParameterProperty(String parameterId, String property)
{

	WiFiClient client;

	while (!client.connect(EIOT_CLOUD_ADDRESS, EIOT_CLOUD_PORT))
	{
		debug(F("connection failed"));
		wifiConnect();
	}


	String url = "GET /RestApi/v1.0/Parameter/" + parameterId + "/" + property;
	debug(F("\r\n"));
	debugln(url.c_str());

	client.print(String(url + " HTTP/1.1\r\n") +
		"Host: " + String(EIOT_CLOUD_ADDRESS) + "\r\n" +
		"EIOT-AuthToken: " + String(_token) + "\r\n" +
		"Connection: close\r\n" +
		"Content-Length: 0\r\n" +
		"\r\n");

	return parseParameter(&client, property);
}

String EIoTCloudRestApi::parseParameter(WiFiClient* client, String property)
{
	String ret = "";

	while (!client->available());
	//delay(300);
	String line = "";
	while (client->available())
	{
		line += client->readStringUntil('\r');
	}

	debug(line.c_str());

	int pos = 0;
	int len = line.length();
	int lenProp = property.length();


	//#ifdef _DEBUG  
	//  debug(F("\r\nlen:\r\n"));
	//  String(len).toCharArray(buff, 300);
	//  debug(buff);
	//
	//  debug(F("\r\nlenProp:\r\n"));
	//  String(lenProp).toCharArray(buff, 300);
	//  debug(buff);
	//#endif

	String propStr = "\"" + property + "\":\"";
	//#ifdef _DEBUG  
	//  debug(F("\r\npropStr:\r\n"));
	//  propStr.toCharArray(buff, 300);
	//  debug(buff);
	//#endif

	int pos1 = 0;

	while (pos < len)
	{
		if (line.substring(pos, pos + 4 + lenProp) == propStr)
		{
			pos = pos + 4 + lenProp;
			pos1 = pos;

			while (pos < len)
			{
				if ((pos = line.indexOf('\"', pos)) != -1)
				{
					//#ifdef _DEBUG  
					//  debug(F("\r\nPos 2:\r\n"));
					//  String(pos).toCharArray(buff, 300);
					//  debug(buff);
					//#endif

					ret = line.substring(pos1, pos);
					debug(F("\r\n"));
					debugln(ret.c_str());
					break;
				}
				else
					break;
			}
			break;
		}
		else
			pos++;
	}

	return ret;
}

bool EIoTCloudRestApi::SetParameterValues(String values)
{
	WiFiClient client;

	while (!client.connect(EIOT_CLOUD_ADDRESS, EIOT_CLOUD_PORT))
	{
		debug(F("connection failed"));
		wifiConnect();
	}

	String url = "POST /RestApi/v1.0/Parameter/Values";
	debug(F("\r\n"));
	debugln(url.c_str());

	client.print(String(url + " HTTP/1.1\r\n") +
		"Host: " + String(EIOT_CLOUD_ADDRESS) + "\r\n" +
		"EIOT-AuthToken: " + String(_token) + "\r\n" +
		"Connection: close\r\n" +
		"Content-Length: " + values.length() +
		"\n\n" +
		values +
		"\r\n");

	return parseResponse(&client);
}

bool EIoTCloudRestApi::setParameterProperty(String parameterId, String property, String value)
{
	WiFiClient client;

	while (!client.connect(EIOT_CLOUD_ADDRESS, EIOT_CLOUD_PORT))
	{
		debug(F("connection failed"));
		wifiConnect();
	}

	String url = "POST /RestApi/v1.0/Parameter/" + parameterId + "/" + property + "/" + value;
	debug(F("\r\n"));
	debugln(url.c_str());

	client.print(String(url + " HTTP/1.1\r\n") +
		"Host: " + String(EIOT_CLOUD_ADDRESS) + "\r\n" +
		"EIOT-AuthToken: " + String(_token) + "\r\n" +
		"Connection: close\r\n" +
		"Content-Length: 0\r\n" +
		"\r\n");

	return parseResponse(&client);
}

bool EIoTCloudRestApi::setParameterProperty(String parameterId, String property, bool value)
{
	WiFiClient client;

	while (!client.connect(EIOT_CLOUD_ADDRESS, EIOT_CLOUD_PORT))
	{
		debug(F("connection failed"));
		wifiConnect();
	}

	String url = "POST /RestApi/v1.0/Parameter/" + parameterId + "/" + property + "/";

	url += (value) ? "true" : "false";

	debug(F("\r\n"));
	debugln(url.c_str());

	client.print(String(url + " HTTP/1.1\r\n") +
		"Host: " + String(EIOT_CLOUD_ADDRESS) + "\r\n" +
		"EIOT-AuthToken: " + String(_token) + "\r\n" +
		"Connection: close\r\n" +
		"Content-Length: 0\r\n" +
		"\r\n");

	return parseResponse(&client);
}

String EIoTCloudRestApi::parseId(WiFiClient* client)
{
	String ret = "";

	while (!client->available());
	//delay(300);
	String line = "";
	while (client->available())
	{
		line += client->readStringUntil('\r');
	}

	debug(line.c_str());

	int pos = 0;

	while ((pos = line.indexOf('{', pos)) != -1)
	{
		if (line.substring(pos, pos + 7) == "{\"Id\":\"")
		{
			int ix1 = line.indexOf("\"}", pos + 8);

			if (ix1 != -1)
			{
				ret = line.substring(pos + 7, ix1);
				debug(F("\r\n"));
				debugln(ret.c_str());
			}
			break;
		}
		else
			pos++;
	}

	return ret;
}

bool EIoTCloudRestApi::parseResponse(WiFiClient* client)
{
	while (!client->available());
	//delay(300);
	String line = "";
	while (client->available())
	{
		line += client->readStringUntil('\r');
	}

	debug(line.c_str());

	int pos = 0;

	if ((pos = line.indexOf('{', pos)) != -1)
	{
		if (line.substring(pos, pos + 16) == "{\"Response\":\"0\"}")
		{
			return true;
		}
	}

	return false;
}

bool EIoTCloudRestApi::wifiConnect()
{
	uint8_t connection_attempt_ = 0;
	debug(F("Connecting to AP"));
	WiFi.begin(_ssid, _password);

	while ((WiFi.status() != WL_CONNECTED) & (connection_attempt_++ < _wifi_max_attempts))
	{
		delay(_wifi_interval);
		debug(".");
	}

	if (WiFi.status() == WL_CONNECTED)
	{
		debugln(F("\nWiFi connection succeeded"));
	}
	else
	{
		debugln(F("\nWiFi connection failed"));
	}

	return (WiFi.status() == WL_CONNECTED);
}

#endif