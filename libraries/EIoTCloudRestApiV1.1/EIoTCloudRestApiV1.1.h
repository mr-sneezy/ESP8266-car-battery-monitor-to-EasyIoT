/*
Created by Igor Jarc
See http://iot-playground.com for details
Please use community forum on website do not contact author directly


This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
version 2 as published by the Free Software Foundation.

Changelog
----------------
10.02.2016 - @iot-playground : first version
04.03.2016 - @iot-playground : added SetParameterValues
22.08.2016 - @micooke : set max number of wifi connection attempts inside wifiConnect()
             Consolidated cpp code into the header. Modified to use _DEBUG define (MSVC++ standard) instead of DEBUG
31.08.2016 - @micooke : Consolidated all API calls into two main functions. 
             Changed TokenList to use the Vector class (https://github.com/zacsketches/Arduino_Vector)
			 Added SetParameterValues(Vector<String> &Id, Vector<String> &Value)
*/
#ifndef EIoTCloudRestApi_h
#define EIoTCloudRestApi_h

#include <Vector.h>
#include <ESP8266WiFi.h>

#ifdef _DEBUG
#define debug(x,...) Serial.print(x, ##__VA_ARGS__)
#define debugln(x,...) Serial.println(x, ##__VA_ARGS__)
#else
#define debug(x,...)
#define debugln(x,...)
#endif

//#define EIOT_CLOUD_ADDRESS "cloud.iot-playground.com" //"192.168.88.110" 
//#define EIOT_CLOUD_PORT                         40404 // 12345

String REST_API_INSTANCE = String("%GET_POST% /RestApi/v1.0/%COMMAND% HTTP/1.1\r\n") +
"Host: %EIOT_ADDRESS%\r\n" +
"EIOT-Instance: %INSTANCE%\r\n" +
"Connection: close\r\n" +
"Content-Length: 0\r\n\r\n";

String REST_API_AUTH_TOKEN = String("%GET_POST% /RestApi/v1.0/%TYPE%/%COMMAND% HTTP/1.1\r\n") +
"Host: %EIOT_ADDRESS%\r\n" +
"EIOT-AuthToken: %TOKEN%\r\n" +
"Connection: close\r\n" +
"Content-Length: %LENGTH%\r\n" +
"%CONTENT%\r\n";

class EIoTCloudRestApi
{
private:
	String _ssid;
	String _password;
	String		_token;
	uint8_t _wifi_max_attempts;
	uint32_t _wifi_interval;
	String _EIoT_Address;
	uint16_t _EIoT_Port;

public:
	EIoTCloudRestApi(String _EIoT_Address = "cloud.iot-playground.com", uint16_t _EIoT_Port = 40404, const uint8_t wifi_max_attempts = 10, const uint32_t wifi_interval = 2000) : _EIoT_Address(_EIoT_Address), _EIoT_Port(_EIoT_Port), _wifi_max_attempts(wifi_max_attempts), _wifi_interval(wifi_interval) {}

	bool begin(String ssid, String password, String token);
	bool begin(String ssid, String password) { return begin(ssid, password, ""); }

	String TokenNew(String instance);
	bool TokenList(String instance, Vector<String> tokens);
	void SetToken(String token) { _token = token; }
	String GetToken() { return _token; }

	String ModuleNew();
	bool SetModuleType(String id, String moduleType);
	bool SetModuleName(String id, String name);
	String NewModuleParameter(String id);
	String NewModuleParameter(String id, String name);

	String GetModuleParameterByName(String id, String parameterName);

	bool SetParameterName(String id, String name);
	String GetParameterName(String id);

	bool SetParameterDescription(String id, String description);
	String GetParameterDescription(String id);
	
	bool SetParameterUnit(String id, String unit) { return setParameterProperty(id, "Unit", unit); }
	String GetParameterUnit(String id) { return getParameterProperty(id, "Unit"); }
	
	bool SetParameterUINotification(String id, bool uiNotification) { return setParameterProperty(id, "UINotification", uiNotification); }
	String GetParameterUINotification(String id) { return getParameterProperty(id, "UINotification"); }
	
	bool SetParameterLogToDatabase(String id, bool logToDatabase) { return setParameterProperty(id, "LogToDatabase", logToDatabase); }
	String GetParameterLogToDatabase(String id) { return getParameterProperty(id, "LogToDatabase"); }
	
	bool SetParameterAverageInterval(String id, String avgInterval) { return setParameterProperty(id, "AverageInterval", avgInterval); }
	String GetParameterAverageInterval(String id) { return getParameterProperty(id, "AverageInterval"); }
	
	bool SetParameterChartSteps(String id, bool chartSteps) { return setParameterProperty(id, "ChartSteps", chartSteps); }
	String GetParameterChartSteps(String id) { return getParameterProperty(id, "ChartSteps"); }
	
