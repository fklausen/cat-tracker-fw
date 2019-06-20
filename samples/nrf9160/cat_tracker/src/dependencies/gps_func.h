#ifndef GPS_FUNC_H__
#define GPS_FUNC_H__

#include <zephyr.h>
#include <stdio.h>
#include <nrf_socket.h>
#include <net/socket.h>

int enable_gps(void);

int gps_init(void);

void print_pvt_data(nrf_gnss_data_frame_t *pvt_data);

int process_gps_data(nrf_gnss_data_frame_t *gps_data);

void print_nmea_data(void);

int get_gps_data(char *gps_dummy_string);

#endif