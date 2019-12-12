/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#include <zephyr.h>
#include <stdio.h>
#include <string.h>
#include <misc/reboot.h>
#include <logging/log_ctrl.h>
#include <net/bsdlib.h>
#include <bsd.h>
#include <lte_lc.h>
#include <net/cloud.h>
#include <net/socket.h>

static struct cloud_backend *cloud_backend;
static struct k_delayed_work cloud_update_work;

enum error_type {
	ERROR_CLOUD,
	ERROR_BSD_RECOVERABLE,
	ERROR_LTE_LC,
	ERROR_SYSTEM_FAULT
};

/**@brief AWS IoT Cloud error handler. */
void error_handler(enum error_type err_type, int err_code)
{

#if !defined(CONFIG_DEBUG) && defined(CONFIG_REBOOT)
	LOG_PANIC();
	sys_reboot(0);
#else
	switch (err_type) {
	case ERROR_CLOUD:
		printk("Error of type ERROR_CLOUD: %d\n", err_code);
		break;
	case ERROR_BSD_RECOVERABLE:
		printk("Error of type ERROR_BSD_RECOVERABLE: %d\n", err_code);
		break;
	case ERROR_SYSTEM_FAULT:
		printk("Error of type ERRO_SSYTEM_FAULT: %d\n", err_code);
	default:
		printk("Unknown error type: %d, code: %d\n",
			err_type, err_code);
		break;
	}

	while (true) {
		k_cpu_idle();
	}
#endif /* CONFIG_DEBUG */
}

void k_sys_fatal_error_handler(unsigned int reason,
			       const z_arch_esf_t *esf)
{
	ARG_UNUSED(esf);

	LOG_PANIC();
	printk("Running main.c error handler");
	error_handler(ERROR_SYSTEM_FAULT, reason);
	CODE_UNREACHABLE;
}

void cloud_error_handler(int err)
{
	error_handler(ERROR_CLOUD, err);
}

/**@brief Recoverable BSD library error. */
void bsd_recoverable_error_handler(uint32_t err)
{
	error_handler(ERROR_BSD_RECOVERABLE, (int)err);
}

/**@brief Reboot the device if CONNACK has not arrived. */
static void cloud_reboot_handler(struct k_work *work)
{
	error_handler(ERROR_CLOUD, -ETIMEDOUT);
}

static void cloud_update_routine_work(struct k_work *work)
{
	int err;

	struct cloud_msg msg = {
		.qos = CLOUD_QOS_AT_MOST_ONCE,
		.endpoint.type = CLOUD_EP_TOPIC_MSG,
		.buf = CONFIG_CLOUD_MESSAGE,
		.len = sizeof(CONFIG_CLOUD_MESSAGE)
	};

	err = cloud_send(cloud_backend, &msg);
	if (err) {
		printk("cloud_send failed, error: %d\n", err);
	}

	k_delayed_work_submit(cloud_update_routine_work,
			      K_SECONDS(CLOUD_MESSAGE_PUBLICATION_INTERVAL));
}

void cloud_event_handler(const struct cloud_backend *const backend,
			 const struct cloud_event *const evt,
			 void *user_data)
{
	ARG_UNUSED(user_data);

	switch (evt->type) {
	case CLOUD_EVT_CONNECTED:
		printk("CLOUD_EVT_CONNECTED\n");
		k_delayed_work_submit(&cloud_update_routine_work);
		break;
	case CLOUD_EVT_READY:
		printk("CLOUD_EVT_READY\n");
		break;
	case CLOUD_EVT_DISCONNECTED:
		printk("CLOUD_EVT_DISCONNECTED\n");
		break;
	case CLOUD_EVT_ERROR:
		printk("CLOUD_EVT_ERROR\n");
		break;
	case CLOUD_EVT_DATA_SENT:
		printk("CLOUD_EVT_DATA_SENT\n");
		break;
	case CLOUD_EVT_DATA_RECEIVED:
		printk("CLOUD_EVT_DATA_RECEIVED\n");
		break;
	case CLOUD_EVT_PAIR_REQUEST:
		printk("CLOUD_EVT_PAIR_REQUEST\n");
		break;
	case CLOUD_EVT_PAIR_DONE:
		printk("CLOUD_EVT_PAIR_DONE\n");
		break;
	case CLOUD_EVT_FOTA_DONE:
		printk("CLOUD_EVT_FOTA_DONE\n");
		break;
	default:
		printk("Unknown cloud event type: %d\n", evt->type);
		break;
	}
}

