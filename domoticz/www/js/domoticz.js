/*
 Domoticz JS v1.0.1 (2012-12-27)

 (c) 2012-2013 Domoticz.com
*/
jQuery.fn.center = function(parent) {
    if (parent) {
        parent = this.parent();
    } else {
        parent = window;
    }
    this.css({
        "position": "absolute",
        "top": ((($(parent).height() - this.outerHeight()) / 2) + $(parent).scrollTop() + "px"),
        "left": ((($(parent).width() - this.outerWidth()) / 2) + $(parent).scrollLeft() + "px")
    });
	return this;
};

/* Get the rows which are currently selected */
function fnGetSelected( oTableLocal )
{
    return oTableLocal.$('tr.row_selected');
}

function GetBackbuttonHTMLTable(backfunction)
{
  var xhtm=
        '\t<table class="bannav" id="bannav" border="0" cellpadding="0" cellspacing="0" width="100%">\n' +
        '\t<tr>\n' +
        '\t  <td>\n' +
        '\t    <a class="btnstylerev" onclick="' + backfunction + '()" data-i18n="Back">Back</a>\n' +
        '\t  </td>\n' +
        '\t</tr>\n' +
        '\t</table>\n' +
        '\t<br>\n';
  return xhtm;
}

function GetBackbuttonHTMLTableWithRight(backfunction,rightfunction,rightlabel)
{
  var xhtm=
        '\t<table class="bannav" id="bannav" border="0" cellpadding="0" cellspacing="0" width="100%">\n' +
        '\t<tr>\n' +
        '\t  <td align="left">\n' +
        '\t    <a class="btnstylerev" onclick="' + backfunction + '()" data-i18n="Back">Back</a>\n' +
        '\t  </td>\n' +
        '\t  <td align="right">\n' +
        '\t    <a class="btnstyle" onclick="' + rightfunction + '" data-i18n="'+rightlabel+'">'+rightlabel+'</a>\n' +
        '\t  </td>\n' +
        '\t</tr>\n' +
        '\t</table>\n' +
        '\t<br>\n';
  return xhtm;
}

function GetBackbuttonTransferHTMLTable(backfunction, id)
{
  var xhtm=
        '\t<table class="bannav" id="bannav" border="0" cellpadding="0" cellspacing="0" width="100%">\n' +
        '\t<tr>\n' +
        '\t  <td align="left">\n' +
        '\t    <a class="btnstylerev" onclick="' + backfunction + '()" data-i18n="Back">Back</a>\n' +
        '\t  </td>\n' +
        '\t  <td align="right">\n' +
        '\t    <a class="btnstyle" onclick="TransferSensor(' + id + ')" data-i18n="Transfer">Transfer</a>\n' +
        '\t  </td>\n' +
        '\t</tr>\n' +
        '\t</table>\n' +
        '\t<br>\n';
  return xhtm;
}

function isiPhone(){
    return (
        //Detect iPhone
        (navigator.platform.indexOf("iPhone") != -1) ||
        //Detect iPod
        (navigator.platform.indexOf("iPod") != -1)
    );
}

function ArmSystem(idx,switchcmd, refreshfunction)
{
	if (window.my_config.userrights==0) {
        HideNotify();
		ShowNotify($.i18n('You do not have permission to do that!'), 2500, true);
		return;
	}
	clearInterval($.myglobals.refreshTimer);

	$.devIdx=idx;

   var $dialog = $('<div>How would you like to Arm the System?</div>').dialog({
			modal: true,
			width: 340,
			resizable: false,
			draggable: false,
            buttons: [
                  {
                        text: $.i18n("Arm Home"),
                        click: function(){
							$dialog.remove();
							switchcmd="Arm Home";
							ShowNotify($.i18n('Switching') + ' ' + $.i18n(switchcmd));
							$.ajax({
							 url: "json.htm?type=command&param=switchlight&idx=" + $.devIdx + "&switchcmd=" + switchcmd + "&level=0",
							 async: false, 
							 dataType: 'json',
							 success: function(data) {
							  //wait 1 second
							  setTimeout(function() {
								HideNotify();
								refreshfunction();
							  }, 1000);
							 },
							 error: function(){
								HideNotify();
								alert($.i18n('Problem sending switch command'));
							 }     
							});
                        }
                  },
                  {
                        text: $.i18n("Arm Away"),
                        click: function(){
							$dialog.remove();
							switchcmd="Arm Away";
							ShowNotify($.i18n('Switching') + ' ' + $.i18n(switchcmd));
							$.ajax({
							 url: "json.htm?type=command&param=switchlight&idx=" + $.devIdx + "&switchcmd=" + switchcmd + "&level=0",
							 async: false, 
							 dataType: 'json',
							 success: function(data) {
							  //wait 1 second
							  setTimeout(function() {
								HideNotify();
								refreshfunction();
							  }, 1000);
							 },
							 error: function(){
								HideNotify();
								alert($.i18n('Problem sending switch command'));
							 }     
							});
                        }
                  }
            ]
      });
}

function SwitchLight(idx,switchcmd, refreshfunction)
{
	if (window.my_config.userrights==0) {
        HideNotify();
		ShowNotify($.i18n('You do not have permission to do that!'), 2500, true);
		return;
	}
  clearInterval($.myglobals.refreshTimer);
  ShowNotify($.i18n('Switching') + ' ' + $.i18n(switchcmd));
 
  $.ajax({
     url: "json.htm?type=command&param=switchlight&idx=" + idx + "&switchcmd=" + switchcmd + "&level=0",
     async: false, 
     dataType: 'json',
     success: function(data) {
      //wait 1 second
      setTimeout(function() {
        HideNotify();
        refreshfunction();
      }, 1000);
     },
     error: function(){
        HideNotify();
        alert($.i18n('Problem sending switch command'));
     }     
  });
}

function SwitchScene(idx,switchcmd, refreshfunction)
{
	if (window.my_config.userrights==0) {
        HideNotify();
		ShowNotify($.i18n('You do not have permission to do that!'), 2500, true);
		return;
	}

  clearInterval($.myglobals.refreshTimer);
  ShowNotify($.i18n('Switching') + ' ' + $.i18n(switchcmd));
 
  $.ajax({
     url: "json.htm?type=command&param=switchscene&idx=" + idx + "&switchcmd=" + switchcmd,
     async: false, 
     dataType: 'json',
     success: function(data) {
      //wait 1 second
      setTimeout(function() {
        HideNotify();
        refreshfunction();
      }, 1000);
     },
     error: function(){
        HideNotify();
        alert($.i18n('Problem sending switch command'));
     }     
  });
}

function ResetSecurityStatus(idx,switchcmd, refreshfunction)
{
	if (window.my_config.userrights==0) {
        HideNotify();
		ShowNotify($.i18n('You do not have permission to do that!'), 2500, true);
		return;
	}

  clearInterval($.myglobals.refreshTimer);
  ShowNotify($.i18n('Switching') + ' ' + $.i18n(switchcmd));
 
  $.ajax({
     url: "json.htm?type=command&param=resetsecuritystatus&idx=" + idx + "&switchcmd=" + switchcmd,
     async: false, 
     dataType: 'json',
     success: function(data) {
      //wait 1 second
      setTimeout(function() {
        HideNotify();
        refreshfunction();
      }, 1000);
     },
     error: function(){
        HideNotify();
        alert($.i18n('Problem sending switch command'));
     }     
  });
}

function RefreshLightLogTable(idx)
{
	var mTable = $($.content + ' #lighttable');
	var oTable = mTable.dataTable();
	oTable.fnClearTable();
  
	$.ajax({
		url: "json.htm?type=lightlog&idx=" + idx, 
		async: false, 
		dataType: 'json',
		success: function(data) {
			if (typeof data.result != 'undefined') {
				var datatable = [];
				var chart=$.LogChart.highcharts();
				var ii=0;
				$.each(data.result, function(i,item){
					var addId = oTable.fnAddData([
						  item.Date,
						  item.Data
						],false);
					var level=-1;
					if (item.Data.indexOf('Off') >= 0) {
						level=0;
					}
					else if (item.Data.indexOf('Set Level:') == 0) {
						var lstr=item.Data.substr(11);
						var idx=lstr.indexOf('%');
						if (idx!=-1) {
							lstr=lstr.substr(0,idx-1);
							level=parseInt(lstr);
						}
					}
					else {
						var idx=item.Data.indexOf('Level: ');
						if (idx!=-1)
						{
							var lstr=item.Data.substr(idx+7);
							var idx=lstr.indexOf('%');
							if (idx!=-1) {
								lstr=lstr.substr(0,idx-1);
								level=parseInt(lstr);
								if (level>100) {
									level=100;
								}
							}
						}
						else {
							if ((item.Data.indexOf('On') == 0)||(item.Data.indexOf('Group On') == 0)||(item.Data.indexOf('Open inline relay') == 0)) {
								level=100;
							}
						}
					}
					if (level!=-1) {
						datatable.unshift( [GetUTCFromStringSec(item.Date), level ] );
					}
				});
				mTable.fnDraw();
				chart.series[0].setData(datatable);
			}
		}
	});
}

function ClearLightLog()
{
	if (window.my_config.userrights!=2) {
        HideNotify();
		ShowNotify($.i18n('You do not have permission to do that!'), 2500, true);
		return;
	}
	var bValid = false;
	bValid=(confirm($.i18n("Are you sure to delete the Log?\n\nThis action can not be undone!"))==true);
	if (bValid == false)
		return;
	$.ajax({
		 url: "json.htm?type=command&param=clearlightlog&idx=" + $.devIdx,
		 async: false, 
		 dataType: 'json',
		 success: function(data) {
				RefreshLightLogTable($.devIdx);
		 },
		 error: function(){
				HideNotify();
				ShowNotify($.i18n('Problem clearing the Log!'), 2500, true);
		 }     
	});
}

function ShowLightLog(id,name,content,backfunction)
{
	clearInterval($.myglobals.refreshTimer);
	$.content=content;

	$.devIdx=id;

	$('#modal').show();
	var htmlcontent = '';
	htmlcontent='<p><h2>Name: ' + name + '</h2></p>\n';
	htmlcontent+=$('#lightlog').html();
	$($.content).html(GetBackbuttonHTMLTable(backfunction)+htmlcontent);
	$($.content).i18n();
	$.LogChart = $($.content + ' #lightgraph');
	$.LogChart.highcharts({
		chart: {
		  type: 'line',
		  zoomType: 'xy',
		  marginRight: 10
		},
		credits: {
		  enabled: true,
		  href: "http://www.domoticz.com",
		  text: "Domoticz.com"
		},
		title: {
			text: null
		},
		xAxis: {
			type: 'datetime'
		},
		yAxis: {
			title: {
				text: $.i18n('Percentage') +' (%)'
			},
			endOnTick: false,
			startOnTick: false
		},
		tooltip: {
			formatter: function() {
					return ''+
					Highcharts.dateFormat('%A<br/>%Y-%m-%d %H:%M:%S', this.x) +': '+ this.y +' %';
			}
		},
		plotOptions: {
			line: {
				lineWidth: 3,
				states: {
					hover: {
						lineWidth: 3
					}
				},
				marker: {
					enabled: false,
					states: {
						hover: {
							enabled: true,
							symbol: 'circle',
							radius: 5,
							lineWidth: 1
						}
					}
				}
			}
		},
		series: [{
			showInLegend: false,
			name: 'percent',
			step: 'left'
		}]
		,
		navigation: {
			menuItemStyle: {
				fontSize: '10px'
			}
		}
	});
  
  var oTable = $($.content + ' #lighttable').dataTable( {
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
                  "sPaginationType": "full_numbers"
                } );

	RefreshLightLogTable($.devIdx);
  $('#modal').hide();
  return false;
}

function GetNotificationSettings()
{
	var nsettings = {};

	nsettings.type=$($.content + " #notificationparamstable #combotype").val();
	var whenvisible=$($.content + " #notificationparamstable #notiwhen").is(":visible");
	if (whenvisible==false) {
		nsettings.when=0;
		nsettings.value=0;
	}
	else {
		nsettings.when=$($.content + " #notificationparamstable #combowhen").val();
		nsettings.value=$($.content + " #notificationparamstable #value").val();
		if ((nsettings.value=="")||(isNaN(nsettings.value)))
		{
			ShowNotify($.i18n('Please correct the Value!'), 2500, true);
			return null;
		}
	}
	return nsettings;
}

function ClearNotifications()
{
	var bValid = false;
	bValid=(confirm($.i18n("Are you sure to delete ALL notifications?\n\nThis action can not be undone!!"))==true);
	if (bValid == false)
		return;

	$.ajax({
		 url: "json.htm?type=command&param=clearnotifications&idx=" + $.devIdx,
		 async: false, 
		 dataType: 'json',
		 success: function(data) {
			RefreshNotificationTable($.devIdx);
		 },
		 error: function(){
				HideNotify();
				ShowNotify($.i18n('Problem clearing notifications!'), 2500, true);
		 }     
	});
}

function UpdateNotification(idx)
{
	var nsettings=GetNotificationSettings();
	if (nsettings==null) {
		return;
	}
	$.ajax({
		 url: "json.htm?type=command&param=updatenotification&idx=" + idx + 
					"&devidx=" + $.devIdx +
					"&ttype=" + nsettings.type +
					"&twhen=" + nsettings.when +
					"&tvalue=" + nsettings.value,
		 async: false, 
		 dataType: 'json',
		 success: function(data) {
			RefreshNotificationTable($.devIdx);
		 },
		 error: function(){
				HideNotify();
				ShowNotify($.i18n('Problem updating notification!'), 2500, true);
		 }     
	});
}

function DeleteNotification(idx)
{
	var bValid = false;
	bValid=(confirm($.i18n("Are you sure to delete this notification?\n\nThis action can not be undone..."))==true);
	if (bValid == false)
		return;
		
	$.ajax({
		 url: "json.htm?type=command&param=deletenotification&idx=" + idx,
		 async: false, 
		 dataType: 'json',
		 success: function(data) {
			RefreshNotificationTable($.devIdx);
		 },
		 error: function(){
				HideNotify();
				ShowNotify($.i18n('Problem deleting notification!'), 2500, true);
		 }     
	});
}

