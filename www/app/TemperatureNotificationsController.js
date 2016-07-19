define(['app'], function (app) {
	app.controller('TemperatureNotificationsController',
		[ '$routeParams', '$scope', '$location',
			function($routeParams, $scope, $location) {
				var ctrl = this;
				ctrl.idx = $routeParams.idx;

				// TODO use $resource
				$.ajax({
					url: "json.htm?type=devices&rid=" + ctrl.idx,
					async: false,
					dataType: 'json',
					success: function(data) {
						if (typeof data.result != 'undefined' && data.result.length > 0) {
							ctrl.device = data.result[0];
							ShowNotifications(ctrl.idx, escape(ctrl.device.Name), "#tempnotifications", "BackToTemperatures");
						}
					}
				});

				BackToTemperatures = function() {
					$location.url("/Temperature");
					$scope.$apply(); // do this because we are not in Angular when executing this function
				};
	} ]);
});
