define(['app', 'log/components/DeviceTextLogTable'], function (app) {
    app.controller('SceneLogController', function ($scope, $routeParams, domoticzApi, dzTimeAndSun, permissions) {
        var vm = this;

        vm.clearLog = clearLog;

        init();

        function getScenes() {
            return domoticzApi
                .sendCommand('getscenes',{})
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
            return domoticzApi
                    .sendCommand('getscenelog', {
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

        function init() {
            vm.autoRefresh = true;
            vm.sceneIdx = $routeParams.id;

            getSceneInfo(vm.sceneIdx).then(function (scene) {
                vm.pageName = scene.Name;
            });

            $scope.$on('scene_update', function(event, scene) {
                if (vm.autoRefresh && scene.idx === vm.sceneIdx) {
                    refreshLog();
                }
            });

            refreshLog();
        }

        function refreshLog() {
            getSceneLog(vm.sceneIdx).then(function (data) {
                vm.log = data
            });
        }

        function clearLog() {
            bootbox.confirm($.t('Are you sure to delete the Log?\n\nThis action can not be undone!'), function (result) {
                if (result !== true) {
                    return;
                }

                clearSceneLog(vm.sceneIdx)
                    .then(refreshLog)
                    .catch(function () {
                        HideNotify();
                        ShowNotify($.t('Problem clearing the Log!'), 2500, true);
                    });
            });
        }
    });
});
