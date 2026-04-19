/*
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
						var sStorageId = 'highcharts_series_visibility';
						if (!this.chart.renderTo.id) { return; }
						var sStateId = this.chart.renderTo.id + '_' + this.options.id;
						var sCurrentState = localStorage.getItem(sStorageId) || '{}';
						var oCurrentState = JSON.parse(sCurrentState);
						oCurrentState[sStateId] = this.visible;
						localStorage.setItem(sStorageId, JSON.stringify(oCurrentState));
					} catch (oException_) { /* too bad, no state */ }
				}
			});

			H_.wrap(H_.Series.prototype, 'init', function (fProceed_, oChart_, oOptions_) {
				try {
					var sStorageId = 'highcharts_series_visibility';
					var renderToId = oChart_.renderTo && oChart_.renderTo.id;
					if (renderToId) {
						var sStateId = renderToId + '_' + oOptions_.id;
						var sCurrentState = localStorage.getItem(sStorageId) || '{}';
						var oCurrentState = JSON.parse(sCurrentState);
						if (sStateId in oCurrentState) {
							oOptions_.visible = oCurrentState[sStateId];
						}
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

function b64EncodeUnicode(str) {
    return btoa(encodeURIComponent(str).replace(/%([0-9A-F]{2})/g, function(match, p1) {
        return String.fromCharCode(parseInt(p1, 16))
    }))
}
function b64DecodeUnicode(str) {
	try {
    return decodeURIComponent(Array.prototype.map.call(atob(str), function(c) {
        return '%' + ('00' + c.charCodeAt(0).toString(16)).slice(-2)
    }).join(''))
	}
	catch(e) {
		// Pff fallback
		return atob(str);
	}
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
	bootbox.prompt({
		title: $.t("Please enter Password") + ":",
		inputType: 'password',
		callback: function (result) {
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
	dReturn.m = m;
	dReturn.b = b;
	return dReturn;
};

function SendX10Command(idx, switchcmd, passcode) {
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
			}, 1000);
		},
		error: function () {
			HideNotify();
			bootbox.alert($.t('Problem sending switch command'));
		}
	});
}

function ArmSystemInt(idx, switchcmd, passcode) {
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
					SendX10Command(idx, "Arm Home", passcode)
				}
			},
			{
				text: $.t("Arm Away"),
				click: function () {
					$dialog.remove();
					SendX10Command(idx, "Arm Away", passcode)
				}
			}
		]
	});
}

function ArmSystem(idx, switchcmd, isprotected) {
	if (window.my_config.userrights == 0) {
		HideNotify();
		ShowNotify($.t('You do not have permission to do that!'), 2500, true);
		return;
	}

	var passcode = "";
	if (typeof isprotected != 'undefined') {
		if (isprotected == true) {
			bootbox.prompt({
				title: $.t("Please enter Password") + ":",
				inputType: 'password',
				callback: function (result) {
					if (result === null) {
						return;
					} else {
						if (result == "") {
							return;
						}
						passcode = result;
						ArmSystemInt(idx, switchcmd, passcode);
					}
				}
			});
		}
		else {
			ArmSystemInt(idx, switchcmd, passcode);
		}
	}
	else {
		ArmSystemInt(idx, switchcmd, passcode);
	}
}

function ArmSystemMeiantechInt(idx, switchcmd, passcode) {
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
					SendX10Command(idx, "Arm Home", passcode)
				}
			},
			{
				text: $.t("Arm Away"),
				click: function () {
					$dialog.remove();
					switchcmd = "Arm Away";
					SendX10Command(idx, "Arm Away", passcode)
				}
			},
			{
				text: $.t("Panic"),
				click: function () {
					$dialog.remove();
					switchcmd = "Panic";
					SendX10Command(idx, "Panic", passcode)
				}
			},
			{
				text: $.t("Disarm"),
				click: function () {
					$dialog.remove();
					switchcmd = "Disarm";
					SendX10Command(idx, "Disarm", passcode)
				}
			}
		]
	});
}

function ArmSystemMeiantech(idx, switchcmd, isprotected) {
	if (window.my_config.userrights == 0) {
		HideNotify();
		ShowNotify($.t('You do not have permission to do that!'), 2500, true);
		return;
	}

	var passcode = "";
	if (typeof isprotected != 'undefined') {
		if (isprotected == true) {
			bootbox.prompt({
				title: $.t("Please enter Password") + ":",
				inputType: 'password',
				callback: function (result) {
					if (result === null) {
						return;
					} else {
						if (result == "") {
							return;
						}
						passcode = result;
						ArmSystemMeiantechInt(idx, switchcmd, passcode);
					}
				}
			});
		}
		else {
			ArmSystemMeiantechInt(idx, switchcmd, passcode);
		}
	}
	else {
		ArmSystemMeiantechInt(idx, switchcmd, passcode);
	}
}

