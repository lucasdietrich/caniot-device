#include <time.h>

#include <avrtos/debug.h>
#include <avrtos/avrtos.h>
#include <avrtos/logging.h>
#define LOG_LEVEL LOG_LEVEL_DBG

#if DEBUG_MONITOR || DEBUG_TIME

#if !CONFIG_KERNEL_TIMERS
#error KERNEL_TIMERS disabled
#endif

static void timer_handler(struct k_timer *tim);
static void work_handler(struct k_work *work);
static void print_datetime_work(struct k_work *w);

K_TIMER_DEFINE(monitor_timer, timer_handler, K_SECONDS(30), 30000U);
K_WORK_DEFINE(monitor_work, work_handler);
K_WORK_DEFINE(datetime_work, print_datetime_work);

void schedule_print_datetime(void)
{
	k_system_workqueue_submit(&datetime_work);
}

static void timer_handler(struct k_timer *timer)
{
#if DEBUG_MONITOR
	k_system_workqueue_submit(&monitor_work);
#endif /* DEBUG_MONITOR */

#if DEBUG_TIME
	schedule_print_datetime();
#endif /* DEBUG_TIME */
}

static void work_handler(struct k_work *work)
{
	k_dump_stack_canaries();

	// k_thread_dump_all();
}

static void print_datetime_work(struct k_work *w)
{
	struct tm time;
	const time_t ktime = k_time_get() - UNIX_OFFSET;

	LOG_DBG("ktime %u\t", ktime);

	localtime_r(&ktime, &time);

	k_show_uptime();

	LOG_DBG("%d-%d-%d %d:%d:%d\n",
		time.tm_year + 1900,
		time.tm_mon + 1,
		time.tm_mday,
		time.tm_hour,
		time.tm_min,
		time.tm_sec);
}

#endif /* DEBUG_MONITOR || DEBUG_TIME */