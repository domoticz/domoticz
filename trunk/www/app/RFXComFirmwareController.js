define(['app'], function (app) {
	app.controller('RFXComFirmwareController', [ '$scope', '$rootScope', '$window', '$location', '$http', '$interval', function($scope,$rootScope,$window,$location,$http,$interval) {
	
		$scope.selected_file="";
		$scope.isUpdating=false;
		$scope.topText = "Updating Firmware...";
		$scope.bottomText = "";
	
		$scope.SetPercentage = function (percentage)
		{
			var perc=parseFloat(parseFloat(percentage).toFixed(2));
			$( "#progressbar" ).progressbar( "value", perc );
		}

		$scope.progressupload = function() {
			if (typeof $scope.mytimer != 'undefined') {
				$interval.cancel($scope.mytimer);
				$scope.mytimer = undefined;
			}
			$http({
			 url: "json.htm?type=command&param=rfxfirmwaregetpercentage&hardwareid="+$rootScope.hwidx
			 }).success(function(data) {
				if (data.status=="ERR") {
					HideNotify();
					bootbox.alert($.t('Problem updating firmware')+"<br>"+$.t(data.message));
					SwitchLayout('Dashboard');
					return;
				}
				var percentage=data.percentage;
				if (percentage==-1) {
					HideNotify();
					bootbox.alert($.t('Problem updating firmware')+"<br>"+$.t(data.message));
					SwitchLayout('Dashboard');
				}
				else if (percentage==100) {
					SwitchLayout('Dashboard');
				}
				else {
					$scope.SetPercentage(data.percentage);
					$scope.bottomText=data.message;
					$scope.mytimer=$interval(function() {
						$scope.progressupload();
					}, 1000);
				}
			 }).error(function() {
					HideNotify();
					bootbox.alert($.t('Problem updating firmware')+"<br>"+$.t(data.message));
					SwitchLayout('Dashboard');
			 });
		}
		$scope.uploadFile = function() {
			$http({
				method: 'POST',
				url: 'rfxupgradefirmware.webem',
				headers: {
					'Content-Type': 'multipart/form-data'
				},
				data: {
					hardwareid: $rootScope.hwidx,
					firmwarefile: $scope.file
				},
				transformRequest: function (data, headersGetter) {
					var formData = new FormData();
					angular.forEach(data, function (value, key) {
						formData.append(key, value);
					});

					var headers = headersGetter();
					delete headers['Content-Type'];
					return formData;
				}
			})
			.success(function (data) {
				$scope.isUpdating=true;
				$scope.mytimer=$interval(function() {
					$scope.progressupload();
				}, 1000);
			})
			.error(function (data, status) {
				$window.location = '/#Dashboard';
			});
   		};
		
		$scope.onSubmit = function() {
			if (typeof $scope.file == 'undefined') {
				return;
			}
			$scope.uploadFile();
		}

		$scope.init = function()
		{
			$scope.bottomText="";
			$scope.isUpdating=false;
			$('#maincontent').i18n();
			
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
			$scope.SetPercentage(0);
		};
		
		$scope.init();
	
		$scope.$on('$destroy', function(){
			if (typeof $scope.mytimer != 'undefined') {
				$interval.cancel($scope.mytimer);
				$scope.mytimer = undefined;
			}
		}); 
		
	} ]);
});