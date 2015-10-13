#pragma once

#include "common.h"

void bus_stop_selection_create();
void bus_stop_selection_destroy();

void bus_stop_selection_show();

void bus_stop_selection_handle_msg_tuple( Tuple* msg_tuple );

void bus_stop_selection_set_update_status_text( const char* status_text );
