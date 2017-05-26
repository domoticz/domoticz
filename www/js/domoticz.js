/*
 (c) 2012-2015 Domoticz.com, Robbert E. Peters
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

function SendX10Command(idx,switchcmd,refreshfunction,passcode)
{
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
					  SendX10Command(idx,"Arm Home",refreshfunction,passcode)
				  }
			  },
			  {
				  text: $.t("Arm Away"),
				  click: function () {
					  $dialog.remove();
					  SendX10Command(idx,"Arm Away",refreshfunction,passcode)
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
					  SendX10Command(idx,"Arm Home",refreshfunction,passcode)
				  }
			  },
			  {
				  text: $.t("Arm Away"),
				  click: function () {
					  $dialog.remove();
					  switchcmd = "Arm Away";
					  SendX10Command(idx,"Arm Away",refreshfunction,passcode)
				  }
			  },
			  {
				  text: $.t("Panic"),
				  click: function () {
					  $dialog.remove();
					  switchcmd = "Panic";
					  SendX10Command(idx,"Panic",refreshfunction,passcode)
				  }
			  },
			  {
				  text: $.t("Disarm"),
				  click: function () {
					  $dialog.remove();
					  switchcmd = "Disarm";
					  SendX10Command(idx,"Disarm",refreshfunction,passcode)
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
					  SendX10Command(idx,"Normal",refreshfunction,passcode);
				  }
			  },
			  {
				  text: $.t("Alarm"),
				  click: function () {
					  $dialog.remove();
					  SendX10Command(idx,"Alarm",refreshfunction,passcode);
				  }
			  },
			  {
				  text: $.t("Arm Home"),
				  click: function () {
					  $dialog.remove();
					  SendX10Command(idx,"Arm Home",refreshfunction,passcode);
				  }
			  },
			  {
				  text: $.t("Arm Home Delayed"),
				  click: function () {
					  $dialog.remove();
					  SendX10Command(idx,"Arm Home Delayed",refreshfunction,passcode);
				  }
			  },
			  {
				  text: $.t("Arm Away"),
				  click: function () {
					  $dialog.remove();
					  SendX10Command(idx,"Arm Away",refreshfunction,passcode);
				  }
			  },
			  {
				  text: $.t("Arm Away Delayed"),
				  click: function () {
					  $dialog.remove();
					  SendX10Command(idx,"Arm Away Delayed",refreshfunction,passcode);
				  }
			  },
			  {
				  text: $.t("Panic"),
				  click: function () {
					  $dialog.remove();
					  SendX10Command(idx,"Panic",refreshfunction,passcode);
				  }
			  },
			  {
				  text: $.t("Disarm"),
				  click: function () {
					  $dialog.remove();
					  SendX10Command(idx,"Disarm",refreshfunction,passcode);
				  }
			  },
			  {
				  text: $.t("Light On"),
				  click: function () {
					  $dialog.remove();
					  SendX10Command(idx,"Light On",refreshfunction,passcode);
				  }
			  },
			  {
				  text: $.t("Light Off"),
				  click: function () {
					  $dialog.remove();
					  SendX10Command(idx,"Light Off",refreshfunction,passcode);
				  }
			  },
			  {
				  text: $.t("Light 2 On"),
				  click: function () {
					  $dialog.remove();
					  SendX10Command(idx,"Light 2 On",refreshfunction,passcode);
				  }
			  },
			  {
				  text: $.t("Light 2 Off"),
				  click: function () {
					  $dialog.remove();
					  SendX10Command(idx,"Light 2 Off",refreshfunction,passcode);
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

function RefreshLightLogTable(idx) {
	var mTable = $($.content + ' #lighttable');
	var oTable = mTable.dataTable();
	oTable.fnClearTable();

	$.ajax({
		url: "json.htm?type=lightlog&idx=" + idx,
		async: false,
		dataType: 'json',
		success: function (data) {
			if (typeof data.result != 'undefined') {
				var datatable = [];
				var chart = $.LogChart.highcharts();
				var ii = 0;
				var haveSelector = data.HaveSelector;
				$.each(data.result, function (i, item) {
					var addId = oTable.fnAddData([
						item.Date,
						item.Data
					], false);
					var level = -1;
					if (
						(item.Status.indexOf('Off') >= 0) ||
						(item.Status.indexOf('Disarm') >= 0) ||
						(item.Status.indexOf('No Motion') >= 0) ||
						(item.Status.indexOf('Normal') >= 0)
						) {
						level = 0;
					}
					else if (haveSelector === true) {
						level = parseInt(item.Level);
					}
					else if (item.Status.indexOf('Set Level:') == 0) {
						var lstr = item.Status.substr(11);
						var idx = lstr.indexOf('%');
						if (idx != -1) {
							lstr = lstr.substr(0, idx - 1);
							level = parseInt(lstr);
						}
					}
					else {
						var idx = item.Status.indexOf('Level: ');
						if (idx != -1) {
							var lstr = item.Status.substr(idx + 7);
							var idx = lstr.indexOf('%');
							if (idx != -1) {
								lstr = lstr.substr(0, idx - 1);
								level = parseInt(lstr);
								if (level > 100) {
									level = 100;
								}
							}
						}
						else {
							level = 100;
						}
					}
					if (level != -1) {
						datatable.unshift([GetUTCFromStringSec(item.Date), level]);
					}
				});
				mTable.fnDraw();
				chart.series[0].setData(datatable);
			}
		}
	});
}

function ClearLightLog() {
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
					RefreshLightLogTable($.devIdx);
				},
				error: function () {
					HideNotify();
					ShowNotify($.t('Problem clearing the Log!'), 2500, true);
				}
			});
		}
	});
}

function ShowLightLog(id, name, content, backfunction) {
	clearInterval($.myglobals.refreshTimer);
	$(window).scrollTop(0);
	$.content = content;

	$.devIdx = id;

	$('#modal').show();
	var htmlcontent = '';
	htmlcontent = '<p><h2>' + $.t('Name') + ': ' + unescape(name) + '</h2></p>\n';
	htmlcontent += $('#lightlog').html();
	$($.content).html(GetBackbuttonHTMLTable(backfunction) + htmlcontent);
	$($.content).i18n();
	$.LogChart = $($.content + ' #lightgraph');
	$.LogChart.highcharts({
		chart: {
			type: 'line',
			zoomType: 'x',
			resetZoomButton: {
				position: {
					x: -30,
					y: -36
				}
			},
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
				text: $.t('Percentage') + ' (%)'
			},
			endOnTick: false,
			startOnTick: false
		},
		tooltip: {
			formatter: function () {
				return '' +
				$.t(Highcharts.dateFormat('%A', this.x)) + '<br/>' + Highcharts.dateFormat('%Y-%m-%d %H:%M:%S', this.x) + ', ' + this.y + ' %';
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

	var oTable = $($.content + ' #lighttable').dataTable({
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

	RefreshLightLogTable($.devIdx);
	$('#modal').hide();
	return false;
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
				   "&tsendalways=" + nsettings.sendalways,
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
				"&tsendalways=" + nsettings.sendalways,
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
					systemsHTML += '<input type="checkbox" id="' + item.description + '" checked />&nbsp;' + item.name;
				});
				$($.content + ' #notificationparamstable #active_systems').html(systemsHTML);
			}
			if (typeof data.result != 'undefined') {
				$.each(data.result, function (i, item) {
					var parts = item.Params.split(';');
					var nvalue = 0;
					if (parts.length > 1) {
						nvalue = parts[2];
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
						else {
							nwhen = $.t("Below") + " ";
						}
						whenstr = nwhen + nvalue + stype;
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
						"CustomMessage": item.CustomMessage,
						"Priority": item.Priority,
						"SendAlways": item.SendAlways,
						"0": ntype,
						"1": whenstr,
						"2": ActiveSystemsString,
						"3": item.CustomMessage,
						"4": priorityStr,
						"5": $.t((item.SendAlways == true) ? "Yes" : "No")
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
				$($.content + " #notificationparamstable #combotype").val(GetValTextInNTypeStrArray(data["0"]));
				ShowNotificationTypeLabel();
				var matchstr = "^" + $.t("Greater");
				if (data["1"].match(matchstr)) {
					$($.content + " #notificationparamstable #combowhen").val(0);
				}
				else {
					$($.content + " #notificationparamstable #combowhen").val(1);
				}
				$($.content + " #notificationparamstable #value").val(data["nvalue"]);
				$($.content + " #notificationparamstable #combopriority").val(data["Priority"]);
				$($.content + " #notificationparamstable #checkalways").prop('checked', (data["SendAlways"] == true));
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
		return;
	}
	$($.content + " #notificationparamstable #notiwhen").show();
	$($.content + " #notificationparamstable #notival").show();

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

function ShowNotifications(id, name, content, backfunction) {
	clearInterval($.myglobals.refreshTimer);
	$.devIdx = id;
	$.content = content;

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
		$("#main-view").load(layout + ".html", function (response, status, xhr) {
			if (status == "error") {
				var msg = "Sorry but there was an error: ";
				$("#main-view").html(msg + xhr.status + " " + xhr.statusText);
			}
		});
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

	window.location = '/#' + layout;
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

function GenerateCamImageURL(address, port, username, password, imageurl) {
	var feedsrc = "http://";
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
		rgb = rgb.match(/^rgba?\((\d+),\s*(\d+),\s*(\d+)(?:,\s*(\d+))?\)$/);
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
						retChart($.content, $.backfunction, $.devIdx, $.devName);
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
	
	var graphtype="counter";

	if (sensortype == "Visibility") {
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
			text: $.t(txtLabelOrg) + ' ' + Get5MinuteHistoryDaysGraphTitle()
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
			gridLineWidth: 0,
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
			name: $.t(txtLabelOrg)
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
							var minValue = 10000000;
							$.each(data.result, function (i, item) {
								datatable1.push([GetDateFromString(item.d), parseFloat(item.v_min)]);
								datatable2.push([GetDateFromString(item.d), parseFloat(item.v_max)]);
								minValue = Math.min(item.v_min, minValue);
								minValue = Math.min(item.v_max, minValue);
							});
							$.LogChart2.highcharts().yAxis[0].update({ min: minValue });
							var series1 = $.LogChart2.highcharts().series[0];
							var series2 = $.LogChart2.highcharts().series[1];
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
			text: $.t(txtLabelOrg) + ' ' + $.t('Last Month')
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
			gridLineWidth: 0,
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
			name: 'min',
			point: {
				events: {
					click: function (event) {
						chartPointClickNewGeneral(event, false, ShowGeneralGraph);
					}
				}
			}
		}, {
			name: 'max',
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
							var minValue = 10000000;

							$.each(data.result, function (i, item) {
								datatable1.push([GetDateFromString(item.d), parseFloat(item.v_min)]);
								datatable2.push([GetDateFromString(item.d), parseFloat(item.v_max)]);
								minValue = Math.min(item.v_min, minValue);
								minValue = Math.min(item.v_max, minValue);
							});
							$.LogChart3.highcharts().yAxis[0].update({ min: minValue });
							var series1 = $.LogChart3.highcharts().series[0];
							var series2 = $.LogChart3.highcharts().series[1];
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
			text: $.t(txtLabelOrg) + ' ' + $.t('Last Year')
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
			gridLineWidth: 0,
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
			name: 'min',
			point: {
				events: {
					click: function (event) {
						chartPointClickNewGeneral(event, false, ShowGeneralGraph);
					}
				}
			}
		}, {
			name: 'max',
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
		chart.addSeries(
		  {
			  id: 'humidity',
			  name: $.t('Humidity'),
			  color: 'limegreen',
			  yAxis: 1,
			  tooltip: {
				  valueSuffix: ' %',
				  valueDecimals: 0
			  }
		  }
		);
		series = chart.get('humidity');
		series.setData(datatablehu);
	}

	if (datatablech.length != 0) {
		chart.addSeries(
		  {
			  id: 'chill',
			  name: $.t('Chill'),
			  color: 'red',
			  zIndex: 1,
			  tooltip: {
				  valueSuffix: ' \u00B0' + $.myglobals.tempsign,
				  valueDecimals: 1
			  },
			  yAxis: 0
		  }
		);
		series = chart.get('chill');
		series.setData(datatablech);

		if (isday == 0) {
			chart.addSeries(
			{
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
			});
			series = chart.get('chillmin');
			series.setData(datatablecm);
		}
	}

	if (datatablese.length != 0) {
		if (isday == 1) {
			chart.addSeries(
				{
					id: 'setpoint',
					name: $.t('Set Point'),
					color: 'blue',
					zIndex: 1,
					tooltip: {
						valueSuffix: ' \u00B0' + $.myglobals.tempsign,
						valueDecimals: 1
					},
					yAxis: 0
				}
			);
			series = chart.get('setpoint');
			series.setData(datatablese);
		} else {
			chart.addSeries(
				{
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
				}
			);
			series = chart.get('setpointavg');
			series.setData(datatablese);
			/*chart.addSeries(
			{
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
			});
			series = chart.get('setpointmin');
			series.setData(datatablesm);
			
			chart.addSeries(
			{
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
			});
			series = chart.get('setpointmax');
			series.setData(datatablesx);*/

			if (datatablesrange.length != 0) {
				chart.addSeries(
				  {
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
				  }
				);
				series = chart.get('setpointrange');
				series.setData(datatablesrange);
			}
			if (datatablese_prev.length != 0) {
				chart.addSeries(
				{
					id: 'prev_setpoint',
					name: 'Prev_' + $.t('Set Point'),
					color: 'rgba(223,212,246,0.8)',
					zIndex: 3,
					yAxis: 0,
					tooltip: {
						valueSuffix: ' \u00B0' + $.myglobals.tempsign,
						valueDecimals: 1
					}
				});
				series = chart.get('prev_setpoint');
				series.setData(datatablese_prev);
				series.setVisible(false);
			}
		}
	}

	if (datatablete.length != 0) {
		//Add Temperature series

		if (isday == 1) {
			chart.addSeries(
			{
				id: 'temperature',
				name: $.t('Temperature'),
				color: 'yellow',
				yAxis: 0,
				tooltip: {
					valueSuffix: ' \u00B0' + $.myglobals.tempsign,
					valueDecimals: 1
				}
			}
		  );
			series = chart.get('temperature');
			series.setData(datatablete);
		}
		else {
			//Min/Max range
			if (datatableta.length != 0) {
				chart.addSeries(
				  {
					  id: 'temperature_avg',
					  name: $.t('Temperature') + '_avg',
					  color: 'yellow',
					  fillOpacity: 0.7,
					  yAxis: 0,
					  zIndex: 2,
					  tooltip: {
						  valueSuffix: ' \u00B0' + $.myglobals.tempsign,
						  valueDecimals: 1
					  }
				  }
				);
				series = chart.get('temperature_avg');
				series.setData(datatableta);
				var trandLine = CalculateTrendLine(datatableta);
				if (typeof trandLine != 'undefined') {
					var datatableTrendline = [];

					datatableTrendline.push([trandLine.x0, trandLine.y0]);
					datatableTrendline.push([trandLine.x1, trandLine.y1]);
				}
			}
			if (datatabletrange.length != 0) {
				chart.addSeries(
				  {
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
				  }
				);
				series = chart.get('temperature');
				series.setData(datatabletrange);
			}
			if (datatablete_prev.length != 0) {
				chart.addSeries(
				{
					id: 'prev_temperature',
					name: 'Prev_' + $.t('Temperature'),
					color: 'rgba(224,224,230,0.8)',
					zIndex: 3,
					yAxis: 0,
					tooltip: {
						valueSuffix: ' \u00B0' + $.myglobals.tempsign,
						valueDecimals: 1
					}
				});
				series = chart.get('prev_temperature');
				series.setData(datatablete_prev);
				series.setVisible(false);
			}
		}
	}
	if (typeof datatableTrendline != 'undefined') {
		if (datatableTrendline.length > 0) {
			chart.addSeries({
				id: 'temp_trendline',
				name: $.t('Trendline') + '_' + $.t('Temperature'),
				zIndex: 3,
				tooltip: {
					valueSuffix: ' \u00B0' + $.myglobals.tempsign,
					valueDecimals: 1
				},
				color: 'rgba(255,255,255,0.8)',
				yAxis: 0
			});
			series = chart.get('temp_trendline');
			series.setData(datatableTrendline);
			series.setVisible(false);
		}
	}
	return;
	if (datatabledp.length != 0) {
		chart.addSeries(
		  {
			  id: 'dewpoint',
			  name: $.t('Dew Point'),
			  color: 'blue',
			  yAxis: 0,
			  tooltip: {
				  valueSuffix: ' \u00B0' + $.myglobals.tempsign,
				  valueDecimals: 1
			  }
		  }
		);
		series = chart.get('dewpoint');
		series.setData(datatabledp);
	}
	if (datatableba.length != 0) {
		chart.addSeries(
		  {
			  id: 'baro',
			  name: $.t('Barometer'),
			  color: 'pink',
			  yAxis: 2,
			  tooltip: {
				  valueSuffix: ' hPa',
				  valueDecimals: 1
			  }
		  }
		);
		series = chart.get('baro');
		series.setData(datatableba);
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
	if (devIdx.length > 0) {
		$.ajax({
			url: "json.htm?type=command&param=kodimediacommand&idx=" + devIdx + "&action=" + action,
			async: true,
			dataType: 'json',
			//			 success: function(data) { $.cachenoty=generate_noty('info', '<b>Sent remote command</b>', 100); },
			error: function () { $.cachenoty = generate_noty('error', '<b>Problem sending remote command</b>', 1000); }
		});
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

function ShowMediaRemote(Name, devIdx, HWType) {
	var divId;
	var svgId;
	if (HWType.indexOf('Kodi') >= 0)
	{
		divId = '#dialog-media-remote';
		svgId = '#MediaRemote';
	}
	else if (HWType.indexOf('Logitech Media Server') >= 0)
	{
		divId = '#dialog-lmsplayer-remote';
		svgId = '#LMSPlayerRemote';
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

function ShowMonthReportTemp(actMonth, actYear) {
	var htmlcontent = '';

	htmlcontent += $('#toptextmonthtemp').html();
	htmlcontent += $('#reportviewtempmonth').html();
	$($.content).html(htmlcontent);
	$($.content).i18n();
	$($.content + ' #ddays').hide();
	$($.content + ' #ftext').html($.t('Day'));

	$($.content + ' #theader').html(unescape($.devName) + " " + $.t($.monthNames[actMonth - 1]) + " " + actYear);

	$($.content + ' #munit').html('\u00B0 ' + $.myglobals.tempsign);

	$($.content + ' #reporttable').dataTable({
		"sDom": '<"H"rC>t<"F">',
		"oTableTools": {
			"sRowSelect": "single"
		},
		"aaSorting": [[0, "asc"]],
		"aoColumnDefs": [
			{ "bSortable": false, "aTargets": [5] }
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
			maxPadding: 0.2,
			endOnTick: false,
			title: {
				text: $.t('Temperature')
			}
		},
		tooltip: {
			formatter: function () {
				return $.t(Highcharts.dateFormat('%A', this.x)) + ' ' + Highcharts.dateFormat('%B %d', this.x) + '<br/>' + this.series.name + ': ' + this.y + ' ' + '\u00B0 ' + $.myglobals.tempsign;
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

	var month_temp_high = -1;
	var month_temp_high_pos = 0;
	var month_temp_high_date = {};
	var month_temp_low = -1;
	var month_temp_low_pos = 0;
	var month_temp_low_date = {};
	var month_hum_high = -1;
	var month_hum_high_pos = 0;
	var month_hum_high_date = {};
	var total_degree_days = 0;
	
	$.getJSON("json.htm?type=graph&sensor=temp&idx=" + $.devIdx + "&range=year&actmonth=" + actMonth + "&actyear=" + actYear,
	function (data) {
		var lastTotal = -1;
		$.each(data.result, function (i, item) {
			var month = parseInt(item.d.substring(5, 7), 10);
			var year = parseInt(item.d.substring(0, 4), 10);

			if ((month == actMonth) && (year == actYear)) {
				var day = parseInt(item.d.substring(8, 10), 10);

				var max_value = parseFloat(item.te);
				var min_value = parseFloat(item.tm);
				var avg_value = parseFloat(item.ta);
				var hum_value = parseFloat(item.hu);

				if ((month_temp_high == -1) || (max_value > month_temp_high)) {
					month_temp_high = max_value;
					month_temp_high_date.day = day;
					month_temp_high_date.month = month;
					month_temp_high_date.year = year;
					month_temp_high_pos = datachart.length;
				}
				if ((month_temp_low == -1) || (min_value < month_temp_low)) {
					month_temp_low = min_value;
					month_temp_low_date.day = day;
					month_temp_low_date.month = month;
					month_temp_low_date.year = year;
					month_temp_low_pos = datachart.length;
				}
				if (typeof item.hu != 'undefined') {
					if ((month_hum_high == -1) || (hum_value > month_hum_high)) {
						month_hum_high = hum_value;
						month_hum_high_date.day = day;
						month_hum_high_date.month = month;
						month_hum_high_date.year = year;
						month_hum_high_pos = datachart.length;
					}
				}

				//Degree Days
				var mvalue = parseFloat(item.ta);
				if (mvalue<$.myglobals.DegreeDaysBaseTemperature) {
					total_degree_days+= ($.myglobals.DegreeDaysBaseTemperature-mvalue);
				}

				var cdate = Date.UTC(actYear, actMonth - 1, day);
				var cday = $.t(dateFormat(cdate, "dddd"));
				datachart.push({ x: cdate, y: max_value, color: 'rgba(3,190,252,0.8)' });

				var img;
				if ((lastTotal == -1) || (lastTotal == max_value)) {
					img = '<img src="images/equal.png"></img>';
				}
				else if (max_value < lastTotal) {
					img = '<img src="images/down.png"></img>';
				}
				else {
					img = '<img src="images/up.png"></img>';
				}
				lastTotal = max_value;

				if (typeof item.hu == 'undefined') {
					hum_value = "-";
				}

				var addId = oTable.fnAddData([
					  day,
					  cday,
					  hum_value,
					  avg_value.toFixed(1),
					  min_value.toFixed(1),
					  max_value.toFixed(1),
					  img
				], false);
			}
		});

		total_degree_days=Math.round( total_degree_days * 10 ) / 10;
		if (total_degree_days) {
			$($.content + ' #mddays').html(total_degree_days);
			$($.content + ' #ddays').show();
		}

		var hlstring = "-";
		if (month_temp_high != -1) {
			hlstring = month_temp_high_date.day + " " + $.t($.monthNames[month_temp_high_date.month - 1]) + ", " + month_temp_high.toFixed(1) + " " + '\u00B0 ' + $.myglobals.tempsign;
			datachart[month_temp_high_pos].color = "#FF0000";
		}
		$($.content + ' #mhigh').html(hlstring);

		hlstring = "-";
		if (month_temp_low != -1) {
			hlstring = month_temp_low_date.day + " " + $.t($.monthNames[month_temp_low_date.month - 1]) + ", " + month_temp_low.toFixed(1) + " " + '\u00B0 ' + $.myglobals.tempsign;
		}
		$($.content + ' #mlow').html(hlstring);

		$.UsageChart.highcharts().addSeries({
			id: 'temp',
			name: $.t('Temp'),
			showInLegend: false,
			color: 'rgba(3,190,252,0.8)',
			yAxis: 0
		});
		var series = $.UsageChart.highcharts().get('temp');
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

function Add2YearTableReportTemp(oTable, avg_temp, hum, temp_low, temp_high, lastTotal, lastMonth, actYear) {
	var img;
	if ((lastTotal == -1) || (lastTotal == avg_temp)) {
		img = '<img src="images/equal.png"></img>';
	}
	else if (avg_temp < lastTotal) {
		img = '<img src="images/down.png"></img>';
	}
	else {
		img = '<img src="images/up.png"></img>';
	}
	var monthtxt = addLeadingZeros(parseInt(lastMonth), 2) + ". " + $.t($.monthNames[lastMonth - 1]) + " ";
	monthtxt += '<img src="images/next.png" onclick="ShowMonthReportTemp(' + lastMonth + ',' + actYear + ')">';

	if (hum == -1) {
		hum = "-";
	}

	var addId = oTable.fnAddData([
		  monthtxt,
		  hum,
		  avg_temp.toFixed(1),
		  temp_low.toFixed(1),
		  temp_high.toFixed(1),
		  img
	], false);
	return avg_temp;
}

function ShowYearReportTemp(actYear) {
	if (actYear == 0) {
		actYear = $.actYear;
	}
	else {
		$.actYear = actYear;
	}

	$.monthNames = ["January", "February", "March", "April", "May", "June",
	"July", "August", "September", "October", "November", "December"];

	var htmlcontent = '';
	htmlcontent += $('#toptextyearreporttemp').html();
	htmlcontent += $('#reportviewtemp').html();

	$($.content).html(htmlcontent);
	$($.content + ' #backbutton').click(function (e) {
		eval($.backfunction)();
	});
	$($.content).i18n();
	$($.content + ' #ddays').hide();
	$($.content + ' #ftext').html($.t('Month'));

	$($.content + ' #theader').html(unescape($.devName) + " " + actYear);

	$($.content + ' #munit').html('\u00B0 ' + $.myglobals.tempsign);

	$($.content + ' #comboyear').val(actYear);

	$($.content + ' #comboyear').change(function () {
		var yearidx = $($.content + ' #comboyear option:selected').val();
		if (typeof yearidx == 'undefined') {
			return;
		}
		ShowYearReportTemp(yearidx);
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
			{ "bSortable": false, "aTargets": [5] }
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
			maxPadding: 0.2,
			endOnTick: false,
			title: {
				text: $.t('Temperature')
			}
		},
		tooltip: {
			formatter: function () {
				return $.t(Highcharts.dateFormat('%B', this.x)) + '<br/>' + this.series.name + ': ' + this.y + '\u00B0 ' + $.myglobals.tempsign
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
	var datachart = [];

	var month_temp_high = -1;
	var month_temp_low = -1;
	var month_hum_high = -1;

	var highest_month_val = -1;
	var highest_month = 0;
	var highest_month_day = 0;
	var highest_month_pos = 0;

	var lowest_month_val = -1;
	var lowest_month = 0;
	var lowest_day = 0;
	
	var total_degree_days = 0;

	$.getJSON("json.htm?type=graph&sensor=temp&idx=" + $.devIdx + "&range=year&actyear=" + actYear,
	function (data) {
		var lastTotal = -1;
		var lastMonth = -1;
		var monthvalues = 0;
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
					var month_avg = total;
					if (monthvalues > 0) {
						month_avg = total / monthvalues;
					}
					lastTotal = Add2YearTableReportTemp(oTable, month_avg, month_hum_high, month_temp_low, month_temp_high, lastTotal, lastMonth, actYear);

					var cdate = Date.UTC(actYear, lastMonth - 1, 1);
					datachart.push({ x: cdate, y: parseFloat(month_avg.toFixed(1)), color: 'rgba(3,190,252,0.8)' });

					lastMonth = month;
					total = 0;
					monthvalues = 0;
					month_temp_high = -1;
					month_temp_low = -1;
					month_hum_high = -1;
				}
				var mvalue = parseFloat(item.te);
				total += parseFloat(item.ta);
				monthvalues++;

				//max
				if ((month_temp_high == -1) || (mvalue > month_temp_high)) {
					month_temp_high = mvalue;
				}
				if ((highest_month_val == -1) || (mvalue > highest_month_val)) {
					highest_month_val = mvalue;
					highest_month_pos = datachart.length;
					highest_month_day = day;
					highest_month = month;
				}
				//min
				mvalue = parseFloat(item.tm);
				if ((month_temp_low == -1) || (mvalue < month_temp_low)) {
					month_temp_low = mvalue;
				}
				if ((lowest_month_val == -1) || (mvalue < lowest_month_val)) {
					lowest_month_val = mvalue;
					lowest_month = month;
					lowest_month_day = day;
				}
				//hum
				if (typeof item.hu != 'undefined') {
					mvalue = parseFloat(item.hu);
					if ((month_hum_high == -1) || (mvalue > month_hum_high)) {
						month_hum_high = mvalue;
					}
				}
				//Degree Days
				var mvalue = parseFloat(item.ta);
				if (mvalue<$.myglobals.DegreeDaysBaseTemperature) {
					total_degree_days+= ($.myglobals.DegreeDaysBaseTemperature-mvalue);
				}
			}
		});
		total_degree_days=Math.round( total_degree_days * 10 ) / 10;
		if (total_degree_days) {
			$($.content + ' #mddays').html(total_degree_days);
			$($.content + ' #ddays').show();
		}

		//add last month
		if (total != 0) {
			var month_avg = total;
			if (monthvalues > 0) {
				month_avg = total / monthvalues;
			}
			lastTotal = Add2YearTableReportTemp(oTable, month_avg, month_hum_high, month_temp_low, month_temp_high, lastTotal, lastMonth, actYear);
			var cdate = Date.UTC(actYear, lastMonth - 1, 1);
			datachart.push({ x: cdate, y: parseFloat(month_avg.toFixed(1)), color: 'rgba(3,190,252,0.8)' });
		}

		var hlstring = "-";
		if (highest_month_val != -1) {
			hlstring = highest_month_day + " " + $.t($.monthNames[highest_month - 1]) + ", " + highest_month_val.toFixed(1) + " " + '\u00B0 ' + $.myglobals.tempsign;
			datachart[highest_month_pos].color = "#FF0000";
		}
		$($.content + ' #mhigh').html(hlstring);

		hlstring = "-";
		if (lowest_month_val != -1) {
			hlstring = lowest_month_day + " " + $.t($.monthNames[lowest_month - 1]) + ", " + lowest_month_val.toFixed(1) + " " + '\u00B0 ' + $.myglobals.tempsign;
		}
		$($.content + ' #mlow').html(hlstring);

		$.UsageChart.highcharts().addSeries({
			id: 'temp',
			name: $.t('Temperature'),
			showInLegend: false,
			color: 'rgba(3,190,252,0.8)',
			yAxis: 0
		});
		var series = $.UsageChart.highcharts().get('temp');
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

function ShowTempLog(contentdiv, backfunction, id, name) {
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

	var d = new Date();
	var actMonth = d.getMonth() + 1;
	var actYear = d.getYear() + 1900;
	$($.content).html(GetBackbuttonHTMLTableWithRight(backfunction, 'ShowYearReportTemp(' + actYear + ')', $.t('Report')) + htmlcontent);
	$($.content).i18n();

	var tempstr = "Celsius";
	if ($.myglobals.tempsign == "F") {
		tempstr = "Fahrenheit";
	}

	$.DayChart = $($.content + ' #daygraph');
	$.DayChart.highcharts({
		chart: {
			type: 'line',
			zoomType: 'x',
			resetZoomButton: {
				position: {
					x: -30,
					y: -36
				}
			},
            alignTicks: false,
			events: {
				load: function () {
					this.showLoading();
					$.getJSON("json.htm?type=graph&sensor=temp&idx=" + id + "&range=day",
					function (data) {
						if (typeof data.result != 'undefined') {
							AddDataToTempChart(data, $.DayChart.highcharts(), 1);
						}
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
			text: $.t('Temperature') + ' ' + Get5MinuteHistoryDaysGraphTitle()
		},
		xAxis: {
			type: 'datetime'
		},
		yAxis: [{ //temp label
			labels: {
				formatter: function () {
					return this.value + '\u00B0 ' + $.myglobals.tempsign;
				},
				style: {
					color: '#CCCC00'
				}
			},
			title: {
				text: 'degrees ' + tempstr,
				style: {
					color: '#CCCC00'
				}
			}
		}, { //humidity label
			labels: {
				formatter: function () {
					return this.value + '%';
				},
				style: {
					color: 'limegreen'
				}
			},
			title: {
				text: $.t('Humidity') + ' %',
				style: {
					color: '#00CC00'
				}
			},
			opposite: true
		}],
		tooltip: {
			crosshairs: true,
			shared: true
		},
		legend: {
			enabled: true
		},
		plotOptions: {
			series: {
				point: {
					events: {
						click: function (event) {
							chartPointClickNew(event, true, ShowTempLog);
						}
					}
				}
			},
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
			zoomType: 'x',
			resetZoomButton: {
				position: {
					x: -30,
					y: -36
				}
			},
			alignTicks: false,
			events: {
				load: function () {
					this.showLoading();
					$.getJSON("json.htm?type=graph&sensor=temp&idx=" + id + "&range=month",
					function (data) {
						if (typeof data.result != 'undefined') {
							AddDataToTempChart(data, $.MonthChart.highcharts(), 0);
						}
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
			text: $.t('Temperature') + ' ' + $.t('Last Month')
		},
		xAxis: {
			type: 'datetime'
		},
		yAxis: [{ //temp label
			labels: {
				format: '{value}\u00B0 ' + $.myglobals.tempsign,
				style: {
					color: '#CCCC00'
				}
			},
			title: {
				text: 'degrees ' + tempstr,
				style: {
					color: '#CCCC00'
				}
			}
		}, { //humidity label
			labels: {
				format: '{value}%',
				style: {
					color: 'limegreen'
				}
			},
			title: {
				text: $.t('Humidity') + ' %',
				style: {
					color: '#00CC00'
				}
			},
			opposite: true
		}],
		tooltip: {
			crosshairs: true,
			shared: true
		},
		legend: {
			enabled: true
		},
		plotOptions: {
			series: {
				point: {
					events: {
						click: function (event) {
							chartPointClickNew(event, false, ShowTempLog);
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
			zoomType: 'x',
			resetZoomButton: {
				position: {
					x: -30,
					y: -36
				}
			},
			alignTicks: false,
			events: {
				load: function () {
					this.showLoading();
					$.getJSON("json.htm?type=graph&sensor=temp&idx=" + id + "&range=year",
					function (data) {
						if (typeof data.result != 'undefined') {
							AddDataToTempChart(data, $.YearChart.highcharts(), 0);
						}
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
			text: $.t('Temperature') + ' ' + $.t('Last Year')
		},
		xAxis: {
			type: 'datetime'
		},
		yAxis: [{ //temp label
			labels: {
				format: '{value}\u00B0 ' + $.myglobals.tempsign,
				style: {
					color: '#CCCC00'
				}
			},
			title: {
				text: 'degrees ' + tempstr,
				style: {
					color: '#CCCC00'
				}
			}
		}, { //humidity label
			labels: {
				format: '{value}%',
				style: {
					color: 'limegreen'
				}
			},
			title: {
				text: $.t('Humidity') + ' %',
				style: {
					color: '#00CC00'
				}
			},
			opposite: true
		}],
		tooltip: {
			crosshairs: true,
			shared: true
		},
		legend: {
			enabled: true
		},
		plotOptions: {
			series: {
				point: {
					events: {
						click: function (event) {
							chartPointClickNew(event, false, ShowTempLog);
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
				chart.addSeries(
					{
						id: 'current1',
						name: 'Current_L1',
						color: 'rgba(3,190,252,0.8)',
						yAxis: 0,
						tooltip: {
							valueSuffix: ' A',
							valueDecimals: 1
						}
					}
				);
				series = chart.get('current1');
				series.setData(datatablev1);
			}
			if (data.haveL2) {
				chart.addSeries(
					{
						id: 'current2',
						name: 'Current_L2',
						color: 'rgba(252,190,3,0.8)',
						yAxis: 0,
						tooltip: {
							valueSuffix: ' A',
							valueDecimals: 1
						}
					}
				);
				series = chart.get('current2');
				series.setData(datatablev2);
			}
			if (data.haveL3) {
				chart.addSeries(
					{
						id: 'current3',
						name: 'Current_L3',
						color: 'rgba(190,252,3,0.8)',
						yAxis: 0,
						tooltip: {
							valueSuffix: ' A',
							valueDecimals: 1
						}
					}
				);
				series = chart.get('current3');
				series.setData(datatablev3);
			}
		}
		else {
			//month/year
			if (data.haveL1) {
				chart.addSeries(
					{
						id: 'current1min',
						name: 'Current_L1_Min',
						color: 'rgba(3,190,252,0.8)',
						yAxis: 0
					}
				);
				chart.addSeries(
					{
						id: 'current1max',
						name: 'Current_L1_Max',
						color: 'rgba(3,252,190,0.8)',
						yAxis: 0,
						tooltip: {
							valueSuffix: ' A',
							valueDecimals: 1
						}
					}
				);
				series = chart.get('current1min');
				series.setData(datatablev1);
				series = chart.get('current1max');
				series.setData(datatablev2);
			}
			if (data.haveL2) {
				chart.addSeries(
					{
						id: 'current2min',
						name: 'Current_L2_Min',
						color: 'rgba(252,190,3,0.8)',
						yAxis: 0,
						tooltip: {
							valueSuffix: ' A',
							valueDecimals: 1
						}
					}
				);
				chart.addSeries(
					{
						id: 'current2max',
						name: 'Current_L2_Max',
						color: 'rgba(252,3,190,0.8)',
						yAxis: 0,
						tooltip: {
							valueSuffix: ' A',
							valueDecimals: 1
						}
					}
				);
				series = chart.get('current2min');
				series.setData(datatablev3);
				series = chart.get('current2max');
				series.setData(datatablev4);
			}
			if (data.haveL3) {
				chart.addSeries(
					{
						id: 'current3min',
						name: 'Current_L3_Min',
						color: 'rgba(190,252,3,0.8)',
						yAxis: 0,
						tooltip: {
							valueSuffix: ' A',
							valueDecimals: 1
						}
					}
				);
				chart.addSeries(
					{
						id: 'current3max',
						name: 'Current_L3_Min',
						color: 'rgba(112,146,190,0.8)',
						yAxis: 0,
						tooltip: {
							valueSuffix: ' A',
							valueDecimals: 1
						}
					}
				);
				series = chart.get('current3min');
				series.setData(datatablev5);
				series = chart.get('current3max');
				series.setData(datatablev6);
			}
		}
		chart.yAxis[0].axisTitle.attr({
			text: 'Current (A)'
		});
	}
	else if (switchtype == 1) {
		//Watt
		if (isday == 1) {
			if (data.haveL1) {
				chart.addSeries(
					{
						id: 'current1',
						name: 'Usage_L1',
						color: 'rgba(3,190,252,0.8)',
						yAxis: 0,
						tooltip: {
							valueSuffix: ' Watt',
							valueDecimals: 1
						}
					}
				);
				series = chart.get('current1');
				series.setData(datatablev1);
			}
			if (data.haveL2) {
				chart.addSeries(
					{
						id: 'current2',
						name: 'Usage_L2',
						color: 'rgba(252,190,3,0.8)',
						yAxis: 0,
						tooltip: {
							valueSuffix: ' Watt',
							valueDecimals: 1
						}
					}
				);
				series = chart.get('current2');
				series.setData(datatablev2);
			}
			if (data.haveL3) {
				chart.addSeries(
					{
						id: 'current3',
						name: 'Usage_L3',
						color: 'rgba(190,252,3,0.8)',
						yAxis: 0,
						tooltip: {
							valueSuffix: ' Watt',
							valueDecimals: 1
						}
					}
				);
				series = chart.get('current3');
				series.setData(datatablev3);
			}
		}
		else {
			if (data.haveL1) {
				chart.addSeries(
					{
						id: 'current1min',
						name: 'Usage_L1_Min',
						color: 'rgba(3,190,252,0.8)',
						yAxis: 0,
						tooltip: {
							valueSuffix: ' Watt',
							valueDecimals: 1
						}
					}
				);
				chart.addSeries(
					{
						id: 'current1max',
						name: 'Usage_L1_Max',
						color: 'rgba(3,252,190,0.8)',
						yAxis: 0,
						tooltip: {
							valueSuffix: ' Watt',
							valueDecimals: 1
						}
					}
				);
				series = chart.get('current1min');
				series.setData(datatablev1);
				series = chart.get('current1max');
				series.setData(datatablev2);
			}
			if (data.haveL2) {
				chart.addSeries(
					{
						id: 'current2min',
						name: 'Usage_L2_Min',
						color: 'rgba(252,190,3,0.8)',
						yAxis: 0,
						tooltip: {
							valueSuffix: ' Watt',
							valueDecimals: 1
						}
					}
				);
				chart.addSeries(
					{
						id: 'current2max',
						name: 'Usage_L2_Max',
						color: 'rgba(252,3,190,0.8)',
						yAxis: 0,
						tooltip: {
							valueSuffix: ' Watt',
							valueDecimals: 1
						}
					}
				);
				series = chart.get('current2min');
				series.setData(datatablev3);
				series = chart.get('current2max');
				series.setData(datatablev4);
			}
			if (data.haveL3) {
				chart.addSeries(
					{
						id: 'current3min',
						name: 'Usage_L3_Min',
						color: 'rgba(190,252,3,0.8)',
						yAxis: 0,
						tooltip: {
							valueSuffix: ' Watt',
							valueDecimals: 1
						}
					}
				);
				chart.addSeries(
					{
						id: 'current3max',
						name: 'Usage_L3_Min',
						color: 'rgba(112,146,190,0.8)',
						yAxis: 0,
						tooltip: {
							valueSuffix: ' Watt',
							valueDecimals: 1
						}
					}
				);
				series = chart.get('current3min');
				series.setData(datatablev5);
				series = chart.get('current3max');
				series.setData(datatablev6);
			}
		}
		chart.yAxis[0].axisTitle.attr({
			text: 'Usage (Watt)'
		});
	}
	chart.yAxis[0].redraw();
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
		switchtype=$.devSwitchType;
	}
	var htmlcontent = '';
	htmlcontent = '<p><center><h2>' + unescape(name) + '</h2></center></p>\n';
	htmlcontent += $('#daymonthyearlog').html();
	$($.content).html(GetBackbuttonHTMLTable(backfunction) + htmlcontent);
	$($.content).i18n();

	$.DayChart = $($.content + ' #daygraph');
	$.DayChart.highcharts({
		chart: {
			type: 'line',
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
							AddDataToCurrentChart(data, $.DayChart.highcharts(), switchtype, 1);
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
							AddDataToCurrentChart(data, $.MonthChart.highcharts(), switchtype, 0);
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

					$.getJSON("json.htm?type=graph&sensor=counter&idx=" + id + "&range=year",
					function (data) {
						if (typeof data.result != 'undefined') {
							AddDataToCurrentChart(data, $.YearChart.highcharts(), switchtype, 0);
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

					$.getJSON("json.htm?type=graph&sensor=uv&idx=" + id + "&range=day",
					function (data) {
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
		}]
		  ,
		legend: {
			enabled: false
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

					$.getJSON("json.htm?type=graph&sensor=uv&idx=" + id + "&range=month",
					function (data) {
						if (typeof data.result != 'undefined') {
							var series = $.MonthChart.highcharts().series[0];
							var datatable = [];
							$.each(data.result, function (i, item) {
								datatable.push([GetDateFromString(item.d), parseFloat(item.uvi)]);
							});
							series.setData(datatable);
							var bHavePrev = (typeof data.resultprev != 'undefined');
							if (bHavePrev) {
								datatable = [];
								series = $.MonthChart.highcharts().series[1];
								$.each(data.resultprev, function (i, item) {
									datatable.push([GetPrevDateFromString(item.d), parseFloat(item.uvi)]);
								});
								series.setData(datatable);
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
			name: 'Prev_uv',
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

					$.getJSON("json.htm?type=graph&sensor=uv&idx=" + id + "&range=year",
					function (data) {
						if (typeof data.result != 'undefined') {
							var series = $.YearChart.highcharts().series[0];
							var datatable = [];
							$.each(data.result, function (i, item) {
								datatable.push([GetDateFromString(item.d), parseFloat(item.uvi)]);
							});
							series.setData(datatable);
							var bHavePrev = (typeof data.resultprev != 'undefined');
							if (bHavePrev) {
								datatable = [];
								series = $.YearChart.highcharts().series[1];
								$.each(data.resultprev, function (i, item) {
									datatable.push([GetPrevDateFromString(item.d), parseFloat(item.uvi)]);
								});
								series.setData(datatable);
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
			name: 'Prev_uv',
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
	if ($.myglobals.windsign=="bf") {
		lscales.push({"from": 0 , "to": 1  });
		lscales.push({"from": 1 , "to":  2  });
		lscales.push({"from": 2 , "to":  3  });
		lscales.push({"from": 3 , "to":  4  });
		lscales.push({"from": 4 , "to":  5  });
		lscales.push({"from": 5 , "to":  6  });
		lscales.push({"from": 6 , "to":  7 });
		lscales.push({"from": 7 , "to":  8  });
		lscales.push({"from": 8 , "to":  9  });
		lscales.push({"from": 9 , "to":  10  });
		lscales.push({"from": 10 , "to":  11  });
		lscales.push({"from": 11 , "to":  12  });
		lscales.push({"from": 12 , "to":  100  });
	} else {
		lscales.push({"from": 0.3 * $.myglobals.windscale, "to": 1.5 * $.myglobals.windscale });
		lscales.push({"from": 1.5 * $.myglobals.windscale, "to":  3.5 * $.myglobals.windscale });
		lscales.push({"from": 1.5 * $.myglobals.windscale, "to":  3.5 * $.myglobals.windscale });
		lscales.push({"from": 3.5 * $.myglobals.windscale, "to":  5.5 * $.myglobals.windscale });
		lscales.push({"from": 5.5 * $.myglobals.windscale, "to":  8 * $.myglobals.windscale });
		lscales.push({"from": 8 * $.myglobals.windscale, "to":  10.8 * $.myglobals.windscale });
		lscales.push({"from": 10.8 * $.myglobals.windscale, "to":  13.9 * $.myglobals.windscale});
		lscales.push({"from": 13.9 * $.myglobals.windscale, "to":  17.2 * $.myglobals.windscale });
		lscales.push({"from": 17.2 * $.myglobals.windscale, "to":  20.8 * $.myglobals.windscale });
		lscales.push({"from": 20.8 * $.myglobals.windscale, "to":  24.5 * $.myglobals.windscale });
		lscales.push({"from": 24.5 * $.myglobals.windscale, "to":  28.5 * $.myglobals.windscale });
		lscales.push({"from": 28.5 * $.myglobals.windscale, "to":  32.7 * $.myglobals.windscale });
		lscales.push({"from": 32.7 * $.myglobals.windscale, "to":  100 * $.myglobals.windscale });
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
					$.getJSON("json.htm?type=graph&sensor=wind&idx=" + id + "&range=day",
					function (data) {
						if (typeof data.result != 'undefined') {
							var seriessp = $.DayChart.highcharts().series[0];
							var seriesgu = $.DayChart.highcharts().series[1];
							var datatablesp = [];
							var datatablegu = [];

							$.each(data.result, function (i, item) {
								datatablesp.push([GetUTCFromString(item.d), parseFloat(item.sp)]);
								datatablegu.push([GetUTCFromString(item.d), parseFloat(item.gu)]);
							});
							seriessp.setData(datatablesp);
							seriesgu.setData(datatablegu);
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
				color: 'rgba(68, 170, 213, 0.3)',
				label: {
					text: $.t('Light air'),
					style: {
						color: '#CCCCCC'
					}
				}
			}, { // Light breeze
				from: lscales[1]["from"],
				to: lscales[1]["to"],
				color: 'rgba(68, 170, 213, 0.5)',
				label: {
					text: $.t('Light breeze'),
					style: {
						color: '#CCCCCC'
					}
				}
			}, { // Gentle breeze
				from: lscales[2]["from"],
				to: lscales[2]["to"],
				color: 'rgba(68, 170, 213, 0.3)',
				label: {
					text: $.t('Gentle breeze'),
					style: {
						color: '#CCCCCC'
					}
				}
			}, { // Moderate breeze
				from: lscales[3]["from"],
				to: lscales[3]["to"],
				color: 'rgba(68, 170, 213, 0.5)',
				label: {
					text: $.t('Moderate breeze'),
					style: {
						color: '#CCCCCC'
					}
				}
			}, { // Fresh breeze
				from: lscales[4]["from"],
				to: lscales[4]["to"],
				color: 'rgba(68, 170, 213, 0.3)',
				label: {
					text: $.t('Fresh breeze'),
					style: {
						color: '#CCCCCC'
					}
				}
			}, { // Strong breeze
				from: lscales[5]["from"],
				to: lscales[5]["to"],
				color: 'rgba(68, 170, 213, 0.5)',
				label: {
					text: $.t('Strong breeze'),
					style: {
						color: '#CCCCCC'
					}
				}
			}, { // High wind
				from: lscales[6]["from"],
				to: lscales[6]["from"],
				color: 'rgba(68, 170, 213, 0.3)',
				label: {
					text: $.t('High wind'),
					style: {
						color: '#CCCCCC'
					}
				}
			}, { // fresh gale
				from: lscales[7]["from"],
				to: lscales[7]["to"],
				color: 'rgba(68, 170, 213, 0.3)',
				label: {
					text: $.t('Fresh gale'),
					style: {
						color: '#CCCCCC'
					}
				}
			}, { // strong gale
				from: lscales[8]["from"],
				to: lscales[8]["to"],
				color: 'rgba(68, 170, 213, 0.5)',
				label: {
					text: $.t('Strong gale'),
					style: {
						color: '#CCCCCC'
					}
				}
			}, { // storm
				from: lscales[9]["from"],
				to: lscales[9]["to"],
				color: 'rgba(68, 170, 213, 0.3)',
				label: {
					text: $.t('Storm'),
					style: {
						color: '#CCCCCC'
					}
				}
			}, { // Violent storm
				from: lscales[10]["from"],
				to: lscales[10]["to"],
				color: 'rgba(68, 170, 213, 0.5)',
				label: {
					text: $.t('Violent storm'),
					style: {
						color: '#CCCCCC'
					}
				}
			}, { // hurricane
				from: lscales[11]["from"],
				to: lscales[11]["to"],
				color: 'rgba(68, 170, 213, 0.3)',
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
			name: $.t('Speed'),
			tooltip: {
				valueSuffix: ' ' + $.myglobals.windsign,
				valueDecimals: 1
			}
		}, {
			name: $.t('Gust'),
			tooltip: {
				valueSuffix: ' ' + $.myglobals.windsign,
				valueDecimals: 1
			}
		}]
		  ,
		navigation: {
			menuItemStyle: {
				fontSize: '10px'
			}
		}
	});

	$.SpeedChart = $($.content + ' #windspeedgraph');
	$.SpeedChart.highcharts({
		chart: {
			polar: true,
			type: 'column',
			events: {
				load: function () {
					$.getJSON("json.htm?type=graph&sensor=winddir&idx=" + id + "&range=day",
					function (data) {
						if (typeof data.result_speed != 'undefined') {
							$.each(data.result_speed, function (i, item) {
								//make the series
								var datatable_speed = [];
								for (jj = 0; jj < 16; jj++) {
									datatable_speed.push([wind_directions[i], parseFloat(item.sp[jj])]);
								}
								$.SpeedChart.highcharts().addSeries(
								{
									id: i + 1,
									name: item.label
								});
								var series = $.SpeedChart.highcharts().get(i + 1);
								series.setData(datatable_speed);
							});
						}
					});
				}
			}
		},
		title: {
			text: $.t('Wind') + ' ' + $.t('Gust') + ' ' + Get5MinuteHistoryDaysGraphTitle()
		},
		pane: {
			size: '85%'
		},
		legend: {
			align: 'right',
			verticalAlign: 'top',
			y: 100,
			layout: 'vertical'
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
			endOnTick: false,
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

					$.getJSON("json.htm?type=graph&sensor=wind&idx=" + id + "&range=month",
					function (data) {
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
							$.each(data.resultprev, function (i, item) {
								datatable_prev_sp.push([GetPrevDateFromString(item.d), parseFloat(item.sp)]);
								datatable_prev_gu.push([GetPrevDateFromString(item.d), parseFloat(item.gu)]);
							});
							seriessp.setData(datatablesp);
							seriesgu.setData(datatablegu);
							series_prev_sp.setData(datatable_prev_sp);
							series_prev_gu.setData(datatable_prev_gu);
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
				color: 'rgba(68, 170, 213, 0.3)',
				label: {
					text: $.t('Light air'),
					style: {
						color: '#CCCCCC'
					}
				}
			}, { // Light breeze
				from: lscales[1]["from"],
				to: lscales[1]["to"],
				color: 'rgba(68, 170, 213, 0.5)',
				label: {
					text: $.t('Light breeze'),
					style: {
						color: '#CCCCCC'
					}
				}
			}, { // Gentle breeze
				from: lscales[2]["from"],
				to: lscales[2]["to"],
				color: 'rgba(68, 170, 213, 0.3)',
				label: {
					text: $.t('Gentle breeze'),
					style: {
						color: '#CCCCCC'
					}
				}
			}, { // Moderate breeze
				from: lscales[3]["from"],
				to: lscales[3]["to"],
				color: 'rgba(68, 170, 213, 0.5)',
				label: {
					text: $.t('Moderate breeze'),
					style: {
						color: '#CCCCCC'
					}
				}
			}, { // Fresh breeze
				from: lscales[4]["from"],
				to: lscales[4]["to"],
				color: 'rgba(68, 170, 213, 0.3)',
				label: {
					text: $.t('Fresh breeze'),
					style: {
						color: '#CCCCCC'
					}
				}
			}, { // Strong breeze
				from: lscales[5]["from"],
				to: lscales[5]["to"],
				color: 'rgba(68, 170, 213, 0.5)',
				label: {
					text: $.t('Strong breeze'),
					style: {
						color: '#CCCCCC'
					}
				}
			}, { // High wind
				from: lscales[6]["from"],
				to: lscales[6]["to"],
				color: 'rgba(68, 170, 213, 0.3)',
				label: {
					text: $.t('High wind'),
					style: {
						color: '#CCCCCC'
					}
				}
			}, { // fresh gale
				from: lscales[7]["from"],
				to: lscales[7]["to"],
				color: 'rgba(68, 170, 213, 0.3)',
				label: {
					text: $.t('Fresh gale'),
					style: {
						color: '#CCCCCC'
					}
				}
			}, { // strong gale
				from: lscales[8]["from"],
				to: lscales[8]["to"],
				color: 'rgba(68, 170, 213, 0.5)',
				label: {
					text: $.t('Strong gale'),
					style: {
						color: '#CCCCCC'
					}
				}
			}, { // storm
				from: lscales[9]["from"],
				to: lscales[9]["to"],
				color: 'rgba(68, 170, 213, 0.3)',
				label: {
					text: $.t('Storm'),
					style: {
						color: '#CCCCCC'
					}
				}
			}, { // Violent storm
				from: lscales[10]["from"],
				to: lscales[10]["to"],
				color: 'rgba(68, 170, 213, 0.5)',
				label: {
					text: $.t('Violent storm'),
					style: {
						color: '#CCCCCC'
					}
				}
			}, { // hurricane
				from: lscales[11]["from"],
				to: lscales[11]["to"],
				color: 'rgba(68, 170, 213, 0.3)',
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
			name: 'Prev_' + $.t('Speed'),
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
			name: 'Prev_' + $.t('Gust'),
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

					$.getJSON("json.htm?type=graph&sensor=wind&idx=" + id + "&range=year",
					function (data) {
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
							$.each(data.resultprev, function (i, item) {
								datatable_prev_sp.push([GetPrevDateFromString(item.d), parseFloat(item.sp)]);
								datatable_prev_gu.push([GetPrevDateFromString(item.d), parseFloat(item.gu)]);
							});

							seriessp.setData(datatablesp);
							seriesgu.setData(datatablegu);
							series_prev_sp.setData(datatable_prev_sp);
							series_prev_gu.setData(datatable_prev_gu);
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
				color: 'rgba(68, 170, 213, 0.3)',
				label: {
					text: $.t('Light air'),
					style: {
						color: '#CCCCCC'
					}
				}
			}, { // Light breeze
				from: lscales[1]["from"],
				to: lscales[1]["to"],
				color: 'rgba(68, 170, 213, 0.5)',
				label: {
					text: $.t('Light breeze'),
					style: {
						color: '#CCCCCC'
					}
				}
			}, { // Gentle breeze
				from: lscales[2]["from"],
				to: lscales[2]["to"],
				color: 'rgba(68, 170, 213, 0.3)',
				label: {
					text: $.t('Gentle breeze'),
					style: {
						color: '#CCCCCC'
					}
				}
			}, { // Moderate breeze
				from: lscales[3]["from"],
				to: lscales[3]["to"],
				color: 'rgba(68, 170, 213, 0.5)',
				label: {
					text: $.t('Moderate breeze'),
					style: {
						color: '#CCCCCC'
					}
				}
			}, { // Fresh breeze
				from: lscales[4]["from"],
				to: lscales[4]["to"],
				color: 'rgba(68, 170, 213, 0.3)',
				label: {
					text: $.t('Fresh breeze'),
					style: {
						color: '#CCCCCC'
					}
				}
			}, { // Strong breeze
				from: lscales[5]["from"],
				to: lscales[5]["to"],
				color: 'rgba(68, 170, 213, 0.5)',
				label: {
					text: $.t('Strong breeze'),
					style: {
						color: '#CCCCCC'
					}
				}
			}, { // High wind
				from: lscales[6]["from"],
				to: lscales[6]["to"],
				color: 'rgba(68, 170, 213, 0.3)',
				label: {
					text: $.t('High wind'),
					style: {
						color: '#CCCCCC'
					}
				}
			}, { // fresh gale
				from: lscales[7]["from"],
				to: lscales[7]["to"],
				color: 'rgba(68, 170, 213, 0.3)',
				label: {
					text: $.t('Fresh gale'),
					style: {
						color: '#CCCCCC'
					}
				}
			}, { // strong gale
				from: lscales[8]["from"],
				to: lscales[8]["to"],
				color: 'rgba(68, 170, 213, 0.5)',
				label: {
					text: $.t('Strong gale'),
					style: {
						color: '#CCCCCC'
					}
				}
			}, { // storm
				from: lscales[9]["from"],
				to: lscales[9]["to"],
				color: 'rgba(68, 170, 213, 0.3)',
				label: {
					text: $.t('Storm'),
					style: {
						color: '#CCCCCC'
					}
				}
			}, { // Violent storm
				from: lscales[10]["from"],
				to: lscales[10]["to"],
				color: 'rgba(68, 170, 213, 0.5)',
				label: {
					text: $.t('Violent storm'),
					style: {
						color: '#CCCCCC'
					}
				}
			}, { // hurricane
				from: lscales[11]["from"],
				to: lscales[11]["to"],
				color: 'rgba(68, 170, 213, 0.3)',
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
			name: 'Prev_' + $.t('Speed'),
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
			name: 'Prev_' + $.t('Gust'),
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
			endOnTick: false,
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
			endOnTick: false,
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

					$.getJSON("json.htm?type=graph&sensor=rain&idx=" + id + "&range=day",
					function (data) {
						if (typeof data.result != 'undefined') {
							var series = $.DayChart.highcharts().series[0];
							var datatable = [];

							$.each(data.result, function (i, item) {
								datatable.push([GetUTCFromString(item.d), parseFloat(item.mm)]);
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
			text: $.t('Rainfall') + ' ' + Get5MinuteHistoryDaysGraphTitle()
		},
		xAxis: {
			type: 'datetime'
		},
		yAxis: {
			min: 0,
			maxPadding: 0.2,
			endOnTick: false,
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
			name: 'mm',
			color: 'rgba(3,190,252,0.8)'
		}]
		  ,
		legend: {
			enabled: false
		}
	});

	$.WeekChart = $($.content + ' #weekgraph');
	$.WeekChart.highcharts({
		chart: {
			type: 'column',
			events: {
				load: function () {

					$.getJSON("json.htm?type=graph&sensor=rain&idx=" + id + "&range=week",
					function (data) {
						if (typeof data.result != 'undefined') {
							var series = $.WeekChart.highcharts().series[0];
							var datatable = [];
							$.each(data.result, function (i, item) {
								datatable.push([GetDateFromString(item.d), parseFloat(item.mm)]);
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
			endOnTick: false,
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
			name: 'mm',
			color: 'rgba(3,190,252,0.8)'
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
			zoomType: 'x',
			resetZoomButton: {
				position: {
					x: -30,
					y: -36
				}
			},
			events: {
				load: function () {
					$.getJSON("json.htm?type=graph&sensor=rain&idx=" + id + "&range=month",
					function (data) {
						if (typeof data.result != 'undefined') {
							var series = $.MonthChart.highcharts().series[0];
							var datatable = [];
							$.each(data.result, function (i, item) {
								datatable.push([GetDateFromString(item.d), parseFloat(item.mm)]);
							});
							series.setData(datatable);
							var bHavePrev = (typeof data.resultprev != 'undefined');
							if (bHavePrev) {
								datatable = [];
								series = $.MonthChart.highcharts().series[1];
								$.each(data.resultprev, function (i, item) {
									datatable.push([GetPrevDateFromString(item.d), parseFloat(item.mm)]);
								});
								series.setData(datatable);
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
			name: 'Prev_mm',
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

					$.getJSON("json.htm?type=graph&sensor=rain&idx=" + id + "&range=year",
					function (data) {
						if (typeof data.result != 'undefined') {
							var series = $.YearChart.highcharts().series[0];
							var datatable = [];
							$.each(data.result, function (i, item) {
								datatable.push([GetDateFromString(item.d), parseFloat(item.mm)]);
							});
							series.setData(datatable);
							var bHavePrev = (typeof data.resultprev != 'undefined');
							if (bHavePrev) {
								datatable = [];
								series = $.YearChart.highcharts().series[1];
								$.each(data.resultprev, function (i, item) {
									datatable.push([GetPrevDateFromString(item.d), parseFloat(item.mm)]);
								});
								series.setData(datatable);
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
			name: 'Prev_mm',
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
					$.getJSON("json.htm?type=graph&sensor=temp&idx=" + id + "&range=day",
					function (data) {
						if (typeof data.result != 'undefined') {
							var series = $.DayChart.highcharts().series[0];
							var datatable = [];
							$.each(data.result, function (i, item) {
								datatable.push([GetUTCFromString(item.d), parseFloat(item.ba)]);
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
			enabled: false
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

					$.getJSON("json.htm?type=graph&sensor=temp&idx=" + id + "&range=month",
					function (data) {
						if (typeof data.result != 'undefined') {
							var series = $.MonthChart.highcharts().series[0];
							var datatable = [];
							$.each(data.result, function (i, item) {
								datatable.push([GetDateFromString(item.d), parseFloat(item.ba)]);
							});
							series.setData(datatable);
							var bHavePrev = (typeof data.resultprev != 'undefined');
							if (bHavePrev) {
								datatable = [];
								series = $.MonthChart.highcharts().series[1];
								$.each(data.resultprev, function (i, item) {
									datatable.push([GetPrevDateFromString(item.d), parseFloat(item.ba)]);
								});
								$.MonthChart.highcharts().addSeries(
								{
									id: 'prev_baro',
									name: 'Prev_Baro',
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
								});
								series = $.MonthChart.highcharts().get('prev_baro');
								series.setData(datatable);
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

					$.getJSON("json.htm?type=graph&sensor=temp&idx=" + id + "&range=year",
					function (data) {
						if (typeof data.result != 'undefined') {
							var series = $.YearChart.highcharts().series[0];
							var datatable = [];

							$.each(data.result, function (i, item) {
								datatable.push([GetDateFromString(item.d), parseFloat(item.ba)]);
							});
							series.setData(datatable);
							var bHavePrev = (typeof data.resultprev != 'undefined');
							if (bHavePrev) {
								datatable = [];
								$.each(data.resultprev, function (i, item) {
									datatable.push([GetPrevDateFromString(item.d), parseFloat(item.ba)]);
								});
								$.YearChart.highcharts().addSeries(
								{
									id: 'prev_baro',
									name: 'Prev_Baro',
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
								});
								series = $.YearChart.highcharts().get('prev_baro');
								series.setData(datatable);
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

					$.getJSON("json.htm?type=graph&sensor=counter&idx=" + id + "&range=day",
					function (data) {
						if (typeof data.result != 'undefined') {
							var series = $.DayChart.highcharts().series[0];
							var datatable = [];

							$.each(data.result, function (i, item) {
								datatable.push([GetUTCFromString(item.d), parseInt(item.co2)]);
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
				color: 'rgba(68, 170, 213, 0.3)',
				label: {
					text: $.t('Excellent'),
					style: {
						color: '#CCCCCC'
					}
				}
			}, { // Good
				from: 700,
				to: 900,
				color: 'rgba(68, 170, 213, 0.5)',
				label: {
					text: $.t('Good'),
					style: {
						color: '#CCCCCC'
					}
				}
			}, { // Fair
				from: 900,
				to: 1100,
				color: 'rgba(68, 170, 213, 0.3)',
				label: {
					text: $.t('Fair'),
					style: {
						color: '#CCCCCC'
					}
				}
			}, { // Mediocre
				from: 1100,
				to: 1600,
				color: 'rgba(68, 170, 213, 0.5)',
				label: {
					text: $.t('Mediocre'),
					style: {
						color: '#CCCCCC'
					}
				}
			}, { // Bad
				from: 1600,
				to: 6000,
				color: 'rgba(68, 170, 213, 0.3)',
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
			name: 'co2',
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

					$.getJSON("json.htm?type=graph&sensor=counter&idx=" + id + "&range=month",
					function (data) {
						if (typeof data.result != 'undefined') {
							var datatable1 = [];
							var datatable2 = [];

							$.each(data.result, function (i, item) {
								datatable1.push([GetDateFromString(item.d), parseInt(item.co2_min)]);
								datatable2.push([GetDateFromString(item.d), parseInt(item.co2_max)]);
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
				color: 'rgba(68, 170, 213, 0.3)',
				label: {
					text: $.t('Excellent'),
					style: {
						color: '#CCCCCC'
					}
				}
			}, { // Good
				from: 700,
				to: 900,
				color: 'rgba(68, 170, 213, 0.5)',
				label: {
					text: $.t('Good'),
					style: {
						color: '#CCCCCC'
					}
				}
			}, { // Fair
				from: 900,
				to: 1100,
				color: 'rgba(68, 170, 213, 0.3)',
				label: {
					text: $.t('Fair'),
					style: {
						color: '#CCCCCC'
					}
				}
			}, { // Mediocre
				from: 1100,
				to: 1600,
				color: 'rgba(68, 170, 213, 0.5)',
				label: {
					text: $.t('Mediocre'),
					style: {
						color: '#CCCCCC'
					}
				}
			}, { // Bad
				from: 1600,
				to: 6000,
				color: 'rgba(68, 170, 213, 0.3)',
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
			name: 'co2_min',
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
			name: 'co2_max',
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
								datatable1.push([GetDateFromString(item.d), parseInt(item.co2_min)]);
								datatable2.push([GetDateFromString(item.d), parseInt(item.co2_max)]);
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
				color: 'rgba(68, 170, 213, 0.3)',
				label: {
					text: $.t('Excellent'),
					style: {
						color: '#CCCCCC'
					}
				}
			}, { // Good
				from: 700,
				to: 900,
				color: 'rgba(68, 170, 213, 0.5)',
				label: {
					text: $.t('Good'),
					style: {
						color: '#CCCCCC'
					}
				}
			}, { // Fair
				from: 900,
				to: 1100,
				color: 'rgba(68, 170, 213, 0.3)',
				label: {
					text: $.t('Fair'),
					style: {
						color: '#CCCCCC'
					}
				}
			}, { // Mediocre
				from: 1100,
				to: 1600,
				color: 'rgba(68, 170, 213, 0.5)',
				label: {
					text: $.t('Mediocre'),
					style: {
						color: '#CCCCCC'
					}
				}
			}, { // Bad
				from: 1600,
				to: 6000,
				color: 'rgba(68, 170, 213, 0.3)',
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
			name: 'co2_min',
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
			name: 'co2_max',
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

					$.getJSON("json.htm?type=graph&sensor=fan&idx=" + id + "&range=day",
					function (data) {
						if (typeof data.result != 'undefined') {
							var series = $.DayChart.highcharts().series[0];
							var datatable = [];

							$.each(data.result, function (i, item) {
								datatable.push([GetUTCFromString(item.d), parseInt(item.v)]);
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
			gridLineWidth: 0,
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
			enabled: false
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

					$.getJSON("json.htm?type=graph&sensor=fan&idx=" + id + "&range=month",
					function (data) {
						if (typeof data.result != 'undefined') {
							var datatable1 = [];
							var datatable2 = [];

							$.each(data.result, function (i, item) {
								datatable1.push([GetDateFromString(item.d), parseInt(item.v_min)]);
								datatable2.push([GetDateFromString(item.d), parseInt(item.v_max)]);
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
			gridLineWidth: 0,
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

					$.getJSON("json.htm?type=graph&sensor=fan&idx=" + id + "&range=year",
					function (data) {
						if (typeof data.result != 'undefined') {
							var datatable1 = [];
							var datatable2 = [];

							$.each(data.result, function (i, item) {
								datatable1.push([GetDateFromString(item.d), parseInt(item.v_min)]);
								datatable2.push([GetDateFromString(item.d), parseInt(item.v_max)]);
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
			gridLineWidth: 0,
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

					$.getJSON("json.htm?type=graph&sensor=Percentage&idx=" + id + "&range=day",
					function (data) {
						if (typeof data.result != 'undefined') {
							var series = $.DayChart.highcharts().series[0];
							var datatable = [];

							$.each(data.result, function (i, item) {
								datatable.push([GetUTCFromString(item.d), parseFloat(item.v)]);
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
			gridLineWidth: 0,
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

					$.getJSON("json.htm?type=graph&sensor=Percentage&idx=" + id + "&range=month",
					function (data) {
						if (typeof data.result != 'undefined') {
							var datatable1 = [];
							var datatable2 = [];
							var datatable3 = [];

							$.each(data.result, function (i, item) {
								datatable1.push([GetDateFromString(item.d), parseFloat(item.v_min)]);
								datatable2.push([GetDateFromString(item.d), parseFloat(item.v_max)]);
								datatable3.push([GetDateFromString(item.d), parseFloat(item.v_avg)]);
							});
							var series1 = $.MonthChart.highcharts().series[0];
							var series2 = $.MonthChart.highcharts().series[1];
							var series3 = $.MonthChart.highcharts().series[2];
							series1.setData(datatable1);
							series2.setData(datatable2);
							series3.setData(datatable3);
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
			gridLineWidth: 0,
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

					$.getJSON("json.htm?type=graph&sensor=Percentage&idx=" + id + "&range=year",
					function (data) {
						if (typeof data.result != 'undefined') {
							var datatable1 = [];
							var datatable2 = [];
							var datatable3 = [];

							$.each(data.result, function (i, item) {
								datatable1.push([GetDateFromString(item.d), parseFloat(item.v_min)]);
								datatable2.push([GetDateFromString(item.d), parseFloat(item.v_max)]);
								datatable3.push([GetDateFromString(item.d), parseFloat(item.v_avg)]);
							});
							var series1 = $.YearChart.highcharts().series[0];
							var series2 = $.YearChart.highcharts().series[1];
							var series3 = $.YearChart.highcharts().series[2];
							series1.setData(datatable1);
							series2.setData(datatable2);
							series3.setData(datatable3);
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
			gridLineWidth: 0,
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
	var method = 1;
	if (typeof data.method != 'undefined') {
		method = data.method;
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
			if (switchtype != 2) {
				var fValue=parseFloat(item.v);
				if (fValue % 1 != 0)
					bHaveFloat=true;
				datatableUsage1.push([cdate, fValue]);
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
				chart.highcharts().addSeries({
					id: 'eUsed',
					type: 'area',
					name: (switchtype == 0) ? $.t('Energy Used') : $.t('Energy Generated'),
					tooltip: {
						valueSuffix: (chart == $.WeekChart) ? ' kWh' : ' Wh',
						valueDecimals: totDecimals
					},
					color: 'rgba(120,150,220,0.9)',
					fillOpacity: 0.2,
					yAxis: 1
				});
				series = chart.highcharts().get('eUsed');
				series.setData(datatableEnergyUsed);
				series.setVisible(false);
			}
			if (datatableEnergyGenerated.length > 0) {
				chart.highcharts().addSeries({
					id: 'eGen',
					type: 'area',
					name: $.t('Energy Generated'),
					tooltip: {
						valueSuffix: (chart == $.WeekChart) ? ' kWh' : ' Wh',
						valueDecimals: totDecimals
					},
					color: 'rgba(120,220,150,0.9)',
					fillOpacity: 0.2,
					yAxis: 1
				});
				series = chart.highcharts().get('eGen');
				series.setData(datatableEnergyGenerated);
				series.setVisible(false);
			}
			if (datatableUsage1.length > 0) {
				if (datatableUsage2.length > 0) {
					chart.highcharts().addSeries({
						id: 'usage1',
						name: 'Usage_1',
						tooltip: {
							valueSuffix: (chart == $.WeekChart) ? ' kWh' : ' Watt',
							valueDecimals: totDecimals
						},
						color: 'rgba(60,130,252,0.8)',
						stack: 'susage',
						yAxis: 0
					});
				}
				else {
					var cType, cUnit;
					if (chart == $.DayChart) {
						cType = (method == 0) ? 'column' : 'spline';
						cUnit = (method == 0) ? 'Wh' : 'Watt';
					} else {
						cType = 'column';
						cUnit = 'kWh';
					}
					chart.highcharts().addSeries({
						id: 'usage1',
						name: (switchtype == 0) ? $.t('Usage') : $.t('Generated'),
						type: cType,
						tooltip: {
							valueSuffix: ' ' + cUnit,
							valueDecimals: totDecimals
						},
						color: 'rgba(3,190,252,0.8)',
						stack: 'susage',
						yAxis: 0
					});
				}
				series = chart.highcharts().get('usage1');
				series.setData(datatableUsage1);
			}
			if (datatableUsage2.length > 0) {
				chart.highcharts().addSeries({
					id: 'usage2',
					name: 'Usage_2',
					tooltip: {
						valueSuffix: (chart == $.WeekChart) ? ' kWh' : ' Watt',
						valueDecimals: totDecimals
					},
					color: 'rgba(3,190,252,0.8)',
					stack: 'susage',
					yAxis: 0
				});
				series = chart.highcharts().get('usage2');
				series.setData(datatableUsage2);
			}
			if (bHaveDelivered) {
				if (datatableReturn1.length > 0) {
					chart.highcharts().addSeries({
						id: 'return1',
						name: 'Return_1',
						tooltip: {
							valueSuffix: (chart == $.WeekChart) ? ' kWh' : ' Watt',
							valueDecimals: totDecimals
						},
						color: 'rgba(30,242,110,0.8)',
						stack: 'sreturn',
						yAxis: 0
					});
					series = chart.highcharts().get('return1');
					series.setData(datatableReturn1);
				}
				if (datatableReturn2.length > 0) {
					chart.highcharts().addSeries({
						id: 'return2',
						name: 'Return_2',
						tooltip: {
							valueSuffix: (chart == $.WeekChart) ? ' kWh' : ' Watt',
							valueDecimals: totDecimals
						},
						color: 'rgba(3,252,190,0.8)',
						stack: 'sreturn',
						yAxis: 0
					});
					series = chart.highcharts().get('return2');
					series.setData(datatableReturn2);
				}
			}
		}
		else {
			//month/year, show total for now
			if (datatableTotalUsage.length > 0) {
				chart.highcharts().addSeries({
					id: 'usage',
					name: (switchtype == 0) ? $.t('Total_Usage') : $.t('Total_Return'),
					zIndex: 2,
					tooltip: {
						valueSuffix: ' kWh',
						valueDecimals: 3
					},
					color: 'rgba(3,190,252,0.8)',
					yAxis: 0
				});
				series = chart.highcharts().get('usage');
				series.setData(datatableTotalUsage);
				var trandLine = CalculateTrendLine(datatableTotalUsage);
				if (typeof trandLine != 'undefined') {
					var datatableTrendlineUsage = [];

					datatableTrendlineUsage.push([trandLine.x0, trandLine.y0]);
					datatableTrendlineUsage.push([trandLine.x1, trandLine.y1]);

					chart.highcharts().addSeries({
						id: 'usage_trendline',
						name: (switchtype == 0) ? $.t('Trendline_Usage') : $.t('Trendline_Return'),
						zIndex: 1,
						tooltip: {
							valueSuffix: ' kWh',
							valueDecimals: 3
						},
						color: 'rgba(252,3,3,0.8)',
						yAxis: 0
					});
					series = chart.highcharts().get('usage_trendline');
					series.setData(datatableTrendlineUsage);
					series.setVisible(false);
				}
			}
			if (bHaveDelivered) {
				if (datatableTotalReturn.length > 0) {
					chart.highcharts().addSeries({
						id: 'return',
						name: 'Total_Return',
						zIndex: 1,
						tooltip: {
							valueSuffix: ' kWh',
							valueDecimals: 3
						},
						color: 'rgba(3,252,190,0.8)',
						yAxis: 0
					});
					series = chart.highcharts().get('return');
					series.setData(datatableTotalReturn);
					var trandLine = CalculateTrendLine(datatableTotalReturn);
					if (typeof trandLine != 'undefined') {
						var datatableTrendlineReturn = [];

						datatableTrendlineReturn.push([trandLine.x0, trandLine.y0]);
						datatableTrendlineReturn.push([trandLine.x1, trandLine.y1]);

						chart.highcharts().addSeries({
							id: 'return_trendline',
							name: 'Trendline_Return',
							zIndex: 1,
							tooltip: {
								valueSuffix: ' kWh',
								valueDecimals: 3
							},
							color: 'rgba(255,127,39,0.8)',
							yAxis: 0
						});
						series = chart.highcharts().get('return_trendline');
						series.setData(datatableTrendlineReturn);
						series.setVisible(false);
					}
				}
			}
			if (datatableTotalUsagePrev.length > 0) {
				chart.highcharts().addSeries({
					id: 'usageprev',
					name: (switchtype == 0) ? $.t('Past_Usage') : $.t('Past_Return'),
					tooltip: {
						valueSuffix: ' kWh',
						valueDecimals: 3
					},
					color: 'rgba(190,3,252,0.8)',
					yAxis: 0
				});
				series = chart.highcharts().get('usageprev');
				series.setData(datatableTotalUsagePrev);
				series.setVisible(false);
			}
			if (bHaveDelivered) {
				if (datatableTotalReturnPrev.length > 0) {
					chart.highcharts().addSeries({
						id: 'returnprev',
						name: 'Past_Return',
						tooltip: {
							valueSuffix: ' kWh',
							valueDecimals: 3
						},
						color: 'rgba(252,190,3,0.8)',
						yAxis: 0
					});
					series = chart.highcharts().get('returnprev');
					series.setData(datatableTotalReturnPrev);
					series.setVisible(false);
				}
			}
		}

		if (chart == $.DayChart) {
			chart.highcharts().yAxis[0].axisTitle.attr({
				text: (method === 0) ? $.t('Energy') + ' Wh' : $.t('Power') + ' Watt'
			});
		} else {
			chart.highcharts().yAxis[0].axisTitle.attr({
				text: $.t('Energy') + ' kWh'
			});
		}
		chart.highcharts().yAxis[0].redraw();
	}
	else if (switchtype == 1) {
		//gas
		chart.highcharts().addSeries({
			id: 'gas',
			name: 'Gas',
			zIndex: 1,
			tooltip: {
				valueSuffix: ' m3',
				valueDecimals: 3
			},
			color: 'rgba(3,190,252,0.8)',
			yAxis: 0
		});
		if ((chart == $.MonthChart) || (chart == $.YearChart)) {
			var trandLine = CalculateTrendLine(datatableUsage1);
			if (typeof trandLine != 'undefined') {
				var datatableTrendlineUsage = [];

				datatableTrendlineUsage.push([trandLine.x0, trandLine.y0]);
				datatableTrendlineUsage.push([trandLine.x1, trandLine.y1]);

				chart.highcharts().addSeries({
					id: 'usage_trendline',
					name: 'Trendline_Gas',
					zIndex: 1,
					tooltip: {
						valueSuffix: ' m3',
						valueDecimals: 3
					},
					color: 'rgba(252,3,3,0.8)',
					yAxis: 0
				});
				series = chart.highcharts().get('usage_trendline');
				series.setData(datatableTrendlineUsage);
				series.setVisible(false);
			}
			if (datatableUsage1Prev.length > 0) {
				chart.highcharts().addSeries({
					id: 'gasprev',
					name: 'Past_Gas',
					tooltip: {
						valueSuffix: ' m3',
						valueDecimals: 3
					},
					color: 'rgba(190,3,252,0.8)',
					yAxis: 0
				});
				series = chart.highcharts().get('gasprev');
				series.setData(datatableUsage1Prev);
				series.setVisible(false);
			}
		}
		series = chart.highcharts().get('gas');
		series.setData(datatableUsage1);
		chart.highcharts().yAxis[0].axisTitle.attr({
			text: 'Gas m3'
		});
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
		});
		chart.highcharts().yAxis[0].axisTitle.attr({
			text: 'Water Liter'
		});
		series = chart.highcharts().get('water');
		series.setData(datatableUsage1);
	}
	else if (switchtype == 3) {
		//counter
		chart.highcharts().addSeries({
			id: 'counter',
			name: 'Counter',
			color: 'rgba(3,190,252,0.8)',
			yAxis: 0
		});
		chart.highcharts().yAxis[0].axisTitle.attr({
			text: 'Count'
		});
		series = chart.highcharts().get('counter');
		series.setData(datatableUsage1);
	}
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
	$.costsGas = 0.6218;
	$.costsWater = 1.6473;

	$.ajax({
		url: "json.htm?type=command&param=getcosts&idx=" + $.devIdx,
		async: false,
		dataType: 'json',
		success: function (data) {
			$.costsT1 = parseFloat(data.CostEnergy) / 10000;
			$.costsT2 = parseFloat(data.CostEnergyT2) / 10000;
			$.costsGas = parseFloat(data.CostGas) / 10000;
			$.costsWater = parseFloat(data.CostWater) / 10000;
			$.CounterT1 = parseFloat(data.CounterT1);
			$.CounterT2 = parseFloat(data.CounterT2);
			$.CounterR1 = parseFloat(data.CounterR1);
			$.CounterR2 = parseFloat(data.CounterR2);
		}
	});

	$.costsR1 = $.costsT1;
	$.costsR2 = $.costsT2;

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
			type: 'line',
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
				text: $.t('Usage')
			},
			min: 0,
			endOnTick: false,
			startOnTick: false
		},
		{
			title: {
				text: $.t('Energy') + ' (kWh)'
			},
			min: 0,
			endOnTick: false,
			startOnTick: false,
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
				var unit = {
					'Usage': 'Watt',
					'Usage_1': 'kWh',
					'Usage_2': 'kWh',
					'Return_1': 'kWh',
					'Return_2': 'kWh',
					'Gas': 'm3',
					'Past_Gas': 'm3',
					'Water': 'm3'
				}[this.series.name];
				return $.t(Highcharts.dateFormat('%A', this.x)) + ' ' + Highcharts.dateFormat('%Y-%m-%d', this.x) + '<br/>' + this.series.name + ': ' + this.y + ' ' + unit + '<br/>Total: ' + this.point.stackTotal + ' ' + unit;
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

	$($.content + ' #theader').html($.t("Usage") + " " + $.t($.monthNames[actMonth - 1]) + " " + actYear);

	if (($.devSwitchType == 0) || ($.devSwitchType == 4)) {
		//Electra
		$($.content + ' #munit').html("kWh");
	}
	else if ($.devSwitchType == 1) {
		//Gas
		$($.content + ' #munit').html("m3");
	}
	else {
		//Water
		$($.content + ' #munit').html("m3");
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
			endOnTick: false,
			title: {
				text: $.t('Usage')
			},
			min: 0
		},
		tooltip: {
			formatter: function () {
				var unit = {
					'Power': 'kWh',
					'Total_Usage': 'kWh',
					'Past_Usage': 'kWh',
					'Return': 'kWh',
					'Gas': 'm3',
					'Past_Gas': 'm3',
					'Water': 'm3'
				}[this.series.name];
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
					img = '<img src="images/down.png"></img>';
				}
				else {
					img = '<img src="images/up.png"></img>';
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
				id: 'power',
				name: 'Power',
				showInLegend: false,
				color: 'rgba(3,190,252,0.8)',
				yAxis: 0
			});
			var series = $.UsageChart.highcharts().get('power');
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
		img = '<img src="images/down.png"></img>';
	}
	else {
		img = '<img src="images/up.png"></img>';
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

	$($.content + ' #theader').html($.t("Usage") + " " + actYear);

	if (($.devSwitchType == 0) || ($.devSwitchType == 4)) {
		//Electra
		$($.content + ' #munit').html("kWh");
	}
	else if ($.devSwitchType == 1) {
		//Gas
		$($.content + ' #munit').html("m3");
	}
	else {
		//Water
		$($.content + ' #munit').html("m3");
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
			endOnTick: false,
			title: {
				text: $.t('Usage')
			},
			min: 0
		},
		tooltip: {
			formatter: function () {
				var unit = {
					'Power': 'kWh',
					'Total_Usage': 'kWh',
					'Past_Usage': 'kWh',
					'Return': 'kWh',
					'Gas': 'm3',
					'Past_Gas': 'm3',
					'Water': 'm3'
				}[this.series.name];
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
				id: 'power',
				name: 'Power',
				showInLegend: false,
				color: 'rgba(3,190,252,0.8)',
				yAxis: 0
			});
			var series = $.UsageChart.highcharts().get('power');
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

	$($.content + ' #theader').html($.t("Usage") + " " + $.t($.monthNames[actMonth - 1]) + " " + actYear);

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
				var unit = {
					'Usage': 'Watt',
					'Usage_1': 'kWh',
					'Usage_2': 'kWh',
					'Return_1': 'kWh',
					'Return_2': 'kWh',
					'Gas': 'm3',
					'Past_Gas': 'm3',
					'Water': 'm3'
				}[this.series.name];
				return $.t(Highcharts.dateFormat('%B', this.x)) + " " + Highcharts.dateFormat('%d', this.x) + '<br/>' + this.series.name + ': ' + this.y + ' ' + unit + '<br/>Total: ' + this.point.stackTotal + ' ' + unit;
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
		oTable.fnSetColumnVis(7, bHaveDelivered);
		oTable.fnSetColumnVis(8, bHaveDelivered);
		oTable.fnSetColumnVis(9, bHaveDelivered);
		oTable.fnSetColumnVis(10, bHaveDelivered);

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
					  textR1,
					  textCostR1,
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
					name: 'Usage_1',
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
				name: 'Usage_2',
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
					name: 'Return_1',
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
					name: 'Return_2',
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

	$($.content + ' #theader').html($.t("Usage") + " " + actYear);

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
				var unit = {
					'Usage': 'Watt',
					'Usage_1': 'kWh',
					'Usage_2': 'kWh',
					'Return_1': 'kWh',
					'Return_2': 'kWh',
					'Gas': 'm3',
					'Past_Gas': 'm3',
					'Water': 'm3'
				}[this.series.name];
				return $.t(Highcharts.dateFormat('%B', this.x)) + '<br/>' + this.series.name + ': ' + this.y + ' ' + unit + '<br/>Total: ' + this.point.stackTotal + ' ' + unit;
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
					name: 'Usage_1',
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
				name: 'Usage_2',
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
					name: 'Return_1',
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
					name: 'Return_2',
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
				$.costsGas = parseFloat(data.CostGas) / 10000;
				$.costsWater = parseFloat(data.CostWater) / 10000;
				$.DividerWater = 1000;//parseFloat(data.DividerWater);
			}
		});

		$.costsR1 = $.costsT1;
		$.costsR2 = $.costsT2;

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
			events: {
				load: function () {

					$.getJSON("json.htm?type=graph&sensor=counter&idx=" + id + "&range=day",
					function (data) {
						if (typeof data.result != 'undefined') {
							AddDataToUtilityChart(data, $.DayChart, switchtype);
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
				text: $.t('Energy') + ' (Watt)'
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
			enabled: false
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
			min: 0,
			maxPadding: 0.2,
			endOnTick: false,
			title: {
				text: $.t('Energy') + ' (kWh)'
			}
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
			enabled: false
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
		$.costsGas = 0.6218;
		$.costsWater = 1.6473;

		$.ajax({
			url: "json.htm?type=command&param=getcosts&idx=" + $.devIdx,
			async: false,
			dataType: 'json',
			success: function (data) {
				$.costsT1 = parseFloat(data.CostEnergy) / 10000;
				$.costsT2 = parseFloat(data.CostEnergyT2) / 10000;
				$.costsGas = parseFloat(data.CostGas) / 10000;
				$.costsWater = parseFloat(data.CostWater) / 10000;
			}
		});

		$.costsR1 = $.costsT1;
		$.costsR2 = $.costsT2;

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

					$.getJSON("json.htm?type=graph&sensor=counter&method=1&idx=" + id + "&range=day",
					function (data) {
						if (typeof data.result != 'undefined') {
							AddDataToUtilityChart(data, $.DayChart, switchtype);
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
				text: $.t('Power') + ' (Watt)'
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
							chartPointClickNew(event, true, ShowCounterLogSpline);
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
			enabled: false
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
			min: 0,
			maxPadding: 0.2,
			endOnTick: false,
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
			enabled: false
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
			name: 'Usage_min',
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
			name: 'Usage_max',
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
			name: 'Usage_min',
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
			name: 'Usage_max',
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
				text: 'Lux'
			},
			min: 0,
			minorGridLineWidth: 0,
			gridLineWidth: 0,
			alternateGridColor: null
		},
		tooltip: {
			formatter: function () {
				return '' +
				$.t(Highcharts.dateFormat('%A', this.x)) + '<br/>' + Highcharts.dateFormat('%Y-%m-%d %H:%M', this.x) + ': ' + this.y + ' Lux';
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
			name: 'lux',
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

							$.each(data.result, function (i, item) {
								datatable1.push([GetDateFromString(item.d), parseInt(item.lux_min)]);
								datatable2.push([GetDateFromString(item.d), parseInt(item.lux_max)]);
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
			text: $.t('Lux') + ' ' + $.t('Last Month')
		},
		xAxis: {
			type: 'datetime'
		},
		yAxis: {
			title: {
				text: 'Lux'
			},
			min: 0,
			minorGridLineWidth: 0,
			gridLineWidth: 0,
			alternateGridColor: null
		},
		tooltip: {
			formatter: function () {
				return '' +
				$.t(Highcharts.dateFormat('%A', this.x)) + '<br/>' + Highcharts.dateFormat('%Y-%m-%d', this.x) + ': ' + this.y + ' Lux';
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
			name: 'lux_min',
			point: {
				events: {
					click: function (event) {
						chartPointClickNew(event, false, ShowLuxLog);
					}
				}
			}
		}, {
			name: 'lux_max',
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
								datatable1.push([GetDateFromString(item.d), parseInt(item.lux_min)]);
								datatable2.push([GetDateFromString(item.d), parseInt(item.lux_max)]);
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
			text: $.t('Lux') + ' ' + $.t('Last Year')
		},
		xAxis: {
			type: 'datetime'
		},
		yAxis: {
			title: {
				text: 'Lux'
			},
			min: 0,
			minorGridLineWidth: 0,
			gridLineWidth: 0,
			alternateGridColor: null
		},
		tooltip: {
			formatter: function () {
				return '' +
				$.t(Highcharts.dateFormat('%A', this.x)) + '<br/>' + Highcharts.dateFormat('%Y-%m-%d', this.x) + ': ' + this.y + ' Lux';
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
			name: 'lux_min',
			point: {
				events: {
					click: function (event) {
						chartPointClickNew(event, false, ShowLuxLog);
					}
				}
			}
		}, {
			name: 'lux_max',
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
function ShowRGBWPopupInt(mouseX, mouseY, idx, refreshfunction, Protected, MaxDimLevel, LevelInt, hue) {
	$.devIdx = idx;
	$('#rgbw_popup #popup_switch_on').attr("href", 'javascript:SwitchLightPopup(' + idx + ',\'On\',' + refreshfunction + ',' + Protected + ');');
	$('#rgbw_popup #popup_switch_off').attr("href", 'javascript:SwitchLightPopup(' + idx + ',\'Off\',' + refreshfunction + ',' + Protected + ');');

	//Create Dimmer Sliders
	$('#rgbw_popup #slider').slider({
		//Config
		range: "min",
		min: 1,
		max: MaxDimLevel,
		value: LevelInt,

		//Slider Events
		create: function (event, ui) {
			$(this).slider("option", "max", $(this).data('maxlevel'));
			$(this).slider("option", "type", $(this).data('type'));
			$(this).slider("option", "isprotected", $(this).data('isprotected'));
			$(this).slider("value", $(this).data('svalue') + 1);
			if ($(this).data('disabled'))
				$(this).slider("option", "disabled", true);
		},
		slide: function (event, ui) { //When the slider is sliding
			clearInterval($.setDimValue);
			var maxValue = $(this).slider("option", "max");
			var dtype = $(this).slider("option", "type");
			var isProtected = $(this).slider("option", "isprotected");
			var fPercentage = 0;
			if (ui.value != 1) {
				fPercentage = parseInt((100.0 / (maxValue - 1)) * ((ui.value - 1)));
			}
			$.setDimValue = setInterval(function () { SetDimValue($.devIdx, ui.value); }, 500);
		},
		stop: function (event, ui) {
			SetDimValue($.devIdx, ui.value);
		}
	});

	//Popup
	$('#rgbw_popup #popup_picker').colpick({
		flat: true,
		layout: 'hex',
		submit: 0,
		onChange: function (hsb, hex, rgb, fromSetColor) {
			if (!fromSetColor) {
				clearInterval($.setColValue);
				var bIsWhite = (hsb.s < 20);
				$.setColValue = setInterval(function () { SetColValue($.devIdx, hsb.h, hsb.b, bIsWhite); }, 400);
			}
		}
	});

	$("#rgbw_popup").css({
		"top": mouseY,
		"left": mouseX + 15
	});
	$("#rgbw_popup").show();
}
function CloseRGBWPopup() {
	$("#rgbw_popup").hide();
}
function ShowRGBWPopup(event, idx, refreshfunction, Protected, MaxDimLevel, LevelInt, hue) {
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
		ShowRGBWPopupInt(mouseX, mouseY, idx, refreshfunction, Protected, MaxDimLevel, LevelInt, hue);
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
	var switchcmd="EnableSunWind";
	if (bDoEnable==false) {
		switchcmd="DisableSunWind";
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
	$.Protected=Protected;
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
