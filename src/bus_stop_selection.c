#include "bus_stop_selection.h"

//==================================================================================================
//==================================================================================================
// Variables

static Window* s_bus_stop_sel_wnd = NULL;
static TextLayer* s_bus_stop_sel_title = NULL;
static BitmapLayer* s_bus_stop_sel_banner = NULL;


//==================================================================================================
//==================================================================================================
// Local functions

void bus_stop_selection_window_load()
{
    common_create_text_layer( &s_bus_stop_sel_title, s_bus_stop_sel_wnd, GRect( 24, 0, 120, 20 ),
                              GColorDarkCandyAppleRed, GColorWhite, FONT_KEY_GOTHIC_18_BOLD,
                              GTextAlignmentLeft );
    text_layer_set_text( s_bus_stop_sel_title, "Select bus stop" );
    
    common_create_h_icon( &s_bus_stop_sel_banner, s_bus_stop_sel_wnd );
}

void bus_stop_selection_window_unload()
{
    bitmap_layer_destroy( s_bus_stop_sel_banner );
    text_layer_destroy( s_bus_stop_sel_title );
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