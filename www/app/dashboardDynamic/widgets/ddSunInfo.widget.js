define([
    'app',
    'dashboardDynamic/widgetRegistry.service'
], function(app, widgetRegistry) {
    'use strict';

    widgetRegistry.register({
        type:        'sun-info',
		transparentBackground: true,
        label:       'Sun Info',
        description: 'Sunrise and sunset times with day length',
        category:    'Weather',
        icon:        'fa-solid fa-sun',
        defaultW:    1,
        defaultH:    2,
        minW:        1,
        minH:        2,
        maxW:        4,
        maxH:        3,
        configSchema: [
            { key: 'showBackground', type: 'boolean', label: 'Show panel background', default: true }
        ]
    });

    app.directive('ddSunInfoWidget', ['$q', function($q) {
        return {
            restrict:         'E',
            templateUrl:      'views/dashboardDynamic/widgets/sun-info.html',
            scope: {
                widgetDef: '=',
                editMode:  '<'
            },
            controllerAs:     'ctrl',
            bindToController: true,
            controller: ['$scope', '$http', '$q', function($scope, $http, $q) {
                var ctrl = this;
                ctrl.sunrise   = '—';
                ctrl.sunset    = '—';
                ctrl.daylength = '—';
                var cancelToken = null;

                function load() {
                    if (cancelToken) { cancelToken.resolve(); }
                    cancelToken = $q.defer();

                    $http.get('json.htm?type=command&param=getSunRiseSet', { timeout: cancelToken.promise })
                        .then(function(resp) {
                            var d = resp.data;
                            ctrl.sunrise   = d.Sunrise   || '—';
                            ctrl.sunset    = d.Sunset    || '—';
                            ctrl.daylength = d.DayLength || '—';
                        })
                        .catch(function(err) {
                            if (err.status === -1) { return; }
                            ctrl.error = 'Failed to load data';
                        });
                }

                // time_update fires every ~10s and carries current sunrise/sunset
                $scope.$on('time_update', function(e, data) {
                    if (data && data.sunrise) { ctrl.sunrise = data.sunrise; }
                    if (data && data.sunset)  { ctrl.sunset  = data.sunset; }
                });

                $scope.$on('$destroy', function() {
                    if (cancelToken) { cancelToken.resolve(); cancelToken = null; }
                });
                $scope.$on('dd:widget:refresh', load);

                ctrl.$onInit = load;
            }]
        };
    }]);
});
