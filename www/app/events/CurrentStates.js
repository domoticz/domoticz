define(['app', 'events/factories'], function (app) {
    app.component('eventsCurrentStates', {
        templateUrl: 'app/events/CurrentStates.html',
        controller: function ($element, domoticzEventsApi, dataTableDefaultSettings) {
            var vm = this;
            var table;

            vm.refresh = refresh;
            vm.$onInit = init;

            function init() {
                table = $element.find('table').dataTable(Object.assign({}, dataTableDefaultSettings, {
                    'sDom': 't',
                    paging: false,
                    columns: [
                        { title: $.t('Idx'), data: 'id', type: 'num', width: '10%' },
                        { title: $.t('Last update'), data: 'lastupdate', type: 'date-us', width: '20%' },
                        { title: $.t('Name'), data: 'name', width: '34%' },
                        { title: $.t('State'), data: 'value', width: '18%' },
                        { title: $.t('Value'), data: 'values', width: '18%' }
                    ]
                })).api();

                refresh();
            }

            function refresh() {
                domoticzEventsApi.fetchCurrentStates()
                    .then(function (states) {
                        var filteredStates = states.filter(function (item) {
                            return item.name !== 'Unknown'
                        });

                        table.clear();
                        table.rows
                            .add(filteredStates)
                            .draw();
                    })
            }
        }
    })
});
