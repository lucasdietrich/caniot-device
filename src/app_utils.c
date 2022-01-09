#include "app_utils.h"

#include <stdint.h>
#include <avrtos/kernel.h>

void show_uptime(void)
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