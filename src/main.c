#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gnss.h>
#include <zephyr/logging/log.h>
#include <stdio.h>

#include "data_handler.h"
#include "gps_config.h"
#include "command_parser.h"
#include "mpu6050_wrapper.h"
#include "ht1621.h"
#include "hmc5883l.h"



LOG_MODULE_REGISTER(main, LOG_LEVEL_DBG);

static void gnss_data_cb(const struct device *dev, const struct gnss_data *data)
{
    if (data->info.fix_status != GNSS_FIX_STATUS_NO_FIX) {
        // Update GPS data
        struct gps_data g_data = {
            data->nav_data.speed,
            data->nav_data.bearing,
            data->utc.hour,
            data->utc.minute,
            data->utc.millisecond,
            data->nav_data.latitude,
            data->nav_data.longitude,
            true,
            true
        };
        set_gps_data(g_data);

        // float heading;
        // hmc5883l_get_heading(&heading);
        // printk("heading: %f", heading);
        
        // Stream if enabled
        if (command_parser_is_streaming()) {
            printk("%02u:%02u:%02u.%03u sog: %u.%03u m/s, cog: %u.%03u deg\n",
                    g_data.hour, g_data.minute,
                    g_data.millisecond / 1000, g_data.millisecond % 1000,
                    g_data.sog / 1000, g_data.sog % 1000,
                    g_data.cog / 1000, g_data.cog % 1000);
        }
    }
}

GNSS_DATA_CALLBACK_DEFINE(DEVICE_DT_GET(DT_ALIAS(gnss)), gnss_data_cb);

int main(void)
{
    LOG_INF("GPS Application Starting");
    
    // Initialize command parser
    // command_parser_init();
    
    k_sleep(K_SECONDS(1));
    gps_enable_standard_messages();

    invalidate_sensor_data();

    int ret = mpu6050_wrapper_init();
    if (ret != 0) {
        printk("Failed to init MPU6050: %d\n", ret);
    }

    ret = compass_init();
    if (ret != 0) {
        printk("Failed to init HMC5883L: %d\n", ret);
    } 
    float heading;
    hmc5883l_get_heading(&heading);
    printk("heading: %f", heading);
    // Main loop - can add other tasks here
    while (1) {
        k_sleep(K_FOREVER);
    }
    
    return 0;
}

