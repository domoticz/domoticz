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
        '\t    <a class="btnstylerev" onclick="' + backfunction + '()">Back</a>\n' +
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
        '\t    <a class="btnstylerev" onclick="' + backfunction + '()">Back</a>\n' +
        '\t  </td>\n' +
        '\t  <td align="right">\n' +
        '\t    <a class="btnstyle" onclick="TransferSensor(' + id + ')">Transfer</a>\n' +
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

function SwitchLight(idx,switchcmd, refreshfunction)
{
  clearInterval($.myglobals.refreshTimer);
  ShowNotify('Switching ' + switchcmd);
 
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
        alert('Problem sending switch command');
     }     
  });
}

function ResetSecurityStatus(idx,switchcmd, refreshfunction)
{
  clearInterval($.myglobals.refreshTimer);
  ShowNotify('Switching ' + switchcmd);
 
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
        alert('Problem sending switch command');
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
        $.each(data.result, function(i,item){
          var addId = oTable.fnAddData([
                      item.Date,
                      item.Data
                    ],false);
        });
        mTable.fnDraw();
      }
     }
  });
}

function ClearLightLog()
{
	var bValid = false;
	bValid=(confirm("Are you sure to delete the Log?\n\nThis action can not be undone!!")==true);
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
				ShowNotify('Problem clearing the Log!', 2500, true);
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
  htmlcontent='<p><h2>Name: ' + name + '</h2></p><br>\n';
  htmlcontent+=$('#lightlog').html();
  $($.content).html(GetBackbuttonHTMLTable(backfunction)+htmlcontent);
  
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
	nsettings.when=$($.content + " #notificationparamstable #combowhen").val();
	nsettings.value=$($.content + " #notificationparamstable #value").val();
	if ((nsettings.value=="")||(isNaN(nsettings.value)))
	{
		ShowNotify('Please correct the Value!', 2500, true);
		return null;
	}

	return nsettings;
}

function ClearNotifications()
{
	var bValid = false;
	bValid=(confirm("Are you sure to delete ALL notifications?\n\nThis action can not be undone!!")==true);
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
				ShowNotify('Problem clearing notifications!', 2500, true);
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
				ShowNotify('Problem updating notification!', 2500, true);
		 }     
	});
}

function DeleteNotification(idx)
{
	var bValid = false;
	bValid=(confirm("Are you sure to delete this notification?\n\nThis action can not be undone...")==true);
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
				ShowNotify('Problem deleting notification!', 2500, true);
		 }     
	});
}

