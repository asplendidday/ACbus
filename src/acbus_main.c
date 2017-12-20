#include "common.h"
#include "bus_display.h"
#include "bus_stop_selection.h"


//==================================================================================================
//==================================================================================================
// Definitions

#define UPDATE_FREQUENCY_IN_SECS    30
    
    
//==================================================================================================
//==================================================================================================
// Variables

static int s_update_age_counter_in_secs = 0;
static int s_currently_updating = 0;
static int s_first_update_performed = 0;
static int s_first_update_after_n_secs = 2;

//==================================================================================================
//==================================================================================================
// App message handling

static void inbox_received_callback( DictionaryIterator* iterator, void* context )
{
    APP_LOG( APP_LOG_LEVEL_INFO, "[ACbus] Message received!" );
    
    Tuple* t = dict_read_first( iterator );
    s_currently_updating = 0;
   
    while( t != NULL )
    {
        // if bus stop data is in the message, it is a success
        if( t->key == BUS_STOP_DATA )
        {
            s_update_age_counter_in_secs = 0;
            s_first_update_performed = 1;
        }
        
        bus_display_handle_msg_tuple( t );
        bus_stop_selection_handle_msg_tuple( t );

        t = dict_read_next( iterator );
    }
}

static void inbox_dropped_callback( AppMessageResult reason, void* context )
{
    APP_LOG( APP_LOG_LEVEL_ERROR, "[ACbus] Message dropped!" );
}

static void outbox_failed_callback( DictionaryIterator* iterator, AppMessageResult reason, void* context )
{
    APP_LOG( APP_LOG_LEVEL_ERROR, "[ACbus] Outbox send failed! Reason: %s",
             common_app_message_result_to_string( reason ) );
    s_currently_updating = 0;
}

static void outbox_sent_callback( DictionaryIterator* iterator, void* context )
{
    APP_LOG( APP_LOG_LEVEL_INFO, "[ACbus] Outbox send successful." );
}


//==================================================================================================
//==================================================================================================
// Update request message

static void send_update_request()
{
    if( s_currently_updating == 0 )
    {
        s_currently_updating = 1;
    
        DictionaryIterator* iter = NULL;
        app_message_outbox_begin( &iter );
        
        dict_write_uint32( iter, REQ_BUS_STOP_ID, common_get_current_bus_stop_id() );
        dict_write_uint8( iter, REQ_UPDATE_BUS_STOP_LIST, 0 );
        
        app_message_outbox_send();
    }    
}


//==================================================================================================
//==================================================================================================
// Tick handling

static void tick_handler( struct tm* tick_time, TimeUnits unites_changed )
{
    ++s_update_age_counter_in_secs;
    
    if( s_update_age_counter_in_secs % UPDATE_FREQUENCY_IN_SECS == 0 ||
        ( s_first_update_performed == 0 && s_update_age_counter_in_secs == s_first_update_after_n_secs ) )
    {   
        APP_LOG( APP_LOG_LEVEL_INFO, "[ACbus] Requesting bus update." ); 
        common_get_update_callback()();
    }
}

static void tap_handler( AccelAxisType axis, int32_t direction )
{
    common_get_update_callback()();
}


//==================================================================================================
//==================================================================================================
// Main and (de)init functions

static void init()
{
    // set up global common state
    common_set_update_callback( send_update_request );
    
    // init windows
    bus_stop_selection_create();
    bus_display_create();
    bus_display_show();
    bus_stop_selection_show();
    
    // set up app messages
    app_message_register_inbox_received( inbox_received_callback );
    app_message_register_inbox_dropped( inbox_dropped_callback );
    app_message_register_outbox_failed( outbox_failed_callback );
    app_message_register_outbox_sent( outbox_sent_callback );
    app_message_open( app_message_inbox_size_maximum(), app_message_outbox_size_maximum() );
    
    // set up periodic updates
    tick_timer_service_subscribe( SECOND_UNIT, tick_handler );
    
    // set up tap recognition
    accel_tap_service_subscribe( tap_handler );
}

static void deinit()
{
    bus_display_destroy();
    bus_stop_selection_destroy();
}


int main()
{
	init();
	app_event_loop();
	deinit();
}
