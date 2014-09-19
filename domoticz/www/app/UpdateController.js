define(['app'], function (app) {
	app.controller('UpdateController', [ '$scope', '$location', '$http', '$interval', '$window', function($scope,$location,$http,$interval,$window) {

		$scope.topText = "";
		$scope.bottomText = "";
		
		$scope.progressdownload = function() {
			if (typeof $scope.mytimer != 'undefined') {
				$interval.cancel($scope.mytimer);
				$scope.mytimer = undefined;
			}
			var val = $( "#progressbar" ).progressbar( "value" ) || 0;
			$( "#progressbar" ).progressbar( "value", val + 1 );
			if ( val < 99 ) {
				if ($.StopProgress==false) {
					$scope.mytimer=$interval(function() {
						$scope.progressdownload();
					}, 600);
				}
			}
			else {
				$("#updatecontent #divprogress").hide();
				$scope.topText = $.i18n("Error while downloading Update,<br>check your internet connection or try again later !...").replace("<br>","\n");
			}
		}

		$scope.progressupdatesystem = function() {
			if (typeof $scope.mytimer != 'undefined') {
				$interval.cancel($scope.mytimer);
				$scope.mytimer = undefined;
			}
			var val = $( "#progressbar" ).progressbar( "value" ) || 0;
			$( "#progressbar" ).progressbar( "value", val + 1 );
			if ( val < 99 ) {
				if ($.StopProgress==false) {
					$scope.mytimer=$interval(function() {
						$scope.progressupdatesystem();
					}, 450);
				}
			}
			else {
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
					var val = $( "#progressbar" ).progressbar( "value" ) || 0;
					$( "#progressbar" ).progressbar( "value", 90 );
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
			var val = $( "#progressbar" ).progressbar( "value" ) || 0;
			if (val == 100) {
				$("#updatecontent #divprogress").hide();
				$scope.topText = $.i18n("Error while downloading Update,<br>check your internet connection or try again later !...").replace("<br>","\n");
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
								$( "#progressbar" ).progressbar( "value",0);
								$scope.topText = $.i18n("Updating system...");
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
							$scope.topText = $.i18n("Error while downloading Update,<br>check your internet connection or try again later !...").replace("<br>","\n");
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
					$scope.topText = $.i18n("Error while downloading Update,<br>check your internet connection or try again later !...").replace("<br>","\n");
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
						$scope.topText = $.i18n("Update Available... Downloading Update !...");
						$.ajax({
							 url: "json.htm?type=command&param=downloadupdate",
							 async: false, 
							 dataType: 'json',
							 success: function(data) {
								if (data.status == "OK") {
									$("#updatecontent #divprogress").show();
									$( "#progressbar" ).progressbar( "value",0);
									$scope.mytimer=$interval(function() {
										$scope.progressdownload();
									}, 1000);
									$scope.mytimer2=$interval(function() {
										$scope.CheckDownloadReady();
									}, 2000);
								}
								else {
									$scope.topText = $.i18n("Could not start download,<br>check your internet connection or try again later !...").replace("<br>","\n");
								}
							 },
							 error: function(){
								$scope.topText = $.i18n("Error communicating with Server !...");
							 }
						});
					}
					else {
						$scope.topText = $.i18n("No Update Available !...");
					}
				 },
				 error: function(){
					$scope.topText = $.i18n("Error communicating with Server !...");
				 }     
			});
		}

		init();

		function init()
		{
			$.StopProgress=false;
			$scope.topText = $.i18n("Checking for updates....");
			$scope.bottomText = $.i18n("Do not poweroff the system while updating !...");
			var progressbar = $( "#progressbar" );
			progressLabel = $( ".progress-label" );
			progressbar.height(22);
			progressbar.progressbar({
				value: false,
				change: function() {
					progressLabel.text( progressbar.progressbar( "value" ) + "%" );
				},
				complete: function() {
					//progressLabel.text( "Complete!" );
				}
			});
			progressbarValue = progressbar.find( ".ui-progressbar-value" );
			progressbarValue.css({
				"background": 'url(images/pbar-ani.gif)'
			});
			
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