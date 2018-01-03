#include "common.h"
#include "bus_display.h"
#include "bus_stop_selection.h"


//==================================================================================================
//==================================================================================================
// Definitions

// Seconds after a successful update before we trigger the next update
#define UPDATE_EVERY_SECS    20

// 1 to reload bus data when shaking the watch.
// 0 to conserve battery.
#define RELOAD_ON_TAP 0
    
//==================================================================================================
//==================================================================================================
// Variables

static int s_secs_since_update = 0;
static int s_secs_before_next_update = 2;
static bool s_offline = false;


//==================================================================================================
//==================================================================================================
// Helper functions

static void refresh_update_status()
{
    // NOTE: This function is called once per second so keep it fast

    // Allow half the update frequency for the actual update before
    // declaring ourselves offline.
    const bool offline = s_secs_since_update > UPDATE_EVERY_SECS * 3 / 2;

    if ( offline != s_offline )
    {
        APP_LOG( APP_LOG_LEVEL_INFO, "[ACbus] Connection status is now o%sline", offline ? "ff" : "n" );

        s_offline = offline;
        bus_display_set_update_status( offline );
        bus_stop_selection_set_update_status( offline );

        // If we are offline, update bus list with ETA estimates based
        // on latest info and time elapsed since
        if( offline )
        {
            bus_display_estimate_eta( s_secs_since_update );
        }
    }
 }


//==================================================================================================
//==================================================================================================
// App message handling

static void inbox_received_callback( DictionaryIterator* iterator, void* context )
{
    APP_LOG( APP_LOG_LEVEL_INFO, "[ACbus] Message received!" );
    
    Tuple* t = dict_read_first( iterator );
   
    while( t != NULL )
    {
        // if bus stop data is in the message, it is a success
        if( t->key == BUS_STOP_DATA )
        {
            s_secs_since_update = 0;
            s_secs_before_next_update = UPDATE_EVERY_SECS;
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

    // Try again sooner than we would if the update had succeeded
    s_secs_before_next_update = UPDATE_EVERY_SECS / 3;
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
    DictionaryIterator* iter = NULL;
    app_message_outbox_begin( &iter );
    
    dict_write_uint32( iter, REQ_BUS_STOP_ID, common_get_current_bus_stop_id() );
    dict_write_uint8( iter, REQ_UPDATE_BUS_STOP_LIST, 0 );
    
    app_message_outbox_send();

    refresh_update_status();
}


//==================================================================================================
//==================================================================================================
// Tick handling

static void tick_handler( struct tm* tick_time, TimeUnits unites_changed )
{
    // NOTE: This function is called once per second so keep it quick

    ++s_secs_since_update;

    refresh_update_status();
   
    if (--s_secs_before_next_update <= 0 )
    {   
        s_secs_before_next_update = UPDATE_EVERY_SECS;
        APP_LOG( APP_LOG_LEVEL_INFO, "[ACbus] Requesting bus update." ); 
        common_get_update_callback()();

        // If we are offline, update bus list with ETA estimates based
        // on latest info and time elapsed since
        if( s_offline )
        {
            bus_display_estimate_eta( s_secs_since_update );
        }
    }
}

#if RELOAD_ON_TAP
static void tap_handler( AccelAxisType axis, int32_t direction )
{
    common_get_update_callback()();
}
#endif


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
    #if RELOAD_ON_TAP
    accel_tap_service_subscribe( tap_handler );
    #endif
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
