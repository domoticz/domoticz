define(['app', 'scenes/factories'], function (app) {
    app.component('entityLogTable', {
        template: '<table class="display entity-log-table"></table>',
        bindings: {
            log: '<'
        },
        controller: function ($element, dataTableDefaultSettings) {
            var $ctrl = this;
            var table;

            $ctrl.$onInit = function () {
                table = $element.find('table').dataTable(Object.assign({}, dataTableDefaultSettings, {
                    columns: [
                        {title: $.t('Date'), data: 'Date', type: 'date'},
                        {title: $.t('Data'), data: 'Data'}
                    ]
                })).api();
            };

            $ctrl.$onChanges = function (changes) {
                if (!table) {
                    return;
                }

                if (changes.log) {
                    table.clear();
                    table.rows
                        .add($ctrl.log)
                        .draw();
                }
            }
        }
    });
});