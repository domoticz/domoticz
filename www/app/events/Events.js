define(['app', 'events/factories', 'events/EventViewer', 'events/CurrentStates'], function (app) {
    app.controller('EventsController', EventsController);

    function EventsController($q, $rootScope, domoticzApi, domoticzEventsApi, bootbox) {
        var vm = this;
        vm.createEvent = createEvent;
        vm.openEvent = openEvent;
        vm.closeEvent = closeEvent;
        vm.updateEvent = updateEvent;
        vm.updateEventState = updateEventState;
        vm.deleteEvent = deleteEvent;
		vm.storeRecentEvents = storeRecentEvents;
		vm.loadRecentEvents = loadRecentEvents;
        vm.setActiveEventId = setActiveEventId;
        vm.isInterpreterSupported = isInterpreterSupported;
        vm.toggleFolder = toggleFolder;
        vm.isFolderExpanded = isFolderExpanded;
        vm.showContextMenu = showContextMenu;
        vm.hideContextMenu = hideContextMenu;
        vm.createFolder = createFolder;
        vm.renameFolder = renameFolder;
        vm.deleteFolder = deleteFolder;
        vm.onDragStart = onDragStart;
        vm.onDragOver = onDragOver;
        vm.onDrop = onDrop;
        vm.onDragEnd = onDragEnd;
        vm.toggleEventSelection = toggleEventSelection;
        vm.isEventSelected = isEventSelected;
        vm.getEventsInFolder = getEventsInFolder;
        vm.getRootEvents = getRootEvents;
		
		vm.storeRecents = true;

        init();

        function init() {
            vm.isListExpanded = true;
            vm.activeEventId = 'states';

            vm.openedEvents = [];
            vm.folders = [];
            vm.expandedFolders = {};
            vm.contextMenu = { visible: false, x: 0, y: 0, target: null, targetType: null };
            vm.selectedEvents = {};
            vm.dragState = { dragging: false };

            vm.dzVentsTemplates = [
                { id: 'All', name: 'All (commented)' },
                { id: 'Bare', name: 'Minimal' },
                { id: 'CustomEvents', name: 'Custom events' },
                { id: 'Device', name: 'Device' },
                { id: 'ExecuteShellCommand', name: 'Async OS command' },
                { id: 'Group', name: 'Group' },
                { id: 'HTTPRequest', name: 'HTTP request' },
                { id: 'Logging', name: 'Logging' },
                { id: 'Scene', name: 'Scene' },
                { id: 'Security', name: 'Security' },
                { id: 'System', name: 'System events' },
                { id: 'Timer', name: 'Timer' },
                { id: 'UserVariable', name: 'User variable' },
                { id: 'global_data', name: 'Global Data' }
            ];

            vm.luaTemplates = [
                { id: 'All', name: 'All (commented)' },
                { id: 'Device', name: 'Device' },
                { id: 'Security', name: 'Security' },
                { id: 'Time', name: 'Time' },
                { id: 'UserVariable', name: 'User variable' }
            ];

            listEvents();

            // Hide context menu on click elsewhere
            $(document).on('click.eventsContextMenu', function () {
                if (vm.contextMenu.visible) {
                    vm.contextMenu.visible = false;
                    $rootScope.$applyAsync();
                }
            });
        }

        function isInterpreterSupported(interpreter) {
            if (!vm.interpreters) {
                return undefined;
            }

            return vm.interpreters.includes(interpreter);
        }

        function listEvents() {
			$rootScope.RefreshTimeAndSun();

            return domoticzEventsApi.listEvents().then(function (data) {
                vm.events = data.events;
                vm.interpreters = data.interpreters;
                vm.folders = data.folders || [];

                if (vm.events.length > 0 && vm.openedEvents.length === 0) {
					loadRecentEvents();
                }
            })
        }

        function getEventsInFolder(folderId) {
            return (vm.events || []).filter(function (event) {
                return event.folderid === String(folderId);
            });
        }

        function getRootEvents() {
            return (vm.events || []).filter(function (event) {
                return !event.folderid || event.folderid === '0';
            });
        }

        function toggleFolder(folder) {
            vm.expandedFolders[folder.id] = !vm.expandedFolders[folder.id];
        }

        function isFolderExpanded(folder) {
            return !!vm.expandedFolders[folder.id];
        }

        // Context menu
        function showContextMenu(event, target, targetType) {
            event.preventDefault();
            event.stopPropagation();
            vm.contextMenu = {
                visible: true,
                x: event.clientX,
                y: event.clientY,
                target: target,
                targetType: targetType
            };
        }

        function hideContextMenu() {
            vm.contextMenu.visible = false;
        }

        function createFolder() {
            hideContextMenu();
            bootbox.prompt($.t('Enter folder name') + ':').then(function (name) {
                if (name && name.trim()) {
                    domoticzEventsApi.createFolder(name.trim()).then(function () {
                        listEvents();
                    });
                }
            });
        }

        function renameFolder(folder) {
            hideContextMenu();
            bootbox.prompt($.t('Enter new folder name') + ':', folder.name).then(function (name) {
                if (name && name.trim()) {
                    domoticzEventsApi.renameFolder(folder.id, name.trim()).then(function () {
                        listEvents();
                    });
                }
            });
        }

        function deleteFolder(folder) {
            hideContextMenu();
            var eventsInFolder = getEventsInFolder(folder.id);
            var message;
            if (eventsInFolder.length > 0) {
                message = $.t('This folder contains') + ' ' + eventsInFolder.length + ' ' + $.t('script(s). Deleting the folder will also delete all scripts in it.') + '\n\n' + $.t('Are you sure?');
            } else {
                message = $.t('Are you sure you want to delete this folder?');
            }
            $q(function(resolve, reject) {
                window.bootbox.confirm(message, function (result) {
                    result === true ? resolve() : reject();
                });
            }).then(function () {
                // Close any open events that are in this folder
                eventsInFolder.forEach(function (evt) {
                    var openedEvent = vm.openedEvents.find(function (item) {
                        return item.id === evt.id;
                    });
                    if (openedEvent) {
                        closeEvent(openedEvent, true);
                    }
                });
                domoticzEventsApi.deleteFolder(folder.id).then(function () {
                    listEvents();
                });
            });
        }

        // Multi-select
        function toggleEventSelection(event, $event) {
            if ($event && ($event.ctrlKey || $event.metaKey)) {
                vm.selectedEvents[event.id] = !vm.selectedEvents[event.id];
            } else {
                vm.selectedEvents = {};
                vm.selectedEvents[event.id] = true;
            }
        }

        function isEventSelected(event) {
            return !!vm.selectedEvents[event.id];
        }

        // Drag and drop
        function onDragStart(event, $event) {
            if (!vm.selectedEvents[event.id]) {
                vm.selectedEvents = {};
                vm.selectedEvents[event.id] = true;
            }
            vm.dragState = { dragging: true };

            var selectedIds = Object.keys(vm.selectedEvents).filter(function (id) {
                return vm.selectedEvents[id];
            });

            $event.dataTransfer.setData('text/plain', JSON.stringify(selectedIds));
            $event.dataTransfer.effectAllowed = 'move';
        }

        function onDragOver($event) {
            $event.preventDefault();
            $event.dataTransfer.dropEffect = 'move';
        }

        function onDrop(targetFolderId, $event) {
            $event.preventDefault();
            $event.stopPropagation();
            vm.dragState = { dragging: false };

            try {
                var selectedIds = JSON.parse($event.dataTransfer.getData('text/plain'));
                var promises = selectedIds.map(function (eventId) {
                    return domoticzEventsApi.moveEvent(eventId, targetFolderId);
                });

                $q.all(promises).then(function () {
                    vm.selectedEvents = {};
                    listEvents();
                });
            } catch (e) {
                // Ignore invalid drag data
            }
        }

        function onDragEnd() {
            vm.dragState = { dragging: false };
        }

        function createEvent(interpreter, eventtype) {
            function isNameExists(name) {
                return []
                    .concat(vm.events)
                    .concat(vm.openedEvents)
                    .some(function (event) {
                        return event.name === name;
                    });
            }

            domoticzEventsApi.getTemplate(interpreter, eventtype).then(function (template) {
                var index = 0;
                var name;

                do {
                    index += 1;
                    name = 'Script #' + index;
                } while (isNameExists(name));

                var event = {
                    id: name,
                    eventstatus: '1',
                    name: name,
                    interpreter: interpreter,
                    type: eventtype || 'All',
                    xmlstatement: template,
                    isChanged: true,
                    isNew: true
                };

                openEvent(event)
            });
        }

        function setActiveEventId(eventId) {
            vm.activeEventId = eventId;
        }

        function openEvent(event) {
            if (!vm.openedEvents.find(function (item) {
                return item.id === event.id
            })) {
                vm.openedEvents.push(event);
				storeRecentEvents();
            }
            setActiveEventId(event.id)
        }

        function closeEvent(event, forceClose) {
			if (event == -1) {
				//close all events
                vm.openedEvents = vm.openedEvents.filter(function (item) {
                    return item.isChanged;
                });
				setActiveEventId('states');
				storeRecentEvents();
				return;
			}

			if (event == 0) {
				for (let i = 0; i < vm.openedEvents.length; i++) {
					if (vm.openedEvents[i].id == vm.activeEventId) {
						event = vm.openedEvents[i];
					}
				}				
			}
            $q.resolve(event.isChanged && !forceClose
                ? bootbox.confirm($.t('This script has unsaved changes.\n\nAre you sure you want to close it?'))
                : true
            ).then(function () {
                if (vm.activeEventId === event.id) {
                    var index = vm.openedEvents.indexOf(event);

                    if (vm.openedEvents[index + 1]) {
                        openEvent(vm.openedEvents[index + 1]);
                    } else if (vm.openedEvents[index - 1]) {
                        openEvent(vm.openedEvents[index - 1]);
                    } else {
                        vm.activeEventId = 'states';
                    }
                }

                vm.openedEvents = vm.openedEvents.filter(function (item) {
                    return item.id !== event.id
                });
				storeRecentEvents();

                event.isChanged = false;
            });
        }

        function updateEvent(event) {
            if (event.isNew) {
                listEvents().then(function () {
                    vm.openedEvents = vm.openedEvents.map(function (item) {
                        if (item.id !== event.id) {
                            return item;
                        }

                        var newEvent = vm.events.find(function (ev) {
                            return ev.name === event.name
                        });

                        if (vm.activeEventId === event.id) {
                            vm.activeEventId = newEvent.id;
                        }

                        return newEvent;
                    });
                });
            } else {
                var updates = { name: event.name, eventstatus: event.eventstatus };

                vm.events = vm.events.map(function (item) {
                    return item.id === event.id
                        ? Object.assign({}, item, updates)
                        : item;
                });

                vm.openedEvents = vm.openedEvents.map(function (item) {
                    return item.id === event.id
                        ? Object.assign({}, item, updates)
                        : item;
                });
            }
        }

        function updateEventState(event) {
            domoticzEventsApi.updateEventState(event.id, event.eventstatus === '1');
        }

        function deleteEvent(event) {
            vm.events = vm.events.filter(function (item) {
                return item.id !== event.id
            });

            closeEvent(event, true);
        }
		
		function loadRecentEvents() {
			vm.storeRecents = false;

			domoticzEventsApi.loadRecents().then(function (data) {
				if (data.length > 0) {
					const recentEvents = data.split(',');
					for (id of recentEvents) {
						vm.events.map(function (item) {
							if (item.id == id) {
								openEvent(item);
							}
						});
					}
				} else {
					//open first event
					openEvent(vm.events[0]);
				}
				vm.storeRecents = true;
			});
		}
		
		function storeRecentEvents() {
			if (vm.storeRecents == false) {
				return;
			}
			var recentEvents = [];
			vm.openedEvents.map(function (item) {
				recentEvents.push(item.id);
			});
			domoticzEventsApi.storeRecents(recentEvents);
		}
    }
});
