#include "bus_stop_selection.h"

//==================================================================================================
//==================================================================================================
// Definitions

#define NUM_BUS_STOPS             7

#define BUS_STOP_MARGIN_TOP      28
#define BUS_STOP_MARGIN_LEFT      3
#define BUS_STOP_HEIGHT          16
#define BUS_STOP_NAME_WIDTH     100
#define BUS_STOP_DIST_WIDTH      38

#define BUS_STOP_NAME_SIZE       32
#define BUS_STOP_DIST_SIZE        8


//==================================================================================================
//==================================================================================================
// Variables

static Window* s_bus_stop_sel_wnd = NULL;
static TextLayer* s_bus_stop_sel_title = NULL;
static TextLayer* s_bus_stop_sel_status = NULL;
static BitmapLayer* s_bus_stop_sel_banner = NULL;

struct {
    TextLayer* name;
    TextLayer* dist;
    
    char name_string[ BUS_STOP_NAME_SIZE ];
    char dist_string[ BUS_STOP_DIST_SIZE ];
    int id;    
} s_bus_stops[ NUM_BUS_STOPS ];

static int s_selected_bus_stop_idx = 0;


//==================================================================================================
//==================================================================================================
// Various helper functions

GRect bus_stop_name_rect( int index )
{
    return GRect( BUS_STOP_MARGIN_LEFT,
                  BUS_STOP_MARGIN_TOP + index * BUS_STOP_HEIGHT,
                  BUS_STOP_NAME_WIDTH,
                  BUS_STOP_HEIGHT );
}

GRect bus_stop_dist_rect( int index )
{
    return GRect( BUS_STOP_MARGIN_LEFT + BUS_STOP_NAME_WIDTH,
                  BUS_STOP_MARGIN_TOP + index * BUS_STOP_HEIGHT,
                  BUS_STOP_DIST_WIDTH,
                  BUS_STOP_HEIGHT );
}


void create_bus_stop_text_layers()
{
    for( int i = 0; i != NUM_BUS_STOPS; ++i )
    {
        common_create_text_layer( &s_bus_stops[ i ].name, s_bus_stop_sel_wnd,
                                  bus_stop_name_rect( i ), GColorWhite, GColorBlack,
                                  FONT_KEY_GOTHIC_14, GTextAlignmentLeft );
        common_create_text_layer( &s_bus_stops[ i ].dist, s_bus_stop_sel_wnd,
                                  bus_stop_dist_rect( i ), GColorWhite, GColorBlack,
                                  FONT_KEY_GOTHIC_14, GTextAlignmentRight );
    }
}

void destroy_bus_stop_text_layers()
{
    for( int i = 0; i != NUM_BUS_STOPS; ++i )
    {
        text_layer_destroy( s_bus_stops[ i ].name );
        text_layer_destroy( s_bus_stops[ i ].dist );
    }
}


void update_bus_stop_selection( int relative_change )
{
    text_layer_set_background_color( s_bus_stops[ s_selected_bus_stop_idx ].name, GColorWhite );
    text_layer_set_background_color( s_bus_stops[ s_selected_bus_stop_idx ].dist, GColorWhite );
    text_layer_set_text_color( s_bus_stops[ s_selected_bus_stop_idx ].name, GColorBlack );
    text_layer_set_text_color( s_bus_stops[ s_selected_bus_stop_idx ].dist, GColorBlack );
       
    int new_selected_idx = s_selected_bus_stop_idx + relative_change;
        
    if( new_selected_idx >= 0 && new_selected_idx < NUM_BUS_STOPS )
    {       
        s_selected_bus_stop_idx += relative_change;
    }
        
    text_layer_set_background_color( s_bus_stops[ s_selected_bus_stop_idx ].name, GColorDarkCandyAppleRed );
    text_layer_set_background_color( s_bus_stops[ s_selected_bus_stop_idx ].dist, GColorDarkCandyAppleRed );
    text_layer_set_text_color( s_bus_stops[ s_selected_bus_stop_idx ].name, GColorWhite );
    text_layer_set_text_color( s_bus_stops[ s_selected_bus_stop_idx ].dist, GColorWhite );
}

