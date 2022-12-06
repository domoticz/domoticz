define(['app'], function (app) {
	app.controller('CamController', ['$scope', '$rootScope', '$location', '$http', '$interval', function ($scope, $rootScope, $location, $http, $interval) {

		init();

		function init() {
			//handles topBar Links
			$scope.tblinks = [
				{
					onclick: "AddCameraDevice", 
					text: "Add Camera", 
					i18n: "Add Camera", 
					icon: "plus-circle"
				}
			];

		};

	}]);
});