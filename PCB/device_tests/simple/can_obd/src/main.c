/*
 * Copyright (c) 2018 Alexander Wachter
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/drivers/can.h>

#define SLEEP_TIME K_MSEC(250)

const struct device *const can_dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_canbus));

enum can_state current_can_state;
struct can_bus_err_cnt current_can_err_cnt;

CAN_MSGQ_DEFINE(obd_msgq, 10);

void tx_irq_callback(const struct device *dev, int error, void *arg)
{
	char *sender = (char *)arg;

	ARG_UNUSED(dev);

	if (error != 0) {
		printk("Callback! error-code: %d\nSender: %s\n",
		       error, sender);
	}
}

char* can_state_to_str(enum can_state state)
{
	switch (state) {
	case CAN_STATE_ERROR_ACTIVE:
		return "error-active";
	case CAN_STATE_ERROR_WARNING:
		return "error-warning";
	case CAN_STATE_ERROR_PASSIVE:
		return "error-passive";
	case CAN_STATE_BUS_OFF:
		return "bus-off";
	case CAN_STATE_STOPPED:
		return "stopped";
	default:
		return "unknown";
	}
}

void state_change_callback(const struct device *dev, enum can_state state,
			   struct can_bus_err_cnt err_cnt, void *user_data)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(user_data);
	current_can_state = state;
	current_can_err_cnt = err_cnt;
	printk("State Change ISR\nstate: %s\n"
			"rx error count: %d\n"
	 		"tx error count: %d\n",
			can_state_to_str(current_can_state),
			current_can_err_cnt.rx_err_cnt, current_can_err_cnt.tx_err_cnt);

	if (current_can_state == CAN_STATE_BUS_OFF) {
		printk("Recover from bus-off\n");

		if (can_recover(can_dev, K_MSEC(1000)) != 0) {
			printk("Recovery timed out\n");
		}
	}
}

#define OBD_BROADCAST_STD_ID 0x7DF
#define OBD_BROADCAST_EXT_ID 0x18DB33F1

void main(void)
{
	const struct can_filter wildcard_filter = {
		.flags = CAN_FILTER_DATA | CAN_FILTER_IDE,
		.id = CAN_EXT_ID_MASK,
		.mask = 0
	};
	struct can_frame obd_standard_frame = {
		.flags = 0,
		.id = OBD_BROADCAST_STD_ID,
		.dlc = 8
	};
	struct can_frame obd_extended_frame = {
		.flags = CAN_FRAME_IDE,
		.id = OBD_BROADCAST_EXT_ID,
		.dlc = 8
	};
	int ret;

	if (!device_is_ready(can_dev)) {
		printk("CAN: Device %s not ready.\n", can_dev->name);
		return;
	}

	ret = can_start(can_dev);
	if (ret != 0) {
		printk("Error starting CAN controller [%d]", ret);
		return;
	}

	ret = can_add_rx_filter_msgq(can_dev, &obd_msgq, &wildcard_filter);
	if (ret == -ENOSPC) {
		printk("Error, no filter available!\n");
		return;
	}

	printk("Wildcard Filter filter ID: %d\n", ret);

	can_set_state_change_callback(can_dev, state_change_callback, NULL);

	printk("Finished init.\n");
	struct can_frame obd_response_frame;

	obd_standard_frame.data[0] = 0x02;
	obd_standard_frame.data[1] = 0x01;
	obd_standard_frame.data[2] = 0x00;
	obd_extended_frame.data[0] = 0x02;
	obd_extended_frame.data[1] = 0x01;
	obd_extended_frame.data[2] = 0x00;

	while (1) {
		can_send(can_dev, &obd_standard_frame, K_FOREVER, tx_irq_callback, "Standard frame");
		can_send(can_dev, &obd_extended_frame, K_FOREVER, tx_irq_callback, "Extended frame");
		k_sleep(SLEEP_TIME);
		while ((ret = k_msgq_get(&obd_msgq, &obd_response_frame, K_MSEC(100))) == 0) {
			printk("Received from %X:", obd_response_frame.id);
			for (int i = 0; i < obd_response_frame.dlc; i++) {
				printk(" %02X", obd_response_frame.data[i]);
			}
			printk("\n");
		}
		k_sleep(SLEEP_TIME);
	}
}
