/*
 * JSonWebConfig.h - Library for ESP8266 config from json config format file.
 * You need to install ArduinoJson library https://github.com/bblanchon/ArduinoJson
 * Authenticated web updates to config and auth.
 * Setups WiFi (AP and STA), webserver, SNTP according to config.
 * To use SNTP, you should also install the SNTPtime library
 * Seeds the random generator with micros after WiFi connection, which is different on every boot.
 * Uses random number for the session cookie to detect and clear old sessions after reboot.
 *
 * Add your custom function CustomConfigHandle(). Called for authenticated clients only.
 *		Or create your on and call WebSrvIsAuthentified()
 * And don't forget to upload the pages to flash ;)
 * 
 * Created by Kaloyan Kovachev, January, 2016.
 * Released into the public domain.
 */

#ifndef WWW_TEMPLATE_H
#define WWW_TEMPLATE_H

#include "WiFiClient.h"
#include "WiFiServer.h"
#include "ESP8266WebServer.h"

extern ESP8266WebServer WebSrv;		// Define it in your sketch with the desired port and IP

void WebSrv_Setup();

void handleWWW();

bool WebSrvIsAuthentified(String RedirectURL, String *AddHeader);

#endif
