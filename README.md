# ESP8266-car-battery-monitor-to-EasyIoT
Small diy project to monitor the 12V car battery voltage of a vehicle in winter storage.
A learning project for ESP8266 usage with lots of my self learning of Arduino and C++ language.
A lot of using examples found elsewhere, glued together with beginner code from me and much help from 'micooke'.
Also uses a DS18B20 temp sensor because it was fun to include...
Project connects to the EasyIoT cloud http://iot-playground.com/ 

The ESP8266 and PCB measure the cars battery voltage once per hour (using deepsleep with a multiplier value stored in NVRAM), longer delay times are a matter of increasing the deepsleep count value. It also measures ambient temperature, but that can be deleted simply enough.
Once a measurement is taken the device connects to a local WiFi router and posts the values to the Easy IoT cloud website via an instance ID you have created previously. The Easy IoT cloud settings allow a user to set trigger threasholds for an email warnings to be sent, and I set the warning to trigger on at 12.2V. When I receive the email I plug the cars charger back in or take it out for a run... 
