/*
 * Copyright (c) 2020 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <device.h>
#include <sys/util.h>

__subsystem struct mouse_sensor_driver_api {
};

struct mouse_sensor_data {
	const struct device *sensor;
	struct k_timer *timer;
	struct k_thread thread;
	k_thread_stack_t *thread_stack;
};

struct mouse_sensor_config {
	const char *sensor_label;
}; 