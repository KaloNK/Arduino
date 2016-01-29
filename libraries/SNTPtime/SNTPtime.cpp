#include <SNTPtime.h>

//	EET / EEST default
TimeChangeRule TZ_std = {"EET", Last, Sun, Oct, 3, 2*60};
TimeChangeRule TZ_dst = {"EEST", Last, Sun, Mar, 2, 3*60};

//	CET / CEST default
//TimeChangeRule TZ_std = {"CET", Last, Sun, Oct, 3, 1*60};
//TimeChangeRule TZ_dst = {"CEST", Last, Sun, Mar, 2, 2*60};

#ifndef _Time_h
void breakTime(time_t timeInput, tmElements_t &tm){
// break the given time_t into time components
// this is a more compact version of the C library localtime function
// note that year is offset from 1970 !!!

	uint8_t year;
	uint8_t month, monthLength;
	uint32_t time;
	unsigned long days;

	time = (uint32_t)timeInput;
	tm.Second = time % 60;
	time /= 60; // now it is minutes
	tm.Minute = time % 60;
	time /= 60; // now it is hours
	tm.Hour = time % 24;
	time /= 24; // now it is days
	tm.Wday = ((time + 4) % 7) + 1;  // Sunday is day 1 
  
	year = 0;  
	days = 0;
	while((unsigned)(days += (LEAP_YEAR(year) ? 366 : 365)) <= time) {
		year++;
	}
	tm.Year = year; // year is offset from 1970 

	days -= LEAP_YEAR(year) ? 366 : 365;
	time  -= days; // now it is days in this year, starting at 0

	days=0;
	month=0;
	monthLength=0;
	for (month=0; month<12; month++) {
		if (month==1) { // february
			if (LEAP_YEAR(year)) {
				monthLength=29;
			} else {
				monthLength=28;
			}
		} else {
			monthLength = monthDays[month];
		}

		if (time >= monthLength) {
			time -= monthLength;
		} else {
			break;
		}
	}
	tm.Month = month + 1;  // jan is month 1  
	tm.Day = time + 1;     // day of month
}

time_t makeTime(tmElements_t &tm)
{
// assemble time elements into time_t 
// note year argument is offset from 1970 (see macros in time.h to convert to other formats)
// previous version used full four digit year (or digits since 2000),i.e. 2009 was 2009 or 9

	int i;
	uint32_t seconds;

	// seconds from 1970 till 1 jan 00:00:00 of the given year
	seconds= tm.Year*(SECS_PER_DAY * 365);
	for (i = 0; i < tm.Year; i++) {
		if (LEAP_YEAR(i)) {
			seconds +=  SECS_PER_DAY;   // add extra days for leap years
		}
	}

	// add days for this year, months start from 1
	for (i = 1; i < tm.Month; i++) {
		if ( (i == 2) && LEAP_YEAR(tm.Year)) { 
			seconds += SECS_PER_DAY * 29;
		} else {
			seconds += SECS_PER_DAY * monthDays[i-1];  //monthDay array starts from 0
		}
	}
	seconds+= (tm.Day-1) * SECS_PER_DAY;
	seconds+= tm.Hour * SECS_PER_HOUR;
	seconds+= tm.Minute * SECS_PER_MIN;
	seconds+= tm.Second;
	return (time_t)seconds; 
}

int weekday(time_t t)	// Sunday is day 1
{
	tmElements_t tm;
	breakTime(t, tm);
	return tm.Wday;
}

int year(time_t t) {	// the year for the given time
	tmElements_t tm;
	breakTime(t, tm);
	return tmYearToCalendar(tm.Year);
}
#endif

#ifndef Timezone_h
static struct
{
	time_t dst_time;
	time_t std_time;
} TZ_local;

/*----------------------------------------------------------------------*
 * Convert the given DST change rule to a time_t value                  *
 * for the given year.                                                  *
 *----------------------------------------------------------------------*/
time_t Timezone::toTime_t(TimeChangeRule r, int yr)
{
	tmElements_t tm;
	time_t t;
	uint8_t m, w;            //temp copies of r.month and r.week

	m = r.month;
	w = r.week;
	if (w == 0) {            //Last week = 0
		if (++m > 12) {      //for "Last", go to the next month
			m = 1;
			yr++;
		}
		w = 1;               //and treat as first week of next month, subtract 7 days later
	}

	tm.Hour = r.hour;
	tm.Minute = 0;
	tm.Second = 0;
	tm.Day = 1;
	tm.Month = m;
	tm.Year = yr - 1970;
	t = makeTime(tm);        //first day of the month, or first day of next month for "Last" rules
	t += (7 * (w - 1) + (r.dow - weekday(t) + 7) % 7) * SECS_PER_DAY;
	if (r.week == 0) t -= 7 * SECS_PER_DAY;    //back up a week if this is a "Last" rule
	return t;
}

