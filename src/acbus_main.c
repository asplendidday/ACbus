#include "common.h"
#include "bus_display.h"
#include "bus_stop_selection.h"
    

//==================================================================================================
//==================================================================================================
// App message handling

void inbox_received_callback( DictionaryIterator* iterator, void* context )
{
    APP_LOG( APP_LOG_LEVEL_INFO, "[ACbus] Message received!" );
    
    Tuple* t = dict_read_first( iterator );
   
    while( t != NULL )
    {
        bus_display_handle_msg_tuple( t );
        bus_stop_selection_handle_msg_tuple( t );

        t = dict_read_next( iterator );
    }
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
    
    //text_layer_set_text( s_next_station, "Updating..." );
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

void update_click_handler( ClickRecognizerRef recognizer, void* context )
{
    send_update_request();
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
// Main and (de)init functions


void init()
{
    bus_display_create();
    bus_display_show();
    
    bus_stop_selection_create();
    
    // register app message handlers and init it
    app_message_register_inbox_received( inbox_received_callback );
    app_message_register_inbox_dropped( inbox_dropped_callback );
    app_message_register_outbox_failed( outbox_failed_callback );
    app_message_register_outbox_sent( outbox_sent_callback );
    app_message_open( app_message_inbox_size_maximum(), app_message_outbox_size_maximum() );
    
    tick_timer_service_subscribe( MINUTE_UNIT, tick_handler );
    
    //window_set_click_config_provider( s_main_window, ( ClickConfigProvider ) click_provider );}
}

void deinit()
{
    bus_stop_selection_destroy();
    bus_display_destroy();
}


int main(void)
{
	init();
	app_event_loop();
	deinit();
}
