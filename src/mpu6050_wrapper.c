#include "mpu6050_wrapper.h"
#include <zephyr/device.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/devicetree.h>
#include <zephyr/logging/log.h>
#include <math.h>

LOG_MODULE_REGISTER(mpu6050_wrapper, LOG_LEVEL_DBG);

#define RAD_TO_DEG (180.0f / 3.14159265359f)

static const struct device *mpu6050_dev = NULL;
static mpu6050_cal_t cal = {0};
static bool calibrating = false;
static float cal_sum_x = 0, cal_sum_y = 0, cal_sum_z = 0;
static int cal_samples = 0;
static K_MUTEX_DEFINE(mpu6050_mutex);

int mpu6050_wrapper_init(void)
{
    mpu6050_dev = DEVICE_DT_GET(DT_NODELABEL(mpu6050));
    
    if (!device_is_ready(mpu6050_dev)) {
        printk("MPU6050 device not ready");
        return -ENODEV;
    }
    
    // Default calibration
    cal.accel_scale_x = 1.0f;
    cal.accel_scale_y = 1.0f;
    cal.accel_scale_z = 1.0f;
    
    LOG_INF("MPU6050 initialized");
    return 0;
}

bool mpu6050_wrapper_is_ready(void)
{
    return (mpu6050_dev != NULL) && device_is_ready(mpu6050_dev);
}

int mpu6050_wrapper_read_accel(float *ax, float *ay, float *az)
{
    struct sensor_value accel[3];
    
    if (!mpu6050_wrapper_is_ready()) {
        return -ENODEV;
    }
    
    if (sensor_sample_fetch(mpu6050_dev) < 0) {
        return -EIO;
    }
    
    sensor_channel_get(mpu6050_dev, SENSOR_CHAN_ACCEL_X, &accel[0]);
    sensor_channel_get(mpu6050_dev, SENSOR_CHAN_ACCEL_Y, &accel[1]);
    sensor_channel_get(mpu6050_dev, SENSOR_CHAN_ACCEL_Z, &accel[2]);
    
    // Apply calibration
    *ax = (sensor_value_to_double(&accel[0]) - cal.accel_offset_x) * cal.accel_scale_x;
    *ay = (sensor_value_to_double(&accel[1]) - cal.accel_offset_y) * cal.accel_scale_y;
    *az = (sensor_value_to_double(&accel[2]) - cal.accel_offset_z) * cal.accel_scale_z;
    
    return 0;
}

int mpu6050_wrapper_get_orientation(float *pitch, float *roll)
{
    float ax, ay, az;
    
    if (mpu6050_wrapper_read_accel(&ax, &ay, &az) != 0) {
        return -EIO;
    }
    
    // Normalize
    float norm = sqrtf(ax*ax + ay*ay + az*az);
    if (norm < 0.1f) {
        return -EINVAL;
    }
    ax /= norm;
    ay /= norm;
    az /= norm;
    
    // Calculate pitch and roll
    *pitch = asinf(-ax) * RAD_TO_DEG;
    *roll = atan2f(ay, az) * RAD_TO_DEG;
    
    return 0;
}

