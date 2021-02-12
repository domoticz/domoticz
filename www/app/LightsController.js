define(['app', 'livesocket'], function (app) {
	app.controller('LightsController', function ($scope, $rootScope, $location, $http, $interval, $route, $routeParams, deviceApi, permissions, livesocket) {
		var $element = $('#main-view #lightcontent').last();

		$scope.HasInitializedAddManualDialog = false;

		MakeFavorite = function (id, isfavorite) {
			deviceApi.makeFavorite(id, isfavorite).then(function() {
				ShowLights();
			});
		};

		SetColValue = function (idx, color, brightness) {
			clearInterval($.setColValue);
			if (!permissions.hasPermission("User")) {
				HideNotify();
				ShowNotify($.t('You do not have permission to do that!'), 2500, true);
				return;
			}
			//TODO: Update local copy of device color instead of waiting for periodic AJAX poll of devices
			$.ajax({
				url: "json.htm?type=command&param=setcolbrightnessvalue&idx=" + idx + "&color=" + color + "&brightness=" + brightness,
				async: false,
				dataType: 'json'
			});
		};

		AddManualLightDevice = function () {
			$("#dialog-addmanuallightdevice #combosubdevice").html("");

			$.each($.LightsAndSwitches, function (i, item) {
				var option = $('<option />');
				option.attr('value', item.idx).text(item.name);
				$("#dialog-addmanuallightdevice #combosubdevice").append(option);
			});
			$("#dialog-addmanuallightdevice #comboswitchtype").val(0);
			$("#dialog-addmanuallightdevice").i18n();
			$("#dialog-addmanuallightdevice").dialog("open");
		}

		AddLightDevice = function () {
			$("#dialog-addlightdevice #combosubdevice").html("");
			$.each($.LightsAndSwitches, function (i, item) {
				var option = $('<option />');
				option.attr('value', item.idx).text(item.name);
				$("#dialog-addlightdevice #combosubdevice").append(option);
			});

			ShowNotify($.t('Press button on Remote...'));

			setTimeout(function () {
				var bHaveFoundDevice = false;
				var deviceID = "";
				var deviceidx = 0;
				var bIsUsed = false;
				var Name = "";

				$.ajax({
					url: "json.htm?type=command&param=learnsw",
					async: false,
					dataType: 'json',
					success: function (data) {
						if (typeof data.status != 'undefined') {
							if (data.status == 'OK') {
								bHaveFoundDevice = true;
								deviceID = data.ID;
								deviceidx = data.idx;
								bIsUsed = data.Used;
								Name = data.Name;
							}
						}
					}
				});
				HideNotify();

				setTimeout(function () {
					if ((bHaveFoundDevice == true) && (bIsUsed == false)) {
						$.devIdx = deviceidx;
						$("#dialog-addlightdevice").i18n();
						$("#dialog-addlightdevice").dialog("open");
					}
					else {
						if (bIsUsed == true)
							ShowNotify($.t('Already used by') + ':<br>"' + Name + '"', 3500, true);
						else
							ShowNotify($.t('Timeout...<br>Please try again!'), 2500, true);
					}
				}, 200);
			}, 600);
		}

		ConfigureAddManualSettings = function () {
			if ($scope.HasInitializedAddManualDialog == true) {
				return;
			}
			$scope.HasInitializedAddManualDialog = true;
			$('#dialog-addmanuallightdevice #lightparams2 #combocmd2 >option').remove();
			$('#dialog-addmanuallightdevice #lightparams2 #combocmd3 >option').remove();
			$('#dialog-addmanuallightdevice #lightparams2 #combocmd4 >option').remove();
			$('#dialog-addmanuallightdevice #lightparams3 #combocmd1 >option').remove();
			$('#dialog-addmanuallightdevice #lightparams3 #combocmd2 >option').remove();
			$('#dialog-addmanuallightdevice #blindsparams #combocmd1 >option').remove();
			$('#dialog-addmanuallightdevice #blindsparams #combocmd2 >option').remove();
			$('#dialog-addmanuallightdevice #blindsparams #combocmd3 >option').remove();
			$('#dialog-addmanuallightdevice #homeconfortparams #combocmd2 >option').remove();
			$('#dialog-addmanuallightdevice #homeconfortparams #combocmd3 >option').remove();
			$('#dialog-addmanuallightdevice #fanparams #combocmd1 >option').remove();
			$('#dialog-addmanuallightdevice #fanparams #combocmd2 >option').remove();
			$('#dialog-addmanuallightdevice #fanparams #combocmd3 >option').remove();
			for (ii = 0; ii < 256; ii++) {
				$('#dialog-addmanuallightdevice #lightparams2 #combocmd2').append($('<option></option>').val(ii).html($.strPad(ii.toString(16).toUpperCase(), 2)));
				$('#dialog-addmanuallightdevice #lightparams2 #combocmd3').append($('<option></option>').val(ii).html($.strPad(ii.toString(16).toUpperCase(), 2)));
				$('#dialog-addmanuallightdevice #lightparams2 #combocmd4').append($('<option></option>').val(ii).html($.strPad(ii.toString(16).toUpperCase(), 2)));
				$('#dialog-addmanuallightdevice #lightparams3 #combocmd1').append($('<option></option>').val(ii).html($.strPad(ii.toString(16).toUpperCase(), 2)));
				$('#dialog-addmanuallightdevice #lightparams3 #combocmd2').append($('<option></option>').val(ii).html($.strPad(ii.toString(16).toUpperCase(), 2)));
				$('#dialog-addmanuallightdevice #blindsparams #combocmd1').append($('<option></option>').val(ii).html($.strPad(ii.toString(16).toUpperCase(), 2)));
				$('#dialog-addmanuallightdevice #blindsparams #combocmd2').append($('<option></option>').val(ii).html($.strPad(ii.toString(16).toUpperCase(), 2)));
				$('#dialog-addmanuallightdevice #blindsparams #combocmd3').append($('<option></option>').val(ii).html($.strPad(ii.toString(16).toUpperCase(), 2)));
				$('#dialog-addmanuallightdevice #homeconfortparams #combocmd2').append($('<option></option>').val(ii).html($.strPad(ii.toString(16).toUpperCase(), 2)));
				$('#dialog-addmanuallightdevice #homeconfortparams #combocmd3').append($('<option></option>').val(ii).html($.strPad(ii.toString(16).toUpperCase(), 2)));
				$('#dialog-addmanuallightdevice #fanparams #combocmd1').append($('<option></option>').val(ii).html($.strPad(ii.toString(16).toUpperCase(), 2)));
				$('#dialog-addmanuallightdevice #fanparams #combocmd2').append($('<option></option>').val(ii).html($.strPad(ii.toString(16).toUpperCase(), 2)));
				$('#dialog-addmanuallightdevice #fanparams #combocmd3').append($('<option></option>').val(ii).html($.strPad(ii.toString(16).toUpperCase(), 2)));
			}
			$('#dialog-addmanuallightdevice #blindsparams #combocmd4 >option').remove();
			$('#dialog-addmanuallightdevice #blindsparams #combounitcode >option').remove();
			for (ii = 0; ii < 16; ii++) {
				$('#dialog-addmanuallightdevice #blindsparams #combocmd4').append($('<option></option>').val(ii).html(ii.toString(16).toUpperCase()));
				$('#dialog-addmanuallightdevice #blindsparams #combounitcode').append($('<option></option>').val(ii).html(ii));
			}
			$('#dialog-addmanuallightdevice #lightparams2 #combounitcode >option').remove();
			for (ii = 1; ii < 16 + 1; ii++) {
				$('#dialog-addmanuallightdevice #lightparams2 #combounitcode').append($('<option></option>').val(ii).html(ii));
			}
			$('#dialog-addmanuallightdevice #he105params #combounitcode >option').remove();
			for (ii = 0; ii < 32; ii++) {
				$('#dialog-addmanuallightdevice #he105params #combounitcode').append($('<option></option>').val(ii).html($.strPad(ii.toString(16).toUpperCase(), 2)));
			}
			//Home confort
			$('#dialog-addmanuallightdevice #homeconfortparams #combocmd1 >option').remove();
			for (ii = 0; ii < 8; ii++) {
				$('#dialog-addmanuallightdevice #homeconfortparams #combocmd1').append($('<option></option>').val(ii).html($.strPad(ii.toString(16).toUpperCase(), 2)));
			}
			$('#dialog-addmanuallightdevice #homeconfortparams #combohousecode >option').remove();
			$('#dialog-addmanuallightdevice #homeconfortparams #combounitcode >option').remove();
			for (ii = 0; ii < 4; ii++) {
				$('#dialog-addmanuallightdevice #homeconfortparams #combohousecode').append($('<option></option>').val(65 + ii).html(String.fromCharCode(65 + ii)));
				$('#dialog-addmanuallightdevice #homeconfortparams #combounitcode').append($('<option></option>').val((ii + 1)).html((ii + 1)));
			}

			RefreshHardwareComboArray();
			$("#dialog-addmanuallightdevice #lighttable #combohardware").html("");
			$.each($.ComboHardware, function (i, item) {
				var option = $('<option />');
				option.attr('value', item.idx).text(item.name);
				$("#dialog-addmanuallightdevice #lighttable #combohardware").append(option);
			});

			RefreshGpioComboArray();
			$("#combogpio").html("");
			$.each($.ComboGpio, function (i, item) {
				var option = $('<option />');
				option.attr('value', item.idx).text(item.name);
				$("#combogpio").append(option);
			});

			RefreshSysfsGpioComboArray();
			$("#combosysfsgpio").html("");
			$.each($.ComboSysfsGpio, function (i, item) {
				var option = $('<option />');
				option.attr('value', item.idx).text(item.name);
				$("#combosysfsgpio").append(option);
            });
		}

		RefreshHardwareComboArray = function () {
			$.ComboHardware = [];
			$.ajax({
				url: "json.htm?type=command&param=getmanualhardware",
				async: false,
				dataType: 'json',
				success: function (data) {
					if (typeof data.result != 'undefined') {
						$.each(data.result, function (i, item) {
							$.ComboHardware.push({
								idx: item.idx,
								name: item.Name
							});
						});
					}
				}
			});
		}

		RefreshGpioComboArray = function () {
			$.ComboGpio = [];
			$.ajax({
				url: "json.htm?type=command&param=getgpio",
				async: false,
				dataType: 'json',
				success: function (data) {
					if (typeof data.result != 'undefined') {
						$.each(data.result, function (i, item) {
							$.ComboGpio.push({
								idx: item.idx,
								name: item.Name
							});
						});
					}
				}
			});
		}

		RefreshSysfsGpioComboArray = function () {
			$.ComboSysfsGpio = [];
			$.ajax({
				url: "json.htm?type=command&param=getsysfsgpio",
				async: false,
				dataType: 'json',
				success: function (data) {
					if (typeof data.result != 'undefined') {
						$.each(data.result, function (i, item) {
							$.ComboSysfsGpio.push({
								idx: item.idx,
								name: item.Name
							});
						});
					}
				}
			});
		}

		//Evohome...

		SwitchModal = function (idx, name, status) {
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
				success: function (data) {
					if (data.status == "ERROR") {
						HideNotify();
						bootbox.alert($.t('Problem sending switch command'));
					}
					//wait 1 second
					setTimeout(function () {
						HideNotify();
					}, 1000);
				},
				error: function () {
					HideNotify();
					alert($.t('Problem sending switch command'));
				}
			});
		}

		//FIXME move this to a shared js ...see temperaturecontroller.js
		EvoDisplayTextMode = function (strstatus) {
			if (strstatus == "Auto")//FIXME better way to convert?
				strstatus = "Normal";
			else if (strstatus == "AutoWithEco")//FIXME better way to convert?
				strstatus = "Economy";
			else if (strstatus == "DayOff")//FIXME better way to convert?
				strstatus = "Day Off";
			else if (strstatus == "DayOffWithEco")//FIXME better way to convert?
				strstatus = "Day Off With Eco";
 			else if (strstatus == "HeatingOff")//FIXME better way to convert?
				strstatus = "Heating Off";
			return strstatus;
		}

		GetLightStatusText = function (item) {
			if (item.SubType == "Evohome") {
				return EvoDisplayTextMode(item.Status);
			}
			else if (item.SwitchType === "Selector") {
				return b64DecodeUnicode(item.LevelNames).split('|')[(item.LevelInt / 10)];
			}
			else
				return item.Status;
		}

		EvohomeAddJS = function () {
			return "<script type='text/javascript'> function deselect(e,id) { $(id).slideFadeToggle('swing', function() { e.removeClass('selected'); });} $.fn.slideFadeToggle = function(easing, callback) {  return this.animate({ opacity: 'toggle',height: 'toggle' }, 'fast', easing, callback);};</script>";
		}

		EvohomeImg = function (item) {
			return '<div title="Quick Actions" class="' + ((item.Status == "Auto") ? "evoimgnorm" : "evoimg") + '"><img src="images/evohome/' + item.Status + '.png" class="lcursor" onclick="if($(this).hasClass(\'selected\')){deselect($(this),\'#evopop_' + item.idx + '\');}else{$(this).addClass(\'selected\');$(\'#evopop_' + item.idx + '\').slideFadeToggle();}return false;"></div>';
		}

		EvohomePopupMenu = function (item) {
			var htm = '\t      <td id="img"><a href="#evohome" id="evohome_' + item.idx + '">' + EvohomeImg(item) + '</a></td>\n<div id="evopop_' + item.idx + '" class="ui-popup ui-body-b ui-overlay-shadow ui-corner-all pop">  <ul class="ui-listview ui-listview-inset ui-corner-all ui-shadow">         <li class="ui-li-divider ui-bar-inherit ui-first-child">Choose an action</li>';
			$.each([{ "name": "Normal", "data": "Auto" }, { "name": "Economy", "data": "AutoWithEco" }, { "name": "Away", "data": "Away" }, { "name": "Day Off", "data": "DayOff" },  { "name": "Day Off With Eco", "data": "DayOffWithEco" }, { "name": "Custom", "data": "Custom" }, { "name": "Heating Off", "data": "HeatingOff" }], function (idx, obj) { htm += '<li><a href="#" class="ui-btn ui-btn-icon-right ui-icon-' + obj.data + '" onclick="SwitchModal(\'' + item.idx + '\',\'' + obj.name + '\',\'' + obj.data + '\');deselect($(this),\'#evopop_' + item.idx + '\');return false;">' + obj.name + '</a></li>'; });
			htm += '</ul></div>';
			return htm;
		}

		RefreshItem = function (item) {
			var id = "#lightcontent #" + item.idx;
			if ($(id + " #name").html() === undefined) {
				return; //idx not found
			}

			if ($(id + " #name").html() != item.Name) {
				$(id + " #name").html(item.Name);
			}
			var isdimmer = false;
			var img = "";
			var img2 = "";
			var img3 = "";
			var status = "";

			var bigtext = TranslateStatusShort(item.Status);
			if (item.UsedByCamera == true) {
				var streamimg = '<img src="images/webcam.png" title="' + $.t('Stream Video') + '" height="16" width="16">';
				var streamurl = "<a href=\"javascript:ShowCameraLiveStream('" + escape(item.Name) + "','" + item.CameraIdx + "')\">" + streamimg + "</a>";
				bigtext += "&nbsp;" + streamurl;
			}

			if (item.SubType == "Security Panel") {
				img = '<a href="secpanel/"><img src="images/security48.png" class="lcursor" height="48" width="48"></a>';
			}
			else if (item.SubType == "Evohome") {
				img = EvohomeImg(item);
				bigtext = GetLightStatusText(item);
			}
			else if (item.SwitchType == "X10 Siren") {
				if (
					(item.Status == 'On') ||
					(item.Status == 'Chime') ||
					(item.Status == 'Group On') ||
					(item.Status == 'All On')
				) {
					img = '<img src="images/siren-on.png" title="' + $.t("Turn Off") + '" onclick="SwitchLight(' + item.idx + ',\'Off\',' + item.Protected + ');" class="lcursor" height="48" width="48">';
				}
				else {
					img = '<img src="images/siren-off.png" title="' + $.t("Turn On") + '" onclick="SwitchLight(' + item.idx + ',\'On\',' + item.Protected + ');" class="lcursor" height="48" width="48">';
				}
			}
			else if (item.SwitchType == "Doorbell") {
				img = '<img src="images/doorbell48.png" title="' + $.t("Turn On") + '" onclick="SwitchLight(' + item.idx + ',\'On\',' + item.Protected + ');" class="lcursor" height="48" width="48">';
			}
			else if (item.SwitchType == "Push On Button") {
				img = '<img src="images/' + item.Image + '48_On.png" title="' + $.t("Turn On") + '" onclick="SwitchLight(' + item.idx + ',\'On\',' + item.Protected + ');" class="lcursor" height="48" width="48">';
			}
			else if (item.SwitchType == "Push Off Button") {
				img = '<img src="images/' + item.Image + '48_Off.png" title="' + $.t("Turn Off") + '" onclick="SwitchLight(' + item.idx + ',\'Off\',' + item.Protected + ');" class="lcursor" height="48" width="48">';
			}
			else if (item.SwitchType == "Door Contact") {
				if (item.InternalState == "Open") {
					img = '<img src="images/' + item.Image + '48_On.png" title="' + $.t(item.InternalState) + '" height="48" width="48">';
				}
				else {
					img = '<img src="images/' + item.Image + '48_Off.png" title="' + $.t(item.InternalState) + '" height="48" width="48">';
				}
			}
			else if (item.SwitchType == "Door Lock") {
				if (item.InternalState == "Unlocked") {
					img = '<img src="images/' + item.Image + '48_On.png" title="' + $.t("Lock") + '" onclick="SwitchLight(' + item.idx + ',\'On\',' + item.Protected + ');" class="lcursor" height="48" width="48">';
				}
				else {
					img = '<img src="images/' + item.Image + '48_Off.png" title="' + $.t("Unlock") + '" onclick="SwitchLight(' + item.idx + ',\'Off\',' + item.Protected + ');" class="lcursor" height="48" width="48">';
				}
			}
			else if (item.SwitchType == "Door Lock Inverted") {
				if (item.InternalState == "Unlocked") {
					img = '<img src="images/' + item.Image + '48_On.png" title="' + $.t("Lock") + '" onclick="SwitchLight(' + item.idx + ',\'Off\',' + item.Protected + ');" class="lcursor" height="48" width="48">';
				}
				else {
					img = '<img src="images/' + item.Image + '48_Off.png" title="' + $.t("Unlock") + '" onclick="SwitchLight(' + item.idx + ',\'On\',' + item.Protected + ');" class="lcursor" height="48" width="48">';
				}
			}
			else if (item.SwitchType == "Contact") {
				if (item.Status == 'Closed') {
					img = '<img src="images/' + item.Image + '48_Off.png" height="48" width="48">';
				}
				else {
					img = '<img src="images/' + item.Image + '48_On.png" height="48" width="48">';
				}
			}
			else if ((item.SwitchType == "Blinds") || (item.SwitchType.indexOf("Venetian Blinds") == 0)) {
				if (
					(item.SubType == "RAEX") ||
					(item.SubType.indexOf('A-OK') == 0) ||
					(item.SubType.indexOf('Hasta') >= 0) ||
					(item.SubType.indexOf('Media Mount') == 0) ||
					(item.SubType.indexOf('Forest') == 0) ||
					(item.SubType.indexOf('Chamberlain') == 0) ||
					(item.SubType.indexOf('Sunpery') == 0) ||
					(item.SubType.indexOf('Dolat') == 0) ||
					(item.SubType.indexOf('ASP') == 0) ||
					(item.SubType == "Harrison") ||
					(item.SubType.indexOf('RFY') == 0) ||
					(item.SubType.indexOf('ASA') == 0) ||
					(item.SubType.indexOf('DC106') == 0) ||
					(item.SubType.indexOf('Confexx') == 0) ||
					(item.SwitchType.indexOf("Venetian Blinds") == 0)
				) {
					if (item.Status == 'Closed') {
						img = '<img src="images/blindsopen48.png" title="' + $.t("Open Blinds") + '" onclick="SwitchLight(' + item.idx + ',\'Off\',' + item.Protected + ');" class="lcursor" height="48" width="48">';
						img3 = '<img src="images/blinds48sel.png" title="' + $.t("Close Blinds") + '" onclick="SwitchLight(' + item.idx + ',\'On\',' + item.Protected + ');" class="lcursor" height="48" width="48">';
					}
					else {
						img = '<img src="images/blindsopen48sel.png" title="' + $.t("Open Blinds") + '" onclick="SwitchLight(' + item.idx + ',\'Off\',' + item.Protected + ');" class="lcursor" height="48" width="48">';
						img3 = '<img src="images/blinds48.png" title="' + $.t("Close Blinds") + '" onclick="SwitchLight(' + item.idx + ',\'On\',' + item.Protected + ');" class="lcursor" height="48" width="48">';
					}
				}
				else {
					if (item.Status == 'Closed') {
						img = '<img src="images/blindsopen48.png" title="' + $.t("Open Blinds") + '" onclick="SwitchLight(' + item.idx + ',\'Off\',' + item.Protected + ');" class="lcursor" height="48" width="48">';
						img2 = '<img src="images/blinds48sel.png" title="' + $.t("Close Blinds") + '" onclick="SwitchLight(' + item.idx + ',\'On\',' + item.Protected + ');" class="lcursor" height="48" width="48">';
					}
					else {
						img = '<img src="images/blindsopen48sel.png" title="' + $.t("Open Blinds") + '" onclick="SwitchLight(' + item.idx + ',\'Off\',' + item.Protected + ');" class="lcursor" height="48" width="48">';
						img2 = '<img src="images/blinds48.png" title="' + $.t("Close Blinds") + '" onclick="SwitchLight(' + item.idx + ',\'On\',' + item.Protected + ');" class="lcursor" height="48" width="48">';
					}
				}
			}
			else if (item.SwitchType == "Blinds Inverted") {
				if (
					(item.SubType == "RAEX") ||
					(item.SubType.indexOf('A-OK') == 0) ||
					(item.SubType.indexOf('Hasta') >= 0) ||
					(item.SubType.indexOf('Media Mount') == 0) ||
					(item.SubType.indexOf('Forest') == 0) ||
					(item.SubType.indexOf('Chamberlain') == 0) ||
					(item.SubType.indexOf('Sunpery') == 0) ||
					(item.SubType.indexOf('Dolat') == 0) ||
					(item.SubType.indexOf('ASP') == 0) ||
					(item.SubType == "Harrison") ||
					(item.SubType.indexOf('RFY') == 0) ||
					(item.SubType.indexOf('ASA') == 0) ||
					(item.SubType.indexOf('DC106') == 0) ||
					(item.SubType.indexOf('Confexx') == 0)
				) {
					if (item.Status == 'Closed') {
						img = '<img src="images/blindsopen48.png" title="' + $.t("Open Blinds") + '" onclick="SwitchLight(' + item.idx + ',\'On\',' + item.Protected + ');" class="lcursor" height="48" width="48">';
						img3 = '<img src="images/blinds48sel.png" title="' + $.t("Close Blinds") + '" onclick="SwitchLight(' + item.idx + ',\'Off\',' + item.Protected + ');" class="lcursor" height="48" width="48">';
					}
					else {
						img = '<img src="images/blindsopen48sel.png" title="' + $.t("Open Blinds") + '" onclick="SwitchLight(' + item.idx + ',\'On\',' + item.Protected + ');" class="lcursor" height="48" width="48">';
						img3 = '<img src="images/blinds48.png" title="' + $.t("Close Blinds") + '" onclick="SwitchLight(' + item.idx + ',\'Off\',' + item.Protected + ');" class="lcursor" height="48" width="48">';
					}
				}
				else {
					if (item.Status == 'Closed') {
						img = '<img src="images/blindsopen48.png" title="' + $.t("Open Blinds") + '" onclick="SwitchLight(' + item.idx + ',\'On\',' + item.Protected + ');" class="lcursor" height="48" width="48">';
						img2 = '<img src="images/blinds48sel.png" title="' + $.t("Close Blinds") + '" onclick="SwitchLight(' + item.idx + ',\'Off\',' + item.Protected + ');" class="lcursor" height="48" width="48">';
					}
					else {
						img = '<img src="images/blindsopen48sel.png" title="' + $.t("Open Blinds") + '" onclick="SwitchLight(' + item.idx + ',\'On\',' + item.Protected + ');" class="lcursor" height="48" width="48">';
						img2 = '<img src="images/blinds48.png" title="' + $.t("Close Blinds") + '" onclick="SwitchLight(' + item.idx + ',\'Off\',' + item.Protected + ');" class="lcursor" height="48" width="48">';
					}
				}
			}
			else if (item.SwitchType == "Blinds Percentage") {
				isdimmer = true;
				if (item.Status == 'Open') {
					img = '<img src="images/blindsopen48sel.png" title="' + $.t("Open Blinds") + '" onclick="SwitchLight(' + item.idx + ',\'Off\',' + item.Protected + ');" class="lcursor" height="48">';
					img2 = '<img src="images/blinds48.png" title="' + $.t("Close Blinds") + '" onclick="SwitchLight(' + item.idx + ',\'On\',' + item.Protected + ');" class="lcursor" height="48">';
				}
				else {
					img = '<img src="images/blindsopen48.png" title="' + $.t("Open Blinds") + '" onclick="SwitchLight(' + item.idx + ',\'Off\',' + item.Protected + ');" class="lcursor" height="48">';
					img2 = '<img src="images/blinds48sel.png" title="' + $.t("Close Blinds") + '" onclick="SwitchLight(' + item.idx + ',\'On\',' + item.Protected + ');" class="lcursor" height="48">';
				}
			}
			else if (item.SwitchType == "Blinds Percentage Inverted") {
				isdimmer = true;
				if (item.Status == 'Closed') {
					img = '<img src="images/blindsopen48.png" title="' + $.t("Open Blinds") + '" onclick="SwitchLight(' + item.idx + ',\'On\',' + item.Protected + ');" class="lcursor" height="48">';
					img2 = '<img src="images/blinds48sel.png" title="' + $.t("Close Blinds") + '" onclick="SwitchLight(' + item.idx + ',\'Off\',' + item.Protected + ');" class="lcursor" height="48">';
				}
				else if ((item.Status == 'Open') || (item.Status.indexOf('Set ') == 0)) {
					img = '<img src="images/blindsopen48sel.png" title="' + $.t("Open Blinds") + '" onclick="SwitchLight(' + item.idx + ',\'On\',' + item.Protected + ');" class="lcursor" height="48">';
					img2 = '<img src="images/blinds48.png" title="' + $.t("Close Blinds") + '" onclick="SwitchLight(' + item.idx + ',\'Off\',' + item.Protected + ');" class="lcursor" height="48">';
				}
			}
			else if (item.SwitchType == "Smoke Detector") {
				if (
					(item.Status == 'Panic') ||
					(item.Status == 'On')
				) {
					img = '<img src="images/smoke48on.png" title="' + $.t("Turn Alarm On") + '" onclick="SwitchLight(' + item.idx + ',\'On\',' + item.Protected + ');" class="lcursor" height="48" width="48">';
					$(id + ' #resetbtn').attr("class", "btnsmall-sel");
				}
				else {
					img = '<img src="images/smoke48off.png" title="' + $.t("Turn Alarm On") + '" onclick="SwitchLight(' + item.idx + ',\'On\',' + item.Protected + ');" class="lcursor" height="48" width="48">';
					$(id + ' #resetbtn').attr("class", "btnsmall-dis");
				}

			}
			else if (item.Type == "Security") {
				if (item.SubType.indexOf('remote') > 0) {
					if ((item.Status.indexOf('Arm') >= 0) || (item.Status.indexOf('Panic') >= 0)) {
						img += '<img src="images/remote48.png" title="' + $.t("Turn Alarm Off") + '" onclick="SwitchLight(' + item.idx + ',\'Off\',' + item.Protected + ');" class="lcursor" height="48" width="48">\n';
					}
					else {
						img += '<img src="images/remote48.png" title="' + $.t("Turn Alarm On") + '" onclick="ArmSystem(' + item.idx + ',\'On\',' + item.Protected + ');" class="lcursor" height="48" width="48">\n';
					}
				}
				else if (item.SubType == "X10 security") {
					if (item.Status.indexOf('Normal') >= 0) {
						img += '<img src="images/security48.png" title="' + $.t("Turn Alarm On") + '" onclick="SwitchLight(' + item.idx + ',\'' + ((item.Status == "Normal Delayed") ? "Alarm Delayed" : "Alarm") + '\',' + item.Protected + ');" class="lcursor" height="48" width="48">';
					}
					else {
						img += '<img src="images/Alarm48_On.png" title="' + $.t("Turn Alarm Off") + '" onclick="SwitchLight(' + item.idx + ',\'' + ((item.Status == "Alarm Delayed") ? "Normal Delayed" : "Normal") + '\',' + item.Protected + ');" class="lcursor" height="48" width="48">';
					}
				}
				else if (item.SubType == "X10 security motion") {
					if ((item.Status == "No Motion")) {
						img += '<img src="images/security48.png" title="' + $.t("Turn Alarm On") + '" onclick="SwitchLight(' + item.idx + ',\'Motion\',' + item.Protected + ');" class="lcursor" height="48" width="48">';
					}
					else {
						img += '<img src="images/Alarm48_On.png" title="' + $.t("Turn Alarm Off") + '" onclick="SwitchLight(' + item.idx + ',\'No Motion\',' + item.Protected + ');" class="lcursor" height="48" width="48">';
					}
				}
				else if ((item.Status.indexOf('Alarm') >= 0) || (item.Status.indexOf('Tamper') >= 0)) {
					img = '<img src="images/Alarm48_On.png" height="48" width="48">';
				}
				else {
					if (item.SubType.indexOf('Meiantech') >= 0) {
						if ((item.Status.indexOf('Arm') >= 0) || (item.Status.indexOf('Panic') >= 0)) {
							img = '<img src="images/security48.png" title="' + $.t("Turn Alarm Off") + '" onclick="SwitchLight(' + item.idx + ',\'Off\',' + item.Protected + ');" class="lcursor" height="48" width="48">';
						}
						else {
							img = '<img src="images/security48.png" title="' + $.t("Turn Alarm On") + '" onclick="ArmSystemMeiantech(' + item.idx + ',\'On\',' + item.Protected + ');" class="lcursor" height="48" width="48">';
						}
					}
					else {
						if (item.SubType.indexOf('KeeLoq') >= 0) {
							img = '<img src="images/pushon48.png" title="' + $.t("Turn On") + '" onclick="SwitchLight(' + item.idx + ',\'On\',' + item.Protected + ');" class="lcursor" height="48" width="48">';
						}
						else {
							img = '<img src="images/security48.png" height="48" width="48">';
						}
					}
				}
			}
			else if (item.SwitchType == "Dimmer") {
				isdimmer = true;
				if (item.CustomImage == 0) item.Image = item.TypeImg;
				item.Image = item.Image.charAt(0).toUpperCase() + item.Image.slice(1);
				if (
					(item.Status == 'On') ||
					(item.Status == 'Chime') ||
					(item.Status == 'Group On') ||
					(item.Status.indexOf('Set ') == 0)
				) {
					if (isLED(item.SubType)) {
						if (item.Image == "Dimmer") {
							img = '<img src="images/RGB48_On.png" onclick="ShowRGBWPopup(event, ' + item.idx + ',' + item.Protected + ',' + item.MaxDimLevel + ',' + item.LevelInt + ',\'' + item.Color.replace(/\"/g , '\&quot;') + '\',\'' + item.SubType + '\',\'' + item.DimmerType + '\');" class="lcursor" height="48" width="48">';
						}
						else {
							img = '<img src="images/' + item.Image + '48_On.png" onclick="ShowRGBWPopup(event, ' + item.idx + ',' + item.Protected + ',' + item.MaxDimLevel + ',' + item.LevelInt + ',\'' + item.Color.replace(/\"/g , '\&quot;') + '\',\'' + item.SubType + '\',\'' + item.DimmerType + '\');" class="lcursor" height="48" width="48">';
						}
					}
					else {
						img = '<img src="images/' + item.Image + '48_On.png" title="' + $.t("Turn Off") + '" onclick="SwitchLight(' + item.idx + ',\'Off\',' + item.Protected + ');" class="lcursor" height="48" width="48">';
					}
				}
				else {
					if (isLED(item.SubType)) {
						if (item.Image == "Dimmer") {
							img = '<img src="images/RGB48_Off.png" onclick="ShowRGBWPopup(event, ' + item.idx + ',' + item.Protected + ',' + item.MaxDimLevel + ',' + item.LevelInt + ',\'' + item.Color.replace(/\"/g , '\&quot;') + '\',\'' + item.SubType + '\',\'' + item.DimmerType + '\');" class="lcursor" height="48" width="48">';
						}
						else {
							img = '<img src="images/' + item.Image + '48_Off.png" onclick="ShowRGBWPopup(event, ' + item.idx + ',' + item.Protected + ',' + item.MaxDimLevel + ',' + item.LevelInt + ',\'' + item.Color.replace(/\"/g , '\&quot;') + '\',\'' + item.SubType + '\',\'' + item.DimmerType + '\');" class="lcursor" height="48" width="48">';
						}
					}
					else {
						img = '<img src="images/' + item.Image + '48_Off.png" title="' + $.t("Turn On") + '" onclick="SwitchLight(' + item.idx + ',\'On\',' + item.Protected + ');" class="lcursor" height="48" width="48">';
					}
				}
			}
			else if (item.SwitchType == "TPI") {
				var RO = (item.Unit < 64 || item.Unit > 95) ? true : false;
				isdimmer = true;
				if (
					(item.Status != 'Off')
				) {
					img = '<img src="images/Fireplace48_On.png" title="' + $.t(RO ? "On" : "Turn Off") + (RO ? '"' : '" onclick="SwitchLight(' + item.idx + ',\'Off\',' + item.Protected + ');" class="lcursor"') + ' height="48" width="48">';
				}
				else {
					img = '<img src="images/Fireplace48_Off.png" title="' + $.t(RO ? "Off" : "Turn On") + (RO ? '"' : '" onclick="SwitchLight(' + item.idx + ',\'On\',' + item.Protected + ');" class="lcursor"') + ' height="48" width="48">';
				}
			}
			else if (item.SwitchType == "Dusk Sensor") {
				if (
					(item.Status == 'On')
				) {
					img = '<img src="images/uvdark.png" title="' + $.t("Nighttime") + '" height="48" width="48">';
				}
				else {
					img = '<img src="images/uvsunny.png" title="' + $.t("Daytime") + '" height="48" width="48">';
				}
			}
			else if (item.SwitchType == "Media Player") {
				if (item.CustomImage == 0) item.Image = item.TypeImg;
				if (item.Status == 'Disconnected') {
					img = '<img src="images/' + item.Image + '48_Off.png" height="48" width="48">';
					img2 = '<img src="images/remote48.png" style="opacity:0.4"; height="48" width="48">';
				}
				else if ((item.Status != 'Off') && (item.Status != '0')) {
					img = '<img src="images/' + item.Image + '48_On.png" onclick="SwitchLight(' + item.idx + ',\'Off\',' + item.Protected + ');" class="lcursor" height="48" width="48">';
					img2 = '<img src="images/remote48.png" onclick="ShowMediaRemote(\'' + escape(item.Name) + "'," + item.idx + ",'" + item.HardwareType + '\');" class="lcursor" height="48" width="48">';
				}
				else {
					img = '<img src="images/' + item.Image + '48_Off.png" onclick="SwitchLight(' + item.idx + ',\'On\',' + item.Protected + ');" class="lcursor" height="48" width="48">';
					img2 = '<img src="images/remote48.png" style="opacity:0.4"; height="48" width="48">';
				}
				if (item.Status.length == 1) item.Status = "";
				status = item.Data;
				if (status == '0') status = "";
			}
			else if (item.SwitchType == "Motion Sensor") {
				if (
					(item.Status == 'On') ||
					(item.Status == 'Chime') ||
					(item.Status == 'Group On') ||
					(item.Status.indexOf('Set ') == 0)
				) {
					img = '<img src="images/motion48-on.png" height="48" width="48">';
				}
				else {
					img = '<img src="images/motion48-off.png" height="48" width="48">';
				}
			}
			else if (item.SwitchType === "Selector") {
				if ((item.Status === "Off")) {
					img += '<img src="images/' + item.Image + '48_Off.png" height="48" width="48">';
				} else if (item.LevelOffHidden) {
					img += '<img src="images/' + item.Image + '48_On.png" height="48" width="48">';
				} else {
					img += '<img src="images/' + item.Image + '48_On.png" title="' + $.t("Turn Off") + '" onclick="SwitchLight(' + item.idx + ',\'Off\',' + item.Protected + ');" class="lcursor" height="48" width="48">';
				}
			}
			else if (
				(item.SubType.indexOf("Itho") == 0) ||
				(item.SubType.indexOf("Lucci") == 0) ||
				(item.SubType.indexOf("Westinghouse") == 0)
				) {
				img = $(id + " #img").html();
			}
			else {
				if (
					(item.Status == 'On') ||
					(item.Status == 'Chime') ||
					(item.Status == 'Group On') ||
					(item.Status.indexOf('Down') != -1) ||
					(item.Status.indexOf('Up') != -1) ||
					(item.Status.indexOf('Set ') == 0)
				) {
					if (item.Type == "Thermostat 3") {
						img = '<img src="images/' + item.Image + '48_On.png" onclick="ShowTherm3Popup(event, ' + item.idx + ',' + item.Protected + ');" class="lcursor" height="48" width="48">';
					}
					else {
						img = '<img src="images/' + item.Image + '48_On.png" title="' + $.t("Turn Off") + '" onclick="SwitchLight(' + item.idx + ',\'Off\',' + item.Protected + ');" class="lcursor" height="48" width="48">';
					}
				}
				else {
					if (item.Type == "Thermostat 3") {
						img = '<img src="images/' + item.Image + '48_Off.png" onclick="ShowTherm3Popup(event, ' + item.idx + ',' + item.Protected + ');" class="lcursor" height="48" width="48">';
					}
					else {
						img = '<img src="images/' + item.Image + '48_Off.png" title="' + $.t("Turn On") + '" onclick="SwitchLight(' + item.idx + ',\'On\',' + item.Protected + ');" class="lcursor" height="48" width="48">';
					}
				}
			}

			var backgroundClass = $rootScope.GetItemBackgroundStatus(item);
			$(id).removeClass('statusNormal').removeClass('statusProtected').removeClass('statusTimeout').removeClass('statusLowBattery').removeClass('statusDisabled');
			$(id).addClass(backgroundClass);

			if ($(id + " #img").html() != img) {
				$(id + " #img").html(img);
			}
			if (img2 != "") {
				if ($(id + " #img2").html() != img2) {
					$(id + " #img2").html(img2);
				}
			}
			if (img3 != "") {
				if ($(id + " #img3").html() != img3) {
					$(id + " #img3").html(img3);
				}
			}
			if (isdimmer == true) {
				var dslider = $(id + " #slider");
				if (typeof dslider != 'undefined') {
					dslider.slider("value", item.LevelInt);
				}
			}
			if (item.SwitchType === "Selector") {
				var selector$ = $("#selector" + item.idx);
				if (typeof selector$ !== 'undefined') {
					if (item.SelectorStyle === 0) {
						var xhtm = '';
						var levelNames = b64DecodeUnicode(item.LevelNames).split('|');
						$.each(levelNames, function (index, levelName) {
							if ((index === 0) && (item.LevelOffHidden)) {
								return;
							}
							xhtm += '<button type="button" class="btn btn-small ';
							if ((index * 10) == item.LevelInt) {
								xhtm += 'btn-selected"';
							}
							else {
								xhtm += 'btn-default"';
							}
							xhtm += 'id="lSelector' + item.idx + 'Level' + index + '" name="selector' + item.idx + 'Level" value="' + (index * 10) + '" onclick="SwitchSelectorLevel(' + item.idx + ',\'' + unescape(levelName) + '\',' + (index * 10) + ',' + item.Protected + ');">' + levelName + '</button>';
						});
						selector$.html(xhtm);
					} else if (item.SelectorStyle === 1) {
						selector$.val(item.LevelInt);
						selector$.selectmenu('refresh');
					}
				}
				bigtext = GetLightStatusText(item);
			}
			if ($(id + " #bigtext").html() != TranslateStatus(GetLightStatusText(item))) {
				$(id + " #bigtext").html(bigtext);
			}
			if ((typeof $(id + " #status") != 'undefined') && ($(id + " #status").html() != status)) {
				$(id + " #status").html(status);
			}
			if ($(id + " #lastupdate").html() != item.LastUpdate) {
				$(id + " #lastupdate").html(item.LastUpdate);
			}
			if ($scope.config.ShowUpdatedEffect == true) {
				$(id + " #name").effect("highlight", { color: '#EEFFEE' }, 1000);
			}
		}

		//We only call this once. After this the widgets are being updated automatically by used of the 'jsonupdate' broadcast event.
		RefreshLights = function () {
			livesocket.getJson("json.htm?type=devices&filter=light&used=true&order=[Order]&lastupdate=" + $.LastUpdateTime + "&plan=" + window.myglobals.LastPlanSelected, function (data) {
				if (typeof data.ServerTime != 'undefined') {
					$rootScope.SetTimeAndSun(data.Sunrise, data.Sunset, data.ServerTime);
				}

				if (typeof data.result != 'undefined') {

					if (typeof data.ActTime != 'undefined') {
						$.LastUpdateTime = parseInt(data.ActTime);
					}

					/*
						Render all the widgets at once.
					*/
					$.each(data.result, function (i, item) {
						RefreshItem(item);
					});
				}
			});
		}

		ShowLights = function () {
			$('#modal').show();

			var htmlcontent = '';
			var bShowRoomplan = false;
			$.RoomPlans = [];
			$.ajax({
				url: "json.htm?type=plans",
				async: false,
				dataType: 'json',
				success: function (data) {
					if (typeof data.result != 'undefined') {
						var totalItems = data.result.length;
						if (totalItems > 0) {
							bShowRoomplan = true;
							//				if (window.myglobals.ismobile==true) {
							//				bShowRoomplan=false;
							//		}
							if (bShowRoomplan == true) {
								$.each(data.result, function (i, item) {
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

			var tophtm = "";
			if ($.RoomPlans.length == 0) {
				tophtm +=
					'\t<table border="0" cellpadding="0" cellspacing="0" width="100%">\n' +
					'\t<tr>\n' +
					'\t  <td align="left" valign="top" id="timesun"></td>\n' +
					'\t</tr>\n' +
					'\t</table>\n';
			}
			else {
				tophtm +=
					'\t<table border="0" cellpadding="0" cellspacing="0" width="100%">\n' +
					'\t<tr>\n' +
					'\t  <td align="left" valign="top" id="timesun"></td>\n' +
					'<td align="right" valign="top">' +
					'<span data-i18n="Room">Room</span>:&nbsp;<select id="comboroom" style="width:160px" class="combobox ui-corner-all">' +
					'<option value="0" data-i18n="All">All</option>' +
					'</select>' +
					'</td>' +
					'\t</tr>\n' +
					'\t</table>\n';
			}
			if (permissions.hasPermission("Admin")) {
				tophtm +=
					'\t<table class="bannav" id="bannav" border="0" cellpadding="0" cellspacing="0" width="100%">\n' +
					'\t<tr>\n' +
					'\t  <td align="left"><a class="btnstylerev" onclick="AddManualLightDevice();" data-i18n="Manual Light/Switch">Manual Light/Switch</a></td>\n' +
					'\t  <td align="right"><a class="btnstyle" onclick="AddLightDevice();" data-i18n="Learn Light/Switch">Learn Light/Switch</a></td>\n' +
					'\t</tr>\n' +
					'\t</table>\n';
			}

			var i = 0;
			var j = 0;
			var roomPlanId = $routeParams.room || window.myglobals.LastPlanSelected;

			$.ajax({
				url: "json.htm?type=devices&filter=light&used=true&order=[Order]&plan=" + roomPlanId,
				async: false,
				dataType: 'json',
				success: function (data) {


					htmlcontent += EvohomeAddJS();

					if (typeof data.result != 'undefined') {
						if (typeof data.ActTime != 'undefined') {
							$.LastUpdateTime = parseInt(data.ActTime);
						}

						$.each(data.result, function (i, item) {
							if (j % 3 == 0) {
								//add devider
								if (bHaveAddedDevider == true) {
									//close previous devider
									htmlcontent += '</div>\n';
								}
								htmlcontent += '<div class="row divider">\n';
								bHaveAddedDevider = true;
							}
							var bAddTimer = true;
							var bIsDimmer = false;

							var backgroundClass = $rootScope.GetItemBackgroundStatus(item);

							var status = "";
							var xhtm =
								'\t<div class="item span4 ' + backgroundClass + '" id="' + item.idx + '">\n' +
								'\t  <section>\n';
							if ((item.SwitchType == "Blinds") || (item.SwitchType == "Blinds Inverted") || (item.SwitchType == "Blinds Percentage") || (item.SwitchType == "Blinds Percentage Inverted") || (item.SwitchType.indexOf("Venetian Blinds") == 0) || (item.SwitchType.indexOf("Media Player") == 0)) {
								if (
									(item.SubType == "RAEX") ||
									(item.SubType.indexOf('A-OK') == 0) ||
									(item.SubType.indexOf('Hasta') >= 0) ||
									(item.SubType.indexOf('Media Mount') == 0) ||
									(item.SubType.indexOf('Forest') == 0) ||
									(item.SubType.indexOf('Chamberlain') == 0) ||
									(item.SubType.indexOf('Sunpery') == 0) ||
									(item.SubType.indexOf('Dolat') == 0) ||
									(item.SubType.indexOf('ASP') == 0) ||
									(item.SubType == "Harrison") ||
									(item.SubType.indexOf('RFY') == 0) ||
									(item.SubType.indexOf('ASA') == 0) ||
									(item.SubType.indexOf('DC106') == 0) ||
									(item.SubType.indexOf('Confexx') == 0) ||
									(item.SwitchType.indexOf("Venetian Blinds") == 0)
								) {
									xhtm += '\t    <table id="itemtabletrippleicon" border="0" cellpadding="0" cellspacing="0">\n';
								}
								else {
									xhtm += '\t    <table id="itemtabledoubleicon" border="0" cellpadding="0" cellspacing="0">\n';
								}
							}
							else {
								xhtm += '\t    <table id="itemtablenostatus" border="0" cellpadding="0" cellspacing="0">\n';
							}

							xhtm +=
								'\t    <tr>\n' +
								'\t      <td id="name">' + item.Name + '</td>\n' +
								'\t      <td id="bigtext">';
							var bigtext = TranslateStatusShort(item.Status);
							if (item.SwitchType === "Selector" || item.SubType == "Evohome") {
								bigtext = GetLightStatusText(item);
							}
							if (item.UsedByCamera == true) {
								var streamimg = '<img src="images/webcam.png" title="' + $.t('Stream Video') + '" height="16" width="16">';
								var streamurl = "<a href=\"javascript:ShowCameraLiveStream('" + escape(item.Name) + "','" + item.CameraIdx + "')\">" + streamimg + "</a>";
								bigtext += "&nbsp;" + streamurl;
							}
							if (permissions.hasPermission("Admin")) {
								if (item.Type == "RFY") {
									var rfysetup = '<img src="images/devices.png" title="' + $.t('Setup') + '" height="16" width="16" onclick="ShowRFYPopup(event, ' + item.idx + ', ' + item.Protected + ', ' + window.myglobals.ismobile +');">';
									bigtext += "&nbsp;" + rfysetup;
								}
							}
							xhtm += bigtext + '</td>\n';
							if (item.SubType == "Security Panel") {
								xhtm += '\t      <td id="img"><a href="secpanel/"><img src="images/security48.png" class="lcursor" height="48" width="48"></a></td>\n';
							}
							else if (item.SubType == "Evohome") {
								xhtm += EvohomePopupMenu(item);
							}
							else if (item.SwitchType == "X10 Siren") {
								if (
									(item.Status == 'On') ||
									(item.Status == 'Chime') ||
									(item.Status == 'Group On') ||
									(item.Status == 'All On')
								) {
									xhtm += '\t      <td id="img"><img src="images/siren-on.png" title="' + $.t("Turn Off") + '" onclick="SwitchLight(' + item.idx + ',\'Off\',' + item.Protected + ');" class="lcursor" height="48" width="48"></td>\n';
								}
								else {
									xhtm += '\t      <td id="img"><img src="images/siren-off.png" title="' + $.t("Turn On") + '" onclick="SwitchLight(' + item.idx + ',\'On\',' + item.Protected + ');" class="lcursor" height="48" width="48"></td>\n';
								}
							}
							else if (item.SwitchType == "Doorbell") {
								xhtm += '\t      <td id="img"><img src="images/doorbell48.png" title="' + $.t("Turn On") + '" onclick="SwitchLight(' + item.idx + ',\'On\',' + item.Protected + ');" class="lcursor" height="48" width="48"></td>\n';
								bAddTimer = false;
							}
							else if (item.SwitchType == "Push On Button") {
								xhtm += '\t      <td id="img"><img src="images/' + item.Image + '48_On.png" title="' + $.t("Turn On") + '" onclick="SwitchLight(' + item.idx + ',\'On\',' + item.Protected + ');" class="lcursor" height="48" width="48"></td>\n';
							}
							else if (item.SwitchType == "Push Off Button") {
								xhtm += '\t      <td id="img"><img src="images/' + item.Image + '48_Off.png" title="' + $.t("Turn Off") + '" onclick="SwitchLight(' + item.idx + ',\'Off\',' + item.Protected + ');" class="lcursor" height="48" width="48"></td>\n';
							}
							else if (item.SwitchType == "Door Contact") {
								if (item.InternalState == "Open") {
                                    xhtm += '\t      <td id="img"><img src="images/' + item.Image + '48_On.png" title="' + $.t(item.InternalState) + '" height="48" width="48"></td>\n';
								}
								else {
                                    xhtm += '\t      <td id="img"><img src="images/' + item.Image + '48_Off.png" title="' + $.t(item.InternalState) + '" height="48" width="48"></td>\n';
								}
								bAddTimer = false;
							}
							else if (item.SwitchType == "Door Lock") {
								if (item.InternalState == "Unlocked") {
									xhtm += '\t      <td id="img"><img src="images/' + item.Image + '48_On.png" title="' + $.t("Lock") + '" onclick="SwitchLight(' + item.idx + ',\'On\',' + item.Protected + ');" class="lcursor" height="48" width="48"></td>\n';
								}
								else {
									xhtm += '\t      <td id="img"><img src="images/' + item.Image + '48_Off.png" title="' + $.t("Unlock") + '" onclick="SwitchLight(' + item.idx + ',\'Off\',' + item.Protected + ');" class="lcursor" height="48" width="48"></td>\n';
								}
								bAddTimer = false;
							}
							else if (item.SwitchType == "Door Lock Inverted") {
								if (item.InternalState == "Unlocked") {
									xhtm += '\t      <td id="img"><img src="images/' + item.Image + '48_On.png" title="' + $.t("Lock") + '" onclick="SwitchLight(' + item.idx + ',\'Off\',' + item.Protected + ');" class="lcursor" height="48" width="48"></td>\n';
								}
								else {
									xhtm += '\t      <td id="img"><img src="images/' + item.Image + '48_Off.png" title="' + $.t("Unlock") + '" onclick="SwitchLight(' + item.idx + ',\'On\',' + item.Protected + ');" class="lcursor" height="48" width="48"></td>\n';
								}
								bAddTimer = false;
							}
							else if (item.SwitchType == "Contact") {
								if (item.Status == 'Closed') {
									xhtm += '\t      <td id="img"><img src="images/' + item.Image + '48_Off.png" height="48" width="48"></td>\n';
								}
								else {
									xhtm += '\t      <td id="img"><img src="images/' + item.Image + '48_On.png" height="48" width="48"></td>\n';
								}
								bAddTimer = false;
							}
							else if (item.SwitchType == "Media Player") {
								if (item.CustomImage == 0) item.Image = item.TypeImg;
								if (item.Status == 'Disconnected') {
									xhtm += '\t      <td id="img"><img src="images/' + item.Image + '48_Off.png" height="48" width="48"></td>\n';
									xhtm += '\t      <td id="img2"><img src="images/remote48.png" style="opacity:0.4"; height="48" width="48"></td>\n';
								}
								else if ((item.Status != 'Off') && (item.Status != '0')) {
									xhtm += '\t      <td id="img"><img src="images/' + item.Image + '48_On.png" onclick="SwitchLight(' + item.idx + ',\'Off\',' + item.Protected + ');" class="lcursor" height="48" width="48"></td>\n';
									xhtm += '\t      <td id="img2"><img src="images/remote48.png" onclick="ShowMediaRemote(\'' + escape(item.Name) + "'," + item.idx + ",'" + item.HardwareType + '\');" class="lcursor" height="48" width="48"></td>\n';
								}
								else {
									xhtm += '\t      <td id="img"><img src="images/' + item.Image + '48_Off.png" onclick="SwitchLight(' + item.idx + ',\'On\',' + item.Protected + ');" class="lcursor" height="48" width="48"></td>\n';
									xhtm += '\t      <td id="img2"><img src="images/remote48.png" style="opacity:0.4"; height="48" width="48"></td>\n';
								}
								if (item.Status.length == 1) item.Status = "";
								status = item.Data;
								bAddTimer = false;
							}
							else if ((item.SwitchType == "Blinds") || (item.SwitchType.indexOf("Venetian Blinds") == 0)) {
								if (
									(item.SubType == "RAEX") ||
									(item.SubType.indexOf('A-OK') == 0) ||
									(item.SubType.indexOf('Hasta') >= 0) ||
									(item.SubType.indexOf('Media Mount') == 0) ||
									(item.SubType.indexOf('Forest') == 0) ||
									(item.SubType.indexOf('Chamberlain') == 0) ||
									(item.SubType.indexOf('Sunpery') == 0) ||
									(item.SubType.indexOf('Dolat') == 0) ||
									(item.SubType.indexOf('ASP') == 0) ||
									(item.SubType == "Harrison") ||
									(item.SubType.indexOf('RFY') == 0) ||
									(item.SubType.indexOf('ASA') == 0) ||
									(item.SubType.indexOf('DC106') == 0) ||
									(item.SubType.indexOf('Confexx') == 0) ||
									(item.SwitchType.indexOf("Venetian Blinds") == 0)
								) {
									if (item.Status == 'Closed') {
										xhtm += '\t      <td id="img"><img src="images/blindsopen48.png" title="' + $.t("Open Blinds") + '" onclick="SwitchLight(' + item.idx + ',\'Off\',' + item.Protected + ');" class="lcursor" height="48" width="48"></td>\n';
										xhtm += '\t      <td id="img2"><img src="images/blindsstop.png" title="' + $.t("Stop Blinds") + '" onclick="SwitchLight(' + item.idx + ',\'Stop\',' + item.Protected + ');" class="lcursor" height="48" width="24"></td>\n';
										xhtm += '\t      <td id="img3"><img src="images/blinds48sel.png" title="' + $.t("Close Blinds") + '" onclick="SwitchLight(' + item.idx + ',\'On\',' + item.Protected + ');" class="lcursor" height="48" width="48"></td>\n';
									}
									else {
										xhtm += '\t      <td id="img"><img src="images/blindsopen48sel.png" title="' + $.t("Open Blinds") + '" onclick="SwitchLight(' + item.idx + ',\'Off\',' + item.Protected + ');" class="lcursor" height="48" width="48"></td>\n';
										xhtm += '\t      <td id="img2"><img src="images/blindsstop.png" title="' + $.t("Stop Blinds") + '" onclick="SwitchLight(' + item.idx + ',\'Stop\',' + item.Protected + ');" class="lcursor" height="48" width="24"></td>\n';
										xhtm += '\t      <td id="img3"><img src="images/blinds48.png" title="' + $.t("Close Blinds") + '" onclick="SwitchLight(' + item.idx + ',\'On\',' + item.Protected + ');" class="lcursor" height="48" width="48"></td>\n';
									}
								}
								else {
									if (item.Status == 'Closed') {
										xhtm += '\t      <td id="img"><img src="images/blindsopen48.png" title="' + $.t("Open Blinds") + '" onclick="SwitchLight(' + item.idx + ',\'Off\',' + item.Protected + ');" class="lcursor" height="48" width="48"></td>\n';
										xhtm += '\t      <td id="img2"><img src="images/blinds48sel.png" title="' + $.t("Close Blinds") + '" onclick="SwitchLight(' + item.idx + ',\'On\',' + item.Protected + ');" class="lcursor" height="48" width="48"></td>\n';
									}
									else {
										xhtm += '\t      <td id="img"><img src="images/blindsopen48sel.png" title="' + $.t("Open Blinds") + '" onclick="SwitchLight(' + item.idx + ',\'Off\',' + item.Protected + ');" class="lcursor" height="48" width="48"></td>\n';
										xhtm += '\t      <td id="img2"><img src="images/blinds48.png" title="' + $.t("Close Blinds") + '" onclick="SwitchLight(' + item.idx + ',\'On\',' + item.Protected + ');" class="lcursor" height="48" width="48"></td>\n';
									}
								}
							}
							else if (item.SwitchType == "Blinds Inverted") {
								if (
									(item.SubType == "RAEX") ||
									(item.SubType.indexOf('A-OK') == 0) ||
									(item.SubType.indexOf('Hasta') >= 0) ||
									(item.SubType.indexOf('Media Mount') == 0) ||
									(item.SubType.indexOf('Forest') == 0) ||
									(item.SubType.indexOf('Chamberlain') == 0) ||
									(item.SubType.indexOf('Sunpery') == 0) ||
									(item.SubType.indexOf('Dolat') == 0) ||
									(item.SubType.indexOf('ASP') == 0) ||
									(item.SubType == "Harrison") ||
									(item.SubType.indexOf('RFY') == 0) ||
									(item.SubType.indexOf('ASA') == 0) ||
									(item.SubType.indexOf('DC106') == 0) ||
									(item.SubType.indexOf('Confexx') == 0)
								) {
									if (item.Status == 'Closed') {
										xhtm += '\t      <td id="img"><img src="images/blindsopen48.png" title="' + $.t("Open Blinds") + '" onclick="SwitchLight(' + item.idx + ',\'On\',' + item.Protected + ');" class="lcursor" height="48" width="48"></td>\n';
										xhtm += '\t      <td id="img2"><img src="images/blindsstop.png" title="' + $.t("Stop Blinds") + '" onclick="SwitchLight(' + item.idx + ',\'Stop\',' + item.Protected + ');" class="lcursor" height="48" width="24"></td>\n';
										xhtm += '\t      <td id="img3"><img src="images/blinds48sel.png" title="' + $.t("Close Blinds") + '" onclick="SwitchLight(' + item.idx + ',\'Off\',' + item.Protected + ');" class="lcursor" height="48" width="48"></td>\n';
									}
									else {
										xhtm += '\t      <td id="img"><img src="images/blindsopen48sel.png" title="' + $.t("Open Blinds") + '" onclick="SwitchLight(' + item.idx + ',\'On\',' + item.Protected + ');" class="lcursor" height="48" width="48"></td>\n';
										xhtm += '\t      <td id="img2"><img src="images/blindsstop.png" title="' + $.t("Stop Blinds") + '" onclick="SwitchLight(' + item.idx + ',\'Stop\',' + item.Protected + ');" class="lcursor" height="48" width="24"></td>\n';
										xhtm += '\t      <td id="img3"><img src="images/blinds48.png" title="' + $.t("Close Blinds") + '" onclick="SwitchLight(' + item.idx + ',\'Off\',' + item.Protected + ');" class="lcursor" height="48" width="48"></td>\n';
									}
								}
								else {
									if (item.Status == 'Closed') {
										xhtm += '\t      <td id="img"><img src="images/blindsopen48.png" title="' + $.t("Open Blinds") + '" onclick="SwitchLight(' + item.idx + ',\'On\',' + item.Protected + ');" class="lcursor" height="48" width="48"></td>\n';
										xhtm += '\t      <td id="img2"><img src="images/blinds48sel.png" title="' + $.t("Close Blinds") + '" onclick="SwitchLight(' + item.idx + ',\'Off\',' + item.Protected + ');" class="lcursor" height="48" width="48"></td>\n';
									}
									else {
										xhtm += '\t      <td id="img"><img src="images/blindsopen48sel.png" title="' + $.t("Open Blinds") + '" onclick="SwitchLight(' + item.idx + ',\'On\',' + item.Protected + ');" class="lcursor" height="48" width="48"></td>\n';
										xhtm += '\t      <td id="img2"><img src="images/blinds48.png" title="' + $.t("Close Blinds") + '" onclick="SwitchLight(' + item.idx + ',\'Off\',' + item.Protected + ');" class="lcursor" height="48" width="48"></td>\n';
									}
								}
							}
							else if (item.SwitchType == "Blinds Percentage") {
								bIsDimmer = true;
								if (item.Status == 'Open') {
									xhtm += '\t      <td id="img"><img src="images/blindsopen48sel.png" title="' + $.t("Open Blinds") + '" onclick="SwitchLight(' + item.idx + ',\'Off\',' + item.Protected + ');" class="lcursor" height="48"></td>\n';
									xhtm += '\t      <td id="img2"><img src="images/blinds48.png" title="' + $.t("Close Blinds") + '" onclick="SwitchLight(' + item.idx + ',\'On\',' + item.Protected + ');" class="lcursor" height="48"></td>\n';
								}
								else {
									xhtm += '\t      <td id="img"><img src="images/blindsopen48.png" title="' + $.t("Open Blinds") + '" onclick="SwitchLight(' + item.idx + ',\'Off\',' + item.Protected + ');" class="lcursor" height="48"></td>\n';
									xhtm += '\t      <td id="img2"><img src="images/blinds48sel.png" title="' + $.t("Close Blinds") + '" onclick="SwitchLight(' + item.idx + ',\'On\',' + item.Protected + ');" class="lcursor" height="48"></td>\n';
								}
							}
							else if (item.SwitchType == "Blinds Percentage Inverted") {
								bIsDimmer = true;
								if (item.Status == 'Closed') {
									xhtm += '\t	  <td id="img"><img src="images/blindsopen48.png" title="' + $.t("Open Blinds") + '" onclick="SwitchLight(' + item.idx + ',\'On\',' + item.Protected + ');" class="lcursor" height="48"></td>\n';
									xhtm += '\t	  <td id="img2"><img src="images/blinds48sel.png" title="' + $.t("Close Blinds") + '" onclick="SwitchLight(' + item.idx + ',\'Off\',' + item.Protected + ');" class="lcursor" height="48"></td>\n';
								}
								else {
									xhtm += '\t	  <td id="img"><img src="images/blindsopen48sel.png" title="' + $.t("Open Blinds") + '" onclick="SwitchLight(' + item.idx + ',\'On\',' + item.Protected + ');" class="lcursor" height="48"></td>\n';
									xhtm += '\t	  <td id="img2"><img src="images/blinds48.png" title="' + $.t("Close Blinds") + '" onclick="SwitchLight(' + item.idx + ',\'Off\',' + item.Protected + ');" class="lcursor" height="48"></td>\n';
								}
							}
							else if (item.SwitchType == "Smoke Detector") {
								if (
									(item.Status == 'Panic') ||
									(item.Status == 'On')
								) {
									xhtm += '\t      <td id="img"><img src="images/smoke48on.png" title="' + $.t("Turn Alarm On") + '" onclick="SwitchLight(' + item.idx + ',\'On\',' + item.Protected + ');" class="lcursor" height="48" width="48"></td>\n';
								}
								else {
									xhtm += '\t      <td id="img"><img src="images/smoke48off.png" title="' + $.t("Turn Alarm On") + '" onclick="SwitchLight(' + item.idx + ',\'On\',' + item.Protected + ');" class="lcursor" height="48" width="48"></td>\n';
								}
							}
							else if (item.Type == "Security") {
								if (item.SubType.indexOf('remote') > 0) {
									if ((item.Status.indexOf('Arm') >= 0) || (item.Status.indexOf('Panic') >= 0)) {
										xhtm += '\t      <td id="img"><img src="images/remote48.png" title="' + $.t("Turn Alarm Off") + '" onclick="SwitchLight(' + item.idx + ',\'Off\',' + item.Protected + ');" class="lcursor" height="48" width="48"></td>\n';
									}
									else {
										xhtm += '\t      <td id="img"><img src="images/remote48.png" title="' + $.t("Turn Alarm On") + '" onclick="ArmSystem(' + item.idx + ',\'On\',' + item.Protected + ');" class="lcursor" height="48" width="48"></td>\n';
									}
								}
								else if (item.SubType == "X10 security") {
									if (item.Status.indexOf('Normal') >= 0) {
										xhtm += '\t      <td id="img"><img src="images/security48.png" title="' + $.t("Turn Alarm On") + '" onclick="SwitchLight(' + item.idx + ',\'' + ((item.Status == "Normal Delayed") ? "Alarm Delayed" : "Alarm") + '\',' + item.Protected + ');" class="lcursor" height="48" width="48"></td>\n';
									}
									else {
										xhtm += '\t      <td id="img"><img src="images/Alarm48_On.png" title="' + $.t("Turn Alarm Off") + '" onclick="SwitchLight(' + item.idx + ',\'' + ((item.Status == "Alarm Delayed") ? "Normal Delayed" : "Normal") + '\',' + item.Protected + ');" class="lcursor" height="48" width="48"></td>\n';
									}
								}
								else if (item.SubType == "X10 security motion") {
									if ((item.Status == "No Motion")) {
										xhtm += '\t      <td id="img"><img src="images/security48.png" title="' + $.t("Turn Alarm On") + '" onclick="SwitchLight(' + item.idx + ',\'Motion\',' + item.Protected + ');" class="lcursor" height="48" width="48"></td>\n';
									}
									else {
										xhtm += '\t      <td id="img"><img src="images/Alarm48_On.png" title="' + $.t("Turn Alarm Off") + '" onclick="SwitchLight(' + item.idx + ',\'No Motion\',' + item.Protected + ');" class="lcursor" height="48" width="48"></td>\n';
									}
								}
								else if ((item.Status.indexOf('Alarm') >= 0) || (item.Status.indexOf('Tamper') >= 0)) {
									xhtm += '\t      <td id="img"><img src="images/Alarm48_On.png" height="48" width="48"></td>\n';
								}
								else {
									if (item.SubType.indexOf('Meiantech') >= 0) {
										if ((item.Status.indexOf('Arm') >= 0) || (item.Status.indexOf('Panic') >= 0)) {
											xhtm += '\t      <td id="img"><img src="images/security48.png" title="' + $.t("Turn Alarm Off") + '" onclick="SwitchLight(' + item.idx + ',\'Off\',' + item.Protected + ');" class="lcursor" height="48" width="48"></td>\n';
										}
										else {
											xhtm += '\t      <td id="img"><img src="images/security48.png" title="' + $.t("Turn Alarm On") + '" onclick="ArmSystemMeiantech(' + item.idx + ',\'On\',' + item.Protected + ');" class="lcursor" height="48" width="48"></td>\n';
										}
									}
									else {
										if (item.SubType.indexOf('KeeLoq') >= 0) {
											xhtm += '\t      <td id="img"><img src="images/pushon48.png" title="' + $.t("Turn On") + '" onclick="SwitchLight(' + item.idx + ',\'On\',' + item.Protected + ');" class="lcursor" height="48" width="48"></td>\n';
										}
										else {
											xhtm += '\t      <td id="img"><img src="images/security48.png" height="48" width="48"></td>\n';
										}
									}
								}
								bAddTimer = false;
							}
							else if (item.SwitchType == "Dimmer") {
								bIsDimmer = true;
								if (item.CustomImage == 0) item.Image = item.TypeImg;
								item.Image = item.Image.charAt(0).toUpperCase() + item.Image.slice(1);
								if (
									(item.Status == 'On') ||
									(item.Status == 'Chime') ||
									(item.Status == 'Group On') ||
									(item.Status.indexOf('Set ') == 0) ||
									(item.Status.indexOf('NightMode') == 0) ||
									(item.Status.indexOf('Disco ') == 0)
								) {
									if (isLED(item.SubType)) {
										if (item.Image == "Dimmer") {
											xhtm += '\t      <td id="img"><img src="images/RGB48_On.png" onclick="ShowRGBWPopup(event, ' + item.idx + ',' + item.Protected + ',' + item.MaxDimLevel + ',' + item.LevelInt + ',\'' + item.Color.replace(/\"/g , '\&quot;') + '\',\'' + item.SubType + '\',\'' + item.DimmerType + '\');" class="lcursor" height="48" width="48"></td>\n';
										}
										else {
											xhtm += '\t      <td id="img"><img src="images/' + item.Image + '48_On.png" onclick="ShowRGBWPopup(event, ' + item.idx + ',' + item.Protected + ',' + item.MaxDimLevel + ',' + item.LevelInt + ',\'' + item.Color.replace(/\"/g , '\&quot;') + '\',\'' + item.SubType + '\',\'' + item.DimmerType + '\');" class="lcursor" height="48" width="48"></td>\n';
										}
									}
									else {
										xhtm += '\t      <td id="img"><img src="images/' + item.Image + '48_On.png" title="' + $.t("Turn Off") + '" onclick="SwitchLight(' + item.idx + ',\'Off\',' + item.Protected + ');" class="lcursor" height="48" width="48"></td>\n';
									}
								}
								else {
									if (isLED(item.SubType)) {
										if (item.Image == "Dimmer") {
											xhtm += '\t      <td id="img"><img src="images/RGB48_Off.png" onclick="ShowRGBWPopup(event, ' + item.idx + ',' + item.Protected + ',' + item.MaxDimLevel + ',' + item.LevelInt + ',\'' + item.Color.replace(/\"/g , '\&quot;') + '\',\'' + item.SubType + '\',\'' + item.DimmerType + '\');" class="lcursor" height="48" width="48"></td>\n';
										}
										else {
											xhtm += '\t      <td id="img"><img src="images/' + item.Image + '48_Off.png" onclick="ShowRGBWPopup(event, ' + item.idx + ',' + item.Protected + ',' + item.MaxDimLevel + ',' + item.LevelInt + ',\'' + item.Color.replace(/\"/g , '\&quot;') + '\',\'' + item.SubType + '\',\'' + item.DimmerType + '\');" class="lcursor" height="48" width="48"></td>\n';
										}
									}
									else {
										xhtm += '\t      <td id="img"><img src="images/' + item.Image + '48_Off.png" title="' + $.t("Turn On") + '" onclick="SwitchLight(' + item.idx + ',\'On\',' + item.Protected + ');" class="lcursor" height="48" width="48"></td>\n';
									}
								}
							}
							else if (item.SwitchType == "TPI") {
								var RO = (item.Unit < 64 || item.Unit > 95) ? true : false;
								bIsDimmer = true;
								if (item.Status != 'Off') {
									xhtm += '\t      <td id="img"><img src="images/Fireplace48_On.png" title="' + $.t(RO ? "On" : "Turn Off") + (RO ? '"' : '" onclick="SwitchLight(' + item.idx + ',\'Off\',' + item.Protected + ');" class="lcursor"') + ' height="48" width="48"></td>\n';
								}
								else {
									xhtm += '\t      <td id="img"><img src="images/Fireplace48_Off.png" title="' + $.t(RO ? "Off" : "Turn On") + (RO ? '"' : '" onclick="SwitchLight(' + item.idx + ',\'On\',' + item.Protected + ');" class="lcursor"') + ' height="48" width="48"></td>\n';
								}
							}
							else if (item.SwitchType == "Dusk Sensor") {
								bAddTimer = false;
								if (item.Status == 'On') {
									xhtm += '\t      <td id="img"><img src="images/uvdark.png" title="' + $.t("Nighttime") + '" height="48" width="48"></td>\n';
								}
								else {
									xhtm += '\t      <td id="img"><img src="images/uvsunny.png" title="' + $.t("Daytime") + '" height="48" width="48"></td>\n';
								}
							}
							else if (item.SwitchType == "Motion Sensor") {
								if (
									(item.Status == 'On') ||
									(item.Status == 'Chime') ||
									(item.Status == 'Group On') ||
									(item.Status.indexOf('Set ') == 0)
								) {
									xhtm += '\t      <td id="img"><img src="images/motion48-on.png" height="48" width="48"></td>\n';
								}
								else {
									xhtm += '\t      <td id="img"><img src="images/motion48-off.png" height="48" width="48"></td>\n';
								}
								bAddTimer = false;
							}
							else if (item.SwitchType === "Selector") {
								if (item.Status === 'Off') {
									xhtm += '\t      <td id="img"><img src="images/' + item.Image + '48_Off.png" height="48" width="48"></td>\n';
								} else if (item.LevelOffHidden) {
									xhtm += '\t      <td id="img"><img src="images/' + item.Image + '48_On.png" height="48" width="48"></td>\n';
								} else {
									xhtm += '\t      <td id="img"><img src="images/' + item.Image + '48_On.png" title="' + $.t("Turn Off") + '" onclick="SwitchLight(' + item.idx + ',\'Off\',' + item.Protected + ');" class="lcursor" height="48" width="48"></td>\n';
								}
							}
							else if (item.SubType.indexOf("Itho") == 0) {
								bAddTimer = false;
								xhtm += '\t      <td id="img"><img src="images/Fan48_On.png" height="48" width="48" class="lcursor" onclick="ShowIthoPopup(event, ' + item.idx + ', ShowLights, ' + item.Protected + ');"></td>\n';
							}
							else if (item.SubType.indexOf("Lucci Air DC") == 0) {
								bAddTimer = false;
								xhtm += '\t      <td id="img"><img src="images/Fan48_On.png" height="48" width="48" class="lcursor" onclick="ShowLucciDCPopup(event, ' + item.idx + ', ShowLights, ' + item.Protected + ');"></td>\n';
							}
							else if (
								(item.SubType.indexOf("Lucci") == 0) ||
								(item.SubType.indexOf("Westinghouse") == 0)
								) {
							    bAddTimer = false;
							    xhtm += '\t      <td id="img"><img src="images/Fan48_On.png" height="48" width="48" class="lcursor" onclick="ShowLucciPopup(event, ' + item.idx + ', ShowLights, ' + item.Protected + ');"></td>\n';
							}
							else {
								if (
									(item.Status == 'On') ||
									(item.Status == 'Chime') ||
									(item.Status == 'Group On') ||
									(item.Status.indexOf('Down') != -1) ||
									(item.Status.indexOf('Up') != -1) ||
									(item.Status.indexOf('Set ') == 0)
								) {
									if (item.Type == "Thermostat 3") {
										xhtm += '\t      <td id="img"><img src="images/' + item.Image + '48_On.png" onclick="ShowTherm3Popup(event, ' + item.idx + ',' + item.Protected + ');" class="lcursor" height="48" width="48"></td>\n';
									}
									else {
										xhtm += '\t      <td id="img"><img src="images/' + item.Image + '48_On.png" title="' + $.t("Turn Off") + '" onclick="SwitchLight(' + item.idx + ',\'Off\',' + item.Protected + ');" class="lcursor" height="48" width="48"></td>\n';
									}
								}
								else {
									if (item.Type == "Thermostat 3") {
										xhtm += '\t      <td id="img"><img src="images/' + item.Image + '48_Off.png" onclick="ShowTherm3Popup(event, ' + item.idx + ',' + item.Protected + ');" class="lcursor" height="48" width="48"></td>\n';
									}
									else {
										xhtm += '\t      <td id="img"><img src="images/' + item.Image + '48_Off.png" title="' + $.t("Turn On") + '" onclick="SwitchLight(' + item.idx + ',\'On\',' + item.Protected + ');" class="lcursor" height="48" width="48"></td>\n';
									}
								}
							}
							xhtm +=
								'\t      <td id="status">' + status + '</td>\n' +
								'\t      <td id="lastupdate">' + item.LastUpdate + '</td>\n' +
								'\t      <td id="type">' + item.Type + ', ' + item.SubType + ', ' + item.SwitchType;
							if (item.SwitchType == "Dimmer") {
								if (item.DimmerType && item.DimmerType!="abs") {
									// Don't show dimmer slider if the device does not support absolute dimming
								}
								else {
									xhtm += '<br><br><div style="margin-left:60px;" class="dimslider" id="slider" data-idx="' + item.idx + '" data-type="norm" data-maxlevel="' + item.MaxDimLevel + '" data-isprotected="' + item.Protected + '" data-svalue="' + item.LevelInt + '" data-isled="' + isLED(item.SubType) + '"></div>';
								}
							}
							else if (item.SwitchType == "TPI") {
								xhtm += '<br><br><div style="margin-left:60px;" class="dimslider" id="slider" data-idx="' + item.idx + '" data-type="relay" data-maxlevel="' + item.MaxDimLevel + '" data-isprotected="' + item.Protected + '" data-svalue="' + item.LevelInt + '"';
								if (item.Unit < 64 || item.Unit > 95)
									xhtm += ' data-disabled="true"';
								xhtm += '></div>';
							}
							else if (item.SwitchType == "Blinds Percentage") {
								xhtm += '<br><div style="margin-left:108px; margin-top:7px;" class="dimslider dimsmall" id="slider" data-idx="' + item.idx + '" data-type="blinds" data-maxlevel="' + item.MaxDimLevel + '" data-isprotected="' + item.Protected + '" data-svalue="' + item.LevelInt + '"></div>';
							}
							else if (item.SwitchType == "Blinds Percentage Inverted") {
								xhtm += '<br><div style="margin-left:108px; margin-top:7px;" class="dimslider dimsmall" id="slider" data-idx="' + item.idx + '" data-type="blinds_inv" data-maxlevel="' + item.MaxDimLevel + '" data-isprotected="' + item.Protected + '" data-svalue="' + item.LevelInt + '"></div>';
							}
							else if (item.SwitchType == "Selector") {
								if (item.SelectorStyle === 0) {
									xhtm += '<br/><div class="btn-group" style="display: block; margin-top: 4px;" id="selector' + item.idx + '" data-idx="' + item.idx + '" data-isprotected="' + item.Protected + '" data-level="' + item.LevelInt + '" data-levelnames="' + item.LevelNames + '" data-selectorstyle="' + item.SelectorStyle + '" data-levelname="' + escape(GetLightStatusText(item)) + '" data-leveloffhidden="' + item.LevelOffHidden + '" data-levelactions="' + item.LevelActions + '">';
									var levelNames = b64DecodeUnicode(item.LevelNames).split('|');
									$.each(levelNames, function (index, levelName) {
										if ((index === 0) && (item.LevelOffHidden)) {
											return;
										}
										xhtm += '<button type="button" class="btn btn-small ';
										if ((index * 10) == item.LevelInt) {
											xhtm += 'btn-selected"';
										}
										else {
											xhtm += 'btn-default"';
										}
										xhtm += 'id="lSelector' + item.idx + 'Level' + index + '" name="selector' + item.idx + 'Level" value="' + (index * 10) + '" onclick="SwitchSelectorLevel(' + item.idx + ',\'' + unescape(levelName) + '\',' + (index * 10) + ',' + item.Protected + ');">' + levelName + '</button>';
									});
									xhtm += '</div>';
								} else if (item.SelectorStyle === 1) {
									xhtm += '<br><div class="selectorlevels" style="margin-top: 0.4em;">';
									xhtm += '<select id="selector' + item.idx + '" data-idx="' + item.idx + '" data-isprotected="' + item.Protected + '" data-level="' + item.LevelInt + '" data-levelnames="' + item.LevelNames + '" data-selectorstyle="' + item.SelectorStyle + '" data-levelname="' + escape(GetLightStatusText(item)) + '" data-leveloffhidden="' + item.LevelOffHidden + '" data-levelactions="' + item.LevelActions + '">';
									var levelNames = b64DecodeUnicode(item.LevelNames).split('|');
									$.each(levelNames, function (index, levelName) {
										if ((index === 0) && (item.LevelOffHidden)) {
											return;
										}
										xhtm += '<option value="' + (index * 10) + '">' + levelName + '</option>';
									});
									xhtm += '</select>';
									xhtm += '</div>';
								}
							}
							xhtm += '</td>\n' +
								'\t      <td class="options">';
							if (item.Favorite == 0) {
								xhtm +=
									'<img src="images/nofavorite.png" title="' + $.t('Add to Dashboard') + '" onclick="MakeFavorite(' + item.idx + ',1);" class="lcursor">&nbsp;&nbsp;&nbsp;&nbsp;';
							}
							else {
								xhtm +=
									'<img src="images/favorite.png" title="' + $.t('Remove from Dashboard') + '" onclick="MakeFavorite(' + item.idx + ',0);" class="lcursor">&nbsp;&nbsp;&nbsp;&nbsp;';
							}
							xhtm +=
								'<a class="btnsmall" href="#/Devices/' + item.idx + '/Log" data-i18n="Log">Log</a> ';
							if (permissions.hasPermission("Admin")) {
								xhtm +=
									'<a class="btnsmall" href="#/Devices/' + item.idx + '/LightEdit" data-i18n="Edit">Edit</a> ';
								if (bAddTimer == true) {
									var timerLink = '#/Devices/'+item.idx+'/Timers';
									if (item.Timers == "true") {
										xhtm += '<a class="btnsmall-sel" href="'+timerLink+'" data-i18n="Timers">Timers</a> ';
									}
									else {
										xhtm += '<a class="btnsmall" href="'+timerLink+'" data-i18n="Timers">Timers</a> ';
									}
								}
								if (item.SwitchType == "Smoke Detector") {
									if (
										(item.Status == 'Panic') ||
										(item.Status == 'On')
									) {
										xhtm += '<a id="resetbtn" class="btnsmall-sel" onclick="ResetSecurityStatus(' + item.idx + ',\'Normal\',ShowLights);" data-i18n="Reset">Reset</a> ';
									}
									else {
										xhtm += '<a id="resetbtn" class="btnsmall-dis" onclick="ResetSecurityStatus(' + item.idx + ',\'Normal\',ShowLights);" data-i18n="Reset">Reset</a> ';
									}
								}
								var notificationLink = '#/Devices/'+item.idx+'/Notifications';
								if (item.Notifications == "true")
									xhtm += '<a class="btnsmall-sel" href="' + notificationLink + '" data-i18n="Notifications">Notifications</a>';
								else
									xhtm += '<a class="btnsmall" href="' + notificationLink + '" data-i18n="Notifications">Notifications</a>';
							}
							xhtm +=
								'</td>\n' +
								'\t    </tr>\n' +
								'\t    </table>\n' +
								'\t  </section>\n' +
								'\t</div>\n';
							htmlcontent += xhtm;
							j += 1;
						});
					}
				}
			});
			if (bHaveAddedDevider == true) {
				//close previous devider
				htmlcontent += '</div>\n';
			}
			if (j == 0) {
				htmlcontent = '<h2>' + $.t('No Lights/Switches found or added in the system...') + '</h2>';
			}
			$('#modal').hide();
			$element.html(tophtm + htmlcontent);
			$element.i18n();
			if (bShowRoomplan == true) {
				$.each($.RoomPlans, function (i, item) {
					var option = $('<option />');
					option.attr('value', item.idx).text(item.name);
					$element.find("#comboroom").append(option);
				});
				if (typeof roomPlanId != 'undefined') {
					$element.find("#comboroom").val(roomPlanId);
				}
				$element.find("#comboroom").change(function () {
					var idx = $element.find("#comboroom option:selected").val();
					window.myglobals.LastPlanSelected = idx;

					$route.updateParams({
						room: idx > 0 ? idx : undefined
					});
					$location.replace();
					$scope.$apply();
				});
			}

			if ($scope.config.AllowWidgetOrdering == true) {
				if (permissions.hasPermission("Admin")) {
					if (window.myglobals.ismobileint == false) {
						$element.find(".span4").draggable({
							drag: function () {
								$.devIdx = $(this).attr("id");
								$(this).css("z-index", 2);
							},
							revert: true
						});
						$element.find(".span4").droppable({
							drop: function () {
								var myid = $(this).attr("id");
								var roomid = $element.find("#comboroom option:selected").val();
								if (typeof roomid == 'undefined') {
									roomid = 0;
								}
								$.ajax({
									url: "json.htm?type=command&param=switchdeviceorder&idx1=" + myid + "&idx2=" + $.devIdx + "&roomid=" + roomid,
									async: false,
									dataType: 'json',
									success: function (data) {
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
			$element.find('.dimslider').slider({
				//Config
				range: "min",
				min: 0,
				max: 15,
				value: 4,

				//Slider Events
				create: function (event, ui) {
					$(this).slider("option", "max", $(this).data('maxlevel'));
					$(this).slider("option", "type", $(this).data('type'));
					$(this).slider("option", "isprotected", $(this).data('isprotected'));
					$(this).slider("value", $(this).data('svalue'));
					if ($(this).data('disabled'))
						$(this).slider("option", "disabled", true);
				},
				slide: function (event, ui) { //When the slider is sliding
					clearInterval($.setDimValue);
					var maxValue = $(this).slider("option", "max");
					var dtype = $(this).slider("option", "type");
					var isled = $(this).data('isled');
					var isProtected = $(this).slider("option", "isprotected");
					var fPercentage = parseInt((100.0 / maxValue) * ui.value);
					var idx = $(this).data('idx');
					id = "#lightcontent #" + idx;
					var obj = $(id);
					if (typeof obj != 'undefined') {
						var img = "";
						var imgname = $('#' + idx + ' .lcursor').prop('src');
						imgname = imgname.substring(imgname.lastIndexOf("/") + 1, imgname.lastIndexOf("_O") + 2);
						if (dtype == "relay")
							imgname = "Fireplace48_O"
						var bigtext;
						if (fPercentage == 0) {
							img = '<img src="images/' + imgname + 'ff.png" title="' + $.t("Turn On") + '" onclick="SwitchLight(' + idx + ',\'On\',' + isProtected + ');" class="lcursor" height="48" width="48">';
							if (dtype == "blinds") {
								bigtext = "Open";
							}
							else if (dtype == "blinds_inv") {
								bigtext = "Closed";
							}
							else {
								bigtext = "Off";
							}
						}
						else {
							img = '<img src="images/' + imgname + 'n.png" title="' + $.t("Turn Off") + '" onclick="SwitchLight(' + idx + ',\'Off\',' + isProtected + ');" class="lcursor" height="48" width="48">';
							bigtext = fPercentage + " %";
						}
						if ((dtype != "blinds") && (dtype != "blinds_inv") && !isled) {
							if ($(id + " #img").html() != img) {
								$(id + " #img").html(img);
							}
						}
						if ($(id + " #bigtext").html() != bigtext) {
							$(id + " #bigtext").html(bigtext);
						}
					}
					if (dtype != "relay")
						$.setDimValue = setInterval(function () { SetDimValue(idx, ui.value); }, 500);
				},
				stop: function (event, ui) {
					var idx = $(this).data('idx');
					var dtype = $(this).slider("option", "type");
					if (dtype == "relay")
						SetDimValue(idx, ui.value);
				}
			});
			$scope.ResizeDimSliders();

			//Create Selector selectmenu
			$element.find('.selectorlevels select').selectmenu({
				//Config
				width: '75%',
				value: 0,
				//Selector selectmenu events
				create: function (event, ui) {
					var select$ = $(this),
						idx = select$.data('idx'),
						isprotected = select$.data('isprotected'),
						disabled = select$.data('disabled'),
						level = select$.data('level'),
						levelname = select$.data('levelname');
					select$.selectmenu("option", "idx", idx);
					select$.selectmenu("option", "isprotected", isprotected);
					select$.selectmenu("option", "disabled", disabled === true);
					select$.selectmenu("menuWidget").addClass('selectorlevels-menu');
					select$.val(level);

					$element.find('#' + idx + " #bigtext").html(unescape(levelname));
				},
				change: function (event, ui) { //When the user selects an option
					var select$ = $(this),
						idx = select$.selectmenu("option", "idx"),
						level = select$.selectmenu().val(),
						levelname = select$.find('option[value="' + level + '"]').text(),
						isprotected = select$.selectmenu("option", "isprotected");
					// Send command
					SwitchSelectorLevel(idx, unescape(levelname), level, isprotected);
					// Synchronize buttons and select attributes
					select$.data('level', level);
					select$.data('levelname', levelname);
				}
			}).selectmenu('refresh');

			RefreshLights();
			return false;
		}

		$scope.ResizeDimSliders = function () {
			var nobj = $element.find("#name");
			if (typeof nobj == 'undefined') {
				return;
			}
			var width = nobj.width() - 50;
			$element.find(".dimslider").width(width);
			$element.find(".dimsmall").width(width - 48);
		}

		$.strPad = function (i, l, s) {
			var o = i.toString();
			if (!s) { s = '0'; }
			while (o.length < l) {
				o = s + o;
			}
			return o;
		};

		UpdateAddManualDialog = function () {
			var lighttype = $("#dialog-addmanuallightdevice #lighttable #combolighttype option:selected").val();
			var bIsARCType = ((lighttype < 20) || (lighttype == 101));
			var bIsType5 = 0;

			var tothousecodes = 1;
			var totunits = 1;
			if ((lighttype == 0) || (lighttype == 1) || (lighttype == 3) || (lighttype == 101)) {
				tothousecodes = 16;
				totunits = 16;
			}
			else if ((lighttype == 2) || (lighttype == 5)) {
				tothousecodes = 16;
				totunits = 64;
			}
			else if (lighttype == 4) {
				tothousecodes = 3;
				totunits = 4;
			}
			else if (lighttype == 6) {
				tothousecodes = 4;
				totunits = 4;
			}
			else if (lighttype == 68) {
				//Do nothing. GPIO uses a custom form
			}
			else if (lighttype == 69) {
				//Do nothing. SysfsGpio uses a custom form
			}
			else if (lighttype == 7) {
				tothousecodes = 16;
				totunits = 8;
			}
			else if (lighttype == 8) {
				tothousecodes = 16;
				totunits = 4;
			}
			else if (lighttype == 9) {
				tothousecodes = 16;
				totunits = 10;
			}
			else if (lighttype == 10) {
				tothousecodes = 4;
				totunits = 4;
			}
			else if (lighttype == 11) {
				tothousecodes = 16;
				totunits = 64;
			}
			else if (lighttype == 106) {
				//Blyss
				tothousecodes = 16;
				totunits = 5;
			}
			else if (lighttype == 70) {
				//EnOcean
				tothousecodes = 128;
				totunits = 2;
			}
			else if (lighttype == 50) {
				//LightwaveRF
				bIsType5 = 1;
				totunits = 16;
			}
			else if (lighttype == 65) {
				//IT (Intertek,FA500,PROmax...)
				bIsType5 = 1;
				totunits = 4;
			}
			else if (lighttype == 55) {
				//Livolo
				bIsType5 = 1;
				totunits = 128;
			}
			else if (lighttype == 57) {
				//Aoke
				bIsType5 = 1;
			}
			else if (lighttype == 100) {
				//Chime/ByronSX
				bIsType5 = 1;
				totunits = 4;
			}
			else if ((lighttype == 102) || (lighttype == 107)) {
				//RFY/RFY2
				bIsType5 = 1;
				totunits = 16;
			}
			else if (lighttype == 103) {
				//Meiantech
				bIsType5 = 1;
				totunits = 0;
			}
			else if (lighttype == 105) {
				//ASA
				bIsType5 = 1;
				totunits = 16;
			}
			else if ((lighttype >= 200) && (lighttype < 300)) {
				//Blinds
			}
			else if (lighttype == 302) {
				//Home Confort
				tothousecodes = 4;
				totunits = 4;
			}
			else if ((lighttype == 400) || (lighttype == 401)) {
				//Openwebnet Bus Blinds/Lights
				totrooms = 11;//area, from 0 to 9 if physical configuration, 0 to 10 if virtual configuration
				totpointofloads = 16;//point of load, from 0 to 9 if physical configuration, 1 to 15 if virtual configuration
				totbus = 10;//maximum 10 local buses
			}
			else if (lighttype == 402) {
				//Openwebnet Bus Auxiliary
				totrooms = 10;
			}
			else if ((lighttype == 403) || (lighttype == 404)) {
				//Openwebnet Zigbee Blinds/Lights
				totunits = 3;//unit number is the button number on the switch (e.g. light1/light2 on a light switch)
			}
			else if (lighttype == 405) {
				//Openwebnet Bus Dry Contact
				totrooms = 200;
			}
			else if (lighttype == 406) {
				//Openwebnet Bus IR Detection
				totrooms = 10;
				totpointofloads = 10
            }
            else if (lighttype == 407) {
                //Openwebnet Bus Custom
                totrooms = 200;
                totbus = 10;//maximum 10 local buses
            }


			$("#dialog-addmanuallightdevice #he105params").hide();
			$("#dialog-addmanuallightdevice #blindsparams").hide();
			$("#dialog-addmanuallightdevice #lightingparams_enocean").hide();
			$("#dialog-addmanuallightdevice #lightingparams_gpio").hide();
			$("#dialog-addmanuallightdevice #lightingparams_sysfsgpio").hide();
			$("#dialog-addmanuallightdevice #homeconfortparams").hide();
			$("#dialog-addmanuallightdevice #fanparams").hide();
			$("#dialog-addmanuallightdevice #openwebnetparamsBus").hide();
			$("#dialog-addmanuallightdevice #openwebnetparamsAUX").hide();
			$("#dialog-addmanuallightdevice #openwebnetparamsZigbee").hide();
			$("#dialog-addmanuallightdevice #openwebnetparamsDryContact").hide();
			$("#dialog-addmanuallightdevice #openwebnetparamsIRdetec").hide();
			$("#dialog-addmanuallightdevice #openwebnetparamsCustom").hide();

			if (lighttype == 104) {
				//HE105
				$("#dialog-addmanuallightdevice #lighting1params").hide();
				$("#dialog-addmanuallightdevice #lighting2params").hide();
				$("#dialog-addmanuallightdevice #lighting3params").hide();
				$("#dialog-addmanuallightdevice #he105params").show();
			}
			else if (lighttype == 303) {
				$("#dialog-addmanuallightdevice #lighting1params").hide();
				$("#dialog-addmanuallightdevice #lighting2params").show();
				$("#dialog-addmanuallightdevice #lighting3params").hide();
			}
			else if (lighttype == 106) {
				//Blyss
				$('#dialog-addmanuallightdevice #lightparams3 #combogroupcode >option').remove();
				$('#dialog-addmanuallightdevice #lightparams3 #combounitcode >option').remove();
				for (ii = 0; ii < tothousecodes; ii++) {
					$('#dialog-addmanuallightdevice #lightparams3 #combogroupcode').append($('<option></option>').val(41 + ii).html(String.fromCharCode(65 + ii)));
				}
				for (ii = 1; ii < totunits + 1; ii++) {
					$('#dialog-addmanuallightdevice #lightparams3 #combounitcode').append($('<option></option>').val(ii).html(ii));
				}
				$("#dialog-addmanuallightdevice #lighting1params").hide();
				$("#dialog-addmanuallightdevice #lighting2params").hide();
				$("#dialog-addmanuallightdevice #lighting3params").show();
			}
			else if (lighttype == 70) {
				//EnOcean
				$("#dialog-addmanuallightdevice #lightparams_enocean #comboid  >option").remove();
				$("#dialog-addmanuallightdevice #lightparams_enocean #combounitcode  >option").remove();
				for (ii = 1; ii < tothousecodes + 1; ii++) {
					$('#dialog-addmanuallightdevice #lightparams_enocean #comboid').append($('<option></option>').val(ii).html(ii));
				}
				for (ii = 1; ii < totunits + 1; ii++) {
					$('#dialog-addmanuallightdevice #lightparams_enocean #combounitcode').append($('<option></option>').val(ii).html(ii));
				}
				$("#dialog-addmanuallightdevice #lighting1params").hide();
				$("#dialog-addmanuallightdevice #lighting2params").hide();
				$("#dialog-addmanuallightdevice #lighting3params").hide();
				$("#dialog-addmanuallightdevice #lightingparams_enocean").show();
			}
			else if (lighttype == 68) {
				//GPIO
				$("#dialog-addmanuallightdevice #lightparams_enocean #comboid  >option").remove();
				$("#dialog-addmanuallightdevice #lightparams_enocean #combounitcode  >option").remove();
				$("#dialog-addmanuallightdevice #lighting1params").hide();
				$("#dialog-addmanuallightdevice #lighting2params").hide();
				$("#dialog-addmanuallightdevice #lighting3params").hide();
				$("#dialog-addmanuallightdevice #lightingparams_gpio").show();
			}
			else if (lighttype == 69) {
				//SysfsGpio
				$("#dialog-addmanuallightdevice #lightparams_enocean #comboid  >option").remove();
				$("#dialog-addmanuallightdevice #lightparams_enocean #combounitcode  >option").remove();
				$("#dialog-addmanuallightdevice #lighting1params").hide();
				$("#dialog-addmanuallightdevice #lighting2params").show();
				$("#dialog-addmanuallightdevice #lighting3params").hide();
				$("#dialog-addmanuallightdevice #lighting2paramsUnitCode").hide()
				$("#dialog-addmanuallightdevice #lightingparams_sysfsgpio").show();
			}
			else if ((lighttype >= 200) && (lighttype < 300)) {
				//Blinds
				$("#dialog-addmanuallightdevice #blindsparams").show();
				var bShow1 = (lighttype == 205) || (lighttype == 206) || (lighttype == 207) || (lighttype == 210) || (lighttype == 211) || (lighttype == 250) || (lighttype == 226);
				var bShow4 = (lighttype == 206) || (lighttype == 207) || (lighttype == 209);
				var bShowUnit = (lighttype == 206) || (lighttype == 207) || (lighttype == 208) || (lighttype == 209) || (lighttype == 212) || (lighttype == 213) || (lighttype == 215) || (lighttype == 250) || (lighttype == 226);
				if (bShow1)
					$('#dialog-addmanuallightdevice #blindsparams #combocmd1').show();
				else {
					$('#dialog-addmanuallightdevice #blindsparams #combocmd1').hide();
					$('#dialog-addmanuallightdevice #blindsparams #combocmd1').val(0);
				}
				if (bShow4)
					$('#dialog-addmanuallightdevice #blindsparams #combocmd4').show();
				else {
					$('#dialog-addmanuallightdevice #blindsparams #combocmd4').hide();
					$('#dialog-addmanuallightdevice #blindsparams #combocmd4').val(0);
				}
				if (bShowUnit)
					$('#dialog-addmanuallightdevice #blindparamsUnitCode').show();
				else
					$('#dialog-addmanuallightdevice #blindparamsUnitCode').hide();

				$("#dialog-addmanuallightdevice #lighting1params").hide();
				$("#dialog-addmanuallightdevice #lighting2params").hide();
				$("#dialog-addmanuallightdevice #lighting3params").hide();
			}
			else if (lighttype == 302) {
				//Home Confort
				$("#dialog-addmanuallightdevice #lighting1params").hide();
				$("#dialog-addmanuallightdevice #lighting2params").hide();
				$("#dialog-addmanuallightdevice #lighting3params").hide();
				$("#dialog-addmanuallightdevice #homeconfortparams").show();
			}
			else if ((lighttype >= 304) && (lighttype <= 313)) {
				//Fan (Itho)
				$("#dialog-addmanuallightdevice #lighting1params").hide();
				$("#dialog-addmanuallightdevice #lighting2params").hide();
				$("#dialog-addmanuallightdevice #lighting3params").hide();
				$("#dialog-addmanuallightdevice #fanparams").show();
			}
			else if ((lighttype == 400) || (lighttype == 401)) {
				//Openwebnet Bus Blinds/Light
				$("#dialog-addmanuallightdevice #openwebnetparamsBus #combocmd1  >option").remove();
				for (ii = 1; ii < totrooms; ii++) {
					$('#dialog-addmanuallightdevice #openwebnetparamsBus #combocmd1').append($('<option></option>').val(ii).html(ii));
				}
				$("#dialog-addmanuallightdevice #openwebnetparamsBus #combocmd2  >option").remove();
				for (ii = 1; ii < totpointofloads; ii++) {
					$('#dialog-addmanuallightdevice #openwebnetparamsBus #combocmd2').append($('<option></option>').val(ii).html(ii));
				}
				$("#dialog-addmanuallightdevice #openwebnetparamsBus #combocmd3  >option").remove();
				$("#dialog-addmanuallightdevice #openwebnetparamsBus #combocmd3").append($('<option></option>').val(0).html("local bus"));
				for (ii = 1; ii < totbus; ii++) {
					$("#dialog-addmanuallightdevice #openwebnetparamsBus #combocmd3").append($('<option></option>').val(ii).html(ii));
				}

				$("#dialog-addmanuallightdevice #lighting1params").hide();
				$("#dialog-addmanuallightdevice #lighting2params").hide();
				$("#dialog-addmanuallightdevice #lighting3params").hide();
				$("#dialog-addmanuallightdevice #openwebnetparamsBus").show();
			}
			else if (lighttype == 402) {
				//Openwebnet Bus Auxiliary
				$("#dialog-addmanuallightdevice #openwebnetparamsAUX #combocmd1  >option").remove();
				for (ii = 1; ii < totrooms; ii++) {
					$('#dialog-addmanuallightdevice #openwebnetparamsAUX #combocmd1').append($('<option></option>').val(ii).html(ii));
				}
				$("#dialog-addmanuallightdevice #lighting1params").hide();
				$("#dialog-addmanuallightdevice #lighting2params").hide();
				$("#dialog-addmanuallightdevice #lighting3params").hide();
				$("#dialog-addmanuallightdevice #openwebnetparamsAUX").show();
			}
			else if ((lighttype == 403) || (lighttype == 404)) {
				//Openwebnet Zigbee Blinds/Light
				$("#dialog-addmanuallightdevice #lighting1params").hide();
				$("#dialog-addmanuallightdevice #lighting2params").hide();
				$("#dialog-addmanuallightdevice #lighting3params").hide();
				$("#dialog-addmanuallightdevice #openwebnetparamsBus").hide();
				$("#dialog-addmanuallightdevice #openwebnetparamsAUX").hide();
				$("#dialog-addmanuallightdevice #openwebnetparamsZigbee").show();
				$("#dialog-addmanuallightdevice #openwebnetparamsZigbee #combocmd2  >option").remove();
				for (ii = 1; ii < totunits + 1; ii++) {
					$('#dialog-addmanuallightdevice #openwebnetparamsZigbee #combocmd2').append($('<option></option>').val(ii).html(ii));
				}
			}
			else if (lighttype == 405) {
				//Openwebnet Dry Contact
				$("#dialog-addmanuallightdevice #openwebnetparamsDryContact #combocmd1  >option").remove();
				for (ii = 1; ii < totrooms; ii++) {
					$('#dialog-addmanuallightdevice #openwebnetparamsDryContact #combocmd1').append($('<option></option>').val(ii).html(ii));
				}

				$("#dialog-addmanuallightdevice #lighting1params").hide();
				$("#dialog-addmanuallightdevice #lighting2params").hide();
				$("#dialog-addmanuallightdevice #lighting3params").hide();
				$("#dialog-addmanuallightdevice #openwebnetparams").hide();
				$("#dialog-addmanuallightdevice #openwebnetparamsDryContact").show();
			}
			else if (lighttype == 406) {
				//Openwebnet IR Detection
				$("#dialog-addmanuallightdevice #openwebnetparamsIRdetec #combocmd1  >option").remove();
				for (ii = 1; ii < totrooms; ii++) {
					$('#dialog-addmanuallightdevice #openwebnetparamsIRdetec #combocmd1').append($('<option></option>').val(ii).html(ii));
				}
				$("#dialog-addmanuallightdevice #openwebnetparamsIRdetec #combocmd2  >option").remove();
				for (ii = 1; ii < totpointofloads; ii++) {
					$('#dialog-addmanuallightdevice #openwebnetparamsIRdetec #combocmd2').append($('<option></option>').val(ii).html(ii));
				}
				$("#dialog-addmanuallightdevice #lighting1params").hide();
				$("#dialog-addmanuallightdevice #lighting2params").hide();
				$("#dialog-addmanuallightdevice #lighting3params").hide();
				$("#dialog-addmanuallightdevice #openwebnetparamsIRdetec").show();
            }
            else if (lighttype == 407) {
                //Openwebnet Bus Custom
                $("#dialog-addmanuallightdevice #openwebnetparamsCustom #combocmd1  >option").remove();
                for (ii = 1; ii < totrooms + 1; ii++) {
                    $('#dialog-addmanuallightdevice #openwebnetparamsCustom #combocmd1').append($('<option></option>').val(ii).html(ii));
                }
                $("#dialog-addmanuallightdevice #openwebnetparamsCustom #combocmd2  >option").remove();
                $("#dialog-addmanuallightdevice #openwebnetparamsCustom #combocmd2").append($('<option></option>').val(0).html("local bus"));
                for (ii = 1; ii < totbus; ii++) {
                    $("#dialog-addmanuallightdevice #openwebnetparamsCustom #combocmd2").append($('<option></option>').val(ii).html(ii));
                }
                $("#dialog-addmanuallightdevice #lighting1params").hide();
                $("#dialog-addmanuallightdevice #lighting2params").hide();
                $("#dialog-addmanuallightdevice #lighting3params").hide();
                $("#dialog-addmanuallightdevice #openwebnetparamsCustom").show();
            }
			else if (bIsARCType == 1) {
				$('#dialog-addmanuallightdevice #lightparams1 #combohousecode >option').remove();
				$('#dialog-addmanuallightdevice #lightparams1 #combounitcode >option').remove();
				for (ii = 0; ii < tothousecodes; ii++) {
					$('#dialog-addmanuallightdevice #lightparams1 #combohousecode').append($('<option></option>').val(65 + ii).html(String.fromCharCode(65 + ii)));
				}
				for (ii = 1; ii < totunits + 1; ii++) {
					$('#dialog-addmanuallightdevice #lightparams1 #combounitcode').append($('<option></option>').val(ii).html(ii));
				}
				$("#dialog-addmanuallightdevice #lighting1params").show();
				$("#dialog-addmanuallightdevice #lighting2params").hide();
				$("#dialog-addmanuallightdevice #lighting3params").hide();
			}
			else {
				if (lighttype == 103) {
					$("#dialog-addmanuallightdevice #lighting2paramsUnitCode").hide();
				}
				else {
					$("#dialog-addmanuallightdevice #lighting2paramsUnitCode").show();
					if (lighttype == 55) {
						$('#dialog-addmanuallightdevice #lightparams2 #combounitcode >option').remove();
						for (ii = 1; ii < totunits + 1; ii++) {
							$('#dialog-addmanuallightdevice #lightparams2 #combounitcode').append($('<option></option>').val(ii).html(ii));
						}
					}
				}
				$("#dialog-addmanuallightdevice #lighting2params #combocmd2").show();
				if (bIsType5 == 0) {
					$("#dialog-addmanuallightdevice #lighting2params #combocmd1").show();
				}
				else {
					$("#dialog-addmanuallightdevice #lighting2params #combocmd1").hide();
					if ((lighttype == 55) || (lighttype == 57) || (lighttype == 65) || (lighttype == 100)) {
						$("#dialog-addmanuallightdevice #lighting2params #combocmd2").hide();
						$("#dialog-addmanuallightdevice #lighting2params #combocmd2").val(0);
						if ((lighttype != 55) && (lighttype != 65) && (lighttype != 100)) {
							$("#dialog-addmanuallightdevice #lighting2paramsUnitCode").hide();
						}
					}
				}
				$("#dialog-addmanuallightdevice #lighting1params").hide();
				$("#dialog-addmanuallightdevice #lighting2params").show();
				$("#dialog-addmanuallightdevice #lighting3params").hide();
			}
		}

		GetManualLightSettings = function (isTest) {
			var mParams = "";
			var hwdID = $("#dialog-addmanuallightdevice #lighttable #combohardware option:selected").val();
			if (typeof hwdID == 'undefined') {
				ShowNotify($.t('No Hardware Device selected!'), 2500, true);
				return "";
			}
			mParams += "&hwdid=" + hwdID;

			var name = $("#dialog-addmanuallightdevice #devicename");
			if ((name.val() == "") || (!checkLength(name, 2, 100))) {
				if (!isTest) {
					ShowNotify($.t('Invalid Name!'), 2500, true);
					return "";
				}
			}
			mParams += "&name=" + encodeURIComponent(name.val());

			var description = $("#dialog-addmanuallightdevice #devicedescription");
			mParams += "&description=" + encodeURIComponent(description.val());

			mParams += "&switchtype=" + $("#dialog-addmanuallightdevice #lighttable #comboswitchtype option:selected").val();
			var lighttype = $("#dialog-addmanuallightdevice #lighttable #combolighttype option:selected").val();
			mParams += "&lighttype=" + lighttype;
			if (lighttype == 106) {
				//Blyss
				mParams += "&groupcode=" + $("#dialog-addmanuallightdevice #lightparams3 #combogroupcode option:selected").val();
				mParams += "&unitcode=" + $("#dialog-addmanuallightdevice #lightparams3 #combounitcode option:selected").val();
				ID =
					$("#dialog-addmanuallightdevice #lightparams3 #combocmd1 option:selected").text() +
					$("#dialog-addmanuallightdevice #lightparams3 #combocmd2 option:selected").text();
				mParams += "&id=" + ID;
			}
			else if (lighttype == 70) {
				//EnOcean
				//mParams+="&groupcode="+$("#dialog-addmanuallightdevice #lightingparams_enocean #comboid option:selected").val();
				//mParams+="&unitcode="+$("#dialog-addmanuallightdevice #lightingparams_enocean #combounitcode option:selected").val();
				mParams += "&groupcode=" + $("#dialog-addmanuallightdevice #lightingparams_enocean #combounitcode option:selected").val();
				mParams += "&unitcode=" + $("#dialog-addmanuallightdevice #lightingparams_enocean #comboid option:selected").val();
				ID = "EnOcean";
				mParams += "&id=" + ID;
			}
			else if (lighttype == 68) {
				//GPIO
                mParams += "&id=GPIO&unitcode=" + $("#dialog-addmanuallightdevice #lightingparams_gpio #combogpio option:selected").val();
			}
			else if (lighttype == 69) {
				//SysfsGpio
                ID =
                    $("#dialog-addmanuallightdevice #lightparams2 #combocmd1 option:selected").text() +
                    $("#dialog-addmanuallightdevice #lightparams2 #combocmd2 option:selected").text() +
                    $("#dialog-addmanuallightdevice #lightparams2 #combocmd3 option:selected").text() +
                    $("#dialog-addmanuallightdevice #lightparams2 #combocmd4 option:selected").text();
                mParams += "&id=" + ID + "&unitcode=" + $("#dialog-addmanuallightdevice #lightingparams_sysfsgpio #combosysfsgpio option:selected").val();
            }
			else if ((lighttype < 20) || (lighttype == 101)) {
				mParams += "&housecode=" + $("#dialog-addmanuallightdevice #lightparams1 #combohousecode option:selected").val();
				mParams += "&unitcode=" + $("#dialog-addmanuallightdevice #lightparams1 #combounitcode option:selected").val();
			}
			else if (lighttype == 104) {
				mParams += "&unitcode=" + $("#dialog-addmanuallightdevice #he105params #combounitcode option:selected").text();
			}
			else if ((lighttype >= 200) && (lighttype < 300)) {
				//Blinds
				ID =
					$("#dialog-addmanuallightdevice #blindsparams #combocmd1 option:selected").text() +
					$("#dialog-addmanuallightdevice #blindsparams #combocmd2 option:selected").text() +
					$("#dialog-addmanuallightdevice #blindsparams #combocmd3 option:selected").text() +
					"0" + $("#dialog-addmanuallightdevice #blindsparams #combocmd4 option:selected").text();
				mParams += "&id=" + ID;
				mParams += "&unitcode=" + $("#dialog-addmanuallightdevice #blindsparams #combounitcode option:selected").val();
			}
			else if (lighttype == 302) {
				//Home Confort
				ID =
					$("#dialog-addmanuallightdevice #homeconfortparams #combocmd1 option:selected").text() +
					$("#dialog-addmanuallightdevice #homeconfortparams #combocmd2 option:selected").text() +
					$("#dialog-addmanuallightdevice #homeconfortparams #combocmd3 option:selected").text();
				mParams += "&id=" + ID;
				mParams += "&housecode=" + $("#dialog-addmanuallightdevice #homeconfortparams #combohousecode option:selected").val();
				mParams += "&unitcode=" + $("#dialog-addmanuallightdevice #homeconfortparams #combounitcode option:selected").val();
			}
			else if ((lighttype >= 304)&&(lighttype <= 313)) {
				//Fan
				ID =
					$("#dialog-addmanuallightdevice #fanparams #combocmd1 option:selected").text() +
					$("#dialog-addmanuallightdevice #fanparams #combocmd2 option:selected").text() +
					$("#dialog-addmanuallightdevice #fanparams #combocmd3 option:selected").text();
				mParams += "&id=" + ID;
			}
			else if (lighttype == 400) {
				//OpenWebNet Bus Blinds
				var appID = parseInt($("#dialog-addmanuallightdevice #openwebnetparamsBus #combocmd1 option:selected").val() +
					$("#dialog-addmanuallightdevice #openwebnetparamsBus #combocmd2 option:selected").val());
				var ID = ("002" + ("0000" + appID.toString(16)).slice(-4)); // WHO_AUTOMATION
				var unitcode = $("#dialog-addmanuallightdevice #openwebnetparamsBus #combocmd3 option:selected").val();
				mParams += "&id=" + ID.toUpperCase() + "&unitcode=" + unitcode;
			}
			else if (lighttype == 401) {
				//OpenWebNet Bus Lights
				var appID = parseInt($("#dialog-addmanuallightdevice #openwebnetparamsBus #combocmd1 option:selected").val() +
					$("#dialog-addmanuallightdevice #openwebnetparamsBus #combocmd2 option:selected").val());
				var ID = ("001" + ("0000" + appID.toString(16)).slice(-4)); // WHO_LIGHTING
				var unitcode = $("#dialog-addmanuallightdevice #openwebnetparamsBus #combocmd3 option:selected").val();
				mParams += "&id=" + ID.toUpperCase() + "&unitcode=" + unitcode;
			}
			else if (lighttype == 402) {
				//OpenWebNet Bus Auxiliary
				var appID = parseInt($("#dialog-addmanuallightdevice #openwebnetparamsAUX #combocmd1 option:selected").val());
				var ID = ("009" + ("0000" + appID.toString(16)).slice(-4)); // WHO_AUXILIARY
				var unitcode = "0";
				mParams += "&id=" + ID.toUpperCase() + "&unitcode=" + unitcode;
			}
			else if (lighttype == 403) {
				//OpenWebNet Zigbee Blinds
				var ID = $("#dialog-addmanuallightdevice #openwebnetparamsZigbee #inputcmd1").val();
				if (parseInt(ID) <= 0 || parseInt(ID) >= 0xFFFFFFFF) {
					ShowNotify($.t('Zigbee id is incorrect!'), 2500, true);
					return "";
				}
				var unitcode = $("#dialog-addmanuallightdevice #openwebnetparamsZigbee #combocmd2 option:selected").val();
				mParams += "&id=" + ID + "&unitcode=" + unitcode;
			}
			else if (lighttype == 404) {
				//OpenWebNet Zigbee Light
				var ID = $("#dialog-addmanuallightdevice #openwebnetparamsZigbee #inputcmd1").val();
				if (parseInt(ID) <= 0 || parseInt(ID) >= 0xFFFFFFFF) {
					ShowNotify($.t('Zigbee id is incorrect!'), 2500, true);
					return "";
				}
				var unitcode = $("#dialog-addmanuallightdevice #openwebnetparamsZigbee #combocmd2 option:selected").val();
				mParams += "&id=" + ID + "&unitcode=" + unitcode;
			}
			else if (lighttype == 405) {
				//OpenWebNet Dry Contact
				var appID = parseInt($("#dialog-addmanuallightdevice #openwebnetparamsDryContact #combocmd1 option:selected").val());
				var ID = ("0019" + ("0000" + appID.toString(16)).slice(-4)); // WHO_DRY_CONTACT_IR_DETECTION (25 = 0x19)
				var unitcode = "0";
				mParams += "&id=" + ID.toUpperCase() + "&unitcode=" + unitcode;
			}
			else if (lighttype == 406) {
				//OpenWebNet IR Detection
				var appID = parseInt($("#dialog-addmanuallightdevice #openwebnetparamsIRdetec #combocmd1 option:selected").val() +
					$("#dialog-addmanuallightdevice #openwebnetparamsIRdetec #combocmd2 option:selected").val());
				var ID = ("0019" + ("0000" + appID.toString(16)).slice(-4)); // WHO_DRY_CONTACT_IR_DETECTION (25 = 0x19)
				var unitcode = "0";
				mParams += "&id=" + ID.toUpperCase() + "&unitcode=" + unitcode;
            }
            else if (lighttype == 407) {
                //Openwebnet Bus Custom
                var appID = parseInt($("#dialog-addmanuallightdevice #openwebnetparamsCustom #combocmd1 option:selected").val());
                var ID = ("F00" + ("0000" + appID.toString(16)).slice(-4));
                var unitcode = $("#dialog-addmanuallightdevice #openwebnetparamsCustom #combocmd2 option:selected").val();

                //var intRegex = /^[0-9*#]$/; 
                //if (!intRegex.test(cmdText)) {
                //    ShowNotify($.t('Open command error. Please use only number, \'*\' or \'#\''), 2500, true);
                //    return "";
                //}
                mParams += "&id=" + ID + "&unitcode=" + unitcode + "&StrParam1=" + encodeURIComponent($("#dialog-addmanuallightdevice #openwebnetparamsCustom #inputcmd1").val());
            }
			else {
				//AC
				var ID = "";
				var bIsType5 = 0;
				if (
					(lighttype == 50) ||
					(lighttype == 55) ||
					(lighttype == 57) ||
					(lighttype == 65) ||
					(lighttype == 100) ||
					(lighttype == 102) ||
					(lighttype == 103) ||
					(lighttype == 105) ||
					(lighttype == 107)
				) {
					bIsType5 = 1;
				}
				if (bIsType5 == 0) {
					ID =
						$("#dialog-addmanuallightdevice #lightparams2 #combocmd1 option:selected").text() +
						$("#dialog-addmanuallightdevice #lightparams2 #combocmd2 option:selected").text() +
						$("#dialog-addmanuallightdevice #lightparams2 #combocmd3 option:selected").text() +
						$("#dialog-addmanuallightdevice #lightparams2 #combocmd4 option:selected").text();
				}
				else {
					if ((lighttype != 57) && (lighttype != 100)) {
						ID =
							$("#dialog-addmanuallightdevice #lightparams2 #combocmd2 option:selected").text() +
							$("#dialog-addmanuallightdevice #lightparams2 #combocmd3 option:selected").text() +
							$("#dialog-addmanuallightdevice #lightparams2 #combocmd4 option:selected").text();
					}
					else {
						ID =
							$("#dialog-addmanuallightdevice #lightparams2 #combocmd3 option:selected").text() +
							$("#dialog-addmanuallightdevice #lightparams2 #combocmd4 option:selected").text();
					}
				}
				if (ID == "") {
					ShowNotify($.t('Invalid Switch ID!'), 2500, true);
					return "";
				}
				mParams += "&id=" + ID;
				mParams += "&unitcode=" + $("#dialog-addmanuallightdevice #lightparams2 #combounitcode option:selected").val();
			}

			var bIsSubDevice = $("#dialog-addmanuallightdevice #howtable #how_2").is(":checked");
			var MainDeviceIdx = "";
			if (bIsSubDevice) {
				var MainDeviceIdx = $("#dialog-addmanuallightdevice #combosubdevice option:selected").val();
				if (typeof MainDeviceIdx == 'undefined') {
					bootbox.alert($.t('No Main Device Selected!'));
					return "";
				}
			}
			if (MainDeviceIdx != "") {
				mParams += "&maindeviceidx=" + MainDeviceIdx;
			}

			return mParams;
		}

		TestManualLight = function () {
			var mParams = GetManualLightSettings(1);
			if (mParams == "") {
				return;
			}
			$.ajax({
				url: "json.htm?type=command&param=testswitch" + mParams,
				async: false,
				dataType: 'json',
				success: function (data) {
					if (typeof data.status != 'undefined') {
						if (data.status != 'OK') {
							ShowNotify($.t("There was an error!<br>Wrong switch parameters?") + ((typeof data.message != 'undefined') ? "<br>" + data.message : ""), 2500, true);
						}
						else {
							ShowNotify($.t("'On' command send!"), 2500);
						}
					}
				}
			});
		}

		EnableDisableSubDevices = function (ElementID, bEnabled) {
			var trow = $(ElementID);
			if (bEnabled == true) {
				trow.show();
			}
			else {
				trow.hide();
			}
		}

		init();

		function init() {
			//global var
			$.devIdx = 0;
			$.LastUpdateTime = parseInt(0);

			$.myglobals = {
				TimerTypesStr: [],
				CommandStr: [],
				OccurenceStr: [],
				MonthStr: [],
				WeekdayStr: [],
				SelectedTimerIdx: 0
			};
			$.LightsAndSwitches = [];
			$scope.MakeGlobalConfig();

			$('#timerparamstable #combotype > option').each(function () {
				$.myglobals.TimerTypesStr.push($(this).text());
			});
			$('#timerparamstable #combocommand > option').each(function () {
				$.myglobals.CommandStr.push($(this).text());
			});
			$('#timerparamstable #occurence > option').each(function () {
				$.myglobals.OccurenceStr.push($(this).text());
			});
			$('#timerparamstable #months > option').each(function () {
				$.myglobals.MonthStr.push($(this).text());
			});
			$('#timerparamstable #weekdays > option').each(function () {
				$.myglobals.WeekdayStr.push($(this).text());
			});

			$scope.$on('device_update', function (event, deviceData) {
				RefreshItem(deviceData);
			});

			$(window).resize(function () { $scope.ResizeDimSliders(); });

			$("#dialog-addlightdevice").dialog({
				autoOpen: false,
				width: 400,
				height: 290,
				modal: true,
				resizable: false,
				title: $.t("Add Light/Switch Device"),
				buttons: {
					"Add Device": function () {
						var bValid = true;
						bValid = bValid && checkLength($("#dialog-addlightdevice #devicename"), 2, 100);
						var bIsSubDevice = $("#dialog-addlightdevice #how_2").is(":checked");
						var MainDeviceIdx = "";
						if (bIsSubDevice) {
							var MainDeviceIdx = $("#dialog-addlightdevice #combosubdevice option:selected").val();
							if (typeof MainDeviceIdx == 'undefined') {
								bootbox.alert($.t('No Main Device Selected!'));
								return;
							}
						}

						if (bValid) {
							$(this).dialog("close");
							$.ajax({
								url: "json.htm?type=setused&idx=" + $.devIdx + '&name=' + encodeURIComponent($("#dialog-addlightdevice #devicename").val()) + '&switchtype=' + $("#dialog-addlightdevice #comboswitchtype").val() + '&used=true&maindeviceidx=' + MainDeviceIdx,
								async: false,
								dataType: 'json',
								success: function (data) {
									ShowLights();
								}
							});

						}
					},
					Cancel: function () {
						$(this).dialog("close");
					}
				},
				close: function () {
					$(this).dialog("close");
				}
			});
			$("#dialog-addlightdevice #how_1").click(function () {
				EnableDisableSubDevices("#dialog-addlightdevice #subdevice", false);
			});
			$("#dialog-addlightdevice #how_2").click(function () {
				EnableDisableSubDevices("#dialog-addlightdevice #subdevice", true);
			});

			var dialog_addmanuallightdevice_buttons = {};
			dialog_addmanuallightdevice_buttons[$.t("Add Device")] = function () {
				var mParams = GetManualLightSettings(0);
				if (mParams == "") {
					return;
				}
				$.pDialog = $(this);
				$.ajax({
					url: "json.htm?type=command&param=addswitch" + mParams,
					async: false,
					dataType: 'json',
					success: function (data) {
						if (typeof data.status != 'undefined') {
							if (data.status == 'OK') {
								$.pDialog.dialog("close");
								ShowLights();
							}
							else {
								ShowNotify(data.message, 3000, true);
							}
						}
					}
				});
			};
			dialog_addmanuallightdevice_buttons[$.t("Cancel")] = function () {
				$(this).dialog("close");
			};

			$("#dialog-addmanuallightdevice").dialog({
				autoOpen: false,
				width: 440,
				height: 480,
				modal: true,
				resizable: false,
				title: $.t("Add Manual Light/Switch Device"),
				buttons: dialog_addmanuallightdevice_buttons,
				open: function () {
					ConfigureAddManualSettings();
					$("#dialog-addmanuallightdevice #lighttable #comboswitchtype").change(function () {
						var switchtype = $("#dialog-addmanuallightdevice #lighttable #comboswitchtype option:selected").val(),
							subtype = -1;
						if (switchtype == 1) {
							subtype = 10;	// Doorbell -> COCO GDR2
						} else if (switchtype == 18) {
							subtype = 303;	// Selector -> Selector Switch
						}
						if (subtype !== -1) {
							$("#dialog-addmanuallightdevice #lighttable #combolighttype").val(subtype);
						}
						UpdateAddManualDialog();
					});
					$("#dialog-addmanuallightdevice #lighttable #combolighttype").change(function () {
						var subtype = $("#dialog-addmanuallightdevice #lighttable #combolighttype option:selected").val(),
							switchtype = -1;
						if (subtype == 303) {
							switchtype = 18;	// Selector -> Selector Switch
						}
						if (switchtype !== -1) {
							$("#dialog-addmanuallightdevice #lighttable #comboswitchtype").val(switchtype);
						}
						UpdateAddManualDialog();
					});
					UpdateAddManualDialog();
				},
				close: function () {
					$(this).dialog("close");
				}
			});

			$("#dialog-addmanuallightdevice #howtable #how_1").click(function () {
				EnableDisableSubDevices("#dialog-addmanuallightdevice #howtable #subdevice", false);
			});
			$("#dialog-addmanuallightdevice #howtable #how_2").click(function () {
				EnableDisableSubDevices("#dialog-addmanuallightdevice #howtable #subdevice", true);
			});

			ShowLights();
		};

		$scope.$on('$destroy', function () {
			$(window).off("resize");
			var popup = $("#rgbw_popup");
			if (typeof popup != 'undefined') {
				popup.hide();
			}
			popup = $("#thermostat3_popup");
			if (typeof popup != 'undefined') {
				popup.hide();
			}
		});
	});
});
