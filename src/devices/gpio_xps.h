#ifndef _GPIO_XPS_H_
#define _GPIO_XPS_H_

#include "gpio_pulse.h"

#include <bsp/bsp.h>
#include <caniot/datatype.h>

struct xps_context {
    pin_descr_t descr;

    uint8_t reset_state : 1;

#if CONFIG_GPIO_PULSE_SUPPORT
    struct pulse_event *pev;
#endif /* CONFIG_GPIO_PULSE_SUPPORT */
};

#define XPS_CONTEXT_INIT(_descr, _reset_state)                                           \
    {                                                                                    \
        .descr = _descr, .reset_state = _reset_state,                                    \
    }

int command_xps(struct xps_context *xpsc,
                caniot_complex_digital_cmd_t cmd,
                uint32_t duration_ms);

#endif /* _GPIO_XPS_H_ */