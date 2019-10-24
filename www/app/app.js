// request permission on page load
document.addEventListener('DOMContentLoaded', function () {
	if (Notification.permission !== "granted")
		Notification.requestPermission();
});

define(['angularAMD', 'app.routes', 'app.constants', 'app.permissions', 'domoticz.api', 'devices/deviceFactory', 'angular-animate', 'ng-grid', 'ng-grid-flexible-height', 'highcharts-ng', 'angular-tree-control', 'ngDraggable', 'ngSanitize', 'angular-md5', 'ui.bootstrap', 'angular.directives-round-progress', 'angular.scrollglue'], function (angularAMD, appRoutesModule, appConstantsModule, appPermissionsModule, apiModule, deviceFactory) {
	var app = angular.module('domoticz', ['ngRoute', 'ngAnimate', 'ngGrid', 'highcharts-ng', 'treeControl', 'ngDraggable', 'ngSanitize', 'angular-md5', 'ui.bootstrap', 'angular.directives-round-progress', 'angular.directives-round-progress', 'angular.scrollglue', appRoutesModule.name, appPermissionsModule.name, apiModule.name, appConstantsModule.name]);

	isOnline = false;
	dashboardType = 1;

	app.directive('sbLoad', ['$parse', function ($parse) {
		return {
			restrict: 'A',
			link: function (scope, elem, attrs) {
				var fn = $parse(attrs.sbLoad);
				elem.on('load', function (event) {
					scope.$apply(function () {
						fn(scope, { $event: event });
					});
				});
			}
		};
	}]);
	app.directive('file', function () {
		return {
			scope: {
				file: '='
			},
			link: function (scope, el, attrs) {
				el.bind('change', function (event) {
					var file = event.target.files[0];
					scope.file = file ? file : undefined;
					scope.$apply();
				});
			}
		};
	});
	app.directive('fileModel', ['$parse', function ($parse) {
		return {
			restrict: 'A',
			link: function (scope, element, attrs) {
				var model = $parse(attrs.fileModel);
				var modelSetter = model.assign;

				element.bind('change', function () {
					scope.$apply(function () {
						modelSetter(scope, element[0].files[0]);
					});
				});
			}
		};
	}]);
	app.directive('formAutofillFix', function () {
		return function (scope, elem, attrs) {
			// Fixes Chrome bug: https://groups.google.com/forum/#!topic/angular/6NlucSskQjY
			elem.prop('method', 'POST');

			// Fix autofill issues where Angular doesn't know about autofilled inputs
			if (attrs.ngSubmit) {
				setTimeout(function () {
					elem.unbind('submit').submit(function (e) {
						e.preventDefault();
						elem.find('input, textarea, select').trigger('input').trigger('change').trigger('keydown');
						scope.$apply(attrs.ngSubmit);
					});
				}, 0);
			}
		};
	});
	app.directive('backButton', function ($window) {
		return {
			restrict: 'A',
			link: function (scope, element, attrs) {
				element.on('click', function() {
					scope.$apply(function () {
						$window.history.back();
					});
				})
			}
		}
	});
	app.directive('gzBackToTop', function(){
		return {
			restrict: 'E'
			, replace: true
			, template: '<div class="gz-back-to-top"></div>'
			, link: function($scope, element, attrs) {
				$(window).scroll(function() {
					if ($(window).scrollTop() <= 0) {
						$(element).fadeOut();
					}
					else {
						$(element).fadeIn();
					}
				});
				$(element).on('click', function(){
					$('html, body').animate({ scrollTop: 0 }, 'fast');
				});
			}
		}
	});

    app.directive('i18n', function() {
        return {
            restrict: 'A',
            priority: 1000,
            link: function(scope, element, attrs) {
                if (!element.is('tr')) {
                    element.text($.t(attrs['i18n']));
                }
            }
        }
    });

	/*
		The notifyBrowser service sets and gets the browser notifications coming on from the domoticz websocket.
		In livesocket.js the notifications are pushed to the message queue by using the notify() method.
		Each notification times out in 2 seconds.
		The alertarea directive reads the messagequeue using the messages() method.
	*/
	app.service('notifyBrowser', [ '$timeout', function ($timeout) {
		var messagequeue = [];
		return {
			notify: function (title, body) {
				var item = { "title": title, "body": body, "time": new Date() };
				messagequeue.push(item);
				$timeout(function () {
					var index = messagequeue.indexOf(item);
					if (index >= 0) {
						messagequeue.splice(index, 1);
					}
				}, 2000);
				if (typeof Notification == "undefined") {
					console.log("Notification: " + title + ": " + body);
					console.log('Desktop notifications not available in your browser. Try Chromium.');
					return;
				}

				if (Notification.permission !== "granted")
					Notification.requestPermission();
				else {
					var notification = new Notification(title, {
						//icon: 'http://cdn.sstatic.net/stackexchange/img/logos/so/so-icon.png',
						body: body,
					});

					notification.onclick = function () {
						window.open("http://stackoverflow.com/a/13328397/1269037");
					};
				}
			},
			messages: function () {
				return messagequeue;
			}
		};
	}]);
	
	/*
		The alertarea is on every page. It displays the browser notifications.
		It uses the notifyBrowser service to collect the pending notifications.
		Todo: Display them below each other instead of left to each other.
		Todo: Make a "dropdown" css effect
	*/
	app.directive('alertarea', function () {
		return {
			template: '<div class="alerts" style="position: absolute; right: 10px;" ng-shw="messages.length>0"><div class="alert alert-{{(m.type)||\'info\'}} alert-dismissable fade in pull-right" ng-repeat="m in messages"><button type="button" class="close" data-dismiss="alert">×</button><label>{{m.title}}</label><div>{{m.body}}</div></div></div>',
			restrict: "A",
			scope: {
			},
			controller: function ($scope, notifyBrowser) {
				$scope.messages = [];
				$scope.messages = notifyBrowser.messages();
					$scope.$watch(notifyBrowser.messages, function (messages) {
						$scope.messages = messages;
				});
			}
		};
	});
	
	app.filter('translate', function() {
		return function(input) {
			return $.t(input);
		}
	});

	app.factory('utils', function (bootbox) {
		return {
            confirmDecorator: bootbox.confirmDecorator
		};
    });

	app.factory('Device', deviceFactory);

    app.factory('bootbox', function($q) {
    	return {
    		confirm: confirm,
            confirmDecorator: confirmDecorator,
            alert: alert
		};

		function confirm(message) {
			return $q(function(resolve, reject) {
                bootbox.confirm($.t(message), function (result) {
                    if (result !== true) {
                        reject();
                    }
                    resolve();
                });
			});
		}

        function confirmDecorator(fn, message) {
            return function() {
                var args = arguments;

                return confirm(message).then(function () {
                    return fn.apply(null, args);
                }, angular.noop);
            };
        }

		function alert(message) {
			return bootbox.alert($.t(message));
        }
	});

    app.factory('dzNotification', function($q) {
        return {
			show: show,
            withNotificationDecorator: withNotificationDecorator
        };

        function show(text, timeout) {
            ShowNotify($.t(text), timeout);
            return {
                hide: HideNotify
			}
        }

        function withNotificationDecorator(fn, text) {
        	return function() {
                var args = arguments;
                var notification = show(text);

                var result = fn.apply(null, args);

                $q.when(result).finally(function() {
                	notification.hide();
				});

                return result;
			}
		}
    });

	app.config(function ($httpProvider) {
		var logsOutUserOn401 = ['$q', '$location', 'permissions', function ($q, $location, permissions) {
			return {
				request: function (config) {
					return config || $q.when(config);
				},
				requestError: function (request) {
					return $q.reject(request);
				},
				response: function (response) {
					return response || $q.when(response);
				},
				responseError: function (response) {
					if (response && response.status === 401) {
						var permissionList = {
							isloggedin: false,
							rights: -1
						};
						permissions.setPermissions(permissionList);
						$location.path('/Login');
						return $q.reject(response);
					}
					return $q.reject(response);
				}
			};
		}];
		$httpProvider.interceptors.push(logsOutUserOn401);
	});

	app.controller('NavbarController', function ($scope, $location) {
		$scope.getClass = function (path) {
			if ($location.path().substr(0, path.length) == path) {
				return true
			} else {
				return false;
			}
		}
	});

	app.controller('MainController', ['$scope', '$location', '$http', function ($scope, $location, $http) {
		$scope.$on('LOAD', function () {
			$scope.loading = true;
		});
		$scope.$on('UNLOAD', function () {
			$scope.loading = false;
		});
		$scope.$on('COMERROR', function () {
			$scope.communicationerror = true;
		});
		$scope.$on('COMOK', function () {
			$scope.communicationerror = false;
		});
	}]);

	app.factory('dzTimeAndSun', function($rootScope) {
		var currentData = {};
		init();

		return {
            getCurrentData: getCurrentData,
			updateData: updateData
		};

        function init() {
            $rootScope.$on('jsonupdate', function (event, data) {
                if (typeof data.ServerTime !== 'undefined') {
                    currentData.serverTime = data.ServerTime;
                }
                if (typeof data.Sunrise !== 'undefined') {
                    currentData.sunrise = data.Sunrise;
                }
                if (typeof data.Sunset !== 'undefined') {
                    currentData.sunset = data.Sunset;
                }
            });
        }

		function getCurrentData() {
			return currentData;
		}

		function updateData(data) {
			Object.assign(currentData, {
				sunrise: data.Sunrise,
				sunset: data.Sunset,
				serverTime: data.ServerTime
			});
		}
	});

    app.component('timesun', {
        templateUrl: 'timesuntemplate',
        controller: function (dzTimeAndSun) {
        	this.isMobile = $.myglobals.ismobile;
            this.data = dzTimeAndSun.getCurrentData();
        }
    });

    app.component('pageLoadingIndicator', {
    	template: '<section class="page-spinner">{{:: "Loading..." | translate }}</section>'
	});

	app.run(function ($rootScope, $location, $window, $route, $http, dzTimeAndSun, permissions) {
		var permissionList = {
			isloggedin: false,
			rights: -1
		};
		permissions.setPermissions(permissionList);

		$rootScope.MakeGlobalConfig = function () {
			//Ver bad (Old code!), should be changed soon!
			$.FiveMinuteHistoryDays = $rootScope.config.FiveMinuteHistoryDays;

			$.myglobals.ismobileint = false;
			if (typeof $rootScope.config.MobileType != 'undefined') {
				if (/Android|webOS|iPhone|iPad|iPod|BlackBerry/i.test(navigator.userAgent)) {
					$.myglobals.ismobile = true;
					$.myglobals.ismobileint = true;
				}
				if ($rootScope.config.MobileType != 0) {
					if (!(/iPhone/i.test(navigator.userAgent))) {
						$.myglobals.ismobile = false;
					}
				}
			}

			$.myglobals.DashboardType = $rootScope.config.DashboardType;
			$.myglobals.DateFormat = $rootScope.config.DateFormat;

			if (typeof $rootScope.config.WindScale != 'undefined') {
				$.myglobals.windscale = parseFloat($rootScope.config.WindScale);
			}
			if (typeof $rootScope.config.WindSign != 'undefined') {
				$.myglobals.windsign = $rootScope.config.WindSign;
			}
			if (typeof $rootScope.config.TempScale != 'undefined') {
				$.myglobals.tempscale = parseFloat($rootScope.config.TempScale);
			}
			if (typeof $rootScope.config.TempSign != 'undefined') {
				$.myglobals.tempsign = $rootScope.config.TempSign;
			}
			if (typeof $rootScope.config.DegreeDaysBaseTemperature != 'undefined') {
				$.myglobals.DegreeDaysBaseTemperature = $rootScope.config.DegreeDaysBaseTemperature;
			}
		}
		$rootScope.currentyear = new Date().getFullYear();
		$rootScope.config = {
			EnableTabProxy: false,
			EnableTabDashboard: false,
			EnableTabFloorplans: false,
			EnableTabLights: false,
			EnableTabScenes: false,
			EnableTabTemp: false,
			EnableTabWeather: false,
			EnableTabUtility: false,
			EnableTabCustom: false,
			AllowWidgetOrdering: true,
			FiveMinuteHistoryDays: 1,
			DashboardType: 1,
			Latitude: "52.216485",
			Longitude: "5.169528",
			MobileType: 0,
			TempScale: 1.0,
			DegreeDaysBaseTemperature: 18.0,
			TempSign: "C",
			WindScale: 3.600000143051148,
			WindSign: "km/h",
			language: "en",
			HaveUpdate: false,
			UseUpdate: true,
			appversion: 0,
			apphash: 0,
			appdate: 0,
			pythonversion: "",
			versiontooltip: "",
			ShowUpdatedEffect: true,
			DateFormat: "yy-mm-dd"
		};

		$rootScope.GetGlobalConfig = function () {
			//Get Config
			$.ajax({
				url: "json.htm?type=command&param=getconfig",
				async: false,
				dataType: 'json',
				success: function (data) {
					isOnline = true;
					if (data.status == "OK") {
						$rootScope.config.AllowWidgetOrdering = data.AllowWidgetOrdering;
						$rootScope.config.FiveMinuteHistoryDays = data.FiveMinuteHistoryDays;
						$rootScope.config.DashboardType = data.DashboardType;
						$rootScope.config.Latitude = data.Latitude;
						$rootScope.config.Longitude = data.Longitude;
						$rootScope.config.MobileType = data.MobileType;
						$rootScope.config.TempScale = data.TempScale;
						$rootScope.config.TempSign = data.TempSign;
						$rootScope.config.WindScale = data.WindScale;
						$rootScope.config.WindSign = data.WindSign;
						$rootScope.config.language = data.language;
						$rootScope.config.EnableTabProxy = data.result.EnableTabProxy,
						$rootScope.config.EnableTabDashboard = data.result.EnableTabDashboard,
						$rootScope.config.EnableTabFloorplans = data.result.EnableTabFloorplans;
						$rootScope.config.EnableTabLights = data.result.EnableTabLights;
						$rootScope.config.EnableTabScenes = data.result.EnableTabScenes;
						$rootScope.config.EnableTabTemp = data.result.EnableTabTemp;
						$rootScope.config.EnableTabWeather = data.result.EnableTabWeather;
						$rootScope.config.EnableTabUtility = data.result.EnableTabUtility;
						$rootScope.config.ShowUpdatedEffect = data.result.ShowUpdatedEffect;
						$rootScope.config.DegreeDaysBaseTemperature = data.result.DegreeDaysBaseTemperature;

						SetLanguage(data.language);

						//Translate Highcharts (partly)
						Highcharts.setOptions({
							lang: {
								months: [
									$.t('January'),
									$.t('February'),
									$.t('March'),
									$.t('April'),
									$.t('May'),
									$.t('June'),
									$.t('July'),
									$.t('August'),
									$.t('September'),
									$.t('October'),
									$.t('November'),
									$.t('December')
								],
								shortMonths: [
									$.t('Jan'),
									$.t('Feb'),
									$.t('Mar'),
									$.t('Apr'),
									$.t('May'),
									$.t('Jun'),
									$.t('Jul'),
									$.t('Aug'),
									$.t('Sep'),
									$.t('Oct'),
									$.t('Nov'),
									$.t('Dec')
								],
								weekdays: [
									$.t('Sunday'),
									$.t('Monday'),
									$.t('Tuesday'),
									$.t('Wednesday'),
									$.t('Thursday'),
									$.t('Friday'),
									$.t('Saturday')
								]
							}
						});

						$rootScope.MakeGlobalConfig();

						if (typeof data.result.templates != 'undefined') {
							var customHTML = "";
							$.each(data.result.templates, function (i, item) {
								var cFile = item;
								var cName = cFile.charAt(0).toUpperCase() + cFile.slice(1);
								var cURL = "templates/" + cFile;
								customHTML += '<li><a href="javascript:SwitchLayout(\'' + cURL + '\')">' + cName + '</a></li>';
							});
							if (customHTML != "") {
								$("#custommenu").html(customHTML);
								$rootScope.config.EnableTabCustom = data.result.EnableTabCustom;
							}
						}
					}
				},
				error: function () {
					isOnline = false;
				}
			});
		}

		$rootScope.GetGlobalConfig();
		$.ajax({
			url: "json.htm?type=command&param=getversion",
			async: false,
			dataType: 'json',
			success: function (data) {
			    isOnline = true;
				if (data.status == "OK") {
				    $rootScope.config.appversion = data.version;
					$rootScope.config.apphash = data.hash;
					$rootScope.config.appdate = data.build_time;
					$rootScope.config.dzventsversion = data.dzvents_version;
					$rootScope.config.pythonversion = data.python_version;
					$rootScope.config.isproxied = data.isproxied;
					$rootScope.config.versiontooltip = "'Build Hash: <b>" + $rootScope.config.apphash + "</b><br>" + "Build Date: " + $rootScope.config.appdate + "'";
					$("#appversion").text("V" + data.version);
					//if (data.SystemName != "windows") {
					    $rootScope.config.HaveUpdate = data.HaveUpdate;
					    $rootScope.config.UseUpdate = data.UseUpdate;
					//}
					//else {
					//    $rootScope.config.UseUpdate = false;
					//}
					if ((data.HaveUpdate == true) && (data.UseUpdate)) {
						ShowUpdateNotification(data.Revision, data.SystemName, data.DomoticzUpdateURL);
					}
				}
			},
			error: function () {
				isOnline = false;
			}
		});

		$.ajax({
			url: "json.htm?type=command&param=getauth",
			async: false,
			dataType: 'json',
			success: function (data) {
				isOnline = true;
				if (data.status == "OK") {
					permissionList.isloggedin = (data.user != "");
					permissionList.rights = parseInt(data.rights);
					dashboardType = data.DashboardType;
				}
			},
			error: function () {
				isOnline = false;
			}
		});

		$rootScope.$on("$routeChangeStart", function (scope, next, current) {
			if (!isOnline) {
				$location.path('/Offline');
			}
			else {
				if (next.templateUrl == "views/offline.html") {
					if (!isOnline) {
						//thats ok
					} else {
						//lets dont go to the offline page when we are online
						$location.path('/Dashboard');
					}
					return;
				}
				//if ((next.templateUrl=="views/dashboard.html")&&(dashboardType==3)) {
				//	$location.path('/Floorplans');
				//	return;
				//}

				if ((!permissions.isAuthenticated()) && (next.templateUrl != "views/login.html")) {
					$location.path('/Login');
					//$window.location = '/#Login';
					//$window.location.reload();
					return;
				}
				else if ((permissions.isAuthenticated()) && (next.templateUrl == "views/login.html")) {
					$location.path('/Dashboard');
					return;
				}

				if (next && next.$$route && next.$$route.permission) {
					var permission = next.$$route.permission;
					if (!permissions.hasPermission(permission)) {
						$location.path('/Dashboard');
					}
				}
			}
		});
		permissions.setPermissions(permissionList);

		Highcharts.setOptions({
			chart: {
				style: {
                    fontFamily: '"Lucida Grande", "Lucida Sans Unicode", Arial, Helvetica, sans-serif'
				}
			},
            credits: {
                enabled: true,
                href: "http://www.domoticz.com",
                text: "Domoticz.com"
            },
            title: {
                style: {
                    textTransform: 'none',
                    fontFamily: 'Trebuchet MS, Verdana, sans-serif',
                    fontSize: '16px',
					fontWeight: 'bold'
                }
            },
            legend: {
                itemStyle: {
                    fontFamily: 'Trebuchet MS, Verdana, sans-serif',
                    fontWeight: 'normal'
                }
            }
		});

		// TODO: use <timesun /> component instead
		$rootScope.SetTimeAndSun = function (sunRise, sunSet, ServerTime) {
			var month = ServerTime.split(' ')[0];
			ServerTime = ServerTime.replace(month, $.t(month));

			var suntext;
			var bIsMobile = $.myglobals.ismobile;
			if (bIsMobile == true) {
				suntext = '<div><font color="yellow">&#9728;</font>' + '&#9650;' + sunRise + ' ' + '&#9660;' + sunSet + '</div>';
			}
			else {
				//$.t('SunRise') + $.t('SunSet')
				suntext = '<div>' + ServerTime + ' <font color="yellow">&#9728;</font>' + '&#9650;' + sunRise + ' ' + '&#9660;' + sunSet + '</div>';
			}
			$("#timesun").html(suntext);
		}

		$rootScope.RefreshTimeAndSun = function (placeholder) {
			if (typeof $("#timesun") != 'undefined') {
				$http({
					url: "json.htm?type=command&param=getSunRiseSet",
					async: true,
					dataType: 'json'
				}).then(function successCallback(response) {
					var data = response.data;
					if (typeof data.Sunrise != 'undefined') {
                        dzTimeAndSun.updateData(response.data);
						$rootScope.SetTimeAndSun(data.Sunrise, data.Sunset, data.ServerTime);
					}
				});
			}
		};
		$rootScope.GetItemBackgroundStatus = function (item) {
			// generate protected/timeout/lowbattery status
			var backgroundClass = "statusNormal";
			if (item.HaveTimeout == true) {
				backgroundClass = "statusTimeout";
			}
			else {
				var BatteryLevel = parseInt(item.BatteryLevel);
				if (BatteryLevel != 255) {
					if (BatteryLevel <= 10) {
						backgroundClass = "statusLowBattery";
					}
					else if (item.Protected == true) {
						backgroundClass = "statusProtected";
					}
				}
				else if (item.Protected == true) {
					backgroundClass = "statusProtected";
				}
			}
			return backgroundClass;
		}
		$rootScope.DisplayTrend = function (trend) {
			//0=Unknown, 1=Stable, 2=Up, 3=Down
			if (typeof trend != 'undefined') {
				if (trend > 1)
					return true;
			}
			return false;
		};
		$rootScope.TrendState = function (trend) {
			if (trend == 0) return "unk";
			if (trend == 1) return "stable";
			if (trend == 2) return "up";
			if (trend == 3) return "down";
			return "unk";
		};

	});

	// Bootstrap Angular when DOM is ready
	return angularAMD.bootstrap(app);
});
