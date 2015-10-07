#pragma once

#include <pebble.h>

// Functions
#ifndef min
#define min(a,b) (((a) < (b)) ? (a) : (b))
#endif

// App message ids
#define BUS_STOP_NAME            0
#define BUS_STOP_DIST            1
#define GPS_COORDS               2
#define BUS_DATA                 3   