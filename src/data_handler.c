#include "data_handler.h"
#include <string.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(gps_data, LOG_LEVEL_DBG);

// Message queue for SD card logging
K_MSGQ_DEFINE(sensor_data_msgq, sizeof(struct sensor_data), 10, 4);

// Current GPS state
// static struct gps_data gps_data = {0};
// static struct acc_data acc_data = {0};
// static struct compass_data compass_data = {0};
static struct sensor_data sensor_data = {0};


static K_MUTEX_DEFINE(gps_data_mutex);
static K_MUTEX_DEFINE(acc_data_mutex);
static K_MUTEX_DEFINE(compass_data_mutex);


void add_to_msgq(){
    if (sensor_data.gps_data.new && sensor_data.compass_data.new && sensor_data.acc_data.new){
        if (k_msgq_put(&sensor_data_msgq, &sensor_data, K_NO_WAIT) != 0) {
            LOG_WRN("SD card queue full");
        }
        else{
            sensor_data.gps_data.new = false;
            sensor_data.compass_data.new = false;
            sensor_data.acc_data.new = false;
        }
    }
}


void set_gps_data(struct gps_data source)
{
    // Update displayed data
    k_mutex_lock(&gps_data_mutex, K_FOREVER);
    sensor_data.gps_data = source;
    k_mutex_unlock(&gps_data_mutex);
    add_to_msgq();

}


bool get_gps_data(struct gps_data *dest){
    k_mutex_lock(&gps_data_mutex, K_FOREVER);
    if (sensor_data.gps_data.valid) {
        memcpy(dest, &sensor_data.gps_data, sizeof(struct gps_data));
        k_mutex_unlock(&gps_data_mutex);
        return true;
    }
    k_mutex_unlock(&gps_data_mutex);
    return false;
}


void set_acc_data(struct acc_data source)
{
    // Update displayed data
    k_mutex_lock(&acc_data_mutex, K_FOREVER);
    sensor_data.acc_data = source;
    k_mutex_unlock(&acc_data_mutex);
    add_to_msgq();

}


bool get_acc_data(struct acc_data *dest){
    k_mutex_lock(&acc_data_mutex, K_FOREVER);
    if (sensor_data.acc_data.valid) {
        memcpy(dest, &sensor_data.acc_data, sizeof(struct acc_data));
        k_mutex_unlock(&acc_data_mutex);
        return true;
    }
    k_mutex_unlock(&acc_data_mutex);
    return false;
}


void set_compass_data(struct compass_data source)
{
    // Update displayed data
    k_mutex_lock(&compass_data_mutex, K_FOREVER);
    sensor_data.compass_data = source;
    k_mutex_unlock(&compass_data_mutex);
    add_to_msgq();
}


bool get_compass_data(struct compass_data *dest){
    k_mutex_lock(&compass_data_mutex, K_FOREVER);
    if (sensor_data.compass_data.valid) {
        memcpy(dest, &sensor_data.compass_data, sizeof(struct compass_data));
        k_mutex_unlock(&compass_data_mutex);
        return true;
    }
    k_mutex_unlock(&compass_data_mutex);
    return false;
}


void invalidate_sensor_data(){
    sensor_data.acc_data.valid = false;
    sensor_data.acc_data.new = false;

    sensor_data.compass_data.valid = false;
    sensor_data.compass_data.new = false;

    sensor_data.gps_data.valid = false;
    sensor_data.gps_data.new = false;

    sensor_data.new = false;
}