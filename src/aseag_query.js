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

function killUmlauts( string ) {
    return string.replace( 'ß', 'ss' ).replace( 'ö', 'oe' ).replace( 'ä', 'ae' ).replace( 'ü', 'ue' );
}

function removeQuotes( string ) {
    return string.slice( 1, string.length - 1);
}

function cleanUpBusStopName( bus_stop_name ) {
    bus_stop_name = removeQuotes( bus_stop_name );
    bus_stop_name = killUmlauts( bus_stop_name );
    return bus_stop_name;
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
// Bus stop parsing

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
            id:   parsed_line[ 2 ],
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


//==================================================================================================
//==================================================================================================
// Data update functions

function findClosestBusStopForCoords( coords, requested_bus_stop_id ) {
    xhrRequest( query_url_stops, 'GET', function( response_text ) {
        var bus_stops = parseBusStops( response_text );

        for( var i = 0; i < bus_stops.length; ++i ) {
            bus_stops[ i ].dist = distanceBetweenGPSCoords( bus_stops[ i ].lon,
                                                            bus_stops[ i ].lat,
                                                            coords.longitude,
                                                            coords.latitude );
        }

        bus_stops.sort( function( lhs, rhs ) {
            return lhs.dist - rhs.dist;
        } );

        // Debug settings        
        bus_stops[ 0 ].id = '100000'; // for debugging in emulator set to 'Aachen Bushof'
        bus_stops[ 0 ].name = 'Aachen Bushof';

        var num_bus_stops = Math.min( 10, bus_stops.length );
        var bus_stop_data = "";
        
        for( var j = 0; j < num_bus_stops; ++j ) {
            bus_stop_data += bus_stops[ j ].name + ';' +
                             ( Math.round( bus_stops[ j ].dist / 100 ) / 10 );
            if( j + 1 < num_bus_stops ) {
                bus_stop_data += ";";
            }
        }
   
        xhrRequest( query_url_bus + bus_stops[ 0 ].id, 'GET', function( response_text ) {
            console.log( '[ACbus] Getting next buses for ' + bus_stops[ 0 ].name + '.' );

            var bus_lines = parseLines( response_text );
            var buses = [];

            console.log( '[ACbus] Found ' + ( bus_lines.length - 1 ) + " buses." );

            var global_now = parseLine( bus_lines[ 0 ] )[ 2 ];

            for( var i = 1; i < bus_lines.length; ++i ) {
                var bus_line = parseLine( bus_lines[ i ] );

                var bus = {
                    number: removeQuotes( bus_line[ 2 ] ),
                    dest:   cleanUpBusStopName( bus_line[ 3 ] ),
                    eta:    bus_line[ 4 ] - global_now
                };

                buses.push( bus );
            }

            buses.sort( function( lhs, rhs ) {
                return lhs.eta - rhs.eta;
            } );

            console.log( '[ACbus] Assembling update data.' );
            
            var num_buses = Math.min( 21, buses.length );
            var bus_data = "";

            for( var j = 0; j < num_buses; ++j ) {
                bus_data += buses[ j ].number + ';' +
                            buses[ j ].dest + ';' +
                            Math.round( buses[ j ].eta / ( 1000 * 60 ) );
                if( j + 1 < num_buses ) {
                    bus_data += ";";
                }
            }

            var dict = {
                'BUS_STOP_DATA': bus_stop_data,
                'BUS_DATA': bus_data
            };
            
            console.log( '[ACbus] Bus data:\n' + bus_stop_data );
            console.log( '[ACbus] Bus stop data:\n' + bus_data );            
            
            console.log( '[ACbus] Sending update.' );

            Pebble.sendAppMessage( dict );

            console.log( '[ACbus] Sent update.' );
        } );
    } );
}


//==================================================================================================
//==================================================================================================
// GPS coord query

function determineClosestBusStop( requested_bus_stop_id ) {
    console.log( '[ACbus] Determining closest bus stop.' );
    console.log( '[ACbus] Querying current GPS coordinates.' );
    
    navigator.geolocation.getCurrentPosition(
        // success
        function( pos ) {
            console.log( '[ACbus] Got new location data. Determining closest bus stop.' );
            findClosestBusStopForCoords( pos.coords, requested_bus_stop_id );
        },
        // failure
        function( err ) {
            console.log( '[ACbus] An error occured while getting new location data. Error: ' + err );
        },
        // geoloc request params    
        { timeout: 15000, maximumAge: 60000 }
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

        var request = JSON.parse( JSON.stringify( e.payload ) );
        console.log( JSON.stringify( e.payload ) );
        
        var requested_bus_stop_id = request[ 'REQ_BUS_STOP_ID' ];
        var update_bus_stop_list = request[ 'REQ_UPDATE_BUS_STOP_LIST' ];
        
        console.log( '[ACbus] Request received with REQ_BUS_STOP_ID <' + requested_bus_stop_id +
                     '> and REQ_UPDATE_BUS_STOP_LIST <' + update_bus_stop_list + '>.' );
        
        determineClosestBusStop( requested_bus_stop_id );
    } );
