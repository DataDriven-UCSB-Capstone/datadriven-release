#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/can.h>
#include <zephyr/drivers/sensor.h>

struct SYSTEM_STATE {
	const struct gpio_dt_spec leds[5];
	const struct gpio_dt_spec buttons[2];
	uint8_t VIN[20];
	uint8_t IMEI[20];
	uint8_t ID[60];
};

struct OBD_STATE {
	enum {UNSUPPORTED = 0,
			STANDARD = 1,
			EXTENDED = 2,
			BOTH = 3  // 1 | 2 = 3 -> BOTH = STANDARD | EXTENDED
	} PID_supported[256];
	struct can_frame standard_frame;
	struct can_frame standard_frame_flow;
	struct can_frame extended_frame;
	struct can_frame extended_frame_flow;
	struct can_frame response_frame;
	enum can_state current_can_state;
	struct can_bus_err_cnt current_can_error_count;
	const struct device *const device;
};

struct IMU_STATE {
	const struct device *const device;
	struct sensor_value raw_accel_x;
	struct sensor_value raw_accel_y;
	struct sensor_value raw_accel_z;
	struct sensor_value raw_gyro_x;
	struct sensor_value raw_gyro_y;
	struct sensor_value raw_gyro_z;
	union {
		float f;
		uint8_t b[4];
	} accel_x, accel_y, accel_z, gyro_x, gyro_y, gyro_z;

};

static struct SYSTEM_STATE system_state = {
	.leds = {
		GPIO_DT_SPEC_GET(DT_ALIAS(led0), gpios), // CAN TX/RX LED
		GPIO_DT_SPEC_GET(DT_ALIAS(led1), gpios), // IMU LED
		GPIO_DT_SPEC_GET(DT_ALIAS(led2), gpios), // GNSS LED
		GPIO_DT_SPEC_GET(DT_ALIAS(led3), gpios), // UDP LED
		GPIO_DT_SPEC_GET(DT_ALIAS(led4), gpios)  // [TODO] LED
	},
	.buttons = {
		GPIO_DT_SPEC_GET(DT_ALIAS(sw0), gpios),
		GPIO_DT_SPEC_GET(DT_ALIAS(sw1), gpios)
	},
	.VIN = "",
	.IMEI = "",
	.ID = ""
};

static struct OBD_STATE obd_state = {
	.PID_supported = {},
	.standard_frame = {
		.flags = 0,
		.id = 0x7DF, // broadcast address for standard frames
		.dlc = 8,
	},
	.standard_frame_flow = {
		.flags = 0,
		.dlc = 8,
		.data = {0x30, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}
	},
	.extended_frame = {
		.flags = CAN_FRAME_IDE,
		.id = 0x18DB33F1, // broadcast address for extended frames
		.dlc = 8,
	},
	.extended_frame_flow = {
		.flags = CAN_FRAME_IDE,
		.dlc = 8,
		.data = {0x30, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}
	},
	.device = DEVICE_DT_GET(DT_CHOSEN(zephyr_canbus))
};
CAN_MSGQ_DEFINE(obd_msgq, 5);

static struct IMU_STATE imu_state = {
	.device = DEVICE_DT_GET(DT_CHOSEN(zephyr_imu))
};
