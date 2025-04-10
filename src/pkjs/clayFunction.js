module.exports = function (_) {
    var clayConfig = this;

    function toggleWeatherVisible(visible) {
        var items = clayConfig.getItemsByGroup("weather");
        for (var i = 0; i < items.length; i++) {
            if (visible)
                items[i].show();
            else
                items[i].hide();
        }

    }

    clayConfig.on(clayConfig.EVENTS.AFTER_BUILD, function () {
        clayConfig.getItemByMessageKey("AppKeyWeatherEnabled").on("change", function () {
            var enabled = this.get();
            toggleWeatherVisible(enabled);
        }).trigger("change");
    });
}