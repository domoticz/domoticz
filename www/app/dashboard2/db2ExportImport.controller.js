define([
    'app',
    'dashboard2/dashboard2Service'
], function(app) {
    'use strict';

    app.controller('Db2ExportImportCtrl', [
        '$scope', '$uibModalInstance', '$http', '$timeout', '$q',
        'dashboard2Service', 'activeLayout', 'activeData', 'layouts', 'onImported',
        function($scope, $uibModalInstance, $http, $timeout, $q,
                 dashboard2Service, activeLayout, activeData, layouts, onImported) {

        $scope.tab        = 'export';
        $scope.exportTab  = 'clipboard';
        $scope.importTab  = 'clipboard';
        $scope.exportScope = 'current';
        $scope.exportJson = '';
        $scope.importJson = '';
        $scope.importTarget = 'new';
        $scope.copied     = false;
        $scope.versionWarning = '';
        $scope.importError = '';
        $scope.selectedFileName = '';
        $scope.activeLayout = activeLayout;
        var _parsedPayload = null;
        var _localRevision = null;

        // ── Get local revision ─────────────────────────────────
        $http.get('json.htm?type=command&param=getversion').then(function(resp) {
            _localRevision = (resp.data && resp.data.Revision) || 0;
        });

        // ── Export ─────────────────────────────────────────────
        $scope.buildExportJson = function() {
            var payload;
            if ($scope.exportScope === 'all') {
                // Export all layouts; activeData is already loaded for the current one
                payload = {
                    domoticzRevision: _localRevision || 0,
                    dashboards: layouts.map(function(l) {
                        if (l.id === activeLayout.id) {
                            return { name: l.name, isDefault: l.isDefault, layout: activeData };
                        }
                        return { name: l.name, isDefault: l.isDefault, layout: null };
                    })
                };
            } else {
                payload = {
                    name:             activeLayout.name,
                    domoticzRevision: _localRevision || 0,
                    layout:           activeData
                };
            }
            $scope.exportJson = JSON.stringify(payload, null, 2);
        };

        $scope.$watch('exportScope', function() {
            if ($scope.exportTab === 'clipboard') { $scope.buildExportJson(); }
        });

        $scope.copyToClipboard = function() {
            if (navigator.clipboard && navigator.clipboard.writeText) {
                navigator.clipboard.writeText($scope.exportJson).then(function() {
                    $scope.$apply(function() {
                        $scope.copied = true;
                        $timeout(function() { $scope.copied = false; }, 2000);
                    });
                });
            } else {
                // Fallback: select the textarea
                var el = document.querySelector('.db2-eim-textarea');
                if (el) { el.select(); document.execCommand('copy'); }
                $scope.copied = true;
                $timeout(function() { $scope.copied = false; }, 2000);
            }
        };

        $scope.downloadFile = function() {
            $scope.buildExportJson();
            var blob = new Blob([$scope.exportJson], { type: 'application/json' });
            var url  = URL.createObjectURL(blob);
            var a    = document.createElement('a');
            var name = ($scope.exportScope === 'all' ? 'all-dashboards' : (activeLayout.name || 'dashboard'))
                       .replace(/[^a-z0-9_\-]/gi, '_');
            a.href = url;
            a.download = name + '.json';
            document.body.appendChild(a);
            a.click();
            document.body.removeChild(a);
            URL.revokeObjectURL(url);
        };

        // ── Import ─────────────────────────────────────────────
        $scope.validateImportJson = function() {
            $scope.importError   = '';
            $scope.versionWarning = '';
            _parsedPayload = null;

            var raw = ($scope.importJson || '').trim();
            if (!raw) { return; }

            var payload;
            try { payload = JSON.parse(raw); } catch(e) {
                $scope.importError = 'Invalid JSON: ' + e.message;
                return;
            }

            // Accept single dashboard {name, layout} or multi {dashboards:[]}
            var hasSingle = payload.layout;
            var hasMulti  = Array.isArray(payload.dashboards);
            if (!hasSingle && !hasMulti) {
                $scope.importError = 'Not a valid dashboard file (missing layout or dashboards key)';
                return;
            }

            if (_localRevision && payload.domoticzRevision && payload.domoticzRevision > _localRevision) {
                $scope.versionWarning = 'Created on build ' + payload.domoticzRevision +
                    ', this system is build ' + _localRevision +
                    '. Some widgets may not work correctly.';
            }

            _parsedPayload = payload;
        };

        $scope.triggerFilePick = function() {
            var el = document.getElementById('db2-eim-file');
            if (el) { el.value = ''; el.click(); }
        };

        $scope.onFileSelected = function(file) {
            if (!file) { return; }
            $scope.selectedFileName = file.name;
            var reader = new FileReader();
            reader.onload = function(e) {
                $scope.$apply(function() {
                    $scope.importJson = e.target.result;
                    $scope.validateImportJson();
                });
            };
            reader.readAsText(file);
        };

        $scope.canImport = function() {
            return !!_parsedPayload;
        };

        $scope.doImport = function() {
            if (!_parsedPayload) { return; }
            var payload = _parsedPayload;
            var replace = $scope.importTarget === 'replace';

            // Normalise to array of {name, layout, isDefault}
            var items = payload.dashboards || [{ name: payload.name, layout: payload.layout, isDefault: false }];

            // Process each dashboard sequentially
            var chain = $q.when();
            items.forEach(function(item) {
                if (!item.layout) { return; } // skip placeholders in multi-export
                chain = chain.then(function() {
                    var data = angular.copy(item.layout);
                    if (data.widgets) {
                        data.widgets.forEach(function(w) {
                            w.id = dashboard2Service.generateId();
                        });
                    }
                    if (replace && items.length === 1) {
                        // Replace current: keep same id/name
                        return onImported({ replace: true, meta: activeLayout, data: data });
                    } else {
                        var meta = {
                            id:        dashboard2Service.generateId(),
                            name:      (item.name || 'Imported') + (items.length === 1 ? ' (imported)' : ''),
                            isDefault: false
                        };
                        return onImported({ replace: false, meta: meta, data: data });
                    }
                });
            });

            chain.then(function() {
                $uibModalInstance.close();
            }).catch(function() {
                $scope.importError = 'Import failed. Please try again.';
            });
        };

        $scope.cancel = function() {
            $uibModalInstance.dismiss('cancel');
        };

        // Build initial export JSON
        $scope.buildExportJson();
    }]);
});
