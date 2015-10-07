#include "common.h"

//==================================================================================================
//==================================================================================================
// Variables

static GenericCallback s_update_callback = NULL;

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
