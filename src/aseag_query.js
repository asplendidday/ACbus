var item_list = 'StopPointName,LineName,DestinationName,EstimatedTime';
var query_url_stub = 'http://ivu.aseag.de/interfaces/ura/instant_V1?ReturnList=';
var query_url = query_url_stub +  item_list + '&StopID=100016';

var xhrRequest = function( url, type, callback ) {
    var xhr = new XMLHttpRequest();
    xhr.onload = function() {
        callback(this.responseText);
    };
    xhr.open(type, url);
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

Pebble.addEventListener('ready',
    function( e ) {
        console.log( 'Pebble JS ready!' );
        
        getBusData();
    } );

Pebble.addEventListener( 'appmessage',
    function( e ) {
       console.log( 'AppMessage received!' ) ;
    } );
