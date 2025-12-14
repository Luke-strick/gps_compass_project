/*
 * ht1621.h - HT1621 LCD Driver Library for Zephyr
 * 
 * Driver for HT1621 RAM mapping LCD controller using bit-banged 3-wire interface
 */

#ifndef HT1621_H
#define HT1621_H

#include <zephyr/kernel.h>
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Display Configuration */
#define HT1621_MAX_DIGITS   6     /* Maximum number of digits supported */

/* Special display values for ht1621_display_digit() */
#define HT1621_BLANK        0xFF  /* Blank digit */
#define HT1621_MINUS        0xFE  /* Minus sign */

/* Bias and Commons Configuration Options */
typedef enum {
    HT1621_BIAS_1_2_2COM = 0x20,  /* 1/2 bias, 2 commons */
    HT1621_BIAS_1_3_2COM = 0x24,  /* 1/3 bias, 2 commons */
    HT1621_BIAS_1_2_3COM = 0x28,  /* 1/2 bias, 3 commons */
    HT1621_BIAS_1_3_3COM = 0x29,  /* 1/3 bias, 3 commons (most common) */
    HT1621_BIAS_1_2_4COM = 0x2C,  /* 1/2 bias, 4 commons */
    HT1621_BIAS_1_3_4COM = 0x2D   /* 1/3 bias, 4 commons */
} ht1621_bias_com_t;

/**
 * @brief Initialize the HT1621 LCD driver
 * 
 * Configures GPIO pins and initializes the HT1621 controller.
 * Must be called before any other HT1621 functions.
 * 
 * @return 0 on success, negative error code on failure
 */
int ht1621_init(void);

/**
 * @brief Initialize the HT1621 LCD driver with custom bias/commons config
 * 
 * Use this if your display requires different bias/commons settings.
 * Common configurations:
 * - HT1621_BIAS_1_3_3COM: 1/3 bias, 3 commons (most common for 6-digit)
 * - HT1621_BIAS_1_3_4COM: 1/3 bias, 4 commons
 * - HT1621_BIAS_1_2_4COM: 1/2 bias, 4 commons
 * 
 * @param bias_com Bias and commons configuration
 * @return 0 on success, negative error code on failure
 */
int ht1621_init_with_config(ht1621_bias_com_t bias_com);

/**
 * @brief Clear the entire display
 * 
 * Sets all display segments to off (blank)
 */
void ht1621_clear(void);

/**
 * @brief Write raw data to a specific HT1621 address
 * 
 * Low-level function for direct memory access.
 * Most users should use the higher-level display functions instead.
 * 
 * @param addr Address to write to (0-31)
 * @param data 4-bit data value to write
 */
void ht1621_write_data(uint8_t addr, uint8_t data);

/**
 * @brief Display a single digit at specified position
 * 
 * @param position Digit position (0 = leftmost, up to HT1621_MAX_DIGITS-1)
 * @param digit Digit to display:
 *              - 0-9: numeric digits
 *              - 0x0A-0x0F: hexadecimal A-F
 *              - HT1621_BLANK (0xFF): blank/space
 *              - HT1621_MINUS (0xFE): minus sign
 * @param decimal_point true to show decimal point after this digit
 */
void ht1621_display_digit(uint8_t position, uint8_t digit, bool decimal_point);

/**
 * @brief Display an integer number
 * 
 * Displays integers from -999999 to 999999.
 * Handles negative numbers with a minus sign.
 * 
 * @param number Integer value to display
 * @param leading_zeros true to show leading zeros (e.g., 000042),
 *                      false to blank leading positions (e.g., "    42")
 * 
 * @example
 * ht1621_display_number(12345, false);  // Shows "12345 "
 * ht1621_display_number(-42, false);    // Shows "-42   "
 * ht1621_display_number(7, true);       // Shows "000007"
 */
void ht1621_display_number(int32_t number, bool leading_zeros);

/**
 * @brief Display a floating point number
 * 
 * Displays floating point values with specified decimal places.
 * Handles negative numbers with a minus sign.
 * 
 * @param number Floating point value to display
 * @param decimals Number of decimal places (0-3)
 * 
 * @example
 * ht1621_display_float(3.14159, 2);   // Shows "3.14  "
 * ht1621_display_float(-98.765, 1);   // Shows "-98.7 "
 * ht1621_display_float(42.0, 0);      // Shows "42    "
 */
void ht1621_display_float(float number, uint8_t decimals);

/**
 * @brief Display a hexadecimal number
 * 
 * Displays hex values from 0x000000 to 0xFFFFFF.
 * All 6 digits are always shown with leading zeros.
 * 
 * @param number Hexadecimal value to display (up to 24 bits)
 * 
 * @example
 * ht1621_display_hex(0xABCDEF);  // Shows "ABCDEF"
 * ht1621_display_hex(0x42);      // Shows "000042"
 */
void ht1621_display_hex(uint32_t number);

/**
 * @brief Test function - cycle through all digits
 * 
 * Displays digits 0-9 sequentially on all positions.
 * Useful for testing display functionality and segment mapping.
 */
void ht1621_test_digits(void);

#ifdef __cplusplus
}
#endif

#endif /* HT1621_H */