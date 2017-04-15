define(['app'], function (app) {
	app.controller('DPFibaroController', [ '$scope', '$rootScope', '$location', '$http', '$interval', function($scope,$rootScope,$location,$http,$interval) {

		TargetTypeUpdate = function()
		{
			var targetType = $("#linkparamstable #combotargettype option:selected").val();
			switch(targetType) {
			case "0":
				$("#linkparamstable #targetdevicename").show();
				$("#linkparamstable #targetonoffdevicename").hide();
				$("#linkparamstable #variabletosend").show();
				$("#linkparamstable #scenevariabletosend").hide();
				$("#linkparamstable #rebootvariabletosend").hide();
				$("#linkparamstable #targetvariablerow").show();
				$("#linkparamstable #targetdeviceidrow").hide();
				$("#linkparamstable #targetpropertyrow").hide();
				$("#linkparamstable #targetincludeunit").show();
				break;
			case "1":
				$("#linkparamstable #targetdevicename").show();
				$("#linkparamstable #targetonoffdevicename").hide();
				$("#linkparamstable #variabletosend").show();
				$("#linkparamstable #scenevariabletosend").hide();
				$("#linkparamstable #rebootvariabletosend").hide();
				$("#linkparamstable #targetvariablerow").hide();
				$("#linkparamstable #targetdeviceidrow").show();
				$("#linkparamstable #targetpropertyrow").show();
				$("#linkparamstable #targetincludeunit").show();
				break;
			case "2":
				$("#linkparamstable #targetdevicename").hide();
				$("#linkparamstable #targetonoffdevicename").show();
				$("#linkparamstable #variabletosend").hide();
				$("#linkparamstable #scenevariabletosend").show();
				$("#linkparamstable #rebootvariabletosend").hide();
				$("#linkparamstable #targetvariablerow").hide();
				$("#linkparamstable #targetdeviceidrow").show();
				$("#linkparamstable #targetpropertyrow").hide();
				$("#linkparamstable #targetincludeunit").hide();
				break;
			case "3":
				$("#linkparamstable #targetdevicename").hide();
				$("#linkparamstable #targetonoffdevicename").show();
				$("#linkparamstable #variabletosend").hide();
				$("#linkparamstable #scenevariabletosend").hide();
				$("#linkparamstable #rebootvariabletosend").show();
				$("#linkparamstable #targetvariablerow").hide();
				$("#linkparamstable #targetdeviceidrow").hide();
				$("#linkparamstable #targetpropertyrow").hide();
				$("#linkparamstable #targetincludeunit").hide();
				break;
			default:
				break;
			}
			
		}
		SaveConfiguration = function()
		{
			var remoteurl = $('#fibaroremote #tcpaddress').val();
			var cleanurl = remoteurl;
			if (remoteurl.indexOf('://')>0) {
				cleanurl = remoteurl.substr(remoteurl.indexOf('://')+3);
			}
			var username = $('#fibaroremote #username').val();
			var password = $('#fibaroremote #password').val();
			var linkactive = 0;	
			if ($('#fibaroremote #fibarolinkenabled').is(":checked"))
			{
				linkactive = 1;
			}
			var isversion4 = 0;	
			if ($('#fibaroremote #fibaroisversion4').is(":checked"))
			{
				isversion4 = 1;
			}
			var debugenabled = 0;
			if ($('#fibaroremote #debugenabled').is(":checked"))
			{
				debugenabled = 1;
			}
			$.ajax({
				 url: "json.htm?type=command&param=savefibarolinkconfig" +
					"&remote=" + encodeURIComponent(cleanurl) + "&username=" + encodeURIComponent(username) + "&password=" + encodeURIComponent(password) + "&linkactive=" + linkactive + "&isversion4=" + isversion4 + "&debugenabled=" + debugenabled,
				 async: false, 
				 dataType: 'json',
				 success: function(data) {
					bootbox.alert($.t('Fibaro link settings saved'));
				 },
				 error: function(){
						ShowNotify($.t('Problem saving Fibaro link settings!'), 2500, true);
				 }     
			});
		}

		DeleteLink = function(idx)
		{
			bootbox.confirm($.t("Are you sure you want to remove this link?"), function(result) {
				if (result==true) {
					$.ajax({
						url: "json.htm?type=command&param=deletefibarolink&idx=" + idx,
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
			
			var url = "";
			if (targettype == 0) {
				url = "json.htm?type=command&param=savefibarolink" + "&idx=" + idx + "&deviceid=" + deviceid + "&valuetosend=" + valuetosend + "&targettype=" + targettype + "&targetvariable=" + targetvariable + "&includeunit=" + includeunit + "&linkactive=" + linkactive;
			}
			else if (targettype == 1) {
				url = "json.htm?type=command&param=savefibarolink" + "&idx=" + idx + "&deviceid=" + deviceid + "&valuetosend=" + valuetosend + "&targettype=" + targettype + "&targetdeviceid=" + targetdeviceid + "&targetproperty=" + targetproperty + "&includeunit=" + includeunit + "&linkactive=" + linkactive;	
			}
			else if (targettype == 2) {
				url = "json.htm?type=command&param=savefibarolink" + "&idx=" + idx + "&deviceid=" + onoffdeviceid + "&valuetosend=" + scenevaluetosend + "&targettype=" + targettype + "&targetdeviceid=" + targetdeviceid + "&linkactive=" + linkactive;	
			}
			else if (targettype == 3) {
				url = "json.htm?type=command&param=savefibarolink" + "&idx=" + idx + "&deviceid=" + onoffdeviceid + "&valuetosend=" + rebootvaluetosend + "&targettype=" + targettype + "&targetdeviceid=0&linkactive=" + linkactive;	
			}
			
			$.ajax({
				 url: url, 
				 async: false, 
				 dataType: 'json',
				 success: function(data) {
					bootbox.alert($.t('Fibaro link saved'));
					RefreshLinkTable();
				 },
				 error: function(){
						ShowNotify($.t('Problem saving Fibaro link!'), 2500, true);
				 }     
			});
		}

		GetConfig = function() 
		{
			$.ajax({
				url: "json.htm?type=command&param=getfibarolinkconfig",
				async: false, 
				dataType: 'json',
				success: function(data) {
					if (typeof data != 'undefined') {
						if (data.status=="OK") {
							$('#fibaroremote #tcpaddress').val(data.FibaroIP);
							$('#fibaroremote #username').val(data.FibaroUsername);
							$('#fibaroremote #password').val(data.FibaroPassword);
							$('#fibaroremote #fibarolinkenabled').prop('checked', false);
							if (data.FibaroActive) {
								$('#fibaroremote #fibarolinkenabled').prop('checked', true);
							}
							$('#fibaroremote #fibaroisversion4').prop('checked', false);
							if (data.FibaroVersion4) {
								$('#fibaroremote #fibaroisversion4').prop('checked', true);
							}
							$('#fibaroremote #debugenabled').prop('checked', false);
							if (data.FibaroDebug) {
								$('#fibaroremote #debugenabled').prop('checked', true);
							}
						}
					}
				},
				error: function(){
					ShowNotify($.t('Problem retrieving Fibaro link settings!'), 2500, true);
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

		  var oTable = $('#linktable').dataTable();
		  oTable.fnClearTable();
		  $.ajax({
			 url: "json.htm?type=command&param=getfibarolinks",
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
			$('#fibaromain').i18n();
			var oTable = $('#linktable').dataTable( {
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
							TargetTypeUpdate();
							if (data["TargetType"]==2) {
								$("#linkparamstable #sendvaluescene").val(data["Delimitedvalue"]);
								$("#linkparamstable #onoffdevicename").val(data["DeviceID"]); 
							}
							if (data["TargetType"]==3) {
								$("#linkparamstable #sendvaluereboot").val(data["Delimitedvalue"]);
								$("#linkparamstable #onoffdevicename").val(data["DeviceID"]); 
							}
							else {
								$("#linkparamstable #devicename").val(data["DeviceID"]); 
								ValueSelectorUpdate();
								$("#linkparamstable #combosendvalue").val(data["Delimitedvalue"]);
							}
							$("#linkparamstable #targetvariable").val(data["3"]);
							$("#linkparamstable #targetdeviceid").val(data["4"]);
							$("#linkparamstable #targetproperty").val(data["5"]);
							if (data["Enabled"] == 1) {
								$('#linkparamstable #linkenabled').prop('checked', true);
							}
							else {
								$('#linkparamstable #linkenabled').prop('checked', false);
							}
							if (data["IncludeUnit"] == 1) {
								$('#linkparamstable #includeunit').prop('checked', true);
							}
							else {
								$('#linkparamstable #includeunit').prop('checked', false);
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
			$("#fibarocontent #linktable #combotype").change(function() { 
				UpdateLinks();
			});
			
			RefreshLinkTable();
		}

		init();

		function init()
		{
			$scope.MakeGlobalConfig();
			//Get devices
			$("#fibaromain #devicename").html("");
			$.ajax({
			 url: "json.htm?type=command&param=devices_list",
			 async: false, 
			 dataType: 'json',
			 success: function(data) {
				if (typeof data.result != 'undefined') {
					$.each(data.result, function(i,item) {
						var option = $('<option />');
						option.attr('value', item.value).text(item.name);
						$("#fibaromain #devicename").append(option);
					});
				}
				ValueSelectorUpdate();
			 }
			});
			
			//Get devices On/Off
			$("#fibaromain #onoffdevicename").html("");
			$.ajax({
			 url: "json.htm?type=command&param=devices_list_onoff",
			 async: false, 
			 dataType: 'json',
			 success: function(data) {
				if (typeof data.result != 'undefined') {
					$.each(data.result, function(i,item) {
						var option = $('<option />');
						option.attr('value', item.value).text(item.name);
						$("#fibaromain #onoffdevicename").append(option);
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