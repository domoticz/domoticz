define(['app'], function (app) {
	'use strict';

	/* Pulls in the stylesheets among the stored web assets (icon fonts such as
	   Material Design Icons). Done once for the whole application: any page can
	   show a device whose icon comes from one, so they have to be there before
	   the first render. */

	var addedLinks = [];
	var bLoaded = false;

	function addStylesheet(asset) {
		if (!asset || !asset.name || !/\.css$/i.test(asset.name)) {
			return;
		}
		var link = document.createElement('link');
		link.rel = 'stylesheet';
		link.href = asset.path || ('assets/' + asset.name);
		link.setAttribute('data-web-asset', asset.name);
		document.head.appendChild(link);
		addedLinks.push(link);
	}

	function removeStylesheets() {
		addedLinks.forEach(function (link) {
			if (link.parentNode) {
				link.parentNode.removeChild(link);
			}
		});
		addedLinks = [];
	}

	function load() {
		return $.ajax({
			url: 'json.htm?type=command&param=getwebassets',
			dataType: 'json'
		}).then(function (data) {
			if (!data || data.status !== 'OK') {
				return;
			}
			bLoaded = true;
			removeStylesheets();
			(data.result || []).forEach(addStylesheet);
		}, function () {
			// Refused (nobody logged in yet) or unreachable; nothing to report.
		});
	}

	// Lets the Custom Icons page apply an install or a removal straight away.
	app.factory('iconLibraries', function () {
		return {
			load: load
		};
	});

	// Fired here rather than from a run block, so it goes out before the first render.
	load();

	app.run(['$rootScope', function ($rootScope) {
		$rootScope.$on('$routeChangeSuccess', function () {
			// The first attempt can land on the login screen and be refused, so
			// retry until one succeeds.
			if (!bLoaded) {
				load();
			}
		});
	}]);
});
