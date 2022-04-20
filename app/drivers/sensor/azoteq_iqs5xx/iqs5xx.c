
/*
 * Copyright (c) 2020 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#define DT_DRV_COMPAT azoteq_iqs5xx

#include <drivers/gpio.h>
#include <sys/util.h>
#include <kernel.h>
#include <sys/__assert.h>
#include <logging/log.h>
#include <drivers/i2c.h>
#include <init.h>
#include <device.h>
#include <drivers/sensor.h>
#include <logging/log.h>
#include "iqs5xx.h"
#include <devicetree.h>

LOG_MODULE_REGISTER(iqs5xx, CONFIG_SENSOR_LOG_LEVEL);

//todo report mouse to driver


static bool iqs5xx_init(const struct device *dev) {
    printk("\ntrackpad setup start\n");

	uint8_t     activeRefreshRate[2] = {0,8};
	uint8_t     idleRefreshRate[2] = {0,8};
	uint8_t		stop = 1;

	i2c_write(AZOTEQ_IQS5XX_ADDR, &activeRefreshRate[0], 2, ActiveRR_adr);
    i2c_write(AZOTEQ_IQS5XX_ADDR, &stop, 1, END_WINDOW);
    i2c_write(AZOTEQ_IQS5XX_ADDR, &idleRefreshRate[0], 2, IdleRR_adr);
    i2c_write(AZOTEQ_IQS5XX_ADDR, &stop, 1, END_WINDOW);
    i2c_write(AZOTEQ_IQS5XX_ADDR,  &idleRefreshRate[0], 2, IdleTouchRR_adr);
    i2c_write(AZOTEQ_IQS5XX_ADDR, &stop, 1, END_WINDOW);
    i2c_write(AZOTEQ_IQS5XX_ADDR, &idleRefreshRate[0], 2, LP1RR_adr);
    i2c_write(AZOTEQ_IQS5XX_ADDR, &stop, 1, END_WINDOW);
    i2c_write(AZOTEQ_IQS5XX_ADDR, &idleRefreshRate[0], 2, LP2RR_adr);
    i2c_write(AZOTEQ_IQS5XX_ADDR, &stop, 1, END_WINDOW);

    
    struct iqs5xx_dev_data *data = dev->data;
	const struct iqs5xx_dev_config *cfg = dev->config;

	return 0;

}

static int iqs5xx_read_reg(const struct device *dev, uint8_t reg,
			      void *data, size_t length)
{
	uint8_t tp_data[44];
	i2c_burst_read(dev, AZOTEQ_IQS5XX_ADDR, GestureEvents0_adr, &tp_data, 44);
	uint8_t 	i; 
	static uint8_t ui8FirstTouch = 0;
	uint8_t 	ui8NoOfFingers;
	uint8_t 	ui8SystemFlags[2];

	struct iqs5xx_sample d;

	d.ui8NoOfFingers 	= 	tp_data[4];


	if(d.ui8NoOfFingers != 0) 
	{
		if (!(ui8FirstTouch)) 
		{
			ui8FirstTouch = 1;
		}
        //gesture bank 1
		switch (tp_data[0])
		{
			case SINGLE_TAP		:  	d.gesture = SINGLE_TAP;
									break;
			case TAP_AND_HOLD	:  	d.gesture = TAP_AND_HOLD;
									break;
			case SWIPE_X_NEG	:  	d.gesture = SWIPE_X_NEG;
									break;
			case SWIPE_X_POS	:  	d.gesture = SWIPE_X_POS;
									break;
			case SWIPE_Y_POS	:  	d.gesture = SWIPE_Y_POS;
									break;
			case SWIPE_Y_NEG	:  	d.gesture = SWIPE_Y_NEG;
									break;
		}

        //gesture bank 2
		switch (tp_data[1])
		{
			case TWO_FINGER_TAP	:  	d.gesture = TWO_FINGER_TAP;
									break;
			case SCROLLG:  			d.gesture = SCROLLG;
									break;
			case ZOOM			:  	d.gesture = ZOOM;
									break;
		}

        //calculate relative data

		d.i16RelX[1] = ((tp_data[5] << 8) | (tp_data[6]));
		d.i16RelY[1] = ((tp_data[7] << 8) | (tp_data[8]));   

        //calculate absolute position of max 5 fingers
        
		for (i = 0; i < 5; i++)
		{
			d.ui16AbsX[i+1] = ((tp_data[(7*i)+9] << 8) | (tp_data[(7*i)+10])); //9-16-23-30-37//10-17-24-31-38
			d.ui16AbsY[i+1] = ((tp_data[(7*i)+11] << 8) | (tp_data[(7*i)+12])); //11-18-25-32-39//12-19-26-33-40
			d.ui16TouchStrength[i+1] = ((tp_data[(7*i)+13] << 8) | (tp_data[(7*i)+14])); //13-20-27-34-11/14-21-28-35-42
			//d.1ui8Area[i+1] = (tp_data[7*i+15]); //15-22-29-36-43
		}
	} 
	else 
	{
		ui8FirstTouch = 0;
	}

	struct iqs5xx_dev_data *iqs5xx_data = dev->data;
	return &d;

}

static int iqs5xx_sample_fetch(const struct device *dev,
				enum sensor_channel chan)
{
    struct iqs5xx_dev_data *data = dev->data;
    return iqs5xx_read_reg(&dev, GestureEvents0_adr, &data->sample, sizeof(data->sample));
}

static int iqs5xx_channel_get(const struct device *dev,
			       enum sensor_channel chan,
			       struct sensor_value *val)
{
	struct iqs5xx_dev_data *data = dev->data;

	switch (chan) {
	case SENSOR_CHAN_POS_DX:
		val->val1 = data->sample.i16RelX;
        val->val2 = 0;
		break;
	case SENSOR_CHAN_POS_DY:
		val->val1 = data->sample.i16RelY;
        val->val2 = 0;
		break;
	default:
		return -ENOTSUP;
	}

	return 0;
}

static const struct sensor_driver_api iqs5xx_api_funcs = {
	.sample_fetch = iqs5xx_sample_fetch,
	.channel_get = iqs5xx_channel_get,
};



static struct iqs5xx_dev_data iqs5xx_data;

static const struct iqs5xx_dev_config iqs5xx_config = {
	.addr = AZOTEQ_IQS5XX_ADDR
};

DEVICE_DT_INST_DEFINE(0, iqs5xx_init, device_pm_control_nop,
		    &iqs5xx_data, &iqs5xx_config, POST_KERNEL,
		    CONFIG_SENSOR_INIT_PRIORITY, &iqs5xx_api_funcs);
