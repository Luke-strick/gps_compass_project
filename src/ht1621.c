/*
 * ht1621.c - HT1621 LCD Driver Library Implementation
 */

#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/sys/printk.h>
#include "ht1621.h"

/* HT1621 Commands */
#define HT1621_CMD_SYS_DIS  0x00  /* System disable */
#define HT1621_CMD_SYS_EN   0x01  /* System enable */
#define HT1621_CMD_LCD_OFF  0x02  /* LCD off */
#define HT1621_CMD_LCD_ON   0x03  /* LCD on */
#define HT1621_CMD_RC_256K  0x18  /* RC 256K */

/* Default bias/commons - can be overridden */
#define HT1621_CMD_BIAS_DEFAULT  0x29  /* 1/3 bias, 3 commons */

/* GPIO Pin Definitions */
#define CS_NODE   DT_ALIAS(ht1621_cs)
#define WR_NODE   DT_ALIAS(ht1621_wr)
#define DATA_NODE DT_ALIAS(ht1621_data)

static const struct gpio_dt_spec cs_pin = GPIO_DT_SPEC_GET(CS_NODE, gpios);
static const struct gpio_dt_spec wr_pin = GPIO_DT_SPEC_GET(WR_NODE, gpios);
static const struct gpio_dt_spec data_pin = GPIO_DT_SPEC_GET(DATA_NODE, gpios);

/* Timing delays (in microseconds) */
#define HT1621_DELAY_US 2

/*
 * 7-segment digit encoding
 * Segment layout:
 *      A
 *     ---
 *  F |   | B
 *     -G-
 *  E |   | C
 *     ---
 *      D
 * 
 * Bit mapping: 0bGFEDCBA (bit 7 = decimal point)
 */
static const uint8_t digit_segments[] = {
    0b00111111,  /* 0 */
    0b00000110,  /* 1 */
    0b01011011,  /* 2 */
    0b01001111,  /* 3 */
    0b01100110,  /* 4 */
    0b01101101,  /* 5 */
    0b01111101,  /* 6 */
    0b00000111,  /* 7 */
    0b01111111,  /* 8 */
    0b01101111,  /* 9 */
};

/* Hex digits A-F */
static const uint8_t hex_segments[] = {
    0b01110111,  /* A */
    0b01111100,  /* b */
    0b00111001,  /* C */
    0b01011110,  /* d */
    0b01111001,  /* E */
    0b01110001,  /* F */
};

/* Special characters */
#define SEG_MINUS   0b01000000  /* - */
#define SEG_BLANK   0b00000000  /* blank */
#define SEG_DP      0b10000000  /* decimal point (bit 7) */

/* Function to write bits to HT1621 */
static void ht1621_write_bits(uint8_t data, uint8_t bits)
{
    for (int i = 0; i < bits; i++) {
        gpio_pin_set_dt(&wr_pin, 0);
        k_usleep(HT1621_DELAY_US);
        
        if (data & (1 << (bits - 1 - i))) {
            gpio_pin_set_dt(&data_pin, 1);
        } else {
            gpio_pin_set_dt(&data_pin, 0);
        }
        
        k_usleep(HT1621_DELAY_US);
        gpio_pin_set_dt(&wr_pin, 1);
        k_usleep(HT1621_DELAY_US);
    }
}

/* Send command to HT1621 */
static void ht1621_send_command(uint8_t cmd)
{
    gpio_pin_set_dt(&cs_pin, 0);
    k_usleep(HT1621_DELAY_US);
    
    ht1621_write_bits(0x80, 4);  /* Command mode: 100 */
    ht1621_write_bits(cmd, 8);   /* Command data */
    
    gpio_pin_set_dt(&cs_pin, 1);
    k_usleep(HT1621_DELAY_US);
}

void ht1621_write_data(uint8_t addr, uint8_t data)
{
    gpio_pin_set_dt(&cs_pin, 0);
    k_usleep(HT1621_DELAY_US);
    
    ht1621_write_bits(0xA0, 3);  /* Write mode: 101 */
    ht1621_write_bits(addr, 6);  /* Address (6 bits) */
    ht1621_write_bits(data, 4);  /* Data (4 bits per address) */
    
    gpio_pin_set_dt(&cs_pin, 1);
    k_usleep(HT1621_DELAY_US);
}

int ht1621_init(void)
{
    return ht1621_init_with_config(HT1621_CMD_BIAS_DEFAULT);
}

int ht1621_init_with_config(ht1621_bias_com_t bias_com)
{
    int ret;
    
    /* Initialize GPIO pins */
    if (!gpio_is_ready_dt(&cs_pin) || 
        !gpio_is_ready_dt(&wr_pin) || 
        !gpio_is_ready_dt(&data_pin)) {
        printk("GPIO devices not ready\n");
        return -ENODEV;
    }
    
    ret = gpio_pin_configure_dt(&cs_pin, GPIO_OUTPUT_INACTIVE);
    if (ret < 0) return ret;
    
    ret = gpio_pin_configure_dt(&wr_pin, GPIO_OUTPUT_INACTIVE);
    if (ret < 0) return ret;
    
    ret = gpio_pin_configure_dt(&data_pin, GPIO_OUTPUT_INACTIVE);
    if (ret < 0) return ret;
    
    /* Initialize HT1621 */
    gpio_pin_set_dt(&cs_pin, 1);
    gpio_pin_set_dt(&wr_pin, 1);
    gpio_pin_set_dt(&data_pin, 1);
    
    k_msleep(100);
    
    printk("HT1621: Using bias/commons config: 0x%02X\n", bias_com);
    ht1621_send_command(bias_com);
    ht1621_send_command(HT1621_CMD_RC_256K);
    ht1621_send_command(HT1621_CMD_SYS_EN);
    ht1621_send_command(HT1621_CMD_LCD_ON);
    
    printk("HT1621 initialized\n");
    return 0;
}

