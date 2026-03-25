define(['app'], function (app) {
	app.controller('MyProfileController', ['$scope', '$rootScope', '$location', '$http', '$interval', 'domoticzApi', 'md5', function ($scope, $rootScope, $location, $http, $interval, domoticzApi, md5) {

		$scope.newMFAEnabled = false;
		$scope.wasMFAEnabled = false;

		$scope.passkeys = [];
		$scope.newPasskeyName = '';
		$scope.registeringPasskey = false;

		$scope.myprofile = {
			enableMFA: false,
			totpsecret: '',
			totpcode: '',
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
					$('#passwdtable').show();
					$scope.loadPasskeys();
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
				if (!$scope.wasMFAEnabled) {
					// New 2FA setup - generate secret and show QR
					$scope.myprofile.totpsecret = $scope.generateTOTPSecret();
					$scope.updateQR($scope.myprofile.qruri + $scope.myprofile.totpsecret);
					$scope.newMFAEnabled = true;
				}
				// If wasMFAEnabled, just re-check the box - nothing changes
				return;
			}
			// Disabling
			$scope.newMFAEnabled = false;
			if ($scope.wasMFAEnabled) {
				bootbox.confirm($.t("Are you sure to disable Two-Factor Authentication?"), function (confirmed) {
					if (!confirmed) {
						$scope.myprofile.enableMFA = true;
						$scope.$apply();
					}
				});
			}
		}

		function base64urlToBuffer(base64url) {
			var base64 = base64url.replace(/-/g, '+').replace(/_/g, '/');
			var padding = base64.length % 4;
			if (padding) base64 += '='.repeat(4 - padding);
			var binary = atob(base64);
			var bytes = new Uint8Array(binary.length);
			for (var i = 0; i < binary.length; i++) {
				bytes[i] = binary.charCodeAt(i);
			}
			return bytes.buffer;
		}

		function bufferToBase64url(buffer) {
			var bytes = new Uint8Array(buffer);
			var binary = '';
			for (var i = 0; i < bytes.length; i++) {
				binary += String.fromCharCode(bytes[i]);
			}
			return btoa(binary).replace(/\+/g, '-').replace(/\//g, '_').replace(/=+$/, '');
		}

		$scope.loadPasskeys = function () {
			$http.get('json.htm?type=command&param=getmypasskeys').then(function (response) {
				if (response.data.status === 'OK') {
					$scope.passkeys = response.data.result || [];
				}
			});
		};

		$scope.registerPasskey = function () {
			if (!window.PublicKeyCredential) {
				ShowNotify($.t('Your browser does not support passkeys'), 3000, true);
				return;
			}

			$scope.registeringPasskey = true;

			// Step 1: Get registration options from server
			$http.get('json.htm?type=command&param=registerpasskey-begin').then(function (response) {
				var data = response.data;
				if (data.status !== 'OK') {
					ShowNotify($.t('Failed to start passkey registration'), 3000, true);
					$scope.registeringPasskey = false;
					return;
				}

				// Step 2: Build PublicKeyCredentialCreationOptions
				var createOptions = {
					publicKey: {
						rp: { name: data.rp.name, id: data.rp.id },
						user: {
							id: base64urlToBuffer(data.user.id),
							name: data.user.name,
							displayName: data.user.displayName
						},
						challenge: base64urlToBuffer(data.challenge),
						pubKeyCredParams: data.pubKeyCredParams,
						timeout: data.timeout,
						attestation: data.attestation,
						authenticatorSelection: data.authenticatorSelection,
						excludeCredentials: (data.excludeCredentials || []).map(function (cred) {
							return { type: cred.type, id: base64urlToBuffer(cred.id) };
						})
					}
				};

				// Step 3: Call WebAuthn API
				return navigator.credentials.create(createOptions);
			}).then(function (credential) {
				if (!credential) return;

				// Step 4: Send attestation to server
				var fd = new FormData();
				fd.append('credentialId', bufferToBase64url(credential.rawId));
				fd.append('clientDataJSON', bufferToBase64url(credential.response.clientDataJSON));
				fd.append('attestationObject', bufferToBase64url(credential.response.attestationObject));
				fd.append('credentialName', $scope.newPasskeyName || 'Passkey');

				return $http.post('json.htm?type=command&param=registerpasskey-complete', fd, {
					transformRequest: angular.identity,
					headers: { 'Content-Type': undefined }
				});
			}).then(function (response) {
				if (response && response.data.status === 'OK') {
					ShowNotify($.t('Passkey registered successfully!'), 2500);
					$scope.newPasskeyName = '';
					$scope.loadPasskeys();
				} else if (response) {
					ShowNotify($.t('Failed to register passkey'), 3000, true);
				}
				$scope.registeringPasskey = false;
			}).catch(function (err) {
				console.error('Passkey registration error:', err);
				if (err.name === 'NotAllowedError') {
					ShowNotify($.t('Passkey registration was cancelled'), 2500, true);
				} else {
					ShowNotify($.t('Passkey registration failed: ') + err.message, 3000, true);
				}
				$scope.registeringPasskey = false;
				$scope.$apply();
			});
		};

		$scope.deletePasskey = function (credentialId) {
			bootbox.confirm($.t('Are you sure you want to delete this passkey?'), function (result) {
				if (result) {
					var fd = new FormData();
					fd.append('credentialId', credentialId);
					$http.post('json.htm?type=command&param=deletepasskey', fd, {
						transformRequest: angular.identity,
						headers: { 'Content-Type': undefined }
					}).then(function (response) {
						if (response.data.status === 'OK') {
							ShowNotify($.t('Passkey deleted'), 2500);
							$scope.loadPasskeys();
						}
					});
				}
			});
		};

		$scope.$on('$destroy', function () {
		});

		$scope.init();
	}]);
});
