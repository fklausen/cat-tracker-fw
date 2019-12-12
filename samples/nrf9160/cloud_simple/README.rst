.. _aws_iot_simple:

nRF9160: Cloud simple
#####################

The AWS IoT simple sample demonstrates how to use the :ref:`lib_aws_iot` to connect an nRF9160-based board to the `AWS IoT` Cloud service via LTE

Overview
********

Write about the behaviour of the example, TLS, credentials must be mentioned, link to how to flash them, change the endpoint accordingly

Utilized functions
******************

The Asset Tracker can run in three power modes that are configured in the Kconfig file of the application.
These settings are currently only supported on the nRF9160 DK.

.. note::
   Not all cellular network providers support these modes, and the granted parameters can vary between networks.

Demo mode
	This is the default setting.
	In this mode, the device maintains a continuous cellular link.
	To enable this mode, set ``CONFIG_POWER_OPTIMIZATION_ENABLE=n``.

Request eDRX mode
	In this mode, the device requests the eDRX feature from the cellular network to save power.
	To enable this mode, set ``CONFIG_POWER_OPTIMIZATION_ENABLE=y`` and then
	set Switch 2 to the N.C. position.

Request Power Saving Mode (PSM)
	In this mode, the device requests the PSM feature from the cellular network to save power.
	To enable this mode, set ``CONFIG_POWER_OPTIMIZATION_ENABLE=y`` and then
	set Switch 2 to the GND position.

Curremt dependencies
********************

This application uses the following |NCS| libraries and drivers:

    * :ref:`lib_nrf_cloud`
    * :ref:`modem_info_readme`
    * :ref:`at_cmd_parser_readme`
    * ``drivers/nrf9160_gps``
    * ``lib/bsd_lib``
    * ``drivers/sensor/sensor_sim``
    * :ref:`dk_buttons_and_leds_readme`
    * ``drivers/lte_link_control``

In addition, it uses the Secure Partition Manager sample:

* :ref:`secure_partition_manager`