void ht1621_clear(void)
{
    for (uint8_t i = 0; i < 32; i++) {
        ht1621_write_data(i, 0x00);
    }
}

void ht1621_display_digit(uint8_t position, uint8_t digit, bool decimal_point)
{
    if (position >= HT1621_MAX_DIGITS) {
        return;
    }
    
    uint8_t segments = 0;
    
    if (digit <= 9) {
        segments = digit_segments[digit];
    } else if (digit >= 0x0A && digit <= 0x0F) {
        segments = hex_segments[digit - 0x0A];
    } else if (digit == HT1621_BLANK) {
        segments = SEG_BLANK;
    } else if (digit == HT1621_MINUS) {
        segments = SEG_MINUS;
    }
    
    if (decimal_point) {
        segments |= SEG_DP;
    }
    
    /* Each digit typically occupies 2 addresses (8 bits for 7 segments + DP)
     * Address = position * 2
     * Adjust this based on your LCD's memory map
     */
    uint8_t addr = position * 2;
    
    /* Write lower nibble */
    ht1621_write_data(addr, segments & 0x0F);
    /* Write upper nibble */
    ht1621_write_data(addr + 1, (segments >> 4) & 0x0F);
}

void ht1621_display_number(int32_t number, bool leading_zeros)
{
    bool is_negative = false;
    uint8_t digits[HT1621_MAX_DIGITS] = {0};
    uint8_t digit_count = 0;
    
    /* Handle negative numbers */
    if (number < 0) {
        is_negative = true;
        number = -number;
    }
    
    /* Extract digits */
    if (number == 0) {
        digits[0] = 0;
        digit_count = 1;
    } else {
        uint32_t temp = number;
        while (temp > 0 && digit_count < HT1621_MAX_DIGITS) {
            digits[digit_count++] = temp % 10;
            temp /= 10;
        }
    }
    
    /* Display digits (reverse order) */
    uint8_t display_pos = 0;
    
    /* Add negative sign if needed */
    if (is_negative && display_pos < HT1621_MAX_DIGITS) {
        ht1621_display_digit(display_pos++, HT1621_MINUS, false);
    }
    
    /* Display leading zeros or blanks */
    if (leading_zeros) {
        for (int i = digit_count - 1; i >= 0 && display_pos < HT1621_MAX_DIGITS; i--) {
            ht1621_display_digit(display_pos++, digits[i], false);
        }
        /* Fill remaining positions with zeros */
        while (display_pos < HT1621_MAX_DIGITS) {
            ht1621_display_digit(display_pos++, 0, false);
        }
    } else {
        /* Display number without leading zeros */
        for (int i = digit_count - 1; i >= 0 && display_pos < HT1621_MAX_DIGITS; i--) {
            ht1621_display_digit(display_pos++, digits[i], false);
        }
        /* Blank remaining positions */
        while (display_pos < HT1621_MAX_DIGITS) {
            ht1621_display_digit(display_pos++, HT1621_BLANK, false);
        }
    }
}

void ht1621_display_float(float number, uint8_t decimals)
{
    if (decimals > 3) decimals = 3;
    
    bool is_negative = false;
    if (number < 0) {
        is_negative = true;
        number = -number;
    }
    
    /* Scale and convert to integer */
    int32_t scaled = (int32_t)(number * 1000);
    uint8_t digits[HT1621_MAX_DIGITS] = {0};
    uint8_t digit_count = 0;
    
    /* Extract digits */
    for (int i = 0; i < 6; i++) {
        digits[i] = scaled % 10;
        scaled /= 10;
        digit_count++;
        if (scaled == 0 && i >= decimals) break;
    }
    
    /* Display */
    uint8_t display_pos = 0;
    
    if (is_negative && display_pos < HT1621_MAX_DIGITS) {
        ht1621_display_digit(display_pos++, HT1621_MINUS, false);
    }
    
    for (int i = digit_count - 1; i >= 0 && display_pos < HT1621_MAX_DIGITS; i--) {
        bool dp = (i == decimals - 1 && decimals > 0);
        ht1621_display_digit(display_pos++, digits[i], dp);
    }
    
    while (display_pos < HT1621_MAX_DIGITS) {
        ht1621_display_digit(display_pos++, HT1621_BLANK, false);
    }
}

void ht1621_display_hex(uint32_t number)
{
    for (int i = HT1621_MAX_DIGITS - 1; i >= 0; i--) {
        uint8_t digit = (number >> (i * 4)) & 0x0F;
        ht1621_display_digit(HT1621_MAX_DIGITS - 1 - i, digit, false);
    }
}

void ht1621_test_digits(void)
{
    printk("Testing digit display...\n");
    
    /* Count 0-9 on all positions */
    for (uint8_t digit = 0; digit <= 9; digit++) {
        for (uint8_t pos = 0; pos < HT1621_MAX_DIGITS; pos++) {
            ht1621_display_digit(pos, digit, false);
        }
        k_msleep(500);
    }
}