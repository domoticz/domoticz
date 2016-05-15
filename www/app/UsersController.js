define(['app'], function (app) {
	app.controller('UsersController', [ '$scope', '$rootScope', '$location', '$http', '$interval','md5', function($scope,$rootScope,$location,$http,$interval,md5) {

		DeleteUser = function(idx)
		{
			bootbox.confirm($.t("Are you sure you want to delete this User?"), function(result) {
				if (result==true) {
					$.ajax({
						 url: "json.htm?type=command&param=deleteuser&idx=" + idx,
						 async: false, 
						 dataType: 'json',
						 success: function(data) {
							RefreshUserTable();
						 },
						 error: function(){
								HideNotify();
								ShowNotify($.t('Problem deleting User!'), 2500, true);
						 }     
					});
				}
			});
		}

		GetUserSettings = function()
		{
			var csettings = {};

			csettings.bEnabled=$('#usercontent #userparamstable #enabled').is(":checked");
			csettings.username=$("#usercontent #userparamstable #username").val();
			if (csettings.username=="")
			{
				ShowNotify($.t('Please enter a Username!'), 2500, true);
				return;
			}
			csettings.password=$("#usercontent #userparamstable #userpassword").val();
			if (csettings.password=="")
			{
				ShowNotify($.t('Please enter a Password!'), 2500, true);
				return;
			}
			if (csettings.password.length!=32) {
				csettings.password=md5.createHash(csettings.password);
			}
			csettings.rights=$("#usercontent #userparamstable #comborights").val();
			csettings.bEnableSharing=$('#usercontent #userparamstable #enablesharing').is(":checked");

			csettings.TabsEnabled=0;
			if ($('#usercontent #userparamstable #EnableTabLights').is(":checked")) {
				csettings.TabsEnabled|=(1<<0);
			}
			if ($('#usercontent #userparamstable #EnableTabScenes').is(":checked")) {
				csettings.TabsEnabled|=(1<<1);
			}
			if ($('#usercontent #userparamstable #EnableTabTemp').is(":checked")) {
				csettings.TabsEnabled|=(1<<2);
			}
			if ($('#usercontent #userparamstable #EnableTabWeather').is(":checked")) {
				csettings.TabsEnabled|=(1<<3);
			}
			if ($('#usercontent #userparamstable #EnableTabUtility').is(":checked")) {
				csettings.TabsEnabled|=(1<<4);
			}
			if ($('#usercontent #userparamstable #EnableTabCustom').is(":checked")) {
				csettings.TabsEnabled|=(1<<5);
			}
			if ($('#usercontent #userparamstable #EnableTabFloorplans').is(":checked")) {
				csettings.TabsEnabled|=(1<<6);
			}
			return csettings;
		}

		UpdateUser = function(idx)
		{
			var csettings=GetUserSettings();
			if (typeof csettings == 'undefined') {
				return;
			}

			$.ajax({
				 url: "json.htm?type=command&param=updateuser&idx=" + idx + 
						"&enabled=" + csettings.bEnabled + 
						"&username=" + csettings.username + 
						"&password=" + csettings.password + 
						"&rights=" + csettings.rights + 
						"&RemoteSharing=" + csettings.bEnableSharing +
						"&TabsEnabled=" + csettings.TabsEnabled,
				 async: false, 
				 dataType: 'json',
				 success: function(data) {
					RefreshUserTable();
				 },
				 error: function(){
					ShowNotify($.t('Problem updating User!'), 2500, true);
				 }     
			});
		}

		SaveUserDevices = function()
		{
			var selecteddevices = $("#usercontent .multiselect option:selected").map(function(){ return this.value }).get().join(";");
			$.ajax({
				 url: "json.htm?type=setshareduserdevices&idx=" + $.devIdx + "&devices=" + selecteddevices,
				 async: false, 
				 dataType: 'json',
				 success: function(data) {
					ShowUsers();
				 },
				 error: function(){
					ShowNotify($.t('Problem setting User Devices!'), 2500, true);
				 }     
			});
		}

		EditSharedDevices = function(idx,name)
		{
			$.devIdx=idx;
			cursordefault();
			var htmlcontent = '';
			htmlcontent='<p><h2>' + $.t('User') +': ' + name + '</h2></p>\n';
			htmlcontent+=$('#userdevices').html();
			$('#usercontent').html(GetBackbuttonHTMLTable('ShowUsers')+htmlcontent);
			$('#usercontent').i18n();
			
			var defaultOptions = {
							availableListPosition: 'left',
							splitRatio: 0.5,
							moveEffect: 'blind',
							moveEffectOptions: {direction:'vertical'},
							moveEffectSpeed: 'fast'
						}; 
			
			$("#usercontent .multiselect").multiselect(defaultOptions);

			$.ajax({
			 url: "json.htm?type=getshareduserdevices&idx=" + idx, 
			 async: false, 
			 dataType: 'json',
			 success: function(data) {
				 if (typeof data.result != 'undefined') {
					$.each(data.result, function(i,item) {
						var selstr='#usercontent .multiselect option[value="' + item.DeviceRowIdx +'"]';
						$(selstr).attr("selected", "selected");
					});
				 }
			 }
			});
		}

		AddUser = function()
		{
			var csettings=GetUserSettings();
			if (typeof csettings == 'undefined') {
				return;
			}

			$.ajax({
				 url: "json.htm?type=command&param=adduser&enabled=" + csettings.bEnabled +
						"&username=" + csettings.username + 
						"&password=" + csettings.password + 
						"&rights=" + csettings.rights + 
						"&RemoteSharing=" + csettings.bEnableSharing +
						"&TabsEnabled=" + csettings.TabsEnabled,
				 async: false, 
				 dataType: 'json',
				 success: function(data) {
					if (data.status != "OK") {
						ShowNotify(data.message, 2500, true);
						return;
					}
					RefreshUserTable();
				 },
				 error: function(){
					ShowNotify($.t('Problem adding User!'), 2500, true);
				 }     
			});		
		}

		RefreshUserTable = function()
		{
			$('#modal').show();

			$.devIdx=-1;

			$('#updelclr #userupdate').attr("class", "btnstyle3-dis");
			$('#updelclr #userdelete').attr("class", "btnstyle3-dis");

			var oTable = $('#usertable').dataTable();
			oTable.fnClearTable();
			$.ajax({
			 url: "json.htm?type=users", 
			 async: false, 
			 dataType: 'json',
			 success: function(data) {
				
			  if (typeof data.result != 'undefined') {
				$.each(data.result, function(i,item){
					var enabledstr=$.t("No");
					if (item.Enabled=="true") {
						enabledstr=$.t("Yes");
					}
					var rightstr="rights";
					if (item.Rights==0) {
						rightstr=$.t("Viewer");
					}
					else if (item.Rights==1) {
						rightstr=$.t("User");
					}
					else {
						rightstr=$.t("Admin");
					}
					
					var sharedstr=$.t("No");
					if (item.RemoteSharing==true) {
						sharedstr=$.t('Yes');
					}
					var devicesstr='<span class="label label-info lcursor" onclick="EditSharedDevices(' + item.idx + ',\'' + item.Username + '\');">' + $.t('Set Devices') +'</span>';
					var addId = oTable.fnAddData( {
						"DT_RowId": item.idx,
						"Enabled": item.Enabled,
						"Username": item.Username,
						"Password": item.Password,
						"Rights": item.Rights,
						"Sharing": item.RemoteSharing,
						"TabsEnabled": item.TabsEnabled,
						"0": enabledstr,
						"1": item.Username,
						"2": rightstr,
						"3": sharedstr,
						"4": devicesstr
					} );
				});
			  }
			 }
			});

			/* Add a click handler to the rows - this could be used as a callback */
			$("#usertable tbody").off();
			$("#usertable tbody").on( 'click', 'tr', function () {
				$.devIdx=-1;

				if ( $(this).hasClass('row_selected') ) {
					$(this).removeClass('row_selected');
					$('#updelclr #userupdate').attr("class", "btnstyle3-dis");
					$('#updelclr #userdelete').attr("class", "btnstyle3-dis");
				}
				else {
					var oTable = $('#usertable').dataTable();
					oTable.$('tr.row_selected').removeClass('row_selected');
					$(this).addClass('row_selected');
					$('#updelclr #userupdate').attr("class", "btnstyle3");
					$('#updelclr #userdelete').attr("class", "btnstyle3");
					var anSelected = fnGetSelected( oTable );
					if ( anSelected.length !== 0 ) {
						var data = oTable.fnGetData( anSelected[0] );
						var idx= data["DT_RowId"];
						$.devIdx=idx;
						$("#updelclr #userupdate").attr("href", "javascript:UpdateUser(" + idx + ")");
						$("#updelclr #userdelete").attr("href", "javascript:DeleteUser(" + idx + ")");
						$('#usercontent #userparamstable #enabled').prop('checked',(data["Enabled"]=="true"));
						$("#usercontent #userparamstable #username").val(data["Username"]);
						$("#usercontent #userparamstable #userpassword").val(data["Password"]);
						$("#usercontent #userparamstable #comborights").val(data["Rights"]);
						$('#usercontent #userparamstable #enablesharing').prop('checked',(data["Sharing"]=="1"));
						
						var EnabledTabs=parseInt(data["TabsEnabled"]);
						$('#usercontent #userparamstable #EnableTabLights').prop('checked',(EnabledTabs & 1));
						$('#usercontent #userparamstable #EnableTabScenes').prop('checked',(EnabledTabs & 2));
						$('#usercontent #userparamstable #EnableTabTemp').prop('checked',(EnabledTabs & 4));
						$('#usercontent #userparamstable #EnableTabWeather').prop('checked',(EnabledTabs & 8));
						$('#usercontent #userparamstable #EnableTabUtility').prop('checked',(EnabledTabs & 16));
						$('#usercontent #userparamstable #EnableTabCustom').prop('checked',(EnabledTabs & 32));
						$('#usercontent #userparamstable #EnableTabFloorplans').prop('checked',(EnabledTabs & 64));
					}
				}
			}); 
		  
		  $('#modal').hide();
		}

		ShowUsers = function()
		{
			var oTable;
			
			$('#modal').show();
			
			$.devIdx=-1;
			
			var htmlcontent = "";
			htmlcontent+=$('#usermain').html();
			$('#usercontent').html(htmlcontent);
			$('#usercontent').i18n();
			
			oTable = $('#usertable').dataTable( {
				"sDom": '<"H"lfrC>t<"F"ip>',
				"oTableTools": {
					"sRowSelect": "single",
				},
				"aoColumnDefs": [
					{ "bSortable": false, "aTargets": [ 0 ] }
				],
				"aaSorting": [[ 1, "asc" ]],
				"bSortClasses": false,
				"bProcessing": true,
				"bStateSave": true,
				"bJQueryUI": true,
				"aLengthMenu": [[25, 50, 100, -1], [25, 50, 100, "All"]],
				"iDisplayLength" : 25,
				"sPaginationType": "full_numbers",
				language: $.DataTableLanguage
				} );

			$('#modal').hide();
			RefreshUserTable();
		}

		init();

		function init()
		{
			$scope.MakeGlobalConfig();
			//Get devices
			$("#userdevices #userdevicestable #devices").html("");
			$.ajax({
			 url: "json.htm?type=command&param=devices_list",
			 async: false, 
			 dataType: 'json',
			 success: function(data) {
				if (typeof data.result != 'undefined') {
					$.each(data.result, function(i,item) {
						var option = $('<option />');
						option.attr('value', item.value).text(item.name);
						$("#userdevices #userdevicestable #devices").append(option);
					});
				}
			 }
			});

			ShowUsers();
		};
	} ]);
});