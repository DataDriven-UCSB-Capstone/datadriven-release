/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

// GNSS imports
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <nrf_modem_at.h>
#include <nrf_modem_gnss.h>
#include <modem/lte_lc.h>
#include <date_time.h>

// UDP import
#include <zephyr/net/socket.h>

// CAN imports
// #include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include <zephyr/device.h>
#include <zephyr/drivers/can.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/sys/byteorder.h>

LOG_MODULE_REGISTER(gnss_sample, CONFIG_GNSS_SAMPLE_LOG_LEVEL);

CAN_MSGQ_DEFINE(obd_msgq, 2);
struct can_frame obd_frame;
const struct device *can_dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_canbus));

#define OBD_BROADCAST_ID 0x7DF
#define ECU_ID 0x7E8

static int can_init()
{
	if (!device_is_ready(can_dev))
	{
		printk("CAN: Device %s not ready.\n", can_dev->name);
		return -1;
	}

	uint32_t ret;
	ret = can_set_mode(can_dev, CAN_MODE_NORMAL);
	if (ret != 0)
	{
		printk("Error setting CAN mode [%d]", ret);
		return -1;
	}

	ret = can_start(can_dev);

	if (ret != 0)
	{
		printk("Error starting CAN controller [%d]", ret);
		return -1;
	}

	const struct can_filter obd_filter = {
		.flags = CAN_FILTER_DATA,
		.id = ECU_ID,
		.mask = CAN_STD_ID_MASK
	};
	int filter_id;

	filter_id = can_add_rx_filter_msgq(can_dev, &obd_msgq, &obd_filter);
	if (filter_id < 0)
	{
		LOG_ERR("Unable to add rx filter [%d]", filter_id);
		return -1;
	}

	return 0;
}

#define PI 3.14159265358979323846
#define EARTH_RADIUS_METERS (6371.0 * 1000.0)

static const char update_indicator[] = {'\\', '|', '/', '-'};

static struct nrf_modem_gnss_pvt_data_frame last_pvt;

/* Reference position. */
static bool ref_used;
static double ref_latitude;
static double ref_longitude;

K_MSGQ_DEFINE(nmea_queue, sizeof(struct nrf_modem_gnss_nmea_data_frame *), 10, 4);
static K_SEM_DEFINE(pvt_data_sem, 0, 1);
K_SEM_DEFINE(lte_connected, 0, 1);
K_SEM_DEFINE(lte_idle, 0, 1);

static struct k_poll_event events[2] = {
	K_POLL_EVENT_STATIC_INITIALIZER(K_POLL_TYPE_SEM_AVAILABLE,
									K_POLL_MODE_NOTIFY_ONLY,
									&pvt_data_sem, 0),
	K_POLL_EVENT_STATIC_INITIALIZER(K_POLL_TYPE_MSGQ_DATA_AVAILABLE,
									K_POLL_MODE_NOTIFY_ONLY,
									&nmea_queue, 0),
};

BUILD_ASSERT(IS_ENABLED(CONFIG_LTE_NETWORK_MODE_LTE_M_GPS) ||
				 IS_ENABLED(CONFIG_LTE_NETWORK_MODE_NBIOT_GPS) ||
				 IS_ENABLED(CONFIG_LTE_NETWORK_MODE_LTE_M_NBIOT_GPS),
			 "CONFIG_LTE_NETWORK_MODE_LTE_M_GPS, "
			 "CONFIG_LTE_NETWORK_MODE_NBIOT_GPS or "
			 "CONFIG_LTE_NETWORK_MODE_LTE_M_NBIOT_GPS must be enabled");

