module.exports = function (_) {
    var clayConfig = this;

    var themes = {
        darkBwTheme: {
            AppKeyHourHandColor: "#FFFFFF",
            AppKeyMinuteHandColor: "#FFFFFF",
            AppKeyTimeColor: "#FFFFFF",
            AppKeyBackgroundColor: "#000000",
            AppKeyInfoColor: "#FFFFFF",
        },
        lightBwTheme: {
            AppKeyHourHandColor: "#000000",
            AppKeyMinuteHandColor: "#000000",
            AppKeyTimeColor: "#000000",
            AppKeyBackgroundColor: "#FFFFFF",
            AppKeyInfoColor: "#000000",
        },
        defaultColorTheme: {
            AppKeyHourHandColor: "#FF0000",
            AppKeyMinuteHandColor: "#FFFFFF",
            AppKeyTimeColor: "#AAAAAA",
            AppKeyBackgroundColor: "#000000",
            AppKeyInfoColor: "#555555",
            AppKeyRainbowMode: false
        },
        rainbowColorTheme: {
            AppKeyHourHandColor: "#FFFFFF",
            AppKeyMinuteHandColor: "#FFFFFF",
            AppKeyTimeColor: "#AAAAAA",
            AppKeyBackgroundColor: "#000000",
            AppKeyInfoColor: "#555555",
            AppKeyRainbowMode: true
        },
        nightColorTheme: {
            AppKeyHourHandColor: "#55AAAA",
            AppKeyMinuteHandColor: "#FFFFFF",
            AppKeyTimeColor: "#FFFF00",
            AppKeyBackgroundColor: "#000055",
            AppKeyInfoColor: "#5555AA",
            AppKeyRainbowMode: false
        },
        lobsterColorTheme: {
            AppKeyHourHandColor: "#FF5500",
            AppKeyMinuteHandColor: "#FFFFFF",
            AppKeyTimeColor: "#AA5500",
            AppKeyBackgroundColor: "#AAAAAA",
            AppKeyInfoColor: "#555555",
            AppKeyRainbowMode: false
        }
    };

    function toggleWeatherVisible(visible) {
        var items = clayConfig.getItemsByGroup("weather");
        for (var i = 0; i < items.length; i++) {
            if (visible)
                items[i].show();
            else
                items[i].hide();
        }
    }

    function themeButtonClicked() {
        var theme = themes[this.id];
        for (var appKey in theme) {
            clayConfig.getItemByMessageKey(appKey).set(theme[appKey]);
        }
    }

    clayConfig.on(clayConfig.EVENTS.AFTER_BUILD, function () {
        clayConfig.getItemByMessageKey("AppKeyWeatherEnabled").on("change", function () {
            var enabled = this.get();
            toggleWeatherVisible(enabled);
        }).trigger("change");

        var rainbowButton = clayConfig.getItemByMessageKey("AppKeyRainbowMode");
        if (rainbowButton) {
            rainbowButton.on("change", function () {
                var min = clayConfig.getItemByMessageKey("AppKeyMinuteHandColor");
                if (this.get())
                    min.hide();
                else
                    min.show();
            });
        }

        var themeButtons = clayConfig.getItemsByGroup("themeButtons");
        for (var i = 0; i < themeButtons.length; i++) {
            themeButtons[i].on("click", themeButtonClicked);
        }
    });
}