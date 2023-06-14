/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>

/* 1000 msec = 1 sec */
#define SLEEP_TIME_MS   500

/* The devicetree node identifier for the "led0" alias. */
#define LED0_NODE DT_ALIAS(led0)
#define LED1_NODE DT_ALIAS(led1)
#define LED2_NODE DT_ALIAS(led2)
#define LED3_NODE DT_ALIAS(led3)
#define LED4_NODE DT_ALIAS(led4)

#define BTN0_NODE DT_ALIAS(sw0)
#define BTN1_NODE DT_ALIAS(sw1)

/*
 * A build error on this line means your board is unsupported.
 * See the sample documentation for information on how to fix this.
 */
static const struct gpio_dt_spec leds[] = {
	GPIO_DT_SPEC_GET(LED0_NODE, gpios),
	GPIO_DT_SPEC_GET(LED1_NODE, gpios),
	GPIO_DT_SPEC_GET(LED2_NODE, gpios),
	GPIO_DT_SPEC_GET(LED3_NODE, gpios),
	GPIO_DT_SPEC_GET(LED4_NODE, gpios),
};

static const struct gpio_dt_spec button0 = GPIO_DT_SPEC_GET(BTN0_NODE, gpios);
static const struct gpio_dt_spec button1 = GPIO_DT_SPEC_GET(BTN1_NODE, gpios);

void main(void)
{
	printk("Hello World\n");
	int ret;

	for (size_t i = 0; i < 5; i++)
	{
		if (!device_is_ready(leds[i].port)) {
			return;
		}

		ret = gpio_pin_configure_dt(&leds[i], GPIO_OUTPUT_ACTIVE);
		if (ret < 0) {
			return;
		}

		ret = gpio_pin_set_dt(&leds[i], 0);
		if (ret < 0) {
			return;
		}
	}

	
	if (!device_is_ready(button0.port)) {
		return;
	}

	ret = gpio_pin_configure_dt(&button0, GPIO_INPUT);
	if (ret < 0) {
		return;
	}

	if (!device_is_ready(button1.port)) {
		return;
	}

	ret = gpio_pin_configure_dt(&button1, GPIO_INPUT);
	if (ret < 0) {
		return;
	}

	size_t currentLED = 0;
	while (1) {
		ret = gpio_pin_toggle_dt(&leds[currentLED]);
		if (ret < 0) {
			return;
		}
		currentLED = (currentLED + 1) % 5;
		if (gpio_pin_get_dt(&button0) == 1) {
			for (size_t i = 0; i < 5; i++) {
				ret = gpio_pin_set_dt(&leds[i], 0);
				if (ret < 0) {
					return;
				}
			}
		}
		if (gpio_pin_get_dt(&button1) == 1) {
			for (size_t i = 0; i < 5; i++) {
				ret = gpio_pin_set_dt(&leds[i], 1);
				if (ret < 0) {
					return;
				}
			}
		}
		k_msleep(SLEEP_TIME_MS);
	}
}
