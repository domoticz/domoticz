require.config({
	baseUrl: "app",
	paths: {
		'angular': '../js/angular.min',
		'angular-route': '../js/angular-route.min',
		'angular-animate': '../js/angular-animate.min',
		'ngSanitize': '../js/angular-sanitize.min',
		'angular-md5': '../js/angular-md5-min',
		'ng-grid': '../js/ng-grid.min',
		'ng-grid-flexible-height': '../js/ng-grid-flexible-height',
		'highcharts-ng': '../js/highcharts-ng.min',
		'angularAMD': '../js/angularAMD.min',
		'angular-tree-control': '../js/angular-tree-control',
		'ngDraggable': '../js/ngDraggable',
		'ui.bootstrap': '../js/ui-bootstrap.min',
		'angular.directives-round-progress': '../js/angular-round-progress-directive',
		'angular.scrollglue': '../js/angular-scrollglue',
		'angular-websocket': '../js/angular-websocket'
	},
	shim: {
		'angularAMD': ['angular'],
		'angular-route': ['angular'],
		'angular-animate': ['angular'],
		'ngSanitize': ['angular'],
		'angular-md5': ['angular'],
		'ng-grid': ['angular'],
		'ng-grid-flexible-height': ['angular'],
		'highcharts-ng': ['angular'],
		'angular-tree-control': ['angular'],
		'ui.bootstrap': ['angular'],
		'ngDraggable': ['angular'],
		'angular.directives-round-progress': ['angular'],
		'angular.scrollglue': ['angular'],
		'angular-websocket': ['angular']
	},
	deps: ['app']
});
