define(['app'], function (app) {
	app.controller('ForecastController', ['$scope', '$rootScope', '$location', '$http', '$interval', function ($scope, $rootScope, $location, $http, $interval) {
		init();

		function init() {
			$scope.MakeGlobalConfig();
			$http({
				url: "json.htm?type=command&param=getforecastconfig",
				async: true,
				dataType: 'json'
			}).then(function successCallback(response) {
				var data = response.data;
				var htmlcontent = '<b>Error loading config</b>';
				if (typeof data.status != 'undefined' && data.status == 'OK') {
					if (typeof data.Forecastdevice == 'undefined' || (typeof data.Forecastdevice != 'undefined' && data.Forecastdevice == 0)) {
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
						htmlcontent = '<iframe style="background: #fff; height:245px;" class="cIFrameLarge" id="IMain" src="//forecast.io/embed/#lat=' + data.Latitude + '&lon=' + data.Longitude + '&units=ca&color=#00aaff"></iframe>';
					} else {
						htmlcontent = 'Using forecastdevice ' + data.Forecastdevice;
					}
				}
				$('#maincontent').html(htmlcontent);
				$('#maincontent').i18n();
			});
		};
	}]);
});