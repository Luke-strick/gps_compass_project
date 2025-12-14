#ifndef DATA_HANDLER_H
#define DATA_HANDLER_H

#include <zephyr/kernel.h>
#include <stdbool.h>
#include <stdint.h>

struct gps_data{
    uint32_t sog;
    uint32_t cog;
    uint8_t hour;
    uint8_t minute;
    uint16_t millisecond;
    uint32_t latitude;
    uint32_t longitude;
    bool new;
    bool valid;
};

struct compass_data{
    uint32_t heading;
    bool new;
    bool valid;
};

struct acc_data{
    uint32_t roll;
    uint32_t pitch;
    bool new;
    bool valid;
};

struct sensor_data{
    struct gps_data gps_data;
    struct compass_data compass_data;
    struct acc_data acc_data;
    bool new;
};

extern struct k_msgq sensor_data_msgq;

bool get_gps_data(struct gps_data *dest);
void set_gps_data(struct gps_data source);

bool get_acc_data(struct acc_data *dest);
void set_acc_data(struct acc_data source);

bool get_compass_data(struct compass_data *dest);
void set_compass_data(struct compass_data source);

void get_sensors_data(struct sensor_data *dest);
void invalidate_sensor_data();

#endif // DATA_HANDLER_H