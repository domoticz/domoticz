define(['app'], function (app) {
    app.factory('deviceNotificationsApi', function (domoticzApi) {
        return {
            getNotifications: getNotifications,
            getNotificationTypes: getNotificationTypes,
            addNotification: addNotification,
            updateNotification: updateNotification,
            deleteNotification: deleteNotification,
            clearNotifications: clearNotifications
        };

        function getNotifications(deviceIdx) {
            return domoticzApi.sendRequest({
                type: 'notifications',
                idx: deviceIdx
            }).then(function (response) {
                return response && response.status !== 'OK'
                    ? $q.reject(response)
                    : response;
            });
        }

        function getNotificationTypes(deviceIdx) {
            return domoticzApi.sendCommand('getnotificationtypes', {
                idx: deviceIdx
            }).then(function (response) {
                return response.result;
            });
        }

        function addNotification(deviceIdx, options) {
            return domoticzApi.sendCommand('addnotification', Object.assign({
                idx: deviceIdx
            }, options));
        }

        function updateNotification(deviceIdx, idx, options) {
            return domoticzApi.sendCommand('updatenotification', Object.assign({
                idx: idx,
                devidx: deviceIdx
            }, options));
        }

        function deleteNotification(idx) {
            return domoticzApi.sendCommand('deletenotification', {
                idx: idx
            });
        }

        function clearNotifications(deviceIdx) {
            return domoticzApi.sendCommand('clearnotifications', {
                idx: deviceIdx
            });
        }
    });
});