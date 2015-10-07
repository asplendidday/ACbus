#include "bus_display.h"

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

static Window* s_main_window = NULL;
static TextLayer* s_bus_station = NULL;
static TextLayer* s_next_station = NULL;
static BitmapLayer* s_banner = NULL;
static GColor s_line_colors[10];
    
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
// Local functions

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

void create_text_layer( TextLayer** text_layer, GRect rect, GColor back_color, GColor text_color, const char* font_name, GTextAlignment text_align )
{
    *text_layer = text_layer_create( rect );
    
    text_layer_set_background_color( *text_layer, back_color );
    text_layer_set_text_color( *text_layer, text_color );
    text_layer_set_font( *text_layer, fonts_get_system_font( font_name ) );
    text_layer_set_text_alignment( *text_layer, text_align );
    text_layer_set_text( *text_layer, "" );
    
    layer_add_child( window_get_root_layer( s_main_window ), text_layer_get_layer( *text_layer ) ); 
}

void create_bus_text_layers()
{    
    for( int i = 0; i < NUM_BUSES; ++i )
    {
        create_text_layer( &s_buses[ i ].line, line_rect( i ), GColorWhite, GColorBlack, FONT_KEY_GOTHIC_14_BOLD, GTextAlignmentCenter);
        create_text_layer( &s_buses[ i ].dest, dest_rect( i ), GColorWhite, GColorBlack, FONT_KEY_GOTHIC_14, GTextAlignmentLeft );
        create_text_layer( &s_buses[ i ].eta, eta_rect( i ), GColorWhite, GColorBlack, FONT_KEY_GOTHIC_14_BOLD, GTextAlignmentRight );
        
        /*
        text_layer_set_text( s_buses[ i ].line, "1234" );
        text_layer_set_text( s_buses[ i ].dest, "Awaiting update..." );
        text_layer_set_text( s_buses[ i ].eta, "999" );
        */
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


void update_proc( Layer* layer, GContext* context )
{
    graphics_context_set_fill_color( context, GColorDarkCandyAppleRed );
    graphics_fill_rect( context, GRect( 0, 0, 144, 25 ), 0, GCornerNone );
    graphics_fill_rect( context, GRect( 0, 148, 144, 20 ), 0, GCornerNone );
}

void fill_line_colors()
{
    s_line_colors[0] = GColorIslamicGreen;
    s_line_colors[1] = GColorMintGreen;
    s_line_colors[2] = GColorMidnightGreen;
    s_line_colors[3] = GColorVividCerulean;
    s_line_colors[4] = GColorChromeYellow;
    s_line_colors[5] = GColorSunsetOrange;
    s_line_colors[6] = GColorIndigo;
    s_line_colors[7] = GColorBrilliantRose;
    s_line_colors[8] = GColorCadetBlue;
    s_line_colors[9] = GColorYellow;
}


/**
 * Use the line name to get color
 */
GColor get_line_color ( const char* line )
{
    int hash = 0;
    char c = line[0]+13;
    int i = 1;
    do
    {   
        hash += ( int )c + 13;
        c = line[i];
        i++;
        
    } while( c != '\0' );
  
    while ( hash > 9 )
    {
        int tmp = ( hash % 10 ) + ( (hash/10) % 10 ) + ( (hash/100) % 10 );
        hash = tmp;
    }
    return s_line_colors[hash];
}


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
    
    text_layer_set_text( s_next_station, timestamp_text );
}


void window_load()
{
    create_text_layer( &s_bus_station, GRect( 24, 0, 120, 20 ), GColorDarkCandyAppleRed, GColorWhite, FONT_KEY_GOTHIC_18_BOLD, GTextAlignmentLeft );
    text_layer_set_text( s_bus_station, "Initializing ..." );
 
    create_text_layer( &s_next_station, GRect( 0, 148, 144, 20 ), GColorDarkCandyAppleRed, GColorWhite, FONT_KEY_GOTHIC_14, GTextAlignmentCenter );
    text_layer_set_text( s_next_station, "No updates, yet." );
    
    GBitmap* h_icon = gbitmap_create_with_resource( RESOURCE_ID_ICON_H );
    s_banner = bitmap_layer_create( GRect( 3, 3, 18, 18 ) );
    bitmap_layer_set_compositing_mode( s_banner, GCompOpSet );
    bitmap_layer_set_background_color( s_banner, GColorClear );
    bitmap_layer_set_bitmap( s_banner, h_icon );
  
    layer_add_child(window_get_root_layer( s_main_window ), bitmap_layer_get_layer(s_banner));
    layer_set_update_proc( window_get_root_layer ( s_main_window ), update_proc );
    
    create_bus_text_layers(); 
}

void window_unload()
{
    destroy_bus_text_layers();
    bitmap_layer_destroy( s_banner );
    text_layer_destroy( s_next_station );
    text_layer_destroy( s_bus_station );    
}



//==================================================================================================
//==================================================================================================
// Interface functions

void bus_display_create()
{
    s_main_window = window_create();
    
	window_set_window_handlers( s_main_window, ( WindowHandlers )
        {
            .load = window_load,
            .unload = window_unload
        } );   
        
    fill_line_colors();    
}

void bus_display_destroy()
{
	window_destroy( s_main_window );
}


void bus_display_show()
{
    window_stack_push( s_main_window, true );   
}


void bus_display_handle_msg_tuple( Tuple* msg_tuple )
{
    switch( msg_tuple->key )
    {
        case BUS_STOP_NAME:
        {
            snprintf( s_bus_stop_name, sizeof( s_bus_stop_name ), "%s", msg_tuple->value->cstring );
            text_layer_set_text( s_bus_station, s_bus_stop_name );
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
