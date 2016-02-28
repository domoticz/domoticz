define(['app'], function (app) {
	app.controller('CustomController', [ '$scope', '$rootScope', '$location', '$http', '$interval', '$routeParams', function($scope,$rootScope,$location,$http,$interval,$routeParams) {
		init();

		function init()
		{
			var custompage=$routeParams.custompage.replace(/ /g,"%20");
			$("#customcontent").load("templates/" + custompage + ".html", function (response, status, xhr) {
				if (status == "error") {
					var msg = "Sorry but there was an error: ";
					$("#customcontent").html(msg + xhr.status + " " + xhr.statusText);
				}
			});		
		};
	} ]);
});