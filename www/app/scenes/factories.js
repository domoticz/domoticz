define(['app'], function (app) {
    app.factory('sceneApi', function ($q, domoticzApi, dzTimeAndSun, permissions) {
        return {
            getScenes: getScenes,
            getSceneInfo: getSceneInfo,
            getSceneLog: getSceneLog,
            clearSceneLog: clearSceneLog
        };

        function getScenes() {
            return domoticzApi
                .sendRequest({type: 'scenes'})
                .then(function (data) {
                    dzTimeAndSun.updateData(data);

                    return data && data.result
                        ? data.result
                        : $q.reject(data);
                });
        }

        function getSceneInfo(idx) {
            return getScenes().then(function (scenes) {
                var scene = scenes.find(function (item) {
                    return item.idx === idx.toString();
                });

                return scene
                    ? scene
                    : $q.reject('Scene not found');
            });
        }

        function getSceneLog(idx) {
            return domoticzApi.sendRequest({
                type: 'scenelog',
                idx: idx
            }).then(function (response) {
                return response && response.status !== 'OK'
                    ? $q.reject(response)
                    : response.result || [];
            });
        }

        function clearSceneLog(idx) {
            if (!permissions.hasPermission('Admin')) {
                var message = $.t('You do not have permission to do that!');
                HideNotify();
                ShowNotify(message, 2500, true);
                return $q.reject(message);
            }

            return domoticzApi
                .sendCommand('clearscenelog', {
                    idx: idx
                })
        }
    });
});