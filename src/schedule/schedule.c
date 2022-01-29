#include <avrtos/kernel.h>

#include <time.h>

#include "schedule.h"

static void print_datetime_process(struct k_work *w)
{
	static struct tm time;
	const time_t ktime = k_time_get() - UNIX_OFFSET;

	localtime_r(&ktime, &time);

	k_show_uptime();

	// print tm structure 
	printf_P(PSTR("%d-%d-%d %d:%d:%d\n"), time.tm_year + 1900,
		 time.tm_mon + 1, time.tm_mday, time.tm_hour,
		 time.tm_min, time.tm_sec);
		 
}

K_WORK_DEFINE(datetime_work, print_datetime_process);

void schedule_print_datetime(void)
{
	k_system_workqueue_submit(&datetime_work);
}