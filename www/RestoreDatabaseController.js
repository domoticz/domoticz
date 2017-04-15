define(['app'], function (app) {
	app.controller('RestoreDatabaseController', [ '$scope', '$rootScope', '$window', '$location', '$http', '$interval', function($scope,$rootScope,$window,$location,$http,$interval) {
	
		$scope.selected_file="";
	
		init();

		$scope.uploadFile = function() {
			$http({
				method: 'POST',
				url: 'restoredatabase.webem',
				headers: {
					'Content-Type': 'multipart/form-data'
				},
				data: {
					//email: Utils.getUserInfo().email,
					//token: Utils.getUserInfo().token,
					dbasefile: $scope.file
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
				$window.location = '/#Dashboard';
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

		function init()
		{
			$('#maincontent').i18n();
		};
	} ]);
});