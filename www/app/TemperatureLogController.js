define([ "app" ], function(app) {
    app.controller("TemperatureLogController", [ "$routeParams", "$scope", "$location", function($routeParams, $scope, $location) {
        var ctrl = this;
        ctrl.idx = $routeParams.idx;
        $.ajax({
            url: "json.htm?type=devices&rid=" + ctrl.idx,
            async: false,
            dataType: "json",
            success: function(data) {
                if (typeof data.result != "undefined" && data.result.length > 0) {
                    ctrl.device = data.result[0];
                    ShowTempLog("#templog", "BackToTemperatures", ctrl.idx, escape(ctrl.device.Name));
                }
            }
        });
        BackToTemperatures = function() {
            $location.url("/Temperature");
            $scope.$apply();
        };
    } ]);
});