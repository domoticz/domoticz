define(['app'], function (app) {
	app.controller('RoomplanController', [ '$scope', '$rootScope', '$location', '$http', '$interval', 'permissions', function($scope,$rootScope,$location,$http,$interval,permissions) {

		ChangePlanOrder = function(order, planid)
		{
			if (!permissions.hasPermission("Admin")) {
				HideNotify();
				ShowNotify($.t('You do not have permission to do that!'), 2500, true);
				return;
			}
		  $.ajax({
			 url: "json.htm?type=command&param=changeplanorder&idx=" + planid + "&way=" + order, 
			 async: false, 
			 dataType: 'json',
			 success: function(data) {
				RefreshPlanTable();
			 }
		  });
		}
		ChangeDeviceOrder = function(order, devid)
		{
			if (!permissions.hasPermission("Admin")) {
				HideNotify();
				ShowNotify($.t('You do not have permission to do that!'), 2500, true);
				return;
			}
		  $.ajax({
			 url: "json.htm?type=command&param=changeplandeviceorder&planid=" + $.LastPlan + "&idx=" + devid + "&way=" + order, 
			 async: false, 
			 dataType: 'json',
			 success: function(data) {
				RefreshActiveDevicesTable($.LastPlan);
			 }
		  });
		}

		AddNewPlan = function()
		{
			$( "#dialog-add-edit-plan" ).dialog({
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
						var csettings=GetPlanSettings();
						if (typeof csettings == 'undefined') {
							return;
						}
						$( this ).dialog( "close" );
						AddPlan();
					}
				},
				close: function() {
					$( this ).dialog( "close" );
				}
			});
		}

		EditPlan = function(idx)
		{
			$( "#dialog-add-edit-plan" ).dialog({
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
						var csettings=GetPlanSettings();
						if (typeof csettings == 'undefined') {
							return;
						}
						$( this ).dialog( "close" );
						UpdatePlan(idx);
					}
				},
				close: function() {
					$( this ).dialog( "close" );
				}
			});
		}

		DeletePlan = function(idx)
		{
			bootbox.confirm($.t("Are you sure you want to delete this Plan?"), function(result) {
				if (result==true) {
					$.ajax({
						 url: "json.htm?type=command&param=deleteplan&idx=" + idx,
						 async: false, 
						 dataType: 'json',
						 success: function(data) {
							RefreshPlanTable();
							RefreshActiveDevicesTable($.devIdx);
						 },
						 error: function(){
								HideNotify();
								ShowNotify('Problem deleting Plan!', 2500, true);
						 }     
					});
				}
			});
		}

		GetPlanSettings = function()
		{
			var csettings = {};

			csettings.name=encodeURIComponent($("#dialog-add-edit-plan #planname").val());
			if (csettings.name=="")
			{
				ShowNotify('Please enter a Name!', 2500, true);
				return;
			}
			return csettings;
		}

		UpdatePlan = function(idx)
		{
			var csettings=GetPlanSettings();
			if (typeof csettings == 'undefined') {
				return;
			}

			$.ajax({
				 url: "json.htm?type=command&param=updateplan&idx=" + idx +"&name=" + csettings.name,
				 async: false, 
				 dataType: 'json',
				 success: function(data) {
					RefreshPlanTable();
				 },
				 error: function(){
					ShowNotify('Problem updating Plan settings!', 2500, true);
				 }     
			});
		}

		AddPlan = function()
		{
			var csettings=GetPlanSettings();
			if (typeof csettings == 'undefined') {
				return;
			}

			$.ajax({
				 url: "json.htm?type=command&param=addplan&name=" + csettings.name,
				 async: false, 
				 dataType: 'json',
				 success: function(data) {
					RefreshPlanTable();
				 },
				 error: function(){
					ShowNotify('Problem adding Plan!', 2500, true);
				 }
			});
		}

		RefreshPlanTable = function()
		{
			$('#modal').show();

			$.devIdx=-1;

			$('#updelclr #planedit').attr("class", "btnstyle3-dis");
			$('#updelclr #plandelete').attr("class", "btnstyle3-dis");
			$('#plancontent #delclractive #activedeviceclear').attr("class", "btnstyle3-dis");

			var oTable = $('#plantable').dataTable();
			oTable.fnClearTable();
			$.ajax({
			 url: "json.htm?type=plans&displayhidden=1",
			 async: false, 
			 dataType: 'json',
			 success: function(data) {
				
			  if (typeof data.result != 'undefined') {
				var totalItems=data.result.length;
				$.each(data.result, function(i,item){
					var updownImg="";
					if (i!=totalItems-1) {
						//Add Down Image
						if (updownImg!="") {
							updownImg+="&nbsp;";
						}
						updownImg+='<img src="images/down.png" onclick="ChangePlanOrder(1,' + item.idx + ');" class="lcursor" width="16" height="16"></img>';
					}
					else {
						updownImg+='<img src="images/empty16.png" width="16" height="16"></img>';
					}
					if (i!=0) {
						//Add Up image
						if (updownImg!="") {
							updownImg+="&nbsp;";
						}
						updownImg+='<img src="images/up.png" onclick="ChangePlanOrder(0,' + item.idx + ');" class="lcursor" width="16" height="16"></img>';
					}
					var displayName=item.Name;
					if (item.Name=="$Hidden Devices") {
						displayName='<span style="color:#008080"><b>'+item.Name+'</b></span>';
					}
					var addId = oTable.fnAddData( {
						"DT_RowId": item.idx,
						"Name": item.Name,
						"Order": item.Order,
						"0": item.idx,
						"1": displayName,
						"2": updownImg
					} );
				});
			  }
			 }
			});

			/* Add a click handler to the rows - this could be used as a callback */
			$("#plantable tbody").off();
			$("#plantable tbody").on( 'click', 'tr', function () {
				$.devIdx=-1;

				if ( $(this).hasClass('row_selected') ) {
					$(this).removeClass('row_selected');
					$('#updelclr #planedit').attr("class", "btnstyle3-dis");
					$('#updelclr #plandelete').attr("class", "btnstyle3-dis");
					$('#plancontent #delclractive #activedeviceclear').attr("class", "btnstyle3-dis");
					$("#dialog-add-edit-plan #planname").val("");
					RefreshActiveDevicesTable(-1);
				}
				else {
					var oTable = $('#plantable').dataTable();
					oTable.$('tr.row_selected').removeClass('row_selected');
					$(this).addClass('row_selected');
					$('#plancontent #delclractive #activedeviceclear').attr("class", "btnstyle3");
					$('#updelclr #planedit').attr("class", "btnstyle3-dis");
					$('#updelclr #plandelete').attr("class", "btnstyle3-dis");
					
					var anSelected = fnGetSelected( oTable );
					if ( anSelected.length !== 0 ) {
						var data = oTable.fnGetData( anSelected[0] );
						var idx= data["DT_RowId"];
						$.devIdx=idx;
						var name=data["Name"];
						if (name!="$Hidden Devices") {
							$('#updelclr #planedit').attr("class", "btnstyle3");
							$('#updelclr #plandelete').attr("class", "btnstyle3");
							$("#updelclr #planedit").attr("href", "javascript:EditPlan(" + idx + ")");
							$("#updelclr #plandelete").attr("href", "javascript:DeletePlan(" + idx + ")");
							$("#dialog-add-edit-plan #planname").val(decodeURIComponent(data["Name"]));
						}
						RefreshActiveDevicesTable(idx);
					}
				}
			}); 
		  
		  $('#modal').hide();
		}

		RefreshUnusedDevicesComboArray = function()
		{
			$.UnusedDevices = [];
			$("#plancontent #comboactivedevice").empty();
			$.ajax({
				url: "json.htm?type=command&param=getunusedplandevices&unique=false", 
				async: false, 
				dataType: 'json',
				success: function(data) {
					if (typeof data.result != 'undefined') {
						$.each(data.result, function(i,item) {
							$.UnusedDevices.push({
								type: item.type,
								idx: item.idx,
								name: item.Name
							});
						});
						$.each($.UnusedDevices, function(i,item){
							var option = $('<option />');
							option.attr('value', item.idx).text(item.name);
							$("#plancontent #comboactivedevice").append(option);
						});
					}
				}
			});
		}

		ShowPlans = function()
		{
			var oTable;
			
			$('#modal').show();
			
			$.devIdx=-1;
			
			var htmlcontent = "";
			htmlcontent+=$('#planmain').html();
			$('#plancontent').html(htmlcontent);
			$('#plancontent').i18n();
			
			oTable = $('#plantable').dataTable( {
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

			RefreshUnusedDevicesComboArray();
			
			$('#modal').hide();
			RefreshPlanTable();
		}

		RefreshActiveDevicesTable = function(idx)
		{
			$.LastPlan=idx;
			$('#modal').show();

			$('#plancontent #delclractive #activedevicedelete').attr("class", "btnstyle3-dis");

			var oTable = $('#plancontent #activetable').dataTable();
			oTable.fnClearTable();
		  
			$.ajax({
				url: "json.htm?type=command&param=getplandevices&idx=" + idx, 
				async: false, 
				dataType: 'json',
				success: function(data) {
					if (typeof data.result != 'undefined') {
						var totalItems=data.result.length;
						$.each(data.result, function(i,item){
							var updownImg="";
							if (i!=totalItems-1) {
								//Add Down Image
								if (updownImg!="") {
									updownImg+="&nbsp;";
								}
								updownImg+='<img src="images/down.png" onclick="ChangeDeviceOrder(1,' + item.idx + ');" class="lcursor" width="16" height="16"></img>';
							}
							else {
								updownImg+='<img src="images/empty16.png" width="16" height="16"></img>';
							}
							if (i!=0) {
								//Add Up image
								if (updownImg!="") {
									updownImg+="&nbsp;";
								}
								updownImg+='<img src="images/up.png" onclick="ChangeDeviceOrder(0,' + item.idx + ');" class="lcursor" width="16" height="16"></img>';
							}
							var addId = oTable.fnAddData( {
								"DT_RowId": item.idx,
								"Order": item.Order,
								"0": item.devidx,
								"1": item.Name,
								"2": updownImg
							} );
						});
					}
				}
			});

			/* Add a click handler to the rows - this could be used as a callback */
			$("#plancontent #activetable tbody").off();
			$("#plancontent #activetable tbody").on( 'click', 'tr', function () {
				if ( $(this).hasClass('row_selected') ) {
					$(this).removeClass('row_selected');
					$('#plancontent #delclractive #activedevicedelete').attr("class", "btnstyle3-dis");
				}
				else {
					var oTable = $('#plancontent #activetable').dataTable();
					oTable.$('tr.row_selected').removeClass('row_selected');
					$(this).addClass('row_selected');
					$('#plancontent #delclractive #activedevicedelete').attr("class", "btnstyle3");
					
					var anSelected = fnGetSelected( oTable );
					if ( anSelected.length !== 0 ) {
						var data = oTable.fnGetData( anSelected[0] );
						var idx= data["DT_RowId"];
						$("#plancontent #delclractive #activedevicedelete").attr("href", "javascript:DeleteActiveDevice(" + idx + ")");
					}
				}
			}); 

		  $('#modal').hide();
		}

		AddActiveDevice = function()
		{
			if ($.devIdx==-1) {
				bootbox.alert('No Plan Selected!');
				return;
			}

			var ADType=0;
			if ($("#plancontent #comboactivedevice option:selected").text().indexOf("[Scene]") == 0) {
				ADType=1;
			}

			var ActiveDeviceIdx=$("#plancontent #comboactivedevice option:selected").val();
			if (typeof ActiveDeviceIdx == 'undefined') {
				bootbox.alert('No Active Device Selected!');
				return ;
			}
			$.ajax({
				url: "json.htm?type=command&param=addplanactivedevice&idx=" + $.devIdx + 
					"&activetype=" + ADType +
					"&activeidx=" + ActiveDeviceIdx,
				async: false, 
				dataType: 'json',
				success: function(data) {
					if (data.status == 'OK') {
						RefreshActiveDevicesTable($.devIdx);
						//RefreshUnusedDevicesComboArray();
					}
					else {
						ShowNotify('Problem adding Device!', 2500, true);
					}
				},
				error: function(){
					HideNotify();
					ShowNotify('Problem adding Device!', 2500, true);
				}     
			});
			
		}

		DeleteActiveDevice = function(idx)
		{
			bootbox.confirm($.t("Are you sure to delete this Active Device?\n\nThis action can not be undone..."), function(result) {
				if (result==true) {
					$.ajax({
						 url: "json.htm?type=command&param=deleteplandevice&idx=" + idx,
						 async: false, 
						 dataType: 'json',
						 success: function(data) {
							RefreshActiveDevicesTable($.devIdx);
							//RefreshUnusedDevicesComboArray();
						 }
					});
				}
			});
		}

		ClearActiveDevices = function()
		{
			bootbox.confirm($.t("Are you sure to delete ALL Active Devices?\n\nThis action can not be undone!!"), function(result) {
				if (result==true) {
					$.ajax({
							url: "json.htm?type=command&param=deleteallplandevices&idx=" + $.devIdx,
							async: false, 
							dataType: 'json',
							success: function(data) {
							RefreshActiveDevicesTable($.devIdx);
							//RefreshUnusedDevicesComboArray();
						}
					});
				}
			});
		}

		init();

		function init()
		{
			$scope.MakeGlobalConfig();
		  ShowPlans();
		};
		$scope.$on('$destroy', function(){
			if (typeof $scope.mytimer != 'undefined') {
				$interval.cancel($scope.mytimer);
				$scope.mytimer = undefined;
			}
		}); 
	} ]);
});