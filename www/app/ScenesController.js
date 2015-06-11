define(['app'], function (app) {
	app.controller('ScenesController', [ '$scope', '$rootScope', '$location', '$http', '$interval', 'permissions', function($scope,$rootScope,$location,$http,$interval,permissions) {

		RemoveCode = function()
		{
			if($("#scenecontent #removecode").hasClass('disabled')) {
				return false;
			}
			$("#scenecontent #removecode").removeClass("btn-danger");
			$("#scenecontent #learncode").removeClass("btn-warning");

			$("#scenecontent #removecode").addClass("disabled");
			$("#scenecontent #learncode").addClass("btn-success");
			$("#scenecontent #learncode").html($.t("Learn Code"));

			$("#scenecontent #learndevicename").html("");

			$.ajax({
				 url: "json.htm?type=command&param=removescenecode&idx="+$.devIdx,
				 async: false, 
				 dataType: 'json',
				 success: function(data) {
				 }
			});
		}

		LearnCode = function()
		{
		  ShowNotify($.t('Press button on Remote...'));
		  
		  setTimeout(function() {
			var bHaveFoundDevice = false;
			var deviceID = "";
			var deviceidx = 0;
			var bIsUsed = false;
			var Cmd = 0;
			var Name = "";
			
			$.ajax({
			   url: "json.htm?type=command&param=learnsw", 
			   async: false, 
			   dataType: 'json',
			   success: function(data) {
				if (typeof data.status != 'undefined') {
				  if (data.status == 'OK')
				  {
					bHaveFoundDevice=true;
					deviceID=data.ID;
					deviceidx=data.idx;
					bIsUsed=data.Used;
					Name=data.Name;
					Cmd=data.Cmd;
				  }
				}
			   }
			});
			HideNotify();
			
			setTimeout(function() {
			  if (bHaveFoundDevice == true)
			  {
						$("#scenecontent #removecode").removeClass("disabled");
						$("#scenecontent #learncode").removeClass("btn-success");

						$("#scenecontent #removecode").addClass("btn-danger");
						$("#scenecontent #learncode").addClass("btn-warning");
						$("#scenecontent #learncode").html($.t("Change Code"));
						
						$("#scenecontent #learndevicename").html("( " + Name + " )");
						
						$.ajax({
							 url: "json.htm?type=command&param=setscenecode&idx="+$.devIdx+"&devid="+deviceidx+"&cmnd="+Cmd, 
							 async: false, 
							 dataType: 'json',
							 success: function(data) {
							 }
						});
			  }
			  else {
				 ShowNotify($.t('Timeout...<br>Please try again!'), 2500, true);
			  }
			}, 200);
			}, 600);
		}

		AddScene = function()
		{
			$( "#dialog-addscene" ).dialog( "open" );
		}

		DeleteScene = function()
		{
			bootbox.confirm($.t("Are you sure to remove this Scene?"), function(result) {
				if (result==true) {
					$.ajax({
						 url: "json.htm?type=deletescene&idx=" + $.devIdx,
						 async: false, 
						 dataType: 'json',
						 success: function(data) {
								ShowScenes();
						 }
					});
				}
			});
		}

		SaveScene = function()
		{
				var bValid = true;
				bValid = bValid && checkLength( $("#scenecontent #devicename"), 2, 100 );
				
				var onaction=$("#scenecontent #onaction").val();
				var offaction=$("#scenecontent #offaction").val();
				
				if (onaction!="") {
					if ( (onaction.indexOf("http://") !=0) && (onaction.indexOf("script://") !=0) ) {
						bootbox.alert($.t("Invalid ON Action!"));
						return;
					}
					else {
						if (checkLength( $("#scenecontent #onaction"), 10, 500 )==false) {
							bootbox.alert($.t("Invalid ON Action!"));
							return;
						}
					}
				}
				if (offaction!="") {
					if ( (offaction.indexOf("http://") !=0) && (offaction.indexOf("script://") !=0) ) {
						bootbox.alert($.t("Invalid Off Action!"));
						return;
					}
					else {
						if (checkLength( $("#scenecontent #offaction"), 10, 500 )==false) {
							bootbox.alert($.t("Invalid Off Action!"));
							return;
						}
					}
				}
				
				if ( bValid ) {
						var SceneType=$("#scenecontent #combotype").val();
						var bIsProtected=$('#scenecontent #protected').is(":checked");
						$.ajax({
							 url: "json.htm?type=updatescene&idx=" + $.devIdx + 
									"&scenetype=" + SceneType + 
									"&name=" + encodeURIComponent($("#scenecontent #devicename").val()) +
									'&onaction=' + btoa(onaction) +
									'&offaction=' + btoa(offaction) +
									"&protected=" + bIsProtected,
							 async: false, 
							 dataType: 'json',
							 success: function(data) {
					  ShowScenes();
							 }
						});
				}
		}

		AddDevice = function()
		{
			var DeviceIdx=$("#scenecontent #combodevice option:selected").val();
			if (typeof DeviceIdx == 'undefined') {
				bootbox.alert($.t('No Device Selected!'));
				return ;
			}

			var Command=$("#scenecontent #combocommand option:selected").val();
			
			var level=100;
			var hue=0;
			$.each($.LightsAndSwitches, function(i,item){
				if (item.idx==DeviceIdx) {
					var bIsLED=(item.SubType.indexOf("RGB") >= 0);
					if (bIsLED==true) {
						level=$("#scenecontent #Brightness").val();
						hue=$("#scenecontent #Hue").val();
						var bIsWhite=$('#scenecontent #ledtable #optionsWhite').is(":checked")
						if (bIsWhite==true) {
							hue=1000;
						}
					}
					else {
						if (item.isdimmer==true) {
							level=$("#scenecontent #combolevel").val();
						}
					}
				}
			});
			var ondelay=$("#scenecontent #ondelaytime").val();
			var offdelay=$("#scenecontent #offdelaytime").val();

			$.ajax({
				 url: "json.htm?type=command&param=addscenedevice&idx=" + $.devIdx + "&isscene=" + $.isScene + "&devidx=" + DeviceIdx + "&command=" + Command + "&level=" + level + "&hue=" + hue + "&ondelay=" + ondelay+ "&offdelay=" + offdelay,
				 async: false, 
				 dataType: 'json',
				 success: function(data) {
					if (data.status == 'OK') {
						RefreshDeviceTable($.devIdx);
					}
					else {
						ShowNotify($.t('Problem adding Device!'), 2500, true);
					}
				 },
				 error: function(){
						HideNotify();
						ShowNotify($.t('Problem adding Device!'), 2500, true);
				 }     
			});
		}

		ClearDevices = function()
		{
			var bValid = false;
			bootbox.confirm($.t("Are you sure to delete ALL Devices?\n\nThis action can not be undone!"), function(result) {
				if (result==true) {
					$.ajax({
						 url: "json.htm?type=command&param=deleteallscenedevices&idx=" + $.devIdx,
						 async: false, 
						 dataType: 'json',
						 success: function(data) {
							RefreshDeviceTable($.devIdx);
						 }
					});
				}
			});
		}

		MakeFavorite = function(id,isfavorite)
		{
			if (!permissions.hasPermission("Admin")) {
				HideNotify();
				ShowNotify($.t('You do not have permission to do that!'), 2500, true);
				return;
			}

			if (typeof $scope.mytimer != 'undefined') {
				$interval.cancel($scope.mytimer);
				$scope.mytimer = undefined;
			}
		  $.ajax({
			 url: "json.htm?type=command&param=makescenefavorite&idx=" + id + "&isfavorite=" + isfavorite, 
			 async: false, 
			 dataType: 'json',
			 success: function(data) {
			  ShowScenes();
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
			 url: "json.htm?type=command&param=changescenedeviceorder&idx=" + devid + "&way=" + order, 
			 async: false, 
			 dataType: 'json',
			 success: function(data) {
				RefreshDeviceTableEx();
			 }
		  });
		}

		SetColValue = function (idx,hue,brightness)
		{
			clearInterval($.setColValue);
			if (permissions.hasPermission("Viewer")) {
				HideNotify();
				ShowNotify($.t('You do not have permission to do that!'), 2500, true);
				return;
			}
			var bIsWhite=$('#scenecontent #ledtable #optionsWhite').is(":checked");
			$.ajax({
				 url: "json.htm?type=command&param=setcolbrightnessvalue&idx=" + idx + "&hue=" + hue + "&brightness=" + brightness + "&iswhite=" + bIsWhite,
				 async: false, 
				 dataType: 'json'
			});
		}

		RefreshDeviceTableEx = function()
		{
			RefreshDeviceTable($.SceneIdx);
		}

		RefreshDeviceTable = function(idx)
		{
		  $('#modal').show();

			$.SceneIdx=idx;

			$('#scenecontent #delclr #devicedelete').attr("class", "btnstyle3-dis");
			$('#scenecontent #delclr #updatedelete').attr("class", "btnstyle3-dis");

		  var oTable = $('#scenecontent #scenedevicestable').dataTable();
		  oTable.fnClearTable();
		  
		  $.ajax({
			 url: "json.htm?type=command&param=getscenedevices&idx=" + idx + "&isscene=" + $.isScene, 
			 async: false, 
			 dataType: 'json',
			 success: function(data) {
				
			  if (typeof data.result != 'undefined') {
				var totalItems=data.result.length;
				$.each(data.result, function(i,item){
						var bIsLED=(item.SubType.indexOf("RGB") >= 0);
						var commandbtns;
						if ($.isScene==false) {
							if (item.IsOn==true) {
								commandbtns='<button class="btn btn-mini btn-info" type="button" onclick="SwitchLight(' + item.DevID + ',\'On\',RefreshDeviceTableEx);">' + $.t('ON') + '</button> <button class="btn btn-mini" type="button" onclick="SwitchLight(' + item.DevID + ',\'Off\',RefreshDeviceTableEx);">' + $.t('OFF') + '</button>';
							}
							else {
								commandbtns='<button class="btn btn-mini" type="button" onclick="SwitchLight(' + item.DevID + ',\'On\',RefreshDeviceTableEx);">' + $.t('ON') + '</button> <button class="btn btn-mini btn-info" type="button" onclick="SwitchLight(' + item.DevID + ',\'Off\',RefreshDeviceTableEx);">' + $.t('OFF') + '</button>';
							}
						}
						else {
							if (item.IsOn==true) {
								commandbtns='<button class="btn btn-mini btn-info" type="button">' + $.t('ON') + '</button> <button class="btn btn-mini" type="button">' + $.t('OFF') + '</button>';
							}
							else {
								commandbtns='<button class="btn btn-mini" type="button">' + $.t('ON') + '</button> <button class="btn btn-mini btn-info" type="button">' + $.t('OFF') + '</button>';
							}
						}
						var updownImg="";
						if (i!=totalItems-1) {
							//Add Down Image
							if (updownImg!="") {
								updownImg+="&nbsp;";
							}
							updownImg+='<img src="images/down.png" onclick="ChangeDeviceOrder(1,' + item.ID + ');" class="lcursor" width="16" height="16"></img>';
						}
						else {
							updownImg+='<img src="images/empty16.png" width="16" height="16"></img>';
						}
						if (i!=0) {
							//Add Up image
							if (updownImg!="") {
								updownImg+="&nbsp;";
							}
							updownImg+='<img src="images/up.png" onclick="ChangeDeviceOrder(0,' + item.ID + ');" class="lcursor" width="16" height="16"></img>';
						}
						var levelstr=item.Level + " %";
						
						if (bIsLED) {
							var hue=item.Hue;
							var sat=100;
							if (hue==1000) {
								hue=0;
								sat=0;
							}
							var cHSB=[];
							cHSB.h=hue;
							cHSB.s=sat;
							cHSB.b=item.Level;
							levelstr+='<div id="picker4" class="ex-color-box" style="background-color: #' + $.colpickHsbToHex(cHSB) + ';"></div>';
						}
						
						
						var addId = oTable.fnAddData( {
							"DT_RowId": item.ID,
							"IsOn": item.IsOn,
							"RealIdx": item.DevRealIdx,
							"Level": item.Level,
							"Hue": item.Hue,
							"OnDelay": item.OnDelay,
							"OffDelay": item.OffDelay,
							"Order": item.Order,
							"0": item.Name,
							"1": commandbtns,
							"2": levelstr,
							"3": item.OnDelay,
							"4": item.OffDelay,
							"5": updownImg
						} );
				});
			  }
			 }
		  });
			/* Add a click handler to the rows - this could be used as a callback */
			$("#scenecontent #scenedevicestable tbody").off();
			$("#scenecontent #scenedevicestable tbody").on( 'click', 'tr', function () {
				if ( $(this).hasClass('row_selected') ) {
						$(this).removeClass('row_selected');
						$('#scenecontent #delclr #devicedelete').attr("class", "btnstyle3-dis");
						$('#scenecontent #delclr #updatedelete').attr("class", "btnstyle3-dis");
				}
				else {
						var oTable = $('#scenecontent #scenedevicestable').dataTable();
						oTable.$('tr.row_selected').removeClass('row_selected');
						$(this).addClass('row_selected');
						
						$('#scenecontent #delclr #devicedelete').attr("class", "btnstyle3");
						
						$('#scenecontent #delclr #updatedelete').attr("class", "btnstyle3");
						$('#scenecontent #delclr #updatedelete').show();
						
						var anSelected = fnGetSelected( oTable );
						if ( anSelected.length !== 0 ) {
							var data = oTable.fnGetData( anSelected[0] );
							var idx= data["DT_RowId"];
							var devidx=data["RealIdx"];
							$("#scenecontent #delclr #devicedelete").attr("href", "javascript:DeleteDevice(" + idx + ")");
							$("#scenecontent #delclr #updatedelete").attr("href", "javascript:UpdateDevice(" + idx + "," + devidx + ")");
							$.lampIdx = devidx;
							$("#scenecontent #combodevice").val(devidx);
							if (data["IsOn"]==true) {
								$("#scenecontent #combocommand").val(1);
							}
							else {
								$("#scenecontent #combocommand").val(0);
							}
							OnSelChangeDevice();
							
							var level=data["Level"];
							$("#scenecontent #combolevel").val(level);
							$('#scenecontent #Brightness').val(level&255);
							
							var hue=data["Hue"];
							var sat=100;
							if (hue==1000) {
								hue=0;
								sat=0;
							}
							$('#scenecontent #Hue').val(hue);
							var cHSB=[];
							cHSB.h=hue;
							cHSB.s=sat;
							cHSB.b=level;
							
							$("#scenecontent #optionsRGB").prop('checked',(sat==100));
							$("#scenecontent #optionsWhite").prop('checked',!(sat==100));

							$('#scenecontent #picker').colpickSetColor(cHSB);
							
							$("#scenecontent #ondelaytime").val(data["OnDelay"]);
							$("#scenecontent #offdelaytime").val(data["OffDelay"]);
						}
				}
			}); 
		  
		  $('#modal').hide();
		}

		UpdateDevice = function(idx, devidx)
		{
			var DeviceIdx=$("#scenecontent #combodevice option:selected").val();
			if (typeof DeviceIdx == 'undefined') {
				bootbox.alert($.t('No Device Selected!'));
				return ;
			}
			if (DeviceIdx!=devidx) {
				bootbox.alert($.t('Device change not allowed!'));
				return ;
			}

			var Command=$("#scenecontent #combocommand option:selected").val();
			
			var level=100;
			var hue=0;
			var ondelay=$("#scenecontent #ondelaytime").val();
			var offdelay=$("#scenecontent #offdelaytime").val();

			$.each($.LightsAndSwitches, function(i,item){
				if (item.idx==DeviceIdx) {
					var bIsLED=(item.SubType.indexOf("RGB") >= 0);
					if (bIsLED==true) {
						level=$("#scenecontent #Brightness").val();
						hue=$("#scenecontent #Hue").val();
						var bIsWhite=$('#scenecontent #ledtable #optionsWhite').is(":checked")
						if (bIsWhite==true) {
							hue=1000;
						}
					}
					else {
						if (item.isdimmer==true) {
							level=$("#scenecontent #combolevel").val();
						}
					}
				}
			});

			$.ajax({
				 url: "json.htm?type=command&param=updatescenedevice&idx=" + idx + "&isscene=" + $.isScene + "&devidx=" + DeviceIdx + "&command=" + Command + "&level=" + level + "&hue=" + hue + "&ondelay=" + ondelay + "&offdelay=" + offdelay,
				 async: false, 
				 dataType: 'json',
				 success: function(data) {
					if (data.status == 'OK') {
						RefreshDeviceTable($.devIdx);
					}
					else {
						ShowNotify($.t('Problem updating Device!'), 2500, true);
					}
				 },
				 error: function(){
					HideNotify();
					ShowNotify($.t('Problem updating Device!'), 2500, true);
				 }     
			});
		}

		DeleteDevice = function(idx)
		{
			bootbox.confirm($.t("Are you sure to delete this Device?\n\nThis action can not be undone..."), function(result) {
				if (result==true) {
					$.ajax({
						 url: "json.htm?type=command&param=deletescenedevice&idx=" + idx,
						 async: false, 
						 dataType: 'json',
						 success: function(data) {
							RefreshDeviceTable($.devIdx);
						 }
					});
				}
			});
		}

		OnSelChangeDevice = function()
		{
			var DeviceIdx=$("#scenecontent #combodevice option:selected").val();
			if (typeof DeviceIdx == 'undefined') {
				$("#scenecontent #LevelDiv").hide();
				return ;
			}
			var bShowLevel=false;
			var bIsLED=false;
			$.each($.LightsAndSwitches, function(i,item){
				if (item.idx==DeviceIdx) {
					bShowLevel=item.isdimmer;
					bIsLED=(item.SubType.indexOf("RGB") >= 0);
				}
			});
			
			if (bIsLED) {
				$("#scenecontent #LedColor").show();
				$("#scenecontent #LevelDiv").hide();
			}
			else {
				$("#scenecontent #LedColor").hide();
				if (bShowLevel==true) {
					$("#scenecontent #LevelDiv").show();
				}
				else {
					$("#scenecontent #LevelDiv").hide();
				}
			}
		}

		EditSceneDevice = function(idx,name,havecode,type,bIsProtected,learndevicename,onaction,offaction)
		{
			if (typeof $scope.mytimer != 'undefined') {
				$interval.cancel($scope.mytimer);
				$scope.mytimer = undefined;
			}
		  $.devIdx=idx;

		 var bIsScene=(type=="Scene");
		 $.isScene=bIsScene;

		  var oTable;
			
		  var htmlcontent = '';
		  htmlcontent+=$('#editscene').html();
		  $('#scenecontent').html(GetBackbuttonHTMLTable('ShowScenes')+htmlcontent);
		  $('#scenecontent').i18n();
		  $("#scenecontent #LevelDiv").hide();
		  $("#scenecontent #LedColor").hide();
		  
		  $("#scenecontent #onaction").val(atob(onaction));
		  $("#scenecontent #offaction").val(atob(offaction));
		  
		  $('#scenecontent #protected').prop('checked',(bIsProtected==true));
		  
		  if (bIsScene==true) {
			$("#scenecontent #combotype").val(0);
			$("#scenecontent #CommandDiv").show();
			$("#scenecontent #CommandHeader").html($.t("Command"));
		  }
		  else {
			$("#scenecontent #combotype").val(1);
			$("#scenecontent #CommandDiv").hide();
			$("#scenecontent #CommandHeader").html($.t("State"));
		  }

			$('#scenecontent #picker').colpick({
				flat:true,
				layout:'hex',
				submit:0,
				onChange:function(hsb,hex,rgb,fromSetColor) {
					if(!fromSetColor) {
						$('#scenecontent #Hue').val(hsb.h);
						$('#scenecontent #Brightness').val(hsb.b);
						var bIsWhite=(hsb.s<20);
						$("#scenecontent #optionsRGB").prop('checked',!bIsWhite);
						$("#scenecontent #optionsWhite").prop('checked',bIsWhite);
						clearInterval($.setColValue);
						$.setColValue = setInterval(function() { SetColValue($.lampIdx,hsb.h,hsb.b); }, 400);
					}
				}
			});

		  oTable = $('#scenecontent #scenedevicestable').dataTable( {
						  "sDom": '<"H"lfrC>t<"F"ip>',
						  "oTableTools": {
							"sRowSelect": "single",
						  },
							"aoColumnDefs": [
								{ "bSortable": false, "aTargets": [ 1 ] }
							],
						  "bSort": false,
						  "bProcessing": true,
						  "bStateSave": false,
						  "bJQueryUI": true,
						  "aLengthMenu": [[25, 50, 100, -1], [25, 50, 100, "All"]],
						  "iDisplayLength" : 25,
						  "sPaginationType": "full_numbers",
						  language: $.DataTableLanguage
						} );
			$("#scenecontent #devicename").val(decodeURIComponent(name));

			$("#scenecontent #combodevice").html("");
			
			if ($.isScene==false) {
				$('#scenecontent #delclr #updatedelete').hide();
			}
			else {
				$('#scenecontent #delclr #updatedelete').show();
			}
			
			$.each($.LightsAndSwitches, function(i,item){
				var option = $('<option />');
				option.attr('value', item.idx).text(item.name);
				$("#scenecontent #combodevice").append(option);
			});

			$("#scenecontent #combodevice").change(function() { 
				OnSelChangeDevice();
			});
			$('#scenecontent #combodevice').keypress(function() {
			  $(this).change();
			});

			OnSelChangeDevice();

			if (havecode==0) {
					$("#scenecontent #removecode").addClass("disabled");
					$("#scenecontent #learncode").addClass("btn-success");
					$("#scenecontent #learncode").html($.t("Learn Code"));
					$("#scenecontent #learndevicename").html("");
			}
			else {
					$("#scenecontent #removecode").addClass("btn-danger");
					$("#scenecontent #learncode").addClass("btn-warning");
					$("#scenecontent #learncode").html($.t("Change Code"));
					$("#scenecontent #learndevicename").html("( " + learndevicename + " )");
			}

		  RefreshDeviceTable(idx);
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
									name: item.Name,
									SubType: item.SubType,
									isdimmer: item.IsDimmer
								 }
							);
				});
			  }
			 }
		  });
		}

		EnableDisableDays = function(TypeStr, bDisabled)
		{
				$('#scenecontent #timerparamstable #ChkMon').prop('checked', ((TypeStr.indexOf("Mon") >= 0)||(TypeStr=="Everyday")||(TypeStr=="Weekdays")) ? true : false);
				$('#scenecontent #timerparamstable #ChkTue').prop('checked', ((TypeStr.indexOf("Tue") >= 0)||(TypeStr=="Everyday")||(TypeStr=="Weekdays")) ? true : false);
				$('#scenecontent #timerparamstable #ChkWed').prop('checked', ((TypeStr.indexOf("Wed") >= 0)||(TypeStr=="Everyday")||(TypeStr=="Weekdays")) ? true : false);
				$('#scenecontent #timerparamstable #ChkThu').prop('checked', ((TypeStr.indexOf("Thu") >= 0)||(TypeStr=="Everyday")||(TypeStr=="Weekdays")) ? true : false);
				$('#scenecontent #timerparamstable #ChkFri').prop('checked', ((TypeStr.indexOf("Fri") >= 0)||(TypeStr=="Everyday")||(TypeStr=="Weekdays")) ? true : false);
				$('#scenecontent #timerparamstable #ChkSat').prop('checked', ((TypeStr.indexOf("Sat") >= 0)||(TypeStr=="Everyday")||(TypeStr=="Weekends")) ? true : false);
				$('#scenecontent #timerparamstable #ChkSun').prop('checked', ((TypeStr.indexOf("Sun") >= 0)||(TypeStr=="Everyday")||(TypeStr=="Weekends")) ? true : false);

				$('#scenecontent #timerparamstable #ChkMon').attr('disabled', bDisabled);
				$('#scenecontent #timerparamstable #ChkTue').attr('disabled', bDisabled);
				$('#scenecontent #timerparamstable #ChkWed').attr('disabled', bDisabled);
				$('#scenecontent #timerparamstable #ChkThu').attr('disabled', bDisabled);
				$('#scenecontent #timerparamstable #ChkFri').attr('disabled', bDisabled);
				$('#scenecontent #timerparamstable #ChkSat').attr('disabled', bDisabled);
				$('#scenecontent #timerparamstable #ChkSun').attr('disabled', bDisabled);
		}

		ClearTimers = function()
		{
			bootbox.confirm($.t("Are you sure to delete ALL timers?\n\nThis action can not be undone!"), function(result) {
				if (result==true) {
					$.ajax({
						 url: "json.htm?type=command&param=clearscenetimers&idx=" + $.devIdx,
						 async: false, 
						 dataType: 'json',
						 success: function(data) {
							RefreshTimerTable($.devIdx);
						 },
						 error: function(){
								HideNotify();
								ShowNotify($.t('Problem clearing timers!'), 2500, true);
						 }     
					});
				}
			});
		}

		DeleteTimer = function(idx)
		{
			bootbox.confirm($.t("Are you sure to delete this timers?\n\nThis action can not be undone..."), function(result) {
				if (result==true) {
					$.ajax({
						 url: "json.htm?type=command&param=deletescenetimer&idx=" + idx,
						 async: false, 
						 dataType: 'json',
						 success: function(data) {
							RefreshTimerTable($.devIdx);
						 },
						 error: function(){
								HideNotify();
								ShowNotify($.t('Problem deleting timer!'), 2500, true);
						 }     
					});
				}
			});
		}

		GetTimerSettings = function()
		{
			var tsettings = {};
			tsettings.level=100;
			tsettings.Active=$('#scenecontent #timerparamstable #enabled').is(":checked");
			tsettings.timertype=$("#scenecontent #timerparamstable #combotype").val();
			tsettings.date=$("#scenecontent #timerparamstable #sdate").val();
			tsettings.hour=$("#scenecontent #timerparamstable #combotimehour").val();
			tsettings.min=$("#scenecontent #timerparamstable #combotimemin").val();
			tsettings.Randomness=$('#scenecontent #timerparamstable #randomness').is(":checked");
			tsettings.cmd=$("#scenecontent #timerparamstable #combocommand").val();
			tsettings.days=0;
			var everyday=$("#scenecontent #timerparamstable #when_1").is(":checked");
			var weekdays=$("#scenecontent #timerparamstable #when_2").is(":checked");
			var weekends=$("#scenecontent #timerparamstable #when_3").is(":checked");
			if (everyday==true)
				tsettings.days=0x80;
			else if (weekdays==true)
				tsettings.days=0x100;
			else if (weekends==true)
				tsettings.days=0x200;
			else {
				if ($('#scenecontent #timerparamstable #ChkMon').is(":checked"))
					tsettings.days|=0x01;
				if ($('#scenecontent #timerparamstable #ChkTue').is(":checked"))
					tsettings.days|=0x02;
				if ($('#scenecontent #timerparamstable #ChkWed').is(":checked"))
					tsettings.days|=0x04;
				if ($('#scenecontent #timerparamstable #ChkThu').is(":checked"))
					tsettings.days|=0x08;
				if ($('#scenecontent #timerparamstable #ChkFri').is(":checked"))
					tsettings.days|=0x10;
				if ($('#scenecontent #timerparamstable #ChkSat').is(":checked"))
					tsettings.days|=0x20;
				if ($('#scenecontent #timerparamstable #ChkSun').is(":checked"))
					tsettings.days|=0x40;
			}
			if (tsettings.cmd==0)
			{
				if ($.isDimmer) {
					tsettings.level=$("#scenecontent #timerparamstable #combolevel").val();
				}
			}
			return tsettings;
		}

		UpdateTimer = function(idx)
		{
			var tsettings=GetTimerSettings();
			if (tsettings.timertype==5) {
				if (tsettings.date=="") {
					ShowNotify($.t('Please select a Date!'), 2500, true);
					return;
				}
				//Check if date/time is valid
				var pickedDate = $("#scenecontent #timerparamstable #sdate").datepicker( 'getDate' );
				var checkDate = new Date(pickedDate.getFullYear(), pickedDate.getMonth(), pickedDate.getDate(), tsettings.hour, tsettings.min, 0, 0);
				var nowDate = new Date();
				if (checkDate<nowDate) {
					ShowNotify($.t('Invalid Date selected!'), 2500, true);
					return;
				}
			}
			else if (tsettings.days==0)
			{
				ShowNotify($.t('Please select some days!'), 2500, true);
				return;
			}
			$.ajax({
				 url: "json.htm?type=command&param=updatescenetimer&idx=" + idx + 
							"&active=" + tsettings.Active + 
							"&timertype=" + tsettings.timertype +
							"&date=" + tsettings.date +
							"&hour=" + tsettings.hour +
							"&min=" + tsettings.min +
							"&randomness=" + tsettings.Randomness +
							"&command=" + tsettings.cmd +
							"&level=" + tsettings.level +
							"&days=" + tsettings.days,
				 async: false, 
				 dataType: 'json',
				 success: function(data) {
					RefreshTimerTable($.devIdx);
				 },
				 error: function(){
						HideNotify();
						ShowNotify($.t('Problem updating timer!'), 2500, true);
				 }     
			});
		}

		AddTimer = function()
		{
			var tsettings=GetTimerSettings();
			if (tsettings.timertype==5) {
				if (tsettings.date=="") {
					ShowNotify($.t('Please select a Date!'), 2500, true);
					return;
				}
				//Check if date/time is valid
				var pickedDate = $("#scenecontent #timerparamstable #sdate").datepicker( 'getDate' );
				var checkDate = new Date(pickedDate.getFullYear(), pickedDate.getMonth(), pickedDate.getDate(), tsettings.hour, tsettings.min, 0, 0);
				var nowDate = new Date();
				if (checkDate<nowDate) {
					ShowNotify($.t('Invalid Date selected!'), 2500, true);
					return;
				}
			}
			else if (tsettings.days==0)
			{
				ShowNotify($.t('Please select some days!'), 2500, true);
				return;
			}
			$.ajax({
				 url: "json.htm?type=command&param=addscenetimer&idx=" + $.devIdx + 
							"&active=" + tsettings.Active + 
							"&timertype=" + tsettings.timertype +
							"&date=" + tsettings.date +
							"&hour=" + tsettings.hour +
							"&min=" + tsettings.min +
							"&randomness=" + tsettings.Randomness +
							"&command=" + tsettings.cmd +
							"&level=" + tsettings.level +
							"&days=" + tsettings.days,
				 async: false, 
				 dataType: 'json',
				 success: function(data) {
					RefreshTimerTable($.devIdx);
				 },
				 error: function(){
						HideNotify();
						ShowNotify($.t('Problem adding timer!'), 2500, true);
				 }     
			});
		}


		$.strPad = function(i,l,s) {
			var o = i.toString();
			if (!s) { s = '0'; }
			while (o.length < l) {
				o = s + o;
			}
			return o;
		};

		RefreshTimerTable = function(idx)
		{
		  $('#modal').show();

			$.isDimmer=false;

			$('#updelclr #timerupdate').attr("class", "btnstyle3-dis");
			$('#updelclr #timerdelete').attr("class", "btnstyle3-dis");

		  var oTable = $('#timertable').dataTable();
		  oTable.fnClearTable();
		  
		  $.ajax({
			 url: "json.htm?type=scenetimers&idx=" + idx, 
			 async: false, 
			 dataType: 'json',
			 success: function(data) {
				
			  if (typeof data.result != 'undefined') {
				$.each(data.result, function(i,item){
					var active="No";
					if (item.Active == "true")
						active="Yes";
					var Command="On";
					if (item.Cmd == 1) {
						Command="Off";
					}
					var tCommand=Command;
					if ((Command=="On") && ($.isDimmer)) {
						tCommand+=" (" + item.Level + "%)";
					}
					var DayStr = "";
					var DayStrOrig = "";
					if (item.Type!=5) {
						var dayflags = parseInt(item.Days);
						if (dayflags & 0x80)
							DayStrOrig="Everyday";
						else if (dayflags & 0x100)
							DayStrOrig="Weekdays";
						else if (dayflags & 0x200)
							DayStrOrig="Weekends";
						else {
							if (dayflags & 0x01) {
								if (DayStrOrig!="") DayStrOrig+=", ";
								DayStrOrig+="Mon";
							}
							if (dayflags & 0x02) {
								if (DayStrOrig!="") DayStrOrig+=", ";
								DayStrOrig+="Tue";
							}
							if (dayflags & 0x04) {
								if (DayStrOrig!="") DayStrOrig+=", ";
								DayStrOrig+="Wed";
							}
							if (dayflags & 0x08) {
								if (DayStrOrig!="") DayStrOrig+=", ";
								DayStrOrig+="Thu";
							}
							if (dayflags & 0x10) {
								if (DayStrOrig!="") DayStrOrig+=", ";
								DayStrOrig+="Fri";
							}
							if (dayflags & 0x20) {
								if (DayStrOrig!="") DayStrOrig+=", ";
								DayStrOrig+="Sat";
							}
							if (dayflags & 0x40) {
								if (DayStrOrig!="") DayStrOrig+=", ";
								DayStrOrig+="Sun";
							}
						}
					}
					//translate daystring
					var res = DayStrOrig.split(", ");
					$.each(res, function(i,item){
						DayStr+=$.t(item);
						if (i!=res.length-1) {
							DayStr+=", ";
						}
					});

					var rEnabled="No";
					if (item.Randomness==true) {
						rEnabled="Yes";
					}
							
				  var addId = oTable.fnAddData( {
						"DT_RowId": item.idx,
						"Command": Command,
						"Level": item.Level,
						"TType": item.Type,
						"TTypeString": $.myglobals.TimerTypesStr[item.Type],
						"Active": active,
						"Random": rEnabled,
						"OrgDayString": DayStrOrig,
						"0": $.t(active),
						"1": $.t($.myglobals.TimerTypesStr[item.Type]),
						"2": item.Date,
						"3": item.Time,
						"4": $.t(rEnabled),
						"5": $.t(tCommand),
						"6": DayStr
					} );
				});
			  }
			 }
		  });
			/* Add a click handler to the rows - this could be used as a callback */
			$("#timertable tbody").off();
			$("#timertable tbody").on( 'click', 'tr', function () {
				if ( $(this).hasClass('row_selected') ) {
						$(this).removeClass('row_selected');
						$('#updelclr #timerupdate').attr("class", "btnstyle3-dis");
						$('#updelclr #timerdelete').attr("class", "btnstyle3-dis");
				}
				else {
						var oTable = $('#timertable').dataTable();
						oTable.$('tr.row_selected').removeClass('row_selected');
						$(this).addClass('row_selected');
						$('#updelclr #timerupdate').attr("class", "btnstyle3");
						$('#updelclr #timerdelete').attr("class", "btnstyle3");
						var anSelected = fnGetSelected( oTable );
						if ( anSelected.length !== 0 ) {
							var data = oTable.fnGetData( anSelected[0] );
							var idx= data["DT_RowId"];
							$.myglobals.SelectedTimerIdx=idx;
							$("#updelclr #timerupdate").attr("href", "javascript:UpdateTimer(" + idx + ")");
							$("#updelclr #timerdelete").attr("href", "javascript:DeleteTimer(" + idx + ")");
							//update user interface with the paramters of this row
							$('#scenecontent #timerparamstable #enabled').prop('checked', (data["Active"]=="Yes") ? true : false);
							$("#scenecontent #timerparamstable #combotype").val(jQuery.inArray(data["TTypeString"], $.myglobals.TimerTypesStr));
							$("#scenecontent #timerparamstable #combotimehour").val(parseInt(data["3"].substring(0,2)));
							$("#scenecontent #timerparamstable #combotimemin").val(parseInt(data["3"].substring(3,5)));
							$('#scenecontent #timerparamstable #randomness').prop('checked', (data["Random"]=="Yes") ? true : false);
							$("#scenecontent #timerparamstable #combocommand").val(jQuery.inArray(data["Command"], $.myglobals.CommandStr));
							if ($.isDimmer) {
								$("#scenecontent #timerparamstable #combolevel").val(data["Level"]);
							}
							
							var timerType=data["TType"];
							if (timerType==5) {
								$("#scenecontent #timerparamstable #sdate").val(data["2"]);
								$("#scenecontent #timerparamstable #rdate").show();
								$("#scenecontent #timerparamstable #rnorm").hide();
							}
							else {
								$("#scenecontent #timerparamstable #rdate").hide();
								$("#scenecontent #timerparamstable #rnorm").show();
							}
							
							var disableDays=false;
							if (data["OrgDayString"]=="Everyday") {
								$("#scenecontent #timerparamstable #when_1").prop('checked', 'checked');
								disableDays=true;
							}
							else if (data["OrgDayString"]=="Weekdays") {
								$("#scenecontent #timerparamstable #when_2").prop('checked', 'checked');
								disableDays=true;
							}
							else if (data["OrgDayString"]=="Weekends") {
								$("#scenecontent #timerparamstable #when_3").prop('checked', 'checked');
								disableDays=true;
							}
							else
								$("#scenecontent #timerparamstable #when_4").prop('checked', 'checked');
								
							EnableDisableDays(data["OrgDayString"],disableDays);
						}
						
				}
			}); 
		  
		  $('#modal').hide();
		}

		ShowTimers = function(id,name,itemtype)
		{
			if (typeof $scope.mytimer != 'undefined') {
				$interval.cancel($scope.mytimer);
				$scope.mytimer = undefined;
			}
		  $.devIdx=id;
		  $.isDimmer=false;
		  
			var oTable;
			
			$('#modal').show();
		  var htmlcontent = '';
		  htmlcontent='<p><h2><span data-i18n="Name"></span>: ' + decodeURIComponent(name) + '</h2></p><br>\n';
		  
		  var sunRise="";
		  var sunSet="";
		  $.ajax({
			 url: "json.htm?type=command&param=getSunRiseSet",
			 async: false, 
			 dataType: 'json',
			 success: function(data) {
				if (typeof data.Sunrise != 'undefined') {
				  sunRise=data.Sunrise;
				  sunSet=data.Sunset;
				}
			 }
		  });
		  
		  if (sunRise!="")
		  {
			htmlcontent+=$.t('SunRise') + ': ' + sunRise + ', ' + $.t('SunSet') + ': ' + sunSet + '<br><br>\n';
		  }
		  
		  htmlcontent+=$('#edittimers').html();
		  $('#scenecontent').html(GetBackbuttonHTMLTable('ShowScenes')+htmlcontent);
		  $('#scenecontent').i18n();
		  $("#scenecontent #timerparamstable #rdate").hide();
		  $("#scenecontent #timerparamstable #rnorm").show();
		  
			var nowTemp = new Date();
			var now = new Date(nowTemp.getFullYear(), nowTemp.getMonth(), nowTemp.getDate(), 0, 0, 0, 0);
			$( "#scenecontent #sdate" ).datepicker({
				minDate: now,
				defaultDate: now,
				dateFormat: "mm/dd/yy",
				showWeek: true,
				firstDay: 1
			});
			$("#scenecontent #combotype").change(function() { 
				var timerType=$("#scenecontent #combotype").val();
				if (timerType==5) {
					$("#scenecontent #timerparamstable #rdate").show();
					$("#scenecontent #timerparamstable #rnorm").hide();
				}
				else {
					$("#scenecontent #timerparamstable #rdate").hide();
					$("#scenecontent #timerparamstable #rnorm").show();
				}
			});

		  oTable = $('#timertable').dataTable( {
						  "sDom": '<"H"lfrC>t<"F"ip>',
						  "oTableTools": {
							"sRowSelect": "single",
						  },
						  "aaSorting": [[ 2, "asc" ]],
						  "bSortClasses": false,
						  "bProcessing": true,
						  "bStateSave": true,
						  "bJQueryUI": true,
						  "aLengthMenu": [[25, 50, 100, -1], [25, 50, 100, "All"]],
						  "iDisplayLength" : 25,
						  "sPaginationType": "full_numbers",
						  language: $.DataTableLanguage
						} );
			$('#timerparamstable #combotimehour >option').remove();
			$('#timerparamstable #combotimemin >option').remove();

			if (itemtype=="Scene") {
				$("#scenecontent #timerparamstable #combocommand option[value='1']").remove();
			}
						
		  //fill hour/minute comboboxes
		  for (ii=0; ii<24; ii++)
		  {
				$('#timerparamstable #combotimehour').append($('<option></option>').val(ii).html($.strPad(ii,2)));  
		  }
		  for (ii=0; ii<60; ii++)
		  {
				$('#timerparamstable #combotimemin').append($('<option></option>').val(ii).html($.strPad(ii,2)));  
		  }
		  $("#scenecontent #timerparamstable #when_1").click(function() {
				EnableDisableDays("Everyday",true);
			});
		  $("#scenecontent #timerparamstable #when_2").click(function() {
				EnableDisableDays("Weekdays",true);
			});
		  $("#scenecontent #timerparamstable #when_3").click(function() {
				EnableDisableDays("Weekends",true);
			});
		  $("#scenecontent #timerparamstable #when_4").click(function() {
				EnableDisableDays("",false);
			});

		  $("#scenecontent #timerparamstable #combocommand").change(function() {
				var cval=$("#scenecontent #timerparamstable #combocommand").val();
				var bShowLevel=false;
				if ($.isDimmer) {
					if (cval==0) {
						bShowLevel=true;
					}
				}
				if (bShowLevel==true) {
					$("#scenecontent #LevelDiv").show();
				}
				else {
					$("#scenecontent #LevelDiv").hide();
				}
			});

			if ($.isDimmer) {
				$("#scenecontent #LevelDiv").show();
			}
			else {
				$("#scenecontent #LevelDiv").hide();
			}
		  
		  $('#modal').hide();
		  RefreshTimerTable(id);
		 }

		RefreshScenes = function()
		{
			if (typeof $scope.mytimer != 'undefined') {
				$interval.cancel($scope.mytimer);
				$scope.mytimer = undefined;
			}
			
		  var id="";
		  
		  $http({
			 url: "json.htm?type=scenes&lastupdate="+$.LastUpdateTime
			 }).success(function(data) {
				$rootScope.SetTimeAndSun(data.Sunrise,data.Sunset,data.ServerTime);
				if (typeof data.result != 'undefined') {
					if (typeof data.ActTime != 'undefined') {
						$.LastUpdateTime=parseInt(data.ActTime);
					}

					$.each(data.result, function(i,item){
						id="#scenecontent #" + item.idx;
						var obj=$(id);
						if (typeof obj != 'undefined') {
							if ($(id + " #name").html()!=item.Name) {
								$(id + " #name").html(item.Name);
							}
							var img1="";
							var img2="";
							
							var bigtext=TranslateStatusShort(item.Status);
							if (item.UsedByCamera==true) {
								var streamimg='<img src="images/webcam.png" title="' + $.t('Stream Video') +'" height="16" width="16">';
								streamurl="<a href=\"javascript:ShowCameraLiveStream('" + encodeURIComponent(item.Name) + "','" + item.CameraIdx + "')\">" + streamimg + "</a>";
								bigtext+="&nbsp;"+streamurl;
							}

							if (item.Type=="Scene") {
								img1='<img src="images/push48.png" title="Activate" onclick="SwitchScene(' + item.idx + ',\'On\',RefreshScenes,' + item.Protected +');" class="lcursor" height="48" width="48">';
							}
							else {
								var onclass="";
								var offclass="";
								if (item.Status == 'On') {
									onclass="transimg";
									offclass="";
								}
								else if (item.Status == 'Off') {
									onclass="";
									offclass="transimg";
								}

								img1='<img class="lcursor ' + onclass + '" src="images/push48.png" title="Turn On" onclick="SwitchScene(' + item.idx + ',\'On\',RefreshScenes, ' + item.Protected +');" height="48" width="48">';
								img2='<img class="lcursor ' + offclass + '"src="images/pushoff48.png" title="Turn Off" onclick="SwitchScene(' + item.idx + ',\'Off\',RefreshScenes, ' + item.Protected +');" height="48" width="48">';
								if ($(id + " #img2").html()!=img2) {
									$(id + " #img2").html(img2);
								}
								if ($(id + " #status").html()!=TranslateStatus(item.Status)) {
									$(id + " #status").html(TranslateStatus(item.Status));
									$(id + " #bigtext").html(bigtext);
								}
							}
									
							if ($(id + " #img1").html()!=img1) {
								$(id + " #img1").html(img1);
							}

							if ($(id + " #lastupdate").html()!=item.LastUpdate) {
								$(id + " #lastupdate").html(item.LastUpdate);
							}
						}
					});
				}
				$scope.mytimer=$interval(function() {
					RefreshScenes();
				}, 10000);
			 }).error(function() {
				$scope.mytimer=$interval(function() {
					RefreshScenes();
				}, 10000);
			 });
		}

		ShowScenes = function()
		{
			if (typeof $scope.mytimer != 'undefined') {
				$interval.cancel($scope.mytimer);
				$scope.mytimer = undefined;
			}
			
		  RefreshLightSwitchesComboArray();

		  var htmlcontent = '';
		  var bHaveAddedDevider = false;
		  var bAllowWidgetReorder=true;

		  var tophtm="";
		  if (permissions.hasPermission("Admin")) {
			tophtm+=
				'\t<table border="0" cellpadding="0" cellspacing="0" width="100%">' +
				'\t<tr>' +
				'\t  <td align="left" valign="top" id="timesun"></td>\n' +
				'\t  <td align="right">' +
				'\t    <a class="btnstyle" onclick="AddScene();" data-i18n="Add Scene">Add Scene</a>' +
				'\t  </td>' +
				'\t</tr>' +
				'\t</table>';
		  }

		  var i=0;
		  var j=0;

		  $.ajax({
			 url: "json.htm?type=scenes", 
			 async: false, 
			 dataType: 'json',
			 success: function(data) {
			  if (typeof data.result != 'undefined') {
				bAllowWidgetReorder=data.AllowWidgetOrdering;
				if (typeof data.ActTime != 'undefined') {
					$.LastUpdateTime=parseInt(data.ActTime);
				}
				
				$.each(data.result, function(i,item){
				  if (j % 3 == 0)
				  {
					//add devider
					if (bHaveAddedDevider == true) {
					  //close previous devider
					  htmlcontent+='</div>\n';
					}
					htmlcontent+='<div class="row divider">\n';
					bHaveAddedDevider=true;
				  }
				  var bAddTimer=true;
				  var xhtm=
						'\t<div class="span4" id="' + item.idx + '">\n' +
						'\t  <section>\n';
					  if (item.Type=="Scene") {
						xhtm+='\t    <table id="itemtablenostatus" border="0" cellpadding="0" cellspacing="0">\n';
					  }
					  else {
						xhtm+='\t    <table id="itemtabledoubleicon" border="0" cellpadding="0" cellspacing="0">\n';
					  }
						var nbackcolor="#D4E1EE";
						if (item.Protected==true) {
							nbackcolor="#A4B1EE";
						}
					  xhtm+=
						'\t    <tr>\n' +
						'\t      <td id="name" style="background-color: ' + nbackcolor + ';">' + item.Name + '</td>\n' +
						'\t      <td id="bigtext">';
						var bigtext=TranslateStatusShort(item.Status);
					  if (item.UsedByCamera==true) {
						var streamimg='<img src="images/webcam.png" title="' + $.t('Stream Video') +'" height="16" width="16">';
						streamurl="<a href=\"javascript:ShowCameraLiveStream('" + encodeURIComponent(item.Name) + "','" + item.CameraIdx + "')\">" + streamimg + "</a>";
						bigtext+="&nbsp;"+streamurl;
					  }
					  xhtm+=bigtext+'</td>\n';

					if (item.Type=="Scene") {
						xhtm+='<td id="img1"><img src="images/push48.png" title="Activate" onclick="SwitchScene(' + item.idx + ',\'On\',RefreshScenes, ' + item.Protected +');" class="lcursor" height="48" width="48"></td>\n';
						xhtm+='\t      <td id="status">&nbsp;</td>\n';
					}
					else {
						var onclass="";
						var offclass="";
						if (item.Status == 'On') {
							onclass="transimg";
							offclass="";
						}
						else if (item.Status == 'Off') {
							onclass="";
							offclass="transimg";
						}

						xhtm+='<td id="img1"><img class="lcursor ' + onclass + '" src="images/push48.png" title="Turn On" onclick="SwitchScene(' + item.idx + ',\'On\',RefreshScenes, ' + item.Protected +');" height="48" width="48"></td>\n';
						xhtm+='<td id="img2"><img class="lcursor ' + offclass + '"src="images/pushoff48.png" title="Turn Off" onclick="SwitchScene(' + item.idx + ',\'Off\',RefreshScenes, ' + item.Protected +');" height="48" width="48"></td>\n';
						xhtm+='\t      <td id="status">&nbsp;</td>\n';
					}
					xhtm+=
						'\t      <td id="lastupdate">' + item.LastUpdate + '</td>\n' +
						'\t      <td id="type">' + $.t(item.Type) +'</td>\n';
					xhtm+='\t      <td>';
				  if (item.Favorite == 0) {
					xhtm+=      
						  '<img src="images/nofavorite.png" title="' + $.t('Add to Dashboard') +'" onclick="MakeFavorite(' + item.idx + ',1);" class="lcursor">&nbsp;&nbsp;&nbsp;&nbsp;';
				  }
				  else {
					xhtm+=      
						  '<img src="images/favorite.png" title="' + $.t('Remove from Dashboard') +'" onclick="MakeFavorite(' + item.idx + ',0);" class="lcursor">&nbsp;&nbsp;&nbsp;&nbsp;';
				  }
				  if (permissions.hasPermission("Admin")) {
						xhtm+='<a class="btnsmall" onclick="EditSceneDevice(' + item.idx + ',\'' + encodeURIComponent(item.Name) + '\',' + item.HardwareID + ',\'' + item.Type + '\', ' + item.Protected + ',\'' + item.CodeDeviceName + '\', \'' + item.OnAction + '\', \'' + item.OffAction + '\');" data-i18n="Edit">Edit</a> ';
						if (bAddTimer == true) {
							if (item.Timers == "true") {
								xhtm+='<a class="btnsmall-sel" onclick="ShowTimers(' + item.idx + ',\'' + encodeURIComponent(item.Name) + '\',\'' + item.Type + '\');" data-i18n="Timers">Timers</a> ';
							}
							else {
								xhtm+='<a class="btnsmall" onclick="ShowTimers(' + item.idx + ',\'' + encodeURIComponent(item.Name) + '\',\'' + item.Type +'\');" data-i18n="Timers">Timers</a> ';
							}
						}
				  }
				  xhtm+=
						'</td>\n' +
						'\t    </tr>\n' +
						'\t    </table>\n' +
						'\t  </section>\n' +
						'\t</div>\n';
				  htmlcontent+=xhtm;
				  j+=1;
				});
			  }
			 }
		  });
		  if (bHaveAddedDevider == true) {
			//close previous devider
			htmlcontent+='</div>\n';
		  }
		  if (htmlcontent == '')
		  {
			htmlcontent='<h2>' + $.t('No Scenes defined yet...') + '</h2>';
		  }
		  $('#scenecontent').html(tophtm+htmlcontent); //tophtm+htmlcontent
		  $('#scenecontent').i18n();
		  
		  $rootScope.RefreshTimeAndSun();

			if (bAllowWidgetReorder==true) {
				if (permissions.hasPermission("Admin")) {
					if (window.myglobals.ismobileint==false) {
						$("#scenecontent .span4").draggable({
								drag: function() {
									if (typeof $scope.mytimer != 'undefined') {
										$interval.cancel($scope.mytimer);
										$scope.mytimer = undefined;
									}
									$.devIdx=$(this).attr("id");
									$(this).css("z-index", 2);
								},
								revert: true
						});
						$("#scenecontent .span4").droppable({
								drop: function() {
									var myid=$(this).attr("id");
									$.devIdx.split(' ');
									$.ajax({
										 url: "json.htm?type=command&param=switchsceneorder&idx1=" + myid + "&idx2=" + $.devIdx,
										 async: false, 
										 dataType: 'json',
										 success: function(data) {
												ShowScenes();
										 }
									});
								}
						});
					}
				}
			}
			$scope.mytimer=$interval(function() {
				RefreshScenes();
			}, 10000);
		  return false;
		}

		init();

		function init()
		{
			$.devIdx=0;
			$.myglobals = {
				TimerTypesStr : [],
				CommandStr : [],
				SelectedTimerIdx: 0
			};
			$.LightsAndSwitches = [];
			$scope.MakeGlobalConfig();

			$('#timerparamstable #combotype > option').each(function() {
						 $.myglobals.TimerTypesStr.push($(this).text());
			});
			$('#timerparamstable #combocommand > option').each(function() {
						 $.myglobals.CommandStr.push($(this).text());
			});

			$( "#dialog-addscene" ).dialog({
						autoOpen: false,
						width: 380,
						height: 200,
						modal: true,
						resizable: false,
						title: $.t("Add Scene"),
						buttons: [{
									text: $.t("Add Scene"),
									click: function() {
											var bValid = true;
											bValid = bValid && checkLength( $("#dialog-addscene #scenename"), 2, 100 );
											if ( bValid ) {
													$.pDialog=$( this );
													var SceneName=encodeURIComponent($("#dialog-addscene #scenename").val());
													var SceneType=$("#dialog-addscene #combotype").val();
													$.ajax({
														 url: "json.htm?type=addscene&name=" + SceneName + "&scenetype=" + SceneType,
														 async: false, 
														 dataType: 'json',
														 success: function(data) {
															if (typeof data.status != 'undefined') {
																if (data.status == 'OK')
																{
																	$.pDialog.dialog( "close" );
																	ShowScenes();
																}
																else {
																	ShowNotify(data.message, 3000,true);
																}
															}
														 }
													});
													
											}
										}
								  }, {
									text: $.t("Cancel"),
									click: function() {
										$( this ).dialog( "close" );
									}
								 }],
						close: function() {
							$( this ).dialog( "close" );
						}
			}).i18n();
			ShowScenes();
		};
		$scope.$on('$destroy', function(){
			if (typeof $scope.mytimer != 'undefined') {
				$interval.cancel($scope.mytimer);
				$scope.mytimer = undefined;
			}
		}); 
	} ]);
});