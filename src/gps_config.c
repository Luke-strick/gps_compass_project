#include "gps_config.h"
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/devicetree.h>
#include <stdio.h>

// UBX command definitions
static const uint8_t ubx_set_1hz[] = {
    0xB5, 0x62, 0x06, 0x08, 0x06, 0x00,
    0xE8, 0x03, 0x01, 0x00, 0x01, 0x00,
    0x01, 0x39
};

static const uint8_t ubx_set_5hz[] = {
    0xB5, 0x62, 0x06, 0x08, 0x06, 0x00,
    0xC8, 0x00, 0x01, 0x00, 0x01, 0x00,
    0xDE, 0x6A
};

static const uint8_t ubx_set_10hz[] = {
    0xB5, 0x62, 0x06, 0x08, 0x06, 0x00,
    0x64, 0x00, 0x01, 0x00, 0x01, 0x00,
    0x79, 0x10
};

static const uint8_t ubx_save_config[] = {
    0xB5, 0x62, 0x06, 0x09, 0x0D, 0x00,
    0x00, 0x00, 0x00, 0x00,
    0xFF, 0xFF, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,
    0x17, 0x31, 0xBF
};

void gps_set_refresh_rate(int hz)
{
    const struct device *uart = DEVICE_DT_GET(DT_ALIAS(gps_usart));
    
    if (!device_is_ready(uart)) {
        printk("Error: GPS UART not ready\n");
        return;
    }
    
    const uint8_t *cmd;
    size_t cmd_len;
    
    switch(hz) {
        case 1:
            cmd = ubx_set_1hz;
            cmd_len = sizeof(ubx_set_1hz);
            break;
        case 5:
            cmd = ubx_set_5hz;
            cmd_len = sizeof(ubx_set_5hz);
            break;
        case 10:
            cmd = ubx_set_10hz;
            cmd_len = sizeof(ubx_set_10hz);
            break;
        default:
            printk("Error: Invalid refresh rate. Use 1, 5, or 10\n");
            return;
    }
    
    printk("Setting GPS refresh rate to %dHz...\n", hz);
    
    for (int i = 0; i < cmd_len; i++) {
        uart_poll_out(uart, cmd[i]);
    }
    
    k_sleep(K_MSEC(100));
    printk("GPS refresh rate set to %dHz\n", hz);
}

void gps_save_config(void)
{
    const struct device *uart = DEVICE_DT_GET(DT_ALIAS(gps_usart));
    
    if (!device_is_ready(uart)) {
        printk("Error: GPS UART not ready\n");
        return;
    }
    
    printk("Saving GPS configuration to flash...\n");
    
    for (int i = 0; i < sizeof(ubx_save_config); i++) {
        uart_poll_out(uart, ubx_save_config[i]);
    }
    
    k_sleep(K_MSEC(500));
    printk("GPS configuration saved\n");
}

// Helper function to calculate UBX checksum
static void ubx_add_checksum(uint8_t *msg, int len)
{
    uint8_t ck_a = 0, ck_b = 0;
    
    // Calculate checksum over message (skip sync chars and checksum bytes)
    for (int i = 2; i < len - 2; i++) {
        ck_a += msg[i];
        ck_b += ck_a;
    }
    
    msg[len - 2] = ck_a;
    msg[len - 1] = ck_b;
}

// UBX-CFG-MSG: Configure message rate for a specific NMEA sentence
// Format: B5 62 06 01 08 00 [CLASS] [ID] [rates for 6 ports] [checksum]
static void gps_set_message_rate(uint8_t msg_class, uint8_t msg_id, uint8_t rate)
{
    const struct device *uart = DEVICE_DT_GET(DT_ALIAS(gps_usart));
    
    if (!device_is_ready(uart)) {
        printk("Error: GPS UART not ready\n");
        return;
    }
    
    uint8_t cmd[] = {
        0xB5, 0x62,     // Header
        0x06, 0x01,     // Class CFG, ID MSG
        0x08, 0x00,     // Length (8 bytes)
        msg_class,      // Message class
        msg_id,         // Message ID
        0x00,           // Rate on I2C
        rate,           // Rate on UART1 (0 = off, 1 = every solution)
        0x00,           // Rate on UART2
        0x00,           // Rate on USB
        0x00,           // Rate on SPI
        0x00,           // Reserved
        0x00, 0x00      // Checksum (will be calculated)
    };
    
    ubx_add_checksum(cmd, sizeof(cmd));
    
    for (int i = 0; i < sizeof(cmd); i++) {
        uart_poll_out(uart, cmd[i]);
    }
    
    k_sleep(K_MSEC(50));
}

// NMEA message classes and IDs
#define NMEA_CLASS 0xF0

#define NMEA_GGA 0x00  // GPS fix data
#define NMEA_GLL 0x01  // Geographic position
#define NMEA_GSA 0x02  // GPS DOP and active satellites
#define NMEA_GSV 0x03  // GPS satellites in view
#define NMEA_RMC 0x04  // Recommended minimum data
#define NMEA_VTG 0x05  // Course over ground and ground speed
#define NMEA_GRS 0x06  // GPS range residuals
#define NMEA_GST 0x07  // GPS pseudorange error statistics
#define NMEA_ZDA 0x08  // Time and date
#define NMEA_GBS 0x09  // GPS satellite fault detection
#define NMEA_DTM 0x0A  // Datum reference
#define NMEA_GNS 0x0D  // GNSS fix data
#define NMEA_THS 0x0E  // True heading and status
#define NMEA_VLW 0x0F  // Dual ground/water distance

