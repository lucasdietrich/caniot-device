#include <avrtos/kernel.h>
#include <avrtos/debug.h>

#include "schedule/schedule.h"

#if DEBUG_MONITOR || DEBUG_TIME

static void timer_handler(struct k_timer *tim);
static void work_handler(struct k_work *work);

K_TIMER_DEFINE(monitor_timer, timer_handler, K_SECONDS(30), 30000U);
K_WORK_DEFINE(monitor_work, work_handler);

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
	dump_stack_canaries();

	k_thread_dump_all();
}


#endif /* DEBUG_MONITOR || DEBUG_TIME */