function AddNotification()
{
	var nsettings=GetNotificationSettings();
	if (nsettings==null)
	{
	alert($.i18n("Invalid Notification Settings"));
		return;
	}
	$.ajax({
		url: "json.htm?type=command&param=addnotification&idx=" + $.devIdx + 
				"&ttype=" + nsettings.type +
				"&twhen=" + nsettings.when +
				"&tvalue=" + nsettings.value,
		async: false, 
		dataType: 'json',
		success: function(data) {
			if (data.status != "OK") {
				HideNotify();
				ShowNotify($.i18n('Problem adding notification!<br>Duplicate Value?'), 2500, true);
				return;
			}
			RefreshNotificationTable($.devIdx);
		},
		error: function(){
			HideNotify();
			ShowNotify($.i18n('Problem adding notification!'), 2500, true);
		}     
	});
}

function RefreshNotificationTable(idx)
{
  $('#modal').show();

	$($.content + ' #updelclr #notificationupdate').attr("class", "btnstyle3-dis");
	$($.content + ' #updelclr #notificationdelete').attr("class", "btnstyle3-dis");

  var oTable = $($.content + ' #notificationtable').dataTable();
  oTable.fnClearTable();
  
  $.ajax({
     url: "json.htm?type=notifications&idx=" + idx, 
     async: false, 
     dataType: 'json',
     success: function(data) {
        
      if (typeof data.result != 'undefined') {
        $.each(data.result, function(i,item){
			var parts = item.Params.split(';');
			var nvalue=0;
			if (parts.length>1) {
				nvalue=parts[2];
			}
			var whenstr = "";
			
			var ntype="";
			var stype="";
			if (parts[0]=="T")
			{
				ntype=$.i18n("Temperature");
				stype=" &deg; C";
			}
			else if (parts[0]=="D")
			{
				ntype=$.i18n("Dew Point");
				stype=" &deg; C";
			}
			else if (parts[0]=="H")
			{
				ntype=$.i18n("Humidity");
				stype=" %";
			}
			else if (parts[0]=="R")
			{
				ntype=$.i18n("Rain");
				stype=" mm";
			}
			else if (parts[0]=="W")
			{
				ntype=$.i18n("Wind");
				stype=" " + $.myglobals.windsign;
			}
			else if (parts[0]=="U")
			{
				ntype=$.i18n("UV");
				stype=" UVI";
			}
			else if (parts[0]=="M")
			{
				ntype=$.i18n("Usage");
			}
			else if (parts[0]=="B")
			{
				ntype=$.i18n("Baro");
				stype=" hPa";
			}
			else if (parts[0]=="S")
			{
				ntype=$.i18n("Switch On");
			}
			else if (parts[0]=="O")
			{
				ntype=$.i18n("Switch Off");
			}
			else if (parts[0]=="E")
			{
				ntype=$.i18n("Today");
				stype=" kWh";
			}
			else if (parts[0]=="G")
			{
				ntype=$.i18n("Today");
				stype=" m3";
			}
			else if (parts[0]=="1")
			{
				ntype=$.i18n("Ampere 1");
				stype=" A";
			}
			else if (parts[0]=="2")
			{
				ntype=$.i18n("Ampere 2");
				stype=" A";
			}
			else if (parts[0]=="3")
			{
				ntype=$.i18n("Ampere 3");
				stype=" A";
			}
			else if (parts[0]=="P")
			{
				ntype=$.i18n("Percentage");
				stype=" %";
			}

			var nwhen="";
			if (ntype==$.i18n("Switch On")) {
				whenstr=$.i18n("On");
			}
			else if (ntype==$.i18n("Switch Off")) {
				whenstr=$.i18n("Off");
			}
			else if (ntype==$.i18n("Dew Point")) {
				whenstr=$.i18n("Dew Point");
			}
			else {
				if (parts[1]==">") {
					nwhen=$.i18n("Greater") + " ";
				}
				else {
					nwhen=$.i18n("Below") + " ";
				}
				whenstr= nwhen + nvalue + stype;
			}
					
			var addId = oTable.fnAddData( {
				"DT_RowId": item.idx,
				"nvalue" : nvalue,
				"0": ntype,
				"1": whenstr
			} );
		});
		/* Add a click handler to the rows - this could be used as a callback */
		$($.content + " #notificationtable tbody tr").click( function( e ) {
			if ( $(this).hasClass('row_selected') ) {
				$(this).removeClass('row_selected');
				$($.content + ' #updelclr #notificationupdate').attr("class", "btnstyle3-dis");
				$($.content + ' #updelclr #notificationdelete').attr("class", "btnstyle3-dis");
			}
			else {
				oTable.$('tr.row_selected').removeClass('row_selected');
				$(this).addClass('row_selected');
				$($.content + ' #updelclr #notificationupdate').attr("class", "btnstyle3");
				$($.content + ' #updelclr #notificationdelete').attr("class", "btnstyle3");
				var anSelected = fnGetSelected( oTable );
				if ( anSelected.length !== 0 ) {
					var data = oTable.fnGetData( anSelected[0] );
					var idx= data["DT_RowId"];
					$.myglobals.SelectedNotificationIdx=idx;
					$($.content + " #updelclr #notificationupdate").attr("href", "javascript:UpdateNotification(" + idx + ")");
					$($.content + " #updelclr #notificationdelete").attr("href", "javascript:DeleteNotification(" + idx + ")");
					//update user interface with the paramters of this row
					$($.content + " #notificationparamstable #combotype").val(GetValTextInNTypeStrArray(data["0"]));
					ShowNotificationTypeLabel();
					var matchstr="^" + $.i18n("Greater");
					if (data["1"].match(matchstr)) {
						$($.content + " #notificationparamstable #combowhen").val(0);
					}
					else {
						$($.content + " #notificationparamstable #combowhen").val(1);
					}
					$($.content + " #notificationparamstable #value").val(data["nvalue"]);
				}
			}
		}); 
      }
     }
  });
  $('#modal').hide();
}

function ShowNotificationTypeLabel()
{
	var typetext = $($.content + " #notificationparamstable #combotype option:selected").text();
	
	if (
		(typetext == $.i18n("Switch On"))||
		(typetext == $.i18n("Switch Off"))||
		(typetext == $.i18n("Dew Point"))
		) {
		$($.content + " #notificationparamstable #notiwhen").hide();
		$($.content + " #notificationparamstable #notival").hide();
		return;
	}
	$($.content + " #notificationparamstable #notiwhen").show();
	$($.content + " #notificationparamstable #notival").show();
	
	if (typetext == $.i18n('Temperature'))
		$($.content + " #notificationparamstable #valuetype").html('&nbsp;&deg; C');
	else if (typetext == $.i18n('Dew Point'))
		$($.content + " #notificationparamstable #valuetype").html('&nbsp;&deg; C');
	else if (typetext == $.i18n('Humidity'))
		$($.content + " #notificationparamstable #valuetype").html('&nbsp;%');
	else if (typetext == $.i18n('UV'))
		$($.content + " #notificationparamstable #valuetype").html('&nbsp;UVI');
	else if (typetext == $.i18n('Rain'))
		$($.content + " #notificationparamstable #valuetype").html('&nbsp;mm');
	else if (typetext == $.i18n('Wind'))
		$($.content + " #notificationparamstable #valuetype").html('&nbsp;' + $.myglobals.windsign);
	else if (typetext == $.i18n('Baro'))
		$($.content + " #notificationparamstable #valuetype").html('&nbsp;hPa');
	else if (typetext == $.i18n('Usage'))
		$($.content + " #notificationparamstable #valuetype").html('&nbsp;');
	else if (typetext == $.i18n('Today'))
		$($.content + " #notificationparamstable #valuetype").html('&nbsp;');
	else if (typetext == $.i18n('Total'))
		$($.content + " #notificationparamstable #valuetype").html('&nbsp;');
	else if (typetext == $.i18n('Ampere 1'))
		$($.content + " #notificationparamstable #valuetype").html('&nbsp;A');
	else if (typetext == $.i18n('Ampere 2'))
		$($.content + " #notificationparamstable #valuetype").html('&nbsp;A');
	else if (typetext == $.i18n('Ampere 3'))
		$($.content + " #notificationparamstable #valuetype").html('&nbsp;A');
	else if (typetext == $.i18n('Percentage'))
		$($.content + " #notificationparamstable #valuetype").html('&nbsp;%');
	else
		$($.content + " #notificationparamstable #valuetype").html('&nbsp;??');
}

function GetValTextInNTypeStrArray(stext)
{
	var pos=-1;
	$.each($.NTypeStr, function(i,item){
		if ($.i18n(item.text) == stext)
		{
			pos = item.val;
		}
	});

	return pos;
}

function ShowNotifications(id,name,content,backfunction)
{
  clearInterval($.myglobals.refreshTimer);
  $.devIdx=id;
  $.content=content;
  
  $.NTypeStr = [];

	$.ajax({
		 url: "json.htm?type=command&param=getnotificationtypes&idx=" + $.devIdx,
		 async: false, 
		 dataType: 'json',
		 success: function(data) {
		 
				if (typeof data.result != 'undefined') {
					$.each(data.result, function(i,item){
						$.NTypeStr.push({
							val: item.val,
							text: item.text,
							tag: item.ptag
						 }
						);
					});
				}
				
				var oTable;
				
				$('#modal').show();
				var htmlcontent = '';
				htmlcontent='<p><h2>Name: ' + name + '</h2></p><br>\n';
				htmlcontent+=$('#editnotifications').html();
				$($.content).html(GetBackbuttonHTMLTable(backfunction)+htmlcontent);
				$($.content).i18n();
				
				//add types to combobox
				$.each($.NTypeStr, function(i,item){
					var option = $('<option />');
					option.attr('value', item.val).text($.i18n(item.text));
					$($.content + ' #notificationparamstable #combotype').append(option);
				});
				
				oTable = $($.content + ' #notificationtable').dataTable( {
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
												"sPaginationType": "full_numbers"
											} );

				$($.content + " #notificationparamstable #combotype").change(function() {
					ShowNotificationTypeLabel();
				});
			 
				ShowNotificationTypeLabel();
				$('#modal').hide();
				RefreshNotificationTable(id);
		 },
		 error: function(){
				HideNotify();
				ShowNotify($.i18n('Problem clearing notifications!'), 2500, true);
		 }     
	});
}

function GetUTCFromString(s)
{
    return Date.UTC(
      parseInt(s.substring(0, 4), 10),
      parseInt(s.substring(5, 7), 10) - 1,
      parseInt(s.substring(8, 10), 10),
      parseInt(s.substring(11, 13), 10),
      parseInt(s.substring(14, 16), 10),
      0
    );
}

function GetUTCFromStringSec(s)
{
    return Date.UTC(
      parseInt(s.substring(0, 4), 10),
      parseInt(s.substring(5, 7), 10) - 1,
      parseInt(s.substring(8, 10), 10),
      parseInt(s.substring(11, 13), 10),
      parseInt(s.substring(14, 16), 10),
      parseInt(s.substring(17, 19), 10)
    );
}

function GetDateFromString(s)
{
    return Date.UTC(
      parseInt(s.substring(0, 4), 10),
      parseInt(s.substring(5, 7), 10) - 1,
      parseInt(s.substring(8, 10), 10));
}

function cursorhand()
{
    document.body.style.cursor = "pointer";
}

function cursordefault()
{
   document.body.style.cursor = "default";
}
      
function ShowNotify(txt, timeout, iserror)
{
	$("#notification").html('<p>' + txt + '</p>');
	
	if (typeof iserror != 'undefined') {
		$("#notification").css("background-color","red");
	} else {
		$("#notification").css("background-color","#204060");
	}
	$("#notification").center();
	$("#notification").fadeIn("slow");

	if (typeof timeout != 'undefined') {
		setTimeout(function() {
			HideNotify();
		}, timeout);
	}
}

function HideNotify()
{
	$("#notification").hide();
}

function ChangeClass(elemname, newclass)
{
	document.getElementById(elemname).setAttribute("class", newclass);
}

function GetLayoutFromURL()
{
	var page = window.location.hash.substr(1);
	return page!=""?page:'Dashboard';
}

function SetLayoutURL(name)
{
	window.location.hash = name;
}

function SwitchLayout(layout)
{
	if (layout=="Logout")
	{
		$.ajax({
		 url: "json.htm?type=command&param=dologout",
		 async: true, 
		 dataType: 'json',
		 success: function(data) {
			window.location = '/';
		 },
		 error: function(){
			window.location = '/';
		 }     
		});
		return;
	}
	else if (layout=="Restart") {
		var bValid = false;
		bValid=(confirm($.i18n("Are you sure to Restart the system?"))==true);
		if ( bValid ) {
			$.ajax({
			 url: "json.htm?type=command&param=system_reboot",
			 async: true, 
			 dataType: 'json',
			 success: function(data) {
			 },
			 error: function(){
			 }     
			});
			alert($.i18n("Restarting System (This could take some time...)"));
		}
		return;
	}
	else if (layout=="Shutdown") {
		var bValid = false;
		bValid=(confirm($.i18n("Are you sure to Shutdown the system?"))==true);
		if ( bValid ) {
			$.ajax({
			 url: "json.htm?type=command&param=system_shutdown",
			 async: true, 
			 dataType: 'json',
			 success: function(data) {
			 },
			 error: function(){
			 }     
			});
			alert($.i18n("The system is being Shutdown (This could take some time...)"));
		}
		return;
	}
	var fullLayout = layout;
	var hyphen = layout.indexOf('-');
	if( hyphen >= 0 ){
		layout = layout.substr(0, hyphen);
	}

	clearInterval($.myglobals.refreshTimer);
	$.myglobals.prevlayout = $.myglobals.layout;
	$.myglobals.actlayout = layout;
	$.myglobals.layoutFull = fullLayout;
	$.myglobals.layoutParameters = fullLayout.substr(hyphen+1);
	
	//update menu items
	$("ul.nav li").removeClass("current_page_item");
	$("ul.nav #m"+layout).addClass("current_page_item");

	var durl=layout.toLowerCase()+".html";
	if (layout == "LightSwitches") {
		durl='lights.html';
	}
	if (window.my_config.userrights!=2) {
		if ((durl=='setup.html')||(durl=='users.html')||(durl=='cam.html')||(durl=='events.html')||(durl=='hardware.html')||(durl=='devices.html')) {
			durl='dashboard.html';
		}
	}

	$.ajax({
	 url: "json.htm?type=command&param=getversion",
	 async: true, 
	 dataType: 'json',
	 success: function(data) {
		if (data.status == "OK") {
			$( "#appversion" ).text("V" + data.version);
		}
	 },
	 error: function(){
	 }     
	});
		
	//stop chaching these pages
	var dt = new Date();
	durl+='?sid='+dt.getTime();

	if(GetLayoutFromURL() != fullLayout) SetLayoutURL(fullLayout);
	
	$(".bannercontent").load(durl, function(response, status, xhr) {
		if (status == "error") {
			var msg = "Sorry but there was an error: ";
			$(".bannercontent").html(msg + xhr.status + " " + xhr.statusText);
		}
	});

$('.btn-navbar').addClass('collapsed');
$('.nav-collapse').removeClass('in').css('height', '0');	
}

