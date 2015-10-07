#include "bus_display.h"
#include "bus_stop_selection.h"

//==================================================================================================
//==================================================================================================
// Definitions

// Layout information
#define NUM_BUSES                7
    
#define BUS_ENTRY_MARGIN_TOP    28
#define BUS_ENTRY_MARGIN_LEFT    3
#define BUS_ENTRY_HEIGHT        16
#define BUS_ENTRY_LINE_WIDTH    20
#define BUS_ENTRY_DEST_WIDTH    92
#define BUS_ENTRY_ETA_WIDTH     18

// Bus data buffer sizes
#define LINE_BUFFER_SIZE         6
#define DEST_BUFFER_SIZE        32
#define ETA_BUFFER_SIZE          6

//==================================================================================================
//==================================================================================================
// Variables

static Window* s_bus_display_wnd = NULL;
static TextLayer* s_bus_display_title = NULL;
static TextLayer* s_bus_display_status = NULL;
static BitmapLayer* s_bus_display_banner = NULL;
static GColor s_line_colors[ 10 ];
    
static char s_bus_stop_name[ DEST_BUFFER_SIZE ];
    
struct {
    TextLayer* line;
    TextLayer* dest;
    TextLayer* eta;
    
    char line_string[ LINE_BUFFER_SIZE ];
    char dest_string[ DEST_BUFFER_SIZE ];
    char eta_string[ ETA_BUFFER_SIZE ];
} s_buses[ NUM_BUSES ];


//==================================================================================================
//==================================================================================================
// Various helper functions

GRect line_rect( int index )
{
    return GRect( 0,
                  BUS_ENTRY_MARGIN_TOP + index * BUS_ENTRY_HEIGHT,
                  BUS_ENTRY_LINE_WIDTH,
                  BUS_ENTRY_HEIGHT );
}

GRect dest_rect( int index )
{
    return GRect( BUS_ENTRY_MARGIN_LEFT + BUS_ENTRY_LINE_WIDTH,
                  BUS_ENTRY_MARGIN_TOP + index * BUS_ENTRY_HEIGHT,
                  BUS_ENTRY_DEST_WIDTH,
                  BUS_ENTRY_HEIGHT );
}

GRect eta_rect( int index )
{
    return GRect( BUS_ENTRY_MARGIN_LEFT + BUS_ENTRY_LINE_WIDTH + BUS_ENTRY_DEST_WIDTH,
                  BUS_ENTRY_MARGIN_TOP + index * BUS_ENTRY_HEIGHT,
                  BUS_ENTRY_ETA_WIDTH,
                  BUS_ENTRY_HEIGHT );
}

void create_bus_text_layers()
{    
    for( int i = 0; i < NUM_BUSES; ++i )
    {
        common_create_text_layer( &s_buses[ i ].line, s_bus_display_wnd, line_rect( i ), GColorWhite, GColorBlack, FONT_KEY_GOTHIC_14_BOLD, GTextAlignmentCenter);
        common_create_text_layer( &s_buses[ i ].dest, s_bus_display_wnd, dest_rect( i ), GColorWhite, GColorBlack, FONT_KEY_GOTHIC_14, GTextAlignmentLeft );
        common_create_text_layer( &s_buses[ i ].eta, s_bus_display_wnd, eta_rect( i ), GColorWhite, GColorBlack, FONT_KEY_GOTHIC_14_BOLD, GTextAlignmentRight );
    }
}

void destroy_bus_text_layers()
{
    for( int i = 0; i < NUM_BUSES; ++i )
    {
        text_layer_destroy( s_buses[ i ].line );
        text_layer_destroy( s_buses[ i ].dest );
        text_layer_destroy( s_buses[ i ].eta );
    }
}


void fill_line_colors()
{
    s_line_colors[ 0 ] = GColorIslamicGreen;
    s_line_colors[ 1 ] = GColorMintGreen;
    s_line_colors[ 2 ] = GColorMidnightGreen;
    s_line_colors[ 3 ] = GColorVividCerulean;
    s_line_colors[ 4 ] = GColorChromeYellow;
    s_line_colors[ 5 ] = GColorSunsetOrange;
    s_line_colors[ 6 ] = GColorIndigo;
    s_line_colors[ 7 ] = GColorBrilliantRose;
    s_line_colors[ 8 ] = GColorCadetBlue;
    s_line_colors[ 9 ] = GColorYellow;
}


/**
 * Use the line name to get color
 */
GColor get_line_color( const char* line )
{
    int hash = 0;
    char c = line[ 0 ]+13;
    int i = 1;
    do
    {   
        hash += ( int ) c + 13;
        c = line[ i ];
        i++;
        
    } while( c != '\0' );
  
    while( hash > 9 )
    {
        int tmp = ( hash % 10 ) + ( (hash/10) % 10 ) + ( (hash/100) % 10 );
        hash = tmp;
    }
    return s_line_colors[ hash ];
}

void update_time_stamp()
{
	// Update last update timestamp
    time_t temp = time( NULL );
    struct tm* tick_time = localtime( &temp );

    // static ensures longevity of buffer
    static char time_buffer[] = "00:00:00";

    if( clock_is_24h_style() == true )
    {
        strftime( time_buffer, sizeof( "00:00:00" ), "%H:%M:%S", tick_time );
    }
    else
    {
        strftime( time_buffer, sizeof( "00:00:00" ), "%I:%M:%S", tick_time );
    }
    
    static char timestamp_text[32];
    snprintf( timestamp_text, sizeof( timestamp_text ), "Last update: %s", time_buffer );
    
    text_layer_set_text( s_bus_display_status, timestamp_text );
}


