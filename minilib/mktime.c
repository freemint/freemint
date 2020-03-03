#include <time.h>
#include <sys/time.h>

time_t mktime(struct tm *tm)
{
	static unsigned char const mon_days [] = { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };
	unsigned long tyears, tdays, leaps, utc_hrs;
	int i;

	tyears = tm->tm_year - 70;
	leaps = (tyears + 2) / 4;
	tdays = 0;
	for (i = 0; i < tm->tm_mon; i++)
		tdays += mon_days[i];

	tdays += tm->tm_mday - 1;
	tdays = tdays + tyears * 365 + leaps;

	utc_hrs = tm->tm_hour;
	return tdays * 86400UL + utc_hrs * 3600UL + tm->tm_min * 60U + tm->tm_sec;	
}
