#include "FS.h"
#include "ESP8266WiFiMulti.h"
#include "base64.h"

#include <ArduinoJson.h>
#include <JSonWebConfig.h>


ESP8266WiFiMulti wifiMulti;
long CookieRandom;

extern "C" void system_restart(void);

void handleConfig();

// #define DEBUG
#ifdef DEBUG
// #define DEBUG_CFG
#endif
#define DEBUG_OUTPUT Serial

#ifndef JSONWEBCONFIG_DEFAULT	// Define in your sketch to replace
#define JSONWEBCONFIG_DEFAULT	F("{\"AP\":{\"SSID\":\"ESP8266AP\", \"PWD\":\"ESP8266AP\"}, \"STA\":[[\"default\", \"\"], [\"ESP8266STA\", \"ESP8266STA\"]], \"SNTP\":[\"0.pool.ntp.org\", \"1.pool.ntp.org\", \"2.pool.ntp.org\"]}")
#endif

#ifndef JSONWEBCONFIG_CAHCE		// Define in your sketch to replace
#define JSONWEBCONFIG_CAHCE		"max-age=900"
#endif

String GetConfig()
{
	String CFG = "";

	if ( SPIFFS.begin() ) {
#ifdef DEBUG
		DEBUG_OUTPUT.println( F("SPIFFS loaded") );
#endif
		File f = SPIFFS.open("/ESP8266.cfg", "r");
		if ( !f ) {
#ifdef DEBUG
			DEBUG_OUTPUT.println( F("Can't open SPIFFS config file!") );
#endif
		} else {
			char buf[1024];
			int siz = f.size();
			while(siz > 0) {
				size_t len = std::min((int)(sizeof(buf) - 1), siz);

				f.read((uint8_t *)buf, len);
				buf[len] = 0;
				CFG += buf;
				siz -= sizeof(buf) - 1;
			}
			f.close();
#ifdef DEBUG
			DEBUG_OUTPUT.println( F("Loaded config file from SPIFFS!") );
#ifdef DEBUG_CFG
			DEBUG_OUTPUT.println( CFG );
#endif
#endif
		}
	} else {
#ifdef DEBUG
		DEBUG_OUTPUT.println( F("Error loading SPIFFS") );
#endif
	}

	return CFG;
}

void WiFi_AP_Setup(JsonObject& APcfg)
{
#ifdef DEBUG
		DEBUG_OUTPUT.print( F("Starting AP: ") );
		DEBUG_OUTPUT.println( APcfg["SSID"].asString() );
#ifdef DEBUG_CFG
		DEBUG_OUTPUT.print( F("Password: ") );
		DEBUG_OUTPUT.println( APcfg["PWD"].asString() );
#endif
#endif
	if (APcfg["IP"].is<JsonArray&>() ) {
		IPAddress apIP(APcfg["IP"][0], APcfg["IP"][1], APcfg["IP"][2], APcfg["IP"][3]);
		IPAddress apMASK(255, 255, 255, 0);
		if ( APcfg["MASK"].is<JsonArray&>() ) {
			apMASK[0] = APcfg["MASK"][0];
			apMASK[1] = APcfg["MASK"][1];
			apMASK[2] = APcfg["MASK"][2];
			apMASK[3] = APcfg["MASK"][3];
		}
		WiFi.softAPConfig(apIP, apIP, apMASK);
#ifdef DEBUG
		DEBUG_OUTPUT.print( F("AP IP: ") );
		DEBUG_OUTPUT.println( apIP );
		DEBUG_OUTPUT.print( F("AP MASK: ") );
		DEBUG_OUTPUT.println( apMASK );
#endif
	}
	WiFi.softAP(APcfg["SSID"].asString(), APcfg["PWD"].asString());
}

