define(['app'], function (app) {
	app.controller('ForecastController', ['$scope', '$rootScope', '$location', '$http', '$interval', function ($scope, $rootScope, $location, $http, $interval) {
		init();

		function init() {
			$scope.MakeGlobalConfig();
			$http({
				url: "json.htm?type=command&param=getlocation",
				async: true,
				dataType: 'json'
			}).then(function successCallback(response) {
				var data = response.data;
				if (typeof data.Latitude != 'undefined') {
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
					//htmlcontent += '<iframe class="cIFrameLarge" id="IMain" src="//darksky.net/forecast/' + data.Latitude + ',' + data.Longitude + '/' + units + '/' + $rootScope.config.language + '"></iframe>';
					htmlcontent += '<iframe style="background: #fff; height:245px;" class="cIFrameLarge" id="IMain" src="//forecast.io/embed/#lat=' + data.Latitude + '&lon=' + data.Longitude + '&units=ca&color=#00aaff"></iframe>';
					//htmlcontent += '<iframe class="cIFrameLarge" id="IMain" src="//forecast.io/embed/#lat=42.3583&lon=-71.0603&color=#00aaff&units=ca"></iframe>';
					//console.log(htmlcontent);
					$('#maincontent').html(htmlcontent);
					$('#maincontent').i18n();
				}
			});
		};
	}]);
});