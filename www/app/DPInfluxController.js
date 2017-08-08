define(['app'], function (app) {
	app.controller('DPInfluxController', [ '$scope', '$rootScope', '$location', '$http', '$interval', function($scope,$rootScope,$location,$http,$interval) {

		LoadConfiguration = function() 
		{
			$.ajax({
				url: "json.htm?type=command&param=getinfluxlinkconfig",
				async: false, 
				dataType: 'json',
				success: function(data) {
					if (typeof data != 'undefined') {
						if (data.status=="OK") {
							$('#influxremote #tcpaddress').val(data.InfluxIP);
							$('#influxremote #tcpport').val(data.InfluxPort);
							$('#influxremote #database').val(data.InfluxDatabase);
							$('#influxremote #influxlinkenabled').prop('checked', false);
							if (data.InfluxActive) {
								$('#influxremote #influxlinkenabled').prop('checked', true);
							}
							$('#influxremote #debugenabled').prop('checked', false);
							if (data.InfluxDebug) {
								$('#influxremote #debugenabled').prop('checked', true);
							}
						}
					}
				},
				error: function(){
					ShowNotify($.t('Problem retrieving InfluxDB link settings!'), 2500, true);
				}
			});
		}

		SaveConfiguration = function()
		{
			var linkactive = 0;	
			if ($('#influxremote #influxlinkenabled').is(":checked"))
			{
				linkactive = 1;
			}
			var remoteurl = $('#influxremote #tcpaddress').val();
			var port = $('#influxremote #tcpport').val();
			if (port.Length==0)
				port="8086";
			var database = $('#influxremote #database').val();
			var debugenabled = 0;
			if ($('#influxremote #debugenabled').is(":checked"))
			{
				debugenabled = 1;
			}
			$.ajax({
				 url:
					"json.htm?type=command&param=saveinfluxlinkconfig" +
					"&linkactive=" + linkactive +
					"&remote=" + encodeURIComponent(remoteurl) +
					"&port=" + port +
					"&database=" + encodeURIComponent(database) +
					"&debugenabled=" + debugenabled,
				 async: false, 
				 dataType: 'json',
				 success: function(data) {
					bootbox.alert($.t('InfluxDB link settings saved'));
				 },
				 error: function(){
						ShowNotify($.t('Problem saving InfluxDB link settings!'), 2500, true);
				 }     
			});
		}

		DeleteLink = function(idx)
		{
			bootbox.confirm($.t("Are you sure you want to remove this link?"), function(result) {
				if (result==true) {
					$.ajax({
						url: "json.htm?type=command&param=deleteinfluxlink&idx=" + idx,
						async: false, 
						dataType: 'json',
						success: function(data) {
							//bootbox.alert($.t('Link deleted!'));
							RefreshLinkTable();
						}
					});
				}
			});
		}
			

		AddLink = function(type)
		{
			var idx = $.linkIdx;
			if (type == "a") {idx="0"};
			var deviceid = $("#linkparamstable #devicename option:selected").val();
			var onoffdeviceid = $("#linkparamstable #onoffdevicename option:selected").val();
			var valuetosend = $('#linkparamstable #combosendvalue option:selected').val();
			var scenevaluetosend = $('#linkparamstable #sendvaluescene option:selected').val();
			var rebootvaluetosend = $('#linkparamstable #sendvaluereboot option:selected').val();
			var targettype = $("#linkparamstable #combotargettype option:selected").val();
			var targetvariable = $('#linkparamstable #targetvariable').val();
			var targetdeviceid = $('#linkparamstable #targetdeviceid').val();
			var targetproperty = $('#linkparamstable #targetproperty').val();
			var linkactive = 0;	
			if ($('#linkparamstable #linkenabled').is(":checked"))
			{
				linkactive = 1;
			}
			var includeunit = 0;
			if ($('#linkparamstable #includeunit').is(":checked"))
			{
				includeunit = 1;
			}
			
			var url =
				"json.htm?type=command&param=saveinfluxlink" +
				"&idx=" + idx +
				"&deviceid=" + deviceid +
				"&valuetosend=" + valuetosend +
				"&targettype=" + targettype +
				"&linkactive=" + linkactive;
			$.ajax({
				 url: url, 
				 async: false, 
				 dataType: 'json',
				 success: function(data) {
					bootbox.alert($.t('InfluxDB link saved'));
					RefreshLinkTable();
				 },
				 error: function(){
						ShowNotify($.t('Problem saving InfluxDB link!'), 2500, true);
				 }     
			});
		}

		ValueSelectorUpdate = function()
		{
			var deviceid = $("#linkparamstable #devicename option:selected").val();	
			var select = document.getElementById("combosendvalue");
			select.options.length = 0;
			$.ajax({
			 url: "json.htm?type=command&param=getdevicevalueoptions&idx="+deviceid,
			 async: false, 
			 dataType: 'json',
			 success: function(data) {   
			  if (typeof data.result != 'undefined') {
				$.each(data.result, function(i,item){
					var option = document.createElement("option");
					option.text = $.t(item.Wording);
					option.value = item.Value;
					select.appendChild(option);
				});
			  }
			 }
		  });
		}
			
		RefreshLinkTable = function()
		{
		  $('#modal').show();

			$('#linkparamstable #linkupdate').attr("class", "btnstyle3-dis");
			$('#linkparamstable #linkdelete').attr("class", "btnstyle3-dis");

		  var oTable = $('#iflinktable').dataTable();
		  oTable.fnClearTable();
		  $.ajax({
			 url: "json.htm?type=command&param=getinfluxlinks",
			 async: false, 
			 dataType: 'json',
			 success: function(data) {   
			  if (typeof data.result != 'undefined') {
				$.each(data.result, function(i,item){
					var enabled = $.t('No');
					if (item.Enabled == 1)
						enabled = $.t('Yes');
					var DelimitedValue = "";
					if (item.Delimitedvalue == 0) {
						DelimitedValue = $.t('Status');
					}
					else {
						DelimitedValue = $.t(GetDeviceValueOptionWording(item.DeviceID,item.Delimitedvalue));
					}
					var TargetType = $.t('On Value Change');
					if (item.TargetType==1) 
						TargetType = $.t('Direct');

					var addId = oTable.fnAddData( {
						"DT_RowId": item.idx,
						"DeviceID": item.DeviceID,
						"TargetType": item.TargetType,
						"Enabled": item.Enabled,
						"Delimitedvalue": item.Delimitedvalue,
						"0": item.DeviceID,
						"1": item.Name,
						"2": DelimitedValue,
						"3": TargetType,
						"4": enabled
					} );
				});
			  }
			 }
		  });
		  $('#modal').hide();
		}

		GetDeviceValueOptionWording = function(idx,pos)
		{
			var wording = "";
			$.ajax({
			 url: "json.htm?type=command&param=getdevicevalueoptionwording&idx="+idx+"&pos="+pos,
			 async: false, 
			 dataType: 'json',
			 success: function(data) {   
			  if (typeof data.wording != 'undefined') {
					wording = data.wording;
			  }
			  }
		   });
		   return wording;
		}

		ShowLinks = function()
		{
			$('#influxmain').i18n();
			var oTable = $('#iflinktable').dataTable( {
			  "sDom": '<"H"lfrC>t<"F"ip>',
			  "oTableTools": {
				"sRowSelect": "single"
			  },
			  "fnDrawCallback": function (oSettings) {
				var nTrs = this.fnGetNodes();
				$(nTrs).click(
					function(){
						$(nTrs).removeClass('row_selected');
						$(this).addClass('row_selected');
						$('#linkparamstable #linkupdate').attr("class", "btnstyle3");
						$('#linkparamstable #linkdelete').attr("class", "btnstyle3");
						var anSelected = fnGetSelected( oTable );
						if ( anSelected.length !== 0 ) {
							var data = oTable.fnGetData( anSelected[0] );
							var idx= data["DT_RowId"];
							$.linkIdx=idx;	
							$("#linkparamstable #linkupdate").attr("href", "javascript:AddLink('u')");
							$("#linkparamstable #linkdelete").attr("href", "javascript:DeleteLink(" + idx + ")");
							$("#linkparamstable #combotargettype").val(data["TargetType"]);
							$("#linkparamstable #devicename").val(data["DeviceID"]); 
							ValueSelectorUpdate();
							$("#linkparamstable #combosendvalue").val(data["Delimitedvalue"]);
							if (data["Enabled"] == 1) {
								$('#linkparamstable #linkenabled').prop('checked', true);
							}
							else {
								$('#linkparamstable #linkenabled').prop('checked', false);
							}
						}
				});
			  },    
			  "aaSorting": [[ 0, "desc" ]],
			  "bSortClasses": false,
			  "bProcessing": true,
			  "bStateSave": true,
			  "bJQueryUI": true,
              "aLengthMenu": [[25, 50, 100, -1], [25, 50, 100, "All"]],
			  "iDisplayLength" : 25,
			  'bLengthChange': false,
			  "sPaginationType": "full_numbers",
			  language: $.DataTableLanguage
			} );
			RefreshLinkTable();
		}

		init();

		function init()
		{
			$scope.MakeGlobalConfig();
			//Get devices
			$("#influxmain #devicename").html("");
			$.ajax({
			 url: "json.htm?type=command&param=devices_list",
			 async: false, 
			 dataType: 'json',
			 success: function(data) {
				if (typeof data.result != 'undefined') {
					$.each(data.result, function(i,item) {
						var option = $('<option />');
						option.attr('value', item.value).text(item.name);
						$("#influxmain #devicename").append(option);
					});
				}
				ValueSelectorUpdate();
			 }
			});
			//global var
			$.linkIdx=0;
			LoadConfiguration();
			ShowLinks();
		};
	} ]);
});