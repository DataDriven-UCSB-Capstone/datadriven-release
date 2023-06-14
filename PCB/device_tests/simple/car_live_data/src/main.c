/*
 * PRESS SW2 TO FETCH VEHICLE DATA AND CREATE A PACKET
 */

#include <zephyr/kernel.h>
#include <zephyr/drivers/can.h>
#include <zephyr/drivers/gpio.h>
#include <nrf_modem_at.h>

#define LED0_NODE DT_ALIAS(led0)
#define LED1_NODE DT_ALIAS(led1)
#define LED2_NODE DT_ALIAS(led2)
#define LED3_NODE DT_ALIAS(led3)
#define LED4_NODE DT_ALIAS(led4)
#define BTN0_NODE DT_ALIAS(sw0)
#define BTN1_NODE DT_ALIAS(sw1)

#define OBD_BROADCAST_STD_ID 0x7DF
#define OBD_BROADCAST_EXT_ID 0x18DB33F1

static const struct gpio_dt_spec my_led0 = GPIO_DT_SPEC_GET(LED0_NODE, gpios);
static const struct gpio_dt_spec my_led1 = GPIO_DT_SPEC_GET(LED1_NODE, gpios);
static const struct gpio_dt_spec my_led2 = GPIO_DT_SPEC_GET(LED2_NODE, gpios);
static const struct gpio_dt_spec my_led3 = GPIO_DT_SPEC_GET(LED3_NODE, gpios);
static const struct gpio_dt_spec my_led4 = GPIO_DT_SPEC_GET(LED4_NODE, gpios);
static const struct gpio_dt_spec my_button0 = GPIO_DT_SPEC_GET(BTN0_NODE, gpios);
static const struct gpio_dt_spec my_button1 = GPIO_DT_SPEC_GET(BTN1_NODE, gpios);
static const struct gpio_dt_spec my_leds[] = {
	GPIO_DT_SPEC_GET(LED0_NODE, gpios),
	GPIO_DT_SPEC_GET(LED1_NODE, gpios),
	GPIO_DT_SPEC_GET(LED2_NODE, gpios),
	GPIO_DT_SPEC_GET(LED3_NODE, gpios),
	GPIO_DT_SPEC_GET(LED4_NODE, gpios)
};
static const struct gpio_dt_spec my_buttons[] = {
	GPIO_DT_SPEC_GET(BTN0_NODE, gpios),
	GPIO_DT_SPEC_GET(BTN1_NODE, gpios)
};

const struct device *const can_dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_canbus));
enum can_state current_can_state;
struct can_bus_err_cnt current_can_err_cnt;
CAN_MSGQ_DEFINE(obd_msgq, 10);
char IMEI[16] = {0};
char VIN[18] = {0};
char UDP_Packet[1000] = {0};
const char* UDP_separator = "$";

enum OBD_TYPE {UNSUPPORTED = 0,
				STANDARD = 1,
				EXTENDED = 2,
				BOTH = 3} PID_supported[256] = {UNSUPPORTED}; // 1 | 2 = 3