function ArmSystemX10Int(idx, switchcmd, passcode) {
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
					SendX10Command(idx, "Normal", passcode);
				}
			},
			{
				text: $.t("Alarm"),
				click: function () {
					$dialog.remove();
					SendX10Command(idx, "Alarm", passcode);
				}
			},
			{
				text: $.t("Arm Home"),
				click: function () {
					$dialog.remove();
					SendX10Command(idx, "Arm Home", passcode);
				}
			},
			{
				text: $.t("Arm Home Delayed"),
				click: function () {
					$dialog.remove();
					SendX10Command(idx, "Arm Home Delayed", passcode);
				}
			},
			{
				text: $.t("Arm Away"),
				click: function () {
					$dialog.remove();
					SendX10Command(idx, "Arm Away", passcode);
				}
			},
			{
				text: $.t("Arm Away Delayed"),
				click: function () {
					$dialog.remove();
					SendX10Command(idx, "Arm Away Delayed", passcode);
				}
			},
			{
				text: $.t("Panic"),
				click: function () {
					$dialog.remove();
					SendX10Command(idx, "Panic", passcode);
				}
			},
			{
				text: $.t("Disarm"),
				click: function () {
					$dialog.remove();
					SendX10Command(idx, "Disarm", passcode);
				}
			},
			{
				text: $.t("Light On"),
				click: function () {
					$dialog.remove();
					SendX10Command(idx, "Light On", passcode);
				}
			},
			{
				text: $.t("Light Off"),
				click: function () {
					$dialog.remove();
					SendX10Command(idx, "Light Off", passcode);
				}
			},
			{
				text: $.t("Light 2 On"),
				click: function () {
					$dialog.remove();
					SendX10Command(idx, "Light 2 On", passcode);
				}
			},
			{
				text: $.t("Light 2 Off"),
				click: function () {
					$dialog.remove();
					SendX10Command(idx, "Light 2 Off", passcode);
				}
			}
		]
	});
}

function ArmSystemX10(idx, switchcmd, isprotected) {
	if (window.my_config.userrights == 0) {
		HideNotify();
		ShowNotify($.t('You do not have permission to do that!'), 2500, true);
		return;
	}

	var passcode = "";
	if (typeof isprotected != 'undefined') {
		if (isprotected == true) {
			bootbox.prompt({
				title: $.t("Please enter Password") + ":",
				inputType: 'password',
				callback: function (result) {
					if (result === null) {
						return;
					} else {
						if (result == "") {
							return;
						}
						passcode = result;
						ArmSystemX10Int(idx, switchcmd, passcode);
					}
				}
			});
		}
		else {
			ArmSystemX10Int(idx, switchcmd, passcode);
		}
	}
	else {
		ArmSystemX10Int(idx, switchcmd, passcode);
	}
}

function SwitchLightInt(idx, switchcmd, passcode) {
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
			}, 1000);
		},
		error: function () {
			HideNotify();
			bootbox.alert($.t('Problem sending switch command'));
		}
	});
}

function SwitchLight(idx, switchcmd, isprotected) {
	if (window.my_config.userrights == 0) {
		HideNotify();
		ShowNotify($.t('You do not have permission to do that!'), 2500, true);
		return;
	}

	var passcode = "";
	if (typeof isprotected != 'undefined') {
		if (isprotected == true) {
			bootbox.prompt({
				title: $.t("Please enter Password") + ":",
				inputType: 'password',
				callback: function (result) {
					if (result === null) {
						return;
					} else {
						if (result == "") {
							return;
						}
						passcode = result;
						SwitchLightInt(idx, switchcmd, passcode);
					}
				}
			});
		}
		else {
			SwitchLightInt(idx, switchcmd, passcode);
		}
	}
	else {
		SwitchLightInt(idx, switchcmd, passcode);
	}
}
function SwitchSelectorLevelInt(idx, levelName, levelValue, passcode) {
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
			}, 1000);
		},
		error: function () {
			HideNotify();
			bootbox.alert($.t('Problem sending switch command'));
		}
	});
}

function SwitchSelectorLevel(idx, levelName, levelValue, isprotected) {
	if (window.my_config.userrights == 0) {
		HideNotify();
		ShowNotify($.t('You do not have permission to do that!'), 2500, true);
		return;
	}

	var passcode = "";
	if (typeof isprotected != 'undefined') {
		if (isprotected == true) {
			bootbox.prompt({
				title: $.t("Please enter Password") + ":",
				inputType: 'password',
				callback: function (result) {
					if (result === null) {
						return;
					} else {
						if (result == "") {
							return;
						}
						passcode = result;
						SwitchSelectorLevelInt(idx, levelName, levelValue, passcode);
					}
				}
			});
		}
		else {
			SwitchSelectorLevelInt(idx, levelName, levelValue, passcode);
		}
	}
	else {
		SwitchSelectorLevelInt(idx, levelName, levelValue, passcode);
	}
}

function SwitchSceneInt(idx, switchcmd, passcode) {
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
			}, 1000);
		},
		error: function () {
			HideNotify();
			bootbox.alert($.t('Problem sending switch command'));
		}
	});
}

function SwitchScene(idx, switchcmd, isprotected) {
	if (window.my_config.userrights == 0) {
		HideNotify();
		ShowNotify($.t('You do not have permission to do that!'), 2500, true);
		return;
	}
	var passcode = "";
	if (typeof isprotected != 'undefined') {
		if (isprotected == true) {
			bootbox.prompt({
				title: $.t("Please enter Password") + ":",
				inputType: 'password',
				callback: function (result) {
					if (result === null) {
						return;
					} else {
						if (result == "") {
							return;
						}
						passcode = result;
						SwitchSceneInt(idx, switchcmd, passcode);
					}
				}
			});
		}
		else {
			SwitchSceneInt(idx, switchcmd, passcode);
		}
	}
	else {
		SwitchSceneInt(idx, switchcmd, passcode);
	}
}

