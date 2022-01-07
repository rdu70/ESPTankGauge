#ifndef _CONFIG_H
#define _CONFIG_H

#include "cred.h"             // Include credential file

#define RELEASE "1.04 - 20220107"

#define CFG_FILE "/config.json"

#define SENSOR_MIN 20         // Min value (sensor cannot read less than 20cm)
#define SENSOR_MAX 135        // Max value (should be tank_ref for real reading)
#define SENSOR_AVG 10         // Number of averaged value

#define TANK_REF 129          // Sensor level to empty
#define TANK_MIN 7            // Not used atm
#define TANK_L_PER_CM 21.75   // Amount of l per cm
#define TANK_CAPACITY 2350    // Usable max capacity

#define DSP_SAVER_TIME 300    // Screen saver time (s) : 5 minutes
#define DSP_COL_RED 0xe0
#define DSP_COL_ORANGE 0xf0
#define DSP_COL_GREEN 0x1c
#define DSP_COL_WHITE 0xff
#define DSP_COL_BLACK 0x00

#define BTN_PIN 13            // Button on GPIO13 (D7)
#define BTN_PINGND 12         // Button on GPIO12 (D6)

#define HOMIE_UPDATE 60       // # of second cycle to check updates (60s)
#define HOMIE_REFRESH 60      // # of update cycle to force refresh (1h)

#endif   /* _CONFIG_H */