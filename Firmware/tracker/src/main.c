/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <stdio.h>
#include <zephyr/kernel.h>
#include <zephyr/net/socket.h>

#include <zephyr/logging/log.h>
#include <modem/lte_lc.h>
#include <nrf_modem_gnss.h>

#define SERVER_HOSTNAME "api.datadrivenucsb.com"
#define SERVER_PORT "65432"

static struct nrf_modem_gnss_pvt_data_frame pvt_data;

static int64_t gnss_start_time;
static bool first_fix = false;

static int sock;
static struct sockaddr_storage server;

static K_SEM_DEFINE(lte_connected, 0, 1);

LOG_MODULE_REGISTER(DataDriven, LOG_LEVEL_ERR);

#include "functions.h"

static K_SEM_DEFINE(gnss_ready, 0, 1);


static uint8_t udp_data[512];
static uint16_t current_msg_size = 0;

static uint8_t gnss_data[100];
static uint16_t gnss_size = 0;

static uint8_t vehicle_data[512];
static uint16_t vehicle_size = 0;

static int server_resolve(void)
{
	int err;
	struct addrinfo *result;
	struct addrinfo hints = {
		.ai_family = AF_INET,
		.ai_socktype = SOCK_DGRAM
	};
	
	err = getaddrinfo(SERVER_HOSTNAME, SERVER_PORT, &hints, &result);
	if (err != 0) {
		LOG_INF("ERROR: getaddrinfo failed %d", err);
		return -EIO;
	}

	if (result == NULL) {
		LOG_INF("ERROR: Address not found");
		return -ENOENT;
	}

	struct sockaddr_in *server4 = ((struct sockaddr_in *)&server);

	server4->sin_addr.s_addr = ((struct sockaddr_in *)result->ai_addr)->sin_addr.s_addr;
	server4->sin_family = AF_INET;
	server4->sin_port = ((struct sockaddr_in *)result->ai_addr)->sin_port;
	
	char ipv4_addr[NET_IPV4_ADDR_LEN];
	inet_ntop(AF_INET, &server4->sin_addr.s_addr, ipv4_addr, sizeof(ipv4_addr));
	LOG_INF("IPv4 Address found %s", ipv4_addr);
	
	freeaddrinfo(result);

	return 0;
}

static int server_connect(void)
{
	int err;
	sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (sock < 0) {
		LOG_ERR("Failed to create socket: %d.", errno);
		return -errno;
	}

	err = connect(sock, (struct sockaddr *)&server, sizeof(struct sockaddr_in));
	if (err < 0) {
		LOG_ERR("Connect failed : %d", errno);
		return -errno;
	}
	LOG_INF("Successfully connected to server");

	return 0;
}

static void lte_handler(const struct lte_lc_evt *const evt)
{
	switch (evt->type) {
	case LTE_LC_EVT_NW_REG_STATUS:
		if ((evt->nw_reg_status != LTE_LC_NW_REG_REGISTERED_HOME) &&
			(evt->nw_reg_status != LTE_LC_NW_REG_REGISTERED_ROAMING)) {
			break;
		}
		LOG_INF("Network registration status: %s", evt->nw_reg_status == LTE_LC_NW_REG_REGISTERED_HOME ? "Connected - home network" : "Connected - roaming");
		k_sem_give(&lte_connected);
		break;	
	case LTE_LC_EVT_RRC_UPDATE:
		LOG_INF("RRC mode: %s",  evt->rrc_mode == LTE_LC_RRC_MODE_CONNECTED ? "Connected" : "Idle");
		break;
	/* STEP 9.1 - On event PSM update, print PSM paramters and check if was enabled */
	case LTE_LC_EVT_PSM_UPDATE:
		LOG_INF("PSM parameter update: Periodic TAU: %d s, Active time: %d s", evt->psm_cfg.tau, evt->psm_cfg.active_time);
		if (evt->psm_cfg.active_time == -1){
			LOG_ERR("Network rejected PSM parameters. Failed to setup network");
		}
		break;
	/* STEP 9.2 - On event eDRX update, print eDRX paramters */
	case LTE_LC_EVT_EDRX_UPDATE:
		LOG_INF("eDRX parameter update: eDRX: %f, PTW: %f", evt->edrx_cfg.edrx, evt->edrx_cfg.ptw);
		break;
	default:
		break;
	}
}

