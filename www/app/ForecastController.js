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
					var fallbackurl = "//forecast.io/embed/#lat=" + data.Latitude + "&lon=" + data.Longitude + "&units=ca&color=#00aaff";
					var fallbackhtml = '<iframe style="background: #fff; height:245px;" class="cIFrame" id="IMain" src="' + fallbackurl + '"></iframe>';
					if (typeof data.Forecastdata != 'undefined' ) {
						if (data.Forecastdevicetype == 83) { // OpenWeatherMap
							htmlcontent = 'Using forecastdevice ' + data.Forecastdata;
							htmlcontent = '<script src="//openweathermap.org/themes/openweathermap/assets/vendor/owm/js/d3.min.js"></script>';
							htmlcontent += '<script>window.myWidgetParam ? window.myWidgetParam : window.myWidgetParam = [];';
							htmlcontent += 'window.myWidgetParam.push({id: 21,cityid: ' + data.Forecastdata.cityid + ',appid: "' + data.Forecastdata.appid + '",units: "metric",containerid: "openweathermap-widget-21",  });';
							htmlcontent += ' (function() {var script = document.createElement("script");script.async = true;script.charset = "utf-8";script.src = "//openweathermap.org/themes/openweathermap/assets/vendor/owm/js/weather-widget-generator.js";';
							htmlcontent += 'var s = document.getElementsByTagName("script")[0];s.parentNode.insertBefore(script, s);  })();</script>';
							$('#openweathermap-widget-21').html(htmlcontent);
							$('#openweathermap-widget-21').i18n();
							$scope.showOWM = true;
						} else {
							htmlcontent = 'Forecastdata for devicetype ' + data.Forecastdevicetype + ' is not supported!';
							$scope.showFallback = true;
						}
					} else if (typeof data.Forecasturl != 'undefined' && data.Forecasturl != '') {
						if (data.Forecasturl.substr(0,4) == 'http') {
							htmlcontent = '<iframe class="cIFrame" id="IMain" src="' + data.Forecasturl + '"></iframe>';
						} else {
							htmlcontent = data.Forecasturl;
						}
						$scope.showFallback = true;
					} else  {
						htmlcontent = fallbackhtml;
						$scope.showFallback = true;
					}
					$('#fallback').html(htmlcontent);
					$('#fallback').i18n();
				} else {
					$scope.showFallback = true;
					$('#fallback').html(htmlcontent);
					$('#fallback').i18n();
				}
			});
		};
	}]);
});