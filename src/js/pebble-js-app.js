// main function to retrieve, format, and send cgm data
function fetchCgmData() {

    // declare local variables for message data
    var response, message;

    //call options & started to get endpoint & start time
    var opts = options( );

    //if endpoint is invalid, return error msg to watch
    if (!opts.endpoint) {
      
        // get app time and format
        var endpointtime = new Date().getTime(),
        endpointTZdate = new Date(),
        endpointTZoffset = endpointTZdate.getTimezoneOffset(),
        formatendpointtime = Math.floor( (endpointtime / 1000) - (endpointTZoffset * 60) ),
        endpointstring = formatendpointtime.toString();
      
        message = {
        icon: "",
        bg: " ",
        readtime: "",
        alert: 0,
        time: endpointstring,
        delta: "NO ENDPOINT",
        battlevel: "",
        t1dname: ""
        };
        
        console.log("NO ENDPOINT send message", JSON.stringify(message));
        MessageQueue.sendAppMessage(message);
        return;
    }

    // call XML
    var req = new XMLHttpRequest();
    //console.log('endpoint: ' + opts.endpoint);
    
    // get cgm data
    req.open('GET', opts.endpoint, true);
    
    req.onload = function(e) {

        if (req.readyState == 4) {

            if(req.status == 200) {
                
                // Load response   
                response = JSON.parse(req.responseText);
                response = response.bgs;
                
                // check response data
                if (response && response.length > 0) {

                    // response data is good; send log with response 
                    // console.log('got response', JSON.stringify(response));

                    // initialize message data

                    // get direction arrow and BG
                    var currentDirection = response[0].direction,
                    currentBG = response[0].sgv,
                    //currentBG = "79",
                    typeBG = opts.radio,

                    // get timezone offset
                    timezonedate = new Date(),
                    timezoneoffset = timezonedate.getTimezoneOffset(),
                        
                    // get CGM time delta and format
                    readingtime = new Date(response[0].datetime).getTime(),
                    formatreadtime = Math.floor( (readingtime / 1000) - (timezoneoffset * 60) ),
                    readtimestring = formatreadtime.toString(),

                    // set alertValue
                    alertValue = 0,

                    // get app time and format
                    apptime = new Date().getTime(),
                    formatapptime = Math.floor( (apptime / 1000) - (timezoneoffset * 60) ),
                    apptimestring = formatapptime.toString(),    
                    
                    // get BG delta and format
                    currentBGDelta = response[0].bgdelta,
                    formatBGdelta = "",                    

                    // get battery level

                    currentBattery = response[0].battery,

                    // get name of T1D
                    NameofT1DPerson = opts.t1name;
                    
                    // console.log("Radio Button: " + typeBG);
                    // console.log("Battery Value: " + currentBattery);
					// if no battery being sent yet, then send nothing to watch
          
                    if (typeof currentBattery == "undefined") {
                      currentBattery = "";  
                    }
                  
                    if (typeBG == "mmol") {
						if ( (currentBGDelta < 5) && (currentBGDelta > -5) ) {
						// only format if valid BG delta; else will send blank to watch
						formatBGdelta = (currentBGDelta > 0 ? '+' : '') + currentBGDelta + " mmol";
						}
                    }
                    else {
						if ( (currentBGDelta < 100) && (currentBGDelta > -100) ) {
							// only format if valid BG delta; else will send blank to watch
							formatBGdelta = (currentBGDelta > 0 ? '+' : '') + currentBGDelta + " mg/dL";
						}
                    }
                  
                    // debug logs; uncomment when need to debug something
 
                    //console.log("current Direction: " + currentDirection);
                    //console.log("current BG: " + currentBG);
                    //console.log("now: " + now);
                    //console.log("readingtime: " + readingtime);
                    //console.log("current BG delta: " + currentBGDelta);
                    //console.log("current Delta: " + delta);              
                    //console.log("current Battery: " + currentBattery);
                    
                    // load message data  
                    message = {
                    icon: currentDirection,
                    bg: currentBG,
                    readtime: readtimestring,
                    alert: alertValue,
                    time: apptimestring,
                    delta: formatBGdelta,
                    battlevel: currentBattery,
                    t1dname: NameofT1DPerson
                    };
                    
                    // send message data to log and to watch
                    console.log("JS send message: " + JSON.stringify(message));
                    MessageQueue.sendAppMessage(message);

                // response data is no good; format error message and send to watch                      
                } else {
                  
                    // get app time and format
                    var offlinetime = new Date().getTime(),
                    offlineTZdate = new Date(),
                    offlineTZoffset = offlineTZdate.getTimezoneOffset(),
                    formatofflinetime = Math.floor( (offlinetime / 1000) - (offlineTZoffset * 60) ),
                    offlinestring = formatofflinetime.toString();
                  
                    message = {
                    icon: "",
                    bg: " ",
                    readtime: "",
                    alert: 0,
                    time: offlinestring,
                    delta: "DATA OFFLINE",
                    battlevel: "",
                    t1dname: ""
                    };
                  
                    console.log("DATA OFFLINE JS message", JSON.stringify(message));
                    MessageQueue.sendAppMessage(message);
                }
            }
        }
    };
    req.send(null);
}