int mpu6050_wrapper_read(mpu6050_data_t *data)
{
    struct sensor_value accel[3], gyro[3];
    
    if (!mpu6050_wrapper_is_ready()) {
        return -ENODEV;
    }
    
    if (sensor_sample_fetch(mpu6050_dev) < 0) {
        return -EIO;
    }
    
    // Get accelerometer
    sensor_channel_get(mpu6050_dev, SENSOR_CHAN_ACCEL_X, &accel[0]);
    sensor_channel_get(mpu6050_dev, SENSOR_CHAN_ACCEL_Y, &accel[1]);
    sensor_channel_get(mpu6050_dev, SENSOR_CHAN_ACCEL_Z, &accel[2]);
    
    // Get gyroscope
    sensor_channel_get(mpu6050_dev, SENSOR_CHAN_GYRO_X, &gyro[0]);
    sensor_channel_get(mpu6050_dev, SENSOR_CHAN_GYRO_Y, &gyro[1]);
    sensor_channel_get(mpu6050_dev, SENSOR_CHAN_GYRO_Z, &gyro[2]);
    
    // Apply calibration
    data->accel_x = (sensor_value_to_double(&accel[0]) - cal.accel_offset_x) * cal.accel_scale_x;
    data->accel_y = (sensor_value_to_double(&accel[1]) - cal.accel_offset_y) * cal.accel_scale_y;
    data->accel_z = (sensor_value_to_double(&accel[2]) - cal.accel_offset_z) * cal.accel_scale_z;
    
    data->gyro_x = sensor_value_to_double(&gyro[0]);
    data->gyro_y = sensor_value_to_double(&gyro[1]);
    data->gyro_z = sensor_value_to_double(&gyro[2]);
    
    // Calculate orientation
    mpu6050_wrapper_get_orientation(&data->pitch, &data->roll);
    
    data->valid = true;
    
    return 0;
}

void mpu6050_wrapper_calibrate_start(void)
{
    k_mutex_lock(&mpu6050_mutex, K_FOREVER);
    calibrating = true;
    cal_sum_x = cal_sum_y = cal_sum_z = 0;
    cal_samples = 0;
    k_mutex_unlock(&mpu6050_mutex);
    LOG_INF("MPU6050 calibration started - keep device still");
}

void mpu6050_wrapper_calibrate_update(void)
{
    if (!calibrating) return;
    
    struct sensor_value accel[3];
    
    if (sensor_sample_fetch(mpu6050_dev) < 0) return;
    
    sensor_channel_get(mpu6050_dev, SENSOR_CHAN_ACCEL_X, &accel[0]);
    sensor_channel_get(mpu6050_dev, SENSOR_CHAN_ACCEL_Y, &accel[1]);
    sensor_channel_get(mpu6050_dev, SENSOR_CHAN_ACCEL_Z, &accel[2]);
    
    float ax = sensor_value_to_double(&accel[0]);
    float ay = sensor_value_to_double(&accel[1]);
    float az = sensor_value_to_double(&accel[2]);
    
    k_mutex_lock(&mpu6050_mutex, K_FOREVER);
    cal_sum_x += ax;
    cal_sum_y += ay;
    cal_sum_z += az;
    cal_samples++;
    k_mutex_unlock(&mpu6050_mutex);
}

void mpu6050_wrapper_calibrate_finish(void)
{
    k_mutex_lock(&mpu6050_mutex, K_FOREVER);
    calibrating = false;
    
    if (cal_samples > 0) {
        cal.accel_offset_x = cal_sum_x / cal_samples;
        cal.accel_offset_y = cal_sum_y / cal_samples;
        cal.accel_offset_z = (cal_sum_z / cal_samples) - 9.81f;
        
        LOG_INF("MPU6050 calibration complete (%d samples)", cal_samples);
        LOG_INF("  Offsets: X=%.3f Y=%.3f Z=%.3f",
                cal.accel_offset_x, cal.accel_offset_y, cal.accel_offset_z);
    }
    k_mutex_unlock(&mpu6050_mutex);
}

void mpu6050_wrapper_get_calibration(mpu6050_cal_t *cal_out)
{
    if (cal_out) {
        k_mutex_lock(&mpu6050_mutex, K_FOREVER);
        *cal_out = cal;
        k_mutex_unlock(&mpu6050_mutex);
    }
}

void mpu6050_wrapper_set_calibration(const mpu6050_cal_t *cal_in)
{
    if (cal_in) {
        k_mutex_lock(&mpu6050_mutex, K_FOREVER);
        cal = *cal_in;
        k_mutex_unlock(&mpu6050_mutex);
        LOG_INF("MPU6050 calibration loaded");
    }
}