function ShowNewBannerContent(content, backlink)
{
	if (typeof backlink == 'undefined') {
		//hide back button
	}
	else {
		//set back button
	}
	$(".bannercontent").slideUp('slow', function() {
		$(".bannercontent").html(content);
		$(".bannercontent").slideDown('slow');
	});      
}

function AddDevice(id)
{
	$( "#dialog-adddevice" ).dialog( "open" );
}

function ShowLog(id)
{
	ShowNewBannerContent("Hier komt de log");
}

function checkLength( o, min, max ) 
{
			if ( o.val().length > max || o.val().length < min ) {
					return false;
			} else {
					return true;
			}
}

function SetDimValue(idx, value)
{
	clearInterval($.setDimValue);

	if (window.my_config.userrights==0) {
        HideNotify();
		ShowNotify($.i18n('You do not have permission to do that!'), 2500, true);
		return;
	}

	$.ajax({
		 url: "json.htm?type=command&param=switchlight&idx=" + idx + "&switchcmd=Set%20Level&level=" + value,
		 async: false, 
		 dataType: 'json'
	});
}

//Some helper for browser detection
function matchua ( ua ) 
{
	ua = ua.toLowerCase();

	var match = /(chrome)[ \/]([\w.]+)/.exec( ua ) ||
		/(webkit)[ \/]([\w.]+)/.exec( ua ) ||
		/(opera)(?:.*version|)[ \/]([\w.]+)/.exec( ua ) ||
		/(msie) ([\w.]+)/.exec( ua ) ||
		ua.indexOf("compatible") < 0 && /(mozilla)(?:.*? rv:([\w.]+)|)/.exec( ua ) ||
		[];

	return {
		browser: match[ 1 ] || "",
		version: match[ 2 ] || "0"
	};
}

function Get5MinuteHistoryDaysGraphTitle()
{
	if ($.FiveMinuteHistoryDays==1) {
		return $.i18n("Last") + " 24 " + $.i18n("Hours");
	}
	else if  ($.FiveMinuteHistoryDays==2) {
		return $.i18n("Last") + " 48 " + $.i18n("Hours");
	}
	return $.i18n("Last") + " " + $.FiveMinuteHistoryDays + " " + $.i18n("Days");
}

function GenerateCamFeedURL(address,port,username,password,videourl)
{
	var feedsrc="http://";
	if (username!="")
	{
		feedsrc+=username+":"+password+"@";
	}
	feedsrc+=address;
	if (port!=80) {
		feedsrc+=":"+port;
	}
	feedsrc+="/" + videourl;
	return feedsrc;
}

function GenerateCamImageURL(address,port,username,password,imageurl)
{
	var feedsrc="http://";
	if (username!="")
	{
		feedsrc+=username+":"+password+"@";
	}
	feedsrc+=address;
	if (port!=80) {
		feedsrc+=":"+port;
	}
	feedsrc+="/" + imageurl;
	return feedsrc;
}

function GetTemp48Item(temp)
{
	if (temp<=0) {
		return "ice.png";
	}
	if (temp<5) {
		return "temp-0-5.png";
	}
	if (temp<10) {
		return "temp-5-10.png";
	}
	if (temp<15) {
		return "temp-10-15.png";
	}
	if (temp<20) {
		return "temp-15-20.png";
	}
	if (temp<25) {
		return "temp-20-25.png";
	}
	if (temp<30) {
		return "temp-25-30.png";
	}
	return "temp-gt-30.png";
}

function generate_noty(ntype, ntext, ntimeout) {
	var n = noty({
		text: ntext,
		type: ntype,
		dismissQueue: true,
		timeout: ntimeout,
		layout: 'topRight',
		theme: 'defaultTheme'
	});
}

function rgb2hex(rgb) {
	if (  rgb.search("rgb") == -1 ) {
			return rgb;
	} else {
			rgb = rgb.match(/^rgba?\((\d+),\s*(\d+),\s*(\d+)(?:,\s*(\d+))?\)$/);
			function hex(x) {
					 return ("0" + parseInt(x).toString(16)).slice(-2);
			}
			return "#" + hex(rgb[1]) + hex(rgb[2]) + hex(rgb[3]);
	}
}

function chartPointClick(event, retChart) {
	if (event.shiftKey!=true) {
		return;
	}
	if (window.my_config.userrights!=2) {
        HideNotify();
		ShowNotify($.i18n('You do not have permission to do that!'), 2500, true);
		return;
	}
	var dateString=Highcharts.dateFormat('%Y-%m-%d', event.point.x);
	var bValid = false;
	bValid=(confirm($.i18n("Are you sure to remove this value at") + " ?:\n\nDate: " + dateString + " \nValue: " + event.point.y)==true);
	if (bValid == false) {
		return;
	}
	$.ajax({
		 url: "json.htm?type=command&param=deletedatapoint&idx=" + $.devIdx + "&date=" + dateString,
		 async: false, 
		 dataType: 'json',
		 success: function(data) {
			if (data.status == "OK") {
				retChart($.devIdx,$.devName);
			}
			else {
				ShowNotify($.i18n('Problem deleting data point!'), 2500, true);
			}
		 },
		 error: function(){
			ShowNotify($.i18n('Problem deleting data point!'), 2500, true);
		 }     
	}); 	
}

function chartPointClickNew(event, retChart) {
	if (event.shiftKey!=true) {
		return;
	}
	if (window.my_config.userrights!=2) {
        HideNotify();
		ShowNotify($.i18n('You do not have permission to do that!'), 2500, true);
		return;
	}
	var dateString=Highcharts.dateFormat('%Y-%m-%d', event.point.x);
	var bValid = false;
	bValid=(confirm($.i18n("Are you sure to remove this value at") + " ?:\n\nDate: " + dateString + " \nValue: " + event.point.y)==true);
	if (bValid == false) {
		return;
	}
	$.ajax({
		 url: "json.htm?type=command&param=deletedatapoint&idx=" + $.devIdx + "&date=" + dateString,
		 async: false, 
		 dataType: 'json',
		 success: function(data) {
			if (data.status == "OK") {
				retChart($.content,$.backfunction,$.devIdx,$.devName);
			}
			else {
				ShowNotify($.i18n('Problem deleting data point!'), 2500, true);
			}
		 },
		 error: function(){
			ShowNotify($.i18n('Problem deleting data point!'), 2500, true);
		 }     
	}); 	
}

function chartPointClickEx(event, retChart) {
	if (event.shiftKey!=true) {
		return;
	}
	if (window.my_config.userrights!=2) {
        HideNotify();
		ShowNotify($.i18n('You do not have permission to do that!'), 2500, true);
		return;
	}
	var dateString=Highcharts.dateFormat('%Y-%m-%d', event.point.x);
	var bValid = false;
	bValid=(confirm($.i18n("Are you sure to remove this value at") + " ?:\n\nDate: " + dateString + " \nValue: " + event.point.y)==true);
	if (bValid == false) {
		return;
	}
	$.ajax({
		 url: "json.htm?type=command&param=deletedatapoint&idx=" + $.devIdx + "&date=" + dateString,
		 async: false, 
		 dataType: 'json',
		 success: function(data) {
			if (data.status == "OK") {
				retChart($.devIdx,$.devName,$.devSwitchType);
			}
			else {
				ShowNotify($.i18n('Problem deleting data point!'), 2500, true);
			}
		 },
		 error: function(){
			ShowNotify($.i18n('Problem deleting data point!'), 2500, true);
		 }     
	}); 	
}

function ExportChart2CSV(chart)
{
	var csv = "";
	for (var i = 0; i < chart.series.length; i++) {
		var series = chart.series[i];
		for (var j = 0; j < series.data.length; j++) {
			if (series.data[j] != undefined && series.data[j].x >= series.xAxis.min && series.data[j].x <= series.xAxis.max) {
				csv = csv + series.name + ',' + Highcharts.dateFormat('%Y-%m-%d %H:%M:%S', series.data[j].x) + ',' + series.data[j].y + '\r\n';
			}
		}
	}
  
	var w = window.open('','csvWindow'); // popup, may be blocked though
	// the following line does not actually do anything interesting with the 
	// parameter given in current browsers, but really should have. 
	// Maybe in some browser it will. It does not hurt anyway to give the mime type
	w.document.open("text/csv");
	w.document.write(csv); // the csv string from for example a jquery plugin
	w.document.close();
}

function RefreshTimeAndSun()
{
	if (typeof $("#timesun") != 'undefined') {
		$.ajax({
		 url: "json.htm?type=command&param=getSunRiseSet",
		 async: true, 
		 dataType: 'json',
		 success: function(data) {
			if (typeof data.Sunrise != 'undefined') {
						var sunRise=data.Sunrise;
						var sunSet=data.Sunset;
						var ServerTime=data.ServerTime;
						var suntext;
						var bIsMobile=$.myglobals.ismobile;
						if (bIsMobile == true) {
							suntext=$.i18n('SunRise') + ': ' + sunRise + ', ' + $.i18n('SunSet') + ': ' + sunSet;
						}
						else {
							suntext=ServerTime + ', ' + $.i18n('SunRise') + ': ' + sunRise + ', ' + $.i18n('SunSet') + ': ' + sunSet;
						}
						$("#timesun").html(suntext);
			}
		 }
		});
	}
}

function Logout()
{
  $.ajax({
     url: "json.htm?type=command&param=logout",
     async: false, 
     dataType: 'json',
     success: function(data) {
		SwitchLayout('Dashboard');
     },
     error: function(){
		SwitchLayout('Dashboard');
     }     
  });
}

function EnableDisableTabs()
{
	$.ajax({
		 url: "json.htm?type=command&param=getactivetabs",
		 async: false, 
		 dataType: 'json',
		 success: function(data) {
			if (typeof data.language != 'undefined') {
				SetLanguage(data.language);
			}
			else {
				SetLanguage('en');
			}
			
			var urights=data.statuscode;
			if (urights!=3) {
				if (urights!=2) {
					//no setup menu
					$("#dLogout").show();
					$("#mLogout").show();
					$("#dLogoutSetup").hide();
					$("#mLogoutSetup").hide();
				}
				else {
					$("#dLogout").hide();
					$("#mLogout").hide();
					$("#dLogoutSetup").show();
					$("#mLogoutSetup").show();
				}
			}
			else {
				$("#dLogout").hide();
				$("#mLogout").hide();
				$("#dLogoutSetup").hide();
				$("#mLogoutSetup").hide();
			}
			
			$.myglobals.ismobileint=false;
			if (typeof data.MobileType != 'undefined') {
				if( /Android|webOS|iPhone|iPad|iPod|BlackBerry/i.test(navigator.userAgent) ) {
					$.myglobals.ismobile=true;
					$.myglobals.ismobileint=true;
				}
				if (data.MobileType!=0) {
					if(!(/iPhone/i.test(navigator.userAgent) )) {
						$.myglobals.ismobile=false;
					}
				}
			}			
			if (typeof data.WindScale != 'undefined') {
				$.myglobals.windscale=parseFloat(data.WindScale);
			}
			if (typeof data.WindSign != 'undefined') {
				$.myglobals.windsign=data.WindSign;
			}

			if (data.result["EnableTabLights"]==0) {
				$("#mLightSwitches").hide();
			}
			else {
				$("#mLightSwitches").show();
			}
			if (data.result["EnableTabScenes"]==0) {
				$("#mScenes").hide();
			}
			else {
				$("#mScenes").show();
			}
			if (data.result["EnableTabTemp"]==0) {
				$("#mTemperature").hide();
			}
			else {
				$("#mTemperature").show();
			}
			if (data.result["EnableTabWeather"]==0) {
				$("#mWeather").hide();
			}
			else {
				$("#mWeather").show();
			}
			if (data.result["EnableTabUtility"]==0) {
				$("#mUtility").hide();
			}
			else {
				$("#mUtility").show();
			}
		 },
		 error: function(){
			if (showdialog) {
				alert($.i18n("Error communicating to server!"));
			}
		 }     
	});
}

function SetLanguage(lng)
{
	$.i18n.debug = true;
	var i18n = $.i18n({
			locale: lng
		});
	$(".nav").i18n();
}

function TranslateStatus(status)
{
	//should of course be changed, but for now a quick sollution
	if (status.indexOf("Set Level") != -1) {
		return status.replace("Set Level",$.i18n('Set Level'));
	}
	else {
		return $.i18n(status);
	}
}

