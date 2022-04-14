#include <drivers/sensor.h>
#include <logging/log.h>
#include <zmk/hid.h>
#include <zmk/endpoints.h>
#include <zmk/keymap.h>
#include <dt-bindings/zmk/mouse.h>
#include "trackpad.h"
#include <drivers/i2c.h>


LOG_MODULE_REGISTER(trackpad, CONFIG_SENSOR_LOG_LEVEL);

const struct device *trackpad = DEVICE_DT_GET(DT_INST(0, azoteq_iqs5xx));


//uint16_t	ui16SnapStatus[15], ui16PrevSnap[15];
uint8_t		stop = 1;



static bool RDY_wait() {
    return true;
}

static void handle_trackpad(const struct device *dev, struct sensor_trigger *trig) {

    int ret = sensor_sample_fetch(dev);
    if (ret < 0) {
        LOG_ERR("fetch: %d", ret);
        return;
    }

    if(RDY_wait() == true) {
        i2c_read(trackpad, &Data_Buff[0], 44, GestureEvents0_adr);
    }

    return 0;
    //data iqs5xx_data;

}

static bool trackpad_init() {
    printk("\ntrackpad setup start\n");

    struct sensor_trigger trigger = {
        .type = SENSOR_TRIG_DATA_READY,
        .chan = SENSOR_CHAN_ALL,
    };

    if (sensor_trigger_set(trackpad, &trigger, handle_trackpad) < 0) {
        LOG_ERR("can't set trigger");
        return -EIO;
    };

    i2c_write(trackpad, &activeRefreshRate[0], 2, ActiveRR_adr);
    i2c_write(trackpad, &stop, 1, END_WINDOW);
    i2c_write(trackpad, &idleRefreshRate[0], 2, IdleRR_adr);
    i2c_write(trackpad, &stop, 1, END_WINDOW);
    i2c_write(trackpad,  &idleRefreshRate[0], 2, IdleTouchRR_adr);
    i2c_write(trackpad, &stop, 1, END_WINDOW);
    i2c_write(trackpad, &idleRefreshRate[0], 2, LP1RR_adr);
    i2c_write(trackpad, &stop, 1, END_WINDOW);
    i2c_write(trackpad, &idleRefreshRate[0], 2, LP2RR_adr);
    i2c_write(trackpad, &stop, 1, END_WINDOW);

    return 0;

}

SYS_INIT(trackpad_init, APPLICATION, CONFIG_ZMK_KSCAN_INIT_PRIORITY);