define(['app', 'livesocket', 'widgets/dzLightWidget'], function (app) {
	app.controller('LightsController', function ($scope, $rootScope, $location, $http, $interval, $timeout, $route, $routeParams, deviceApi, permissions, livesocket) {
		var $element = $('#main-view #lightcontent').last();

		$scope.HasInitializedAddManualDialog = false;

		$scope.lightTypes = [
			{ value: 0, name: 'X10' },
			{ value: 1, name: 'ARC' },
			{ value: 2, name: 'AB400D' },
			{ value: 3, name: 'Waveman' },
			{ value: 4, name: 'EMW200' },
			{ value: 5, name: 'Impuls' },
			{ value: 6, name: 'RisingSun' },
			{ value: 7, name: 'Philips SBC' },
			{ value: 8, name: 'Energenie' },
			{ value: 9, name: 'Energenie 5-gang' },
			{ value: 10, name: 'COCO GDR2' },
			{ value: 11, name: 'HQ COCO-20' },
			{ value: 20, name: 'AC' },
			{ value: 21, name: 'HE EU' },
			{ value: 22, name: 'ANSLUT' },
			{ value: 50, name: 'LightwaveRF' },
			{ value: 51, name: 'EMW100 GAO/Everflourish' },
			{ value: 53, name: 'MDRemote' },
			{ value: 55, name: 'Livolo' },
			{ value: 57, name: 'Aoke' },
			{ value: 62, name: 'MDRemote 107' },
			{ value: 65, name: 'IT' },
			{ value: 66, name: 'MDRemote 108' },
			{ value: 68, name: 'GPIO' },
			{ value: 69, name: 'Sysfs GPIO' },
			{ value: 70, name: 'EnOcean' },
			{ value: 100, name: 'ByronSX' },
			{ value: 101, name: 'Harrison Curtain' },
			{ value: 102, name: 'RFY' },
			{ value: 103, name: 'Meiantech Security' },
			{ value: 104, name: 'HE105' },
			{ value: 105, name: 'ASA' },
			{ value: 106, name: 'Blyss' },
			{ value: 107, name: 'RFY-2' },
			{ value: 205, name: 'Blinds T5 Media Mount' },
			{ value: 206, name: 'Blinds T6 DC106' },
			{ value: 207, name: 'Blinds T7 Forest' },
			{ value: 208, name: 'Blinds T8 Chamberlain' },
			{ value: 209, name: 'Blinds T9 Sunpery' },
			{ value: 210, name: 'Blinds T10 Dolat DLM-1' },
			{ value: 211, name: 'Blinds T11 ASP' },
			{ value: 212, name: 'Blinds T12 Confexx' },
			{ value: 213, name: 'Blinds T13 Screenline' },
			{ value: 214, name: 'Blinds T14 Hualite' },
			{ value: 215, name: 'Blinds T15 RFU' },
			{ value: 216, name: 'Blinds T16 Zemismart' },
			{ value: 226, name: 'Eurodomest' },
			{ value: 250, name: 'EV1527' },
			{ value: 301, name: 'Smartwares Radiator' },
			{ value: 302, name: 'Home Confort TEL-010' },
			{ value: 303, name: 'Selector Switch' },
			{ value: 304, name: 'Itho CVE RFT' },
			{ value: 305, name: 'Lucci Air' },
			{ value: 306, name: 'Lucci Air DC' },
			{ value: 307, name: 'Westinghouse' },
			{ value: 308, name: 'Casafan' },
			{ value: 309, name: 'FT1211R' },
			{ value: 310, name: 'Falmec' },
			{ value: 311, name: 'Lucci Air DC II' },
			{ value: 312, name: 'Itho ECO' },
			{ value: 313, name: 'Novy' },
			{ value: 314, name: 'Orcon' },
			{ value: 315, name: 'Itho HRU400' },
			{ value: 316, name: 'Brel DDxxxx' },
			{ value: 400, name: 'Blinds OpenWebNet Bus' },
			{ value: 401, name: 'Lights OpenWebNet Bus' },
			{ value: 402, name: 'Auxiliary OpenWebNet Bus' },
			{ value: 403, name: 'Blinds OpenWebNet Zigbee' },
			{ value: 404, name: 'Lights OpenWebNet Zigbee' },
			{ value: 405, name: 'Dry Contact OpenWebNet' },
			{ value: 406, name: 'IR Detection OpenWebNet' },
			{ value: 407, name: 'Custom OpenWebNet Bus' },
		];
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
			$('#dialog-addmanuallightdevice #combolighttype').hide();
			$('#dialog-addmanuallightdevice #combolighttype2').hide();
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
			$("#dialog-addmanuallightdevice #lighttable #combohardware").change(function () {
				UpdateAddManualDialog();
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
								name: item.Name,
								config: item.config
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
			$.each([{ "name": "Normal", "data": "Auto" }, { "name": "Economy", "data": "AutoWithEco" }, { "name": "Away", "data": "Away" }, { "name": "Day Off", "data": "DayOff" }, { "name": "Custom", "data": "Custom" }, { "name": "Heating Off", "data": "HeatingOff" }], function (idx, obj) { htm += '<li><a href="#" class="ui-btn ui-btn-icon-right ui-icon-' + obj.data + '" onclick="SwitchModal(\'' + item.idx + '\',\'' + obj.name + '\',\'' + obj.data + '\');deselect($(this),\'#evopop_' + item.idx + '\');return false;">' + obj.name + '</a></li>'; });
			htm += '</ul></div>';
			return htm;
		}
		
		RefreshItem = function (item) {
			if (!$scope.devices) return;

			for (var i = 0; i < $scope.devices.length; i++) {
				if ($scope.devices[i].idx == item.idx) {
					angular.extend($scope.devices[i], item);

					if (!document.hidden) {
						if ($scope.config.ShowUpdatedEffect == true) {
							var id = "#" + item.idx;
							$(id + " #name").effect("highlight", { color: '#EEFFEE' }, 1000);
						}
					}
					RefreshLiveSearch();
					return;
				}
			}
		}


		//We only call this once. After this the widgets are being updated automatically by used of the 'jsonupdate' broadcast event.
		RefreshLights = function () {
			var roomPlanId = $routeParams.room || window.myglobals.LastPlanSelected;
			livesocket.getJson("json.htm?type=command&param=getdevices&filter=light&used=true&order=[Order]&lastupdate=" + $.LastUpdateTime + "&plan=" + roomPlanId, function (data) {
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
					if (!$scope.$$phase) {
						$scope.$apply();
					}
				}
			});
		}

		ShowLights = function () {
			$('#modal').show();

			$scope.showLightList = true;
			var roomPlanId = $routeParams.room || window.myglobals.LastPlanSelected;

			$.ajax({
				url: "json.htm?type=command&param=getdevices&filter=light&used=true&order=[Order]&plan=" + roomPlanId,
				dataType: 'json',
				success: function (data) {
					if (typeof data.result != 'undefined') {
						if (typeof data.ActTime != 'undefined') {
							$.LastUpdateTime = parseInt(data.ActTime);
						}
						$scope.devices = data.result;
						$scope.deviceRows = [];
						for (var i = 0; i < $scope.devices.length; i += 3) {
							$scope.deviceRows.push($scope.devices.slice(i, i + 3));
						}
					} else {
						$scope.devices = [];
						$scope.deviceRows = [];
					}

					$scope.loading = false;

					if (!$scope.$$phase) {
						$scope.$apply();
					}

					$('#modal').hide();

					// Set up drag/drop, i18n and live search after Angular renders
					$timeout(function() {
						if ($scope.config.AllowWidgetOrdering == true) {
							if (permissions.hasPermission('User')) {
								if (window.myglobals.ismobileint == false) {
									$element.find('.span4').draggable({
										helper: 'clone',
										opacity: 0.7,
										zIndex: 1000,
										revert: 'invalid',
										scrollSensitivity: 40,
										scrollSpeed: 20,
										drag: function () {
											$.devIdx = $(this).attr('id');
										}
									});
									$element.find('.span4').droppable({
										drop: function () {
											var myid = $(this).attr('id');
											var roomid = window.myglobals.LastPlanSelected;
											if (typeof roomid == 'undefined') {
												roomid = 0;
											}
											$.ajax({
												url: 'json.htm?type=command&param=switchdeviceorder&idx1=' + myid + '&idx2=' + $.devIdx + '&roomid=' + roomid,
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
						$element.i18n();
						RefreshLiveSearch();
					}, 100);

					$rootScope.RefreshTimeAndSun();
					RefreshLights();
				},
				error: function () {
					$('#modal').hide();
				}
			});

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
			$element.find(".dimsmall3").width(width - 72);
		}

		$.strPad = function (i, l, s) {
			var o = i.toString();
			if (!s) { s = '0'; }
			while (o.length < l) {
				o = s + o;
			}
			return o;
		};
		UpdateAddManualDialogForConfiguredHardware = function (hardware) {
			$("#dialog-addmanuallightdevice #lighttable #combolighttype2").show();
			$("#dialog-addmanuallightdevice #lighttable #combolighttype").hide();
			$("#dialog-addmanuallightdevice #lighttable #combolighttype2").unbind();
			$("#dialog-addmanuallightdevice #lighttable #combolighttype2").change(function () {
				UpdateAddManualDialogForConfiguredHardware(hardware);
			});
			$.each(hardware.config, function (i, item) {
				var option = $('<option />');
				option.attr('value', item.idx).text(item.name);
				$("#dialog-addmanuallightdevice #lighttable #combolighttype2").append(option);
			});

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
			$("#dialog-addmanuallightdevice #lighting1params").hide();
			$("#dialog-addmanuallightdevice #lighting2params").hide();
			$("#dialog-addmanuallightdevice #lighting3params").hide();

			var lighttype = $("#dialog-addmanuallightdevice #lighttable #combolighttype2 option:selected").val();
			var param = hardware.config.find(p => p.idx == lighttype);
			$("#dialog-addmanuallightdevice #confparams").show();
			$("#dialog-addmanuallightdevice #confparams").empty();
			$.each(param.parameters, function (i, item) {
				var line = $("<tr/>");
				var label = $('<td align="right" style="width:110px"><label><span data-i18n="House Code">' + item.display + '</span>:</label></td>')
				line.append(label);
				var val = $('<td/>');
				line.append(val);
				if (item.type.startsWith("housecode")) {
					var select = $('<select id="config' + item.name + '" style="width:60px" class="combobox ui-corner-all"></select>');
					val.append(select);
					var nb = parseInt(item.type.substring(9), 10);
					for (ii = 0; ii < nb; ii++) {
						select.append($('<option></option>').val(65 + ii).html(String.fromCharCode(65 + ii)));
					}
				} else if (item.type.startsWith("unitcode")) {
					var select = $('<select id="config' + item.name + '" style="width:60px" class="combobox ui-corner-all"></select>');
					val.append(select);
					var nb = parseInt(item.type.substring(8), 10);
					for (ii = 1; ii < nb + 1; ii++) {
						select.append($('<option></option>').val(ii).html(ii));
					}
				} else if (item.type == "bool") {
					var select = $('<select id="config' + item.name + '" style="width:60px" class="combobox ui-corner-all"></select>');
					val.append(select);
					select.append($('<option></option>').val("false").html("false"));
					select.append($('<option></option>').val("true").html("true"));
				}
				$("#dialog-addmanuallightdevice #confparams").append(line);
			});
		};

		UpdateAddManualDialog = function () {
			var hwdId = $("#dialog-addmanuallightdevice #lighttable #combohardware option:selected").val();
			var hardware;

			$.each($.ComboHardware, function (i, item) {
				if (item.idx == hwdId) {
					hardware = item;
				}
			});
			if (hardware != undefined && hardware.config != undefined) {
				UpdateAddManualDialogForConfiguredHardware(hardware);
				return;
			} else {
				$("#dialog-addmanuallightdevice #lighttable #combolighttype").show();
				$("#dialog-addmanuallightdevice #lighttable #combolighttype2").hide();
			}
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
			else if ((lighttype >= 304) && (lighttype <= 315)) {
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

		GetAddManualLightSettingsForConfiguredHardware = function (isTest, mParams, hardware) {
			var lighttype = $("#dialog-addmanuallightdevice #lighttable #combolighttype2 option:selected").val();
			mParams += "&lighttype=" + lighttype;
			$.each(hardware.config.find(e => e.idx == lighttype).parameters, function (i, item) {
				mParams += "&" + item.name + "=" + $("#dialog-addmanuallightdevice #config" + item.name + " option:selected").val();
			});
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

		GetManualLightSettings = function (isTest) {
			var mParams = "";
			var hwdID = $("#dialog-addmanuallightdevice #lighttable #combohardware option:selected").val();
			if (typeof hwdID == 'undefined') {
				ShowNotify($.t('No Hardware Device selected!'), 2500, true);
				return "";
			}
			var hwdId = $("#dialog-addmanuallightdevice #lighttable #combohardware option:selected").val();
			var hardware;

			$.each($.ComboHardware, function (i, item) {
				if (item.idx == hwdId) {
					hardware = item;
				}
			});

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
			if (hardware != undefined && hardware.config != undefined) {
				return GetAddManualLightSettingsForConfiguredHardware(isTest, mParams, hardware);
			}
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
			else if ((lighttype >= 304) && (lighttype <= 315)) {
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

			//Get SwitchTypes
			$.ajax({
				url: "json.htm?type=command&param=getswitchtypes",
				async: false,
				dataType: 'json',
				success: function (data) {
					if (typeof data.result != 'undefined') {
						$("#dialog-addlightdevice #comboswitchtype").html("");
						$("#dialog-addmanuallightdevice #comboswitchtype").html("");
						$.each(data.result, function (stcode, stdesc) {
							if (stdesc != null) {
								var option = $('<option />');
								option.attr('value', stcode).text(stdesc);
								$("#dialog-addlightdevice #comboswitchtype").append(option);
								$("#dialog-addmanuallightdevice #comboswitchtype").append(option);
							}
						});
					}
				}
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
								url: "json.htm?type=command&param=setused&idx=" + $.devIdx + '&name=' + encodeURIComponent($("#dialog-addlightdevice #devicename").val()) + '&switchtype=' + $("#dialog-addlightdevice #comboswitchtype").val() + '&used=true&maindeviceidx=' + MainDeviceIdx,
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



			//handles TopBar Links
			$scope.tblinks=[];
			if (permissions.hasPermission("Admin")) {
				$scope.tblinks = [
					{
						onclick:"AddLightDevice", 
						text: "Learn Light/Switch",
						i18n: "Learn Light/Switch", 
						icon: "camera"
					},
					{
						onclick:"AddManualLightDevice", 
						text: "Manual Light/Switch",
						i18n: "Manual Light/Switch", 
						icon: "plus-circle"
					}
				];
			}

			//handles RoomPlans
			var ctrl={};
			ctrl.RoomPlans=$rootScope.GetRoomPlans();	
			var roomPlanId = $routeParams.room || window.myglobals.LastPlanSelected;
	
			if (typeof roomPlanId != 'undefined') {
				ctrl.roomSelected = roomPlanId;
				window.myglobals.LastPlanSelected = roomPlanId;
			}
			ctrl.changeRoom = function () {
				var idx = ctrl.roomSelected;
				window.myglobals.LastPlanSelected = idx;
	
				$route.updateParams({
						room: idx >= 0 ? idx : undefined
					});
					$location.replace();
			};
			$scope.ctrl=ctrl;

			$scope.devices = [];
			$scope.deviceRows = [];
			$scope.showLightList = true;
			$scope.loading = true;

			$scope.refreshLights = function() {
				ShowLights();
			};

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