BUILD_ASSERT((sizeof(CONFIG_GNSS_SAMPLE_REFERENCE_LATITUDE) == 1 &&
			  sizeof(CONFIG_GNSS_SAMPLE_REFERENCE_LONGITUDE) == 1) ||
				 (sizeof(CONFIG_GNSS_SAMPLE_REFERENCE_LATITUDE) > 1 &&
				  sizeof(CONFIG_GNSS_SAMPLE_REFERENCE_LONGITUDE) > 1),
			 "CONFIG_GNSS_SAMPLE_REFERENCE_LATITUDE and "
			 "CONFIG_GNSS_SAMPLE_REFERENCE_LONGITUDE must be both either set or empty");

/* Returns the distance between two coordinates in meters. The distance is calculated using the
 * haversine formula.
 */
static double distance_calculate(double lat1, double lon1,
								 double lat2, double lon2)
{
	double d_lat_rad = (lat2 - lat1) * PI / 180.0;
	double d_lon_rad = (lon2 - lon1) * PI / 180.0;

	double lat1_rad = lat1 * PI / 180.0;
	double lat2_rad = lat2 * PI / 180.0;

	double a = pow(sin(d_lat_rad / 2), 2) +
			   pow(sin(d_lon_rad / 2), 2) *
				   cos(lat1_rad) * cos(lat2_rad);

	double c = 2 * asin(sqrt(a));

	return EARTH_RADIUS_METERS * c;
}

static void print_distance_from_reference(struct nrf_modem_gnss_pvt_data_frame *pvt_data)
{
	if (!ref_used)
	{
		return;
	}

	double distance = distance_calculate(pvt_data->latitude, pvt_data->longitude,
										 ref_latitude, ref_longitude);

	if (IS_ENABLED(CONFIG_GNSS_SAMPLE_MODE_TTFF_TEST))
	{
		LOG_INF("Distance from reference: %.01f", distance);
	}
	else
	{
		printk("\nDistance from reference: %.01f\n", distance);
	}
}

static void gnss_event_handler(int event)
{
	int retval;
	struct nrf_modem_gnss_nmea_data_frame *nmea_data;

	switch (event)
	{
	case NRF_MODEM_GNSS_EVT_PVT:
		retval = nrf_modem_gnss_read(&last_pvt, sizeof(last_pvt), NRF_MODEM_GNSS_DATA_PVT);
		if (retval == 0)
		{
			k_sem_give(&pvt_data_sem);
		}
		break;

	case NRF_MODEM_GNSS_EVT_NMEA:
		nmea_data = k_malloc(sizeof(struct nrf_modem_gnss_nmea_data_frame));
		if (nmea_data == NULL)
		{
			LOG_ERR("Failed to allocate memory for NMEA");
			break;
		}

		retval = nrf_modem_gnss_read(nmea_data,
									 sizeof(struct nrf_modem_gnss_nmea_data_frame),
									 NRF_MODEM_GNSS_DATA_NMEA);
		if (retval == 0)
		{
			retval = k_msgq_put(&nmea_queue, &nmea_data, K_NO_WAIT);
		}

		if (retval != 0)
		{
			k_free(nmea_data);
		}
		break;
	default:
		break;
	}
}

