define(['app'], function (app) {
	app.controller('UpdateController', [ '$scope', '$rootScope', '$location', '$http', '$interval', '$window', function($scope,$rootScope,$location,$http,$interval,$window) {

		$scope.topText = "";
		$scope.bottomText = "";

		$scope.ProgressData = {
          label: 0,
          percentage: 0
        }
		
		$scope.progressdownload = function() {
			if (typeof $scope.mytimer != 'undefined') {
				$interval.cancel($scope.mytimer);
				$scope.mytimer = undefined;
			}
			var val = $scope.ProgressData.label;
			$scope.ProgressData.label = val + 1;
			if ( val < 99 ) {
				if ($.StopProgress==false) {
					$scope.mytimer=$interval(function() {
						$scope.progressdownload();
					}, 600);
				}
			}
			else {
				$("#updatecontent #divprogress").hide();
				$scope.topText = $.t("Error while downloading Update,<br>check your internet connection or try again later !...");
			}
		}

		$scope.progressupdatesystem = function() {
			if (typeof $scope.mytimer != 'undefined') {
				$interval.cancel($scope.mytimer);
				$scope.mytimer = undefined;
			}
			var val = $scope.ProgressData.label;
			$scope.ProgressData.label = val + 1;
			if ( val < 99 ) {
				if ($.StopProgress==false) {
					$scope.mytimer=$interval(function() {
						$scope.progressupdatesystem();
					}, 450);
				}
			}
			else {
				$.ajax({
				 url: "json.htm?type=command&param=getversion",
				 async: false, 
				 dataType: 'json',
				 success: function(data) {
					if (data.status == "OK") {
						$( "#appversion" ).text("V" + data.version);
					}
				 }
				});
				$window.location = '/#Dashboard';
				$window.location.reload();
			}
		}

		$scope.SendUpdateCommand = function() {
			if (typeof $scope.mytimer2 != 'undefined') {
				$interval.cancel($scope.mytimer2);
				$scope.mytimer2 = undefined;
			}
			$.ajax({
				 url: "json.htm?type=command&param=execute_script&scriptname=update_domoticz&direct=true",
				 async: true, 
				 timeout: 20000,
				 dataType: 'json',
				 success: function(data) {
					$scope.topText = $.t("Restarting System (This could take some time...)");
				 },
				 error: function(){
				 }     
			});
		}

		$scope.CheckDownloadReady = function()
		{
			if (typeof $scope.mytimer2 != 'undefined') {
				$interval.cancel($scope.mytimer2);
				$scope.mytimer2 = undefined;
			}
			var val = $scope.ProgressData.label;
			if (val == 100) {
				$("#updatecontent #divprogress").hide();
				$scope.topText = $.t("Error while downloading Update,<br>check your internet connection or try again later !...");
				return;
			}
			$.ajax({
				 url: "json.htm?type=command&param=downloadready",
				 async: false, 
				 dataType: 'json',
				 success: function(data) {
					if (data.status == "OK") {
						if (data.downloadok==true) {
								if (typeof $scope.mytimer != 'undefined') {
									$interval.cancel($scope.mytimer);
									$scope.mytimer = undefined;
								}
								$scope.ProgressData.label = 0;
								$scope.topText = $.t("Updating system...");
								$scope.mytimer=$interval(function() {
									$scope.progressupdatesystem();
								}, 200);
								$scope.mytimer2=$interval(function() {
									$scope.SendUpdateCommand();
								}, 500);
						}
						else {
							$.StopProgress=true;
							$("#updatecontent #divprogress").hide();
							$scope.topText = $.t("Error while downloading Update,<br>check your internet connection or try again later !...");
						}
					}
					else {
						$scope.mytimer2=$interval(function() {
							$scope.CheckDownloadReady();
						}, 1000);
					}
				 },
				 error: function(){
					$.StopProgress=true;
					$("#updatecontent #divprogress").hide();
					$scope.topText = $.t("Error while downloading Update,<br>check your internet connection or try again later !...");
				 }     
			});
		}

		$scope.CheckForUpdate = function()
		{
			if (typeof $scope.mytimer != 'undefined') {
				$interval.cancel($scope.mytimer);
				$scope.mytimer = undefined;
			}
			$.ajax({
				 url: "json.htm?type=command&param=checkforupdate&forced=true",
				 async: false, 
				 dataType: 'json',
				 success: function(data) {
					if (data.HaveUpdate == true) {
						$scope.topText = $.t("Update Available... Downloading Update !...");
						$.ajax({
							 url: "json.htm?type=command&param=downloadupdate",
							 async: false, 
							 dataType: 'json',
							 success: function(data) {
								if (data.status == "OK") {
									$("#updatecontent #divprogress").show();
									$scope.ProgressData.label = 0;
									$scope.mytimer=$interval(function() {
										$scope.progressdownload();
									}, 1000);
									$scope.mytimer2=$interval(function() {
										$scope.CheckDownloadReady();
									}, 2000);
								}
								else {
									$scope.topText = $.t("Could not start download,<br>check your internet connection or try again later !...");
								}
							 },
							 error: function(){
								$scope.topText = $.t("Error communicating with Server !...");
							 }
						});
					}
					else {
						$scope.topText = $.t("No Update Available !...");
					}
				 },
				 error: function(){
					$scope.topText = $.t("Error communicating with Server !...");
				 }     
			});
		}

		init();

		function init()
		{
			// Here I synchronize the value of label and percentage in order to have a nice demo
			$scope.$watch('ProgressData', function (newValue, oldValue) {
				newValue.percentage = newValue.label / 100;
			}, true);
        		
			$.StopProgress=false;
			$scope.topText = $.t("Checking for updates....");
			$scope.bottomText = $.t("Do not poweroff the system while updating !...");
			
			$scope.mytimer=$interval(function() {
				$scope.CheckForUpdate();
			}, 1000);
		};
		$scope.$on('$destroy', function(){
			if (typeof $scope.mytimer != 'undefined') {
				$interval.cancel($scope.mytimer);
				$scope.mytimer = undefined;
			}
			if (typeof $scope.mytimer2 != 'undefined') {
				$interval.cancel($scope.mytimer2);
				$scope.mytimer2 = undefined;
			}
		}); 
	} ]);
});