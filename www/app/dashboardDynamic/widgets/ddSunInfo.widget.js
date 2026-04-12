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
            controller: ['$scope', '$http', '$interval', '$q', function($scope, $http, $interval, $q) {
                var ctrl = this;
                ctrl.sunrise   = '\u2014';
                ctrl.sunset    = '\u2014';
                ctrl.daylength = '\u2014';
                var cancelToken = null;

                function load() {
                    if (cancelToken) { cancelToken.resolve(); }
                    cancelToken = $q.defer();

                    $http.get('json.htm?type=command&param=getSunRiseSet', { timeout: cancelToken.promise })
                        .then(function(resp) {
                            var d = resp.data;
                            ctrl.sunrise   = d.Sunrise   || '\u2014';
                            ctrl.sunset    = d.Sunset    || '\u2014';
                            ctrl.daylength = d.DayLength || '\u2014';
                        })
                        .catch(function(err) {
                            if (err.status === -1) { return; }
                            ctrl.error = 'Failed to load data';
                            ctrl.loading = false;
                        });
                }

                // Refresh once per hour — sunrise/sunset changes slowly
                var timer = $interval(load, 3600000);

                $scope.$on('$destroy', function() {
                    if (cancelToken) { cancelToken.resolve(); cancelToken = null; }
                    $interval.cancel(timer);
                });
                $scope.$on('dd:widget:refresh', load);

                ctrl.$onInit = load;
            }]
        };
    }]);
});
