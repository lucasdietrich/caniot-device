#include "alarm.h"

#include <avrtos/kernel.h>

#include <string.h>
#include <stdio.h>
#include <avr/pgmspace.h>

#include "custompcb/board.h"

#include "dev.h"

static void show_uptime(void)
{
        struct timespec ts;
        k_timespec_get(&ts);

        uint32_t seconds = ts.tv_sec;
        uint32_t minutes = seconds / 60;
        uint32_t hours = minutes / 60;

        printf_P(PSTR("%02lu:%02hhu:%02hhu [%lu.%03u s] : "),
                 hours, (uint8_t)(minutes % 60), (uint8_t)(seconds % 60),
                 ts.tv_sec, ts.tv_msec);
}

#define RELAY_ALARM_MASK	BIT(RELAY1)
// #define RELAY_WARN_MASK		RELAY2

#define INPUT_MASK		(BIT(IN0) | BIT(IN1) | BIT(IN2) | BIT(IN3))
#define INPUT_ACCEPT_FILTER	(0b0000U)

static inline void sound(void)
{
	ll_relays_set_mask(RELAY_ALARM_MASK, RELAY_ALARM_MASK);
}

static inline void stop_sound(void)
{
	ll_relays_set_mask(0, RELAY_ALARM_MASK);
}

typedef enum { init = 0, ringing, waiting, terminated } siren_state_t; 

static siren_state_t siren_state;

// multiply by 10 to get real durations
#define RINGING_DURATION			12000LLU
#define CYCLE_WAITING_DURATION			3000LLU
#define SUCCESSIVE_CYCLES_WAITING_DURATION	18000LLU
#define AUTO_RESET_DURATION			180000LLU
#define MAX_SUCCESSIVE_CYCLES			3U
#define MAX_CYCLES				(MAX_SUCCESSIVE_CYCLES * 3U)

static uint8_t cycles_counter;

static void siren_reset(void)
{
	cycles_counter = 0;
	siren_state = init;
}

static void siren_state_machine(void)
{
	static uint8_t cycles_counter;
	static uint64_t waiting_duration;
	static uint64_t marker;

	const uint64_t now = k_uptime_get_ms64();

	switch (siren_state) {
	case init:
	{
		cycles_counter = 0;

		sound();
		marker = now;
		siren_state = ringing;

		break;
	}
	case ringing:
	{
		if (now - marker > RINGING_DURATION) {
			stop_sound();
			marker = now;

			cycles_counter++;
			if (cycles_counter < MAX_CYCLES) {
				waiting_duration =
					cycles_counter % MAX_SUCCESSIVE_CYCLES == 0 ?
					SUCCESSIVE_CYCLES_WAITING_DURATION :
					CYCLE_WAITING_DURATION;
				siren_state = waiting;
			} else {
				siren_state = terminated;
			}
		}
		break;
	}
	case waiting:
	{
		if (now - marker > waiting_duration) {
			sound();
			marker = now;
			siren_state = ringing;
		}
		break;
	}
	case terminated:
	{
		break;
	}
	}
}

/*___________________________________________________________________________*/

void alarm_loop(void *ctx);
K_THREAD_DEFINE(talarm, alarm_loop, 0xA0, K_COOPERATIVE, NULL, 'A');

K_SEM_DEFINE(reset_sem, 0, 1);

static bool alarm_enabled;
static alarm_state_t alarm_state;
static bool alarm_initialized = false;

void alarm_init(void)
{
	alarm_enabled = true; /* TODO set to false */
	alarm_state = inactive;

	alarm_initialized = true;

	siren_reset();

	/* TODO read alarm_enabled from config (EEPROM) */
}

static bool verify_status(void)
{
	const uint8_t inputs = ll_inputs_read();
	
	return (inputs & INPUT_MASK) == INPUT_ACCEPT_FILTER;
}

static const char *state_str(alarm_state_t state)
{
	switch (state) {
	case inactive:
		return PSTR("inactive");
	case observing:
		return PSTR("observing");
	case sounding:
		return PSTR("sounding");
	case recovering:
		return PSTR("recovering");
	default:
		return NULL;
	}
}

static void set_state(alarm_state_t state)
{
	if (alarm_state != state) {
		char cur[11], next[11];
		strcpy_P(cur, state_str(alarm_state));
		strcpy_P(next, state_str(state));

		/* TODO remove */
		show_uptime();

		printf_P(PSTR("alarm: %s -> %s\n"), cur, next);

		/* do action in transition */
		alarm_state = state;
		
		/* request telemetry on alarm state change */
		request_telemetry();
	}
}

// static inline void transition(alarm_state_t state)
// {
// 	// switch(state) {
	
// 	// }
// 	set_state(state);
// }

static int alarm_state_machine(void)
{
	static uint64_t recovering_time;

	/* force state to inactive if alarm is globally disabled */
	if (alarm_enabled == false) {
		set_state(inactive);
	}

	/* state machine */
	switch (alarm_state) {
	case inactive:
	{
		if (alarm_enabled && alarm_initialized && verify_status()) {
			siren_reset();
			set_state(observing);
		}

		break;
	}

	case observing:
	{
		if (verify_status() == false) {
			sound();

			set_state(sounding);
		}
		break;
	}

	case sounding:
	{	
		siren_state_machine();

		if (siren_state == waiting && verify_status() == true) {
			set_state(observing);
		} else if (siren_state == terminated) {
			recovering_time = k_uptime_get_ms64();

			set_state(recovering);
		}
		break;
	}

	case recovering:
	{
		// reset siren state machine after 30 minutes or admin manual reset
		if ((k_sem_take(&reset_sem, K_NO_WAIT) == 0) ||
		    (k_uptime_get_ms64() - recovering_time > AUTO_RESET_DURATION)) {
			siren_reset();

			set_state(verify_status() ? observing : inactive);
		}
		break;
	}

	default:
		return -EINVAL;
	}

	return 0;
}

void alarm_loop(void *ctx)
{
	alarm_init();

	for (;;) {
		if (alarm_state_machine() != 0) {
			__fault(K_FAULT); /* what to do ? */
		}
		k_sleep(K_MSEC(500));
	}
}

void alarm_enable(void)
{
	if (alarm_enabled == false) {
		alarm_enabled = true;
	}
}

void alarm_disabled(void)
{
	if (alarm_enabled == true) {
		alarm_enabled = false;
	}
}

void alarm_recovery_reset(void)
{
	if (alarm_state == recovering) {
		k_sem_give(&reset_sem);
	}
}

alarm_state_t alarm_get_state(void)
{
	return alarm_state;
}