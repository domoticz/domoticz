define(['app'], function (app) {
	app.controller('MyProfileController', ['$scope', '$rootScope', '$location', '$http', '$interval', 'domoticzApi', 'md5', function ($scope, $rootScope, $location, $http, $interval, domoticzApi, md5) {

		$scope.myprofile = {
			enableMFA: false,
			totpsecret: '',
			oldpwd: '',
			newpwd: '',
			vfypwd: ''
		};

		$scope.updateQR = function (qrdata) {
			var typeNumber = 4;
			var errorCorrectionLevel = 'L';
			var qr = qrcode(typeNumber, errorCorrectionLevel);
			qr.addData(qrdata);
			qr.make();
			document.getElementById('qrcode').innerHTML = qr.createImgTag(4);
		}

		$scope.init = function () {
			$('#profiletable').hide();
			$('#passwdtable').hide();
			$scope.MakeGlobalConfig();
			domoticzApi.sendCommand('getauth', {
				}).then(function (data) {
					$scope.myprofile.enableMFA = (data.rights == 2);
					$('#profiletable').show();
					$('#passwdtable').show();
					$scope.updateQR('Hello there!');
				})
				.catch(function () {
					ShowNotify($.t('Problem retrieving Profile!'), 2500, true);
				});
		}

		$scope.btnCancel = function () {
			$location.path('/Dashboard');
		}

		$scope.updateMyProfile = function () {
			if ($scope.myprofile.enableMFA == true && $scope.myprofile.totpsecret.length != 32) {
				ShowNotify($.t('Please provide a valid (base32 encoded) 20 character secret!'), 2500, true);
				return;
			}

			var fd = new FormData();
			fd.append('username', $scope.config.userName);
			fd.append('enablemfa', $scope.myprofile.enableMFA);
			if ($scope.myprofile.enableMFA == true) {
				fd.append('totpsecret', $scope.myprofile.totpsecret);
			}
			$http.post('json.htm?type=command&param=logincheck', fd, {
				transformRequest: angular.identity,
				headers: { 'Content-Type': undefined }
			}).then(function successCallback(response) {
			    var data = response.data;
			    if (data.status != "OK") {
					ShowNotify($.t('Unable to update Profile!'), 2500, true);
					return;
			    }
				$location.path('/Dashboard');
				return;
			}, function errorCallback(response) {
				ShowNotify($.t('Problem updating Profile!'), 2500, true);
				return;
			});
		}

		$scope.updateMyPasswd = function () {
			if ($scope.myprofile.oldpwd != '' && $scope.myprofile.newpwd != '') {
				if ($scope.myprofile.newpwd.length < 8 || $scope.myprofile.newpwd != $scope.myprofile.vfypwd) {
					ShowNotify($.t('Passwords do not match (or to short)!'), 2500, true);
					return;
				}
			}

			var fd = new FormData();
			var sOldPwd = encodeURIComponent(md5.createHash($scope.myprofile.oldpwd));
			var sNewPwd = encodeURIComponent(md5.createHash($scope.myprofile.newpwd));
			fd.append('oldpwd', sOldPwd);
			fd.append('newpwd', sNewPwd);
			$http.post('json.htm?type=command&param=logincheck', fd, {
				transformRequest: angular.identity,
				headers: { 'Content-Type': undefined }
			}).then(function successCallback(response) {
			    var data = response.data;
			    if (data.status != "OK") {
					ShowNotify($.t('Unable to update Password!'), 2500, true);
					return;
			    }
				$location.path('/Dashboard');
				return;
			}, function errorCallback(response) {
				ShowNotify($.t('Problem updating Password!'), 2500, true);
				return;
			});
		}

		$scope.useMFA = function (line) {
			return $scope.myprofile.enableMFA;
		}

		$scope.$on('$destroy', function () {
		});

		$scope.init();
	}]);
});
