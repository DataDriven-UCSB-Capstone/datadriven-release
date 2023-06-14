#include "structures.h"
#include <nrf_modem_at.h>

static inline float out_ev(struct sensor_value *val)
{
	return (val->val1 + (float)val->val2 / 1000000);
}

static void setup_GPIO(void)
{
	int ret;
	for (size_t i = 0; i < 5; i++)
	{
		if (!device_is_ready(system_state.leds[i].port)) {
			return;
		}
		ret = gpio_pin_configure_dt(&system_state.leds[i], GPIO_OUTPUT_ACTIVE);
		if (ret < 0) {
			return;
		}
		ret = gpio_pin_set_dt(&system_state.leds[i], 0);
		if (ret < 0) {
			return;
		}
	}

	for (size_t i = 0; i < 2; i++)
	{

		if (!device_is_ready(system_state.buttons[i].port)) {
			return;
		}
		ret = gpio_pin_configure_dt(&system_state.buttons[i], GPIO_INPUT);
		if (ret < 0) {
			return;
		}
	}
	int can_filter_id;
	const struct can_filter can_extended_filter = { // catch all standard AND extended data frames
		.flags = CAN_FILTER_DATA | CAN_FILTER_IDE,
		.id = 0x18000000,
		.mask = 0x1F000000 // for Brian's car since it floods bus
	};
	const struct can_filter can_standard_filter = { // catch all standard AND extended data frames
		.flags = CAN_FILTER_DATA,
		.id = 0,
		.mask = 0
	};
	if (!device_is_ready(obd_state.device)) {
		LOG_ERR("[FAIL] CAN: Device %s not ready.", obd_state.device->name);
	}
	else
	{
		LOG_INF("[SUCCESS] CAN Device %s is ready.", obd_state.device->name);
	}

	can_filter_id = can_add_rx_filter_msgq(obd_state.device, &obd_msgq, &can_standard_filter);
	LOG_INF("OBD standard filter id: %d", can_filter_id);
	can_filter_id = can_add_rx_filter_msgq(obd_state.device, &obd_msgq, &can_extended_filter);
	LOG_INF("OBD extended filter id: %d", can_filter_id);

	ret = can_start(obd_state.device);
	if (ret != 0) {
		LOG_ERR("[FAIL] Error starting CAN controller [%d]", ret);
	}
	else {
		LOG_INF("[SUCCCESS] Started CAN controller");
	}

	if (!device_is_ready(imu_state.device)) {
		LOG_ERR("[FAIL] IMU: Device %s not ready.", imu_state.device->name);
	}
	else {
		LOG_INF("[SUCCESS] IMU Device %s is ready.", imu_state.device->name);
	}

	struct sensor_value odr_attr;
	/* set accel/gyro sampling frequency to 104 Hz */
	odr_attr.val1 = 104;
	odr_attr.val2 = 0;

	if (sensor_attr_set(imu_state.device, SENSOR_CHAN_ACCEL_XYZ, SENSOR_ATTR_SAMPLING_FREQUENCY, &odr_attr) < 0) {
		printk("Cannot set sampling frequency for accelerometer.\n");
		return;
	}

	if (sensor_attr_set(imu_state.device, SENSOR_CHAN_GYRO_XYZ, SENSOR_ATTR_SAMPLING_FREQUENCY, &odr_attr) < 0) {
		printk("Cannot set sampling frequency for gyro.\n");
		return;
	}

	if (sensor_sample_fetch(imu_state.device) < 0) {
		printk("Sensor sample update error\n");
		return;
	}
}

static void can_tx_callback(const struct device *dev, int error, void *arg)
{
	gpio_pin_toggle_dt(&system_state.leds[0]);
	char *sender = (char *)arg;

	ARG_UNUSED(dev);

	if (error != 0) {
		printf("Callback! error-code: %d\nSender: %s\n",
			error, sender);
	}
}

static void get_ID(void) {
	char response[64];
	int err;
	bool foundVIN = false;
	obd_state.standard_frame.data[0] = 0x02;
	obd_state.standard_frame.data[1] = 0x09;
	obd_state.standard_frame.data[2] = 0x02;
	obd_state.extended_frame.data[0] = 0x02;
	obd_state.extended_frame.data[1] = 0x09;
	obd_state.extended_frame.data[2] = 0x02;

	gpio_pin_set_dt(&system_state.leds[0], 0);

	err = can_send(obd_state.device, &obd_state.standard_frame, K_MSEC(500), can_tx_callback, "Requesting VIN through Standard");
	if (err == 0 && k_msgq_get(&obd_msgq, &obd_state.response_frame, K_MSEC(500)) == 0) {
		gpio_pin_toggle_dt(&system_state.leds[0]);
		foundVIN = true;
		obd_state.standard_frame_flow.id = obd_state.response_frame.id - 8;
		err = can_send(obd_state.device, &obd_state.standard_frame_flow, K_MSEC(100), can_tx_callback, "Sending Flow Control through Standard");
	}
	else
	{
		err = can_send(obd_state.device, &obd_state.extended_frame, K_MSEC(500), can_tx_callback, "Requesting VIN through Extended");
		if (err == 0 && k_msgq_get(&obd_msgq, &obd_state.response_frame, K_MSEC(500)) == 0) {
			gpio_pin_toggle_dt(&system_state.leds[0]);
			foundVIN = true;
			obd_state.extended_frame_flow.id = (obd_state.response_frame.id & 0xFFFF0000) | 
								((obd_state.response_frame.id & 0xFF) << 8) |
								((obd_state.response_frame.id & 0xFF00) >> 8);
			err = can_send(obd_state.device, &obd_state.extended_frame_flow, K_MSEC(100), can_tx_callback, "Sending Flow Control through Extended");
		}
	}
	if (foundVIN)
	{
		memcpy(system_state.VIN, obd_state.response_frame.data + 5, 3);
		err = k_msgq_get(&obd_msgq, &obd_state.response_frame, K_MSEC(500));
		memcpy(system_state.VIN + 3, obd_state.response_frame.data + 1, 7);
		err = k_msgq_get(&obd_msgq, &obd_state.response_frame, K_MSEC(500));
		memcpy(system_state.VIN + 10, obd_state.response_frame.data + 1, 7);
	}
	else {
		LOG_ERR("Failed to read VIN");
		strcpy(system_state.VIN, "_");
	}
	LOG_INF("VIN: %s", system_state.VIN);

	err = nrf_modem_at_cmd(response, sizeof(response), "AT+CGSN=1");
	if (err) {
		LOG_ERR("Failed to read IMEI, err %d", err);
		strcpy(system_state.IMEI, "_");
	}
	else {
		memcpy(system_state.IMEI, response + 8, 15);
	}
	LOG_INF("IMEI: %s", system_state.IMEI);
	gpio_pin_set_dt(&system_state.leds[0], 0);
}
