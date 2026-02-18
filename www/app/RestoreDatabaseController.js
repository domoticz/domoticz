define(['app'], function (app) {
	app.controller('RestoreDatabaseController', ['$scope', '$rootScope', '$window', '$location', '$http', '$interval', function ($scope, $rootScope, $window, $location, $http, $interval) {

		$scope.selected_file = "";
		$scope.uploading = false;
		$scope.restoring = false;
		$scope.uploadProgress = 0;
		$scope.errorMessage = "";

		var restoreHandled = false;
		var pollTimer = null;

		init();

		$scope.uploadFile = function () {
			$scope.uploading = true;
			$scope.restoring = false;
			$scope.uploadProgress = 0;
			$scope.errorMessage = "";
			restoreHandled = false;

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
							if ($scope.uploadProgress >= 100 && !$scope.restoring) {
								$scope.restoring = true;
								// Upload complete - server is now restoring the database.
								// The POST response is delayed while hardware reinitializes.
								// Poll server readiness instead of waiting for the response.
								startServerPoll();
							}
						}
					}
				},
				timeout: 600000
			}).then(function successCallback(response) {
				handleRestoreComplete();
			}, function errorCallback(response) {
				if (restoreHandled) return;
				// If polling and connection was lost, let poll handle recovery
				if (pollTimer && response.status === 0) return;
				cancelPoll();
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

		function handleRestoreComplete() {
			if (restoreHandled) return;
			restoreHandled = true;
			cancelPoll();
			$scope.uploading = false;
			$scope.restoring = false;
			bootbox.alert($.t('Database restored successfully. The system will now reload.'), function () {
				$window.location = '/#Dashboard';
			});
		}

		function cancelPoll() {
			if (pollTimer) {
				$interval.cancel(pollTimer);
				pollTimer = null;
			}
		}

		function startServerPoll() {
			var checkCount = 0;
			pollTimer = $interval(function () {
				checkCount++;
				// Skip first tick to give the server time to start restoring
				if (checkCount < 2) return;
				// Timeout after 2 minutes
				if (checkCount > 60) {
					cancelPoll();
					$scope.uploading = false;
					$scope.restoring = false;
					$scope.errorMessage = $.t('Server did not respond in time. Please check the system and reload the page.');
					return;
				}
				$http.get('json.htm?type=command&param=getversion', { timeout: 3000 })
					.then(function (resp) {
						if (resp.data && resp.data.status === 'OK') {
							handleRestoreComplete();
						}
					}, function () {
						// Server not ready yet, keep polling
					});
			}, 2000);
		}

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

		$scope.$on('$destroy', function () {
			cancelPoll();
		});

		function init() {
			$('#maincontent').i18n();
		};
	}]);
});
