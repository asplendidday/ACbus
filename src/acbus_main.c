#include <pebble.h>
    
    
//==================================================================================================
//==================================================================================================
// Definitions
    
// Functions
#ifndef min
#define min(a,b) (((a) < (b)) ? (a) : (b))
#endif

// App message ids
#define BUS_STOP_NAME            0
#define BUS_STOP_DIST            1
#define GPS_COORDS               2
#define BUS_DATA                 3   

// Layout information
#define NUM_BUSES                7
    
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
// Global variables

static Window* s_main_window = NULL;
static TextLayer* s_bus_station = NULL;
static TextLayer* s_next_station = NULL;
static BitmapLayer* s_banner = NULL;
    
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
// Helper functions

GRect line_rect( int index )
{
    return GRect( BUS_ENTRY_MARGIN_LEFT,
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
        create_text_layer( &s_buses[ i ].line, line_rect( i ), GColorWhite, GColorBlack, FONT_KEY_GOTHIC_14_BOLD, GTextAlignmentLeft );
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
        text_layer_set_text( s_buses[ i ].dest, s_buses[ i ].dest_string );
        text_layer_set_text( s_buses[ i ].eta, s_buses[ i ].eta_string );
    }
}


//==================================================================================================
//==================================================================================================
// App message handling

void inbox_received_callback( DictionaryIterator* iterator, void* context )
{
    APP_LOG( APP_LOG_LEVEL_INFO, "[ACbus] Message received!" );
    
    Tuple* t = dict_read_first( iterator );
   
    while( t != NULL ) {
        switch( t->key ) {
            case BUS_STOP_NAME:
            {
                snprintf( s_bus_stop_name, sizeof( s_bus_stop_name ), "%s", t->value->cstring );
                text_layer_set_text( s_bus_station, s_bus_stop_name );
            }            
            break;
            case BUS_STOP_DIST:
            /* not used at the moment */
            break;
            case GPS_COORDS:
            /* not used at the moment */
            break;
            case BUS_DATA:
            parse_bus_data( t->value->cstring );
            break;
            default:
            APP_LOG( APP_LOG_LEVEL_WARNING, "[ACbus] Unknown message key %d!", ( int ) t->key );
            break;
        }
    
        t = dict_read_next( iterator );
    }
    
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

void inbox_dropped_callback( AppMessageResult reason, void* context )
{
    APP_LOG( APP_LOG_LEVEL_ERROR, "[ACbus] Message dropped!" );
}

void outbox_failed_callback( DictionaryIterator* iterator, AppMessageResult reason, void* context )
{
    APP_LOG( APP_LOG_LEVEL_ERROR, "[ACbus] Outbox send failed!" );
}

void outbox_sent_callback( DictionaryIterator* iterator, void* context )
{
    APP_LOG( APP_LOG_LEVEL_INFO, "[ACbus] Outbox send successful." );
}


//==================================================================================================
//==================================================================================================
// Update request message

void send_update_request()
{
    DictionaryIterator* iter = NULL;
    app_message_outbox_begin( &iter );
    
    dict_write_uint8( iter, 0, 0 );
    
    app_message_outbox_send();
    
    text_layer_set_text( s_next_station, "Updating..." );
}


//==================================================================================================
//==================================================================================================
// Tick handling

void tick_handler( struct tm* tick_time, TimeUnits unites_changed )
{
    send_update_request();
}


//==================================================================================================
//==================================================================================================
// Button click handling

void button_single_click_handler( ClickRecognizerRef recognizer, void* context )
{
    send_update_request();
}

void click_provider( Window* window )
{
    window_single_click_subscribe( BUTTON_ID_SELECT, button_single_click_handler );
}


//==================================================================================================
//==================================================================================================
// Main and (de)init functions


void main_window_load()
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

void main_window_unload()
{
    destroy_bus_text_layers();
    bitmap_layer_destroy( s_banner );
    text_layer_destroy( s_next_station );
    text_layer_destroy( s_bus_station );
}


void init()
{
    // create main window
    s_main_window = window_create();
    
	window_set_window_handlers( s_main_window, ( WindowHandlers )
        {
            .load = main_window_load,
            .unload = main_window_unload
        } );
    
    window_stack_push( s_main_window, true );
    
    // register app message handlers and init it
    app_message_register_inbox_received( inbox_received_callback );
    app_message_register_inbox_dropped( inbox_dropped_callback );
    app_message_register_outbox_failed( outbox_failed_callback );
    app_message_register_outbox_sent( outbox_sent_callback );
    app_message_open( app_message_inbox_size_maximum(), app_message_outbox_size_maximum() );
    
    tick_timer_service_subscribe( MINUTE_UNIT, tick_handler );
    
    window_set_click_config_provider( s_main_window, ( ClickConfigProvider ) click_provider );
}

void deinit()
{
	window_destroy( s_main_window );
}


int main(void)
{
	init();
	app_event_loop();
	deinit();
}