static void lte_handler(const struct lte_lc_evt *const evt)
{
	switch (evt->type)
	{
	case LTE_LC_EVT_NW_REG_STATUS:
		if ((evt->nw_reg_status != LTE_LC_NW_REG_REGISTERED_HOME) &&
			(evt->nw_reg_status != LTE_LC_NW_REG_REGISTERED_ROAMING))
		{
			break;
		}

		printk("Network registration status: %s\n",
			   evt->nw_reg_status == LTE_LC_NW_REG_REGISTERED_HOME ? "Connected - home network" : "Connected - roaming\n");
		k_sem_give(&lte_connected);
		break;
	case LTE_LC_EVT_PSM_UPDATE:
		printk("PSM parameter update: TAU: %d, Active time: %d\n",
			   evt->psm_cfg.tau, evt->psm_cfg.active_time);
		break;
	case LTE_LC_EVT_EDRX_UPDATE:
	{
		char log_buf[60];
		ssize_t len;

		len = snprintk(log_buf, sizeof(log_buf),
					   "eDRX parameter update: eDRX: %f, PTW: %f\n",
					   evt->edrx_cfg.edrx, evt->edrx_cfg.ptw);
		if (len > 0)
		{
			printk("%s\n", log_buf);
		}
		break;
	}
	case LTE_LC_EVT_RRC_UPDATE:
		printk("RRC mode: %s\n",
			   evt->rrc_mode == LTE_LC_RRC_MODE_CONNECTED ? "Connected" : "Idle\n");
		if (evt->rrc_mode == LTE_LC_RRC_MODE_IDLE)
		{
			k_sem_give(&lte_idle);
		}
		break;
	case LTE_LC_EVT_CELL_UPDATE:
		printk("LTE cell changed: Cell ID: %d, Tracking area: %d\n",
			   evt->cell.id, evt->cell.tac);
		break;
	default:
		break;
	}
}

static int modem_init(void)
{
	int err;

	err = lte_lc_init();
	if (err)
	{
		LOG_ERR("Failed to initialize LTE link controller");
		return -1;
	}

	err = lte_lc_edrx_req(true);
	if (err)
	{
		LOG_ERR("lte_lc_edrx_req, error: %d\n", err);
		return -1;
	}

	err = lte_lc_connect_async(lte_handler);
	if (err)
	{
		printk("Connecting to LTE network failed, error: %d\n", err);
		return -1;
	}

	return 0;
}

static int gnss_init_and_start(void)
{
	/* Enable GNSS. */
	if (lte_lc_func_mode_set(LTE_LC_FUNC_MODE_ACTIVATE_GNSS) != 0)
	{
		LOG_ERR("Failed to activate GNSS functional mode");
		return -1;
	}

	/* Configure GNSS. */
	if (nrf_modem_gnss_event_handler_set(gnss_event_handler) != 0)
	{
		LOG_ERR("Failed to set GNSS event handler");
		return -1;
	}

	/* Enable all supported NMEA messages. */
	uint16_t nmea_mask = NRF_MODEM_GNSS_NMEA_RMC_MASK;

	if (nrf_modem_gnss_nmea_mask_set(nmea_mask) != 0)
	{
		LOG_ERR("Failed to set GNSS NMEA mask");
		return -1;
	}

	/* This use case flag should always be set. */
	uint8_t use_case = NRF_MODEM_GNSS_USE_CASE_MULTIPLE_HOT_START;

	if (nrf_modem_gnss_use_case_set(use_case) != 0)
	{
		LOG_WRN("Failed to set GNSS use case");
	}

	if (nrf_modem_gnss_start() != 0)
	{
		LOG_ERR("Failed to start GNSS");
		return -1;
	}

	return 0;
}

static void print_satellite_stats(struct nrf_modem_gnss_pvt_data_frame *pvt_data)
{
	uint8_t tracked = 0;
	uint8_t in_fix = 0;
	uint8_t unhealthy = 0;

	for (int i = 0; i < NRF_MODEM_GNSS_MAX_SATELLITES; ++i)
	{
		if (pvt_data->sv[i].sv > 0)
		{
			tracked++;

			if (pvt_data->sv[i].flags & NRF_MODEM_GNSS_SV_FLAG_USED_IN_FIX)
			{
				in_fix++;
			}

			if (pvt_data->sv[i].flags & NRF_MODEM_GNSS_SV_FLAG_UNHEALTHY)
			{
				unhealthy++;
			}
		}
	}

	printk("Tracking: %2d Using: %2d Unhealthy: %d\n", tracked, in_fix, unhealthy);
}