/**@brief Initializes and submits delayed work. */
static void work_init(void)
{
	k_delayed_work_init(&cloud_update_routine_work, cloud_update_routine_work_fn);
}

/**@brief Configures modem to provide LTE link. Blocks until link is
 * successfully established.
 */
static void modem_configure(void)
{
#if defined(CONFIG_BSD_LIBRARY)
	if (IS_ENABLED(CONFIG_LTE_AUTO_INIT_AND_CONNECT)) {
		/* Do nothing, modem is already turned on
		 * and connected.
		 */
	} else {
		int err;

		printk("Connecting to LTE network. ");
		printk("This may take several minutes.\n");

		err = lte_lc_init_and_connect();
		if (err) {
			printk("LTE link could not be established.\n");
			error_handler(ERROR_LTE_LC, err);
		}

		printk("Connected to LTE network\n");
	}
#endif
}

void main(void)
{
	int ret;

	printk("AWS IoT simple started\n");

	cloud_backend = cloud_get_binding("AWS_IOT");
	__ASSERT(cloud_backend != NULL, "AWS IoT Cloud backend not found");

	ret = cloud_init(cloud_backend, cloud_event_handler);
	if (ret) {
		printk("Cloud backend could not be initialized, error: %d\n",
			ret);
		cloud_error_handler(ret);
	}

	work_init();
	modem_configure();
connect:
	ret = cloud_connect(cloud_backend);
	if (ret) {
		printk("cloud_connect failed: %d\n", ret);
		cloud_error_handler(ret);
	}

	struct pollfd fds[] = {
		{
			.fd = cloud_backend->config->socket,
			.events = POLLIN
		}
	};

	while (true) {
		/* The timeout is set to (keepalive / 3), so that the worst case
		 * time between two messages from device to broker is
		 * ((4 / 3) * keepalive + connection overhead), which is within
		 * MQTT specification of (1.5 * keepalive) before the broker
		 * must close the connection.
		 */
		ret = poll(fds, ARRAY_SIZE(fds),
			K_SECONDS(CONFIG_MQTT_KEEPALIVE / 3));

		if (ret < 0) {
			printk("poll() returned an error: %d\n", ret);
			error_handler(ERROR_CLOUD, ret);
			continue;
		}

		if (ret == 0) {
			cloud_ping(cloud_backend);
			continue;
		}

		if ((fds[0].revents & POLLIN) == POLLIN) {
			cloud_input(cloud_backend);
		}

		if ((fds[0].revents & POLLNVAL) == POLLNVAL) {
			printk("Socket error: POLLNVAL\n");
			printk("The cloud socket was unexpectedly closed.\n");
			error_handler(ERROR_CLOUD, -EIO);
			return;
		}

		if ((fds[0].revents & POLLHUP) == POLLHUP) {
			printk("Socket error: POLLHUP\n");
			printk("Connection was closed by the cloud.\n");
			error_handler(ERROR_CLOUD, -EIO);
			return;
		}

		if ((fds[0].revents & POLLERR) == POLLERR) {
			printk("Socket error: POLLERR\n");
			printk("Cloud connection was unexpectedly closed.\n");
			error_handler(ERROR_CLOUD, -EIO);
			return;
		}
	}

	cloud_disconnect(cloud_backend);
	goto connect;
}
