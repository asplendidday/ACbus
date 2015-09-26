#include <pebble.h>

enum AppMessageIds
{
    NEW_BUS_STOP = 1,
    NEW_BUS = 2
};
    
static Window* s_main_window = NULL;
static TextLayer* s_bus_station = NULL;

static void main_window_load()
{
    s_bus_station = text_layer_create( GRect( 0, 55, 144, 50 ) );
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