	bool SetParameterValue(String id, String value) { return setParameterProperty(id, "Value", value); }
	String GetParameterValue(String id) { return getParameterProperty(id, "Value"); }

	bool SetParameterValues(String values);
	bool SetParameterValues(Vector<String> &Id, Vector<String> &Value);

private:
	String parseId(String response);
	bool parseResponse(String response);
	String parseParameter(String response, String property);

	String getParameterProperty(String id, String property);

	bool setParameterProperty(String id, String property, String value);
	bool setParameterProperty(String id, String property, bool value);

	String RestApi_Instance(String getORpost, String command, String Instance);
	String RestApi_AuthToken(String getORpost, String id, String command, String token, String type = "Module", String content = "");

protected:
	bool wifiConnect();
};

bool EIoTCloudRestApi::begin(String ssid, String password, String token)
{
	_ssid = ssid;
	_password = password;
	_token = token;

	return wifiConnect();
}

bool EIoTCloudRestApi::wifiConnect()
{
	uint8_t connection_attempt_ = 0;
	debug(F("Connecting to AP"));
	WiFi.begin(_ssid.c_str(), _password.c_str());

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

bool EIoTCloudRestApi::SetParameterName(String id, String name)
{
	name.replace(" ", "%20");
	return setParameterProperty(id, "Name", name);
}

String EIoTCloudRestApi::GetParameterName(String id)
{
	String name = getParameterProperty(id, "Name");

	name.replace("%20", " ");
	return name;
}

bool EIoTCloudRestApi::SetParameterDescription(String id, String description)
{
	description.replace(" ", "%20");
	return setParameterProperty(id, "Description", description);
}

String EIoTCloudRestApi::GetParameterDescription(String id)
{
	String description = getParameterProperty(id, "Description");
	description.replace("%20", " ");
	return description;
}

// API calls
String EIoTCloudRestApi::RestApi_Instance(String getORpost, String command, String instance)
{
	WiFiClient client;

	uint8_t connection_attempts = 0;
	while (!client.connect(_EIoT_Address.c_str(), _EIoT_Port))
	{
		debugln(F("connection failed"));
		wifiConnect();
		
		if (++connection_attempts > _wifi_max_attempts)
		{
			debugln("RestApi_Instance : Maximum client connection attempts reached.\nReturning to main program");
			return "";
		}
	}

	// Generate the RestApi command
	String cmd = REST_API_INSTANCE;
	cmd.replace("%GET_POST%", getORpost); 
	cmd.replace("%COMMAND%", command);
	cmd.replace("%EIOT_ADDRESS%", _EIoT_Address); 
	cmd.replace("%INSTANCE%", instance);

	// Send the RestApi command
	client.print(cmd);
	debug(String(getORpost + " /RestApi/v1.0/ " + command + " : ").c_str());

	// Get the RestApi response
	while (!client.available());

	String response = "";
	while (client.available())
	{
		response += client.readStringUntil('\r');
	}

	return response;
}

String EIoTCloudRestApi::RestApi_AuthToken(String getORpost, String id, String command, String token, String type, String content)
{
	WiFiClient client;

	uint8_t connection_attempts = 0;
	while (!client.connect(_EIoT_Address.c_str(), _EIoT_Port))
	{
		debugln(F("connection failed"));
		wifiConnect();
		
		if (++connection_attempts > _wifi_max_attempts)
		{
			debugln("RestApi_AuthToken : Maximum client connection attempts reached.\nReturning to main program");
			return "";
		}
	}

	// Generate the RestApi command
	String cmd = REST_API_AUTH_TOKEN;
	cmd.replace("%GET_POST%", getORpost);
	cmd.replace("%TYPE%", type);

	if (id.length() == 0)
	{
		cmd.replace("%COMMAND%", command);
		debug(String(getORpost + " /RestApi/v1.0/" + type + "/" + command + " : ").c_str());
	}
	else
	{
		cmd.replace("%COMMAND%", id + "/" + command);
		debug(String(getORpost + " /RestApi/v1.0/" + type + "/" + id + "/" + command + " : ").c_str());
	}
	
	cmd.replace("%EIOT_ADDRESS%", _EIoT_Address);
	cmd.replace("%TOKEN%", token);
	cmd.replace("%LENGTH%", String(content.length()));
	cmd.replace("%CONTENT%", "\r\n" + content);
	debugln(cmd);

	// Send the RestApi command
	client.print(cmd);	

	// Get the RestApi response
	while (!client.available());
	String response = "";
	while (client.available())
	{
		response += client.readStringUntil('\r');
	}
	return response;
}

String EIoTCloudRestApi::parseId(String response)
{
	String ret = "";
	int pos = 0;

	while ((pos = response.indexOf('{', pos)) != -1)
	{
		if (response.substring(pos, pos + 7) == "{\"Id\":\"")
		{
			int ix1 = response.indexOf("\"}", pos + 8);

			if (ix1 != -1)
			{
				ret = response.substring(pos + 7, ix1);
				debugln(ret.c_str());
			}
			break;
		}
		else
			pos++;
	}

	return ret;
}

bool EIoTCloudRestApi::parseResponse(String response)
{
	int pos = 0;

	if ((pos = response.indexOf('{', pos)) != -1)
	{
		if (response.substring(pos, pos + 16) == "{\"Response\":\"0\"}")
		{
			return true;
		}
	}

	return false;
}

String EIoTCloudRestApi::parseParameter(String response, String property)
{
	String ret = "";

	int pos = 0;
	int len = response.length();
	int lenProp = property.length();


	String propStr = "\"" + property + "\":\"";

	int pos1 = 0;

	while (pos < len)
	{
		if (response.substring(pos, pos + 4 + lenProp) == propStr)
		{
			pos = pos + 4 + lenProp;
			pos1 = pos;

			while (pos < len)
			{
				if ((pos = response.indexOf('\"', pos)) != -1)
				{
					ret = response.substring(pos1, pos);
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

String EIoTCloudRestApi::TokenNew(String instance)
{
	String response = RestApi_Instance("POST", "Token/New", instance);

	String ret = "";
	int pos = 0;
	while ((pos = response.indexOf('{', pos)) != -1)
	{
		if (response.substring(pos, pos + 10) == "{\"Token\":\"")
		{
			int ix1 = response.indexOf("\"}", pos + 11);

			if (ix1 != -1)
			{
				ret = response.substring(pos + 10, ix1);
				
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

bool EIoTCloudRestApi::TokenList(String instance, Vector<String> tokens)
{
	String response = RestApi_Instance("GET", "TokenList", instance);

	int ix = response.indexOf('[');

	if (ix != -1)
		response = response.substring(ix);

	int pos = 0;
	int ix1;
	int i = 0;
	while ((pos = response.indexOf('T', pos)) != -1)
	{
		if (response.substring(pos, pos + 8) == "Token\":\"")
		{
			response = response.substring(pos + 8);
			ix1 = response.indexOf("\"}");

			tokens.push_back(response.substring(0, ix1));

			debugln(tokens[i].c_str());

			i++;
			pos = 0;
		}
		else
			pos++;
	}
	return (i > 0);
}

String EIoTCloudRestApi::ModuleNew()
{
	String response = RestApi_AuthToken("POST", "", "New", _token);

	return parseId(response);
}

bool EIoTCloudRestApi::SetModuleType(String id, String moduleType)
{
	String response = RestApi_AuthToken("POST", id, "Type/" + moduleType, _token);

	return parseResponse(response);
}

bool EIoTCloudRestApi::SetModuleName(String id, String name)
{
	name.replace(" ", "%20");

	String response = RestApi_AuthToken("POST", id, "Name/" + name, _token);

	return parseResponse(response);
}

String EIoTCloudRestApi::NewModuleParameter(String id)
{
	String response = RestApi_AuthToken("POST", id, "NewParameter", _token);

	return parseId(response);
}

String EIoTCloudRestApi::NewModuleParameter(String id, String name)
{
	//name.replace(" ", "%20"); // Addition?

	String response = RestApi_AuthToken("POST", id, "NewParameter/" + name, _token);

	return parseId(response);
}

String EIoTCloudRestApi::GetModuleParameterByName(String id, String name)
{
	String response = RestApi_AuthToken("GET", id, "ParameterByName/" + name, _token);

	return parseId(response);
}

String EIoTCloudRestApi::getParameterProperty(String id, String property)
{
	String response = RestApi_AuthToken("GET", id, property, _token, "Parameter");

	return parseParameter(response, property);
}

bool EIoTCloudRestApi::SetParameterValues(String values)
{
	String response = RestApi_AuthToken("POST", "", "Values", _token, "Parameter", values);
	
	return parseResponse(response);
}

bool EIoTCloudRestApi::SetParameterValues(Vector<String> &Id, Vector<String> &Value)
{
	String content = "[{\"Id\": \"" + Id[0] + "\", \"Value\": \"" + Value[0] + "\" }";
	for (uint8_t i = 1; i < Id.size(); ++i)
	{
		content += ",{\"Id\": \"" + Id[i] + "\", \"Value\": \"" + Value[i] + "\" }";
	}
	content += "]";

	String response = RestApi_AuthToken("POST", "", "Values", _token, "Parameter", content);

	return parseResponse(response);
}

bool EIoTCloudRestApi::setParameterProperty(String id, String property, String value)
{
	String response = RestApi_AuthToken("POST", id, property + "/" + value, _token, "Parameter");

	return parseResponse(response);
}

bool EIoTCloudRestApi::setParameterProperty(String id, String property, bool value)
{
	String response = RestApi_AuthToken("POST", id, property + "/" + (value) ? "true" : "false", _token, "Parameter");

	return parseResponse(response);
}

#endif