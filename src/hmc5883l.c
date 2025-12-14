#include "hmc5883l.h"
#include <zephyr/device.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/devicetree.h>
#include <zephyr/logging/log.h>
#include <math.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846f
#endif

static const struct device *hmc5883l_dev = NULL;
static K_MUTEX_DEFINE(hmc5883l_mutex);

int compass_init(void){
    hmc5883l_dev = DEVICE_DT_GET_ANY(honeywell_hmc5883l);;
    if (!device_is_ready(hmc5883l_dev)) {
        printk("HMC5883L device not ready\n");
        return -ENODEV;
    }
    
    return 0; 
}

int hmc5883l_read_mag(float *mx, float *my, float *mz){
    struct sensor_value mag[3];
    
    if (hmc5883l_dev == NULL || !device_is_ready(hmc5883l_dev)) {
        return -ENODEV;
    }
    
    // Fetch sensor data
    if (sensor_sample_fetch(hmc5883l_dev) < 0) {
        return -EIO;
    }
    
    // Get X, Y, Z magnetic field values
    sensor_channel_get(hmc5883l_dev, SENSOR_CHAN_MAGN_X, &mag[0]);
    sensor_channel_get(hmc5883l_dev, SENSOR_CHAN_MAGN_Y, &mag[1]);
    sensor_channel_get(hmc5883l_dev, SENSOR_CHAN_MAGN_Z, &mag[2]);
    
    // Convert to float (Gauss) - USE sensor_value_to_double!
    *mx = sensor_value_to_double(&mag[0]);
    *my = sensor_value_to_double(&mag[1]);
    *mz = sensor_value_to_double(&mag[2]);
 
    return 0;
}

int hmc5883l_get_heading(float *heading){
    float mx, my, mz;
    
    if (hmc5883l_read_mag(&mx, &my, &mz) != 0) {
        return -1;
    }
    
    // Calculate heading in radians
    float heading_rad = atan2f(my, mx);
    
    // Convert to degrees
    float heading_deg = heading_rad * 180.0f / M_PI;  // Use M_PI constant
    
    // Apply declination correction
    heading_deg += -11.5f;
    
    // Normalize to 0-360 degrees
    if (heading_deg < 0) {
        heading_deg += 360.0f;
    }
    if (heading_deg >= 360.0f) {
        heading_deg -= 360.0f;
    }
    
    *heading = heading_deg;
    return 0;
}

int hmc5883l_is_ready(void){
    return (hmc5883l_dev != NULL) && device_is_ready(hmc5883l_dev);
}