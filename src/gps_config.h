#ifndef GPS_CONFIG_H
#define GPS_CONFIG_H

#include <stdbool.h>

// Refresh rate control
void gps_set_refresh_rate(int hz);
void gps_save_config(void);

// Message configuration presets
void gps_disable_all_messages(void);
void gps_enable_minimal_messages(void);    // RMC only
void gps_enable_standard_messages(void);   // GGA, RMC, VTG
void gps_enable_all_messages(void);

// Individual message control
void gps_set_gga(bool enable);  // GPS fix data
void gps_set_rmc(bool enable);  // Recommended minimum
void gps_set_vtg(bool enable);  // Speed/course
void gps_set_gsa(bool enable);  // Satellites used
void gps_set_gsv(bool enable);  // Satellites in view
void gps_set_gll(bool enable);  // Geographic position

// // Sets baud to 38400
// void gps_set_baud_38400();      

#endif // GPS_CONFIG_H