void WiFi_STA_Setup(JsonArray& STAcfg)
{
	int i = 0;
	while (STAcfg[i].is<JsonArray&>()) {
#ifdef DEBUG
		DEBUG_OUTPUT.print( F("Adding SSID: ") );
		DEBUG_OUTPUT.println( STAcfg[i][0].asString() );
#ifdef DEBUG_CFG
		DEBUG_OUTPUT.print( F("Password: ") );
		DEBUG_OUTPUT.println( STAcfg[i][1].asString() );
#endif
#endif
		wifiMulti.addAP( STAcfg[i][0].asString(), STAcfg[i][1].asString() );
		i++;
	}

	byte NTPretries = 100;
	while (wifiMulti.run() != WL_CONNECTED && NTPretries--)
	{
		delay(100);
#ifdef DEBUG
		DEBUG_OUTPUT.print( F(".") );
	}

	DEBUG_OUTPUT.println();
	if ( WiFi.status() == WL_CONNECTED )
	{
		DEBUG_OUTPUT.print( F("Connected To: ") );
		DEBUG_OUTPUT.println( WiFi.SSID() );
		DEBUG_OUTPUT.print( F("Got IP: ") );
		DEBUG_OUTPUT.println( WiFi.localIP() );
#ifdef DEBUG_CFG
		WiFi.printDiag(DEBUG_OUTPUT);
#endif
#endif
	}
}

void WebSrv_Setup()
{
	String CFG = GetConfig();

	DynamicJsonBuffer jsonBuffer;
	JsonObject& JScfg = jsonBuffer.parseObject(CFG);
	if ( !CFG || !JScfg.success()) {
		/*
		 * No config file or some error occurred - use defaults:
		 * AP with SSID and Passphrase "ESP8266AP"
		 * Try to connect to SSID "default" without password
		 * or to SSID "ESP8266STA" with Passphrase "ESP8266STA"
		 * Sync time form pool.ntp.org
		 * Leave the timezone to default at compile time
		 */
		CFG = JSONWEBCONFIG_DEFAULT;
		JsonObject& JScfg = jsonBuffer.parseObject(CFG);
	}

	if ( JScfg["AP"]["SSID"].asString() )
	{
		if ( JScfg["STA"].is<JsonArray&>() ) {
			WiFi.mode(WIFI_AP_STA);
		} else {
			WiFi.mode(WIFI_AP);
		}
		WiFi_AP_Setup(JScfg["AP"]);
	} else if ( JScfg["STA"].is<JsonArray&>() ) {
#ifdef DEBUG
		DEBUG_OUTPUT.println( "Have AP config" );
#endif
		WiFi.mode(WIFI_STA);
	}
	if ( JScfg["STA"].is<JsonArray&>() ) {
#ifdef DEBUG
		DEBUG_OUTPUT.println( "Have STA config" );
#endif
		WiFi_STA_Setup(JScfg["STA"]);
	}

	randomSeed(micros());
	CookieRandom = random(0xFFFF);

	if ( JScfg["SNTP"].is<JsonArray&>() ) {
		configTime(0, 0, JScfg["SNTP"][0], JScfg["SNTP"][1], JScfg["SNTP"][2]);
	}

#ifdef _SNTP_TIME_H
	if ( JScfg["TZ"]["DST"].is<JsonArray&>() ) {
		SNTP_TimeZone_RuleSet(SNTP_RULE_DST, JScfg["TZ"]["DST"][0], JScfg["TZ"]["DST"][1], JScfg["TZ"]["DST"][2], JScfg["TZ"]["DST"][3], JScfg["TZ"]["DST"][4], JScfg["TZ"]["DST"][5]);
	}

	if ( JScfg["TZ"]["STD"].is<JsonArray&>() ) {
		SNTP_TimeZone_RuleSet(SNTP_RULE_STD, JScfg["TZ"]["STD"][0], JScfg["TZ"]["STD"][1], JScfg["TZ"]["STD"][2], JScfg["TZ"]["STD"][3], JScfg["TZ"]["STD"][4], JScfg["TZ"]["STD"][5]);
	}
#endif

	WebSrv.on ( "/Config", handleConfig );
#ifdef DEBUG_CFG
	WebSrv.serveStatic( "/Auth.cfg", SPIFFS, "/Auth.cfg", "private, max-age=0, no-cache");
	WebSrv.serveStatic( "/ESP8266.cfg", SPIFFS, "/ESP8266.cfg", "private, max-age=0, no-cache");
#endif
	WebSrv.serveStatic( "/Static/", SPIFFS, "/Static_", JSONWEBCONFIG_CAHCE);
	WebSrv.serveStatic( "/css/", SPIFFS, "/css_", JSONWEBCONFIG_CAHCE);
	WebSrv.serveStatic( "/js/", SPIFFS, "/js_",JSONWEBCONFIG_CAHCE);
	WebSrv.serveStatic( "/img/", SPIFFS, "/img_", JSONWEBCONFIG_CAHCE);
	const char *NeedHeaders[] = { "Cookie", "Authorization" };
	WebSrv.collectHeaders( NeedHeaders, 2);
	WebSrv.begin();
#ifdef DEBUG
	DEBUG_OUTPUT.println( F("HTTP server started") );
#endif
}

