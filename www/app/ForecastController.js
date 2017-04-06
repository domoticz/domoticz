define(['app'], function (app) {
	app.controller('ForecastController', [ '$scope', '$rootScope', '$location', '$http', '$interval', function($scope,$rootScope,$location,$http,$interval) {
		init();

		function init()
		{
			$scope.MakeGlobalConfig();
			var htmlcontent = '';
			htmlcontent+='<iframe class="cIFrameLarge" id="IMain" src="//darksky.net/#/f/' + $scope.config.Latitude + ',' + $scope.config.Longitude + '"></iframe>';
			$('#maincontent').html(htmlcontent);
			$('#maincontent').i18n();
		};
	} ]);
});