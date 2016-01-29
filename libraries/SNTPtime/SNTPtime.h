/*
 * SNTPtime.h - Library for ESP8266 builtin SNTP.
 * Call "configTime(0, 0, "server 1", "server 2", "server 3");" with your desired time servers.
 * Allways uses UTC with SNTP, but allows converting to local time, based on configurable (DST/STD) rules.
 * Borrowed some code from Time and Timezone libraries - include them first if you use them
 * #include <Time.h>        //http://www.arduino.cc/playground/Code/Time
 * #include <Timezone.h>    //https://github.com/JChristensen/Timezone
 * Note: Timezone library may need some changes for ESP8266, that's why most of it is duplicated here
 * 
 * Created by Kaloyan Kovachev, January, 2016.
 * Released into the public domain.
 */

#ifndef _SNTP_TIME_H
#define _SNTP_TIME_H

#include "Arduino.h"

extern "C" uint32 sntp_get_current_timestamp();
// Convenient constants for the Time Zones and Rules
enum TimeZoneType { TZ_UTC, TZ_LOCAL, TZ_LOCAL2UTC };
enum SNTPrule { SNTP_RULE_DST, SNTP_RULE_STD };

#ifndef _Time_h
/* Useful Constants */
#define SECS_PER_MIN  (60UL)
#define SECS_PER_HOUR (3600UL)
#define SECS_PER_DAY  (SECS_PER_HOUR * 24UL)

/* and macros */
#define  tmYearToCalendar(Y) ((Y) + 1970)  // Full four digit year
// leap year calulator expects year argument as years offset from 1970
#define LEAP_YEAR(Y)     ( ((1970+Y)>0) && !((1970+Y)%4) && ( ((1970+Y)%100) || !((1970+Y)%400) ) )
static  const uint8_t monthDays[]={31,28,31,30,31,30,31,31,30,31,30,31}; // API starts months from 1, this array starts from 0

typedef struct  {
	uint8_t Second; 
	uint8_t Minute; 
	uint8_t Hour; 
	uint8_t Wday;   // day of week, sunday is day 1
	uint8_t Day;
	uint8_t Month; 
	uint8_t Year;   // offset from 1970; 
}	tmElements_t;
#endif

#ifndef Timezone_h
enum week_t {Last=0, First, Second, Third, Fourth}; 
enum dow_t {Sun=1, Mon, Tue, Wed, Thu, Fri, Sat};
enum month_t {Jan=1, Feb, Mar, Apr, May, Jun, Jul, Aug, Sep, Oct, Nov, Dec};

struct TimeChangeRule
{
	char abbrev[6];	// Five chars max
	uint8_t week;	// First, Second, Third, Fourth, or Last week of the month
	uint8_t dow;	// day of week, 1=Sun, 2=Mon, ... 7=Sat
	uint8_t month;	// 1=Jan, 2=Feb, ... 12=Dec
	uint8_t hour;	// 0-23
	int offset;		// Offset from UTC in minutes
};

class Timezone
{
	public:
		Timezone(TimeChangeRule dstStart, TimeChangeRule stdStart) {}
		time_t toLocal(time_t utc);
		time_t toUTC(time_t local);

	private:
		void calcTimeChanges(int yr);
		int TimezoneOffset(time_t use_time);
		time_t toTime_t(TimeChangeRule r, int yr);
};
#endif

void SNTP_TimeZone_RuleSet(SNTPrule rule, String rule_name, uint8_t week, uint8_t dow, uint8_t month, uint8_t hour, int offset);

String SNTP_TimeZone_RuleName(time_t use_time);
String SNTP_TimeZone_RuleName();

int SNTP_TimeZone_Offset(time_t use_time);
int SNTP_TimeZone_Offset();

time_t SNTP_GetTime(TimeZoneType GetTime, time_t use_time);
time_t SNTP_GetTime(TimeZoneType GetTime);

#endif