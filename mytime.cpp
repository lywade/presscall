#include <time.h>
#include <stdint.h>
#include <string.h>
#include <sys/time.h>
#include <unistd.h>
#include "mytime.hpp"
//#include "thread.h"
//#include "myconfig.h"
//#include "daemon.h"

volatile uint32_t gnow;
char gnowstr[30];

static unsigned lastgtime;

const char weekstr[] =
"Sun,Mon,Tue,Wed,Thu,Fri,Sat,";

static const char mdaystr[] =
" 00  01  02  03  04  05  06  07  08  09 "
" 10  11  12  13  14  15  16  17  18  19 "
" 20  21  22  23  24  25  26  27  28  29  30  31  32 ";

static const char monthstr[] =
"Jan Feb Mar Apr May Jun Jul Aug Sep Oct Nov Dec ";

static const char monthstr1[] =
"Jan/Feb/Mar/Apr/May/Jun/Jul/Aug/Sep/Oct/Nov/Dec/";

static const char yearstr[] =
"1970197119721973197419751976197719781979"
"1980198119821983198419851986198719881989"
"1990199119921993199419951996199719981999"
"2000200120022003200420052006200720082009"
"2010201120122013201420152016201720182019"
"2020202120222023202420252026202720282029"
"2030203120322033203420352036203720382039";

static const char secondstr[] =
"00010203040506070809"
"10111213141516171819"
"20212223242526272829"
"30313233343536373839"
"40414243444546474849"
"50515253545556575859";

struct __tstr {
	uint32_t wname;
	uint32_t mday;
	uint32_t mname;
	uint32_t year;
	char s[1]; 
	uint16_t hour;
	char c1[1];
	uint16_t min;
	char c2[1];
	uint16_t sec;
	uint32_t gmt;
	char n[1];
} __attribute__((packed));

/*
static int timekeeper = 0;

static int timekeeper_run(void* priv) {
	
	while(!stop) {
		gnow = time(0);
		sleep(1);
	}
	return 0;
}

int init_time() {
	
	timekeeper = myconfig_get_intval("timekeeper", 1);
	if(timekeeper) {
		gnow = time(0);
		if(register_thread("", timekeeper_run, NULL) != 0)
			return -1;	
	}
	return 0;
}

uint32_t nowtime() {
	if(!timekeeper)
		gnow = time(0);
	return gnow;
}
*/

const char* nowtimestr(char* timestr) {
//	uint32_t now = time(0);
	gnow = time(0);
	if(gnow == lastgtime && gnowstr[0]) {
		strcpy(timestr, gnowstr);		
	}
	else {
		//gettimestr(timestr, now);
		//gnow = now;
		gettimestr(timestr, gnow);
		lastgtime = gnow;
		strcpy(gnowstr, timestr);
	}

	return timestr;
}
const char* gettimestr(char* timestr, uint32_t tm) {

	struct __tstr *t = (struct __tstr *)timestr;
	int year, month;

	t->sec = ((uint16_t *)secondstr)[tm%60];
	tm /= 60;

	t->min = ((uint16_t *)secondstr)[tm%60];
	tm /= 60;

	t->hour = ((uint16_t *)secondstr)[tm%24];
	tm /= 24;

	t->wname = ((uint32_t *)weekstr)[(tm+4)%7];

	tm += 365 * 2 - 59;

	year = (tm * 4 + 3) / (365 * 4 + 1);
	tm = tm - year * 365 - year / 4 + 31;

	month = tm * 17 / 520;
	tm = tm - month * 520 / 17;

	t->mday = ((uint32_t *)mdaystr)[tm];

	if(month < 11) {
		month++;
		year -= 2;
	} 
	else {
		month -= 11;
		year--;
	}
	t->mname = ((uint32_t *)monthstr)[month];
	t->year = ((uint32_t *)yearstr)[year];

	t->s[0] = ' ';
	t->c1[0] = ':';
	t->c2[0] = ':';
	t->gmt = *(uint32_t *)" GMT";
	t->n[0] = '\0';
	
	return timestr;
}

/*
uint64_t gettick() {
	struct timeval tv;
	gettimeofday(&tv, NULL);
	return ((uint64_t)tv.tv_sec) * 100000ULL + tv.tv_usec / 10;
}
uint64_t maketick(double sec) {
	return (uint64_t)(sec * 100000);
}
*/


