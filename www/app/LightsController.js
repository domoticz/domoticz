define(['app'], function (app) {
	app.controller('LightsController', [ '$scope', '$rootScope', '$location', '$http', '$interval', 'permissions', function($scope,$rootScope,$location,$http,$interval,permissions) {

		DeleteTimer = function(idx)
		{
			bootbox.confirm($.t("Are you sure to delete this timers?\n\nThis action can not be undone..."), function(result) {
				if (result==true) {
					$.ajax({
						 url: "json.htm?type=command&param=deletetimer&idx=" + idx,
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

		ClearTimers = function()
		{
			bootbox.confirm($.t("Are you sure to delete ALL timers?\n\nThis action can not be undone!"), function(result) {
				if (result==true) {
					$.ajax({
						 url: "json.htm?type=command&param=cleartimers&idx=" + $.devIdx,
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

		GetTimerSettings = function()
		{
			var tsettings = {};
			tsettings.level=100;
			tsettings.hue=0;
			tsettings.Active=$('#lightcontent #timerparamstable #enabled').is(":checked");
			tsettings.timertype=$("#lightcontent #timerparamstable #combotype").val();
			tsettings.date=$("#lightcontent #timerparamstable #sdate").val();
			tsettings.hour=$("#lightcontent #timerparamstable #combotimehour").val();
			tsettings.min=$("#lightcontent #timerparamstable #combotimemin").val();
			tsettings.Randomness=$('#lightcontent #timerparamstable #randomness').is(":checked");
			tsettings.cmd=$("#lightcontent #timerparamstable #combocommand").val();
			tsettings.days=0;
			var everyday=$("#lightcontent #timerparamstable #when_1").is(":checked");
			var weekdays=$("#lightcontent #timerparamstable #when_2").is(":checked");
			var weekends=$("#lightcontent #timerparamstable #when_3").is(":checked");
			if (everyday==true)
				tsettings.days=0x80;
			else if (weekdays==true)
				tsettings.days=0x100;
			else if (weekends==true)
				tsettings.days=0x200;
			else {
				if ($('#lightcontent #timerparamstable #ChkMon').is(":checked"))
					tsettings.days|=0x01;
				if ($('#lightcontent #timerparamstable #ChkTue').is(":checked"))
					tsettings.days|=0x02;
				if ($('#lightcontent #timerparamstable #ChkWed').is(":checked"))
					tsettings.days|=0x04;
				if ($('#lightcontent #timerparamstable #ChkThu').is(":checked"))
					tsettings.days|=0x08;
				if ($('#lightcontent #timerparamstable #ChkFri').is(":checked"))
					tsettings.days|=0x10;
				if ($('#lightcontent #timerparamstable #ChkSat').is(":checked"))
					tsettings.days|=0x20;
				if ($('#lightcontent #timerparamstable #ChkSun').is(":checked"))
					tsettings.days|=0x40;
			}
			if (tsettings.cmd==0)
			{
				if ($.bIsLED) {
					tsettings.level=$("#lightcontent #Brightness").val();
					tsettings.hue=$("#lightcontent #Hue").val();
					var bIsWhite=$('#lightcontent #ledtable #optionWhite').is(":checked")
					if (bIsWhite==true) {
						tsettings.hue=1000;
					}
				}
				else {
					if ($.isDimmer) {
						tsettings.level=$("#lightcontent #timerparamstable #combolevel").val();
					}
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
				var pickedDate = $("#lightcontent #timerparamstable #sdate").datepicker( 'getDate' );
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
				 url: "json.htm?type=command&param=updatetimer&idx=" + idx + 
							"&active=" + tsettings.Active + 
							"&timertype=" + tsettings.timertype +
							"&date=" + tsettings.date +
							"&hour=" + tsettings.hour +
							"&min=" + tsettings.min +
							"&randomness=" + tsettings.Randomness +
							"&command=" + tsettings.cmd +
							"&level=" + tsettings.level +
							"&hue=" + tsettings.hue +
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
				var pickedDate = $("#lightcontent #timerparamstable #sdate").datepicker( 'getDate' );
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
				 url: "json.htm?type=command&param=addtimer&idx=" + $.devIdx + 
							"&active=" + tsettings.Active + 
							"&timertype=" + tsettings.timertype +
							"&date=" + tsettings.date +
							"&hour=" + tsettings.hour +
							"&min=" + tsettings.min +
							"&randomness=" + tsettings.Randomness +
							"&command=" + tsettings.cmd +
							"&level=" + tsettings.level +
							"&hue=" + tsettings.hue +
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

		EnableDisableDays = function(TypeStr, bDisabled)
		{
				$('#lightcontent #timerparamstable #ChkMon').prop('checked', ((TypeStr.indexOf("Mon") >= 0)||(TypeStr=="Everyday")||(TypeStr=="Weekdays")) ? true : false);
				$('#lightcontent #timerparamstable #ChkTue').prop('checked', ((TypeStr.indexOf("Tue") >= 0)||(TypeStr=="Everyday")||(TypeStr=="Weekdays")) ? true : false);
				$('#lightcontent #timerparamstable #ChkWed').prop('checked', ((TypeStr.indexOf("Wed") >= 0)||(TypeStr=="Everyday")||(TypeStr=="Weekdays")) ? true : false);
				$('#lightcontent #timerparamstable #ChkThu').prop('checked', ((TypeStr.indexOf("Thu") >= 0)||(TypeStr=="Everyday")||(TypeStr=="Weekdays")) ? true : false);
				$('#lightcontent #timerparamstable #ChkFri').prop('checked', ((TypeStr.indexOf("Fri") >= 0)||(TypeStr=="Everyday")||(TypeStr=="Weekdays")) ? true : false);
				$('#lightcontent #timerparamstable #ChkSat').prop('checked', ((TypeStr.indexOf("Sat") >= 0)||(TypeStr=="Everyday")||(TypeStr=="Weekends")) ? true : false);
				$('#lightcontent #timerparamstable #ChkSun').prop('checked', ((TypeStr.indexOf("Sun") >= 0)||(TypeStr=="Everyday")||(TypeStr=="Weekends")) ? true : false);

				$('#lightcontent #timerparamstable #ChkMon').attr('disabled', bDisabled);
				$('#lightcontent #timerparamstable #ChkTue').attr('disabled', bDisabled);
				$('#lightcontent #timerparamstable #ChkWed').attr('disabled', bDisabled);
				$('#lightcontent #timerparamstable #ChkThu').attr('disabled', bDisabled);
				$('#lightcontent #timerparamstable #ChkFri').attr('disabled', bDisabled);
				$('#lightcontent #timerparamstable #ChkSat').attr('disabled', bDisabled);
				$('#lightcontent #timerparamstable #ChkSun').attr('disabled', bDisabled);
		}

		RefreshTimerTable = function(idx)
		{
		  $('#modal').show();

			$('#updelclr #timerupdate').attr("class", "btnstyle3-dis");
			$('#updelclr #timerdelete').attr("class", "btnstyle3-dis");

		  var oTable = $('#timertable').dataTable();
		  oTable.fnClearTable();
		  
		  $.ajax({
			 url: "json.htm?type=timers&idx=" + idx, 
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
						if ($.bIsLED) {
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
							tCommand+='<div id="picker4" class="ex-color-box" style="background-color: #' + $.colpickHsbToHex(cHSB) + ';"></div>';
						}
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
						"Active": active,
						"Command": Command,
						"Level": item.Level,
						"Hue": item.Hue,
						"TType": item.Type,
						"TTypeString": $.myglobals.TimerTypesStr[item.Type],
						"Random": rEnabled,
						"Days": DayStrOrig,
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
						$('#lightcontent #timerparamstable #enabled').prop('checked', (data["Active"]=="Yes") ? true : false);
						$("#lightcontent #timerparamstable #combotype").val(jQuery.inArray(data["TTypeString"], $.myglobals.TimerTypesStr));
						$("#lightcontent #timerparamstable #combotimehour").val(parseInt(data["3"].substring(0,2)));
						$("#lightcontent #timerparamstable #combotimemin").val(parseInt(data["3"].substring(3,5)));
						$('#lightcontent #timerparamstable #randomness').prop('checked', (data["Random"]=="Yes") ? true : false);
						$("#lightcontent #timerparamstable #combocommand").val(jQuery.inArray(data["Command"], $.myglobals.CommandStr));
						var level=data["Level"];
						if ($.bIsLED) {
							$('#lightcontent #Brightness').val(level&255);
							var hue=data["Hue"];
							var sat=100;
							if (hue==1000) {
								hue=0;
								sat=0;
							}
							$('#lightcontent #Hue').val(hue);
							var cHSB=[];
							cHSB.h=hue;
							cHSB.s=sat;
							cHSB.b=level;
							
							$("#lightcontent #optionRGB").prop('checked',(sat==100));
							$("#lightcontent #optionWhite").prop('checked',!(sat==100));

							$('#lightcontent #picker').colpickSetColor(cHSB);
						}
						else if ($.isDimmer) {
							$("#lightcontent #timerparamstable #combolevel").val(level);
						}
						
						var timerType=data["TType"];
						if (timerType==5) {
							$("#lightcontent #timerparamstable #sdate").val(data["2"]);
							$("#lightcontent #timerparamstable #rdate").show();
							$("#lightcontent #timerparamstable #rnorm").hide();
						}
						else {
							$("#lightcontent #timerparamstable #rdate").hide();
							$("#lightcontent #timerparamstable #rnorm").show();
						}
						
						var disableDays=false;
						if (data["Days"]=="Everyday") {
							$("#lightcontent #timerparamstable #when_1").prop('checked', 'checked');
							disableDays=true;
						}
						else if (data["Days"]=="Weekdays") {
							$("#lightcontent #timerparamstable #when_2").prop('checked', 'checked');
							disableDays=true;
						}
						else if (data["Days"]=="Weekends") {
							$("#lightcontent #timerparamstable #when_3").prop('checked', 'checked');
							disableDays=true;
						}
						else
							$("#lightcontent #timerparamstable #when_4").prop('checked', 'checked');
							
						EnableDisableDays(data["Days"],disableDays);
					}
				}
			}); 
		  
			$rootScope.RefreshTimeAndSun();
		  
			$('#modal').hide();
		}

		ShowTimers = function (id,name, isdimmer, stype,devsubtype)
		{
			if (typeof $scope.mytimer != 'undefined') {
				$interval.cancel($scope.mytimer);
				$scope.mytimer = undefined;
			}
			$.devIdx=id;
			$.isDimmer=isdimmer;
			
			$.bIsRGBW=(devsubtype.indexOf("RGBW") >= 0);
			$.bIsLED=(devsubtype.indexOf("RGB") >= 0);
		  
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
		  
			var suntext='<div id="timesun" /><br>\n';
			htmlcontent+=suntext;
		  
			htmlcontent+=$('#edittimers').html();
			$('#lightcontent').html(GetBackbuttonHTMLTable('ShowLights')+htmlcontent);
			$('#lightcontent').i18n();
			$("#lightcontent #timerparamstable #rdate").hide();
			$("#lightcontent #timerparamstable #rnorm").show();

			$rootScope.RefreshTimeAndSun();

			var nowTemp = new Date();
			var now = new Date(nowTemp.getFullYear(), nowTemp.getMonth(), nowTemp.getDate(), 0, 0, 0, 0);
			
			$( "#lightcontent #sdate" ).datepicker({
				minDate: now,
				defaultDate: now,
				dateFormat: "mm/dd/yy",
				showWeek: true,
				firstDay: 1
			});
			$("#lightcontent #combotype").change(function() { 
				var timerType=$("#lightcontent #combotype").val();
				if (timerType==5) {
					$("#lightcontent #timerparamstable #rdate").show();
					$("#lightcontent #timerparamstable #rnorm").hide();
				}
				else {
					$("#lightcontent #timerparamstable #rdate").hide();
					$("#lightcontent #timerparamstable #rnorm").show();
				}
			});

			var sat=180;
			var cHSB=[];
			cHSB.h=128;
			cHSB.s=sat;
			cHSB.b=100;
			$('#lightcontent #Brightness').val(100);
			$('#lightcontent #Hue').val(128);
			
			if ($.bIsLED==true) {
				$("#lightcontent #LedColor").show();
			}
			else {
				$("#lightcontent #LedColor").hide();
			}
			if ($.bIsRGBW==true) {
				$("#lightcontent #optionsRGBW").show();
			}
			else {
				$("#lightcontent #optionsRGBW").hide();
			}
			$('#lightcontent #picker').colpick({
				flat:true,
				layout:'hex',
				submit:0,
				onChange:function(hsb,hex,rgb,fromSetColor) {
					if(!fromSetColor) {
						$('#lightcontent #Hue').val(hsb.h);
						$('#lightcontent #Brightness').val(hsb.b);
						var bIsWhite=(hsb.s<20);
						$("#lightcontent #optionRGB").prop('checked',!bIsWhite);
						$("#lightcontent #optionWhite").prop('checked',bIsWhite);
						clearInterval($.setColValue);
						$.setColValue = setInterval(function() { SetColValue($.devIdx,hsb.h,hsb.b, bIsWhite); }, 400);
					}
				}
			});

			$("#lightcontent #optionRGB").prop('checked',(sat==180));
			$("#lightcontent #optionWhite").prop('checked',!(sat==180));

			$('#lightcontent #picker').colpickSetColor(cHSB);

			oTable = $('#timertable').dataTable( {
			  "sDom": '<"H"lfrC>t<"F"ip>',
			  "oTableTools": {
				"sRowSelect": "single",
			  },
			  "aaSorting": [[ 0, "desc" ]],
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
						
			//fill hour/minute comboboxes
			for (ii=0; ii<24; ii++)
			{
				$('#timerparamstable #combotimehour').append($('<option></option>').val(ii).html($.strPad(ii,2)));  
			}
			for (ii=0; ii<60; ii++)
			{
				$('#timerparamstable #combotimemin').append($('<option></option>').val(ii).html($.strPad(ii,2)));  
			}
		  
			$("#lightcontent #timerparamstable #when_1").click(function() {
				EnableDisableDays("Everyday",true);
			});
			$("#lightcontent #timerparamstable #when_2").click(function() {
				EnableDisableDays("Weekdays",true);
			});
			$("#lightcontent #timerparamstable #when_3").click(function() {
				EnableDisableDays("Weekends",true);
			});
			$("#lightcontent #timerparamstable #when_4").click(function() {
				EnableDisableDays("",false);
			});

			$("#lightcontent #timerparamstable #combocommand").change(function() {
				var cval=$("#lightcontent #timerparamstable #combocommand").val();
				var bShowLevel=false;
				if (!$.bIsLED) {
					if ($.isDimmer) {
						if (cval==0) {
							bShowLevel=true;
						}
					}
				}
				if (bShowLevel==true) {
					$("#lightcontent #LevelDiv").show();
				}
				else {
					$("#lightcontent #LevelDiv").hide();
				}
			});

			if (($.isDimmer)&&(!$.bIsLED)) {
				$("#lightcontent #LevelDiv").show();
			}
			else {
				$("#lightcontent #LevelDiv").hide();
			}
		  
			$('#modal').hide();
			RefreshTimerTable(id);
		}

		MakeFavorite = function (id,isfavorite)
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
				url: "json.htm?type=command&param=makefavorite&idx=" + id + "&isfavorite=" + isfavorite, 
				async: false, 
				dataType: 'json',
				success: function(data) {
					ShowLights();
				}
			});
		}

		DeleteLightSwitchIntern = function (bRemoveSubDevices)
		{
			$.ajax({
				 url: "json.htm?type=setused&idx=" + $.devIdx + '&name=' + encodeURIComponent($("#lightcontent #devicename").val()) + '&used=false&RemoveSubDevices=' + bRemoveSubDevices,
				 async: false, 
				 dataType: 'json',
				 success: function(data) {
						ShowLights();
				 }
			});
		}

		DeleteLightSwitch = function ()
		{
			bootbox.confirm($.t("Are you sure to remove this Light/Switch?"), function(result) {
				if (result==true) {
					var bRemoveSubDevices=true;
					if ($.isslave==true) {
							var result=false;
							$( "#dialog-confirm-delete-subs" ).dialog({
								resizable: false,
								width: 440,
								height:180,
								modal: true,
								buttons: {
									"Yes": function() {
											$( this ).dialog( "close" );
											DeleteLightSwitchIntern(true);
								},
									"No": function() {
											$( this ).dialog( "close" );
											DeleteLightSwitchIntern(false);
								}
							}
						});
					}
					else {
						DeleteLightSwitchIntern(false);
					}
				}
			});
		}

		SaveLightSwitch = function()
		{
			var bValid = true;
			bValid = bValid && checkLength( $("#lightcontent #devicename"), 2, 100 );

			var strParam1=$("#lightcontent #onaction").val();
			var strParam2=$("#lightcontent #offaction").val();
			
			var bIsProtected=$('#lightcontent #protected').is(":checked");

			if (strParam1!="") {
				if ( (strParam1.indexOf("http://") !=0) && (strParam1.indexOf("script://") !=0) ) {
					bootbox.alert($.t("Invalid ON Action!"));
					return;
				}
				else {
					if (checkLength( $("#lightcontent #onaction"), 10, 500 )==false) {
						bootbox.alert($.t("Invalid ON Action!"));
						return;
					}
				}
			}
			if (strParam2!="") {
				if ( (strParam2.indexOf("http://") !=0) && (strParam2.indexOf("script://") !=0) ) {
					bootbox.alert($.t("Invalid Off Action!"));
					return;
				}
				else {
					if (checkLength( $("#lightcontent #offaction"), 10, 500 )==false) {
						bootbox.alert($.t("Invalid Off Action!"));
						return;
					}
				}
			}

			if ( bValid ) {
				if ($.stype=="Security") {
					$.ajax({
						 url: "json.htm?type=setused&idx=" + $.devIdx +
						  '&name=' + encodeURIComponent($("#lightcontent #devicename").val()) +
						  '&strparam1=' + btoa(strParam1) +
						  '&strparam2=' + btoa(strParam2) +
						  '&protected=' + bIsProtected +
						  '&used=true',
						 async: false, 
						 dataType: 'json',
						 success: function(data) {
								ShowLights();
						 }
					});
				}
				else {
					var addjvalstr="";
					var switchtype=$("#lightcontent #comboswitchtype").val();
					if (switchtype==8) {
						addjvalstr="&addjvalue=" + $("#lightcontent #motionoffdelay").val();
					}
					else if ((switchtype==0)||(switchtype==7)||(switchtype==9)||(switchtype==11)) {
						addjvalstr="&addjvalue=" + $("#lightcontent #offdelay").val();
						addjvalstr+="&addjvalue2=" + $("#lightcontent #ondelay").val();
					}
					var CustomImage=0;
					if (switchtype==0) {
						var cval=$('#lightcontent #comboswitchicon').data('ddslick').selectedIndex;
						CustomImage=$.ddData[cval].value;
					}
					$.ajax({
						 url: "json.htm?type=setused&idx=" + $.devIdx + 
							'&name=' + encodeURIComponent($("#lightcontent #devicename").val()) + 
							'&strparam1=' + btoa(strParam1) +
							'&strparam2=' + btoa(strParam2) +
							'&protected=' + bIsProtected +
							'&switchtype=' + $("#lightcontent #comboswitchtype").val() + 
							'&customimage=' + CustomImage + 
							'&used=true' + addjvalstr ,
						 async: false, 
						 dataType: 'json',
						 success: function(data) {
								ShowLights();
						 }
					});
				}
			}
		}

		ClearSubDevices = function()
		{
			bootbox.confirm($.t("Are you sure to delete ALL Sub/Slave Devices?\n\nThis action can not be undone!"), function(result) {
				if (result==true) {
					$.ajax({
							url: "json.htm?type=command&param=deleteallsubdevices&idx=" + $.devIdx,
							async: false, 
							dataType: 'json',
							success: function(data) {
							RefreshSubDeviceTable($.devIdx);
						}
					});
				}
			});
		}

		DeleteSubDevice = function(idx)
		{
			bootbox.confirm($.t("Are you sure to delete this Sub/Slave Device?\n\nThis action can not be undone..."), function(result) {
				if (result==true) {
					$.ajax({
						 url: "json.htm?type=command&param=deletesubdevice&idx=" + idx,
						 async: false, 
						 dataType: 'json',
						 success: function(data) {
							RefreshSubDeviceTable($.devIdx);
						 }
					});
				}
			});
		}

		RefreshSubDeviceTable = function(idx)
		{
			$('#modal').show();

			$('#lightcontent #delclr #subdevicedelete').attr("class", "btnstyle3-dis");

			var oTable = $('#lightcontent #subdevicestable').dataTable();
			oTable.fnClearTable();
		  
			$.ajax({
				url: "json.htm?type=command&param=getsubdevices&idx=" + idx, 
				async: false, 
				dataType: 'json',
				success: function(data) {
					if (typeof data.result != 'undefined') {
						$.each(data.result, function(i,item){
						  var addId = oTable.fnAddData( {
												"DT_RowId": item.ID,
								"0": item.Name
											} );
						});
					}
				}
			});

			/* Add a click handler to the rows - this could be used as a callback */
			$("#lightcontent #subdevicestable tbody").off();
			$("#lightcontent #subdevicestable tbody").on( 'click', 'tr', function () {
				if ( $(this).hasClass('row_selected') ) {
					$(this).removeClass('row_selected');
					$('#lightcontent #delclr #subdevicedelete').attr("class", "btnstyle3-dis");
				}
				else {
					var oTable = $('#lightcontent #subdevicestable').dataTable();
					oTable.$('tr.row_selected').removeClass('row_selected');
					$(this).addClass('row_selected');
					$('#lightcontent #delclr #subdevicedelete').attr("class", "btnstyle3");
					
					var anSelected = fnGetSelected( oTable );
					if ( anSelected.length !== 0 ) {
						var data = oTable.fnGetData( anSelected[0] );
						var idx= data["DT_RowId"];
						$("#lightcontent #delclr #subdevicedelete").attr("href", "javascript:DeleteSubDevice(" + idx + ")");
					}
				}
			}); 

		  $('#modal').hide();
		}

		AddSubDevice = function()
		{
			var SubDeviceIdx=$("#lightcontent #combosubdevice option:selected").val();
			if (typeof SubDeviceIdx == 'undefined') {
				bootbox.alert($.t('No Sub/Slave Device Selected!'));
				return ;
			}
			$.ajax({
				url: "json.htm?type=command&param=addsubdevice&idx=" + $.devIdx + "&subidx=" + SubDeviceIdx,
				async: false, 
				dataType: 'json',
				success: function(data) {
					if (data.status == 'OK') {
						RefreshSubDeviceTable($.devIdx);
					}
					else {
						ShowNotify($.t('Problem adding Sub/Slave Device!'), 2500, true);
					}
				},
				error: function(){
					HideNotify();
					ShowNotify($.t('Problem adding Sub/Slave Device!'), 2500, true);
				}     
			});
		}

		SetColValue = function (idx,hue,brightness, isWhite)
		{
			clearInterval($.setColValue);
			if (permissions.hasPermission("Viewer")) {
				HideNotify();
				ShowNotify($.t('You do not have permission to do that!'), 2500, true);
				return;
			}
			$.ajax({
				 url: "json.htm?type=command&param=setcolbrightnessvalue&idx=" + idx + "&hue=" + hue + "&brightness=" + brightness + "&iswhite=" + isWhite,
				 async: false, 
				 dataType: 'json'
			});
		}

		appLampBrightnessUp = function()
		{
			$.ajax({
				 url: "json.htm?type=command&param=brightnessup&idx=" + $.devIdx,
				 async: false, 
				 dataType: 'json'
			});
		}

		appLampBrightnessDown = function()
		{
			$.ajax({
				 url: "json.htm?type=command&param=brightnessdown&idx=" + $.devIdx,
				 async: false, 
				 dataType: 'json'
			});
		}
		appLampDiscoUp = function()
		{
			$.ajax({
				 url: "json.htm?type=command&param=discoup&idx=" + $.devIdx,
				 async: false, 
				 dataType: 'json'
			});
		}

		appLampDiscoDown = function()
		{
			$.ajax({
				 url: "json.htm?type=command&param=discodown&idx=" + $.devIdx,
				 async: false, 
				 dataType: 'json'
			});
		}
		appLampSpeedUp = function()
		{
			$.ajax({
				 url: "json.htm?type=command&param=speedup&idx=" + $.devIdx,
				 async: false, 
				 dataType: 'json'
			});
		}
		appLampSpeedUpLong = function()
		{
			$.ajax({
				 url: "json.htm?type=command&param=speeduplong&idx=" + $.devIdx,
				 async: false, 
				 dataType: 'json'
			});
		}

		appLampSpeedDown = function()
		{
			$.ajax({
				 url: "json.htm?type=command&param=speeddown&idx=" + $.devIdx,
				 async: false, 
				 dataType: 'json'
			});
		}

		appLampWarmer = function()
		{
			$.ajax({
				 url: "json.htm?type=command&param=warmer&idx=" + $.devIdx,
				 async: false, 
				 dataType: 'json'
			});
		}

		appLampFull = function()
		{
			$.ajax({
				 url: "json.htm?type=command&param=fulllight&idx=" + $.devIdx,
				 async: false, 
				 dataType: 'json'
			});
		}

		appLampNight = function()
		{
			$.ajax({
				 url: "json.htm?type=command&param=nightlight&idx=" + $.devIdx,
				 async: false, 
				 dataType: 'json'
			});
		}

		appLampCooler = function()
		{
			$.ajax({
				 url: "json.htm?type=command&param=cooler&idx=" + $.devIdx,
				 async: false, 
				 dataType: 'json'
			});
		}

		EditLightDevice = function(idx,name,stype,switchtype,addjvalue,addjvalue2,isslave,customimage,devsubtype,strParam1,strParam2,bIsProtected)
		{
			if (typeof $scope.mytimer != 'undefined') {
				$interval.cancel($scope.mytimer);
				$scope.mytimer = undefined;
			}
			$.devIdx=idx;
			$.isslave=isslave;
			$.stype=stype;

			var oTable;
			
			$('#modal').show();
			var htmlcontent = '';
			htmlcontent+=$('#editlightswitch').html();
			$('#lightcontent').html(GetBackbuttonHTMLTable('ShowLights')+htmlcontent);
			$('#lightcontent').i18n();
			
			oTable = $('#lightcontent #subdevicestable').dataTable( {
				"sDom": '<"H"frC>t<"F"i>',
				"oTableTools": {
					"sRowSelect": "single",
				},
				"aaSorting": [[ 0, "desc" ]],
				"bSortClasses": false,
				"bProcessing": true,
				"bStateSave": true,
				"bJQueryUI": true,
				"sPaginationType": "full_numbers",
				language: $.DataTableLanguage
			});

			var sat=100;
			var cHSB=[];
			cHSB.h=128;
			cHSB.s=sat;
			cHSB.b=100;
			$('#lightcontent #Brightness').val(100);
			$('#lightcontent #Hue').val(128);
			
			$.bIsLED=(devsubtype.indexOf("RGB") >= 0);
			$.bIsRGB=(devsubtype=="RGB");
			$.bIsRGBW=(devsubtype=="RGBW");
			$.bIsWhite=(devsubtype=="White");

			if ($.bIsLED==true) {
				$("#lightcontent #LedColor").show();
			}
			else {
				$("#lightcontent #LedColor").hide();
			}
			if ($.bIsRGB==true) {
				$("#lightcontent #optionsRGB").show();
			}
			else {
				$("#lightcontent #optionsRGB").hide();
			}
			if ($.bIsRGBW==true) {
				$("#lightcontent #optionsRGBW").show();
			}
			else {
				$("#lightcontent #optionsRGBW").hide();
			}
			if ($.bIsWhite) {
				$("#lightcontent #optionsWhite").show();
			}
			else {
				$("#lightcontent #optionsWhite").hide();
			}
			$('#lightcontent #picker').colpick({
				flat:true,
				layout:'hex',
				submit:0,
				onChange:function(hsb,hex,rgb,fromSetColor) {
					if(!fromSetColor) {
						$('#lightcontent #Hue').val(hsb.h);
						$('#lightcontent #Brightness').val(hsb.b);
						var bIsWhite=(hsb.s<20);
						$("#lightcontent #optionRGB").prop('checked',!bIsWhite);
						$("#lightcontent #optionWhite").prop('checked',bIsWhite);
						clearInterval($.setColValue);
						$.setColValue = setInterval(function() { SetColValue($.devIdx,hsb.h,hsb.b, bIsWhite); }, 400);
					}
				}
			});
			$("#lightcontent #optionRGB").prop('checked',(sat==100));
			$("#lightcontent #optionWhite").prop('checked',!(sat==100));

			$('#lightcontent #picker').colpickSetColor(cHSB);

			$("#lightcontent #devicename").val(decodeURIComponent(name));
			
			if ($.stype=="Security") {
				$("#lightcontent #SwitchType").hide();
				$("#lightcontent #OnDelayDiv").hide();
				$("#lightcontent #OffDelayDiv").hide();
				$("#lightcontent #MotionDiv").hide();
				$("#lightcontent #SwitchIconDiv").hide();
				$("#lightcontent #onaction").val(atob(strParam1));
				$("#lightcontent #offaction").val(atob(strParam2));
			}
			else {
				$("#lightcontent #SwitchType").show();
				$("#lightcontent #comboswitchtype").change(function() {
					var switchtype=$("#lightcontent #comboswitchtype").val();
					$("#lightcontent #OnDelayDiv").hide();
					$("#lightcontent #OffDelayDiv").hide();
					$("#lightcontent #MotionDiv").hide();
					$("#lightcontent #SwitchIconDiv").hide();
					if (switchtype==8) {
						$("#lightcontent #MotionDiv").show();
						$("#lightcontent #motionoffdelay").val(addjvalue);
					}
					else if ((switchtype==0)||(switchtype==7)||(switchtype==9)||(switchtype==11)) {
						$("#lightcontent #OnDelayDiv").show();
						$("#lightcontent #OffDelayDiv").show();
						$("#lightcontent #offdelay").val(addjvalue);
						$("#lightcontent #ondelay").val(addjvalue2);
					}
					if (switchtype==0) {
						$("#lightcontent #SwitchIconDiv").show();
					}
				});
				
				$("#lightcontent #comboswitchtype").val(switchtype);

				$("#lightcontent #OnDelayDiv").hide();
				$("#lightcontent #OffDelayDiv").hide();
				$("#lightcontent #MotionDiv").hide();
				$("#lightcontent #SwitchIconDiv").hide();
				if (switchtype==8) {
					$("#lightcontent #MotionDiv").show();
					$("#lightcontent #motionoffdelay").val(addjvalue);
				}
				else if ((switchtype==0)||(switchtype==7)||(switchtype==9)||(switchtype==11)) {
					$("#lightcontent #OnDelayDiv").show();
					$("#lightcontent #OffDelayDiv").show();
					$("#lightcontent #offdelay").val(addjvalue);
					$("#lightcontent #ondelay").val(addjvalue2);
				}
				if (switchtype==0) {
					$("#lightcontent #SwitchIconDiv").show();
				}
				$("#lightcontent #combosubdevice").html("");
				
				$("#lightcontent #onaction").val(atob(strParam1));
				$("#lightcontent #offaction").val(atob(strParam2));
				
				$('#lightcontent #protected').prop('checked',(bIsProtected==true));
				
				$.each($.LightsAndSwitches, function(i,item){
					var option = $('<option />');
					option.attr('value', item.idx).text(item.name);
					$("#lightcontent #combosubdevice").append(option);
				});

				$('#lightcontent #comboswitchicon').ddslick({
					data: $.ddData,
					width:200,
					selectText: "Select Switch Icon",
					imagePosition:"left"
				});
				//find our custom image index and select it
				$.each($.ddData, function(i,item){
					if (item.value==customimage) {
						$('#lightcontent #comboswitchicon').ddslick('select', {index: i });
					}
				});
			}
			
			RefreshSubDeviceTable(idx);
		}

		AddManualLightDevice = function()
		{
			if (typeof $scope.mytimer != 'undefined') {
				$interval.cancel($scope.mytimer);
				$scope.mytimer = undefined;
			}

			$("#dialog-addmanuallightdevice #combosubdevice").html("");
			
			$.each($.LightsAndSwitches, function(i,item){
				var option = $('<option />');
				option.attr('value', item.idx).text(item.name);
				$("#dialog-addmanuallightdevice #combosubdevice").append(option);
			});
			$( "#dialog-addmanuallightdevice #comboswitchtype" ).val(0);
			$( "#dialog-addmanuallightdevice" ).i18n();
			$( "#dialog-addmanuallightdevice" ).dialog( "open" );
		}

		AddLightDevice = function()
		{
			if (typeof $scope.mytimer != 'undefined') {
				$interval.cancel($scope.mytimer);
				$scope.mytimer = undefined;
			}
		  
			$("#dialog-addlightdevice #combosubdevice").html("");
			$.each($.LightsAndSwitches, function(i,item){
				var option = $('<option />');
				option.attr('value', item.idx).text(item.name);
				$("#dialog-addlightdevice #combosubdevice").append(option);
			});
		  
			ShowNotify($.t('Press button on Remote...'));

			setTimeout(function() {
				var bHaveFoundDevice = false;
				var deviceID = "";
				var deviceidx = 0;
				var bIsUsed = false;
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
					  }
					}
				   }
				});
				HideNotify();

				setTimeout(function() {
					if ((bHaveFoundDevice == true) && (bIsUsed == false))
					{
						$.devIdx = deviceidx;
						$( "#dialog-addlightdevice" ).i18n();
						$( "#dialog-addlightdevice" ).dialog( "open" );
					}
					else {
						if (bIsUsed == true)
							ShowNotify($.t('Already used by') + ':<br>"' + Name +'"', 3500, true);
						else
							ShowNotify($.t('Timeout...<br>Please try again!'), 2500, true);
					}
				}, 200);
			}, 600);
		}

		RefreshHardwareComboArray = function()
		{
			$.ComboHardware = [];
			$.ajax({
				url: "json.htm?type=command&param=getmanualhardware", 
				async: false, 
				dataType: 'json',
				success: function(data) {
					if (typeof data.result != 'undefined') {
						$.each(data.result, function(i,item) {
							$.ComboHardware.push({
								idx: item.idx,
								name: item.Name
							});
						});
					}
				}
			});
		}

		RefreshGpioComboArray = function()
		{
			$.ComboGpio = [];
			$.ajax({
				url: "json.htm?type=command&param=getgpio", 
				async: false, 
				dataType: 'json',
				success: function(data) {
					if (typeof data.result != 'undefined') {
						$.each(data.result, function(i,item) {
							$.ComboGpio.push({
								idx: item.idx,
								name: item.Name
							});
						});
					}
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
						$.each(data.result, function(i,item) {
							$.LightsAndSwitches.push({
								idx: item.idx,
								name: item.Name
							});
						});
					}
				}
			});
		}

		//Evohome...
		
		SwitchModal= function(idx, name, status, refreshfunction)
		{
			clearInterval($.myglobals.refreshTimer);
			
			ShowNotify($.t('Setting Evohome ') + ' ' + $.t(name));
			
			//FIXME avoid conflicts when setting a new status while reading the status from the web gateway at the same time
			//(the status can flick back to the previous status after an update)...now implemented with script side lockout
			$.ajax({
			url: "json.htm?type=command&param=switchmodal" + 
						"&idx=" + idx + 
						"&status=" + status +
						"&action=1",
			async: false, 
			dataType: 'json',
			success: function(data) {
					if (data.status=="ERROR") {
						HideNotify();
						bootbox.alert($.t('Problem sending switch command'));
					}
			//wait 1 second
			setTimeout(function() {
				HideNotify();
				refreshfunction();
			}, 1000);
			},
			error: function(){
				HideNotify();
				alert($.t('Problem sending switch command'));
			}     
			});
		}
		
		//FIXME move this to a shared js ...see temperaturecontroller.js
		EvoDisplayTextMode = function(strstatus){
			if(strstatus=="Auto")//FIXME better way to convert?
				strstatus="Normal";
			else if(strstatus=="AutoWithEco")//FIXME better way to convert?
				strstatus="Economy";
			else if(strstatus=="DayOff")//FIXME better way to convert?
				strstatus="Day Off";
			else if(strstatus=="HeatingOff")//FIXME better way to convert?
				strstatus="Heating Off";
			return strstatus;
		}
		
		GetLightStatusText = function(item){
			if(item.SubType=="Evohome")
				return EvoDisplayTextMode(item.Status);
			else
				return item.Status;
		}
		
		EvohomeAddJS = function()
		{
			  return "<script type='text/javascript'> function deselect(e,id) { $(id).slideFadeToggle('swing', function() { e.removeClass('selected'); });} $.fn.slideFadeToggle = function(easing, callback) {  return this.animate({ opacity: 'toggle',height: 'toggle' }, 'fast', easing, callback);};</script>";	  
		}
		
		EvohomeImg = function(item)
		{
			//see http://www.theevohomeshop.co.uk/evohome-controller-display-icons/
			return '<div title="Quick Actions" class="'+((item.Status=="Auto") ? "evoimgnorm" : "evoimg")+'"><img src="images/evohome/'+item.Status+'.png" class="lcursor" onclick="if($(this).hasClass(\'selected\')){deselect($(this),\'#evopop_'+ item.idx +'\');}else{$(this).addClass(\'selected\');$(\'#evopop_'+ item.idx +'\').slideFadeToggle();}return false;"></div>';
		}

		EvohomePopupMenu = function(item)
		{
			var htm='\t      <td id="img"><a href="#evohome" id="evohome_'+ item.idx +'">'+EvohomeImg(item)+'</a></td>\n<div id="evopop_'+ item.idx +'" class="ui-popup ui-body-b ui-overlay-shadow ui-corner-all pop">  <ul class="ui-listview ui-listview-inset ui-corner-all ui-shadow">         <li class="ui-li-divider ui-bar-inherit ui-first-child">Choose an action</li>';
			$.each([{"name":"Normal","data":"Auto"},{"name":"Economy","data":"AutoWithEco"},{"name":"Away","data":"Away"},{"name":"Day Off","data":"DayOff"},{"name":"Custom","data":"Custom"},{"name":"Heating Off","data":"HeatingOff"}],function(idx, obj){htm+='<li><a href="#" class="ui-btn ui-btn-icon-right ui-icon-'+obj.data+'" onclick="SwitchModal(\''+item.idx+'\',\''+obj.name+'\',\''+obj.data+'\',RefreshLights);deselect($(this),\'#evopop_'+ item.idx +'\');return false;">'+obj.name+'</a></li>';});
			htm+='</ul></div>';
			return htm;
		}
		
		RefreshLights = function()
		{
			if (typeof $scope.mytimer != 'undefined') {
				$interval.cancel($scope.mytimer);
				$scope.mytimer = undefined;
			}
		  var id="";
		  
		  $.ajax({
			 url: "json.htm?type=devices&filter=light&used=true&order=Name&lastupdate="+$.LastUpdateTime+"&plan="+window.myglobals.LastPlanSelected,
			 async: false, 
			 dataType: 'json',
			 success: function(data) {
				$rootScope.SetTimeAndSun(data.Sunrise,data.Sunset,data.ServerTime);

			  if (typeof data.result != 'undefined') {

				if (typeof data.ActTime != 'undefined') {
					$.LastUpdateTime=parseInt(data.ActTime);
				}
				
				$.each(data.result, function(i,item){
					id="#lightcontent #" + item.idx;
					var obj=$(id);
					if (typeof obj != 'undefined') {
						if ($(id + " #name").html()!=item.Name) {
							$(id + " #name").html(item.Name);
						}
						var isdimmer=false;
						var img="";
						var img2="";
						var img3="";
						
						var bigtext=TranslateStatusShort(item.Status);
						if (item.UsedByCamera==true) {
							var streamimg='<img src="images/webcam.png" title="' + $.t('Stream Video') +'" height="16" width="16">';
							streamurl="<a href=\"javascript:ShowCameraLiveStream('" + encodeURIComponent(item.Name) + "','" + item.CameraIdx + "')\">" + streamimg + "</a>";
							bigtext+="&nbsp;"+streamurl;
						}
						
						if (item.SubType=="Security Panel") {
							img='<a href="secpanel/"><img src="images/security48.png" class="lcursor" height="48" width="48"></a>';
						}
						else if (item.SubType=="Evohome") {
							img=EvohomeImg(item);
						}
						else if (item.SwitchType == "X10 Siren") {
							if (
									(item.Status == 'On')||
									(item.Status == 'Chime')||
									(item.Status == 'Group On')||
									(item.Status == 'All On')
								 )
							{
											img='<img src="images/siren-on.png" title="' + $.t("Turn Off") +'" onclick="SwitchLight(' + item.idx + ',\'Off\',RefreshLights,' + item.Protected +');" class="lcursor" height="48" width="48">';
							}
							else {
											img='<img src="images/siren-off.png" title="' + $.t("Turn On") + '" onclick="SwitchLight(' + item.idx + ',\'On\',RefreshLights,' + item.Protected +');" class="lcursor" height="48" width="48">';
							}
						}
						else if (item.SwitchType == "Doorbell") {
							img='<img src="images/doorbell48.png" title="' + $.t("Turn On") + '" onclick="SwitchLight(' + item.idx + ',\'On\',RefreshLights,' + item.Protected +');" class="lcursor" height="48" width="48">';
						}
						else if (item.SwitchType == "Push On Button") {
							if (item.InternalState=="On") {
								img='<img src="images/pushon48.png" title="' + $.t("Turn On") + '" onclick="SwitchLight(' + item.idx + ',\'On\',RefreshLights,' + item.Protected +');" class="lcursor" height="48" width="48">';
							}
							else {
								img='<img src="images/push48.png" title="' + $.t("Turn On") + '" onclick="SwitchLight(' + item.idx + ',\'On\',RefreshLights,' + item.Protected +');" class="lcursor" height="48" width="48">';
							}
						}
						else if (item.SwitchType == "Door Lock") {
							if (item.InternalState=="Open") {
								img='<img src="images/door48open.png" title="' + $.t("Close Door") + '" onclick="SwitchLight(' + item.idx + ',\'Off\',RefreshLights,' + item.Protected +');" class="lcursor" height="48" width="48">';
							}
							else {
								img='<img src="images/door48.png" title="' + $.t("Open Door") + '" onclick="SwitchLight(' + item.idx + ',\'On\',RefreshLights,' + item.Protected +');" class="lcursor" height="48" width="48">';
							}
						}
						else if (item.SwitchType == "Push Off Button") {
							img='<img src="images/pushoff48.png" title="' + $.t("Turn Off") +'" onclick="SwitchLight(' + item.idx + ',\'Off\',RefreshLights,' + item.Protected +');" class="lcursor" height="48" width="48">';
						}
						else if (item.SwitchType == "Contact") {
							if (item.Status == 'Closed') {
								img='<img src="images/contact48.png" height="48" width="48">';
							}
							else {
								img='<img src="images/contact48_open.png" height="48" width="48">';
							}
						}
						else if ((item.SwitchType == "Blinds")||(item.SwitchType.indexOf("Venetian Blinds") == 0)) {
							if ((item.SubType=="RAEX")||(item.SubType.indexOf('A-OK') == 0)||(item.SubType.indexOf('RollerTrol') == 0)||(item.SubType=="Harrison")||(item.SubType.indexOf('RFY') == 0)||(item.SubType.indexOf('T6 DC') == 0)||(item.SwitchType.indexOf("Venetian Blinds") == 0)) {
								if (item.Status == 'Closed') {
									img='<img src="images/blindsopen48.png" title="' + $.t("Open Blinds") + '" onclick="SwitchLight(' + item.idx + ',\'Off\',RefreshLights,' + item.Protected +');" class="lcursor" height="48" width="48">';
									img3='<img src="images/blinds48sel.png" title="' + $.t("Close Blinds") + '" onclick="SwitchLight(' + item.idx + ',\'On\',RefreshLights,' + item.Protected +');" class="lcursor" height="48" width="48">';
								}
								else {
									img='<img src="images/blindsopen48sel.png" title="' + $.t("Open Blinds") + '" onclick="SwitchLight(' + item.idx + ',\'Off\',RefreshLights,' + item.Protected +');" class="lcursor" height="48" width="48">';
									img3='<img src="images/blinds48.png" title="' + $.t("Close Blinds") + '" onclick="SwitchLight(' + item.idx + ',\'On\',RefreshLights,' + item.Protected +');" class="lcursor" height="48" width="48">';
								}
							}
							else {
								if (item.Status == 'Closed') {
									img='<img src="images/blindsopen48.png" title="' + $.t("Open Blinds") + '" onclick="SwitchLight(' + item.idx + ',\'Off\',RefreshLights,' + item.Protected +');" class="lcursor" height="48" width="48">';
									img2='<img src="images/blinds48sel.png" title="' + $.t("Close Blinds") + '" onclick="SwitchLight(' + item.idx + ',\'On\',RefreshLights,' + item.Protected +');" class="lcursor" height="48" width="48">';
								}
								else {
									img='<img src="images/blindsopen48sel.png" title="' + $.t("Open Blinds") + '" onclick="SwitchLight(' + item.idx + ',\'Off\',RefreshLights,' + item.Protected +');" class="lcursor" height="48" width="48">';
									img2='<img src="images/blinds48.png" title="' + $.t("Close Blinds") + '" onclick="SwitchLight(' + item.idx + ',\'On\',RefreshLights,' + item.Protected +');" class="lcursor" height="48" width="48">';
								}
							}
						}
						else if (item.SwitchType == "Blinds Inverted") {
							if ((item.SubType=="RAEX")||(item.SubType.indexOf('A-OK') == 0)||(item.SubType.indexOf('RollerTrol') == 0)||(item.SubType=="Harrison")||(item.SubType.indexOf('RFY') == 0)||(item.SubType.indexOf('T6 DC') == 0)) {
								if (item.Status == 'Closed') {
									img='<img src="images/blindsopen48.png" title="' + $.t("Open Blinds") + '" onclick="SwitchLight(' + item.idx + ',\'On\',RefreshLights,' + item.Protected +');" class="lcursor" height="48" width="48">';
									img3='<img src="images/blinds48sel.png" title="' + $.t("Close Blinds") + '" onclick="SwitchLight(' + item.idx + ',\'Off\',RefreshLights,' + item.Protected +');" class="lcursor" height="48" width="48">';
								}
								else {
									img='<img src="images/blindsopen48sel.png" title="' + $.t("Open Blinds") + '" onclick="SwitchLight(' + item.idx + ',\'On\',RefreshLights,' + item.Protected +');" class="lcursor" height="48" width="48">';
									img3='<img src="images/blinds48.png" title="' + $.t("Close Blinds") + '" onclick="SwitchLight(' + item.idx + ',\'Off\',RefreshLights,' + item.Protected +');" class="lcursor" height="48" width="48">';
								}
							}
							else {
								if (item.Status == 'Closed') {
									img='<img src="images/blindsopen48.png" title="' + $.t("Open Blinds") + '" onclick="SwitchLight(' + item.idx + ',\'On\',RefreshLights,' + item.Protected +');" class="lcursor" height="48" width="48">';
									img2='<img src="images/blinds48sel.png" title="' + $.t("Close Blinds") + '" onclick="SwitchLight(' + item.idx + ',\'Off\',RefreshLights,' + item.Protected +');" class="lcursor" height="48" width="48">';
								}
								else {
									img='<img src="images/blindsopen48sel.png" title="' + $.t("Open Blinds") + '" onclick="SwitchLight(' + item.idx + ',\'On\',RefreshLights,' + item.Protected +');" class="lcursor" height="48" width="48">';
									img2='<img src="images/blinds48.png" title="' + $.t("Close Blinds") + '" onclick="SwitchLight(' + item.idx + ',\'Off\',RefreshLights,' + item.Protected +');" class="lcursor" height="48" width="48">';
								}
							}
						}
						else if (item.SwitchType == "Blinds Percentage") {
							isdimmer=true;
							if (item.Status == 'Closed') {
								img='<img src="images/blindsopen48.png" title="' + $.t("Open Blinds") + '" onclick="SwitchLight(' + item.idx + ',\'Off\',RefreshLights,' + item.Protected +');" class="lcursor" height="48">';
								img2='<img src="images/blinds48sel.png" title="' + $.t("Close Blinds") + '" onclick="SwitchLight(' + item.idx + ',\'On\',RefreshLights,' + item.Protected +');" class="lcursor" height="48">';
							}
							else {
								img='<img src="images/blindsopen48sel.png" title="' + $.t("Open Blinds") + '" onclick="SwitchLight(' + item.idx + ',\'Off\',RefreshLights,' + item.Protected +');" class="lcursor" height="48">';
								img2='<img src="images/blinds48.png" title="' + $.t("Close Blinds") + '" onclick="SwitchLight(' + item.idx + ',\'On\',RefreshLights,' + item.Protected +');" class="lcursor" height="48">';
							}
						}
						else if (item.SwitchType == "Blinds Percentage Inverted") {
							isdimmer=true;
							if (item.Status == 'Closed') {
								img = '<img src="images/blindsopen48.png" title="' + $.t("Open Blinds") + '" onclick="SwitchLight(' + item.idx + ',\'On\',RefreshLights,' + item.Protected + ');" class="lcursor" height="48">';
								img2 = '<img src="images/blinds48sel.png" title="' + $.t("Close Blinds") + '" onclick="SwitchLight(' + item.idx + ',\'Off\',RefreshLights,' + item.Protected + ');" class="lcursor" height="48">';
							}
							else {
								img = '<img src="images/blindsopen48sel.png" title="' + $.t("Open Blinds") + '" onclick="SwitchLight(' + item.idx + ',\'On\',RefreshLights,' + item.Protected + ');" class="lcursor" height="48">';
								img2 = '<img src="images/blinds48.png" title="' + $.t("Close Blinds") + '" onclick="SwitchLight(' + item.idx + ',\'Off\',RefreshLights,' + item.Protected + ');" class="lcursor" height="48">';
							}
						}
						else if (item.SwitchType=="Smoke Detector") {
								if (
										(item.Status == 'Panic')||
										(item.Status == 'On')
									) {
									img='<img src="images/smoke48on.png" title="' + $.t("Turn Alarm On") + '" onclick="SwitchLight(' + item.idx + ',\'On\',RefreshLights,' + item.Protected +');" class="lcursor" height="48" width="48">';
									$(id + ' #resetbtn').attr("class", "btnsmall-sel");
								}
								else {
									img='<img src="images/smoke48off.png" title="' + $.t("Turn Alarm On") + '" onclick="SwitchLight(' + item.idx + ',\'On\',RefreshLights,' + item.Protected +');" class="lcursor" height="48" width="48">';
									$(id + ' #resetbtn').attr("class", "btnsmall-dis");
								}

						}
						else if (item.Type == "Security") {
							if (item.SubType.indexOf('remote') > 0) {
								if ((item.Status.indexOf('Arm') >= 0)||(item.Status.indexOf('Panic') >= 0)) {
									img+='<img src="images/remote48.png" title="' + $.t("Turn Alarm Off") + '" onclick="SwitchLight(' + item.idx + ',\'Off\',RefreshLights,' + item.Protected +');" class="lcursor" height="48" width="48">\n';
								}
								else {
									img+='<img src="images/remote48.png" title="' + $.t("Turn Alarm On") + '" onclick="ArmSystem(' + item.idx + ',\'On\',RefreshLights,' + item.Protected +');" class="lcursor" height="48" width="48">\n';
								}
							}
						else if (item.SubType == "X10 security") {
							if (item.Status.indexOf('Normal') >= 0) {
								img+='<img src="images/security48.png" title="' + $.t("Turn Alarm On") + '" onclick="SwitchLight(' + item.idx + ',\'' + ((item.Status == "Normal Delayed")?"Alarm Delayed":"Alarm") + '\',RefreshLights,' + item.Protected +');" class="lcursor" height="48" width="48">';
							}
							else {
								img+='<img src="images/Alarm48_On.png" title="' + $.t("Turn Alarm Off") + '" onclick="SwitchLight(' + item.idx + ',\'' + ((item.Status == "Alarm Delayed")?"Normal Delayed":"Normal") + '\',RefreshLights,' + item.Protected +');" class="lcursor" height="48" width="48">';
							}
						}
						else if (item.SubType == "X10 security motion") {
							if ((item.Status == "No Motion")) {
								img+='<img src="images/security48.png" title="' + $.t("Turn Alarm On") + '" onclick="SwitchLight(' + item.idx + ',\'Motion\',RefreshLights,' + item.Protected +');" class="lcursor" height="48" width="48">';
							}
							else {
								img+='<img src="images/Alarm48_On.png" title="' + $.t("Turn Alarm Off") + '" onclick="SwitchLight(' + item.idx + ',\'No Motion\',RefreshLights,' + item.Protected +');" class="lcursor" height="48" width="48">';
							}
						}
						else if ((item.Status.indexOf('Alarm') >= 0)||(item.Status.indexOf('Tamper') >= 0)) {
							img='<img src="images/Alarm48_On.png" height="48" width="48">';
						}
						else {
							if (item.SubType.indexOf('Meiantech') >= 0) {
								if ((item.Status.indexOf('Arm') >= 0)||(item.Status.indexOf('Panic') >= 0)) {
									img='<img src="images/security48.png" title="' + $.t("Turn Alarm Off") + '" onclick="SwitchLight(' + item.idx + ',\'Off\',RefreshLights,' + item.Protected +');" class="lcursor" height="48" width="48">';
								}
								else {
									img='<img src="images/security48.png" title="' + $.t("Turn Alarm On") + '" onclick="ArmSystemMeiantech(' + item.idx + ',\'On\',RefreshLights,' + item.Protected +');" class="lcursor" height="48" width="48">';
								}
							}
							else {
								if (item.SubType.indexOf('KeeLoq') >= 0) {
									img='<img src="images/pushon48.png" title="' + $.t("Turn On") + '" onclick="SwitchLight(' + item.idx + ',\'On\',RefreshLights,' + item.Protected +');" class="lcursor" height="48" width="48">';
								}
								else
								{
									img='<img src="images/security48.png" height="48" width="48">';
								}
							}
						}
					}
					else if (item.SwitchType == "Dimmer") {
						isdimmer=true;
						if (
								(item.Status == 'On')||
								(item.Status == 'Chime')||
								(item.Status == 'Group On')||
								(item.Status.indexOf('Set ') == 0)
							 ) {
							if (item.SubType=="RGBW") {
								img='<img src="images/RGB48_On.png" onclick="ShowRGBWPopup(event, ' + item.idx + ', \'RefreshLights\',' + item.Protected + ',' + item.MaxDimLevel + ',' + item.LevelInt + ',' + item.Hue + ');" class="lcursor" height="48" width="48">';
							}
							else {
								img='<img src="images/Dimmer48_On.png" title="' + $.t("Turn Off") +'" onclick="SwitchLight(' + item.idx + ',\'Off\',RefreshLights,' + item.Protected +');" class="lcursor" height="48" width="48">';
							}
						}
						else {
							if (item.SubType=="RGBW") {
								img='<img src="images/RGB48_Off.png" onclick="ShowRGBWPopup(event, ' + item.idx + ',\'RefreshLights\',' + item.Protected + ',' + item.MaxDimLevel + ',' + item.LevelInt + ',' + item.Hue + ');" class="lcursor" height="48" width="48">';
							}
							else {
								img='<img src="images/Dimmer48_Off.png" title="' + $.t("Turn On") + '" onclick="SwitchLight(' + item.idx + ',\'On\',RefreshLights,' + item.Protected +');" class="lcursor" height="48" width="48">';
							}
						}
					}
					else if (item.SwitchType == "TPI") {
						var RO=(item.Unit>100)?true:false;
						isdimmer=true;
						if (
								(item.Status == 'On')
							 ) {
										img='<img src="images/Fireplace48_On.png" title="' + $.t(RO?"On":"Turn Off") + (RO?'"':'" onclick="SwitchLight(' + item.idx + ',\'Off\',RefreshLights,' + item.Protected +');" class="lcursor"')+' height="48" width="48">';
						}
						else {
										img='<img src="images/Fireplace48_Off.png" title="' + $.t(RO?"Off":"Turn On") + (RO?'"':'" onclick="SwitchLight(' + item.idx + ',\'On\',RefreshLights,' + item.Protected +');" class="lcursor"') + ' height="48" width="48">';
						}
					}
					else if (item.SwitchType == "Dusk Sensor") {
						if (
								(item.Status == 'On')
							 ) {
										img='<img src="images/uvdark.png" title="' + $.t("Nighttime") +'"  height="48" width="48">';
						}
						else {
										img='<img src="images/uvsunny.png" title="' + $.t("Daytime") +'"  height="48" width="48">';
						}
					}            
					else if (item.SwitchType == "Motion Sensor") {
						if (
								(item.Status == 'On')||
								(item.Status == 'Chime')||
								(item.Status == 'Group On')||
								(item.Status.indexOf('Set ') == 0)
							 ) {
										img='<img src="images/motion48-on.png" height="48" width="48">';
						}
						else {
										img='<img src="images/motion48-off.png" height="48" width="48">';
						}
					}
					else {
						if (
							(item.Status == 'On')||
							(item.Status == 'Chime')||
							(item.Status == 'Group On')||
							(item.Status.indexOf('Down')!=-1)||
							(item.Status.indexOf('Up')!=-1)||
							(item.Status.indexOf('Set ') == 0)
						 ) {
								if (item.Type == "Thermostat 3") {
									img='<img src="images/' + item.Image + '48_On.png" onclick="ShowTherm3Popup(event, ' + item.idx + ', \'RefreshLights\',' + item.Protected + ');" class="lcursor" height="48" width="48">';
								}
								else {
									img='<img src="images/' + item.Image + '48_On.png" title="' + $.t("Turn Off") +'" onclick="SwitchLight(' + item.idx + ',\'Off\',RefreshLights,' + item.Protected +');" class="lcursor" height="48" width="48">';
								}
						}
						else {
								if (item.Type == "Thermostat 3") {
									img='<img src="images/' + item.Image + '48_Off.png" onclick="ShowTherm3Popup(event, ' + item.idx + ', \'RefreshLights\',' + item.Protected + ');" class="lcursor" height="48" width="48">';
								}
								else {
									img='<img src="images/' + item.Image + '48_Off.png" title="' + $.t("Turn On") + '" onclick="SwitchLight(' + item.idx + ',\'On\',RefreshLights,' + item.Protected +');" class="lcursor" height="48" width="48">';
								}
						}
					}
								
						var nbackcolor="#D4E1EE";
						if (item.HaveTimeout==true) {
							nbackcolor="#DF2D3A";
						}
						else if (item.Protected==true) {
							nbackcolor="#A4B1EE";
						}

						var obackcolor=rgb2hex($(id + " #name").css( "background-color" ));
						if (obackcolor!=nbackcolor) {
							$(id + " #name").css( "background-color", nbackcolor );
						}
						
						if ($(id + " #img").html()!=img) {
							$(id + " #img").html(img);
						}
						if (img2!="")
						{
							if ($(id + " #img2").html()!=img2) {
								$(id + " #img2").html(img2);
							}
						}
						if (img3!="")
						{
							if ($(id + " #img3").html()!=img3) {
								$(id + " #img3").html(img3);
							}
						}
						if (isdimmer==true) {
							var dslider=$(id + " #slider");
							if (typeof dslider != 'undefined') {
								dslider.slider( "value", item.LevelInt+1 );
							}
						}
						if ($(id + " #bigtext").html()!=TranslateStatus(GetLightStatusText(item))) {
							$(id + " #bigtext").html(bigtext);
						}
						if ($(id + " #lastupdate").html()!=item.LastUpdate) {
							$(id + " #lastupdate").html(item.LastUpdate);
						}
					}
				});
			  }
			 }
		  });
			$scope.mytimer=$interval(function() {
				RefreshLights();
			}, 10000);
		}

		ShowLights = function()
		{
			if (typeof $scope.mytimer != 'undefined') {
				$interval.cancel($scope.mytimer);
				$scope.mytimer = undefined;
			}
		  $('#modal').show();
		  
		  RefreshLightSwitchesComboArray();
		  
		  var htmlcontent = '';
			var bShowRoomplan=false;
			$.RoomPlans = [];
		  $.ajax({
			 url: "json.htm?type=plans",
			 async: false, 
			 dataType: 'json',
			 success: function(data) {
				if (typeof data.result != 'undefined') {
					var totalItems=data.result.length;
					if (totalItems>0) {
						bShowRoomplan=true;
		//				if (window.myglobals.ismobile==true) {
			//				bShowRoomplan=false;
				//		}
						if (bShowRoomplan==true) {
							$.each(data.result, function(i,item) {
								$.RoomPlans.push({
									idx: item.idx,
									name: item.Name
								});
							});
						}
					}
				}
			 }
		  });
		  
		  var bHaveAddedDevider = false;

		  var tophtm="";
		  if ($.RoomPlans.length==0) {
				tophtm+=
					'\t<table border="0" cellpadding="0" cellspacing="0" width="100%">\n' +
					'\t<tr>\n' +
					'\t  <td align="left" valign="top" id="timesun"></td>\n' +
					'\t</tr>\n' +
					'\t</table>\n';
		  }
		  else {
				tophtm+=
					'\t<table border="0" cellpadding="0" cellspacing="0" width="100%">\n' +
					'\t<tr>\n' +
					'\t  <td align="left" valign="top" id="timesun"></td>\n' +
					'<td align="right">'+
					'<span data-i18n="Room">Room</span>:&nbsp;<select id="comboroom" style="width:160px" class="combobox ui-corner-all">'+
					'<option value="0" data-i18n="All">All</option>'+
					'</select>'+
					'</td>'+
					'\t</tr>\n' +
					'\t</table>\n';
		   }
			if (permissions.hasPermission("Admin")) {
				tophtm+=
					'\t<table class="bannav" id="bannav" border="0" cellpadding="0" cellspacing="0" width="100%">\n' +
					'\t<tr>\n' +
					'\t  <td align="left"><a class="btnstylerev" onclick="AddManualLightDevice();" data-i18n="Manual Light/Switch">Manual Light/Switch</a></td>\n' +
					'\t  <td align="right"><a class="btnstyle" onclick="AddLightDevice();" data-i18n="Learn Light/Switch">Learn Light/Switch</a></td>\n' +
					'\t</tr>\n' +
					'\t</table>\n';
			}

		  var i=0;
			var j=0;

		  $.ajax({
			 url: "json.htm?type=devices&filter=light&used=true&order=Name&plan="+window.myglobals.LastPlanSelected, 
			 async: false, 
			 dataType: 'json',
			 success: function(data) {
				 
				 
			  htmlcontent+=EvohomeAddJS();

			  if (typeof data.result != 'undefined') {
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
				  var bIsDimmer=false;
				  var xhtm=
						'\t<div class="span4" id="' + item.idx + '">\n' +
						'\t  <section>\n';
					if ((item.SwitchType == "Blinds") || (item.SwitchType == "Blinds Inverted") || (item.SwitchType == "Blinds Percentage") || (item.SwitchType == "Blinds Percentage Inverted") || (item.SwitchType.indexOf("Venetian Blinds") == 0)) {
						if ((item.SubType=="RAEX")||(item.SubType.indexOf('A-OK') == 0)||(item.SubType.indexOf('RollerTrol') == 0)||(item.SubType=="Harrison")||(item.SubType.indexOf('RFY') == 0)||(item.SubType.indexOf('T6 DC') == 0)||(item.SwitchType.indexOf("Venetian Blinds") == 0)) {
							xhtm+='\t    <table id="itemtabletrippleicon" border="0" cellpadding="0" cellspacing="0">\n';
						}
						else {
							xhtm+='\t    <table id="itemtabledoubleicon" border="0" cellpadding="0" cellspacing="0">\n';
						}
					}
					else {
						xhtm+='\t    <table id="itemtablenostatus" border="0" cellpadding="0" cellspacing="0">\n';
					}
					
					var nbackcolor="#D4E1EE";
					if (item.HaveTimeout==true) {
						nbackcolor="#DF2D3A";
					}
					else if (item.Protected==true) {
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
				  if (item.SubType=="Security Panel") {
					xhtm+='\t      <td id="img"><a href="secpanel/"><img src="images/security48.png" class="lcursor" height="48" width="48"></a></td>\n';
				  }
				  else if (item.SubType=="Evohome") {
					xhtm+=EvohomePopupMenu(item);
				  }
				  else if (item.SwitchType == "X10 Siren") {
					if (
						(item.Status == 'On')||
						(item.Status == 'Chime')||
						(item.Status == 'Group On')||
						(item.Status == 'All On')
					   ) {
							xhtm+='\t      <td id="img"><img src="images/siren-on.png" title="' + $.t("Turn Off") +'" onclick="SwitchLight(' + item.idx + ',\'Off\',RefreshLights,' + item.Protected +');" class="lcursor" height="48" width="48"></td>\n';
					}
					else {
							xhtm+='\t      <td id="img"><img src="images/siren-off.png" title="' + $.t("Turn On") + '" onclick="SwitchLight(' + item.idx + ',\'On\',RefreshLights,' + item.Protected +');" class="lcursor" height="48" width="48"></td>\n';
					}
				  }
				  else if (item.SwitchType == "Doorbell") {
					xhtm+='\t      <td id="img"><img src="images/doorbell48.png" title="' + $.t("Turn On") + '" onclick="SwitchLight(' + item.idx + ',\'On\',RefreshLights,' + item.Protected +');" class="lcursor" height="48" width="48"></td>\n';
					bAddTimer=false;
				  }
				  else if (item.SwitchType == "Push On Button") {
					if (item.InternalState=="On") {
						xhtm+='\t      <td id="img"><img src="images/pushon48.png" title="' + $.t("Turn On") + '" onclick="SwitchLight(' + item.idx + ',\'On\',RefreshLights,' + item.Protected +');" class="lcursor" height="48" width="48"></td>\n';
					}
					else {
						xhtm+='\t      <td id="img"><img src="images/push48.png" title="' + $.t("Turn On") + '" onclick="SwitchLight(' + item.idx + ',\'On\',RefreshLights,' + item.Protected +');" class="lcursor" height="48" width="48"></td>\n';
					}
				  }
				  else if (item.SwitchType == "Door Lock") {
					if (item.InternalState=="Open") {
						xhtm+='\t      <td id="img"><img src="images/door48open.png" title="' + $.t("Close Door") + '" onclick="SwitchLight(' + item.idx + ',\'Off\',RefreshLights,' + item.Protected +');" class="lcursor" height="48" width="48"></td>\n';
					}
					else {
						xhtm+='\t      <td id="img"><img src="images/door48.png" title="' + $.t("Open Door") + '" onclick="SwitchLight(' + item.idx + ',\'On\',RefreshLights,' + item.Protected +');" class="lcursor" height="48" width="48"></td>\n';
					}
					bAddTimer=false;
				  }
				  else if (item.SwitchType == "Push Off Button") {
					xhtm+='\t      <td id="img"><img src="images/pushoff48.png" title="' + $.t("Turn Off") +'" onclick="SwitchLight(' + item.idx + ',\'Off\',RefreshLights,' + item.Protected +');" class="lcursor" height="48" width="48"></td>\n';
				  }
				  else if (item.SwitchType == "Contact") {
					if (item.Status == 'Closed') {
						xhtm+='\t      <td id="img"><img src="images/contact48.png" height="48" width="48"></td>\n';
					}
					else {
						xhtm+='\t      <td id="img"><img src="images/contact48_open.png" height="48" width="48"></td>\n';
					}
					bAddTimer=false;
				  }
				  else if ((item.SwitchType == "Blinds") || (item.SwitchType.indexOf("Venetian Blinds") == 0)) {
					if ((item.SubType=="RAEX")||(item.SubType.indexOf('A-OK') == 0)||(item.SubType.indexOf('RollerTrol') == 0)||(item.SubType=="Harrison")||(item.SubType.indexOf('RFY') == 0)||(item.SubType.indexOf('T6 DC') == 0)||(item.SwitchType.indexOf("Venetian Blinds") == 0)) {
						if (item.Status == 'Closed') {
							xhtm+='\t      <td id="img"><img src="images/blindsopen48.png" title="' + $.t("Open Blinds") + '" onclick="SwitchLight(' + item.idx + ',\'Off\',RefreshLights,' + item.Protected +');" class="lcursor" height="48" width="48"></td>\n';
							xhtm+='\t      <td id="img2"><img src="images/blindsstop.png" title="Stop Blinds" onclick="SwitchLight(' + item.idx + ',\'Stop\',RefreshLights,' + item.Protected +');" class="lcursor" height="48" width="24"></td>\n';
							xhtm+='\t      <td id="img3"><img src="images/blinds48sel.png" title="' + $.t("Close Blinds") + '" onclick="SwitchLight(' + item.idx + ',\'On\',RefreshLights,' + item.Protected +');" class="lcursor" height="48" width="48"></td>\n';
						}
						else {
							xhtm+='\t      <td id="img"><img src="images/blindsopen48sel.png" title="' + $.t("Open Blinds") + '" onclick="SwitchLight(' + item.idx + ',\'Off\',RefreshLights,' + item.Protected +');" class="lcursor" height="48" width="48"></td>\n';
							xhtm+='\t      <td id="img2"><img src="images/blindsstop.png" title="Stop Blinds" onclick="SwitchLight(' + item.idx + ',\'Stop\',RefreshLights,' + item.Protected +');" class="lcursor" height="48" width="24"></td>\n';
							xhtm+='\t      <td id="img3"><img src="images/blinds48.png" title="' + $.t("Close Blinds") + '" onclick="SwitchLight(' + item.idx + ',\'On\',RefreshLights,' + item.Protected +');" class="lcursor" height="48" width="48"></td>\n';
						}
					}
					else {
						if (item.Status == 'Closed') {
							xhtm+='\t      <td id="img"><img src="images/blindsopen48.png" title="' + $.t("Open Blinds") + '" onclick="SwitchLight(' + item.idx + ',\'Off\',RefreshLights,' + item.Protected +');" class="lcursor" height="48" width="48"></td>\n';
							xhtm+='\t      <td id="img2"><img src="images/blinds48sel.png" title="' + $.t("Close Blinds") + '" onclick="SwitchLight(' + item.idx + ',\'On\',RefreshLights,' + item.Protected +');" class="lcursor" height="48" width="48"></td>\n';
						}
						else {
							xhtm+='\t      <td id="img"><img src="images/blindsopen48sel.png" title="' + $.t("Open Blinds") + '" onclick="SwitchLight(' + item.idx + ',\'Off\',RefreshLights,' + item.Protected +');" class="lcursor" height="48" width="48"></td>\n';
							xhtm+='\t      <td id="img2"><img src="images/blinds48.png" title="' + $.t("Close Blinds") + '" onclick="SwitchLight(' + item.idx + ',\'On\',RefreshLights,' + item.Protected +');" class="lcursor" height="48" width="48"></td>\n';
						}
					}
				  }
				  else if (item.SwitchType == "Blinds Inverted") {
					if ((item.SubType=="RAEX")||(item.SubType.indexOf('A-OK') == 0)||(item.SubType.indexOf('RollerTrol') == 0)||(item.SubType=="Harrison")||(item.SubType.indexOf('RFY') == 0)||(item.SubType.indexOf('T6 DC') == 0)) {
						if (item.Status == 'Closed') {
							xhtm+='\t      <td id="img"><img src="images/blindsopen48.png" title="' + $.t("Open Blinds") + '" onclick="SwitchLight(' + item.idx + ',\'On\',RefreshLights,' + item.Protected +');" class="lcursor" height="48" width="48"></td>\n';
							xhtm+='\t      <td id="img2"><img src="images/blindsstop.png" title="Stop Blinds" onclick="SwitchLight(' + item.idx + ',\'Stop\',RefreshLights,' + item.Protected +');" class="lcursor" height="48" width="24"></td>\n';
							xhtm+='\t      <td id="img3"><img src="images/blinds48sel.png" title="' + $.t("Close Blinds") + '" onclick="SwitchLight(' + item.idx + ',\'Off\',RefreshLights,' + item.Protected +');" class="lcursor" height="48" width="48"></td>\n';
						}
						else {
							xhtm+='\t      <td id="img"><img src="images/blindsopen48sel.png" title="' + $.t("Open Blinds") + '" onclick="SwitchLight(' + item.idx + ',\'On\',RefreshLights,' + item.Protected +');" class="lcursor" height="48" width="48"></td>\n';
							xhtm+='\t      <td id="img2"><img src="images/blindsstop.png" title="Stop Blinds" onclick="SwitchLight(' + item.idx + ',\'Stop\',RefreshLights,' + item.Protected +');" class="lcursor" height="48" width="24"></td>\n';
							xhtm+='\t      <td id="img3"><img src="images/blinds48.png" title="' + $.t("Close Blinds") + '" onclick="SwitchLight(' + item.idx + ',\'Off\',RefreshLights,' + item.Protected +');" class="lcursor" height="48" width="48"></td>\n';
						}
					}
					else {
						if (item.Status == 'Closed') {
							xhtm+='\t      <td id="img"><img src="images/blindsopen48.png" title="' + $.t("Open Blinds") + '" onclick="SwitchLight(' + item.idx + ',\'On\',RefreshLights,' + item.Protected +');" class="lcursor" height="48" width="48"></td>\n';
							xhtm+='\t      <td id="img2"><img src="images/blinds48sel.png" title="' + $.t("Close Blinds") + '" onclick="SwitchLight(' + item.idx + ',\'Off\',RefreshLights,' + item.Protected +');" class="lcursor" height="48" width="48"></td>\n';
						}
						else {
							xhtm+='\t      <td id="img"><img src="images/blindsopen48sel.png" title="' + $.t("Open Blinds") + '" onclick="SwitchLight(' + item.idx + ',\'On\',RefreshLights,' + item.Protected +');" class="lcursor" height="48" width="48"></td>\n';
							xhtm+='\t      <td id="img2"><img src="images/blinds48.png" title="' + $.t("Close Blinds") + '" onclick="SwitchLight(' + item.idx + ',\'Off\',RefreshLights,' + item.Protected +');" class="lcursor" height="48" width="48"></td>\n';
						}
					}
				  }
				  else if (item.SwitchType == "Blinds Percentage") {
					bIsDimmer=true;
					if (item.Status == 'Closed') {
						xhtm+='\t      <td id="img"><img src="images/blindsopen48.png" title="' + $.t("Open Blinds") + '" onclick="SwitchLight(' + item.idx + ',\'Off\',RefreshLights,' + item.Protected +');" class="lcursor" height="48"></td>\n';
						xhtm+='\t      <td id="img2"><img src="images/blinds48sel.png" title="' + $.t("Close Blinds") + '" onclick="SwitchLight(' + item.idx + ',\'On\',RefreshLights,' + item.Protected +');" class="lcursor" height="48"></td>\n';
					}
					else {
						xhtm+='\t      <td id="img"><img src="images/blindsopen48sel.png" title="' + $.t("Open Blinds") + '" onclick="SwitchLight(' + item.idx + ',\'Off\',RefreshLights,' + item.Protected +');" class="lcursor" height="48"></td>\n';
						xhtm+='\t      <td id="img2"><img src="images/blinds48.png" title="' + $.t("Close Blinds") + '" onclick="SwitchLight(' + item.idx + ',\'On\',RefreshLights,' + item.Protected +');" class="lcursor" height="48"></td>\n';
					}
				}
							else if (item.SwitchType == "Blinds Percentage Inverted") {
								bIsDimmer = true;
								if (item.Status == 'Closed') {
									xhtm += '\t	  <td id="img"><img src="images/blindsopen48.png" title="' + $.t("Open Blinds") + '" onclick="SwitchLight(' + item.idx + ',\'On\',RefreshLights,' + item.Protected + ');" class="lcursor" height="48"></td>\n';
									xhtm += '\t	  <td id="img2"><img src="images/blinds48sel.png" title="' + $.t("Close Blinds") + '" onclick="SwitchLight(' + item.idx + ',\'Off\',RefreshLights,' + item.Protected + ');" class="lcursor" height="48"></td>\n';
								}
								else {
									xhtm += '\t	  <td id="img"><img src="images/blindsopen48sel.png" title="' + $.t("Open Blinds") + '" onclick="SwitchLight(' + item.idx + ',\'On\',RefreshLights,' + item.Protected + ');" class="lcursor" height="48"></td>\n';
									xhtm += '\t	  <td id="img2"><img src="images/blinds48.png" title="' + $.t("Close Blinds") + '" onclick="SwitchLight(' + item.idx + ',\'Off\',RefreshLights,' + item.Protected + ');" class="lcursor" height="48"></td>\n';
								}
							}
					else if (item.SwitchType=="Smoke Detector") {
						if (
								(item.Status == 'Panic')||
								(item.Status == 'On')
							 ) {
								xhtm+='\t      <td id="img"><img src="images/smoke48on.png" title="' + $.t("Turn Alarm On") + '" onclick="SwitchLight(' + item.idx + ',\'On\',RefreshLights,' + item.Protected +');" class="lcursor" height="48" width="48"></td>\n';
						}
						else {
							xhtm+='\t      <td id="img"><img src="images/smoke48off.png" title="' + $.t("Turn Alarm On") + '" onclick="SwitchLight(' + item.idx + ',\'On\',RefreshLights,' + item.Protected +');" class="lcursor" height="48" width="48"></td>\n';
						}
					}
							else if (item.Type == "Security") {
								if (item.SubType.indexOf('remote') > 0) {
									if ((item.Status.indexOf('Arm') >= 0)||(item.Status.indexOf('Panic') >= 0)) {
										xhtm+='\t      <td id="img"><img src="images/remote48.png" title="' + $.t("Turn Alarm Off") + '" onclick="SwitchLight(' + item.idx + ',\'Off\',RefreshLights,' + item.Protected +');" class="lcursor" height="48" width="48"></td>\n';
									}
									else {
										xhtm+='\t      <td id="img"><img src="images/remote48.png" title="' + $.t("Turn Alarm On") + '" onclick="ArmSystem(' + item.idx + ',\'On\',RefreshLights,' + item.Protected +');" class="lcursor" height="48" width="48"></td>\n';
									}
								}
								else if (item.SubType == "X10 security") {
									if (item.Status.indexOf('Normal') >= 0) {
										xhtm+='\t      <td id="img"><img src="images/security48.png" title="' + $.t("Turn Alarm On") + '" onclick="SwitchLight(' + item.idx + ',\'' + ((item.Status == "Normal Delayed")?"Alarm Delayed":"Alarm") + '\',RefreshLights,' + item.Protected +');" class="lcursor" height="48" width="48"></td>\n';
									}
									else {
										xhtm+='\t      <td id="img"><img src="images/Alarm48_On.png" title="' + $.t("Turn Alarm Off") + '" onclick="SwitchLight(' + item.idx + ',\'' + ((item.Status == "Alarm Delayed")?"Normal Delayed":"Normal") + '\',RefreshLights,' + item.Protected +');" class="lcursor" height="48" width="48"></td>\n';
									}
								}
								else if (item.SubType == "X10 security motion") {
									if ((item.Status == "No Motion")) {
										xhtm+='\t      <td id="img"><img src="images/security48.png" title="' + $.t("Turn Alarm On") + '" onclick="SwitchLight(' + item.idx + ',\'Motion\',RefreshLights,' + item.Protected +');" class="lcursor" height="48" width="48"></td>\n';
									}
									else {
										xhtm+='\t      <td id="img"><img src="images/Alarm48_On.png" title="' + $.t("Turn Alarm Off") + '" onclick="SwitchLight(' + item.idx + ',\'No Motion\',RefreshLights,' + item.Protected +');" class="lcursor" height="48" width="48"></td>\n';
									}
								}
								else if ((item.Status.indexOf('Alarm') >= 0)||(item.Status.indexOf('Tamper') >= 0)) {
									xhtm+='\t      <td id="img"><img src="images/Alarm48_On.png" height="48" width="48"></td>\n';
								}
								else {
									if (item.SubType.indexOf('Meiantech') >= 0) {
										if ((item.Status.indexOf('Arm') >= 0)||(item.Status.indexOf('Panic') >= 0)) {
											xhtm+='\t      <td id="img"><img src="images/security48.png" title="' + $.t("Turn Alarm Off") + '" onclick="SwitchLight(' + item.idx + ',\'Off\',RefreshLights,' + item.Protected +');" class="lcursor" height="48" width="48"></td>\n';
										}
										else {
											xhtm+='\t      <td id="img"><img src="images/security48.png" title="' + $.t("Turn Alarm On") + '" onclick="ArmSystemMeiantech(' + item.idx + ',\'On\',RefreshLights,' + item.Protected +');" class="lcursor" height="48" width="48"></td>\n';
										}
									}
									else {
										if (item.SubType.indexOf('KeeLoq') >= 0) {
												xhtm+='\t      <td id="img"><img src="images/pushon48.png" title="' + $.t("Turn On") + '" onclick="SwitchLight(' + item.idx + ',\'On\',RefreshLights,' + item.Protected +');" class="lcursor" height="48" width="48"></td>\n';
										}
										else
										{
											xhtm+='\t      <td id="img"><img src="images/security48.png" height="48" width="48"></td>\n';
										}
									}
								}
								bAddTimer=false;
							}
							else if (item.SwitchType == "Dimmer") {
								bIsDimmer=true;
								if (
									(item.Status == 'On')||
									(item.Status == 'Chime')||
									(item.Status == 'Group On')||
									(item.Status.indexOf('Set ') == 0)
								   ) {
										if (item.SubType=="RGBW") {
											xhtm+='\t      <td id="img"><img src="images/RGB48_On.png" onclick="ShowRGBWPopup(event, ' + item.idx + ', \'RefreshLights\',' + item.Protected + ',' + item.MaxDimLevel + ',' + item.LevelInt + ',' + item.Hue + ');" class="lcursor" height="48" width="48"></td>\n';
										}
										else {
											xhtm+='\t      <td id="img"><img src="images/Dimmer48_On.png" title="' + $.t("Turn Off") +'" onclick="SwitchLight(' + item.idx + ',\'Off\',\'RefreshLights\',' + item.Protected + ');" class="lcursor" height="48" width="48"></td>\n';
										}
									 }
									 else {
										if (item.SubType=="RGBW") {
											xhtm+='\t      <td id="img"><img src="images/RGB48_Off.png" onclick="ShowRGBWPopup(event, ' + item.idx + ',\'RefreshLights\',' + item.Protected + ',' + item.MaxDimLevel + ',' + item.LevelInt + ',' + item.Hue + ');" class="lcursor" height="48" width="48"></td>\n';
										}
										else {
											xhtm+='\t      <td id="img"><img src="images/Dimmer48_Off.png" title="' + $.t("Turn On") + '" onclick="SwitchLight(' + item.idx + ',\'On\',RefreshLights,' + item.Protected +');" class="lcursor" height="48" width="48"></td>\n';
										}
									 }
							}
							else if (item.SwitchType == "TPI") {
									var RO=(item.Unit>100)?true:false;
									bIsDimmer=true;
									if (item.Status == 'On')
									{
										xhtm+='\t      <td id="img"><img src="images/Fireplace48_On.png" title="' + $.t(RO?"On":"Turn Off") + (RO?'"':'" onclick="SwitchLight(' + item.idx + ',\'Off\',RefreshLights,' + item.Protected +');" class="lcursor"') + ' height="48" width="48"></td>\n';
									}
									else {
										xhtm+='\t      <td id="img"><img src="images/Fireplace48_Off.png" title="' + $.t(RO?"Off":"Turn On") + (RO?'"':'" onclick="SwitchLight(' + item.idx + ',\'On\',RefreshLights,' + item.Protected +');" class="lcursor"') + ' height="48" width="48"></td>\n';
									}
							}
							else if (item.SwitchType == "Dusk Sensor") {
								bAddTimer=false;
								if (item.Status == 'On')
									{
										xhtm+='\t      <td id="img"><img src="images/uvdark.png" title="' + $.t("Nighttime") +'  height="48" width="48"></td>\n';
									}
									else {
										xhtm+='\t      <td id="img"><img src="images/uvsunny.png" title="' + $.t("Daytime") + '  height="48" width="48"></td>\n';
									}
							}					
							else if (item.SwitchType == "Motion Sensor") {
								if (
									(item.Status == 'On')||
									(item.Status == 'Chime')||
									(item.Status == 'Group On')||
									(item.Status.indexOf('Set ') == 0)
								   ) {
										xhtm+='\t      <td id="img"><img src="images/motion48-on.png" height="48" width="48"></td>\n';
									}
									else {
										xhtm+='\t      <td id="img"><img src="images/motion48-off.png" height="48" width="48"></td>\n';
									}
									bAddTimer=false;
							}
						  else {
							if (
								(item.Status == 'On')||
								(item.Status == 'Chime')||
								(item.Status == 'Group On')||
								(item.Status.indexOf('Down')!=-1)||
								(item.Status.indexOf('Up')!=-1)||
								(item.Status.indexOf('Set ') == 0)
							   ) {
									if (item.Type == "Thermostat 3") {
										xhtm+='\t      <td id="img"><img src="images/' + item.Image + '48_On.png" onclick="ShowTherm3Popup(event, ' + item.idx + ',\'RefreshLights\',' + item.Protected +');" class="lcursor" height="48" width="48"></td>\n';
									}
									else {
										xhtm+='\t      <td id="img"><img src="images/' + item.Image + '48_On.png" title="' + $.t("Turn Off") +'" onclick="SwitchLight(' + item.idx + ',\'Off\',RefreshLights,' + item.Protected +');" class="lcursor" height="48" width="48"></td>\n';
									}
								}
								else {
									if (item.Type == "Thermostat 3") {
										xhtm+='\t      <td id="img"><img src="images/' + item.Image + '48_Off.png" onclick="ShowTherm3Popup(event, ' + item.idx + ',\'RefreshLights\',' + item.Protected +');" class="lcursor" height="48" width="48"></td>\n';
									}
									else {
										xhtm+='\t      <td id="img"><img src="images/' + item.Image + '48_Off.png" title="' + $.t("Turn On") + '" onclick="SwitchLight(' + item.idx + ',\'On\',RefreshLights,' + item.Protected +');" class="lcursor" height="48" width="48"></td>\n';
									}
								}
						  }
					xhtm+=
						'\t      <td id="status"></td>\n' +
						'\t      <td id="lastupdate">' + item.LastUpdate + '</td>\n' +
						'\t      <td id="type">' + item.Type + ', ' + item.SubType + ', ' + item.SwitchType;
					if (item.SwitchType == "Dimmer") {
						if (item.SubType=="RGBW") {
						}
						else {
							xhtm+='<br><br><div style="margin-left:60px;" class="dimslider" id="slider" data-idx="' + item.idx + '" data-type="norm" data-maxlevel="' + item.MaxDimLevel + '" data-isprotected="' + item.Protected + '" data-svalue="' + item.LevelInt + '"></div>';
						}
					}
					else if (item.SwitchType == "TPI") {
						xhtm+='<br><br><div style="margin-left:60px;" class="dimslider" id="slider" data-idx="' + item.idx + '" data-type="relay" data-maxlevel="' + item.MaxDimLevel + '" data-isprotected="' + item.Protected + '" data-svalue="' + item.LevelInt + '"';
						if(item.Unit>100)
							xhtm+=' data-disabled="true"';
						xhtm+='></div>';
					}
					else if ((item.SwitchType == "Blinds Percentage") || (item.SwitchType == "Blinds Percentage Inverted")) {
						xhtm+='<br><div style="margin-left:108px; margin-top:7px;" class="dimslider dimsmall" id="slider" data-idx="' + item.idx + '" data-type="blinds" data-maxlevel="' + item.MaxDimLevel + '" data-isprotected="' + item.Protected + '" data-svalue="' + item.LevelInt + '"></div>';
					}
					xhtm+='</td>\n' +
							'\t      <td>';
					  if (item.Favorite == 0) {
						xhtm+=      
							  '<img src="images/nofavorite.png" title="' + $.t('Add to Dashboard') + '" onclick="MakeFavorite(' + item.idx + ',1);" class="lcursor">&nbsp;&nbsp;&nbsp;&nbsp;';
					  }
					  else {
						xhtm+=      
							  '<img src="images/favorite.png" title="' + $.t('Remove from Dashboard') +'" onclick="MakeFavorite(' + item.idx + ',0);" class="lcursor">&nbsp;&nbsp;&nbsp;&nbsp;';
					  }
				  xhtm+=
						'<a class="btnsmall" onclick="ShowLightLog(' + item.idx + ',\'' + encodeURIComponent(item.Name)  + '\', \'#lightcontent\', \'ShowLights\');" data-i18n="Log">Log</a> ';
				  if (permissions.hasPermission("Admin")) {
					  xhtm+=
							'<a class="btnsmall" onclick="EditLightDevice(' + item.idx + ',\'' + encodeURIComponent(item.Name) + '\', ' + '\'' + item.Type + '\', ' + item.SwitchTypeVal +', ' + item.AddjValue + ', ' + item.AddjValue2 + ', ' + item.IsSubDevice + ', ' + item.CustomImage + ', \'' + item.SubType + '\', \'' + item.StrParam1 + '\', \'' + item.StrParam2 + '\', ' + item.Protected +');" data-i18n="Edit">Edit</a> ';
								if (bAddTimer == true) {
											if (item.Timers == "true") {
												xhtm+='<a class="btnsmall-sel" onclick="ShowTimers(' + item.idx + ',\'' + encodeURIComponent(item.Name) + '\',' + bIsDimmer + ',\'' + item.Type + '\'' + ', \'' + item.SubType + '\');" data-i18n="Timers">Timers</a> ';
											}
											else {
												xhtm+='<a class="btnsmall" onclick="ShowTimers(' + item.idx + ',\'' + encodeURIComponent(item.Name) + '\',' + bIsDimmer + ',\'' + item.Type + '\'' + ', \'' + item.SubType + '\');" data-i18n="Timers">Timers</a> ';
											}
								}
								if (item.SwitchType == "Smoke Detector") {
									if (
											(item.Status == 'Panic')||
											(item.Status == 'On')
										 ) {
										xhtm+='<a id="resetbtn" class="btnsmall-sel" onclick="ResetSecurityStatus(' + item.idx + ',\'Normal\',ShowLights);" data-i18n="Reset">Reset</a> ';
									}
									else {
										xhtm+='<a id="resetbtn" class="btnsmall-dis" onclick="ResetSecurityStatus(' + item.idx + ',\'Normal\',ShowLights);" data-i18n="Reset">Reset</a> ';
									}
					  }					
					  if (item.Notifications == "true")
						xhtm+='<a class="btnsmall-sel" onclick="ShowNotifications(' + item.idx + ',\'' + encodeURIComponent(item.Name) + '\', \'#lightcontent\', \'ShowLights\');" data-i18n="Notifications">Notifications</a>';
					  else
						xhtm+='<a class="btnsmall" onclick="ShowNotifications(' + item.idx + ',\'' + encodeURIComponent(item.Name) + '\', \'#lightcontent\', \'ShowLights\');" data-i18n="Notifications">Notifications</a>';
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
			htmlcontent='<h2>' + $.t('No Lights/Switches found or added in the system...') + '</h2>';
		  }
		  $('#modal').hide();
		  $('#lightcontent').html(tophtm+htmlcontent);
		  $('#lightcontent').i18n();
			if (bShowRoomplan==true) {
				$.each($.RoomPlans, function(i,item){
					var option = $('<option />');
					option.attr('value', item.idx).text(item.name);
					$("#lightcontent #comboroom").append(option);
				});
				if (typeof window.myglobals.LastPlanSelected!= 'undefined') {
					$("#lightcontent #comboroom").val(window.myglobals.LastPlanSelected);
				}
				$("#lightcontent #comboroom").change(function() { 
					var idx = $("#lightcontent #comboroom option:selected").val();
					window.myglobals.LastPlanSelected=idx;
					ShowLights();
				});
			}

			if ($scope.config.AllowWidgetOrdering==true) {
				if (permissions.hasPermission("Admin")) {
				  if (window.myglobals.ismobileint==false) {
						$("#lightcontent .span4").draggable({
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
						$("#lightcontent .span4").droppable({
								drop: function() {
									var myid=$(this).attr("id");
									$.devIdx.split(' ');
									$.ajax({
										 url: "json.htm?type=command&param=switchdeviceorder&idx1=" + myid + "&idx2=" + $.devIdx,
										 async: false, 
										 dataType: 'json',
										 success: function(data) {
												ShowLights();
										 }
									});
								}
						});
					}
				}
			}
			$rootScope.RefreshTimeAndSun();

			//Create Dimmer Sliders
			$('#lightcontent .dimslider').slider({
				//Config
				range: "min",
				min: 1,
				max: 16,
				value: 5,

				//Slider Events
				create: function(event,ui ) {
					$( this ).slider( "option", "max", $( this ).data('maxlevel'));
					$( this ).slider( "option", "type", $( this ).data('type'));
					$( this ).slider( "option", "isprotected", $( this ).data('isprotected'));
					$( this ).slider( "value", $( this ).data('svalue')+1 );
					if($( this ).data('disabled'))
						$( this ).slider( "option", "disabled", true );
				},
				slide: function(event, ui) { //When the slider is sliding
					clearInterval($.setDimValue);
					var maxValue=$( this ).slider( "option", "max");
					var dtype=$( this ).slider( "option", "type");
					var isProtected=$( this ).slider( "option", "isprotected");
					var fPercentage=0;
					if (ui.value!=1) {
						fPercentage=parseInt((100.0/(maxValue-1))*((ui.value-1)));
					}
					var idx=$( this ).data('idx');
					id="#lightcontent #" + idx;
					var obj=$(id);
					if (typeof obj != 'undefined') {
						var img="";
						var imgname="Dimmer48_O";
						if (dtype=="relay")
							imgname="Fireplace48_O"
						var bigtext;
						if (fPercentage==0)
						{
							img='<img src="images/'+imgname+'ff.png" title="' + $.t("Turn On") + '" onclick="SwitchLight(' + idx + ',\'On\',RefreshLights,' + isProtected +');" class="lcursor" height="48" width="48">';
							bigtext="Off";
						}
						else {
							img='<img src="images/'+imgname+'n.png" title="' + $.t("Turn Off") +'" onclick="SwitchLight(' + idx + ',\'Off\',RefreshLights,' + isProtected +');" class="lcursor" height="48" width="48">';
							bigtext=fPercentage + " %";
						}
						if (dtype!="blinds") {
							if ($(id + " #img").html()!=img) {
								$(id + " #img").html(img);
							}
						}
						if ($(id + " #bigtext").html()!=bigtext) {
							$(id + " #bigtext").html(bigtext);
						}
					}
					if (dtype!="relay")
						$.setDimValue = setInterval(function() { SetDimValue(idx,ui.value); }, 500);
				},
				stop: function(event, ui) {
					var idx=$( this ).data('idx');
					var dtype=$( this ).slider( "option", "type");
					if (dtype=="relay")
						SetDimValue(idx,ui.value);
				}
			});
			$scope.ResizeDimSliders();
			$scope.mytimer=$interval(function() {
				RefreshLights();
			}, 10000);
		  return false;
		}
		
		$scope.ResizeDimSliders = function()
		{
			var nobj = $("#lightcontent #name");
			if (typeof nobj == 'undefined') {
				return;
			}
			var width=nobj.width()-50;
			$("#lightcontent .dimslider").width(width);
			$("#lightcontent .dimsmall").width(width-48);
		}

		$.strPad = function(i,l,s) {
			var o = i.toString();
			if (!s) { s = '0'; }
			while (o.length < l) {
				o = s + o;
			}
			return o;
		};

		UpdateAddManualDialog = function()
		{
			var lighttype=$("#dialog-addmanuallightdevice #lighttable #combolighttype option:selected").val();
			var bIsARCType=((lighttype<20)||(lighttype==101));
			var bIsType5=0;
			
			var tothousecodes=1;
			var totunits=1;
			if ((lighttype==0)||(lighttype==1)||(lighttype==3)||(lighttype==101)) {
				tothousecodes=16;
				totunits=16;
			}
			else if ((lighttype==2)||(lighttype==5)) {
				tothousecodes=16;
				totunits=64;
			}
			else if (lighttype==4) {
				tothousecodes=3;
				totunits=4;
			}
			else if (lighttype==6) {
				tothousecodes=4;
				totunits=4;
			}
			else if (lighttype==68) {
				//Do nothing. GPIO uses a custom form
			}
			else if (lighttype==7) {
				tothousecodes=16;
				totunits=8;
			}
			else if (lighttype==8) {
				tothousecodes=16;
				totunits=4;
			}
			else if (lighttype==9) {
				tothousecodes=4;
				totunits=4;
			}
			else if (lighttype==10) {
				tothousecodes=4;
				totunits=4;
			}
			else if (lighttype==60) {
				//Blyss
				tothousecodes=16;
				totunits=5;
			}
			else if (lighttype==70) {
				//EnOcean
				tothousecodes=128;
				totunits=2;
			}
			else if (lighttype==50) {
				//LightwaveRF
				bIsType5=1;
				totunits=16;
			}
			else if ((lighttype==55)||(lighttype==57)) {
				//Livolo
				bIsType5=1;
			}
			else if (lighttype==100) {
				//Chime/ByronSX
				bIsType5=1;
				totunits=4;
			}
			else if (lighttype==102) {
				//RFY
				bIsType5=1;
				totunits=16;
			}
			else if (lighttype==103) {
				//Meiantech
				bIsType5=1;
				totunits=0;
			}
			else if ((lighttype>=200)&&(lighttype<300)) {
				//Blinds
			}
			
			$("#dialog-addmanuallightdevice #he105params").hide();
			$("#dialog-addmanuallightdevice #blindsparams").hide();
			$("#dialog-addmanuallightdevice #lightingparams_enocean").hide();
			$("#dialog-addmanuallightdevice #lightingparams_gpio").hide();

			if (lighttype==104) {
				//HE105
				$("#dialog-addmanuallightdevice #lighting1params").hide();
				$("#dialog-addmanuallightdevice #lighting2params").hide();
				$("#dialog-addmanuallightdevice #lighting3params").hide();
				$("#dialog-addmanuallightdevice #he105params").show();
			}
			else if (lighttype==60) {
				//Blyss
				$('#dialog-addmanuallightdevice #lightparams3 #combogroupcode >option').remove();
				$('#dialog-addmanuallightdevice #lightparams3 #combounitcode >option').remove();
				for (ii=0; ii<tothousecodes; ii++)
				{
					$('#dialog-addmanuallightdevice #lightparams3 #combogroupcode').append($('<option></option>').val(65+ii).html(String.fromCharCode(65+ii)));
				}
				for (ii=1; ii<totunits+1; ii++)
				{
					$('#dialog-addmanuallightdevice #lightparams3 #combounitcode').append($('<option></option>').val(ii).html(ii));
				}
				$("#dialog-addmanuallightdevice #lighting1params").hide();
				$("#dialog-addmanuallightdevice #lighting2params").hide();
				$("#dialog-addmanuallightdevice #lighting3params").show();
			}
			else if (lighttype==70) {
				//EnOcean
				$("#dialog-addmanuallightdevice #lightparams_enocean #comboid  >option").remove();
				$("#dialog-addmanuallightdevice #lightparams_enocean #combounitcode  >option").remove();
				for (ii=1; ii<tothousecodes+1; ii++)
				{
					$('#dialog-addmanuallightdevice #lightparams_enocean #comboid').append($('<option></option>').val(ii).html(ii));
				}
				for (ii=1; ii<totunits+1; ii++)
				{
					$('#dialog-addmanuallightdevice #lightparams_enocean #combounitcode').append($('<option></option>').val(ii).html(ii));
				}
				$("#dialog-addmanuallightdevice #lighting1params").hide();
				$("#dialog-addmanuallightdevice #lighting2params").hide();
				$("#dialog-addmanuallightdevice #lighting3params").hide();
				$("#dialog-addmanuallightdevice #lightingparams_enocean").show();
			}
			else if (lighttype==68) {
				//GPIO
				$("#dialog-addmanuallightdevice #lightparams_enocean #comboid  >option").remove();
				$("#dialog-addmanuallightdevice #lightparams_enocean #combounitcode  >option").remove();
				$("#dialog-addmanuallightdevice #lighting1params").hide();
				$("#dialog-addmanuallightdevice #lighting2params").hide();
				$("#dialog-addmanuallightdevice #lighting3params").hide();
				$("#dialog-addmanuallightdevice #lightingparams_gpio").show();
			}
			else if ((lighttype>=200)&&(lighttype<300)) {
				//Blinds
				$("#dialog-addmanuallightdevice #blindsparams").show();
				$("#dialog-addmanuallightdevice #lighting1params").hide();
				$("#dialog-addmanuallightdevice #lighting2params").hide();
				$("#dialog-addmanuallightdevice #lighting3params").hide();
			}
			else if (bIsARCType==1) {
				$('#dialog-addmanuallightdevice #lightparams1 #combohousecode >option').remove();
				$('#dialog-addmanuallightdevice #lightparams1 #combounitcode >option').remove();
				for (ii=0; ii<tothousecodes; ii++)
				{
					$('#dialog-addmanuallightdevice #lightparams1 #combohousecode').append($('<option></option>').val(65+ii).html(String.fromCharCode(65+ii)));
				}
				for (ii=1; ii<totunits+1; ii++)
				{
					$('#dialog-addmanuallightdevice #lightparams1 #combounitcode').append($('<option></option>').val(ii).html(ii));
				}
				$("#dialog-addmanuallightdevice #lighting1params").show();
				$("#dialog-addmanuallightdevice #lighting2params").hide();
				$("#dialog-addmanuallightdevice #lighting3params").hide();
			}
			else {
				if (lighttype==103) {
					$("#dialog-addmanuallightdevice #lighting2paramsUnitCode").hide();
				}
				else {
					$("#dialog-addmanuallightdevice #lighting2paramsUnitCode").show();
				}
				$("#dialog-addmanuallightdevice #lighting2params #combocmd2").show();
				if (bIsType5==0) {
					$("#dialog-addmanuallightdevice #lighting2params #combocmd1").show();
				}
				else {
					$("#dialog-addmanuallightdevice #lighting2params #combocmd1").hide();
					if ((lighttype==55)||(lighttype==57)||(lighttype==100)) {
						$("#dialog-addmanuallightdevice #lighting2params #combocmd2").hide();
						if (lighttype!=100) {
							$("#dialog-addmanuallightdevice #lighting2paramsUnitCode").hide();
						}
					}
				}
				$("#dialog-addmanuallightdevice #lighting1params").hide();
				$("#dialog-addmanuallightdevice #lighting2params").show();
				$("#dialog-addmanuallightdevice #lighting3params").hide();
			}
		}

		GetManualLightSettings = function(isTest)
		{
			var mParams="";
			var hwdID=$("#dialog-addmanuallightdevice #lighttable #combohardware option:selected").val();
			if (typeof hwdID == 'undefined') {
					ShowNotify($.t('No Hardware Device selected!'), 2500, true);
					return "";
			}
			mParams+="&hwdid="+hwdID;
			
			var name=$("#dialog-addmanuallightdevice #devicename");
			if ((name.val()=="")||(!checkLength(name,2,100))) {
				if (!isTest) {
					ShowNotify($.t('Invalid Name!'), 2500, true);
					return "";
				}
			}
			mParams+="&name="+encodeURIComponent(name.val());
			mParams+="&switchtype="+$("#dialog-addmanuallightdevice #lighttable #comboswitchtype option:selected").val();
			var lighttype=$("#dialog-addmanuallightdevice #lighttable #combolighttype option:selected").val();
			mParams+="&lighttype="+lighttype;
			if (lighttype==60) {
				//Blyss
				mParams+="&groupcode="+$("#dialog-addmanuallightdevice #lightparams3 #combogroupcode option:selected").val();
				mParams+="&unitcode="+$("#dialog-addmanuallightdevice #lightparams3 #combounitcode option:selected").val();
				ID=
					$("#dialog-addmanuallightdevice #lightparams3 #combocmd1 option:selected").text()+
					$("#dialog-addmanuallightdevice #lightparams3 #combocmd2 option:selected").text();
				mParams+="&id="+ID;
			}
			else if (lighttype==70) {
				//EnOcean
				mParams+="&groupcode="+$("#dialog-addmanuallightdevice #lightingparams_enocean #comboid option:selected").val();
				mParams+="&unitcode="+$("#dialog-addmanuallightdevice #lightingparams_enocean #combounitcode option:selected").val();
				ID="EnOcean";
				mParams+="&id="+ID;
			}
			else if (lighttype==68) {
				//GPIO
				mParams+="&id=GPIO&unitcode="+$("#dialog-addmanuallightdevice #lightingparams_gpio #combogpio option:selected").val();
			}
			else if ((lighttype<20)||(lighttype==101)) {
				mParams+="&housecode="+$("#dialog-addmanuallightdevice #lightparams1 #combohousecode option:selected").val();
				mParams+="&unitcode="+$("#dialog-addmanuallightdevice #lightparams1 #combounitcode option:selected").val();
			}
			else if (lighttype==104) {
				mParams+="&unitcode="+$("#dialog-addmanuallightdevice #he105params #combounitcode option:selected").text();
			}
			else if ((lighttype>=200)&&(lighttype<300)) {
				//Blinds
				ID=
					$("#dialog-addmanuallightdevice #blindsparams #combocmd1 option:selected").text()+
					$("#dialog-addmanuallightdevice #blindsparams #combocmd2 option:selected").text()+
					$("#dialog-addmanuallightdevice #blindsparams #combocmd3 option:selected").text()+
					"0"+$("#dialog-addmanuallightdevice #blindsparams #combocmd4 option:selected").text();
				mParams+="&id="+ID;
				mParams+="&unitcode="+$("#dialog-addmanuallightdevice #blindsparams #combounitcode option:selected").val();
			}
			else {
				//AC
				var ID="";
				var bIsType5=0;
				if ((lighttype==50)||(lighttype==55)||(lighttype==57)||(lighttype==100)||(lighttype==102)||(lighttype==103))
				{
					bIsType5=1;
				}
				if (bIsType5==0) {
					ID=
						$("#dialog-addmanuallightdevice #lightparams2 #combocmd1 option:selected").text()+
						$("#dialog-addmanuallightdevice #lightparams2 #combocmd2 option:selected").text()+
						$("#dialog-addmanuallightdevice #lightparams2 #combocmd3 option:selected").text()+
						$("#dialog-addmanuallightdevice #lightparams2 #combocmd4 option:selected").text();
				}
				else {
					if ((lighttype!=55)&&(lighttype!=57)&&(lighttype!=100)) {
						ID=
							$("#dialog-addmanuallightdevice #lightparams2 #combocmd2 option:selected").text()+
							$("#dialog-addmanuallightdevice #lightparams2 #combocmd3 option:selected").text()+
							$("#dialog-addmanuallightdevice #lightparams2 #combocmd4 option:selected").text();
					}
					else {
						ID=
							$("#dialog-addmanuallightdevice #lightparams2 #combocmd3 option:selected").text()+
							$("#dialog-addmanuallightdevice #lightparams2 #combocmd4 option:selected").text();
					}
				}
				if (ID=="") {
					ShowNotify($.t('Invalid Switch ID!'), 2500, true);
					return "";
				}
				mParams+="&id="+ID;
				mParams+="&unitcode="+$("#dialog-addmanuallightdevice #lightparams2 #combounitcode option:selected").val();
			}

			var bIsSubDevice=$("#dialog-addmanuallightdevice #howtable #how_2").is(":checked");
			var MainDeviceIdx="";
			if (bIsSubDevice)
			{
				var MainDeviceIdx=$("#dialog-addmanuallightdevice #combosubdevice option:selected").val();
				if (typeof MainDeviceIdx == 'undefined') {
					bootbox.alert($.t('No Main Device Selected!'));
					return "";
				}
			}
			if (MainDeviceIdx!="")
			{
				mParams+="&maindeviceidx=" + MainDeviceIdx;
			}
			
			return mParams;
		}

		TestManualLight = function()
		{
			var mParams=GetManualLightSettings(1);
			if (mParams=="") {
				return;
			}
			$.ajax({
				 url: "json.htm?type=command&param=testswitch"+mParams, 
				 async: false, 
				 dataType: 'json',
				 success: function(data) {
					if (typeof data.status != 'undefined') {
						if (data.status != 'OK')
						{
							ShowNotify($.t("There was an error!<br>Wrong switch parameters?") + ((typeof data.message != 'undefined')?"<br>" + data.message:""), 2500,true);
						}
						else {
							ShowNotify($.t("'On' command send!"), 2500);
						}
					}
				 }
			});
		}

		EnableDisableSubDevices = function(ElementID, bEnabled)
		{
			var trow=$(ElementID);
			if (bEnabled == true) {
				trow.show();
			}
			else {
				trow.hide();
			}
		}

		init();

		function init()
		{
			//global var
			$.devIdx=0;
			$.LastUpdateTime=parseInt(0);
			
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

			$(window).resize(function() { $scope.ResizeDimSliders(); });

			$( "#dialog-addlightdevice" ).dialog({
				  autoOpen: false,
				  width: 400,
				  height: 290,
				  modal: true,
				  resizable: false,
				  title: $.t("Add Light/Switch Device"),
				  buttons: {
					  "Add Device": function() {
						  var bValid = true;
						  bValid = bValid && checkLength( $("#dialog-addlightdevice #devicename"), 2, 100 );
											var bIsSubDevice=$("#dialog-addlightdevice #how_2").is(":checked");
											var MainDeviceIdx="";
											if (bIsSubDevice)
											{
												var MainDeviceIdx=$("#dialog-addlightdevice #combosubdevice option:selected").val();
												if (typeof MainDeviceIdx == 'undefined') {
													bootbox.alert($.t('No Main Device Selected!'));
													return;
												}
											}
						  
						  if ( bValid ) {
							  $( this ).dialog( "close" );
							  $.ajax({
								 url: "json.htm?type=setused&idx=" + $.devIdx + '&name=' + encodeURIComponent($("#dialog-addlightdevice #devicename").val()) + '&switchtype=' + $("#dialog-addlightdevice #comboswitchtype").val() + '&used=true&maindeviceidx=' + MainDeviceIdx,
								 async: false, 
								 dataType: 'json',
								 success: function(data) {
									ShowLights();
								 }
							  });
							  
						  }
					  },
					  Cancel: function() {
						  $( this ).dialog( "close" );
					  }
				  },
				  close: function() {
					$( this ).dialog( "close" );
				  }
			});
				$("#dialog-addlightdevice #how_1").click(function() {
					EnableDisableSubDevices("#dialog-addlightdevice #subdevice",false);
				});
				$("#dialog-addlightdevice #how_2").click(function() {
					EnableDisableSubDevices("#dialog-addlightdevice #subdevice",true);
				});

			var dialog_addmanuallightdevice_buttons = {};
			dialog_addmanuallightdevice_buttons[$.t("Add Device")]=function() {
				var mParams=GetManualLightSettings(0);
				if (mParams=="") {
					return;
				}
				$.pDialog=$( this );
				$.ajax({
					 url: "json.htm?type=command&param=addswitch"+mParams, 
					 async: false, 
					 dataType: 'json',
					 success: function(data) {
						if (typeof data.status != 'undefined') {
							if (data.status == 'OK')
							{
								$.pDialog.dialog( "close" );
								ShowLights();
							}
							else {
								ShowNotify(data.message, 3000,true);
							}
						}
					 }
				});
			};
			dialog_addmanuallightdevice_buttons[$.t("Cancel")]=function() {
				$( this ).dialog( "close" );
			};

			$( "#dialog-addmanuallightdevice" ).dialog({
				  autoOpen: false,
				  width: 440,
				  height: 480,
				  modal: true,
				  resizable: false,
  				  title: $.t("Add Manual Light/Switch Device"),
				  buttons: dialog_addmanuallightdevice_buttons,
				  open: function() {
						RefreshHardwareComboArray();
						
						$("#dialog-addmanuallightdevice #lighttable #combohardware").html("");
						$.each($.ComboHardware, function(i,item){
							var option = $('<option />');
							option.attr('value', item.idx).text(item.name);
							$("#dialog-addmanuallightdevice #lighttable #combohardware").append(option);
						});

						RefreshGpioComboArray();
						$("#combogpio").html("");
						$.each($.ComboGpio, function(i,item){
							var option = $('<option />');
							option.attr('value', item.idx).text(item.name);
							$("#combogpio").append(option);
						});

						$("#dialog-addmanuallightdevice #lighttable #comboswitchtype").change(function() { 
										var switchtype=$("#dialog-addmanuallightdevice #lighttable #comboswitchtype option:selected").val();
										if (switchtype==1)
										{
											//AC device
											$("#dialog-addmanuallightdevice #lighttable #combolighttype").val(10);
											UpdateAddManualDialog();
										}
									});
						$("#dialog-addmanuallightdevice #lighttable #combolighttype").change(function() { 
										UpdateAddManualDialog();
									});
									for (ii=0; ii<256; ii++)
									{
										$('#dialog-addmanuallightdevice #lightparams2 #combocmd2').append($('<option></option>').val(ii).html($.strPad(ii.toString(16).toUpperCase(),2)));
										$('#dialog-addmanuallightdevice #lightparams2 #combocmd3').append($('<option></option>').val(ii).html($.strPad(ii.toString(16).toUpperCase(),2)));
										$('#dialog-addmanuallightdevice #lightparams2 #combocmd4').append($('<option></option>').val(ii).html($.strPad(ii.toString(16).toUpperCase(),2)));
										$('#dialog-addmanuallightdevice #lightparams3 #combocmd1').append($('<option></option>').val(ii).html($.strPad(ii.toString(16).toUpperCase(),2)));
										$('#dialog-addmanuallightdevice #lightparams3 #combocmd2').append($('<option></option>').val(ii).html($.strPad(ii.toString(16).toUpperCase(),2)));
										$('#dialog-addmanuallightdevice #blindsparams #combocmd1').append($('<option></option>').val(ii).html($.strPad(ii.toString(16).toUpperCase(),2)));
										$('#dialog-addmanuallightdevice #blindsparams #combocmd2').append($('<option></option>').val(ii).html($.strPad(ii.toString(16).toUpperCase(),2)));
										$('#dialog-addmanuallightdevice #blindsparams #combocmd3').append($('<option></option>').val(ii).html($.strPad(ii.toString(16).toUpperCase(),2)));
									}
									$('#dialog-addmanuallightdevice #blindsparams #combounitcode >option').remove();
									for (ii=0; ii<16; ii++)
									{
										$('#dialog-addmanuallightdevice #blindsparams #combocmd4').append($('<option></option>').val(ii).html(ii.toString(16).toUpperCase()));
										$('#dialog-addmanuallightdevice #blindsparams #combounitcode').append($('<option></option>').val(ii).html(ii));
									}
									$('#dialog-addmanuallightdevice #lightparams2 #combounitcode >option').remove();
									for (ii=1; ii<16+1; ii++)
									{
										$('#dialog-addmanuallightdevice #lightparams2 #combounitcode').append($('<option></option>').val(ii).html(ii));
									}
									$('#dialog-addmanuallightdevice #he105params #combounitcode >option').remove();
									for (ii=0; ii<32; ii++)
									{
										$('#dialog-addmanuallightdevice #he105params #combounitcode').append($('<option></option>').val(ii).html($.strPad(ii.toString(16).toUpperCase(),2)));
									}
									UpdateAddManualDialog();
				  },
				  close: function() {
					$( this ).dialog( "close" );
				  }
			});

			$("#dialog-addmanuallightdevice #howtable #how_1").click(function() {
				EnableDisableSubDevices("#dialog-addmanuallightdevice #howtable #subdevice",false);
			});
			$("#dialog-addmanuallightdevice #howtable #how_2").click(function() {
				EnableDisableSubDevices("#dialog-addmanuallightdevice #howtable #subdevice",true);
			});

			$.ddData=[];
			//Get Custom icons
			$.ajax({
			 url: "json.htm?type=custom_light_icons", 
			 async: false, 
			 dataType: 'json',
			 success: function(data) {
				if (typeof data.result != 'undefined') {
					var totalItems=data.result.length;
					$.each(data.result, function(i,item){
						var bSelected=false;
						if (i==0) {
							bSelected=true;
						}
						var img="images/"+item.imageSrc+"48_On.png";
						$.ddData.push({ text: item.text, value: item.idx, selected: bSelected, description: item.description, imageSrc: img });
					});
				}
			 }
		   });

			ShowLights();

		};
		$scope.$on('$destroy', function(){
			if (typeof $scope.mytimer != 'undefined') {
				$interval.cancel($scope.mytimer);
				$scope.mytimer = undefined;
			}
			$(window).off("resize");
			var popup=$("#rgbw_popup");
			if (typeof popup != 'undefined') {
				popup.hide();
			}
			popup=$("#thermostat3_popup");
			if (typeof popup != 'undefined') {
				popup.hide();
			}
		}); 
	} ]);
});