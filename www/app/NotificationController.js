define(['app'], function (app) {
	app.controller('NotificationController', [ '$scope', '$rootScope', '$location', '$http', '$interval', function($scope,$rootScope,$location,$http,$interval) {
		init();

		function init()
		{
			$scope.MakeGlobalConfig();
		};
	} ]);
});