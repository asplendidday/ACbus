#include "bus_stop_selection.h"

//==================================================================================================
//==================================================================================================
// Variables

static Window* s_bus_stop_sel_wnd = NULL;
static TextLayer* s_title = NULL;


//==================================================================================================
//==================================================================================================
// Local functions

void bus_stop_selection_window_load()
{
}

void bus_stop_selection_window_unload()
{
}


//==================================================================================================
//==================================================================================================
// Interface functions

void bus_stop_selection_create()
{
    s_bus_stop_sel_wnd = window_create();

    window_set_window_handlers( s_bus_stop_sel_wnd, ( WindowHandlers )
    {
        .load = bus_stop_selection_window_load,
        .unload = bus_stop_selection_window_unload
    } );
}

void bus_stop_selection_destroy()
{
    window_destroy( s_bus_stop_sel_wnd );
}


void bus_stop_selection_show()
{
    if( s_bus_stop_sel_wnd )
    {
        window_stack_push( s_bus_stop_sel_wnd, true );
    }
}
    

void bus_stop_selection_handle_msg_tuple( Tuple* msg_tuple )
{
    
}