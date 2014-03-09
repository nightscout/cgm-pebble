

function fetchCgmData(lastReadTime, lastBG) {

    var response;
    var req = new XMLHttpRequest();
    req.open('GET', "https://000.000.000.000/pebbledata, true);

    req.onload = function(e) {
        console.log(req.readyState);
        if (req.readyState == 4) {
            console.log(req.status);
            if(req.status == 200) {

                response = JSON.parse(req.responseText);

                Pebble.sendAppMessage({
                                      "icon":response[0].trend,
                                      "bg":response[0].sgv,
                                      "readtime":response[0].readtime,
                                      "alert":response[0].alert,
                                      "time": response[0].currenttime,
                                      "delta":response[0].delta
                                      });
            } else {

            }
        } else
        {

        }
    }
    req.send(null);
}

Pebble.addEventListener("ready",
                        function(e) {
                        console.log("connect: " + e.ready);
                        });

Pebble.addEventListener("appmessage",
                        function(e) {
                        console.log("Received message: " + JSON.stringify(e.payload));
                        fetchCgmData(e.payload.readtime, e.payload.bg);
                        });



