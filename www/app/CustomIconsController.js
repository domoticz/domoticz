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
					ShowNotify($.t('Error uploading Iconset') + ": " + data.error, 5000, true);
				}
				$scope.RefreshIconList();
			})
			.error(function(data){
				HideNotify();
				ShowNotify($.t('Error uploading Iconset'), 5000, true);
			});
		}
		
		$scope.UploadIconSet = function() {
			var file = $scope.myFile;
			if (typeof file == 'undefined') {
				HideNotify();
				ShowNotify($.t('Choose a File first!'), 2500, true);
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
			var bWasSelected = icon.selected;
			$.each($scope.iconset, function(i,item){
				item.selected = false;
			});
			icon.selected = true;
			$scope.selectedIcon = icon;
		}
		
		$scope.UpdateIconTitleDescription = function() {
		  var bValid = true;
		  bValid = bValid && checkLength( $("#iconname"), 2, 100 );
		  bValid = bValid && checkLength( $("#icondescription"), 2, 100 );
		  if ( bValid == false ) {
				ShowNotify($.t('Please enter a Name and Description!...'), 3500, true);
				return;
		  }
		  $.ajax({
			 url: "json.htm?type=command&param=updatecustomicon&idx=" + $scope.selectedIcon.idx +
				'&name=' + encodeURIComponent($("#iconname").val()) + 
				'&description=' + encodeURIComponent($("#icondescription").val()),
			 async: false,
			 dataType: 'json',
			 success: function(data) {
				$scope.RefreshIconList();
			 }
		  });
		}
		
		$scope.DeleteIcon = function() {
			bootbox.confirm($.t("Are you sure to delete this Icon?"), function(result) {
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
			$('#iconsmain').i18n();
			$scope.RefreshIconList();
		};

	} ]);
});