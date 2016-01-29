#include <SNTPtime.h>
#include <ESP8266WiFi.h>

const char* ssid     = "your-ssid";
const char* password = "your-password";

void setup() {
  Serial.begin(115200);
  delay(10);

  // We start by connecting to a WiFi network

  Serial.println();
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);
  
  WiFi.begin(ssid, password);
  
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");  
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());

  configTime(0, 0, "0.pool.ntp.org", "1.pool.ntp.org", "2.pool.ntp.org");	// define our NTP servers
  delay(10000);	// Wait some time to syncronize

  Serial.print("ESP8266 timestamp is: ");
  Serial.println(sntp_get_current_timestamp());
  Serial.print("or ");
  Serial.println(SNTP_GetTime(TZ_UTC));

  Serial.print("Compile time zone (");
  Serial.print(SNTP_TimeZone_RuleName());
  Serial.print(") local timestamp is: ");
  Serial.println(SNTP_GetTime(TZ_LOCAL));

  randomSeed(micros());	// So we can get different start for random each boot
}

void loop() {
  delay(random(1000, 60000));	// random delay between 1sec and 1min

  Serial.print("UTC timestamp is: ");
  Serial.println(sntp_get_current_timestamp());

  // Change the time zone rules at random
  switch (random(5)) {
    case 1:
      TimeZone_UScentral();
    case 2:
      TimeZone_USPacific();
    case 3:
      TimeZone_AustraliaEast();
    case 4:
      TimeZone_EuropeUK();
    case 5:
      TimeZone_CentralEurope();
    default:
      TimeZone_EastEurope();
  }

  Serial.print(SNTP_TimeZone_RuleName());
  Serial.print(" local timestamp is: ");
  Serial.println(SNTP_GetTime(TZ_LOCAL));
}

void TimeZone_UScentral()
{
  SNTP_TimeZone_RuleSet(SNTP_RULE_STD, "CST", First, Sun, Nov, 2, -6*60);
  SNTP_TimeZone_RuleSet(SNTP_RULE_DST, "CDT", Second, Sun, Mar, 2, -5*60);
}

void TimeZone_USPacific()
{
  SNTP_TimeZone_RuleSet(SNTP_RULE_STD, "PDT", Second, Sun, Mar, 2, -7*60);
  SNTP_TimeZone_RuleSet(SNTP_RULE_DST, "PST", First, Sun, Nov, 2, -8*60);
}

void TimeZone_AustraliaEast()
{
  SNTP_TimeZone_RuleSet(SNTP_RULE_STD, "AEDT", First, Sun, Oct, 2, 11*60);
  SNTP_TimeZone_RuleSet(SNTP_RULE_DST, "AEST", First, Sun, Apr, 3, 10*60);
}

void TimeZone_EuropeUK()
{
  SNTP_TimeZone_RuleSet(SNTP_RULE_STD, "BST", Last, Sun, Mar, 1, 1*60);
  SNTP_TimeZone_RuleSet(SNTP_RULE_DST, "GMT", Last, Sun, Oct, 2, 0*60);
}

void TimeZone_CentralEurope()
{
  SNTP_TimeZone_RuleSet(SNTP_RULE_STD, "CET", Last, Sun, Oct, 3, 1*60);
  SNTP_TimeZone_RuleSet(SNTP_RULE_DST, "CEST", Last, Sun, Mar, 2, 2*60);
}

void TimeZone_EastEurope()
{
  SNTP_TimeZone_RuleSet(SNTP_RULE_STD, "EET", Last, Sun, Oct, 3, 2*60);
  SNTP_TimeZone_RuleSet(SNTP_RULE_DST, "EEST", Last, Sun, Mar, 2, 3*60);
}
