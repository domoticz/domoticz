define(['app'], function (app) {
	app.controller('LogController', [ '$scope', '$rootScope', '$location', '$http', '$interval', function($scope,$rootScope,$location,$http,$interval) {
	
		$scope.LastLogTime=0;
		$scope.logitems = [];
		
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

			$.ajax({
			 url: "json.htm?type=command&param=getlog&lastlogtime=" + $scope.LastLogTime,
			 async: false, 
			 dataType: 'json',
			 success: function(data) {
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
				});
			  }
			 }
			});
			$scope.mytimer=$interval(function() {
				$scope.RefreshLog();
			}, 5000);
		}

		$scope.ResizeLogWindow = function ()
		{
			var pheight=$(window).innerHeight();
			$("#logcontent #logdata").height(pheight-130);
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