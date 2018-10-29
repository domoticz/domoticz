define(['app'], function (app) {
	app.controller('TimerplanController', [ '$scope', '$rootScope', '$location', '$http', '$interval', 'permissions', function($scope,$rootScope,$location,$http,$interval,permissions) {

		AddNewTimerPlan = function()
		{
			$( "#dialog-add-edit-timerplan" ).dialog({
				resizable: false,
				width: 390,
				height:200,
				modal: true,
				title: 'Add New Plan',
				buttons: {
					"Cancel": function() {
						$( this ).dialog( "close" );
					},
					"Add": function() {
						var csettings=GetTimerPlanSettings();
						if (typeof csettings == 'undefined') {
							return;
						}
						$( this ).dialog( "close" );
						AddTimerPlan();
					}
				},
				close: function() {
					$( this ).dialog( "close" );
				}
			});
		}

		EditTimerPlan = function(idx)
		{
			$( "#dialog-add-edit-timerplan" ).dialog({
				resizable: false,
				width: 390,
				height:200,
				modal: true,
				title: 'Edit Plan',
				buttons: {
					"Cancel": function() {
						$( this ).dialog( "close" );
					},
					"Update": function() {
						var csettings=GetTimerPlanSettings();
						if (typeof csettings == 'undefined') {
							return;
						}
						$( this ).dialog( "close" );
						UpdateTimerPlan(idx);
					}
				},
				close: function() {
					$( this ).dialog( "close" );
				}
			});
		}

		CopyTimerPlan = function(idx)
		{
			$( "#dialog-add-edit-timerplan" ).dialog({
				resizable: false,
				width: 390,
				height:200,
				modal: true,
				title: 'Duplicate Plan',
				buttons: {
					"Cancel": function() {
						$( this ).dialog( "close" );
					},
					"Duplicate": function() {
						var csettings=GetTimerPlanSettings();
						if (typeof csettings == 'undefined') {
							return;
						}
						$( this ).dialog( "close" );
						DuplicateTimerPlan(idx);
					}
				},
				close: function() {
					$( this ).dialog( "close" );
				}
			});
		}

		DeleteTimerPlan = function(idx)
		{
			bootbox.confirm($.t("Are you sure you want to delete this Plan?"), function(result) {
				if (result==true) {
					$.ajax({
						 url: "json.htm?type=command&param=deletetimerplan&idx=" + idx,
						 async: false, 
						 dataType: 'json',
						 success: function(data) {
							RefreshTimerPlanTable();
						 },
						 error: function(){
								HideNotify();
								ShowNotify('Problem deleting Plan!', 2500, true);
						 }     
					});
				}
			});
		}

		GetTimerPlanSettings = function()
		{
			var csettings = {};

			csettings.name=encodeURIComponent($("#dialog-add-edit-timerplan #planname").val());
			if (csettings.name=="")
			{
				ShowNotify('Please enter a Name!', 2500, true);
				return;
			}
			return csettings;
		}

		UpdateTimerPlan = function(idx)
		{
			var csettings=GetTimerPlanSettings();
			if (typeof csettings == 'undefined') {
				return;
			}

			$.ajax({
				 url: "json.htm?type=command&param=updatetimerplan&idx=" + idx +"&name=" + csettings.name,
				 async: false, 
				 dataType: 'json',
				 success: function(data) {
					RefreshTimerPlanTable();
				 },
				 error: function(){
					ShowNotify('Problem updating Plan settings!', 2500, true);
				 }     
			});
		}

		DuplicateTimerPlan = function(idx)
		{
			var csettings=GetTimerPlanSettings();
			if (typeof csettings == 'undefined') {
				return;
			}

			$.ajax({
				 url: "json.htm?type=command&param=duplicatetimerplan&idx=" + idx +"&name=" + csettings.name,
				 async: false, 
				 dataType: 'json',
				 success: function(data) {
					RefreshTimerPlanTable();
				 },
				 error: function(){
					ShowNotify('Problem duplicating Plan!', 2500, true);
				 }     
			});
		}

		AddTimerPlan = function()
		{
			var csettings=GetTimerPlanSettings();
			if (typeof csettings == 'undefined') {
				return;
			}

			$.ajax({
				 url: "json.htm?type=command&param=addtimerplan&name=" + csettings.name,
				 async: false, 
				 dataType: 'json',
				 success: function(data) {
					RefreshTimerPlanTable();
				 },
				 error: function(){
					ShowNotify('Problem adding Plan!', 2500, true);
				 }
			});
		}

		RefreshTimerPlanTable = function()
		{
			$('#modal').show();

			$.devIdx=-1;

			$('#updelclr #timerplanedit').attr("class", "btnstyle3-dis");
			$('#updelclr #timerplancopy').attr("class", "btnstyle3-dis");
			$('#updelclr #timerplandelete').attr("class", "btnstyle3-dis");

			var oTable = $('#timerplantable').dataTable();
			oTable.fnClearTable();
			$.ajax({
			 url: "json.htm?type=command&param=gettimerplans",
			 async: false, 
			 dataType: 'json',
			 success: function(data) {
				
			  if (typeof data.result != 'undefined') {
				var totalItems=data.result.length;
				$.each(data.result, function(i,item){
					var DisplayName = decodeURIComponent(item.Name);
					if (DisplayName == 'default') {
						DisplayName = $.t(DisplayName);
					}
					var addId = oTable.fnAddData( {
						"DT_RowId": item.idx,
						"Name": item.Name,
						"0": item.idx,
						"1": DisplayName
					} );
				});
			  }
			 }
			});

			/* Add a click handler to the rows - this could be used as a callback */
			$("#timerplantable tbody").off();
			$("#timerplantable tbody").on( 'click', 'tr', function () {
				$.devIdx=-1;
				if ( $(this).hasClass('row_selected') ) {
					$(this).removeClass('row_selected');
					$('#updelclr #timerplanedit').attr("class", "btnstyle3-dis");
					$('#updelclr #timerplancopy').attr("class", "btnstyle3-dis");
					$('#updelclr #timerplandelete').attr("class", "btnstyle3-dis");
					$("#dialog-add-edit-plan #planname").val("");
				}
				else {
					var oTable = $('#timerplantable').dataTable();
					oTable.$('tr.row_selected').removeClass('row_selected');
					$(this).addClass('row_selected');
					$('#updelclr #timerplanedit').attr("class", "btnstyle3-dis");
					$('#updelclr #timerplancopy').attr("class", "btnstyle3-dis");
					$('#updelclr #timerplandelete').attr("class", "btnstyle3-dis");
					
					var anSelected = fnGetSelected( oTable );
					if ( anSelected.length !== 0 ) {
						var data = oTable.fnGetData( anSelected[0] );
						var idx= data["DT_RowId"];
						$.devIdx=idx;
						$('#updelclr #timerplanedit').attr("class", "btnstyle3");
						$("#updelclr #timerplanedit").attr("href", "javascript:EditTimerPlan(" + idx + ")");
						$('#updelclr #timerplancopy').attr("class", "btnstyle3");
						$("#updelclr #timerplancopy").attr("href", "javascript:CopyTimerPlan(" + idx + ")");
						if (idx!=0) {
							//not allowed to delete the default timer plan
							$('#updelclr #timerplandelete').attr("class", "btnstyle3");
							$("#updelclr #timerplandelete").attr("href", "javascript:DeleteTimerPlan(" + idx + ")");
						}
						var DisplayName = decodeURIComponent(data["Name"]);
						if (DisplayName == 'default') {
							DisplayName = $.t(DisplayName);
						}
						$("#dialog-add-edit-timerplan #planname").val(DisplayName);
					}
				}
			}); 
		  
		  $('#modal').hide();
		}

		ShowTimerPlans = function()
		{
			var oTable;
			
			$('#modal').show();
			
			$.devIdx=-1;
			
			var htmlcontent = "";
			htmlcontent+=$('#timerplanmain').html();
			$('#timerplancontent').html(htmlcontent);
			$('#timerplancontent').i18n();
			
			oTable = $('#timerplantable').dataTable( {
				"sDom": '<"H"lfrC>t<"F"ip>',
				"oTableTools": {
					"sRowSelect": "single",
				},
				"bSort": false,
				"bProcessing": true,
				"bStateSave": true,
				"bJQueryUI": true,
				"aLengthMenu": [[25, 50, 100, -1], [25, 50, 100, "All"]],
				"iDisplayLength" : 25,
				"sPaginationType": "full_numbers",
				language: $.DataTableLanguage
				} );

			oTable = $('#activetable').dataTable( {
				"sDom": '<"H"lfrC>t<"F"ip>',
				"oTableTools": {
					"sRowSelect": "single",
				},
				"bSort": false,
				"bProcessing": true,
				"bStateSave": false,
				"bJQueryUI": true,
				"iDisplayLength" : -1,
				language: $.DataTableLanguage
			});

			$('#modal').hide();
			RefreshTimerPlanTable();
		}

		init();

		function init()
		{
			$scope.MakeGlobalConfig();
			ShowTimerPlans();
		};
		$scope.$on('$destroy', function(){
			if (typeof $scope.mytimer != 'undefined') {
				$interval.cancel($scope.mytimer);
				$scope.mytimer = undefined;
			}
		}); 
	} ]);
});