static void modem_configure(void)
{
	int err;
	
	/* STEP 8 - Request PSM and eDRX from the network */
	err = lte_lc_psm_req(true);
	if (err) {
		LOG_ERR("lte_lc_psm_req, error: %d", err);
	} 
	err = lte_lc_edrx_req(true);
	if (err) {
		LOG_ERR("lte_lc_edrx_req, error: %d", err);
	}

	LOG_INF("Connecting to LTE network");

	err = lte_lc_init_and_connect_async(lte_handler);
	if (err) {
		LOG_ERR("Modem could not be configured, error: %d", err);
		return;
	}
	k_sem_take(&lte_connected, K_FOREVER);
	LOG_INF("Connected to LTE network");
}

static void print_fix_data(struct nrf_modem_gnss_pvt_data_frame *pvt_data)
{
	LOG_INF("Latitude:       %.06f", pvt_data->latitude);
	LOG_INF("Longitude:      %.06f", pvt_data->longitude);
	LOG_INF("Altitude:       %.01f m", pvt_data->altitude);
	LOG_INF("Time (UTC):     %02u:%02u:%02u.%03u",
	       pvt_data->datetime.hour,
	       pvt_data->datetime.minute,
	       pvt_data->datetime.seconds,
	       pvt_data->datetime.ms);
	
	
	union {
		double d;
		uint8_t b[8];
	} lat, lon;

	union {
		float f;
		uint8_t b[4];
	} alt, speed, vertical_speed, heading;

	lat.d = pvt_data->latitude;
	lon.d = pvt_data->longitude;
	alt.f = pvt_data->altitude;
	speed.f = pvt_data->speed;
	vertical_speed.f = pvt_data->vertical_speed;
	heading.f = pvt_data->heading;
	
	/* STEP 3.2 - Store latitude and longitude in gnss_data buffer */
	snprintf(gnss_data, sizeof(gnss_data), "%s %s %04u-%02u-%02u %02u:%02u:%02u ",
													system_state.VIN, system_state.IMEI,
													pvt_data->datetime.year, pvt_data->datetime.month, pvt_data->datetime.day,
													pvt_data->datetime.hour, pvt_data->datetime.minute, pvt_data->datetime.seconds);
	gnss_size = strlen(gnss_data);
	memcpy(gnss_data + gnss_size, lat.b, 8);
	gnss_size += 8;
	memcpy(gnss_data + gnss_size, lon.b, 8);
	gnss_size += 8;
	memcpy(gnss_data + gnss_size, alt.b, 4);
	gnss_size += 4;
	memcpy(gnss_data + gnss_size, speed.b, 4);
	gnss_size += 4;
	memcpy(gnss_data + gnss_size, vertical_speed.b, 4);
	gnss_size += 4;
	memcpy(gnss_data + gnss_size, heading.b, 4);
	gnss_size += 4;
}

static void gnss_event_handler(int event)
{
	int err, num_satellites;

	switch (event) {
	case NRF_MODEM_GNSS_EVT_PVT:
		num_satellites = 0;
		for (int i = 0; i < 12 ; i++) {
			if (pvt_data.sv[i].signal != 0) {
				num_satellites++;
			}	
		}
		if (!first_fix) {
			printk("[%2.1lld s] Searching. Current satellites: %d\n", (k_uptime_get())/1000, num_satellites);
		}
		err = nrf_modem_gnss_read(&pvt_data, sizeof(pvt_data), NRF_MODEM_GNSS_DATA_PVT);
		if (err) {
			LOG_ERR("nrf_modem_gnss_read failed, err %d", err);
			return;
		}
		if (pvt_data.flags & NRF_MODEM_GNSS_PVT_FLAG_FIX_VALID) {
			print_fix_data(&pvt_data);
			if (!first_fix) {
				printk("Time to first fix: %2.1lld s\n", (k_uptime_get() - gnss_start_time)/1000);
				first_fix = true;
			}
			return;
		} 
		/* STEP 5 - Check for the flags indicating GNSS is blocked */		
		if (pvt_data.flags & NRF_MODEM_GNSS_PVT_FLAG_DEADLINE_MISSED) {
			LOG_INF("GNSS blocked by LTE activity");
		} else if (pvt_data.flags & NRF_MODEM_GNSS_PVT_FLAG_NOT_ENOUGH_WINDOW_TIME) {
			LOG_INF("Insufficient GNSS time windows");
		}
		break;

	case NRF_MODEM_GNSS_EVT_PERIODIC_WAKEUP:
		LOG_INF("GNSS has woken up");
		gpio_pin_set_dt(&system_state.leds[0], 1);
		break;
	case NRF_MODEM_GNSS_EVT_SLEEP_AFTER_FIX:
		LOG_INF("GNSS enter sleep after fix");
		k_sem_give(&gnss_ready);
		gpio_pin_set_dt(&system_state.leds[0], 0);
		break;		
	default:
		break;
	}
}

