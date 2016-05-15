define(['app'], function (app) {
	app.controller('UserVariablesController', [ '$scope', '$rootScope', '$location', '$http', '$interval', function($scope,$rootScope,$location,$http,$interval) {

		$scope.userVariableIdx=0;
		$scope.varNames = [];

		DeleteVariable = function(idx)
		{
			bootbox.confirm($.t("Are you sure you want to remove this variable?"), function(result) {
				if (result==true) {
					$.ajax({
						url: "json.htm?type=command&param=deleteuservariable&idx=" + idx,
						async: false, 
						dataType: 'json',
						success: function(data) {
							$scope.RefreshUserVariablesTable();
						}
					});
				}
			});
		}

		AddVariable = function(type)
		{
			var idx = $scope.userVariableIdx;
			var uservariablename = $('#uservariablesedittable #uservariablename').val();
			var uservariabletype = $('#uservariablesedittable #uservariabletype option:selected').val();
			var uservariablevalue = $('#uservariablesedittable #uservariablevalue').val();
			
			if((type=="a") && (jQuery.inArray(uservariablename,$scope.varNames) != -1)){
				ShowNotify($.t('Variable name already exists!'), 2500, true);
			}
			else {
				if (type == "a") {
					var url = "json.htm?type=command&param=saveuservariable&vname=" + uservariablename + "&vtype=" + uservariabletype + "&vvalue=" + uservariablevalue;
				}
				else if (type == "u") {
					var url = "json.htm?type=command&param=updateuservariable&idx=" + idx + "&vname=" + uservariablename + "&vtype=" + uservariabletype + "&vvalue=" + uservariablevalue;
				}
				
				$.ajax({
					 url: url, 
					 async: false, 
					 dataType: 'json',
					 success: function(data) {
						if (typeof data != 'undefined') {
							if (data.status=="OK") {
								bootbox.alert($.t('User variable saved'));
								$scope.RefreshUserVariablesTable();
								$('#uservariablesedittable #uservariablename').val("");
								$('#uservariablesedittable #uservariablevalue').val("");
								$('#uservariablesedittable #uservariabletype').val("0");
							}
							else {
								ShowNotify($.t(data.status), 2500, true);
							}
						}
					 },
					 error: function(){
							ShowNotify($.t('Problem saving user variable!'), 2500, true);
					 }     
				});
			}
				
		}

		$scope.RefreshUserVariablesTable = function()
		{
		  $('#modal').show();
			$scope.userVariableIdx=0;
			$('#uservariableupdate').attr("class", "btnstyle3-dis");
			$('#uservariabledelete').attr("class", "btnstyle3-dis");
			$("#uservariableupdate").attr("href", "");
			$("#uservariabledelete").attr("href", "");

		  $scope.varNames = [];	
		  var oTable = $('#uservariablestable').dataTable();
		  oTable.fnClearTable();
		  $.ajax({
			 url: "json.htm?type=command&param=getuservariables",
			 async: false, 
			 dataType: 'json',
			 success: function(data) {   
			  if (typeof data.result != 'undefined') {
				$.each(data.result, function(i,item){
					$scope.varNames.push(item.Name);
					var typeWording;
					switch( item.Type ) {
						case "0" :
							typeWording = $.t('Integer');
							break;
						case "1" :
							typeWording = $.t('Float');
							break;
						case "2" :
							typeWording = $.t('String');
							break;					
						case "3" :
							typeWording = $.t('Date');
							break;				
						case "4" :
							typeWording = $.t('Time');
							break;
						case "5" :
							typeWording = $.t('DateTime');
							break;
						default:
							typeWording = "undefined";
					}
					var addId = oTable.fnAddData( {
						"DT_RowId": item.idx,
						"DT_ItemType": item.Type,
										"0": item.idx,
										"1": item.Name,
										"2": typeWording,
										"3": item.Value,
										"4": item.LastUpdate
					} );
				});
			  }
			 }
		  });
			/* Add a click handler to the rows - this could be used as a callback */
			$("#uservariablestable tbody").off();
			$("#uservariablestable tbody").on( 'click', 'tr', function () {
				if ( $(this).hasClass('row_selected') ) {
					$(this).removeClass('row_selected');
					$('#uservariableupdate').attr("class", "btnstyle3-dis");
					$('#uservariabledelete').attr("class", "btnstyle3-dis");
					$("#uservariableupdate").attr("href", "");
					$("#uservariabledelete").attr("href", "");
				}
				else {
					var oTable = $('#uservariablestable').dataTable();
					oTable.$('tr.row_selected').removeClass('row_selected');
					$(this).addClass('row_selected');
					var anSelected = fnGetSelected( oTable );
					if ( anSelected.length !== 0 ) {
						var data = oTable.fnGetData( anSelected[0] );
						var idx= data["DT_RowId"];
						$scope.userVariableIdx=idx;
						$("#uservariableupdate").attr("href", "javascript:AddVariable('u')");
						$("#uservariabledelete").attr("href", "javascript:DeleteVariable(" + idx + ")");
						$('#uservariableupdate').attr("class", "btnstyle3");
						$('#uservariabledelete').attr("class", "btnstyle3");
						$("#uservariablesedittable #uservariablename").val(data["1"]);
						$("#uservariablesedittable #uservariabletype").val(data["DT_ItemType"]);
						$("#uservariablesedittable #uservariablevalue").val(data["3"]);
					}
				}
			}); 
		  
		  $('#modal').hide();
		}

		$scope.ShowUserVariables = function()
		{
			$('#fibaromain').i18n();
			var oTable = $('#uservariablestable').dataTable( {
			  "sDom": '<"H"lfrC>t<"F"ip>',
				"oTableTools": {
					"sRowSelect": "single",
				},
				"aaSorting": [[0, "desc"]],
				"bSortClasses": false,
				"bProcessing": true,
				"bStateSave": true,
				"bJQueryUI": true,
				"aLengthMenu": [[25, 50, 100, -1], [25, 50, 100, "All"]],
				"iDisplayLength": 25,
				"sPaginationType": "full_numbers",
			  language: $.DataTableLanguage
			} );
			
			$scope.RefreshUserVariablesTable()
		}

		init();

		function init()
		{
			$scope.MakeGlobalConfig();
			$scope.ShowUserVariables();
		};
	} ]);
});