/*----------------------------------------------------------------------*
 * Calculate the DST and standard time change points for the given      *
 * given year as local and UTC time_t values.                           *
 *----------------------------------------------------------------------*/
void Timezone::calcTimeChanges(int yr)
{
	TZ_local.dst_time = toTime_t(TZ_dst, yr);
	TZ_local.std_time = toTime_t(TZ_std, yr);
}

int Timezone::TimezoneOffset(time_t use_time)
{
	if (year(use_time) != year(TZ_local.dst_time)) calcTimeChanges(year(use_time));

	if (TZ_local.std_time > TZ_local.dst_time) {	// Northern hemisphere
		return ((use_time >= TZ_local.dst_time && use_time < TZ_local.std_time) ? TZ_dst.offset : TZ_std.offset);
	} else {										// Southern hemisphere
		return (!(use_time >= TZ_local.std_time && use_time < TZ_local.dst_time) ? TZ_dst.offset : TZ_std.offset);
	}
}

/*----------------------------------------------------------------------*
 * Convert the given UTC time to local time, standard or                *
 * daylight time, as appropriate.                                       *
 *----------------------------------------------------------------------*/
time_t Timezone::toLocal(time_t utc)
{
	return utc + 60 * TimezoneOffset(utc);
}

/*----------------------------------------------------------------------*
 * Convert the given local time to UTC time.                            *
 *                                                                      *
 * WARNING:                                                             *
 * This function is provided for completeness, but should seldom be     *
 * needed and should be used sparingly and carefully.                   *
 *                                                                      *
 * Ambiguous situations occur after the Standard-to-DST and the         *
 * DST-to-Standard time transitions. When changing to DST, there is     *
 * one hour of local time that does not exist, since the clock moves    *
 * forward one hour. Similarly, when changing to standard time, there   *
 * is one hour of local times that occur twice since the clock moves    *
 * back one hour.                                                       *
 *                                                                      *
 * This function does not test whether it is passed an erroneous time   *
 * value during the Local -> DST transition that does not exist.        *
 * If passed such a time, an incorrect UTC time value will be returned. *
 *                                                                      *
 * If passed a local time value during the DST -> Local transition      *
 * that occurs twice, it will be treated as the earlier time, i.e.      *
 * the time that occurs before the transistion.                         *
 *                                                                      *
 * Calling this function with local times during a transition interval  *
 * should be avoided!                                                   *
 *----------------------------------------------------------------------*/
time_t Timezone::toUTC(time_t local)
{
	return local - 60 * TimezoneOffset(local);
}
#endif

void SNTP_TimeZone_RuleSet(SNTPrule rule, String rule_name, uint8_t week, uint8_t dow, uint8_t month, uint8_t hour, int offset)
{
	if (rule == SNTP_RULE_STD) {
		for (int i=0; i<rule_name.length() && i<sizeof(TZ_std.abbrev); i++) TZ_std.abbrev[i] = rule_name.charAt(i);
		TZ_std.week = week;
		TZ_std.dow = dow;
		TZ_std.month = month;
		TZ_std.hour = hour;
		TZ_std.offset = offset;
	} else {
		for (int i=0; i<rule_name.length() && i<sizeof(TZ_dst.abbrev); i++) TZ_dst.abbrev[i] = rule_name.charAt(i);
		TZ_dst.week = week;
		TZ_dst.dow = dow;
		TZ_dst.month = month;
		TZ_dst.hour = hour;
		TZ_dst.offset = offset;
	}
}

String SNTP_TimeZone_RuleName(time_t use_time)
{
	Timezone TZ(TZ_dst, TZ_std);
	int OffsetDiff = use_time - TZ.toLocal(use_time);

	if ( OffsetDiff == TZ_std.offset ) return TZ_std.abbrev;
	else return TZ_dst.abbrev;
}

String SNTP_TimeZone_RuleName()
{
	return SNTP_TimeZone_RuleName(sntp_get_current_timestamp());
}

time_t SNTP_GetTime(TimeZoneType GetTime, time_t use_time)
{
	Timezone TZ(TZ_dst, TZ_std);
	switch ( GetTime ) {
		case TZ_LOCAL:
			return TZ.toLocal(use_time);
		case TZ_LOCAL2UTC:
			return TZ.toUTC(use_time);
		default:
			return use_time;
	}
}

time_t SNTP_GetTime(TimeZoneType GetTime)
{
	return SNTP_GetTime(GetTime, sntp_get_current_timestamp());
}
