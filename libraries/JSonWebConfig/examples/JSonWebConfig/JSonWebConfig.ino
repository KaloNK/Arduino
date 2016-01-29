#if 0
#include "FS.h"
#include "ESP8266WiFi.h"
#include "WiFiClient.h"
#include "WiFiServer.h"
#include "ESP8266WebServer.h"
#include "ESP8266WiFiMulti.h"
#include "base64.h"

#include <ArduinoJson.h>	// You need to install ArduinoJson library
#endif

#include <SNTPtime.h>		// To use SNTP, you should also install the SNTPtime library
#include <JSonWebConfig.h>
ESP8266WebServer WebSrv ( 80 );

void setup() {
  Serial.begin(115200);
  Serial.println();
  Serial.println( F("Ready") );

  WebSrv_Setup();
}

void loop() {

  handleWWW();
  delay(500);
}
