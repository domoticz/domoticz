define(['angularAMD', 'angular-route', 'angular-animate', 'ng-grid', 'ng-grid-flexible-height', 'highcharts-ng', 'angular-tree-control','ngDraggable'], function (angularAMD) {
	var app = angular.module('domoticz', ['ngRoute','ngAnimate','ngGrid','highcharts-ng', 'treeControl','ngDraggable']);

	  app.factory('permissions', function ($rootScope) {
		var permissionList;
		return {
		  setPermissions: function(permissions) {
			permissionList = permissions;
			$rootScope.$broadcast('permissionsChanged')
		  },
		  hasPermission: function (permission) {
			if (permission == "Admin") {
				return (permissionList.rights==2);
			}
			if (permission == "User") {
				return (permissionList.rights>=0);
			}
			alert("Unknown permission: " + permission);
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
	
	app.config(function($routeProvider) {
			$routeProvider.
			  when('/Dashboard', angularAMD.route({
				templateUrl: 'views/dashboard.html',
				controller: 'DashboardController'
			  })).
			  when('/Cam', angularAMD.route({
				templateUrl: 'views/cam.html',
				controller: 'CamController'
			  })).
			  when('/Devices', angularAMD.route({
				templateUrl: 'views/devices.html',
				controller: 'DevicesController'
			  })).
			  when('/DPFibaro', angularAMD.route({
				templateUrl: 'views/dpfibaro.html',
				controller: 'DPFibaroController'
			  })).
			  when('/Events', angularAMD.route({
				templateUrl: 'views/events.html',
				controller: 'EventsController'
			  })).
			  when('/Floorplans', angularAMD.route({
				templateUrl: 'views/floorplans.html',
				controller: 'FloorplanController'
			  })).
			  when('/Floorplanedit', angularAMD.route({
				templateUrl: 'views/floorplanedit.html',
				controller: 'FloorplanController'
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
				controller: 'HardwareController'
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
				controller: 'LogController'
			  })).
			  when('/Login', angularAMD.route({
				templateUrl: 'views/login.html',
				controller: 'LoginController'
			  })).
			  when('/Logout', angularAMD.route({
				templateUrl: 'views/logout.html',
				controller: 'LogoutController'
			  })).
			  when('/Notification', angularAMD.route({
				templateUrl: 'views/notification.html',
				controller: 'NotificationController'
			  })).
			  when('/RestoreDatabase', angularAMD.route({
				templateUrl: 'views/restoredatabase.html',
				controller: 'RestoreDatabaseController'
			  })).
			  when('/Roomplan', angularAMD.route({
				templateUrl: 'views/roomplan.html',
				controller: 'RoomplanController'
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
				controller: 'TemperateController'
			  })).
			  when('/Update', angularAMD.route({
				templateUrl: 'views/update.html',
				controller: 'UpdateController'
			  })).
			  when('/Users', angularAMD.route({
				templateUrl: 'views/users.html',
				controller: 'UsersController'
			  })).
			  when('/UserVariables', angularAMD.route({
				templateUrl: 'views/uservariables.html',
				controller: 'UserVariablesController'
			  })).
			  when('/Utility', angularAMD.route({
				templateUrl: 'views/utility.html',
				controller: 'UtilityController'
			  })).
			  when('/Weather', angularAMD.route({
				templateUrl: 'views/weather.html',
				controller: 'WeatherController'
			  })).
			  when('/ZWaveTopology', angularAMD.route({
				templateUrl: 'views/zwavetopology.html',
				controller: 'ZWaveTopologyController'
			  })).
			  otherwise({
				redirectTo: '/Dashboard'
			  });
	});
	/*
	app.config(['$httpProvider',function($httpProvider) {
		//Http Intercpetor to check auth failures for xhr requests
		alert("AA");
		$httpProvider.interceptors.push('securityInterceptor');
		//$httpProvider.responseInterceptors.push('securityInterceptor');
	}]);

	app.provider('securityInterceptor', function() {
		alert("hoi!");
		this.$get = function($location, $q) {
			return function(promise) {
				return promise.then(null, function(response) {
					if(response.status === 403 || response.status === 401) {
						$location.path('/Login');
					}
					return $q.reject(response);
				});
			};
		};
	});
	*/
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

	
	app.run(function($rootScope, $location, $route, permissions) {
		alert("Run!");
		$rootScope.$on("$routeChangeStart", function (scope, next, current) {
			if (
				(!permissions.isAuthenticated()) &&
				(next.templateUrl!="views/login.html")
				) {
					$location.path('/Login');
					return;
			}
			if (next && next.$$route && next.$$route.permission) {
				var permission = next.$$route.permission;
				if(!permissions.hasPermission(permission)) {
					$location.path('/Dashboard');
				}
			}
        });
        permissions.setPermissions(permissionList);
	});

	$.ajax({
	 url: "json.htm?type=command&param=getversion",
	 async: true, 
	 dataType: 'json',
	 success: function(data) {
		if (data.status == "OK") {
			$( "#appversion" ).text("V" + data.version);
		}
	 },
	 error: function(){
	 }
	});
	permissionList = {
			isloggedin: false,
			rights: -1
	};
	$.ajax({
	 url: "json.htm?type=command&param=getauth",
	 async: false, 
	 dataType: 'json',
	 success: function(data) {
		if (data.status == "OK") {
			permissionList.isloggedin=(data.user!="");
			permissionList.rights=parseInt(data.rights);
		}
	 }
	});
	
	
    // Bootstrap Angular when DOM is ready
    return angularAMD.bootstrap(app);
}); 
