var item_list = 'StopPointName,LineName,DestinationName,EstimatedTime';
var query_url_base = 'http://ivu.aseag.de/interfaces/ura/instant_V1?ReturnList=';
var query_url = query_url_base + item_list + '&StopID=100016';
var query_url_stops = query_url_base + 'StopPointName,StopID,Longitude,Latitude';

var xhrRequest = function( url, type, callback ) {
    var xhr = new XMLHttpRequest();
    xhr.onload = function() {
        callback( this.responseText );
    };
    xhr.open( type, url );
    xhr.send();
};

function getBusData() {
    xhrRequest( query_url, 'GET',
        function( response_text ) {
            console.log( 'Received new bus data.' );
            
            var result = response_text.split(/\r?\n/);
            var now0 = result[0].split(',')[2];
            
            
            var line = result[4].split(',');
            var stop = line[1];
            var bus = line[2];
            var dir = line[3];
            var time = new Date(parseInt(line[4].slice(0, line[4].length -1)));
            var now = new Date().getTime();
            var eta = Math.floor((time-now)/(1000*60));
            
            console.log( stop + "\n" + bus + " to " + dir + " in " + eta + " min." );
        } );
}

function parseLines( lines ) {
    return lines.split(/\r?\n/);
}

function parseLine( line ) {
    var parsed = line.slice( 1, line.length - 1 );
    return parsed.split( ',' );
}

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
            name: parsed_line[ 1 ],
            id:   parsed_line[ 2 ],
            lon:  parsed_line[ 3 ],
            lat:  parsed_line[ 4 ]
        };
        
        bus_stops.push( bus_stop );
    }
    
    console.log( '[ACbus] Parsed ' + bus_stops.length + ' bus stops.' );
    return bus_stops;
}

function updateBusStops() {
    xhrRequest( query_url_stops, 'GET', parseBusStops );
}

function findClosestBusStopForCoords( coords ) {
    xhrRequest( query_url_stops, 'GET', function( response_text ) {
        bus_stops = parseBusStops( response_text );
        
        // add code to determine closest bus station here
    } );
}

function locationSuccess( pos ) {
    console.log( '[ACbus] Got new location data. Determining closest bus stop.' );
    findClosestBusStopForCoords( pos.coords );
}
 
function locationFailure( err ) {
    console.log( '[ACbus] An error occured while getting new location data. Error: ' + err );
}

function determineClosestBusStop() {
    navigator.geolocation.getCurrentPosition(
        locationSuccess,
        locationFailure,
        { timeout: 15000, maximumAge: 60000 }
    );
}

Pebble.addEventListener( 'ready',
    function( e ) {
        console.log( '[ACbus] Pebble JS ready!' );
        
        determineClosestBusStop();
    } );

Pebble.addEventListener( 'appmessage',
    function( e ) {
       console.log( '[ACbus] AppMessage received!' ) ;
    } );