//==================================================================================================
//==================================================================================================
// Message parsing

const char* find_next_separator( const char* cursor, const char separator )
{
    while( *cursor != separator && *cursor != '\0' )
    {
        ++cursor;
    }
    return cursor;    
}

const char* read_bus_item( const char* bus_data, char* target, int max_bytes )
{
    const char* end_cursor = find_next_separator( bus_data, ';' );
    
    // use "max_bytes - 1" as comparison to save a byte for the trailing '\0' char
    const int num_bytes = min( end_cursor - bus_data, max_bytes - 1 );
    
    // if num_bytes is 0 then target will be set to the empty string
    // in this case, memcpy will essentially be a noop
    memcpy( target, bus_data, num_bytes );
    target[ num_bytes ] = '\0';
    
    if( *end_cursor == '\0' )
    {
        return end_cursor;
    }
    else
    {
        return ++end_cursor;
    }
}

/**
 * This function takes the string as provided by a BUS_DATA app message, parses it,
 * and uses the parsed data to update all bus text layers.
 */
void parse_bus_data( const char* bus_data )
{   
    for( int i = 0; i < NUM_BUSES; ++i )
    {
        if( *bus_data != '\0' ) // eof reached?
        {
            // read line
            bus_data = read_bus_item( bus_data, s_buses[ i ].line_string, LINE_BUFFER_SIZE );
            // read destination
            bus_data = read_bus_item( bus_data, s_buses[ i ].dest_string, DEST_BUFFER_SIZE );
            // read eta
            bus_data = read_bus_item( bus_data, s_buses[ i ].eta_string, ETA_BUFFER_SIZE );
        }
        
        // set texts layers
        // always do this to clear data in case there is no data for some layers
        text_layer_set_text( s_buses[ i ].line, s_buses[ i ].line_string );
        text_layer_set_background_color(s_buses[ i ].line, get_line_color(s_buses[ i ].line_string));
        text_layer_set_text( s_buses[ i ].dest, s_buses[ i ].dest_string );
        text_layer_set_text( s_buses[ i ].eta, s_buses[ i ].eta_string );
    }   
}


//==================================================================================================
//==================================================================================================
// Button click handling

void update_click_handler( ClickRecognizerRef recognizer, void* context )
{
    common_get_update_callback()();
}

void open_bus_stop_select_window_handler( ClickRecognizerRef recognizer, void* context )
{
    bus_stop_selection_show();
}

void click_provider( Window* window )
{
    window_single_click_subscribe( BUTTON_ID_SELECT, open_bus_stop_select_window_handler );
    
    window_single_click_subscribe( BUTTON_ID_DOWN, update_click_handler );
    window_single_click_subscribe( BUTTON_ID_UP, update_click_handler );
}


//==================================================================================================
//==================================================================================================
// Window (un)loading

void bus_display_window_load()
{
    common_create_text_layer( &s_bus_display_title, s_bus_display_wnd, GRect( 24, 0, 120, 20 ), GColorDarkCandyAppleRed, GColorWhite, FONT_KEY_GOTHIC_18_BOLD, GTextAlignmentLeft );
    text_layer_set_text( s_bus_display_title, "Initializing ..." );
 
    common_create_text_layer( &s_bus_display_status, s_bus_display_wnd, GRect( 0, 148, 144, 20 ), GColorDarkCandyAppleRed, GColorWhite, FONT_KEY_GOTHIC_14, GTextAlignmentCenter );
    text_layer_set_text( s_bus_display_status, "No updates, yet." );
    
    common_create_h_icon( &s_bus_display_banner, s_bus_display_wnd );
    
    create_bus_text_layers(); 
}

void bus_display_window_unload()
{
    destroy_bus_text_layers();
    bitmap_layer_destroy( s_bus_display_banner );
    text_layer_destroy( s_bus_display_status );
    text_layer_destroy( s_bus_display_title );    
}


//==================================================================================================
//==================================================================================================
// Interface functions

void bus_display_create()
{
    s_bus_display_wnd = window_create();
    
	window_set_window_handlers( s_bus_display_wnd, ( WindowHandlers )
        {
            .load = bus_display_window_load,
            .unload = bus_display_window_unload
        } );   
       
    fill_line_colors();
    
    window_set_click_config_provider( s_bus_display_wnd, ( ClickConfigProvider ) click_provider );  
}

void bus_display_destroy()
{
	window_destroy( s_bus_display_wnd );
}


void bus_display_show()
{
    window_stack_push( s_bus_display_wnd, true );   
}


void bus_display_handle_msg_tuple( Tuple* msg_tuple )
{
    switch( msg_tuple->key )
    {
        case BUS_STOP_NAME:
        {
            snprintf( s_bus_stop_name, sizeof( s_bus_stop_name ), "%s", msg_tuple->value->cstring );
            text_layer_set_text( s_bus_display_title, s_bus_stop_name );
        }            
        break;
        case BUS_DATA:
        {
            update_time_stamp();
            parse_bus_data( msg_tuple->value->cstring );
        }
        break;
        default:
        // intentionally left blank
        break;
    }
}


void bus_display_indicate_update_pending()
{
    text_layer_set_text( s_bus_display_status, "Updating..." );
}
