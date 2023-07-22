define(['app'], function (app) {
	app.controller('MyProfileController', ['$scope', '$rootScope', '$location', '$http', '$interval', 'domoticzApi', 'md5', function ($scope, $rootScope, $location, $http, $interval, domoticzApi, md5) {

		$scope.newMFAEnabled = false;
		$scope.wasMFAEnabled = false;

		$scope.myprofile = {
			enableMFA: false,
			totpsecret: '',
			oldpwd: '',
			newpwd: '',
			vfypwd: '',
			qruri: 'otpauth://totp/domoticz?algorithm=SHA1&digits=6&secret='
		};

		$scope.generateTOTPSecret = function()
		{
			var chars = 'ABCDEFGHIJKLMNOPQRSTUVWXYZ234567';
			var result = '';
			for (var i = 32; i > 0; --i) result += chars[Math.floor(Math.random() * chars.length)];
			return result;
		}		

		$scope.updateQR = function (qrdata) {
			var typeNumber = 6;		// Large enough to contain the otpauth URI data
			var errorCorrectionLevel = 'M';	// Higher error correction (then L)
			var qr = qrcode(typeNumber, errorCorrectionLevel);
			qr.addData(qrdata);
			qr.make();
			$('#qrcode').html(qr.createImgTag(4));
		}

		$scope.init = function () {
			$('#profiletable').hide();
			$('#passwdtable').hide();
			$scope.MakeGlobalConfig();
			domoticzApi.sendCommand('getmyprofile', {
				'username': $scope.config.userName
				}).then(function (data) {
					if (typeof data.mfasecret != 'undefined' && data.mfasecret != '') {
						$scope.myprofile.enableMFA = true;
						$scope.wasMFAEnabled = true;
						$scope.myprofile.totpsecret = data.mfasecret;
					}
					$('#profiletable').show();
					$scope.updateQR($scope.myprofile.qruri + $scope.myprofile.totpsecret);
					$('#passwdtable').show();
				})
				.catch(function () {
					ShowNotify($.t('Problem retrieving Profile!'), 2500, true);
				});
		}

		$scope.changeTOTP = function () {
			if ($scope.myprofile.enableMFA == true) {
				$scope.updateQR($scope.myprofile.qruri + $scope.myprofile.totpsecret);
			}
		}

		$scope.btnCancel = function () {
			$location.path('/Dashboard');
		}

		$scope.updateMyProfile = function () {
			var fd = new FormData();
			fd.append('username', $scope.config.userName);

			if ($scope.myprofile.oldpwd != '' && $scope.myprofile.newpwd != '') {
				if ($scope.myprofile.newpwd.length < 8 || $scope.myprofile.newpwd != $scope.myprofile.vfypwd) {
					ShowNotify($.t('Passwords do not match (or to short)!'), 2500, true);
					return;
				}
				var sOldPwd = encodeURIComponent(md5.createHash($scope.myprofile.oldpwd));
				var sNewPwd = encodeURIComponent(md5.createHash($scope.myprofile.newpwd));
				fd.append('oldpwd', sOldPwd);
				fd.append('newpwd', sNewPwd);
			}


			fd.append('enablemfa', $scope.myprofile.enableMFA);
			if ($scope.myprofile.enableMFA == true) {
				if ($scope.myprofile.totpsecret.length != 32) {
					ShowNotify($.t('Please provide a valid (base32 encoded) 20 character secret!'), 2500, true);
					return;
				}
				fd.append('totpsecret', $scope.myprofile.totpsecret);
				if ($scope.newMFAEnabled == true) {
					if ($scope.myprofile.totpcode.length != 6) {
						ShowNotify($.t('Please provide a valid 6 digit code!'), 2500, true);
						return;
					}
					fd.append('totpcode', $scope.myprofile.totpcode);
				}
			}

			$http.post('json.htm?type=command&param=updatemyprofile', fd, {
				transformRequest: angular.identity,
				headers: { 'Content-Type': undefined }
			}).then(function successCallback(response) {
			    var data = response.data;
			    if (data.status != "OK") {
					ShowNotify($.t(data.error), 2500, true);
					return;
			    }
				$location.path('/Dashboard');
				return;
			}, function errorCallback(response) {
				ShowNotify($.t('Problem updating Profile!'), 2500, true);
				return;
			});
		}
		
		$scope.onMFAChange = function()
		{
			if ($scope.myprofile.enableMFA == true) {
				$scope.myprofile.totpsecret = $scope.generateTOTPSecret();
				$scope.updateQR($scope.myprofile.qruri + $scope.myprofile.totpsecret);
				$scope.newMFAEnabled = true;
				return;
			}
			if ($scope.wasMFAEnabled) {
				bootbox.confirm($.t("Are you sure to disable 2FA? When enabling again it will you will need to re-authenticate again!!"), function (confirmed) {
					if (confirmed == false) {
						$scope.myprofile.enableMFA = true;
						$scope.$apply();
					} else {
						$scope.wasMFAEnabled = false;
					}
				});
			}
		}

		$scope.$on('$destroy', function () {
		});

		$scope.init();
	}]);
});