function ShowGeneralGraph(id,name,switchtype,sensortype)
{
	clearInterval($.myglobals.refreshTimer);
  $('#modal').show();
  $.devIdx=id;
  $.devName=name;
  var htmlcontent = '';
  htmlcontent='<p><center><h2>' + name + '</h2></center></p>\n';
  htmlcontent+=$('#globaldaylog').html();
  
  var txtLabelOrg="?";
  var txtUnit="?";
  var graphReturn;
  var graphcontent;
  
  if (sensortype=="Visibility") {
	graphReturn="ShowWeathers";
	txtLabelOrg="Visibility";
	$.content="#weathercontent";
	graphcontent=$('#weathercontent');
	txtUnit="km";
	if (switchtype==1) {
		txtUnit="mi";
	}
  }
  else if (sensortype=="Radiation") {
	graphReturn="ShowWeathers";
	txtLabelOrg="Radiation";
	$.content="#weathercontent";
	graphcontent=$('#weathercontent');
	txtUnit="Watt/m2";
  }
  else if (sensortype=="Soil Moisture") {
	graphReturn="ShowUtilities";
	txtLabelOrg="Moisture";
	$.content="#utilitycontent";
	graphcontent=$('#utilitycontent');
	txtUnit="cb";
  }
  else if (sensortype=="Leaf Wetness") {
	graphReturn="ShowUtilities";
	txtLabelOrg="Wetness";
	$.content="#utilitycontent";
	graphcontent=$('#utilitycontent');
	txtUnit="Range";
  }
  else if (sensortype=="Voltage") {
	graphReturn="ShowUtilities";
	txtLabelOrg="Voltage";
	$.content="#utilitycontent";
	graphcontent=$('#utilitycontent');
	txtUnit="mV";
  }
  else {
	return;
  }
  graphcontent.html(GetBackbuttonHTMLTable(graphReturn)+htmlcontent);
  graphcontent.i18n();
  
  var txtLabel= txtLabelOrg + " (" + txtUnit + ")";

	$.LogChart1 = $($.content + ' #globaldaygraph');
	$.LogChart1.highcharts({
      chart: {
          type: 'spline',
          zoomType: 'xy',
          marginRight: 10,
          events: {
              load: function() {
                $.getJSON("json.htm?type=graph&sensor=counter&idx="+id+"&range=day",
                function(data) {
                      var datatable = [];
                      
                      $.each(data.result, function(i,item)
                      {
                        datatable.push( [GetUTCFromString(item.d), parseFloat(item.v) ] );
                      });
                      var series = $.LogChart1.highcharts().series[0];
                      series.setData(datatable);
                });
              }
          }
        },
       credits: {
          enabled: true,
          href: "http://www.domoticz.com",
          text: "Domoticz.com"
        },
        title: {
            text: $.i18n(txtLabelOrg) + ' '  + Get5MinuteHistoryDaysGraphTitle()
        },
        xAxis: {
            type: 'datetime'
        },
        yAxis: {
            title: {
                text: txtLabel
            },
			labels: {
				formatter: function() {
					if (txtUnit=="mV") {
						return Highcharts.numberFormat(this.value, 0);
					}
					return this.value;
				}
			},
			min: 0,
            minorGridLineWidth: 0,
            gridLineWidth: 0,
            alternateGridColor: null
        },
        tooltip: {
            formatter: function() {
                    return ''+
                    Highcharts.dateFormat('%A<br/>%Y-%m-%d %H:%M', this.x) +': '+ this.y +' ' + txtUnit;
            }
        },
        plotOptions: {
            spline: {
                lineWidth: 3,
                states: {
                    hover: {
                        lineWidth: 3
                    }
                },
                marker: {
                    enabled: false,
                    states: {
                        hover: {
                            enabled: true,
                            symbol: 'circle',
                            radius: 5,
                            lineWidth: 1
                        }
                    }
                }
            }
        },
        series: [{
            name: txtLabelOrg
        }]
        ,
        navigation: {
            menuItemStyle: {
                fontSize: '10px'
            }
        }
    });

	$.LogChart2 = $($.content + ' #globalmonthgraph');
	$.LogChart2.highcharts({
      chart: {
          type: 'spline',
          zoomType: 'xy',
          marginRight: 10,
          events: {
              load: function() {
                  
                $.getJSON("json.htm?type=graph&sensor=counter&idx="+id+"&range=month",
                function(data) {
                      var datatable1 = [];
                      var datatable2 = [];
                      
                      $.each(data.result, function(i,item)
                      {
                        datatable1.push( [GetDateFromString(item.d), parseFloat(item.v_min) ] );
                        datatable2.push( [GetDateFromString(item.d), parseFloat(item.v_max) ] );
                      });
                      var series1 = $.LogChart2.highcharts().series[0];
                      var series2 = $.LogChart2.highcharts().series[1];
                      series1.setData(datatable1);
                      series2.setData(datatable2);
                });
              }
          }
        },
       credits: {
          enabled: true,
          href: "http://www.domoticz.com",
          text: "Domoticz.com"
        },
        title: {
            text: $.i18n(txtLabelOrg) + ' ' + $.i18n('Last Month')
        },
        xAxis: {
            type: 'datetime'
        },
        yAxis: {
            title: {
                text: txtLabel
            },
			labels: {
				formatter: function() {
					if (txtUnit=="mV") {
						return Highcharts.numberFormat(this.value, 0);
					}
					return this.value;
				}
			},
            min: 0,
            minorGridLineWidth: 0,
            gridLineWidth: 0,
            alternateGridColor: null
        },
        tooltip: {
            formatter: function() {
                    return ''+
                    Highcharts.dateFormat('%A<br/>%Y-%m-%d %H:%M', this.x) +': '+ this.y +' ' + txtUnit;
            }
        },
        plotOptions: {
            spline: {
                lineWidth: 3,
                states: {
                    hover: {
                        lineWidth: 3
                    }
                },
                marker: {
                    enabled: false,
                    states: {
                        hover: {
                            enabled: true,
                            symbol: 'circle',
                            radius: 5,
                            lineWidth: 1
                        }
                    }
                }
            }
        },
        series: [{
            name: 'min',
			point: {
				events: {
					click: function(event) {
						chartPointClick(event,arguments.callee.name);
					}
				}
			}
		}, {
            name: 'max',
			point: {
				events: {
					click: function(event) {
						chartPointClick(event,arguments.callee.name);
					}
				}
			}
        }]
        ,
        navigation: {
            menuItemStyle: {
                fontSize: '10px'
            }
        }
    });

	$.LogChart3 = $($.content + ' #globalyeargraph');
	$.LogChart3.highcharts({
      chart: {
          type: 'spline',
          zoomType: 'xy',
          marginRight: 10,
          events: {
              load: function() {
                  
                $.getJSON("json.htm?type=graph&sensor=counter&idx="+id+"&range=year",
                function(data) {
                      var datatable1 = [];
                      var datatable2 = [];
                      
                      $.each(data.result, function(i,item)
                      {
                        datatable1.push( [GetDateFromString(item.d), parseFloat(item.v_min) ] );
                        datatable2.push( [GetDateFromString(item.d), parseFloat(item.v_max) ] );
                      });
                      var series1 = $.LogChart3.highcharts().series[0];
                      var series2 = $.LogChart3.highcharts().series[1];
                      series1.setData(datatable1);
                      series2.setData(datatable2);
                });
              }
          }
        },
       credits: {
          enabled: true,
          href: "http://www.domoticz.com",
          text: "Domoticz.com"
        },
        title: {
            text: $.i18n(txtLabelOrg) + ' ' + $.i18n('Last Year')
        },
        xAxis: {
            type: 'datetime'
        },
        yAxis: {
            title: {
                text: txtLabel
            },
			labels: {
				formatter: function() {
					if (txtUnit=="mV") {
						return Highcharts.numberFormat(this.value, 0);
					}
					return this.value;
				}
			},
            min: 0,
            minorGridLineWidth: 0,
            gridLineWidth: 0,
            alternateGridColor: null
        },
        tooltip: {
            formatter: function() {
                    return ''+
                    Highcharts.dateFormat('%A<br/>%Y-%m-%d %H:%M', this.x) +': '+ this.y +' ' + txtUnit;
            }
        },
        plotOptions: {
            spline: {
                lineWidth: 3,
                states: {
                    hover: {
                        lineWidth: 3
                    }
                },
                marker: {
                    enabled: false,
                    states: {
                        hover: {
                            enabled: true,
                            symbol: 'circle',
                            radius: 5,
                            lineWidth: 1
                        }
                    }
                }
            }
        },
        series: [{
            name: 'min',
			point: {
				events: {
					click: function(event) {
						chartPointClick(event,arguments.callee.name);
					}
				}
			}
		}, {
            name: 'max',
			point: {
				events: {
					click: function(event) {
						chartPointClick(event,arguments.callee.name);
					}
				}
			}
        }]
        ,
        navigation: {
            menuItemStyle: {
                fontSize: '10px'
            }
        }
    });
}

function AddDataToTempChart(data,chart,isday)
{
    var datatablete = [];
    var datatabletm = [];
    var datatablehu = [];
    var datatablech = [];
    var datatablecm = [];
    var datatabledp = [];
    var datatableba = [];

    $.each(data.result, function(i,item)
    {
      if (isday==1) {
        if (typeof item.te != 'undefined') {
          datatablete.push( [GetUTCFromString(item.d), parseFloat(item.te) ] );
        }
        if (typeof item.hu != 'undefined') {
          datatablehu.push( [GetUTCFromString(item.d), parseFloat(item.hu) ] );
        }
        if (typeof item.ch != 'undefined') {
          datatablech.push( [GetUTCFromString(item.d), parseFloat(item.ch) ] );
        }
        if (typeof item.dp != 'undefined') {
          datatabledp.push( [GetUTCFromString(item.d), parseFloat(item.dp) ] );
        }
        if (typeof item.ba != 'undefined') {
          datatableba.push( [GetUTCFromString(item.d), parseFloat(item.ba) ] );
        }
      } else {
        if (typeof item.te != 'undefined') {
          datatablete.push( [GetDateFromString(item.d), parseFloat(item.te) ] );
          datatabletm.push( [GetDateFromString(item.d), parseFloat(item.tm) ] );
        }
        if (typeof item.hu != 'undefined') {
          datatablehu.push( [GetDateFromString(item.d), parseFloat(item.hu) ] );
        }
        if (typeof item.ch != 'undefined') {
          datatablech.push( [GetDateFromString(item.d), parseFloat(item.ch) ] );
          datatablecm.push( [GetDateFromString(item.d), parseFloat(item.cm) ] );
        }
        if (typeof item.dp != 'undefined') {
          datatabledp.push( [GetDateFromString(item.d), parseFloat(item.dp) ] );
        }
        if (typeof item.ba != 'undefined') {
          datatableba.push( [GetDateFromString(item.d), parseFloat(item.ba) ] );
        }
      }
    });
    var series;
    if (datatablehu.length!=0)
    {
      chart.addSeries(
        {
          id: 'humidity',
          name: $.i18n('Humidity'),
          color: 'limegreen',
          yAxis: 1
        }
      );
      series = chart.get('humidity');
      series.setData(datatablehu);
    }

    if (datatablech.length!=0)
    {
      chart.addSeries(
        {
          id: 'chill',
          name: $.i18n('Chill'),
          color: 'red',
          yAxis: 0
        }
      );
      series = chart.get('chill');
      series.setData(datatablech);
      
      if (isday==0) {
        chart.addSeries(
          {
            id: 'chillmin',
            name: $.i18n('Chill') + '_min',
            color: 'rgba(255,127,39,0.8)',
            yAxis: 0
          }
        );
        series = chart.get('chillmin');
        series.setData(datatablecm);
      }
    }
    if (datatablete.length!=0)
    {
      //Add Temperature series
      chart.addSeries(
        {
          id: 'temperature',
          name: $.i18n('Temperature'),
          color: 'yellow',
          yAxis: 0
        }
      );
      series = chart.get('temperature');
      series.setData(datatablete);
      if (isday==0) {
        chart.addSeries(
          {
            id: 'temperaturemin',
            name: $.i18n('Temperature') + '_min',
            color: 'rgba(3,190,252,0.8)',
            yAxis: 0
          }
        );
        series = chart.get('temperaturemin');
        series.setData(datatabletm);
      }
    }
    if (datatabledp.length!=0)
    {
      chart.addSeries(
        {
          id: 'dewpoint',
          name: $.i18n('Dew Point'),
          color: 'blue',
          yAxis: 0
        }
      );
      series = chart.get('dewpoint');
      series.setData(datatabledp);
    }
    if (datatableba.length!=0)
    {
      chart.addSeries(
        {
          id: 'baro',
          name: $.i18n('Barometer'),
          color: 'pink',
          yAxis: 2
        }
      );
      series = chart.get('baro');
      series.setData(datatableba);
    }
}

