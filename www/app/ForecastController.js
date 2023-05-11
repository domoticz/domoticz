define(['app'], function (app) {
	app.controller('ForecastController', ['$scope', '$rootScope', '$location', '$http', '$interval', function ($scope, $rootScope, $location, $http, $interval) {
		init();

		goBack = function () {
			SwitchLayout("Weather");
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
					var fallbackurl = "//www.visualcrossing.com/weather-history/" + data.Latitude + "," + data.Longitude + "/metric/next7days";
					var fallbackhtml = '<iframe style="background: #fff; height: calc(100% - 140px);" class="cIFrame" id="IMain" src="' + fallbackurl + '"></iframe>';
					if (typeof data.Forecastdata != 'undefined' ) {
						if (data.Forecasthardwaretype == 83) { // OpenWeatherMap
							htmlcontent = '<div id="openweathermap-widget-21"></div>';
							htmlcontent += '<script src="https://openweathermap.org/themes/openweathermap/assets/vendor/owm/js/d3.min.js"></script>';
							htmlcontent += '<script>window.myWidgetParam ? window.myWidgetParam : window.myWidgetParam = [];';
							htmlcontent += 'window.myWidgetParam.push({id: 21,cityid: ' + data.Forecastdata.cityid + ',appid: "' + data.Forecastdata.appid + '",units: "metric",containerid: "openweathermap-widget-21",  });';
							htmlcontent += ' (function() {var script = document.createElement("script");script.async = true;script.charset = "utf-8";script.src = "https://openweathermap.org/themes/openweathermap/assets/vendor/owm/js/weather-widget-generator.js";';
							htmlcontent += 'var s = document.getElementsByTagName("script")[0];s.parentNode.insertBefore(script, s);  })();</script>';
							$('#openweathermap').html(htmlcontent);
							$('#openweathermap').i18n();
							$scope.showOWM = true;
						} else {
							htmlcontent = 'Forecastdata for hardwaretype ' + data.Forecasthardwaretype + ' is not supported!';
							$('#fallback').html(htmlcontent);
							$('#fallback').i18n();
							$scope.showFallback = true;
						}
					} else { 
						htmlcontent = fallbackhtml;
						if (typeof data.Forecasturl != 'undefined' && data.Forecasturl != '') {
							if (data.Forecasturl.substr(0,4) == 'http') {
								htmlcontent = '<iframe style="height: calc(100% - 140px);" class="cIFrame" id="IMain" src="' + data.Forecasturl + '"></iframe>';
							} else {
								htmlcontent = data.Forecasturl;
							}
						}
						$('#fallback').html(htmlcontent);
						$('#fallback').i18n();
						$scope.showFallback = true;
					}
				} else {
					$('#fallback').html(htmlcontent);
					$('#fallback').i18n();
					$scope.showFallback = true;
				}
			});

			$scope.tblinks = [
				{
					onclick:"goBack", 
					text:"Back", 
					i18n: "Back", 
					icon: "reply"
				}
			];


		};
	}]);
});