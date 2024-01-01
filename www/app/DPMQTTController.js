define(['app'], function (app) {
	app.controller('DPMQTTController', [ '$scope', '$rootScope', '$location', '$http', '$interval', function($scope,$rootScope,$location,$http,$interval) {

		$scope.LoadConfiguration = function() 
		{
			$.ajax({
				url: "json.htm?type=command&param=getmqttlinkconfig",
				async: false, 
				dataType: 'json',
				success: function(data) {
					if (typeof data != 'undefined') {
						if (data.status=="OK") {
							$('#mqttremote #ipaddress').val(data.ipaddress);
							$('#mqttremote #port').val(data.port);
							$('#mqttremote #username').val(data.username);
							$('#mqttremote #password').val(data.password);
							$('#mqttremote #topicout').val(data.topicout);
							$('#mqttremote #cafile').val(data.cafile);
							
							$('#mqttremote #linkactive').prop('checked', false);
							if (data.linkactive) {
								$('#mqttremote #linkactive').prop('checked', true);
							}
							var tls_version = 2;
							if (tls_version == 0)
								tls_version = 2;
							$("#mqttremote #combotlsversion").val(tls_version);
						}
					}
				},
				error: function(){
					ShowNotify($.t('Problem retrieving MQTT link settings!'), 2500, true);
				}
			});
		}

		SaveConfiguration = function()
		{
			var linkactive = 0;	
			if ($('#mqttremote #linkactive').is(":checked"))
			{
				linkactive = 1;
			}
			var ipaddress = $('#mqttremote #ipaddress').val();
			var port = $('#mqttremote #port').val();
			if (port.Length==0)
				port="1883";
			var username = $('#mqttremote #username').val();
			var password = $('#mqttremote #password').val();
			var topicout = $('#mqttremote #topicout').val();
			var cafile = $('#mqttremote #cafile').val();
			var tlsversion = $("#mqttremote #combotlsversion").val();

			$.ajax({
				 url:
					"json.htm?type=command&param=savemqttlinkconfig" +
					"&linkactive=" + linkactive +
					"&ipaddress=" + encodeURIComponent(ipaddress) +
					"&port=" + port +
					"&username=" + encodeURIComponent(username) +
					"&password=" + encodeURIComponent(password) +
					"&topicout=" + encodeURIComponent(topicout) +
					"&cafile=" + encodeURIComponent(cafile) +
					"&tlsversion=" + tlsversion,
				 async: false, 
				 dataType: 'json',
				 success: function(data) {
					bootbox.alert($.t('MQTT link settings saved'));
				 },
				 error: function(){
						ShowNotify($.t('Problem saving MQTT link settings!'), 2500, true);
				 }     
			});
		}

		DeleteLink = function(idx)
		{
			if ($('#linkparamstable #linkdelete').attr("class") == "btnstyle3-dis") {
				return;
			}
		
			bootbox.confirm($.t("Are you sure you want to remove this link?"), function(result) {
				if (result==true) {
					$.ajax({
						url: "json.htm?type=command&param=deletemqttlink&idx=" + idx,
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
			if (type == "u") {
				if ($('#linkparamstable #linkupdate').attr("class") == "btnstyle3-dis") {
					return;
				}
			}
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
				"json.htm?type=command&param=savemqttlink" +
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
					bootbox.alert($.t('MQTT link saved'));
					RefreshLinkTable();
				 },
				 error: function(){
						ShowNotify($.t('Problem saving MQTT link!'), 2500, true);
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

		  var oTable = $('#mqlinktable').dataTable();
		  oTable.fnClearTable();
		  $.ajax({
			 url: "json.htm?type=command&param=getmqttlinks",
			 async: false, 
			 dataType: 'json',
			 success: function(data) {   
			  if (typeof data.result != 'undefined') {
				$.each(data.result, function(i,item){
					var addId = oTable.fnAddData( {
						"DT_RowId": item.idx,
						"DeviceID": item.DeviceID,
						"TargetType": item.TargetType,
						"Enabled": item.Enabled,
						"Delimitedvalue": item.Delimitedvalue,
						"0": item.DeviceID,
						"1": item.Name,
						"2": $.t(item.Delimitedname),
						"3": (item.TargetType==0) ? $.t('On Value Change') : $.t('Direct'),
						"4": (item.Enabled == 1) ? $.t("Yes") : $.t("No")
					} );
				});
			  }
			 }
		  });
		  $('#modal').hide();
		}

		ShowLinks = function()
		{
			$('#mqttmain').i18n();
			var oTable = $('#mqlinktable').dataTable( {
			  "sDom": '<"H"lfrC>t<"F"ip>',
			  "oTableTools": {
				"sRowSelect": "single"
			  },
			  "aaSorting": [[ 0, "desc" ]],
			  "bSortClasses": false,
			  "bProcessing": true,
			  "bStateSave": true,
			  "bJQueryUI": true,
			  "aLengthMenu": [[15, 50, 100, -1], [15, 50, 100, "All"]],
			  "iDisplayLength" : 15,
			  "sPaginationType": "full_numbers",
			  language: $.DataTableLanguage
			} );
			
			/* Add a click handler to the rows - this could be used as a callback */
            $("#mqlinktable tbody").off();
            $("#mqlinktable tbody").on('click', 'tr', function () {
                if ($(this).hasClass('row_selected')) {
                    $(this).removeClass('row_selected');
					$('#linkparamstable #linkupdate').attr("class", "btnstyle3-dis");
					$('#linkparamstable #linkdelete').attr("class", "btnstyle3-dis");
                }
                else {
					$('#linkparamstable #linkupdate').attr("class", "btnstyle3");
					$('#linkparamstable #linkdelete').attr("class", "btnstyle3");
                    var oTable = $('#mqlinktable').dataTable();
                    oTable.$('tr.row_selected').removeClass('row_selected');
                    $(this).addClass('row_selected');
                    var anSelected = fnGetSelected(oTable);
                    if (anSelected.length !== 0) {
                        var data = oTable.fnGetData(anSelected[0]);
                        var idx = data["DT_RowId"];
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
                }
            });

			RefreshLinkTable();
		}

		init();

		function init()
		{
			$scope.MakeGlobalConfig();
			//Get devices
			$("#mqttmain #devicename").html("");
			$.ajax({
			 url: "json.htm?type=command&param=devices_list",
			 async: false, 
			 dataType: 'json',
			 success: function(data) {
				if (typeof data.result != 'undefined') {
					$.each(data.result, function(i,item) {
						var option = $('<option />');
						option.attr('value', item.idx).text(item.name);
						$("#mqttmain #devicename").append(option);
					});
				}
				ValueSelectorUpdate();
			 }
			});
			//global var
			$.linkIdx=0;
			$scope.LoadConfiguration();
			ShowLinks();
		};
	} ]);
});