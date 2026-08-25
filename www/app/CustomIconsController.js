// 'iconLibraries' is listed explicitly so the service this controller injects
// is guaranteed to be registered, whatever order the modules happen to load in.
define(['app', 'iconLibraries'], function (app) {
	app.controller('CustomIconsController', ['$scope', '$rootScope', '$location', '$http', '$interval', 'iconLibraries', function ($scope, $rootScope, $location, $http, $interval, iconLibraries) {

		$scope.iconset = [];
		$scope.selectedIcon = [];
		$scope.iconlibraries = [];
		$scope.newLibrary = { name: '', prefix: '', url: '' };

		/* Prefills for the libraries we know work. The version is pinned to a
		   major so the CDN keeps serving a matching stylesheet + font pair.
		   Note that the Weather Icons package on npm is "weathericons"; the
		   more obvious "weather-icons" does not exist. */
		$scope.librarySuggestions = [
			{ name: 'Material Design Icons', prefix: 'mdi', url: 'https://cdn.jsdelivr.net/npm/@mdi/font@7/css/materialdesignicons.min.css' },
			{ name: 'Bootstrap Icons', prefix: 'bi', url: 'https://cdn.jsdelivr.net/npm/bootstrap-icons@1/font/bootstrap-icons.min.css' },
			{ name: 'Phosphor Icons', prefix: 'ph', url: 'https://cdn.jsdelivr.net/npm/@phosphor-icons/web@2/src/regular/style.css' },
			{ name: 'Remix Icon', prefix: 'ri', url: 'https://cdn.jsdelivr.net/npm/remixicon@4/fonts/remixicon.css' },
			{ name: 'Tabler Icons', prefix: 'ti', url: 'https://cdn.jsdelivr.net/npm/@tabler/icons-webfont@2/tabler-icons.min.css' },
			{ name: 'Weather Icons', prefix: 'wi', url: 'https://cdn.jsdelivr.net/npm/weathericons@2.0.10/css/weather-icons.min.css' }
		];

		$scope.uploadIcon = function (file) {
			var fd = new FormData();
			fd.append('file', file);
			$http.post('/json.htm?type=command&param=uploadcustomicon', fd, {
				transformRequest: angular.identity,
				headers: { 'Content-Type': undefined }
			}).then(function successCallback(response) {
			    var data = response.data;
			    if (data.status != "OK") {
			        HideNotify();
			        ShowNotify($.t('Error uploading Iconset') + ": " + data.error, 5000, true);
			    }
			    $scope.RefreshIconList();
			}, function errorCallback(response) {
			    HideNotify();
			    ShowNotify($.t('Error uploading Iconset'), 5000, true);
			});
		}

		$scope.UploadIconSet = function () {
			var file = $scope.myFile;
			if (typeof file == 'undefined') {
				HideNotify();
				ShowNotify($.t('Choose a File first!'), 2500, true);
				return;
			}
			$scope.uploadIcon(file);
		}

		$scope.RefreshIconList = function () {
			$scope.iconset = [];
			$scope.selectedIcon = [];

			$http({
                url: "json.htm?type=command&param=getcustomiconset",
            }).then(function successCallback(response) {
                var data = response.data;
				if (typeof data.result != 'undefined') {
					$scope.iconset = data.result;
				}
			});
		}

		$scope.OnIconSelected = function (icon) {
			var bWasSelected = icon.selected;
			$.each($scope.iconset, function (i, item) {
				item.selected = false;
			});
			icon.selected = true;
			$scope.selectedIcon = icon;
		}

		$scope.UpdateIconTitleDescription = function () {
			var bValid = true;
			bValid = bValid && checkLength($("#iconname"), 2, 100);
			bValid = bValid && checkLength($("#icondescription"), 2, 100);
			if (bValid == false) {
				ShowNotify($.t('Please enter a Name and Description!...'), 3500, true);
				return;
			}
			$.ajax({
				url: "json.htm?type=command&param=updatecustomicon&idx=" + $scope.selectedIcon.idx +
				'&name=' + encodeURIComponent($("#iconname").val()) +
				'&description=' + encodeURIComponent($("#icondescription").val()),
				async: false,
				dataType: 'json',
				success: function (data) {
					$scope.RefreshIconList();
				}
			});
		}

		$scope.DeleteIcon = function () {
			bootbox.confirm($.t("Are you sure to delete this Icon?"), function (result) {
				if (result == true) {
					$.ajax({
						url: "json.htm?type=command&param=deletecustomicon&idx=" + $scope.selectedIcon.idx,
						async: false,
						dataType: 'json',
						success: function (data) {
							$scope.RefreshIconList();
						}
					});
				}
			});
		}

		/* ---- Icon libraries -------------------------------------------------
		   A library is an icon font that Domoticz downloaded and now serves
		   from its own assets folder. Installing one means the server fetches a
		   stylesheet and every font it points at, which is not instant, so the
		   user gets told what is going on. */
		$scope.RefreshLibraryList = function () {
			$http({
				url: "json.htm?type=command&param=geticonlibraries",
			}).then(function successCallback(response) {
				var data = response.data;
				$scope.iconlibraries = (typeof data.result != 'undefined') ? data.result : [];
			}, function errorCallback(response) {
				$scope.iconlibraries = [];
			});
		}

		$scope.PrefillLibrary = function (suggestion) {
			$scope.newLibrary = {
				name: suggestion.name,
				prefix: suggestion.prefix,
				url: suggestion.url
			};
		}

		$scope.AddLibrary = function () {
			var library = $scope.newLibrary;
			if (!library.name || !library.url || !library.prefix) {
				ShowNotify($.t('Please enter a Name, URL and Prefix!'), 3500, true);
				return;
			}
			// The prefix becomes both a filename and a CSS class prefix, so the
			// server only accepts lowercase letters and digits. Say so here
			// instead of letting the request come back with an error.
			if (!/^[a-z0-9]{1,32}$/.test(library.prefix)) {
				ShowNotify($.t('The prefix may only contain lowercase letters and digits'), 3500, true);
				return;
			}
			if (!/^https?:\/\//i.test(library.url)) {
				ShowNotify($.t('Only http:// and https:// URLs are supported'), 3500, true);
				return;
			}

			ShowNotify($.t('Downloading icon library...'));
			$http({
				url: "json.htm?type=command&param=addiconlibrary" +
					'&name=' + encodeURIComponent(library.name) +
					'&prefix=' + encodeURIComponent(library.prefix) +
					'&url=' + encodeURIComponent(library.url),
			}).then(function successCallback(response) {
				HideNotify();
				var data = response.data;
				if (data.status != "OK") {
					ShowNotify($.t('Error adding Icon Library') + ": " + $.t(data.error), 5000, true);
					return;
				}
				$scope.newLibrary = { name: '', prefix: '', url: '' };
				$scope.RefreshLibraryList();
				iconLibraries.load();
			}, function errorCallback(response) {
				HideNotify();
				ShowNotify($.t('Error adding Icon Library'), 5000, true);
			});
		}

		$scope.RefreshLibrary = function (library) {
			ShowNotify($.t('Downloading icon library...'));
			$http({
				url: "json.htm?type=command&param=refreshiconlibrary&idx=" + library.idx,
			}).then(function successCallback(response) {
				HideNotify();
				var data = response.data;
				if (data.status != "OK") {
					ShowNotify($.t('Error refreshing Icon Library') + ": " + $.t(data.error), 5000, true);
					return;
				}
				$scope.RefreshLibraryList();
				iconLibraries.load();
			}, function errorCallback(response) {
				HideNotify();
				ShowNotify($.t('Error refreshing Icon Library'), 5000, true);
			});
		}

		$scope.DeleteLibrary = function (library) {
			bootbox.confirm($.t("Are you sure to remove this Icon Library?"), function (result) {
				if (result != true) {
					return;
				}
				$http({
					url: "json.htm?type=command&param=deleteiconlibrary&idx=" + library.idx,
				}).then(function successCallback(response) {
					$scope.RefreshLibraryList();
					iconLibraries.load();
				}, function errorCallback(response) {
					ShowNotify($.t('Error removing Icon Library'), 5000, true);
				});
			});
		}

		init();

		function init() {
			$('#iconsmain').i18n();
			// The Icon Libraries section sits outside #iconsmain, so it needs
			// its own pass.
			$('#iconlibrariesmain').i18n();
			$scope.RefreshIconList();
			$scope.RefreshLibraryList();
		};

	}]);
});