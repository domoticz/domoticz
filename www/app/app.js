define(['angularAMD', 'angular-route', 'angular-animate', 'ng-grid', 'ng-grid-flexible-height', 'highcharts-ng', 'angular-tree-control','ngDraggable','ngSanitize','angular-md5','ui.bootstrap','angular.directives-round-progress','angular.scrollglue'], function (angularAMD) {
	var app = angular.module('domoticz', ['ngRoute','ngAnimate','ngGrid','highcharts-ng', 'treeControl','ngDraggable','ngSanitize','angular-md5','ui.bootstrap','angular.directives-round-progress','angular.directives-round-progress','angular.scrollglue']);

		isOnline=false;
		dashboardType=1;

	  app.factory('permissions', function ($rootScope) {
		var permissionList;
		return {
		  setPermissions: function(permissions) {
			permissionList = permissions;
			window.my_config =
			{
				userrights : permissionList.rights
			}; 			
			$rootScope.$broadcast('permissionsChanged');
		  },
		  hasPermission: function (permission) {
			if (permission == "Admin") {
				return (permissionList.rights==2);
			}
			if (permission == "User") {
				return (permissionList.rights>=0);
			}
			if (permission == "Viewer") {
				return (permissionList.rights==0);
			}
			alert("Unknown permission request: " + permission);
			return false;
		  },
		  hasLogin: function (isloggedin) {
			return (permissionList.isloggedin == isloggedin);
		  },
		  isAuthenticated: function () {
			return (permissionList.rights!=-1);
		  }
	   };
	});
	app.directive('hasPermission', function(permissions) {
	  return {
		link: function(scope, element, attrs) {
		  var value = attrs.hasPermission.trim();
		  var notPermissionFlag = value[0] === '!';
		  if(notPermissionFlag) {
			value = value.slice(1).trim();
		  }
	 
		  function toggleVisibilityBasedOnPermission() {
			var hasPermission = permissions.hasPermission(value);
			if(hasPermission && !notPermissionFlag || !hasPermission && notPermissionFlag)
			  element.show();
			else
			  element.hide();
		  }
		  toggleVisibilityBasedOnPermission();
		  scope.$on('permissionsChanged', toggleVisibilityBasedOnPermission);
		}
	  };
	});
	app.directive('hasLogin', function(permissions) {
	  return {
		link: function(scope, element, attrs) {
			var bvalue = (attrs.hasLogin === 'true');
			function toggleVisibilityBasedOnPermission() {
				if(permissions.hasLogin(bvalue))
					element.show();
				else
					element.hide();
			}
			toggleVisibilityBasedOnPermission();
			scope.$on('permissionsChanged', toggleVisibilityBasedOnPermission);
		}
	  };
	});
	app.directive('hasLoginNoAdmin', function(permissions) {
	  return {
		link: function(scope, element, attrs) {
			function toggleVisibilityBasedOnPermission() {
				var bVisible=!permissions.hasPermission("Admin");
				if (bVisible)
				{
					bVisible=permissions.hasLogin(true);
				}
				if(bVisible == true)
					element.show();
				else
					element.hide();
			}
			toggleVisibilityBasedOnPermission();
			scope.$on('permissionsChanged', toggleVisibilityBasedOnPermission);
		}
	  };
	});
	app.directive('hasUser', function(permissions) {
	  return {
		link: function(scope, element, attrs) {
			function toggleVisibilityBasedOnPermission() {
				if(permissions.isAuthenticated())
					element.show();
				else
					element.hide();
			}
			toggleVisibilityBasedOnPermission();
			scope.$on('permissionsChanged', toggleVisibilityBasedOnPermission);
		}
	  };
	});
	app.directive('sbLoad', ['$parse', function ($parse) {
	return {
	  restrict: 'A',
	  link: function (scope, elem, attrs) {
		var fn = $parse(attrs.sbLoad);
		elem.on('load', function (event) {
		  scope.$apply(function() {
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
			link: function(scope, element, attrs) {
				var model = $parse(attrs.fileModel);
				var modelSetter = model.assign;
				
				element.bind('change', function(){
					scope.$apply(function(){
						modelSetter(scope, element[0].files[0]);
					});
				});
			}
		};
	}]);
	app.directive('formAutofillFix', function() {
	  return function(scope, elem, attrs) {
		// Fixes Chrome bug: https://groups.google.com/forum/#!topic/angular/6NlucSskQjY
		elem.prop('method', 'POST');

		// Fix autofill issues where Angular doesn't know about autofilled inputs
		if(attrs.ngSubmit) {
		  setTimeout(function() {
			elem.unbind('submit').submit(function(e) {
			  e.preventDefault();
			  elem.find('input, textarea, select').trigger('input').trigger('change').trigger('keydown');
			  scope.$apply(attrs.ngSubmit);
			});
		  }, 0);
		}
	  };
	});
	app.config(function($routeProvider,$locationProvider) {
			$routeProvider.
			  when('/Dashboard', angularAMD.route({
				templateUrl: 'views/dashboard.html',
				controller: 'DashboardController'
			  })).
			  when('/Devices', angularAMD.route({
				templateUrl: 'views/devices.html',
				controller: 'DevicesController'
			  })).
			  when('/DPFibaro', angularAMD.route({
				templateUrl: 'views/dpfibaro.html',
				controller: 'DPFibaroController',
				permission: 'Admin'
			  })).
			  when('/DPHttp', angularAMD.route({
				templateUrl: 'views/dphttp.html',
				controller: 'DPHttpController',
				permission: 'Admin'
			  })).
			  when('/DPGooglePubSub', angularAMD.route({
				templateUrl: 'views/dpgooglepubsub.html',
				controller: 'DPGooglePubSubController',
				permission: 'Admin'
			  })).
			  when('/Events', angularAMD.route({
				templateUrl: 'views/events.html',
				controller: 'EventsController',
				permission: 'Admin'
			  })).
			  when('/Floorplans', angularAMD.route({
				templateUrl: 'views/floorplans.html',
				controller: 'FloorplanController'
			  })).
			  when('/Floorplanedit', angularAMD.route({
				templateUrl: 'views/floorplanedit.html',
				controller: 'FloorplanEditController',
				permission: 'Admin'
			  })).
			  when('/Forecast', angularAMD.route({
				templateUrl: 'views/forecast.html',
				controller: 'ForecastController'
			  })).
			  when('/Frontpage', angularAMD.route({
				templateUrl: 'views/frontpage.html',
				controller: 'FrontpageController'
			  })).
			  when('/Hardware', angularAMD.route({
				templateUrl: 'views/hardware.html',
				controller: 'HardwareController',
				permission: 'Admin'
			  })).
			  when('/History', angularAMD.route({
				templateUrl: 'views/history.html',
				controller: 'HistoryController'
			  })).
			  when('/LightSwitches', angularAMD.route({
				templateUrl: 'views/lights.html',
				controller: 'LightsController',
			  })).
			  when('/Lights', angularAMD.route({
				templateUrl: 'views/lights.html',
				controller: 'LightsController'
			  })).
			  when('/Log', angularAMD.route({
				templateUrl: 'views/log.html',
				controller: 'LogController',
				permission: 'Admin'
			  })).
			  when('/Login', angularAMD.route({
				templateUrl: 'views/login.html',
				controller: 'LoginController'
			  })).
			  when('/Logout', angularAMD.route({
				templateUrl: 'views/logout.html',
				controller: 'LogoutController'
			  })).
			  when('/Offline', angularAMD.route({
				templateUrl: 'views/offline.html',
				controller: 'OfflineController'
			  })).
			  when('/Notification', angularAMD.route({
				templateUrl: 'views/notification.html',
				controller: 'NotificationController',
				permission: 'Admin'
			  })).
			  when('/RestoreDatabase', angularAMD.route({
				templateUrl: 'views/restoredatabase.html',
				controller: 'RestoreDatabaseController',
				permission: 'Admin'
			  })).
			  when('/RFXComFirmware', angularAMD.route({
				templateUrl: 'views/rfxcomfirmware.html',
				controller: 'RFXComFirmwareController',
				permission: 'Admin'
			  })).
			  when('/Cam', angularAMD.route({
				templateUrl: 'views/cam.html',
				controller: 'CamController',
				permission: 'Admin'
			  })).
			  when('/CustomIcons', angularAMD.route({
				templateUrl: 'views/customicons.html',
				controller: 'CustomIconsController',
				permission: 'Admin'
			  })).
			  when('/Roomplan', angularAMD.route({
				templateUrl: 'views/roomplan.html',
				controller: 'RoomplanController',
				permission: 'Admin'
			  })).
			  when('/Scenes', angularAMD.route({
				templateUrl: 'views/scenes.html',
				controller: 'ScenesController'
			  })).
			  when('/Setup', angularAMD.route({
				templateUrl: 'views/setup.html',
				controller: 'SetupController',
				permission: 'Admin'
			  })).
			  when('/Temperature', angularAMD.route({
				templateUrl: 'views/temperature.html',
				controller: 'TemperatureController',
				controllerAs: 'ctrl'
			  })).
				when('/Temperature/:idx/Notifications', angularAMD.route({
					templateUrl: 'views/temperature_notifications.html',
					controller: 'TemperatureNotificationsController',
					controllerAs: 'ctrl'
				})).
			when('/Temperature/CustomTempLog', angularAMD.route({
				templateUrl: 'views/temperature_custom_temp_log.html',
				controller: 'TemperatureCustomLogController',
				controllerAs: 'ctrl'
			})).
			when('/Temperature/:idx/Log', angularAMD.route({
				templateUrl: 'views/temperature_log.html',
				controller: 'TemperatureLogController',
				controllerAs: 'ctrl'
			})).
			  when('/Update', angularAMD.route({
				templateUrl: 'views/update.html',
				controller: 'UpdateController',
				permission: 'Admin'
			  })).
			  when('/Users', angularAMD.route({
				templateUrl: 'views/users.html',
				controller: 'UsersController',
				permission: 'Admin'
			  })).
			  when('/UserVariables', angularAMD.route({
				templateUrl: 'views/uservariables.html',
				controller: 'UserVariablesController',
				permission: 'Admin'
			  })).
			  when('/Utility', angularAMD.route({
				templateUrl: 'views/utility.html',
				controller: 'UtilityController'
			  })).
			  when('/Weather', angularAMD.route({
				templateUrl: 'views/weather.html',
				controller: 'WeatherController',
				controllerAs: 'ctrl'
			  })).
			  when('/ZWaveTopology', angularAMD.route({
				templateUrl: 'zwavetopology.html',
				controller: 'ZWaveTopologyController',
				permission: 'Admin'
			  })).
			  when('/About', angularAMD.route({
				templateUrl: 'views/about.html',
				controller: 'AboutController'
			  })).
			  when('/Custom/:custompage', angularAMD.route({
					templateUrl: function(params) {
						return 'templates/' + params.custompage + '.html';
					},
					controller: 'DummyController'
				})
			  ).
			  otherwise({
				redirectTo: '/Dashboard'
			  });
			// Use html5 mode.
			//$locationProvider.html5Mode(true);
	});
	
	app.config(function($httpProvider) {
		var logsOutUserOn401 = ['$q', '$location', 'permissions', function ($q, $location,permissions) {
			return {
				request: function (config) {
					return config || $q.when(config);
				},
				requestError: function(request){
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

	app.controller('MainController', [ '$scope', '$location', '$http', function($scope,$location,$http) {
		$scope.$on('LOAD', function() {
			$scope.loading=true;
		});
		$scope.$on('UNLOAD', function() {
			$scope.loading=false;
		});
		$scope.$on('COMERROR', function() {
			$scope.communicationerror=true;
		});
		$scope.$on('COMOK', function() {
			$scope.communicationerror=false;
		});
	} ]);

	
	app.run(function($rootScope, $location, $window, $route, $http, permissions) {
		var permissionList = {
				isloggedin: false,
				rights: -1
		};
		permissions.setPermissions(permissionList);					

		$rootScope.MakeGlobalConfig = function()
		{
			//Ver bad (Old code!), should be changed soon!
			$.FiveMinuteHistoryDays = $rootScope.config.FiveMinuteHistoryDays;
			
			$.myglobals.ismobileint=false;
			if (typeof $rootScope.config.MobileType != 'undefined') {
				if( /Android|webOS|iPhone|iPad|iPod|BlackBerry/i.test(navigator.userAgent) ) {
					$.myglobals.ismobile=true;
					$.myglobals.ismobileint=true;
				}
				if ($rootScope.config.MobileType!=0) {
					if(!(/iPhone/i.test(navigator.userAgent) )) {
						$.myglobals.ismobile=false;
					}
				}
			}
			
			$.myglobals.DashboardType=$rootScope.config.DashboardType;
			
			if (typeof $rootScope.config.WindScale != 'undefined') {
				$.myglobals.windscale=parseFloat($rootScope.config.WindScale);
			}
			if (typeof $rootScope.config.WindSign != 'undefined') {
				$.myglobals.windsign=$rootScope.config.WindSign;
			}
			if (typeof $rootScope.config.TempScale != 'undefined') {
				$.myglobals.tempscale=parseFloat($rootScope.config.TempScale);
			}
			if (typeof $rootScope.config.TempSign != 'undefined') {
				$.myglobals.tempsign=$rootScope.config.TempSign;
			}
			if (typeof $rootScope.config.DegreeDaysBaseTemperature != 'undefined') {
				$.myglobals.DegreeDaysBaseTemperature=$rootScope.config.DegreeDaysBaseTemperature;
			}
		}
		$rootScope.currentyear = new Date().getFullYear();
		$rootScope.config={
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
				DashboardType : 1,
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
				appversion: 0,
				apphash: 0,
				appdate: 0,
				versiontooltip: "",
				ShowUpdatedEffect: true
				};

		$rootScope.GetGlobalConfig = function()
		{
			//Get Config
			$.ajax({
			 url: "json.htm?type=command&param=getconfig",
			 async: false, 
			 dataType: 'json',
			 success: function(data) {
				isOnline = true;
				if (data.status == "OK") {
					$rootScope.config.AllowWidgetOrdering=data.AllowWidgetOrdering;
					$rootScope.config.FiveMinuteHistoryDays=data.FiveMinuteHistoryDays;
					$rootScope.config.DashboardType=data.DashboardType;
					$rootScope.config.Latitude=data.Latitude;
					$rootScope.config.Longitude=data.Longitude;
					$rootScope.config.MobileType=data.MobileType;
					$rootScope.config.TempScale=data.TempScale;
					$rootScope.config.TempSign=data.TempSign;
					$rootScope.config.WindScale=data.WindScale;
					$rootScope.config.WindSign=data.WindSign;
					$rootScope.config.language=data.language;
					$rootScope.config.EnableTabProxy=data.result.EnableTabProxy,
					$rootScope.config.EnableTabDashboard=data.result.EnableTabDashboard,
					$rootScope.config.EnableTabFloorplans=data.result.EnableTabFloorplans;
					$rootScope.config.EnableTabLights=data.result.EnableTabLights;
					$rootScope.config.EnableTabScenes=data.result.EnableTabScenes;
					$rootScope.config.EnableTabTemp=data.result.EnableTabTemp;
					$rootScope.config.EnableTabWeather=data.result.EnableTabWeather;
					$rootScope.config.EnableTabUtility=data.result.EnableTabUtility;
					$rootScope.config.ShowUpdatedEffect=data.result.ShowUpdatedEffect;
					$rootScope.config.DegreeDaysBaseTemperature=data.result.DegreeDaysBaseTemperature;
				
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

					if (typeof data.result.templates!= 'undefined') {
						var customHTML="";
						$.each(data.result.templates, function(i,item)
						{
							var cFile=item;
							var cName=cFile.charAt(0).toUpperCase() + cFile.slice(1);
							var cURL="templates/"+cFile;
							customHTML+='<li><a href="javascript:SwitchLayout(\'' + cURL + '\')">' + cName + '</a></li>';
						});
						if (customHTML!="") {
							$("#custommenu").html(customHTML);
							$rootScope.config.EnableTabCustom=data.result.EnableTabCustom;
						}
					}
				}
			 },
			 error: function() {
				isOnline=false;
			 }
			});
		}
		
		$rootScope.GetGlobalConfig();

		$.ajax({
		 url: "json.htm?type=command&param=getversion",
		 async: false, 
		 dataType: 'json',
		 success: function(data) {
			isOnline = true;
			if (data.status == "OK") {
				$rootScope.config.appversion=data.version;
				$rootScope.config.apphash=data.hash;
				$rootScope.config.appdate=data.build_time;
				$rootScope.config.versiontooltip="'Build Hash: <b>" + $rootScope.config.apphash + "</b><br>" + "Build Date: " + $rootScope.config.appdate+"'";
				$( "#appversion" ).text("V" + data.version);
				if (data.SystemName!="windows") {
					$rootScope.config.HaveUpdate=data.HaveUpdate;
				}
				$rootScope.config.isproxied=data.isproxied;
				if (data.HaveUpdate == true)
				{
					ShowUpdateNotification(data.Revision,data.SystemName,data.DomoticzUpdateURL);
				}
			}
		 },
		 error: function() {
			isOnline=false;
		 }
		});
		
		$.ajax({
		 url: "json.htm?type=command&param=getauth",
		 async: false, 
		 dataType: 'json',
		 success: function(data) {
			isOnline = true;
			if (data.status == "OK") {
				permissionList.isloggedin=(data.user!="");
				permissionList.rights=parseInt(data.rights);
				dashboardType=data.DashboardType;
			}
		 },
		 error: function() {
			isOnline=false;
		 }
		});
	
		$rootScope.$on("$routeChangeStart", function (scope, next, current) {
			if (!isOnline) {
				$location.path('/Offline');
			}
			else {
				if (next.templateUrl=="views/offline.html") {
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
				
				if ( (!permissions.isAuthenticated()) && (next.templateUrl!="views/login.html") ) {
					$location.path('/Login');
					//$window.location = '/#Login';
					//$window.location.reload();
					return;
				}
				else if ( (permissions.isAuthenticated()) && (next.templateUrl=="views/login.html") ) {
					$location.path('/Dashboard');
					return;
				}
				
				if (next && next.$$route && next.$$route.permission) {
					var permission = next.$$route.permission;
					if(!permissions.hasPermission(permission)) {
						$location.path('/Dashboard');
					}
				}
			}
        });
        permissions.setPermissions(permissionList);
        
        $rootScope.SetTimeAndSun = function(sunRise, sunSet, ServerTime)
        {
			var month=ServerTime.split(' ')[0];
			ServerTime=ServerTime.replace(month,$.t(month));
			
			var suntext;
			var bIsMobile=$.myglobals.ismobile;
			if (bIsMobile == true) {
				suntext='<div><font color="yellow">&#9728;</font>' + '&#9650;' + sunRise + ' ' +  '&#9660;' + sunSet + '</div>';
			}
			else {
				//$.t('SunRise') + $.t('SunSet')
				suntext='<div>'+ServerTime + ' <font color="yellow">&#9728;</font>' + '&#9650;' + sunRise + ' ' +  '&#9660;' + sunSet + '</div>';
			}
			$("#timesun").html(suntext);
        }
        
		$rootScope.RefreshTimeAndSun = function(placeholder) {
			if (typeof $("#timesun") != 'undefined') {
				$http({
				 url: "json.htm?type=command&param=getSunRiseSet",
				 async: true, 
				 dataType: 'json'
				}).success(function(data) {
					if (typeof data.Sunrise != 'undefined') {
						$rootScope.SetTimeAndSun(data.Sunrise,data.Sunset,data.ServerTime);
					}
				});
			}
		};
	});

    // Bootstrap Angular when DOM is ready
    return angularAMD.bootstrap(app);
}); 
