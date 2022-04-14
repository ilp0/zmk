/*
 * Copyright (c) 2020 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#define DT_DRV_COMPAT zmk_mouse_sensor

#include <device.h>
#include <drivers/sensor.h>
#include <logging/log.h>
#include "mouse_sensor.h"

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

#if DT_HAS_COMPAT_STATUS_OKAY(DT_DRV_COMPAT)

#define STACK_SIZE 1024

static void mouse_sensor_thread_code(void *p1, void *p2, void *p3) {
    struct mouse_sensor_data *const data = (struct mouse_sensor_data *const)p1;
    k_timer_start(data->timer, K_MSEC(8), K_MSEC(8));

    while (1) {
        if (sensor_sample_fetch(data->sensor) < 0) {
            LOG_DBG("Error fetching sample");
            continue;
        }

        struct sensor_value dx;
        struct sensor_value dy;
        if (sensor_channel_get(data->sensor, SENSOR_CHAN_POS_DX, &dx) < 0) {
            LOG_DBG("Unable to get mouse dX");
            continue;
        }

        if (sensor_channel_get(data->sensor, SENSOR_CHAN_POS_DY, &dy) < 0) {
            LOG_DBG("Unable to get mouse dY");
            continue;
        }

        zmk_hid_mouse_movement_set(0, 0);
        zmk_hid_mouse_scroll_set(0, 0);
        zmk_hid_mouse_movement_update((int8_t)dx.val1, (int8_t)dy.val1);
        zmk_endpoints_send_mouse_report();
        k_timer_status_sync(data->timer);
    }
}

static int mouse_sensor_init(const struct device *dev) {
    const struct mouse_sensor_config *const cfg = dev->config;
    struct mouse_sensor_data *const data = (struct mouse_sensor_data *const)dev->data;

    LOG_DBG("mouse_sensor_init");

    const struct device *sensor = device_get_binding(cfg->sensor_label);
    if (!dev) {
        LOG_WRN("Failed to load mouse sensor device %s", log_strdup(cfg->sensor_label));
    }

    data->sensor = sensor;

    k_thread_create(&data->thread, data->thread_stack, STACK_SIZE, mouse_sensor_thread_code,
                    data, NULL, NULL, K_PRIO_PREEMPT(8), 0, K_NO_WAIT);

    return 0;
}

static const struct mouse_sensor_driver_api mouse_sensor_driver_api = {
};

#define MOUSE_SENSOR_INIT(inst)                                                                    \
    static struct mouse_sensor_config mouse_sensor_##inst##_config = {                                     \
        .sensor_label = DT_LABEL(DT_INST_PHANDLE(inst, sensor)),                                         \
    };                                                                                             \
                                                                                                   \
    K_TIMER_DEFINE(mouse_sensor_##inst##_timer, NULL, NULL);  \
    static K_THREAD_STACK_DEFINE(mouse_sensor_##inst##_thread_stack, STACK_SIZE); \
    static struct mouse_sensor_data mouse_sensor_##inst##_data = {                                  \
        .timer = &mouse_sensor_##inst##_timer, \
        .thread_stack = mouse_sensor_##inst##_thread_stack, \
    };                                                                                             \
                                                                                                   \
    \
    \
    /* This has to init after SPI master */                                                        \
    DEVICE_DT_INST_DEFINE(inst, mouse_sensor_init, device_pm_control_nop, &mouse_sensor_##inst##_data,  \
                          &mouse_sensor_##inst##_config, APPLICATION,                                  \
                          CONFIG_APPLICATION_INIT_PRIORITY, &mouse_sensor_driver_api);

DT_INST_FOREACH_STATUS_OKAY(MOUSE_SENSOR_INIT)

#endif /* DT_HAS_COMPAT_STATUS_OKAY(DT_DRV_COMPAT) */ 