static void print_fix_data(struct nrf_modem_gnss_pvt_data_frame *pvt_data)
{
	printk("Latitude:       %.06f\n", pvt_data->latitude);
	printk("Longitude:      %.06f\n", pvt_data->longitude);
	printk("Altitude:       %.01f m\n", pvt_data->altitude);
	printk("Accuracy:       %.01f m\n", pvt_data->accuracy);
	printk("Speed:          %.01f m/s\n", pvt_data->speed);
	printk("Speed accuracy: %.01f m/s\n", pvt_data->speed_accuracy);
	printk("Heading:        %.01f deg\n", pvt_data->heading);
	printk("Date:           %04u-%02u-%02u\n",
		   pvt_data->datetime.year,
		   pvt_data->datetime.month,
		   pvt_data->datetime.day);
	printk("Time (UTC):     %02u:%02u:%02u.%03u\n",
		   pvt_data->datetime.hour,
		   pvt_data->datetime.minute,
		   pvt_data->datetime.seconds,
		   pvt_data->datetime.ms);
	printk("PDOP:           %.01f\n", pvt_data->pdop);
	printk("HDOP:           %.01f\n", pvt_data->hdop);
	printk("VDOP:           %.01f\n", pvt_data->vdop);
	printk("TDOP:           %.01f\n", pvt_data->tdop);
}

static int client_fd;
static struct sockaddr_storage host_addr;
static struct k_work_delayable server_transmission_work;

static void server_disconnect(void)
{
	(void)close(client_fd);
}

static int server_init(void)
{
	struct sockaddr_in *server4 = ((struct sockaddr_in *)&host_addr);

	server4->sin_family = PF_INET;
	server4->sin_port = htons(65432);

	inet_pton(AF_INET, "137.184.6.77",
			  &server4->sin_addr);

	return 0;
}

static int server_connect(void)
{
	int err;

	client_fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (client_fd < 0)
	{
		printk("Failed to create UDP socket: %d\n", errno);
		err = -errno;
		goto error;
	}

	err = connect(client_fd, (struct sockaddr *)&host_addr,
				  sizeof(struct sockaddr_in));
	if (err < 0)
	{
		printk("Connect failed : %d\n", errno);
		goto error;
	}

	return 0;

error:
	server_disconnect();

	return err;
}

