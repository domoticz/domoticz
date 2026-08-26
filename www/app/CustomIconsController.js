define(['app', 'iconLibraries'], function (app) {
	app.controller('CustomIconsController', ['$scope', '$rootScope', '$location', '$http', '$interval', 'iconLibraries', function ($scope, $rootScope, $location, $http, $interval, iconLibraries) {

		$scope.iconset = [];
		$scope.selectedIcon = [];
		$scope.iconlibraries = [];
		$scope.newLibrary = { title: '', prefix: '', url: '' };

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

		$scope.RefreshLibraryList = function () {
			$http({
				url: "json.htm?type=command&param=getwebassets",
			}).then(function successCallback(response) {
				var assets = (typeof response.data.result != 'undefined') ? response.data.result : [];
				$scope.iconlibraries = assets.filter(function (asset) {
					return asset.name && /\.css$/i.test(asset.name);
				}).map(function (asset) {
					var prefix = asset.name.split('.')[0];
					return {
						name: asset.name,
						prefix: prefix,
						Title: asset.Title || '',
						// Libraries installed before titles existed have none stored.
						TitleDisplay: asset.Title || prefix,
						SourceURL: asset.SourceURL || '',
						LastUpdate: asset.LastUpdate || ''
					};
				}).sort(function (a, b) {
					return a.TitleDisplay.localeCompare(b.TitleDisplay);
				});
			}, function errorCallback(response) {
				$scope.iconlibraries = [];
			});
		}

		$scope.PrefillLibrary = function (suggestion) {
			$scope.newLibrary = {
				title: suggestion.name,
				prefix: suggestion.prefix,
				url: suggestion.url
			};
		}

		function InstallLibrary(name, url, title, szError, bClearForm) {
			ShowNotify($.t('Downloading icon library...'));
			$http({
				url: "json.htm?type=command&param=uploadwebasset" +
					'&name=' + encodeURIComponent(name) +
					'&url=' + encodeURIComponent(url) +
					'&title=' + encodeURIComponent(title || ''),
			}).then(function successCallback(response) {
				HideNotify();
				var data = response.data;
				if (data.status != "OK") {
					ShowNotify(szError + ": " + $.t(data.error), 5000, true);
					return;
				}
				if (bClearForm) {
					$scope.newLibrary = { title: '', prefix: '', url: '' };
				}
				$scope.RefreshLibraryList();
				iconLibraries.load();
			}, function errorCallback(response) {
				HideNotify();
				ShowNotify(szError, 5000, true);
			});
		}

		$scope.AddLibrary = function () {
			var library = $scope.newLibrary;
			if (!library.url || !library.prefix) {
				ShowNotify($.t('Please enter a URL and Prefix!'), 3500, true);
				return;
			}
			if (!/^[a-z0-9]{1,32}$/.test(library.prefix)) {
				ShowNotify($.t('The prefix may only contain lowercase letters and digits'), 3500, true);
				return;
			}
			if (!/^https?:\/\//i.test(library.url)) {
				ShowNotify($.t('Only http:// and https:// URLs are supported'), 3500, true);
				return;
			}

			// The title never takes part in the stored name, that stays prefix based.
			InstallLibrary(library.prefix + '.css', library.url, library.title, $.t('Error adding Icon Library'), true);
		}

		$scope.RefreshLibrary = function (library) {
			if (!library.SourceURL) {
				ShowNotify($.t('This library has no source URL to refresh from'), 3500, true);
				return;
			}
			InstallLibrary(library.name, library.SourceURL, library.Title, $.t('Error refreshing Icon Library'), false);
		}

		$scope.DeleteLibrary = function (library) {
			bootbox.confirm($.t("Are you sure to remove this Icon Library?"), function (result) {
				if (result != true) {
					return;
				}
				$http({
					url: "json.htm?type=command&param=deletewebasset&name=" + encodeURIComponent(library.name),
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
			$('#iconlibrariesmain').i18n();
			$scope.RefreshIconList();
			$scope.RefreshLibraryList();
		};

	}]);
});