// main function to retrieve, format, and send cgm data
function fetchCgmData() {
  
    //console.log ("START fetchCgmData");
                
    // declare local variables for message data
    var response, message;

    //get options from configuration window
    var opts = [ ].slice.call(arguments).pop( );
    opts = JSON.parse(window.localStorage.getItem('cgmPebble'));

    // show current options
    //console.log("fetchCgmData IN OPTIONS = " + JSON.stringify(opts));
  
    // call XML
    var req = new XMLHttpRequest();
    
    // get cgm data
    req.open('GET', opts.endpoint, true);
    
    req.setRequestHeader('Cache-Control', 'no-cache');
    
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
                    currentIcon = "10",
                    currentBG = response[0].sgv,
                    //currentBG = "100",
                    //typeBG = opts.radio,

                    // get timezone offset
                    timezoneDate = new Date(),
                    timezoneOffset = timezoneDate.getTimezoneOffset(),
                        
                    // get CGM time delta and format
                    readingTime = new Date(response[0].datetime).getTime(),
                    //readingTime = null,
                    formatReadTime = Math.floor( (readingTime / 1000) - (timezoneOffset * 60) ),

                    // get app time and format
                    appTime = new Date().getTime(),
                    //appTime = null,
                    formatAppTime = Math.floor( (appTime / 1000) - (timezoneOffset * 60) ),   
                    
                    // get BG delta and format
                    currentBGDelta = response[0].bgdelta,
                    //currentBGDelta = -8,
                    formatBGDelta = " ",

                    // get battery level
                    currentBattery = response[0].battery,
                    //currentBattery = "100",
                    // get name of T1D
                    NameofT1DPerson = opts.t1name;
                    
                    //currentDirection = "NONE";
                  
                    // convert arrow to a number string; sending number string to save memory
                    // putting NOT COMPUTABLE first because that's most common and can get out fastest
                    switch (currentDirection) {
                      case "NOT COMPUTABLE": currentIcon = "8"; break;
                      case "NONE": currentIcon = "0"; break;
                      case "DoubleUp": currentIcon = "1"; break;
                      case "SingleUp": currentIcon = "2"; break;
                      case "FortyFiveUp": currentIcon = "3"; break;
                      case "Flat": currentIcon = "4"; break;
                      case "FortyFiveDown": currentIcon = "5"; break;
                      case "SingleDown": currentIcon = "6"; break;
                      case "DoubleDown": currentIcon = "7"; break;
                      case "RATE OUT OF RANGE": currentIcon = "9"; break;
                      default: currentIcon = "10";
                    }
					
                    // if no battery being sent yet, then send nothing to watch
                    // console.log("Battery Value: " + currentBattery);
                    if (typeof currentBattery == "undefined") {
                      currentBattery = " ";  
                    }
                  
                    // assign bg delta string
                    formatBGDelta = ((currentBGDelta > 0 ? '+' : '') + currentBGDelta);
                  
                    // debug logs; uncomment when need to debug something
 
                    //console.log("current Direction: " + currentDirection);
                    //console.log("current Icon: " + currentIcon);
                    //console.log("current BG: " + currentBG);
                    //console.log("now: " + formatAppTime);
                    //console.log("readingtime: " + formatReadTime);
                    //console.log("current BG delta: " + currentBGDelta);
                    //console.log("current Formatted Delta: " + formatBGDelta);              
                    //console.log("current Battery: " + currentBattery);
                    
                    // load message data  
                    message = {
                      icon: currentIcon,
                      bg: currentBG,
                      readtime: formatReadTime,
                      time: formatAppTime,
                      delta: formatBGDelta,
                      battlevel: currentBattery,
                      t1dname: NameofT1DPerson
                    };
                    
                    // send message data to log and to watch
                    console.log("JS send message: " + JSON.stringify(message));
                    MessageQueue.sendAppMessage(message);

                // response data is not good; format error message and send to watch
                // have to send space in BG field for logo to show up on screen				
                } else {
                  
                    message = {
                      icon: " ",
                      bg: " ",
                      readtime: 0,
                      time: 0,
                      delta: "ERR",
                      battlevel: " ",
                      t1dname: " "
                    };
                  
                    console.log("DATA OFFLINE JS message", JSON.stringify(message));
                    MessageQueue.sendAppMessage(message);
                }
            } // end req.status == 200
        } // end req.readyState == 4
    }; // req.onload
    req.send(null);
} // end fetchCgmData

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
                        var opts = JSON.parse(decodeURIComponent(e.response));
                        console.log("CLOSE CONFIG OPTIONS = " + JSON.stringify(opts));
                        // store endpoint in local storage
                        window.localStorage.setItem('cgmPebble', JSON.stringify(opts));                      
                        });


