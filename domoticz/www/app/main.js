require.config({
    baseUrl: "app",
    paths: {
        'angular': '../js/angular.min',
        'angular-route': '../js/angular-route.min',
        'angular-animate': '../js/angular-animate.min',
        'ng-grid': '../js/ng-grid.min',
        'ng-grid-flexible-height': '../js/ng-grid-flexible-height',
        'highcharts-ng': '../js/highcharts-ng.min',
        'angularAMD': '../js/angularAMD.min',
        'angular-tree-control': '../js/angular-tree-control',
        'ngDraggable': '../js/ngDraggable'
    },
    shim: { 
		'angularAMD': ['angular'], 
		'angular-route': ['angular'],
		'angular-animate': ['angular'],
		'ng-grid': ['angular'],
		'ng-grid-flexible-height': ['angular'],
		'highcharts-ng': ['angular'],
		'angular-tree-control': ['angular'],
		'ngDraggable': ['angular']
	},
    deps: ['app']
});
