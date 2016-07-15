#pragma once

#include <stdint.h>
#include <stddef.h>

// TODO

struct tm {
	int tm_sec;   	// Seconds [0,60].
	int tm_min;   	// Minutes [0,59].
	int tm_hour;  	// Hour [0,23].
	int tm_mday;  	// Day of month [1,31].
	int tm_mon;   	// Month of year [0,11].
	int tm_year;  	// Years since 1900.
	int tm_wday;  	// Day of week [0,6] (Sunday =0).
	int tm_yday;  	// Day of year [0,365].
	int tm_isdst; 	// Daylight Savings flag.
};

typedef int64_t time_t;

time_t time(time_t*);
double difftime(time_t time1, time_t time0);

char *asctime(const struct tm *tm);

char *ctime(const time_t *timep);

struct tm *gmtime(const time_t *timep);

struct tm *localtime(const time_t *timep);

time_t mktime(struct tm *tm);

size_t strftime(char *s, size_t max, const char *format, const struct tm *tm);


#define CLOCKS_PER_SEC 1
typedef int clock_t;
clock_t clock(void);



/* vim: set sts=0 ts=4 sw=4 tw=0 noet :*/
