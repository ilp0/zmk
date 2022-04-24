
/*
 * Copyright (c) 2020 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#define DT_DRV_COMPAT azoteq_iqs5xx

#include <drivers/gpio.h>
#include <init.h>
#include <drivers/sensor.h>
#include <logging/log.h>
#include <drivers/i2c.h>
#include <init.h>
#include <device.h>
#include "iqs5xx.h"
#include <drivers/sensor.h>
#include <devicetree.h>

// DT_DRV_INST(0);
LOG_MODULE_REGISTER(azoteq_iqs5xx, 1);

/**
 * @brief Read from the iqs550 chip via i2c
 * example: iqs5xx_seq_read(dev, GestureEvents0_adr, buffer, 44)
 *
 * @param dev Pointer to device driver MASTER
 * @param start start address for reading
 * @param  pointer to buffer to be read into
 * @param len number of bytes to read
 * @return int
 */
static int iqs5xx_seq_read(const struct device *dev, const uint16_t start, uint8_t *read_buf,
                           const uint8_t len) {
    LOG_ERR("\nseq read iqs5xx");
    const struct iqs5xx_data *data = dev->data;
    const struct iqs5xx_config *config = dev->config;
    return i2c_write_read(data->i2c, AZOTEQ_IQS5XX_ADDR, &start, sizeof(start), read_buf, len);
}

/**
 * @brief Write to the iqs550 chip via i2c
 * example: iqs5xx_write(dev, GestureEvents0_adr, buffer, 44)
 * @param dev Pointer to device driver MASTER
 * @param start address of the i2c slave
 * @param buf Buffer to be written
 * @param len number of bytes to write
 * @return int
 */
static int iqs5xx_write(const struct device *dev, const uint16_t start_addr, uint8_t *buf,
                        uint32_t num_bytes) {

    LOG_ERR("\nwrite iqs5xx");
    const struct iqs5xx_data *data = dev->data;
    // const struct iqs5xx_config *config = dev->config;
    /*
        uint8_t nbuf[64];
        nbuf[1] = addr & 0xFF;
        nbuf[1] = addr >> 8;
        for(int i = 0; i < len; i++) {
            nbuf[i + 2] = buf[i];
        }

        int err = i2c_write(data->i2c, (const uint8_t*)nbuf, len + 2, AZOTEQ_IQS5XX_ADDR);
        LOG_ERR("\n%d", err);

        return err;*/
    LOG_ERR("\nwrite iqs5xx");

    uint8_t addr_buffer[2];
    struct i2c_msg msg[2];

    addr_buffer[1] = start_addr & 0xFF;
    addr_buffer[0] = start_addr >> 8;
    msg[0].buf = addr_buffer;
    msg[0].len = 2U;
    msg[0].flags = I2C_MSG_WRITE;

    msg[1].buf = (uint8_t *)buf;
    msg[1].len = num_bytes;
    msg[1].flags = I2C_MSG_WRITE | I2C_MSG_STOP;

    int err = i2c_transfer(data->i2c, msg, 2, AZOTEQ_IQS5XX_ADDR);
    LOG_ERR("\n%d", err);
    return err;
}

static int iqs5xx_attr_set(const struct device *dev, enum sensor_channel chan,
                           enum sensor_attribute attr, const struct sensor_value *val) {
	LOG_ERR("\nSetting attributes\n");
    const struct iqs5xx_config *config = dev->config;
    return 0;
}

/**
 * @brief Get sensor channel. Not to be confused with trackpad internal channel.
 *
 * @param dev
 * @param chan
 * @param val
 * @return int
 */
static int iqs5xx_channel_get(const struct device *dev, enum sensor_channel chan,
                              struct sensor_value *val) {
    LOG_ERR("\nCHANNEL GET");

    const struct iqs5xx_data *data = dev->data;
    switch (chan) {
    case SENSOR_CHAN_POS_DX:
        val->val1 = data->rx;
        break;
    case SENSOR_CHAN_POS_DY:
        val->val1 = data->ry;
        break;
    case SENSOR_CHAN_POS_DZ:
        val->val1 = data->gesture;
        break;
    default:
        return -ENOTSUP;
    }
    return 0;
}