function AddNotification()
{
	var nsettings=GetNotificationSettings();
	if (nsettings==null)
		return;
	$.ajax({
		 url: "json.htm?type=command&param=addnotification&idx=" + $.devIdx + 
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
				ShowNotify('Problem adding notification!', 2500, true);
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

					var nvalue=parts[2];
					var whenstr = "";
					
					var ntype="";
					var stype="";
					if (parts[0]=="T")
					{
						ntype="Temperature";
						stype=" &deg; C";
					}
					else if (parts[0]=="H")
					{
						ntype="Humidity";
						stype=" %";
					}
					else if (parts[0]=="R")
					{
						ntype="Rain";
						stype=" mm";
					}
					else if (parts[0]=="W")
					{
						ntype="Wind";
						stype=" m/s";
					}
					else if (parts[0]=="U")
					{
						ntype="UV";
						stype=" UVI";
					}
					else if (parts[0]=="M")
					{
						ntype="Usage";
					}
					else if (parts[0]=="B")
					{
						ntype="Baro";
						stype=" hPa";
					}
					else if (parts[0]=="S")
					{
						ntype="Switch";
					}
					else if (parts[0]=="E")
					{
						ntype="Today";
						stype=" kWh";
					}
					else if (parts[0]=="G")
					{
						ntype="Today";
						stype=" m3";
					}
					else if (parts[0]=="1")
					{
						ntype="Ampere 1";
						stype=" A";
					}
					else if (parts[0]=="2")
					{
						ntype="Ampere 2";
						stype=" A";
					}
					else if (parts[0]=="3")
					{
						ntype="Ampere 3";
						stype=" A";
					}
						
					var nwhen="";
					if (parts[1]==">") {
						nwhen="Greater then ";
					}
					else {
						nwhen="Below ";
					}
					whenstr= nwhen + nvalue + stype;
					
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
										if (data["1"].match("^Greater")) {
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
		if (typetext == 'Temperature')
			$($.content + " #notificationparamstable #valuetype").html('&nbsp;&deg; C');
		else if (typetext == 'Humidity')
			$($.content + " #notificationparamstable #valuetype").html('&nbsp;%');
		else if (typetext == 'UV')
			$($.content + " #notificationparamstable #valuetype").html('&nbsp;UVI');
		else if (typetext == 'Rain')
			$($.content + " #notificationparamstable #valuetype").html('&nbsp;mm');
		else if (typetext == 'Wind')
			$($.content + " #notificationparamstable #valuetype").html('&nbsp;m/s');
		else if (typetext == 'Baro')
			$($.content + " #notificationparamstable #valuetype").html('&nbsp;hPa');
		else if (typetext == 'Usage')
			$($.content + " #notificationparamstable #valuetype").html('&nbsp;');
		else if (typetext == 'Today')
			$($.content + " #notificationparamstable #valuetype").html('&nbsp;');
		else if (typetext == 'Total')
			$($.content + " #notificationparamstable #valuetype").html('&nbsp;');
		else if (typetext == 'Ampere 1')
			$($.content + " #notificationparamstable #valuetype").html('&nbsp;A');
		else if (typetext == 'Ampere 2')
			$($.content + " #notificationparamstable #valuetype").html('&nbsp;A');
		else if (typetext == 'Ampere 3')
			$($.content + " #notificationparamstable #valuetype").html('&nbsp;A');
		else
			$($.content + " #notificationparamstable #valuetype").html('&nbsp;??');
}

function GetValTextInNTypeStrArray(stext)
{
	var pos=-1;
	$.each($.NTypeStr, function(i,item){
		if (item.text == stext)
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
				
				//add types to combobox
				$.each($.NTypeStr, function(i,item){
					var option = $('<option />');
					option.attr('value', item.val).text(item.text);
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
				ShowNotify('Problem clearing notifications!', 2500, true);
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

function SwitchLayout(layout)
{
	clearInterval($.myglobals.refreshTimer);
	$.myglobals.prevlayout = $.myglobals.layout;
	$.myglobals.actlayout = layout;
	
	//update menu items
	ChangeClass("mDashboard","");
	ChangeClass("mLightSwitches","");
	ChangeClass("mTemperature","");
	ChangeClass("mWeather","");
	ChangeClass("mUtility","");
	ChangeClass("mDevices","");
	ChangeClass("mHardware","");
	ChangeClass("mSetup","");
	
	var durl='';
	
	if (layout == "Dashboard") {
		durl='dashboard.html';
		ChangeClass("mDashboard","current_page_item");
	}
	else if (layout == "LightSwitches") {
		durl='lights.html';
		ChangeClass("mLightSwitches","current_page_item");
	}
	else if (layout == "Temperature") {
		durl='temperature.html';
		ChangeClass("mTemperature","current_page_item");
	}
	else if (layout == "Weather") {
		durl='weather.html';
		ChangeClass("mWeather","current_page_item");
	}
	else if (layout == "Utility") {
		durl='utility.html';
		ChangeClass("mUtility","current_page_item");
	}
	else if (layout == "Devices") {
		durl='devices.html';
		ChangeClass("mDevices","current_page_item");
	}
	else if (layout == "Hardware") {
		durl='hardware.html';
		ChangeClass("mHardware","current_page_item");
	}
	else if (layout == "Setup") {
		durl='setup.html';
		ChangeClass("mSetup","current_page_item");
	}
	else
		return;
		
	//stop chaching these pages
	var dt = new Date();
	durl+='?sid='+dt.getTime();
	
	$(".bannercontent").load(durl, function(response, status, xhr) {
		if (status == "error") {
			var msg = "Sorry but there was an error: ";
			$(".bannercontent").html(msg + xhr.status + " " + xhr.statusText);
		}
	});
	
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
	$.ajax({
		 url: "json.htm?type=command&param=switchlight&idx=" + idx + "&switchcmd=Set%20Level&level=" + value,
		 async: false, 
		 dataType: 'json'
	});
}

//Some helper for browser detection
function matchua ( ua ) {
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
};
