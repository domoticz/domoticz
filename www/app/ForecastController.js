define(['app'], function (app) {
	app.controller('ForecastController', ['$scope', '$rootScope', '$location', '$http', '$interval', function ($scope, $rootScope, $location, $http, $interval) {
		init();

		goBack = function () {
			SwitchLayout("Dashboard");
		}

		function init() {
			$scope.MakeGlobalConfig();
			$http({
				url: "json.htm?type=command&param=getforecastconfig",
				async: true,
				dataType: 'json'
			}).then(function successCallback(response) {
				var data = response.data;
				var htmlcontent = '<b>Error loading config or incorrect config data!</b>';
				if (typeof data.status != 'undefined' && data.status == 'OK') {
					if (typeof data.Forecasturl != 'undefined' && data.Forecasturl != '') {
						htmlcontent = data.Forecasturl;
					} else if (typeof data.Forecastdata != 'undefined' ) {
						htmlcontent = 'Using forecastdevice ' + data.Forecastdata;
					} else {
						htmlcontent = 'Neither a forecast URL or valid forecast data provided!';
					}
				}
				$('#maincontent').html(htmlcontent);
				$('#maincontent').i18n();
			});
		};
	}]);
});