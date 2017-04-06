define(['app'], function (app) {
	app.controller('DPHttpController', [ '$scope', '$rootScope', '$location', '$http', '$interval', function($scope,$rootScope,$location,$http,$interval) {

		MethodAuthenticationUpdate = function()
		{
			var targetType = $("#httpremote #comboauth option:selected").val();
			switch(targetType) {
			case "0":
				$("#httpremote #lblauthbasiclogin").hide();
				$("#httpremote #authBasicUser").hide();
				$("#httpremote #lblauthbasicpassword").hide();
				$("#httpremote #authBasicPassword").hide();
				break;
			case "1":
				$("#httpremote #lblauthbasiclogin").show();
				$("#httpremote #authBasicUser").show();
				$("#httpremote #lblauthbasicpassword").show();
				$("#httpremote #authBasicPassword").show();
				break;
			default:
				break;
			}
		}
		
		MethodUpdate = function()
		{
			var targetType = $("#httpremote #combomethod option:selected").val();
			switch(targetType) {
			case "0":
				$("#httpremote #lblremotedata").hide();
				$("#httpremote #lblremoteheaders").hide();
				$("#httpremote #data").hide();
				$("#httpremote #headers").hide();
				break;
			case "1":
				$("#httpremote #lblremotedata").show();
				$("#httpremote #data").show();
				$("#httpremote #lblremoteheaders").show();
				$("#httpremote #headers").show();
				break;
			case "2":
				$("#httpremote #lblremotedata").show();
				$("#httpremote #data").show();
				$("#httpremote #lblremoteheaders").show();
				$("#httpremote #headers").show();
				break;
			default:
				break;
			}
		}
		
		TargetTypeUpdate = function()
		{
			var targetType = $("#linkhttpparamstable #combotargettype option:selected").val();
			switch(targetType) {
			case "0":
				$("#linkhttpparamstable #targetdevicename").show();
				$("#linkhttpparamstable #targetonoffdevicename").hide();
				$("#linkhttpparamstable #variabletosend").show();
				$("#linkhttpparamstable #scenevariabletosend").hide();
				$("#linkhttpparamstable #rebootvariabletosend").hide();
				$("#linkhttpparamstable #targetvariablerow").show();
				$("#linkhttpparamstable #targetdeviceidrow").hide();
				$("#linkhttpparamstable #targetpropertyrow").hide();
				$("#linkhttpparamstable #targetincludeunit").show();
				break;
			case "1":
				$("#linkhttpparamstable #targetdevicename").show();
				$("#linkhttpparamstable #targetonoffdevicename").hide();
				$("#linkhttpparamstable #variabletosend").show();
				$("#linkhttpparamstable #scenevariabletosend").hide();
				$("#linkhttpparamstable #rebootvariabletosend").hide();
				$("#linkhttpparamstable #targetvariablerow").hide();
				$("#linkhttpparamstable #targetdeviceidrow").show();
				$("#linkhttpparamstable #targetpropertyrow").show();
				$("#linkhttpparamstable #targetincludeunit").show();
				break;
			case "2":
				$("#linkhttpparamstable #targetdevicename").hide();
				$("#linkhttpparamstable #targetonoffdevicename").show();
				$("#linkhttpparamstable #variabletosend").hide();
				$("#linkhttpparamstable #scenevariabletosend").show();
				$("#linkhttpparamstable #rebootvariabletosend").hide();
				$("#linkhttpparamstable #targetvariablerow").hide();
				$("#linkhttpparamstable #targetdeviceidrow").show();
				$("#linkhttpparamstable #targetpropertyrow").hide();
				$("#linkhttpparamstable #targetincludeunit").hide();
				break;
			case "3":
				$("#linkhttpparamstable #targetdevicename").hide();
				$("#linkhttpparamstable #targetonoffdevicename").show();
				$("#linkhttpparamstable #variabletosend").hide();
				$("#linkhttpparamstable #scenevariabletosend").hide();
				$("#linkhttpparamstable #rebootvariabletosend").show();
				$("#linkhttpparamstable #targetvariablerow").hide();
				$("#linkhttpparamstable #targetdeviceidrow").hide();
				$("#linkhttpparamstable #targetpropertyrow").hide();
				$("#linkhttpparamstable #targetincludeunit").hide();
				break;
			default:
				break;
			}
			
		}
		SaveConfiguration = function()
		{
			var cleanurl = $('#httpremote #url').val();
			var httpdata = $('#httpremote #data').val();
			var httpheaders = $('#httpremote #headers').val();
			var method = $('#httpremote #combomethod').val();
			var linkactive = 0;	
			if ($('#httpremote #httplinkenabled').is(":checked"))
			{
				linkactive = 1;
			}
			var debugenabled = 0;
			if ($('#httpremote #debugenabled').is(":checked"))
			{
				debugenabled = 1;
			}
			
			var auth = $('#httpremote #comboauth').val();
			var authbasiclogin = $('#httpremote #authBasicUser').val();
			var authbasicpassword = $('#httpremote #authBasicPassword').val();
			$.ajax({
				 url: "json.htm?type=command&param=savehttplinkconfig" +
					"&url=" + encodeURIComponent(cleanurl) + "&method=" + method + "&headers=" + encodeURIComponent(httpheaders) + "&data=" + encodeURIComponent(httpdata) + "&linkactive=" + linkactive + "&debugenabled=" + debugenabled + "&auth=" + auth + "&authbasiclogin=" + authbasiclogin + "&authbasicpassword=" + authbasicpassword,
				 async: false, 
				 dataType: 'json',
				 success: function(data) {
					bootbox.alert($.t('Http link settings saved'));
				 },
				 error: function(){
						ShowNotify($.t('Problem saving Http link settings!'), 2500, true);
				 }     
			});
		}

		DeleteLink = function(idx)
		{
			bootbox.confirm($.t("Are you sure you want to remove this link?"), function(result) {
				if (result==true) {
					$.ajax({
						url: "json.htm?type=command&param=deletehttplink&idx=" + idx,
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
			var deviceid = $("#linkhttpparamstable #devicename option:selected").val();
			var onoffdeviceid = $("#linkhttpparamstable #onoffdevicename option:selected").val();
			var valuetosend = $('#linkhttpparamstable #combosendvalue option:selected').val();
			var scenevaluetosend = $('#linkhttpparamstable #sendvaluescene option:selected').val();
			var rebootvaluetosend = $('#linkhttpparamstable #sendvaluereboot option:selected').val();
			var targettype = $("#linkhttpparamstable #combotargettype option:selected").val();
			var targetvariable = $('#linkhttpparamstable #targetvariable').val();
			var targetdeviceid = $('#linkhttpparamstable #targetdeviceid').val();
			var targetproperty = $('#linkhttpparamstable #targetproperty').val();
			var linkactive = 0;	
			if ($('#linkhttpparamstable #linkenabled').is(":checked"))
			{
				linkactive = 1;
			}
			var includeunit = 0;
			if ($('#linkhttpparamstable #includeunit').is(":checked"))
			{
				includeunit = 1;
			}
			
			var url = "";
			if (targettype == 0) {
				url = "json.htm?type=command&param=savehttplink" + "&idx=" + idx + "&deviceid=" + deviceid + "&valuetosend=" + valuetosend + "&targettype=" + targettype + "&targetvariable=" + targetvariable + "&includeunit=" + includeunit + "&linkactive=" + linkactive;
			}
			else if (targettype == 1) {
				url = "json.htm?type=command&param=savehttplink" + "&idx=" + idx + "&deviceid=" + deviceid + "&valuetosend=" + valuetosend + "&targettype=" + targettype + "&targetdeviceid=" + targetdeviceid + "&targetproperty=" + targetproperty + "&includeunit=" + includeunit + "&linkactive=" + linkactive;	
			}
			else if (targettype == 2) {
				url = "json.htm?type=command&param=savehttplink" + "&idx=" + idx + "&deviceid=" + onoffdeviceid + "&valuetosend=" + scenevaluetosend + "&targettype=" + targettype + "&targetdeviceid=" + targetdeviceid + "&linkactive=" + linkactive;	
			}
			else if (targettype == 3) {
				url = "json.htm?type=command&param=savehttplink" + "&idx=" + idx + "&deviceid=" + onoffdeviceid + "&valuetosend=" + rebootvaluetosend + "&targettype=" + targettype + "&targetdeviceid=0&linkactive=" + linkactive;	
			}
			
			$.ajax({
				 url: url, 
				 async: false, 
				 dataType: 'json',
				 success: function(data) {
					bootbox.alert($.t('Http link saved'));
					RefreshLinkTable();
				 },
				 error: function(){
						ShowNotify($.t('Problem saving Http link!'), 2500, true);
				 }     
			});
		}

		GetConfig = function() 
		{
			$.ajax({
				url: "json.htm?type=command&param=gethttplinkconfig",
				async: false, 
				dataType: 'json',
				success: function(data) {
					if (typeof data != 'undefined') {
						if (data.status=="OK") {
							$('#httpremote #url').val(data.HttpUrl);
							$('#httpremote #combomethod').val(data.HttpMethod);
							$('#httpremote #data').val(data.HttpData);
							$('#httpremote #headers').val(data.HttpHeaders);
							$('#httpremote #httplinkenabled').prop('checked', false);
							if (data.HttpActive) {
								$('#httpremote #httplinkenabled').prop('checked', true);
							}
							$('#httpremote #debugenabled').prop('checked', false);
							if (data.HttpDebug) {
								$('#httpremote #debugenabled').prop('checked', true);
							}
							$('#httpremote #comboauth').val(data.HttpAuth);
							$('#httpremote #authBasicUser').val(data.HttpAuthBasicLogin);
							$('#httpremote #authBasicPassword').val(data.HttpAuthBasicPassword);
				
							MethodUpdate();
							MethodAuthenticationUpdate();
						}
					}
				},
				error: function(){
					ShowNotify($.t('Problem retrieving Http link settings!'), 2500, true);
				}
			});
		}
			

		ValueSelectorUpdate = function()
		{
			var deviceid = $("#linkhttpparamstable #devicename option:selected").val();	
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

			$('#linkhttpparamstable #linkupdate').attr("class", "btnstyle3-dis");
			$('#linkhttpparamstable #linkdelete').attr("class", "btnstyle3-dis");

		  var oTable = $('#linkhttptable').dataTable();
		  oTable.fnClearTable();
		  $.ajax({
			 url: "json.htm?type=command&param=gethttplinks",
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
			$('#httpmain').i18n();
			var oTable = $('#linkhttptable').dataTable( {
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
						$('#linkhttpparamstable #linkupdate').attr("class", "btnstyle3");
						$('#linkhttpparamstable #linkdelete').attr("class", "btnstyle3");
						var anSelected = fnGetSelected( oTable );
						if ( anSelected.length !== 0 ) {
							var data = oTable.fnGetData( anSelected[0] );
							var idx= data["DT_RowId"];
							$.linkIdx=idx;	
							$("#linkhttpparamstable #linkupdate").attr("href", "javascript:AddLink('u')");
							$("#linkhttpparamstable #linkdelete").attr("href", "javascript:DeleteLink(" + idx + ")");
							$("#linkhttpparamstable #combotargettype").val(data["TargetType"]);
							TargetTypeUpdate();
							if (data["TargetType"]==2) {
								$("#linkhttpparamstable #sendvaluescene").val(data["Delimitedvalue"]);
								$("#linkhttpparamstable #onoffdevicename").val(data["DeviceID"]); 
							}
							if (data["TargetType"]==3) {
								$("#linkhttpparamstable #sendvaluereboot").val(data["Delimitedvalue"]);
								$("#linkhttpparamstable #onoffdevicename").val(data["DeviceID"]); 
							}
							else {
								$("#linkhttpparamstable #devicename").val(data["DeviceID"]); 
								ValueSelectorUpdate();
								$("#linkhttpparamstable #combosendvalue").val(data["Delimitedvalue"]);
							}
							$("#linkhttpparamstable #targetvariable").val(data["3"]);
							$("#linkhttpparamstable #targetdeviceid").val(data["4"]);
							$("#linkhttpparamstable #targetproperty").val(data["5"]);
							if (data["Enabled"] == 1) {
								$('#linkhttpparamstable #linkenabled').prop('checked', true);
							}
							else {
								$('#linkhttpparamstable #linkenabled').prop('checked', false);
							}
							if (data["IncludeUnit"] == 1) {
								$('#linkhttpparamstable #includeunit').prop('checked', true);
							}
							else {
								$('#linkhttpparamstable #includeunit').prop('checked', false);
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
			$("#httpcontent #linkhttptable #combotype").change(function() { 
				UpdateLinks();
			});
			
			RefreshLinkTable();
		}

		init();

		function init()
		{
			$scope.MakeGlobalConfig();
			//Get devices
			$("#httpmain #devicename").html("");
			$.ajax({
			 url: "json.htm?type=command&param=devices_list",
			 async: false, 
			 dataType: 'json',
			 success: function(data) {
				if (typeof data.result != 'undefined') {
					$.each(data.result, function(i,item) {
						var option = $('<option />');
						option.attr('value', item.value).text(item.name);
						$("#httpmain #devicename").append(option);
					});
				}
				ValueSelectorUpdate();
			 }
			});
			
			//Get devices On/Off
			$("#httpmain #onoffdevicename").html("");
			$.ajax({
			 url: "json.htm?type=command&param=devices_list_onoff",
			 async: false, 
			 dataType: 'json',
			 success: function(data) {
				if (typeof data.result != 'undefined') {
					$.each(data.result, function(i,item) {
						var option = $('<option />');
						option.attr('value', item.value).text(item.name);
						$("#httpmain #onoffdevicename").append(option);
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