function ShowTempLog(contentdiv,backfunction,id,name)
{
  clearInterval($.myglobals.refreshTimer);
  $('#modal').show();
  $.content=contentdiv;
  $.backfunction=backfunction;
  $.devIdx=id;
  $.devName=name;
  var htmlcontent = '';
  htmlcontent='<p><center><h2>' + name + '</h2></center></p>\n';
  htmlcontent+=$('#daymonthyearlog').html();

  $($.content).html(GetBackbuttonHTMLTable(backfunction)+htmlcontent);
  $($.content).i18n();

  $.DayChart = $($.content + ' #daygraph');
  $.DayChart.highcharts({
      chart: {
          type: 'line',
          zoomType: 'xy',
          alignTicks: false,
          events: {
              load: function() {
                this.showLoading();
                $.getJSON("json.htm?type=graph&sensor=temp&idx="+id+"&range=day",
                function(data) {
                      AddDataToTempChart(data,$.DayChart.highcharts(),1);
                      $.DayChart.hideLoading();
                });
                this.hideLoading();
              }
          }
      },
      loading: {
          hideDuration: 1000,
          showDuration: 1000
      },
      credits: {
        enabled: true,
        href: "http://www.domoticz.com",
        text: "Domoticz.com"
      },
      title: {
          text: $.i18n('Temperature') + ' ' + Get5MinuteHistoryDaysGraphTitle()
      },
      xAxis: {
          type: 'datetime'
      },
      yAxis: [{ //temp label
          labels:  {
                   formatter: function() {
                        return this.value +'\u00B0 C';
                   },
                   style: {
                      color: '#CCCC00'
                   }
          },
          title: {
              text: 'degrees Celsius',
               style: {
                  color: '#CCCC00'
               }
          }
      }, { //humidity label
          labels:  {
                   formatter: function() {
                        return this.value +'%';
                   },
                   style: {
                      color: 'limegreen'
                   }
          },
          title: {
              text: $.i18n('Humidity') +' %',
               style: {
                  color: '#00CC00'
               }
          },
          opposite: true
      }],
      tooltip: {
          formatter: function() {
			var unit="";
			if (this.series.name==$.i18n('Humidity')) {
				unit="%";
			}
			else if (this.series.name.indexOf($.i18n('Temperature'))==0) {
				unit="\u00B0 C";
			}
			else if (this.series.name.indexOf($.i18n('Chill'))==0) {
				unit="\u00B0 C";
			}
            return $.i18n(Highcharts.dateFormat('%A',this.x)) + '<br/>' + Highcharts.dateFormat('%Y-%m-%d %H:%M', this.x) +'<br/>'+ this.series.name + ': ' + this.y + unit;
          }
      },
      legend: {
          enabled: true
      },
      plotOptions: {
               line: {
                    lineWidth: 3,
                    states: {
                        hover: {
                            lineWidth: 3
                        }
                    },
                    marker: {
                        enabled: false,
                        states: {
                            hover: {
                                enabled: true,
                                symbol: 'circle',
                                radius: 5,
                                lineWidth: 1
                            }
                        }
                    }
                }
      }
  });

  $.MonthChart = $($.content + ' #monthgraph');
  $.MonthChart.highcharts({
      chart: {
          type: 'spline',
          zoomType: 'xy',
          alignTicks: false,
          events: {
              load: function() {
                this.showLoading();
                $.getJSON("json.htm?type=graph&sensor=temp&idx="+id+"&range=month",
                function(data) {
                      AddDataToTempChart(data,$.MonthChart.highcharts(),0);
                      $.MonthChart.hideLoading();
                });
                this.hideLoading();
              }
          }
      },
      loading: {
          hideDuration: 1000,
          showDuration: 1000
      },
      credits: {
        enabled: true,
        href: "http://www.domoticz.com",
        text: "Domoticz.com"
      },
      title: {
          text: $.i18n('Temperature') + ' ' + $.i18n('Last Month')
      },
      xAxis: {
          type: 'datetime'
      },
      yAxis: [{ //temp label
          labels:  {
                   formatter: function() {
                        return this.value +'\u00B0 C';
                   },
                   style: {
                      color: '#CCCC00'
                   }
          },
          title: {
              text: 'degrees Celsius',
               style: {
                  color: '#CCCC00'
               }
          }
      }, { //humidity label
          labels:  {
                   formatter: function() {
                        return this.value +'%';
                   },
                   style: {
                      color: 'limegreen'
                   }
          },
          title: {
              text: $.i18n('Humidity')+' %',
               style: {
                  color: '#00CC00'
               }
          },
          opposite: true
      }],
      tooltip: {
          formatter: function() {
			var unit="";
			if (this.series.name==$.i18n('Humidity')) {
				unit="%";
			}
			else if (this.series.name.indexOf($.i18n('Temperature'))==0) {
				unit="\u00B0 C";
			}
			else if (this.series.name.indexOf($.i18n('Chill'))==0) {
				unit="\u00B0 C";
			}
            return $.i18n(Highcharts.dateFormat('%A',this.x)) + '<br/>' + Highcharts.dateFormat('%Y-%m-%d %H:%M', this.x) +'<br/>'+ this.series.name + ': ' + this.y + unit;
          }
      },
      legend: {
          enabled: true
      },
      plotOptions: {
				series: {
					point: {
						events: {
							click: function(event) {
								chartPointClickNew(event,ShowTempLog);
							}
						}
					}
				},
				spline: {
                    lineWidth: 3,
                    states: {
                        hover: {
                            lineWidth: 3
                        }
                    },
                    marker: {
                        enabled: false,
                        states: {
                            hover: {
                                enabled: true,
                                symbol: 'circle',
                                radius: 5,
                                lineWidth: 1
                            }
                        }
                    }
                }
      }
  });

  $.YearChart = $($.content + ' #yeargraph');
  $.YearChart.highcharts({
      chart: {
          type: 'spline',
          zoomType: 'xy',
          alignTicks: false,
          events: {
              load: function() {
                this.showLoading();
                $.getJSON("json.htm?type=graph&sensor=temp&idx="+id+"&range=year",
                function(data) {
                      AddDataToTempChart(data,$.YearChart.highcharts(),0);
                      $.YearChart.hideLoading();
                });
                this.hideLoading();
              }
          }
      },
      loading: {
          hideDuration: 1000,
          showDuration: 1000
      },
      credits: {
        enabled: true,
        href: "http://www.domoticz.com",
        text: "Domoticz.com"
      },
      title: {
          text: $.i18n('Temperature') + ' ' + $.i18n('Last Year')
      },
      xAxis: {
          type: 'datetime'
      },
      yAxis: [{ //temp label
          labels:  {
                   formatter: function() {
                        return this.value +'\u00B0 C';
                   },
                   style: {
                      color: '#CCCC00'
                   }
          },
          title: {
              text: 'degrees Celsius',
               style: {
                  color: '#CCCC00'
               }
          }
      }, { //humidity label
          labels:  {
                   formatter: function() {
                        return this.value +'%';
                   },
                   style: {
                      color: 'limegreen'
                   }
          },
          title: {
              text: $.i18n('Humidity')+' %',
               style: {
                  color: '#00CC00'
               }
          },
          opposite: true
      }],
      tooltip: {
          formatter: function() {
			var unit="";
			if (this.series.name==$.i18n('Humidity')) {
				unit="%";
			}
			else if (this.series.name.indexOf($.i18n('Temperature'))==0) {
				unit="\u00B0 C";
			}
			else if (this.series.name.indexOf($.i18n('Chill'))==0) {
				unit="\u00B0 C";
			}
            return $.i18n(Highcharts.dateFormat('%A',this.x)) + '<br/>' + Highcharts.dateFormat('%Y-%m-%d %H:%M', this.x) +'<br/>'+ this.series.name + ': ' + this.y + unit;
          }
      },
      legend: {
          enabled: true
      },
      plotOptions: {
				series: {
					point: {
						events: {
							click: function(event) {
								chartPointClickNew(event,ShowTempLog);
							}
						}
					}
				},
				spline: {
                    lineWidth: 3,
                    states: {
                        hover: {
                            lineWidth: 3
                        }
                    },
                    marker: {
                        enabled: false,
                        states: {
                            hover: {
                                enabled: true,
                                symbol: 'circle',
                                radius: 5,
                                lineWidth: 1
                            }
                        }
                    }
                }
      }
  });

  $('#modal').hide();
  cursordefault();
  return true;
}

function ShowUVLog(contentdiv,backfunction,id,name)
{
	clearInterval($.myglobals.refreshTimer);
  $('#modal').show();
  $.content=contentdiv;
  $.backfunction=backfunction;
  $.devIdx=id;
  $.devName=name;
  var htmlcontent = '';
  htmlcontent='<p><center><h2>' + name + '</h2></center></p><br>\n';
  htmlcontent+=$('#daymonthyearlog').html();
  $($.content).html(GetBackbuttonHTMLTable(backfunction)+htmlcontent);
  $($.content).i18n();
  
  $.DayChart = $($.content + ' #daygraph');
  $.DayChart.highcharts({
      chart: {
          type: 'spline',
          zoomType: 'xy',
          marginRight: 10,
          events: {
              load: function() {
                  
                $.getJSON("json.htm?type=graph&sensor=uv&idx="+id+"&range=day",
                function(data) {
                      var series = $.DayChart.highcharts().series[0];
                      var datatable = [];
                      
                      $.each(data.result, function(i,item)
                      {
                        datatable.push( [GetUTCFromString(item.d), parseFloat(item.uvi) ] );
                      });
                      series.setData(datatable);
                });
              }
          }
        },
       credits: {
          enabled: true,
          href: "http://www.domoticz.com",
          text: "Domoticz.com"
        },
        title: {
            text: 'UV '  + Get5MinuteHistoryDaysGraphTitle()
        },
        xAxis: {
            type: 'datetime'
        },
        yAxis: {
            title: {
                text: 'UV (UVI)'
            },
            min: 0
        },
        tooltip: {
            formatter: function() {
                    return ''+
                    $.i18n(Highcharts.dateFormat('%A',this.x)) + '<br/>' + Highcharts.dateFormat('%Y-%m-%d %H:%M', this.x) +': '+ this.y +' UVI';
            }
        },
        plotOptions: {
            spline: {
                lineWidth: 3,
                states: {
                    hover: {
                        lineWidth: 3
                    }
                },
                marker: {
                    enabled: false,
                    states: {
                        hover: {
                            enabled: true,
                            symbol: 'circle',
                            radius: 5,
                            lineWidth: 1
                        }
                    }
                }
            }
        },
        series: [{
            name: 'uv'
        }]
        ,
        navigation: {
            menuItemStyle: {
                fontSize: '10px'
            }
        }
    });

  $.MonthChart = $($.content + ' #monthgraph');
  $.MonthChart.highcharts({
      chart: {
          type: 'spline',
          zoomType: 'xy',
          marginRight: 10,
          events: {
              load: function() {
                  
                $.getJSON("json.htm?type=graph&sensor=uv&idx="+id+"&range=month",
                function(data) {
                      var series = $.MonthChart.highcharts().series[0];
                      var datatable = [];
                      
                      $.each(data.result, function(i,item)
                      {
                        datatable.push( [GetDateFromString(item.d), parseFloat(item.uvi) ] );
                      });
                      series.setData(datatable);
                });
              }
          }
        },
       credits: {
          enabled: true,
          href: "http://www.domoticz.com",
          text: "Domoticz.com"
        },
        title: {
            text: 'UV ' + $.i18n('Last Month')
        },
        xAxis: {
            type: 'datetime'
        },
        yAxis: {
            title: {
                text: 'UV (UVI)'
            },
            min: 0
        },
        tooltip: {
            formatter: function() {
                    return ''+
                    $.i18n(Highcharts.dateFormat('%A',this.x)) + '<br/>' + Highcharts.dateFormat('%Y-%m-%d %H:%M', this.x) +': '+ this.y +' UVI';
            }
        },
        plotOptions: {
            spline: {
                lineWidth: 3,
                states: {
                    hover: {
                        lineWidth: 3
                    }
                },
                marker: {
                    enabled: false,
                    states: {
                        hover: {
                            enabled: true,
                            symbol: 'circle',
                            radius: 5,
                            lineWidth: 1
                        }
                    }
                }
            }
        },
        series: [{
            name: 'uv',
			point: {
				events: {
					click: function(event) {
						chartPointClickNew(event,ShowUVLog);
					}
				}
			}
        }]
        ,
        navigation: {
            menuItemStyle: {
                fontSize: '10px'
            }
        }
    });

  $.YearChart = $($.content + ' #yeargraph');
  $.YearChart.highcharts({
      chart: {
          type: 'spline',
          zoomType: 'xy',
          marginRight: 10,
          events: {
              load: function() {
                  
                $.getJSON("json.htm?type=graph&sensor=uv&idx="+id+"&range=year",
                function(data) {
                      var series = $.YearChart.highcharts().series[0];
                      var datatable = [];
                      
                      $.each(data.result, function(i,item)
                      {
                        datatable.push( [GetDateFromString(item.d), parseFloat(item.uvi) ] );
                      });
                      series.setData(datatable);
                });
              }
          }
        },
       credits: {
          enabled: true,
          href: "http://www.domoticz.com",
          text: "Domoticz.com"
        },
        title: {
            text: 'UV ' + $.i18n('Last Year')
        },
        xAxis: {
            type: 'datetime'
        },
        yAxis: {
            title: {
                text: 'UV (UVI)'
            },
            min: 0
        },
        tooltip: {
            formatter: function() {
                    return ''+
                    $.i18n(Highcharts.dateFormat('%A',this.x)) + '<br/>' + Highcharts.dateFormat('%Y-%m-%d %H:%M', this.x) +': '+ this.y +' UVI';
            }
        },
        plotOptions: {
            spline: {
                lineWidth: 3,
                states: {
                    hover: {
                        lineWidth: 3
                    }
                },
                marker: {
                    enabled: false,
                    states: {
                        hover: {
                            enabled: true,
                            symbol: 'circle',
                            radius: 5,
                            lineWidth: 1
                        }
                    }
                }
            }
        },
        series: [{
            name: 'uv',
			point: {
				events: {
					click: function(event) {
						chartPointClickNew(event,ShowUVLog);
					}
				}
			}
        }]
        ,
        navigation: {
            menuItemStyle: {
                fontSize: '10px'
            }
        }
    });
  $('#modal').hide();
  cursordefault();
  return false;
}

