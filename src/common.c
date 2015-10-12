#include "common.h"

//==================================================================================================
//==================================================================================================
// Variables

static GenericCallback s_update_callback = NULL;
static int s_current_bus_stop_id = -1;


//==================================================================================================
//==================================================================================================
// Helper functions

void update_proc( Layer* layer, GContext* context )
{
    // @TODO reusing this function for every h_icon is a dirty thing to do, since it must be
    //       ensured that the draw calls below fit every window that has an h_icon 
    
    graphics_context_set_fill_color( context, GColorDarkCandyAppleRed );
    graphics_fill_rect( context, GRect( 0, 0, 144, 25 ), 0, GCornerNone );
    
    graphics_context_set_fill_color( context, GColorWhite );
    graphics_fill_rect( context, GRect( 0, 25, 144, 143 ), 0, GCornerNone );
}


//==================================================================================================
//==================================================================================================
// Interface functions

void common_set_update_callback( GenericCallback callback )
{
	s_update_callback = callback;
}

GenericCallback common_get_update_callback()
{
	return s_update_callback;
}


void common_create_text_layer( TextLayer** text_layer, Window* window, GRect rect, GColor back_color, GColor text_color, const char* font_name, GTextAlignment text_align )
{
    *text_layer = text_layer_create( rect );
    
    text_layer_set_background_color( *text_layer, back_color );
    text_layer_set_text_color( *text_layer, text_color );
    text_layer_set_font( *text_layer, fonts_get_system_font( font_name ) );
    text_layer_set_text_alignment( *text_layer, text_align );
    text_layer_set_text( *text_layer, "" );
    
    layer_add_child( window_get_root_layer( window ), text_layer_get_layer( *text_layer ) ); 
}


void common_create_h_icon( BitmapLayer** bitmap_layer, Window* window )
{
    GBitmap* h_icon = gbitmap_create_with_resource( RESOURCE_ID_ICON_H );
    *bitmap_layer = bitmap_layer_create( GRect( 3, 3, 18, 18 ) );
    bitmap_layer_set_compositing_mode( *bitmap_layer, GCompOpSet );
    bitmap_layer_set_background_color( *bitmap_layer, GColorClear );
    bitmap_layer_set_bitmap( *bitmap_layer, h_icon );
  
    layer_add_child( window_get_root_layer( window ), bitmap_layer_get_layer( *bitmap_layer ) );
    layer_set_update_proc( window_get_root_layer ( window ), update_proc );    
}


void common_set_current_bus_stop_id( int id )
{
    s_current_bus_stop_id = id;
}

int common_get_current_bus_stop_id()
{
    return s_current_bus_stop_id;
}


const char* common_find_next_separator( const char* cursor, const char separator )
{
    while( *cursor != separator && *cursor != '\0' )
    {
        ++cursor;
    }
    return cursor;    
}

const char* common_read_csv_item( const char* csv_data, char* target, int max_bytes )
{
    const char* end_cursor = common_find_next_separator( csv_data, ';' );
    
    const int num_bytes = min( end_cursor - csv_data, max_bytes - 1 );
    // use "max_bytes - 1" as comparison to save a byte for the trailing '\0' char
    
    // if num_bytes is 0 then target will be set to the empty string
    // in this case, memcpy will essentially be a noop
    memcpy( target, csv_data, num_bytes );
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


const char* common_app_message_result_to_string( AppMessageResult result )
{
    switch( result )
    {
        case APP_MSG_OK:                return "APP_MSG_OK";                break;
        case APP_MSG_SEND_TIMEOUT:      return "APP_MSG_SEND_TIMEOUT";      break;
        case APP_MSG_SEND_REJECTED:     return "APP_MSG_SEND_REJECTED";     break;
        case APP_MSG_NOT_CONNECTED:     return "APP_MSG_NOT_CONNECTED";     break;
        case APP_MSG_APP_NOT_RUNNING:   return "APP_MSG_APP_NOT_RUNNING";   break;
        case APP_MSG_INVALID_ARGS:      return "APP_MSG_INVALID_ARGS";      break;
        case APP_MSG_BUSY:              return "APP_MSG_BUSY";              break;
        case APP_MSG_BUFFER_OVERFLOW:   return "APP_MSG_BUFFER_OVERFLOW";   break;
        case APP_MSG_ALREADY_RELEASED:  return "APP_MSG_ALREADY_RELEASED";  break;
        case APP_MSG_CALLBACK_ALREADY_REGISTERED: return "APP_MSG_CALLBACK_ALREADY_REGISTERED"; break;
        case APP_MSG_CALLBACK_NOT_REGISTERED: return "APP_MSG_CALLBACK_NOT_REGISTERED"; break;
        case APP_MSG_OUT_OF_MEMORY:     return "APP_MSG_OUT_OF_MEMORY";     break;
        case APP_MSG_CLOSED:            return "APP_MSG_CLOSED";            break;
        case APP_MSG_INTERNAL_ERROR:    return "APP_MSG_INTERNAL_ERROR";    break;
        default: return "Unknown result id."; break;
    }
    
    return "";
}
