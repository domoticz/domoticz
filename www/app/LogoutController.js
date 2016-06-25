define(['app'], function (app) {
	app.controller('LogoutController', [ 'permissions', '$scope', '$rootScope', '$location', '$http', '$interval', function (permissions, $scope, $rootScope, $location, $http, $interval) {

		(function init() {
			var permissionList = {
				isloggedin: true,
				rights: -1
			};
			permissions.setPermissions(permissionList);
			$.ajax({
				url: "json.htm?type=command&param=dologout",
				async: true, 
				dataType: 'json',
				success: function (data) {
					$.ajax({
						url: "json.htm?type=command&param=getauth",
						async: false, 
						dataType: 'json',
						success: function (data) {
							if (data.status === "OK") {
								if (data.user !== "") {
									permissionList.isloggedin = true;
								}
								permissionList.rights = parseInt(data.rights, 10);
							}
						}
					});
					permissions.setPermissions(permissionList);
					window.location = '#/Dashboard';
				},
				error: function (xhr, status, error) {
					var authenticate = xhr.getResponseHeader("WWW-Authenticate");
					if (authenticate && (authenticate.indexOf("Basic") > -1)) {
						// When Basic authentication is used, user should close window after logout
						$('#logout').html('<div style="text-align: center;"><span>Please close this browser tab or browser window before you log in again.</span></div>')
						return;
					}
					window.location = '#/Dashboard';
				}
			});
		})();

	}]);
});