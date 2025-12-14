#ifndef MPU6050_WRAPPER_H
#define MPU6050_WRAPPER_H

#include <zephyr/kernel.h>
#include <stdbool.h>

// Accelerometer data structure
typedef struct {
    float accel_x;  // m/s²
    float accel_y;
    float accel_z;
    float gyro_x;   // rad/s
    float gyro_y;
    float gyro_z;
    float pitch;    // Calculated pitch (degrees)
    float roll;     // Calculated roll (degrees)
    bool valid;
} mpu6050_data_t;

// Accelerometer calibration
typedef struct {
    float accel_offset_x;
    float accel_offset_y;
    float accel_offset_z;
    float accel_scale_x;
    float accel_scale_y;
    float accel_scale_z;
} mpu6050_cal_t;

// Initialize MPU6050
int mpu6050_wrapper_init(void);

// Read accelerometer data (raw)
int mpu6050_wrapper_read_accel(float *ax, float *ay, float *az);

// Read full data with calculated pitch/roll
int mpu6050_wrapper_read(mpu6050_data_t *data);

// Get pitch and roll
int mpu6050_wrapper_get_orientation(float *pitch, float *roll);

// Calibration
void mpu6050_wrapper_calibrate_start(void);
void mpu6050_wrapper_calibrate_update(void);
void mpu6050_wrapper_calibrate_finish(void);
void mpu6050_wrapper_get_calibration(mpu6050_cal_t *cal);
void mpu6050_wrapper_set_calibration(const mpu6050_cal_t *cal);

// Check if ready
bool mpu6050_wrapper_is_ready(void);

#endif // MPU6050_WRAPPER_H