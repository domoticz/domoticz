define(['app'], function (app) {
	app.controller('UserVariablesController', [ '$scope', '$rootScope', '$location', '$http', '$interval', function($scope,$rootScope,$location,$http,$interval) {
		init();

		function init()
		{
			$scope.MakeGlobalConfig();
		};
	} ]);
});