define(['app', 'events/factories'], function (app) {

    app.component('eventViewer', {
        bindings: {
            event: '=',
            onUpdate: '&',
            onDelete: '&'
        },
        templateUrl: 'app/events/EventViewer.html',
        controller: function ($scope, $element, $q, $timeout, $modal, bootbox, domoticzEventsApi, blocklyToolbox) {
            var vm = this;
            var aceEditor;
            var blocklyWorkspace;

            vm.$onInit = init;
            vm.setEventState = setEventState;
            vm.saveEvent = saveEvent;
            vm.deleteEvent = deleteEvent;
            vm.importEvent = importEvent;
            vm.exportEvent = exportEvent;
            vm.markEventAsUpdated = markEventAsUpdated;
            vm.isTriggerAvailable = isTriggerAvailable;

            function init() {
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
                        : domoticzEventsApi.fetchEvent(vm.event.id)
                    )
                    .then(function (eventData) {
                        vm.eventData = eventData;

                        if (eventData.interpreter === 'Blockly') {
                            initBlockly(eventData);
                        } else {
                            initAce(eventData)
                        }

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
                $modal.open({
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

                $modal.open({
                    scope: scope,
                    templateUrl: 'app/events/exportEventModal.html'
                });
            }

            function initAce(eventData) {
                require(['ace', 'ace-language-tools'], function () {
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

                    aceEditor.setTheme('ace/theme/xcode');
                    aceEditor.setValue(eventData.xmlstatement);
                    aceEditor.getSession().setMode('ace/mode/' + interpreter.toLowerCase());
                    aceEditor.gotoLine(1);
                    aceEditor.scrollToLine(1, true, true);

                    aceEditor.on('change', function (event) {
                        markEventAsUpdated();
                        $scope.$apply();
                    });

                    $scope.$apply();
                });
            }

            function initBlockly(eventData) {
                var container = $element.find('.js-script-content')[0];

                require(['blockly', 'app/events/blockly_blocks_domoticz.js'], function () {
                    blocklyToolbox.get().then(function (toolbox) {
                        blocklyWorkspace = Blockly.inject(container, {
                            path: './',
                            toolbox: toolbox,
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
                                markEventAsUpdated();
                                $scope.$apply();
                            });
                        }, 200);

                        // Supported only in Chrome 67+
                        if (window.ResizeObserver) {
                            var ro = new ResizeObserver(function (entries) {
                                if (entries.length && entries[0].contentRect.width > 0) {
                                    Blockly.svgResize(blocklyWorkspace);
                                }
                            });

                            ro.observe(container);

                            $scope.$on('$destroy', function() {
                                ro.disconnect();
                            })
                        }
                    });

                    $scope.$apply();
                });
            }
        }
    });
});
