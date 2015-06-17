// main function to retrieve, format, and send cgm data
function fetchCgmData() {
  
    //console.log ("START fetchCgmData");
                
    // declare local variables for message data
    var response, responsebgs, responsecals, message;

    //get options from configuration window
    var opts = [ ].slice.call(arguments).pop( );
    opts = JSON.parse(window.localStorage.getItem('cgmPebble'));

	// check if endpoint exists
    if (!opts.endpoint) {
        // endpoint doesn't exist, return no endpoint to watch
		// " " (space) shows these are init values, not bad or null values
        message = {
          icon: " ",
          bg: " ",
          tcgm: 0,
          tapp: 0,
          dlta: "NOEP",
          ubat: " ",
          name: " ",
          vals: " ",
          clrw: " ",
          rwuf: " ",
          noiz: 0
        };
        
        console.log("NO ENDPOINT JS message", JSON.stringify(message));
        MessageQueue.sendAppMessage(message);
        return;
    } // if (!opts.endpoint)
	
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
                
                // clear the XML timeout
                clearTimeout(myCGMTimeout);
              
                // Load response   
                response = JSON.parse(req.responseText);
                responsebgs = response.bgs;
                responsecals = response.cals;
                
                // check response data
                if (responsebgs && responsebgs.length > 0) {

                    // response data is good; send log with response 
                    // console.log('got response', JSON.stringify(response));

                    // initialize message data
                  
                    // get direction arrow and BG
                    var currentDirection = responsebgs[0].direction,
                    values = " ",
                    currentIcon = "10",
                    currentBG = responsebgs[0].sgv,
                    //currentBG = "107",
                    currentConvBG = currentBG,
                    rawCalcOffset = 5,
                    specialValue = false,
                    calibrationValue = false,

                    // get timezone offset
                    timezoneDate = new Date(),
                    timezoneOffset = timezoneDate.getTimezoneOffset(),
                        
                    // get CGM time delta and format
                    readingTime = new Date(responsebgs[0].datetime).getTime(),
                    //readingTime = null,
                    formatReadTime = Math.floor( (readingTime / 1000) - (timezoneOffset * 60) ),

                    // get app time and format
                    appTime = new Date().getTime(),
                    //appTime = null,
                    formatAppTime = Math.floor( (appTime / 1000) - (timezoneOffset * 60) ),   
                    
                    // get BG delta and format
                    currentBGDelta = responsebgs[0].bgdelta,
                    //currentBGDelta = -8,
                    formatBGDelta = " ",

                    // get battery level
                    currentBattery = responsebgs[0].battery,
                    //currentBattery = "100",
 
                   // get NameofT1DPerson and IOB
                    NameofT1DPerson = opts.t1name,
                    currentIOB = responsebgs[0].iob,
 
                    // sensor fields
                    currentCalcRaw = 0,
                    //currentCalcRaw = 100000,
                    formatCalcRaw = " ",
                    currentRawFilt = responsebgs[0].filtered,
                    formatRawFilt = " ",
                    currentRawUnfilt = responsebgs[0].unfiltered,
                    formatRawUnfilt = " ",
                    currentNoise = responsebgs[0].noise,
                    currentIntercept = "undefined",
                    currentSlope = "undefined",
                    currentScale = "undefined",
                    currentRatio = 0;
  
                    // get name of T1D; if iob (case insensitive), use IOB
                    if ( (NameofT1DPerson.toUpperCase() === "IOB") && 
                    ((typeof currentIOB != "undefined") && (currentIOB !== null)) ) {
                      NameofT1DPerson = "IOB:" + currentIOB;
                    }
                    else {
                      NameofT1DPerson = opts.t1name;
                    }
  
                    if (responsecals && responsecals.length > 0) {
                      currentIntercept = responsecals[0].intercept;
                      currentSlope = responsecals[0].slope;
                      currentScale = responsecals[0].scale;
                    }
                  
                    //currentDirection = "NONE";

                    // set some specific flags needed for later
                    if (opts.radio == "mgdl_form") { 
                      if ( (currentBG < 40) || (currentBG > 400) ) { specialValue = true; }
                      if (currentBG == 5) { calibrationValue = true; }
                    }
                    else {
                      if ( (currentBG < 2.3) || (currentBG > 22.2) ) { specialValue = true; }
                      if (currentBG == 0.3) { calibrationValue = true; }
                      currentConvBG = (Math.round(currentBG * 18.018).toFixed(0));                                                                   
                    }
              
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
                    if ( (typeof currentBattery == "undefined") || (currentBattery === null) ) {
                      currentBattery = " ";  
                    }
                  
                    // assign bg delta string
                    formatBGDelta = ((currentBGDelta > 0 ? '+' : '') + currentBGDelta);

                    //console.log("Current Unfiltered: " + currentRawUnfilt);                  
                    //console.log("Current Intercept: " + currentIntercept);
                    //console.log("Special Value Flag: " + specialValue);
                    //console.log("Current BG: " + currentBG);
                  
                    // assign calculated raw value if we can
                    if ( (typeof currentIntercept != "undefined") && (currentIntercept !== null) ){
                        if (specialValue) {
                          // don't use ratio adjustment
                          currentCalcRaw = ((currentScale * (currentRawUnfilt - currentIntercept) / currentSlope)*1 - rawCalcOffset*1);
                          //console.log("Special Value Calculated Raw: " + currentCalcRaw);
                        } 
                        else {
                          currentRatio = (currentScale * (currentRawFilt - currentIntercept) / currentSlope / (currentConvBG*1 + rawCalcOffset*1));
                          currentCalcRaw = ((currentScale * (currentRawUnfilt - currentIntercept) / currentSlope / currentRatio)*1 - rawCalcOffset*1);
                          //console.log("Current Converted BG: " + currentConvBG);
                          //console.log("Current Ratio: " + currentRatio);
                          //console.log("Normal BG Calculated Raw: " + currentCalcRaw);
                        }          
                    } // if currentIntercept                  

                    // assign raw sensor values if they exist
                    if ( (typeof currentRawUnfilt != "undefined") && (currentRawUnfilt !== null) ) {
                      
                      // zero out any invalid values; defined anything not between 0 and 900
                      if ( (currentRawFilt < 0) || (currentRawFilt > 900000) || 
                            (isNaN(currentRawFilt)) ) { currentRawFilt = "ERR"; }
                      if ( (currentRawUnfilt < 0) || (currentRawUnfilt > 900000) || 
                            (isNaN(currentRawUnfilt)) ) { currentRawUnfilt = "ERR"; }
                      
                      // set 0, LO and HI in calculated raw
                      if ( (currentCalcRaw >= 0) && (currentCalcRaw < 30) ) { formatCalcRaw = "LO"; }
                      if ( (currentCalcRaw > 500) && (currentCalcRaw <= 900) ) { formatCalcRaw = "HI"; }
                      if ( (currentCalcRaw < 0 ) || (currentCalcRaw > 900) ) { formatCalcRaw = "ERR"; }
                      
                      // if slope or intercept are at 0, or if currentCalcRaw is NaN, 
                      // calculated raw is invalid and need a calibration
                      if ( (currentSlope === 0) || (currentIntercept === 0) || 
                           (isNaN(currentCalcRaw)) ) { formatCalcRaw = "CAL"; }
                      
                      // check for compression warning
                      if ( ((currentCalcRaw < (currentRawFilt/1000)) && (!calibrationValue)) && (currentRawFilt !== 0) ){
                        var compressionSlope = 0;
                        compressionSlope = (((currentRawFilt/1000) - currentCalcRaw)/(currentRawFilt/1000));
                        //console.log("compression slope: " + compressionSlope);
                        if (compressionSlope > 0.7) {
                          // set COMPRESSION? message
                          formatBGDelta = "PRSS";
                        } // if compressionSlope
                      } // if check for compression condition
                      
                      if (opts.radio == "mgdl_form") { 
                        formatRawFilt = ((Math.round(currentRawFilt / 1000)).toFixed(0));
                        formatRawUnfilt = ((Math.round(currentRawUnfilt / 1000)).toFixed(0));
                        if ( (formatCalcRaw != "LO") && (formatCalcRaw != "HI") && 
                             (formatCalcRaw != "ERR") && (formatCalcRaw != "CAL") ) 
                            { formatCalcRaw = ((Math.round(currentCalcRaw)).toFixed(0)); }
                        //console.log("Format Unfiltered: " + formatRawUnfilt);
                      } 
                      else {
                        formatRawFilt = ((Math.round(((currentRawFilt/1000)*0.0555) * 10) / 10).toFixed(1));
                        formatRawUnfilt = ((Math.round(((currentRawUnfilt/1000)*0.0555) * 10) / 10).toFixed(1));
                        if ( (formatCalcRaw != "LO") && (formatCalcRaw != "HI") &&
                             (formatCalcRaw != "ERR") && (formatCalcRaw != "CAL") ) 
                        { formatCalcRaw = ((Math.round(currentCalcRaw)*0.0555).toFixed(1)); }
                        //console.log("Format Unfiltered: " + formatRawUnfilt);
                      }
                    } // if currentRawUnfilt 
                  
                    //console.log("Calculated Raw To Be Sent: " + formatCalcRaw);
                  
                    // assign blank noise if it doesn't exist
                    if ( (typeof currentNoise == "undefined") || (currentNoise === null) ) {
                      currentNoise = 0;  
                    }
                    
                    if (opts.radio == "mgdl_form") {
                      values = "0";  //mgdl selected
                    } else {
                      values = "1"; //mmol selected
                    }
                    values += "," + opts.lowbg;  //Low BG Level
                    values += "," + opts.highbg; //High BG Level                      
                    values += "," + opts.lowsnooze;  //LowSnooze minutes 
                    values += "," + opts.highsnooze; //HighSnooze minutes
                    values += "," + opts.lowvibe;  //Low Vibration 
                    values += "," + opts.highvibe; //High Vibration
                    values += "," + opts.vibepattern; //Vibration Pattern
                    if (opts.timeformat == "12"){
                      values += ",0";  //Time Format 12 Hour  
                    } else {
                      values += ",1";  //Time Format 24 Hour  
                    }
                    // Vibrate on raw value in special value; Yes = 1; No = 0;
                    if ( (currentCalcRaw !== 0) && (opts.rawvibrate == "1") ) {
                      values += ",1";  // Vibrate on raw value when in special values  
                    } else {
                      values += ",0";  // Do not vibrate on raw value when in special values                        
                    }
                    
                    //console.log("Current Value: " + values);
                    //console.log("Current rawvibrate: " + opts.rawvibrate);
                    //console.log("Current currentCalcRaw: " + currentCalcRaw);
                  
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
                      tcgm: formatReadTime,
                      tapp: formatAppTime,
                      dlta: formatBGDelta,
                      ubat: currentBattery,
                      name: NameofT1DPerson,
                      vals: values,
                      clrw: formatCalcRaw,
                      rwuf: formatRawUnfilt,
                      noiz: currentNoise
                    };
                    
                    // send message data to log and to watch
                    console.log("JS send message: " + JSON.stringify(message));
                    MessageQueue.sendAppMessage(message);

                // response data is not good; format error message and send to watch
                // have to send space in BG field for logo to show up on screen				
                } else {
                  
                    // " " (space) shows these are init values (even though it's an error), not bad or null values
                    message = {
                      dlta: "OFF"
                    };
                  
                    console.log("DATA OFFLINE JS message", JSON.stringify(message));
                    MessageQueue.sendAppMessage(message);
                }
            } // end req.status == 200
        } // end req.readyState == 4
    }; // req.onload
    req.send(null);
    var myCGMTimeout = setTimeout (function () {
      req.abort();
      message = {
        dlta: "OFF"
      };          
      console.log("DATA OFFLINE JS message", JSON.stringify(message));
      MessageQueue.sendAppMessage(message);
    }, 59000 ); // timeout in ms; set at 45 seconds; can not go beyond 59 seconds      
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
                        Pebble.openURL('http://nightscout.github.io/cgm-pebble/s1-config-7.html');
                        });

Pebble.addEventListener("webviewclosed", function(e) {
                        var opts = JSON.parse(decodeURIComponent(e.response));
                        console.log("CLOSE CONFIG OPTIONS = " + JSON.stringify(opts));
                        // store endpoint in local storage
                        window.localStorage.setItem('cgmPebble', JSON.stringify(opts));                      
                        });
