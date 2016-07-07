define(['app'], function (app) {
	app.controller('DPGooglePubSubController', [ '$scope', '$rootScope', '$location', '$http', '$interval', function($scope,$rootScope,$location,$http,$interval) {

		MethodUpdate = function()
		{
			var targetType = $("#googlepubsubremote #combomethod option:selected").val();
			switch(targetType) {
			case "0":
				$("#googlepubsubremote #lblremotedata").hide();
				$("#googlepubsubremote #data").hide();
				break;
			case "1":
				$("#googlepubsubremote #lblremotedata").show();
				$("#googlepubsubremote #data").show();
				break;
			case "2":
				$("#googlepubsubremote #lblremotedata").show();
				$("#googlepubsubremote #data").show();
				break;
			default:
				break;
			}
		}
		
		TargetTypeUpdate = function()
		{
			var targetType = $("#linkgooglepubsubparamstable #combotargettype option:selected").val();
			switch(targetType) {
			case "0":
				$("#linkgooglepubsubparamstable #targetdevicename").show();
				$("#linkgooglepubsubparamstable #targetonoffdevicename").hide();
				$("#linkgooglepubsubparamstable #variabletosend").show();
				$("#linkgooglepubsubparamstable #scenevariabletosend").hide();
				$("#linkgooglepubsubparamstable #rebootvariabletosend").hide();
				$("#linkgooglepubsubparamstable #targetvariablerow").show();
				$("#linkgooglepubsubparamstable #targetdeviceidrow").hide();
				$("#linkgooglepubsubparamstable #targetpropertyrow").hide();
				$("#linkgooglepubsubparamstable #targetincludeunit").show();
				break;
			case "1":
				$("#linkgooglepubsubparamstable #targetdevicename").show();
				$("#linkgooglepubsubparamstable #targetonoffdevicename").hide();
				$("#linkgooglepubsubparamstable #variabletosend").show();
				$("#linkgooglepubsubparamstable #scenevariabletosend").hide();
				$("#linkgooglepubsubparamstable #rebootvariabletosend").hide();
				$("#linkgooglepubsubparamstable #targetvariablerow").hide();
				$("#linkgooglepubsubparamstable #targetdeviceidrow").show();
				$("#linkgooglepubsubparamstable #targetpropertyrow").show();
				$("#linkgooglepubsubparamstable #targetincludeunit").show();
				break;
			case "2":
				$("#linkgooglepubsubparamstable #targetdevicename").hide();
				$("#linkgooglepubsubparamstable #targetonoffdevicename").show();
				$("#linkgooglepubsubparamstable #variabletosend").hide();
				$("#linkgooglepubsubparamstable #scenevariabletosend").show();
				$("#linkgooglepubsubparamstable #rebootvariabletosend").hide();
				$("#linkgooglepubsubparamstable #targetvariablerow").hide();
				$("#linkgooglepubsubparamstable #targetdeviceidrow").show();
				$("#linkgooglepubsubparamstable #targetpropertyrow").hide();
				$("#linkgooglepubsubparamstable #targetincludeunit").hide();
				break;
			case "3":
				$("#linkgooglepubsubparamstable #targetdevicename").hide();
				$("#linkgooglepubsubparamstable #targetonoffdevicename").show();
				$("#linkgooglepubsubparamstable #variabletosend").hide();
				$("#linkgooglepubsubparamstable #scenevariabletosend").hide();
				$("#linkgooglepubsubparamstable #rebootvariabletosend").show();
				$("#linkgooglepubsubparamstable #targetvariablerow").hide();
				$("#linkgooglepubsubparamstable #targetdeviceidrow").hide();
				$("#linkgooglepubsubparamstable #targetpropertyrow").hide();
				$("#linkgooglepubsubparamstable #targetincludeunit").hide();
				break;
			default:
				break;
			}
			
		}
		SaveConfiguration = function()
		{
			var httpdata = $('#googlepubsubremote #data').val();
			var linkactive = 0;	
			if ($('#googlepubsubremote #googlepubsublinkenabled').is(":checked"))
			{
				linkactive = 1;
			}
			var debugenabled = 0;
			if ($('#googlepubsubremote #debugenabled').is(":checked"))
			{
				debugenabled = 1;
			}
			
			$.ajax({
				 url: "json.htm?type=command&param=savegooglepubsublinkconfig" +
					"&data=" + encodeURIComponent(httpdata) + "&linkactive=" + linkactive + "&debugenabled=" + debugenabled,
				 async: false, 
				 dataType: 'json',
				 success: function(data) {
					bootbox.alert($.t('Google PubSub link settings saved'));
				 },
				 error: function(){
						ShowNotify($.t('Problem saving Google PubSub link settings!'), 2500, true);
				 }     
			});
		}

		DeleteLink = function(idx)
		{
			bootbox.confirm($.t("Are you sure you want to remove this link?"), function(result) {
				if (result==true) {
					$.ajax({
						url: "json.htm?type=command&param=deletegooglepubsublink&idx=" + idx,
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
			var deviceid = $("#linkgooglepubsubparamstable #devicename option:selected").val();
			var onoffdeviceid = $("#linkgooglepubsubparamstable #onoffdevicename option:selected").val();
			var valuetosend = $('#linkgooglepubsubparamstable #combosendvalue option:selected').val();
			var scenevaluetosend = $('#linkgooglepubsubparamstable #sendvaluescene option:selected').val();
			var rebootvaluetosend = $('#linkgooglepubsubparamstable #sendvaluereboot option:selected').val();
			var targettype = $("#linkgooglepubsubparamstable #combotargettype option:selected").val();
			var targetvariable = $('#linkgooglepubsubparamstable #targetvariable').val();
			var targetdeviceid = $('#linkgooglepubsubparamstable #targetdeviceid').val();
			var targetproperty = $('#linkgooglepubsubparamstable #targetproperty').val();
			var linkactive = 0;	
			if ($('#linkgooglepubsubparamstable #linkenabled').is(":checked"))
			{
				linkactive = 1;
			}
			var includeunit = 0;
			if ($('#linkgooglepubsubparamstable #includeunit').is(":checked"))
			{
				includeunit = 1;
			}
			
			var url = "";
			if (targettype == 0) {
				url = "json.htm?type=command&param=savegooglepubsublink" + "&idx=" + idx + "&deviceid=" + deviceid + "&valuetosend=" + valuetosend + "&targettype=" + targettype + "&targetvariable=" + targetvariable + "&includeunit=" + includeunit + "&linkactive=" + linkactive;
			}
			else if (targettype == 1) {
				url = "json.htm?type=command&param=savegooglepubsublink" + "&idx=" + idx + "&deviceid=" + deviceid + "&valuetosend=" + valuetosend + "&targettype=" + targettype + "&targetdeviceid=" + targetdeviceid + "&targetproperty=" + targetproperty + "&includeunit=" + includeunit + "&linkactive=" + linkactive;	
			}
			else if (targettype == 2) {
				url = "json.htm?type=command&param=savegooglepubsublink" + "&idx=" + idx + "&deviceid=" + onoffdeviceid + "&valuetosend=" + scenevaluetosend + "&targettype=" + targettype + "&targetdeviceid=" + targetdeviceid + "&linkactive=" + linkactive;	
			}
			else if (targettype == 3) {
				url = "json.htm?type=command&param=savegooglepubsublink" + "&idx=" + idx + "&deviceid=" + onoffdeviceid + "&valuetosend=" + rebootvaluetosend + "&targettype=" + targettype + "&targetdeviceid=0&linkactive=" + linkactive;	
			}
			
			$.ajax({
				 url: url, 
				 async: false, 
				 dataType: 'json',
				 success: function(data) {
					bootbox.alert($.t('Google PubSub link saved'));
					RefreshLinkTable();
				 },
				 error: function(){
						ShowNotify($.t('Problem saving Google PubSub link!'), 2500, true);
				 }     
			});
		}

		GetConfig = function() 
		{
			$.ajax({
				url: "json.htm?type=command&param=getgooglepubsublinkconfig",
				async: false, 
				dataType: 'json',
				success: function(data) {
					if (typeof data != 'undefined') {
						if (data.status=="OK") {
							$('#googlepubsubremote #data').val(data.GooglePubSubData);
							$('#googlepubsubremote #googlepubsublinkenabled').prop('checked', false);
							if (data.GooglePubSubActive) {
								$('#googlepubsubremote #googlepubsublinkenabled').prop('checked', true);
							}
							$('#googlepubsubremote #debugenabled').prop('checked', false);
							if (data.GooglePubSubDebug) {
								$('#googlepubsubremote #debugenabled').prop('checked', true);
							}
							MethodUpdate();
						}
					}
				},
				error: function(){
					ShowNotify($.t('Problem retrieving Google PubSub link settings!'), 2500, true);
				}
			});
		}
			

		ValueSelectorUpdate = function()
		{
			var deviceid = $("#linkgooglepubsubparamstable #devicename option:selected").val();	
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

			$('#linkgooglepubsubparamstable #linkupdate').attr("class", "btnstyle3-dis");
			$('#linkgooglepubsubparamstable #linkdelete').attr("class", "btnstyle3-dis");

		  var oTable = $('#linkgooglepubsubtable').dataTable();
		  oTable.fnClearTable();
		  $.ajax({
			 url: "json.htm?type=command&param=getgooglepubsublinks",
			 async: false, 
			 dataType: 'json',
			 success: function(data) {   
			  if (typeof data.result != 'undefined') {
				$.each(data.result, function(i,item){
					var enabled = $.t('No');
					var includeUnit = $.t('No');
					if (item.Enabled == 1)
						enabled = $.t('Yes');
					if (item.IncludeUnit == 1)
						includeUnit = $.t('Yes');
					var TargetType = $.t('Global variable');
					if (item.TargetType==1) 
						TargetType = $.t('Virtual device');
					if (item.TargetType==2) {
						TargetType = $.t('Scene');	
						includeUnit = "-";
					}
					if (item.TargetType==3) {
						TargetType = $.t('Reboot');	
						includeUnit = "-";
					}
					if (item.TargetDevice == 0)
						item.TargetDevice="";
					var DelimitedValue = "";
					
					if (item.TargetType==2) {
						if (item.Delimitedvalue == 0) {
							DelimitedValue = $.t('Off');
						}
						else if (item.Delimitedvalue == 1) {
							DelimitedValue = $.t('On');
						}
					}
					else if (item.TargetType==3) {
						if (item.Delimitedvalue == 0) {
							DelimitedValue = $.t('Off');
						}
						else if (item.Delimitedvalue == 1) {
							DelimitedValue = $.t('On');
						}
					}
					else {
						if (item.Delimitedvalue == 0) {
							DelimitedValue = $.t('Status');
						}
						else {
							DelimitedValue = $.t(GetDeviceValueOptionWording(item.DeviceID,item.Delimitedvalue));
						}
					}
					var addId = oTable.fnAddData( {
						"DT_RowId": item.idx,
						"TargetType": item.TargetType,
						"DeviceID": item.DeviceID,
						"Enabled": item.Enabled,
						"IncludeUnit": item.IncludeUnit,
						"Delimitedvalue": item.Delimitedvalue,
						"0": item.Name,
						"1": DelimitedValue,
						"2": TargetType,
						"3": item.TargetVariable,
						"4": item.TargetDevice,
						"5": item.TargetProperty,
						"6": includeUnit,
						"7": enabled
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
			$('#googlepubsubmain').i18n();
			var oTable = $('#linkgooglepubsubtable').dataTable( {
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
						$('#linkgooglepubsubparamstable #linkupdate').attr("class", "btnstyle3");
						$('#linkgooglepubsubparamstable #linkdelete').attr("class", "btnstyle3");
						var anSelected = fnGetSelected( oTable );
						if ( anSelected.length !== 0 ) {
							var data = oTable.fnGetData( anSelected[0] );
							var idx= data["DT_RowId"];
							$.linkIdx=idx;	
							$("#linkgooglepubsubparamstable #linkupdate").attr("href", "javascript:AddLink('u')");
							$("#linkgooglepubsubparamstable #linkdelete").attr("href", "javascript:DeleteLink(" + idx + ")");
							$("#linkgooglepubsubparamstable #combotargettype").val(data["TargetType"]);
							TargetTypeUpdate();
							if (data["TargetType"]==2) {
								$("#linkgooglepubsubparamstable #sendvaluescene").val(data["Delimitedvalue"]);
								$("#linkgooglepubsubparamstable #onoffdevicename").val(data["DeviceID"]); 
							}
							if (data["TargetType"]==3) {
								$("#linkgooglepubsubparamstable #sendvaluereboot").val(data["Delimitedvalue"]);
								$("#linkgooglepubsubparamstable #onoffdevicename").val(data["DeviceID"]); 
							}
							else {
								$("#linkgooglepubsubparamstable #devicename").val(data["DeviceID"]); 
								ValueSelectorUpdate();
								$("#linkgooglepubsubparamstable #combosendvalue").val(data["Delimitedvalue"]);
							}
							$("#linkgooglepubsubparamstable #targetvariable").val(data["3"]);
							$("#linkgooglepubsubparamstable #targetdeviceid").val(data["4"]);
							$("#linkgooglepubsubparamstable #targetproperty").val(data["5"]);
							if (data["Enabled"] == 1) {
								$('#linkgooglepubsubparamstable #linkenabled').prop('checked', true);
							}
							else {
								$('#linkgooglepubsubparamstable #linkenabled').prop('checked', false);
							}
							if (data["IncludeUnit"] == 1) {
								$('#linkgooglepubsubparamstable #includeunit').prop('checked', true);
							}
							else {
								$('#linkgooglepubsubparamstable #includeunit').prop('checked', false);
							}
						}
				});
			  },    
			  "aaSorting": [[ 0, "desc" ]],
			  "bSortClasses": false,
			  "bProcessing": true,
			  "bStateSave": true,
			  "bJQueryUI": true,
			  "iDisplayLength" : 10,
			  'bLengthChange': false,
			  "sPaginationType": "full_numbers",
			  language: $.DataTableLanguage
			} );
			$("#googlepubsubcontent #linkgooglepubsubtable #combotype").change(function() { 
				UpdateLinks();
			});
			
			RefreshLinkTable();
		}

		init();

		function init()
		{
			$scope.MakeGlobalConfig();
			//Get devices
			$("#googlepubsubmain #devicename").html("");
			$.ajax({
			 url: "json.htm?type=command&param=devices_list",
			 async: false, 
			 dataType: 'json',
			 success: function(data) {
				if (typeof data.result != 'undefined') {
					$.each(data.result, function(i,item) {
						var option = $('<option />');
						option.attr('value', item.value).text(item.name);
						$("#googlepubsubmain #devicename").append(option);
					});
				}
				ValueSelectorUpdate();
			 }
			});
			
			//Get devices On/Off
			$("#googlepubsubmain #onoffdevicename").html("");
			$.ajax({
			 url: "json.htm?type=command&param=devices_list_onoff",
			 async: false, 
			 dataType: 'json',
			 success: function(data) {
				if (typeof data.result != 'undefined') {
					$.each(data.result, function(i,item) {
						var option = $('<option />');
						option.attr('value', item.value).text(item.name);
						$("#googlepubsubmain #onoffdevicename").append(option);
					});
				}
			 }
			});
			
			//global var
			$.linkIdx=0;
			GetConfig();
			ShowLinks();
		};
	} ]);
});