static int gnss_init_and_start(void)
{
	gpio_pin_set_dt(&system_state.leds[0], 1);
	/* STEP 4 - Set the modem mode to normal */
	if (lte_lc_func_mode_set(LTE_LC_FUNC_MODE_NORMAL) != 0) {
		LOG_ERR("Failed to activate GNSS functional mode");
		return -1;
	}

	if (nrf_modem_gnss_event_handler_set(gnss_event_handler) != 0) {
		LOG_ERR("Failed to set GNSS event handler");
		return -1;
	}

	if (nrf_modem_gnss_fix_interval_set(CONFIG_GNSS_PERIODIC_INTERVAL) != 0) {
		LOG_ERR("Failed to set GNSS fix interval");
		return -1;
	}

	if (nrf_modem_gnss_fix_retry_set(CONFIG_GNSS_PERIODIC_TIMEOUT) != 0) {
		LOG_ERR("Failed to set GNSS fix retry");
		return -1;
	}

	LOG_INF("Starting GNSS");
	if (nrf_modem_gnss_start() != 0) {
		LOG_ERR("Failed to start GNSS");
		return -1;
	}

	gnss_start_time = k_uptime_get();

	return 0;
}

void main(void)
{
	modem_configure();
	setup_GPIO();
	get_ID();

	if (server_resolve() != 0) {
		LOG_INF("Failed to resolve server name");
		return;
	}
	
	if (server_connect() != 0) {
		LOG_INF("Failed to initialize client");
		return;
	}

	if (gnss_init_and_start() != 0) {
		LOG_ERR("Failed to initialize and start GNSS");
		return;
	}

	int can_ret;
	obd_state.PID_supported[0x00] = BOTH;
	obd_state.standard_frame.data[0] = 0x02;
	obd_state.standard_frame.data[1] = 0x01;
	obd_state.extended_frame.data[0] = 0x02;
	obd_state.extended_frame.data[1] = 0x01;
	for (uint8_t checkPID = 0x00; checkPID < 0xFF && obd_state.PID_supported[checkPID] != UNSUPPORTED; checkPID += 0x20)
	{
		if ((obd_state.PID_supported[checkPID] & STANDARD) == STANDARD) {
			obd_state.standard_frame.data[2] = checkPID;
			can_ret = can_send(obd_state.device, &obd_state.standard_frame, K_MSEC(500), can_tx_callback, "Requesting Standard PID");
		}
		if ((obd_state.PID_supported[checkPID] & EXTENDED) == EXTENDED) {
			obd_state.extended_frame.data[2] = checkPID;
			can_ret = can_send(obd_state.device, &obd_state.extended_frame, K_MSEC(500), can_tx_callback, "Requesting Extended PID");
		}
		while ((can_ret = k_msgq_get(&obd_msgq, &obd_state.response_frame, K_MSEC(100))) == 0) { // can reduce K_MSEC() to very low, NO_WAIT is too fast
			for (int i = 0; i < 8; i++)
			{
				if (obd_state.response_frame.data[3] & (1 << i))
				{
					obd_state.PID_supported[checkPID + (8 - i)] |=  ((obd_state.response_frame.flags & CAN_FRAME_IDE) == CAN_FRAME_IDE) ? EXTENDED : STANDARD;
				}
				if (obd_state.response_frame.data[4] & (1 << i))
				{
					obd_state.PID_supported[checkPID + (16 - i)] |= ((obd_state.response_frame.flags & CAN_FRAME_IDE) == CAN_FRAME_IDE) ? EXTENDED : STANDARD;
				}
				if (obd_state.response_frame.data[5] & (1 << i))
				{
					obd_state.PID_supported[checkPID + (24 - i)] |= ((obd_state.response_frame.flags & CAN_FRAME_IDE) == CAN_FRAME_IDE) ? EXTENDED : STANDARD;
				}
				if (obd_state.response_frame.data[6] & (1 << i))
				{
					obd_state.PID_supported[checkPID + (32 - i)] |= ((obd_state.response_frame.flags & CAN_FRAME_IDE) == CAN_FRAME_IDE) ? EXTENDED : STANDARD;
				}
			}
		}
	}
	printf("PIDs Available:");
	for (int i = 0; i < 256; i++)
	{
		if (obd_state.PID_supported[i] != UNSUPPORTED)
		{
			printf(" %02X,", i);
		}
	}
	printf("\n");

	while (true) {
		if (k_sem_take(&gnss_ready, K_NO_WAIT) == 0) {
			memcpy(udp_data, gnss_data, gnss_size);
			current_msg_size = gnss_size;
			gpio_pin_set_dt(&system_state.leds[1], 1);
			memcpy(udp_data + current_msg_size, vehicle_data, vehicle_size);
			current_msg_size += vehicle_size;

			int err = send(sock, &udp_data, current_msg_size, 0);
			if (err < 0) {
				LOG_INF("Failed to send message, %d", errno);
				return;
			} else {
				printk("[%2.1lld s] Data (%d bytes) sent to %s:%s\n", (k_uptime_get())/1000, current_msg_size, SERVER_HOSTNAME, SERVER_PORT);
			}
			gpio_pin_set_dt(&system_state.leds[1], 0);
		} else {
			gpio_pin_set_dt(&system_state.leds[2], 1);
			vehicle_size = 0;
			// get IMU data
			sensor_sample_fetch_chan(imu_state.device, SENSOR_CHAN_ACCEL_XYZ);
			sensor_channel_get(imu_state.device, SENSOR_CHAN_ACCEL_X, &imu_state.raw_accel_x);
			sensor_channel_get(imu_state.device, SENSOR_CHAN_ACCEL_Y, &imu_state.raw_accel_y);
			sensor_channel_get(imu_state.device, SENSOR_CHAN_ACCEL_Z, &imu_state.raw_accel_z);

			sensor_sample_fetch_chan(imu_state.device, SENSOR_CHAN_GYRO_XYZ);
			sensor_channel_get(imu_state.device, SENSOR_CHAN_GYRO_X, &imu_state.raw_gyro_x);
			sensor_channel_get(imu_state.device, SENSOR_CHAN_GYRO_Y, &imu_state.raw_gyro_y);
			sensor_channel_get(imu_state.device, SENSOR_CHAN_GYRO_Z, &imu_state.raw_gyro_z);
			imu_state.accel_x.f = out_ev(&imu_state.raw_accel_x);
			imu_state.accel_y.f = out_ev(&imu_state.raw_accel_y);
			imu_state.accel_z.f = out_ev(&imu_state.raw_accel_z);
			imu_state.gyro_x.f = out_ev(&imu_state.raw_gyro_x);
			imu_state.gyro_y.f = out_ev(&imu_state.raw_gyro_y);
			imu_state.gyro_z.f = out_ev(&imu_state.raw_gyro_z);

			memcpy(vehicle_data + vehicle_size, imu_state.accel_x.b, 4);
			vehicle_size += 4;
			memcpy(vehicle_data + vehicle_size, imu_state.accel_y.b, 4);
			vehicle_size += 4;
			memcpy(vehicle_data + vehicle_size, imu_state.accel_z.b, 4);
			vehicle_size += 4;
			memcpy(vehicle_data + vehicle_size, imu_state.gyro_x.b, 4);
			vehicle_size += 4;
			memcpy(vehicle_data + vehicle_size, imu_state.gyro_y.b, 4);
			vehicle_size += 4;
			memcpy(vehicle_data + vehicle_size, imu_state.gyro_z.b, 4);
			vehicle_size += 4;

			// get vehicle data
			// printf("BEGIN VEHICLE DATA FETCH\n");
			for (uint8_t currentPID = 0x00; currentPID < 0xFF; currentPID++) {
				if (currentPID % 0x20 == 0) {
					continue;
				}
				if ((obd_state.PID_supported[currentPID] & STANDARD) == STANDARD) {
					obd_state.standard_frame.data[2] = currentPID;
					can_send(obd_state.device, &obd_state.standard_frame, K_MSEC(500), can_tx_callback, "Standard frame");
				}
				if ((obd_state.PID_supported[currentPID] & EXTENDED) == EXTENDED) {
					obd_state.extended_frame.data[2] = currentPID;
					can_send(obd_state.device, &obd_state.extended_frame, K_MSEC(500), can_tx_callback, "Extended frame");
				}
				if (obd_state.PID_supported[currentPID] != UNSUPPORTED) {
					// printf("\tRequested PID %02X\tResponse:", currentPID);
					int MFL = 0; // for multi frames
					while ((can_ret = k_msgq_get(&obd_msgq, &obd_state.response_frame, K_MSEC(500))) == 0) {
						if ((obd_state.response_frame.data[0] & 0x10) == 0x10) { // First Frame
							if ((obd_state.response_frame.flags & CAN_FRAME_IDE) == CAN_FRAME_IDE)
							{
								obd_state.extended_frame_flow.id = (obd_state.response_frame.id & 0xFFFF0000) |
													((obd_state.response_frame.id & 0xFF) << 8) |
													((obd_state.response_frame.id & 0xFF00) >> 8);
								can_ret = can_send(obd_state.device, &obd_state.extended_frame_flow, K_MSEC(100), can_tx_callback, "Sending Flow Control through Extended");
							}
							else
							{
								obd_state.standard_frame_flow.id = obd_state.response_frame.id - 8;
								can_ret = can_send(obd_state.device, &obd_state.standard_frame_flow, K_MSEC(100), can_tx_callback, "Sending Flow Control through Standard");
							}
							// MFL = (obd_state.response_frame.data[0] & 0xF) << 8 | obd_state.response_frame.data[1];
							MFL = obd_state.response_frame.data[1];
							// printf(" %02X", MFL);
							udp_data[current_msg_size] = MFL;
							current_msg_size++;
							// for (int i = 2; i < 8; i++) {
							// 	printf(" %02X", obd_state.response_frame.data[i]);
							// }
							memcpy(vehicle_data + vehicle_size, obd_state.response_frame.data + 2, 6);
							vehicle_size += 6;
							MFL -= 6;
						}
						else if ((obd_state.response_frame.data[0] & 0x20) == 0x20) { // consecutive frames
							int bytesToGet = Z_MIN(MFL, 7);
							// for (int i = 1; i < bytesToGet + 1; i++) {
							// 	printf(" %02X", obd_state.response_frame.data[i]);
							// }
							memcpy(vehicle_data + vehicle_size, obd_state.response_frame.data + 1, bytesToGet);
							vehicle_size += bytesToGet;
							MFL -= bytesToGet;
						}
						else {
							// for (int i = 0; i < obd_state.response_frame.data[0] + 1; i++) { // standard single frame
							// 	printf(" %02X", obd_state.response_frame.data[i]);
							// }
							memcpy(vehicle_data + vehicle_size, obd_state.response_frame.data, obd_state.response_frame.data[0] + 1);
							vehicle_size += obd_state.response_frame.data[0] + 1;
						}
					}
					// printf("\n");
				}
			}
			// printf("END VEHICLE DATA FETCH\n");
			gpio_pin_set_dt(&system_state.leds[2], 0);
		}
	}
	(void)close(sock);
}