void handleWWW() {
#ifdef DEBUG
	DEBUG_OUTPUT.print( F("Free heap:") );
	DEBUG_OUTPUT.println( ESP.getFreeHeap(), DEC );
#endif
	WebSrv.handleClient();
}

void handleInternalError()
{
	WebSrv.sendContent( F("HTTP/1.1 500 Internal Server Error\r\n\r\n") );
}

void handleRedirect(String MSG, String *AddHeader = NULL)
{
	String SendResponse = F("HTTP/1.1 302 OK\r\nLocation: /Static/");
	if ( MSG.length() > 1 ) {
		SendResponse += F("?MSG=");
		SendResponse += MSG;
	}
	SendResponse += F("\r\n");
	if ( AddHeader ) {
		SendResponse += *AddHeader;
	}
	SendResponse += F("Cache-Control: private, max-age=0, no-cache\r\n\r\n");
	WebSrv.sendContent( SendResponse );
}

bool WebSrvIsAuthentified(String RedirectURL, String *AddHeader = NULL)
{
	if ( WebSrv.hasArg("Logout") || (WebSrv.hasArg("Action") && WebSrv.arg("Action") == F("Logout")) ) {
		String SendResponse = F("HTTP/1.1 302 OK\r\nSet-Cookie: ESPSESSIONID=0\r\nLocation:");
		SendResponse += RedirectURL;
		SendResponse += F("\r\nCache-Control: private, max-age=0, no-cache\r\n\r\n");
		WebSrv.sendContent( SendResponse );
#ifdef DEBUG
		DEBUG_OUTPUT.println( F("Logged out") );
#endif
		return false;
	}

	if ( WebSrv.hasArg("Action") && WebSrv.arg("Action") == F("ClearAuth") ) {
		WebSrv.sendContent( F("HTTP/1.1 401 Not Authorized\r\nWWW-Authenticate: Basic realm=\"ESP8266\"\r\n\r\n") );
#ifdef DEBUG
		DEBUG_OUTPUT.println( F("Authentication cleared") );
#endif
		return false;
	}

#ifdef DEBUG
		DEBUG_OUTPUT.println( F("Checking Auth") );
#endif

	String OurCookie = F("ESPSESSIONID=");
	OurCookie += ESP.getChipId();
	OurCookie += F(".");
	OurCookie += ESP.getFlashChipId();
	OurCookie += F(".");
	OurCookie += CookieRandom;
	if ( WebSrv.hasHeader("Cookie") ) {
		if (WebSrv.header("Cookie").indexOf( OurCookie ) != -1) {
			return true;
		}
		if ( WebSrv.header("Cookie").indexOf(F("ESPSESSIONID=")) != -1 && WebSrv.header("Cookie") != F("ESPSESSIONID=0") && AddHeader ) {
#ifdef DEBUG
		DEBUG_OUTPUT.println( F("Deleting old cookie") );
#endif
		*AddHeader += F("Set-Cookie: ESPSESSIONID=0\r\n");
		}
	}

	if ( WebSrv.hasHeader("Authorization") ) {
		String AuthStr = WebSrv.header("Authorization").substring(WebSrv.header("Authorization").lastIndexOf(' ')+1);

		File f = SPIFFS.open("/Auth.cfg", "r");
		if ( !f ) {
			if ( AuthStr == F("YWRtaW46YWRtaW4=") ) { // admin:admin
				if ( AddHeader ) {
					*AddHeader += F("Set-Cookie: ");
					*AddHeader += OurCookie;
					*AddHeader += F("\r\n");
				} else {
					String SendResponse = F("HTTP/1.1 302 OK\r\nSet-Cookie: ");
					SendResponse += OurCookie;
					SendResponse += F("\r\nLocation: ");
					SendResponse += RedirectURL;
					SendResponse += F("\r\nCache-Control: private, max-age=0, no-cache\r\n\r\n");
					WebSrv.sendContent( SendResponse );
				}
#ifdef DEBUG
				DEBUG_OUTPUT.println( F("Authorized (Basic default)") );
#endif
				return true;
			}
		} else {
			String RAuth = f.readString();
			f.close();
			if ( AuthStr == RAuth ) {
				if ( AddHeader ) {
					*AddHeader += F("Set-Cookie: ");
					*AddHeader += OurCookie;
					*AddHeader += F("\r\n");
				} else {
					String SendResponse = F("HTTP/1.1 302 OK\r\nSet-Cookie: ");
					SendResponse += OurCookie;
					SendResponse += F("\r\nLocation: ");
					SendResponse += RedirectURL;
					SendResponse += F("\r\nCache-Control: private, max-age=0, no-cache\r\n\r\n");
					WebSrv.sendContent( SendResponse );
				}
#ifdef DEBUG
				DEBUG_OUTPUT.println( F("Authorized (Basic file)") );
#endif
				return true;
			}
		}
	} else if ( WebSrv.hasArg("username") && WebSrv.hasArg("password") ) {
		String AuthStr = base64::encode(String(WebSrv.arg("username")) + F(":") + WebSrv.arg("password"));

		File f = SPIFFS.open("/Auth.cfg", "r");
		if ( !f ) {
			if ( AuthStr == F("YWRtaW46YWRtaW4=") ) { // admin:admin
				if ( AddHeader ) {
					*AddHeader += F("Set-Cookie: ");
					*AddHeader += OurCookie;
					*AddHeader += F("\r\n");
				} else {
					String SendResponse = F("HTTP/1.1 302 OK\r\nSet-Cookie: ");
					SendResponse += OurCookie;
					SendResponse += F("\r\nLocation: ");
					SendResponse += RedirectURL;
					SendResponse += F("\r\nCache-Control: private, max-age=0, no-cache\r\n\r\n");
					WebSrv.sendContent( SendResponse );
				}
#ifdef DEBUG
				DEBUG_OUTPUT.println( F("Authorized (formdata default)") );
#endif
				return true;
			}
		} else {
			uint8_t RAuth[f.size()+1];
			f.read(RAuth, f.size());
			RAuth[f.size()] = 0;
			f.close();
			if ( AuthStr == (const char *)RAuth ) {
				if ( AddHeader ) {
					*AddHeader += F("Set-Cookie: ");
					*AddHeader += OurCookie;
					*AddHeader += F("\r\n");
				} else {
					String SendResponse = F("HTTP/1.1 302 OK\r\nSet-Cookie: ");
					SendResponse += OurCookie;
					SendResponse += F("\r\nLocation: ");
					SendResponse += RedirectURL;
					SendResponse += F("\r\nCache-Control: private, max-age=0, no-cache\r\n\r\n");
					WebSrv.sendContent( SendResponse );
				}
#ifdef DEBUG
				DEBUG_OUTPUT.println( F("Authorized (formdata file)") );
#endif
				return true;
			}
		}
	}

	if ( *AddHeader == F("Set-Cookie: ESPSESSIONID=0\r\n") ) {
		WebSrv.sendContent( F("HTTP/1.1 401 Not Authorized\r\nSet-Cookie: ESPSESSIONID=0\r\nWWW-Authenticate: Basic realm=\"ESP8266\"\r\n\r\n") );
	} else {
		WebSrv.sendContent( F("HTTP/1.1 401 Not Authorized\r\nWWW-Authenticate: Basic realm=\"ESP8266\"\r\n\r\n") );
	}
#ifdef DEBUG
	DEBUG_OUTPUT.println( F("Not Authorized") );
#endif
	return false;  
}

