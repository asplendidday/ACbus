#include <pebble.h>
#include "bus_stop_selection.h"

static Window* s_bus_stop_sel_wnd = NULL;
static TextLayer* s_title = NULL;

void window_load()
{
}

void window_unload()
{
}


void bus_stop_selection_window_create()
{
    s_bus_stop_sel_wnd = window_create();

    window_set_window_handlers( s_bus_stop_sel_wnd, ( WindowHandlers )
    {
      .load = window_load,
      .unload = window_unload
    } );
}

void bus_stop_selection_window_destroy()
{
    if( s_bus_stop_sel_wnd )
    {
        window_destroy( s_bus_stop_sel_wnd );
    }
}

void bus_stop_selection_window_show()
{
    if( s_bus_stop_sel_wnd )
    {
        window_stack_push( s_bus_stop_sel_wnd, true );
    }
}
    
void bus_stop_selection_window_hide()
{
    if( s_bus_stop_sel_wnd )
    {
        // @TODO implement
    }
}
