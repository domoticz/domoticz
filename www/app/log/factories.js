define(['app'], function (app) {

    function valueKeyForDevice(device) {
        if (device.SubType === 'Lux') {
            return 'lux'
        } else if (device.Type === 'Usage') {
            return 'u'
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
            deletePoint: deletePoint
        };

        function deletePoint(deviceIdx, point, isShort, timezoneOffset=0) {
            return $q(function (resolve, reject) {
                if (!permissions.hasPermission('Admin')) {
                    HideNotify();
                    ShowNotify($.t('You do not have permission to do that!'), 2500, true);
                    reject();
                }

                var dateString = isShort
                    ? Highcharts.dateFormat('%Y-%m-%d %H:%M:%S', point.x - timezoneOffset*60*1000)
                    : Highcharts.dateFormat('%Y-%m-%d', point.x - timezoneOffset*60*1000);

                var message = $.t("Are you sure to remove this value at") + " ?:\n\n" + $.t("Date") + ": " + dateString + " \n" + $.t("Value") + ": " + point.y;

                bootbox.confirm(message, function (result) {
                    if (result !== true) {
                        reject();
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
    });
});
