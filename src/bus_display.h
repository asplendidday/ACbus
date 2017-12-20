#pragma once

// Includes
#include "common.h"

// Function declarations
void bus_display_create();
void bus_display_destroy();

void bus_display_show();

void bus_display_handle_msg_tuple( Tuple* msg_tuple );
