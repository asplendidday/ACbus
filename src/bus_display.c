#include "bus_display.h"

//==================================================================================================
//==================================================================================================
// Definitions

// Layout information
#define NUM_PAGES                3
#define NUM_BUSES_PER_PAGE       6
#define NUM_BUSES                ( NUM_BUSES_PER_PAGE * NUM_PAGES )
    
#define BUS_ENTRY_MARGIN_TOP    27
#define BUS_ENTRY_MARGIN_LEFT    3
#define BUS_ENTRY_HEIGHT        23
#define BUS_ENTRY_LINE_WIDTH    26
#define BUS_ENTRY_DEST_WIDTH    94
#define BUS_ENTRY_ETA_WIDTH     18

// Bus data buffer sizes
#define LINE_BUFFER_SIZE         6
#define DEST_BUFFER_SIZE        32
#define ETA_BUFFER_SIZE          6


//==================================================================================================
//==================================================================================================
// Variables

static Window* s_bus_display_wnd = NULL;
static TextLayer* s_bus_display_title = NULL;
static TextLayer* s_bus_display_offline = NULL;
static BitmapLayer* s_bus_display_banner = NULL;
static const uint8_t s_line_colors[] = {
    // https://developer.pebble.com/guides/tools-and-resources/color-picker/
    GColorIslamicGreenARGB8,
    GColorMintGreenARGB8,
    GColorElectricBlueARGB8,
    GColorVividCeruleanARGB8,
    GColorChromeYellowARGB8,
    GColorSunsetOrangeARGB8,
    GColorVividVioletARGB8,
    GColorBrilliantRoseARGB8,
    GColorCadetBlueARGB8,
    GColorYellowARGB8
};

// Currently displayed page (0 to NUM_PAGES-1)
static int s_current_page = 0;

// Index of currently selected bus on current page (0 to NUM_BUSES_PER_PAGE-1)
static int s_current_bus = 0;
    
static char s_bus_stop_name[ DEST_BUFFER_SIZE ];
static int s_bus_stop_dist = 0; // in meters
static int s_num_buses_transmitted = 0;
    
static struct {
    TextLayer* line;
    TextLayer* dest;
    TextLayer* eta;
} s_bus_display_lines[ NUM_BUSES_PER_PAGE ];
    
static struct Bus {
    char line_string[ LINE_BUFFER_SIZE ];
    char dest_string[ DEST_BUFFER_SIZE ];
    char eta_string[ ETA_BUFFER_SIZE ];
} s_buses[ NUM_BUSES ];

// Seconds since bus data was loaded from the internet
static int s_sec_since_update = 0;

// false=bus list, true=zoom current bus
static bool s_zooming = false;

// Buffer for zoomed text
static char s_zoom_line_buf[ LINE_BUFFER_SIZE + DEST_BUFFER_SIZE ];

// Layers for zoomed text
static TextLayer* s_zoom_background_layer = NULL;
static TextLayer* s_zoom_eta_layer = NULL;
static TextLayer* s_zoom_line_layer = NULL;


//==================================================================================================
//==================================================================================================
// Various helper functions