void handleConfig() {
	String AddHeader = "";
	if ( !WebSrvIsAuthentified( F("/Static/"), &AddHeader) ) {
		return;
	}
	if ( WebSrv.hasArg("Logout") ) {
		return;
	}
	if ( WebSrv.hasArg("Action") ) {
		if ( WebSrv.arg("Action") == F("Logout") ) return;

		if ( WebSrv.arg("Action") == F("Login") ) {
			if ( AddHeader.length() > 1 ) {
				handleRedirect( F("Login successful"), &AddHeader );
			} else {
				handleRedirect( F("Login successful") );
			}
			return;
		}

		if ( WebSrv.arg("Action") == F("Reboot") ) {
			WebSrv.send( 302, "text/html", F("<html><head><META http-equiv=\"Refresh\" content=\"15;URL=/Static/\"></head><body>Please wait 15 seconds. If not redirected click <a href=\"/Static/\">here</a></body></html>") );
#ifdef DEBUG
			DEBUG_OUTPUT.println( F("Rebooting") );
#endif
			delay(1000);
			system_restart();
			delay(1000);
			return;
		}

		if ( WebSrv.arg("Action") == F("Reset to defaults") ) {
			SPIFFS.remove("/Auth.cfg");
			SPIFFS.remove("/ESP8266.cfg");
#ifdef DEBUG
			DEBUG_OUTPUT.println( F("Reset to defaults") );
#endif
		}
	}

	if( WebSrv.arg("Action") == F("Change Credentials") && WebSrv.hasArg("NewUSR") && WebSrv.hasArg("NewPWD") ) {
		File f = SPIFFS.open("/Auth.cfg", "w");
		if ( !f ) {
			handleInternalError();
			return;
		} else {
			String base64Auth = base64::encode(WebSrv.arg("NewUSR") + F(":") + WebSrv.arg("NewPWD"));

			f.print(base64Auth);
			f.flush();
			f.close();
#ifdef DEBUG
			DEBUG_OUTPUT.print( F("Auth changed to ") );
			DEBUG_OUTPUT.println( base64Auth );
#endif
		}
	} else if ( WebSrv.arg("Action") == F("Save Config") && WebSrv.hasArg("JScfg") ) {
		File f = SPIFFS.open("/ESP8266.cfg", "w");
		if ( !f ) {
			handleInternalError();
			return;
		} else {
			f.print(WebSrv.arg("JScfg"));
			f.flush();
			f.close();
		}
	} else if ( WebSrv.hasArg("SSID_NNN") ) {
		DynamicJsonBuffer jsonBuffer;
		JsonObject& JScfgObj = jsonBuffer.createObject();
		if ( WebSrv.arg("SSID").length() ) {
			JScfgObj["SSID"] = WebSrv.arg("SSID");
			JScfgObj["PWD"] = WebSrv.arg("PWD");
		}
		if ( WebSrv.arg("IP").length() > 6 ) {
			JsonArray&  JArray  = JScfgObj.createNestedArray("IP");

			int startPos = 0;
			for (int i=0; i<4; i++) {
				int endPos = WebSrv.arg("IP").indexOf('.', startPos);
				int val = WebSrv.arg("IP").substring(startPos, endPos).toInt();

				if ( val < 0 || val > 254 || (val == 0 && (i == 0 || i == 3)) ) {
					i = 4;
					endPos = 15;
				} else {
					JArray.add(val);
				}
				startPos = endPos + 1;
			}
			if ( startPos >= WebSrv.arg("IP").length() ) JScfgObj.remove("IP");
		}
		if ( WebSrv.arg("MASK").length() > 6 ) {
			JsonArray&  JArray  = JScfgObj.createNestedArray("MASK");

			int startPos = 0;
			for (int i=0; i<4; i++) {
				int endPos = WebSrv.arg("MASK").indexOf('.', startPos);
				int val = WebSrv.arg("MASK").substring(startPos, endPos).toInt();

				if ( val < 0 || val > 255 ) {
					i = 4;
					endPos = 15;
				} else {
					JArray.add(val);
				}
				startPos = endPos + 1;
			}
			if ( startPos >= WebSrv.arg("MASK").length() ) JScfgObj.remove("MASK");
		}

		if ( WebSrv.hasArg("SSID_1") ) {
			JsonArray&  JArray  = JScfgObj.createNestedArray("STA");

			int next = 1;
			while ( WebSrv.arg("SSID_"+next) ) {
				JsonArray&  JArraySTA  = JArray.createNestedArray();
				JArraySTA.add(WebSrv.arg("SSID_"+next));
				JArraySTA.add(WebSrv.arg("PWD_"+next));
				next++;
			}
		}

		if ( WebSrv.arg("SNTP_1").length() > 4 || WebSrv.arg("SNTP_2").length() > 4 || WebSrv.arg("SNTP_3").length() > 4 ) {
			JsonArray&  JArray  = JScfgObj.createNestedArray("SNTP");
			JArray.add(WebSrv.arg("SNTP_1"));
			JArray.add(WebSrv.arg("SNTP_2"));
			JArray.add(WebSrv.arg("SNTP_3"));
		}
		if ( WebSrv.arg("DST_Month").length() || WebSrv.arg("STD_Month").length() ) {
			JsonObject& ObjTZ = JScfgObj.createNestedObject("TZ");
			if ( WebSrv.arg("DST_Month").length() ) {
				JsonArray&  JArray  = ObjTZ.createNestedArray("DST");
				JArray.add(WebSrv.arg("DST_Name"));
				JArray.add(WebSrv.arg("DST_Week"));
				JArray.add(WebSrv.arg("DST_DoW"));
				JArray.add(WebSrv.arg("DST_Month"));
				JArray.add(WebSrv.arg("DST_Hour"));
				JArray.add(WebSrv.arg("DST_Offset"));
			}
			if ( WebSrv.arg("STD_Month").length() ) {
				JsonArray&  JArray  = ObjTZ.createNestedArray("STD");
				JArray.add(WebSrv.arg("STD_Name"));
				JArray.add(WebSrv.arg("STD_Week"));
				JArray.add(WebSrv.arg("STD_DoW"));
				JArray.add(WebSrv.arg("STD_Month"));
				JArray.add(WebSrv.arg("STD_Hour"));
				JArray.add(WebSrv.arg("STD_Offset"));
			}
		}

		File f = SPIFFS.open("/ESP8266.cfg", "w");
		if ( !f ) {
			handleInternalError();
			return;
		} else {
			JScfgObj.printTo(f);
			f.flush();
			f.close();
		}

	} else {
		if ( AddHeader.length() ) {
			handleRedirect( F("Login successful"), &AddHeader );
		} else {
			handleRedirect( F("Login successful") );
		}
		return;
	}

	if ( AddHeader.length() ) {
		handleRedirect( F("Changed successfuly"), &AddHeader );
	} else {
		handleRedirect( F("Changed successfuly") );
	}
}
