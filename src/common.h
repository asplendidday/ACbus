#pragma once

#include <pebble.h>

// App message ids
#define BUS_STOP_NAME            0
#define BUS_STOP_DIST            1
#define GPS_COORDS               2
#define BUS_DATA                 3
#define REQ_BUS_STOP_ID			 4
#define REQ_UPDATE_BUS_STOP_LIST 5

// Typedefs
typedef void( *GenericCallback )( void );

// Functions
#ifndef min
#define min(a,b) (((a) < (b)) ? (a) : (b))
#endif

void common_set_update_callback( GenericCallback callback );
GenericCallback common_get_update_callback();

void common_create_text_layer( TextLayer** text_layer, Window* window, GRect rect, GColor back_color,
							   GColor text_color, const char* font_name, GTextAlignment text_align );

void common_create_h_icon( BitmapLayer** bitmap_layer, Window* window );


void common_set_current_bus_stop_id( int id );
int common_get_current_bus_stop_id();
