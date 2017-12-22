//==================================================================================================
//==================================================================================================
// Variables

var query_url_base = 'http://ivu.aseag.de/interfaces/ura/instant_V1?ReturnList=';
var query_url_bus = query_url_base + 'StopPointName,LineName,DestinationName,EstimatedTime&StopID=';
var query_url_stops = query_url_base + 'StopPointName,StopID,Longitude,Latitude';


//==================================================================================================
//==================================================================================================
// Helper functions

function degToRad( angleInDeg ) {
    return angleInDeg / 180.0 * Math.PI;
}

function removeQuotes( string ) {
    return string.slice( 1, string.length - 1);
}

function cleanUpBusStopName( bus_stop_name ) {
    return removeQuotes( bus_stop_name );
}


var xhrRequest = function( url, type, callback ) {
    console.log( '[ACbus] Sending http request to URL <' + url + '>.' );
    
    var xhr = new XMLHttpRequest();
    xhr.onload = function() {
        callback( this.responseText );
    };
    xhr.open( type, url );
    xhr.send( null );
};

function distanceBetweenGPSCoords( lon1, lat1, lon2, lat2 ) {
    var R = 6371000; // earth mean radius
    var phi1 = degToRad( lat1 );
    var phi2 = degToRad( lat2 );
    var deltaPhi = degToRad( lat2 - lat1 );
    var deltaLambda = degToRad( lon2 - lon1 );

    var a = Math.sin( deltaPhi / 2.0 ) * Math.sin( deltaPhi / 2.0 ) +
            Math.cos( phi1 ) * Math.cos( phi2 ) *
            Math.sin( deltaLambda / 2.0 ) * Math.sin( deltaLambda / 2.0 );
    var c = 2 * Math.atan2( Math.sqrt( a ), Math.sqrt( 1 - a ) );

    var dist = R * c;

    return dist; // in meters
}

function parseLines( lines ) {
    return lines.split(/\r?\n/);
}

function parseLine( line ) {
    var parsed = line.slice( 1, line.length - 1 );
    return parsed.split( ',' );
}


//==================================================================================================
//==================================================================================================
// Bus data parsing and handling

function parseBusStops( response_text ) {
    console.log( '[ACbus] Parsing bus stops.' );

    var lines = parseLines( response_text );
    var bus_stops = [];

    // first item is not a bus stop
    for( var i = 1; i < lines.length; ++i )
    {
        var parsed_line = parseLine( lines[ i ] );

        // compare this to the query url for bus stops and its return list params
        var bus_stop = {
            name: cleanUpBusStopName( parsed_line[ 1 ] ),
            id:   removeQuotes( parsed_line[ 2 ] ),
            lon:  parsed_line[ 4 ],
            lat:  parsed_line[ 3 ],
            dist: Infinity // will be updated later
        };

        // There are lots of bus stops at coord (0, 0). Maybe that is deprecated data.
        // We will filter it out here.
        if( bus_stop.lon > 0.1 && bus_stop.lat > 0.1 ) {
            bus_stops.push( bus_stop );
        }
    }

    console.log( '[ACbus] Parsed ' + bus_stops.length + ' bus stops.' );
    return bus_stops;
}

function updateBusStopDistances( gps_coords, bus_stops ) {
    for( var i = 0; i < bus_stops.length; ++i ) {
    bus_stops[ i ].dist = distanceBetweenGPSCoords( bus_stops[ i ].lon,
                                                    bus_stops[ i ].lat,
                                                    gps_coords.longitude,
                                                    gps_coords.latitude );
    }
    
    return bus_stops;
}

function compileListOfClosestBusStops( bus_stops, num_closest_bus_stops ) {
    // sort bus stops with respect to distance to current position
    bus_stops.sort( function( lhs, rhs ) {
        return lhs.dist - rhs.dist;
    } );
    
    var num_bus_stops = Math.min( num_closest_bus_stops, bus_stops.length );
    var bus_stop_data = "";
    
    for( var j = 0; j < num_bus_stops; ++j ) {
        bus_stop_data += bus_stops[ j ].name + ';' +
                         Math.round( bus_stops[ j ].dist ) + ';' +
                         bus_stops[ j ].id;
        if( j + 1 < num_bus_stops ) {
            bus_stop_data += ";";
        }
    }
    
    console.log( '[ACbus] Compiled list of closest ' + num_bus_stops + ' bus stops: ' + bus_stop_data ); 
    return bus_stop_data;
}


function parseBuses( response_text ) {
    console.log( '[ACbus] Parsing buses.' );
    
    var bus_lines = parseLines( response_text );
    var buses = [];

    var global_now = parseLine( bus_lines[ 0 ] )[ 2 ];

    for( var i = 1; i < bus_lines.length; ++i ) {
        var bus_line = parseLine( bus_lines[ i ] );

        var bus = {
            number: removeQuotes( bus_line[ 2 ] ),
            dest:   cleanUpBusStopName( bus_line[ 3 ] ),
            eta:    Math.max( bus_line[ 4 ] - global_now, 0 )
        };

        buses.push( bus );
    }
    
    console.log( '[ACbus] Parsed ' + buses.length + ' buses.' );
    return buses;
}

