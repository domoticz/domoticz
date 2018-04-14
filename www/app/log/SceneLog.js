define(['app', 'scenes/factories', 'log/components'], function (app) {
    app.controller('SceneLogController', function ($routeParams, sceneApi) {
        var vm = this;

        vm.clearLog = clearLog;

        init();

        function init() {
            vm.sceneIdx = $routeParams.id;

            sceneApi.getSceneInfo(vm.sceneIdx).then(function (scene) {
                vm.pageName = scene.Name;
            });

            refreshLog();
        }

        function refreshLog() {
            sceneApi.getSceneLog(vm.sceneIdx).then(function (data) {
                vm.log = data
            });
        }

        function clearLog() {
            bootbox.confirm($.t('Are you sure to delete the Log?\n\nThis action can not be undone!'), function (result) {
                if (result !== true) {
                    return;
                }

                sceneApi.clearSceneLog(vm.sceneIdx)
                    .then(refreshLog)
                    .catch(function () {
                        HideNotify();
                        ShowNotify($.t('Problem clearing the Log!'), 2500, true);
                    });
            });
        }
    });
});