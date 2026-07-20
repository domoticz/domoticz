define(['app', 'events/factories'], function (app) {

    app.component('eventViewer', {
        bindings: {
            event: '=',
            onUpdate: '&',
            onDelete: '&'
        },
        templateUrl: 'app/events/EventViewer.html',
        controller: function ($scope, $element, $q, $timeout, $uibModal, bootbox, domoticzEventsApi, blocklyToolbox) {
            var vm = this;
            var aceEditor;
            var blocklyWorkspace;
            var debounceTimer;
            var statusBarEl;
            var headerResizeObserver;
            var scriptHeaderEl;
            var scriptContentEl;
            var syncContentOffsetFrame;
            var usesWindowResizeFallback;
            var isDestroyed;

            var ACE_SETTINGS_KEY = 'domoticz_ace_settings';
            var DEFAULT_SETTINGS = {
                theme: 'ace/theme/xcode',
                fontSize: 14,
                wordWrap: false
            };

            var THEMES = [
                { id: 'ace/theme/chrome', name: 'Chrome', dark: false },
                { id: 'ace/theme/eclipse', name: 'Eclipse', dark: false },
                { id: 'ace/theme/github', name: 'GitHub', dark: false },
                { id: 'ace/theme/solarized_light', name: 'Solarized Light', dark: false },
                { id: 'ace/theme/tomorrow', name: 'Tomorrow', dark: false },
                { id: 'ace/theme/xcode', name: 'XCode', dark: false },
                { id: 'ace/theme/monokai', name: 'Monokai', dark: true },
                { id: 'ace/theme/cobalt', name: 'Cobalt', dark: true },
                { id: 'ace/theme/solarized_dark', name: 'Solarized Dark', dark: true },
                { id: 'ace/theme/tomorrow_night', name: 'Tomorrow Night', dark: true },
                { id: 'ace/theme/twilight', name: 'Twilight', dark: true },
                { id: 'ace/theme/terminal', name: 'Terminal', dark: true }
            ];

            vm.$onInit = init;
            vm.setEventState = setEventState;
            vm.saveEvent = saveEvent;
            vm.deleteEvent = deleteEvent;
            vm.importEvent = importEvent;
            vm.exportEvent = exportEvent;
            vm.markEventAsUpdated = markEventAsUpdated;
            vm.isTriggerAvailable = isTriggerAvailable;
            vm.themes = THEMES;
            vm.setTheme = setTheme;
            vm.increaseFontSize = increaseFontSize;
            vm.decreaseFontSize = decreaseFontSize;
            vm.toggleWordWrap = toggleWordWrap;

            function loadSettings() {
                try {
                    var stored = localStorage.getItem(ACE_SETTINGS_KEY);
                    return stored ? angular.extend({}, DEFAULT_SETTINGS, JSON.parse(stored)) : angular.copy(DEFAULT_SETTINGS);
                } catch (e) {
                    return angular.copy(DEFAULT_SETTINGS);
                }
            }

            function saveSettings(settings) {
                try {
                    localStorage.setItem(ACE_SETTINGS_KEY, JSON.stringify(settings));
                } catch (e) {
                    // localStorage not available
                }
            }

            function setTheme(themeId) {
                if (!aceEditor) return;
                vm.aceSettings.theme = themeId;
                aceEditor.setTheme(themeId);
                saveSettings(vm.aceSettings);
            }

            function increaseFontSize() {
                if (!aceEditor) return;
                vm.aceSettings.fontSize = Math.min(vm.aceSettings.fontSize + 1, 32);
                aceEditor.setFontSize(vm.aceSettings.fontSize);
                saveSettings(vm.aceSettings);
            }

            function decreaseFontSize() {
                if (!aceEditor) return;
                vm.aceSettings.fontSize = Math.max(vm.aceSettings.fontSize - 1, 8);
                aceEditor.setFontSize(vm.aceSettings.fontSize);
                saveSettings(vm.aceSettings);
            }

            function toggleWordWrap() {
                if (!aceEditor) return;
                vm.aceSettings.wordWrap = !vm.aceSettings.wordWrap;
                aceEditor.getSession().setUseWrapMode(vm.aceSettings.wordWrap);
                saveSettings(vm.aceSettings);
            }

            function init() {
                vm.aceSettings = loadSettings();

                vm.eventTypes = [
                    { value: 'All', label: 'All' },
                    { value: 'Device', label: 'Device' },
                    { value: 'Security', label: 'Security' },
                    { value: 'Time', label: 'Time' },
                    { value: 'UserVariable', label: 'User variable' }
                ];

                $q
                    .resolve(vm.event.interpreter
                        ? Object.assign({}, vm.event)
                        : domoticzEventsApi.loadEvent(vm.event.id)
                    )
                    .then(function (eventData) {
                        eventData.eventstatus = String(eventData.eventstatus);
                        vm.eventData = eventData;

                        if (eventData.interpreter === 'Blockly') {
                            initBlockly(eventData);
                        } else {
                            initAce(eventData)
                        }

                        bindScriptHeaderResize();
                        syncContentOffsetWithHeader();

                        $element.on('keydown', function(event) {
                            if ((event.ctrlKey || event.metaKey) && String.fromCharCode(event.which).toLowerCase() === 's') {

                                if (vm.event.isChanged) {
                                    $scope.$apply(saveEvent);
                                }

                                event.preventDefault();
                                return false
                            }
                        });
                    });

                $scope.$on('$destroy', function () {
                    isDestroyed = true;
                    if (debounceTimer) {
                        $timeout.cancel(debounceTimer);
                    }
                    if (aceEditor) {
                        aceEditor.destroy();
                        aceEditor = null;
                    }
                    if (blocklyWorkspace) {
                        blocklyWorkspace.dispose();
                        blocklyWorkspace = null;
                    }
                    if (statusBarEl && statusBarEl.parentNode) {
                        statusBarEl.parentNode.removeChild(statusBarEl);
                        statusBarEl = null;
                    }
                    if (headerResizeObserver) {
                        headerResizeObserver.disconnect();
                        headerResizeObserver = null;
                    }
                    if (syncContentOffsetFrame) {
                        window.cancelAnimationFrame(syncContentOffsetFrame);
                        syncContentOffsetFrame = null;
                    }
                    scriptHeaderEl = null;
                    scriptContentEl = null;
                    if (usesWindowResizeFallback) {
                        angular.element(window).off('resize', requestContentOffsetSync);
                        usesWindowResizeFallback = false;
                    }
                    $element.off('keydown');
                });
            }

            function bindScriptHeaderResize() {
                // The component can be destroyed while loadEvent() is still in flight,
                // in which case $destroy has already run and there is nothing to clean up after us.
                if (isDestroyed) {
                    return;
                }

                scriptHeaderEl = $element[0].querySelector('.events-editor-file__header--editor');
                scriptContentEl = $element[0].querySelector('.events-editor-file__content--editor');

                if (!scriptHeaderEl || !scriptContentEl) {
                    return;
                }

                if (typeof window.ResizeObserver !== 'undefined') {
                    headerResizeObserver = new ResizeObserver(requestContentOffsetSync);
                    headerResizeObserver.observe(scriptHeaderEl);
                } else {
                    usesWindowResizeFallback = true;
                    angular.element(window).on('resize', requestContentOffsetSync);
                }
            }

            function requestContentOffsetSync() {
                if (syncContentOffsetFrame) {
                    window.cancelAnimationFrame(syncContentOffsetFrame);
                }

                syncContentOffsetFrame = window.requestAnimationFrame(function () {
                    syncContentOffsetFrame = null;
                    syncContentOffsetWithHeader();
                });
            }

            function syncContentOffsetWithHeader() {
                if (!scriptHeaderEl || !scriptContentEl) {
                    return;
                }

                var headerHeight = scriptHeaderEl.offsetHeight;

                // Hidden tabs (ng-show) measure as 0, which would collapse the offset and
                // let the header overlap the editor once the tab is shown again.
                if (headerHeight === 0) {
                    return;
                }

                scriptContentEl.style.top = headerHeight + 'px';

                // Ace 1.2.2 only recalculates its size on window resize, so a container
                // height change has to be pushed to it explicitly.
                if (aceEditor) {
                    aceEditor.resize();
                }
            }

            function isTriggerAvailable() {
                return vm.eventData && ['Blockly', 'Lua', 'Python'].includes(vm.eventData.interpreter);
            }

            function markEventAsUpdated() {
                vm.event.isChanged = true;
            }

            function setEventState(isEnabled) {
                var newState = isEnabled ? '1' : '0';

                if (vm.eventData.eventstatus === newState) {
                    return;
                }

                vm.eventData.eventstatus = newState;
                vm.event.isChanged = true;
            }

            function saveEvent() {
                var event = Object.assign({}, vm.eventData, vm.event.isNew ? { id: undefined } : {});

                if (event.interpreter === 'dzVents') {
                    if (event.name.indexOf('.lua') >= 0) {
                        return bootbox.alert('You cannot have .lua in the name.');
                    }

                    if ([
                        'Device', 'Domoticz', 'dzVents', 'EventHelpers', 'HistoricalStorage',
                        'persistence', 'Time', 'TimedCommand', 'Timer', 'Utils', 'Security',
                        'lodash', 'JSON', 'Variable'
                    ].includes(event.name)) {
                        return bootbox.alert('You cannot use these names for your scripts: Device, Domoticz, dzVents, EventHelpers, HistoricalStorage, persistence, Time, TimedCommand, Utils or Variable. It interferes with the dzVents subsystem. Please rename your script.');
                    }
                }

                return $q(function (resolve, reject) {
                    if (event.interpreter !== 'Blockly') {
                        event.xmlstatement = aceEditor.getValue();
                        resolve(event);
                    } else {
                        require(['events/blockly_xml_parser'], function (xmlParser) {
                            try {
                                var xml = Blockly.Xml.workspaceToDom(blocklyWorkspace);

                                event.xmlstatement = Blockly.Xml.domToText(xml);
                                event.logicarray = JSON.stringify(xmlParser.parseXml(xml));
                                resolve(event);
                            } catch (e) {
                                reject(e.message)
                            }
                        })
                    }
                })
                    .then(domoticzEventsApi.updateEvent)
                    .then(function () {
                        vm.event.isChanged = false;
                        vm.onUpdate({
                            event: Object.assign({}, vm.event, {
                                name: event.name,
                                eventstatus: event.eventstatus
                            })
                        })
                    })
                    .catch(bootbox.alert);
            }

            function deleteEvent() {
                var message = 'Are you sure to delete this Event?\n\nThis action can not be undone...';

                return $q
                    .resolve(vm.event.isNew
                        ? true
                        : bootbox
                            .confirm(message)
                            .then(domoticzEventsApi.deleteEvent.bind(domoticzEventsApi, vm.event.id))
                    )
                    .then(function() {
                        ShowNotify($.t('Script successfully removed'), 1500);
                        vm.onDelete({ event: vm.event });
                    });
            }

            function importEvent() {
                $uibModal.open({
                    templateUrl: 'app/events/importEventModal.html'
                }).result.then(function (scriptData) {
                    try {
                        var xml = Blockly.Xml.textToDom(scriptData);
                        Blockly.Xml.domToWorkspace(xml, blocklyWorkspace);

                        vm.markEventAsUpdated();
                    } catch (e) {
                        ShowNotify($.t('Error importing script: data is not valid'), 2500, true);
                    }
                });
            }

            function exportEvent() {
                var xml = Blockly.Xml.workspaceToDom(blocklyWorkspace);
                var scope = $scope.$new(true);

                scope.scriptData = Blockly.Xml.domToText(xml);

                $uibModal.open({
                    scope: scope,
                    templateUrl: 'app/events/exportEventModal.html'
                });
            }

            function initAce(eventData) {
                require(['ace', 'ace-language-tools', 'ace-searchbox', 'ace-statusbar'], function () {
                    ace.config.set('workerPath', '../js/ace');
                    var element = $element.find('.js-script-content')[0];

                    aceEditor = ace.edit(element);
                    var interpreter = eventData.interpreter === 'dzVents'
                        ? 'lua'
                        : eventData.interpreter;

                    aceEditor.setOptions({
                        enableBasicAutocompletion: true,
                        enableSnippets: true,
                        enableLiveAutocompletion: true
                    });

                    // Apply persisted settings
                    aceEditor.setTheme(vm.aceSettings.theme);
                    aceEditor.setFontSize(vm.aceSettings.fontSize);
                    aceEditor.getSession().setUseWrapMode(vm.aceSettings.wordWrap);

                    aceEditor.setValue(eventData.xmlstatement);
                    aceEditor.getSession().setMode('ace/mode/' + interpreter.toLowerCase());
                    aceEditor.gotoLine(1);
                    aceEditor.scrollToLine(1, true, true);

                    // Status bar
                    var StatusBar = ace.require('ace/ext/statusbar').StatusBar;
                    statusBarEl = document.createElement('div');
                    statusBarEl.className = 'ace-statusbar';
                    element.parentNode.appendChild(statusBarEl);
                    element.style.bottom = '21px';
                    new StatusBar(aceEditor, statusBarEl);
                    $timeout(function () { aceEditor.resize(); }, 0);

                    // Debounced change handler
                    aceEditor.on('change', function () {
                        markEventAsUpdated();
                        if (debounceTimer) {
                            $timeout.cancel(debounceTimer);
                        }
                        debounceTimer = $timeout(function () {
                            // digest cycle runs automatically via $timeout
                        }, 300);
                    });

                    $scope.$apply();
                });
            }

            function initBlockly(eventData) {
                var container = $element.find('.js-script-content')[0];

                require(['blockly', 'app/events/blockly_blocks_domoticz.js'], function () {
                    var suppressChangeEvents = false;
                    var needsRerender = false;

                    blocklyToolbox.get().then(function (toolbox) {
                        // If the container is hidden at init time (e.g. inactive tab), blocks
                        // will be measured with zero dimensions.  Flag for re-render on first
                        // time the workspace becomes visible.
                        needsRerender = container.offsetWidth === 0;

                        blocklyWorkspace = Blockly.inject(container, {
                            path: './',
                            toolbox: toolbox,
                            sounds: false,
                            zoom: {
                                controls: true,
                                wheel: true,
                                startScale: 1.0,
                                maxScale: 2,
                                minScale: 0.3,
                                scaleSpeed: 1.2
                            },
                            trashcan: true,
                        });

                        blocklyWorkspace.clear();

                        if (eventData.xmlstatement) {
                            var xml = Blockly.Xml.textToDom(eventData.xmlstatement);
                            Blockly.Xml.domToWorkspace(xml, blocklyWorkspace);
                        }

                        $('body > .blocklyToolboxDiv').appendTo(container);

                        $timeout(function () {
                            //Timeout is required as Blockly fires change event right after it was initialized
                            blocklyWorkspace.addChangeListener(function () {
                                if (!suppressChangeEvents) {
                                    markEventAsUpdated();
                                    $scope.$apply();
                                }
                            });
                        }, 200);

                        // Supported only in Chrome 67+
                        if (window.ResizeObserver) {
                            var ro = new ResizeObserver(function (entries) {
                                if (entries.length && entries[0].contentRect.width > 0) {
                                    Blockly.svgResize(blocklyWorkspace);

                                    // Re-render blocks the first time the workspace becomes
                                    // visible after having been initialised in a hidden container.
                                    // svgResize alone cannot fix block dimensions that were
                                    // measured as zero while the container was display:none.
                                    if (needsRerender) {
                                        needsRerender = false;
                                        suppressChangeEvents = true;
                                        var savedXml = Blockly.Xml.workspaceToDom(blocklyWorkspace);
                                        blocklyWorkspace.clear();
                                        Blockly.Xml.domToWorkspace(savedXml, blocklyWorkspace);
                                        suppressChangeEvents = false;
                                    }
                                }
                            });

                            ro.observe(container);

                            $scope.$on('$destroy', function() {
                                ro.disconnect();
                            });
                        }
                    });

                    $scope.$apply();
                });
            }
        }
    });
});
