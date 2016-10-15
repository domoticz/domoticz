define(['app'], function (app) {
	app.controller('DevicesController', [ '$scope', '$rootScope', '$location', '$http', '$interval', function($scope,$rootScope,$location,$http,$interval) {

		AddDevice = function(idx,itemname)
		{
			$.devIdx = idx;
			if (name!='Unknown') {
				$( "#dialog-adddevice #devicename" ).val(unescape(itemname));
			}
			$("#dialog-adddevice").i18n();
			$("#dialog-adddevice").dialog( "open" );
		}

		RenameDevice = function(idx,itype,itemname)
		{
			$.devType=itype;
			$.devIdx = idx;
			if (name!='Unknown') {
				$( "#dialog-renamedevice #devicename" ).val(unescape(itemname));
			}
			$("#dialog-renamedevice").i18n();
			$("#dialog-renamedevice").dialog( "open" );
		}

		AddLightDeviceDev = function (idx, name)
		{
			$.devIdx = idx;
			
			$("#dialog-addlightdevicedev #combosubdevice").html("");
			
				$.each($.LightsAndSwitches, function(i,item){
					var option = $('<option />');
					option.attr('value', item.idx).text(item.name);
					$("#dialog-addlightdevicedev #combosubdevice").append(option);
				});
				$("#dialog-addlightdevicedev #combosubdevice").val(0);
			
			if (name!='Unknown') {
				$( "#dialog-addlightdevicedev #devicename" ).val(unescape(name));
			}
			$("#dialog-addlightdevicedev").i18n();
			$("#dialog-addlightdevicedev" ).dialog( "open" );
		}

		SetUnused = function(idx)
		{
			bootbox.confirm($.t("Are you sure to remove this Device from your used devices?"), function(result) {
				if (result==true) {
					$.ajax({
						url: "json.htm?type=command&param=setunused&idx=" + idx,
						async: false, 
						dataType: 'json',
						success: function(data) {
							ShowDevices();
						}
					});
				}
			});
		}

		InvertCheck = function()
		{
			$('#devices input:checkbox').each(function(){
				$(this).prop('checked', !$(this).is(":checked"));
			});
		}
		DeleteMultipleDevices = function()
		{
			var totalselected=$('#devices input:checkbox:checked').length;
			if (totalselected==0) {
				bootbox.alert($.t('No Devices selected to Delete!'));
				return;
			}
			var d2delete = "";
			var delCount = 0;
			$('#devices input:checkbox:checked').each(function() {
				if (d2delete!="")
					d2delete+=";";
				d2delete+=$(this).val();
				delCount++;
			});
			bootbox.confirm($.t("Are you sure you want to delete the selected Devices?") + " (" + delCount + ")", function(result) {
				if (result==true) {
					$.ajax({
						url: "json.htm?type=deletedevice&idx=" + d2delete,
						async: false, 
						dataType: 'json',
						success: function(data) {
							bootbox.alert(delCount+" " + $.t("Devices deleted."));
							ShowDevices();
						}
					});
				}
			});
		}

		RefreshLightSwitchesComboArray = function()
		{
			$.LightsAndSwitches = [];
		  $.ajax({
			 url: "json.htm?type=command&param=getlightswitches", 
			 async: false, 
			 dataType: 'json',
			 success: function(data) {
			  if (typeof data.result != 'undefined') {
				$.each(data.result, function(i,item){
							$.LightsAndSwitches.push({
									idx: item.idx,
									name: item.Name
								 }
							);
				});
			  }
			 }
		  });
		}

		ShowDevices = function(filter)
		{
			if (typeof filter != 'undefined') {
				$.DevicesFilter=filter;
			}
			else {
				if (typeof $.DevicesFilter != 'undefined') {
					filter=$.DevicesFilter;
				}
			}
		  
		  RefreshLightSwitchesComboArray();
		  
		  $("#devicestable #mUsed").attr('class', 'btnstyle3');
		  $("#devicestable #mAll").attr('class', 'btnstyle3');
		  $("#devicestable #mUnknown").attr('class', 'btnstyle3');
		 
		  var ifilter="all";
		  if (typeof filter != 'undefined') {
			if (filter == "used") {
			  ifilter = "true";
					$("#devicestable #mUsed").attr('class', 'btnstyle3-sel');
			}
			else if (filter == "unknown") {
					$("#devicestable #mUnknown").attr('class', 'btnstyle3-sel');
			  ifilter = "false";
			}
			else if (filter == "all") {
					$("#devicestable #mAll").attr('class', 'btnstyle3-sel');
			  ifilter = "all";
			}
		  }
		  else {
			  ChangeClass("mAll","btnstyle3-sel");
		  }
		  
		  var htmlcontent = '';
		  htmlcontent+=$('#devicestable').html();
		  $('#devicescontent').html(htmlcontent);
		  $('#devicescontent').i18n();

			$('#devicescontent #devices').dataTable( {
				"sDom": '<"H"lfrC>t<"F"ip>',
				"oTableTools": {
					"sRowSelect": "single"
				},
				"aoColumnDefs": [
					{ "bSortable": false, "aTargets": [ 0,11 ] }
				],
				"aoColumns": [
					null,
					null,
					null,
					null,
					null,
					null,
					null,
					null,
					null,
					null,
					{ "sType": "numeric-battery" },
					null,
					null
				],
				"aaSorting": [[ 12, "desc" ]],
				"bSortClasses": false,
				"bProcessing": true,
				"bStateSave": true,
				"bJQueryUI": true,
				"aLengthMenu": [[25, 50, 100, -1], [25, 50, 100, "All"]],
				"iDisplayLength" : 25,
				"sPaginationType": "full_numbers",
				language: $.DataTableLanguage
			} );

		  var mTable = $('#devicescontent #devices');
		  var oTable = mTable.dataTable();
		  oTable.fnClearTable();
		  
		  $.ajax({
			 url: "json.htm?type=devices&displayhidden=1&filter=all&used=" + ifilter, 
			 async: false, 
			 dataType: 'json',
			 success: function(data) {
			  if (typeof data.result != 'undefined') {
				$.each(data.result, function(i,item){
					if ((item.Type=="Group")||(item.Type=="Scene")) {
					  item.HardwareName="Domoticz";
					  item.ID="-";
					  item.Unit="-";
					  item.SubType="-";
					  item.SignalLevel="-";
					  item.BatteryLevel=255;
					}
				  var itemSubIcons="";
							var itemChecker = '<input type="checkbox" name="Check-' + item.ID + ' id="Check-' + item.ID + '" value="'+item.idx+'" />';
				  var TypeImg=item.TypeImg;
				  var itemImage='<img src="images/' + TypeImg + '.png" width="16" height="16">';
				  if (TypeImg.indexOf("Alert")==0) {
									itemImage='<img src="images/Alert48_' + item.Level + '.png" width="16" height="16">';
				  }
				  else if ((TypeImg.indexOf("lightbulb")==0)||(TypeImg.indexOf("dimmer")==0)) {
									if (
											(item.Status == 'On')||
											(item.Status == 'Chime')||
											(item.Status == 'Group On')||
											(item.Status.indexOf('Set ') == 0)
										 ) {
													itemImage='<img src="images/lightbulb.png" title="Turn Off" onclick="SwitchLight(' + item.idx + ',\'Off\',ShowDevices);" class="lcursor">';
									}
									else {
													itemImage='<img src="images/lightbulboff.png" title="Turn On" onclick="SwitchLight(' + item.idx + ',\'On\',ShowDevices);" class="lcursor">';
									}
				  }
				  else if (TypeImg.indexOf("pushoff")==0) {
					itemImage='<img src="images/pushoff.png" title="Turn Off" onclick="SwitchLight(' + item.idx + ',\'Off\',ShowDevices);" class="lcursor">';
				  }
				  else if (TypeImg.indexOf("push")==0) {
					itemImage='<img src="images/push.png" title="Turn On" onclick="SwitchLight(' + item.idx + ',\'On\',ShowDevices);" class="lcursor">';
				  }
				  else if (TypeImg.indexOf("motion")==0) {
									if (
											(item.Status == 'On')||
											(item.Status == 'Chime')||
											(item.Status == 'Group On')||
											(item.Status.indexOf('Set ') == 0)
										 ) {
													itemImage='<img src="images/motion.png">';
									}
									else {
													itemImage='<img src="images/motionoff.png">';
									}
				  }
				  else if (TypeImg.indexOf("smoke")==0) {
									if (item.Status == 'Panic') {
											itemImage='<img src="images/smoke.png">';
									}
									else {
											itemImage='<img src="images/smokeoff.png">';
									}
				  }
				  else if (TypeImg.indexOf("scene")==0) {
									itemImage='<img src="images/push.png" title="Switch Scene" onclick="SwitchScene(' + item.idx + ',\'On\',ShowDevices);" class="lcursor">';
				  }
				  else if (TypeImg.indexOf("group")==0) {
									if (
											(item.Status == 'On')||
											(item.Status == 'Mixed')
										 ) {
													itemImage='<img src="images/pushoff.png" title="Turn Off" onclick="SwitchScene(' + item.idx + ',\'Off\',ShowDevices);" class="lcursor">';
									}
									else {
													itemImage='<img src="images/push.png" title="Turn On" onclick="SwitchScene(' + item.idx + ',\'On\',ShowDevices);" class="lcursor">';
									}
				  } else if (item.SubType === "Selector Switch") {
					var imagePath; 
					if (item.CustomImage !== 0) {
						imagePath = (this.levelName === "Off") ? "images/" + item.Image + "48_Off.png" : "images/" + item.Image + "48_On.png";
					} else {
						imagePath = (this.levelName === "Off") ? "images/" + item.TypeImg + "48_Off.png" : "images/" + item.TypeImg + "48_On.png";
					}
					itemImage = '<img src="' + imagePath + '" width="16" height="16">';
				  }
				  if ((item.Type == "Group")||(item.Type == "Scene")) {
					itemSubIcons+='&nbsp;<img src="images/empty16.png">';
					itemSubIcons+='<img src="images/rename.png" title="' + $.t('Rename Device') +'" onclick="RenameDevice(' + item.idx +',\'' + item.Type +'\',\'' + escape(item.Name) + '\')">';
				  }
				  else {
					  if (item.Used!=0) {
						itemSubIcons+='<img src="images/remove.png" title="' + $.t('Set Unused') +'" onclick="SetUnused(' + item.idx +')">';
						itemSubIcons+='<img src="images/rename.png" title="' + $.t('Rename Device') +'" onclick="RenameDevice(' + item.idx +',\'' + item.Type+'\',\'' + escape(item.Name) + '\')">';
					  }
					  else {
						if (
								(item.Type.indexOf("Light")==0)||
								(item.Type.indexOf("Security")==0)
							 )
						{
							itemSubIcons+='<img src="images/add.png" title="' + $.t('Add Light/Switch Device') + '" onclick="AddLightDeviceDev(' + item.idx +',\'' + escape(item.Name) + '\')">';
						}
						else {
							itemSubIcons+='<img src="images/add.png" title="' + $.t('Add Device') +'" onclick="AddDevice(' + item.idx +',\'' + escape(item.Name) + '\')">';
						}
						itemSubIcons+='<img src="images/rename.png" title="' + $.t('Rename Device') +'" onclick="RenameDevice(' + item.idx +',\'' + item.Type +'\',\'' + escape(item.Name) + '\')">';
					  }
				  }
				  if (
						(item.Type.indexOf("Light")==0)||
						(item.Type.indexOf("Chime")==0)||
						(item.Type.indexOf("Security")==0)||
						(item.Type.indexOf("RFY")==0)||
						(item.Type.indexOf("ASA")==0)
					 )
				  {
					itemSubIcons+='&nbsp;<img src="images/log.png" title="' + $.t('Log') +'" onclick="ShowLightLog(' + item.idx + ',\'' + escape(item.Name)  + '\', \'#devicescontent\', \'ShowDevices\');">';
				  }
				  else if ((item.Type.indexOf("Temp")==0)||(item.Type.indexOf("Thermostat")==0)||(item.Type.indexOf("Humidity")==0)) {
					itemSubIcons+='&nbsp;<img src="images/log.png" title="' + $.t('Log') +'" onclick="ShowTempLog(\'#devicescontent\',\'ShowDevices\',' + item.idx + ',\'' + escape(item.Name) + '\');">';
				  }
				  else if (item.SubType=="Voltage") {
					itemSubIcons+='&nbsp;<img src="images/log.png" title="' + $.t('Log') +'" onclick="ShowGeneralGraph(\'#devicescontent\',\'ShowDevices\',' + item.idx + ',\'' + escape(item.Name) + '\',' + item.SwitchTypeVal +', \'VoltageGeneral\');">';
				  }
				  else if (item.SubType=="Current") {
					itemSubIcons+='&nbsp;<img src="images/log.png" title="' + $.t('Log') +'" onclick="ShowGeneralGraph(\'#devicescontent\',\'ShowDevices\',' + item.idx + ',\'' + escape(item.Name) + '\',' + item.SwitchTypeVal +', \'CurrentGeneral\');">';
				  }
				  else if (item.SubType=="Pressure") {
					itemSubIcons+='&nbsp;<img src="images/log.png" title="' + $.t('Log') +'" onclick="ShowGeneralGraph(\'#devicescontent\',\'ShowDevices\',' + item.idx + ',\'' + escape(item.Name) + '\',' + item.SwitchTypeVal +', \'Pressure\');">';
				  }
				  else if (item.SubType=="Custom Sensor") {
					itemSubIcons+='&nbsp;<img src="images/log.png" title="' + $.t('Log') +'" onclick="ShowGeneralGraph(\'#devicescontent\',\'ShowDevices\',' + item.idx + ',\'' + escape(item.Name) + '\',\'' + escape(item.SensorUnit) +'\', \'' + item.SubType + '\');">';
				  }
				  else if (item.SubType == "Percentage") {
					itemSubIcons+='&nbsp;<img src="images/log.png" title="' + $.t('Log') +'" onclick="ShowPercentageLog(\'#devicescontent\',\'ShowDevices\',' + item.idx + ',\'' + escape(item.Name) + '\');">';
				  }
				  else if (item.SubType=="Sound Level") {
					itemSubIcons+='&nbsp;<img src="images/log.png" title="' + $.t('Log') +'" onclick="ShowGeneralGraph(\'#devicescontent\',\'ShowDevices\',' + item.idx + ',\'' + escape(item.Name) + '\',' + item.SwitchTypeVal +', \'' + item.SubType + '\');">';
				  }
				  else if (typeof item.Counter != 'undefined') {
					  if ((item.Type == "P1 Smart Meter")&&(item.SubType=="Energy")) {
						itemSubIcons+='&nbsp;<img src="images/log.png" title="' + $.t('Log') +'" onclick="ShowSmartLog(\'#devicescontent\',\'ShowDevices\',' + item.idx + ',\'' + escape(item.Name) + '\', ' + item.SwitchTypeVal + ');">';
					  }
					  else if ((item.Type == "YouLess Meter")&&(item.SwitchTypeVal==0 || item.SwitchTypeVal==4)) {
						itemSubIcons+='&nbsp;<img src="images/log.png" title="' + $.t('Log') +'" onclick="ShowCounterLogSpline(\'#devicescontent\',\'ShowDevices\',' + item.idx + ',\'' + escape(item.Name) + '\', ' + item.SwitchTypeVal + ');">';
					  }
					  else {
						itemSubIcons+='&nbsp;<img src="images/log.png" title="' + $.t('Log') +'" onclick="ShowCounterLog(\'#devicescontent\',\'ShowDevices\',' + item.idx + ',\'' + escape(item.Name) + '\', ' + item.SwitchTypeVal + ');">';
					  }
				  }
				  else if (typeof item.Direction != 'undefined') {
					itemSubIcons+='&nbsp;<img src="images/log.png" title="' + $.t('Log') +'" onclick="ShowWindLog(\'#devicescontent\',\'ShowDevices\',' + item.idx + ',\'' + escape(item.Name) + '\');">';
				  }
				  else if (typeof item.UVI != 'undefined') {
					itemSubIcons+='&nbsp;<img src="images/log.png" title="' + $.t('Log') +'" onclick="ShowUVLog(\'#devicescontent\',\'ShowDevices\',' + item.idx + ',\'' + escape(item.Name) + '\');">';
				  }
				  else if (typeof item.Rain != 'undefined') {
					itemSubIcons+='&nbsp;<img src="images/log.png" title="' + $.t('Log') +'" onclick="ShowRainLog(\'#devicescontent\',\'ShowDevices\',' + item.idx + ',\'' + escape(item.Name) + '\');">';
				  }
				  else if ((item.Type == "Energy")||(item.SubType == "kWh")) {
					itemSubIcons+='&nbsp;<img src="images/log.png" title="' + $.t('Log') +'" onclick="ShowCounterLogSpline(\'#devicescontent\',\'ShowDevices\',' + item.idx + ',\'' + escape(item.Name) + '\', ' + item.SwitchTypeVal + ');">';
				  }
				  else if (item.Type.indexOf("Current")==0) {
					itemSubIcons+='&nbsp;<img src="images/log.png" title="' + $.t('Log') +'" onclick="ShowCurrentLog(\'#devicescontent\',\'ShowDevices\',' + item.idx + ',\'' + escape(item.Name) + '\', ' + item.displaytype + ');">';
				  }
				  else if (item.Type == "Air Quality") {
					itemSubIcons+='&nbsp;<img src="images/log.png" title="' + $.t('Log') +'" onclick="ShowAirQualityLog(\'#devicescontent\',\'ShowDevices\',' + item.idx + ',\'' + escape(item.Name) + '\');">';
				  }
				  else if (item.Type == "Lux") {
					itemSubIcons+='&nbsp;<img src="images/log.png" title="' + $.t('Log') +'" onclick="ShowLuxLog(\'#devicescontent\',\'ShowDevices\',' + item.idx + ',\'' + escape(item.Name) + '\', ' + item.SwitchTypeVal + ');">';
				  }
				  else if (item.Type == "Usage") {
					itemSubIcons+='&nbsp;<img src="images/log.png" title="' + $.t('Log') +'" onclick="ShowUsageLog(\'#devicescontent\',\'ShowDevices\',' + item.idx + ',\'' + escape(item.Name) + '\', ' + item.SwitchTypeVal + ');">';
				  }
				  else if (item.SubType == "Solar Radiation") {
					itemSubIcons+='&nbsp;<img src="images/log.png" title="' + $.t('Log') +'" onclick="ShowGeneralGraph(\'#devicescontent\',\'ShowDevices\',' + item.idx + ',\'' + escape(item.Name) + '\',' + item.SwitchTypeVal +', \'Radiation\');">';
				  }
				  else if (item.SubType == "Visibility") {
					itemSubIcons+='&nbsp;<img src="images/log.png" title="' + $.t('Log') +'" onclick="ShowGeneralGraph(\'#devicescontent\',\'ShowDevices\',' + item.idx + ',\'' + escape(item.Name) + '\',' + item.SwitchTypeVal +', \'Visibility\');">';
				  }
				  else if (item.SubType == "Distance") {
					itemSubIcons+='&nbsp;<img src="images/log.png" title="' + $.t('Log') +'" onclick="ShowGeneralGraph(\'#devicescontent\',\'ShowDevices\',' + item.idx + ',\'' + escape(item.Name) + '\',' + item.SwitchTypeVal +', \'DistanceGeneral\');">';
				  }
				  else if (item.SubType == "Barometer") {
					itemSubIcons+='&nbsp;<img src="images/log.png" title="' + $.t('Log') +'" onclick="ShowBaroLog(\'#devicescontent\',\'ShowDevices\',' + item.idx + ',\'' + escape(item.Name) + '\');">';
				  }
				  else if ((item.SubType == "Text") || (item.SubType == "Alert")) {
					itemSubIcons+='&nbsp;<img src="images/log.png" title="' + $.t('Log') +'" onclick="ShowTextLog(\'#devicescontent\',\'ShowDevices\',' + item.idx + ',\'' + escape(item.Name) + '\');">';
				  }
				  else if ((item.SubType == "Soil Moisture")||(item.SubType == "Leaf Wetness")||(item.SubType == "Waterflow")) {
					itemSubIcons+='&nbsp;<img src="images/log.png" title="' + $.t('Log') +'" onclick="ShowGeneralGraph(\'#devicescontent\',\'ShowDevices\',' + item.idx + ',\'' + escape(item.Name) + '\',' + item.SwitchTypeVal +', \'' + item.SubType + '\');">';
				  }
				  else {
					itemSubIcons+='&nbsp;<img src="images/empty16.png">';
				  }
				  var ID = item.ID;
				  if (item.Type=="Lighting 1") {
								ID = String.fromCharCode(item.ID);
				  }
				  var BatteryLevel=item.BatteryLevel;
				  if (BatteryLevel=="255") {
								BatteryLevel="-";
				  }
				  else if (BatteryLevel=="0") {
								BatteryLevel=$.t("Low");
				  }
				  var addId = oTable.fnAddData([
							  itemChecker + "&nbsp;&nbsp;" + itemImage,
							  item.idx,
							  item.HardwareName,
							  ID,
							  item.Unit,
							  item.Name,
							  item.Type,
							  item.SubType,
							  item.Data,
							  item.SignalLevel,
							  BatteryLevel,
							  itemSubIcons,
							  item.LastUpdate
							], false);
				});
				mTable.fnDraw();
			  }
			 }
		  });
		}

		EnableDisableSubDevices = function(bEnabled)
		{
			var trow=$("#dialog-addlightdevicedev #lighttable #subdevice");
			if (bEnabled == true) {
				trow.show();
			}
			else {
				trow.hide();
			}
		}

		jQuery.fn.dataTableExt.oSort['numeric-battery-asc']  = function(a,b) {
			var x = a;
			var y = b;
			if (x=="-") x=101;
			if (x=="Low") x=1;
			if (y=="-") y=101;
			if (y=="Low") y=1;
			x = parseFloat( x );
			y = parseFloat( y );
			return ((x < y) ? -1 : ((x > y) ?  1 : 0));
		};
		 
		jQuery.fn.dataTableExt.oSort['numeric-battery-desc'] = function(a,b) {
			var x = a;
			var y = b;
			if (x=="-") x=101;
			if (x=="Low") x=1;
			if (y=="-") y=101;
			if (y=="Low") y=1;
			x = parseFloat( x );
			y = parseFloat( y );
			return ((x < y) ?  1 : ((x > y) ? -1 : 0));
		};
		init();

		function init()
		{
			//global var
			$.devIdx=0;
			$.LightsAndSwitches = [];
			$scope.MakeGlobalConfig();
					
			var dialog_adddevice_buttons = {};
			dialog_adddevice_buttons[$.t("Add Device")]=function() {
			  var bValid = true;
			  bValid = bValid && checkLength( $("#dialog-adddevice #devicename"), 2, 100 );
			  if ( bValid ) {
				  $( this ).dialog( "close" );
				  $.ajax({
					 url: "json.htm?type=setused&idx=" + $.devIdx + '&name=' + encodeURIComponent($("#dialog-adddevice #devicename").val()) + '&used=true',
					 async: false, 
					 dataType: 'json',
					 success: function(data) {
						ShowDevices();
					 }
				  });
			  }
			};
			dialog_adddevice_buttons[$.t("Cancel")]=function() {
			  $( this ).dialog( "close" );
			};

			$( "#dialog-adddevice" ).dialog({
				  autoOpen: false,
				  width: 'auto',
				  height: 'auto',
				  modal: true,
				  resizable: false,
				  title: $.t("Add Device"),
				  buttons: dialog_adddevice_buttons
			});

			var dialog_renamedevice_buttons = {};
			dialog_renamedevice_buttons[$.t("Rename Device")]=function() {
			  var bValid = true;
			  bValid = bValid && checkLength( $("#dialog-renamedevice #devicename"), 2, 100 );
			  if ( bValid ) {
				  $( this ).dialog( "close" );
				  var durl;
				  if (($.devType=="Group")||($.devType=="Scene")) {
					durl="json.htm?type=command&param=renamescene&idx=" + $.devIdx + '&name=' + encodeURIComponent($("#dialog-renamedevice #devicename").val());
				  }
				  else {
					durl="json.htm?type=command&param=renamedevice&idx=" + $.devIdx + '&name=' + encodeURIComponent($("#dialog-renamedevice #devicename").val());
				  }
				  $.ajax({
					 url: durl,
					 async: false, 
					 dataType: 'json',
					 success: function(data) {
						ShowDevices();
					 }
				  });
			  }
			};
			dialog_renamedevice_buttons[$.t("Cancel")]=function() {
			  $( this ).dialog( "close" );
			};

			$( "#dialog-renamedevice" ).dialog({
				  autoOpen: false,
				  width: 'auto',
				  height: 'auto',
				  modal: true,
				  resizable: false,
				  title: $.t("Rename Device"),
				  buttons: dialog_renamedevice_buttons
			});

			var dialog_addlightdevicedev_buttons = {};
			dialog_addlightdevicedev_buttons[$.t("Add Device")]=function() {
			  var bValid = true;
			  bValid = bValid && checkLength( $("#dialog-addlightdevicedev #devicename"), 2, 100 );
			  var bIsSubDevice=$("#dialog-addlightdevicedev #lighttable #how_2").is(":checked");
			  var MainDeviceIdx="";
			  if (bIsSubDevice)
			  {
					var MainDeviceIdx=$("#dialog-addlightdevicedev #combosubdevice option:selected").val();
					if (typeof MainDeviceIdx == 'undefined') {
						bootbox.alert($.t('No Main Device Selected!'));
						return;
					}
			  }
			  if ( bValid ) {
				  $( this ).dialog( "close" );
				  $.ajax({
					 url: "json.htm?type=setused&idx=" + $.devIdx + '&name=' + encodeURIComponent($("#dialog-addlightdevicedev #devicename").val()) + '&used=true&maindeviceidx=' + MainDeviceIdx,
					 async: false, 
					 dataType: 'json',
					 success: function(data) {
						ShowDevices();
					 }
				  });
			  }
			};
			dialog_addlightdevicedev_buttons[$.t("Cancel")]=function() {
				$( this ).dialog( "close" );
			};

				$( "#dialog-addlightdevicedev" ).dialog({
					  autoOpen: false,
					  width: 'auto',
					  height: 'auto',
					  modal: true,
					  resizable: false,
					  title: $.t("Add Light/Switch Device"),
					  buttons: dialog_addlightdevicedev_buttons
				});
						
			$("#dialog-addlightdevicedev #lighttable #how_1").click(function() {
				EnableDisableSubDevices(false);
			});
			$("#dialog-addlightdevicedev #lighttable #how_2").click(function() {
				EnableDisableSubDevices(true);
			});
			
			ShowDevices();
			
			$( "#dialog-adddevice" ).keydown(function (event) {
				if (event.keyCode == 13) {
					$(this).siblings('.ui-dialog-buttonpane').find('button:eq(0)').trigger("click");
					return false;
				}
			});
			$( "#dialog-addlightdevicedev" ).keydown(function (event) {
				if (event.keyCode == 13) {
					$(this).siblings('.ui-dialog-buttonpane').find('button:eq(0)').trigger("click");
					return false;
				}
			});
	
		};
	} ]);
});