function ResetSecurityStatus(idx, switchcmd, callback) {
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
				if (typeof callback === 'function') {
					callback();
				}
			}, 1000);
		},
		error: function () {
			HideNotify();
			bootbox.alert($.t('Problem sending switch command'));
		}
	});
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

function GetLocalDateTimeFromString(s, yearOffset=0) {
	return new Date(
		parseInt(s.substring(0, 4), 10) + yearOffset,
		parseInt(s.substring(5, 7), 10) - 1,
		parseInt(s.substring(8, 10), 10),
		parseInt(s.substring(11, 13), 10),
		parseInt(s.substring(14, 16), 10),
		19 <= s.length ? parseInt(s.substring(17, 19), 10) : 0
	).getTime();
}

function GetLocalTimestampFromString(s, yearOffset=0) {
	return new Date(
		parseInt(s.substring(0, 4), 10) + yearOffset,
		parseInt(s.substring(5, 7), 10) - 1,
		parseInt(s.substring(8, 10), 10),
		parseInt(s.substring(11, 13), 10),
		parseInt(s.substring(14, 16), 10),
		parseInt(s.substring(17, 19), 10)
	).getTime();
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

function GetLocalDateFromString(s, yearOffset=0) {
	return new Date(
		parseInt(s.substring(0, 4), 10) + yearOffset,
		parseInt(s.substring(5, 7), 10) - 1,
		parseInt(s.substring(8, 10), 10)).getTime();
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
function checkLengthText(text, min, max) {
	if (text.length > max || text.length < min) {
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
	return new Noty({
		type: ntype,
		layout: 'topRight',
		text: ntext,
		dismissQueue: true,
		timeout: ntimeout,
		theme: 'relax'
	}).show();
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
	if (typeof status == 'undefined')
		return "-?-";

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

function ShowCameraLiveStream(Name, camIdx, AspectRatio) {
	$.count = 0;
	$.camfeed = "camsnapshot.jpg?idx=" + camIdx;

	$('#dialog-camera-live #camfeed').attr("src", "images/camera_default.png");
	//$('#dialog-camera-live #camfeed').attr("src", FeedURL);

	var windowWidth = $(window).width() - 20;
	var windowHeight = $(window).height() - 150;

	var AspectSource = (AspectRatio == 0) ? (4/3) : (16/9);

	var height = windowHeight;
	var width = Math.round(height * AspectSource) & ~1;
	if (width > windowWidth) {
		width = windowWidth;
		height = Math.round(width / AspectSource) & ~1;
	}

	//Set inner Camera feed width/height
	$("#dialog-camera-live #camfeed").width(width - 30);
	$("#dialog-camera-live #camfeed").height(height - 16);

	$("#dialog-camera-live").dialog({
		resizable: false,
		width: width + 2,
		height: height + 50,
		position: {
			my: "center",
			at: "center",
			of: window
		},
		modal: true,
		title: unescape(Name),
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
	var dwidth = dheight * svgRatio ;
	// for v2.0, if screen is wide enough add room to show media at the side of the remote
	$(divId).dialog({
		resizable: false,
		//show: "blind", // effects are causing issue with changing the height during the animation
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
			if ( HWType.indexOf('Panasonic') >= 0) {
				// Here is a little painful because we need to get hardware id  first...
				$.ajax({
					url: "json.htm?type=command&param=getdevices&rid=" + devIdx,
					async: true,
					dataType: 'json',
					success: function (data) { 
						hwId = data.result[0].HardwareID;
						$.ajax({
							url: "json.htm?type=command&param=gethardware",
							async: true,
							dataType: 'json',
							success: function (data) { 
								// Need to iterate over all hardware to find the good one
								for(var i in data.result) {
									var hw = data.result[i];
									if (hw.idx == hwId) {
										if (hw.Extra !== null && hw.Extra !== '') {
											// We finally have the custombuttons string, process!
											var bspacing = 20;
											var bvspacing = 20;
											var bheight = 100;
											var bindex = 0;
											// Reset buttons
											$("#MediaRemote-custom-buttons").html('');
											$(svgId).prop("viewBox").baseVal.height = 1875;
											// Loop lines
											hw.Extra.split(';').forEach(function (line) {
												// Add line
												var vBox = $(svgId).prop("viewBox").baseVal;
												var bvline = vBox.y + vBox.height +  bvspacing;
												$(svgId).prop("viewBox").baseVal.height = vBox.height + bheight + bvspacing;
												var buttons = line.split(',');
												var bwidth = (vBox.width + bspacing) / buttons.length - bspacing; 
												// Loop buttons
												buttons.forEach(function (val, index) {
													var tokens = val.split(':');
													var btitle = tokens[0];
													var bcommand = tokens[1];
													var buttonSVG = "";
													bindex++;
													bx = $(svgId).prop("viewBox").baseVal.x + index * (bwidth+bspacing);
													// Button shadow
													buttonSVG += '<rect id="toto" class="remoteshadow" x="'+bx+'" y="'+(bvline+10)+'" width="'+bwidth+'" height="'+bheight+'" rx="50" ry="50"></rect>';
													// Button 
													buttonSVG += '<rect class="remotehoverable" fill="url(#grad1)" x="'+bx+'" y="'+(bvline)+'" width="'+bwidth+'" height="'+bheight+'"  rx="50" ry="50" ';
													buttonSVG += 'onclick="javascript: click_media_remote(\'' + bcommand + '\');" ';
													buttonSVG += '><title id="dialog-media-remote-opt1-title">' + btitle + '</title></rect>';
													// Button text
													buttonSVG += '<text text-anchor="middle" x="'+(bx+bwidth/2)+'" y="'+(bvline+bheight*0.55)+'" class="remotetext" ';
													buttonSVG += 'fill="black"  style="font-size: 60px; font-weight: bold;">' + btitle + '</text>';
													// Add button
													$("#MediaRemote-custom-buttons").append(buttonSVG);
												});
											});

											// Refresh SVG
											$(svgId).parent().html($(svgId).parent().html());
											// Ajust dialog width
											var vBox = $(svgId).prop("viewBox").baseVal;
											var svgRatio = (vBox.width - vBox.x) / (vBox.height - vBox.y);
											var dheight = $(window).height() * 0.85;
											var dwidth = dheight * svgRatio;
											$(divId).dialog( "option", "width", dwidth);
											$(divId).dialog( "option", "height", dheight);
										}
									}
								}
							}
						});
					}
				});
			}
		},
		close: function () {
			if (typeof $.myglobals.refreshTimer != 'undefined') {
				clearTimeout($.myglobals.refreshTimer)
			}
			$(this).dialog("close");
		}
	});
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

function addLeadingZeros(n, length) {
	var str = n.toString();
	var zeros = "";
	for (var i = length - str.length; i > 0; i--)
		zeros += "0";
	zeros += str;
	return zeros;
}

function SwitchLightPopup(idx, switchcmd, isprotected) {
	SwitchLight(idx, switchcmd, isprotected);
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

function ShowRGBWPicker(selector, idx, Protected, MaxDimLevel, LevelInt, colorJSON, iSubType, iDimmerType, callback) {

	var color = {};
	var devIdx = idx;
	var SubType = iSubType;
	var DimmerType = iDimmerType;
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
		else if (mode == "color_no_master") {
			$(selector + ' #popup_picker').wheelColorPicker('setOptions', {sliders:'w', preserveWheel:true});
		}
		else if (mode == "white") {
			$(selector + ' #popup_picker').wheelColorPicker('setOptions', {sliders:'m', preserveWheel:true});
		}
		else if (mode == "white_no_master") {
			// TODO: Silly, nothing to show!
			$(selector + ' #popup_picker').wheelColorPicker('setOptions', {sliders:'', preserveWheel:true});
		}
		else if (mode == "temperature") {
			$(selector + ' #popup_picker').wheelColorPicker('setOptions', {sliders:'xm'});
		}
		else if (mode == "temperature_no_master") {
			// TODO: Silly, nothing to show!
			$(selector + ' #popup_picker').wheelColorPicker('setOptions', {sliders:''});
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
		var supportedModes = 0;
		if (LEDType.bHasRGB) supportedModes++;
		if (LEDType.bHasWhite && !LEDType.bHasTemperature && DimmerType!="rel") supportedModes++;
		if (LEDType.bHasTemperature) supportedModes++;
		if (LEDType.bHasCustom && !LEDType.bHasTemperature) supportedModes++;
		if (LEDType.bHasCustom && LEDType.bHasTemperature) supportedModes++;
		if (supportedModes > 1)
		{
			if (LEDType.bHasRGB) {
				if (mode == "color" || mode == "color_no_master") {
					$(selector + ' .pickermodergb.selected').show();
				}
				else {
					$(selector + ' .pickermodergb.unselected').show();
				}
			}
			if (LEDType.bHasWhite && !LEDType.bHasTemperature && DimmerType!="rel") {
				if (mode == "white" || mode == "white_no_master") {
					$(selector + ' .pickermodewhite.selected').show();
				}
				else {
					$(selector + ' .pickermodewhite.unselected').show();
				}
			}
			if (LEDType.bHasTemperature && DimmerType!="rel") {
				if (mode == "temperature" || mode == "temperature_no_master") {
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
		}

		$(selector + ' .pickerrgbcolorrow').hide();
		// Show RGB hex input
		if (LEDType.bHasRGB) {
			if (mode == "color" || mode == "color_no_master") {
				$(selector + ' .pickerrgbcolorrow').show();
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
	if (color_m == 3 && !LEDType.bHasRGB)
	{
		if (LEDType.bHasTemperature) color_m = 2; // Default to temperature if light does not support RGB but does support temperature
		else color_m = 1;                         // Default to white if light does not support either RGB or temperature (in this case just a dimmer slider should be shown though)
	}

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

	// TODO: white_no_master and temperature_no_master are meaningless, remove
	if (color_m == 1) { // White mode
		colorPickerMode = DimmerType!="rel"?"white":"white_no_master";
	}
	if (color_m == 2) { // Color temperature mode
		colorPickerMode = DimmerType!="rel"?"temperature":"temperature_no_master";
	}
	else if (color_m == 3){ // Color  mode
		colorPickerMode = DimmerType!="rel"?"color":"color_no_master";
	}
	else if (color_m == 4){ // Custom  mode
		colorPickerMode = "customw";
		if (LEDType.bHasTemperature) {
			colorPickerMode = "customww";
		}
	}

	$(selector + ' .pickermodergb').off().click(function(){
		UpdateColorPicker(DimmerType!="rel"?"color":"color_no_master");
	});
	$(selector + ' .pickermodewhite').off().click(function(){
		UpdateColorPicker(DimmerType!="rel"?"white":"white_no_master");
	});
	$(selector + ' .pickermodetemp').off().click(function(){
		UpdateColorPicker(DimmerType!="rel"?"temperature":"temperature_no_master");
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

	var rgbhex = $(selector + ' #popup_picker').wheelColorPicker('getValue', 'hex').toUpperCase();
	$(selector + ' .pickerrgbcolorinput').val(rgbhex);

	// Update color picker controls
	UpdateColorPicker(colorPickerMode);

	$(selector + ' #popup_picker').off('slidermove sliderup').on('slidermove sliderup', function() {
		clearTimeout($.setColValue);

		var color = $(this).wheelColorPicker('getColor');
		var rgbhex = $(this).wheelColorPicker('getValue', 'hex').toUpperCase();
		var dimlevel = Math.round((color.m*99)+1); // 1..100
		var JSONColor = $(selector + ' #popup_picker')[0].getJSONColor();
		//TODO: Rate limit instead of debounce
		$.setColValue = setTimeout(function () {
			var fn = callback || SetColValue;
			fn(devIdx, JSONColor, dimlevel);
		}, 400);
		$(selector + ' .pickerrgbcolorinput').val(rgbhex);
	});
	$(selector + ' .pickerrgbcolorinput').off('input').on('input', function() {
		$(selector + ' #popup_picker').wheelColorPicker('setValue', this.value)
	});
}

function ShowRGBWPopupInt(mouseX, mouseY, idx, Protected, MaxDimLevel, LevelInt, colorJSON, SubType, DimmerType) {
	var ledType = getLEDType(SubType);
	var devIdx = idx;

	ShowRGBWPicker("#rgbw_popup", idx, Protected, MaxDimLevel, LevelInt, colorJSON, SubType, DimmerType);

	// Setup on and Off buttons
	$('#rgbw_popup #popup_switch_on').attr("href", 'javascript:SwitchLightPopup(' + idx + ',\'On\',' + Protected + ');');
	$('#rgbw_popup #popup_switch_off').attr("href", 'javascript:SwitchLightPopup(' + idx + ',\'Off\',' + Protected + ');');

	// Show brightness and temperature buttons
	$('#rgbw_popup #popup_bright_up').hide();
	$('#rgbw_popup #popup_bright_down').hide();
	$('#rgbw_popup #popup_warmer').hide();
	$('#rgbw_popup #popup_colder').hide();

	if (DimmerType && DimmerType === "rel")
	{
		$('#rgbw_popup #popup_bright_up').show();
		$('#rgbw_popup #popup_bright_down').show();
		$('#rgbw_popup #popup_bright_up').off().click(function(){
			$.ajax({
				url: "json.htm?type=command&param=brightnessup&idx=" + devIdx,
				async: false,
				dataType: 'json'
			});
		});
		$('#rgbw_popup #popup_bright_down').off().click(function(){
			$.ajax({
				url: "json.htm?type=command&param=brightnessdown&idx=" + devIdx,
				async: false,
				dataType: 'json'
			});
		});
	}

	if (DimmerType && DimmerType === "rel" && ledType.bHasTemperature)
	{
		$('#rgbw_popup #popup_warmer').show();
		$('#rgbw_popup #popup_colder').show();
		$('#rgbw_popup #popup_warmer').off().click(function(){
			$.ajax({
				url: "json.htm?type=command&param=warmer&idx=" + devIdx,
				async: false,
				dataType: 'json'
			});
		});
		$('#rgbw_popup #popup_colder').off().click(function(){
			$.ajax({
				url: "json.htm?type=command&param=cooler&idx=" + devIdx,
				async: false,
				dataType: 'json'
			});
		});
	}

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
function ShowRGBWPopup(event, idx, Protected, MaxDimLevel, LevelInt, color, SubType, DimmerType) {
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
		ShowRGBWPopupInt(mouseX, mouseY, idx, Protected, MaxDimLevel, LevelInt, color, SubType, DimmerType);
	});
}

function SwitchTherm3Popup(idx, switchcmd) {
	SwitchLightInt(idx, switchcmd, $.devpwd);
	$("#thermostat3_popup").hide();
}
function ShowTherm3PopupInt(mouseX, mouseY, idx, pwd) {
	$.devIdx = idx;
	$.devpwd = pwd;
	$('#thermostat3_popup #popup_therm_on').attr("href", 'javascript:SwitchTherm3Popup(' + idx + ',\'On\');');
	$('#thermostat3_popup #popup_therm_off').attr("href", 'javascript:SwitchTherm3Popup(' + idx + ',\'Off\');');

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

function ShowTherm3Popup(event, idx, Protected, MaxDimLevel, LevelInt, hue) {
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
		ShowTherm3PopupInt(mouseX, mouseY, idx, pwd);
	});
}

function RFYEnableSunWind(bDoEnable) {
	var switchcmd = "EnableSunWind";
	if (bDoEnable == false) {
		switchcmd = "DisableSunWind";
	}
	$("#rfy_popup").hide();
	SwitchLight($.devIdx, switchcmd, $.Protected);
}

function ShowSetpointPopupInt(mouseX, mouseY, idx, currentvalue, ismobile, step, min, max) {
	$.devIdx = idx;
	$.setstep = step;
	$.setmin = min;
	$.setmax = max;
	var curValue = (Number.isInteger(currentvalue)) ? currentvalue : parseFloat(currentvalue).toFixed(1);
	$('#setpoint_popup #actual_value').html(curValue);
	$('#setpoint_popup #popup_setpoint').val(curValue);

	var bIsMobile = false;
	if (typeof ismobile !== 'undefined') {
		bIsMobile = ismobile;
	}

	if (bIsMobile == false) {
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

function CloseSetpointPopup() {
	$("#setpoint_popup").hide();
}

function SetpointUp() {
	var curValue = parseFloat($('#setpoint_popup #popup_setpoint').val());
	curValue += $.setstep;
	curValue = Math.round(curValue / $.setstep) * $.setstep;
	if (curValue > $.setmax)
		curValue = $.setmax;
	var curValueStr = (Number.isInteger(curValue)) ? curValue : curValue.toFixed(1);
	$('#setpoint_popup #popup_setpoint').val(curValueStr);
}

function SetpointDown() {
	var curValue = parseFloat($('#setpoint_popup #popup_setpoint').val());
	curValue -= $.setstep;
	curValue = Math.round(curValue / $.setstep) * $.setstep;
	if (curValue < $.setmin)
		curValue = $.setmin;
	var curValueStr = (Number.isInteger(curValue)) ? curValue : curValue.toFixed(1);
	$('#setpoint_popup #popup_setpoint').val(curValueStr);
}

function SetSetpoint() {
	var currentvalue = parseFloat($('#setpoint_popup #popup_setpoint').val());
	var curValue = (Number.isInteger(currentvalue)) ? currentvalue : currentvalue.toFixed(1);
	if ((curValue < $.setmin) || (curValue > $.setmax)) {
		var betmsg = "!";
		betmsg = " " + $.t('between') + " " + $.setmin + " " + $.t('and') + " " + $.setmax + "!";
		var msg = $.t('Please enter a valid integer') + betmsg;
		bootbox.alert(msg);
		return;
	}
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
			}, 1000);
		},
		error: function () {
			HideNotify();
			bootbox.alert($.t('Problem setting Setpoint value'));
		}
	});
}


function ShowSetpointPopup(event, idx, Protected, currentvalue, ismobile, step, min, max) {
	$.Protected = Protected;

	if (typeof step == 'undefined') step = 0.5;
	if (typeof min == 'undefined') min = -200;
	if (typeof max == 'undefined') max = 200;

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
		ShowSetpointPopupInt(mouseX, mouseY, idx, currentvalue, ismobile, step, min, max);
	});
}

function CloseRFYPopup() {
	$("#rfy_popup").hide();
}

function ShowRFYPopupInt(mouseX, mouseY, idx, ismobile) {
	$.devIdx = idx;

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

function ShowRFYPopup(event, idx, Protected, ismobile) {
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
		ShowRFYPopupInt(mouseX, mouseY, idx, ismobile);
	});
}

function CloseIthoPopup() {
	$("#itho_popup").hide();
}

function IthoSendCommand(itho_cmnd) {
	var switchcmd = itho_cmnd;
	$("#itho_popup").hide();
	SwitchLight($.devIdx, switchcmd, $.Protected);
}

function ShowIthoPopupInt(mouseX, mouseY, idx, ismobile) {
	$.devIdx = idx;

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

function ShowIthoPopup(event, idx, Protected, ismobile) {
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
		ShowIthoPopupInt(mouseX, mouseY, idx, ismobile);
	});
}

//Lucci
function CloseLucciPopup() {
    $("#lucci_popup").hide();
    $("#lucci_dc_popup").hide();
}

function LucciSendCommand(lucci_cmnd) {
	var switchcmd = lucci_cmnd;
	$("#lucci_popup").hide();
	$("#lucci_dc_popup").hide();
	SwitchLight($.devIdx, switchcmd, $.Protected);
}

function ShowLucciPopupInt(mouseX, mouseY, idx, ismobile) {
	$.devIdx = idx;

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

function ShowLucciPopup(event, idx, Protected, ismobile) {
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
		ShowLucciPopupInt(mouseX, mouseY, idx, ismobile);
	});
}

function ShowLucciDCPopupInt(mouseX, mouseY, idx, ismobile) {
    $.devIdx = idx;

    if (typeof ismobile == 'undefined') {
        $("#lucci_dc_popup").css({
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
        $("#lucci_dc_popup").css({
            "position": "fixed",
            "left": "50%",
            "top": "50%",
            "-ms-transform": "translate(-50%,-50%)",
            "-moz-transform": "translate(-50%,-50%)",
            "-webkit-transform": "translate(-50%,-50%)",
            "transform": "translate(-50%,-50%)"
        });
    }
    $('#lucci_dc_popup').i18n();
    $("#lucci_dc_popup").show();
}

function ShowLucciDCPopup(event, idx, Protected, ismobile) {
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
        ShowLucciDCPopupInt(mouseX, mouseY, idx, ismobile);
    });
}

//Falmec
function CloseFalmecPopup() {
    $("#falmec_popup").hide();
}

function FalmecSendCommand(falmec_cmnd) {
	var switchcmd = falmec_cmnd;
	$("#falmec_popup").hide();
	SwitchLight($.devIdx, switchcmd, $.Protected);
}

function ShowFalmecPopupInt(mouseX, mouseY, idx, ismobile) {
	$.devIdx = idx;

	if (typeof ismobile == 'undefined') {
		$("#falmec_popup").css({
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
		$("#falmec_popup").css({
			"position": "fixed",
			"left": "50%",
			"top": "50%",
			"-ms-transform": "translate(-50%,-50%)",
			"-moz-transform": "translate(-50%,-50%)",
			"-webkit-transform": "translate(-50%,-50%)",
			"transform": "translate(-50%,-50%)"
		});
	}
	$('#falmec_popup').i18n();
	$("#falmec_popup").show();
}

function ShowFalmecPopup(event, idx, Protected, ismobile) {
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
		ShowFalmecPopupInt(mouseX, mouseY, idx, ismobile);
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

function fromInstanceOrFunction(functionTemplate = f => f()) {
	return function (instanceOrFunction) {
		if (typeof instanceOrFunction === 'function') {
			return functionTemplate(instanceOrFunction);
		} else {
			return instanceOrFunction;
		}
	}
}


/* LiveSearch Functions: Filters devices when typing in the INPUT field ##################################### */
var _debug_livesearch= false;

function AddToLiveSearch(current_data, new_value) {
	if (
		(typeof new_value == 'undefined') ||
		(new_value === "")
	   ) {
		return current_data;
	}
	if (
		(typeof current_data == 'undefined') ||
		(current_data === "")
	) {
		return new_value;
	}
	if (current_data.includes(new_value))
		return current_data;
	return current_data + " " +  new_value;
}

GenerateLiveSearchTextDefault = function (item) {
	var searchText = "";
	searchText = AddToLiveSearch(searchText, item.idx);
	searchText = AddToLiveSearch(searchText, item.Name);
	searchText = AddToLiveSearch(searchText, item.Description.replace('"',"'"));
	searchText = AddToLiveSearch(searchText, item.Type);
	searchText = AddToLiveSearch(searchText, item.HardwareName);
	if (typeof item.SubType != 'undefined') {
		searchText = AddToLiveSearch(searchText, item.SubType);
	}
	return searchText;
}
//Lights
GenerateLiveSearchTextL = function (item, bigtext) {
	var searchText = GenerateLiveSearchTextDefault(item);
	if (typeof (bigtext) !== 'undefined') {
		if (bigtext !== "") {
			if (bigtext.includes(' %')) {
				if (item.SwitchType=="Dimmer") {
					//treat dimmer percentage as on
					searchText = AddToLiveSearch(searchText, "On");
				} else {
					//possible a blind
				}
			}
			else 
				searchText = AddToLiveSearch(searchText, bigtext);
		}
	}
	if (item.SwitchType!=="On/Off") {
		searchText = AddToLiveSearch(searchText, item.SwitchType);
	}
	return searchText;
}
//Scenes/Groups
GenerateLiveSearchTextSG = function (item, bigtext) {
	var searchText = GenerateLiveSearchTextDefault(item);
	searchText = AddToLiveSearch(searchText, bigtext);
	return searchText;
}
//Temperature (do we need to search for temp/humidity/gust or only name/type?)
GenerateLiveSearchTextT = function (item) {
	var searchText = GenerateLiveSearchTextDefault(item);
	//searchText = AddToLiveSearch(searchText, item.Temp);
	//searchText = AddToLiveSearch(searchText, item.Humidity);
	searchText = AddToLiveSearch(searchText, item.HumidityStatus);
	searchText = AddToLiveSearch(searchText, item.Gust);
	return searchText;
}

//Weather (do we need to search for temp/humidity/gust or only name/type?)
GenerateLiveSearchTextW = function (item) {
	var searchText = GenerateLiveSearchTextDefault(item);
	//searchText = AddToLiveSearch(searchText, item.Temp);
	//searchText = AddToLiveSearch(searchText, item.Humidity);
	searchText = AddToLiveSearch(searchText, item.HumidityStatus);
	//searchText = AddToLiveSearch(searchText, item.Gust);
	//searchText = AddToLiveSearch(searchText, item.Barometer);
	searchText = AddToLiveSearch(searchText, item.ForecastStr);
	//searchText = AddToLiveSearch(searchText, item.Rain);
	//searchText = AddToLiveSearch(searchText, item.Radiation);
	return searchText;
}
GenerateLiveSearchTextU = function (item, bigtext) {
	var searchText = GenerateLiveSearchTextDefault(item);
	//searchText = AddToLiveSearch(searchText, bigtext);
	return searchText;
}


/* Triggers LiveSearch change ----------------------------------  */
function RefreshLiveSearch(){
	if(_debug_livesearch) console.log('LiveSearch: Refreshing...');
	$('.jsLiveSearch').trigger('change');
}

/* Restores saved search filter once both the input and items are in the DOM.
   Called from WatchLiveSearch (topbar loaded) and from controllers (data loaded),
   so whichever event fires last will trigger the restore. ------------------- */
function ScheduleLiveSearchRestore(){
	if(!window.myglobals || !window.myglobals.LastSearchFilter) return;
	if(window._lsRestoreInterval) clearInterval(window._lsRestoreInterval);
	var _attempts = 0;
	window._lsRestoreInterval = setInterval(function(){
		var input = $('.jsLiveSearch');
		var searchable = $('.itemBlock [data-search]');
		if(input.length > 0 && searchable.length > 0){
			clearInterval(window._lsRestoreInterval);
			window._lsRestoreInterval = null;
			input.val(window.myglobals.LastSearchFilter);
			RefreshLiveSearch();
		} else if(++_attempts >= 100){
			clearInterval(window._lsRestoreInterval);
			window._lsRestoreInterval = null;
		}
	}, 50);
}

/* Watches the LiveSearch INPUT field -------------------------------- */
function WatchLiveSearch(){
	if(_debug_livesearch) console.log('LiveSearch: Start Watching ...');
	_tbDisplayResults(false,0);

	/* Watches INPUT ++++++++++++++++++++ */
	$('.jsLiveSearch').off().on('keyup change',function(e){
		if(_debug_livesearch)  console.log('LiveSearch: processing on keyup - "'+$(this).val()+'"');
		var query	=$(this).val();
		if(window.myglobals) window.myglobals.LastSearchFilter = query;
		var div		=$('.divider');
		var cont	=$('.devicesList');
		var items	=$('.itemBlock');
		var cl_shown	='liveSearchShown';
		var filt_search		=$(this).closest('.jsTbFiltSearch');
		var cl_withres	='tbFiltSearchWithResults';

		if(query.length == 0){
			filt_search.removeClass(cl_withres);
			if(cont.hasClass('devicesListFiltered')){
				cont.removeClass('devicesListFiltered');
				div.css('display','block');
				div.addClass('row');
				div.find('.clearfix').show(); /* only for Weather and Temperatures pages */
				items.show().removeClass('liveSearchShown');	
			}
		}
		else{
			filt_search.addClass(cl_withres);
			if(! cont.hasClass('devicesListFiltered')){
				cont.addClass('devicesListFiltered');
				div.css('display','inline');
			}
			div.removeClass('row');
			div.find('.clearfix').hide();  /* only for Weather and Temperatures pages */

			var searchString=query.replace('\\','').replace('[','\\[').replace(']','\\]').replace('.','\\.')
			const regexStr = '(?=.*' + searchString.split(/\,|\s/).join(')(?=.*') + ')';
			const searchRegEx = new RegExp(regexStr, 'gi');

			items.each(function(index){
				var searchText	=$(this).find('#name').attr('data-search')	|| '';
				var to_hide=$(this);

				if (searchText.match(searchRegEx) !== null) {
					to_hide.show();
					to_hide.addClass(cl_shown);
				}
				else{
					to_hide.hide();
					to_hide.removeClass(cl_shown);
				}
			});
		}

		var count   =$('.' + cl_shown).length;
		if(_debug_livesearch)  console.log('LiveSearch: Found '+ count +' items');
		_tbDisplayResults(count || query.length, count);
	});

	/* Watches Close icon ++++++++++++++++++++ */
	$(".jsTbResultsClose,.jsTbResults").off().on('click',function(e) {
		e.preventDefault();
		if(_debug_livesearch)  console.log('LiveSearch: Close Clicked');
		if(window.myglobals) window.myglobals.LastSearchFilter = '';
		$('.jsLiveSearch').val('').trigger('change');
	});

	ScheduleLiveSearchRestore();
}

/* Toggle Results display ------------------------------------------ */
function _tbDisplayResults(on, count){
	if(on){
		$('.jsTbSearch').hide();
		$('.jsTbResults').show();
		$('.jsTbResultsCount').html(_tbPadCount(count));
	}
	else{
		$('.jsTbSearch').show();
		$('.jsTbResults').hide();
		$('.jsTbResultsCount').html('');
	}
}

/* Pad Left with spaces ------------------------------------------- */
function _tbPadCount(txt){
	return String('xxx' + txt).slice(-4).replace(/x/g, '&nbsp;');
}

function truncateString(str, num) {
  if (str.length <= num) {
    return str
  }
  return str.slice(0, num) + '...'
}

/* Display descriptions when hovering name ################################################################## */
function WatchDescriptions(){
	/* Show description when hovering item's name */
	$(".item-name").hover(function() {
		if(_debug_livesearch) console.log("Hover Description!");
		var desc=$(this).attr('data-desc');
		if(desc.length > 0){
			$(this).css('cursor','pointer').attr('title', desc);
		}
	}, function() {
		$(this).css('cursor','auto');
	});
};

/**
 * Calculate self-sufficiency percentage.
 *
 * self_sufficiency = min(100, (solar - p1Export + bat_discharge) / house x 100)
 *
 * Subtracting p1Export from solar removes the exported portion, leaving only
 * solar that stayed in the local system.  Adding bat_discharge back cancels
 * the battery's contribution to p1Export (battery-to-grid), so the numerator
 * algebraically reduces to (solarToHouse + batToHouse) — the energy that
 * actually powered the house from local sources.
 *
 * This is exact for:
 *   - No-battery setups
 *   - Arbitrage batteries (charge from grid, discharge to grid)
 *   - Standard daily solar→battery→house cycles (solar covers house → clamps correctly to 100%)
 *
 * @param {number} solarKwh        - Solar production today (kWh)
 * @param {number} p1ExportKwh     - Total energy exported to grid today (kWh)
 * @param {number} batDischargeKwh - Battery discharged today (kWh), 0 if no battery
 * @param {number} houseKwh        - Total house consumption today (kWh)
 * @returns {number} Self-sufficiency percentage [0..100]
 */
function calcSelfSufficiency(solarKwh, p1ExportKwh, batDischargeKwh, houseKwh) {
	if (houseKwh <= 0) return 0;
	return Math.min(100, Math.max(0, (solarKwh - p1ExportKwh + batDischargeKwh) / houseKwh * 100));
}