static int iqs5xx_sample_fetch(const struct device *dev, enum sensor_channel chan) {
    LOG_ERR("\nSAMPLE FETCH");

    uint8_t buffer[44];
    int res = iqs5xx_seq_read(dev, GestureEvents0_adr, buffer, 44);
    if (res < 0) {
        LOG_ERR("\ntrackpad res: %d", res);
        return res;
    }

    struct iqs5xx_data *data = dev->data;
    // define struct to make data harvest easier
    struct iqs5xx_sample {
        int16_t i16RelX[6];
        int16_t i16RelY[6];
        uint16_t ui16AbsX[6];
        uint16_t ui16AbsY[6];
        uint16_t ui16TouchStrength[6];
        uint8_t ui8NoOfFingers;
        uint8_t gesture;
    } d;

    d.ui8NoOfFingers = buffer[4];
    uint8_t i;
    static uint8_t ui8FirstTouch = 0;
    uint8_t ui8NoOfFingers;
    if (d.ui8NoOfFingers != 0) {
        if (!(ui8FirstTouch)) {
            ui8FirstTouch = 1;
        }
        // gesture bank 1
        switch (buffer[0]) {
        case SINGLE_TAP:
            d.gesture = SINGLE_TAP;
            break;
        case TAP_AND_HOLD:
            d.gesture = TAP_AND_HOLD;
            break;
        case SWIPE_X_NEG:
            d.gesture = SWIPE_X_NEG;
            break;
        case SWIPE_X_POS:
            d.gesture = SWIPE_X_POS;
            break;
        case SWIPE_Y_POS:
            d.gesture = SWIPE_Y_POS;
            break;
        case SWIPE_Y_NEG:
            d.gesture = SWIPE_Y_NEG;
            break;
        }
        LOG_ERR("\nGESTURE %d", d.gesture);
        // gesture bank 2
        switch (buffer[1]) {
        case TWO_FINGER_TAP:
            d.gesture = TWO_FINGER_TAP;
            break;
        case SCROLLG:
            d.gesture = SCROLLG;
            break;
        case ZOOM:
            d.gesture = ZOOM;
            break;
        }

        // calculate relative data

        d.i16RelX[1] = ((buffer[5] << 8) | (buffer[6]));
        d.i16RelY[1] = ((buffer[7] << 8) | (buffer[8]));
        LOG_ERR("\nX %d", d.i16RelX[1]);
        LOG_ERR("\nY %d", d.i16RelY[1]);

        // calculate absolute position of max 5 fingers

        for (i = 0; i < 5; i++) {
            d.ui16AbsX[i + 1] = ((buffer[(7 * i) + 9] << 8) |
                                 (buffer[(7 * i) + 10])); // 9-16-23-30-37//10-17-24-31-38
            d.ui16AbsY[i + 1] = ((buffer[(7 * i) + 11] << 8) |
                                 (buffer[(7 * i) + 12])); // 11-18-25-32-39//12-19-26-33-40
            d.ui16TouchStrength[i + 1] = ((buffer[(7 * i) + 13] << 8) |
                                          (buffer[(7 * i) + 14])); // 13-20-27-34-11/14-21-28-35-42
            // d.1ui8Area[i+1] = (buffer[7*i+15]); //15-22-29-36-43
        }
    } else {
        ui8FirstTouch = 0;
    }
    // set data to device data portion
    data->rx = (int16_t)(int8_t)d.i16RelX[1];
    data->ry = (int16_t)(int8_t)d.i16RelY[1];
    data->ax = d.ui16AbsX[1];
    data->ay = d.ui16AbsY[1];
    data->gesture = d.gesture;

    return 0;
}

/**
 * @brief Set the interrupt
 *
 * @param dev
 * @param en
 */
static void set_int(const struct device *dev, const bool en) {
	LOG_ERR("\nSetting interrupt\n");
    const struct iqs5xx_config *config = dev->config;
    int ret = gpio_pin_interrupt_configure(config->dr_port, config->dr_pin,
                                           en ? GPIO_INT_LEVEL_ACTIVE : GPIO_INT_DISABLE);
    if (ret < 0) {
        LOG_ERR("\ncan't set interrupt\n");
    }
}

/**
 * @brief set trigger for interrupt
 *
 * @param dev
 * @param trig
 * @param handler
 * @return int
 */
static int iqs5xx_trigger_set(const struct device *dev, const struct sensor_trigger *trig,
                              sensor_trigger_handler_t handler) {
    struct iqs5xx_data *data = dev->data;
    LOG_ERR("\nTRIGGER_SET");

    set_int(dev, false);
    if (trig->type != SENSOR_TRIG_DATA_READY) {
        LOG_ERR("\nENOTSUP");

        return -ENOTSUP;
    }
    LOG_ERR("\nNOT ENOTSUP");

    data->data_ready_trigger = trig;
    data->data_ready_handler = handler;
    set_int(dev, true);
    return 0;
}

static void iqs5xx_int_cb(const struct device *dev) {
    struct iqs5xx_data *data = dev->data;
    data->data_ready_handler(dev, data->data_ready_trigger);
    LOG_ERR("\nINT_CB");
    set_int(dev, true);
}