function compileListOfNextBuses( buses, num_next_buses ) {
    // order list with respect to estimated time of arrival
    buses.sort( function( lhs, rhs ) {
        return lhs.eta - rhs.eta;
    } );
    
    var max_buses = Math.min( num_next_buses, buses.length );
    var num_buses = 0;
    var bus_data = "";

    for( var j = 0; j < max_buses; ++j ) {
        var eta = Math.round( buses[ j ].eta / ( 1000 * 60 ) );
        if (eta > 99) break;

        ++num_buses;
        bus_data += buses[ j ].number + ';' +
                    buses[ j ].dest + ';' +
                    eta;
        if( j + 1 < max_buses ) {
            bus_data += ";";
        }
    }
    
    bus_data = num_buses + ';' + bus_data;
    
    console.log( '[ACbus] Compiled list of next ' + num_buses + ' buses: ' + bus_data ); 
    return bus_data;
}


//==================================================================================================
//==================================================================================================
// Data update functions

function sendUpdate( bus_stop_data, bus_data ) {
     var dict = {
        'BUS_STOP_DATA': bus_stop_data,
        'BUS_DATA': bus_data
    };
    
    console.log( '[ACbus] Sending update.' );
    Pebble.sendAppMessage( dict );
    console.log( '[ACbus] Sent update.' );
}

function findClosestBusStopForCoords( coords, requested_bus_stop_id ) {       
    xhrRequest( query_url_stops, 'GET', function( response_text ) {
        var bus_stops = parseBusStops( response_text );
        bus_stops = updateBusStopDistances( coords, bus_stops );
        var bus_stop_data = compileListOfClosestBusStops( bus_stops, 6 );
   
        // closest bus stop is default
        var selected_bus_stop_id = bus_stops[ 0 ].id;
        var selected_bus_stop_name = bus_stops[ 0 ].name;
   
        // if another one was requested, we update the data structure
        if( requested_bus_stop_id != -1 )
        {   
            var requested_bus_stop_name = "";
            var requested_bus_stop_distance = 0.0;
            
            for( var i = 0; i != bus_stops.length; ++i ) {
                if( bus_stops[ i ].id == requested_bus_stop_id ) {
                    requested_bus_stop_name = bus_stops[ i ].name;
                    requested_bus_stop_distance = bus_stops[ i ].dist;
                }
            }
            
            selected_bus_stop_name = requested_bus_stop_name;
            selected_bus_stop_id = requested_bus_stop_id;
        }
   
        xhrRequest( query_url_bus + selected_bus_stop_id, 'GET', function( response_text ) {
            console.log( '[ACbus] Getting next buses for ' + selected_bus_stop_name + '.' );

            var buses = parseBuses( response_text );
            var bus_data = compileListOfNextBuses( buses, 21 );            
            
            sendUpdate( bus_stop_data, bus_data );
        } );
    } );
}


//==================================================================================================
//==================================================================================================
// GPS coord query

function determineClosestBusStop( requested_bus_stop_id ) {
    console.log( '[ACbus] ######## Initiated new bus stop update.' );
    console.log( '[ACbus] Querying current GPS coordinates.' );
    
    navigator.geolocation.getCurrentPosition(
        // success
        function( pos ) {
            console.log( '[ACbus] GPS request succeeded.' );
            var gps_coords = pos.coords;
            // Debug info for Aachen Bushof
            //var gps_coords = { longitude: 6.0908191, latitude: 50.7775936 };
            
            console.log( '[ACbus] Received new gps coords at ' +
                         '(lon: ' + gps_coords.longitude + ', lat: ' + gps_coords.latitude + ').'  );
            findClosestBusStopForCoords( gps_coords, requested_bus_stop_id );
        },
        // failure
        function( err ) {
            console.log( '[ACbus] An error occured while getting new location data. Error: ' + err );
        },
        // geoloc request params    
        { timeout: 10000, maximumAge: 10000 }
    );
}

//==================================================================================================
//==================================================================================================
// Pebble JS setup

Pebble.addEventListener( 'ready',
    function( e ) {
        console.log( '[ACbus] Pebble JS ready!' );
    } );

Pebble.addEventListener( 'appmessage',
    function( e ) {
        console.log( '[ACbus] AppMessage received!' ) ;

        var stringified = JSON.stringify( e.payload );
        var request = JSON.parse( stringified );
        console.log( '[ACbus] payload:', JSON.stringify( e.payload ) );
                
        var requested_bus_stop_id = request.REQ_BUS_STOP_ID;
        var update_bus_stop_list = request.REQ_UPDATE_BUS_STOP_LIST;
        
        console.log( '[ACbus] Request received with REQ_BUS_STOP_ID <' + requested_bus_stop_id +
                     '> and REQ_UPDATE_BUS_STOP_LIST <' + update_bus_stop_list + '>.' );
        
        determineClosestBusStop( requested_bus_stop_id );
    } );
