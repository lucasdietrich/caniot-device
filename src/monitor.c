#include <avrtos/kernel.h>
#include <avrtos/debug.h>

#include "schedule/schedule.h"

static void timer_handler(struct k_timer *tim);
static void work_handler(struct k_work *work);

// K_TIMER_DEFINE(monitor_timer, timer_handler, K_SECONDS(30), 30000U);
K_WORK_DEFINE(monitor_work, work_handler);

static void timer_handler(struct k_timer *timer)
{
	k_system_workqueue_submit(&monitor_work);

	schedule_print_datetime();
}

static void work_handler(struct k_work *work)
{
	dump_stack_canaries();

	k_thread_dump_all();
}
