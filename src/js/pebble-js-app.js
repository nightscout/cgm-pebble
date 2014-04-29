var TIME_15_MINS = 15 * 60 * 1000,
	TIME_30_MINS = TIME_15_MINS * 2;

var lastAlert = 0;

function fetchCgmData(lastReadTime, lastBG) {

	var response;
	var req = new XMLHttpRequest();
	req.open('GET', "http://localhost:9000/pebble", true);

	req.onload = function(e) {
		console.log(req.readyState);
		if (req.readyState == 4) {
			console.log(req.status);
			if(req.status == 200) {
				console.log("status: " + req.status);
				response = JSON.parse(req.responseText);

				var now = Date.now(),
					sinceLastAlert = now - lastAlert,
					alertValue = 0,
					bgs = response.bgs,
					currentBG = bgs[0].sgv,
					currentDelta = bgs[0].bgdelta,
					currentTrend = bgs[0].trend,
					delta = "Change: " + currentDelta + "mg/dL";


				if (currentBG < 55)
					alertValue = 2;
				else if (currentBG < 60 && currentDelta < 0)
					alertValue = 2;
				else if (currentBG < 70 && sinceLastAlert > TIME_15_MINS)
					alertValue = 2;
				else if (currentBG < 120 && currentTrend == 7) //DBL_DOWN
					alertValue = 2;
				else if (currentBG == 100 && currentTrend == 4 && sinceLastAlert > TIME_15_MINS) //PERFECT SCORE
					alertValue = 1;
				else if (currentBG > 120 && currentTrend == 1) //DBL_UP
					alertValue = 3;
				else if (currentBG > 200 && sinceLastAlert > TIME_30_MINS && currentDelta > 0)
					alertValue = 3;
				else if (currentBG > 250 && sinceLastAlert > TIME_30_MINS)
					alertValue = 3;
				else if (currentBG > 300 && sinceLastAlert > TIME_15_MINS)
					alertValue = 3;

				if (alertValue > 0) {
					lastAlert = now;
				}

				var message = {
					icon: bgs[0].trend,
					bg: currentBG,
					readtime: formatDate(new Date(bgs[0].datetime)),
					alert: alertValue,
					time: formatDate(new Date()),
					delta: delta
				};

				console.log("message: " + JSON.stringify(message));
				Pebble.sendAppMessage(message);
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