void apply_bus_stop_data()
{
    for( int i = 0; i != NUM_BUS_STOPS; ++i )
    {
        if( s_bus_stops[ i ].name != NULL && s_bus_stops[ i ].dist != NULL )
        {        
            text_layer_set_text( s_bus_stops[ i ].name, s_bus_stops[ i ].name_string );
            text_layer_set_text( s_bus_stops[ i ].dist, s_bus_stops[ i ].dist_string );
        }
    }
}

void parse_bus_stop_data( const char* bus_stop_data )
{
    for( int i = 0; i != NUM_BUS_STOPS; ++i )
    {
        if( *bus_stop_data != '\0' )
        {
            bus_stop_data = common_read_csv_item( bus_stop_data, s_bus_stops[ i ].name_string, BUS_STOP_NAME_SIZE );
            bus_stop_data = common_read_csv_item( bus_stop_data, s_bus_stops[ i ].dist_string, BUS_STOP_DIST_SIZE );
        }    
    }
    
    apply_bus_stop_data();
}


//==================================================================================================
//==================================================================================================
// Button click handling

void bus_stop_selection_previous_page( ClickRecognizerRef recognizer, void* context )
{
    update_bus_stop_selection( -1 );
}

void bus_stop_selection_next_page( ClickRecognizerRef recognizer, void* context )
{
    update_bus_stop_selection( +1 );
}

void bus_stop_selection_make_choice( ClickRecognizerRef recognizer, void* context )
{
    window_stack_pop( true );
}


void bus_stop_selection_click_provider( Window* window )
{
    window_single_click_subscribe( BUTTON_ID_SELECT, bus_stop_selection_make_choice );
    
    window_single_click_subscribe( BUTTON_ID_UP, bus_stop_selection_previous_page );
    window_single_click_subscribe( BUTTON_ID_DOWN, bus_stop_selection_next_page );    
}


//==================================================================================================
//==================================================================================================
// Window (un)loading

void bus_stop_selection_window_load()
{
    common_create_text_layer( &s_bus_stop_sel_title, s_bus_stop_sel_wnd, GRect( 24, 0, 120, 20 ),
                              GColorDarkCandyAppleRed, GColorWhite, FONT_KEY_GOTHIC_18_BOLD,
                              GTextAlignmentLeft );
    text_layer_set_text( s_bus_stop_sel_title, "Select bus stop" );
    
    common_create_h_icon( &s_bus_stop_sel_banner, s_bus_stop_sel_wnd );
        
    common_create_text_layer( &s_bus_stop_sel_status, s_bus_stop_sel_wnd, GRect( 0, 148, 144, 20 ), GColorDarkCandyAppleRed, GColorWhite, FONT_KEY_GOTHIC_14, GTextAlignmentCenter );
    text_layer_set_text( s_bus_stop_sel_status, "No updates, yet." );
    
    create_bus_stop_text_layers();
    update_bus_stop_selection( 0 );
}

void bus_stop_selection_window_unload()
{
    destroy_bus_stop_text_layers();
    text_layer_destroy( s_bus_stop_sel_status );
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
    
    window_set_click_config_provider( s_bus_stop_sel_wnd,
                                      ( ClickConfigProvider ) bus_stop_selection_click_provider );
}

void bus_stop_selection_destroy()
{
    common_set_current_bus_stop_id( s_bus_stops[ s_selected_bus_stop_idx ].id );
    window_destroy( s_bus_stop_sel_wnd );
}


void bus_stop_selection_show()
{
    if( s_bus_stop_sel_wnd )
    {
        window_stack_push( s_bus_stop_sel_wnd, true );
        apply_bus_stop_data(); // to show data that might have arrived before window
                               // was shown for the first time
    }
}
    

void bus_stop_selection_handle_msg_tuple( Tuple* msg_tuple )
{
    switch( msg_tuple->key )
    {
        case BUS_STOP_DATA:
        {
            parse_bus_stop_data( msg_tuple->value->cstring );
        }
        break;
        default:
        // intentionally left blank
        break;
    }
}