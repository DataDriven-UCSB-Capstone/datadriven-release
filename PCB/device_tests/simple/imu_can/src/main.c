/*
 * Copyright (c) 2018 STMicroelectronics
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/drivers/can.h>
#include <stdio.h>
#include <zephyr/sys/util.h>

static inline float out_ev(struct sensor_value *val)
{
	return (val->val1 + (float)val->val2 / 1000000);
}

static int print_samples;
static int lsm6dsl_trig_cnt;

static struct sensor_value accel_x_out, accel_y_out, accel_z_out;
static struct sensor_value gyro_x_out, gyro_y_out, gyro_z_out;


static void lsm6dsl_trigger_handler(const struct device *dev,
				    const struct sensor_trigger *trig)
{
	static struct sensor_value accel_x, accel_y, accel_z;
	static struct sensor_value gyro_x, gyro_y, gyro_z;
	lsm6dsl_trig_cnt++;

	sensor_sample_fetch_chan(dev, SENSOR_CHAN_ACCEL_XYZ);
	sensor_channel_get(dev, SENSOR_CHAN_ACCEL_X, &accel_x);
	sensor_channel_get(dev, SENSOR_CHAN_ACCEL_Y, &accel_y);
	sensor_channel_get(dev, SENSOR_CHAN_ACCEL_Z, &accel_z);

	/* lsm6dsl gyro */
	sensor_sample_fetch_chan(dev, SENSOR_CHAN_GYRO_XYZ);
	sensor_channel_get(dev, SENSOR_CHAN_GYRO_X, &gyro_x);
	sensor_channel_get(dev, SENSOR_CHAN_GYRO_Y, &gyro_y);
	sensor_channel_get(dev, SENSOR_CHAN_GYRO_Z, &gyro_z);

	if (print_samples) {
		print_samples = 0;

		accel_x_out = accel_x;
		accel_y_out = accel_y;
		accel_z_out = accel_z;

		gyro_x_out = gyro_x;
		gyro_y_out = gyro_y;
		gyro_z_out = gyro_z;
	}

}


const struct device *const can_dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_canbus));

enum can_state current_can_state;
struct can_bus_err_cnt current_can_err_cnt;

CAN_MSGQ_DEFINE(obd_msgq, 10);

void can_tx_callback(const struct device *dev, int error, void *arg)
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

void can_state_change_callback(const struct device *dev, enum can_state state,
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
	int cnt = 0;
	char out_str[64];
	struct sensor_value odr_attr;
	const struct device *const lsm6dsl_dev = DEVICE_DT_GET_ONE(st_lsm6dsl);

	if (!device_is_ready(lsm6dsl_dev)) {
		printk("sensor: device not ready.\n");
		return;
	}

	/* set accel/gyro sampling frequency to 104 Hz */
	odr_attr.val1 = 104;
	odr_attr.val2 = 0;

	if (sensor_attr_set(lsm6dsl_dev, SENSOR_CHAN_ACCEL_XYZ,
			    SENSOR_ATTR_SAMPLING_FREQUENCY, &odr_attr) < 0) {
		printk("Cannot set sampling frequency for accelerometer.\n");
		return;
	}

	if (sensor_attr_set(lsm6dsl_dev, SENSOR_CHAN_GYRO_XYZ,
			    SENSOR_ATTR_SAMPLING_FREQUENCY, &odr_attr) < 0) {
		printk("Cannot set sampling frequency for gyro.\n");
		return;
	}


	struct sensor_trigger trig;

	trig.type = SENSOR_TRIG_DATA_READY;
	trig.chan = SENSOR_CHAN_ACCEL_XYZ;

	if (sensor_trigger_set(lsm6dsl_dev, &trig, lsm6dsl_trigger_handler) != 0) {
		printk("Could not set sensor type and channel\n");
		return;
	}

	if (sensor_sample_fetch(lsm6dsl_dev) < 0) {
		printk("Sensor sample update error\n");
		return;
	}

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

	can_set_state_change_callback(can_dev, can_state_change_callback, NULL);

	printk("Finished init.\n");

	struct can_frame obd_response_frame;

	obd_standard_frame.data[0] = 0x02;
	obd_standard_frame.data[1] = 0x01;
	obd_standard_frame.data[2] = 0x00;
	obd_extended_frame.data[0] = 0x02;
	obd_extended_frame.data[1] = 0x01;
	obd_extended_frame.data[2] = 0x00;

	while (1) {
		printk("LSM6DSL sensor samples:\n\n");

		/* lsm6dsl accel */
		sprintf(out_str, "accel x:%f ms/2 y:%f ms/2 z:%f ms/2",
							  out_ev(&accel_x_out),
							  out_ev(&accel_y_out),
							  out_ev(&accel_z_out));
		printk("%s\n", out_str);

		/* lsm6dsl gyro */
		sprintf(out_str, "gyro x:%f dps y:%f dps z:%f dps",
							   out_ev(&gyro_x_out),
							   out_ev(&gyro_y_out),
							   out_ev(&gyro_z_out));
		printk("%s\n", out_str);

		printk("loop:%d trig_cnt:%d\n\n", ++cnt, lsm6dsl_trig_cnt);

		print_samples = 1;

		can_send(can_dev, &obd_standard_frame, K_FOREVER, can_tx_callback, "Standard frame");
		can_send(can_dev, &obd_extended_frame, K_FOREVER, can_tx_callback, "Extended frame");
		k_sleep(K_MSEC(500));
		while ((ret = k_msgq_get(&obd_msgq, &obd_response_frame, K_MSEC(100))) == 0) {
			printk("Received from %X:", obd_response_frame.id);
			for (int i = 0; i < obd_response_frame.dlc; i++) {
				printk(" %02X", obd_response_frame.data[i]);
			}
			printk("\n");
		}
		k_sleep(K_MSEC(500));
	}
}
