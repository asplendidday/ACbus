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
    
    dict_write_uint32( iter, REQ_BUS_STOP_ID, common_get_current_bus_stop_id() );
    dict_write_uint8( iter, REQ_UPDATE_BUS_STOP_LIST, 0 );
    
    app_message_outbox_send();
    
    bus_display_indicate_update_pending();
}


//==================================================================================================
//==================================================================================================
// Tick handling

void tick_handler( struct tm* tick_time, TimeUnits unites_changed )
{
    common_get_update_callback()();
}

void tap_handler( AccelAxisType axis, int32_t direction )
{
    common_get_update_callback()();
}


//==================================================================================================
//==================================================================================================
// Main and (de)init functions


void init()
{
    // set up global common state
    common_set_update_callback( send_update_request );
    
    // init windows
    bus_stop_selection_create();
    
    bus_display_create();
    bus_display_show();
    
    // set up app messages
    app_message_register_inbox_received( inbox_received_callback );
    app_message_register_inbox_dropped( inbox_dropped_callback );
    app_message_register_outbox_failed( outbox_failed_callback );
    app_message_register_outbox_sent( outbox_sent_callback );
    app_message_open( app_message_inbox_size_maximum(), app_message_outbox_size_maximum() );
    
    // set up periodic updates
    tick_timer_service_subscribe( MINUTE_UNIT, tick_handler );
    
    // set up tap recognition
    accel_tap_service_subscribe( tap_handler );
}

void deinit()
{
    bus_display_destroy();
    bus_stop_selection_destroy();
}


int main(void)
{
	init();
	app_event_loop();
	deinit();
}
