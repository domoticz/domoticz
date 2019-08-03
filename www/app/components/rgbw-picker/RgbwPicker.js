define(['app'], function (app) {
    app.component('rgbwPicker', {
        bindings: {
            color: '=',
            level: '=',
            dimmerType: '<',
            maxDimmerLevel: '<',
            colorSettingsType: '<'
        },
        templateUrl: 'app/components/rgbw-picker/RgbwPicker.html',
        controller: RgbwPickerController
    });

    function RgbwPickerController($scope) {
        var vm = this;
        var currentValue;

        var getColor = function () {
            return vm.color + vm.level;
        };

        var onColorChange = function (idx, color, level) {
            vm.color = color;
            vm.level = level;
            currentValue = getColor();

            $scope.$apply();
        };

        vm.$onInit = function () {
            $scope.$watch(getColor, vm.render);
        };

        vm.render = function (value) {
            if (value === currentValue) {
                return;
            }

            ShowRGBWPicker(
                '#rgbw-picker',
                null,
                0,
                vm.maxDimmerLevel || 100,
                vm.level,
                vm.color,
                vm.colorSettingsType,
                vm.dimmerType,
                onColorChange
            );

            currentValue = value
        }
    }
});
