#ifndef HMC5883L_H
#define HMC5883L_H

#include <zephyr/kernel.h>
#include <stdbool.h>

// Accelerometer data structure
typedef struct {
    float xmag;
    float ymag;
    float zmag;
} hmc5883l_data_t;


int compass_init(void);

int hmc5883l_read_mag(float *mx, float *my, float *mz);

int hmc5883l_get_heading(float *heading);

int hmc5883l_is_ready(void);

#endif // HMC5883L_H