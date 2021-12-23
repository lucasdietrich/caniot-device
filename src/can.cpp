#include "can.h"

#include <avrtos/kernel.h>

#include <mcp2515_can.h>
#include <mcp2515_can_dfs.h>

#define K_MODULE_CAN    0x21
#define K_MODULE K_MODULE_CAN

#define SPI_CS_PIN  10
#define CAN_INT 0
#define CAN_CLOCKSET MCP_16MHz
#define CAN_SPEEDSET CAN_500KBPS

static mcp2515_can can(SPI_CS_PIN);

K_SIGNAL_DEFINE(can_sig_rx);

static void can_rx_entry(void *context);

K_THREAD_DEFINE(can_rx_thread, can_rx_entry, 0x64, K_COOPERATIVE, NULL, 'R');

/* maybe unecessary */
K_MUTEX_DEFINE(can_mutex_if);

void can_init(void)
{
        k_mutex_lock(&can_mutex_if, K_FOREVER);

        while (CAN_OK != can.begin(CAN_SPEEDSET, CAN_CLOCKSET)) {
                printf_P(PSTR("can begin failed retry ..\n"));
                k_sleep(K_MSEC(500));
        }

	// TODO configure masks and filters
        // can.init_Mask(0, CAN_EXTID, cfg->masks[0]);
        // if (cfg->masks[0]) {
        //         can.init_Filt(0, CAN_EXTID, cfg->filters[0]);
        //         can.init_Filt(1, CAN_EXTID, cfg->filters[1]);
        // }

        // can.init_Mask(1, CAN_EXTID, cfg->masks[1]);
        // if (cfg->masks[0]) {
        //         can.init_Filt(2, CAN_EXTID, cfg->filters[2]);
        //         can.init_Filt(3, CAN_EXTID, cfg->filters[3]);
        //         can.init_Filt(4, CAN_EXTID, cfg->filters[4]);
        //         can.init_Filt(5, CAN_EXTID, cfg->filters[5]);
        // }

	/* configure interrupt on falling on INT0 */
        EICRA |= 1 << ISC01;
        EICRA &= ~(1 << ISC00);
        EIMSK |= 1 << INT0;

	k_mutex_unlock(&can_mutex_if);
}

ISR(INT0_vect)
{
	k_signal_raise(&can_sig_rx, 0u);
}

static uint8_t can_recv(can_message *msg)
{
	__ASSERT_NOTNULL(msg);

	uint8_t rc = k_mutex_lock(&can_mutex_if, K_MSEC(100));
	if (rc == 0) {
		rc = -1;
		if (can.checkReceive() == CAN_MSGAVAIL) {
			uint8_t isext, rtr;
			rc = can.readMsgBufID(can.readRxTxStatus(),
					      (unsigned long *)&msg->id, &isext, &rtr,
					      &msg->len, msg->buf);
			if (rc == 0) {
				msg->isext = isext ? CAN_EXTID : CAN_STDID;
				msg->rtr = rtr ? 1 : 0;
			}
		}
		k_mutex_unlock(&can_mutex_if);
	}
	return rc;
}

uint8_t can_send(can_message *msg)
{
	__ASSERT_NOTNULL(msg);

	uint8_t rc = k_mutex_lock(&can_mutex_if, K_MSEC(100));
	if (rc == 0) {
		rc = can.sendMsgBuf(msg->id, msg->isext, msg->rtr, msg->len,
				    msg->buf, true);

		k_mutex_unlock(&can_mutex_if);
	}
	return rc;
}

static void can_rx_entry(void *context)
{
	static can_message msg;

	for (;;) {
		k_poll_signal(&can_sig_rx, K_FOREVER);
		can_sig_rx.flags = K_POLL_STATE_NOT_READY;

		while (can_recv(&msg) == 0) {
			can_print_msg(&msg);

			/* yield to allow tx thread to process
			 * loopback packet if any */
			k_yield();
		}
	}
}

// print can_message
void can_print_msg(can_message *msg)
{
	printf_P(PSTR("id: %08lx, isext: %d, rtr: %d, len: %d\n"),
		 msg->id, msg->isext, msg->rtr, msg->len);
	for (int i = 0; i < msg->len; i++) {
		printf_P(PSTR("%02x "), msg->buf[i]);
	}
	printf_P(PSTR("\n"));
}