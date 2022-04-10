#include "can.h"

#include <avrtos/kernel.h>

#include <mcp2515_can.h>
#include <mcp2515_can_dfs.h>

#include <device.h>

#include "dev.h"

#define K_MODULE_CAN    0x21
#define K_MODULE K_MODULE_CAN

#define SPI_CS_PIN  10

#if CONFIG_CAN_CLOCKSET_16MHZ
#	define CAN_CLOCKSET MCP_16MHz
#else
#	define CAN_CLOCKSET MCP_8MHz
#endif

#define CAN_SPEEDSET CAN_500KBPS

static mcp2515_can can(SPI_CS_PIN);

/* maybe unecessary */
static K_MUTEX_DEFINE(can_mutex_if);

void can_init(void)
{
	__ASSERT_INTERRUPT();

        k_mutex_lock(&can_mutex_if, K_FOREVER);

        while (CAN_OK != can.begin(CAN_SPEEDSET, CAN_CLOCKSET)) {
                printf_P(PSTR("can init failed\n"));
                k_sleep(K_MSEC(500));
        }

	const unsigned long mask = caniot_device_get_mask();
	const unsigned long filter_self = caniot_device_get_filter(did);
	const unsigned long filter_broadcast = caniot_device_get_filter_broadcast(did);

        can.init_Mask(0, CAN_STDID, mask);
	can.init_Filt(0, CAN_STDID, filter_self);
	can.init_Filt(1, CAN_STDID, filter_self);

	can.init_Mask(1, CAN_STDID, mask);
	can.init_Filt(2, CAN_STDID, filter_broadcast);
	can.init_Filt(3, CAN_STDID, filter_broadcast);
	can.init_Filt(4, CAN_STDID, filter_broadcast);
	can.init_Filt(5, CAN_STDID, filter_broadcast);

#if CONFIG_CAN_INT == 0
	/* configure interrupt on falling on INT0 */
        EICRA |= 1 << ISC01;
        EICRA &= ~(1 << ISC00);
        EIMSK |= 1 << INT0;
#else
#	error MCP2515 on INT1 not supported
#endif

	k_mutex_unlock(&can_mutex_if);
}

ISR(INT0_vect)
{
	trigger_process();

#if DEBUG_INT
	usart_transmit('%');
#endif 
}

int can_recv(can_message *msg)
{
	__ASSERT_NOTNULL(msg);

	int rc = k_mutex_lock(&can_mutex_if, K_MSEC(100));
	if (rc == 0) {
#if DEBUG_DRV_CAN
		printf_P(PSTR("[R"));
#endif /* DEBUG_DRV_CAN */
		if (can.checkReceive() == CAN_MSGAVAIL) {
			uint8_t isext, rtr;
			rc = can.readMsgBufID(can.readRxTxStatus(),
					      (unsigned long *)&msg->id, &isext, &rtr,
					      &msg->len, msg->buf);
			if (rc == 0) {
				msg->isext = isext ? CAN_EXTID : CAN_STDID;
				msg->rtr = rtr ? 1 : 0;
			}
		} else {
			rc = -EAGAIN;
		}
#if DEBUG_DRV_CAN
		usart_transmit(']');
#endif /* DEBUG_DRV_CAN */
		k_mutex_unlock(&can_mutex_if);
	} else {
#if DEBUG_DRV_CAN
		printf_P(PSTR("mutex lock failed\n"));
#endif /* DEBUG_DRV_CAN */
	}
	return rc;
}

static int can_send(can_message *msg)
{
	__ASSERT_NOTNULL(msg);

	int rc = k_mutex_lock(&can_mutex_if, K_MSEC(100));
	if (rc == 0) {
#if DEBUG_DRV_CAN
		printf_P(PSTR("[T"));
#endif /* DEBUG_DRV_CAN */
		rc = can.sendMsgBuf(msg->id, msg->isext, msg->rtr, msg->len,
				    msg->buf, true);
#if DEBUG_DRV_CAN
		usart_transmit(']');
#endif /* DEBUG_DRV_CAN */
		k_mutex_unlock(&can_mutex_if);
	}
	return rc;
}

#define CAN_MSGQ_SIZE 2
static uint8_t buf[CAN_MSGQ_SIZE * sizeof(can_message)];
K_MSGQ_DEFINE(txq, buf, sizeof(can_message), CAN_MSGQ_SIZE);

static void can_tx_entry(void *arg);

K_THREAD_DEFINE(can_tx_thread, can_tx_entry, 110, K_COOPERATIVE, NULL, 'C');

static void can_tx_entry(void *arg)
{
	can_message msg;
	while (1) {
		if (k_msgq_get(&txq, &msg, K_FOREVER) == 0) {
			// can_print_msg(&msg);

			can_send(&msg);
		}
	}
}

int can_txq_message(can_message *msg)
{
	return k_msgq_put(&txq, msg, K_NO_WAIT);
}

// print can_message
void can_print_msg(can_message *msg)
{
	printf_P(PSTR("id: %08lx, isext: %d, rtr: %d, len: %d : "),
		 msg->id, msg->isext, msg->rtr, msg->len);
		 
	for (int i = 0; i < msg->len; i++) {
		printf_P(PSTR("%02x "), msg->buf[i]);
	}
	printf_P(PSTR("\n"));
}