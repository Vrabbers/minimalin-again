"use strict";

var Clay = require('pebble-clay');
var clayConfig = require('./config.json');
var clayFunction = require('./clayFunction.js');
var clay = new Clay(clayConfig, clayFunction, { autoHandleEvents: false });

var Weather = function (pebble) {
    var GET = 'GET';
    var BASE_URL = 'https://api.open-meteo.com/v1/forecast';
    var BASE_GEOCODE_URL = 'https://photon.komoot.io/api';
    var ICONS = {
        0: 'a',
        1: 'b',
        2: 'c',
        3: 'd',
        51: 'e',
        53: 'e',
        55: 'e',
        56: 'e',
        57: 'e',
        61: 'f',
        63: 'f',
        65: 'f',
        66: 'f',
        67: 'f',
        80: 'f',
        81: 'f',
        82: 'f',
        95: 'g',
        96: 'g',
        99: 'g',

        71: 'h',
        73: 'h',
        75: 'h',
        77: 'h',
        85: 'h',
        86: 'h',

        45: 'i',
        48: 'i',
    };
    var LOCATION_OPTS = {
        'timeout': 5000,
        'maximumAge': 30 * 60 * 1000
    };

    var parseIcon = function (icon, is_day) {
        var i = ICONS[icon];
        if (!is_day)
            i = i.toUpperCase();
        return i.charCodeAt(0);
    };

    var GEOCODE_FETCH_CACHE = "geocodeFetchCache";

    var fetchLocation = function (location, callbackSuccess, callbackError) {
        var lastGeocode = localStorage.getItem(GEOCODE_FETCH_CACHE);

        if (lastGeocode !== null) {
            var geo = JSON.parse(lastGeocode);
            if (geo.location === location) {
                if (geo.failed)
                    callbackError("geocode already failed, don't even try again...");
                else
                    callbackSuccess(geo.latitude, geo.longitude);
                return; 
            }
        }

        var url = BASE_GEOCODE_URL + '?limit=1&q=' + location;
        console.log("Fetching location " + url);
        var req = new XMLHttpRequest();
        req.open(GET, url, true);
        req.onload = function () {
            if (req.status === 200) {
                var resp = JSON.parse(req.responseText);
                if (resp.features.length != 1) {
                    localStorage.setItem(GEOCODE_FETCH_CACHE, JSON.stringify({
                        location: location,
                        failed: true
                    }));
                    callbackError("geocode failed: features.length != 1");
                    return;
                }
                var coords = resp.features[0].geometry.coordinates;
                var lat = coords[1];
                var long = coords[0];
                console.log("geocode success: long:" + long + ", lat:" + lat);
                var cache = {
                    location: location,
                    latitude: lat,
                    longitude: long
                };
                localStorage.setItem(GEOCODE_FETCH_CACHE, JSON.stringify(cache))
                callbackSuccess(lat, long);
            }
            else {
                callbackError("geocode error: status " + req.status);
            }
        };
        req.onerror = function() { callbackError("geocode error: request failed"); };
        req.send(null);
    };

    var fetchWeatherForLocation = function (location) {
        fetchLocation(location, fetchWeatherForCoordinates, weatherError);
    };

    var fetchWeatherForCoordinates = function (latitude, longitude) {
        var query = 'latitude=' + latitude + '&longitude=' + longitude;
        fetchWeather(query);
    };

    var fetchWeather = function (query) {
        localStorage.removeItem("lastCoordFetch");
        var req = new XMLHttpRequest();
        query += '&current=temperature_2m,weather_code,is_day';
        console.log('query: ' + query);
        req.open(GET, BASE_URL + '?' + query, true);
        req.onload = function () {
            if (req.status === 200) {
                var response = JSON.parse(req.responseText);
                var temperature = Math.round(response.current.temperature_2m);
                var icon = parseIcon(response.current.weather_code, !!response.current.is_day);
                var data = {
                    'AppKeyWeatherIcon': icon,
                    'AppKeyWeatherTemperature': temperature
                };
                console.log('fetchWeather sendAppMessage:', JSON.stringify(data));
                pebble.sendAppMessage(data);
            } else {
                weatherError("weather query failed, request status: " + req.status);
            }
        };
        req.onerror = function () { weatherError("weather query failed (onerror)"); };
        req.send(null);
    };

    var locationSuccess = function (pos) {
        var coordinates = pos.coords;
        fetchWeatherForCoordinates(coordinates.latitude, coordinates.longitude);
    };

    var weatherError = function (err) {
        console.log('weather fetch error: ' + err);
        pebble.sendAppMessage({
            'AppKeyWeatherFailed': 1
        });
    };

    pebble.addEventListener('appmessage', function (e) {
        var dict = e.payload;
        //console.log('appmessage:', JSON.stringify(dict));
        if (dict['AppKeyWeatherRequest']) {
            var location = localStorage.getItem("local.WeatherLocation");
            console.log("got location " + location);
            if (location) {
                fetchWeatherForLocation(location);
            } else {
                window.navigator.geolocation.getCurrentPosition(locationSuccess, weatherError, LOCATION_OPTS);
            }
        }
    });
}(Pebble);

Pebble.addEventListener('ready', function (e) {
    var data = { 'AppKeyJsReady': 1 };
    Pebble.sendAppMessage(data);
});

Pebble.addEventListener('showConfiguration', function (e) {
    Pebble.openURL(clay.generateUrl());
});

Pebble.addEventListener('webviewclosed', function (e) {
    if (e && !e.response) {
        return;
    }

    // Get the keys and values from each config item
    var dict = clay.getSettings(e.response, false);

    for (var key in dict) {
        console.log(key);
        // HACK: Cheat the system so that persistent keys are still saved on by Clay. 
        // Set local storage for easy access but are not sent over to device
        if (key.substring(0, "local.".length) === "local.") {
            console.log("getting rid of " + key);
            localStorage.setItem(key, dict[key].value.trim());
            delete dict[key];
            continue;
        }
        else if (key === "AppKeyBluetoothIcon" || key === "AppKeyTemperatureUnit") {
            dict[key].value = parseInt(dict[key].value);
        }
    }

    dict["AppKeyConfig"] = 1;

    console.log(JSON.stringify(dict));


    var dictConverted = Clay.prepareSettingsForAppMessage(dict);

    console.log(JSON.stringify(dictConverted));

    // Send settings values to watch side
    Pebble.sendAppMessage(dictConverted, function (e) {
        console.log('Sent config data to Pebble');

    }, function (e) {
        console.log('Failed to send config data');
    });
});
