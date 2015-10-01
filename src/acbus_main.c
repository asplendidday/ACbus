#include <pebble.h>

#define BUS_STOP_NAME 0
#define BUS_STOP_DIST 1
#define GPS_COORDS    2
    
static Window* s_main_window = NULL;
static TextLayer* s_bus_station = NULL;

static void main_window_load()
{
    s_bus_station = text_layer_create( GRect( 0, 5, 144, 150 ) );
    text_layer_set_background_color( s_bus_station, GColorClear );
    text_layer_set_text_color( s_bus_station, GColorBlack );
    text_layer_set_text( s_bus_station, "Unknown station." );
    
    text_layer_set_text_alignment( s_bus_station, GTextAlignmentCenter );
    
    layer_add_child( window_get_root_layer( s_main_window ), text_layer_get_layer( s_bus_station ) ); 
}

static void main_window_unload()
{
    text_layer_destroy( s_bus_station );
}

static void inbox_received_callback( DictionaryIterator* iterator, void* context )
{
    APP_LOG( APP_LOG_LEVEL_INFO, "Received new message!" );
      
    Tuple* t = dict_read_first( iterator );
   
    char* bus_stop_name = NULL;
    char* bus_stop_dist = NULL;
    char* gps_coords = NULL;    
    
    while( t != NULL ) {
        switch( t->key ) {
            case BUS_STOP_NAME:
            bus_stop_name = t->value->cstring;
            break;
            case BUS_STOP_DIST:
            bus_stop_dist = t->value->cstring;
            break;
            case GPS_COORDS:
            gps_coords = t->value->cstring;
            break;
            default:
            APP_LOG( APP_LOG_LEVEL_ERROR, "[ACbus] Key %d not recognized!", ( int ) t->key );
            break;
        }
    
        t = dict_read_next( iterator );
    }
    
    char* output = "Closest bus stop\n";
    if( bus_stop_name != NULL )
    {
        output = strcat( output, bus_stop_name );
        output = strcat( output, "\nin " );
    };
    
    if( bus_stop_dist != NULL )
    {
        output = strcat( output, bus_stop_dist );
        output = strcat( output, " meters." );
    }
    
    if( gps_coords != NULL )
    {
        output = strcat( output, "\ngps coords (lon, lat):\n" );
        output = strcat( output, gps_coords );
    }
    
    text_layer_set_text( s_bus_station, output );    
}

static void inbox_dropped_callback( AppMessageResult reason, void* context )
{
    APP_LOG( APP_LOG_LEVEL_ERROR, "Message dropped!" );
}

static void outbox_failed_callback( DictionaryIterator* iterator, AppMessageResult reason, void* context )
{
    APP_LOG( APP_LOG_LEVEL_ERROR, "Outbox send failer!" );
}

static void outbox_sent_callback( DictionaryIterator* iterator, void* context )
{
    APP_LOG( APP_LOG_LEVEL_INFO, "Outbox send successful." );
}


static void init()
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
}

static void deinit()
{
	window_destroy( s_main_window );
}

int main(void)
{
	init();
	app_event_loop();
	deinit();
}
