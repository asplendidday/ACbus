#include "bus_stop_selection.h"

//==================================================================================================
//==================================================================================================
// Definitions

//==================================================================================================
//==================================================================================================
// Variables

static Window* s_bus_stop_sel_wnd = NULL;
static TextLayer* s_bus_stop_sel_title = NULL;
static TextLayer* s_bus_stop_sel_status = NULL;
static BitmapLayer* s_bus_stop_sel_banner = NULL;
static SimpleMenuLayer* s_bus_stop_sel_menu = NULL;
static SimpleMenuSection s_menu_section;
static SimpleMenuItem s_menu_entries[ 8 ];


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
    
    for( int i = 0; i != 8; ++i )
    {
        s_menu_entries[ i ] = ( SimpleMenuItem ) { .title = "some bus stop name" };
    }
    
    s_menu_section = ( SimpleMenuSection ) { .num_items = 8, .items = s_menu_entries };
    s_bus_stop_sel_menu = simple_menu_layer_create( GRect( 0, 24, 144, 140 ), s_bus_stop_sel_wnd, &s_menu_section, 1, NULL );
    layer_add_child( window_get_root_layer( s_bus_stop_sel_wnd ), simple_menu_layer_get_layer( s_bus_stop_sel_menu ) );
    
    common_create_text_layer( &s_bus_stop_sel_status, s_bus_stop_sel_wnd, GRect( 0, 148, 144, 20 ), GColorDarkCandyAppleRed, GColorWhite, FONT_KEY_GOTHIC_14, GTextAlignmentCenter );
    text_layer_set_text( s_bus_stop_sel_status, "No updates, yet." );
}

void bus_stop_selection_window_unload()
{
    text_layer_destroy( s_bus_stop_sel_status );
    simple_menu_layer_destroy( s_bus_stop_sel_menu );
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