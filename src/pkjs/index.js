"use strict"

var Clay = require('pebble-clay');
var clayConfig = require('./config.json');
var clayFunction = require('./clayFunction.js')
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
    }

    var fetchLocation = function (location, callbackSuccess, callbackError) {
        var url = BASE_GEOCODE_URL + '?limit=1&q=' + location;
        console.log("Fetching location " + url);
        var req = new XMLHttpRequest();
        req.open(GET, url, true);
        req.onloadend = function () {
            if (req.status === 200) {
                var resp = JSON.parse(req.responseText);
                if (resp.features.length != 1) {
                    localStorage.setItem("lastCoordFetch", "failed")
                    callbackError("geocode failed: features.length != 1")
                    return;
                }
                var coords = resp.features[0].geometry.coordinates;
                var lat = coords[1];
                var lon = coords[0];
                console.log("geocode success: long:" + lon + ", lat:" + lat);
                localStorage.setItem("lastCoordFetch", + "lat:" + lat + ", long:" + lon)

                callbackSuccess(lat, lon);
            }
        }
        req.send(null);
    }

    var fetchWeatherForLocation = function (location) {
        var loc = fetchLocation(location, fetchWeatherForCoordinates, function (e) {
            console.log(e);
            console.log("pebble.sendAppMessage({'AppKeyWeatherFailed': 1})")
            pebble.sendAppMessage({
                'AppKeyWeatherFailed': 1
            });
        });
    }

    var fetchWeatherForCoordinates = function (latitude, longitude) {
        var query = 'latitude=' + latitude + '&longitude=' + longitude;
        fetchWeather(query);
    }

    var fetchWeather = function (query) {
        // if (!API_ID || API_ID.length == 0) {
        //   console.error("no weather API key");
        //   Pebble.sendAppMessage({ 'AppKeyWeatherFailed': 1 });
        // }
        localStorage.removeItem("lastCoordFetch");
        var req = new XMLHttpRequest();
        query += '&current=temperature_2m,weather_code,is_day'
        console.log('query: ' + query);
        req.open(GET, BASE_URL + '?' + query, true);
        req.onloadend = function () {
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
                console.log('error: fetchWeather AppKeyWeatherFailed:', JSON.stringify(req));
                pebble.sendAppMessage({ 'AppKeyWeatherFailed': 1 });
            }
        };
        req.send(null);
    }

    var locationSuccess = function (pos) {
        //console.log('AppKeyWeatherFailed: locationSuccess');
        var coordinates = pos.coords;
        fetchWeatherForCoordinates(coordinates.latitude, coordinates.longitude);
    }

    var locationError = function (err) {
        //console.log('error: AppKeyWeatherFailed: locationError');
        pebble.sendAppMessage({
            'AppKeyWeatherFailed': 1
        });
    }

    pebble.addEventListener('appmessage', function (e) {
        var dict = e.payload;
        //console.log('appmessage:', JSON.stringify(dict));
        if (dict['AppKeyWeatherRequest']) {
            var location = localStorage.getItem("local.WeatherLocation")
            console.log("got location " + location)
            if (location) {
                fetchWeatherForLocation(location);
            } else {
                window.navigator.geolocation.getCurrentPosition(locationSuccess, locationError, LOCATION_OPTS);
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
        // HACK: Cheat the system so that persistent keys are still saved on by Clay. 
        // Set local storage for easy access but are not sent over to device
        if (key.startsWith("local.")) {
            localStorage.setItem(key, dict[key].value)
            dict[key] = undefined;
            continue;
        }
        else if (key == "AppKeyBluetoothIcon") {
            dict[key].value = parseInt(dict[key].value)
        }
    }

    dict["AppKeyConfig"] = 1

    var dictConverted = Clay.prepareSettingsForAppMessage(dict)

    // Send settings values to watch side
    Pebble.sendAppMessage(dictConverted, function (e) {
        console.log('Sent config data to Pebble');

    }, function (e) {
        console.log('Failed to send config data');
    });
});
