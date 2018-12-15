define(['app'], function (app) {
	app.controller('ForecastController', ['$scope', '$rootScope', '$location', '$http', '$interval', function ($scope, $rootScope, $location, $http, $interval) {
		init();

		function init() {
			$scope.MakeGlobalConfig();
			var htmlcontent = '';
			var units = "ca24";
			if ($rootScope.config.TempSign == "F") {
			    units = "us12";
			} else {
			    if ($rootScope.config.WindSign == "m/s")
			        units = "si24";
			    else if ($rootScope.config.WindSign == "km/h")
			        units = "ca24";
			    else if ($rootScope.config.WindSign == "mph")
			        units = "uk224";
			}
			htmlcontent += '<iframe class="cIFrameLarge" id="IMain" src="//darksky.net/forecast/' + $scope.config.Latitude + ',' + $scope.config.Longitude + '/' + units + '/' + $rootScope.config.language + '"></iframe>';
			$('#maincontent').html(htmlcontent);
			$('#maincontent').i18n();
		};
	}]);
});