// get endpoint for XML request
function options ( ) {
    var opts = [ ].slice.call(arguments).pop( );
    if (opts) {
        window.localStorage.setItem('cgmPebble', JSON.stringify(opts));
    } else {
        opts = JSON.parse(window.localStorage.getItem('cgmPebble'));
    }
    return opts;
}

// message queue-ing to pace calls from C function on watch
var MessageQueue = (function () {
                    
                    var RETRY_MAX = 5;
                    
                    var queue = [];
                    var sending = false;
                    var timer = null;
                    
                    return {
                    reset: reset,
                    sendAppMessage: sendAppMessage,
                    size: size
                    };
                    
                    function reset() {
                    queue = [];
                    sending = false;
                    }
                    
                    function sendAppMessage(message, ack, nack) {
                    
                    if (! isValidMessage(message)) {
                    return false;
                    }
                    
                    queue.push({
                               message: message,
                               ack: ack || null,
                               nack: nack || null,
                               attempts: 0
                               });
                    
                    setTimeout(function () {
                               sendNextMessage();
                               }, 1);
                    
                    return true;
                    }
                    
                    function size() {
                    return queue.length;
                    }
                    
                    function isValidMessage(message) {
                    // A message must be an object.
                    if (message !== Object(message)) {
                    return false;
                    }
                    var keys = Object.keys(message);
                    // A message must have at least one key.
                    if (! keys.length) {
                    return false;
                    }
                    for (var k = 0; k < keys.length; k += 1) {
                    var validKey = /^[0-9a-zA-Z-_]*$/.test(keys[k]);
                    if (! validKey) {
                    return false;
                    }
                    var value = message[keys[k]];
                    if (! validValue(value)) {
                    return false;
                    }
                    }
                    
                    return true;
                    
                    function validValue(value) {
                    switch (typeof(value)) {
                    case 'string':
                    return true;
                    case 'number':
                    return true;
                    case 'object':
                    if (toString.call(value) == '[object Array]') {
                    return true;
                    }
                    }
                    return false;
                    }
                    }
                    
                    function sendNextMessage() {
                    
                    if (sending) { return; }
                    var message = queue.shift();
                    if (! message) { return; }
                    
                    message.attempts += 1;
                    sending = true;
                    Pebble.sendAppMessage(message.message, ack, nack);
                    
                    timer = setTimeout(function () {
                                       timeout();
                                       }, 1000);
                    
                    function ack() {
                    clearTimeout(timer);
                    setTimeout(function () {
                               sending = false;
                               sendNextMessage();
                               }, 200);
                    if (message.ack) {
                    message.ack.apply(null, arguments);
                    }
                    }
                    
                    function nack() {
                    clearTimeout(timer);
                    if (message.attempts < RETRY_MAX) {
                    queue.unshift(message);
                    setTimeout(function () {
                               sending = false;
                               sendNextMessage();
                               }, 200 * message.attempts);
                    }
                    else {
                    if (message.nack) {
                    message.nack.apply(null, arguments);
                    }
                    }
                    }
                    
                    function timeout() {
                    setTimeout(function () {
                               sending = false;
                               sendNextMessage();
                               }, 1000);
                    if (message.ack) {
                    message.ack.apply(null, arguments);
                    }
                    }
                    
                    }
                    
                    }());

// pebble specific calls with watch
Pebble.addEventListener("ready",
                        function(e) {
                        "use strict";
                        console.log("Pebble JS ready");
                        });

Pebble.addEventListener("appmessage",
                        function(e) {
                        console.log("JS Recvd Msg From Watch: " + JSON.stringify(e.payload));
                        fetchCgmData();
                        });

Pebble.addEventListener("showConfiguration", function(e) {
                        console.log("Showing Configuration", JSON.stringify(e));
                        Pebble.openURL('http://m2oore.github.io/cgm-pebble/configurable.html');
                        });

Pebble.addEventListener("webviewclosed", function(e) {
                        var opts = e.response.length > 5
                        ? JSON.parse(decodeURIComponent(e.response)): null;
                        
                        options(opts);
                        
                        });







