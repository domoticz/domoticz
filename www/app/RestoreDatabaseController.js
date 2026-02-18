define(['app'], function (app) {
	app.controller('RestoreDatabaseController', ['$scope', '$rootScope', '$window', '$location', '$http', '$interval', function ($scope, $rootScope, $window, $location, $http, $interval) {

		$scope.selected_file = "";
		$scope.uploading = false;
		$scope.restoring = false;
		$scope.uploadProgress = 0;
		$scope.errorMessage = "";

		init();

		$scope.uploadFile = function () {
			$scope.uploading = true;
			$scope.restoring = false;
			$scope.uploadProgress = 0;
			$scope.errorMessage = "";

			var formData = new FormData();
			formData.append('dbasefile', $scope.file);

			$http({
				method: 'POST',
				url: 'restoredatabase.webem',
				data: formData,
				headers: { 'Content-Type': undefined },
				uploadEventHandlers: {
					progress: function (e) {
						if (e.lengthComputable) {
							$scope.uploadProgress = Math.round(100 * e.loaded / e.total);
							if ($scope.uploadProgress >= 100) {
								$scope.restoring = true;
							}
						}
					}
				},
				timeout: 600000
			}).then(function successCallback(response) {
				$scope.uploading = false;
				$scope.restoring = false;
				bootbox.alert($.t('Database restored successfully. The system will now reload.'), function () {
					$window.location = '/#Dashboard';
				});
			}, function errorCallback(response) {
				$scope.uploading = false;
				$scope.restoring = false;
				if (response.status === 413) {
					$scope.errorMessage = $.t('Database file is too large.');
				} else if (response.status === 0) {
					$scope.errorMessage = $.t('Connection lost during restore. Please wait and check the system.');
				} else {
					$scope.errorMessage = $.t('Database restore failed. Please check the Domoticz log for details.');
				}
			});
		};

		$scope.onSubmit = function () {
			if (typeof $scope.file == 'undefined') {
				return;
			}
			bootbox.confirm($.t('Are you sure you want to restore this database? Current data will be overwritten.'), function (result) {
				if (result) {
					$scope.$apply(function () {
						$scope.uploadFile();
					});
				}
			});
		};

		function init() {
			$('#maincontent').i18n();
		};
	}]);
});
