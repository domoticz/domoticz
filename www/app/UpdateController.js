define(['app'], function (app) {
	app.controller('UpdateController', ['$scope', '$rootScope', '$location', '$http', '$interval', '$window', function ($scope, $rootScope, $location, $http, $interval, $window) {

		$scope.appVersion = 0;
		$scope.newVersion = 0;
		$scope.topText = "";
		$scope.bottomText = "";
		$scope.updateReady = false;
		$scope.statusText = "";

		$scope.ProgressData = {
			label: 0,
			percentage: 0
		}
		
		$scope.CheckUpdateReader = function() {
			if (typeof $scope.mytimer2 != 'undefined') {
				$interval.cancel($scope.mytimer2);
				$scope.mytimer2 = undefined;
			}
			//$scope.statusText = "Getting version number...";
			$http({
				method : "GET",
				url : "json.htm?type=command&param=getversion",
				timeout: 5000
			}).then(
				function mySuccess(response) {
					var data = response.data;
					if (data.status == "OK") {
						$scope.newVersion = data.version;
						$("#appversion").text("V" + $scope.newVersion);
						if ($scope.appVersion != $scope.newVersion) {
							//We are ready !
							//$scope.statusText = "Got new version..";
							$scope.updateReady = true;
						} else {
							//$scope.statusText = "Got same version...";
							$scope.mytimer2 = $interval(function () {
								$scope.CheckUpdateReader();
							}, 1000);
						}
					} else {
						//$scope.statusText = "Got a error returned!...";
						$scope.mytimer2 = $interval(function () {
							$scope.CheckUpdateReader();
						}, 1500);
					}
				},
				function myError(response)   {
					//$scope.statusText = "Could not get version !";
					$scope.mytimer2 = $interval(function () {
						$scope.CheckUpdateReader();
					}, 1000);
				});
		}

		$scope.progressupdatesystem = function () {
			var val = $scope.ProgressData.label;
			$scope.ProgressData.label = val + 1;
			if ((val == 100)||($scope.updateReady)) {
				if (typeof $scope.mytimer != 'undefined') {
					$interval.cancel($scope.mytimer);
					$scope.mytimer = undefined;
				}
				if (typeof $scope.mytimer2 != 'undefined') {
					$interval.cancel($scope.mytimer2);
					$scope.mytimer2 = undefined;
				}
				if ($scope.updateReady == false) {
					$("#updatecontent #divprogress").hide();
					$scope.topText = $.t("Error while downloading Update,<br>check your internet connection or try again later !...");
				} else {
					$window.location = '/#Dashboard';
					$window.location.reload();
				}
			}
		}

		$scope.StartUpdate = function() {
			if (typeof $scope.mytimer != 'undefined') {
				$interval.cancel($scope.mytimer);
				$scope.mytimer = undefined;
			}
			$scope.ProgressData.label = 0;
			$("#updatecontent #divprogress").show();

			$http({
				method : "GET",
				url : "json.htm?type=command&param=update_application",
			}).then(
				function mySuccess(response) {
					var data = response.data;
					if (data.status == "OK") {
						$scope.mytimer = $interval(function () {
							$scope.progressupdatesystem();
						}, 600);
						$scope.mytimer2 = $interval(function () {
							$scope.CheckUpdateReader();
						}, 1000);
					} else {
						$scope.topText = $.t("Could not start download,<br>check your internet connection or try again later !...");					
					}
				},
				function myError(response)   {
					$scope.topText = $.t("Error communicating with Server !...");
				}
			);
		}

		$scope.CheckForUpdate = function() {
			//$scope.statusText = "Getting version number...";
			$http({
				method : "GET",
				url : "json.htm?type=command&param=checkforupdate&forced=true"
			}).then(
				function mySuccess(response) {
					var data = response.data;
					if (data.HaveUpdate == true) {
						$scope.topText = $.t("Update Available... Downloading Update !...");
						$scope.mytimer = $interval(function () {
							$scope.StartUpdate();
						}, 400);
					} else {
						$scope.topText = $.t("No Update Available !...");
					}
				},
				function myError(response)   {
					$scope.topText = $.t("Error communicating with Server !...");
				});
		}

		$scope.init = function() {
			$scope.$watch('ProgressData', function (newValue, oldValue) {
				newValue.percentage = newValue.label / 100;
			}, true);

			$scope.appVersion = $rootScope.config.appversion;

			$scope.topText = $.t("Checking for updates....");
			$scope.bottomText = $.t("Do not poweroff the system while updating !...");

			$scope.CheckForUpdate();
		};
		
		$scope.init();
		
		$scope.$on('$destroy', function () {
			if (typeof $scope.mytimer != 'undefined') {
				$interval.cancel($scope.mytimer);
				$scope.mytimer = undefined;
			}
			if (typeof $scope.mytimer2 != 'undefined') {
				$interval.cancel($scope.mytimer2);
				$scope.mytimer2 = undefined;
			}
		});
	}]);
});