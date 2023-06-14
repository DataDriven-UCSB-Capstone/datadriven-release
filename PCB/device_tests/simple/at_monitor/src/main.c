/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <string.h>
#include <zephyr/kernel.h>
#include <nrf_modem_at.h>

void main(void)
{
	
	char response[64];
	int err;
	err = nrf_modem_at_cmd(response, sizeof(response), "AT+CGSN");
	if (err) {
		printk("Failed to read IMEI, err %d\n", err);
		return;
	}

	char IMEI[16];

	strncpy(IMEI, response, 15);
	IMEI[15] = '\0';

	printk("IMEI: %s\n", IMEI);

	err = nrf_modem_at_cmd(response, sizeof(response), "AT+CIMI");
	if (err) {
		printk("Failed to read IMSI, err %d\n", err);
		return;
	}

	printk("IMSI: %s\n", response);
}
