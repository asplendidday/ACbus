#include "bus_display.h"
#include "bus_stop_selection.h"

//==================================================================================================
//==================================================================================================
// Definitions

// Layout information
#define NUM_BUSES               21
#define NUM_BUSES_PER_PAGE       7
    
#define BUS_ENTRY_MARGIN_TOP    28
#define BUS_ENTRY_MARGIN_LEFT    3
#define BUS_ENTRY_HEIGHT        16
#define BUS_ENTRY_LINE_WIDTH    28
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
static int s_current_page = 0;
    
static char s_bus_stop_name[ DEST_BUFFER_SIZE ];
    
struct {
    TextLayer* line;
    TextLayer* dest;
    TextLayer* eta;
} s_bus_display_lines[ NUM_BUSES_PER_PAGE ];
    
struct {    
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
    for( int i = 0; i < NUM_BUSES_PER_PAGE; ++i )
    {
        common_create_text_layer( &s_bus_display_lines[ i ].line, s_bus_display_wnd, line_rect( i ), GColorWhite, GColorBlack, FONT_KEY_GOTHIC_14_BOLD, GTextAlignmentCenter );
        common_create_text_layer( &s_bus_display_lines[ i ].dest, s_bus_display_wnd, dest_rect( i ), GColorWhite, GColorBlack, FONT_KEY_GOTHIC_14, GTextAlignmentLeft );
        common_create_text_layer( &s_bus_display_lines[ i ].eta, s_bus_display_wnd, eta_rect( i ), GColorWhite, GColorBlack, FONT_KEY_GOTHIC_14_BOLD, GTextAlignmentRight );
    }
}

void destroy_bus_text_layers()
{
    for( int i = 0; i < NUM_BUSES_PER_PAGE; ++i )
    {
        text_layer_destroy( s_bus_display_lines[ i ].line );
        text_layer_destroy( s_bus_display_lines[ i ].dest );
        text_layer_destroy( s_bus_display_lines[ i ].eta );
    }
}

void set_bus_text_layer( int index, const char* line, GColor line_color, const char* dest, const char* eta )
{
    text_layer_set_text( s_bus_display_lines[ index ].line, line );
    text_layer_set_background_color( s_bus_display_lines[ index ].line, line_color );
    text_layer_set_text( s_bus_display_lines[ index ].dest, dest );
    text_layer_set_text( s_bus_display_lines[ index ].eta, eta );  
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
    if( *line == '\0' )
    {
        return GColorWhite;    
    }
    
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


void update_bus_text_layers()
{
    for( int i = 0; i < NUM_BUSES_PER_PAGE; ++i )
    {
        int base_index = NUM_BUSES_PER_PAGE * s_current_page;
        int bus_index = base_index + i;
        
        set_bus_text_layer( i, s_buses[ bus_index ].line_string,
                               get_line_color( s_buses[ bus_index ].line_string ),
                               s_buses[ bus_index ].dest_string,
                               s_buses[ bus_index ].eta_string );
    }
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

void parse_first_bus_stop( const char* bus_stop_data )
{
    if( bus_stop_data != NULL && bus_stop_data != '\0' )
    {
        // add no indicator if bus stop is detected automatically
        if( common_get_current_bus_stop_id() == -1 )
        {
            common_read_csv_item( bus_stop_data, s_bus_stop_name, DEST_BUFFER_SIZE );            
        }
        else
        {
            char bus_stop_name[ DEST_BUFFER_SIZE ];
            common_read_csv_item( bus_stop_data, bus_stop_name, DEST_BUFFER_SIZE );
            // 2 + strlen --> * char + \0 char + bus stop name
            snprintf( s_bus_stop_name, 2 + strlen( bus_stop_name ), "*%s", bus_stop_name );
        }
        
        text_layer_set_text( s_bus_display_title, s_bus_stop_name );
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
            bus_data = common_read_csv_item( bus_data, s_buses[ i ].line_string, LINE_BUFFER_SIZE );
            // read destination
            bus_data = common_read_csv_item( bus_data, s_buses[ i ].dest_string, DEST_BUFFER_SIZE );
            // read eta
            bus_data = common_read_csv_item( bus_data, s_buses[ i ].eta_string, ETA_BUFFER_SIZE );
        }
        else
        {
            // if no data is available, we still need to reset the strings
            s_buses[ i ].line_string[ 0 ] = '\0';
            s_buses[ i ].dest_string[ 0 ] = '\0';
            s_buses[ i ].eta_string[ 0 ] = '\0';
        }
    }
    
    update_bus_text_layers();
}


//==================================================================================================
//==================================================================================================
// Button click handling

void bus_display_previous_page( ClickRecognizerRef recognizer, void* context )
{
    if( s_current_page > 0 )
    {
        --s_current_page;
        update_bus_text_layers();
    }
}

void bus_display_next_page( ClickRecognizerRef recognizer, void* context )
{
    int pages_required = ( NUM_BUSES / NUM_BUSES_PER_PAGE ) + ( NUM_BUSES % NUM_BUSES_PER_PAGE != 0 ? 1 : 0 );
    if( s_current_page + 1 < pages_required )
    {
        ++s_current_page;
        update_bus_text_layers();
    }
}

void open_bus_stop_select_window_handler( ClickRecognizerRef recognizer, void* context )
{
    bus_stop_selection_show();
}

void click_provider( Window* window )
{
    window_single_click_subscribe( BUTTON_ID_SELECT, open_bus_stop_select_window_handler );
    
    window_single_click_subscribe( BUTTON_ID_UP, bus_display_previous_page );
    window_single_click_subscribe( BUTTON_ID_DOWN, bus_display_next_page );    
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
        case BUS_STOP_DATA:
        {
            parse_first_bus_stop( msg_tuple->value->cstring );
        }            
        break;
        case BUS_DATA:
        {
            parse_bus_data( msg_tuple->value->cstring );
        }
        break;
        default:
        // intentionally left blank
        break;
    }
}


void bus_display_set_update_status_text( const char* status_text )
{
    text_layer_set_text( s_bus_display_status, status_text );
}
