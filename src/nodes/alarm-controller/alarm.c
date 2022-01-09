#include "alarm.h"

#include <avrtos/kernel.h>

#include <string.h>
#include <stdio.h>
#include <avr/pgmspace.h>

#include "custompcb/board.h"

#include "dev.h"
#include "app_utils.h"

static K_SIGNAL_DEFINE(alarm_process_signal);

#define RELAY_ALARM_MASK	BIT(RELAY1)
// #define RELAY_WARN_MASK		RELAY2

#define INPUT_MASK		(BIT(IN0) | BIT(IN1) | BIT(IN2) | BIT(IN3))
#define INPUT_ACCEPT_FILTER	(0b0000U)

typedef enum { init = 0, ringing, waiting, terminated } siren_state_t; 

static siren_state_t siren_state;

#if DEBUG_ALARM

// multiply by 10 to get real durations
#define RINGING_DURATION			12000LLU
#define CYCLE_WAITING_DURATION			3000LLU
#define SUCCESSIVE_CYCLES_WAITING_DURATION	18000LLU
#define AUTO_RESET_DURATION			1800000LLU
#define MAX_SUCCESSIVE_CYCLES			1U
#define MAX_CYCLES				(MAX_SUCCESSIVE_CYCLES * 3U)

#else

#define RINGING_DURATION			120000LLU
#define CYCLE_WAITING_DURATION			30000LLU
#define SUCCESSIVE_CYCLES_WAITING_DURATION	180000LLU
#define AUTO_RESET_DURATION			1800000LLU
#define MAX_SUCCESSIVE_CYCLES			2U
#define MAX_CYCLES				(MAX_SUCCESSIVE_CYCLES * 3U)

#endif

static uint8_t cycles_counter;

static alarm_mode_t mode = ALARM_MODE_NORMAL;

static bool siren_enabled = false;

static inline void ll_start_siren(void)
{
	siren_enabled = true;

	ll_relays_set_mask(RELAY_ALARM_MASK, RELAY_ALARM_MASK);
}

static inline void ll_stop_siren(void)
{
	ll_relays_set_mask(0, RELAY_ALARM_MASK);

	siren_enabled = false;
}

static void sound(void)
{
	if (mode == ALARM_MODE_NORMAL) {
		ll_start_siren();
	}
}

static void stop_sound(void)
{
	if (mode == ALARM_MODE_NORMAL) {
		ll_stop_siren();
	}
}

static void siren_reset(void)
{
	ll_stop_siren();

	cycles_counter = 0;
	siren_state = init;
	siren_enabled = false;
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

K_SEM_DEFINE(recover_sem, 0, 1);

static bool alarm_enabled;
static alarm_state_t alarm_state;
static bool alarm_initialized = false;

static void alarm_reset(void)
{
	alarm_state = inactive;	
	siren_reset();
}


void alarm_init(void)
{
	alarm_reset();

	mode = ALARM_MODE_NORMAL;

	alarm_enabled = true; /* TODO set to false */

	/* TODO read alarm_enabled from config (EEPROM) */

	alarm_initialized = true;
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
	if (alarm_state != inactive && alarm_enabled == false) {
		alarm_reset();
	}

	/* state machine */
	switch (alarm_state) {
	case inactive:
	{
		if (alarm_enabled && alarm_initialized) {
			if (verify_status()) {
				siren_reset();
				set_state(observing);
			} else {
				/* if alarm has been enabled but signals are not valid,
				 * disable the alarm */
				alarm_enabled = false;
			}
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
		if ((k_sem_take(&recover_sem, K_NO_WAIT) == 0) ||
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
		if (k_poll_signal(&alarm_process_signal, K_MSEC(500)) == 0) {
			K_SIGNAL_SET_UNREADY(&alarm_process_signal);
		}

		if (alarm_state_machine() != 0) {
			__fault(K_FAULT); /* what to do ? */
		}
	}
}

void alarm_enable(void)
{
	if (alarm_enabled == false) {
		alarm_enabled = true;
	}

	k_signal_raise(&alarm_process_signal, 1);
}

void alarm_disable(void)
{
	if (alarm_enabled == true) {
		alarm_enabled = false;
	}

	k_signal_raise(&alarm_process_signal, 0);
}

void alarm_recover(void)
{
	if (alarm_state == recovering) {
		k_sem_give(&recover_sem);
	}

	k_signal_raise(&alarm_process_signal, 2);
}

alarm_state_t alarm_get_state(void)
{
	return alarm_state;
}

void alarm_set_mode(alarm_mode_t new_mode)
{
	if (mode == new_mode) {
		return;
	}

	switch (new_mode) {
	case ALARM_MODE_NORMAL:
		alarm_reset(); /* reset alarm */
		break;
	case ALARM_MODE_SILENT:
		stop_sound();
		break;
	default:
		return;
	}

	mode = new_mode;

	k_signal_raise(&alarm_process_signal, 3);
}

alarm_mode_t alarm_get_mode(void)
{
	return mode;
}

void alarm_test_siren_start(void)
{
	if (alarm_state == inactive) {
		sound();
	}
}

void alarm_test_siren_stop(void)
{
	if (alarm_state == inactive) {
		stop_sound();
	}
}
void alarm_test_siren_toggle(void)
{
	if (alarm_state != inactive) {
		return;
	}

	if (siren_enabled) {
		stop_sound();
	} else {
		sound();
	}
}

bool alarm_get_siren_state(void)
{
	return siren_enabled;
}