function ShowWindLog(contentdiv,backfunction,id,name)
{
	clearInterval($.myglobals.refreshTimer);
  $('#modal').show();
  $.content=contentdiv;
  $.backfunction=backfunction;
  $.devIdx=id;
  $.devName=name;
  var htmlcontent = '';
        
  htmlcontent='<p><center><h2>' + name + '</h2></center></p><br>\n';
  htmlcontent+=$('#windlog').html();
  $($.content).html(GetBackbuttonHTMLTable(backfunction)+htmlcontent);
  $($.content).i18n();
  
  $.DayChart = $($.content + ' #winddaygraph');
  $.DayChart.highcharts({
      chart: {
          type: 'spline',
          zoomType: 'xy',
          marginRight: 10,
          events: {
              load: function() {
                $.getJSON("json.htm?type=graph&sensor=wind&idx="+id+"&range=day",
                function(data) {
                      var seriessp = $.DayChart.highcharts().series[0];
                      var seriesgu = $.DayChart.highcharts().series[1];
                      var datatablesp = [];
                      var datatablegu = [];
                      
                      $.each(data.result, function(i,item)
                      {
                        datatablesp.push( [GetUTCFromString(item.d), parseFloat(item.sp) ] );
                        datatablegu.push( [GetUTCFromString(item.d), parseFloat(item.gu) ] );
                      });
                      seriessp.setData(datatablesp);
                      seriesgu.setData(datatablegu);
                });
              }
          }
        },
       credits: {
          enabled: true,
          href: "http://www.domoticz.com",
          text: "Domoticz.com"
        },
        title: {
            text: $.i18n('Wind') + ' ' + $.i18n('speed/gust') + ' '  + Get5MinuteHistoryDaysGraphTitle()
        },
        xAxis: {
            type: 'datetime'
        },
        yAxis: {
            title: {
                text: $.i18n('Speed') + ' (' + $.myglobals.windsign + ')'
            },
            min: 0,
            minorGridLineWidth: 0,
            gridLineWidth: 0,
            alternateGridColor: null,
            plotBands: [{ // Light air
                from: 0.3*$.myglobals.windscale,
                to: 1.5*$.myglobals.windscale,
                color: 'rgba(68, 170, 213, 0.3)',
                label: {
                    text: $.i18n('Light air'),
                    style: {
                        color: '#CCCCCC'
                    }
                }
            }, { // Light breeze
                from: 1.5*$.myglobals.windscale,
                to: 3.5*$.myglobals.windscale,
                color: 'rgba(68, 170, 213, 0.5)',
                label: {
                    text: $.i18n('Light breeze'),
                    style: {
                        color: '#CCCCCC'
                    }
                }
            }, { // Gentle breeze
                from: 3.5*$.myglobals.windscale,
                to: 5.5*$.myglobals.windscale,
                color: 'rgba(68, 170, 213, 0.3)',
                label: {
                    text: $.i18n('Gentle breeze'),
                    style: {
                        color: '#CCCCCC'
                    }
                }
            }, { // Moderate breeze
                from: 5.5*$.myglobals.windscale,
                to: 8*$.myglobals.windscale,
                color: 'rgba(68, 170, 213, 0.5)',
                label: {
                    text: $.i18n('Moderate breeze'),
                    style: {
                        color: '#CCCCCC'
                    }
                }
            }, { // Fresh breeze
                from: 8*$.myglobals.windscale,
                to: 10.8*$.myglobals.windscale,
                color: 'rgba(68, 170, 213, 0.3)',
                label: {
                    text: $.i18n('Fresh breeze'),
                    style: {
                        color: '#CCCCCC'
                    }
                }
            }, { // Strong breeze
                from: 10.8*$.myglobals.windscale,
                to: 13.9*$.myglobals.windscale,
                color: 'rgba(68, 170, 213, 0.5)',
                label: {
                    text: $.i18n('Strong breeze'),
                    style: {
                        color: '#CCCCCC'
                    }
                }
            }, { // High wind
                from: 13.9*$.myglobals.windscale,
                to: 17.2*$.myglobals.windscale,
                color: 'rgba(68, 170, 213, 0.3)',
                label: {
                    text: $.i18n('High wind'),
                    style: {
                        color: '#CCCCCC'
                    }
                }
            }, { // fresh gale
                from: 17.2*$.myglobals.windscale,
                to: 20.8*$.myglobals.windscale,
                color: 'rgba(68, 170, 213, 0.3)',
                label: {
                    text: $.i18n('Fresh gale'),
                    style: {
                        color: '#CCCCCC'
                    }
                }
            }, { // strong gale
                from: 20.8*$.myglobals.windscale,
                to: 24.5*$.myglobals.windscale,
                color: 'rgba(68, 170, 213, 0.5)',
                label: {
                    text: $.i18n('Strong gale'),
                    style: {
                        color: '#CCCCCC'
                    }
                }
            }, { // storm
                from: 24.5*$.myglobals.windscale,
                to: 28.5*$.myglobals.windscale,
                color: 'rgba(68, 170, 213, 0.3)',
                label: {
                    text: $.i18n('Storm'),
                    style: {
                        color: '#CCCCCC'
                    }
                }
            }, { // Violent storm
                from: 28.5*$.myglobals.windscale,
                to: 32.7*$.myglobals.windscale,
                color: 'rgba(68, 170, 213, 0.5)',
                label: {
                    text: $.i18n('Violent storm'),
                    style: {
                        color: '#CCCCCC'
                    }
                }
            }, { // hurricane
                from: 32.7*$.myglobals.windscale,
                to: 100*$.myglobals.windscale,
                color: 'rgba(68, 170, 213, 0.3)',
                label: {
                    text: $.i18n('Hurricane'),
                    style: {
                        color: '#CCCCCC'
                    }
                }
            }
            ]
        },
        tooltip: {
            formatter: function() {
                    return ''+
                    $.i18n(Highcharts.dateFormat('%A',this.x)) + '<br/>' + Highcharts.dateFormat('%Y-%m-%d %H:%M', this.x) +': '+ this.y +' ' + $.myglobals.windsign;
            }
        },
        plotOptions: {
            spline: {
                lineWidth: 3,
                states: {
                    hover: {
                        lineWidth: 3
                    }
                },
                marker: {
                    enabled: false,
                    states: {
                        hover: {
                            enabled: true,
                            symbol: 'circle',
                            radius: 5,
                            lineWidth: 1
                        }
                    }
                }
            }
        },
        series: [{
            name: $.i18n('Speed')
        }, {
            name: $.i18n('Gust')
        }]
        ,
        navigation: {
            menuItemStyle: {
                fontSize: '10px'
            }
        }
    });

  $.DirChart = $($.content + ' #winddirgraph');
  $.DirChart.highcharts({
      chart: {
          polar: true,
          events: {
              load: function() {
                $.getJSON("json.htm?type=graph&sensor=winddir&idx="+id+"&range=day",
                function(data) {
                      var series = $.DirChart.highcharts().series[0];
                      var datatable = [];
                      
                      $.each(data.result, function(i,item)
                      {
                        datatable.push( [item.dig, parseFloat(item.div) ] );
                      });
                      series.setData(datatable);
                });
              }
          }
        },
        title: {
            text: $.i18n('Wind') + ' ' + $.i18n('Direction') + ' '  + Get5MinuteHistoryDaysGraphTitle()
        },
        pane: {
            startAngle: 0,
            endAngle: 360
        },
        credits: {
          enabled: true,
          href: "http://www.domoticz.com",
          text: "Domoticz.com"
        },
        xAxis: {
            min: 0,
            max: 360,
            tickWidth: 1,
            tickPosition: 'outside',
            tickLength: 20,
            tickColor: '#999',
            tickInterval:45,
            labels: {
                formatter:function(){
                    if(this.value == 0) { return 'N'; }
                    else if(this.value == 45) { return 'NE'; }
                    else if(this.value == 90) { return 'E'; }
                    else if(this.value == 135) { return 'SE'; }
                    else if(this.value == 180) { return 'S'; }
                    else if(this.value == 225) { return 'SW'; }
                    else if(this.value == 270) { return 'W'; }
                    else if(this.value == 315) { return 'NW'; }
                }
            }
        },
        yAxis: {
            min: 0
        },
        tooltip: {
            formatter: function() {
                    return ''+
                    this.x +'\u00B0: '+ this.y +' %';
            }
        },
        plotOptions: {
            area: {
                lineWidth: 2,
                states: {
                    hover: {
                        lineWidth: 2
                    }
                },
                marker: {
                    enabled: false,
                    states: {
                        hover: {
                            enabled: true,
                            symbol: 'circle',
                            radius: 5,
                            lineWidth: 1
                        }
                    }
                }
            }
        },
        series: [{
            type: 'area',
            color: 'rgba(68, 170, 213, 0.2)',
            name: $.i18n('Direction')
        }]
    });

  $.MonthChart = $($.content + ' #windmonthgraph');
  $.MonthChart.highcharts({
      chart: {
          type: 'spline',
          zoomType: 'xy',
          marginRight: 10,
          events: {
              load: function() {
                  
                $.getJSON("json.htm?type=graph&sensor=wind&idx="+id+"&range=month",
                function(data) {
                      var seriessp = $.MonthChart.highcharts().series[0];
                      var seriesgu = $.MonthChart.highcharts().series[1];
                      var datatablesp = [];
                      var datatablegu = [];
                      
                      $.each(data.result, function(i,item)
                      {
                        datatablesp.push( [GetDateFromString(item.d), parseFloat(item.sp) ] );
                        datatablegu.push( [GetDateFromString(item.d), parseFloat(item.gu) ] );
                      });
                      seriessp.setData(datatablesp);
                      seriesgu.setData(datatablegu);
                });
              }
          }
        },
       credits: {
          enabled: true,
          href: "http://www.domoticz.com",
          text: "Domoticz.com"
        },
        title: {
            text: $.i18n('Wind') + ' ' + $.i18n('speed/gust') + ' ' + $.i18n('Last Month')
        },
        xAxis: {
            type: 'datetime'
        },
        yAxis: {
            title: {
                text: $.i18n('Speed') + ' (' + $.myglobals.windsign + ')'
            },
            min: 0,
            minorGridLineWidth: 0,
            gridLineWidth: 0,
            alternateGridColor: null,
            plotBands: [{ // Light air
                from: 0.3*$.myglobals.windscale,
                to: 1.5*$.myglobals.windscale,
                color: 'rgba(68, 170, 213, 0.3)',
                label: {
                    text: $.i18n('Light air'),
                    style: {
                        color: '#CCCCCC'
                    }
                }
            }, { // Light breeze
                from: 1.5*$.myglobals.windscale,
                to: 3.5*$.myglobals.windscale,
                color: 'rgba(68, 170, 213, 0.5)',
                label: {
                    text: $.i18n('Light breeze'),
                    style: {
                        color: '#CCCCCC'
                    }
                }
            }, { // Gentle breeze
                from: 3.5*$.myglobals.windscale,
                to: 5.5*$.myglobals.windscale,
                color: 'rgba(68, 170, 213, 0.3)',
                label: {
                    text: $.i18n('Gentle breeze'),
                    style: {
                        color: '#CCCCCC'
                    }
                }
            }, { // Moderate breeze
                from: 5.5*$.myglobals.windscale,
                to: 8*$.myglobals.windscale,
                color: 'rgba(68, 170, 213, 0.5)',
                label: {
                    text: $.i18n('Moderate breeze'),
                    style: {
                        color: '#CCCCCC'
                    }
                }
            }, { // Fresh breeze
                from: 8*$.myglobals.windscale,
                to: 10.8*$.myglobals.windscale,
                color: 'rgba(68, 170, 213, 0.3)',
                label: {
                    text: $.i18n('Fresh breeze'),
                    style: {
                        color: '#CCCCCC'
                    }
                }
            }, { // Strong breeze
                from: 10.8*$.myglobals.windscale,
                to: 13.9*$.myglobals.windscale,
                color: 'rgba(68, 170, 213, 0.5)',
                label: {
                    text: $.i18n('Strong breeze'),
                    style: {
                        color: '#CCCCCC'
                    }
                }
            }, { // High wind
                from: 13.9*$.myglobals.windscale,
                to: 17.2*$.myglobals.windscale,
                color: 'rgba(68, 170, 213, 0.3)',
                label: {
                    text: $.i18n('High wind'),
                    style: {
                        color: '#CCCCCC'
                    }
                }
            }, { // fresh gale
                from: 17.2*$.myglobals.windscale,
                to: 20.8*$.myglobals.windscale,
                color: 'rgba(68, 170, 213, 0.3)',
                label: {
                    text: $.i18n('Fresh gale'),
                    style: {
                        color: '#CCCCCC'
                    }
                }
            }, { // strong gale
                from: 20.8*$.myglobals.windscale,
                to: 24.5*$.myglobals.windscale,
                color: 'rgba(68, 170, 213, 0.5)',
                label: {
                    text: $.i18n('Strong gale'),
                    style: {
                        color: '#CCCCCC'
                    }
                }
            }, { // storm
                from: 24.5*$.myglobals.windscale,
                to: 28.5*$.myglobals.windscale,
                color: 'rgba(68, 170, 213, 0.3)',
                label: {
                    text: $.i18n('Storm'),
                    style: {
                        color: '#CCCCCC'
                    }
                }
            }, { // Violent storm
                from: 28.5*$.myglobals.windscale,
                to: 32.7*$.myglobals.windscale,
                color: 'rgba(68, 170, 213, 0.5)',
                label: {
                    text: $.i18n('Violent storm'),
                    style: {
                        color: '#CCCCCC'
                    }
                }
            }, { // hurricane
                from: 32.7*$.myglobals.windscale,
                to: 100*$.myglobals.windscale,
                color: 'rgba(68, 170, 213, 0.3)',
                label: {
                    text: $.i18n('Hurricane'),
                    style: {
                        color: '#CCCCCC'
                    }
                }
            }
           ]
        },
        tooltip: {
            formatter: function() {
                    return ''+
                    $.i18n(Highcharts.dateFormat('%A',this.x)) + '<br/>' + Highcharts.dateFormat('%Y-%m-%d %H:%M', this.x) +': '+ this.y +' ' + $.myglobals.windsign;
            }
        },
        plotOptions: {
            spline: {
                lineWidth: 3,
                states: {
                    hover: {
                        lineWidth: 3
                    }
                },
                marker: {
                    enabled: false,
                    states: {
                        hover: {
                            enabled: true,
                            symbol: 'circle',
                            radius: 5,
                            lineWidth: 1
                        }
                    }
                }
            }
        },
        series: [{
            name: $.i18n('Speed'),
			point: {
				events: {
					click: function(event) {
						chartPointClickNew(event,ShowWindLog);
					}
				}
			}
        }, {
            name: $.i18n('Gust'),
			point: {
				events: {
					click: function(event) {
						chartPointClickNew(event,ShowWindLog);
					}
				}
			}
        }]
        ,
        navigation: {
            menuItemStyle: {
                fontSize: '10px'
            }
        }
    });

  $.YearChart = $($.content + ' #windyeargraph');
  $.YearChart.highcharts({
      chart: {
          type: 'spline',
          zoomType: 'xy',
          marginRight: 10,
          events: {
              load: function() {
                  
                $.getJSON("json.htm?type=graph&sensor=wind&idx="+id+"&range=year",
                function(data) {
                      var seriessp = $.YearChart.highcharts().series[0];
                      var seriesgu = $.YearChart.highcharts().series[1];
                      var datatablesp = [];
                      var datatablegu = [];
                      
                      $.each(data.result, function(i,item)
                      {
                        datatablesp.push( [GetDateFromString(item.d), parseFloat(item.sp) ] );
                        datatablegu.push( [GetDateFromString(item.d), parseFloat(item.gu) ] );
                      });
                      seriessp.setData(datatablesp);
                      seriesgu.setData(datatablegu);
                });
              }
          }
        },
       credits: {
          enabled: true,
          href: "http://www.domoticz.com",
          text: "Domoticz.com"
        },
        title: {
            text: $.i18n('Wind') + ' ' + $.i18n('speed/gust') + ' ' + $.i18n('Last Year')
        },
        xAxis: {
            type: 'datetime'
        },
        yAxis: {
            title: {
                text: $.i18n('Speed') + ' (' + $.myglobals.windsign + ')'
            },
            min: 0,
            minorGridLineWidth: 0,
            gridLineWidth: 0,
            alternateGridColor: null,
            plotBands: [{ // Light air
                from: 0.3*$.myglobals.windscale,
                to: 1.5*$.myglobals.windscale,
                color: 'rgba(68, 170, 213, 0.3)',
                label: {
                    text: $.i18n('Light air'),
                    style: {
                        color: '#CCCCCC'
                    }
                }
            }, { // Light breeze
                from: 1.5*$.myglobals.windscale,
                to: 3.5*$.myglobals.windscale,
                color: 'rgba(68, 170, 213, 0.5)',
                label: {
                    text: $.i18n('Light breeze'),
                    style: {
                        color: '#CCCCCC'
                    }
                }
            }, { // Gentle breeze
                from: 3.5*$.myglobals.windscale,
                to: 5.5*$.myglobals.windscale,
                color: 'rgba(68, 170, 213, 0.3)',
                label: {
                    text: $.i18n('Gentle breeze'),
                    style: {
                        color: '#CCCCCC'
                    }
                }
            }, { // Moderate breeze
                from: 5.5*$.myglobals.windscale,
                to: 8*$.myglobals.windscale,
                color: 'rgba(68, 170, 213, 0.5)',
                label: {
                    text: $.i18n('Moderate breeze'),
                    style: {
                        color: '#CCCCCC'
                    }
                }
            }, { // Fresh breeze
                from: 8*$.myglobals.windscale,
                to: 10.8*$.myglobals.windscale,
                color: 'rgba(68, 170, 213, 0.3)',
                label: {
                    text: $.i18n('Fresh breeze'),
                    style: {
                        color: '#CCCCCC'
                    }
                }
            }, { // Strong breeze
                from: 10.8*$.myglobals.windscale,
                to: 13.9*$.myglobals.windscale,
                color: 'rgba(68, 170, 213, 0.5)',
                label: {
                    text: $.i18n('Strong breeze'),
                    style: {
                        color: '#CCCCCC'
                    }
                }
            }, { // High wind
                from: 13.9*$.myglobals.windscale,
                to: 17.2*$.myglobals.windscale,
                color: 'rgba(68, 170, 213, 0.3)',
                label: {
                    text: $.i18n('High wind'),
                    style: {
                        color: '#CCCCCC'
                    }
                }
            }, { // fresh gale
                from: 17.2*$.myglobals.windscale,
                to: 20.8*$.myglobals.windscale,
                color: 'rgba(68, 170, 213, 0.3)',
                label: {
                    text: $.i18n('Fresh gale'),
                    style: {
                        color: '#CCCCCC'
                    }
                }
            }, { // strong gale
                from: 20.8*$.myglobals.windscale,
                to: 24.5*$.myglobals.windscale,
                color: 'rgba(68, 170, 213, 0.5)',
                label: {
                    text: $.i18n('Strong gale'),
                    style: {
                        color: '#CCCCCC'
                    }
                }
            }, { // storm
                from: 24.5*$.myglobals.windscale,
                to: 28.5*$.myglobals.windscale,
                color: 'rgba(68, 170, 213, 0.3)',
                label: {
                    text: $.i18n('Storm'),
                    style: {
                        color: '#CCCCCC'
                    }
                }
            }, { // Violent storm
                from: 28.5*$.myglobals.windscale,
                to: 32.7*$.myglobals.windscale,
                color: 'rgba(68, 170, 213, 0.5)',
                label: {
                    text: $.i18n('Violent storm'),
                    style: {
                        color: '#CCCCCC'
                    }
                }
            }, { // hurricane
                from: 32.7*$.myglobals.windscale,
                to: 100*$.myglobals.windscale,
                color: 'rgba(68, 170, 213, 0.3)',
                label: {
                    text: $.i18n('Hurricane'),
                    style: {
                        color: '#CCCCCC'
                    }
                }
            }
           ]
        },
        tooltip: {
            formatter: function() {
                    return ''+
                    $.i18n(Highcharts.dateFormat('%A',this.x)) + '<br/>' + Highcharts.dateFormat('%Y-%m-%d %H:%M', this.x) +': '+ this.y +' ' + $.myglobals.windsign;
            }
        },
        plotOptions: {
            spline: {
                lineWidth: 3,
                states: {
                    hover: {
                        lineWidth: 3
                    }
                },
                marker: {
                    enabled: false,
                    states: {
                        hover: {
                            enabled: true,
                            symbol: 'circle',
                            radius: 5,
                            lineWidth: 1
                        }
                    }
                }
            }
        },
        series: [{
            name: $.i18n('Speed'),
			point: {
				events: {
					click: function(event) {
						chartPointClickNew(event,ShowWindLog);
					}
				}
			}
        }, {
            name: $.i18n('Gust'),
			point: {
				events: {
					click: function(event) {
						chartPointClickNew(event,ShowWindLog);
					}
				}
			}
        }]
        ,
        navigation: {
            menuItemStyle: {
                fontSize: '10px'
            }
        }
    });
  $('#modal').hide();
  cursordefault();
  return false;
}