static GRect line_rect( int index )
{
    return GRect( 0,
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
    // The zoom layers are initially hidden
    const int w = BUS_ENTRY_LINE_WIDTH + BUS_ENTRY_DEST_WIDTH + BUS_ENTRY_ETA_WIDTH;
    const int h = BUS_ENTRY_HEIGHT * ( NUM_BUSES_PER_PAGE - 1 );
    common_create_text_layer(
        &s_zoom_background_layer,
        s_bus_display_wnd,
        GRect(
            BUS_ENTRY_MARGIN_LEFT,
            BUS_ENTRY_MARGIN_TOP,
            w,
            h
        ),
        GColorWhite,
        GColorBlack,
        FONT_KEY_ROBOTO_BOLD_SUBSET_49,
        GTextAlignmentCenter
    );
    common_create_text_layer(
        &s_zoom_eta_layer,
        s_bus_display_wnd,
        GRect(
            BUS_ENTRY_MARGIN_LEFT,
            BUS_ENTRY_MARGIN_TOP + h / 4,
            w,
            h / 2
        ),
        GColorWhite,
        GColorBlack,
        FONT_KEY_ROBOTO_BOLD_SUBSET_49,
        GTextAlignmentCenter
    );
    common_create_text_layer(
        &s_zoom_line_layer,
        s_bus_display_wnd,
        GRect(
            BUS_ENTRY_MARGIN_LEFT,
            BUS_ENTRY_MARGIN_TOP + ( NUM_BUSES_PER_PAGE - 1 ) * BUS_ENTRY_HEIGHT,
            w,
            BUS_ENTRY_HEIGHT
        ),
        GColorWhite,
        GColorBlack,
        FONT_KEY_GOTHIC_18,
        GTextAlignmentCenter
    );
    layer_set_hidden( (Layer*) s_zoom_background_layer, true );
    layer_set_hidden( (Layer*) s_zoom_eta_layer, true );
    layer_set_hidden( (Layer*) s_zoom_line_layer, true );

    // Create the bus list layers (three per line)
    for( int i = 0; i < NUM_BUSES_PER_PAGE; ++i )
    {
        common_create_text_layer(
            &s_bus_display_lines[ i ].line,
            s_bus_display_wnd,
            line_rect( i ),
            GColorWhite,
            GColorBlack,
            FONT_KEY_GOTHIC_18_BOLD,
            GTextAlignmentCenter
        );
        common_create_text_layer(
            &s_bus_display_lines[ i ].dest,
            s_bus_display_wnd,
            dest_rect( i ),
            GColorWhite,
            GColorBlack,
            FONT_KEY_GOTHIC_18,
            GTextAlignmentLeft
        );
        common_create_text_layer(
            &s_bus_display_lines[ i ].eta,
            s_bus_display_wnd,
            eta_rect( i ),
            GColorWhite,
            GColorBlack,
            FONT_KEY_GOTHIC_18_BOLD,
            GTextAlignmentRight
        );
    }
}

static void destroy_bus_text_layers()
{
    text_layer_destroy( s_zoom_background_layer );
    text_layer_destroy( s_zoom_eta_layer );
    text_layer_destroy( s_zoom_line_layer );

    for( int i = 0; i < NUM_BUSES_PER_PAGE; ++i )
    {
        text_layer_destroy( s_bus_display_lines[ i ].line );
        text_layer_destroy( s_bus_display_lines[ i ].dest );
        text_layer_destroy( s_bus_display_lines[ i ].eta );
    }
}

static void set_bus_text_layer( int index, const char* line, GColor line_color, const char* dest, const char* eta )
{
    text_layer_set_text( s_bus_display_lines[ index ].line, line );
    text_layer_set_background_color( s_bus_display_lines[ index ].line, line_color );

    text_layer_set_text( s_bus_display_lines[ index ].dest, dest );
    text_layer_set_text_color( s_bus_display_lines[ index ].dest, index==s_current_bus ? GColorWhite : GColorBlack );
    text_layer_set_background_color( s_bus_display_lines[ index ].dest, index==s_current_bus ? GColorDarkCandyAppleRed : GColorWhite );

    text_layer_set_text( s_bus_display_lines[ index ].eta, eta );  
}

/**
 * Use the line name to get color
 */
static GColor get_line_color( const char* line )
{
    if( *line == '\0' )
    {
        return GColorWhite;    
    }
    
    int hash = 0;
    char c = line[ 0 ]+13;
    int i = 1;
    do
    {   
        hash += ( int ) c + 13;
        c = line[ i ];
        i++;
        
    } while( c != '\0' );
  
    while( hash > 9 )
    {
        hash = ( hash % 10 ) + ( (hash/10) % 10 ) + ( (hash/100) % 10 );
    }

    return (GColor8){ .argb=s_line_colors[ hash ] };
}


static void update_bus_text_layers()
{
    for( int i = 0; i < NUM_BUSES_PER_PAGE; ++i )
    {
        const struct Bus *const bus = s_buses + ( NUM_BUSES_PER_PAGE * s_current_page + i );

        set_bus_text_layer( i,
            bus->line_string,
            get_line_color( bus->line_string ),
            bus->dest_string,
            bus->eta_string );
    }

    // Make texts for zoom mode
    const struct Bus *const bus = s_buses + ( s_current_bus + NUM_BUSES_PER_PAGE * s_current_page );
    strcpy( s_zoom_line_buf, bus->line_string);
    strcat( s_zoom_line_buf, " ");
    strcat( s_zoom_line_buf, bus->dest_string);
    text_layer_set_text( s_zoom_eta_layer, bus->eta_string );
    text_layer_set_text( s_zoom_line_layer, s_zoom_line_buf );

    // Average walking speed is 4 km/h (faster when walking steadily but
    // slower when crossing streets etc.) That's 1 km in 15 minutes, or
    // 100 meters in 90 seconds. So, reaching a bus stop that's m meters
    // away takes (m/100)*1.5 minutes. This, plus one minute since that's
    // the precision we're working at, is the ETA below which we show the
    // counter in red. Add 50 for 5/4 rounding.
    const int limit = 1 + ( ( s_bus_stop_dist + 50 ) / 100 ) * 3 / 2;
    const bool red = atoi( bus->eta_string ) <= limit;
    text_layer_set_text_color( s_zoom_background_layer,         red ? GColorYellow : GColorBlack );
    text_layer_set_text_color( s_zoom_eta_layer,                red ? GColorYellow : GColorBlack );
    text_layer_set_background_color( s_zoom_background_layer,   red ? GColorDarkCandyAppleRed : GColorWhite );
    text_layer_set_background_color( s_zoom_eta_layer,          red ? GColorDarkCandyAppleRed : GColorWhite );
}

static void switch_zoom_mode( bool zoom )
{
    s_zooming = zoom;
    APP_LOG( APP_LOG_LEVEL_INFO, "[ACbus] Display mode switched to %s", s_zooming ? "zoom" : "list" );

    layer_set_hidden( (Layer*) s_zoom_background_layer, ! s_zooming );
    layer_set_hidden( (Layer*) s_zoom_eta_layer,        ! s_zooming );
    layer_set_hidden( (Layer*) s_zoom_line_layer,       ! s_zooming );

    for( int i = 0; i < NUM_BUSES_PER_PAGE; ++i )
    {
        layer_set_hidden( (Layer*) s_bus_display_lines[ i ].line, s_zooming );
        layer_set_hidden( (Layer*) s_bus_display_lines[ i ].dest, s_zooming );
        layer_set_hidden( (Layer*) s_bus_display_lines[ i ].eta,  s_zooming );
    }
}


//==================================================================================================
//==================================================================================================
// Message parsing

static void parse_first_bus_stop( const char* bus_stop_data )
{
	const int curr = common_get_current_bus_stop_id();
	while (bus_stop_data && *bus_stop_data)
	{
		char dist[8],id[8];
		bus_stop_data = common_read_csv_item( bus_stop_data, s_bus_stop_name, sizeof(s_bus_stop_name) );
		bus_stop_data = common_read_csv_item( bus_stop_data, dist, sizeof(dist) );
		bus_stop_data = common_read_csv_item( bus_stop_data, id, sizeof(id) );

        if (curr<0 || atoi(id)==curr)
        {
            s_bus_stop_dist = atoi( dist );
            text_layer_set_text( s_bus_display_title, s_bus_stop_name );
			break;
        }
	}
}

/**
 * This function takes the string as provided by a BUS_DATA app message, parses it,
 * and uses the parsed data to update all bus text layers. It tries to keep the
 * cursor on the same bus even if it is no longer in the same position or has
 * changed its ETA.
 */
static void parse_bus_data( const char* bus_data )
{   
    // Save the location the cursor currently is on
    char cur_line[ LINE_BUFFER_SIZE ];
    char cur_dest[ DEST_BUFFER_SIZE ];
    int cur_eta;
    const int cur_idx = s_current_bus + NUM_BUSES_PER_PAGE * s_current_page;
    if( cur_idx < s_num_buses_transmitted )
    {
        strcpy( cur_line, s_buses[ cur_idx ].line_string );
        strcpy( cur_dest, s_buses[ cur_idx ].dest_string );
        cur_eta = atoi(s_buses[ cur_idx ].eta_string);
    }
    else
    {
        // WTF
        *cur_line = *cur_dest = '\0';
        cur_eta = 0;
    }

    // Initialize bus array
    memset( s_buses, 0, sizeof( s_buses ) );

    // Find out how many buses we received
    if( *bus_data )
    {
        char num_buses_string[ 8 ];
        bus_data = common_read_csv_item( bus_data, num_buses_string, sizeof( num_buses_string ) );
        s_num_buses_transmitted = atoi( num_buses_string );
        // Ok by what do we need it for in bus_data? Could just as well count
        // the buses we scan in the following loop ...
    }
    else
    {
        s_num_buses_transmitted = 0;
    }

    // Copy new buses into display buffers, and find the index of the best
    // match for the bus the cursor is on. best_delta is the difference
    // between the cursor position's and the best match's ETA.
    int best_idx = -1;
    int best_delta = 0;
    for( int i = 0; i < NUM_BUSES; ++i )
    {
        if( *bus_data )
        {
            // Where we put this bus' data
            char *const line = s_buses[ i ].line_string;
            char *const dest = s_buses[ i ].dest_string;
            char *const etaS = s_buses[ i ].eta_string;

            // Scan this bus' data into the display buffers
            bus_data = common_read_csv_item( bus_data, line, LINE_BUFFER_SIZE );
            bus_data = common_read_csv_item( bus_data, dest, DEST_BUFFER_SIZE );
            bus_data = common_read_csv_item( bus_data, etaS, ETA_BUFFER_SIZE );

            // Check best match for cursor position
            if ( ! strcmp( dest, cur_dest ) )
            {
                // It's the same destination, check ETA and add a penalty if
                // it's not the same line (so that, if there is no bus with
                // the same line and destination, we'll pick the bus with the
                // same destination and different line, if there is one).
                // Note that ETA is always < 100.
                const int delta
                    = abs( cur_eta - atoi( etaS ) )
                    + strcmp( line, cur_line ) ? 100 : 0;
                if ( best_idx < 0 || delta < best_delta )
                {
                    // Found a new best match
                    best_delta = delta;
                    best_idx = i;
                }
            }
        }
        else
        {
            // Reached the end
            if ( i==0 )
            {
                // ... without finding anything
                strcpy( s_buses->dest_string, "Keine Fahrt" );
                switch_zoom_mode(false);
            }
            break;
        }
    }

    // Reconstruct cursor position
    if( best_idx < 0 )
    {
        // No match found, reset to beginning, but disable zoom mode
        // because the bus we've been zooming into is obviously no
        // longer available
        s_current_page = s_current_bus = 0;
        switch_zoom_mode(false);
    }
    else
    {
        // Go to best match
        s_current_page = best_idx / NUM_BUSES_PER_PAGE;
        s_current_bus  = best_idx % NUM_BUSES_PER_PAGE;
    }
    
    // Update the display
    update_bus_text_layers();
}


//==================================================================================================
//==================================================================================================
// Button click handling

static void bus_display_up( ClickRecognizerRef recognizer, void* context )
{
    if( s_current_bus > 0 )
    {
        // Move cursor up on this page
        --s_current_bus;
    }
    else if( s_current_page > 0 )
    {
        // Cursor is at top of page, move to bottom of previous page
        s_current_bus = NUM_BUSES_PER_PAGE - 1;
        --s_current_page;
    }

    update_bus_text_layers();
}

static void bus_display_down( ClickRecognizerRef recognizer, void* context )
{
    const int num_buses = min( NUM_BUSES, s_num_buses_transmitted );
    const int num_pages = ( num_buses / NUM_BUSES_PER_PAGE ) +
                    ( num_buses % NUM_BUSES_PER_PAGE != 0 ? 1 : 0 );

    if( s_current_bus < NUM_BUSES_PER_PAGE - 1 )
    {
        // Move cursor one bus down, unless we are at the bottom of
        // the last page
        if ( (s_current_bus + 1) + (s_current_page * NUM_BUSES_PER_PAGE) < num_buses )
        {
            ++s_current_bus;
        }
    } else if( s_current_page + 1 < num_pages )
    {
        // Cursor is at bottom of page, move to top of next page
        s_current_bus = 0;
        ++s_current_page;
    }

    update_bus_text_layers();
}

static void bus_display_select( ClickRecognizerRef recognizer, void* context )
{
    // No zoom mode if there is nothing to zoom in on
    switch_zoom_mode( s_num_buses_transmitted==0 ? false : ! s_zooming );
}

static void bus_display_back( ClickRecognizerRef recognizer, void* context )
{
    // If in zoom mode, return to list mode, else return to bus stop selection
    if( s_zooming )
    {
        bus_display_select( recognizer, context );
    }
    else
    {
        window_stack_pop( true );
    }
}

static void click_provider( Window* window )
{
    const int ms = 200;     // Auto repeat time in milliseconds
    window_single_repeating_click_subscribe( BUTTON_ID_UP, ms, bus_display_up );
    window_single_repeating_click_subscribe( BUTTON_ID_DOWN, ms, bus_display_down );

    window_single_click_subscribe( BUTTON_ID_SELECT, bus_display_select );
    window_single_click_subscribe( BUTTON_ID_BACK, bus_display_back );
}


//==================================================================================================
//==================================================================================================
// Window (un)loading

static void bus_display_window_load()
{
    common_create_text_layer(
        &s_bus_display_title,
        s_bus_display_wnd,
        GRect( 24, 0, 120, 20 ),
        GColorDarkCandyAppleRed,
        GColorWhite,
        FONT_KEY_GOTHIC_18_BOLD,
        GTextAlignmentLeft
    );

   common_create_text_layer(
        &s_bus_display_offline,
        s_bus_display_wnd,
        GRect( 24, 0, 120, BUS_ENTRY_MARGIN_TOP - 2),
        GColorYellow,
        GColorDarkCandyAppleRed,
        FONT_KEY_GOTHIC_18_BOLD,
        GTextAlignmentCenter
    );
    layer_set_hidden( (Layer*)s_bus_display_offline, true );
    text_layer_set_text( s_bus_display_offline, "O F F L I N E" );

    common_create_h_icon( &s_bus_display_banner, s_bus_display_wnd );
    
    create_bus_text_layers(); 
}

static void bus_display_window_unload()
{
    destroy_bus_text_layers();
    bitmap_layer_destroy( s_bus_display_banner );
    text_layer_destroy( s_bus_display_offline );
    text_layer_destroy( s_bus_display_title );    
}


//==================================================================================================
//==================================================================================================
// Interface functions

void bus_display_create()
{
    s_bus_display_wnd = window_create();
    
	window_set_window_handlers( s_bus_display_wnd, ( WindowHandlers )
        {
            .load = bus_display_window_load,
            .unload = bus_display_window_unload
        } );   
       
    window_set_click_config_provider( s_bus_display_wnd, ( ClickConfigProvider ) click_provider );  
}

void bus_display_destroy()
{
	window_destroy( s_bus_display_wnd );
}


void bus_display_show()
{
    window_stack_push( s_bus_display_wnd, true );   
}


void bus_display_handle_msg_tuple( Tuple* msg_tuple )
{
    if ( ! window_stack_contains_window( s_bus_display_wnd ) ) return;

    switch( msg_tuple->key )
    {
        case BUS_STOP_DATA:
        {
            parse_first_bus_stop( msg_tuple->value->cstring );
        }            
        break;
        case BUS_DATA:
        {
            s_sec_since_update = 0;
            parse_bus_data( msg_tuple->value->cstring );
        }
        break;
        default:
        // intentionally left blank
        break;
    }
}

void bus_display_estimate_eta( int sec_since_update )
{
    s_sec_since_update = sec_since_update;

    if ( window_stack_contains_window( s_bus_display_wnd ) )
    {
        update_bus_text_layers();
    }
}

/*
 * status_text==NULL if we are online, else offline message.
 */
void bus_display_set_update_status( bool offline )
{
    if( s_bus_display_offline )
    {
        layer_set_hidden( (Layer*)s_bus_display_offline, ! offline );
    }
}
