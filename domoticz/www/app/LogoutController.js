define(['app'], function (app) {
	app.controller('LogoutController', [ 'permissions', '$scope', '$location', '$http', '$interval', function(permissions, $scope,$location,$http,$interval) {

		init();

		function init()
		{
			var permissionList = {
					isloggedin: true,
					rights: -1
			};
			$.ajax({
			 url: "json.htm?type=command&param=dologout",
			 async: true, 
			 dataType: 'json',
			 success: function(data) {
				$.ajax({
				 url: "json.htm?type=command&param=getauth",
				 async: false, 
				 dataType: 'json',
				 success: function(data) {
					if (data.status == "OK") {
						if (data.user!="") {
							permissionList.isloggedin=true;
						}
						permissionList.rights=parseInt(data.rights);
					}
				 }
				});
				permissions.setPermissions(permissionList);
				window.location = '#/Dashboard';
			 },
			 error: function(){
				permissions.setPermissions(permissionList);
				window.location = '#/Dashboard';
			 }     
			});
		};
	} ]);
});