function ShowRainLog(contentdiv,backfunction,id,name)
{
	clearInterval($.myglobals.refreshTimer);
  $('#modal').show();
  $.content=contentdiv;
  $.backfunction=backfunction;
  $.devIdx=id;
  $.devName=name;
  var htmlcontent = '';
  htmlcontent='<p><center><h2>' + name + '</h2></center></p>\n';
  htmlcontent+=$('#daymonthyearlog').html();
  $($.content).html(GetBackbuttonHTMLTable(backfunction)+htmlcontent);
  $($.content).i18n();
  
  $.DayChart = $($.content + ' #daygraph');
  $.DayChart.highcharts({
      chart: {
          type: 'column',
          marginRight: 10,
          events: {
              load: function() {
                  
                $.getJSON("json.htm?type=graph&sensor=rain&idx="+id+"&range=day",
                function(data) {
                      var series = $.DayChart.highcharts().series[0];
                      var datatable = [];
                      
                      $.each(data.result, function(i,item)
                      {
                        datatable.push( [GetDateFromString(item.d), parseFloat(item.mm) ] );
                      });
                      series.setData(datatable);
                });
              }
          }
        },
       credits: {
          enabled: true,
          href: "http://www.domoticz.com",
          text: "Domoticz.com"
        },
        title: {
            text: $.i18n('Rainfall') + ' ' + $.i18n('Last Week')
        },
        xAxis: {
            type: 'datetime',
            dateTimeLabelFormats: {
                day: '%a'
            },
            tickInterval: 24 * 3600 * 1000
        },
        yAxis: {
            title: {
                text: $.i18n('Rainfall') + ' (mm)'
            },
            min: 0
        },
        tooltip: {
            formatter: function() {
                    return ''+
                    $.i18n(Highcharts.dateFormat('%A',this.x)) + '<br/>' + Highcharts.dateFormat('%Y-%m-%d', this.x) +': '+ this.y +' mm';
            }
        },
        plotOptions: {
            column: {
                minPointLength: 4,
                pointPadding: 0.1,
                groupPadding: 0
            }
        },
        series: [{
            name: 'mm',
            color: 'rgba(3,190,252,0.8)',
            dataLabels: {
                    verticalAlign: 'top',
                    enabled: true,
                    color: '#FFFFFF',
                    y: -23,
                    formatter: function() {
                        return this.y;
                    },
                    style: {
                        fontSize: '13px',
                        fontFamily: 'Verdana, sans-serif'
                    }
                }
        }]
        ,
        legend: {
            enabled: false
        }
    });

  $.MonthChart = $($.content + ' #monthgraph');
  $.MonthChart.highcharts({
      chart: {
          type: 'spline',
          marginRight: 10,
          zoomType: 'xy',
          events: {
              load: function() {
                $.getJSON("json.htm?type=graph&sensor=rain&idx="+id+"&range=month",
                function(data) {
                      var series = $.MonthChart.highcharts().series[0];
                      var datatable = [];
                      
                      $.each(data.result, function(i,item)
                      {
                        datatable.push( [GetDateFromString(item.d), parseFloat(item.mm) ] );
                      });
                      series.setData(datatable);
                });
              }
          }
        },
       credits: {
          enabled: true,
          href: "http://www.domoticz.com",
          text: "Domoticz.com"
        },
        title: {
            text: $.i18n('Rainfall') + ' ' + $.i18n('Last Month')
        },
        xAxis: {
            type: 'datetime'
        },
        yAxis: {
            title: {
                text: $.i18n('Rainfall') + ' (mm)'
            },
            min: 0
        },
        tooltip: {
            formatter: function() {
                    return ''+
                    $.i18n(Highcharts.dateFormat('%A',this.x)) + '<br/>' + Highcharts.dateFormat('%Y-%m-%d', this.x) +': '+ this.y +' mm';
            }
        },
        plotOptions: {
           spline: {
                lineWidth: 3,
                states: {
                    hover: {
                        lineWidth: 3
                    }
                },
                marker: {
                    enabled: false,
                    states: {
                        hover: {
                            enabled: true,
                            symbol: 'circle',
                            radius: 5,
                            lineWidth: 1
                        }
                    }
                }
            }
        },
        series: [{
            name: 'mm',
            color: 'rgba(3,190,252,0.8)',
			point: {
				events: {
					click: function(event) {
						chartPointClickNew(event,ShowRainLog);
					}
				}
			}
        }]
        ,
        legend: {
            enabled: false
        }
    });

  $.YearChart = $($.content + ' #yeargraph');
  $.YearChart.highcharts({
      chart: {
          type: 'spline',
          marginRight: 10,
          zoomType: 'xy',
          events: {
              load: function() {
                  
                $.getJSON("json.htm?type=graph&sensor=rain&idx="+id+"&range=year",
                function(data) {
                      var series = $.YearChart.highcharts().series[0];
                      var datatable = [];
                      
                      $.each(data.result, function(i,item)
                      {
                        datatable.push( [GetDateFromString(item.d), parseFloat(item.mm) ] );
                      });
                      series.setData(datatable);
                });
              }
          }
        },
       credits: {
          enabled: true,
          href: "http://www.domoticz.com",
          text: "Domoticz.com"
        },
        title: {
            text: $.i18n('Rainfall') + ' ' + $.i18n('Last Year')
        },
        xAxis: {
            type: 'datetime'
        },
        yAxis: {
            title: {
                text: $.i18n('Rainfall') + ' (mm)'
            },
            min: 0
        },
        tooltip: {
            formatter: function() {
                    return ''+
                    $.i18n(Highcharts.dateFormat('%A',this.x)) + '<br/>' + Highcharts.dateFormat('%Y-%m-%d', this.x) +': '+ this.y +' mm';
            }
        },
        plotOptions: {
           spline: {
                lineWidth: 3,
                states: {
                    hover: {
                        lineWidth: 3
                    }
                },
                marker: {
                    enabled: false,
                    states: {
                        hover: {
                            enabled: true,
                            symbol: 'circle',
                            radius: 5,
                            lineWidth: 1
                        }
                    }
                }
            }
        },
        series: [{
            name: 'mm',
            color: 'rgba(3,190,252,0.8)',
			point: {
				events: {
					click: function(event) {
						chartPointClickNew(event,ShowRainLog);
					}
				}
			}
        }]
        ,
        legend: {
            enabled: false
        }
    });
  $('#modal').hide();
  cursordefault();
  return false;
}

