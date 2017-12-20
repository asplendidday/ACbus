#include "bus_stop_selection.h"

//==================================================================================================
//==================================================================================================
// Definitions

#define NUM_BUS_STOPS             9

#define BUS_STOP_MARGIN_TOP      25
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
static BitmapLayer* s_bus_stop_sel_banner = NULL;

static struct {
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

static GRect bus_stop_name_rect( int index )
{
    return GRect( BUS_STOP_MARGIN_LEFT,
                  BUS_STOP_MARGIN_TOP + index * BUS_STOP_HEIGHT,
                  BUS_STOP_NAME_WIDTH,
                  BUS_STOP_HEIGHT );
}

static GRect bus_stop_dist_rect( int index )
{
    return GRect( BUS_STOP_MARGIN_LEFT + BUS_STOP_NAME_WIDTH,
                  BUS_STOP_MARGIN_TOP + index * BUS_STOP_HEIGHT,
                  BUS_STOP_DIST_WIDTH,
                  BUS_STOP_HEIGHT );
}


static void create_bus_stop_text_layers()
{
    for( int i = 0; i < NUM_BUS_STOPS; ++i )
    {
        common_create_text_layer( &s_bus_stops[ i ].name, s_bus_stop_sel_wnd,
                                  bus_stop_name_rect( i ), GColorWhite, GColorBlack,
                                  FONT_KEY_GOTHIC_14, GTextAlignmentLeft );
        common_create_text_layer( &s_bus_stops[ i ].dist, s_bus_stop_sel_wnd,
                                  bus_stop_dist_rect( i ), GColorWhite, GColorBlack,
                                  FONT_KEY_GOTHIC_14, GTextAlignmentRight );
    }
}

static void destroy_bus_stop_text_layers()
{
    for( int i = 0; i < NUM_BUS_STOPS; ++i )
    {
        text_layer_destroy( s_bus_stops[ i ].name );
        text_layer_destroy( s_bus_stops[ i ].dist );
    }
}


static void update_bus_stop_selection( int relative_change )
{
    text_layer_set_background_color( s_bus_stops[ s_selected_bus_stop_idx ].name, GColorWhite );
    text_layer_set_background_color( s_bus_stops[ s_selected_bus_stop_idx ].dist, GColorWhite );
    text_layer_set_text_color( s_bus_stops[ s_selected_bus_stop_idx ].name, GColorBlack );
    text_layer_set_text_color( s_bus_stops[ s_selected_bus_stop_idx ].dist, GColorBlack );
       
    const int new_selected_idx = s_selected_bus_stop_idx + relative_change;
        
    if( new_selected_idx >= 0 && new_selected_idx < NUM_BUS_STOPS )
    {       
        s_selected_bus_stop_idx += relative_change;
    }
        
    text_layer_set_background_color( s_bus_stops[ s_selected_bus_stop_idx ].name, GColorDarkCandyAppleRed );
    text_layer_set_background_color( s_bus_stops[ s_selected_bus_stop_idx ].dist, GColorDarkCandyAppleRed );
    text_layer_set_text_color( s_bus_stops[ s_selected_bus_stop_idx ].name, GColorWhite );
    text_layer_set_text_color( s_bus_stops[ s_selected_bus_stop_idx ].dist, GColorWhite );
}

static void apply_bus_stop_data()
{
    for( int i = 0; i < NUM_BUS_STOPS; ++i )
    {
        text_layer_set_text( s_bus_stops[ i ].name, s_bus_stops[ i ].name_string );
        text_layer_set_text( s_bus_stops[ i ].dist, s_bus_stops[ i ].dist_string );
    }
}

static void parse_bus_stop_data( const char* bus_stop_data )
{
    APP_LOG( APP_LOG_LEVEL_INFO, "[ACbus] parse_bus_stop_data: %s", bus_stop_data );
    
    char id_buffer[ 8 ];
    
    for( int i = 0; i < NUM_BUS_STOPS; ++i )
	{
		if( !*bus_stop_data ) break;

		bus_stop_data = common_read_csv_item( bus_stop_data, s_bus_stops[ i ].name_string, BUS_STOP_NAME_SIZE );
		bus_stop_data = common_read_csv_item( bus_stop_data, s_bus_stops[ i ].dist_string, BUS_STOP_DIST_SIZE );
		bus_stop_data = common_read_csv_item( bus_stop_data, id_buffer, sizeof( id_buffer ) );
		s_bus_stops[ i ].id = atoi( id_buffer );
	}
    
    apply_bus_stop_data();
}


//==================================================================================================
//==================================================================================================
// Button click handling

static void bus_stop_selection_previous_page( ClickRecognizerRef recognizer, void* context )
{
    update_bus_stop_selection( -1 );
}

static void bus_stop_selection_next_page( ClickRecognizerRef recognizer, void* context )
{
    update_bus_stop_selection( +1 );
}

static void bus_stop_selection_make_choice( ClickRecognizerRef recognizer, void* context )
{
    common_set_current_bus_stop_id( s_bus_stops[ s_selected_bus_stop_idx ].id );
    common_get_update_callback()();
    window_stack_pop( true );
}


static void bus_stop_selection_click_provider( Window* window )
{
    window_single_click_subscribe( BUTTON_ID_SELECT, bus_stop_selection_make_choice );
    
    window_single_click_subscribe( BUTTON_ID_UP, bus_stop_selection_previous_page );
    window_single_click_subscribe( BUTTON_ID_DOWN, bus_stop_selection_next_page );    
}


//==================================================================================================
//==================================================================================================
// Window (un)loading

static void bus_stop_selection_create_resources()
{
    common_create_text_layer( &s_bus_stop_sel_title, s_bus_stop_sel_wnd, GRect( 24, 0, 120, 20 ),
                              GColorDarkCandyAppleRed, GColorWhite, FONT_KEY_GOTHIC_18_BOLD,
                              GTextAlignmentLeft );
    text_layer_set_text( s_bus_stop_sel_title, "Select bus stop" );
    
    common_create_h_icon( &s_bus_stop_sel_banner, s_bus_stop_sel_wnd );
        
    create_bus_stop_text_layers();    
}

static void bus_stop_selection_destroy_resources()
{   
    destroy_bus_stop_text_layers();
    bitmap_layer_destroy( s_bus_stop_sel_banner );
    text_layer_destroy( s_bus_stop_sel_title );   
}


static void bus_stop_selection_window_load()
{
    update_bus_stop_selection( -s_selected_bus_stop_idx ); // reset to index 0
}



//==================================================================================================
//==================================================================================================
// Interface functions

void bus_stop_selection_create()
{
    s_bus_stop_sel_wnd = window_create();
    bus_stop_selection_create_resources();

    window_set_window_handlers( s_bus_stop_sel_wnd, ( WindowHandlers )
    {
        .load = bus_stop_selection_window_load
    } );
    
    window_set_click_config_provider( s_bus_stop_sel_wnd,
                                      ( ClickConfigProvider ) bus_stop_selection_click_provider );
}

void bus_stop_selection_destroy()
{
    bus_stop_selection_destroy_resources();
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
