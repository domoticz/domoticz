define(['app'], function (app) {
    app.factory('domoticzEventsApi', function ($http, $httpParamSerializer, $q, domoticzApi) {
        return {
            fetchCurrentStates: fetchCurrentStates,
            fetchEvents: fetchEvents,
            fetchEvent: fetchEvent,
            updateEvent: updateEvent,
            updateEventState: updateEventState,
            deleteEvent: deleteEvent,
            getTemplate: getTemplate
        };

        function fetchCurrentStates() {
            return domoticzApi.sendRequest({
                type: 'events',
                param: 'currentstates'
            }).then(function (response) {
                return response && response.status !== 'OK'
                    ? $q.reject(response)
                    : response.result || [];
            });
        }

        function fetchEvents() {
            return domoticzApi.sendRequest({
                type: 'events',
                param: 'list'
            }).then(function (data) {
                return {
                    events: data.result || [],
                    interpreters: data.interpreters ? data.interpreters.split(':') : []
                }
            });
        }

        function fetchEvent(eventId) {
            return domoticzApi.sendRequest({
                type: 'events',
                param: 'load',
                event: eventId
            }).then(function (data) {
                return data.result[0];
            });
        }

        function updateEvent(event) {
            return $http({
                method: 'POST',
                url: 'event_create.webem',
                data: $httpParamSerializer({
                    eventid: event.id,
                    name: event.name,
                    interpreter: event.interpreter,
                    eventtype: event.type,
                    xml: event.xmlstatement,
                    logicarray: event.logicarray,
                    eventstatus: event.eventstatus
                }),
                headers: {
                    'Content-Type': 'application/x-www-form-urlencoded'
                }
            }).then(function (response) {
                return domoticzApi.errorHandler(response.data)
            });
        }

        function updateEventState(eventId, isEnabled) {
            return domoticzApi.sendCommand('updatestatus', {
                type: 'events',
                eventid: eventId,
                eventstatus: isEnabled ? '1' : '0'
            });
        }

        function deleteEvent(eventId) {
            return domoticzApi.sendCommand('delete', {
                type: 'events',
                event: eventId
            });
        }

        function getTemplate(interpreter, eventType) {
            return domoticzApi.sendRequest({
                type: 'events',
                param: 'new',
                interpreter: interpreter,
                eventtype: eventType || 'All'
            }).then(function (response) {
                return response.template;
            });
        }
    });

    app.factory('blocklyToolbox', function ($http) {
        var toolbox;

        return {
            get: get
        };

        function get() {
            if (!toolbox) {
                toolbox = $http.get('app/events/blockly_toolbox.xml').then(function (response) {
                    return response.data;
                })
            }

            return toolbox;
        }
    });
});
