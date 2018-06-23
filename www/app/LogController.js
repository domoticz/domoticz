define(['app'], function (app) {
	app.controller('LogController', ['$scope', '$rootScope', '$location', '$http', '$interval', '$sce', function ($scope, $rootScope, $location, $http, $interval, $sce) {

		$scope.LastLogTime = 0;
		$scope.logitems = [];
		$scope.logitems_status = [];
		$scope.logitems_error = [];
		$scope.logitems_debug = [];
		var LOG_NORM = 0x0000001;
		var LOG_STATUS = 0x0000002;
		var LOG_ERROR = 0x0000004;
		var LOG_DEBUG = 0x0000008;
		var LOG_ALL = 0xFFFFFFF;

		$scope.RefreshLog = function () {
			if (typeof $scope.mytimer != 'undefined') {
				$interval.cancel($scope.mytimer);
				$scope.mytimer = undefined;
			}

			var bWasAtBottom = true;
			var div = $("#logcontent #logdata");
			if (div != null) {
				if (div[0] != null) {
					var diff = Math.abs((div[0].scrollHeight - div.scrollTop()) - div.height());
					if (diff > 9) {
						bWasAtBottom = false;
					}
				}
			}
			var lastscrolltop = $("#logcontent #logdata").scrollTop();
			var llogtime = $scope.LastLogTime;
			$http({
			    url: "json.htm?type=command&param=getlog&lastlogtime=" + $scope.LastLogTime + "&loglevel=" + LOG_ALL,
				async: false,
				dataType: 'json'
			}).then(function successCallback(response) {
				var data = response.data;
				if (typeof data.result != 'undefined') {
					if (typeof data.LastLogTime != 'undefined') {
						$scope.LastLogTime = parseInt(data.LastLogTime);
					}
					$.each(data.result, function (i, item) {
					    var message = item.message.replace(/\n/gi, "<br>");
					    var lines = message.split("<br>")
					    if (lines.length < 1) return;
					    var fline = lines[0].split(" ")
					    if (fline.length < 3) return;

					    var sdate = fline[0];
					    var stime = fline[1];

					    for (i = 0; i < lines.length; i++) {
					        var lmessage = "";
					        if (i == 0) {
					            lmessage = lines[i];
					        }
					        else {
					            lmessage = sdate + " " + stime + " " + lines[i];
					        }
					        var logclass = "";
					        logclass = getLogClass(item.level);
					        $scope.logitems = $scope.logitems.concat({
					            mclass: logclass,
					            text: lmessage
					        });
					        if ($scope.logitems.length >= 300)
					            $scope.logitems.splice(0, ($scope.logitems.length - 300));
					        if (item.level == LOG_ERROR) {
					            //Error
					            $scope.logitems_error = $scope.logitems_error.concat({
					                mclass: logclass,
					                text: lmessage
					            });
					            if ($scope.logitems_error.length >= 300)
					                $scope.logitems_error.splice(0, ($scope.logitems_error.length - 300));
                            }
					        else if (item.level == LOG_STATUS) {
					            //Status
					            $scope.logitems_status = $scope.logitems_status.concat({
					                mclass: logclass,
					                text: lmessage
					            });
					            if ($scope.logitems_status.length >= 300)
					                $scope.logitems_status.splice(0, ($scope.logitems_status.length - 300));
                            }
					        else if (item.level == LOG_DEBUG) {
					            //Debug
					            $scope.logitems_debug = $scope.logitems_debug.concat({
					                mclass: logclass,
					                text: lmessage
					            });
					            if ($scope.logitems_debug.length >= 300)
					                $scope.logitems_debug.splice(0, ($scope.logitems_debug.length - 300));
                            }
                        }
					});
				}
				$scope.mytimer = $interval(function () {
					$scope.RefreshLog();
				}, 5000);
			}, function errorCallback(response) {
				$scope.mytimer = $interval(function () {
					$scope.RefreshLog();
				}, 5000);
			});
		}

		$scope.ClearLog = function () {
			if (typeof $scope.mytimer != 'undefined') {
				$interval.cancel($scope.mytimer);
				$scope.mytimer = undefined;
			}
			$http({
				url: "json.htm?type=command&param=clearlog",
				async: false,
				dataType: 'json'
			}).then(function successCallback(response) {
				var data = response.data;
				$scope.logitems = [];
				$scope.logitems_error = [];
				$scope.logitems_status = [];
				$scope.logitems_debug = [];
				$scope.mytimer = $interval(function () {
					$scope.RefreshLog();
				}, 5000);
			}, function errorCallback(response) {
				$scope.mytimer = $interval(function () {
					$scope.RefreshLog();
				}, 5000);
			});
		}

		$scope.ResizeLogWindow = function () {
			var pheight = $(window).innerHeight();
			$("#logcontent #logdata").height(pheight - 160);
			$("#logcontent #logdata_status").height(pheight - 160);
			$("#logcontent #logdata_error").height(pheight - 160);
			$("#logcontent #logdata_debug").height(pheight - 160);
		}

		init();

		function init() {
			$("#logcontent").i18n();
			$scope.LastLogTime = 0;
			$scope.RefreshLog();
			$(window).resize(function () { $scope.ResizeLogWindow(); });
			$scope.ResizeLogWindow();
		};

		function getLogClass(level) {
			var logclass;
			if (level == LOG_STATUS) {
				logclass = "logstatus";
			}
			else if (level == LOG_ERROR) {
				logclass = "logerror";
			}
			else {
				logclass = "lognorm";
			}
			return (logclass);
		}

		$scope.$on('$destroy', function () {
			if (typeof $scope.mytimer != 'undefined') {
				$interval.cancel($scope.mytimer);
				$scope.mytimer = undefined;
			}
			$(window).off("resize");
		});

	}]);
});