function ShowBaroLog(contentdiv,backfunction,id,name)
{
	clearInterval($.myglobals.refreshTimer);
  $('#modal').show();
  $.content=contentdiv;
  $.backfunction=backfunction;
  $.devIdx=id;
  $.devName=name;
  var htmlcontent = '';
  htmlcontent='<p><center><h2>' + name + '</h2></center></p><br>\n';
  htmlcontent+=$('#daymonthyearlog').html();
  $($.content).html(GetBackbuttonHTMLTable(backfunction)+htmlcontent);
  $($.content).i18n();
  
  $.DayChart = $($.content + ' #daygraph');
  $.DayChart.highcharts({
      chart: {
          type: 'spline',
          zoomType: 'xy',
          marginRight: 10,
          events: {
              load: function() {
                  
                $.getJSON("json.htm?type=graph&sensor=temp&idx="+id+"&range=day",
                function(data) {
                      var series = $.DayChart.highcharts().series[0];
                      var datatable = [];
                      $.each(data.result, function(i,item)
                      {
                        datatable.push( [GetUTCFromString(item.d), parseFloat(item.ba) ] );
                      });
                      series.setData(datatable);
                });
              }
          }
        },
       credits: {
          enabled: true,
          href: "http://www.domoticz.com",
          text: "Domoticz.com"
        },
        title: {
            text: $.i18n('Barometer') + ' '  + Get5MinuteHistoryDaysGraphTitle()
        },
        xAxis: {
            type: 'datetime'
        },
        yAxis: {
            title: {
                text: $.i18n('Pressure') + ' (hPa)'
            },
            labels: {
							formatter: function(){
									return this.value;       
							}
						}
        },
        tooltip: {
            formatter: function() {
                    return ''+
                    $.i18n(Highcharts.dateFormat('%A',this.x)) + '<br/>' + Highcharts.dateFormat('%Y-%m-%d %H:%M', this.x) +': '+ this.y +' hPa';
            }
        },
        plotOptions: {
            spline: {
                lineWidth: 3,
                states: {
                    hover: {
                        lineWidth: 3
                    }
                },
                marker: {
                    enabled: false,
                    states: {
                        hover: {
                            enabled: true,
                            symbol: 'circle',
                            radius: 5,
                            lineWidth: 1
                        }
                    }
                }
            }
        },
        series: [{
            name: 'hPa'
        }]
        ,
        navigation: {
            menuItemStyle: {
                fontSize: '10px'
            }
        }
    });

  $.MonthChart = $($.content + ' #monthgraph');
  $.MonthChart.highcharts({
      chart: {
          type: 'spline',
          zoomType: 'xy',
          marginRight: 10,
          events: {
              load: function() {
                  
                $.getJSON("json.htm?type=graph&sensor=temp&idx="+id+"&range=month",
                function(data) {
                      var series = $.MonthChart.highcharts().series[0];
                      var datatable = [];
                      
                      $.each(data.result, function(i,item)
                      {
                        datatable.push( [GetDateFromString(item.d), parseFloat(item.ba) ] );
                      });
                      series.setData(datatable);
                });
              }
          }
        },
       credits: {
          enabled: true,
          href: "http://www.domoticz.com",
          text: "Domoticz.com"
        },
        title: {
            text: $.i18n('Barometer') + ' ' + $.i18n('Last Month')
        },
        xAxis: {
            type: 'datetime'
        },
        yAxis: {
            title: {
                text: $.i18n('Pressure') + ' (hPa)'
            },
            labels: {
							formatter: function(){
									return this.value;       
							}
						}
        },
        tooltip: {
            formatter: function() {
                    return ''+
                    $.i18n(Highcharts.dateFormat('%A',this.x)) + '<br/>' + Highcharts.dateFormat('%Y-%m-%d %H:%M', this.x) +': '+ this.y +' hPa';
            }
        },
        plotOptions: {
            spline: {
                lineWidth: 3,
                states: {
                    hover: {
                        lineWidth: 3
                    }
                },
                marker: {
                    enabled: false,
                    states: {
                        hover: {
                            enabled: true,
                            symbol: 'circle',
                            radius: 5,
                            lineWidth: 1
                        }
                    }
                }
            }
        },
        series: [{
            name: 'hPa',
			point: {
				events: {
					click: function(event) {
						chartPointClickNew(event,ShowBaroLog);
					}
				}
			}
        }]
        ,
        navigation: {
            menuItemStyle: {
                fontSize: '10px'
            }
        }
    });

  $.YearChart = $($.content + ' #yeargraph');
  $.YearChart.highcharts({
      chart: {
          type: 'spline',
          zoomType: 'xy',
          marginRight: 10,
          events: {
              load: function() {
                  
                $.getJSON("json.htm?type=graph&sensor=temp&idx="+id+"&range=year",
                function(data) {
                      var series = $.YearChart.highcharts().series[0];
                      var datatable = [];
                      
                      $.each(data.result, function(i,item)
                      {
                        datatable.push( [GetDateFromString(item.d), parseFloat(item.ba) ] );
                      });
                      series.setData(datatable);
                });
              }
          }
        },
       credits: {
          enabled: true,
          href: "http://www.domoticz.com",
          text: "Domoticz.com"
        },
        title: {
            text: $.i18n('Barometer') + ' ' + $.i18n('Last Year')
        },
        xAxis: {
            type: 'datetime'
        },
        yAxis: {
            title: {
                text: $.i18n('Pressure') + ' (hPa)'
            },
            labels: {
							formatter: function(){
									return this.value;       
							}
						}
        },
        tooltip: {
            formatter: function() {
                    return ''+
                    $.i18n(Highcharts.dateFormat('%A',this.x)) + '<br/>' + Highcharts.dateFormat('%Y-%m-%d %H:%M', this.x) +': '+ this.y +' hPa';
            }
        },
        plotOptions: {
            spline: {
                lineWidth: 3,
                states: {
                    hover: {
                        lineWidth: 3
                    }
                },
                marker: {
                    enabled: false,
                    states: {
                        hover: {
                            enabled: true,
                            symbol: 'circle',
                            radius: 5,
                            lineWidth: 1
                        }
                    }
                }
            }
        },
        series: [{
            name: 'hPa',
			point: {
				events: {
					click: function(event) {
						chartPointClickNew(event,ShowBaroLog);
					}
				}
			}
        }]
        ,
        navigation: {
            menuItemStyle: {
                fontSize: '10px'
            }
        }
    });
  $('#modal').hide();
  cursordefault();
  return false;
}

function ShowAirQualityLog(contentdiv,backfunction,id,name)
{
	clearInterval($.myglobals.refreshTimer);
  $('#modal').show();
  $.content=contentdiv;
  $.backfunction=backfunction;
  $.devIdx=id;
  $.devName=name;
  var htmlcontent = '';
  htmlcontent='<p><center><h2>' + name + '</h2></center></p>\n';
  htmlcontent+=$('#daymonthyearlog').html();
  $($.content).html(GetBackbuttonHTMLTable(backfunction)+htmlcontent);
  $($.content).i18n();

  $.DayChart = $($.content + ' #daygraph');
  $.DayChart.highcharts({
      chart: {
          type: 'spline',
          zoomType: 'xy',
          marginRight: 10,
          events: {
              load: function() {
                  
                $.getJSON("json.htm?type=graph&sensor=counter&idx="+id+"&range=day",
                function(data) {
                      var series = $.DayChart.highcharts().series[0];
                      var datatable = [];
                      
                      $.each(data.result, function(i,item)
                      {
                        datatable.push( [GetUTCFromString(item.d), parseInt(item.co2) ] );
                      });
                      series.setData(datatable);
                });
              }
          }
        },
       credits: {
          enabled: true,
          href: "http://www.domoticz.com",
          text: "Domoticz.com"
        },
        title: {
            text: $.i18n('Air Quality') + ' '  + Get5MinuteHistoryDaysGraphTitle()
        },
        xAxis: {
            type: 'datetime'
        },
        yAxis: {
            title: {
                text: 'co2 (ppm)'
            },
            min: 0,
            minorGridLineWidth: 0,
            gridLineWidth: 0,
            alternateGridColor: null,
            plotBands: [{ // Excellent
                from: 0,
                to: 700,
                color: 'rgba(68, 170, 213, 0.3)',
                label: {
                    text: $.i18n('Excellent'),
                    style: {
                        color: '#CCCCCC'
                    }
                }
            }, { // Good
                from: 700,
                to: 900,
                color: 'rgba(68, 170, 213, 0.5)',
                label: {
                    text: $.i18n('Good'),
                    style: {
                        color: '#CCCCCC'
                    }
                }
            }, { // Fair
                from: 900,
                to: 1100,
                color: 'rgba(68, 170, 213, 0.3)',
                label: {
                    text: $.i18n('Fair'),
                    style: {
                        color: '#CCCCCC'
                    }
                }
            }, { // Mediocre
                from: 1100,
                to: 1600,
                color: 'rgba(68, 170, 213, 0.5)',
                label: {
                    text: $.i18n('Mediocre'),
                    style: {
                        color: '#CCCCCC'
                    }
                }
            }, { // Bad
                from: 1600,
                to: 6000,
                color: 'rgba(68, 170, 213, 0.3)',
                label: {
                    text: $.i18n('Bad'),
                    style: {
                        color: '#CCCCCC'
                    }
                }
            }
           ]
        },
        tooltip: {
            formatter: function() {
                    return ''+
                    $.i18n(Highcharts.dateFormat('%A',this.x)) + '<br/>' + Highcharts.dateFormat('%Y-%m-%d %H:%M', this.x) +': '+ this.y +' ppm';
            }
        },
        plotOptions: {
            spline: {
                lineWidth: 3,
                states: {
                    hover: {
                        lineWidth: 3
                    }
                },
                marker: {
                    enabled: false,
                    states: {
                        hover: {
                            enabled: true,
                            symbol: 'circle',
                            radius: 5,
                            lineWidth: 1
                        }
                    }
                }
            }
        },
        series: [{
            name: 'co2'
        }]
        ,
        navigation: {
            menuItemStyle: {
                fontSize: '10px'
            }
        }
    });

  $.MonthChart = $($.content + ' #monthgraph');
  $.MonthChart.highcharts({
      chart: {
          type: 'spline',
          zoomType: 'xy',
          marginRight: 10,
          events: {
              load: function() {
                  
                $.getJSON("json.htm?type=graph&sensor=counter&idx="+id+"&range=month",
                function(data) {
                      var datatable1 = [];
                      var datatable2 = [];
                      
                      $.each(data.result, function(i,item)
                      {
                        datatable1.push( [GetDateFromString(item.d), parseInt(item.co2_min) ] );
                        datatable2.push( [GetDateFromString(item.d), parseInt(item.co2_max) ] );
                      });
                      var series1 = $.MonthChart.highcharts().series[0];
                      var series2 = $.MonthChart.highcharts().series[1];
                      series1.setData(datatable1);
                      series2.setData(datatable2);
                });
              }
          }
        },
       credits: {
          enabled: true,
          href: "http://www.domoticz.com",
          text: "Domoticz.com"
        },
        title: {
            text: $.i18n('Air Quality') + ' ' + $.i18n('Last Month')
        },
        xAxis: {
            type: 'datetime'
        },
        yAxis: {
            title: {
                text: 'co2 (ppm)'
            },
            min: 0,
            minorGridLineWidth: 0,
            gridLineWidth: 0,
            alternateGridColor: null,
            plotBands: [{ // Excellent
                from: 0,
                to: 700,
                color: 'rgba(68, 170, 213, 0.3)',
                label: {
                    text: $.i18n('Excellent'),
                    style: {
                        color: '#CCCCCC'
                    }
                }
            }, { // Good
                from: 700,
                to: 900,
                color: 'rgba(68, 170, 213, 0.5)',
                label: {
                    text: $.i18n('Good'),
                    style: {
                        color: '#CCCCCC'
                    }
                }
            }, { // Fair
                from: 900,
                to: 1100,
                color: 'rgba(68, 170, 213, 0.3)',
                label: {
                    text: $.i18n('Fair'),
                    style: {
                        color: '#CCCCCC'
                    }
                }
            }, { // Mediocre
                from: 1100,
                to: 1600,
                color: 'rgba(68, 170, 213, 0.5)',
                label: {
                    text: $.i18n('Mediocre'),
                    style: {
                        color: '#CCCCCC'
                    }
                }
            }, { // Bad
                from: 1600,
                to: 6000,
                color: 'rgba(68, 170, 213, 0.3)',
                label: {
                    text: $.i18n('Bad'),
                    style: {
                        color: '#CCCCCC'
                    }
                }
            }
           ]
        },
        tooltip: {
            formatter: function() {
                    return ''+
                    $.i18n(Highcharts.dateFormat('%A',this.x)) + '<br/>' + Highcharts.dateFormat('%Y-%m-%d %H:%M', this.x) +': '+ this.y +' ppm';
            }
        },
        plotOptions: {
            spline: {
                lineWidth: 3,
                states: {
                    hover: {
                        lineWidth: 3
                    }
                },
                marker: {
                    enabled: false,
                    states: {
                        hover: {
                            enabled: true,
                            symbol: 'circle',
                            radius: 5,
                            lineWidth: 1
                        }
                    }
                }
            }
        },
        series: [{
            name: 'co2_min',
			point: {
				events: {
					click: function(event) {
						chartPointClickNew(event,ShowAirQualityLog);
					}
				}
			}
		}, {
            name: 'co2_max',
			point: {
				events: {
					click: function(event) {
						chartPointClickNew(event,ShowAirQualityLog);
					}
				}
			}
        }]
        ,
        navigation: {
            menuItemStyle: {
                fontSize: '10px'
            }
        }
    });

  $.YearChart = $($.content + ' #yeargraph');
  $.YearChart.highcharts({
      chart: {
          type: 'spline',
          zoomType: 'xy',
          marginRight: 10,
          events: {
              load: function() {
                  
                $.getJSON("json.htm?type=graph&sensor=counter&idx="+id+"&range=year",
                function(data) {
                      var datatable1 = [];
                      var datatable2 = [];
                      
                      $.each(data.result, function(i,item)
                      {
                        datatable1.push( [GetDateFromString(item.d), parseInt(item.co2_min) ] );
                        datatable2.push( [GetDateFromString(item.d), parseInt(item.co2_max) ] );
                      });
                      var series1 = $.YearChart.highcharts().series[0];
                      var series2 = $.YearChart.highcharts().series[1];
                      series1.setData(datatable1);
                      series2.setData(datatable2);
                });
              }
          }
        },
       credits: {
          enabled: true,
          href: "http://www.domoticz.com",
          text: "Domoticz.com"
        },
        title: {
            text: $.i18n('Air Quality') + ' ' + $.i18n('Last Year')
        },
        xAxis: {
            type: 'datetime'
        },
        yAxis: {
            title: {
                text: 'co2 (ppm)'
            },
            min: 0,
            minorGridLineWidth: 0,
            gridLineWidth: 0,
            alternateGridColor: null,
            plotBands: [{ // Excellent
                from: 0,
                to: 700,
                color: 'rgba(68, 170, 213, 0.3)',
                label: {
                    text: $.i18n('Excellent'),
                    style: {
                        color: '#CCCCCC'
                    }
                }
            }, { // Good
                from: 700,
                to: 900,
                color: 'rgba(68, 170, 213, 0.5)',
                label: {
                    text: $.i18n('Good'),
                    style: {
                        color: '#CCCCCC'
                    }
                }
            }, { // Fair
                from: 900,
                to: 1100,
                color: 'rgba(68, 170, 213, 0.3)',
                label: {
                    text: $.i18n('Fair'),
                    style: {
                        color: '#CCCCCC'
                    }
                }
            }, { // Mediocre
                from: 1100,
                to: 1600,
                color: 'rgba(68, 170, 213, 0.5)',
                label: {
                    text: $.i18n('Mediocre'),
                    style: {
                        color: '#CCCCCC'
                    }
                }
            }, { // Bad
                from: 1600,
                to: 6000,
                color: 'rgba(68, 170, 213, 0.3)',
                label: {
                    text: $.i18n('Bad'),
                    style: {
                        color: '#CCCCCC'
                    }
                }
            }
           ]
        },
        tooltip: {
            formatter: function() {
                    return ''+
                    $.i18n(Highcharts.dateFormat('%A',this.x)) + '<br/>' + Highcharts.dateFormat('%Y-%m-%d %H:%M', this.x) +': '+ this.y +' ppm';
            }
        },
        plotOptions: {
            spline: {
                lineWidth: 3,
                states: {
                    hover: {
                        lineWidth: 3
                    }
                },
                marker: {
                    enabled: false,
                    states: {
                        hover: {
                            enabled: true,
                            symbol: 'circle',
                            radius: 5,
                            lineWidth: 1
                        }
                    }
                }
            }
        },
        series: [{
            name: 'co2_min',
			point: {
				events: {
					click: function(event) {
						chartPointClickNew(event,ShowAirQualityLog);
					}
				}
			}
		}, {
            name: 'co2_max',
			point: {
				events: {
					click: function(event) {
						chartPointClickNew(event,ShowAirQualityLog);
					}
				}
			}
        }]
        ,
        navigation: {
            menuItemStyle: {
                fontSize: '10px'
            }
        }
    });
  $('#modal').hide();
  cursordefault();
  return false;
}
