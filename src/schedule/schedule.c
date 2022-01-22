#include <avrtos/kernel.h>

#include <time.h>

#include "schedule.h"


static void scheduling_process(void *ctx);


// void            localtime_r(const time_t * timer, struct tm * timeptr);

K_THREAD_DEFINE(tschedule, scheduling_process, 0x80, K_COOPERATIVE, NULL, 'S');

static void scheduling_process(void *ctx)
{
	static struct tm time;

	set_zone(+1 * ONE_HOUR);

	for (;;) {
		const time_t ktime = k_time_get() - UNIX_OFFSET;

		localtime_r(&ktime, &time);

		k_show_uptime();

		// print tm structure 
		printf_P(PSTR("%d-%d-%d %d:%d:%d\n"), time.tm_year + 1900,
			 time.tm_mon + 1, time.tm_mday, time.tm_hour,
			 time.tm_min, time.tm_sec);

		k_sleep(K_SECONDS(10));
	}
}