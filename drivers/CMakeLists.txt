#
# Copyright (c) 2018-2019 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
#

add_subdirectory_ifdef(CONFIG_AT_CMD at_cmd)
add_subdirectory_ifdef(CONFIG_GPS_SIM gps_sim)
add_subdirectory_ifdef(CONFIG_LTE_LINK_CONTROL lte_link_control)
add_subdirectory_ifdef(CONFIG_SENSOR sensor)
add_subdirectory_ifdef(CONFIG_NETWORKING net)
add_subdirectory_ifdef(CONFIG_ADP536X adp536x)
add_subdirectory_ifdef(CONFIG_ST25R3911B_LIB st25r3911b)
add_subdirectory_ifdef(CONFIG_BT_LL_NRFXLIB bt_ll_nrfxlib)
add_subdirectory_ifdef(CONFIG_CLOCK_CONTROL_NRFXLIB clock_control)
add_subdirectory_ifdef(CONFIG_NRF9160_GPS nrf9160_gps)
add_subdirectory_ifdef(CONFIG_FPROTECT fprotect)
add_subdirectory(flash_patch)
add_subdirectory_ifdef(CONFIG_HW_CC310 hw_cc310)
add_subdirectory(entropy)
