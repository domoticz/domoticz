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
        vm.setActiveEventId = setActiveEventId;
        vm.isInterpreterSupported = isInterpreterSupported;

        init();

        function init() {
            vm.isListExpanded = true;
            vm.activeEventId = 'states';

            vm.openedEvents = [];
            vm.dzVentsTemplates = [
                { id: 'All', name: 'All (commented)' },
                { id: 'Bare', name: 'Minimal' },
                { id: 'Device', name: 'Device' },
                { id: 'Group', name: 'Group' },
                { id: 'Security', name: 'Security' },
                { id: 'Scene', name: 'Scene' },
                { id: 'Timer', name: 'Timer' },
                { id: 'UserVariable', name: 'User variable' },
                { id: 'HTTPRequest', name: 'HTTP request' },
                { id: 'global_data', name: 'Global Data' },
            ];

            vm.luaTemplates = [
                { id: 'All', name: 'All (commented)' },
                { id: 'Device', name: 'Device' },
                { id: 'Security', name: 'Security' },
                { id: 'Time', name: 'Time' },
                { id: 'UserVariable', name: 'User variable' },
            ];

            fetchEvents();
        }

        function isInterpreterSupported(interpreter) {
            if (!vm.interpreters) {
                return undefined;
            }

            return vm.interpreters.includes(interpreter);
        }

        function fetchEvents() {
			$rootScope.RefreshTimeAndSun();

            return domoticzEventsApi.fetchEvents().then(function (data) {
                vm.events = data.events;
                vm.interpreters = data.interpreters;

                if (vm.events.length > 0 && vm.openedEvents.length === 0) {
                    openEvent(vm.events[0]);
                }
            })
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
            }

            setActiveEventId(event.id)
        }

        function closeEvent(event, forceClose) {
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

                event.isChanged = false;
            });
        }

        function updateEvent(event) {
            if (event.isNew) {
                fetchEvents().then(function () {
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
    }
});
