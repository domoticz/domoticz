define(['app'], function (app) {

    function valueKeyForDevice(device) {
        if (device.SubType === 'Lux') {
            return 'lux'
        } else if (device.Type === 'Usage') {
            return 'u'
        } else if (device.Type === 'Rain') {
            return 'mm'
        } else {
            return 'v';
        }
    }

    function sensorTypeForDevice(device) {
        if (['Custom Sensor', 'Waterflow', 'Percentage'].includes(device.SubType)) {
            return 'Percentage';
        } else if (device.Type.includes('Rain') === true) {
            return 'rain';
        } else if (device.Type.includes('Fan') === true) {
            return 'fan';
        } else if (device.Type.includes('Wind') === true) {
            return 'wind';
        } else if (device.Type.includes('UV') === true) {
            return 'uv';
        } else {
            return 'counter';
        }
    }

    function sensorNameForDevice(device) {
        if (device.Type === 'Usage') {
            return $.t('Usage');
		}
        else if (device.Type === 'Rain') {
            return $.t('Rain');
        } else {
            return $.t(device.SubType)
        }
    }

    app.constant('domoticzGlobals', {
        Get5MinuteHistoryDaysGraphTitle: Get5MinuteHistoryDaysGraphTitle,
        chartPointClickNew: chartPointClickNew,
        valueKeyForDevice: valueKeyForDevice,
        sensorTypeForDevice: sensorTypeForDevice,
        sensorNameForDevice: sensorNameForDevice,
        axisTitleForDevice: function(device) {
            const sensorName = sensorNameForDevice(device);
            const unit = device.getUnit();
            return device.SubType === 'Custom Sensor' ? unit : sensorName + ' (' + unit + ')';
        }
    });

    app.factory('domoticzDataPointApi', function ($q, domoticzApi, permissions) {
        return {
            deletePoint: deletePoint,
            spreadPoint: spreadPoint
        };

        function deletePoint(deviceIdx, point, isShort, timezone) {
            return $q(function (resolve, reject) {
                if (!permissions.hasPermission('Admin')) {
                    HideNotify();
                    ShowNotify($.t('You do not have permission to do that!'), 2500, true);
                    return;
                }
                var dateFormat = isShort ? '%Y-%m-%d %H:%M:%S' : '%Y-%m-%d';
                var dateString = timezone !== undefined
                    ? new Highcharts.Time({ timezone: timezone }).dateFormat(dateFormat, point.x)
                    : Highcharts.dateFormat(dateFormat, point.x);

                var message = $.t("Are you sure to remove this value at") + " ?:\n\n" + $.t("Date") + ": " + dateString + " \n" + $.t("Value") + ": " + point.y;

                bootbox.confirm(message, function (result) {
                    if (result !== true) {
                        return;
                    }
                    domoticzApi
                        .sendCommand('deletedatapoint', {
                            idx: deviceIdx,
                            date: dateString
                        })
                        .then(resolve)
                        .catch(function () {
                            HideNotify();
                            ShowNotify($.t('Problem deleting data point!'), 2500, true);
                            reject();
                        });
                });
            });
        }

        function spreadPoint(deviceIdx, point, timezone) {
            return $q(function (resolve, reject) {
                if (!permissions.hasPermission('Admin')) {
                    HideNotify();
                    ShowNotify($.t('You do not have permission to do that!'), 2500, true);
                    return;
                }
                var dateString = timezone !== undefined
                    ? new Highcharts.Time({ timezone: timezone }).dateFormat('%Y-%m-%d', point.x)
                    : Highcharts.dateFormat('%Y-%m-%d', point.x);
                domoticzApi
                    .sendCommand('spreadcounterspike', {
                        idx: deviceIdx,
                        date: dateString
                    })
                    .then(resolve)
                    .catch(function () {
                        HideNotify();
                        ShowNotify($.t('Problem spreading data point!'), 2500, true);
                        reject();
                    });
            });
        }
    });
});
