define(['app'], function (app) {
	app.controller('LogController', [ '$scope', '$rootScope', '$location', '$http', '$interval', '$sce', function($scope,$rootScope,$location,$http,$interval,$sce) {
	
		$scope.LastLogTime=0;
		$scope.logitems = [];
		$scope.logitems_status = [];
		$scope.logitems_error = [];
		
		$scope.to_trusted = function(html_code) {
			return $sce.trustAsHtml(html_code);
		}

		$scope.RefreshLog = function()
		{
			if (typeof $scope.mytimer != 'undefined') {
				$interval.cancel($scope.mytimer);
				$scope.mytimer = undefined;
			}

			var bWasAtBottom=true;
			var div = $("#logcontent #logdata");
			if (div!=null) {
				if (div[0]!=null) {
					var diff=Math.abs((div[0].scrollHeight- div.scrollTop())-div.height());
					if (diff>9) {
						bWasAtBottom=false;
					}
				}
			}
			var lastscrolltop=$("#logcontent #logdata").scrollTop();
			var llogtime = $scope.LastLogTime;
			$http({
				url: "json.htm?type=command&param=getlog&lastlogtime=" + $scope.LastLogTime + "&loglevel=0",
				async: false, 
				dataType: 'json'
			}).success(function(data) {
				if (typeof data.result != 'undefined') {
					if (typeof data.LastLogTime != 'undefined') {
						$scope.LastLogTime=parseInt(data.LastLogTime);
					}
					$.each(data.result, function(i,item){
						var message=item.message.replace(/\n/gi,"<br>");
						var logclass="";
						if (item.level==0) {
							logclass="lognorm";
						}
						else if (item.level==1) {
							logclass="logerror";
						}
						else {
							logclass="logstatus";
						}
						$scope.logitems = $scope.logitems.concat({
							mclass: logclass, 
							text: message
						});
						if (llogtime!=0) {
							if (item.level==1) {
								//Error
								$scope.logitems_error = $scope.logitems_error.concat({
									mclass: logclass, 
									text: message
								});
							}
							else if (item.level==2) {
								//Status
								$scope.logitems_status = $scope.logitems_status.concat({
									mclass: logclass, 
									text: message
								});
							}
						}
					});
				}
				$scope.mytimer=$interval(function() {
					$scope.RefreshLog();
				}, 5000);
			}).error(function() {
				$scope.mytimer=$interval(function() {
					$scope.RefreshLog();
				}, 5000);
			});
			if (llogtime==0) {
				//Error
				$http({
					url: "json.htm?type=command&param=getlog&lastlogtime=" + $scope.LastLogTime + "&loglevel=1",
					async: false, 
					dataType: 'json'
				}).success(function(data) {
					if (typeof data.result != 'undefined') {
						$.each(data.result, function(i,item){
							var message=item.message.replace(/\n/gi,"<br>");
							var logclass="";
							if (item.level==0) {
								logclass="lognorm";
							}
							else if (item.level==1) {
								logclass="logerror";
							}
							else {
								logclass="logstatus";
							}
							$scope.logitems_error = $scope.logitems_error.concat({
								mclass: logclass, 
								text: message
							});
						});
					}
				});
				//Status
				$http({
					url: "json.htm?type=command&param=getlog&lastlogtime=" + $scope.LastLogTime + "&loglevel=2",
					async: false, 
					dataType: 'json'
				}).success(function(data) {
					if (typeof data.result != 'undefined') {
						$.each(data.result, function(i,item){
							var message=item.message.replace(/\n/gi,"<br>");
							var logclass="";
							if (item.level==0) {
								logclass="lognorm";
							}
							else if (item.level==1) {
								logclass="logerror";
							}
							else {
								logclass="logstatus";
							}
							$scope.logitems_status = $scope.logitems_status.concat({
								mclass: logclass, 
								text: message
							});
						});
					}
				});
			}
		}
		
		$scope.ClearLog = function ()
		{
			if (typeof $scope.mytimer != 'undefined') {
				$interval.cancel($scope.mytimer);
				$scope.mytimer = undefined;
			}
			$http({
				url: "json.htm?type=command&param=clearlog",
				async: false, 
				dataType: 'json'
			}).success(function(data) {
				$scope.logitems = [];
				$scope.logitems_error = [];
				$scope.logitems_status = [];
				$scope.mytimer=$interval(function() {
					$scope.RefreshLog();
				}, 5000);
			}).error(function() {
				$scope.mytimer=$interval(function() {
					$scope.RefreshLog();
				}, 5000);
			});
		}

		$scope.ResizeLogWindow = function ()
		{
			var pheight=$(window).innerHeight();
			$("#logcontent #logdata").height(pheight-160);
			$("#logcontent #logdata_status").height(pheight-160);
			$("#logcontent #logdata_error").height(pheight-160);
		}

		init();

		function init()
		{
			$("#logcontent").i18n();
			$scope.LastLogTime=0;
			$scope.RefreshLog();
			$(window).resize(function() { $scope.ResizeLogWindow(); });
			$scope.ResizeLogWindow();
		};
		
		$scope.$on('$destroy', function(){
			if (typeof $scope.mytimer != 'undefined') {
				$interval.cancel($scope.mytimer);
				$scope.mytimer = undefined;
			}
			$(window).off("resize");
		}); 
		
	} ]);
});