var TIME_5_MINS = 5 * 60 * 1000,
    TIME_10_MINS = 10 * 60 * 1000,
    TIME_15_MINS = 15 * 60 * 1000,
    TIME_30_MINS = TIME_15_MINS * 2;

var lastAlert = 0;

function fetchCgmData(lastReadTime, lastBG) {
    
    var response, message;
    var req = new XMLHttpRequest();
    //req.open('GET', "http://192.168.1.105:9000/pebble", true);
  req.open('GET', "http://nightscouttd1.azurewebsites.net/pebble", true);
    
    req.onload = function(e) {
        
        console.log(req.readyState);
        if (req.readyState == 4) {
          var now = new Date().getTime();
       
          console.log(req.status);
            if(req.status == 200) {
                console.log("status: " + req.status);
                response = JSON.parse(req.responseText);
                
                var
                    sinceLastAlert = now - lastAlert,
                    alertValue = 0,
                    bgs = response.bgs;
              console.log('bgs?', JSON.stringify(bgs));
              if (bgs && bgs.length > 0) {
              console.log('got bgs', JSON.stringify(bgs));
              var
                    currentBG = bgs[0].sgv,
                    currentBGDelta = bgs[0].bgdelta,
                    currentTrend = bgs[0].trend,
                    delta = currentBGDelta + " mg/dL",
                    readingtime = new Date(bgs[0].datetime).getTime(),
                    readago = now - readingtime;
              
                console.log("now: " + now);
                console.log("readingtime: " + readingtime);
                console.log("readago: " + readago);
                  
                if (currentBG < 39) {
                  if (sinceLastAlert > TIME_10_MINS) alertValue = 2;
                } else if (currentBG < 55)
                  alertValue = 2;
                else if (currentBG < 60 && currentBGDelta < 0)
                  alertValue = 2;
                else if (currentBG < 70 && sinceLastAlert > TIME_15_MINS)
                    alertValue = 2;
                else if (currentBG < 120 && currentTrend == 7 && sinceLastAlert > TIME_5_MINS) //DBL_DOWN
                  alertValue = 2;
                else if (currentBG == 100 && currentTrend == 4 && sinceLastAlert > TIME_15_MINS) //PERFECT SCORE
                  alertValue = 1;
                else if (currentBG > 120 && currentTrend == 1 && sinceLastAlert > TIME_15_MINS) //DBL_UP
                  alertValue = 3;
                else if (currentBG > 200 && sinceLastAlert > TIME_30_MINS && currentBGDelta > 0)
                  alertValue = 3;
                else if (currentBG > 250 && sinceLastAlert > TIME_30_MINS)
                  alertValue = 3;
                else if (currentBG > 300 && sinceLastAlert > TIME_15_MINS)
                  alertValue = 3;
              
                if (alertValue === 0 && readago > TIME_10_MINS) {
                  alertValue = 1;
                }
              
                if (alertValue > 0) {
                  lastAlert = now;
                }
              
                message = {
                  icon: bgs[0].trend,
                  bg: currentBG,
                  readtime: timeago(new Date().getTime() - (new Date(bgs[0].datetime).getTime())),
                  alert: alertValue,
                  time: formatDate(new Date()),
                  delta: delta
                };
                
                console.log("message: " + JSON.stringify(message));
                Pebble.sendAppMessage(message);
            
              } else {
                message = {
                  icon: 0,
                  bg: '???',
                  readtime: timeago(new Date().getTime() - (now)),
                  alert: 0,
                  time: formatDate(new Date()),
                  delta: 'offline'
                
                };
                console.log("sending message", JSON.stringify(message)); 
                Pebble.sendAppMessage(message);
            
              }
              }
        }
    };
    req.send(null);
}

function formatDate(date) {
  var minutes = date.getMinutes(),
    hours = date.getHours() || 12,
    meridiem = " PM",
    formatted;
  
  if (hours > 12)
    hours = hours - 12; 
  else
    meridiem = " AM";
  
  if (minutes < 10)
    formatted = hours + ":0" + date.getMinutes() + meridiem;
  else
    formatted = hours + ":" + date.getMinutes() + meridiem;
  
  return formatted;
}

function timeago(offset) {
  var parts = {},
    MINUTE = 60 * 1000,
    HOUR = 3600 * 1000,
    DAY = 86400 * 1000,
    WEEK = 604800 * 1000;
          
  if (offset <= MINUTE)              parts = { lablel: 'now' };
  if (offset <= MINUTE * 2)          parts = { label: '1 min ago' };
  else if (offset < (MINUTE * 60))   parts = { value: Math.round(Math.abs(offset / MINUTE)), label: 'mins' };
  else if (offset < (HOUR * 2))      parts = { label: '1 hr ago' };
  else if (offset < (HOUR * 24))     parts = { value: Math.round(Math.abs(offset / HOUR)), label: 'hrs' };
  else if (offset < (DAY * 1))       parts = { label: '1 day ago' };
  else if (offset < (DAY * 7))       parts = { value: Math.round(Math.abs(offset / DAY)), label: 'day' };
  else if (offset < (WEEK * 52))     parts = { value: Math.round(Math.abs(offset / WEEK)), label: 'week' };
  else                               parts = { label: 'a long time ago' };
  
  if (parts.value)
    return parts.value + ' ' + parts.label + ' ago';
  else
    return parts.label;

}

Pebble.addEventListener("ready",
                        function(e) {
                        console.log("connect: " + e.ready);
                        //fetchCgmData(0, 0);
                        });

Pebble.addEventListener("appmessage",
                        function(e) {
                        console.log("Received message: " + JSON.stringify(e.payload));
                        fetchCgmData(e.payload.readtime, e.payload.bg);
                        });