void can_tx_callback(const struct device *dev, int error, void *arg)
{
	gpio_pin_toggle_dt(&my_led0);
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

void setup_gpios() {
	int ret;
	for (size_t i = 0; i < 5; i++)
	{
		if (!device_is_ready(my_leds[i].port)) {
			return;
		}
		ret = gpio_pin_configure_dt(&my_leds[i], GPIO_OUTPUT_ACTIVE);
		if (ret < 0) {
			return;
		}
		ret = gpio_pin_set_dt(&my_leds[i], 0);
		if (ret < 0) {
			return;
		}
	}

	for (size_t i = 0; i < 2; i++)
	{

		if (!device_is_ready(my_buttons[i].port)) {
			return;
		}
		ret = gpio_pin_configure_dt(&my_buttons[i], GPIO_INPUT);
		if (ret < 0) {
			return;
		}
	}


	if (!device_is_ready(can_dev)) {
		printk("CAN: Device %s not ready.\n", can_dev->name);
		return;
	}
	ret = can_start(can_dev);
	if (ret != 0) {
		printk("Error starting CAN controller [%d]", ret);
		return;
	}
	const struct can_filter wildcard_filter = {
		.flags = CAN_FILTER_DATA | CAN_FILTER_IDE,
		.id = CAN_EXT_ID_MASK,
		.mask = 0
	};
	ret = can_add_rx_filter_msgq(can_dev, &obd_msgq, &wildcard_filter);
	if (ret == -ENOSPC) {
		printk("Error, no filter available!\n");
		return;
	}
	printk("Wildcard Filter filter ID: %d\n", ret);
	can_set_state_change_callback(can_dev, can_state_change_callback, NULL);
}

void getIMEI() {
	char response[64];
	int err;
	err = nrf_modem_at_cmd(response, sizeof(response), "AT+CGSN=1");
	if (err) {
		printk("Failed to read IMEI, err %d\n", err);
		return;
	}
	memcpy(IMEI, response + 8, 15);
}

void getVIN() {
	int err;
	struct can_frame obd_response_frame;
	struct can_frame obd_request_std_frame = {
		.flags = 0,
		.id = OBD_BROADCAST_STD_ID,
		.dlc = 8
	};
	struct can_frame obd_request_ext_frame = {
		.flags = CAN_FRAME_IDE,
		.id = OBD_BROADCAST_EXT_ID,
		.dlc = 8
	};
	struct can_frame flow_std_frame = {
		.flags = 0,
		.dlc = 8,
		.data = {0x30, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}
	};
	struct can_frame flow_ext_frame = {
		.flags = CAN_FRAME_IDE,
		.dlc = 8,
		.data = {0x30, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}
	};

	obd_request_std_frame.data[0] = 0x02; // 2 bytes of data
	obd_request_std_frame.data[1] = 0x09; // Service 09 - Vehicle Info
	obd_request_std_frame.data[2] = 0x02; // PID 02 - VIN

	obd_request_ext_frame.data[0] = 0x02; // 2 bytes of data
	obd_request_ext_frame.data[1] = 0x09; // Service 09 - Vehicle Info
	obd_request_ext_frame.data[2] = 0x02; // PID 09 - VIN

	bool foundVIN = false;

	err = can_send(can_dev, &obd_request_std_frame, K_FOREVER, can_tx_callback, "Requesting VIN through Standard");
	if ((err = k_msgq_get(&obd_msgq, &obd_response_frame, K_MSEC(500))) == 0) {
		foundVIN = true;
		flow_std_frame.id = obd_response_frame.id - 8;
		err = can_send(can_dev, &flow_std_frame, K_FOREVER, can_tx_callback, "Sending Flow Control through Standard");
	}
	else
	{
		err = can_send(can_dev, &obd_request_ext_frame, K_FOREVER, can_tx_callback, "Requesting VIN through Extended");
		if ((err = k_msgq_get(&obd_msgq, &obd_response_frame, K_MSEC(500))) == 0) {
			foundVIN = true;
			flow_ext_frame.id = (obd_response_frame.id & 0xFFFF0000) | ((obd_response_frame.id & 0xFF) << 8) | ((obd_response_frame.id & 0xFF00) >> 8);
			err = can_send(can_dev, &flow_ext_frame, K_FOREVER, can_tx_callback, "Sending Flow Control through Extended");
		}
	}

	if (foundVIN)
	{
		memcpy(VIN, obd_response_frame.data + 5, 3);
		err = k_msgq_get(&obd_msgq, &obd_response_frame, K_MSEC(500));
		memcpy(VIN + 3, obd_response_frame.data + 1, 7);
		err = k_msgq_get(&obd_msgq, &obd_response_frame, K_MSEC(500));
		memcpy(VIN + 10, obd_response_frame.data + 1, 7);
	}
}

void getPIDs() {
	int ret;
	struct can_frame obd_response_frame;
	struct can_frame obd_standard_frame = {
		.flags = 0,
		.id = OBD_BROADCAST_STD_ID,
		.dlc = 8,
		.data = {0x02, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}
	};
	struct can_frame obd_extended_frame = {
		.flags = CAN_FRAME_IDE,
		.id = OBD_BROADCAST_EXT_ID,
		.dlc = 8,
		.data = {0x02, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}
	};
	for (int checkPID = 0; checkPID < 0xFF && PID_supported[checkPID] != UNSUPPORTED; checkPID += 0x20)
	{
		if ((PID_supported[checkPID] & STANDARD) == STANDARD) {
			obd_standard_frame.data[2] = checkPID;
			ret = can_send(can_dev, &obd_standard_frame, K_FOREVER, can_tx_callback, "Requesting Standard PID");
		}
		if ((PID_supported[checkPID] & EXTENDED) == EXTENDED) {
			obd_extended_frame.data[2] = checkPID;
			ret = can_send(can_dev, &obd_extended_frame, K_FOREVER, can_tx_callback, "Requesting Extended PID");
		}
		while ((ret = k_msgq_get(&obd_msgq, &obd_response_frame, K_MSEC(10))) == 0) { // can reduce K_MSEC() to very low, NO_WAIT is too fast
			gpio_pin_toggle_dt(&my_led1);
			enum OBD_TYPE current_type = ((obd_response_frame.flags & CAN_FRAME_IDE) == CAN_FRAME_IDE) ? EXTENDED : STANDARD;
			for (int i = 0; i < 8; i++)
			{
				if (obd_response_frame.data[3] & (1 << i))
				{
					PID_supported[checkPID + (8 - i)] |= current_type;
				}
				if (obd_response_frame.data[4] & (1 << i))
				{
					PID_supported[checkPID + (16 - i)] |= current_type;
				}
				if (obd_response_frame.data[5] & (1 << i))
				{
					PID_supported[checkPID + (24 - i)] |= current_type;
				}
				if (obd_response_frame.data[6] & (1 << i))
				{
					PID_supported[checkPID + (32 - i)] |= current_type;
				}
			}
		}
	}
}
void main(void)
{
	setup_gpios();
	printk("Finished init.\n");
	getIMEI();
	getVIN();
	printk("VIN: %s\nIMEI: %s\n", VIN, IMEI);

	PID_supported[0] = BOTH;
	getPIDs();
	printk("Supported PIDS:");
	for (size_t currentPID = 0; currentPID < 256; currentPID++)
	{
		if (PID_supported[currentPID] != UNSUPPORTED)
		{
			printk(" %02X", currentPID);
		}
	}
	printk("\n");

	struct can_frame obd_response_frame;
	struct can_frame obd_standard_frame = {
		.flags = 0,
		.id = OBD_BROADCAST_STD_ID,
		.dlc = 8,
		.data = {0x02, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}
	};
	struct can_frame obd_extended_frame = {
		.flags = CAN_FRAME_IDE,
		.id = OBD_BROADCAST_EXT_ID,
		.dlc = 8,
		.data = {0x02, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}
	};
	struct can_frame flow_std_frame = {
		.flags = 0,
		.dlc = 8,
		.data = {0x30, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}
	};
	struct can_frame flow_ext_frame = {
		.flags = CAN_FRAME_IDE,
		.dlc = 8,
		.data = {0x30, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}
	};
	strcat(UDP_Packet, UDP_separator);
	strcat(UDP_Packet, VIN);
	strcat(UDP_Packet, UDP_separator);
	strcat(UDP_Packet, IMEI);
	strcat(UDP_Packet, UDP_separator);
	// *INSERT GNSS HERE*

	unsigned int PREFIX_LENGTH = strlen(UDP_Packet);

	unsigned int currentLen = PREFIX_LENGTH;
	int ret;
	while (true) {
		if (gpio_pin_get_dt(&my_button0) == 1) {
			gpio_pin_set_dt(&my_led4, 1);
			for (size_t currentPID = 0x00; currentPID < 0xFF; currentPID++) {
				if ((PID_supported[currentPID] & STANDARD) == STANDARD) {
					obd_standard_frame.data[2] = currentPID;
					can_send(can_dev, &obd_standard_frame, K_FOREVER, can_tx_callback, "Standard frame");
				}
				if ((PID_supported[currentPID] & EXTENDED) == EXTENDED) {
					obd_extended_frame.data[2] = currentPID;
					can_send(can_dev, &obd_extended_frame, K_FOREVER, can_tx_callback, "Extended frame");
				}
				if (PID_supported[currentPID] != UNSUPPORTED) {
					// printk("Sent PID %02X\n", currentPID);
					int MFL = 0; // for multi frames
					while ((ret = k_msgq_get(&obd_msgq, &obd_response_frame, K_MSEC(100))) == 0) {
						gpio_pin_toggle_dt(&my_led1);
						if ((obd_response_frame.data[0] & 0x10) == 0x10) { // First Frame
							if ((obd_response_frame.flags & CAN_FRAME_IDE) == CAN_FRAME_IDE)
							{
								flow_ext_frame.id = (obd_response_frame.id & 0xFFFF0000) | ((obd_response_frame.id & 0xFF) << 8) | ((obd_response_frame.id & 0xFF00) >> 8);
								ret = can_send(can_dev, &flow_ext_frame, K_FOREVER, can_tx_callback, "Sending Flow Control through Extended");
							}
							else
							{
								flow_std_frame.id = obd_response_frame.id - 8;
								ret = can_send(can_dev, &flow_std_frame, K_FOREVER, can_tx_callback, "Sending Flow Control through Standard");
							}
							MFL = (obd_response_frame.data[0] & 0xF) << 8 | obd_response_frame.data[1];
							UDP_Packet[currentLen] = MFL;
							currentLen++;
							memcpy(UDP_Packet + currentLen, obd_response_frame.data + 2, 6);
							currentLen += 6;
							MFL -= 6;
						}
						else if ((obd_response_frame.data[0] & 0x20) == 0x20) {
							int bytesToGet = Z_MIN(MFL, 7);
							memcpy(UDP_Packet + currentLen, obd_response_frame.data + 1, bytesToGet);
							currentLen += bytesToGet;
							MFL -= bytesToGet;
						}
						else {
							memcpy(UDP_Packet + currentLen, obd_response_frame.data, obd_response_frame.data[0] + 1);
							currentLen += obd_response_frame.data[0] + 1;
						}
						// printk("Current Length: %d", currentLen);
						// printk("\tReceived from %X:", obd_response_frame.id);
						// for (int i = 0; i < obd_response_frame.dlc; i++) {
						// 	printk(" %02X", obd_response_frame.data[i]);
						// }
						// printk("\n");
					}
				}
			}
			printk("Packet of Size %d:\n", currentLen);
			for (int i = 0; i < currentLen; i++) {
				printk(" %02X", UDP_Packet[i]);
			}
			printk("\n");
			currentLen = PREFIX_LENGTH;
		}
		else {
			gpio_pin_set_dt(&my_led4, 0);
		}
	}
}
