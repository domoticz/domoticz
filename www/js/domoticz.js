﻿/*
 (c) 2012-2017 Domoticz.com, Robbert E. Peters
*/
jQuery.fn.center = function (parent) {
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

// Add custom behaviour to highcharts.
if (typeof (Highcharts) !== 'undefined') {
	if (typeof (Storage) !== 'undefined') {
		(function (H_) {
			/*
					// Use this code to debug excessive redrawing of graphs.
					H_.wrap( H_.Chart.prototype, 'redraw', function ( fProceed_ ) {
						console.log( 'draw ' + $( this.container ).parent().attr( 'id' ) );
						fProceed_.apply( this, Array.prototype.slice.call( arguments, 1 ) );
					} );
			*/

			H_.wrap(H_.Series.prototype, 'setVisible', function (fProceed_) {
				var iVisibles = 0, me = this;
				$.each(this.chart.series, function (iIndex_, oSerie_) {
					if (
						oSerie_ != me
						&& oSerie_.visible
					) {
						iVisibles++;
					}
				});
				if (
					iVisibles > 0
					|| this.visible == false
				) {
					fProceed_.apply(this, Array.prototype.slice.call(arguments, 1));
					try {
						var sStorageId = 'highcharts_series_visibility_' + $.devIdx;
						var sStateId = $(this.chart.container).parent().attr('id') + '_' + this.options.id;
						var sCurrentState = localStorage.getItem(sStorageId) || '{}';
						var oCurrentState = JSON.parse(sCurrentState);
						oCurrentState[sStateId] = this.visible;
						localStorage.setItem(sStorageId, JSON.stringify(oCurrentState));
					} catch (oException_) { /* too bad, no state */ }
				}
			});

			H_.wrap(H_.Series.prototype, 'init', function (fProceed_, oChart_, oOptions_) {
				try {
					var sStorageId = 'highcharts_series_visibility_' + $.devIdx;
					var sStateId = $(oChart_.container).parent().attr('id') + '_' + oOptions_.id;
					var sCurrentState = localStorage.getItem(sStorageId) || '{}';
					var oCurrentState = JSON.parse(sCurrentState);
					if (sStateId in oCurrentState) {
						oOptions_.visible = oCurrentState[sStateId];
					}
				} catch (oException_) { /* too bad, no state */ }
				fProceed_.apply(this, Array.prototype.slice.call(arguments, 1));
			});
		}(Highcharts));
	}
}

/* Get the rows which are currently selected */
function fnGetSelected(oTableLocal) {
	return oTableLocal.$('tr.row_selected');
}

function GetBackbuttonHTMLTable(backfunction) {
	var xhtm =
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

function GetBackbuttonHTMLTableWithRight(backfunction, rightfunction, rightlabel) {
	var xhtm =
		'\t<table class="bannav" id="bannav" border="0" cellpadding="0" cellspacing="0" width="100%">\n' +
		'\t<tr>\n' +
		'\t  <td align="left">\n' +
		'\t    <a class="btnstylerev" onclick="' + backfunction + '()" data-i18n="Back">Back</a>\n' +
		'\t  </td>\n' +
		'\t  <td align="right">\n' +
		'\t    <a class="btnstyle" onclick="' + rightfunction + '" data-i18n="' + rightlabel + '">' + rightlabel + '</a>\n' +
		'\t  </td>\n' +
		'\t</tr>\n' +
		'\t</table>\n' +
		'\t<br>\n';
	return xhtm;
}

function HandleProtection(isprotected, callbackfunction) {
	if (typeof isprotected == 'undefined') {
		callbackfunction("");
		return;
	}
	if (isprotected == false) {
		callbackfunction("");
		return;
	}
	bootbox.prompt($.t("Please enter Password") + ":", function (result) {
		if (result === null) {
			return;
		} else {
			if (result == "") {
				return;
			}
			//verify password
			$.ajax({
				url: "json.htm?type=command&param=verifypasscode" +
				"&passcode=" + result,
				async: false,
				dataType: 'json',
				success: function (data) {
					if (data.status == "OK") {
						callbackfunction(result);
					}
				},
				error: function () {
				}
			});
		}
	});
}

function CalculateTrendLine(data) {
	//function taken from jquery.flot.trendline.js
	var ii = 0, x, y, x0, x1, y0, y1, dx,
		m = 0, b = 0, cs, ns,
		n = data.length, Sx = 0, Sy = 0, Sxy = 0, Sx2 = 0, S2x = 0;

	// Not enough data
	if (n < 2) return;

	// Do math stuff
	for (ii; ii < data.length; ii++) {
		x = data[ii][0];
		y = data[ii][1];
		Sx += x;
		Sy += y;
		Sxy += (x * y);
		Sx2 += (x * x);
	}
	// Calculate slope and intercept
	m = (n * Sx2 - S2x) != 0 ? (n * Sxy - Sx * Sy) / (n * Sx2 - Sx * Sx) : 0;
	b = (Sy - m * Sx) / n;

	// Calculate minimal coordinates to draw the trendline
	dx = 0;// parseFloat(data[1][0]) - parseFloat(data[0][0]);
	x0 = parseFloat(data[0][0]) - dx;
	y0 = parseFloat(m * x0 + b);
	x1 = parseFloat(data[ii - 1][0]) + dx;
	y1 = parseFloat(m * x1 + b);

	var dReturn = {};
	dReturn.x0 = x0;
	dReturn.y0 = y0;
	dReturn.x1 = x1;
	dReturn.y1 = y1;
	return dReturn;
};

function SendX10Command(idx, switchcmd, refreshfunction, passcode) {
	ShowNotify($.t('Switching') + ' ' + $.t(switchcmd));
	$.ajax({
		url: "json.htm?type=command&param=switchlight&idx=" + $.devIdx +
		"&switchcmd=" + switchcmd +
		"&level=0" +
		"&passcode=" + passcode,
		async: false,
		dataType: 'json',
		success: function (data) {
			if (data.status == "ERROR") {
				HideNotify();
				bootbox.alert($.t(data.message));
			}
			//wait 1 second
			setTimeout(function () {
				HideNotify();
				refreshfunction();
			}, 1000);
		},
		error: function () {
			HideNotify();
			bootbox.alert($.t('Problem sending switch command'));
		}
	});
}

function ArmSystemInt(idx, switchcmd, refreshfunction, passcode) {
	clearInterval($.myglobals.refreshTimer);

	$.devIdx = idx;

	var $dialog = $('<div>How would you like to Arm the System?</div>').dialog({
		modal: true,
		width: 340,
		resizable: false,
		draggable: false,
		buttons: [
			{
				text: $.t("Arm Home"),
				click: function () {
					$dialog.remove();
					SendX10Command(idx, "Arm Home", refreshfunction, passcode)
				}
			},
			{
				text: $.t("Arm Away"),
				click: function () {
					$dialog.remove();
					SendX10Command(idx, "Arm Away", refreshfunction, passcode)
				}
			}
		]
	});
}

function ArmSystem(idx, switchcmd, refreshfunction, isprotected) {
	if (window.my_config.userrights == 0) {
		HideNotify();
		ShowNotify($.t('You do not have permission to do that!'), 2500, true);
		return;
	}

	var passcode = "";
	if (typeof isprotected != 'undefined') {
		if (isprotected == true) {
			bootbox.prompt($.t("Please enter Password") + ":", function (result) {
				if (result === null) {
					return;
				} else {
					if (result == "") {
						return;
					}
					passcode = result;
					ArmSystemInt(idx, switchcmd, refreshfunction, passcode);
				}
			});
		}
		else {
			ArmSystemInt(idx, switchcmd, refreshfunction, passcode);
		}
	}
	else {
		ArmSystemInt(idx, switchcmd, refreshfunction, passcode);
	}
}

function ArmSystemMeiantechInt(idx, switchcmd, refreshfunction, passcode) {
	clearInterval($.myglobals.refreshTimer);

	$.devIdx = idx;

	var $dialog = $('<div>How would you like to Arm the System?</div>').dialog({
		modal: true,
		width: 420,
		resizable: false,
		draggable: false,
		buttons: [
			{
				text: $.t("Arm Home"),
				click: function () {
					$dialog.remove();
					switchcmd = "Arm Home";
					SendX10Command(idx, "Arm Home", refreshfunction, passcode)
				}
			},
			{
				text: $.t("Arm Away"),
				click: function () {
					$dialog.remove();
					switchcmd = "Arm Away";
					SendX10Command(idx, "Arm Away", refreshfunction, passcode)
				}
			},
			{
				text: $.t("Panic"),
				click: function () {
					$dialog.remove();
					switchcmd = "Panic";
					SendX10Command(idx, "Panic", refreshfunction, passcode)
				}
			},
			{
				text: $.t("Disarm"),
				click: function () {
					$dialog.remove();
					switchcmd = "Disarm";
					SendX10Command(idx, "Disarm", refreshfunction, passcode)
				}
			}
		]
	});
}

function ArmSystemMeiantech(idx, switchcmd, refreshfunction, isprotected) {
	if (window.my_config.userrights == 0) {
		HideNotify();
		ShowNotify($.t('You do not have permission to do that!'), 2500, true);
		return;
	}

	var passcode = "";
	if (typeof isprotected != 'undefined') {
		if (isprotected == true) {
			bootbox.prompt($.t("Please enter Password") + ":", function (result) {
				if (result === null) {
					return;
				} else {
					if (result == "") {
						return;
					}
					passcode = result;
					ArmSystemMeiantechInt(idx, switchcmd, refreshfunction, passcode);
				}
			});
		}
		else {
			ArmSystemMeiantechInt(idx, switchcmd, refreshfunction, passcode);
		}
	}
	else {
		ArmSystemMeiantechInt(idx, switchcmd, refreshfunction, passcode);
	}
}

function ArmSystemX10Int(idx, switchcmd, refreshfunction, passcode) {
	clearInterval($.myglobals.refreshTimer);

	$.devIdx = idx;

	var $dialog = $('<div>How would you like to Arm the System?</div>').dialog({
		modal: true,
		width: 420,
		resizable: false,
		draggable: false,
		buttons: [
			{
				text: $.t("Normal"),
				click: function () {
					$dialog.remove();
					SendX10Command(idx, "Normal", refreshfunction, passcode);
				}
			},
			{
				text: $.t("Alarm"),
				click: function () {
					$dialog.remove();
					SendX10Command(idx, "Alarm", refreshfunction, passcode);
				}
			},
			{
				text: $.t("Arm Home"),
				click: function () {
					$dialog.remove();
					SendX10Command(idx, "Arm Home", refreshfunction, passcode);
				}
			},
			{
				text: $.t("Arm Home Delayed"),
				click: function () {
					$dialog.remove();
					SendX10Command(idx, "Arm Home Delayed", refreshfunction, passcode);
				}
			},
			{
				text: $.t("Arm Away"),
				click: function () {
					$dialog.remove();
					SendX10Command(idx, "Arm Away", refreshfunction, passcode);
				}
			},
			{
				text: $.t("Arm Away Delayed"),
				click: function () {
					$dialog.remove();
					SendX10Command(idx, "Arm Away Delayed", refreshfunction, passcode);
				}
			},
			{
				text: $.t("Panic"),
				click: function () {
					$dialog.remove();
					SendX10Command(idx, "Panic", refreshfunction, passcode);
				}
			},
			{
				text: $.t("Disarm"),
				click: function () {
					$dialog.remove();
					SendX10Command(idx, "Disarm", refreshfunction, passcode);
				}
			},
			{
				text: $.t("Light On"),
				click: function () {
					$dialog.remove();
					SendX10Command(idx, "Light On", refreshfunction, passcode);
				}
			},
			{
				text: $.t("Light Off"),
				click: function () {
					$dialog.remove();
					SendX10Command(idx, "Light Off", refreshfunction, passcode);
				}
			},
			{
				text: $.t("Light 2 On"),
				click: function () {
					$dialog.remove();
					SendX10Command(idx, "Light 2 On", refreshfunction, passcode);
				}
			},
			{
				text: $.t("Light 2 Off"),
				click: function () {
					$dialog.remove();
					SendX10Command(idx, "Light 2 Off", refreshfunction, passcode);
				}
			}
		]
	});
}

function ArmSystemX10(idx, switchcmd, refreshfunction, isprotected) {
	if (window.my_config.userrights == 0) {
		HideNotify();
		ShowNotify($.t('You do not have permission to do that!'), 2500, true);
		return;
	}

	var passcode = "";
	if (typeof isprotected != 'undefined') {
		if (isprotected == true) {
			bootbox.prompt($.t("Please enter Password") + ":", function (result) {
				if (result === null) {
					return;
				} else {
					if (result == "") {
						return;
					}
					passcode = result;
					ArmSystemX10Int(idx, switchcmd, refreshfunction, passcode);
				}
			});
		}
		else {
			ArmSystemX10Int(idx, switchcmd, refreshfunction, passcode);
		}
	}
	else {
		ArmSystemX10Int(idx, switchcmd, refreshfunction, passcode);
	}
}

function SwitchLightInt(idx, switchcmd, refreshfunction, passcode) {
	clearInterval($.myglobals.refreshTimer);

	ShowNotify($.t('Switching') + ' ' + $.t(switchcmd));

	$.ajax({
		url: "json.htm?type=command&param=switchlight" +
		"&idx=" + idx +
		"&switchcmd=" + switchcmd +
		"&level=0" +
		"&passcode=" + passcode,
		async: false,
		dataType: 'json',
		success: function (data) {
			if (data.status == "ERROR") {
				HideNotify();
				bootbox.alert($.t(data.message));
			}
			//wait 1 second
			setTimeout(function () {
				HideNotify();
				refreshfunction();
			}, 1000);
		},
		error: function () {
			HideNotify();
			bootbox.alert($.t('Problem sending switch command'));
		}
	});
}

function SwitchLight(idx, switchcmd, refreshfunction, isprotected) {
	if (window.my_config.userrights == 0) {
		HideNotify();
		ShowNotify($.t('You do not have permission to do that!'), 2500, true);
		return;
	}

	var passcode = "";
	if (typeof isprotected != 'undefined') {
		if (isprotected == true) {
			bootbox.prompt($.t("Please enter Password") + ":", function (result) {
				if (result === null) {
					return;
				} else {
					if (result == "") {
						return;
					}
					passcode = result;
					SwitchLightInt(idx, switchcmd, refreshfunction, passcode);
				}
			});
		}
		else {
			SwitchLightInt(idx, switchcmd, refreshfunction, passcode);
		}
	}
	else {
		SwitchLightInt(idx, switchcmd, refreshfunction, passcode);
	}
}
function SwitchSelectorLevelInt(idx, levelName, levelValue, refreshfunction, passcode) {
	clearInterval($.myglobals.refreshTimer);

	ShowNotify($.t('Switching') + ' ' + levelName);

	$.ajax({
		url: "json.htm?type=command&param=switchlight" +
		"&idx=" + idx +
		"&switchcmd=Set%20Level&level=" + levelValue +
		"&passcode=" + passcode,
		async: false,
		dataType: 'json',
		success: function (data) {
			if (data.status == "ERROR") {
				HideNotify();
				bootbox.alert($.t(data.message));
			}
			//wait 1 second
			setTimeout(function () {
				HideNotify();
				refreshfunction();
			}, 1000);
		},
		error: function () {
			HideNotify();
			bootbox.alert($.t('Problem sending switch command'));
		}
	});
}

function SwitchSelectorLevel(idx, levelName, levelValue, refreshfunction, isprotected) {
	if (window.my_config.userrights == 0) {
		HideNotify();
		ShowNotify($.t('You do not have permission to do that!'), 2500, true);
		return;
	}

	var passcode = "";
	if (typeof isprotected != 'undefined') {
		if (isprotected == true) {
			bootbox.prompt($.t("Please enter Password") + ":", function (result) {
				if (result === null) {
					return;
				} else {
					if (result == "") {
						return;
					}
					passcode = result;
					SwitchSelectorLevelInt(idx, levelName, levelValue, refreshfunction, passcode);
				}
			});
		}
		else {
			SwitchSelectorLevelInt(idx, levelName, levelValue, refreshfunction, passcode);
		}
	}
	else {
		SwitchSelectorLevelInt(idx, levelName, levelValue, refreshfunction, passcode);
	}
}

function SwitchSceneInt(idx, switchcmd, refreshfunction, passcode) {
	clearInterval($.myglobals.refreshTimer);
	ShowNotify($.t('Switching') + ' ' + $.t(switchcmd));

	$.ajax({
		url: "json.htm?type=command&param=switchscene&idx=" + idx +
		"&switchcmd=" + switchcmd +
		"&passcode=" + passcode,
		async: false,
		dataType: 'json',
		success: function (data) {
			if (data.status == "ERROR") {
				HideNotify();
				bootbox.alert($.t(data.message));
			}
			//wait 1 second
			setTimeout(function () {
				HideNotify();
				refreshfunction();
			}, 1000);
		},
		error: function () {
			HideNotify();
			bootbox.alert($.t('Problem sending switch command'));
		}
	});
}

function SwitchScene(idx, switchcmd, refreshfunction, isprotected) {
	if (window.my_config.userrights == 0) {
		HideNotify();
		ShowNotify($.t('You do not have permission to do that!'), 2500, true);
		return;
	}
	var passcode = "";
	if (typeof isprotected != 'undefined') {
		if (isprotected == true) {
			bootbox.prompt($.t("Please enter Password") + ":", function (result) {
				if (result === null) {
					return;
				} else {
					if (result == "") {
						return;
					}
					passcode = result;
					SwitchSceneInt(idx, switchcmd, refreshfunction, passcode);
				}
			});
		}
		else {
			SwitchSceneInt(idx, switchcmd, refreshfunction, passcode);
		}
	}
	else {
		SwitchSceneInt(idx, switchcmd, refreshfunction, passcode);
	}
}

function ResetSecurityStatus(idx, switchcmd, refreshfunction) {
	if (window.my_config.userrights == 0) {
		HideNotify();
		ShowNotify($.t('You do not have permission to do that!'), 2500, true);
		return;
	}

	clearInterval($.myglobals.refreshTimer);
	ShowNotify($.t('Switching') + ' ' + $.t(switchcmd));

	$.ajax({
		url: "json.htm?type=command&param=resetsecuritystatus&idx=" + idx + "&switchcmd=" + switchcmd,
		async: false,
		dataType: 'json',
		success: function (data) {
			//wait 1 second
			setTimeout(function () {
				HideNotify();
				refreshfunction();
			}, 1000);
		},
		error: function () {
			HideNotify();
			bootbox.alert($.t('Problem sending switch command'));
		}
	});
}

function RefreshTextLogTable(idx) {
	var mTable = $($.content + ' #texttable');
	var oTable = mTable.dataTable();
	oTable.fnClearTable();

	$.ajax({
		url: "json.htm?type=textlog&idx=" + idx,
		async: false,
		dataType: 'json',
		success: function (data) {
			if (typeof data.result != 'undefined') {
				var datatable = [];
				var ii = 0;
				$.each(data.result, function (i, item) {
					var addId = oTable.fnAddData([
						item.Date,
						item.Data
					], false);
				});
				mTable.fnDraw();
			}
		}
	});
}

function ClearTextLog() {
	if (window.my_config.userrights != 2) {
		HideNotify();
		ShowNotify($.t('You do not have permission to do that!'), 2500, true);
		return;
	}
	bootbox.confirm($.t("Are you sure to delete the Log?\n\nThis action can not be undone!"), function (result) {
		if (result == true) {
			$.ajax({
				url: "json.htm?type=command&param=clearlightlog&idx=" + $.devIdx,
				async: false,
				dataType: 'json',
				success: function (data) {
					RefreshTextLogTable($.devIdx);
				},
				error: function () {
					HideNotify();
					ShowNotify($.t('Problem clearing the Log!'), 2500, true);
				}
			});
		}
	});
}

function ShowTextLog(content, backfunction, id, name) {
	clearInterval($.myglobals.refreshTimer);
	$(window).scrollTop(0);
	$.content = content;

	$.devIdx = id;

	$('#modal').show();
	var htmlcontent = '';
	htmlcontent = '<p><h2>' + $.t('Name') + ': ' + unescape(name) + '</h2></p>\n';
	htmlcontent += $('#textlog').html();
	$($.content).html(GetBackbuttonHTMLTable(backfunction) + htmlcontent);
	$($.content).i18n();

	var oTable = $($.content + ' #texttable').dataTable({
		"sDom": '<"H"lfrC>t<"F"ip>',
		"oTableTools": {
			"sRowSelect": "single",
		},
		"aaSorting": [[0, "desc"]],
		"bSortClasses": false,
		"bProcessing": true,
		"bStateSave": true,
		"bJQueryUI": true,
		"aLengthMenu": [[25, 50, 100, -1], [25, 50, 100, "All"]],
		"iDisplayLength": 25,
		"sPaginationType": "full_numbers",
		language: $.DataTableLanguage
	});

	RefreshTextLogTable($.devIdx);
	$('#modal').hide();
	return false;
}

function GetNotificationSettings() {
	var nsettings = {};
	var typetext = $($.content + " #notificationparamstable #combotype option:selected").text();

	nsettings.type = $($.content + " #notificationparamstable #combotype").val();
	var whenvisible = $($.content + " #notificationparamstable #notiwhen").is(":visible");
	if (whenvisible == false) {
		nsettings.when = 0;
		nsettings.value = 0;
	}
	else {
		nsettings.when = $($.content + " #notificationparamstable #combowhen").val();
		nsettings.value = $($.content + " #notificationparamstable #value").val();
		if ((nsettings.value == "") || (isNaN(nsettings.value))) {
			ShowNotify($.t('Please correct the Value!'), 2500, true);
			return null;
		}
	}

	nsettings.priority = $($.content + " #notificationparamstable #combopriority").val();
	nsettings.sendalways = $($.content + " #notificationparamstable #checkalways").is(":checked");
	nsettings.recovery = $($.content + " #notificationparamstable #checkrecovery").is(":checked");
	if ((nsettings.priority == "") || (isNaN(nsettings.priority))) {
		ShowNotify($.t('Please select a Priority level!'), 2500, true);
		return null;
	}
	nsettings.CustomMessage = encodeURIComponent($($.content + " #notificationparamstable #custommessage").val());

	nsettings.ActiveSystems = '';
	var totalDisabledSystems = 0;
	$($.content + ' #notificationparamstable #active_systems').children("input:checkbox:not(:checked)").map(function () {
		totalDisabledSystems++;
	});
	if (totalDisabledSystems != 0) {
		$($.content + ' #notificationparamstable #active_systems').children("input:checked").map(function () {
			if (nsettings.ActiveSystems.length != 0) {
				nsettings.ActiveSystems += ';';
			}
			nsettings.ActiveSystems += $(this).attr('id');
		});
	}
	if (nsettings.type == 7) // Switch On
	{
		if ($.isDimmer || $.isSelector) {
			nsettings.value = $($.content + " #notificationparamstable #combolevel").val();
			nsettings.when = 2; // '='
		}
	}
	return nsettings;
}

function ClearNotifications() {
	bootbox.confirm($.t("Are you sure to delete ALL notifications?\n\nThis action can not be undone!!"), function (result) {
		if (result == true) {
			$.ajax({
				url: "json.htm?type=command&param=clearnotifications&idx=" + $.devIdx,
				async: false,
				dataType: 'json',
				success: function (data) {
					RefreshNotificationTable($.devIdx);
				},
				error: function () {
					HideNotify();
					ShowNotify($.t('Problem clearing notifications!'), 2500, true);
				}
			});
		}
	});
}

function UpdateNotification(idx) {
	var nsettings = GetNotificationSettings();
	if (nsettings == null) {
		return;
	}
	$.ajax({
		url: "json.htm?type=command&param=updatenotification&idx=" + idx +
		"&devidx=" + $.devIdx +
		"&ttype=" + nsettings.type +
		"&twhen=" + nsettings.when +
		"&tvalue=" + nsettings.value +
		"&tmsg=" + nsettings.CustomMessage +
		"&tsystems=" + nsettings.ActiveSystems +
		"&tpriority=" + nsettings.priority +
		"&tsendalways=" + nsettings.sendalways +
		"&trecovery=" + nsettings.recovery,
		async: false,
		dataType: 'json',
		success: function (data) {
			RefreshNotificationTable($.devIdx);
		},
		error: function () {
			HideNotify();
			ShowNotify($.t('Problem updating notification!'), 2500, true);
		}
	});
}

function DeleteNotification(idx) {
	bootbox.confirm($.t("Are you sure to delete this notification?\n\nThis action can not be undone..."), function (result) {
		if (result == true) {
			$.ajax({
				url: "json.htm?type=command&param=deletenotification&idx=" + idx,
				async: false,
				dataType: 'json',
				success: function (data) {
					RefreshNotificationTable($.devIdx);
				},
				error: function () {
					HideNotify();
					ShowNotify($.t('Problem deleting notification!'), 2500, true);
				}
			});
		}
	});
}

function AddNotification() {
	var nsettings = GetNotificationSettings();
	if (nsettings == null) {
		bootbox.alert($.t("Invalid Notification Settings"));
		return;
	}

	$.ajax({
		url: "json.htm?type=command&param=addnotification&idx=" + $.devIdx +
		"&ttype=" + nsettings.type +
		"&twhen=" + nsettings.when +
		"&tvalue=" + nsettings.value +
		"&tmsg=" + nsettings.CustomMessage +
		"&tsystems=" + nsettings.ActiveSystems +
		"&tpriority=" + nsettings.priority +
		"&tsendalways=" + nsettings.sendalways +
		"&trecovery=" + nsettings.recovery,
		async: false,
		dataType: 'json',
		success: function (data) {
			if (data.status != "OK") {
				HideNotify();
				ShowNotify($.t('Problem adding notification!<br>Duplicate Value?'), 2500, true);
				return;
			}
			RefreshNotificationTable($.devIdx);
		},
		error: function () {
			HideNotify();
			ShowNotify($.t('Problem adding notification!'), 2500, true);
		}
	});
}

function RefreshNotificationTable(idx) {
	$('#modal').show();

	$($.content + ' #updelclr #notificationupdate').attr("class", "btnstyle3-dis");
	$($.content + ' #updelclr #notificationdelete').attr("class", "btnstyle3-dis");

	var oTable = $($.content + ' #notificationtable').dataTable();
	oTable.fnClearTable();

	$.ajax({
		url: "json.htm?type=notifications&idx=" + idx,
		async: false,
		dataType: 'json',
		success: function (data) {
			if (typeof data.notifiers != 'undefined') {
				//Make a list of our notification systems
				var systemsHTML = "";
				$.each(data.notifiers, function (i, item) {
					if (systemsHTML.length != 0) {
						systemsHTML += '<br>';
					}
					systemsHTML += '<input type="checkbox" id="' + item.description + '" checked />&nbsp;<label for="' + item.description + '">' + item.name +'</label>';
				});
				$($.content + ' #notificationparamstable #active_systems').html(systemsHTML);
			}
			if (typeof data.result != 'undefined') {
				$.each(data.result, function (i, item) {
					var parts = item.Params.split(';');
					var custommessage = item.CustomMessage.split(';;');
					var nvalue = 0;
					var rvalue = 0;
					if (parts.length > 1) {
						nvalue = parts[2];
					}
					if (parts.length > 2) {
						rvalue = parts[3];
					}
					if (custommessage.length > 0) {
						custommessage = custommessage[0];
					}
					else {
						custommessage = item.CustomMessage;
					}

					var whenstr = "";
					var ntype = "";
					var stype = "";
					if (parts[0] == "T") {
						ntype = $.t("Temperature");
						stype = " &deg; " + $.myglobals.tempsign;
					}
					else if (parts[0] == "D") {
						ntype = $.t("Dew Point");
						stype = " &deg; " + $.myglobals.tempsign;
					}
					else if (parts[0] == "H") {
						ntype = $.t("Humidity");
						stype = " %";
					}
					else if (parts[0] == "R") {
						ntype = $.t("Rain");
						stype = " mm";
					}
					else if (parts[0] == "W") {
						ntype = $.t("Wind");
						stype = " " + $.myglobals.windsign;
					}
					else if (parts[0] == "U") {
						ntype = $.t("UV");
						stype = " UVI";
					}
					else if (parts[0] == "M") {
						ntype = $.t("Usage");
					}
					else if (parts[0] == "B") {
						ntype = $.t("Baro");
						stype = " hPa";
					}
					else if (parts[0] == "S") {
						ntype = $.t("Switch On");
					}
					else if (parts[0] == "O") {
						ntype = $.t("Switch Off");
					}
					else if (parts[0] == "E") {
						ntype = $.t("Today");
						stype = " kWh";
					}
					else if (parts[0] == "G") {
						ntype = $.t("Today");
						stype = " m3";
					}
					else if (parts[0] == "C") {
						ntype = $.t("Today");
						stype = " cnt";
					}
					else if (parts[0] == "1") {
						ntype = $.t("Ampere 1");
						stype = " A";
					}
					else if (parts[0] == "2") {
						ntype = $.t("Ampere 2");
						stype = " A";
					}
					else if (parts[0] == "3") {
						ntype = $.t("Ampere 3");
						stype = " A";
					}
					else if (parts[0] == "P") {
						ntype = $.t("Percentage");
						stype = " %";
					}
					else if (parts[0] == "V") {
						ntype = $.t("Play Video");
						stype = "";
					}
					else if (parts[0] == "A") {
						ntype = $.t("Play Audio");
						stype = "";
					}
					else if (parts[0] == "X") {
						ntype = $.t("View Photo");
						stype = "";
					}
					else if (parts[0] == "Y") {
						ntype = $.t("Pause Stream");
						stype = "";
					}
					else if (parts[0] == "Q") {
						ntype = $.t("Stop Stream");
						stype = "";
					}
					else if (parts[0] == "a") {
						ntype = $.t("Play Stream");
						stype = "";
					}
					else if (parts[0] == "J") {
						ntype = $.t("Last Update");
						stype = " min";
					}
					var nwhen = "";
					if (ntype == $.t("Switch On")) {
						whenstr = $.t("On");
					}
					else if (ntype == $.t("Switch Off")) {
						whenstr = $.t("Off");
					}
					else if (ntype == $.t("Dew Point")) {
						whenstr = $.t("Dew Point");
					}
					else if (ntype == $.t("Play Video")) {
						whenstr = $.t("Video");
					}
					else if (ntype == $.t("Play Audio")) {
						whenstr = $.t("Audio");
					}
					else if (ntype == $.t("View Photo")) {
						whenstr = $.t("Photo");
					}
					else if (ntype == $.t("Pause Stream")) {
						whenstr = $.t("Paused");
					}
					else if (ntype == $.t("Stop Stream")) {
						whenstr = $.t("Stopped");
					}
					else if (ntype == $.t("Play Stream")) {
						whenstr = $.t("Playing");
					}
					else {
						if (parts[1] == ">") {
							nwhen = $.t("Greater") + " ";
						}
						else if (parts[1] == ">=") {
							nwhen = $.t("Greater or Equal") + " ";
						}
						else if (parts[1] == "=") {
							nwhen = $.t("Equal") + " ";
						}
						else if (parts[1] == "!=") {
							nwhen = $.t("Not Equal") + " ";
						}
						else if (parts[1] == "<=") {
							nwhen = $.t("Less or Equal") + " ";
						}
						else {
							nwhen = $.t("Less") + " ";
						}
						whenstr = nwhen + nvalue + stype;
					}
					if ((whenstr == $.t("On")) && ($.isDimmer || $.isSelector)) {
						if ($.isSelector) {
							whenstr += " (" + $.selectorSwitchLevels[nvalue / 10] + ")";
						} else {
							whenstr += " (" + nvalue + "%)";
						}
					}
					var priorityStr = "";
					if (item.Priority == -2) {
						priorityStr = $.t("Very Low");
					}
					else if (item.Priority == -1) {
						priorityStr = $.t("Moderate");
					}
					else if (item.Priority == 0) {
						priorityStr = $.t("Normal");
					}
					else if (item.Priority == 1) {
						priorityStr = $.t("High");
					}
					else if (item.Priority == 2) {
						priorityStr = $.t("Emergency");
					}

					var ActiveSystemsString = "All";
					if (item.ActiveSystems.length != 0) {
						ActiveSystemsString = item.ActiveSystems;
					}
					var addId = oTable.fnAddData({
						"DT_RowId": item.idx,
						"nvalue": nvalue,
						"ActiveSystems": item.ActiveSystems,
						"CustomMessage": custommessage,
						"Priority": item.Priority,
						"SendAlways": item.SendAlways,
						"SendRecovery": rvalue,
						"0": ntype,
						"1": whenstr,
						"2": ActiveSystemsString,
						"3": custommessage,
						"4": priorityStr,
						"5": $.t((item.SendAlways == true) ? "Yes" : "No"),
						"6": $.t((rvalue == true) ? "Yes" : "No")
					});
				});
			}
		}
	});
	/* Add a click handler to the rows - this could be used as a callback */
	$($.content + " #notificationtable tbody").off();
	$($.content + " #notificationtable tbody").on('click', 'tr', function () {
		if ($(this).hasClass('row_selected')) {
			$(this).removeClass('row_selected');
			$($.content + ' #updelclr #notificationupdate').attr("class", "btnstyle3-dis");
			$($.content + ' #updelclr #notificationdelete').attr("class", "btnstyle3-dis");
		}
		else {
			var oTable = $($.content + ' #notificationtable').dataTable();
			oTable.$('tr.row_selected').removeClass('row_selected');
			$(this).addClass('row_selected');
			$($.content + ' #updelclr #notificationupdate').attr("class", "btnstyle3");
			$($.content + ' #updelclr #notificationdelete').attr("class", "btnstyle3");
			var anSelected = fnGetSelected(oTable);
			if (anSelected.length !== 0) {
				var data = oTable.fnGetData(anSelected[0]);
				var idx = data["DT_RowId"];
				$.myglobals.SelectedNotificationIdx = idx;
				$($.content + " #updelclr #notificationupdate").attr("href", "javascript:UpdateNotification(" + idx + ")");
				$($.content + " #updelclr #notificationdelete").attr("href", "javascript:DeleteNotification(" + idx + ")");
				//update user interface with the paramters of this row
				var command = GetValTextInNTypeStrArray(data["0"]);
				$($.content + " #notificationparamstable #combotype").val(command);
				ShowNotificationTypeLabel();
				if (data["1"].match("^" + $.t("Greater"))) {
					$($.content + " #notificationparamstable #combowhen").val(0);
				}
				else if (data["1"].match("^" + $.t("Greater or Equal"))) {
					$($.content + " #notificationparamstable #combowhen").val(1);
				}
				else if (data["1"].match("^" + $.t("Equal"))) {
					$($.content + " #notificationparamstable #combowhen").val(2);
				}
				else if (data["1"].match("^" + $.t("Not Equal"))) {
					$($.content + " #notificationparamstable #combowhen").val(3);
				}
				else if (data["1"].match("^" + $.t("Less or Equal"))) {
					$($.content + " #notificationparamstable #combowhen").val(4);
				}
				else {
					$($.content + " #notificationparamstable #combowhen").val(5);
				}
				$($.content + " #notificationparamstable #value").val(data["nvalue"]);
				$($.content + " #notificationparamstable #combopriority").val(data["Priority"]);
				$($.content + " #notificationparamstable #checkalways").prop('checked', (data["SendAlways"] == true));
				$($.content + " #notificationparamstable #checkrecovery").prop('checked', (data["SendRecovery"] == true));
				$($.content + " #notificationparamstable #custommessage").val(data["CustomMessage"]);

				var ActiveSystems = data["ActiveSystems"];
				if (ActiveSystems.length == 0) {
					//check all
					$($.content + ' #notificationparamstable #active_systems').children("input:checked").map(function () {
						$(this).prop('checked', true);
					});
				}
				else {
					$($.content + ' #notificationparamstable #active_systems').children("input:checked").map(function () {
						$(this).prop('checked', false);
					});
					var systems = ActiveSystems.split(";");
					$.each(systems, function (i, item) {
						$($.content + ' #notificationparamstable #active_systems #' + item).prop('checked', true);
					});
				}
				var level = data["nvalue"];
				if ($.isDimmer || $.isSelector) {
					$($.content + " #notificationparamstable #notilevel").hide();
					$($.content + " #notificationparamstable #combolevel").val(level);
					if (command == 7) { // Switch On
						$("#notificationparamstable #notilevel").show();
					}
				}
			}
		}
	});

	$('#modal').hide();
}

function ShowNotificationTypeLabel() {
	var typetext = $($.content + " #notificationparamstable #combotype option:selected").text();

	if (
		(typetext == $.t("Switch On")) ||
		(typetext == $.t("Switch Off")) ||
		(typetext == $.t("Dew Point")) ||
		(typetext == $.t("Play Video")) ||
		(typetext == $.t("Play Audio")) ||
		(typetext == $.t("View Photo")) ||
		(typetext == $.t("Pause Stream")) ||
		(typetext == $.t("Stop Stream")) ||
		(typetext == $.t("Play Stream"))
	) {
		$($.content + " #notificationparamstable #notiwhen").hide();
		$($.content + " #notificationparamstable #notival").hide();
		$($.content + " #notificationparamstable #notirecovery").hide();
		return;
	}
	else {
		$($.content + " #notificationparamstable #notiwhen").show();
		$($.content + " #notificationparamstable #notival").show();
		$($.content + " #notificationparamstable #notirecovery").show();
	}

	if (typetext == $.t('Temperature'))
		$($.content + " #notificationparamstable #valuetype").html('&nbsp;&deg; ' + $.myglobals.tempsign);
	else if (typetext == $.t('Dew Point'))
		$($.content + " #notificationparamstable #valuetype").html('&nbsp;&deg; ' + $.myglobals.tempsign);
	else if (typetext == $.t('Humidity'))
		$($.content + " #notificationparamstable #valuetype").html('&nbsp;%');
	else if (typetext == $.t('UV'))
		$($.content + " #notificationparamstable #valuetype").html('&nbsp;UVI');
	else if (typetext == $.t('Rain'))
		$($.content + " #notificationparamstable #valuetype").html('&nbsp;mm');
	else if (typetext == $.t('Wind'))
		$($.content + " #notificationparamstable #valuetype").html('&nbsp;' + $.myglobals.windsign);
	else if (typetext == $.t('Baro'))
		$($.content + " #notificationparamstable #valuetype").html('&nbsp;hPa');
	else if (typetext == $.t('Usage'))
		$($.content + " #notificationparamstable #valuetype").html('&nbsp;');
	else if (typetext == $.t('Today'))
		$($.content + " #notificationparamstable #valuetype").html('&nbsp;');
	else if (typetext == $.t('Total'))
		$($.content + " #notificationparamstable #valuetype").html('&nbsp;');
	else if (typetext == $.t('Ampere 1'))
		$($.content + " #notificationparamstable #valuetype").html('&nbsp;A');
	else if (typetext == $.t('Ampere 2'))
		$($.content + " #notificationparamstable #valuetype").html('&nbsp;A');
	else if (typetext == $.t('Ampere 3'))
		$($.content + " #notificationparamstable #valuetype").html('&nbsp;A');
	else if (typetext == $.t('Percentage'))
		$($.content + " #notificationparamstable #valuetype").html('&nbsp;%');
	else if (typetext == $.t('RPM'))
		$($.content + " #notificationparamstable #valuetype").html('&nbsp;RPM');
	else if (typetext == $.t('Last Update'))
		$($.content + " #notificationparamstable #valuetype").html('&nbsp;minutes');
	else
		$($.content + " #notificationparamstable #valuetype").html('&nbsp;??');
}

function GetValTextInNTypeStrArray(stext) {
	var pos = -1;
	$.each($.NTypeStr, function (i, item) {
		if ($.t(item.text) == stext) {
			pos = item.val;
		}
	});

	return pos;
}

function ShowNotifications(id, name, content, backfunction, isdimmer, stype, devsubtype) {
	clearInterval($.myglobals.refreshTimer);
	$.devIdx = id;
	$.content = content;
	//$.isDimmer=isdimmer;
	$.isDimmer = false; // Skip Dimmers for now; not implemented yet
	$.isSelector = (devsubtype === "Selector Switch");

	if ($.isSelector) {
		// backup selector switch level names before displaying edit edit form
		var selectorSwitch$ = $("#selector" + $.devIdx);
		$.selectorSwitchLevels = unescape(selectorSwitch$.data("levelnames")).split('|');
		$.selectorSwitchLevelOffHidden = selectorSwitch$.data("leveloffhidden");
	}

	$.NTypeStr = [];

	$.ajax({
		url: "json.htm?type=command&param=getnotificationtypes&idx=" + $.devIdx,
		async: false,
		dataType: 'json',
		success: function (data) {

			if (typeof data.result != 'undefined') {
				$.each(data.result, function (i, item) {
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
			htmlcontent = '<p><h2>' + $.t('Name') + ': ' + unescape(name) + '</h2></p>\n';
			htmlcontent += $('#editnotifications').html();
			$($.content).html(GetBackbuttonHTMLTable(backfunction) + htmlcontent);
			$($.content).i18n();

			var systemsHTML = '<input type="checkbox" id="checkalways"/>&nbsp;<label for="checkalways">(<span>' + $.t('If enabled, it will bypass the Notification interval specified in the Settings page') + '</span>)</label>';
			$($.content + ' #notificationparamstable #id_checkalways').html(systemsHTML);
			
			systemsHTML = '<input type="checkbox" id="checkrecovery"/>&nbsp;<label for="checkrecovery">(<span>' + $.t('If enabled, a notification will be send when given condition has been recovered') + '</span>)</label>';
			$($.content + ' #notificationparamstable #id_checkrecovery').html(systemsHTML);
			
			//add types to combobox
			$.each($.NTypeStr, function (i, item) {
				var option = $('<option />');
				option.attr('value', item.val).text($.t(item.text));
				$($.content + ' #notificationparamstable #combotype').append(option);
			});

			oTable = $($.content + ' #notificationtable').dataTable({
				"sDom": '<"H"lfrC>t<"F"ip>',
				"oTableTools": {
					"sRowSelect": "single",
				},
				"aaSorting": [[0, "desc"]],
				"bSortClasses": false,
				"bProcessing": true,
				"bStateSave": true,
				"bJQueryUI": true,
				"aLengthMenu": [[25, 50, 100, -1], [25, 50, 100, "All"]],
				"iDisplayLength": 25,
				"sPaginationType": "full_numbers",
				language: $.DataTableLanguage
			});
			$($.content + " #notificationparamstable #combotype").change(function () {
				ShowNotificationTypeLabel();
			});

			ShowNotificationTypeLabel();
			$('#modal').hide();
			RefreshNotificationTable(id);
		},
		error: function () {
			HideNotify();
			ShowNotify($.t('Problem clearing notifications!'), 2500, true);
		}
	});

	$($.content + " #notificationparamstable #combotype").change(function () {
		var cval = $($.content + " #notificationparamstable #combotype").val(),
			lval = $($.content + " #notificationparamstable #combolevel").val(),
			bShowLevel = false;
		if (!$.bIsLED) {
			if ($.isDimmer || $.isSelector) {
				if (cval == 7) { // Switch On
					bShowLevel = true;
				}
			}
		}
		if (bShowLevel == true) {
			if (lval == null) {
				if ($.isSelector) {
					$($.content + " #notificationparamstable #combolevel").val(10); // first selector level value
				} else {
					$($.content + " #notificationparamstable #combolevel").val(5); // first dimmer level value
				}
			}
			$($.content + " #notificationparamstable #notilevel").show();
		}
		else {
			$($.content + " #notificationparamstable #notilevel").hide();
		}
	});

	if (($.isDimmer) && (!$.bIsLED)) {
		$($.content + " #notificationparamstable #notilevel").show();
	} else if ($.isSelector) {
		// Replace dimmer levels by selector level names
		var levelDiv$ = $($.content + " #notificationparamstable #notilevel"),
			html = [];
		$.each($.selectorSwitchLevels, function (index, item) {
			var level = index * 10,
				levelName = item;
			if (level === 0) {
				return;
			}
			html.push('<option value="');
			html.push(level);
			html.push('">');
			html.push(levelName);
			html.push('</option>');
		});
		levelDiv$.find('select')
			.attr('style', '')
			.css({ 'width': 'auto' })
			.html(html.join(''));
		levelDiv$.find('span[data-i18n="Level"]')
			.attr('data-i18n', "Level name")
			.html($.t("Level name"));
		levelDiv$.show();
	}
	else {
		$($.content + " #notificationparamstable #notilevel").hide();
	}
}

function ShowForecast(forecast_url, name, content, backfunction) {
	clearInterval($.myglobals.refreshTimer);
	$.content = content;
	var htmlcontent = '';
	htmlcontent = '<iframe class="cIFrame" id="IMain" src="' + forecast_url + '"></iframe>';
	$($.content).html(GetBackbuttonHTMLTable(backfunction) + htmlcontent);
	$($.content).i18n();
}

function GetUTCFromString(s) {
	return Date.UTC(
		parseInt(s.substring(0, 4), 10),
		parseInt(s.substring(5, 7), 10) - 1,
		parseInt(s.substring(8, 10), 10),
		parseInt(s.substring(11, 13), 10),
		parseInt(s.substring(14, 16), 10),
		0
	);
}

function GetUTCFromStringSec(s) {
	return Date.UTC(
		parseInt(s.substring(0, 4), 10),
		parseInt(s.substring(5, 7), 10) - 1,
		parseInt(s.substring(8, 10), 10),
		parseInt(s.substring(11, 13), 10),
		parseInt(s.substring(14, 16), 10),
		parseInt(s.substring(17, 19), 10)
	);
}

function GetDateFromString(s) {
	return Date.UTC(
		parseInt(s.substring(0, 4), 10),
		parseInt(s.substring(5, 7), 10) - 1,
		parseInt(s.substring(8, 10), 10));
}

function GetPrevDateFromString(s) {
	return Date.UTC(
		parseInt(s.substring(0, 4), 10) + 1,
		parseInt(s.substring(5, 7), 10) - 1,
		parseInt(s.substring(8, 10), 10));
}

function cursorhand() {
	document.body.style.cursor = "pointer";
}

function cursordefault() {
	document.body.style.cursor = "default";
}

function ShowNotify(txt, timeout, iserror) {
	$("#notification").html('<p>' + txt + '</p>');

	if (typeof iserror != 'undefined') {
		$("#notification").css("background-color", "red");
	} else {
		$("#notification").css("background-color", "#204060");
	}
	$("#notification").center();
	$("#notification").fadeIn("slow");

	if (typeof timeout != 'undefined') {
		setTimeout(function () {
			HideNotify();
		}, timeout);
	}
}

function HideNotify() {
	$("#notification").hide();
}

function ChangeClass(elemname, newclass) {
	document.getElementById(elemname).setAttribute("class", newclass);
}

function GetLayoutFromURL() {
	var page = window.location.hash.substr(1);
	return page != "" ? page : 'Dashboard';
}

function SetLayoutURL(name) {
	window.location.hash = name;
}

function SwitchLayout(layout) {
	if (layout.indexOf('templates') == 0) {
		clearInterval($.myglobals.refreshTimer);
		var dest_layout = layout.substring(10);
		dest_layout = dest_layout.replace(/ /g, "%20");
		window.location = '#/Custom/' + dest_layout;
		return;
	}
	if (layout == "Restart") {
		bootbox.confirm($.t("Are you sure to Restart the system?"), function (result) {
			if (result == true) {
				$.ajax({
					url: "json.htm?type=command&param=system_reboot",
					async: true,
					dataType: 'json',
					success: function (data) {
					},
					error: function () {
					}
				});
				bootbox.alert($.t("Restarting System (This could take some time...)"));
			}
		});
		return;
	}
	else if (layout == "Shutdown") {
		bootbox.confirm($.t("Are you sure to Shutdown the system?"), function (result) {
			if (result == true) {
				$.ajax({
					url: "json.htm?type=command&param=system_shutdown",
					async: true,
					dataType: 'json',
					success: function (data) {
					},
					error: function () {
					}
				});
				bootbox.alert($.t("The system is being Shutdown (This could take some time...)"));
			}
		});
		return;
	}
	var fullLayout = layout;
	var hyphen = layout.indexOf('-');
	if (hyphen >= 0) {
		layout = layout.substr(0, hyphen);
	}

	clearInterval($.myglobals.refreshTimer);
	$.myglobals.prevlayout = $.myglobals.layout;
	$.myglobals.actlayout = layout;
	$.myglobals.layoutFull = fullLayout;
	$.myglobals.layoutParameters = fullLayout.substr(hyphen + 1);

	if (window.my_config.userrights != 2) {
		if ((layout == 'Setup') || (layout == 'Users') || (layout == 'Cam') || (layout == 'Events') || (layout == 'Hardware') || (layout == 'Devices') || (layout == 'Restoredatabase')) {
			layout = 'Dashboard';
		}
	}

	if ((layout == "Dashboard") && ($.myglobals.DashboardType == 3)) {
		layout = 'Floorplans';
	}

	window.location = '#' + layout;
}

function checkLength(o, min, max) {
	if (o.val().length > max || o.val().length < min) {
		return false;
	} else {
		return true;
	}
}

function SetDimValue(idx, value) {
	clearInterval($.setDimValue);

	if (window.my_config.userrights == 0) {
		HideNotify();
		ShowNotify($.t('You do not have permission to do that!'), 2500, true);
		return;
	}

	$.ajax({
		url: "json.htm?type=command&param=switchlight&idx=" + idx + "&switchcmd=Set%20Level&level=" + value,
		async: false,
		dataType: 'json'
	});
}

//Some helper for browser detection
function matchua(ua) {
	ua = ua.toLowerCase();

	var match = /(chrome)[ \/]([\w.]+)/.exec(ua) ||
		/(webkit)[ \/]([\w.]+)/.exec(ua) ||
		/(opera)(?:.*version|)[ \/]([\w.]+)/.exec(ua) ||
		/(msie) ([\w.]+)/.exec(ua) ||
		ua.indexOf("compatible") < 0 && /(mozilla)(?:.*? rv:([\w.]+)|)/.exec(ua) ||
		[];

	return {
		browser: match[1] || "",
		version: match[2] || "0"
	};
}

function Get5MinuteHistoryDaysGraphTitle() {
	if ($.FiveMinuteHistoryDays == 1) {
		return $.t("Last") + " 24 " + $.t("Hours");
	}
	else if ($.FiveMinuteHistoryDays == 2) {
		return $.t("Last") + " 48 " + $.t("Hours");
	}
	return $.t("Last") + " " + $.FiveMinuteHistoryDays + " " + $.t("Days");
}

function GenerateCamImageURL(address, port, username, password, imageurl, protocol) {
	var feedsrc;
	if (protocol==0)
		feedsrc = "http://";
	else
		feedsrc = "https://";
	var bHaveUPinURL = (imageurl.indexOf("#USERNAME") != -1) || (imageurl.indexOf("#PASSWORD") != -1);
	if (!bHaveUPinURL) {
		if (username != "") {
			feedsrc += username + ":" + password + "@";
		}
	}
	feedsrc += address;
	if (port != 80) {
		feedsrc += ":" + port;
	}
	feedsrc += "/" + imageurl;
	if (bHaveUPinURL) {
		feedsrc = feedsrc.replace("#USERNAME", username);
		feedsrc = feedsrc.replace("#PASSWORD", password);
	}
	return feedsrc;
}

function GetTemp48Item(temp) {
	if ($.myglobals.tempsign == "C") {
		if (temp <= 0) {
			return "ice.png";
		}
		if (temp < 5) {
			return "temp-0-5.png";
		}
		if (temp < 10) {
			return "temp-5-10.png";
		}
		if (temp < 15) {
			return "temp-10-15.png";
		}
		if (temp < 20) {
			return "temp-15-20.png";
		}
		if (temp < 25) {
			return "temp-20-25.png";
		}
		if (temp < 30) {
			return "temp-25-30.png";
		}
		return "temp-gt-30.png";
	}
	else {
		if (temp <= 32) {
			return "ice.png";
		}
		if (temp < 41) {
			return "temp-0-5.png";
		}
		if (temp < 50) {
			return "temp-5-10.png";
		}
		if (temp < 59) {
			return "temp-10-15.png";
		}
		if (temp < 68) {
			return "temp-15-20.png";
		}
		if (temp < 77) {
			return "temp-20-25.png";
		}
		if (temp < 86) {
			return "temp-25-30.png";
		}
		return "temp-gt-30.png";
	}
}

function generate_noty(ntype, ntext, ntimeout) {
	return noty({
		text: ntext,
		type: ntype,
		dismissQueue: true,
		timeout: ntimeout,
		layout: 'topRight',
		theme: 'defaultTheme'
	});
}

function generate_noty_tl(ntype, ntext, ntimeout) {
	return noty({
		text: ntext,
		type: ntype,
		dismissQueue: true,
		timeout: ntimeout,
		layout: 'topLeft',
		theme: 'defaultTheme'
	});
}

function rgb2hex(rgb) {
	if (typeof rgb == 'undefined')
		return rgb;
	if (rgb.search("rgb") == -1) {
		return rgb.toUpperCase();
	} else {
        rgb = rgb.match(/^rgba?\((\d+),\s*(\d+),\s*(\d+)(?:,\s*(\d+(\.\d+)?))?\)$/);
		function hex(x) {
			return ("0" + parseInt(x).toString(16)).slice(-2).toUpperCase();
		}
		return "#" + hex(rgb[1]) + hex(rgb[2]) + hex(rgb[3]);
	}
}

function chartPointClick(event, retChart) {
	if (event.shiftKey != true) {
		return;
	}
	if (window.my_config.userrights != 2) {
		HideNotify();
		ShowNotify($.t('You do not have permission to do that!'), 2500, true);
		return;
	}
	var dateString = Highcharts.dateFormat('%Y-%m-%d', event.point.x);

	bootbox.confirm($.t("Are you sure to remove this value at") + " ?:\n\n" + $.t("Date") + ": " + dateString + " \n" + $.t("Value") + ": " + event.point.y, function (result) {
		if (result == true) {
			$.ajax({
				url: "json.htm?type=command&param=deletedatapoint&idx=" + $.devIdx + "&date=" + dateString,
				async: false,
				dataType: 'json',
				success: function (data) {
					if (data.status == "OK") {
						retChart($.devIdx, $.devName);
					}
					else {
						ShowNotify($.t('Problem deleting data point!'), 2500, true);
					}
				},
				error: function () {
					ShowNotify($.t('Problem deleting data point!'), 2500, true);
				}
			});
		}
	});
}

// TODO: use domoticzDataPointApi.deletePoint in your angular controllers instead
function chartPointClickNew(event, isShort, retChart) {
	if (event.shiftKey != true) {
		return;
	}
	if (window.my_config.userrights != 2) {
		HideNotify();
		ShowNotify($.t('You do not have permission to do that!'), 2500, true);
		return;
	}
	var dateString;
	if (isShort == false) {
		dateString = Highcharts.dateFormat('%Y-%m-%d', event.point.x);
	}
	else {
		dateString = Highcharts.dateFormat('%Y-%m-%d %H:%M:%S', event.point.x);
	}

	bootbox.confirm($.t("Are you sure to remove this value at") + " ?:\n\n" + $.t("Date") + ": " + dateString + " \n" + $.t("Value") + ": " + event.point.y, function (result) {
		if (result == true) {
			$.ajax({
				url: "json.htm?type=command&param=deletedatapoint&idx=" + $.devIdx + "&date=" + dateString,
				async: false,
				dataType: 'json',
				success: function (data) {
					if (data.status == "OK") {
						retChart($.content, $.backfunction, $.devIdx, $.devName, $.devType);
					}
					else {
						ShowNotify($.t('Problem deleting data point!'), 2500, true);
					}
				},
				error: function () {
					ShowNotify($.t('Problem deleting data point!'), 2500, true);
				}
			});
		}
	});
}

function chartPointClickNewGeneral(event, isShort, retChart) {
	if (event.shiftKey != true) {
		return;
	}
	if (window.my_config.userrights != 2) {
		HideNotify();
		ShowNotify($.t('You do not have permission to do that!'), 2500, true);
		return;
	}
	var dateString;
	if (isShort == false) {
		dateString = Highcharts.dateFormat('%Y-%m-%d', event.point.x);
	}
	else {
		dateString = Highcharts.dateFormat('%Y-%m-%d %H:%M:%S', event.point.x);
	}

	bootbox.confirm($.t("Are you sure to remove this value at") + " ?:\n\n" + $.t("Date") + ": " + dateString + " \n" + $.t("Value") + ": " + event.point.y, function (result) {
		if (result == true) {
			$.ajax({
				url: "json.htm?type=command&param=deletedatapoint&idx=" + $.devIdx + "&date=" + dateString,
				async: false,
				dataType: 'json',
				success: function (data) {
					if (data.status == "OK") {
						retChart($.content, $.backfunction, $.devIdx, $.devName, $.switchtype, $.sensortype);
					}
					else {
						ShowNotify($.t('Problem deleting data point!'), 2500, true);
					}
				},
				error: function () {
					ShowNotify($.t('Problem deleting data point!'), 2500, true);
				}
			});
		}
	});
}

function chartPointClickEx(event, retChart) {
	if (event.shiftKey != true) {
		return;
	}
	if (window.my_config.userrights != 2) {
		HideNotify();
		ShowNotify($.t('You do not have permission to do that!'), 2500, true);
		return;
	}
	var dateString = Highcharts.dateFormat('%Y-%m-%d', event.point.x);
	var bValid = false;

	bootbox.confirm($.t("Are you sure to remove this value at") + " ?:\n\n" + $.t("Date") + ": " + dateString + " \n" + $.t("Value") + ": " + event.point.y, function (result) {
		if (result == true) {
			$.ajax({
				url: "json.htm?type=command&param=deletedatapoint&idx=" + $.devIdx + "&date=" + dateString,
				async: false,
				dataType: 'json',
				success: function (data) {
					if (data.status == "OK") {
						retChart($.devIdx, $.devName, $.devSwitchType);
					}
					else {
						ShowNotify($.t('Problem deleting data point!'), 2500, true);
					}
				},
				error: function () {
					ShowNotify($.t('Problem deleting data point!'), 2500, true);
				}
			});
		}
	});
}

function chartPointClickNewEx(event, isShort, retChart) {
	if (event.shiftKey != true) {
		return;
	}
	if (window.my_config.userrights != 2) {
		HideNotify();
		ShowNotify($.t('You do not have permission to do that!'), 2500, true);
		return;
	}
	var dateString;
	if (isShort == false) {
		dateString = Highcharts.dateFormat('%Y-%m-%d', event.point.x);
	}
	else {
		dateString = Highcharts.dateFormat('%Y-%m-%d %H:%M:%S', event.point.x);
	}

	bootbox.confirm($.t("Are you sure to remove this value at") + " ?:\n\n" + $.t("Date") + ": " + dateString + " \n" + $.t("Value") + ": " + event.point.y, function (result) {
		if (result == true) {
			$.ajax({
				url: "json.htm?type=command&param=deletedatapoint&idx=" + $.devIdx + "&date=" + dateString,
				async: false,
				dataType: 'json',
				success: function (data) {
					if (data.status == "OK") {
						retChart($.content, $.backfunction, $.devIdx, $.devName, $.devSwitchType);
					}
					else {
						ShowNotify($.t('Problem deleting data point!'), 2500, true);
					}
				},
				error: function () {
					ShowNotify($.t('Problem deleting data point!'), 2500, true);
				}
			});
		}
	});
}

function ExportChart2CSV(chart) {
	var csv = "";
	for (var i = 0; i < chart.series.length; i++) {
		var series = chart.series[i];
		for (var j = 0; j < series.data.length; j++) {
			if (series.data[j] != undefined && series.data[j].x >= series.xAxis.min && series.data[j].x <= series.xAxis.max) {
				csv = csv + series.name + ',' + Highcharts.dateFormat('%Y-%m-%d %H:%M:%S', series.data[j].x) + ',' + series.data[j].y + '\r\n';
			}
		}
	}

	var w = window.open('', 'csvWindow'); // popup, may be blocked though
	// the following line does not actually do anything interesting with the
	// parameter given in current browsers, but really should have.
	// Maybe in some browser it will. It does not hurt anyway to give the mime type
	w.document.open("text/csv");
	w.document.write(csv); // the csv string from for example a jquery plugin
	w.document.close();
}

function SetLanguage(lng) {
	$.i18n.init({
		resGetPath: 'i18n/domoticz-__lng__.json',
		fallbackLng: false,
		getAsync: false,
		debug: false,
		useCookie: false,
		nsseparator: 'aadd',
		keyseparator: 'bbcc',
		lng: lng
	});
	$(".nav").i18n();
	MakeDatatableTranslations();
}

function TranslateStatus(status) {
	//should of course be changed, but for now a quick solution
	if (status.indexOf("Set Level") != -1) {
		return status.replace("Set Level", $.t('Set Level'));
	}
	else {
		return $.t(status);
	}
}

function TranslateStatusShort(status) {
	//will remove the Set Level
	if (status.indexOf("Set Level") != -1) {
		if (status.substring(11) == "100 %") {
			return "On";
		}
		else {
			return status.substring(11);
		}
	}
	else {
		return $.t(status);
	}
}

function ShowGeneralGraph(contentdiv, backfunction, id, name, switchtype, sensortype) {
	clearInterval($.myglobals.refreshTimer);
	$('#modal').show();
	$.content = contentdiv;
	$(window).scrollTop(0);
	$.backfunction = backfunction;
	$.devIdx = id;
	$.devName = name;
	$.switchtype = switchtype;
	$.sensortype = sensortype;
	var htmlcontent = '';
	htmlcontent = '<p><center><h2>' + unescape(name) + '</h2></center></p>\n';
	htmlcontent += $('#globaldaylog').html();

	var txtLabelOrg = sensortype;
	var txtUnit = "?";

	var graphtype = "counter";

	if (sensortype == "Custom Sensor") {
		txtUnit = unescape(switchtype);
		graphtype = "Percentage";
	}
	else if (sensortype == "Visibility") {
		txtUnit = "km";
		if (switchtype == 1) {
			txtUnit = "mi";
		}
	}
	else if (sensortype == "Radiation") {
		txtUnit = "Watt/m2";
	}
	else if (sensortype == "Pressure") {
		txtUnit = "Bar";
	}
	else if (sensortype == "Soil Moisture") {
		txtUnit = "cb";
	}
	else if (sensortype == "Leaf Wetness") {
		txtUnit = "Range";
	}
	else if ((sensortype == "Voltage") || (sensortype == "A/D")) {
		txtUnit = "mV";
	}
	else if (sensortype == "VoltageGeneral") {
		txtLabelOrg = "Voltage";
		txtUnit = "V";
	}
	else if ((sensortype == "DistanceGeneral") || (sensortype == "Distance")) {
		txtLabelOrg = "Distance";
		txtUnit = "cm";
		if (switchtype == 1) {
			txtUnit = "in";
		}
	}
	else if (sensortype == "Sound Level") {
		txtUnit = "dB";
	}
	else if ((sensortype == "CurrentGeneral") || (sensortype == "Current")) {
		txtLabelOrg = "Current";
		txtUnit = "A";
	}
	else if (switchtype == "Weight") {
		txtUnit = "kg";
	}
	else if (sensortype == "Waterflow") {
		txtUnit = "l/min";
		graphtype = "Percentage";
	}
	else {
		return;
	}
	$($.content).html(GetBackbuttonHTMLTable(backfunction) + htmlcontent);
	$($.content).i18n();

	var txtLabel = $.t(txtLabelOrg) + " (" + txtUnit + ")";
	var txtTopLabel = $.t(txtLabelOrg) + ' ';
	if (sensortype == "Custom Sensor") {
		txtLabel = txtUnit;
		txtTopLabel = "";
	}

	$.LogChart1 = $($.content + ' #globaldaygraph');
	$.LogChart1.highcharts({
		chart: {
			type: 'spline',
			zoomType: 'x',
			resetZoomButton: {
				position: {
					x: -30,
					y: -36
				}
			},
			marginRight: 10,
			events: {
				load: function () {
					$.getJSON("json.htm?type=graph&sensor=" + graphtype + "&idx=" + id + "&range=day",
						function (data) {
							if (typeof data.result != 'undefined') {
								var datatable = [];
								var minValue = 10000000;
								$.each(data.result, function (i, item) {
									datatable.push([GetUTCFromString(item.d), parseFloat(item.v)]);
									minValue = Math.min(item.v, minValue);
								});
								$.LogChart1.highcharts().yAxis[0].update({ min: minValue });
								var series = $.LogChart1.highcharts().series[0];
								series.setData(datatable);
							}
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
			text: Get5MinuteHistoryDaysGraphTitle()
		},
		xAxis: {
			type: 'datetime'
		},
		yAxis: {
			title: {
				text: txtLabel
			},
			labels: {
				formatter: function () {
					if (txtUnit == "mV") {
						return Highcharts.numberFormat(this.value, 0);
					}
					return this.value;
				}
			},
			min: 0,
			minorGridLineWidth: 0,
			alternateGridColor: null
		},
		tooltip: {
			formatter: function () {
				return '' +
					$.t(Highcharts.dateFormat('%A', this.x)) + '<br/>' + Highcharts.dateFormat('%Y-%m-%d %H:%M', this.x) + ': ' + this.y + ' ' + txtUnit;
			}
		},
		plotOptions: {
			series: {
				point: {
					events: {
						click: function (event) {
							chartPointClickNewGeneral(event, true, ShowGeneralGraph);
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
		},
		series: [{
			id: 'log1',
			showInLegend: false,
			name: txtTopLabel
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
			zoomType: 'x',
			resetZoomButton: {
				position: {
					x: -30,
					y: -36
				}
			},
			marginRight: 10,
			events: {
				load: function () {
					$.getJSON("json.htm?type=graph&sensor=" + graphtype + "&idx=" + id + "&range=month",
						function (data) {
							if (typeof data.result != 'undefined') {
								var datatable1 = [];
								var datatable2 = [];
								var datatable3 = [];
								var minValue = 10000000;
								$.each(data.result, function (i, item) {
									datatable1.push([GetDateFromString(item.d), parseFloat(item.v_min)]);
									datatable2.push([GetDateFromString(item.d), parseFloat(item.v_max)]);
									if (typeof item.v_avg != 'undefined') {
										var avg_val = parseFloat(item.v_avg);
										if (avg_val != 0) {
											datatable3.push([GetDateFromString(item.d), avg_val]);
										}
									}
									minValue = Math.min(item.v_min, minValue);
									minValue = Math.min(item.v_max, minValue);
								});
								$.LogChart2.highcharts().yAxis[0].update({ min: minValue });
								var series1 = $.LogChart2.highcharts().series[0];
								var series2 = $.LogChart2.highcharts().series[1];
								series1.setData(datatable1);
								series2.setData(datatable2);
								if (datatable3.length > 0) {
									var series3 = $.LogChart2.highcharts().series[2];
									series3.setData(datatable3);
								}
								else {
									$.LogChart2.highcharts().series[2].remove();
								}
							}
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
			text: txtTopLabel + $.t('Last Month')
		},
		xAxis: {
			type: 'datetime'
		},
		yAxis: {
			title: {
				text: txtLabel
			},
			labels: {
				formatter: function () {
					if (txtUnit == "mV") {
						return Highcharts.numberFormat(this.value, 0);
					}
					return this.value;
				}
			},
			min: 0,
			minorGridLineWidth: 0,
			alternateGridColor: null
		},
		tooltip: {
			crosshairs: true,
			shared: true
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
			id: 'logmin2',
			name: 'min',
			tooltip: {
				valueSuffix: ' ' + txtUnit
			},
			point: {
				events: {
					click: function (event) {
						chartPointClickNewGeneral(event, false, ShowGeneralGraph);
					}
				}
			}
		}, {
			id: 'logmax2',
			name: 'max',
			tooltip: {
				valueSuffix: ' ' + txtUnit
			},
			point: {
				events: {
					click: function (event) {
						chartPointClickNewGeneral(event, false, ShowGeneralGraph);
					}
				}
			}
		}, {
			id: 'logavg2',
			name: 'avg',
			tooltip: {
				valueSuffix: ' ' + txtUnit
			},
			point: {
				events: {
					click: function (event) {
						chartPointClickNewGeneral(event, false, ShowGeneralGraph);
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
			zoomType: 'x',
			resetZoomButton: {
				position: {
					x: -30,
					y: -36
				}
			},
			marginRight: 10,
			events: {
				load: function () {

					$.getJSON("json.htm?type=graph&sensor=" + graphtype + "&idx=" + id + "&range=year",
						function (data) {
							if (typeof data.result != 'undefined') {
								var datatable1 = [];
								var datatable2 = [];
								var datatable3 = [];
								var minValue = 10000000;

								$.each(data.result, function (i, item) {
									datatable1.push([GetDateFromString(item.d), parseFloat(item.v_min)]);
									datatable2.push([GetDateFromString(item.d), parseFloat(item.v_max)]);
									if (typeof item.v_avg != 'undefined') {
										var avg_val = parseFloat(item.v_avg);
										if (avg_val != 0) {
											datatable3.push([GetDateFromString(item.d), avg_val]);
										}
									}
									minValue = Math.min(item.v_min, minValue);
									minValue = Math.min(item.v_max, minValue);
								});
								$.LogChart3.highcharts().yAxis[0].update({ min: minValue });
								var series1 = $.LogChart3.highcharts().series[0];
								var series2 = $.LogChart3.highcharts().series[1];
								series1.setData(datatable1);
								series2.setData(datatable2);
								if (datatable3.length > 0) {
									var series3 = $.LogChart3.highcharts().series[2];
									series3.setData(datatable3);
								}
								else {
									$.LogChart3.highcharts().series[2].remove();
								}
							}
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
			text: txtTopLabel + $.t('Last Year')
		},
		xAxis: {
			type: 'datetime'
		},
		yAxis: {
			title: {
				text: txtLabel
			},
			labels: {
				formatter: function () {
					if (txtUnit == "mV") {
						return Highcharts.numberFormat(this.value, 0);
					}
					return this.value;
				}
			},
			min: 0,
			minorGridLineWidth: 0,
			alternateGridColor: null
		},
		tooltip: {
			formatter: function () {
				return '' +
					$.t(Highcharts.dateFormat('%A', this.x)) + '<br/>' + Highcharts.dateFormat('%Y-%m-%d %H:%M', this.x) + ': ' + this.y + ' ' + txtUnit;
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
			id: 'log3min',
			name: 'min',
			point: {
				events: {
					click: function (event) {
						chartPointClickNewGeneral(event, false, ShowGeneralGraph);
					}
				}
			}
		}, {
			id: 'log3max',
			name: 'max',
			point: {
				events: {
					click: function (event) {
						chartPointClickNewGeneral(event, false, ShowGeneralGraph);
					}
				}
			}
		}, {
			id: 'log3avg',
			name: 'avg',
			point: {
				events: {
					click: function (event) {
						chartPointClickNewGeneral(event, false, ShowGeneralGraph);
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

function AddDataToTempChart(data, chart, isday) {
	var datatablete = [];
	var datatabletm = [];
	var datatableta = [];
	var datatabletrange = [];

	var datatablehu = [];
	var datatablech = [];
	var datatablecm = [];
	var datatabledp = [];
	var datatableba = [];

	var datatablese = [];
	var datatablesm = [];
	var datatablesx = [];
	var datatablesrange = [];

	var datatablete_prev = [];
	var datatabletm_prev = [];
	var datatableta_prev = [];
	var datatabletrange_prev = [];

	var datatablehu_prev = [];
	var datatablech_prev = [];
	var datatablecm_prev = [];
	var datatabledp_prev = [];
	var datatableba_prev = [];

	var datatablese_prev = [];
	var datatablesm_prev = [];
	var datatablesx_prev = [];
	var datatablesrange_prev = [];

	var bHavePrev = (typeof data.resultprev != 'undefined');
	if (bHavePrev) {
		$.each(data.resultprev, function (i, item) {
			if (typeof item.te != 'undefined') {
				datatablete_prev.push([GetPrevDateFromString(item.d), parseFloat(item.te)]);
				datatabletm_prev.push([GetPrevDateFromString(item.d), parseFloat(item.tm)]);
				datatabletrange_prev.push([GetPrevDateFromString(item.d), parseFloat(item.tm), parseFloat(item.te)]);
				if (typeof item.ta != 'undefined') {
					datatableta_prev.push([GetPrevDateFromString(item.d), parseFloat(item.ta)]);
				}
			}
			if (typeof item.hu != 'undefined') {
				datatablehu_prev.push([GetPrevDateFromString(item.d), parseFloat(item.hu)]);
			}
			if (typeof item.ch != 'undefined') {
				datatablech_prev.push([GetPrevDateFromString(item.d), parseFloat(item.ch)]);
				datatablecm_prev.push([GetPrevDateFromString(item.d), parseFloat(item.cm)]);
			}
			if (typeof item.dp != 'undefined') {
				datatabledp_prev.push([GetPrevDateFromString(item.d), parseFloat(item.dp)]);
			}
			if (typeof item.ba != 'undefined') {
				datatableba_prev.push([GetPrevDateFromString(item.d), parseFloat(item.ba)]);
			}
			if (typeof item.se != 'undefined') {
				datatablese_prev.push([GetPrevDateFromString(item.d), parseFloat(item.se)]);
			}
			if (typeof item.sm != 'undefined' && typeof item.sx != 'undefined') {
				datatablesm_prev.push([GetPrevDateFromString(item.d), parseFloat(item.sm)]);
				datatablesx_prev.push([GetPrevDateFromString(item.d), parseFloat(item.sx)]);
				datatablesrange_prev.push([GetPrevDateFromString(item.d), parseFloat(item.sm), parseFloat(item.sx)]);
			}
		});
	}

	$.each(data.result, function (i, item) {
		if (isday == 1) {
			if (typeof item.te != 'undefined') {
				datatablete.push([GetUTCFromString(item.d), parseFloat(item.te)]);
			}
			if (typeof item.hu != 'undefined') {
				datatablehu.push([GetUTCFromString(item.d), parseFloat(item.hu)]);
			}
			if (typeof item.ch != 'undefined') {
				datatablech.push([GetUTCFromString(item.d), parseFloat(item.ch)]);
			}
			if (typeof item.dp != 'undefined') {
				datatabledp.push([GetUTCFromString(item.d), parseFloat(item.dp)]);
			}
			if (typeof item.ba != 'undefined') {
				datatableba.push([GetUTCFromString(item.d), parseFloat(item.ba)]);
			}
			if (typeof item.se != 'undefined') {
				datatablese.push([GetUTCFromString(item.d), parseFloat(item.se)]);
			}
		} else {
			if (typeof item.te != 'undefined') {
				datatablete.push([GetDateFromString(item.d), parseFloat(item.te)]);
				datatabletm.push([GetDateFromString(item.d), parseFloat(item.tm)]);
				datatabletrange.push([GetDateFromString(item.d), parseFloat(item.tm), parseFloat(item.te)]);
				if (typeof item.ta != 'undefined') {
					datatableta.push([GetDateFromString(item.d), parseFloat(item.ta)]);
				}
			}
			if (typeof item.hu != 'undefined') {
				datatablehu.push([GetDateFromString(item.d), parseFloat(item.hu)]);
			}
			if (typeof item.ch != 'undefined') {
				datatablech.push([GetDateFromString(item.d), parseFloat(item.ch)]);
				datatablecm.push([GetDateFromString(item.d), parseFloat(item.cm)]);
			}
			if (typeof item.dp != 'undefined') {
				datatabledp.push([GetDateFromString(item.d), parseFloat(item.dp)]);
			}
			if (typeof item.ba != 'undefined') {
				datatableba.push([GetDateFromString(item.d), parseFloat(item.ba)]);
			}
			if (typeof item.se != 'undefined') {
				datatablese.push([GetDateFromString(item.d), parseFloat(item.se)]);//avergae
				datatablesm.push([GetDateFromString(item.d), parseFloat(item.sm)]);//min
				datatablesx.push([GetDateFromString(item.d), parseFloat(item.sx)]);//max
				datatablesrange.push([GetDateFromString(item.d), parseFloat(item.sm), parseFloat(item.sx)]);
			}
		}
	});
	var series;
	if (datatablehu.length != 0) {
		chart.addSeries({
			id: 'humidity',
			name: $.t('Humidity'),
			color: 'limegreen',
			yAxis: 1,
			tooltip: {
				valueSuffix: ' %',
				valueDecimals: 0
			}
		}, false);
		series = chart.get('humidity');
		series.setData(datatablehu, false);
	}

	if (datatablech.length != 0) {
		chart.addSeries({
			id: 'chill',
			name: $.t('Chill'),
			color: 'red',
			zIndex: 1,
			tooltip: {
				valueSuffix: ' \u00B0' + $.myglobals.tempsign,
				valueDecimals: 1
			},
			yAxis: 0
		}, false);
		series = chart.get('chill');
		series.setData(datatablech, false);

		if (isday == 0) {
			chart.addSeries({
				id: 'chillmin',
				name: $.t('Chill') + '_min',
				color: 'rgba(255,127,39,0.8)',
				linkedTo: ':previous',
				zIndex: 1,
				tooltip: {
					valueSuffix: ' \u00B0' + $.myglobals.tempsign,
					valueDecimals: 1
				},
				yAxis: 0
			}, false);
			series = chart.get('chillmin');
			series.setData(datatablecm, false);
		}
	}

	if (datatablese.length != 0) {
		if (isday == 1) {
			chart.addSeries({
				id: 'setpoint',
				name: $.t('Set Point'),
				color: 'blue',
				zIndex: 1,
				tooltip: {
					valueSuffix: ' \u00B0' + $.myglobals.tempsign,
					valueDecimals: 1
				},
				yAxis: 0
			}, false);
			series = chart.get('setpoint');
			series.setData(datatablese, false);
		} else {
			chart.addSeries({
				id: 'setpointavg',
				name: $.t('Set Point') + '_avg',
				color: 'blue',
				fillOpacity: 0.7,
				zIndex: 2,
				tooltip: {
					valueSuffix: ' \u00B0' + $.myglobals.tempsign,
					valueDecimals: 1
				},
				yAxis: 0
			}, false);
			series = chart.get('setpointavg');
			series.setData(datatablese, false);
			/*
						chart.addSeries({
							id: 'setpointmin',
							name: $.t('Set Point') + '_min',
							color: 'rgba(39,127,255,0.8)',
							linkedTo: ':previous',
							zIndex: 1,
							tooltip: {
								valueSuffix: ' \u00B0' + $.myglobals.tempsign,
								valueDecimals: 1
							},
							yAxis: 0
						}, false);
						series = chart.get('setpointmin');
						series.setData(datatablesm, false);
			
						chart.addSeries( {
							id: 'setpointmax',
							name: $.t('Set Point') + '_max',
							color: 'rgba(127,39,255,0.8)',
							linkedTo: ':previous',
							zIndex: 1,
							tooltip: {
								valueSuffix: ' \u00B0' + $.myglobals.tempsign,
								valueDecimals: 1
							},
							yAxis: 0
						}, false);
						series = chart.get('setpointmax');
						series.setData(datatablesx, false);
			*/

			if (datatablesrange.length != 0) {
				chart.addSeries({
					id: 'setpointrange',
					name: $.t('Set Point') + '_range',
					color: 'rgba(164,75,148,1.0)',
					type: 'areasplinerange',
					linkedTo: ':previous',
					zIndex: 1,
					lineWidth: 0,
					fillOpacity: 0.5,
					yAxis: 0,
					tooltip: {
						valueSuffix: ' \u00B0' + $.myglobals.tempsign,
						valueDecimals: 1
					}
				}, false);
				series = chart.get('setpointrange');
				series.setData(datatablesrange, false);
			}
			if (datatablese_prev.length != 0) {
				chart.addSeries({
					id: 'prev_setpoint',
					name: $.t('Past') + ' ' + $.t('Set Point'),
					color: 'rgba(223,212,246,0.8)',
					zIndex: 3,
					yAxis: 0,
					tooltip: {
						valueSuffix: ' \u00B0' + $.myglobals.tempsign,
						valueDecimals: 1
					},
					visible: false
				}, false);
				series = chart.get('prev_setpoint');
				series.setData(datatablese_prev, false);
			}
		}
	}

	if (datatablete.length != 0) {
		//Add Temperature series
		if (isday == 1) {
			chart.addSeries({
				id: 'temperature',
				name: $.t('Temperature'),
				color: 'yellow',
				yAxis: 0,
				tooltip: {
					valueSuffix: ' \u00B0' + $.myglobals.tempsign,
					valueDecimals: 1
				}
			}, false);
			series = chart.get('temperature');
			series.setData(datatablete, false);
		} else {
			//Min/Max range
			if (datatableta.length != 0) {
				chart.addSeries({
					id: 'temperature_avg',
					name: $.t('Temperature'),
					color: 'yellow',
					fillOpacity: 0.7,
					yAxis: 0,
					zIndex: 2,
					tooltip: {
						valueSuffix: ' \u00B0' + $.myglobals.tempsign,
						valueDecimals: 1
					}
				}, false);
				series = chart.get('temperature_avg');
				series.setData(datatableta, false);
				var trandLine = CalculateTrendLine(datatableta);
				if (typeof trandLine != 'undefined') {
					var datatableTrendline = [];
					datatableTrendline.push([trandLine.x0, trandLine.y0]);
					datatableTrendline.push([trandLine.x1, trandLine.y1]);
				}
			}
			if (datatabletrange.length != 0) {
				chart.addSeries({
					id: 'temperature',
					name: $.t('Temperature') + '_range',
					color: 'rgba(3,190,252,1.0)',
					type: 'areasplinerange',
					linkedTo: ':previous',
					zIndex: 0,
					lineWidth: 0,
					fillOpacity: 0.5,
					yAxis: 0,
					tooltip: {
						valueSuffix: ' \u00B0' + $.myglobals.tempsign,
						valueDecimals: 1
					}
				}, false);
				series = chart.get('temperature');
				series.setData(datatabletrange, false);
			}
			if (datatableta_prev.length != 0) {
				chart.addSeries({
					id: 'prev_temperature',
					name: $.t('Past') + ' ' + $.t('Temperature'),
					color: 'rgba(224,224,230,0.8)',
					zIndex: 3,
					yAxis: 0,
					tooltip: {
						valueSuffix: ' \u00B0' + $.myglobals.tempsign,
						valueDecimals: 1
					},
					visible: false
				}, false);
				series = chart.get('prev_temperature');
				series.setData(datatableta_prev, false);
			}
		}
	}
	if (typeof datatableTrendline != 'undefined') {
		if (datatableTrendline.length > 0) {
			chart.addSeries({
				id: 'temp_trendline',
				name: $.t('Trendline') + ' ' + $.t('Temperature'),
				zIndex: 1,
				tooltip: {
					valueSuffix: ' \u00B0' + $.myglobals.tempsign,
					valueDecimals: 1
				},
				color: 'rgba(255,3,3,0.8)',
				dashStyle: 'LongDash',
				yAxis: 0,
				visible: false
			}, false);
			series = chart.get('temp_trendline');
			series.setData(datatableTrendline, false);
		}
	}
	return;
	if (datatabledp.length != 0) {
		chart.addSeries({
			id: 'dewpoint',
			name: $.t('Dew Point'),
			color: 'blue',
			yAxis: 0,
			tooltip: {
				valueSuffix: ' \u00B0' + $.myglobals.tempsign,
				valueDecimals: 1
			}
		}, false);
		series = chart.get('dewpoint');
		series.setData(datatabledp, false);
	}
	if (datatableba.length != 0) {
		chart.addSeries({
			id: 'baro',
			name: $.t('Barometer'),
			color: 'pink',
			yAxis: 2,
			tooltip: {
				valueSuffix: ' hPa',
				valueDecimals: 1
			}
		}, false);
		series = chart.get('baro');
		series.setData(datatableba, false);
	}
}

function load_cam_video() {
	if ((typeof $.camfeed == 'undefined') || ($.camfeed == ""))
		return;
	reload_cam_image();
	$.myglobals.refreshTimer = setTimeout(reload_cam_image, 100);
}

function reload_cam_image() {
	if (typeof $.myglobals.refreshTimer != 'undefined') {
		clearTimeout($.myglobals.refreshTimer)
	}
	if ($.camfeed == "")
		return;
	var xx = new Image();
	xx.src = $.camfeed + "&count=" + $.count + "?t=" + new Date().getTime();
	$.count++;
	$('#dialog-camera-live #camfeed').attr("src", xx.src);
}

function ShowCameraLiveStream(Name, camIdx) {
	$.count = 0;
	$.camfeed = "camsnapshot.jpg?idx=" + camIdx;

	$('#dialog-camera-live #camfeed').attr("src", "images/camera_default.png");
	//$('#dialog-camera-live #camfeed').attr("src", FeedURL);

	var dwidth = $(window).width() / 2;
	var dheight = $(window).height() / 2;

	if (dwidth > 630) {
		dwidth = 630;
		dheight = parseInt((dwidth / 16) * 9);
	}
	if (dheight > 470) {
		dheight = 470;
		dwidth = parseInt((dheight / 9) * 16);
	}
	if (dwidth > dheight) {
		dwidth = parseInt((dheight / 9) * 16);
	}
	else {
		dheight = parseInt((dwidth / 16) * 9);
	}
	//Set inner Camera feed width/height
	$("#dialog-camera-live #camfeed").width(dwidth - 30);
	$("#dialog-camera-live #camfeed").height(dheight - 16);

	$("#dialog-camera-live").dialog({
		resizable: false,
		width: dwidth + 2,
		height: dheight + 118,
		position: {
			my: "center",
			at: "center",
			of: window
		},
		modal: true,
		title: unescape(Name),
		buttons: {
			"OK": function () {
				$(this).dialog("close");
			}
		},
		open: function () {
			load_cam_video();
		},
		close: function () {
			$.camfeed = "";
			if (typeof $.myglobals.refreshTimer != 'undefined') {
				clearTimeout($.myglobals.refreshTimer)
			}
			$('#dialog-camera-live #camfeed').attr("src", "images/camera_default.png");
			$(this).dialog("close");
		}
	});
}

function reload_media_remote() {
	if (typeof $.myglobals.refreshTimer != 'undefined') {
		clearTimeout($.myglobals.refreshTimer)
	}
	return;
}

function click_media_remote(action) {
	var devIdx = $("#dialog-media-remote").attr("DeviceIndex");
	var HWType = $("#dialog-media-remote").attr("HardwareType");
	if (devIdx.length > 0) {
		if (HWType.indexOf('Kodi') >= 0) {
			$.ajax({
				url: "json.htm?type=command&param=kodimediacommand&idx=" + devIdx + "&action=" + action,
				async: true,
				dataType: 'json',
				//			 success: function(data) { $.cachenoty=generate_noty('info', '<b>Sent remote command</b>', 100); },
				error: function () { $.cachenoty = generate_noty('error', '<b>Problem sending remote command</b>', 1000); }
			});
		}
		else if (HWType.indexOf('Panasonic') >= 0) {
			$.ajax({
				url: "json.htm?type=command&param=panasonicmediacommand&idx=" + devIdx + "&action=" + action,
				async: true,
				dataType: 'json',
				//			 success: function(data) { $.cachenoty=generate_noty('info', '<b>Sent remote command</b>', 100); },
				error: function () { $.cachenoty = generate_noty('error', '<b>Problem sending remote command</b>', 1000); }
			});
		}
		else
			$.cachenoty = generate_noty('error', '<b>Device Hardware is unknown.</b>', 1000);
	} else $.cachenoty = generate_noty('error', '<b>Device Index is unknown.</b>', 1000);
}

function click_lmsplayer_remote(action) {
	var devIdx = $("#dialog-lmsplayer-remote").attr("DeviceIndex");
	if (devIdx.length > 0) {
		$.ajax({
			url: "json.htm?type=command&param=lmsmediacommand&idx=" + devIdx + "&action=" + action,
			async: true,
			dataType: 'json',
			error: function () { $.cachenoty = generate_noty('error', '<b>Problem sending remote command</b>', 1000); }
		});
	} else $.cachenoty = generate_noty('error', '<b>Device Index is unknown.</b>', 1000);
}

function click_heosplayer_remote(action) {
	var devIdx = $("#dialog-heosplayer-remote").attr("DeviceIndex");
	if (devIdx.length > 0) {
		$.ajax({
			url: "json.htm?type=command&param=heosmediacommand&idx=" + devIdx + "&action=" + action,
			async: true,
			dataType: 'json',
			error: function () { $.cachenoty = generate_noty('error', '<b>Problem sending remote command</b>', 1000); }
		});
	} else $.cachenoty = generate_noty('error', '<b>Device Index is unknown.</b>', 1000);
}

function ShowMediaRemote(Name, devIdx, HWType) {
	var divId;
	var svgId;
	if (HWType.indexOf('Kodi') >= 0 || HWType.indexOf('Panasonic') >= 0) {
		divId = '#dialog-media-remote';
		svgId = '#MediaRemote';
	}
	else if (HWType.indexOf('Logitech Media Server') >= 0) {
		divId = '#dialog-lmsplayer-remote';
		svgId = '#LMSPlayerRemote';
	}
	else if (HWType.indexOf('HEOS by DENON') >= 0) {
		divId = '#dialog-heosplayer-remote';
		svgId = '#HEOSPlayerRemote';
	}
	else return;
	// Need to make as big as possible so work out maximum height then set width appropriately
	var vBox = $(svgId).prop("viewBox").baseVal;
	var svgRatio = (vBox.width - vBox.x) / (vBox.height - vBox.y);
	var dheight = $(window).height() * 0.85;
	var dwidth = dheight * svgRatio;
	// for v2.0, if screen is wide enough add room to show media at the side of the remote
	$(divId).dialog({
		resizable: false,
		show: "blind",
		hide: "blind",
		width: dwidth,
		height: dheight,
		position: { my: "center", at: "center", of: window },
		fluid: true,
		modal: true,
		title: unescape(Name),
		open: function () {
			$(divId).attr("DeviceIndex", devIdx);
			$(divId).attr("HardwareType", HWType);
			$(svgId).css("-ms-overflow-style", "none");
			$(divId).bind('touchstart', function () { });
		},
		close: function () {
			if (typeof $.myglobals.refreshTimer != 'undefined') {
				clearTimeout($.myglobals.refreshTimer)
			}
			$(this).dialog("close");
		}
	});
}

function AddDataToCurrentChart(data, chart, switchtype, isday) {
	var datatablev1 = [];
	var datatablev2 = [];
	var datatablev3 = [];
	var datatablev4 = [];
	var datatablev5 = [];
	var datatablev6 = [];
	$.each(data.result, function (i, item) {
		if (isday == 1) {
			datatablev1.push([GetUTCFromString(item.d), parseFloat(item.v1)]);
			datatablev2.push([GetUTCFromString(item.d), parseFloat(item.v2)]);
			datatablev3.push([GetUTCFromString(item.d), parseFloat(item.v3)]);
		}
		else {
			datatablev1.push([GetDateFromString(item.d), parseFloat(item.v1)]);
			datatablev2.push([GetDateFromString(item.d), parseFloat(item.v2)]);
			datatablev3.push([GetDateFromString(item.d), parseFloat(item.v3)]);
			datatablev4.push([GetDateFromString(item.d), parseFloat(item.v4)]);
			datatablev5.push([GetDateFromString(item.d), parseFloat(item.v5)]);
			datatablev6.push([GetDateFromString(item.d), parseFloat(item.v6)]);
		}
	});

	var series;
	if (switchtype == 0) {
		//Current (A)
		if (isday == 1) {
			if (data.haveL1) {
				chart.addSeries({
					id: 'current1',
					name: 'Current_L1',
					color: 'rgba(3,190,252,0.8)',
					yAxis: 0,
					tooltip: {
						valueSuffix: ' A',
						valueDecimals: 1
					}
				}, false);
				series = chart.get('current1');
				series.setData(datatablev1, false);
			}
			if (data.haveL2) {
				chart.addSeries({
					id: 'current2',
					name: 'Current_L2',
					color: 'rgba(252,190,3,0.8)',
					yAxis: 0,
					tooltip: {
						valueSuffix: ' A',
						valueDecimals: 1
					}
				}, false);
				series = chart.get('current2');
				series.setData(datatablev2, false);
			}
			if (data.haveL3) {
				chart.addSeries({
					id: 'current3',
					name: 'Current_L3',
					color: 'rgba(190,252,3,0.8)',
					yAxis: 0,
					tooltip: {
						valueSuffix: ' A',
						valueDecimals: 1
					}
				}, false);
				series = chart.get('current3');
				series.setData(datatablev3, false);
			}
		}
		else {
			//month/year
			if (data.haveL1) {
				chart.addSeries({
					id: 'current1min',
					name: 'Current_L1_Min',
					color: 'rgba(3,190,252,0.8)',
					yAxis: 0
				}, false);
				chart.addSeries({
					id: 'current1max',
					name: 'Current_L1_Max',
					color: 'rgba(3,252,190,0.8)',
					yAxis: 0,
					tooltip: {
						valueSuffix: ' A',
						valueDecimals: 1
					}
				}, false);
				series = chart.get('current1min');
				series.setData(datatablev1, false);
				series = chart.get('current1max');
				series.setData(datatablev2, false);
			}
			if (data.haveL2) {
				chart.addSeries({
					id: 'current2min',
					name: 'Current_L2_Min',
					color: 'rgba(252,190,3,0.8)',
					yAxis: 0,
					tooltip: {
						valueSuffix: ' A',
						valueDecimals: 1
					}
				}, false);
				chart.addSeries({
					id: 'current2max',
					name: 'Current_L2_Max',
					color: 'rgba(252,3,190,0.8)',
					yAxis: 0,
					tooltip: {
						valueSuffix: ' A',
						valueDecimals: 1
					}
				}, false);
				series = chart.get('current2min');
				series.setData(datatablev3, false);
				series = chart.get('current2max');
				series.setData(datatablev4, false);
			}
			if (data.haveL3) {
				chart.addSeries({
					id: 'current3min',
					name: 'Current_L3_Min',
					color: 'rgba(190,252,3,0.8)',
					yAxis: 0,
					tooltip: {
						valueSuffix: ' A',
						valueDecimals: 1
					}
				}, false);
				chart.addSeries({
					id: 'current3max',
					name: 'Current_L3_Min',
					color: 'rgba(112,146,190,0.8)',
					yAxis: 0,
					tooltip: {
						valueSuffix: ' A',
						valueDecimals: 1
					}
				}, false);
				series = chart.get('current3min');
				series.setData(datatablev5, false);
				series = chart.get('current3max');
				series.setData(datatablev6, false);
			}
		}
		chart.yAxis[0].options.title.text = 'Current (A)';
	}
	else if (switchtype == 1) {
		//Watt
		if (isday == 1) {
			if (data.haveL1) {
				chart.addSeries({
					id: 'current1',
					name: $.t('Usage') + ' L1',
					color: 'rgba(3,190,252,0.8)',
					yAxis: 0,
					tooltip: {
						valueSuffix: ' Watt',
						valueDecimals: 1
					}
				}, false);
				series = chart.get('current1');
				series.setData(datatablev1, false);
			}
			if (data.haveL2) {
				chart.addSeries({
					id: 'current2',
					name: $.t('Usage') + ' L2',
					color: 'rgba(252,190,3,0.8)',
					yAxis: 0,
					tooltip: {
						valueSuffix: ' Watt',
						valueDecimals: 1
					}
				}, false);
				series = chart.get('current2');
				series.setData(datatablev2, false);
			}
			if (data.haveL3) {
				chart.addSeries({
					id: 'current3',
					name: $.t('Usage') + ' L3',
					color: 'rgba(190,252,3,0.8)',
					yAxis: 0,
					tooltip: {
						valueSuffix: ' Watt',
						valueDecimals: 1
					}
				}, false);
				series = chart.get('current3');
				series.setData(datatablev3, false);
			}
		}
		else {
			if (data.haveL1) {
				chart.addSeries({
					id: 'current1min',
					name: $.t('Usage') + ' L1_Min',
					color: 'rgba(3,190,252,0.8)',
					yAxis: 0,
					tooltip: {
						valueSuffix: ' Watt',
						valueDecimals: 1
					}
				}, false);
				chart.addSeries({
					id: 'current1max',
					name: $.t('Usage') + ' L1_Max',
					color: 'rgba(3,252,190,0.8)',
					yAxis: 0,
					tooltip: {
						valueSuffix: ' Watt',
						valueDecimals: 1
					}
				}, false);
				series = chart.get('current1min');
				series.setData(datatablev1, false);
				series = chart.get('current1max');
				series.setData(datatablev2, false);
			}
			if (data.haveL2) {
				chart.addSeries({
					id: 'current2min',
					name: $.t('Usage') + ' L2_Min',
					color: 'rgba(252,190,3,0.8)',
					yAxis: 0,
					tooltip: {
						valueSuffix: ' Watt',
						valueDecimals: 1
					}
				}, false);
				chart.addSeries({
					id: 'current2max',
					name: $.t('Usage') + ' L2_Max',
					color: 'rgba(252,3,190,0.8)',
					yAxis: 0,
					tooltip: {
						valueSuffix: ' Watt',
						valueDecimals: 1
					}
				}, false);
				series = chart.get('current2min');
				series.setData(datatablev3, false);
				series = chart.get('current2max');
				series.setData(datatablev4, false);
			}
			if (data.haveL3) {
				chart.addSeries({
					id: 'current3min',
					name: $.t('Usage') + ' L3_Min',
					color: 'rgba(190,252,3,0.8)',
					yAxis: 0,
					tooltip: {
						valueSuffix: ' Watt',
						valueDecimals: 1
					}
				}, false);
				chart.addSeries({
					id: 'current3max',
					name: $.t('Usage') + ' L3_Min',
					color: 'rgba(112,146,190,0.8)',
					yAxis: 0,
					tooltip: {
						valueSuffix: ' Watt',
						valueDecimals: 1
					}
				}, false);
				series = chart.get('current3min');
				series.setData(datatablev5, false);
				series = chart.get('current3max');
				series.setData(datatablev6, false);
			}
		}
		chart.yAxis[0].options.title.text = 'Usage (Watt)';
	}
}

function ShowCurrentLog(contentdiv, backfunction, id, name, switchtype) {
	clearInterval($.myglobals.refreshTimer);
	$(window).scrollTop(0);
	$('#modal').show();
	$.content = contentdiv;
	$.backfunction = backfunction;
	$.devIdx = id;
	$.devName = name;
	if (typeof switchtype != 'undefined') {
		$.devSwitchType = switchtype;
	}
	else {
		switchtype = $.devSwitchType;
	}
	var htmlcontent = '';
	htmlcontent = '<p><center><h2>' + unescape(name) + '</h2></center></p>\n';
	htmlcontent += $('#daymonthyearlog').html();
	$($.content).html(GetBackbuttonHTMLTable(backfunction) + htmlcontent);
	$($.content).i18n();

	$.DayChart = $($.content + ' #daygraph');
	$.DayChart.highcharts({
		chart: {
			type: 'spline',
			zoomType: 'x',
			resetZoomButton: {
				position: {
					x: -30,
					y: -36
				}
			},
			events: {
				load: function () {
					$.getJSON("json.htm?type=graph&sensor=counter&idx=" + id + "&range=day", function (data) {
						if (typeof data.result != 'undefined') {
							AddDataToCurrentChart(data, $.DayChart.highcharts(), switchtype, 1);
							$.DayChart.highcharts().redraw();
						}
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
			text: Get5MinuteHistoryDaysGraphTitle()
		},
		xAxis: {
			type: 'datetime'
		},
		yAxis: {
			title: {
				text: $.t('Usage')
			},
			min: 0
		},
		tooltip: {
			crosshairs: true,
			shared: true
		},
		plotOptions: {
			series: {
				point: {
					events: {
						click: function (event) {
							chartPointClickNew(event, true, ShowCurrentLog);
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
		},
		legend: {
			enabled: true
		}
	});

	$.MonthChart = $($.content + ' #monthgraph');
	$.MonthChart.highcharts({
		chart: {
			type: 'spline',
			marginRight: 10,
			zoomType: 'x',
			resetZoomButton: {
				position: {
					x: -30,
					y: -36
				}
			},
			events: {
				load: function () {
					$.getJSON("json.htm?type=graph&sensor=counter&idx=" + id + "&range=month", function (data) {
						if (typeof data.result != 'undefined') {
							AddDataToCurrentChart(data, $.MonthChart.highcharts(), switchtype, 0);
							$.MonthChart.highcharts().redraw();
						}
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
			text: $.t('Last Month')
		},
		xAxis: {
			type: 'datetime'
		},
		yAxis: {
			title: {
				text: $.t('Usage')
			},
			min: 0
		},
		tooltip: {
			crosshairs: true,
			shared: true
		},
		plotOptions: {
			series: {
				point: {
					events: {
						click: function (event) {
							chartPointClickNew(event, false, ShowCurrentLog);
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
		},
		legend: {
			enabled: true
		}
	});

	$.YearChart = $($.content + ' #yeargraph');
	$.YearChart.highcharts({
		chart: {
			type: 'spline',
			marginRight: 10,
			zoomType: 'x',
			resetZoomButton: {
				position: {
					x: -30,
					y: -36
				}
			},
			events: {
				load: function () {

					$.getJSON("json.htm?type=graph&sensor=counter&idx=" + id + "&range=year", function (data) {
						if (typeof data.result != 'undefined') {
							AddDataToCurrentChart(data, $.YearChart.highcharts(), switchtype, 0);
							$.YearChart.highcharts().redraw();
						}
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
			text: $.t('Last Year')
		},
		xAxis: {
			type: 'datetime'
		},
		yAxis: {
			title: {
				text: $.t('Usage')
			},
			min: 0
		},
		tooltip: {
			crosshairs: true,
			shared: true
		},
		plotOptions: {
			series: {
				point: {
					events: {
						click: function (event) {
							chartPointClickNew(event, false, ShowCurrentLog);
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
		},
		legend: {
			enabled: true
		}
	});
}

function ShowUVLog(contentdiv, backfunction, id, name) {
	clearInterval($.myglobals.refreshTimer);
	$(window).scrollTop(0);
	$('#modal').show();
	$.content = contentdiv;
	$.backfunction = backfunction;
	$.devIdx = id;
	$.devName = name;
	var htmlcontent = '';
	htmlcontent = '<p><center><h2>' + unescape(name) + '</h2></center></p><br>\n';
	htmlcontent += $('#daymonthyearlog').html();
	$($.content).html(GetBackbuttonHTMLTable(backfunction) + htmlcontent);
	$($.content).i18n();

	$.DayChart = $($.content + ' #daygraph');
	$.DayChart.highcharts({
		chart: {
			type: 'spline',
			zoomType: 'x',
			resetZoomButton: {
				position: {
					x: -30,
					y: -36
				}
			},
			marginRight: 10,
			events: {
				load: function () {
					$.getJSON("json.htm?type=graph&sensor=uv&idx=" + id + "&range=day", function (data) {
						if (typeof data.result != 'undefined') {
							var series = $.DayChart.highcharts().series[0];
							var datatable = [];

							$.each(data.result, function (i, item) {
								datatable.push([GetUTCFromString(item.d), parseFloat(item.uvi)]);
							});
							series.setData(datatable);
						}
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
			text: 'UV ' + Get5MinuteHistoryDaysGraphTitle()
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
			crosshairs: true,
			shared: true
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
			id: 'uv',
			name: 'uv',
			tooltip: {
				valueSuffix: ' UVI',
				valueDecimals: 1
			},
			point: {
				events: {
					click: function (event) {
						chartPointClickNew(event, true, ShowUVLog);
					}
				}
			}
		}],
		legend: {
			enabled: true
		},
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
			zoomType: 'x',
			resetZoomButton: {
				position: {
					x: -30,
					y: -36
				}
			},
			marginRight: 10,
			events: {
				load: function () {

					$.getJSON("json.htm?type=graph&sensor=uv&idx=" + id + "&range=month", function (data) {
						if (typeof data.result != 'undefined') {
							var series = $.MonthChart.highcharts().series[0];
							var datatable = [];
							$.each(data.result, function (i, item) {
								datatable.push([GetDateFromString(item.d), parseFloat(item.uvi)]);
							});
							series.setData(datatable, false);
							var bHavePrev = (typeof data.resultprev != 'undefined');
							if (bHavePrev) {
								datatable = [];
								series = $.MonthChart.highcharts().series[1];
								$.each(data.resultprev, function (i, item) {
									datatable.push([GetPrevDateFromString(item.d), parseFloat(item.uvi)]);
								});
								series.setData(datatable, false);
							}
							$.MonthChart.highcharts().redraw();
						}
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
			text: 'UV ' + $.t('Last Month')
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
			crosshairs: true,
			shared: true
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
			id: 'uv',
			name: 'uv',
			tooltip: {
				valueSuffix: ' UVI',
				valueDecimals: 1
			},
			point: {
				events: {
					click: function (event) {
						chartPointClickNew(event, false, ShowUVLog);
					}
				}
			}
		}, {
			id: 'past_uv',
			name: $.t('Past') + ' uv',
			visible: false,
			tooltip: {
				valueSuffix: ' UVI',
				valueDecimals: 1
			},
			point: {
				events: {
					click: function (event) {
						chartPointClickNew(event, false, ShowUVLog);
					}
				}
			}
		}],
		legend: {
			enabled: true
		},
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
			zoomType: 'x',
			resetZoomButton: {
				position: {
					x: -30,
					y: -36
				}
			},
			marginRight: 10,
			events: {
				load: function () {

					$.getJSON("json.htm?type=graph&sensor=uv&idx=" + id + "&range=year", function (data) {
						if (typeof data.result != 'undefined') {
							var series = $.YearChart.highcharts().series[0];
							var datatable = [];
							$.each(data.result, function (i, item) {
								datatable.push([GetDateFromString(item.d), parseFloat(item.uvi)]);
							});
							series.setData(datatable, false);
							var bHavePrev = (typeof data.resultprev != 'undefined');
							if (bHavePrev) {
								datatable = [];
								series = $.YearChart.highcharts().series[1];
								$.each(data.resultprev, function (i, item) {
									datatable.push([GetPrevDateFromString(item.d), parseFloat(item.uvi)]);
								});
								series.setData(datatable, false);
							}
							$.YearChart.highcharts().redraw();
						}
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
			text: 'UV ' + $.t('Last Year')
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
			crosshairs: true,
			shared: true
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
			id: 'uv',
			name: 'uv',
			tooltip: {
				valueSuffix: ' UVI',
				valueDecimals: 1
			},
			point: {
				events: {
					click: function (event) {
						chartPointClickNew(event, false, ShowUVLog);
					}
				}
			}
		}, {
			id: 'past_uv',
			name: $.t('Past') + ' uv',
			visible: false,
			tooltip: {
				valueSuffix: ' UVI',
				valueDecimals: 1
			},
			point: {
				events: {
					click: function (event) {
						chartPointClickNew(event, false, ShowUVLog);
					}
				}
			}
		}],
		legend: {
			enabled: true
		},
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

var wind_directions = ['N', 'NNE', 'NE', 'ENE', 'E', 'ESE', 'SE', 'SSE', 'S', 'SSW', 'SW', 'WSW', 'W', 'WNW', 'NW', 'NNW'];
function ShowWindLog(contentdiv, backfunction, id, name) {
	clearInterval($.myglobals.refreshTimer);
	$(window).scrollTop(0);
	$('#modal').show();
	$.content = contentdiv;
	$.backfunction = backfunction;
	$.devIdx = id;
	$.devName = name;
	var htmlcontent = '';

	htmlcontent = '<p><center><h2>' + unescape(name) + '</h2></center></p><br>\n';
	htmlcontent += $('#windlog').html();
	$($.content).html(GetBackbuttonHTMLTable(backfunction) + htmlcontent);
	$($.content).i18n();

	$.DayChart = $($.content + ' #winddaygraph');

	var lscales = [];
	if ($.myglobals.windsign == "bf") {
		lscales.push({ "from": 0, "to": 1 });
		lscales.push({ "from": 1, "to": 2 });
		lscales.push({ "from": 2, "to": 3 });
		lscales.push({ "from": 3, "to": 4 });
		lscales.push({ "from": 4, "to": 5 });
		lscales.push({ "from": 5, "to": 6 });
		lscales.push({ "from": 6, "to": 7 });
		lscales.push({ "from": 7, "to": 8 });
		lscales.push({ "from": 8, "to": 9 });
		lscales.push({ "from": 9, "to": 10 });
		lscales.push({ "from": 10, "to": 11 });
		lscales.push({ "from": 11, "to": 12 });
		lscales.push({ "from": 12, "to": 100 });
	} else {
		lscales.push({ "from": 0.3 * $.myglobals.windscale, "to": 1.5 * $.myglobals.windscale });
		lscales.push({ "from": 1.5 * $.myglobals.windscale, "to": 3.3 * $.myglobals.windscale });
		lscales.push({ "from": 3.3 * $.myglobals.windscale, "to": 5.5 * $.myglobals.windscale });
		lscales.push({ "from": 5.5 * $.myglobals.windscale, "to": 8 * $.myglobals.windscale });
		lscales.push({ "from": 8.0 * $.myglobals.windscale, "to": 10.8 * $.myglobals.windscale });
		lscales.push({ "from": 10.8 * $.myglobals.windscale, "to": 13.9 * $.myglobals.windscale });
		lscales.push({ "from": 13.9 * $.myglobals.windscale, "to": 17.2 * $.myglobals.windscale });
		lscales.push({ "from": 17.2 * $.myglobals.windscale, "to": 20.8 * $.myglobals.windscale });
		lscales.push({ "from": 20.8 * $.myglobals.windscale, "to": 24.5 * $.myglobals.windscale });
		lscales.push({ "from": 24.5 * $.myglobals.windscale, "to": 28.5 * $.myglobals.windscale });
		lscales.push({ "from": 28.5 * $.myglobals.windscale, "to": 32.7 * $.myglobals.windscale });
		lscales.push({ "from": 32.7 * $.myglobals.windscale, "to": 100 * $.myglobals.windscale });
		lscales.push({ "from": 32.7 * $.myglobals.windscale, "to": 100 * $.myglobals.windscale });
	}
	$.DayChart.highcharts({
		chart: {
			type: 'spline',
			zoomType: 'x',
			resetZoomButton: {
				position: {
					x: -30,
					y: -36
				}
			},
			marginRight: 10,
			events: {
				load: function () {
					$.getJSON("json.htm?type=graph&sensor=wind&idx=" + id + "&range=day", function (data) {
						if (typeof data.result != 'undefined') {
							var seriessp = $.DayChart.highcharts().series[0];
							var seriesgu = $.DayChart.highcharts().series[1];
							var datatablesp = [];
							var datatablegu = [];

							$.each(data.result, function (i, item) {
								datatablesp.push([GetUTCFromString(item.d), parseFloat(item.sp)]);
								datatablegu.push([GetUTCFromString(item.d), parseFloat(item.gu)]);
							});
							seriessp.setData(datatablesp, false);
							seriesgu.setData(datatablegu, false);
							$.DayChart.highcharts().redraw();
						}
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
			text: $.t('Wind') + ' ' + $.t('speed/gust') + ' ' + Get5MinuteHistoryDaysGraphTitle()
		},
		xAxis: {
			type: 'datetime'
		},
		yAxis: {
			title: {
				text: $.t('Speed') + ' (' + $.myglobals.windsign + ')'
			},
			min: 0,
			minorGridLineWidth: 0,
			gridLineWidth: 0,
			alternateGridColor: null,
			plotBands: [{ // Light air
				from: lscales[0]["from"],
				to: lscales[0]["to"],
				color: 'rgba(68, 170, 213, 0.1)',
				label: {
					text: $.t('Light air'),
					style: {
						color: '#CCCCCC'
					}
				}
			}, { // Light breeze
				from: lscales[1]["from"],
				to: lscales[1]["to"],
				color: 'rgba(68, 170, 213, 0.2)',
				label: {
					text: $.t('Light breeze'),
					style: {
						color: '#CCCCCC'
					}
				}
			}, { // Gentle breeze
				from: lscales[2]["from"],
				to: lscales[2]["to"],
				color: 'rgba(68, 170, 213, 0.1)',
				label: {
					text: $.t('Gentle breeze'),
					style: {
						color: '#CCCCCC'
					}
				}
			}, { // Moderate breeze
				from: lscales[3]["from"],
				to: lscales[3]["to"],
				color: 'rgba(68, 170, 213, 0.2)',
				label: {
					text: $.t('Moderate breeze'),
					style: {
						color: '#CCCCCC'
					}
				}
			}, { // Fresh breeze
				from: lscales[4]["from"],
				to: lscales[4]["to"],
				color: 'rgba(68, 170, 213, 0.1)',
				label: {
					text: $.t('Fresh breeze'),
					style: {
						color: '#CCCCCC'
					}
				}
			}, { // Strong breeze
				from: lscales[5]["from"],
				to: lscales[5]["to"],
				color: 'rgba(68, 170, 213, 0.2)',
				label: {
					text: $.t('Strong breeze'),
					style: {
						color: '#CCCCCC'
					}
				}
			}, { // High wind
				from: lscales[6]["from"],
				to: lscales[6]["from"],
				color: 'rgba(68, 170, 213, 0.1)',
				label: {
					text: $.t('High wind'),
					style: {
						color: '#CCCCCC'
					}
				}
			}, { // fresh gale
				from: lscales[7]["from"],
				to: lscales[7]["to"],
				color: 'rgba(68, 170, 213, 0.1)',
				label: {
					text: $.t('Fresh gale'),
					style: {
						color: '#CCCCCC'
					}
				}
			}, { // strong gale
				from: lscales[8]["from"],
				to: lscales[8]["to"],
				color: 'rgba(68, 170, 213, 0.2)',
				label: {
					text: $.t('Strong gale'),
					style: {
						color: '#CCCCCC'
					}
				}
			}, { // storm
				from: lscales[9]["from"],
				to: lscales[9]["to"],
				color: 'rgba(68, 170, 213, 0.1)',
				label: {
					text: $.t('Storm'),
					style: {
						color: '#CCCCCC'
					}
				}
			}, { // Violent storm
				from: lscales[10]["from"],
				to: lscales[10]["to"],
				color: 'rgba(68, 170, 213, 0.2)',
				label: {
					text: $.t('Violent storm'),
					style: {
						color: '#CCCCCC'
					}
				}
			}, { // hurricane
				from: lscales[11]["from"],
				to: lscales[11]["to"],
				color: 'rgba(68, 170, 213, 0.1)',
				label: {
					text: $.t('Hurricane'),
					style: {
						color: '#CCCCCC'
					}
				}
			}
			]
		},
		tooltip: {
			crosshairs: true,
			shared: true
		},
		plotOptions: {
			series: {
				point: {
					events: {
						click: function (event) {
							chartPointClickNew(event, true, ShowWindLog);
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
		},
		series: [{
			id: 'speed',
			name: $.t('Speed'),
			tooltip: {
				valueSuffix: ' ' + $.myglobals.windsign,
				valueDecimals: 1
			}
		}, {
			id: 'gust',
			name: $.t('Gust'),
			tooltip: {
				valueSuffix: ' ' + $.myglobals.windsign,
				valueDecimals: 1
			}
		}],
		navigation: {
			menuItemStyle: {
				fontSize: '10px'
			}
		}
	});

	$.DirectionChart = $($.content + ' #winddirectiongraph');
	$.DirectionChart.highcharts({
		chart: {
			polar: true,
			type: 'column',
			events: {
				load: function () {
					$.getJSON("json.htm?type=graph&sensor=winddir&idx=" + id + "&range=day", function (data) {
						if (typeof data.result_speed != 'undefined') {
							$.each(data.result_speed, function (i, item) {
								//make the series
								var datatable_speed = [];
								for (jj = 0; jj < 16; jj++) {
									datatable_speed.push([wind_directions[i], parseFloat(item.sp[jj])]);
								}
								$.DirectionChart.highcharts().addSeries({
									id: i + 1,
									name: item.label
								}, false);
								var series = $.DirectionChart.highcharts().get(i + 1);
								series.setData(datatable_speed, false);
							});
							$.DirectionChart.highcharts().redraw();
						}
					});
				}
			}
		},
		title: {
			text: $.t('Wind') + ' ' + $.t('Direction') + ' ' + Get5MinuteHistoryDaysGraphTitle()
		},
		pane: {
			size: '85%'
		},
		credits: {
			enabled: true,
			href: "http://www.domoticz.com",
			text: "Domoticz.com"
		},
		xAxis: {
			tickmarkPlacement: 'on',
			tickWidth: 1,
			tickPosition: 'outside',
			tickLength: 5,
			tickColor: '#999',
			tickInterval: 1,
			labels: {
				formatter: function () {
					return wind_directions[this.value] + '°'; // return text for label
				}
			}
		},
		yAxis: {
			min: 0,
			showLastLabel: true,
			title: {
				text: 'Frequency (%)'
			},
			labels: {
				formatter: function () {
					return this.value + '%';
				}
			},
			reversedStacks: false
		},
		tooltip: {
			formatter: function () {
				return '' +
					wind_directions[this.x] + ': ' + this.y + ' %';
			}
		},
		plotOptions: {
			series: {
				stacking: 'normal',
				shadow: false,
				groupPadding: 0,
				pointPlacement: 'on'
			}
		},
		legend: {
			align: 'right',
			verticalAlign: 'top',
			y: 100,
			layout: 'vertical'
		}
	});

	$.MonthChart = $($.content + ' #windmonthgraph');
	$.MonthChart.highcharts({
		chart: {
			type: 'spline',
			zoomType: 'x',
			resetZoomButton: {
				position: {
					x: -30,
					y: -36
				}
			},
			marginRight: 10,
			events: {
				load: function () {
					$.getJSON("json.htm?type=graph&sensor=wind&idx=" + id + "&range=month", function (data) {
						if (typeof data.result != 'undefined') {
							var seriessp = $.MonthChart.highcharts().series[0];
							var seriesgu = $.MonthChart.highcharts().series[1];

							var series_prev_sp = $.MonthChart.highcharts().series[2];
							var series_prev_gu = $.MonthChart.highcharts().series[3];

							var datatablesp = [];
							var datatablegu = [];
							var datatable_prev_sp = [];
							var datatable_prev_gu = [];

							$.each(data.result, function (i, item) {
								datatablesp.push([GetDateFromString(item.d), parseFloat(item.sp)]);
								datatablegu.push([GetDateFromString(item.d), parseFloat(item.gu)]);
							});
							var bHavePrev = (typeof data.resultprev != 'undefined');
							if (bHavePrev) {
								$.each(data.resultprev, function (i, item) {
									datatable_prev_sp.push([GetPrevDateFromString(item.d), parseFloat(item.sp)]);
									datatable_prev_gu.push([GetPrevDateFromString(item.d), parseFloat(item.gu)]);
								});
							}
							seriessp.setData(datatablesp, false);
							seriesgu.setData(datatablegu, false);
							series_prev_sp.setData(datatable_prev_sp, false);
							series_prev_gu.setData(datatable_prev_gu, false);
							$.MonthChart.highcharts().redraw();
						}
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
			text: $.t('Wind') + ' ' + $.t('speed/gust') + ' ' + $.t('Last Month')
		},
		xAxis: {
			type: 'datetime'
		},
		yAxis: {
			title: {
				text: $.t('Speed') + ' (' + $.myglobals.windsign + ')'
			},
			min: 0,
			minorGridLineWidth: 0,
			gridLineWidth: 0,
			alternateGridColor: null,
			plotBands: [{ // Light air
				from: lscales[0]["from"],
				to: lscales[0]["to"],
				color: 'rgba(68, 170, 213, 0.1)',
				label: {
					text: $.t('Light air'),
					style: {
						color: '#CCCCCC'
					}
				}
			}, { // Light breeze
				from: lscales[1]["from"],
				to: lscales[1]["to"],
				color: 'rgba(68, 170, 213, 0.2)',
				label: {
					text: $.t('Light breeze'),
					style: {
						color: '#CCCCCC'
					}
				}
			}, { // Gentle breeze
				from: lscales[2]["from"],
				to: lscales[2]["to"],
				color: 'rgba(68, 170, 213, 0.1)',
				label: {
					text: $.t('Gentle breeze'),
					style: {
						color: '#CCCCCC'
					}
				}
			}, { // Moderate breeze
				from: lscales[3]["from"],
				to: lscales[3]["to"],
				color: 'rgba(68, 170, 213, 0.2)',
				label: {
					text: $.t('Moderate breeze'),
					style: {
						color: '#CCCCCC'
					}
				}
			}, { // Fresh breeze
				from: lscales[4]["from"],
				to: lscales[4]["to"],
				color: 'rgba(68, 170, 213, 0.1)',
				label: {
					text: $.t('Fresh breeze'),
					style: {
						color: '#CCCCCC'
					}
				}
			}, { // Strong breeze
				from: lscales[5]["from"],
				to: lscales[5]["to"],
				color: 'rgba(68, 170, 213, 0.2)',
				label: {
					text: $.t('Strong breeze'),
					style: {
						color: '#CCCCCC'
					}
				}
			}, { // High wind
				from: lscales[6]["from"],
				to: lscales[6]["to"],
				color: 'rgba(68, 170, 213, 0.1)',
				label: {
					text: $.t('High wind'),
					style: {
						color: '#CCCCCC'
					}
				}
			}, { // fresh gale
				from: lscales[7]["from"],
				to: lscales[7]["to"],
				color: 'rgba(68, 170, 213, 0.1)',
				label: {
					text: $.t('Fresh gale'),
					style: {
						color: '#CCCCCC'
					}
				}
			}, { // strong gale
				from: lscales[8]["from"],
				to: lscales[8]["to"],
				color: 'rgba(68, 170, 213, 0.2)',
				label: {
					text: $.t('Strong gale'),
					style: {
						color: '#CCCCCC'
					}
				}
			}, { // storm
				from: lscales[9]["from"],
				to: lscales[9]["to"],
				color: 'rgba(68, 170, 213, 0.1)',
				label: {
					text: $.t('Storm'),
					style: {
						color: '#CCCCCC'
					}
				}
			}, { // Violent storm
				from: lscales[10]["from"],
				to: lscales[10]["to"],
				color: 'rgba(68, 170, 213, 0.2)',
				label: {
					text: $.t('Violent storm'),
					style: {
						color: '#CCCCCC'
					}
				}
			}, { // hurricane
				from: lscales[11]["from"],
				to: lscales[11]["to"],
				color: 'rgba(68, 170, 213, 0.1)',
				label: {
					text: $.t('Hurricane'),
					style: {
						color: '#CCCCCC'
					}
				}
			}
			]
		},
		tooltip: {
			crosshairs: true,
			shared: true
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
			id: 'speed',
			name: $.t('Speed'),
			tooltip: {
				valueSuffix: ' ' + $.myglobals.windsign,
				valueDecimals: 1
			},
			point: {
				events: {
					click: function (event) {
						chartPointClickNew(event, false, ShowWindLog);
					}
				}
			}
		}, {
			id: 'gust',
			name: $.t('Gust'),
			tooltip: {
				valueSuffix: ' ' + $.myglobals.windsign,
				valueDecimals: 1
			},
			point: {
				events: {
					click: function (event) {
						chartPointClickNew(event, false, ShowWindLog);
					}
				}
			}
		}, {
			id: 'past_speed',
			name: $.t('Past') + ' ' + $.t('Speed'),
			visible: false,
			tooltip: {
				valueSuffix: ' ' + $.myglobals.windsign,
				valueDecimals: 1
			},
			point: {
				events: {
					click: function (event) {
						chartPointClickNew(event, false, ShowWindLog);
					}
				}
			}
		}, {
			id: 'past_gust',
			name: $.t('Past') + ' ' + $.t('Gust'),
			visible: false,
			color: 'rgba(234,234,240,0.8)',
			tooltip: {
				valueSuffix: ' ' + $.myglobals.windsign,
				valueDecimals: 1
			},
			point: {
				events: {
					click: function (event) {
						chartPointClickNew(event, false, ShowWindLog);
					}
				}
			}
		}],
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
			zoomType: 'x',
			resetZoomButton: {
				position: {
					x: -30,
					y: -36
				}
			},
			marginRight: 10,
			events: {
				load: function () {

					$.getJSON("json.htm?type=graph&sensor=wind&idx=" + id + "&range=year", function (data) {
						if (typeof data.result != 'undefined') {
							var seriessp = $.YearChart.highcharts().series[0];
							var seriesgu = $.YearChart.highcharts().series[1];
							var series_prev_sp = $.YearChart.highcharts().series[2];
							var series_prev_gu = $.YearChart.highcharts().series[3];
							var datatablesp = [];
							var datatablegu = [];
							var datatable_prev_sp = [];
							var datatable_prev_gu = [];

							$.each(data.result, function (i, item) {
								datatablesp.push([GetDateFromString(item.d), parseFloat(item.sp)]);
								datatablegu.push([GetDateFromString(item.d), parseFloat(item.gu)]);
							});
							var bHavePrev = (typeof data.resultprev != 'undefined');
							if (bHavePrev) {
								$.each(data.resultprev, function (i, item) {
									datatable_prev_sp.push([GetPrevDateFromString(item.d), parseFloat(item.sp)]);
									datatable_prev_gu.push([GetPrevDateFromString(item.d), parseFloat(item.gu)]);
								});
							}
							seriessp.setData(datatablesp, false);
							seriesgu.setData(datatablegu, false);
							series_prev_sp.setData(datatable_prev_sp, false);
							series_prev_gu.setData(datatable_prev_gu, false);
							$.YearChart.highcharts().redraw();
						}
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
			text: $.t('Wind') + ' ' + $.t('speed/gust') + ' ' + $.t('Last Year')
		},
		xAxis: {
			type: 'datetime'
		},
		yAxis: {
			title: {
				text: $.t('Speed') + ' (' + $.myglobals.windsign + ')'
			},
			min: 0,
			minorGridLineWidth: 0,
			gridLineWidth: 0,
			alternateGridColor: null,
			plotBands: [{ // Light air
				from: lscales[0]["from"],
				to: lscales[0]["to"],
				color: 'rgba(68, 170, 213, 0.1)',
				label: {
					text: $.t('Light air'),
					style: {
						color: '#CCCCCC'
					}
				}
			}, { // Light breeze
				from: lscales[1]["from"],
				to: lscales[1]["to"],
				color: 'rgba(68, 170, 213, 0.2)',
				label: {
					text: $.t('Light breeze'),
					style: {
						color: '#CCCCCC'
					}
				}
			}, { // Gentle breeze
				from: lscales[2]["from"],
				to: lscales[2]["to"],
				color: 'rgba(68, 170, 213, 0.1)',
				label: {
					text: $.t('Gentle breeze'),
					style: {
						color: '#CCCCCC'
					}
				}
			}, { // Moderate breeze
				from: lscales[3]["from"],
				to: lscales[3]["to"],
				color: 'rgba(68, 170, 213, 0.2)',
				label: {
					text: $.t('Moderate breeze'),
					style: {
						color: '#CCCCCC'
					}
				}
			}, { // Fresh breeze
				from: lscales[4]["from"],
				to: lscales[4]["to"],
				color: 'rgba(68, 170, 213, 0.1)',
				label: {
					text: $.t('Fresh breeze'),
					style: {
						color: '#CCCCCC'
					}
				}
			}, { // Strong breeze
				from: lscales[5]["from"],
				to: lscales[5]["to"],
				color: 'rgba(68, 170, 213, 0.2)',
				label: {
					text: $.t('Strong breeze'),
					style: {
						color: '#CCCCCC'
					}
				}
			}, { // High wind
				from: lscales[6]["from"],
				to: lscales[6]["to"],
				color: 'rgba(68, 170, 213, 0.1)',
				label: {
					text: $.t('High wind'),
					style: {
						color: '#CCCCCC'
					}
				}
			}, { // fresh gale
				from: lscales[7]["from"],
				to: lscales[7]["to"],
				color: 'rgba(68, 170, 213, 0.1)',
				label: {
					text: $.t('Fresh gale'),
					style: {
						color: '#CCCCCC'
					}
				}
			}, { // strong gale
				from: lscales[8]["from"],
				to: lscales[8]["to"],
				color: 'rgba(68, 170, 213, 0.2)',
				label: {
					text: $.t('Strong gale'),
					style: {
						color: '#CCCCCC'
					}
				}
			}, { // storm
				from: lscales[9]["from"],
				to: lscales[9]["to"],
				color: 'rgba(68, 170, 213, 0.1)',
				label: {
					text: $.t('Storm'),
					style: {
						color: '#CCCCCC'
					}
				}
			}, { // Violent storm
				from: lscales[10]["from"],
				to: lscales[10]["to"],
				color: 'rgba(68, 170, 213, 0.2)',
				label: {
					text: $.t('Violent storm'),
					style: {
						color: '#CCCCCC'
					}
				}
			}, { // hurricane
				from: lscales[11]["from"],
				to: lscales[11]["to"],
				color: 'rgba(68, 170, 213, 0.1)',
				label: {
					text: $.t('Hurricane'),
					style: {
						color: '#CCCCCC'
					}
				}
			}
			]
		},
		tooltip: {
			crosshairs: true,
			shared: true
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
			id: 'speed',
			name: $.t('Speed'),
			tooltip: {
				valueSuffix: ' ' + $.myglobals.windsign,
				valueDecimals: 1
			},
			point: {
				events: {
					click: function (event) {
						chartPointClickNew(event, false, ShowWindLog);
					}
				}
			}
		}, {
			id: 'gust',
			name: $.t('Gust'),
			tooltip: {
				valueSuffix: ' ' + $.myglobals.windsign,
				valueDecimals: 1
			},
			point: {
				events: {
					click: function (event) {
						chartPointClickNew(event, false, ShowWindLog);
					}
				}
			}
		}, {
			id: 'past_speed',
			name: $.t('Past') + ' ' + $.t('Speed'),
			visible: false,
			tooltip: {
				valueSuffix: ' ' + $.myglobals.windsign,
				valueDecimals: 1
			},
			point: {
				events: {
					click: function (event) {
						chartPointClickNew(event, false, ShowWindLog);
					}
				}
			}
		}, {
			id: 'past_gust',
			name: $.t('Past') + ' ' + $.t('Gust'),
			visible: false,
			color: 'rgba(234,234,240,0.8)',
			tooltip: {
				valueSuffix: ' ' + $.myglobals.windsign,
				valueDecimals: 1
			},
			point: {
				events: {
					click: function (event) {
						chartPointClickNew(event, false, ShowWindLog);
					}
				}
			}
		}],
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

function ShowMonthReportRain(actMonth, actYear) {
	var htmlcontent = '';

	htmlcontent += $('#toptextmonthrain').html();
	htmlcontent += $('#reportviewrain').html();
	$($.content).html(htmlcontent);
	$($.content).i18n();
	$($.content + ' #ftext').html($.t('Day'));

	$($.content + ' #theader').html(unescape($.devName) + " " + $.t($.monthNames[actMonth - 1]) + " " + actYear);

	$($.content + ' #munit').html("mm");

	$($.content + ' #reporttable').dataTable({
		"sDom": '<"H"rC>t<"F">',
		"oTableTools": {
			"sRowSelect": "single"
		},
		"aaSorting": [[0, "asc"]],
		"aoColumnDefs": [
			{ "bSortable": false, "aTargets": [2] }
		],
		"bSortClasses": false,
		"bProcessing": true,
		"bStateSave": false,
		"bJQueryUI": true,
		"aLengthMenu": [[50, 100, -1], [50, 100, "All"]],
		"iDisplayLength": 50,
		language: $.DataTableLanguage
	});
	var mTable = $($.content + ' #reporttable');
	var oTable = mTable.dataTable();
	oTable.fnClearTable();

	$.UsageChart = $($.content + ' #usagegraph');
	$.UsageChart.highcharts({
		chart: {
			type: 'column'
		},
		credits: {
			enabled: true,
			href: "http://www.domoticz.com",
			text: "Domoticz.com"
		},
		title: {
			text: ''
		},
		xAxis: {
			type: 'datetime'
		},
		yAxis: {
			min: 0,
			maxPadding: 0.2,
			title: {
				text: $.t('Rain')
			},
			min: 0
		},
		tooltip: {
			formatter: function () {
				return $.t(Highcharts.dateFormat('%A', this.x)) + ' ' + Highcharts.dateFormat('%B %d', this.x) + '<br/>' + this.series.name + ': ' + this.y + ' mm';
			}
		},
		plotOptions: {
			column: {
				minPointLength: 4,
				pointPadding: 0.1,
				groupPadding: 0
			}
		},
		legend: {
			enabled: true
		}
	});

	var total = 0;
	var datachart = [];

	var highest_val = -1;
	var highest_pos = 0;
	var highest_date = {};

	$.getJSON("json.htm?type=graph&sensor=rain&idx=" + $.devIdx + "&range=year&actmonth=" + actMonth + "&actyear=" + actYear,
		function (data) {
			var lastTotal = -1;
			$.each(data.result, function (i, item) {
				var month = parseInt(item.d.substring(5, 7), 10);
				var year = parseInt(item.d.substring(0, 4), 10);

				if ((month == actMonth) && (year == actYear)) {
					var day = parseInt(item.d.substring(8, 10), 10);
					var mvalue = parseFloat(item.mm);

					total += mvalue;

					if ((highest_val == -1) || (mvalue > highest_val)) {
						highest_val = mvalue;
						highest_pos = datachart.length;
						highest_date.day = day;
						highest_date.month = month;
						highest_date.year = year;
					}

					var cdate = Date.UTC(actYear, actMonth - 1, day);
					datachart.push({ x: cdate, y: mvalue, color: 'rgba(3,190,252,0.8)' });

					var img;
					if ((lastTotal == -1) || (lastTotal == mvalue)) {
						img = '<img src="images/equal.png"></img>';
					}
					else if (mvalue < lastTotal) {
						img = '<img src="images/down.png"></img>';
					}
					else {
						img = '<img src="images/up.png"></img>';
					}
					lastTotal = mvalue;

					var addId = oTable.fnAddData([
						day,
						mvalue.toFixed(1),
						img
					], false);
				}
			});

			$($.content + ' #tu').html(total.toFixed(1));

			var hlstring = "-";
			if (highest_val != -1) {
				hlstring = highest_date.day + " " + $.t($.monthNames[highest_date.month - 1]) + " " + highest_date.year + ", " + highest_val.toFixed(1) + " mm";
				datachart[highest_pos].color = "#FF0000";
			}
			$($.content + ' #vhigh').html(hlstring);

			$.UsageChart.highcharts().addSeries({
				id: 'rain',
				name: $.t('Rain'),
				showInLegend: false,
				color: 'rgba(3,190,252,0.8)',
				yAxis: 0
			});
			var series = $.UsageChart.highcharts().get('rain');
			series.setData(datachart);

			mTable.fnDraw();
			/* Add a click handler to the rows - this could be used as a callback */
			$($.content + ' #reporttable tbody tr').click(function (e) {
				if ($(this).hasClass('row_selected')) {
					$(this).removeClass('row_selected');
				}
				else {
					oTable.$('tr.row_selected').removeClass('row_selected');
					$(this).addClass('row_selected');
				}
			});
		});

	return false;
}

function Add2YearTableReportRain(oTable, total, lastTotal, lastMonth, actYear) {
	var img;
	if ((lastTotal == -1) || (lastTotal == total)) {
		img = '<img src="images/equal.png"></img>';
	}
	else if (total < lastTotal) {
		img = '<img src="images/down.png"></img>';
	}
	else {
		img = '<img src="images/up.png"></img>';
	}
	var monthtxt = addLeadingZeros(parseInt(lastMonth), 2) + ". " + $.t($.monthNames[lastMonth - 1]) + " ";
	monthtxt += '<img src="images/next.png" onclick="ShowMonthReportRain(' + lastMonth + ',' + actYear + ')">';

	var addId = oTable.fnAddData([
		monthtxt,
		total.toFixed(1),
		img
	], false);
	return total;
}

function ShowYearReportRain(actYear) {
	if (actYear == 0) {
		actYear = $.actYear;
	}
	else {
		$.actYear = actYear;
	}

	$.monthNames = ["January", "February", "March", "April", "May", "June",
		"July", "August", "September", "October", "November", "December"];

	var htmlcontent = '';
	htmlcontent += $('#toptextyearreportrain').html();
	htmlcontent += $('#reportviewrain').html();

	$($.content).html(htmlcontent);
	$($.content + ' #backbutton').click(function (e) {
		eval($.backfunction)();
	});
	$($.content).i18n();
	$($.content + ' #ftext').html($.t('Month'));

	$($.content + ' #theader').html(unescape($.devName) + " " + actYear);

	$($.content + ' #munit').html("mm");

	$($.content + ' #comboyear').val(actYear);

	$($.content + ' #comboyear').change(function () {
		var yearidx = $($.content + ' #comboyear option:selected').val();
		if (typeof yearidx == 'undefined') {
			return;
		}
		ShowYearReportRain(yearidx);
	});
	$($.content + ' #comboyear').keypress(function () {
		$(this).change();
	});

	$($.content + ' #reporttable').dataTable({
		"sDom": '<"H"rC>t<"F">',
		"oTableTools": {
			"sRowSelect": "single"
		},
		"aaSorting": [[0, "asc"]],
		"aoColumnDefs": [
			{ "bSortable": false, "aTargets": [2] }
		],
		"bSortClasses": false,
		"bProcessing": true,
		"bStateSave": false,
		"bJQueryUI": true,
		"aLengthMenu": [[50, 100, -1], [50, 100, "All"]],
		"iDisplayLength": 50,
		language: $.DataTableLanguage
	});

	var mTable = $($.content + ' #reporttable');
	var oTable = mTable.dataTable();
	oTable.fnClearTable();

	$.UsageChart = $($.content + ' #usagegraph');
	$.UsageChart.highcharts({
		chart: {
			type: 'column'
		},
		credits: {
			enabled: true,
			href: "http://www.domoticz.com",
			text: "Domoticz.com"
		},
		title: {
			text: ''
		},
		xAxis: {
			type: 'datetime'
		},
		yAxis: {
			min: 0,
			maxPadding: 0.2,
			title: {
				text: $.t('Rain')
			},
			min: 0
		},
		tooltip: {
			formatter: function () {
				return $.t(Highcharts.dateFormat('%B', this.x)) + '<br/>' + this.series.name + ': ' + this.y + ' mm';
			}
		},
		plotOptions: {
			column: {
				minPointLength: 4,
				pointPadding: 0.1,
				groupPadding: 0,
				dataLabels: {
					enabled: true,
					color: 'white'
				}
			}
		},
		legend: {
			enabled: true
		}
	});

	var total = 0;
	var global = 0;
	var datachart = [];

	var highest_month_val = -1;
	var highest_month = 0;
	var highest_month_pos = 0;
	var lowest_month_val = -1;
	var lowest_month = 0;
	var highest_val = -1;
	var highest_date = {};

	$.getJSON("json.htm?type=graph&sensor=rain&idx=" + $.devIdx + "&range=year&actyear=" + actYear,
		function (data) {
			var lastTotal = -1;
			var lastMonth = -1;
			$.each(data.result, function (i, item) {
				var month = parseInt(item.d.substring(5, 7), 10);
				var year = parseInt(item.d.substring(0, 4), 10);
				var day = parseInt(item.d.substring(8, 10), 10);
				if (year == actYear) {
					if (lastMonth == -1) {
						lastMonth = month;
					}
					if (lastMonth != month) {
						//add totals to table
						lastTotal = Add2YearTableReportRain(oTable, total, lastTotal, lastMonth, actYear);

						if ((highest_month_val == -1) || (total > highest_month_val)) {
							highest_month_val = total;
							highest_month_pos = datachart.length;
							highest_month = lastMonth;
						}
						if ((lowest_month_val == -1) || (total < lowest_month_val)) {
							lowest_month_val = total;
							lowest_month = lastMonth;
						}

						var cdate = Date.UTC(actYear, lastMonth - 1, 1);
						datachart.push({ x: cdate, y: parseFloat(total.toFixed(1)), color: 'rgba(3,190,252,0.8)' });

						lastMonth = month;
						global += total;
						total = 0;
					}
					var mvalue = parseFloat(item.mm);
					total += mvalue;

					if ((highest_val == -1) || (mvalue > highest_val)) {
						highest_val = mvalue;
						highest_date.day = day;
						highest_date.month = month;
						highest_date.year = year;
					}
				}
			});

			//add last month
			if (total != 0) {
				lastTotal = Add2YearTableReportRain(oTable, total, lastTotal, lastMonth, actYear);

				if ((highest_month_val == -1) || (lastTotal > highest_month_val)) {
					highest_month_val = lastTotal;
					highest_month_pos = datachart.length;
					highest_month = lastMonth;
				}
				if ((lowest_month_val == -1) || (lastTotal < lowest_month_val)) {
					lowest_month_val = lastTotal;
					lowest_month = lastMonth;
				}

				var cdate = Date.UTC(actYear, lastMonth - 1, 1);
				datachart.push({ x: cdate, y: parseFloat(total.toFixed(1)), color: 'rgba(3,190,252,0.8)' });
				global += lastTotal;
			}

			$($.content + ' #tu').html(global.toFixed(1));

			var hlstring = "-";
			if (highest_month_val != -1) {
				hlstring = $.t($.monthNames[highest_month - 1]) + ", " + highest_month_val.toFixed(1) + " mm";
				datachart[highest_month_pos].color = "#FF0000";
			}
			$($.content + ' #mhigh').html(hlstring);
			hlstring = "-";
			if (lowest_month_val != -1) {
				hlstring = $.t($.monthNames[lowest_month - 1]) + ", " + lowest_month_val.toFixed(1) + " mm";
			}
			$($.content + ' #mlow').html(hlstring);
			hlstring = "-";
			if (highest_val != -1) {
				hlstring = highest_date.day + " " + $.t($.monthNames[highest_date.month - 1]) + " " + highest_date.year + ", " + highest_val.toFixed(1) + " mm";
			}
			$($.content + ' #vhigh').html(hlstring);

			$.UsageChart.highcharts().addSeries({
				id: 'rain',
				name: $.t('Rain'),
				showInLegend: false,
				color: 'rgba(3,190,252,0.8)',
				yAxis: 0
			});
			var series = $.UsageChart.highcharts().get('rain');
			series.setData(datachart);

			mTable.fnDraw();
			/* Add a click handler to the rows - this could be used as a callback */
			$($.content + ' #reporttable tbody tr').click(function (e) {
				if ($(this).hasClass('row_selected')) {
					$(this).removeClass('row_selected');
				}
				else {
					oTable.$('tr.row_selected').removeClass('row_selected');
					$(this).addClass('row_selected');
				}
			});
		});

	return false;
}

function ShowRainLog(contentdiv, backfunction, id, name) {
	clearInterval($.myglobals.refreshTimer);
	$(window).scrollTop(0);
	$('#modal').show();
	$.content = contentdiv;
	$.backfunction = backfunction;
	$.devIdx = id;
	$.devName = name;
	var htmlcontent = '';
	htmlcontent = '<p><center><h2>' + unescape(name) + '</h2></center></p>\n';
	htmlcontent += $('#dayweekmonthyearlog').html();

	var d = new Date();
	var actMonth = d.getMonth() + 1;
	var actYear = d.getYear() + 1900;
	$($.content).html(GetBackbuttonHTMLTableWithRight(backfunction, 'ShowYearReportRain(' + actYear + ')', $.t('Report')) + htmlcontent);
	$($.content).i18n();

	$.DayChart = $($.content + ' #daygraph');
	$.DayChart.highcharts({
		chart: {
			type: 'column',
			events: {
				load: function () {
					$.getJSON("json.htm?type=graph&sensor=rain&idx=" + id + "&range=day", function (data) {
						if (typeof data.result != 'undefined') {
							var series = $.DayChart.highcharts().series[0];
							var datatable = [];

							$.each(data.result, function (i, item) {
								datatable.push([GetUTCFromString(item.d), parseFloat(item.mm)]);
							});
							series.setData(datatable); // redraws
						}
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
			text: $.t('Rainfall') + ' ' + Get5MinuteHistoryDaysGraphTitle()
		},
		xAxis: {
			type: 'datetime'
		},
		yAxis: {
			min: 0,
			maxPadding: 0.2,
			title: {
				text: $.t('Rainfall') + ' (mm)'
			}
		},
		tooltip: {
			formatter: function () {
				return '' +
					$.t(Highcharts.dateFormat('%A', this.x)) + '<br/>' + Highcharts.dateFormat('%Y-%m-%d %H:%M', this.x) + ': ' + this.y + ' mm';
			}
		},
		plotOptions: {
			column: {
				minPointLength: 3,
				pointPadding: 0.1,
				groupPadding: 0,
				dataLabels: {
					enabled: false,
					color: 'white'
				}
			}
		},
		series: [{
			id: 'mm',
			name: 'mm',
			color: 'rgba(3,190,252,0.8)'
		}]
		,
		legend: {
			enabled: true
		}
	});

	$.WeekChart = $($.content + ' #weekgraph');
	$.WeekChart.highcharts({
		chart: {
			type: 'column',
			events: {
				load: function () {
					$.getJSON("json.htm?type=graph&sensor=rain&idx=" + id + "&range=week", function (data) {
						if (typeof data.result != 'undefined') {
							var series = $.WeekChart.highcharts().series[0];
							var datatable = [];
							$.each(data.result, function (i, item) {
								datatable.push([GetDateFromString(item.d), parseFloat(item.mm)]);
							});
							series.setData(datatable); // redraws
						}
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
			text: $.t('Rainfall') + ' ' + $.t('Last Week')
		},
		xAxis: {
			type: 'datetime',
			dateTimeLabelFormats: {
				day: '%a'
			},
			tickInterval: 24 * 3600 * 1000
		},
		yAxis: {
			min: 0,
			maxPadding: 0.2,
			title: {
				text: $.t('Rainfall') + ' (mm)'
			}
		},
		tooltip: {
			formatter: function () {
				return '' +
					$.t(Highcharts.dateFormat('%A', this.x)) + '<br/>' + Highcharts.dateFormat('%Y-%m-%d', this.x) + ': ' + this.y + ' mm';
			}
		},
		plotOptions: {
			column: {
				minPointLength: 4,
				pointPadding: 0.1,
				groupPadding: 0,
				dataLabels: {
					enabled: true,
					color: 'white'
				}
			}
		},
		series: [{
			id: 'mm',
			name: 'mm',
			color: 'rgba(3,190,252,0.8)'
		}]
		,
		legend: {
			enabled: true
		}
	});

	$.MonthChart = $($.content + ' #monthgraph');
	$.MonthChart.highcharts({
		chart: {
			type: 'spline',
			marginRight: 10,
			zoomType: 'x',
			resetZoomButton: {
				position: {
					x: -30,
					y: -36
				}
			},
			events: {
				load: function () {
					$.getJSON("json.htm?type=graph&sensor=rain&idx=" + id + "&range=month", function (data) {
						if (typeof data.result != 'undefined') {
							var series = $.MonthChart.highcharts().series[0];
							var datatable = [];
							$.each(data.result, function (i, item) {
								datatable.push([GetDateFromString(item.d), parseFloat(item.mm)]);
							});
							series.setData(datatable, false);
							var bHavePrev = (typeof data.resultprev != 'undefined');
							if (bHavePrev) {
								datatable = [];
								series = $.MonthChart.highcharts().series[1];
								$.each(data.resultprev, function (i, item) {
									datatable.push([GetPrevDateFromString(item.d), parseFloat(item.mm)]);
								});
								series.setData(datatable, false);
							}
							$.MonthChart.highcharts().redraw();
						}
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
			text: $.t('Rainfall') + ' ' + $.t('Last Month')
		},
		xAxis: {
			type: 'datetime'
		},
		yAxis: {
			title: {
				text: $.t('Rainfall') + ' (mm)'
			},
			min: 0
		},
		tooltip: {
			crosshairs: true,
			shared: true
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
			id: 'mm',
			name: 'mm',
			color: 'rgba(3,190,252,0.8)',
			tooltip: {
				valueSuffix: ' mm',
				valueDecimals: 1
			},
			point: {
				events: {
					click: function (event) {
						chartPointClickNew(event, false, ShowRainLog);
					}
				}
			}
		}, {
			id: 'past_mm',
			name: $.t('Past') + ' mm',
			visible: false,
			color: 'rgba(190,3,252,0.8)',
			tooltip: {
				valueSuffix: ' mm',
				valueDecimals: 1
			},
			point: {
				events: {
					click: function (event) {
						chartPointClickNew(event, false, ShowRainLog);
					}
				}
			}
		}]
		,
		legend: {
			enabled: true
		}
	});

	$.YearChart = $($.content + ' #yeargraph');
	$.YearChart.highcharts({
		chart: {
			type: 'spline',
			marginRight: 10,
			zoomType: 'x',
			resetZoomButton: {
				position: {
					x: -30,
					y: -36
				}
			},
			events: {
				load: function () {

					$.getJSON("json.htm?type=graph&sensor=rain&idx=" + id + "&range=year", function (data) {
						if (typeof data.result != 'undefined') {
							var series = $.YearChart.highcharts().series[0];
							var datatable = [];
							$.each(data.result, function (i, item) {
								datatable.push([GetDateFromString(item.d), parseFloat(item.mm)]);
							});
							series.setData(datatable, false);
							var bHavePrev = (typeof data.resultprev != 'undefined');
							if (bHavePrev) {
								datatable = [];
								series = $.YearChart.highcharts().series[1];
								$.each(data.resultprev, function (i, item) {
									datatable.push([GetPrevDateFromString(item.d), parseFloat(item.mm)]);
								});
								series.setData(datatable, false);
							}
							$.YearChart.highcharts().redraw();
						}
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
			text: $.t('Rainfall') + ' ' + $.t('Last Year')
		},
		xAxis: {
			type: 'datetime'
		},
		yAxis: {
			title: {
				text: $.t('Rainfall') + ' (mm)'
			},
			min: 0
		},
		tooltip: {
			crosshairs: true,
			shared: true
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
			id: 'mm',
			name: 'mm',
			color: 'rgba(3,190,252,0.8)',
			tooltip: {
				valueSuffix: ' mm',
				valueDecimals: 1
			},
			point: {
				events: {
					click: function (event) {
						chartPointClickNew(event, false, ShowRainLog);
					}
				}
			}
		}, {
			id: 'past_mm',
			name: $.t('Past') + ' mm',
			visible: false,
			color: 'rgba(190,3,252,0.8)',
			tooltip: {
				valueSuffix: ' mm',
				valueDecimals: 1
			},
			point: {
				events: {
					click: function (event) {
						chartPointClickNew(event, false, ShowRainLog);
					}
				}
			}
		}]
		,
		legend: {
			enabled: true
		}
	});
	$('#modal').hide();
	cursordefault();
	return false;
}

function ShowBaroLog(contentdiv, backfunction, id, name) {
	clearInterval($.myglobals.refreshTimer);
	$(window).scrollTop(0);
	$('#modal').show();
	$.content = contentdiv;
	$.backfunction = backfunction;
	$.devIdx = id;
	$.devName = name;
	var htmlcontent = '';
	htmlcontent = '<p><center><h2>' + unescape(name) + '</h2></center></p>\n';
	htmlcontent += $('#daymonthyearlog').html();
	$($.content).html(GetBackbuttonHTMLTable(backfunction) + htmlcontent);
	$($.content).i18n();

	$.DayChart = $($.content + ' #daygraph');
	$.DayChart.highcharts({
		chart: {
			type: 'spline',
			zoomType: 'x',
			resetZoomButton: {
				position: {
					x: -30,
					y: -36
				}
			},
			marginRight: 10,
			events: {
				load: function () {
					$.getJSON("json.htm?type=graph&sensor=temp&idx=" + id + "&range=day", function (data) {
						if (typeof data.result != 'undefined') {
							var series = $.DayChart.highcharts().series[0];
							var datatable = [];
							$.each(data.result, function (i, item) {
								datatable.push([GetUTCFromString(item.d), parseFloat(item.ba)]);
							});
							series.setData(datatable); // redraws
						}
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
			text: $.t('Barometer') + ' ' + Get5MinuteHistoryDaysGraphTitle()
		},
		xAxis: {
			type: 'datetime'
		},
		yAxis: {
			title: {
				text: $.t('Pressure') + ' (hPa)'
			},
			labels: {
				formatter: function () {
					return this.value;
				}
			}
		},
		tooltip: {
			crosshairs: true,
			shared: true
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
			id: 'baro',
			name: 'Baro',
			tooltip: {
				valueSuffix: ' hPa',
				valueDecimals: 1
			},
			point: {
				events: {
					click: function (event) {
						chartPointClickNew(event, true, ShowBaroLog);
					}
				}
			}
		}]
		,
		legend: {
			enabled: true
		},
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
			zoomType: 'x',
			resetZoomButton: {
				position: {
					x: -30,
					y: -36
				}
			},
			marginRight: 10,
			events: {
				load: function () {
					$.getJSON("json.htm?type=graph&sensor=temp&idx=" + id + "&range=month", function (data) {
						if (typeof data.result != 'undefined') {
							var series = $.MonthChart.highcharts().series[0];
							var datatable = [];
							$.each(data.result, function (i, item) {
								datatable.push([GetDateFromString(item.d), parseFloat(item.ba)]);
							});
							series.setData(datatable, false);
							var bHavePrev = (typeof data.resultprev != 'undefined');
							if (bHavePrev) {
								datatable = [];
								series = $.MonthChart.highcharts().series[1];
								$.each(data.resultprev, function (i, item) {
									datatable.push([GetPrevDateFromString(item.d), parseFloat(item.ba)]);
								});
								$.MonthChart.highcharts().addSeries({
									id: 'prev_baro',
									name: $.t('Past') + ' Baro',
									visible: false,
									tooltip: {
										valueSuffix: ' hPA',
										valueDecimals: 1
									},
									point: {
										events: {
											click: function (event) {
												chartPointClickNew(event, false, ShowBaroLog);
											}
										}
									}
								}, false);
								series = $.MonthChart.highcharts().get('prev_baro');
								series.setData(datatable, false);
							}
							$.MonthChart.highcharts().redraw();
						}
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
			text: $.t('Barometer') + ' ' + $.t('Last Month')
		},
		xAxis: {
			type: 'datetime'
		},
		yAxis: {
			title: {
				text: $.t('Pressure') + ' (hPa)'
			},
			labels: {
				formatter: function () {
					return this.value;
				}
			}
		},
		tooltip: {
			crosshairs: true,
			shared: true
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
			id: 'baro',
			name: 'Baro',
			tooltip: {
				valueSuffix: ' hPa',
				valueDecimals: 1
			},
			point: {
				events: {
					click: function (event) {
						chartPointClickNew(event, false, ShowBaroLog);
					}
				}
			}
		}]
		,
		legend: {
			enabled: true
		},
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
			zoomType: 'x',
			resetZoomButton: {
				position: {
					x: -30,
					y: -36
				}
			},
			marginRight: 10,
			events: {
				load: function () {
					$.getJSON("json.htm?type=graph&sensor=temp&idx=" + id + "&range=year", function (data) {
						if (typeof data.result != 'undefined') {
							var series = $.YearChart.highcharts().series[0];
							var datatable = [];

							$.each(data.result, function (i, item) {
								datatable.push([GetDateFromString(item.d), parseFloat(item.ba)]);
							});
							series.setData(datatable, false);
							var bHavePrev = (typeof data.resultprev != 'undefined');
							if (bHavePrev) {
								datatable = [];
								$.each(data.resultprev, function (i, item) {
									datatable.push([GetPrevDateFromString(item.d), parseFloat(item.ba)]);
								});
								$.YearChart.highcharts().addSeries({
									id: 'prev_baro',
									name: $.t('Past') + ' Baro',
									visible: false,
									tooltip: {
										valueSuffix: ' hPA',
										valueDecimals: 1
									},
									point: {
										events: {
											click: function (event) {
												chartPointClickNew(event, false, ShowBaroLog);
											}
										}
									}
								}, false);
								series = $.YearChart.highcharts().get('prev_baro');
								series.setData(datatable, false);
							}
							$.YearChart.highcharts().redraw();
						}
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
			text: $.t('Barometer') + ' ' + $.t('Last Year')
		},
		xAxis: {
			type: 'datetime'
		},
		yAxis: {
			title: {
				text: $.t('Pressure') + ' (hPa)'
			},
			labels: {
				formatter: function () {
					return this.value;
				}
			}
		},
		tooltip: {
			crosshairs: true,
			shared: true
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
			id: 'baro',
			name: 'Baro',
			tooltip: {
				valueSuffix: ' hPa',
				valueDecimals: 1
			},
			point: {
				events: {
					click: function (event) {
						chartPointClickNew(event, false, ShowBaroLog);
					}
				}
			}
		}]
		,
		legend: {
			enabled: true
		},
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

function ShowAirQualityLog(contentdiv, backfunction, id, name) {
	clearInterval($.myglobals.refreshTimer);
	$(window).scrollTop(0);
	$('#modal').show();
	$.content = contentdiv;
	$.backfunction = backfunction;
	$.devIdx = id;
	$.devName = name;
	var htmlcontent = '';
	htmlcontent = '<p><center><h2>' + unescape(name) + '</h2></center></p>\n';
	htmlcontent += $('#daymonthyearlog').html();
	$($.content).html(GetBackbuttonHTMLTable(backfunction) + htmlcontent);
	$($.content).i18n();

	$.DayChart = $($.content + ' #daygraph');
	$.DayChart.highcharts({
		chart: {
			type: 'spline',
			zoomType: 'x',
			resetZoomButton: {
				position: {
					x: -30,
					y: -36
				}
			},
			marginRight: 10,
			events: {
				load: function () {
					$.getJSON("json.htm?type=graph&sensor=counter&idx=" + id + "&range=day", function (data) {
						if (typeof data.result != 'undefined') {
							var series = $.DayChart.highcharts().series[0];
							var datatable = [];

							$.each(data.result, function (i, item) {
								datatable.push([GetUTCFromString(item.d), parseInt(item.co2)]);
							});
							series.setData(datatable); // redraws
						}
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
			text: $.t('Air Quality') + ' ' + Get5MinuteHistoryDaysGraphTitle()
		},
		xAxis: {
			type: 'datetime'
		},
		yAxis: {
			title: {
				text: 'co2 (ppm)'
			},
			minorGridLineWidth: 0,
			gridLineWidth: 0,
			alternateGridColor: null,
			plotBands: [{ // Excellent
				from: 0,
				to: 700,
				color: 'rgba(68, 170, 213, 0.1)',
				label: {
					text: $.t('Excellent'),
					style: {
						color: '#CCCCCC'
					}
				}
			}, { // Good
				from: 700,
				to: 900,
				color: 'rgba(68, 170, 213, 0.2)',
				label: {
					text: $.t('Good'),
					style: {
						color: '#CCCCCC'
					}
				}
			}, { // Fair
				from: 900,
				to: 1100,
				color: 'rgba(68, 170, 213, 0.1)',
				label: {
					text: $.t('Fair'),
					style: {
						color: '#CCCCCC'
					}
				}
			}, { // Mediocre
				from: 1100,
				to: 1600,
				color: 'rgba(68, 170, 213, 0.2)',
				label: {
					text: $.t('Mediocre'),
					style: {
						color: '#CCCCCC'
					}
				}
			}, { // Bad
				from: 1600,
				to: 6000,
				color: 'rgba(68, 170, 213, 0.1)',
				label: {
					text: $.t('Bad'),
					style: {
						color: '#CCCCCC'
					}
				}
			}
			]
		},
		tooltip: {
			crosshairs: true,
			shared: true
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
			id: 'co2',
			name: 'co2',
			showInLegend: false,
			tooltip: {
				valueSuffix: ' ppm',
				valueDecimals: 0
			},
			point: {
				events: {
					click: function (event) {
						chartPointClickNew(event, true, ShowAirQualityLog);
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

	$.MonthChart = $($.content + ' #monthgraph');
	$.MonthChart.highcharts({
		chart: {
			type: 'spline',
			zoomType: 'x',
			resetZoomButton: {
				position: {
					x: -30,
					y: -36
				}
			},
			marginRight: 10,
			events: {
				load: function () {
					$.getJSON("json.htm?type=graph&sensor=counter&idx=" + id + "&range=month", function (data) {
						if (typeof data.result != 'undefined') {
							var datatable1 = [];
							var datatable2 = [];
							var datatable3 = [];
							var datatable_prev = [];

							$.each(data.result, function (i, item) {
								datatable1.push([GetDateFromString(item.d), parseInt(item.co2_min)]);
								datatable2.push([GetDateFromString(item.d), parseInt(item.co2_max)]);
								var avg_val = parseInt(item.co2_avg);
								if (avg_val != 0) {
									datatable3.push([GetDateFromString(item.d), avg_val]);
								}
							});
							$.each(data.resultprev, function (i, item) {
								var cdate = GetPrevDateFromString(item.d);
								datatable_prev.push([cdate, parseInt(item.co2_max)]);
							});

							var series1 = $.MonthChart.highcharts().series[0];
							var series2 = $.MonthChart.highcharts().series[1];
							series1.setData(datatable1, false);
							series2.setData(datatable2, false);
							if (datatable3.length > 0) {
								var series3 = $.MonthChart.highcharts().series[2];
								series3.setData(datatable3);
							}
							else {
								$.MonthChart.highcharts().series[2].remove();
							}
							if (datatable_prev.length != 0) {
								$.MonthChart.highcharts().addSeries({
									id: 'prev_co2',
									name: $.t('Past') + ' co2',
									color: 'rgba(223,212,246,0.8)',
									zIndex: 3,
									yAxis: 0,
									tooltip: {
										valueSuffix: ' ppm',
										valueDecimals: 0
									},
									visible: false
								}, false);
								series = $.MonthChart.highcharts().get('prev_co2');
								series.setData(datatable_prev, false);
							}
							$.MonthChart.highcharts().redraw();
						}
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
			text: $.t('Air Quality') + ' ' + $.t('Last Month')
		},
		xAxis: {
			type: 'datetime'
		},
		yAxis: {
			title: {
				text: 'co2 (ppm)'
			},
			minorGridLineWidth: 0,
			gridLineWidth: 0,
			alternateGridColor: null,
			plotBands: [{ // Excellent
				from: 0,
				to: 700,
				color: 'rgba(68, 170, 213, 0.1)',
				label: {
					text: $.t('Excellent'),
					style: {
						color: '#CCCCCC'
					}
				}
			}, { // Good
				from: 700,
				to: 900,
				color: 'rgba(68, 170, 213, 0.2)',
				label: {
					text: $.t('Good'),
					style: {
						color: '#CCCCCC'
					}
				}
			}, { // Fair
				from: 900,
				to: 1100,
				color: 'rgba(68, 170, 213, 0.1)',
				label: {
					text: $.t('Fair'),
					style: {
						color: '#CCCCCC'
					}
				}
			}, { // Mediocre
				from: 1100,
				to: 1600,
				color: 'rgba(68, 170, 213, 0.2)',
				label: {
					text: $.t('Mediocre'),
					style: {
						color: '#CCCCCC'
					}
				}
			}, { // Bad
				from: 1600,
				to: 6000,
				color: 'rgba(68, 170, 213, 0.1)',
				label: {
					text: $.t('Bad'),
					style: {
						color: '#CCCCCC'
					}
				}
			}
			]
		},
		tooltip: {
			crosshairs: true,
			shared: true
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
			id: 'co2_min',
			name: 'min',
			tooltip: {
				valueSuffix: ' ppm',
				valueDecimals: 0
			},
			point: {
				events: {
					click: function (event) {
						chartPointClickNew(event, false, ShowAirQualityLog);
					}
				}
			}
		}, {
			id: 'co2_max',
			name: 'max',
			tooltip: {
				valueSuffix: ' ppm',
				valueDecimals: 0
			},
			point: {
				events: {
					click: function (event) {
						chartPointClickNew(event, false, ShowAirQualityLog);
					}
				}
			}
		}, {
			id: 'co2_avg',
			name: 'avg',
			tooltip: {
				valueSuffix: ' ppm',
				valueDecimals: 0
			},
			point: {
				events: {
					click: function (event) {
						chartPointClickNewGeneral(event, false, ShowAirQualityLog);
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
			zoomType: 'x',
			resetZoomButton: {
				position: {
					x: -30,
					y: -36
				}
			},
			marginRight: 10,
			events: {
				load: function () {
					$.getJSON("json.htm?type=graph&sensor=counter&idx=" + id + "&range=year", function (data) {
						if (typeof data.result != 'undefined') {
							var datatable1 = [];
							var datatable2 = [];
							var datatable3 = [];
							var datatable_prev = [];

							$.each(data.result, function (i, item) {
								datatable1.push([GetDateFromString(item.d), parseInt(item.co2_min)]);
								datatable2.push([GetDateFromString(item.d), parseInt(item.co2_max)]);
								var avg_val = parseInt(item.co2_avg);
								if (avg_val != 0) {
									datatable3.push([GetDateFromString(item.d), avg_val]);
								}
							});

							$.each(data.resultprev, function (i, item) {
								var cdate = GetPrevDateFromString(item.d);
								datatable_prev.push([cdate, parseInt(item.co2_max)]);
							});

							var series1 = $.YearChart.highcharts().series[0];
							var series2 = $.YearChart.highcharts().series[1];
							series1.setData(datatable1, false);
							series2.setData(datatable2, false);
							if (datatable3.length > 0) {
								var series3 = $.YearChart.highcharts().series[2];
								series3.setData(datatable3);
							}
							else {
								$.YearChart.highcharts().series[2].remove();
							}
							if (datatable_prev.length != 0) {
								$.YearChart.highcharts().addSeries({
									id: 'prev_co2',
									name: $.t('Past') + ' co2',
									color: 'rgba(223,212,246,0.8)',
									zIndex: 3,
									yAxis: 0,
									tooltip: {
										valueSuffix: ' ppm',
										valueDecimals: 0
									},
									visible: false
								}, false);
								series = $.YearChart.highcharts().get('prev_co2');
								series.setData(datatable_prev, false);
							}
							$.YearChart.highcharts().redraw();
						}
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
			text: $.t('Air Quality') + ' ' + $.t('Last Year')
		},
		xAxis: {
			type: 'datetime'
		},
		yAxis: {
			title: {
				text: 'co2 (ppm)'
			},
			minorGridLineWidth: 0,
			gridLineWidth: 0,
			alternateGridColor: null,
			plotBands: [{ // Excellent
				from: 0,
				to: 700,
				color: 'rgba(68, 170, 213, 0.1)',
				label: {
					text: $.t('Excellent'),
					style: {
						color: '#CCCCCC'
					}
				}
			}, { // Good
				from: 700,
				to: 900,
				color: 'rgba(68, 170, 213, 0.2)',
				label: {
					text: $.t('Good'),
					style: {
						color: '#CCCCCC'
					}
				}
			}, { // Fair
				from: 900,
				to: 1100,
				color: 'rgba(68, 170, 213, 0.1)',
				label: {
					text: $.t('Fair'),
					style: {
						color: '#CCCCCC'
					}
				}
			}, { // Mediocre
				from: 1100,
				to: 1600,
				color: 'rgba(68, 170, 213, 0.2)',
				label: {
					text: $.t('Mediocre'),
					style: {
						color: '#CCCCCC'
					}
				}
			}, { // Bad
				from: 1600,
				to: 6000,
				color: 'rgba(68, 170, 213, 0.1)',
				label: {
					text: $.t('Bad'),
					style: {
						color: '#CCCCCC'
					}
				}
			}
			]
		},
		tooltip: {
			crosshairs: true,
			shared: true
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
			id: 'co2_min',
			name: 'min',
			tooltip: {
				valueSuffix: ' ppm',
				valueDecimals: 0
			},
			point: {
				events: {
					click: function (event) {
						chartPointClickNew(event, false, ShowAirQualityLog);
					}
				}
			}
		}, {
			id: 'co2_max',
			name: 'max',
			tooltip: {
				valueSuffix: ' ppm',
				valueDecimals: 0
			},
			point: {
				events: {
					click: function (event) {
						chartPointClickNew(event, false, ShowAirQualityLog);
					}
				}
			}
		}, {
			id: 'co2_avg',
			name: 'avg',
			tooltip: {
				valueSuffix: ' ppm',
				valueDecimals: 0
			},
			point: {
				events: {
					click: function (event) {
						chartPointClickNewGeneral(event, false, ShowAirQualityLog);
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


function ShowFanLog(contentdiv, backfunction, id, name, sensor) {
	clearInterval($.myglobals.refreshTimer);
	$(window).scrollTop(0);
	$('#modal').show();
	$.content = contentdiv;
	$.backfunction = backfunction;
	$.devIdx = id;
	$.devName = name;
	var htmlcontent = '';
	htmlcontent = '<p><center><h2>' + unescape(name) + '</h2></center></p>\n';
	htmlcontent += $('#daymonthyearlog').html();
	$($.content).html(GetBackbuttonHTMLTable(backfunction) + htmlcontent);
	$($.content).i18n();

	$.DayChart = $($.content + ' #daygraph');
	$.DayChart.highcharts({
		chart: {
			type: 'spline',
			zoomType: 'x',
			resetZoomButton: {
				position: {
					x: -30,
					y: -36
				}
			},
			marginRight: 10,
			events: {
				load: function () {
					$.getJSON("json.htm?type=graph&sensor=fan&idx=" + id + "&range=day", function (data) {
						if (typeof data.result != 'undefined') {
							var series = $.DayChart.highcharts().series[0];
							var datatable = [];

							$.each(data.result, function (i, item) {
								datatable.push([GetUTCFromString(item.d), parseInt(item.v)]);
							});
							series.setData(datatable); // redraws
						}
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
			text: $.t('RPM') + ' ' + Get5MinuteHistoryDaysGraphTitle()
		},
		xAxis: {
			type: 'datetime'
		},
		yAxis: {
			title: {
				text: 'RPM'
			},
			min: 0,
			minorGridLineWidth: 0,
			alternateGridColor: null
		},
		tooltip: {
			crosshairs: true,
			shared: true
		},
		plotOptions: {
			series: {
				point: {
					events: {
						click: function (event) {
							chartPointClickNew(event, true, ShowFanLog);
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
		},
		series: [{
			id: 'fan',
			name: sensor,
			tooltip: {
				valueSuffix: ' rpm',
				valueDecimals: 0
			},
		}]
		,
		navigation: {
			menuItemStyle: {
				fontSize: '10px'
			}
		},
		legend: {
			enabled: true
		}
	});

	$.MonthChart = $($.content + ' #monthgraph');
	$.MonthChart.highcharts({
		chart: {
			type: 'spline',
			zoomType: 'x',
			resetZoomButton: {
				position: {
					x: -30,
					y: -36
				}
			},
			marginRight: 10,
			events: {
				load: function () {
					$.getJSON("json.htm?type=graph&sensor=fan&idx=" + id + "&range=month", function (data) {
						if (typeof data.result != 'undefined') {
							var datatable1 = [];
							var datatable2 = [];

							$.each(data.result, function (i, item) {
								datatable1.push([GetDateFromString(item.d), parseInt(item.v_min)]);
								datatable2.push([GetDateFromString(item.d), parseInt(item.v_max)]);
							});
							var series1 = $.MonthChart.highcharts().series[0];
							var series2 = $.MonthChart.highcharts().series[1];
							series1.setData(datatable1, false);
							series2.setData(datatable2, false);
							$.MonthChart.highcharts().redraw();
						}
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
			text: $.t('RPM') + ' ' + $.t('Last Month')
		},
		xAxis: {
			type: 'datetime'
		},
		yAxis: {
			title: {
				text: 'RPM'
			},
			min: 0,
			minorGridLineWidth: 0,
			alternateGridColor: null
		},
		tooltip: {
			crosshairs: true,
			shared: true
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
			id: 'fan_min',
			name: 'min',
			tooltip: {
				valueSuffix: ' rpm',
				valueDecimals: 0
			},
			point: {
				events: {
					click: function (event) {
						chartPointClickNew(event, false, ShowFanLog);
					}
				}
			}
		}, {
			id: 'fan_max',
			name: 'max',
			tooltip: {
				valueSuffix: ' rpm',
				valueDecimals: 0
			},
			point: {
				events: {
					click: function (event) {
						chartPointClickNew(event, false, ShowFanLog);
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
			zoomType: 'x',
			resetZoomButton: {
				position: {
					x: -30,
					y: -36
				}
			},
			marginRight: 10,
			events: {
				load: function () {
					$.getJSON("json.htm?type=graph&sensor=fan&idx=" + id + "&range=year", function (data) {
						if (typeof data.result != 'undefined') {
							var datatable1 = [];
							var datatable2 = [];

							$.each(data.result, function (i, item) {
								datatable1.push([GetDateFromString(item.d), parseInt(item.v_min)]);
								datatable2.push([GetDateFromString(item.d), parseInt(item.v_max)]);
							});
							var series1 = $.YearChart.highcharts().series[0];
							var series2 = $.YearChart.highcharts().series[1];
							series1.setData(datatable1, false);
							series2.setData(datatable2, false);
							$.YearChart.highcharts().redraw();
						}
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
			text: $.t('RPM') + ' ' + $.t('Last Year')
		},
		xAxis: {
			type: 'datetime'
		},
		yAxis: {
			title: {
				text: 'RPM'
			},
			min: 0,
			minorGridLineWidth: 0,
			alternateGridColor: null
		},
		tooltip: {
			crosshairs: true,
			shared: true
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
			id: 'fan_min',
			name: 'min',
			tooltip: {
				valueSuffix: ' rpm',
				valueDecimals: 0
			},
			point: {
				events: {
					click: function (event) {
						chartPointClickNew(event, false, ShowFanLog);
					}
				}
			}
		}, {
			id: 'fan_max',
			name: 'max',
			tooltip: {
				valueSuffix: ' rpm',
				valueDecimals: 0
			},
			point: {
				events: {
					click: function (event) {
						chartPointClickNew(event, false, ShowFanLog);
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

function ShowPercentageLog(contentdiv, backfunction, id, name) {
	clearInterval($.myglobals.refreshTimer);
	$(window).scrollTop(0);
	$('#modal').show();
	$.content = contentdiv;
	$.backfunction = backfunction;
	$.devIdx = id;
	$.devName = name;
	var htmlcontent = '';
	htmlcontent = '<p><center><h2>' + unescape(name) + '</h2></center></p>\n';
	htmlcontent += $('#daymonthyearlog').html();
	$($.content).html(GetBackbuttonHTMLTable(backfunction) + htmlcontent);
	$($.content).i18n();

	$.DayChart = $($.content + ' #daygraph');
	$.DayChart.highcharts({
		chart: {
			type: 'spline',
			zoomType: 'x',
			resetZoomButton: {
				position: {
					x: -30,
					y: -36
				}
			},
			marginRight: 10,
			events: {
				load: function () {
					$.getJSON("json.htm?type=graph&sensor=Percentage&idx=" + id + "&range=day", function (data) {
						if (typeof data.result != 'undefined') {
							var series = $.DayChart.highcharts().series[0];
							var datatable = [];
							var minValue = 10000000;
							$.each(data.result, function (i, item) {
								datatable.push([GetUTCFromString(item.d), parseFloat(item.v)]);
								minValue = Math.min(item.v, minValue);
							});
							$.DayChart.highcharts().yAxis[0].update({ min: minValue });
							series.setData(datatable); // redraws
						}
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
			text: $.t('Percentage') + ' ' + Get5MinuteHistoryDaysGraphTitle()
		},
		xAxis: {
			type: 'datetime'
		},
		yAxis: {
			title: {
				text: 'Percentage'
			},
			min: 0,
			minorGridLineWidth: 0,
			alternateGridColor: null
		},
		tooltip: {
			crosshairs: true,
			shared: true
		},
		plotOptions: {
			series: {
				point: {
					events: {
						click: function (event) {
							chartPointClickNew(event, true, ShowPercentageLog);
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
		},
		series: [{
			id: 'percentage',
			name: 'Percentage',
			tooltip: {
				valueSuffix: ' %',
				valueDecimals: 2
			},
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
			zoomType: 'x',
			resetZoomButton: {
				position: {
					x: -30,
					y: -36
				}
			},
			marginRight: 10,
			events: {
				load: function () {
					$.getJSON("json.htm?type=graph&sensor=Percentage&idx=" + id + "&range=month", function (data) {
						if (typeof data.result != 'undefined') {
							var datatable1 = [];
							var datatable2 = [];
							var datatable3 = [];
							var minValue = 10000000;

							$.each(data.result, function (i, item) {
								datatable1.push([GetDateFromString(item.d), parseFloat(item.v_min)]);
								datatable2.push([GetDateFromString(item.d), parseFloat(item.v_max)]);
								datatable3.push([GetDateFromString(item.d), parseFloat(item.v_avg)]);
								minValue = Math.min(item.v_min, minValue);
							});

							$.MonthChart.highcharts().yAxis[0].update({ min: minValue });
							var series1 = $.MonthChart.highcharts().series[0];
							var series2 = $.MonthChart.highcharts().series[1];
							var series3 = $.MonthChart.highcharts().series[2];
							series1.setData(datatable1, false);
							series2.setData(datatable2, false);
							series3.setData(datatable3, false);
							$.MonthChart.highcharts().redraw();
						}
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
			text: $.t('Percentage') + ' ' + $.t('Last Month')
		},
		xAxis: {
			type: 'datetime'
		},
		yAxis: {
			title: {
				text: 'Percentage'
			},
			min: 0,
			minorGridLineWidth: 0,
			alternateGridColor: null
		},
		tooltip: {
			crosshairs: true,
			shared: true
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
			id: 'percentage_min',
			name: 'min',
			tooltip: {
				valueSuffix: ' %',
				valueDecimals: 2
			},
			point: {
				events: {
					click: function (event) {
						chartPointClickNew(event, false, ShowPercentageLog);
					}
				}
			}
		}, {
			id: 'percentage_max',
			name: 'max',
			tooltip: {
				valueSuffix: ' %',
				valueDecimals: 2
			},
			point: {
				events: {
					click: function (event) {
						chartPointClickNew(event, false, ShowPercentageLog);
					}
				}
			}
		}, {
			id: 'percentage_avg',
			name: 'avg',
			tooltip: {
				valueSuffix: ' %',
				valueDecimals: 2
			},
			point: {
				events: {
					click: function (event) {
						chartPointClickNew(event, false, ShowPercentageLog);
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
			zoomType: 'x',
			resetZoomButton: {
				position: {
					x: -30,
					y: -36
				}
			},
			marginRight: 10,
			events: {
				load: function () {
					$.getJSON("json.htm?type=graph&sensor=Percentage&idx=" + id + "&range=year", function (data) {
						if (typeof data.result != 'undefined') {
							var datatable1 = [];
							var datatable2 = [];
							var datatable3 = [];
							var minValue = 10000000;

							$.each(data.result, function (i, item) {
								datatable1.push([GetDateFromString(item.d), parseFloat(item.v_min)]);
								datatable2.push([GetDateFromString(item.d), parseFloat(item.v_max)]);
								datatable3.push([GetDateFromString(item.d), parseFloat(item.v_avg)]);
								minValue = Math.min(item.v_min, minValue);
							});
							$.YearChart.highcharts().yAxis[0].update({ min: minValue });
							var series1 = $.YearChart.highcharts().series[0];
							var series2 = $.YearChart.highcharts().series[1];
							var series3 = $.YearChart.highcharts().series[2];
							series1.setData(datatable1, false);
							series2.setData(datatable2, false);
							series3.setData(datatable3, false);
							$.YearChart.highcharts().redraw();
						}
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
			text: $.t('Percentage') + ' ' + $.t('Last Year')
		},
		xAxis: {
			type: 'datetime'
		},
		yAxis: {
			title: {
				text: 'Percentage'
			},
			min: 0,
			minorGridLineWidth: 0,
			alternateGridColor: null
		},
		tooltip: {
			crosshairs: true,
			shared: true
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
			id: 'percentage_min',
			name: 'min',
			tooltip: {
				valueSuffix: ' %',
				valueDecimals: 2
			},
			point: {
				events: {
					click: function (event) {
						chartPointClickNew(event, false, ShowPercentageLog);
					}
				}
			}
		}, {
			id: 'percentage_max',
			name: 'max',
			tooltip: {
				valueSuffix: ' %',
				valueDecimals: 2
			},
			point: {
				events: {
					click: function (event) {
						chartPointClickNew(event, false, ShowPercentageLog);
					}
				}
			}
		}, {
			id: 'percentage_avg',
			name: 'avg',
			tooltip: {
				valueSuffix: ' %',
				valueDecimals: 2
			},
			point: {
				events: {
					click: function (event) {
						chartPointClickNew(event, false, ShowPercentageLog);
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




function AddDataToUtilityChart(data, chart, switchtype) {

	var datatableEnergyUsed = [];
	var datatableEnergyGenerated = [];

	var datatableUsage1 = [];
	var datatableUsage2 = [];
	var datatableReturn1 = [];
	var datatableReturn2 = [];
	var datatableTotalUsage = [];
	var datatableTotalReturn = [];

	var datatableUsage1Prev = [];
	var datatableUsage2Prev = [];
	var datatableReturn1Prev = [];
	var datatableReturn2Prev = [];
	var datatableTotalUsagePrev = [];
	var datatableTotalReturnPrev = [];

	var bHaveFloat = false;

	var valueQuantity = "Count";
	if (typeof data.ValueQuantity != 'undefined') {
		valueQuantity = data.ValueQuantity;
	}

	var valueUnits = "";
	if (typeof data.ValueUnits != 'undefined') {
		valueUnits = data.ValueUnits;
	}

	var bHaveDelivered = (typeof data.delivered != 'undefined');
	var bHavePrev = (typeof data.resultprev != 'undefined');

	if (bHavePrev) {
		$.each(data.resultprev, function (i, item) {
			var cdate = GetPrevDateFromString(item.d);
			datatableUsage1Prev.push([cdate, parseFloat(item.v)]);
			if (typeof item.v2 != 'undefined') {
				datatableUsage2Prev.push([cdate, parseFloat(item.v2)]);
			}
			if (bHaveDelivered) {
				datatableReturn1Prev.push([cdate, parseFloat(item.r1)]);
				if (typeof item.r2 != 'undefined') {
					datatableReturn2Prev.push([cdate, parseFloat(item.r2)]);
				}
			}
			if (datatableUsage2Prev.length > 0) {
				datatableTotalUsagePrev.push([cdate, parseFloat(item.v) + parseFloat(item.v2)]);
			}
			else {
				datatableTotalUsagePrev.push([cdate, parseFloat(item.v)]);
			}
			if (datatableUsage2Prev.length > 0) {
				datatableTotalReturnPrev.push([cdate, parseFloat(item.r1) + parseFloat(item.r2)]);
			}
			else {
				if (typeof item.r1 != 'undefined') {
					datatableTotalReturnPrev.push([cdate, parseFloat(item.r1)]);
				}
			}
		});
	}

	$.each(data.result, function (i, item) {
		if (chart == $.DayChart) {
			var cdate = GetUTCFromString(item.d);
			if (typeof item.v != 'undefined') {
				if (switchtype != 2) {
					var fValue = parseFloat(item.v);
					if (fValue % 1 != 0)
						bHaveFloat = true;
					datatableUsage1.push([cdate, fValue]);
				}
				else {
					datatableUsage1.push([cdate, parseFloat(item.v) * $.DividerWater]);
				}
			}
			if (typeof item.v2 != 'undefined') {
				datatableUsage2.push([cdate, parseFloat(item.v2)]);
			}
			if (bHaveDelivered) {
				datatableReturn1.push([cdate, parseFloat(item.r1)]);
				if (typeof item.r2 != 'undefined') {
					datatableReturn2.push([cdate, parseFloat(item.r2)]);
				}
			}
			if (typeof item.eu != 'undefined') {
				datatableEnergyUsed.push([cdate, parseFloat(item.eu)]);
			}
			if (typeof item.eg != 'undefined') {
				datatableEnergyGenerated.push([cdate, parseFloat(item.eg)]);
			}
		}
		else {
			var cdate = GetDateFromString(item.d);
			if (switchtype != 2) {
				datatableUsage1.push([cdate, parseFloat(item.v)]);
			}
			else {
				datatableUsage1.push([cdate, parseFloat(item.v) * $.DividerWater]);
			}
			if (typeof item.v2 != 'undefined') {
				datatableUsage2.push([cdate, parseFloat(item.v2)]);
			}
			if (bHaveDelivered) {
				datatableReturn1.push([cdate, parseFloat(item.r1)]);
				if (typeof item.r2 != 'undefined') {
					datatableReturn2.push([cdate, parseFloat(item.r2)]);
				}
			}
			if (datatableUsage2.length > 0) {
				datatableTotalUsage.push([cdate, parseFloat(item.v) + parseFloat(item.v2)]);
			}
			else {
				datatableTotalUsage.push([cdate, parseFloat(item.v)]);
			}
			if (datatableUsage2.length > 0) {
				datatableTotalReturn.push([cdate, parseFloat(item.r1) + parseFloat(item.r2)]);
			}
			else {
				if (typeof item.r1 != 'undefined') {
					datatableTotalReturn.push([cdate, parseFloat(item.r1)]);
				}
			}
		}
	});

	var series;
	if ((switchtype == 0) || (switchtype == 4)) {

		//Electra Usage/Return
		if ((chart == $.DayChart) || (chart == $.WeekChart)) {
			var totDecimals = 3;
			if (chart == $.DayChart) {
				if (bHaveFloat == true) {
					totDecimals = 1;
				}
				else {
					totDecimals = 0;
				}
			}
			if (datatableEnergyUsed.length > 0) {
				if (datatableUsage2.length == 0) {
					// instant + counter type
					chart.highcharts().addSeries({
						id: 'eUsed',
						type: 'column',
						pointRange: 3600 * 1000, // 1 hour in ms
						zIndex: 5,
						animation: false,
						name: (switchtype == 0) ? $.t('Energy Usage') : $.t('Energy Generated'),
						tooltip: {
							valueSuffix: (chart == $.WeekChart) ? ' kWh' : ' Wh',
							valueDecimals: totDecimals
						},
						color: 'rgba(3,190,252,0.8)',
						yAxis: 0,
						visible: datatableUsage2.length == 0
					}, false)
				} else {
					// p1 type
					chart.highcharts().addSeries({
						id: 'eUsed',
						type: 'area',
						name: $.t('Energy Usage'),
						tooltip: {
							valueSuffix: (chart == $.WeekChart) ? ' kWh' : ' Wh',
							valueDecimals: totDecimals
						},
						color: 'rgba(120,150,220,0.9)',
						fillOpacity: 0.2,
						yAxis: 0,
						visible: datatableUsage2.length == 0
					}, false);
				}
				series = chart.highcharts().get('eUsed');
				series.setData(datatableEnergyUsed, false);
			}
			if (datatableEnergyGenerated.length > 0) {
				// p1 type
				chart.highcharts().addSeries({
					id: 'eGen',
					type: 'area',
					name: $.t('Energy Returned'),
					tooltip: {
						valueSuffix: (chart == $.WeekChart) ? ' kWh' : ' Wh',
						valueDecimals: totDecimals
					},
					color: 'rgba(120,220,150,0.9)',
					fillOpacity: 0.2,
					yAxis: 0,
					visible: false
				}, false);
				series = chart.highcharts().get('eGen');
				series.setData(datatableEnergyGenerated, false);
			}
			if (datatableUsage1.length > 0) {
				if (datatableUsage2.length == 0) {
					if (datatableEnergyUsed.length == 0) {
						// counter type (no power)
						chart.highcharts().addSeries({
							id: 'usage1',
							name: (switchtype == 0) ? $.t('Energy Usage') : $.t('Energy Generated'),
							tooltip: {
								valueSuffix: (chart == $.DayChart) ? ' Wh' : ' kWh',
								valueDecimals: totDecimals
							},
							color: 'rgba(3,190,252,0.8)',
							stack: 'susage',
							yAxis: 0
						}, false);
					} else {
						// instant + counter type
						chart.highcharts().addSeries({
							id: 'usage1',
							name: (switchtype == 0) ? $.t('Power Usage') : $.t('Power Generated'),
							zIndex: 10,
							type: (chart == $.DayChart) ? 'spline' : 'column', // power vs energy
							tooltip: {
								valueSuffix: (chart == $.DayChart) ? ' Watt' : ' kWh',
								valueDecimals: totDecimals
							},
							color: (chart == $.DayChart) ? 'rgba(255,255,0,0.8)' : 'rgba(3,190,252,0.8)', // yellow vs blue
							stack: 'susage',
							yAxis: (chart == $.DayChart && chart.highcharts().yAxis.length > 1) ? 1 : 0
						}, false);
					}
				} else {
					// p1 type
					chart.highcharts().addSeries({
						id: 'usage1',
						name: $.t('Usage') + ' 1',
						tooltip: {
							valueSuffix: (chart == $.WeekChart) ? ' kWh' : ' Watt',
							valueDecimals: totDecimals
						},
						color: 'rgba(60,130,252,0.8)',
						stack: 'susage',
						yAxis: (chart == $.WeekChart) ? 0 : 1
					}, false);
				}
				series = chart.highcharts().get('usage1');
				series.setData(datatableUsage1, false);
			}
			if (datatableUsage2.length > 0) {
				// p1 type
				chart.highcharts().addSeries({
					id: 'usage2',
					name: $.t('Usage') + ' 2',
					tooltip: {
						valueSuffix: (chart == $.WeekChart) ? ' kWh' : ' Watt',
						valueDecimals: totDecimals
					},
					color: 'rgba(3,190,252,0.8)',
					stack: 'susage',
					yAxis: (chart == $.WeekChart) ? 0 : 1
				}, false);
				series = chart.highcharts().get('usage2');
				series.setData(datatableUsage2, false);
			}
			if (bHaveDelivered) {
				if (datatableReturn1.length > 0) {
					chart.highcharts().addSeries({
						id: 'return1',
						name: $.t('Return') + ' 1',
						tooltip: {
							valueSuffix: (chart == $.WeekChart) ? ' kWh' : ' Watt',
							valueDecimals: totDecimals
						},
						color: 'rgba(30,242,110,0.8)',
						stack: 'sreturn',
						yAxis: (chart == $.WeekChart) ? 0 : 1
					}, false);
					series = chart.highcharts().get('return1');
					series.setData(datatableReturn1, false);
				}
				if (datatableReturn2.length > 0) {
					chart.highcharts().addSeries({
						id: 'return2',
						name: $.t('Return') + ' 2',
						tooltip: {
							valueSuffix: (chart == $.WeekChart) ? ' kWh' : ' Watt',
							valueDecimals: totDecimals
						},
						color: 'rgba(3,252,190,0.8)',
						stack: 'sreturn',
						yAxis: (chart == $.WeekChart) ? 0 : 1
					}, false);
					series = chart.highcharts().get('return2');
					series.setData(datatableReturn2, false);
				}
			}
		}
		else {
			//month/year, show total for now
			if (datatableTotalUsage.length > 0) {
				chart.highcharts().addSeries({
					id: 'usage',
					name: (switchtype == 0) ? $.t('Total Usage') : ((switchtype == 4) ? $.t('Total Generated') : $.t('Total Return')),
					zIndex: 2,
					tooltip: {
						valueSuffix: ' kWh',
						valueDecimals: 3
					},
					color: 'rgba(3,190,252,0.8)',
					yAxis: 0
				}, false);
				series = chart.highcharts().get('usage');
				series.setData(datatableTotalUsage, false);
				var trandLine = CalculateTrendLine(datatableTotalUsage);
				if (typeof trandLine != 'undefined') {
					var datatableTrendlineUsage = [];

					datatableTrendlineUsage.push([trandLine.x0, trandLine.y0]);
					datatableTrendlineUsage.push([trandLine.x1, trandLine.y1]);

					chart.highcharts().addSeries({
						id: 'usage_trendline',
						name: $.t('Trendline') + ' ' + ((switchtype == 0) ? $.t('Usage') : ((switchtype == 4) ? $.t('Generated') : $.t('Return'))),
						zIndex: 1,
						tooltip: {
							valueSuffix: ' kWh',
							valueDecimals: 3
						},
						color: 'rgba(252,3,3,0.8)',
						dashStyle: 'LongDash',
						yAxis: 0,
						visible: false
					}, false);
					series = chart.highcharts().get('usage_trendline');
					series.setData(datatableTrendlineUsage, false);
				}
			}
			if (bHaveDelivered) {
				if (datatableTotalReturn.length > 0) {
					chart.highcharts().addSeries({
						id: 'return',
						name: $.t('Total Return'),
						zIndex: 1,
						tooltip: {
							valueSuffix: ' kWh',
							valueDecimals: 3
						},
						color: 'rgba(3,252,190,0.8)',
						yAxis: 0
					}, false);
					series = chart.highcharts().get('return');
					series.setData(datatableTotalReturn, false);
					var trandLine = CalculateTrendLine(datatableTotalReturn);
					if (typeof trandLine != 'undefined') {
						var datatableTrendlineReturn = [];

						datatableTrendlineReturn.push([trandLine.x0, trandLine.y0]);
						datatableTrendlineReturn.push([trandLine.x1, trandLine.y1]);

						chart.highcharts().addSeries({
							id: 'return_trendline',
							name: $.t('Trendline') + ' ' + $.t('Return'),
							zIndex: 1,
							tooltip: {
								valueSuffix: ' kWh',
								valueDecimals: 3
							},
							color: 'rgba(255,127,39,0.8)',
							dashStyle: 'LongDash',
							yAxis: 0,
							visible: false
						}, false);
						series = chart.highcharts().get('return_trendline');
						series.setData(datatableTrendlineReturn, false);
					}
				}
			}
			if (datatableTotalUsagePrev.length > 0) {
				chart.highcharts().addSeries({
					id: 'usageprev',
					name: $.t('Past') + ' ' + ((switchtype == 0) ? $.t('Usage') : ((switchtype == 4) ? $.t('Generated') : $.t('Return'))),
					tooltip: {
						valueSuffix: ' kWh',
						valueDecimals: 3
					},
					color: 'rgba(190,3,252,0.8)',
					yAxis: 0,
					visible: false
				}, false);
				series = chart.highcharts().get('usageprev');
				series.setData(datatableTotalUsagePrev, false);
			}
			if (bHaveDelivered) {
				if (datatableTotalReturnPrev.length > 0) {
					chart.highcharts().addSeries({
						id: 'returnprev',
						name: $.t('Past') + ' ' + $.t('Return'),
						tooltip: {
							valueSuffix: ' kWh',
							valueDecimals: 3
						},
						color: 'rgba(252,190,3,0.8)',
						yAxis: 0,
						visible: false
					}, false);
					series = chart.highcharts().get('returnprev');
					series.setData(datatableTotalReturnPrev, false);
				}
			}
		}
	}
	else if (switchtype == 1) {
		//gas
		chart.highcharts().addSeries({
			id: 'gas',
			name: 'Gas',
			zIndex: 2,
			tooltip: {
				valueSuffix: ' m3',
				valueDecimals: 3
			},
			color: 'rgba(3,190,252,0.8)',
			yAxis: 0
		}, false);
		if ((chart == $.MonthChart) || (chart == $.YearChart)) {
			var trandLine = CalculateTrendLine(datatableUsage1);
			if (typeof trandLine != 'undefined') {
				var datatableTrendlineUsage = [];

				datatableTrendlineUsage.push([trandLine.x0, trandLine.y0]);
				datatableTrendlineUsage.push([trandLine.x1, trandLine.y1]);

				chart.highcharts().addSeries({
					id: 'usage_trendline',
					name: 'Trendline ' + $.t('Gas'),
					zIndex: 1,
					tooltip: {
						valueSuffix: ' m3',
						valueDecimals: 3
					},
					color: 'rgba(252,3,3,0.8)',
					dashStyle: 'LongDash',
					yAxis: 0,
					visible: false
				}, false);
				series = chart.highcharts().get('usage_trendline');
				series.setData(datatableTrendlineUsage, false);
			}
			if (datatableUsage1Prev.length > 0) {
				chart.highcharts().addSeries({
					id: 'gasprev',
					name: $.t('Past') + ' ' + $.t('Gas'),
					tooltip: {
						valueSuffix: ' m3',
						valueDecimals: 3
					},
					color: 'rgba(190,3,252,0.8)',
					yAxis: 0,
					visible: false
				}, false);
				series = chart.highcharts().get('gasprev');
				series.setData(datatableUsage1Prev, false);
			}
		}
		series = chart.highcharts().get('gas');
		series.setData(datatableUsage1, false);
		chart.highcharts().yAxis[0].options.title.text = 'Gas m3';
	}
	else if (switchtype == 2) {
		//water
		chart.highcharts().addSeries({
			id: 'water',
			name: 'Water',
			tooltip: {
				valueSuffix: ' Liter',
				valueDecimals: 0
			},
			color: 'rgba(3,190,252,0.8)',
			yAxis: 0
		}, false);
		chart.highcharts().yAxis[0].options.title.text = 'Water Liter';
		series = chart.highcharts().get('water');
		series.setData(datatableUsage1, false);
	}
	else if (switchtype == 3) {
		//counter
		chart.highcharts().addSeries({
			id: 'counter',
			name: valueQuantity,
			tooltip: {
				valueSuffix: ' ' + valueUnits,
				valueDecimals: 0
			},
			color: 'rgba(3,190,252,0.8)',
			yAxis: 0
		}, false);
		chart.highcharts().yAxis[0].options.title.text = valueQuantity + ' ' + valueUnits;
		series = chart.highcharts().get('counter');
		series.setData(datatableUsage1, false);
	}
}

function GetGraphUnit(uname) {
	if (uname == $.t('Usage'))
		return 'kWh';
	if (uname == $.t('Usage') + ' 1')
		return 'kWh';
	if (uname == $.t('Usage') + ' 2')
		return 'kWh';
	if (uname == $.t('Return') + ' 1')
		return 'kWh';
	if (uname == $.t('Return') + ' 2')
		return 'kWh';
	if (uname == $.t('Gas'))
		return 'm3';
	if (uname == $.t('Past') + ' ' + $.t('Gas'))
		return 'm3';
	if (uname == $.t('Water'))
		return 'm3';
	if (uname == $.t('Power'))
		return 'Watt';
	if (uname == $.t('Total Usage'))
		return 'kWh';
	if (uname == $.t('Past') + ' ' + $.t('Usage'))
		return 'kWh';
	if (uname == $.t('Past') + ' ' + $.t('Return'))
		return 'kWh';
	if (uname == $.t('Return'))
		return 'kWh';
	if (uname == $.t('Generated'))
		return 'kWh';

	return '?';
}

function ShowSmartLog(contentdiv, backfunction, id, name, switchtype) {
	clearInterval($.myglobals.refreshTimer);
	$(window).scrollTop(0);
	$('#modal').show();
	$.content = contentdiv;
	$.backfunction = backfunction;
	$.devIdx = id;
	$.devName = name;
	$.devSwitchType = switchtype;
	var htmlcontent = '';
	htmlcontent = '<p><center><h2>' + unescape(name) + '</h2></center></p>\n';
	htmlcontent += $('#dayweekmonthyearlog').html();

	$.costsT1 = 0.2389;
	$.costsT2 = 0.2389;
	$.costsR1 = 0.08;
	$.costsR2 = 0.08;
	$.costsGas = 0.6218;
	$.costsWater = 1.6473;

	$.ajax({
		url: "json.htm?type=command&param=getcosts&idx=" + $.devIdx,
		async: false,
		dataType: 'json',
		success: function (data) {
			$.costsT1 = parseFloat(data.CostEnergy) / 10000;
			$.costsT2 = parseFloat(data.CostEnergyT2) / 10000;
			$.costsR1 = parseFloat(data.CostEnergyR1) / 10000;
			$.costsR2 = parseFloat(data.CostEnergyR2) / 10000;
			$.costsGas = parseFloat(data.CostGas) / 10000;
			$.costsWater = parseFloat(data.CostWater) / 10000;
			$.CounterT1 = parseFloat(data.CounterT1);
			$.CounterT2 = parseFloat(data.CounterT2);
			$.CounterR1 = parseFloat(data.CounterR1);
			$.CounterR2 = parseFloat(data.CounterR2);
		}
	});

	$.monthNames = ["January", "February", "March", "April", "May", "June",
		"July", "August", "September", "October", "November", "December"];

	var d = new Date();
	var actMonth = d.getMonth() + 1;
	var actYear = d.getYear() + 1900;

	$($.content).html(GetBackbuttonHTMLTableWithRight(backfunction, 'ShowP1YearReport(' + actYear + ')', $.t('Report')) + htmlcontent);
	$($.content).i18n();

	$.DayChart = $($.content + ' #daygraph');
	$.DayChart.highcharts({
		chart: {
			type: 'spline',
			zoomType: 'x',
			resetZoomButton: {
				position: {
					x: -30,
					y: -36
				}
			},
			events: {
				load: function () {
					$.getJSON("json.htm?type=graph&sensor=counter&idx=" + id + "&range=day",
						function (data) {
							if (typeof data.result != 'undefined') {
								AddDataToUtilityChart(data, $.DayChart, switchtype);
								$.DayChart.highcharts().redraw();
							}
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
			text: Get5MinuteHistoryDaysGraphTitle()
		},
		xAxis: {
			type: 'datetime'
		},
		yAxis: [{
			title: {
				text: $.t('Energy') + ' (Wh)'
			},
			min: 0
		},
		{
			title: {
				text: $.t('Power') + ' (Watt)'
			},
			min: 0,
			opposite: true
		}],
		tooltip: {
			crosshairs: true,
			shared: true
		},
		plotOptions: {
			series: {
				point: {
					events: {
						click: function (event) {
							chartPointClickNewEx(event, true, ShowSmartLog);
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
		},
		legend: {
			enabled: true
		}
	});

	$.WeekChart = $($.content + ' #weekgraph');
	$.WeekChart.highcharts({
		chart: {
			type: 'column',
			marginRight: 10,
			events: {
				load: function () {
					$.getJSON("json.htm?type=graph&sensor=counter&idx=" + id + "&range=week",
						function (data) {
							if (typeof data.result != 'undefined') {
								AddDataToUtilityChart(data, $.WeekChart, switchtype);
								$.WeekChart.highcharts().redraw();
							}
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
			text: $.t('Last Week')
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
				text: $.t('Energy') + ' (kWh)'
			},
			min: 0
		},
		tooltip: {
			formatter: function () {
				var unit = GetGraphUnit(this.series.name);
				return $.t(Highcharts.dateFormat('%A', this.x)) + ' ' + Highcharts.dateFormat('%Y-%m-%d', this.x) + '<br/>' + $.t(this.series.name) + ': ' + this.y + ' ' + unit + '<br/>Total: ' + this.point.stackTotal + ' ' + unit;
			}
		},
		plotOptions: {
			column: {
				stacking: 'normal',
				minPointLength: 4,
				pointPadding: 0.1,
				groupPadding: 0
			}
		},
		legend: {
			enabled: true
		}
	});

	$.MonthChart = $($.content + ' #monthgraph');
	$.MonthChart.highcharts({
		chart: {
			type: 'spline',
			marginRight: 10,
			zoomType: 'x',
			resetZoomButton: {
				position: {
					x: -30,
					y: -36
				}
			},
			events: {
				load: function () {
					$.getJSON("json.htm?type=graph&sensor=counter&idx=" + id + "&range=month",
						function (data) {
							if (typeof data.result != 'undefined') {
								AddDataToUtilityChart(data, $.MonthChart, switchtype);
								$.MonthChart.highcharts().redraw();
							}
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
			text: $.t('Last Month')
		},
		xAxis: {
			type: 'datetime'
		},
		yAxis: {
			title: {
				text: $.t('Energy') + ' (kWh)'
			},
			min: 0
		},
		tooltip: {
			crosshairs: true,
			shared: true
		},
		plotOptions: {
			series: {
				point: {
					events: {
						click: function (event) {
							chartPointClickNewEx(event, false, ShowSmartLog);
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
		},
		legend: {
			enabled: true
		}
	});

	$.YearChart = $($.content + ' #yeargraph');
	$.YearChart.highcharts({
		chart: {
			type: 'spline',
			marginRight: 10,
			zoomType: 'x',
			resetZoomButton: {
				position: {
					x: -30,
					y: -36
				}
			},
			events: {
				load: function () {
					$.getJSON("json.htm?type=graph&sensor=counter&idx=" + id + "&range=year",
						function (data) {
							if (typeof data.result != 'undefined') {
								AddDataToUtilityChart(data, $.YearChart, switchtype);
								$.YearChart.highcharts().redraw();
							}
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
			text: $.t('Last Year')
		},
		xAxis: {
			type: 'datetime'
		},
		yAxis: {
			title: {
				text: $.t('Energy') + ' (kWh)'
			},
			min: 0
		},
		tooltip: {
			crosshairs: true,
			shared: true
		},
		plotOptions: {
			series: {
				point: {
					events: {
						click: function (event) {
							chartPointClickNewEx(event, false, ShowSmartLog);
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
		},
		legend: {
			enabled: true
		}
	});
}

function OnSelChangeYearP1ReportGas() {
	var yearidx = $($.content + ' #comboyear option:selected').val();
	if (typeof yearidx == 'undefined') {
		return;
	}
	ShowP1YearReportGas(yearidx);
}

function ShowP1MonthReportGas(actMonth, actYear) {
	var htmlcontent = '';

	htmlcontent += $('#toptextmonthgas').html();
	htmlcontent += $('#monthreportviewgas').html();
	$($.content).html(htmlcontent);
	$($.content).i18n();

	$($.content + ' #theader').html(unescape($.devName) + " " + $.t($.monthNames[actMonth - 1]) + " " + actYear);
	$($.content + ' #spanmonthgastotalusage').html(($.devSwitchType == 4) ? $.t('Total Generated') : $.t('Total Usage'));

	var yAxisTitle = $.t('Energy') + ' (kWh)';
	if (($.devSwitchType == 0) || ($.devSwitchType == 4)) {
		//Electra
		$($.content + ' #munit').html("kWh");
		$($.content + ' #thmonthgasusage').html(($.devSwitchType == 4) ? $.t('Generated') : $.t('Usage'));
	}
	else if ($.devSwitchType == 1) {
		//Gas
		$($.content + ' #munit').html("m3");
		yAxisTitle = $.t('Usage') + ' (m3)';
	}
	else {
		//Water
		$($.content + ' #munit').html("m3");
		yAxisTitle = $.t('Usage') + ' (m3)';
	}

	$($.content + ' #monthreport').dataTable({
		"sDom": '<"H"rC>t<"F">',
		"oTableTools": {
			"sRowSelect": "single"
		},
		"aaSorting": [[0, "asc"]],
		"aoColumnDefs": [
			{ "bSortable": false, "aTargets": [3] }
		],
		"bSortClasses": false,
		"bProcessing": true,
		"bStateSave": false,
		"bJQueryUI": true,
		"aLengthMenu": [[50, 100, -1], [50, 100, "All"]],
		"iDisplayLength": 50,
		language: $.DataTableLanguage
	});
	var mTable = $($.content + ' #monthreport');
	var oTable = mTable.dataTable();
	oTable.fnClearTable();

	$.UsageChart = $($.content + ' #usagegraph');
	$.UsageChart.highcharts({
		chart: {
			type: 'column'
		},
		credits: {
			enabled: true,
			href: "http://www.domoticz.com",
			text: "Domoticz.com"
		},
		title: {
			text: ''
		},
		xAxis: {
			type: 'datetime'
		},
		yAxis: {
			min: 0,
			maxPadding: 0.2,
			title: {
				text: yAxisTitle
			},
			min: 0
		},
		tooltip: {
			formatter: function () {
				var unit = GetGraphUnit(this.series.name);
				return $.t(Highcharts.dateFormat('%A', this.x)) + ' ' + Highcharts.dateFormat('%B %d', this.x) + '<br/>' + this.series.name + ': ' + this.y + ' ' + unit;
			}
		},
		plotOptions: {
			column: {
				minPointLength: 4,
				pointPadding: 0.1,
				groupPadding: 0
			}
		},
		legend: {
			enabled: true
		}
	});

	var total = 0;
	var datachart = [];

	$.getJSON("json.htm?type=graph&sensor=counter&idx=" + $.devIdx + "&range=year&actmonth=" + actMonth + "&actyear=" + actYear,
		function (data) {
			var lastTotal = -1;
			$.each(data.result, function (i, item) {
				var month = parseInt(item.d.substring(5, 7), 10);
				var year = parseInt(item.d.substring(0, 4), 10);

				if ((month == actMonth) && (year == actYear)) {
					var day = parseInt(item.d.substring(8, 10), 10);
					var Usage = parseFloat(item.v);
					var Counter = parseFloat(item.c);

					total += Usage;

					var cdate = Date.UTC(actYear, actMonth - 1, day);
					var cday = $.t(dateFormat(cdate, "dddd"));
					datachart.push([cdate, parseFloat(item.v)]);

					var rcost;
					if (($.devSwitchType == 0) || ($.devSwitchType == 4)) {
						//Electra
						rcost = Usage * $.costsT1;
					}
					else if ($.devSwitchType == 1) {
						//Gas
						rcost = Usage * $.costsGas;
					}
					else {
						//Water
						rcost = Usage * $.costsWater;
					}

					var img;
					if ((lastTotal == -1) || (lastTotal == Usage)) {
						img = '<img src="images/equal.png"></img>';
					}
					else if (Usage < lastTotal) {
						if ($.devSwitchType == 4) {
							img = '<img src="images/up.png" class="vflip"></img>';
						} else {
							img = '<img src="images/down.png"></img>';
						}
					}
					else {
						if ($.devSwitchType == 4) {
							img = '<img src="images/down.png" class="vflip"></img>';
						} else {
							img = '<img src="images/up.png"></img>';
						}
					}
					lastTotal = Usage;

					var addId = oTable.fnAddData([
						day,
						cday,
						Counter.toFixed(3),
						Usage.toFixed(3),
						rcost.toFixed(2),
						img
					], false);
				}
			});

			$($.content + ' #tu').html(total.toFixed(3));

			var montlycosts;
			if (($.devSwitchType == 0) || ($.devSwitchType == 4)) {
				//Electra
				montlycosts = (total * $.costsT1)
				$.UsageChart.highcharts().addSeries({
					id: 'energy',
					name: ($.devSwitchType == 0) ? $.t('Usage') : $.t('Generated'),
					showInLegend: false,
					color: 'rgba(3,190,252,0.8)',
					yAxis: 0
				});
				var series = $.UsageChart.highcharts().get('energy');
				series.setData(datachart);
			}
			else if ($.devSwitchType == 1) {
				//Gas
				montlycosts = (total * $.costsGas)
				$.UsageChart.highcharts().addSeries({
					id: 'gas',
					name: 'Gas',
					showInLegend: false,
					color: 'rgba(3,190,252,0.8)',
					yAxis: 0
				});
				var series = $.UsageChart.highcharts().get('gas');
				series.setData(datachart);
			}
			else {
				//Water
				montlycosts = (total * $.costsWater)
				$.UsageChart.highcharts().addSeries({
					id: 'water',
					name: 'Water',
					showInLegend: false,
					color: 'rgba(3,190,252,0.8)',
					yAxis: 0
				});
				var series = $.UsageChart.highcharts().get('water');
				series.setData(datachart);
			}
			$($.content + ' #mc').html(montlycosts.toFixed(2));

			mTable.fnDraw();
			/* Add a click handler to the rows - this could be used as a callback */
			$($.content + ' #monthreport tbody tr').click(function (e) {
				if ($(this).hasClass('row_selected')) {
					$(this).removeClass('row_selected');
				}
				else {
					oTable.$('tr.row_selected').removeClass('row_selected');
					$(this).addClass('row_selected');
				}
			});
		});

	return false;
}

function addLeadingZeros(n, length) {
	var str = n.toString();
	var zeros = "";
	for (var i = length - str.length; i > 0; i--)
		zeros += "0";
	zeros += str;
	return zeros;
}

function Add2YearTableP1ReportGas(oTable, total, lastTotal, lastMonth, actYear) {
	var rcost;
	if (($.devSwitchType == 0) || ($.devSwitchType == 4)) {
		//Electra
		rcost = total * $.costsT1;
	}
	else if ($.devSwitchType == 1) {
		//Gas
		rcost = total * $.costsGas;
	}
	else {
		//Water
		rcost = total * $.costsWater;
	}

	var img;
	if ((lastTotal == -1) || (lastTotal == total)) {
		img = '<img src="images/equal.png"></img>';
	}
	else if (total < lastTotal) {
		if ($.devSwitchType == 4) {
			img = '<img src="images/up.png" class="vflip"></img>';
		} else {
			img = '<img src="images/down.png"></img>';
		}
	}
	else {
		if ($.devSwitchType == 4) {
			img = '<img src="images/down.png" class="vflip"></img>';
		} else {
			img = '<img src="images/up.png"></img>';
		}
	}

	var monthtxt = addLeadingZeros(parseInt(lastMonth), 2) + ". " + $.t($.monthNames[lastMonth - 1]) + " ";
	monthtxt += '<img src="images/next.png" onclick="ShowP1MonthReportGas(' + lastMonth + ',' + actYear + ')">';

	var addId = oTable.fnAddData([
		monthtxt,
		total.toFixed(3),
		rcost.toFixed(2),
		img
	], false);
	return total;
}

function ShowP1YearReportGas(actYear) {
	if (actYear == 0) {
		actYear = $.actYear;
	}
	else {
		$.actYear = actYear;
	}
	var htmlcontent = '';
	htmlcontent += $('#toptextyeargas').html();
	htmlcontent += $('#yearreportviewgas').html();

	$($.content).html(htmlcontent);
	$($.content + ' #backbutton').click(function (e) {
		eval($.backfunction)();
	});
	$($.content).i18n();

	$($.content + ' #theader').html(unescape($.devName) + " " + actYear);
	$($.content + ' #spanyeargastotalusage').html(($.devSwitchType == 4) ? $.t('Total Generated') : $.t('Total Usage'));

	var yAxisTitle = $.t('Energy') + ' (kWh)';
	if (($.devSwitchType == 0) || ($.devSwitchType == 4)) {
		//Electra
		$($.content + ' #munit').html("kWh");
		$($.content + ' #thyeargasusage').html(($.devSwitchType == 4) ? $.t('Generated') : $.t('Usage'));
	}
	else if ($.devSwitchType == 1) {
		//Gas
		$($.content + ' #munit').html("m3");
		yAxisTitle = $.t('Usage') + ' (m3)';
	}
	else {
		//Water
		$($.content + ' #munit').html("m3");
		yAxisTitle = $.t('Usage') + ' (m3)';
	}

	$($.content + ' #comboyear').val(actYear);

	$($.content + ' #comboyear').change(function () {
		OnSelChangeYearP1ReportGas();
	});
	$($.content + ' #comboyear').keypress(function () {
		$(this).change();
	});

	$($.content + ' #yearreport').dataTable({
		"sDom": '<"H"rC>t<"F">',
		"oTableTools": {
			"sRowSelect": "single"
		},
		"aaSorting": [[0, "asc"]],
		"aoColumnDefs": [
			{ "bSortable": false, "aTargets": [3] }
		],
		"bSortClasses": false,
		"bProcessing": true,
		"bStateSave": false,
		"bJQueryUI": true,
		"aLengthMenu": [[50, 100, -1], [50, 100, "All"]],
		"iDisplayLength": 50,
		language: $.DataTableLanguage
	});

	var mTable = $($.content + ' #yearreport');
	var oTable = mTable.dataTable();
	oTable.fnClearTable();

	$.UsageChart = $($.content + ' #usagegraph');
	$.UsageChart.highcharts({
		chart: {
			type: 'column'
		},
		credits: {
			enabled: true,
			href: "http://www.domoticz.com",
			text: "Domoticz.com"
		},
		title: {
			text: ''
		},
		xAxis: {
			type: 'datetime'
		},
		yAxis: {
			min: 0,
			maxPadding: 0.2,
			title: {
				text: yAxisTitle
			},
			min: 0
		},
		tooltip: {
			formatter: function () {
				var unit = GetGraphUnit(this.series.name);
				return $.t(Highcharts.dateFormat('%B', this.x)) + '<br/>' + this.series.name + ': ' + this.y + ' ' + unit;
			}
		},
		plotOptions: {
			column: {
				minPointLength: 4,
				pointPadding: 0.1,
				groupPadding: 0,
				dataLabels: {
					enabled: true,
					color: 'white'
				}
			}
		},
		legend: {
			enabled: true
		}
	});

	var total = 0;
	var global = 0;
	var datachart = [];

	var actual_counter = "Unknown?";

	$.getJSON("json.htm?type=graph&sensor=counter&idx=" + $.devIdx + "&range=year&actyear=" + actYear,
		function (data) {
			var lastTotal = -1;
			var lastMonth = -1;
			if (typeof data.counter != 'undefined') {
				actual_counter = data.counter;
			}

			$.each(data.result, function (i, item) {
				var month = parseInt(item.d.substring(5, 7), 10);
				var year = parseInt(item.d.substring(0, 4), 10);
				if (year == actYear) {
					if (lastMonth == -1) {
						lastMonth = month;
					}
					if (lastMonth != month) {
						//add totals to table
						lastTotal = Add2YearTableP1ReportGas(oTable, total, lastTotal, lastMonth, actYear);

						var cdate = Date.UTC(actYear, lastMonth - 1, 1);
						datachart.push([cdate, parseFloat(total.toFixed(3))]);

						lastMonth = month;
						global += total;

						total = 0;
					}
					var day = parseInt(item.d.substring(8, 10), 10);
					var Usage = 0;

					Usage = parseFloat(item.v);
					total += Usage;
				}
			});

			//add last month
			if (total != 0) {
				lastTotal = Add2YearTableP1ReportGas(oTable, total, lastTotal, lastMonth, actYear);
				var cdate = Date.UTC(actYear, lastMonth - 1, 1);
				datachart.push([cdate, parseFloat(total.toFixed(3))]);
				global += lastTotal;
			}

			$($.content + ' #tu').html(global.toFixed(3));
			var montlycosts = 0;
			if (($.devSwitchType == 0) || ($.devSwitchType == 4)) {
				//Electra
				montlycosts = (global * $.costsT1);
				$.UsageChart.highcharts().addSeries({
					id: 'energy',
					name: ($.devSwitchType == 0) ? $.t('Usage') : $.t('Generated'),
					showInLegend: false,
					color: 'rgba(3,190,252,0.8)',
					yAxis: 0
				});
				var series = $.UsageChart.highcharts().get('energy');
				series.setData(datachart);
			}
			else if ($.devSwitchType == 1) {
				//Gas
				montlycosts = (global * $.costsGas);
				$.UsageChart.highcharts().addSeries({
					id: 'gas',
					name: 'Gas',
					showInLegend: false,
					color: 'rgba(3,190,252,0.8)',
					yAxis: 0
				});
				var series = $.UsageChart.highcharts().get('gas');
				series.setData(datachart);
			}
			else {
				//Water
				montlycosts = (global * $.costsWater);
				$.UsageChart.highcharts().addSeries({
					id: 'water',
					name: 'Water',
					showInLegend: false,
					color: 'rgba(3,190,252,0.8)',
					yAxis: 0
				});
				var series = $.UsageChart.highcharts().get('water');
				series.setData(datachart);
			}

			$($.content + ' #mc').html(montlycosts.toFixed(2));
			$($.content + ' #cntr').html(actual_counter);

			mTable.fnDraw();
			/* Add a click handler to the rows - this could be used as a callback */
			$($.content + ' #yearreport tbody tr').click(function (e) {
				if ($(this).hasClass('row_selected')) {
					$(this).removeClass('row_selected');
				}
				else {
					oTable.$('tr.row_selected').removeClass('row_selected');
					$(this).addClass('row_selected');
				}
			});
		});

	return false;
}

function ShowP1MonthReport(actMonth, actYear) {
	var htmlcontent = '';

	htmlcontent += $('#toptextmonth').html();
	htmlcontent += $('#monthreportview').html();
	$($.content).html(htmlcontent);
	$($.content).i18n();

	$($.content + ' #theader').html(unescape($.devName) + " " + $.t($.monthNames[actMonth - 1]) + " " + actYear);

	$($.content + ' #monthreport').dataTable({
		"sDom": '<"H"rC>t<"F">',
		"oTableTools": {
			"sRowSelect": "single"
		},
		"aaSorting": [[0, "asc"]],
		"aoColumnDefs": [
			{ "bSortable": false, "aTargets": [10] }
		],
		"bSortClasses": false,
		"bProcessing": true,
		"bStateSave": false,
		"bJQueryUI": true,
		"aLengthMenu": [[50, 100, -1], [50, 100, "All"]],
		"iDisplayLength": 50,
		language: $.DataTableLanguage
	});
	var mTable = $($.content + ' #monthreport');
	var oTable = mTable.dataTable();
	oTable.fnClearTable();

	$.UsageChart = $($.content + ' #usagegraph');
	$.UsageChart.highcharts({
		chart: {
			type: 'column',
			marginRight: 10
		},
		credits: {
			enabled: true,
			href: "http://www.domoticz.com",
			text: "Domoticz.com"
		},
		title: {
			text: ''
		},
		xAxis: {
			type: 'datetime'
		},
		yAxis: {
			title: {
				text: $.t('Energy') + ' (kWh)'
			},
			min: 0
		},
		tooltip: {
			formatter: function () {
				var unit = GetGraphUnit(this.series.name);
				return $.t(Highcharts.dateFormat('%B', this.x)) + " " + Highcharts.dateFormat('%d', this.x) + '<br/>' + $.t(this.series.name) + ': ' + this.y + ' ' + unit + '<br/>Total: ' + this.point.stackTotal + ' ' + unit;
			}
		},
		plotOptions: {
			column: {
				stacking: 'normal',
				minPointLength: 4,
				pointPadding: 0.1,
				groupPadding: 0
			}
		},
		legend: {
			enabled: true
		}
	});
	var datachartT1 = [];
	var datachartT2 = [];
	var datachartR1 = [];
	var datachartR2 = [];

	var totalT1 = 0;
	var totalT2 = 0;
	var totalR1 = 0;
	var totalR2 = 0;

	var bHaveDelivered = false;

	$.getJSON("json.htm?type=graph&sensor=counter&idx=" + $.devIdx + "&range=year&actmonth=" + actMonth + "&actyear=" + actYear,
		function (data) {
			bHaveDelivered = (typeof data.delivered != 'undefined');
			if (bHaveDelivered == false) {
				$($.content + ' #dreturn').hide();
			}
			else {
				$($.content + ' #dreturn').show();
			}

			oTable.fnSetColumnVis(8, bHaveDelivered);
			oTable.fnSetColumnVis(9, bHaveDelivered);
			oTable.fnSetColumnVis(10, bHaveDelivered);
			oTable.fnSetColumnVis(11, bHaveDelivered);
			oTable.fnSetColumnVis(12, bHaveDelivered);
			oTable.fnSetColumnVis(13, bHaveDelivered);

			var lastTotal = -1;

			$.each(data.result, function (i, item) {
				var month = parseInt(item.d.substring(5, 7), 10);
				var year = parseInt(item.d.substring(0, 4), 10);

				if ((month == actMonth) && (year == actYear)) {
					var day = parseInt(item.d.substring(8, 10), 10);
					var UsageT1 = 0;
					var UsageT2 = 0;
					var ReturnT1 = 0;
					var ReturnT2 = 0;

					UsageT1 = parseFloat(item.v);
					if (typeof item.v2 != 'undefined') {
						UsageT2 = parseFloat(item.v2);
					}
					if (typeof item.r1 != 'undefined') {
						ReturnT1 = parseFloat(item.r1);
					}
					if (typeof item.r2 != 'undefined') {
						ReturnT2 = parseFloat(item.r2);
					}

					var cdate = Date.UTC(actYear, actMonth - 1, day);
					var cday = $.t(dateFormat(cdate, "dddd"));
					datachartT1.push([cdate, parseFloat(UsageT1.toFixed(3))]);
					datachartT2.push([cdate, parseFloat(UsageT2.toFixed(3))]);
					datachartR1.push([cdate, parseFloat(ReturnT1.toFixed(3))]);
					datachartR2.push([cdate, parseFloat(ReturnT2.toFixed(3))]);

					totalT1 += UsageT1;
					totalT2 += UsageT2;
					totalR1 += ReturnT1;
					totalR2 += ReturnT2;

					var rcostT1 = UsageT1 * $.costsT1;
					var rcostT2 = UsageT2 * $.costsT2;
					var rcostR1 = -(ReturnT1 * $.costsR1);
					var rcostR2 = -(ReturnT2 * $.costsR2);
					var rTotal = rcostT1 + rcostT2 + rcostR1 + rcostR2;

					var textR1 = "";
					var textR2 = "";
					var textCostR1 = "";
					var textCostR2 = "";

					if (ReturnT1 != 0) {
						textR1 = ReturnT1.toFixed(3);
						textCostR1 = rcostR1.toFixed(2);
					}
					if (ReturnT2 != 0) {
						textR2 = ReturnT2.toFixed(3);
						textCostR2 = rcostR2.toFixed(2);
					}
					var CounterT1 = parseFloat(item.c1).toFixed(3);
					var CounterT2 = parseFloat(item.c3).toFixed(3);
					var CounterR1 = parseFloat(item.c2).toFixed(3);
					var CounterR2 = parseFloat(item.c4).toFixed(3);

					var img;
					if ((lastTotal == -1) || (lastTotal == rTotal)) {
						img = '<img src="images/equal.png"></img>';
					}
					else if (rTotal < lastTotal) {
						img = '<img src="images/down.png"></img>';
					}
					else {
						img = '<img src="images/up.png"></img>';
					}
					lastTotal = rTotal;

					var addId = oTable.fnAddData([
						day,
						cday,
						CounterT1,
						UsageT1.toFixed(3),
						rcostT1.toFixed(2),
						CounterT2,
						UsageT2.toFixed(3),
						rcostT2.toFixed(2),
						CounterR1,
						textR1,
						textCostR1,
						CounterR2,
						textR2,
						textCostR2,
						rTotal.toFixed(2),
						img
					], false);
				}
			});

			if (datachartT1.length > 0) {
				if (datachartT2.length > 0) {
					$.UsageChart.highcharts().addSeries({
						id: 'usage1',
						name: $.t('Usage') + ' 1',
						color: 'rgba(60,130,252,0.8)',
						stack: 'susage',
						yAxis: 0
					});
				}
				else {
					$.UsageChart.highcharts().addSeries({
						id: 'usage1',
						name: 'Usage',
						color: 'rgba(3,190,252,0.8)',
						stack: 'susage',
						yAxis: 0
					});
				}
				series = $.UsageChart.highcharts().get('usage1');
				series.setData(datachartT1);
			}
			if (datachartT2.length > 0) {
				$.UsageChart.highcharts().addSeries({
					id: 'usage2',
					name: $.t('Usage') + ' 2',
					color: 'rgba(3,190,252,0.8)',
					stack: 'susage',
					yAxis: 0
				});
				series = $.UsageChart.highcharts().get('usage2');
				series.setData(datachartT2);
			}
			if (bHaveDelivered) {
				if (datachartR1.length > 0) {
					$.UsageChart.highcharts().addSeries({
						id: 'return1',
						name: $.t('Return') + ' 1',
						color: 'rgba(30,242,110,0.8)',
						stack: 'sreturn',
						yAxis: 0
					});
					series = $.UsageChart.highcharts().get('return1');
					series.setData(datachartR1);
				}
				if (datachartR2.length > 0) {
					$.UsageChart.highcharts().addSeries({
						id: 'return2',
						name: $.t('Return') + ' 2',
						color: 'rgba(3,252,190,0.8)',
						stack: 'sreturn',
						yAxis: 0
					});
					series = $.UsageChart.highcharts().get('return2');
					series.setData(datachartR2);
				}
			}

			$($.content + ' #tut1').html(totalT1.toFixed(3));
			$($.content + ' #tut2').html(totalT2.toFixed(3));
			$($.content + ' #trt1').html(totalR1.toFixed(3));
			$($.content + ' #trt2').html(totalR2.toFixed(3));

			var gtotal = totalT1 + totalT2;
			var greturn = totalR1 + totalR2;
			$($.content + ' #tu').html(gtotal.toFixed(3));
			$($.content + ' #tr').html(greturn.toFixed(3));
			var montlycosts = (totalT1 * $.costsT1) + (totalT2 * $.costsT2) - (totalR1 * $.costsR1) - (totalR2 * $.costsR2);
			$($.content + ' #mc').html(montlycosts.toFixed(2));

			mTable.fnDraw();
			/* Add a click handler to the rows - this could be used as a callback */
			$($.content + ' #monthreport tbody tr').click(function (e) {
				if ($(this).hasClass('row_selected')) {
					$(this).removeClass('row_selected');
				}
				else {
					oTable.$('tr.row_selected').removeClass('row_selected');
					$(this).addClass('row_selected');
				}
			});
		});

	return false;
}

function OnSelChangeYearP1Report() {
	var yearidx = $($.content + ' #comboyear option:selected').val();
	if (typeof yearidx == 'undefined') {
		return;
	}
	ShowP1YearReport(yearidx);
}

function Add2YearTableP1Report(oTable, totalT1, totalT2, totalR1, totalR2, lastTotal, lastMonth, actYear) {
	var rcostT1 = totalT1 * $.costsT1;
	var rcostT2 = totalT2 * $.costsT2;
	var rcostR1 = -(totalR1 * $.costsR1);
	var rcostR2 = -(totalR2 * $.costsR2);
	var rTotal = rcostT1 + rcostT2 + rcostR1 + rcostR2;

	var textR1 = "";
	var textR2 = "";
	var textCostR1 = "";
	var textCostR2 = "";

	if (totalR1 != 0) {
		textR1 = totalR1.toFixed(3);
		textCostR1 = rcostR1.toFixed(2);
	}
	if (totalR2 != 0) {
		textR2 = totalR2.toFixed(3);
		textCostR2 = rcostR2.toFixed(2);
	}


	var img;
	if ((lastTotal == -1) || (lastTotal == rTotal)) {
		img = '<img src="images/equal.png"></img>';
	}
	else if (rTotal < lastTotal) {
		img = '<img src="images/down.png"></img>';
	}
	else {
		img = '<img src="images/up.png"></img>';
	}

	var monthtxt = addLeadingZeros(parseInt(lastMonth), 2) + ". " + $.t($.monthNames[lastMonth - 1]) + " ";
	monthtxt += '<img src="images/next.png" onclick="ShowP1MonthReport(' + lastMonth + ',' + actYear + ')">';

	var addId = oTable.fnAddData([
		monthtxt,
		totalT1.toFixed(3),
		rcostT1.toFixed(2),
		totalT2.toFixed(3),
		rcostT2.toFixed(2),
		textR1,
		textCostR1,
		textR2,
		textCostR2,
		rTotal.toFixed(2),
		img
	], false);
	return rTotal;
}

function ShowP1YearReport(actYear) {
	if (actYear == 0) {
		actYear = $.actYear;
	}
	else {
		$.actYear = actYear;
	}
	var htmlcontent = '';
	htmlcontent += $('#toptextyear').html();
	htmlcontent += $('#yearreportview').html();

	$($.content).html(htmlcontent);
	$($.content + ' #backbutton').click(function (e) {
		eval($.backfunction)();
	});
	$($.content).i18n();

	$($.content + ' #theader').html(unescape($.devName) + " " + actYear);

	$($.content + ' #comboyear').val(actYear);

	$($.content + ' #comboyear').change(function () {
		OnSelChangeYearP1Report();
	});
	$($.content + ' #comboyear').keypress(function () {
		$(this).change();
	});

	$($.content + ' #yearreport').dataTable({
		"sDom": '<"H"rC>t<"F">',
		"oTableTools": {
			"sRowSelect": "single"
		},
		"aaSorting": [[0, "asc"]],
		"aoColumnDefs": [
			{ "bSortable": false, "aTargets": [10] }
		],
		"bSortClasses": false,
		"bProcessing": true,
		"bStateSave": false,
		"bJQueryUI": true,
		"aLengthMenu": [[50, 100, -1], [50, 100, "All"]],
		"iDisplayLength": 50,
		language: $.DataTableLanguage
	});
	var mTable = $($.content + ' #yearreport');
	var oTable = mTable.dataTable();
	oTable.fnClearTable();

	$.UsageChart = $($.content + ' #usagegraph');
	$.UsageChart.highcharts({
		chart: {
			type: 'column',
			marginRight: 10
		},
		credits: {
			enabled: true,
			href: "http://www.domoticz.com",
			text: "Domoticz.com"
		},
		title: {
			text: ''
		},
		xAxis: {
			type: 'datetime'
		},
		yAxis: {
			title: {
				text: $.t('Energy') + ' (kWh)'
			},
			min: 0
		},
		tooltip: {
			formatter: function () {
				var unit = GetGraphUnit(this.series.name);
				return $.t(Highcharts.dateFormat('%B', this.x)) + '<br/>' + $.t(this.series.name) + ': ' + this.y + ' ' + unit + '<br/>Total: ' + this.point.stackTotal + ' ' + unit;
			}
		},
		plotOptions: {
			column: {
				stacking: 'normal',
				minPointLength: 4,
				pointPadding: 0.1,
				groupPadding: 0
			}
		},
		legend: {
			enabled: true
		}
	});
	var datachartT1 = [];
	var datachartT2 = [];
	var datachartR1 = [];
	var datachartR2 = [];

	var totalT1 = 0;
	var totalT2 = 0;
	var totalR1 = 0;
	var totalR2 = 0;

	var globalT1 = 0;
	var globalT2 = 0;
	var globalR1 = 0;
	var globalR2 = 0;

	var bHaveDelivered = false;

	$.getJSON("json.htm?type=graph&sensor=counter&idx=" + $.devIdx + "&range=year&actyear=" + actYear,
		function (data) {
			bHaveDelivered = (typeof data.delivered != 'undefined');
			if (bHaveDelivered == false) {
				$($.content + ' #dreturn').hide();
			}
			else {
				$($.content + ' #dreturn').show();
			}

			oTable.fnSetColumnVis(5, bHaveDelivered);
			oTable.fnSetColumnVis(6, bHaveDelivered);
			oTable.fnSetColumnVis(7, bHaveDelivered);
			oTable.fnSetColumnVis(8, bHaveDelivered);

			var lastTotal = -1;
			var lastMonth = -1;

			$.each(data.result, function (i, item) {
				var month = parseInt(item.d.substring(5, 7), 10);
				var year = parseInt(item.d.substring(0, 4), 10);

				if (year == actYear) {
					if (lastMonth == -1) {
						lastMonth = month;
					}
					if (lastMonth != month) {
						//add totals to table
						lastTotal = Add2YearTableP1Report(oTable, totalT1, totalT2, totalR1, totalR2, lastTotal, lastMonth, actYear);

						var cdate = Date.UTC(actYear, lastMonth - 1, 1);
						datachartT1.push([cdate, parseFloat(totalT1.toFixed(3))]);
						datachartT2.push([cdate, parseFloat(totalT2.toFixed(3))]);
						datachartR1.push([cdate, parseFloat(totalR1.toFixed(3))]);
						datachartR2.push([cdate, parseFloat(totalR2.toFixed(3))]);

						lastMonth = month;
						globalT1 += totalT1;
						globalT2 += totalT2;
						globalR1 += totalR1;
						globalR2 += totalR2;

						totalT1 = 0;
						totalT2 = 0;
						totalR1 = 0;
						totalR2 = 0;
					}
					var day = parseInt(item.d.substring(8, 10), 10);
					var UsageT1 = 0;
					var UsageT2 = 0;
					var ReturnT1 = 0;
					var ReturnT2 = 0;

					UsageT1 = parseFloat(item.v);
					if (typeof item.v2 != 'undefined') {
						UsageT2 = parseFloat(item.v2);
					}
					if (typeof item.r1 != 'undefined') {
						ReturnT1 = parseFloat(item.r1);
					}
					if (typeof item.r2 != 'undefined') {
						ReturnT2 = parseFloat(item.r2);
					}

					totalT1 += UsageT1;
					totalT2 += UsageT2;
					totalR1 += ReturnT1;
					totalR2 += ReturnT2;
				}
			});

			//add last month
			if ((totalT1 != 0) || (totalT2 != 0) || (totalR1 != 0) || (totalR2 != 0)) {
				lastTotal = Add2YearTableP1Report(oTable, totalT1, totalT2, totalR1, totalR2, lastTotal, lastMonth, actYear);
				var cdate = Date.UTC(actYear, lastMonth - 1, 1);
				datachartT1.push([cdate, parseFloat(totalT1.toFixed(3))]);
				datachartT2.push([cdate, parseFloat(totalT2.toFixed(3))]);
				datachartR1.push([cdate, parseFloat(totalR1.toFixed(3))]);
				datachartR2.push([cdate, parseFloat(totalR2.toFixed(3))]);

				globalT1 += totalT1;
				globalT2 += totalT2;
				globalR1 += totalR1;
				globalR2 += totalR2;
			}

			if (datachartT1.length > 0) {
				if (datachartT2.length > 0) {
					$.UsageChart.highcharts().addSeries({
						id: 'usage1',
						name: $.t('Usage') + ' 1',
						color: 'rgba(60,130,252,0.8)',
						stack: 'susage',
						yAxis: 0
					});
				}
				else {
					$.UsageChart.highcharts().addSeries({
						id: 'usage1',
						name: 'Usage',
						color: 'rgba(3,190,252,0.8)',
						stack: 'susage',
						yAxis: 0
					});
				}
				series = $.UsageChart.highcharts().get('usage1');
				series.setData(datachartT1);
			}
			if (datachartT2.length > 0) {
				$.UsageChart.highcharts().addSeries({
					id: 'usage2',
					name: $.t('Usage') + ' 2',
					color: 'rgba(3,190,252,0.8)',
					stack: 'susage',
					yAxis: 0
				});
				series = $.UsageChart.highcharts().get('usage2');
				series.setData(datachartT2);
			}
			if (bHaveDelivered) {
				if (datachartR1.length > 0) {
					$.UsageChart.highcharts().addSeries({
						id: 'return1',
						name: $.t('Return') + ' 1',
						color: 'rgba(30,242,110,0.8)',
						stack: 'sreturn',
						yAxis: 0
					});
					series = $.UsageChart.highcharts().get('return1');
					series.setData(datachartR1);
				}
				if (datachartR2.length > 0) {
					$.UsageChart.highcharts().addSeries({
						id: 'return2',
						name: $.t('Return') + ' 2',
						color: 'rgba(3,252,190,0.8)',
						stack: 'sreturn',
						yAxis: 0
					});
					series = $.UsageChart.highcharts().get('return2');
					series.setData(datachartR2);
				}
			}

			$($.content + ' #tut1').html(globalT1.toFixed(3));
			$($.content + ' #tut2').html(globalT2.toFixed(3));
			$($.content + ' #trt1').html(globalR1.toFixed(3));
			$($.content + ' #trt2').html(globalR2.toFixed(3));

			$($.content + ' #cntrt1').html($.CounterT1.toFixed(3));
			$($.content + ' #cntrt2').html($.CounterT2.toFixed(3));
			$($.content + ' #cntrr1').html($.CounterR1.toFixed(3));
			$($.content + ' #cntrr2').html($.CounterR2.toFixed(3));

			var gtotal = globalT1 + globalT2;
			var greturn = globalR1 + globalR2;
			$($.content + ' #tu').html(gtotal.toFixed(3));
			$($.content + ' #tr').html(greturn.toFixed(3));
			var montlycosts = (globalT1 * $.costsT1) + (globalT2 * $.costsT2) - (globalR1 * $.costsR1) - (globalR2 * $.costsR2);
			$($.content + ' #mc').html(montlycosts.toFixed(2));

			mTable.fnDraw();
			/* Add a click handler to the rows - this could be used as a callback */
			$($.content + ' #tbody tr').click(function (e) {
				if ($(this).hasClass('row_selected')) {
					$(this).removeClass('row_selected');
				}
				else {
					oTable.$('tr.row_selected').removeClass('row_selected');
					$(this).addClass('row_selected');
				}
			});
		});

	return false;
}

function ShowCounterLog(contentdiv, backfunction, id, name, switchtype) {
	clearInterval($.myglobals.refreshTimer);
	$(window).scrollTop(0);
	$('#modal').show();
	$.content = contentdiv;
	$.backfunction = backfunction;
	$.devIdx = id;
	$.devName = name;
	if (typeof switchtype != 'undefined') {
		$.devSwitchType = switchtype;
	}
	else {
		switchtype = $.devSwitchType;
	}
	var htmlcontent = '';
	htmlcontent = '<p><center><h2>' + unescape(name) + '</h2></center></p>\n';
	htmlcontent += $('#dayweekmonthyearlog').html();
	if ((switchtype == 0) || (switchtype == 1) || (switchtype == 2) || (switchtype == 4)) {
		$.costsT1 = 0.2389;
		$.costsT2 = 0.2389;
		$.costsR1 = 0.08;
		$.costsR2 = 0.08;
		$.costsGas = 0.6218;
		$.costsWater = 1.6473;
		$.DividerWater = 1000;

		$.ajax({
			url: "json.htm?type=command&param=getcosts&idx=" + $.devIdx,
			async: false,
			dataType: 'json',
			success: function (data) {
				$.costsT1 = parseFloat(data.CostEnergy) / 10000;
				$.costsT2 = parseFloat(data.CostEnergyT2) / 10000;
				$.costsR1 = parseFloat(data.CostEnergyR1) / 10000;
				$.costsR2 = parseFloat(data.CostEnergyR2) / 10000;
				$.costsGas = parseFloat(data.CostGas) / 10000;
				$.costsWater = parseFloat(data.CostWater) / 10000;
				$.DividerWater = 1000;//parseFloat(data.DividerWater);
			}
		});

		$.monthNames = ["January", "February", "March", "April", "May", "June",
			"July", "August", "September", "October", "November", "December"];

		var d = new Date();
		var actMonth = d.getMonth() + 1;
		var actYear = d.getYear() + 1900;
		$($.content).html(GetBackbuttonHTMLTableWithRight(backfunction, 'ShowP1YearReportGas(' + actYear + ')', $.t('Report')) + htmlcontent);
	}
	else {
		$($.content).html(GetBackbuttonHTMLTable(backfunction) + htmlcontent);
	}
	$($.content).i18n();

	var graph_title = (switchtype == 4) ? $.t('Generated') : $.t('Usage');
	graph_title += ' ' + Get5MinuteHistoryDaysGraphTitle();

	$.DayChart = $($.content + ' #daygraph');
	$.DayChart.highcharts({
		chart: {
			type: 'column',
			marginRight: 10,
			zoomType: 'x',
			events: {
				load: function () {

					$.getJSON("json.htm?type=graph&sensor=counter&idx=" + id + "&range=day",
						function (data) {
							if (typeof data.result != 'undefined') {
								AddDataToUtilityChart(data, $.DayChart, switchtype);
								$.DayChart.highcharts().redraw();
							}
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
			text: graph_title
		},
		xAxis: {
			type: 'datetime',
			labels: {
				formatter: function () {
					return Highcharts.dateFormat("%H:%M", this.value);
				}
			}
		},
		yAxis: {
			title: {
				text: $.t('Energy') + ' (Wh)'
			}
		},
		tooltip: {
			crosshairs: true,
			shared: true
		},
		plotOptions: {
			series: {
				point: {
					events: {
						click: function (event) {
							chartPointClickNew(event, true, ShowCounterLog);
						}
					}
				}
			},
			column: {
				minPointLength: 4,
				pointPadding: 0.1,
				groupPadding: 0
			}
		},
		legend: {
			enabled: true
		}
	});

	$.WeekChart = $($.content + ' #weekgraph');
	$.WeekChart.highcharts({
		chart: {
			type: 'column',
			marginRight: 10,
			events: {
				load: function () {

					$.getJSON("json.htm?type=graph&sensor=counter&idx=" + id + "&range=week",
						function (data) {
							if (typeof data.result != 'undefined') {
								AddDataToUtilityChart(data, $.WeekChart, switchtype);
								$.WeekChart.highcharts().redraw();
							}
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
			text: $.t('Last Week')
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
				text: $.t('Energy') + ' (kWh)'
			},
			maxPadding: 0.2
		},
		tooltip: {
			crosshairs: true,
			shared: true
		},
		plotOptions: {
			column: {
				minPointLength: 4,
				pointPadding: 0.1,
				groupPadding: 0,
				dataLabels: {
					enabled: true,
					color: 'white'
				}
			}
		},
		legend: {
			enabled: true
		}
	});

	$.MonthChart = $($.content + ' #monthgraph');
	$.MonthChart.highcharts({
		chart: {
			type: 'spline',
			marginRight: 10,
			zoomType: 'x',
			resetZoomButton: {
				position: {
					x: -30,
					y: -36
				}
			},
			events: {
				load: function () {

					$.getJSON("json.htm?type=graph&sensor=counter&idx=" + id + "&range=month",
						function (data) {
							if (typeof data.result != 'undefined') {
								AddDataToUtilityChart(data, $.MonthChart, switchtype);
								$.MonthChart.highcharts().redraw();
							}
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
			text: $.t('Last Month')
		},
		xAxis: {
			type: 'datetime'
		},
		yAxis: {
			title: {
				text: $.t('Energy') + ' (kWh)'
			}
		},
		tooltip: {
			crosshairs: true,
			shared: true
		},
		plotOptions: {
			series: {
				point: {
					events: {
						click: function (event) {
							chartPointClickNewEx(event, false, ShowCounterLog);
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
		},
		legend: {
			enabled: true
		}
	});

	$.YearChart = $($.content + ' #yeargraph');
	$.YearChart.highcharts({
		chart: {
			type: 'spline',
			marginRight: 10,
			zoomType: 'x',
			resetZoomButton: {
				position: {
					x: -30,
					y: -36
				}
			},
			events: {
				load: function () {

					$.getJSON("json.htm?type=graph&sensor=counter&idx=" + id + "&range=year",
						function (data) {
							if (typeof data.result != 'undefined') {
								AddDataToUtilityChart(data, $.YearChart, switchtype);
								$.YearChart.highcharts().redraw();
							}
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
			text: $.t('Last Year')
		},
		xAxis: {
			type: 'datetime'
		},
		yAxis: {
			title: {
				text: $.t('Energy') + ' (kWh)'
			}
		},
		tooltip: {
			crosshairs: true,
			shared: true
		},
		plotOptions: {
			series: {
				point: {
					events: {
						click: function (event) {
							chartPointClickNewEx(event, false, ShowCounterLog);
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
		},
		legend: {
			enabled: true
		}
	});
}

function ShowCounterLogSpline(contentdiv, backfunction, id, name, switchtype) {
	clearInterval($.myglobals.refreshTimer);
	$(window).scrollTop(0);
	$('#modal').show();

	$.content = contentdiv;
	$.backfunction = backfunction;
	$.devIdx = id;
	$.devName = name;
	if (typeof switchtype != 'undefined') {
		$.devSwitchType = switchtype;
	}
	else {
		switchtype = $.devSwitchType;
	}
	var htmlcontent = '';
	htmlcontent = '<p><center><h2>' + unescape(name) + '</h2></center></p>\n';
	htmlcontent += $('#dayweekmonthyearlog').html();

	if ((switchtype == 0) || (switchtype == 1) || (switchtype == 2) || (switchtype == 4)) {
		$.costsT1 = 0.2389;
		$.costsT2 = 0.2389;
		$.costsR1 = 0.08;
		$.costsR2 = 0.08;
		$.costsGas = 0.6218;
		$.costsWater = 1.6473;

		$.ajax({
			url: "json.htm?type=command&param=getcosts&idx=" + $.devIdx,
			async: false,
			dataType: 'json',
			success: function (data) {
				$.costsT1 = parseFloat(data.CostEnergy) / 10000;
				$.costsT2 = parseFloat(data.CostEnergyT2) / 10000;
				$.costsR1 = parseFloat(data.CostEnergyR1) / 10000;
				$.costsR2 = parseFloat(data.CostEnergyR2) / 10000;
				$.costsGas = parseFloat(data.CostGas) / 10000;
				$.costsWater = parseFloat(data.CostWater) / 10000;
			}
		});

		$.monthNames = ["January", "February", "March", "April", "May", "June",
			"July", "August", "September", "October", "November", "December"];

		var d = new Date();
		var actMonth = d.getMonth() + 1;
		var actYear = d.getYear() + 1900;
		$($.content).html(GetBackbuttonHTMLTableWithRight(backfunction, 'ShowP1YearReportGas(' + actYear + ')', $.t('Report')) + htmlcontent);
	}
	else {
		$($.content).html(GetBackbuttonHTMLTable(backfunction) + htmlcontent);
	}
	$($.content).i18n();

	var graph_title = (switchtype == 4) ? $.t('Generated') : $.t('Usage');
	graph_title += ' ' + Get5MinuteHistoryDaysGraphTitle();

	$.DayChart = $($.content + ' #daygraph');
	$.DayChart.highcharts({
		chart: {
			type: 'spline',
			zoomType: 'x',
			resetZoomButton: {
				position: {
					x: -30,
					y: -36
				}
			},
			alignTicks: false, // necessary for y-axis extremes matching
			events: {
				load: function () {
					$.getJSON("json.htm?type=graph&sensor=counter&method=1&idx=" + id + "&range=day",
						function (data) {
							if (typeof data.result != 'undefined') {
								AddDataToUtilityChart(data, $.DayChart, switchtype);
								$.DayChart.highcharts().redraw();
							}
						});
				},
				redraw: function () {
					var me = this, iMin = 0, iMax = 0, bRedraw = false;
					me.redrawCnt = me.redrawCnt || 0; // failsafe
					me.redrawCnt++;
					$.each(me.series, function (iIndex_, oSerie_) {
						var iAxisMin = me.yAxis[oSerie_.options.yAxis].min;
						if (iAxisMin < iMin) {
							bRedraw = bRedraw || (0 != iMin);
							iMin = iAxisMin;
						}
						var iAxisMax = me.yAxis[oSerie_.options.yAxis].max;
						if (iAxisMax > iMax) {
							bRedraw = bRedraw || (0 != iMax);
							iMax = iAxisMax;
						}
					});
					if (bRedraw && me.redrawCnt == 1) {
						$.each($.DayChart.highcharts().yAxis, function (iIndex_, oAxis_) {
							oAxis_.setExtremes((iMin != 0) ? iMin : null, (iMax != 0) ? iMax : null, false);
						});
						me.redraw();
					} else {
						$.each($.DayChart.highcharts().yAxis, function (iIndex_, oAxis_) {
							oAxis_.setExtremes(null, null, false); // next time rescale yaxis if necessary
						});
						me.redrawCnt = 0;
					}
				}
			}
		},
		credits: {
			enabled: true,
			href: "http://www.domoticz.com",
			text: "Domoticz.com"
		},
		title: {
			text: graph_title
		},
		xAxis: {
			type: 'datetime',
		},
		yAxis: [{
			title: {
				text: $.t('Energy') + ' (Wh)'
			}
		}, {
			title: {
				text: $.t('Power') + ' (Watt)'
			},
			opposite: true
		}],
		tooltip: {
			crosshairs: true,
			shared: false
		},
		plotOptions: {
			series: {
				point: {
					events: {
						click: function (event) {
							chartPointClickNew(event, true, ShowCounterLogSpline);
						}
					}
				},
				matchExtremes: true
			},
			spline: {
				lineWidth: 3,
				states: {
					hover: {
						lineWidth: 3
					}
				},
				marker: {
					states: {
						hover: {
							enabled: true,
							symbol: 'circle',
							radius: 5,
							lineWidth: 1
						}
					}
				}
			},
			areaspline: {
				lineWidth: 3,
				marker: {
					enabled: false
				},
				states: {
					hover: {
						lineWidth: 3
					}
				}
			},
			column: {
				minPointLength: 4,
				pointPadding: 0.1,
				groupPadding: 0,
				dataLabels: {
					enabled: false,
					color: 'white'
				}
			}
		},
		legend: {
			enabled: true
		}
	});
	$.WeekChart = $($.content + ' #weekgraph');
	$.WeekChart.highcharts({
		chart: {
			type: 'column',
			marginRight: 10,
			events: {
				load: function () {

					$.getJSON("json.htm?type=graph&sensor=counter&idx=" + id + "&range=week",
						function (data) {
							if (typeof data.result != 'undefined') {
								AddDataToUtilityChart(data, $.WeekChart, switchtype);
								$.WeekChart.highcharts().redraw();
							}
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
			text: $.t('Last Week')
		},
		xAxis: {
			type: 'datetime',
			dateTimeLabelFormats: {
				day: '%a'
			},
			tickInterval: 24 * 3600 * 1000
		},
		yAxis: {
			maxPadding: 0.2,
			title: {
				text: $.t('Energy') + ' (kWh)'
			}
		},
		plotOptions: {
			column: {
				minPointLength: 4,
				pointPadding: 0.1,
				groupPadding: 0,
				dataLabels: {
					enabled: true,
					color: 'white'
				}
			}
		},
		legend: {
			enabled: true
		}
	});

	$.MonthChart = $($.content + ' #monthgraph');
	$.MonthChart.highcharts({
		chart: {
			type: 'spline',
			marginRight: 10,
			zoomType: 'x',
			resetZoomButton: {
				position: {
					x: -30,
					y: -36
				}
			},
			events: {
				load: function () {

					$.getJSON("json.htm?type=graph&sensor=counter&idx=" + id + "&range=month",
						function (data) {
							if (typeof data.result != 'undefined') {
								AddDataToUtilityChart(data, $.MonthChart, switchtype);
								$.MonthChart.highcharts().redraw();
							}
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
			text: $.t('Last Month')
		},
		xAxis: {
			type: 'datetime'
		},
		yAxis: {
			title: {
				text: $.t('Energy') + ' (kWh)'
			}
		},
		tooltip: {
			crosshairs: true,
			shared: true
		},
		plotOptions: {
			series: {
				point: {
					events: {
						click: function (event) {
							chartPointClickNewEx(event, false, ShowCounterLogSpline);
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
		},
		legend: {
			enabled: true
		}
	});

	$.YearChart = $($.content + ' #yeargraph');
	$.YearChart.highcharts({
		chart: {
			type: 'spline',
			marginRight: 10,
			zoomType: 'x',
			resetZoomButton: {
				position: {
					x: -30,
					y: -36
				}
			},
			events: {
				load: function () {

					$.getJSON("json.htm?type=graph&sensor=counter&idx=" + id + "&range=year",
						function (data) {
							if (typeof data.result != 'undefined') {
								AddDataToUtilityChart(data, $.YearChart, switchtype);
								$.YearChart.highcharts().redraw();
							}
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
			text: $.t('Last Year')
		},
		xAxis: {
			type: 'datetime'
		},
		yAxis: {
			title: {
				text: $.t('Energy') + ' (kWh)'
			}
		},
		tooltip: {
			crosshairs: true,
			shared: true
		},
		plotOptions: {
			series: {
				point: {
					events: {
						click: function (event) {
							chartPointClickNewEx(event, false, ShowCounterLogSpline);
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
		},
		legend: {
			enabled: true
		}
	});
}

function ShowUsageLog(contentdiv, backfunction, id, name) {
	clearInterval($.myglobals.refreshTimer);
	$(window).scrollTop(0);
	$('#modal').show();
	$.content = contentdiv;
	$.backfunction = backfunction;
	$.devIdx = id;
	$.devName = name;
	var htmlcontent = '';
	htmlcontent = '<p><center><h2>' + unescape(name) + '</h2></center></p>\n';
	htmlcontent += $('#daymonthyearlog').html();
	$($.content).html(GetBackbuttonHTMLTable(backfunction) + htmlcontent);
	$($.content).i18n();

	$.DayChart = $($.content + ' #daygraph');
	$.DayChart.highcharts({
		chart: {
			type: 'spline',
			zoomType: 'x',
			resetZoomButton: {
				position: {
					x: -30,
					y: -36
				}
			},
			marginRight: 10,
			events: {
				load: function () {
					$.getJSON("json.htm?type=graph&sensor=counter&idx=" + id + "&range=day",
						function (data) {
							if (typeof data.result != 'undefined') {
								var series = $.DayChart.highcharts().series[0];
								var datatable = [];

								$.each(data.result, function (i, item) {
									datatable.push([GetUTCFromString(item.d), parseFloat(item.u)]);
								});
								series.setData(datatable);
							}
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
			text: $.t('Usage') + ' ' + Get5MinuteHistoryDaysGraphTitle()
		},
		xAxis: {
			type: 'datetime'
		},
		yAxis: {
			title: {
				text: $.t('Usage') + ' (Watt)'
			},
			min: 0
		},
		tooltip: {
			crosshairs: true,
			shared: true
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
			id: 'usage',
			name: $.t('Usage'),
			tooltip: {
				valueSuffix: ' Watt',
				valueDecimals: 1
			},
			point: {
				events: {
					click: function (event) {
						chartPointClickNew(event, true, ShowUsageLog);
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

	$.MonthChart = $($.content + ' #monthgraph');
	$.MonthChart.highcharts({
		chart: {
			type: 'spline',
			zoomType: 'x',
			resetZoomButton: {
				position: {
					x: -30,
					y: -36
				}
			},
			marginRight: 10,
			events: {
				load: function () {

					$.getJSON("json.htm?type=graph&sensor=counter&idx=" + id + "&range=month",
						function (data) {
							if (typeof data.result != 'undefined') {
								var datatable1 = [];
								var datatable2 = [];

								$.each(data.result, function (i, item) {
									datatable1.push([GetDateFromString(item.d), parseFloat(item.u_min)]);
									datatable2.push([GetDateFromString(item.d), parseFloat(item.u_max)]);
								});
								var series1 = $.MonthChart.highcharts().series[0];
								var series2 = $.MonthChart.highcharts().series[1];
								series1.setData(datatable1);
								series2.setData(datatable2);
							}
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
			text: $.t('Usage') + ' ' + $.t('Last Month')
		},
		xAxis: {
			type: 'datetime'
		},
		yAxis: {
			title: {
				text: $.t('Usage') + ' (Watt)'
			},
			min: 0
		},
		tooltip: {
			crosshairs: true,
			shared: true
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
			id: 'usage_min',
			name: $.t('Usage') + ' min',
			tooltip: {
				valueSuffix: ' Watt',
				valueDecimals: 1
			},
			point: {
				events: {
					click: function (event) {
						chartPointClickNew(event, false, ShowUsageLog);
					}
				}
			}
		}, {
			id: 'usage_max',
			name: $.t('Usage') + ' max',
			tooltip: {
				valueSuffix: ' Watt',
				valueDecimals: 1
			},
			point: {
				events: {
					click: function (event) {
						chartPointClickNew(event, false, ShowUsageLog);
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
			zoomType: 'x',
			resetZoomButton: {
				position: {
					x: -30,
					y: -36
				}
			},
			marginRight: 10,
			events: {
				load: function () {

					$.getJSON("json.htm?type=graph&sensor=counter&idx=" + id + "&range=year",
						function (data) {
							if (typeof data.result != 'undefined') {
								var datatable1 = [];
								var datatable2 = [];

								$.each(data.result, function (i, item) {
									datatable1.push([GetDateFromString(item.d), parseFloat(item.u_min)]);
									datatable2.push([GetDateFromString(item.d), parseFloat(item.u_max)]);
								});
								var series1 = $.YearChart.highcharts().series[0];
								var series2 = $.YearChart.highcharts().series[1];
								series1.setData(datatable1);
								series2.setData(datatable2);
							}
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
			text: $.t('Usage') + ' ' + $.t('Last Year')
		},
		xAxis: {
			type: 'datetime'
		},
		yAxis: {
			title: {
				text: $.t('Usage') + ' (Watt)'
			},
			min: 0
		},
		tooltip: {
			crosshairs: true,
			shared: true
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
			id: 'usage_min',
			name: $.t('Usage') + ' min',
			tooltip: {
				valueSuffix: ' Watt',
				valueDecimals: 1
			},
			point: {
				events: {
					click: function (event) {
						chartPointClickNew(event, false, ShowUsageLog);
					}
				}
			}
		}, {
			id: 'usage_max',
			name: $.t('Usage') + ' max',
			tooltip: {
				valueSuffix: ' Watt',
				valueDecimals: 1
			},
			point: {
				events: {
					click: function (event) {
						chartPointClickNew(event, false, ShowUsageLog);
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

function ShowLuxLog(contentdiv, backfunction, id, name) {
	clearInterval($.myglobals.refreshTimer);
	$(window).scrollTop(0);
	$('#modal').show();
	$.content = contentdiv;
	$.backfunction = backfunction;
	$.devIdx = id;
	$.devName = name;
	var htmlcontent = '';
	htmlcontent = '<p><center><h2>' + unescape(name) + '</h2></center></p>\n';
	htmlcontent += $('#daymonthyearlog').html();
	$($.content).html(GetBackbuttonHTMLTable(backfunction) + htmlcontent);
	$($.content).i18n();

	$.DayChart = $($.content + ' #daygraph');
	$.DayChart.highcharts({
		chart: {
			type: 'spline',
			zoomType: 'x',
			resetZoomButton: {
				position: {
					x: -30,
					y: -36
				}
			},
			marginRight: 10,
			events: {
				load: function () {

					$.getJSON("json.htm?type=graph&sensor=counter&idx=" + id + "&range=day",
						function (data) {
							if (typeof data.result != 'undefined') {
								var series = $.DayChart.highcharts().series[0];
								var datatable = [];

								$.each(data.result, function (i, item) {
									datatable.push([GetUTCFromString(item.d), parseInt(item.lux)]);
								});
								series.setData(datatable);
							}
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
			text: $.t('Lux') + ' ' + Get5MinuteHistoryDaysGraphTitle()
		},
		xAxis: {
			type: 'datetime'
		},
		yAxis: {
			title: {
				text: $.t('Lux')
			},
			min: 0,
			minorGridLineWidth: 0,
			alternateGridColor: null
		},
		tooltip: {
			formatter: function () {
				return '' +
					$.t(Highcharts.dateFormat('%A', this.x)) + '<br/>' + Highcharts.dateFormat('%Y-%m-%d %H:%M', this.x) + ': ' + this.y + ' ' + $.t('Lux');
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
			id: 'lux',
			name: $.t('Lux'),
			events: {
				click: function (event) {
					chartPointClickNew(event, true, ShowLuxLog);
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

	$.MonthChart = $($.content + ' #monthgraph');
	$.MonthChart.highcharts({
		chart: {
			type: 'spline',
			zoomType: 'x',
			resetZoomButton: {
				position: {
					x: -30,
					y: -36
				}
			},
			marginRight: 10,
			events: {
				load: function () {

					$.getJSON("json.htm?type=graph&sensor=counter&idx=" + id + "&range=month",
						function (data) {
							if (typeof data.result != 'undefined') {
								var datatable1 = [];
								var datatable2 = [];
								var datatable3 = [];
								var minValue = 10000000;

								$.each(data.result, function (i, item) {
									datatable1.push([GetDateFromString(item.d), parseInt(item.lux_min)]);
									datatable2.push([GetDateFromString(item.d), parseInt(item.lux_max)]);
									datatable3.push([GetDateFromString(item.d), parseFloat(item.lux_avg)]);
									minValue = Math.min(item.lux_min, minValue);
								});
								$.MonthChart.highcharts().yAxis[0].update({ min: minValue });
								var series1 = $.MonthChart.highcharts().series[0];
								var series2 = $.MonthChart.highcharts().series[1];
								var series3 = $.MonthChart.highcharts().series[2];
								series1.setData(datatable1, false);
								series2.setData(datatable2, false);
								series3.setData(datatable3, false);
								$.MonthChart.highcharts().redraw();
							}
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
			text: $.t('Lux') + ' ' + $.t('Last Month')
		},
		xAxis: {
			type: 'datetime'
		},
		yAxis: {
			title: {
				text: $.t('Lux')
			},
			min: 0,
			minorGridLineWidth: 0,
			alternateGridColor: null
		},
		tooltip: {
			crosshairs: true,
			shared: true
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
			id: 'lux_min',
			name: 'min',
			tooltip: {
				valueSuffix: ' lux',
				valueDecimals: 0
			},
			point: {
				events: {
					click: function (event) {
						chartPointClickNew(event, false, ShowLuxLog);
					}
				}
			}
		}, {
			id: 'lux_max',
			name: 'max',
			tooltip: {
				valueSuffix: ' lux',
				valueDecimals: 0
			},
			point: {
				events: {
					click: function (event) {
						chartPointClickNew(event, false, ShowLuxLog);
					}
				}
			}
		}, {
			id: 'lux_avg',
			name: 'avg',
			tooltip: {
				valueSuffix: ' lux',
				valueDecimals: 0
			},
			point: {
				events: {
					click: function (event) {
						chartPointClickNew(event, false, ShowLuxLog);
					}
				}
			}
		}],
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
			zoomType: 'x',
			resetZoomButton: {
				position: {
					x: -30,
					y: -36
				}
			},
			marginRight: 10,
			events: {
				load: function () {

					$.getJSON("json.htm?type=graph&sensor=counter&idx=" + id + "&range=year",
						function (data) {
							if (typeof data.result != 'undefined') {
								var datatable1 = [];
								var datatable2 = [];
								var datatable3 = [];
								var minValue = 10000000;

								$.each(data.result, function (i, item) {
									datatable1.push([GetDateFromString(item.d), parseInt(item.lux_min)]);
									datatable2.push([GetDateFromString(item.d), parseInt(item.lux_max)]);
									datatable3.push([GetDateFromString(item.d), parseFloat(item.lux_avg)]);
									minValue = Math.min(item.lux_min, minValue);
								});
								$.YearChart.highcharts().yAxis[0].update({ min: minValue });
								var series1 = $.YearChart.highcharts().series[0];
								var series2 = $.YearChart.highcharts().series[1];
								var series3 = $.YearChart.highcharts().series[2];
								series1.setData(datatable1, false);
								series2.setData(datatable2, false);
								series3.setData(datatable3, false);
								$.YearChart.highcharts().redraw();
							}
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
			text: $.t('Lux') + ' ' + $.t('Last Year')
		},
		xAxis: {
			type: 'datetime'
		},
		yAxis: {
			title: {
				text: $.t('Lux')
			},
			min: 0,
			minorGridLineWidth: 0,
			alternateGridColor: null
		},
		tooltip: {
			crosshairs: true,
			shared: true
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
			id: 'lux_min',
			name: 'min',
			tooltip: {
				valueSuffix: ' lux',
				valueDecimals: 0
			},
			point: {
				events: {
					click: function (event) {
						chartPointClickNew(event, false, ShowLuxLog);
					}
				}
			}
		}, {
			id: 'lux_max',
			name: 'max',
			tooltip: {
				valueSuffix: ' lux',
				valueDecimals: 0
			},
			point: {
				events: {
					click: function (event) {
						chartPointClickNew(event, false, ShowLuxLog);
					}
				}
			}
		}, {
			id: 'lux_avg',
			name: 'avg',
			tooltip: {
				valueSuffix: ' lux',
				valueDecimals: 0
			},
			point: {
				events: {
					click: function (event) {
						chartPointClickNew(event, false, ShowLuxLog);
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

function SwitchLightPopup(idx, switchcmd, refreshfunction, isprotected) {
	SwitchLight(idx, switchcmd, refreshfunction, isprotected);
	$("#rgbw_popup").hide();
}

function isLED(SubType) {
	return (SubType.indexOf("RGB") >= 0 || SubType.indexOf("WW") >= 0);
}

function getLEDType(SubType) {
	var LEDType = {bIsLED: false, bHasRGB: false, bHasWhite:false, bHasTemperature:false, bHasCustom:false};
	LEDType.bIsLED = (SubType.indexOf("RGB") >= 0 || SubType.indexOf("WW") >= 0);
	LEDType.bHasRGB = (SubType.indexOf("RGB") >= 0);
	LEDType.bHasWhite = (SubType.indexOf("W") >= 0);
	LEDType.bHasTemperature = (SubType.indexOf("WW") >= 0);
	LEDType.bHasCustom = (SubType.indexOf("RGBWZ") >= 0 || SubType.indexOf("RGBWWZ") >= 0);

	return LEDType;
}

function ShowRGBWPicker(selector, idx, Protected, MaxDimLevel, LevelInt, colorJSON, iSubType, callback) {
	
	var color = {};
	var devIdx = idx;
	var SubType = iSubType;
	var LEDType = getLEDType(SubType);

	try {
		color = JSON.parse(colorJSON);
	}
	catch(e) {
		// forget about it :)
	}
	var colorPickerMode = "color"; // Default
	
	// TODO: A little bit hackish, maybe extend the wheelColorPicker instead..
	$(selector + ' #popup_picker')[0].getJSONColor = function() {
		var colorJSON = ""; // Empty string, intentionally illegal JSON
		var fcolor = $(this).wheelColorPicker('getColor'); // Colors as floats 0..1
		if (colorPickerMode == "white") {
			var color = {m:1, t:0, r:0, g:0, b:0, cw:255, ww:255};
			colorJSON = JSON.stringify(color);
		}
		if (colorPickerMode == "temperature") {
			var color = {m:2, t:Math.round(fcolor.t*255), r:0, g:0, b:0, cw:Math.round((1-fcolor.t)*255), ww:Math.round(fcolor.t*255)};
			colorJSON = JSON.stringify(color);
		}
		else if (colorPickerMode == "color") {
			// Set value to 1 in color mode
			$(this).wheelColorPicker('setHsv', fcolor.h, fcolor.s, 1);
			fcolor = $(this).wheelColorPicker('getColor'); // Colors as floats 0..1
			var color = {m:3, t:0, r:Math.round(fcolor.r*255), g:Math.round(fcolor.g*255), b:Math.round(fcolor.b*255), cw:0, ww:0};
			colorJSON = JSON.stringify(color);
		}
		else if (colorPickerMode == "customw") {
			var color = {m:4, t:0, r:Math.round(fcolor.r*255), g:Math.round(fcolor.g*255), b:Math.round(fcolor.b*255), cw:Math.round(fcolor.w*255), ww:Math.round(fcolor.w*255)};
			colorJSON = JSON.stringify(color);
		}
		else if (colorPickerMode == "customww") {
			var color = {m:4, t:Math.round(fcolor.t*255), r:Math.round(fcolor.r*255), g:Math.round(fcolor.g*255), b:Math.round(fcolor.b*255), cw:Math.round(fcolor.w*(1-fcolor.t)*255), ww:Math.round(fcolor.w*fcolor.t*255)};
			colorJSON = JSON.stringify(color);
		}
		return colorJSON;
	}

	function UpdateColorPicker(mode)
	{
		colorPickerMode = mode;
		if (mode == "color") {
			$(selector + ' #popup_picker').wheelColorPicker('setOptions', {sliders:'wm', preserveWheel:true});
		}
		else if (mode == "white") {
			$(selector + ' #popup_picker').wheelColorPicker('setOptions', {sliders:'m', preserveWheel:true});
		}
		else if (mode == "temperature") {
			$(selector + ' #popup_picker').wheelColorPicker('setOptions', {sliders:'xm'});
		}
		else if (mode == "customw") {
			$(selector + ' #popup_picker').wheelColorPicker('setOptions', {sliders:'wvlm', preserveWheel:false});
		}
		else if (mode == "customww") {
			$(selector + ' #popup_picker').wheelColorPicker('setOptions', {sliders:'wvklm', preserveWheel:false});
		}

		$(selector + ' .pickermodergb').hide();
		$(selector + ' .pickermodewhite').hide();
		$(selector + ' .pickermodetemp').hide();
		$(selector + ' .pickermodecustomw').hide();
		$(selector + ' .pickermodecustomww').hide();
		// Show buttons for choosing input mode
		if (LEDType.bHasRGB) {
			if (mode == "color") {
				$(selector + ' .pickermodergb.selected').show();
			}
			else {
				$(selector + ' .pickermodergb.unselected').show();
			}
		}
		if (LEDType.bHasWhite && !LEDType.bHasTemperature) {
			if (mode == "white") {
				$(selector + ' .pickermodewhite.selected').show();
			}
			else {
				$(selector + ' .pickermodewhite.unselected').show();
			}
		}
		if (LEDType.bHasTemperature) {
			if (mode == "temperature") {
				$(selector + ' .pickermodetemp.selected').show();
			}
			else {
				$(selector + ' .pickermodetemp.unselected').show();
			}
		}
		if (LEDType.bHasCustom && !LEDType.bHasTemperature) {
			if (mode == "customw") {
				$(selector + ' .pickermodecustomw.selected').show();
			}
			else {
				$(selector + ' .pickermodecustomw.unselected').show();
			}
		}
		if (LEDType.bHasCustom && LEDType.bHasTemperature) {
			if (mode == "customww") {
				$(selector + ' .pickermodecustomww.selected').show();
			}
			else {
				$(selector + ' .pickermodecustomww.unselected').show();
			}
		}

		$(selector + ' #popup_picker').wheelColorPicker('refreshWidget');
		$(selector + ' #popup_picker').wheelColorPicker('updateSliders');
		$(selector + ' #popup_picker').wheelColorPicker('redrawSliders');
	}
	
	/**enum ColorMode {
		ColorModeNone = 0, // Illegal
		ColorModeWhite,    // White. Valid fields: none
		ColorModeTemp,     // White with color temperature. Valid fields: t
		ColorModeRGB,      // Color. Valid fields: r, g, b
		ColorModeCustom,   // Custom (color + white). Valid fields: r, g, b, cw, ww, depending on device capabilities
	};*/

	var color_m = (color.m==null)?3:color.m; // Default to 3: ColorModeRGB

	if (color_m != 1 && color_m != 2 && color_m != 3 && color_m != 4) color_m = 3; // Default to RGB if not valid
	if (color_m == 4 && !LEDType.bHasCustom) color_m = 3; // Default to RGB if light does not support custom color
	if (color_m == 1 && !LEDType.bHasWhite) color_m = 3; // Default to RGB if light does not support white
	if (color_m == 2 && !LEDType.bHasTemperature) color_m = 3; // Default to RGB if light does not support temperature

	var color_t = 128;
	var color_cw = 128;
	var color_ww = 255 - color_cw;
	var color_r = 255;
	var color_g = 255;
	var color_b = 255;

	if (color_m == 1) // White
	{
		// Nothing..
	}
	if (color_m == 2) // White with temperature
	{
		color_t = (color.t==null)?128:color.t;
		color_cw = (color.cw==null)?128:color.cw;
		color_ww = (color.ww==null)?255 - color_cw:color.ww;
	}
	if (color_m == 3) // Color
	{
		color_r = (color.r==null)?255:color.r;
		color_g = (color.g==null)?255:color.g;
		color_b = (color.b==null)?255:color.b;
	}
	if (color_m == 4) // Custom
	{
		color_t = (color.t==null)?128:color.t;
		color_cw = (color.cw==null)?128:color.cw;
		color_ww = (color.ww==null)?255 - color_cw:color.ww;
		color_r = (color.r==null)?255:color.r;
		color_g = (color.g==null)?255:color.g;
		color_b = (color.b==null)?255:color.b;
	}

	if (color_m == 1) { // White mode
		colorPickerMode = "white";
	}
	if (color_m == 2) { // Color temperature mode
		colorPickerMode = "temperature";
	}
	else if (color_m == 3){ // Color  mode
		colorPickerMode = "color";
	}
	else if (color_m == 4){ // Custom  mode
		colorPickerMode = "customw";
		if (LEDType.bHasTemperature) {
			colorPickerMode = "customww";
		}
	}
	
	// Update color picker controls
	UpdateColorPicker(colorPickerMode);
	
	$(selector + ' .pickermodergb').off().click(function(){
		UpdateColorPicker("color");
	});
	$(selector + ' .pickermodewhite').off().click(function(){
		UpdateColorPicker("white");
	});
	$(selector + ' .pickermodetemp').off().click(function(){
		UpdateColorPicker("temperature");
	});
	$(selector + ' .pickermodecustomw').off().click(function(){
		UpdateColorPicker("customw");
	});
	$(selector + ' .pickermodecustomww').off().click(function(){
		UpdateColorPicker("customww");
	});

	$(selector + ' #popup_picker').wheelColorPicker('setTemperature', color_t/255);
	$(selector + ' #popup_picker').wheelColorPicker('setWhite', color_cw/255+color_ww/255);
	$(selector + ' #popup_picker').wheelColorPicker('setRgb', color_r/255, color_g/255, color_b/255);
	$(selector + ' #popup_picker').wheelColorPicker('setMaster', LevelInt/MaxDimLevel);
	
	$(selector + ' #popup_picker').off('slidermove sliderup').on('slidermove sliderup', function() {
		clearTimeout($.setColValue);

		var color = $(this).wheelColorPicker('getColor');
		var dimlevel = Math.round((color.m*99)+1); // 1..100
		var JSONColor = $(selector + ' #popup_picker')[0].getJSONColor();
		//TODO: Rate limit instead of debounce
		$.setColValue = setTimeout(function () {
			var fn = callback || SetColValue;
			fn(devIdx, JSONColor, dimlevel);
		}, 400);
	});
}

function ShowRGBWPopupInt(mouseX, mouseY, idx, refreshfunction, Protected, MaxDimLevel, LevelInt, colorJSON, SubType) {

	ShowRGBWPicker("#rgbw_popup", idx, Protected, MaxDimLevel, LevelInt, colorJSON, SubType);

	// Setup on and Off buttons
	$('#rgbw_popup #popup_switch_on').attr("href", 'javascript:SwitchLightPopup(' + idx + ',\'On\',' + refreshfunction + ',' + Protected + ');');
	$('#rgbw_popup #popup_switch_off').attr("href", 'javascript:SwitchLightPopup(' + idx + ',\'Off\',' + refreshfunction + ',' + Protected + ');');

	$("#rgbw_popup").css({
		"top": mouseY,
		"left": mouseX + 15
	});
	$("#rgbw_popup").show();
	// Update color picker after popup is shown
	$('#rgbw_popup #popup_picker').wheelColorPicker('updateSliders');
	$('#rgbw_popup #popup_picker').wheelColorPicker('redrawSliders');
}
function CloseRGBWPopup() {
	$("#rgbw_popup").hide();
}
function ShowRGBWPopup(event, idx, refreshfunction, Protected, MaxDimLevel, LevelInt, color, SubType) {
	clearInterval($.setColValue);
	var event = event || window.event;
	// If pageX/Y aren't available and clientX/Y are,
	// calculate pageX/Y - logic taken from jQuery.
	// (This is to support old IE)
	if (event.pageX == null && event.clientX != null) {
		eventDoc = (event.target && event.target.ownerDocument) || document;
		doc = eventDoc.documentElement;
		body = eventDoc.body;

		event.pageX = event.clientX +
			(doc && doc.scrollLeft || body && body.scrollLeft || 0) -
			(doc && doc.clientLeft || body && body.clientLeft || 0);
		event.pageY = event.clientY +
			(doc && doc.scrollTop || body && body.scrollTop || 0) -
			(doc && doc.clientTop || body && body.clientTop || 0);
	}
	var mouseX = event.pageX;
	var mouseY = event.pageY;

	HandleProtection(Protected, function () {
		ShowRGBWPopupInt(mouseX, mouseY, idx, refreshfunction, Protected, MaxDimLevel, LevelInt, color, SubType);
	});
}

function SwitchTherm3Popup(idx, switchcmd, refreshfunction) {
	SwitchLightInt(idx, switchcmd, refreshfunction, $.devpwd);
	$("#thermostat3_popup").hide();
}
function ShowTherm3PopupInt(mouseX, mouseY, idx, refreshfunction, pwd) {
	$.devIdx = idx;
	$.devpwd = pwd;
	$('#thermostat3_popup #popup_therm_on').attr("href", 'javascript:SwitchTherm3Popup(' + idx + ',\'On\',' + refreshfunction + ');');
	$('#thermostat3_popup #popup_therm_off').attr("href", 'javascript:SwitchTherm3Popup(' + idx + ',\'Off\',' + refreshfunction + ');');

	$("#thermostat3_popup").css({
		"top": mouseY,
		"left": mouseX + 15
	});
	$("#thermostat3_popup").show();
}
function CloseTherm3Popup() {
	$("#thermostat3_popup").hide();
}

function ThermUp() {
	$.ajax({
		url: "json.htm?type=command&param=switchlight&idx=" + $.devIdx +
		"&switchcmd=Up" +
		"&level=0",
		async: false,
		dataType: 'json',
		success: function (data) {
			if (data.status == "ERROR") {
				HideNotify();
				bootbox.alert($.t(data.message));
			}
		},
		error: function () {
			HideNotify();
			bootbox.alert($.t('Problem sending switch command'));
		}
	});
}

function ThermDown() {
	$.ajax({
		url: "json.htm?type=command&param=switchlight&idx=" + $.devIdx +
		"&switchcmd=Down" +
		"&level=0",
		async: false,
		dataType: 'json',
		success: function (data) {
			if (data.status == "ERROR") {
				HideNotify();
				bootbox.alert($.t(data.message));
			}
		},
		error: function () {
			HideNotify();
			bootbox.alert($.t('Problem sending switch command'));
		}
	});
}

function ThermUp2() {
	$.ajax({
		url: "json.htm?type=command&param=switchlight&idx=" + $.devIdx +
		"&switchcmd=Run Up" +
		"&level=0",
		async: false,
		dataType: 'json',
		success: function (data) {
			if (data.status == "ERROR") {
				HideNotify();
				bootbox.alert($.t(data.message));
			}
		},
		error: function () {
			HideNotify();
			bootbox.alert($.t('Problem sending switch command'));
		}
	});
}

function ThermDown2() {
	$.ajax({
		url: "json.htm?type=command&param=switchlight&idx=" + $.devIdx +
		"&switchcmd=Run Down" +
		"&level=0",
		async: false,
		dataType: 'json',
		success: function (data) {
			if (data.status == "ERROR") {
				HideNotify();
				bootbox.alert($.t(data.message));
			}
		},
		error: function () {
			HideNotify();
			bootbox.alert($.t('Problem sending switch command'));
		}
	});
}

function ThermStop() {
	$.ajax({
		url: "json.htm?type=command&param=switchlight&idx=" + $.devIdx +
		"&switchcmd=Stop" +
		"&level=0",
		async: false,
		dataType: 'json',
		success: function (data) {
			if (data.status == "ERROR") {
				HideNotify();
				bootbox.alert($.t(data.message));
			}
		},
		error: function () {
			HideNotify();
			bootbox.alert($.t('Problem sending switch command'));
		}
	});
}

function ShowTherm3Popup(event, idx, refreshfunction, Protected, MaxDimLevel, LevelInt, hue) {
	clearInterval($.setColValue);
	var event = event || window.event;
	// If pageX/Y aren't available and clientX/Y are,
	// calculate pageX/Y - logic taken from jQuery.
	// (This is to support old IE)
	if (event.pageX == null && event.clientX != null) {
		eventDoc = (event.target && event.target.ownerDocument) || document;
		doc = eventDoc.documentElement;
		body = eventDoc.body;

		event.pageX = event.clientX +
			(doc && doc.scrollLeft || body && body.scrollLeft || 0) -
			(doc && doc.clientLeft || body && body.clientLeft || 0);
		event.pageY = event.clientY +
			(doc && doc.scrollTop || body && body.scrollTop || 0) -
			(doc && doc.clientTop || body && body.clientTop || 0);
	}
	var mouseX = event.pageX;
	var mouseY = event.pageY;

	HandleProtection(Protected, function (pwd) {
		ShowTherm3PopupInt(mouseX, mouseY, idx, refreshfunction, pwd);
	});
}


function CloseSetpointPopup() {
	$("#setpoint_popup").hide();
}

function SetpointUp() {
	var curValue = parseFloat($('#setpoint_popup #popup_setpoint').val());
	curValue += 0.5;
	curValue = Math.round(curValue / 0.5) * 0.5;
	var curValueStr = curValue.toFixed(1);
	$('#setpoint_popup #popup_setpoint').val(curValueStr);
}

function SetpointDown() {
	var curValue = parseFloat($('#setpoint_popup #popup_setpoint').val());
	curValue -= 0.5;
	curValue = Math.round(curValue / 0.5) * 0.5;
	if (curValue < 0) {
		curValue = 0;
	}
	var curValueStr = curValue.toFixed(1);
	$('#setpoint_popup #popup_setpoint').val(curValueStr);
}

function SetSetpoint() {
	var curValue = parseFloat($('#setpoint_popup #popup_setpoint').val());
	$.ajax({
		url: "json.htm?type=command&param=setsetpoint&idx=" + $.devIdx +
		"&setpoint=" + curValue,
		async: false,
		dataType: 'json',
		success: function (data) {
			CloseSetpointPopup();
			if (data.status == "ERROR") {
				HideNotify();
				bootbox.alert($.t('Problem setting Setpoint value'));
			}
			//wait 1 second
			setTimeout(function () {
				HideNotify();
				$.refreshfunction();
			}, 1000);
		},
		error: function () {
			HideNotify();
			bootbox.alert($.t('Problem setting Setpoint value'));
		}
	});
}

function RFYEnableSunWind(bDoEnable) {
	var switchcmd = "EnableSunWind";
	if (bDoEnable == false) {
		switchcmd = "DisableSunWind";
	}
	$("#rfy_popup").hide();
	SwitchLight($.devIdx, switchcmd, $.refreshfunction, $.Protected);
}

function ShowSetpointPopupInt(mouseX, mouseY, idx, refreshfunction, currentvalue, ismobile) {
	$.devIdx = idx;
	$.refreshfunction = refreshfunction;
	var curValue = parseFloat(currentvalue).toFixed(1);
	$('#setpoint_popup #actual_value').html(curValue);
	$('#setpoint_popup #popup_setpoint').val(curValue);

	if (typeof ismobile == 'undefined') {
		$("#setpoint_popup").css({
			"top": mouseY,
			"left": mouseX + 15,
			"position": "absolute",
			"-ms-transform": "none",
			"-moz-transform": "none",
			"-webkit-transform": "none",
			"transform": "none"
		});
	}
	else {
		$("#setpoint_popup").css({
			"position": "fixed",
			"left": "50%",
			"top": "50%",
			"-ms-transform": "translate(-50%,-50%)",
			"-moz-transform": "translate(-50%,-50%)",
			"-webkit-transform": "translate(-50%,-50%)",
			"transform": "translate(-50%,-50%)"
		});
	}
	$('#setpoint_popup').i18n();
	$("#setpoint_popup").show();
}

function ShowSetpointPopup(event, idx, refreshfunction, Protected, currentvalue, ismobile) {
	$.Protected = Protected;
	event = event || window.event;
	// If pageX/Y aren't available and clientX/Y are,
	// calculate pageX/Y - logic taken from jQuery.
	// (This is to support old IE)
	if (event.pageX == null && event.clientX != null) {
		eventDoc = (event.target && event.target.ownerDocument) || document;
		doc = eventDoc.documentElement;
		body = eventDoc.body;

		event.pageX = event.clientX +
			(doc && doc.scrollLeft || body && body.scrollLeft || 0) -
			(doc && doc.clientLeft || body && body.clientLeft || 0);
		event.pageY = event.clientY +
			(doc && doc.scrollTop || body && body.scrollTop || 0) -
			(doc && doc.clientTop || body && body.clientTop || 0);
	}
	var mouseX = event.pageX;
	var mouseY = event.pageY;

	ShowSetpointPopupInt(mouseX, mouseY, idx, refreshfunction, currentvalue, ismobile);
}

function CloseRFYPopup() {
	$("#rfy_popup").hide();
}

function ShowRFYPopupInt(mouseX, mouseY, idx, refreshfunction, ismobile) {
	$.devIdx = idx;
	$.refreshfunction = refreshfunction;

	if (typeof ismobile == 'undefined') {
		$("#rfy_popup").css({
			"top": mouseY,
			"left": mouseX + 15,
			"position": "absolute",
			"-ms-transform": "none",
			"-moz-transform": "none",
			"-webkit-transform": "none",
			"transform": "none"
		});
	}
	else {
		$("#rfy_popup").css({
			"position": "fixed",
			"left": "50%",
			"top": "50%",
			"-ms-transform": "translate(-50%,-50%)",
			"-moz-transform": "translate(-50%,-50%)",
			"-webkit-transform": "translate(-50%,-50%)",
			"transform": "translate(-50%,-50%)"
		});
	}
	$('#rfy_popup').i18n();
	$("#rfy_popup").show();
}

function ShowRFYPopup(event, idx, refreshfunction, Protected, ismobile) {
	event = event || window.event;
	// If pageX/Y aren't available and clientX/Y are,
	// calculate pageX/Y - logic taken from jQuery.
	// (This is to support old IE)
	if (event.pageX == null && event.clientX != null) {
		eventDoc = (event.target && event.target.ownerDocument) || document;
		doc = eventDoc.documentElement;
		body = eventDoc.body;

		event.pageX = event.clientX +
			(doc && doc.scrollLeft || body && body.scrollLeft || 0) -
			(doc && doc.clientLeft || body && body.clientLeft || 0);
		event.pageY = event.clientY +
			(doc && doc.scrollTop || body && body.scrollTop || 0) -
			(doc && doc.clientTop || body && body.clientTop || 0);
	}
	var mouseX = event.pageX;
	var mouseY = event.pageY;

	HandleProtection(Protected, function () {
		ShowRFYPopupInt(mouseX, mouseY, idx, refreshfunction, ismobile);
	});
}

function CloseIthoPopup() {
	$("#itho_popup").hide();
}

function IthoSendCommand(itho_cmnd) {
	var switchcmd = itho_cmnd;
	$("#itho_popup").hide();
	SwitchLight($.devIdx, switchcmd, $.refreshfunction, $.Protected);
}

function ShowIthoPopupInt(mouseX, mouseY, idx, refreshfunction, ismobile) {
	$.devIdx = idx;
	$.refreshfunction = refreshfunction;

	if (typeof ismobile == 'undefined') {
		$("#itho_popup").css({
			"top": mouseY,
			"left": mouseX + 15,
			"position": "absolute",
			"-ms-transform": "none",
			"-moz-transform": "none",
			"-webkit-transform": "none",
			"transform": "none"
		});
	}
	else {
		$("#itho_popup").css({
			"position": "fixed",
			"left": "50%",
			"top": "50%",
			"-ms-transform": "translate(-50%,-50%)",
			"-moz-transform": "translate(-50%,-50%)",
			"-webkit-transform": "translate(-50%,-50%)",
			"transform": "translate(-50%,-50%)"
		});
	}
	$('#itho_popup').i18n();
	$("#itho_popup").show();
}

function ShowIthoPopup(event, idx, refreshfunction, Protected, ismobile) {
	event = event || window.event;
	// If pageX/Y aren't available and clientX/Y are,
	// calculate pageX/Y - logic taken from jQuery.
	// (This is to support old IE)
	if (event.pageX == null && event.clientX != null) {
		eventDoc = (event.target && event.target.ownerDocument) || document;
		doc = eventDoc.documentElement;
		body = eventDoc.body;

		event.pageX = event.clientX +
			(doc && doc.scrollLeft || body && body.scrollLeft || 0) -
			(doc && doc.clientLeft || body && body.clientLeft || 0);
		event.pageY = event.clientY +
			(doc && doc.scrollTop || body && body.scrollTop || 0) -
			(doc && doc.clientTop || body && body.clientTop || 0);
	}
	var mouseX = event.pageX;
	var mouseY = event.pageY;

	HandleProtection(Protected, function () {
		ShowIthoPopupInt(mouseX, mouseY, idx, refreshfunction, ismobile);
	});
}

//Lucci
function CloseLucciPopup() {
	$("#lucci_popup").hide();
}

function LucciSendCommand(lucci_cmnd) {
	var switchcmd = lucci_cmnd;
	$("#lucci_popup").hide();
	SwitchLight($.devIdx, switchcmd, $.refreshfunction, $.Protected);
}

function ShowLucciPopupInt(mouseX, mouseY, idx, refreshfunction, ismobile) {
	$.devIdx = idx;
	$.refreshfunction = refreshfunction;

	if (typeof ismobile == 'undefined') {
		$("#lucci_popup").css({
			"top": mouseY,
			"left": mouseX + 15,
			"position": "absolute",
			"-ms-transform": "none",
			"-moz-transform": "none",
			"-webkit-transform": "none",
			"transform": "none"
		});
	}
	else {
		$("#lucci_popup").css({
			"position": "fixed",
			"left": "50%",
			"top": "50%",
			"-ms-transform": "translate(-50%,-50%)",
			"-moz-transform": "translate(-50%,-50%)",
			"-webkit-transform": "translate(-50%,-50%)",
			"transform": "translate(-50%,-50%)"
		});
	}
	$('#lucci_popup').i18n();
	$("#lucci_popup").show();
}

function ShowLucciPopup(event, idx, refreshfunction, Protected, ismobile) {
	event = event || window.event;
	// If pageX/Y aren't available and clientX/Y are,
	// calculate pageX/Y - logic taken from jQuery.
	// (This is to support old IE)
	if (event.pageX == null && event.clientX != null) {
		eventDoc = (event.target && event.target.ownerDocument) || document;
		doc = eventDoc.documentElement;
		body = eventDoc.body;

		event.pageX = event.clientX +
			(doc && doc.scrollLeft || body && body.scrollLeft || 0) -
			(doc && doc.clientLeft || body && body.clientLeft || 0);
		event.pageY = event.clientY +
			(doc && doc.scrollTop || body && body.scrollTop || 0) -
			(doc && doc.clientTop || body && body.clientTop || 0);
	}
	var mouseX = event.pageX;
	var mouseY = event.pageY;

	HandleProtection(Protected, function () {
		ShowLucciPopupInt(mouseX, mouseY, idx, refreshfunction, ismobile);
	});
}

function MakeDatatableTranslations() {
	$.DataTableLanguage = {};
	$.DataTableLanguage["search"] = $.t("Search") + "&nbsp;:";
	$.DataTableLanguage["lengthMenu"] = $.t("Show _MENU_ entries");
	$.DataTableLanguage["info"] = $.t("Showing _START_ to _END_ of _TOTAL_ entries");
	$.DataTableLanguage["infoEmpty"] = $.t("Showing 0 to 0 of 0 entries");
	$.DataTableLanguage["infoFiltered"] = $.t("(filtered from _MAX_ total entries)");
	$.DataTableLanguage["infoPostFix"] = "";
	$.DataTableLanguage["zeroRecords"] = $.t("No matching records found");
	$.DataTableLanguage["emptyTable"] = $.t("No data available in table");
	$.DataTableLanguage["paginate"] = {};
	$.DataTableLanguage["paginate"]["first"] = $.t("First");
	$.DataTableLanguage["paginate"]["previous"] = $.t("Previous");
	$.DataTableLanguage["paginate"]["next"] = $.t("Next");
	$.DataTableLanguage["paginate"]["last"] = $.t("Last");
}
