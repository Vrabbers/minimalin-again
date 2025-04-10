"use strict"

var Clay = require('pebble-clay');
var clayConfig = require('./config.json');
var clay = new Clay(clayConfig, null, { autoHandleEvents: false });

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
                    callbackError("geocode failed: features.length != 1")
                    return;
                }
                var coords = resp.features[0].geometry.coordinates;
                var lat = coords[1];
                var lon = coords[0];
                console.log("geocode success: long:" + lon + ", lat:" + lat);
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

// Pebble.addEventListener('showConfiguration', function() {
//   var URL = 'https://lanrat.github.io/minimalin-reborn/';
//   var config = Config('config');
//   var params = config.load();
//   params.platform = Pebble.getActiveWatchInfo().platform;
//   var query = '?config=' + encodeURIComponent(JSON.stringify(params));
//   Pebble.openURL(URL + query);
// });

// Pebble.addEventListener('webviewclosed', function(e) {
//   if(e.response){
//     var config = Config('config');
//     var configData = JSON.parse(decodeURIComponent(e.response));
//     config.store(configData);
//     var mapping = {
//       minute_hand_color: 'AppKeyMinuteHandColor',
//       hour_hand_color: 'AppKeyHourHandColor',
//       background_color: 'AppKeyBackgroundColor',
//       time_color: 'AppKeyTimeColor',
//       info_color: 'AppKeyInfoColor',
//       date_displayed: 'AppKeyDateDisplayed',
//       health_enabled: 'AppKeyHealthEnabled',
//       weather_enabled: 'AppKeyWeatherEnabled',
//       bluetooth_icon: 'AppKeyBluetoothIcon',
//       battery_displayed_at: 'AppKeyBatteryDisplayedAt',
//       temperature_unit: 'AppKeyTemperatureUnit',
//       refresh_rate: 'AppKeyRefreshRate',
//       rainbow_mode: 'AppKeyRainbowMode',
//       vibrate_on_the_hour: 'AppKeyVibrateOnTheHour',
//       military_time: 'AppKeyMilitaryTime'
//     };
//     var dict = { AppKeyConfig: 1 };
//     for(var key in mapping){
//       if (mapping.hasOwnProperty(key)) {
//         dict[mapping[key]] = configData[key];
//       }
//     }
//     dict['AppKeyConfig'] = 1;
//     Pebble.sendAppMessage(dict, function() {}, function() {});
//   }
// });

