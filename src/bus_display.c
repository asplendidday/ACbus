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
static int s_num_buses_transmitted = 0;
    
static struct {
    TextLayer* line;
    TextLayer* dest;
    TextLayer* eta;
} s_bus_display_lines[ NUM_BUSES_PER_PAGE ];
    
static struct {
    char line_string[ LINE_BUFFER_SIZE ];
    char dest_string[ DEST_BUFFER_SIZE ];
    char eta_string[ ETA_BUFFER_SIZE ];
} s_buses[ NUM_BUSES ];

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
        const int base_index = NUM_BUSES_PER_PAGE * s_current_page;
        const int bus_index = base_index + i;

        set_bus_text_layer( i,
            s_buses[ bus_index ].line_string,
            get_line_color( s_buses[ bus_index ].line_string ),
            s_buses[ bus_index ].dest_string,
            s_buses[ bus_index ].eta_string );
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
			text_layer_set_text( s_bus_display_title, s_bus_stop_name );
			break;
		}
	}
}

/**
 * This function takes the string as provided by a BUS_DATA app message, parses it,
 * and uses the parsed data to update all bus text layers.
 */
static void parse_bus_data( const char* bus_data )
{   
    memset( s_buses, 0, sizeof( s_buses ) );

    if( *bus_data )
    {
        char num_buses_string[ 8 ];
        bus_data = common_read_csv_item( bus_data, num_buses_string, sizeof( num_buses_string ) );
        s_num_buses_transmitted = atoi( num_buses_string );
    }
    else
    {
        s_num_buses_transmitted = 0;
    }
    
    for( int i = 0; i < NUM_BUSES; ++i )
    {
        if( *bus_data != '\0' ) // eof reached?
        {
            // read line
            bus_data = common_read_csv_item( bus_data, s_buses[ i ].line_string, LINE_BUFFER_SIZE );
            // read destination
            bus_data = common_read_csv_item( bus_data, s_buses[ i ].dest_string, DEST_BUFFER_SIZE );
            // read eta
            bus_data = common_read_csv_item( bus_data, s_buses[ i ].eta_string, ETA_BUFFER_SIZE );
        }
        else
        {
            if ( i==0 )
                strcpy( s_buses->dest_string,"Keine Fahrt");
            break;
        }
    }
    
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
    APP_LOG( APP_LOG_LEVEL_INFO, "SELECT in bus display");
}

static void click_provider( Window* window )
{
    const int ms = 200;     // Auto repeat time in milliseconds
    window_single_repeating_click_subscribe( BUTTON_ID_UP, ms, bus_display_up );
    window_single_repeating_click_subscribe( BUTTON_ID_DOWN, ms, bus_display_down );

    window_single_click_subscribe( BUTTON_ID_SELECT, bus_display_select );
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
 
    common_create_h_icon( &s_bus_display_banner, s_bus_display_wnd );
    
    create_bus_text_layers(); 
}

static void bus_display_window_unload()
{
    destroy_bus_text_layers();
    bitmap_layer_destroy( s_bus_display_banner );
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
            s_current_page = s_current_bus = 0; // reset cursor if new data arrives
            parse_bus_data( msg_tuple->value->cstring );
        }
        break;
        default:
        // intentionally left blank
        break;
    }
}
