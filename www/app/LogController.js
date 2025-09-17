define(['app'], function (app) {
	app.controller('LogController', ['$scope', '$rootScope', '$location', '$http', '$interval', '$sce', 'livesocket', function ($scope, $rootScope, $location, $http, $interval, $sce, livesocket) {

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

		$scope.addLogLine = function(item) {
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

				//All
				if ($scope.logitems.length >= 300)
					$scope.logitems.splice(0, ($scope.logitems.length - 300));
				$scope.logitems = $scope.logitems.concat({
					mclass: logclass,
					text: lmessage
				});
				if (item.level == LOG_ERROR) {
					//Error
					if ($scope.logitems_error.length >= 300)
						$scope.logitems_error.splice(0, ($scope.logitems_error.length - 300));
					$scope.logitems_error = $scope.logitems_error.concat({
						mclass: logclass,
						text: lmessage
					});
				}
				else if (item.level == LOG_STATUS) {
					//Status
					if ($scope.logitems_status.length >= 300)
						$scope.logitems_status.splice(0, ($scope.logitems_status.length - 300));
					$scope.logitems_status = $scope.logitems_status.concat({
						mclass: logclass,
						text: lmessage
					});
				}
				else if (item.level == LOG_DEBUG) {
					//Debug
					$scope.logitems_debug = $scope.logitems_debug.concat({
						mclass: logclass,
						text: lmessage
					});
				}
			}
		}

		$scope.RefreshLog = function () {
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
						$scope.addLogLine(item);
					});
				}
			}, function errorCallback(response) {
			});
		}

		$scope.ClearLog = function () {
			livesocket.unsubscribeFrom("log");
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
				livesocket.subscribeTo("log");
			}, function errorCallback(response) {
				livesocket.subscribeTo("log");
			});
		}

		$scope.ResizeLogWindow = function () {
			var pheight = $(window).innerHeight();
			var poffset = 210;
			$("#logcontent #logdata").height(pheight - poffset);
			$("#logcontent #logdata_status").height(pheight - poffset);
			$("#logcontent #logdata_error").height(pheight - poffset);
			$("#logcontent #logdata_debug").height(pheight - poffset);
		}

		init();

		function init() {
			$("#logcontent").i18n();
			$scope.LastLogTime = 0;
			$scope.RefreshLog();
			$(window).resize(function () { $scope.ResizeLogWindow(); });
			$scope.ResizeLogWindow();
			livesocket.subscribeTo("log");
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

		$scope.$on('log', function (event, data) {
			$scope.addLogLine(data);
			$scope.$digest();
		});

		$scope.$on('$destroy', function () {
			$(window).off("resize");
			livesocket.unsubscribeFrom("log");
		});

	}]);
});