define(['app', 'events/factories', 'events/EventViewer', 'events/CurrentStates', 'events/AutomationWizard'], function (app) {
    app.controller('EventsController', EventsController);

    function EventsController($scope, $q, $rootScope, $uibModal, domoticzApi, domoticzEventsApi, bootbox) {
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
        vm.openWizard = openWizard;
        vm.closeWizard = closeWizard;
        vm.wizardOpen = false;
        vm.toggleFolder = toggleFolder;
        vm.isFolderExpanded = isFolderExpanded;
        vm.showContextMenu = showContextMenu;
        vm.hideContextMenu = hideContextMenu;
        vm.createFolder = createFolder;
        vm.renameFolder = renameFolder;
        vm.deleteFolder = deleteFolder;
        vm.onDragStart = onDragStart;
        vm.onDrop = onDrop;
        vm.onDragEnd = onDragEnd;
        vm.expandAllFolders = expandAllFolders;
        vm.collapseAllFolders = collapseAllFolders;
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

            $scope.$on('$destroy', function () {
                $(document).off('click.eventsContextMenu');
                $(document).off('mouseup.eventsDeselect');
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
                var isFirstLoad = vm.folders.length === 0;
                vm.folders = data.folders || [];

                if (isFirstLoad) {
                    expandAllFolders();
                }

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

        function expandAllFolders() {
            vm.folders.forEach(function (folder) {
                vm.expandedFolders[folder.id] = true;
            });
        }

        function collapseAllFolders() {
            vm.expandedFolders = {};
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
            var scope = $rootScope.$new(true);
            scope.folder = { name: '' };

            $uibModal
                .open({
                    templateUrl: 'app/events/folderNameModal.html',
                    scope: scope
                }).result
                .then(function (name) {
                    if (name && name.trim()) {
                        domoticzEventsApi.createFolder(name.trim()).then(function () {
                            return listEvents();
                        }).then(function () {
                            ShowNotify($.t('Folder created'), 2500);
                        }).catch(function () {
                            ShowNotify($.t('Failed to create folder'), 2500, true);
                        });
                    }
                });
        }

        function renameFolder(folder) {
            hideContextMenu();
            var scope = $rootScope.$new(true);
            scope.folder = { name: folder.name };

            $uibModal
                .open({
                    templateUrl: 'app/events/folderNameModal.html',
                    scope: scope
                }).result
                .then(function (name) {
                    if (name && name.trim()) {
                        domoticzEventsApi.renameFolder(folder.id, name.trim()).then(function () {
                            return listEvents();
                        }).then(function () {
                            ShowNotify($.t('Folder renamed'), 2500);
                        }).catch(function () {
                            ShowNotify($.t('Failed to rename folder'), 2500, true);
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
                    return listEvents();
                }).then(function () {
                    ShowNotify($.t('Folder deleted'), 2500);
                }).catch(function () {
                    ShowNotify($.t('Failed to delete folder'), 2500, true);
                });
            });
        }

        // Multi-select
        function toggleEventSelection(event, $event) {
            var isSelected = vm.selectedEvents[event.id];
            var selectedCount = Object.keys(vm.selectedEvents).filter(function (id) {
                return vm.selectedEvents[id];
            }).length;

            if ($event && ($event.ctrlKey || $event.metaKey)) {
                if (isSelected) {
                    // Defer deselection to mouseup in case user is starting a drag
                    vm._pendingDeselect = event.id;
                    $(document).off('mouseup.eventsDeselect');
                    $(document).one('mouseup.eventsDeselect', function () {
                        if (vm._pendingDeselect === event.id) {
                            vm._pendingDeselect = null;
                            vm.selectedEvents[event.id] = false;
                            $rootScope.$applyAsync();
                        }
                    });
                } else {
                    vm.selectedEvents[event.id] = true;
                }
            } else if (isSelected && selectedCount > 1) {
                // Clicked on an already-selected item in a multi-selection:
                // defer narrowing to mouseup so drag can use the full selection
                vm._pendingDeselect = 'narrow:' + event.id;
                $(document).off('mouseup.eventsDeselect');
                $(document).one('mouseup.eventsDeselect', function () {
                    if (vm._pendingDeselect === 'narrow:' + event.id) {
                        vm._pendingDeselect = null;
                        vm.selectedEvents = {};
                        vm.selectedEvents[event.id] = true;
                        $rootScope.$applyAsync();
                    }
                });
            } else {
                vm.selectedEvents = {};
                vm.selectedEvents[event.id] = true;
            }
        }

        function isEventSelected(event) {
            return !!vm.selectedEvents[event.id];
        }

        // Drag and drop
        function onDragStart(event) {
            // Cancel any pending deselection from mousedown - we're dragging now
            vm._pendingDeselect = null;
            $(document).off('mouseup.eventsDeselect');

            if (!vm.selectedEvents[event.id]) {
                vm.selectedEvents = {};
                vm.selectedEvents[event.id] = true;
            }
            vm.dragState = {
                dragging: true,
                eventIds: Object.keys(vm.selectedEvents).filter(function (id) {
                    return vm.selectedEvents[id];
                })
            };
        }

        function onDrop(targetFolderId) {
            var eventIds = vm.dragState.eventIds || [];
            vm.dragState = { dragging: false };

            var promises = eventIds.map(function (eventId) {
                return domoticzEventsApi.moveEvent(eventId, targetFolderId);
            });

            $q.all(promises).then(function () {
                vm.selectedEvents = {};
                vm.expandedFolders[targetFolderId] = true;
                listEvents();
            });
        }

        function onDragEnd() {
            vm.dragState = { dragging: false };
            $('.drag-over').removeClass('drag-over');
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
            if (event === -1) {
                // Close all events (keep unsaved)
                var hadUnsaved = vm.openedEvents.some(function (item) { return item.isChanged; });
                vm.openedEvents = vm.openedEvents.filter(function (item) {
                    return item.isChanged;
                });
                if (hadUnsaved && vm.openedEvents.length > 0) {
                    setActiveEventId(vm.openedEvents[0].id);
                } else {
                    setActiveEventId('states');
                }
                storeRecentEvents();
                return;
            }

            if (event === -2) {
                // Close other events (keep active + unsaved)
                vm.openedEvents = vm.openedEvents.filter(function (item) {
                    return item.id === vm.activeEventId || item.isChanged;
                });
                storeRecentEvents();
                return;
            }

            if (event === 0) {
                // Close current active event
                var found = vm.openedEvents.find(function (item) {
                    return item.id === vm.activeEventId;
                });
                if (!found) return;
                event = found;
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
					var recentIds = data.split(',');
					for (var i = 0; i < recentIds.length; i++) {
						var match = vm.events.find(function (item) {
							return String(item.id) === recentIds[i];
						});
						if (match) {
							openEvent(match);
						}
					}
				} else {
					//open first event
					openEvent(vm.events[0]);
				}
				vm.storeRecents = true;
			});
		}

		function storeRecentEvents() {
			if (vm.storeRecents === false) {
				return;
			}
			var recentEvents = vm.openedEvents.map(function (item) {
				return item.id;
			});
			domoticzEventsApi.storeRecents(recentEvents);
		}

        function openWizard() {
            vm.wizardOpen = true;
        }

        function closeWizard() {
            vm.wizardOpen = false;
        }
    }

    app.directive('eventsDragSource', function () {
        return {
            restrict: 'A',
            link: function (scope, element, attrs) {
                var el = element[0];
                el.draggable = true;

                function handleDragStart(e) {
                    e.dataTransfer.setData('text/plain', '');
                    e.dataTransfer.effectAllowed = 'move';
                    scope.$apply(function () {
                        scope.$eval(attrs.eventsDragSource);
                    });

                    // Show a badge with the count when dragging multiple items
                    var dragState = scope.$ctrl && scope.$ctrl.dragState;
                    if (dragState && dragState.eventIds && dragState.eventIds.length > 1) {
                        var badge = document.createElement('div');
                        badge.textContent = dragState.eventIds.length + ' ' + $.t('scripts');
                        badge.style.cssText = 'position:absolute;top:-9999px;left:-9999px;padding:4px 10px;background:#0078d7;color:#fff;border-radius:3px;font-size:12px;white-space:nowrap;';
                        document.body.appendChild(badge);
                        e.dataTransfer.setDragImage(badge, 0, 0);
                        setTimeout(function () { document.body.removeChild(badge); }, 0);
                    }
                }

                function handleDragEnd() {
                    scope.$apply(function () {
                        scope.$eval(attrs.eventsDragEnd);
                    });
                }

                el.addEventListener('dragstart', handleDragStart);
                el.addEventListener('dragend', handleDragEnd);

                scope.$on('$destroy', function () {
                    el.removeEventListener('dragstart', handleDragStart);
                    el.removeEventListener('dragend', handleDragEnd);
                });
            }
        };
    });

    app.directive('eventsDropTarget', function () {
        return {
            restrict: 'A',
            link: function (scope, element, attrs) {
                var el = element[0];
                var dragCounter = 0;

                function handleDragOver(e) {
                    e.preventDefault();
                    e.dataTransfer.dropEffect = 'move';
                }

                function handleDragEnter(e) {
                    e.preventDefault();
                    dragCounter++;
                    element.addClass('drag-over');
                }

                function handleDragLeave() {
                    dragCounter--;
                    if (dragCounter === 0) {
                        element.removeClass('drag-over');
                    }
                }

                function handleDrop(e) {
                    e.preventDefault();
                    e.stopPropagation();
                    dragCounter = 0;
                    element.removeClass('drag-over');
                    scope.$apply(function () {
                        scope.$eval(attrs.eventsDropTarget);
                    });
                }

                el.addEventListener('dragover', handleDragOver);
                el.addEventListener('dragenter', handleDragEnter);
                el.addEventListener('dragleave', handleDragLeave);
                el.addEventListener('drop', handleDrop);

                scope.$on('$destroy', function () {
                    el.removeEventListener('dragover', handleDragOver);
                    el.removeEventListener('dragenter', handleDragEnter);
                    el.removeEventListener('dragleave', handleDragLeave);
                    el.removeEventListener('drop', handleDrop);
                });
            }
        };
    });
});