static void iqs5xx_thread(void *arg) {
    const struct device *dev = arg;
    struct iqs5xx_data *data = dev->data;
    LOG_ERR("\nSTART THREAD");
    while (1) {
        k_sem_take(&data->gpio_sem, K_FOREVER);
        iqs5xx_int_cb(dev);
        iqs5xx_write(dev, END_WINDOW, 0, 1);
    }
}

static void iqs5xx_gpio_cb(const struct device *port, struct gpio_callback *cb, uint32_t pins) {
    LOG_ERR("\nGPIO_CB");

    struct iqs5xx_data *data = CONTAINER_OF(cb, struct iqs5xx_data, gpio_cb);
    const struct device *dev = data->dev;
    k_sem_give(&data->gpio_sem);
}

//#define I2C_BUS DT_BUS(DT_NODELABEL(trackpad))
//#define I2C_REG DT_REG_ADDR(DT_NODELABEL(trackpad))

static int iqs5xx_init(const struct device *dev) {
    LOG_ERR("\nstarting trackpad INIT\n");
    struct iqs5xx_data *data = dev->data;
    const struct iqs5xx_config *config = dev->config;

    data->dev = dev;

	uint8_t activeRefreshRate[2] = {0, 8};
    uint8_t idleRefreshRate[2] = {0, 16};
    uint8_t stop = 1;
	iqs5xx_write(dev, ActiveRR_adr, &activeRefreshRate[0], 2);
	 LOG_ERR("\nactiverr\n");
	iqs5xx_write(dev, END_WINDOW, 0, 1);
	LOG_ERR("\nend_window\n");
    iqs5xx_write(dev, IdleRR_adr, &idleRefreshRate[0], 2);
	LOG_ERR("\nidlerr\n");
    iqs5xx_write(dev, END_WINDOW, 0, 1);
	LOG_ERR("\nend_window1\n");
    iqs5xx_write(dev, IdleTouchRR_adr, &idleRefreshRate[0], 2);
	LOG_ERR("\nidlerr2\n");
    iqs5xx_write(dev, END_WINDOW, 0, 1);
	LOG_ERR("\nend_window2\n");
    iqs5xx_write(dev, LP2RR_adr, &activeRefreshRate[0], 2);
	LOG_ERR("\nactiverr2\n");
    iqs5xx_write(dev, END_WINDOW, 0, 1);
	LOG_ERR("\nend_window3\n");
    iqs5xx_write(dev, LP2RR_adr, &activeRefreshRate[0], 2);
	LOG_ERR("\nactiverr3\n");
    iqs5xx_write(dev, END_WINDOW, 0, 1);
	LOG_ERR("\nend_window2\n");

    gpio_pin_configure(config->dr_port, config->dr_pin, GPIO_INPUT | config->dr_flags);
    gpio_init_callback(&data->gpio_cb, iqs5xx_gpio_cb, BIT(config->dr_pin));
    int ret = gpio_add_callback(config->dr_port, &data->gpio_cb);
    if (ret < 0) {
        LOG_ERR("Failed to set DR callback: %d", ret);
        return -EIO;
    }

    k_sem_init(&data->gpio_sem, 0, UINT_MAX);

    k_thread_create(&data->thread, data->thread_stack, 1024, (k_thread_entry_t)iqs5xx_thread,
                    (void *)dev, 0, NULL, K_PRIO_COOP(CONFIG_IQS5XX_THREAD_PRIORITY), 0, K_NO_WAIT);
    return 0;
}

static const struct sensor_driver_api iqs5xx_driver_api = {
    .trigger_set = iqs5xx_trigger_set,
    .sample_fetch = iqs5xx_sample_fetch,
    .channel_get = iqs5xx_channel_get,
    .attr_set = iqs5xx_attr_set,
};

static const struct iqs5xx_data iqs5xx_data = {
    .i2c = DEVICE_DT_GET(DT_BUS(DT_DRV_INST(0))),
};

static const struct iqs5xx_config iqs5xx_config = {
    .dr_port = DEVICE_DT_GET(DT_GPIO_CTLR(DT_NODELABEL(trackpad), dr_gpios)),
    .dr_pin = DT_INST_GPIO_PIN(0, dr_gpios),
    .dr_flags = DT_INST_GPIO_FLAGS(0, dr_gpios),
};

DEVICE_DT_INST_DEFINE(0, iqs5xx_init, device_pm_control_nop, &iqs5xx_data, &iqs5xx_config,
                      POST_KERNEL, 90, &iqs5xx_driver_api);