// Preset configurations
void gps_disable_all_messages(void)
{
    printk("Disabling all NMEA messages...\n");
    
    gps_set_message_rate(NMEA_CLASS, NMEA_GGA, 0);
    gps_set_message_rate(NMEA_CLASS, NMEA_GLL, 0);
    gps_set_message_rate(NMEA_CLASS, NMEA_GSA, 0);
    gps_set_message_rate(NMEA_CLASS, NMEA_GSV, 0);
    gps_set_message_rate(NMEA_CLASS, NMEA_RMC, 0);
    gps_set_message_rate(NMEA_CLASS, NMEA_VTG, 0);
    gps_set_message_rate(NMEA_CLASS, NMEA_GRS, 0);
    gps_set_message_rate(NMEA_CLASS, NMEA_GST, 0);
    gps_set_message_rate(NMEA_CLASS, NMEA_ZDA, 0);
    gps_set_message_rate(NMEA_CLASS, NMEA_GBS, 0);
    gps_set_message_rate(NMEA_CLASS, NMEA_DTM, 0);
    
    printk("All NMEA messages disabled\n");
}

void gps_enable_minimal_messages(void)
{
    printk("Enabling minimal NMEA messages (RMC only)...\n");
    
    // Disable all
    gps_disable_all_messages();
    
    // Enable only RMC (recommended minimum - has position, speed, time)
    gps_set_message_rate(NMEA_CLASS, NMEA_RMC, 1);
    
    printk("Minimal messages enabled (RMC)\n");
}

void gps_enable_standard_messages(void)
{
    printk("Enabling standard NMEA messages...\n");
    
    // Disable all first
    gps_disable_all_messages();
    
    // Enable common messages
    gps_set_message_rate(NMEA_CLASS, NMEA_GGA, 1);  // Position fix
    gps_set_message_rate(NMEA_CLASS, NMEA_RMC, 1);  // Recommended minimum
    gps_set_message_rate(NMEA_CLASS, NMEA_VTG, 1);  // Speed/course
    
    printk("Standard messages enabled (GGA, RMC, VTG)\n");
}

void gps_enable_all_messages(void)
{
    printk("Enabling all NMEA messages...\n");
    
    gps_set_message_rate(NMEA_CLASS, NMEA_GGA, 1);
    gps_set_message_rate(NMEA_CLASS, NMEA_GLL, 1);
    gps_set_message_rate(NMEA_CLASS, NMEA_GSA, 1);
    gps_set_message_rate(NMEA_CLASS, NMEA_GSV, 1);
    gps_set_message_rate(NMEA_CLASS, NMEA_RMC, 1);
    gps_set_message_rate(NMEA_CLASS, NMEA_VTG, 1);
    
    printk("All main NMEA messages enabled\n");
}

// Individual message control
void gps_set_gga(bool enable) { 
    gps_set_message_rate(NMEA_CLASS, NMEA_GGA, enable ? 1 : 0); 
}

void gps_set_rmc(bool enable) { 
    gps_set_message_rate(NMEA_CLASS, NMEA_RMC, enable ? 1 : 0); 
}

void gps_set_vtg(bool enable) { 
    gps_set_message_rate(NMEA_CLASS, NMEA_VTG, enable ? 1 : 0); 
}

void gps_set_gsa(bool enable) { 
    gps_set_message_rate(NMEA_CLASS, NMEA_GSA, enable ? 1 : 0); 
}

void gps_set_gsv(bool enable) { 
    gps_set_message_rate(NMEA_CLASS, NMEA_GSV, enable ? 1 : 0); 
}

void gps_set_gll(bool enable) { 
    gps_set_message_rate(NMEA_CLASS, NMEA_GLL, enable ? 1 : 0); 
}

// // Set UART baud rate to 38400
// static const uint8_t ubx_set_baud_38400[] = {
//     0xB5, 0x62,     // Header
//     0x06, 0x00,     // Class CFG, ID PRT (port config)
//     0x14, 0x00,     // Length (20 bytes)
//     0x01,           // Port ID (UART1)
//     0x00,           // Reserved
//     0x00, 0x00,     // txReady (disabled)
//     0xD0, 0x08, 0x00, 0x00,  // Mode (8N1)
//     0x00, 0x96, 0x00, 0x00,  // Baud rate: 38400 (0x9600 in little-endian)
//     0x07, 0x00,     // Input protocols (UBX + NMEA + RTCM)
//     0x03, 0x00,     // Output protocols (UBX + NMEA)
//     0x00, 0x00,     // Flags
//     0x00, 0x00,     // Reserved
//     0x97, 0x6F      // Checksum
// };

// void gps_set_baud_38400(void)
// {
//     const struct device *uart = DEVICE_DT_GET(DT_ALIAS(gps_usart));
    
//     if (!device_is_ready(uart)) {
//         return;
//     }
    
//     printk("Setting GPS UART to 38400 baud...\n");
    
//     for (int i = 0; i < sizeof(ubx_set_baud_38400); i++) {
//         uart_poll_out(uart, ubx_set_baud_38400[i]);
//     }
    
//     k_sleep(K_MSEC(100));
// }