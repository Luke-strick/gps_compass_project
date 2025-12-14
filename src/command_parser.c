#include "command_parser.h"
#include "gps_config.h"
#include "data_handler.h"
#include "mpu6050_wrapper.h"
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/devicetree.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

static bool stream_enabled = false;
static K_MUTEX_DEFINE(stream_mutex);

void command_parser_set_streaming(bool enable)
{
    k_mutex_lock(&stream_mutex, K_FOREVER);
    stream_enabled = enable;
    k_mutex_unlock(&stream_mutex);
}

bool command_parser_is_streaming(void)
{
    bool enabled;
    k_mutex_lock(&stream_mutex, K_FOREVER);
    enabled = stream_enabled;
    k_mutex_unlock(&stream_mutex);
    return enabled;
}

static void process_command(char *cmd)
{
    // Trim trailing whitespace/newline
    int len = strlen(cmd);
    while (len > 0 && (cmd[len-1] == '\n' || cmd[len-1] == '\r' || cmd[len-1] == ' ')) {
        cmd[len-1] = '\0';
        len--;
    }
    
    if (len == 0) {
        return;
    }
    
    // Parse "gps refresh <rate>"
    if (strncmp(cmd, "gps refresh ", 12) == 0) {
        int rate = atoi(cmd + 12);
        if (rate == 1 || rate == 5 || rate == 10) {
            gps_set_refresh_rate(rate);
        } else {
            printk("Error: Invalid rate '%d'. Use: gps refresh <1|5|10>\n", rate);
        }
    }
    // Parse "gps save"
    else if (strcmp(cmd, "gps save") == 0) {
        gps_save_config();
    }
    else if (strcmp(cmd, "accel cal start") == 0) {
        mpu6050_wrapper_calibrate_start();
        printk("Keep device still on level surface for 5 seconds...\n");
    }
    else if (strcmp(cmd, "accel cal stop") == 0) {
        mpu6050_wrapper_calibrate_finish();
    }
    else if (strcmp(cmd, "accel") == 0) {
        mpu6050_data_t data;
        if (mpu6050_wrapper_read(&data) == 0) {
            printk("Pitch: %.1f°, Roll: %.1f°\n", data.pitch, data.roll);
            printk("Accel: X=%.2f Y=%.2f Z=%.2f m/s²\n",
                   data.accel_x, data.accel_y, data.accel_z);
        }
        else{
            printk("Error reading accel values\n");
        }
    }
    // Parse "stream on"
    else if (strcmp(cmd, "stream on") == 0) {
        command_parser_set_streaming(true);
        printk("GPS streaming enabled\n");
    }
    // Parse "stream off"
    else if (strcmp(cmd, "stream off") == 0) {
        command_parser_set_streaming(false);
        printk("GPS streaming disabled\n");
    }
    // Parse "help"
    else if (strcmp(cmd, "help") == 0) {
        printk("\nAvailable commands:\n");
        printk("  gps refresh <1|5|10>  - Set GPS update rate (Hz)\n");
        printk("  gps save              - Save GPS config to flash\n");
        printk("  stream on             - Enable GPS data streaming\n");
        printk("  stream off            - Disable GPS data streaming\n");
        printk("  help                  - Show this help\n\n");
    }
    // Unknown command
    else {
        printk("Unknown command: '%s'\n", cmd);
        printk("Type 'help' for available commands\n");
    }
}

static void command_thread(void)
{
    char line[128];
    int idx = 0;
    uint8_t c;
    
    const struct device *console = DEVICE_DT_GET(DT_CHOSEN(zephyr_console));
    
    if (!device_is_ready(console)) {
        printk("ERROR: Console device not ready!\n");
        return;
    }
    
    k_sleep(K_MSEC(500));
    printk("> ");
    
    while (1) {
        if (uart_poll_in(console, &c) == 0) {
            if (c == '\n' || c == '\r') {
                printk("\n");
                line[idx] = '\0';
                
                if (idx > 0) {
                    process_command(line);
                }
                
                idx = 0;
                printk("> ");
            }
            else if (c == '\b' || c == 127) {
                if (idx > 0) {
                    idx--;
                    printk("\b \b");
                }
            }
            else if (idx < sizeof(line) - 1 && c >= 32 && c <= 126) {
                line[idx++] = c;
                printk("%c", c);
            }
        } else {
            k_msleep(10);
        }
    }
}

K_THREAD_DEFINE(cmd_thread_id, 1024, command_thread, NULL, NULL, NULL, 7, 0, 0);

// void command_parser_init(void) {}