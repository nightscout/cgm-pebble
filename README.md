pebble-cgm
==========

Basic pebble watch face (or app if you change the appinfo.json)

It does not require a companion app, other than the Pebble made app. It uses a JS file to grab data from an existing web service.

Please review the src/js/pebble-js-app.js file

Wherever the CGM data is, this file will need to "get" it. I use a web service (API) to get a response object that contains the info I use, but I've taken out the actual address of my svc.

You can dummy up the response to play around with the watch face.

Please check out Pebble's guides to get rolling,

and as with everything I have committed here: This is presented for educational purposes only, BE smart! don't make medical decisions based on data provided by this app.