int main(void)
{
	if (can_init() != 0)
	{
		LOG_ERR("Failed to initialize CAN");
		return -1;
	}
	printk("CAN INIT SUCCESS\n");
	uint8_t cnt = 0;
	struct nrf_modem_gnss_nmea_data_frame *nmea_data;

	LOG_INF("Starting GNSS sample");

	/* Initialize reference coordinates (if used). */
	if (sizeof(CONFIG_GNSS_SAMPLE_REFERENCE_LATITUDE) > 1 &&
		sizeof(CONFIG_GNSS_SAMPLE_REFERENCE_LONGITUDE) > 1)
	{
		ref_used = true;
		ref_latitude = atof(CONFIG_GNSS_SAMPLE_REFERENCE_LATITUDE);
		ref_longitude = atof(CONFIG_GNSS_SAMPLE_REFERENCE_LONGITUDE);
	}

	printk("K_SEM_COUNT_GET(LTE_CONNECTED)       %d\n", k_sem_count_get(&lte_connected));
	printk("K_SEM_COUNT_GET(LTE_IDLE)       %d\n", k_sem_count_get(&lte_idle));

	printk("MODEM INIT\n");
	if (modem_init() != 0)
	{
		LOG_ERR("Failed to initialize modem");
		return -1;
	}

	printk("K_SEM_COUNT_GET(LTE_CONNECTED)       %d\n", k_sem_count_get(&lte_connected));
	printk("K_SEM_COUNT_GET(LTE_IDLE)       %d\n", k_sem_count_get(&lte_idle));

	// while (k_sem_count_get(&lte_idle) == 0) {printk("WAITING FOR IDLE\n");}

	printk("GNSS INIT AND START\n");
	if (gnss_init_and_start() != 0)
	{
		LOG_ERR("Failed to initialize and start GNSS");
		return -1;
	}

	printk("K_SEM_COUNT_GET(LTE_CONNECTED)       %d\n", k_sem_count_get(&lte_connected));
	printk("K_SEM_COUNT_GET(LTE_IDLE)       %d\n", k_sem_count_get(&lte_idle));

	int64_t fix_timestamp = k_uptime_get();

	int err;
	unsigned long num_gnss_frames = 1;

	
	struct can_frame obd_request_frame = {
		.flags = 0,
		.id = OBD_BROADCAST_ID,
		.dlc = 8
	};

	obd_request_frame.data[0] = 2; // 2 bytes of data
	obd_request_frame.data[1] = 0x01; // Service 01 - show current data
	obd_request_frame.data[2] = 0x00; // PID 00 - Show supported PIDs 1-32
	// obd_request_frame.data[3] = DON'T CARE 3..7 don't matter since it is a 2-byte message
	
	bool PID_supported[256] = {false};
	printk("Identifying Available PIDs...");
	for (int checkPID = 0; checkPID < 0xFF; checkPID += 0x20)
	{
		obd_request_frame.data[2] = checkPID;
		err = can_send(can_dev, &obd_request_frame, K_MSEC(2000), NULL, NULL);
		if (err == 0)
		{
			err = k_msgq_get(&obd_msgq, &obd_frame, K_MSEC(2000));
			if (err == 0)
			{
				// printk("\tService %d PID %d:", obd_frame.data[1] - 0x40, obd_frame.data[2]);
				for (int i = 0; i < 8; i++)
				{
					if (obd_frame.data[3] & (1 << i))
					{
						PID_supported[checkPID + (8 - i)] = true;
					}
					if (obd_frame.data[4] & (1 << i))
					{
						PID_supported[checkPID + (16 - i)] = true;
					}
					if (obd_frame.data[5] & (1 << i))
					{
						PID_supported[checkPID + (24 - i)] = true;
					}
					if (obd_frame.data[6] & (1 << i))
					{
						PID_supported[checkPID + (32 - i)] = true;
					}
				}
			}
		}
	}
	printk("DONE\n");

	printk("PIDs Available:");
	for (int i = 0; i < 256; i++)
	{
		if (PID_supported[i])
		{
			printk(" %d,", i);
		}
	}
	printk("\n");
	obd_request_frame.data[2] = 0x00; // PID 00 - Show supported PIDs 1-32
	for (;;)
	{
		// FETCH ALL SUPPORTED PIDs
		for (int current_pid = 0; current_pid < 256; current_pid++)
		{
			if (PID_supported[current_pid])
			{
				obd_request_frame.data[2] = current_pid;
				printk("Sending OBD request...");
				err = can_send(can_dev, &obd_request_frame, K_MSEC(2000), NULL, NULL);
				if (err == 0)
				{
					printk("DONE\n");
				}
				else
				{
					printk("FAIL\n");
				}

				printk("Waiting for CAN...");
				err = k_msgq_get(&obd_msgq, &obd_frame, K_MSEC(2000));
				if (err == 0)
				{
					printk("DONE\n");
					printk("CAN RX frame:\n");
					printk("\tID: %03X\n", obd_frame.id);
					printk("\tDATA:");
					for (int i = 0; i < obd_frame.dlc; i++)
					{
						printk(" %02X", obd_frame.data[i]);
					}
					printk("\n");
					printk("\tService %d PID %d:", obd_frame.data[1] - 0x40, obd_frame.data[2]);
					for (int i = 3; i <= obd_frame.data[0]; i++)
					{
						printk(" %02X", obd_frame.data[i]);
					}
					printk("\n");
				}
				else
				{
					printk("SKIPPED\n");
				}
			}
		}

		k_msleep(2000);

		if (num_gnss_frames % 10 == 0)
		{
			printk("Publish - start\n");

			err = k_sem_take(&lte_idle, K_MSEC(1000));
			if (err == 0)
			{
				err = lte_lc_connect();
				if (err)
				{
					printk("Connecting to LTE network failed, error: %d\n", err);
					return -1;
				}

				err = server_init();
				if (err)
				{
					printk("Not able to initialize UDP server connection\n");
					return -1;
				}

				err = server_connect();
				if (err)
				{
					printk("Not able to connect to UDP server\n");
					return -1;
				}
				char buffer_nmea[NRF_MODEM_GNSS_NMEA_MAX_LEN];
				strncpy(buffer_nmea, nmea_data->nmea_str, NRF_MODEM_GNSS_NMEA_MAX_LEN);
				// TODO: BUFFER FOR PVT DATA

				printk("Transmitting UDP/IP payload of %d bytes to the ",
					NRF_MODEM_GNSS_NMEA_MAX_LEN + 28);
				printk("IP address %s, port number %d\n",
					"137.184.6.77",
					65432);

				err = send(client_fd, buffer_nmea, sizeof(buffer_nmea), 0);
				if (err < 0)
				{
					printk("Failed to transmit UDP packet, %d\n", errno);
					return -1;
				}

				server_disconnect();
				num_gnss_frames++;
				printk("Publish - complete\n");
				continue;
			}
		}
		// printk("Waiting on LTE to idle...");
		// while (k_sem_count_get(&lte_idle) == 0);
		// printk("DONE\nWaiting on GNSS event...");
		// (void)k_poll(events, 2, K_MSEC(1000));
		// printk("DONE\n");

		// if (events[0].state == K_POLL_STATE_SEM_AVAILABLE &&
		// 	k_sem_take(events[0].sem, K_NO_WAIT) == 0)
		// {
		// 	/* New PVT data available */
		// 	printk("\033[1;1H");
		// 	printk("\033[2J");
		// 	print_satellite_stats(&last_pvt);

		// 	if (last_pvt.flags & NRF_MODEM_GNSS_PVT_FLAG_DEADLINE_MISSED)
		// 	{
		// 		printk("GNSS operation blocked by LTE\n");
		// 	}
		// 	if (last_pvt.flags &
		// 		NRF_MODEM_GNSS_PVT_FLAG_NOT_ENOUGH_WINDOW_TIME)
		// 	{
		// 		printk("Insufficient GNSS time windows\n");
		// 	}
		// 	if (last_pvt.flags & NRF_MODEM_GNSS_PVT_FLAG_SLEEP_BETWEEN_PVT)
		// 	{
		// 		printk("Sleep period(s) between PVT notifications\n");
		// 	}
		// 	printk("-----------------------------------\n");

		// 	if (last_pvt.flags & NRF_MODEM_GNSS_PVT_FLAG_FIX_VALID)
		// 	{
		// 		fix_timestamp = k_uptime_get();
		// 		print_fix_data(&last_pvt);
		// 		print_distance_from_reference(&last_pvt);
		// 		num_gnss_frames++;
		// 	}
		// 	else
		// 	{
		// 		printk("Seconds since last fix: %d\n",
		// 			   (uint32_t)((k_uptime_get() - fix_timestamp) / 1000));
		// 		cnt++;
		// 		printk("Searching [%c]\n", update_indicator[cnt % 4]);
		// 	}

		// 	printk("\nNMEA strings:\n\n");
		// }
		// if (events[1].state == K_POLL_STATE_MSGQ_DATA_AVAILABLE &&
		// 	k_msgq_get(events[1].msgq, &nmea_data, K_NO_WAIT) == 0)
		// {
		// 	/* New NMEA data available */

		// 	printk("%s", nmea_data->nmea_str);
		// 	k_free(nmea_data);
		// }

		// events[0].state = K_POLL_STATE_NOT_READY;
		// events[1].state = K_POLL_STATE_NOT_READY;
	}

	return 0;
}
