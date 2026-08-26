define(['app'], function (app) {
	'use strict';

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
		});
	}

	app.factory('iconLibraries', function () {
		return {
			load: load
		};
	});

	load();

	app.run(['$rootScope', function ($rootScope) {
		$rootScope.$on('$routeChangeSuccess', function () {
			if (!bLoaded) {
				load();
			}
		});
	}]);
});
