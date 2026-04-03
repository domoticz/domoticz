define([
    'app',
    'dashboardDynamic/dashboardDynamicService'
], function(app) {
    'use strict';

    // Calls scope expression with $file when a file <input> changes
    app.directive('ddFileImport', function() {
        return {
            restrict: 'A',
            link: function(scope, el, attrs) {
                el.on('change', function(e) {
                    var file = (e.target || e.srcElement).files[0];
                    if (file) {
                        scope.$apply(function() {
                            scope.$eval(attrs.ddFileImport, { $file: file });
                        });
                    }
                });
            }
        };
    });

    // Adds drag-and-drop file support to any element
    app.directive('ddFileDrop', function() {
        return {
            restrict: 'A',
            link: function(scope, el, attrs) {
                el.on('dragover dragenter', function(e) {
                    e.preventDefault();
                    e.stopPropagation();
                    scope.$apply(function() { scope.dropHover = true; });
                });
                el.on('dragleave dragend', function() {
                    scope.$apply(function() { scope.dropHover = false; });
                });
                el.on('drop', function(e) {
                    e.preventDefault();
                    e.stopPropagation();
                    var dt   = e.dataTransfer || (e.originalEvent && e.originalEvent.dataTransfer);
                    var file = dt && dt.files && dt.files[0];
                    scope.$apply(function() {
                        scope.dropHover = false;
                        if (file) { scope.$eval(attrs.onDrop, { $file: file }); }
                    });
                });
            }
        };
    });

    app.controller('DdExportImportCtrl', [
        '$scope', '$uibModalInstance', '$http', '$timeout', '$q',
        'dashboardDynamicService', 'activeLayout', 'activeData', 'layouts', 'onImported',
        function($scope, $uibModalInstance, $http, $timeout, $q,
                 dashboardDynamicService, activeLayout, activeData, layouts, onImported) {

        $scope.tab          = 'export';
        $scope.exportJson   = '';
        $scope.importJson   = '';
        $scope.importTarget = 'new';
        $scope.importNewName = '';
        $scope.copied       = false;
        $scope.dropHover    = false;
        $scope.versionWarning = '';
        $scope.importError  = '';
        $scope.activeLayout = activeLayout;
        var _parsedPayload  = null;
        var _localRevision  = null;

        // ── Get local revision ─────────────────────────────────
        $http.get('json.htm?type=command&param=getversion').then(function(resp) {
            _localRevision = (resp.data && resp.data.Revision) || 0;
        });

        // ── Export ─────────────────────────────────────────────
        $scope.buildExportJson = function() {
            var payload = {
                name:             activeLayout.name,
                domoticzRevision: _localRevision || 0,
                layout:           activeData
            };
            $scope.exportJson = JSON.stringify(payload, null, 2);
        };

        $scope.copyToClipboard = function() {
            if (navigator.clipboard && navigator.clipboard.writeText) {
                navigator.clipboard.writeText($scope.exportJson).then(function() {
                    $scope.$apply(function() {
                        $scope.copied = true;
                        $timeout(function() { $scope.copied = false; }, 2000);
                    });
                });
            } else {
                var el = document.querySelector('.dd-eim-textarea');
                if (el) { el.select(); document.execCommand('copy'); }
                $scope.copied = true;
                $timeout(function() { $scope.copied = false; }, 2000);
            }
        };

        $scope.downloadFile = function() {
            var blob = new Blob([$scope.exportJson], { type: 'application/json' });
            var url  = URL.createObjectURL(blob);
            var a    = document.createElement('a');
            var name = (activeLayout.name || 'dashboard').replace(/[^a-z0-9_\-]/gi, '_');
            a.href = url;
            a.download = name + '.json';
            document.body.appendChild(a);
            a.click();
            document.body.removeChild(a);
            URL.revokeObjectURL(url);
        };

        // ── Import ─────────────────────────────────────────────
        $scope.validateImportJson = function() {
            $scope.importError    = '';
            $scope.versionWarning = '';
            _parsedPayload = null;

            var raw = ($scope.importJson || '').trim();
            if (!raw) { return; }

            var payload;
            try { payload = JSON.parse(raw); } catch(e) {
                $scope.importError = 'Invalid JSON: ' + e.message;
                return;
            }

            if (!payload.layout) {
                $scope.importError = 'Not a valid dashboard file (missing layout key)';
                return;
            }

            if (_localRevision && payload.domoticzRevision && payload.domoticzRevision > _localRevision) {
                $scope.versionWarning = 'Created on build ' + payload.domoticzRevision +
                    ', this system is build ' + _localRevision +
                    '. Some widgets may not work correctly.';
            }

            _parsedPayload = payload;
            $scope.importNewName = payload.name || '';
        };

        function loadFile(file) {
            if (!file) { return; }
            if (!file.name.match(/\.json$/i)) {
                $scope.importError = 'Please select a .json file';
                return;
            }
            var reader = new FileReader();
            reader.onload = function(e) {
                $scope.$apply(function() {
                    $scope.importJson = e.target.result;
                    $scope.validateImportJson();
                });
            };
            reader.readAsText(file);
        }

        $scope.triggerFilePick = function() {
            var el = document.getElementById('dd-eim-file');
            if (el) { el.value = ''; el.click(); }
        };

        $scope.onFileSelected = function(file) { loadFile(file); };
        $scope.onFileDrop     = function(file) { loadFile(file); };

        $scope.canImport = function() { return !!_parsedPayload; };

        $scope.doImport = function() {
            if (!_parsedPayload) { return; }
            var payload = _parsedPayload;
            var replace = $scope.importTarget === 'replace';
            var data    = angular.copy(payload.layout);

            if (data.widgets) {
                data.widgets.forEach(function(w) {
                    w.id = dashboardDynamicService.generateId();
                });
            }

            var result;
            if (replace) {
                result = onImported({ replace: true, meta: activeLayout, data: data });
            } else {
                var meta = {
                    id:        dashboardDynamicService.generateId(),
                    name:      ($scope.importNewName || payload.name || 'Imported').trim(),
                    isDefault: false
                };
                result = onImported({ replace: false, meta: meta, data: data });
            }

            $q.when(result).then(function() {
                $uibModalInstance.close();
            }).catch(function() {
                $scope.importError = 'Import failed. Please try again.';
            });
        };

        $scope.cancel = function() {
            $uibModalInstance.dismiss('cancel');
        };
    }]);
});
