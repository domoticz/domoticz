define(['app'], function (app) {
	'use strict';

	/*
	 * Icon font libraries the user installed (Material Design Icons and
	 * friends). The server downloaded them and serves them from www/assets/;
	 * all the browser has to do is pull the stylesheets in.
	 *
	 * This happens once for the whole application rather than from the page
	 * that needs the icons: any page can show a device whose icon comes from a
	 * library, so the stylesheets have to be there before the first render.
	 */

	var addedLinks = [];
	var bLoaded = false;

	function addStylesheet(library) {
		if (!library || !library.Path) {
			return;
		}
		var link = document.createElement('link');
		link.rel = 'stylesheet';
		link.href = library.Path;
		link.setAttribute('data-icon-library', library.Prefix || '');
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
			url: 'json.htm?type=command&param=geticonlibraries',
			dataType: 'json'
		}).then(function (data) {
			if (!data || data.status !== 'OK') {
				return;
			}
			bLoaded = true;
			removeStylesheets();
			(data.result || []).forEach(addStylesheet);
		}, function () {
			// Refused (nobody logged in yet) or unreachable. Nothing to report
			// to the user: without a library there is simply nothing to load.
		});
	}

	// Lets the Custom Icons page apply an install or a removal straight away,
	// instead of leaving the user with a stale set of stylesheets until the
	// next page load.
	app.factory('iconLibraries', function () {
		return {
			load: load
		};
	});

	/* Fired here rather than from a run block so the request goes out as early
	   as possible, before the first view is rendered. */
	load();

	app.run(['$rootScope', function ($rootScope) {
		$rootScope.$on('$routeChangeSuccess', function () {
			// The first attempt can land on the login screen, where the
			// request is refused. Retry until one succeeds, so a user who logs
			// in later in the same page load still gets the icons.
			if (!bLoaded) {
				load();
			}
		});
	}]);
});
