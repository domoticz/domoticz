define(['app'], function (app) {
	app.controller('LoginController', ['permissions', '$scope', '$rootScope', '$location', '$http', '$interval', '$window', 'md5', function (permissions, $scope, $rootScope, $location, $http, $interval, $window, md5) {

		$scope.failcounter = 0;

		$scope.DoLogin = function () {
			var musername = encodeURIComponent(btoa($('#username').val()));
			var mpassword = encodeURIComponent(md5.createHash($('#password').val()));
			var bRememberMe = $('#rememberme').is(":checked");

			var fd = new FormData();
			fd.append('username', musername);
			fd.append('password', mpassword);
			fd.append('rememberme', bRememberMe);
			$http.post('logincheck', fd, {
				transformRequest: angular.identity,
				headers: { 'Content-Type': undefined }
			}).then(function successCallback(response) {
			    var data = response.data;
			    if (data.status != "OK") {
			        HideNotify();
					$scope.failcounter += 1;
					if ($scope.failcounter > 3) {
						window.location.href = "http://www.1112.net/lastpage.html";
						return;
					}
					else {
						ShowNotify($.t('Incorrect Username/Password!'), 2500, true);
					}
					return;
			    }
				var permissionList = {
					isloggedin: true,
					rights: 0
				};
				if (data.user != "") {
					permissionList.isloggedin = true;
				}
				permissionList.rights = parseInt(data.rights);
				permissions.setPermissions(permissionList);

				$rootScope.GetGlobalConfig();

				$location.path('/Dashboard');
			}, function errorCallback(response) {
			    HideNotify();
				$scope.failcounter += 1;
				if ($scope.failcounter > 3) {
					window.location.href = "http://www.1112.net/lastpage.html";
					return;
				}
				else {
					ShowNotify($.t('Incorrect Username/Password!'), 2500, true);
				}
				return;
			});
		}

		init();

		function init() {
			$.ajax({
				url: "json.htm?type=command&param=getlanguage",
				async: false,
				dataType: 'json',
				success: function (data) {
					if (typeof data.language != 'undefined') {
						SetLanguage(data.language);
					}
					else {
						SetLanguage('en');
					}
				},
				error: function () {
				}
			});

			var $inputs = $('#login :input');
			$inputs.each(function () {
				if ($(this).attr("id") != "submit") {
					$(this).attr("placeholder", $.t($(this).attr("placeholder")));
				}
				else {
					$(this).attr("value", $.t("Login"));
				}
			});
			$("#remembermelbl").text($.t("Remember me"));
		};
	}]);
});