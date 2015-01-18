define(['app'], function (app) {
	app.controller('CustomIconsController', [ '$scope', '$rootScope', '$location', '$http', '$interval', function($scope,$rootScope,$location,$http,$interval) {

		$scope.iconset = [];
		$scope.selectedIcon = [];

		$scope.uploadFileToUrl = function(file, uploadUrl){
			var fd = new FormData();
			fd.append('file', file);
			$http.post(uploadUrl, fd, {
				transformRequest: angular.identity,
				headers: {'Content-Type': undefined}
			})
			.success(function(data){
				if (data.status!="OK") {
					HideNotify();
					ShowNotify($.i18n('Error uploading Iconset') + ": " + data.error, 5000, true);
				}
				$scope.RefreshIconList();
			})
			.error(function(data){
				HideNotify();
				ShowNotify($.i18n('Error uploading Iconset'), 5000, true);
			});
		}
		
		$scope.UploadIconSet = function() {
			var file = $scope.myFile;
			if (typeof file == 'undefined') {
				HideNotify();
				ShowNotify($.i18n('Choose a File first!'), 2500, true);
				return;
			}
			$scope.uploadFileToUrl(file, "uploadcustomicon");
		}
		
		$scope.RefreshIconList = function() {
			$scope.iconset = [];
			$scope.selectedIcon = [];
			
			$http.get("json.htm?type=command&param=getcustomiconset").
				success(function(data, status, headers, config) {
					if (typeof data.result != 'undefined') {
						$scope.iconset = data.result;
					}
			});
		}
		
		$scope.OnIconSelected = function(icon) {
			$.each($scope.iconset, function(i,item){
				item.selected = false;
			});
			icon.selected = true;
			$scope.selectedIcon = icon;
		}
		
		$scope.DeleteIcon = function() {
			bootbox.confirm($.i18n("Are you sure to delete this Icon?"), function(result) {
				if (result==true) {
					$.ajax({
							url: "json.htm?type=command&param=deletecustomicon&idx=" + $scope.selectedIcon.idx,
							async: false, 
							dataType: 'json',
							success: function(data) {
								$scope.RefreshIconList();
							}
					});
				}
			});
		}
		
		init();

		function init()
		{
			$scope.RefreshIconList();
		};

	} ]);
});