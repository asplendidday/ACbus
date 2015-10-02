#include <pebble.h>

#define BUS_STOP_NAME 0
#define BUS_STOP_DIST 1
#define GPS_COORDS    2
#define BUS_ONE       10
#define BUS_TWO       11
#define BUS_THREE     12
    
static Window* s_main_window = NULL;
static TextLayer* s_bus_station = NULL;
static TextLayer* s_next_station = NULL;
static BitmapLayer* s_banner = NULL;

#define NUM_BUSES 7
#define BUS_ENTRY_MARGIN_TOP 28
#define BUS_ENTRY_MARGIN_LEFT 3
#define BUS_ENTRY_HEIGHT 16
#define BUS_ENTRY_LINE_WIDTH 18
#define BUS_ENTRY_DEST_WIDTH 102
#define BUS_ENTRY_ETA_WIDTH 18
    
struct {
    TextLayer* line;
    TextLayer* dest;
    TextLayer* eta;
} s_buses[ NUM_BUSES ];

static void create_text_layer( TextLayer** text_layer, GRect rect, GColor back_color, GColor text_color, const char* font_name, GTextAlignment text_align )
{
    *text_layer = text_layer_create( rect );
    
    text_layer_set_background_color( *text_layer, back_color );
    text_layer_set_text_color( *text_layer, text_color );
    text_layer_set_font( *text_layer, fonts_get_system_font( font_name ) );
    text_layer_set_text_alignment( *text_layer, text_align );
    text_layer_set_text( *text_layer, "" );
    
    layer_add_child( window_get_root_layer( s_main_window ), text_layer_get_layer( *text_layer ) ); 
}

static GRect line_rect( int index )
{
    return GRect( BUS_ENTRY_MARGIN_LEFT,
                  BUS_ENTRY_MARGIN_TOP + index * BUS_ENTRY_HEIGHT,
                  BUS_ENTRY_LINE_WIDTH,
                  BUS_ENTRY_HEIGHT );
}

static GRect dest_rect( int index )
{
    return GRect( BUS_ENTRY_MARGIN_LEFT + BUS_ENTRY_LINE_WIDTH,
                  BUS_ENTRY_MARGIN_TOP + index * BUS_ENTRY_HEIGHT,
                  BUS_ENTRY_DEST_WIDTH,
                  BUS_ENTRY_HEIGHT );
}

static GRect eta_rect( int index )
{
    return GRect( BUS_ENTRY_MARGIN_LEFT + BUS_ENTRY_LINE_WIDTH + BUS_ENTRY_DEST_WIDTH,
                  BUS_ENTRY_MARGIN_TOP + index * BUS_ENTRY_HEIGHT,
                  BUS_ENTRY_ETA_WIDTH,
                  BUS_ENTRY_HEIGHT );
}

static void create_bus_text_layers()
{    
    for( int i = 0; i < NUM_BUSES; ++i )
    {
        create_text_layer( &s_buses[ i ].line, line_rect( i ), GColorWhite, GColorBlack, FONT_KEY_GOTHIC_14_BOLD, GTextAlignmentLeft );
        create_text_layer( &s_buses[ i ].dest, dest_rect( i ), GColorWhite, GColorBlack, FONT_KEY_GOTHIC_14, GTextAlignmentLeft );
        create_text_layer( &s_buses[ i ].eta, eta_rect( i ), GColorWhite, GColorBlack, FONT_KEY_GOTHIC_14_BOLD, GTextAlignmentRight );
        
        text_layer_set_text( s_buses[ i ].line, "33" );
        text_layer_set_text( s_buses[ i ].dest, "Bushof Vaals oder nicht?" );
        text_layer_set_text( s_buses[ i ].eta, "15" );
    }
}

static void destroy_bus_text_layers()
{
    for( int i = 0; i < NUM_BUSES; ++i )
    {
        text_layer_destroy( s_buses[ i ].line );
        text_layer_destroy( s_buses[ i ].dest );
        text_layer_destroy( s_buses[ i ].eta );
    }
}


static void update_proc(Layer *layer, GContext *ctx) {
    graphics_context_set_fill_color(ctx, GColorDarkCandyAppleRed);
    graphics_fill_rect(ctx, GRect(0, 0, 144, 25), 0, GCornerNone);
    graphics_fill_rect(ctx, GRect(0, 148, 144, 20), 0, GCornerNone);
}

static void main_window_load()
{
    create_text_layer( &s_bus_station, GRect( 24, 0, 120, 20 ), GColorDarkCandyAppleRed, GColorWhite, FONT_KEY_GOTHIC_18_BOLD, GTextAlignmentLeft );
    text_layer_set_text( s_bus_station, "Waiting for first update..." );
 
    create_text_layer( &s_next_station, GRect( 0, 148, 144, 20 ), GColorDarkCandyAppleRed, GColorWhite, FONT_KEY_GOTHIC_14, GTextAlignmentCenter );
    text_layer_set_text( s_next_station, "next" );
    
    GBitmap* h_icon = gbitmap_create_with_resource(RESOURCE_ID_ICON_H);
    s_banner = bitmap_layer_create(GRect(3, 3, 18, 18));
    bitmap_layer_set_compositing_mode(s_banner, GCompOpSet);
    bitmap_layer_set_background_color(s_banner, GColorClear);
    bitmap_layer_set_bitmap(s_banner, h_icon);   
  
    layer_add_child(window_get_root_layer(s_main_window), bitmap_layer_get_layer(s_banner));
    layer_set_update_proc(window_get_root_layer (s_main_window), update_proc);
    
    create_bus_text_layers();
}

static void main_window_unload()
{
    destroy_bus_text_layers();
    bitmap_layer_destroy( s_banner );
    text_layer_destroy( s_next_station );
    text_layer_destroy( s_bus_station );
}

static void inbox_received_callback( DictionaryIterator* iterator, void* context )
{
    APP_LOG( APP_LOG_LEVEL_INFO, "Received new message!" );
      
    Tuple* t = dict_read_first( iterator );
   
    char* bus_stop_name = NULL;
//    char* bus_stop_dist = NULL;
//    char* gps_coords = NULL;    
    
    while( t != NULL ) {
        switch( t->key ) {
            case BUS_STOP_NAME:
            bus_stop_name = t->value->cstring;
            break;
            case BUS_STOP_DIST:
//           bus_stop_dist = t->value->cstring;
            break;
            case GPS_COORDS:
//            gps_coords = t->value->cstring;
            break;
            default:
            APP_LOG( APP_LOG_LEVEL_ERROR, "[ACbus] Key %d not recognized!", ( int ) t->key );
            break;
        }
    
        t = dict_read_next( iterator );
    }
 
    text_layer_set_text( s_bus_station, bus_stop_name );
}

static void inbox_dropped_callback( AppMessageResult reason, void* context )
{
    APP_LOG( APP_LOG_LEVEL_ERROR, "Message dropped!" );
}

static void outbox_failed_callback( DictionaryIterator* iterator, AppMessageResult reason, void* context )
{
    APP_LOG( APP_LOG_LEVEL_ERROR, "Outbox send failed!" );
}

static void outbox_sent_callback( DictionaryIterator* iterator, void* context )
{
    APP_LOG( APP_LOG_LEVEL_INFO, "Outbox send successful." );
}


static void tick_handler( struct tm* tick_time, TimeUnits unites_changed )
{
    DictionaryIterator* iter;
    app_message_outbox_begin( &iter );
    
    dict_write_uint8( iter, 0, 0 );
    
    app_message_outbox_send();
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
    
    tick_timer_service_subscribe( MINUTE_UNIT, tick_handler );
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
