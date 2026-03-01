define(['app', 'livesocket', 'widgets/dzUtilityWidget'], function (app) {
	app.controller('UtilityController', function ($scope, $rootScope, $location, $http, $interval, $timeout, $route, $routeParams, deviceApi, domoticzApi, permissions, livesocket) {
		var $element = $('#main-view #utilitycontent').last();

		$.strPad = function (i, l, s) {
			var o = i.toString();
			if (!s) { s = '0'; }
			while (o.length < l) {
				o = s + o;
			}
			return o;
		};

		MakeFavorite = function (id, isfavorite) {
			deviceApi.makeFavorite(id, isfavorite).then(function() {
				ShowUtilities();
			});
		};

		LoadCustomIcons = function () {
			$.ddData = [];
			$.ddData.push({
				text: 'Default',
				value: 0,
				selected: false,
				description: 'Default icon'
			});
			
			//Get Custom icons
			$.ajax({
				url: "json.htm?type=command&param=custom_light_icons",
				async: false,
				dataType: 'json',
				success: function (data) {
					if (typeof data.result != 'undefined') {
						var totalItems = data.result.length;
						$.each(data.result, function (i, item) {
							var bSelected = false;
							if (i == 0) {
								bSelected = true;
							}
							var itext = item.text;
							var idescription = item.description;

							var img = "images/";
							if (item.idx == 0) {
								img = "";
								itext = "Default";
								idescription = "";
							}
							else {
								img += item.imageSrc;
								img += "48_On.png";
							}
							$.ddData.push({ text: itext, value: item.idx, selected: bSelected, description: idescription, imageSrc: img });
						});
					}
				}
			});
		}

		EditUtilityDevice = function (idx, name, description, customimage) {
			$.devIdx = idx;
			$("#dialog-editutilitydevice #deviceidx").text(idx);
			$("#dialog-editutilitydevice #devicename").val(unescape(name));
			$("#dialog-editutilitydevice #devicedescription").val(unescape(description));
			$('#dialog-editutilitydevice #combosensoricon').ddslick({
				data: $.ddData,
				width: 260,
				height: 390,
				selectText: "Sensor Icon",
				imagePosition: "left"
			});
			//find our custom image index and select it
			$.each($.ddData, function (i, item) {
				if (item.value == customimage) {
					$('#dialog-editutilitydevice #combosensoricon').ddslick('select', { index: i });
				}
			});
			$("#dialog-editutilitydevice").i18n();
			$("#dialog-editutilitydevice").dialog("open");
		}

		EditTextDevice = function (idx, name, text, description, customimage) {
			$.devIdx = idx;
			$("#dialog-edittextdevice #deviceidx").text(idx);
			$("#dialog-edittextdevice #devicename").val(unescape(name));
			$("#dialog-edittextdevice #devicetext").val(unescape(text));
			$("#dialog-edittextdevice #devicedescription").val(unescape(description));
			$('#dialog-edittextdevice #combosensoricon').ddslick({
				data: $.ddData,
				width: 260,
				height: 490,
				selectText: "Sensor Icon",
				imagePosition: "left"
			});
			//find our custom image index and select it
			$.each($.ddData, function (i, item) {
				if (item.value == customimage) {
					$('#dialog-edittextdevice #combosensoricon').ddslick('select', { index: i });
				}
			});
			$("#dialog-edittextdevice").i18n();
			$("#dialog-edittextdevice").dialog("open");
		}

		EditCustomSensorDevice = function (idx, name, description, customimage, sensortype, axislabel) {
			$.devIdx = idx;
			$.sensorType = sensortype;
			$("#dialog-editcustomsensordevice #deviceidx").text(idx);
			$("#dialog-editcustomsensordevice #devicename").val(unescape(name));
			$("#dialog-editcustomsensordevice #sensoraxis").val(unescape(axislabel));

			$("#dialog-editcustomsensordevice #devicedescription").val(unescape(description));

			$('#dialog-editcustomsensordevice #combosensoricon').ddslick({
				data: $.ddData,
				width: 260,
				height: 390,
				selectText: "Sensor Icon",
				imagePosition: "left"
			});
			//find our custom image index and select it
			$.each($.ddData, function (i, item) {
				if (item.value == customimage) {
					$('#dialog-editcustomsensordevice #combosensoricon').ddslick('select', { index: i });
				}
			});

			$("#dialog-editcustomsensordevice").i18n();
			$("#dialog-editcustomsensordevice").dialog("open");
		}

		EditDistanceDevice = function (idx, name, description, switchtype, customimage) {
			$.devIdx = idx;
			$("#dialog-editdistancedevice #deviceidx").text(idx);
			$("#dialog-editdistancedevice #devicename").val(unescape(name));
			$("#dialog-editdistancedevice #devicedescription").val(unescape(description));
			$("#dialog-editdistancedevice #combometertype").val(switchtype);
			$('#dialog-editdistancedevice #combosensoricon').ddslick({
				data: $.ddData,
				width: 260,
				height: 390,
				selectText: "Sensor Icon",
				imagePosition: "left"
			});
			//find our custom image index and select it
			$.each($.ddData, function (i, item) {
				if (item.value == customimage) {
					$('#dialog-editdistancedevice #combosensoricon').ddslick('select', { index: i });
				}
			});
			$("#dialog-editdistancedevice").i18n();
			$("#dialog-editdistancedevice").dialog("open");
		}

		EditMeterDevice = function (idx, name, description, switchtype, meteroffset, meterdivider, valuequantity, valueunits, customimage) {
			$.devIdx = idx;
			$("#dialog-editmeterdevice #deviceidx").text(idx);
			$("#dialog-editmeterdevice #devicename").val(unescape(name));
			$("#dialog-editmeterdevice #devicedescription").val(unescape(description));
			$("#dialog-editmeterdevice #combometertype").val(switchtype);
			$("#dialog-editmeterdevice #meterdivider").val(meterdivider);
			$("#dialog-editmeterdevice #meteroffset").val(meteroffset);
			$("#dialog-editmeterdevice #valuequantity").val(unescape(valuequantity));
			$("#dialog-editmeterdevice #valueunits").val(unescape(valueunits));
			$("#dialog-editmeterdevice #metertable #customcounter").hide();
			if (switchtype == 3) { //Counter
				$("#dialog-editmeterdevice #metertable #customcounter").show();
			}

			$("#dialog-editmeterdevice #combometertype").change(function () {
				$("#dialog-editmeterdevice #metertable #customcounter").hide();
				var meterType = $("#dialog-editmeterdevice #combometertype").val();
				if (meterType == 3) { //Counter
					if (($("#dialog-editmeterdevice #valuequantity").val() == "")
						&& ($("#dialog-editmeterdevice #valueunits").val() == "")) {
						$("#dialog-editmeterdevice #valuequantity").val("Custom");
					}
					$("#dialog-editmeterdevice #metertable #customcounter").show();
				}
			});
			$('#dialog-editmeterdevice #combosensoricon').ddslick({
				data: $.ddData,
				width: 260,
				height: 390,
				selectText: "Sensor Icon",
				imagePosition: "left"
			});
			//find our custom image index and select it
			$.each($.ddData, function (i, item) {
				if (item.value == customimage) {
					$('#dialog-editmeterdevice #combosensoricon').ddslick('select', { index: i });
				}
			});

			$("#dialog-editmeterdevice").i18n();
			$("#dialog-editmeterdevice").dialog("open");
		}

		EditEnergyDevice = function (idx, name, description, switchtype, EnergyMeterMode, customimage) {
			$.devIdx = idx;
			$("#dialog-editenergydevice #deviceidx").text(idx);
			$("#dialog-editenergydevice #devicename").val(unescape(name));
			$("#dialog-editenergydevice #devicedescription").val(unescape(description));
			$("#dialog-editenergydevice #combometertype").val(switchtype);

			$('#dialog-editenergydevice input:radio[name=EnergyMeterMode][value="' + EnergyMeterMode + '"]').attr('checked', true);
			$('#dialog-editenergydevice input:radio[name=EnergyMeterMode][value="' + EnergyMeterMode + '"]').prop('checked', true);
			$('#dialog-editenergydevice input:radio[name=EnergyMeterMode][value="' + EnergyMeterMode + '"]').trigger('change');
			$('#dialog-editenergydevice #combosensoricon').ddslick({
				data: $.ddData,
				width: 260,
				height: 390,
				selectText: "Sensor Icon",
				imagePosition: "left"
			});
			//find our custom image index and select it
			$.each($.ddData, function (i, item) {
				if (item.value == customimage) {
					$('#dialog-editenergydevice #combosensoricon').ddslick('select', { index: i });
				}
			});
			$("#dialog-editenergydevice").i18n();
			$("#dialog-editenergydevice").dialog("open");
		}

		EditSetPoint = function (idx, name, description, unit, step, min, max, isprotected, customimage) {
			HandleProtection(isprotected, function () {
				$.devIdx = idx;
				$("#dialog-editsetpointdevice #deviceidx").text(idx);
				$("#dialog-editsetpointdevice #devicename").val(unescape(name));
				$("#dialog-editsetpointdevice #devicedescription").val(unescape(description));
				$('#dialog-editsetpointdevice #protected').prop('checked', (isprotected == true));
				$("#dialog-editsetpointdevice #unit").val(unescape(unit));
				$("#dialog-editsetpointdevice #step").val(step);
				$("#dialog-editsetpointdevice #min").val(min);
				$("#dialog-editsetpointdevice #max").val(max);
				$('#dialog-editsetpointdevice #combosensoricon').ddslick({
					data: $.ddData,
					width: 260,
					height: 390,
					selectText: "Sensor Icon",
					imagePosition: "left"
				});
				//find our custom image index and select it
				$.each($.ddData, function (i, item) {
					if (item.value == customimage) {
						$('#dialog-editsetpointdevice #combosensoricon').ddslick('select', { index: i });
					}
				});
				$("#dialog-editsetpointdevice").i18n();
				$("#dialog-editsetpointdevice").dialog("open");
			});
		}

		EditThermostatClock = function (idx, name, description, daytime, isprotected, customimage) {
			HandleProtection(isprotected, function () {
				var sarray = daytime.split(";");
				$.devIdx = idx;
				$("#dialog-editthermostatclockdevice #deviceidx").text(idx);
				$("#dialog-editthermostatclockdevice #devicename").val(unescape(name));
				$("#dialog-editthermostatclockdevice #devicedescription").val(unescape(description));
				$('#dialog-editthermostatclockdevice #protected').prop('checked', (isprotected == true));
				$("#dialog-editthermostatclockdevice #comboclockday").val(parseInt(sarray[0]));
				$("#dialog-editthermostatclockdevice #clockhour").val(sarray[1]);
				$("#dialog-editthermostatclockdevice #clockminute").val(sarray[2]);
				$('#dialog-editthermostatclockdevice #combosensoricon').ddslick({
					data: $.ddData,
					width: 260,
					height: 390,
					selectText: "Sensor Icon",
					imagePosition: "left"
				});
				//find our custom image index and select it
				$.each($.ddData, function (i, item) {
					if (item.value == customimage) {
						$('#dialog-editthermostatclockdevice #combosensoricon').ddslick('select', { index: i });
					}
				});
				$("#dialog-editthermostatclockdevice").i18n();
				$("#dialog-editthermostatclockdevice").dialog("open");
			});
		}

		EditThermostatMode = function (idx, name, description, actmode, modes, isprotected, customimage) {
			HandleProtection(isprotected, function () {
				var sarray = modes.split(";");
				$.devIdx = idx;
				$.isFan = false;
				$("#dialog-editthermostatmode #deviceidx").text(idx);
				$("#dialog-editthermostatmode #devicename").val(unescape(name));
				$("#dialog-editthermostatmode #devicedescription").val(unescape(description));
				$('#dialog-editthermostatmode #protected').prop('checked', (isprotected == true));
				//populate mode combo
				$("#dialog-editthermostatmode #combomode").html("");
				var ii = 0;
				while (ii < sarray.length - 1) {
					var option = $('<option />');
					option.attr('value', sarray[ii]).text(sarray[ii + 1]);
					$("#dialog-editthermostatmode #combomode").append(option);
					ii += 2;
				}
				$('#dialog-editthermostatmode #combosensoricon').ddslick({
					data: $.ddData,
					width: 260,
					height: 390,
					selectText: "Sensor Icon",
					imagePosition: "left"
				});
				//find our custom image index and select it
				$.each($.ddData, function (i, item) {
					if (item.value == customimage) {
						$('#dialog-editthermostatmode #combosensoricon').ddslick('select', { index: i });
					}
				});
				$("#dialog-editthermostatmode #combomode").val(parseInt(actmode));
				$("#dialog-editthermostatmode").i18n();
				$("#dialog-editthermostatmode").dialog("open");
			});
		}
		EditThermostatFanMode = function (idx, name, description, actmode, modes, isprotected, customimage) {
			HandleProtection(isprotected, function () {
				var sarray = modes.split(";");
				$.devIdx = idx;
				$.isFan = true;
				$("#dialog-editthermostatmode #deviceidx").text(idx);
				$("#dialog-editthermostatmode #devicename").val(unescape(name));
				$("#dialog-editthermostatmode #devicedescription").val(unescape(description));
				$('#dialog-editthermostatmode #protected').prop('checked', (isprotected == true));
				//populate mode combo
				$("#dialog-editthermostatmode #combomode").html("");
				var ii = 0;
				while (ii < sarray.length - 1) {
					var option = $('<option />');
					option.attr('value', sarray[ii]).text(sarray[ii + 1]);
					$("#dialog-editthermostatmode #combomode").append(option);
					ii += 2;
				}
				$('#dialog-editthermostatmode #combosensoricon').ddslick({
					data: $.ddData,
					width: 260,
					height: 390,
					selectText: "Sensor Icon",
					imagePosition: "left"
				});
				//find our custom image index and select it
				$.each($.ddData, function (i, item) {
					if (item.value == customimage) {
						$('#dialog-editthermostatmode #combosensoricon').ddslick('select', { index: i });
					}
				});
				$("#dialog-editthermostatmode #combomode").val(parseInt(actmode));
				$("#dialog-editthermostatmode").i18n();
				$("#dialog-editthermostatmode").dialog("open");
			});
		}

		AddUtilityDevice = function () {
			bootbox.alert($.t('Please use the devices tab for this.'));
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
		};

		//We only call this once. After this the widgets are being updated automatically by used of the 'jsonupdate' broadcast event.
		RefreshUtilities = function () {
			var roomPlanId = $routeParams.room || window.myglobals.LastPlanSelected;
			livesocket.getJson("json.htm?type=command&param=getdevices&filter=utility&used=true&order=[Order]&lastupdate=" + $.LastUpdateTime + "&plan=" + roomPlanId, function (data) {
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
		};

		ShowUtilities = function () {
			$('#modal').show();
			$scope.showUtilityList = true;
			var roomPlanId = $routeParams.room || window.myglobals.LastPlanSelected;

			$.ajax({
				url: 'json.htm?type=command&param=getdevices&filter=utility&used=true&order=[Order]&plan=' + roomPlanId,
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

					WatchDescriptions();

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
													ShowUtilities();
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
					RefreshUtilities();
				},
				error: function () {
					$('#modal').hide();
				}
			});

			return false;
		}

		function populatemetertypes() {
            domoticzApi.sendCommand('getmetertypes', {})
                .then(function (data) {
                    if ( data.status === 'OK' ) {
						$("#dialog-editmeterdevice #combometertype").html("");
						$.each(data.result, function (stcode, stdesc) {
							if (stdesc != null) {
								var option = $('<option />');
								option.attr('value', stcode).text(stdesc);
								$("#dialog-editmeterdevice #combometertype").append(option);
							}
						});
                    }
                });
        }

		init();

		function init() {
			$.LastUpdateTime = parseInt(0);

			$.myglobals = {
				TimerTypesStr: [],
				OccurenceStr: [],
				MonthStr: [],
				WeekdayStr: [],
				SelectedTimerIdx: 0
			};

			$scope.MakeGlobalConfig();

			$('#timerparamstable #combotype > option').each(function () {
				$.myglobals.TimerTypesStr.push($(this).text());
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

			populatemetertypes();

			$scope.$on('device_update', function (event, deviceData) {
				RefreshItem(deviceData);
			});

			var dialog_editutilitydevice_buttons = {};

			dialog_editutilitydevice_buttons[$.t("Update")] = function () {
				var bValid = true;
				bValid = bValid && checkLength($("#dialog-editutilitydevice #devicename"), 2, 100);
				if (bValid) {
					var cval = $('#dialog-editutilitydevice #combosensoricon').data('ddslick').selectedIndex;
					var CustomImage = $.ddData[cval].value;
					$(this).dialog("close");
					$.ajax({
						url: "json.htm?type=command&param=setused&idx=" + $.devIdx +
						'&name=' + encodeURIComponent($("#dialog-editutilitydevice #devicename").val()) +
						'&customimage=' + CustomImage +
						'&description=' + encodeURIComponent($("#dialog-editutilitydevice #devicedescription").val()) +
						'&used=true',
						async: false,
						dataType: 'json',
						success: function (data) {
							ShowUtilities();
						}
					});

				}
			};
			dialog_editutilitydevice_buttons[$.t("Remove Device")] = function () {
				$(this).dialog("close");
				bootbox.confirm($.t("Are you sure to remove this Device?"), function (result) {
					if (result == true) {
						$.ajax({
							url: "json.htm?type=command&param=setused&idx=" + $.devIdx +
							'&name=' + encodeURIComponent($("#dialog-editutilitydevice #devicename").val()) +
							'&description=' + encodeURIComponent($("#dialog-editutilitydevice #devicedescription").val()) +
							'&used=false',
							async: false,
							dataType: 'json',
							success: function (data) {
								ShowUtilities();
							}
						});
					}
				});
			};
			dialog_editutilitydevice_buttons[$.t("Replace")] = function () {
				$(this).dialog("close");
				ReplaceDevice($.devIdx, ShowUtilities);
			};
			dialog_editutilitydevice_buttons[$.t("Cancel")] = function () {
				$(this).dialog("close");
			};

			$("#dialog-editutilitydevice").dialog({
				autoOpen: false,
				width: 'auto',
				height: 'auto',
				modal: true,
				resizable: false,
				title: $.t("Edit Device"),
				buttons: dialog_editutilitydevice_buttons,
				close: function () {
					$(this).dialog("close");
				}
			});

			var dialog_edittextdevice_buttons = {};

			dialog_edittextdevice_buttons[$.t("Update")] = function () {
				var bValid = true;
				bValid = bValid && checkLength($("#dialog-edittextdevice #devicename"), 2, 100);
				if (bValid) {
					var cval = $('#dialog-edittextdevice #combosensoricon').data('ddslick').selectedIndex;
					var CustomImage = $.ddData[cval].value;
					$(this).dialog("close");
					$.ajax({
						url: "json.htm?type=command&param=setused&idx=" + $.devIdx +
						'&name=' + encodeURIComponent($("#dialog-edittextdevice #devicename").val()) +
						'&customimage=' + CustomImage +
						'&text=' + encodeURIComponent($("#dialog-edittextdevice #devicetext").val()) +
						'&description=' + encodeURIComponent($("#dialog-edittextdevice #devicedescription").val()) +
						'&used=true',
						async: false,
						dataType: 'json',
						success: function (data) {
							ShowUtilities();
						}
					});

				}
			};
			dialog_edittextdevice_buttons[$.t("Remove Device")] = function () {
				$(this).dialog("close");
				bootbox.confirm($.t("Are you sure to remove this Device?"), function (result) {
					if (result == true) {
						$.ajax({
							url: "json.htm?type=command&param=setused&idx=" + $.devIdx +
							'&name=' + encodeURIComponent($("#dialog-edittextdevice #devicename").val()) +
							'&description=' + encodeURIComponent($("#dialog-edittextdevice #devicedescription").val()) +
							'&used=false',
							async: false,
							dataType: 'json',
							success: function (data) {
								ShowUtilities();
							}
						});
					}
				});
			};
			dialog_edittextdevice_buttons[$.t("Replace")] = function () {
				$(this).dialog("close");
				ReplaceDevice($.devIdx, ShowUtilities);
			};
			dialog_edittextdevice_buttons[$.t("Cancel")] = function () {
				$(this).dialog("close");
			};

			$("#dialog-edittextdevice").dialog({
				autoOpen: false,
				width: 'auto',
				height: 'auto',
				modal: true,
				resizable: false,
				title: $.t("Edit Device"),
				buttons: dialog_edittextdevice_buttons,
				close: function () {
					$(this).dialog("close");
				}
			});

			var dialog_editcustomsensordevice_buttons = {};

			dialog_editcustomsensordevice_buttons[$.t("Update")] = function () {
				var bValid = true;
				bValid = bValid && checkLength($("#dialog-editcustomsensordevice #devicename"), 2, 100);
				bValid = bValid && checkLength($("#dialog-editcustomsensordevice #sensoraxis"), 1, 100);
				if (!bValid) {
					bootbox.alert($.t('Please provide a Name and Axis label!'));
					return;
				}
				$(this).dialog("close");
				var soptions = $.sensorType + ";" + encodeURIComponent($("#dialog-editcustomsensordevice #sensoraxis").val());
				var cval = $('#dialog-editcustomsensordevice #combosensoricon').data('ddslick').selectedIndex;
				var CustomImage = $.ddData[cval].value;

				$.ajax({
					url: "json.htm?type=command&param=setused&idx=" + $.devIdx +
					'&name=' + encodeURIComponent($("#dialog-editcustomsensordevice #devicename").val()) +
					'&description=' + encodeURIComponent($("#dialog-editcustomsensordevice #devicedescription").val()) +
					'&switchtype=0' +
					'&customimage=' + CustomImage +
					'&devoptions=' + encodeURIComponent(soptions) +
					'&used=true',
					async: false,
					dataType: 'json',
					success: function (data) {
						ShowUtilities();
					}
				});
			};
			dialog_editcustomsensordevice_buttons[$.t("Remove Device")] = function () {
				$(this).dialog("close");
				bootbox.confirm($.t("Are you sure to remove this Device?"), function (result) {
					if (result == true) {
						$.ajax({
							url: "json.htm?type=command&param=setused&idx=" + $.devIdx +
							'&name=' + encodeURIComponent($("#dialog-editcustomsensordevice #devicename").val()) +
							'&description=' + encodeURIComponent($("#dialog-editcustomsensordevice #devicedescription").val()) +
							'&used=false',
							async: false,
							dataType: 'json',
							success: function (data) {
								ShowUtilities();
							}
						});
					}
				});
			};
			dialog_editcustomsensordevice_buttons[$.t("Replace")] = function () {
				$(this).dialog("close");
				ReplaceDevice($.devIdx, ShowUtilities);
			};
			dialog_editcustomsensordevice_buttons[$.t("Cancel")] = function () {
				$(this).dialog("close");
			};

			$("#dialog-editcustomsensordevice").dialog({
				autoOpen: false,
				width: 'auto',
				height: 'auto',
				modal: true,
				resizable: false,
				title: $.t("Edit Device"),
				buttons: dialog_editcustomsensordevice_buttons,
				close: function () {
					$(this).dialog("close");
				}
			});

			var dialog_editdistancedevice_buttons = {};
			dialog_editdistancedevice_buttons[$.t("Update")] = function () {
				var bValid = true;
				bValid = bValid && checkLength($("#dialog-editdistancedevice #devicename"), 2, 100);
				if (bValid) {
					var cval = $('#dialog-editdistancedevice #combosensoricon').data('ddslick').selectedIndex;
					var CustomImage = $.ddData[cval].value;
					
					$(this).dialog("close");
					$.ajax({
						url: "json.htm?type=command&param=setused&idx=" + $.devIdx +
						'&name=' + encodeURIComponent($("#dialog-editdistancedevice #devicename").val()) +
						'&description=' + encodeURIComponent($("#dialog-editdistancedevice #devicedescription").val()) +
						'&switchtype=' + $("#dialog-editdistancedevice #combometertype").val() +
						'&customimage=' + CustomImage +
						'&used=true',
						async: false,
						dataType: 'json',
						success: function (data) {
							ShowUtilities();
						}
					});

				}
			};
			dialog_editdistancedevice_buttons[$.t("Remove Device")] = function () {
				$(this).dialog("close");
				bootbox.confirm($.t("Are you sure to remove this Device?"), function (result) {
					if (result == true) {
						$.ajax({
							url: "json.htm?type=command&param=setused&idx=" + $.devIdx +
							'&name=' + encodeURIComponent($("#dialog-editdistancedevice #devicename").val()) +
							'&description=' + encodeURIComponent($("#dialog-editdistancedevice #devicedescription").val()) +
							'&used=false',
							async: false,
							dataType: 'json',
							success: function (data) {
								ShowUtilities();
							}
						});
					}
				});
			};
			dialog_editdistancedevice_buttons[$.t("Cancel")] = function () {
				$(this).dialog("close");
			};

			$("#dialog-editdistancedevice").dialog({
				autoOpen: false,
				width: 'auto',
				height: 'auto',
				modal: true,
				resizable: false,
				title: $.t("Edit Device"),
				buttons: dialog_editdistancedevice_buttons,
				close: function () {
					$(this).dialog("close");
				}
			}).i18n();

			var dialog_editmeterdevice_buttons = {};
			dialog_editmeterdevice_buttons[$.t("Update")] = function () {
				var bValid = true;
				var devOptionsParam = [], devOptions = [];
				var meterType = $("#dialog-editmeterdevice #combometertype").val();
				bValid = bValid && checkLength($("#dialog-editmeterdevice #devicename"), 2, 100);
				if (bValid) {
					var meteroffset = $("#dialog-editmeterdevice #meteroffset").val();
					var meterdivider = $("#dialog-editmeterdevice #meterdivider").val();
					var cval = $('#dialog-editmeterdevice #combosensoricon').data('ddslick').selectedIndex;
					var CustomImage = $.ddData[cval].value;
					if (meterType == 3) //Counter
					{
						devOptions.push("ValueQuantity:");
						devOptions.push($("#dialog-editmeterdevice #valuequantity").val());
						devOptions.push(";");
						devOptions.push("ValueUnits:");
						devOptions.push($("#dialog-editmeterdevice #valueunits").val());
						devOptions.push(";");
						devOptionsParam.push(devOptions.join(''));
					}
					$(this).dialog("close");
					$.ajax({
						url: "json.htm?type=command&param=setused&idx=" + $.devIdx +
						'&name=' + encodeURIComponent($("#dialog-editmeterdevice #devicename").val()) +
						'&description=' + encodeURIComponent($("#dialog-editmeterdevice #devicedescription").val()) +
						'&switchtype=' + meterType +
						'&addjvalue=' + meteroffset +
						'&addjvalue2=' + meterdivider +
						'&customimage=' + CustomImage +
						'&used=true' +
						'&options=' + b64EncodeUnicode(devOptionsParam.join('')),
						async: false,
						dataType: 'json',
						success: function (data) {
							ShowUtilities();
						}
					});

				}
			};
			dialog_editmeterdevice_buttons[$.t("Remove Device")] = function () {
				$(this).dialog("close");
				bootbox.confirm($.t("Are you sure to remove this Device?"), function (result) {
					if (result == true) {
						$.ajax({
							url: "json.htm?type=command&param=setused&idx=" + $.devIdx +
							'&name=' + encodeURIComponent($("#dialog-editmeterdevice #devicename").val()) +
							'&description=' + encodeURIComponent($("#dialog-editmeterdevice #devicedescription").val()) +
							'&used=false',
							async: false,
							dataType: 'json',
							success: function (data) {
								ShowUtilities();
							}
						});
					}
				});
			};
			dialog_editmeterdevice_buttons[$.t("Replace")] = function () {
				$(this).dialog("close");
				ReplaceDevice($.devIdx, ShowUtilities);
			};
			dialog_editmeterdevice_buttons[$.t("Cancel")] = function () {
				$(this).dialog("close");
			};

			$("#dialog-editmeterdevice").dialog({
				autoOpen: false,
				width: 'auto',
				height: 'auto',
				modal: true,
				resizable: false,
				title: $.t("Edit Device"),
				buttons: dialog_editmeterdevice_buttons,
				close: function () {
					$(this).dialog("close");
				}
			});

			var dialog_editenergydevice_buttons = {};
			dialog_editenergydevice_buttons[$.t("Update")] = function () {
				var bValid = true;
				bValid = bValid && checkLength($("#dialog-editenergydevice #devicename"), 2, 100);
				if (bValid) {
					var cval = $('#dialog-editenergydevice #combosensoricon').data('ddslick').selectedIndex;
					var CustomImage = $.ddData[cval].value;
					$(this).dialog("close");
					$.ajax({
						url: "json.htm?type=command&param=setused&idx=" + $.devIdx +
						'&name=' + encodeURIComponent($("#dialog-editenergydevice #devicename").val()) +
						'&description=' + encodeURIComponent($("#dialog-editenergydevice #devicedescription").val()) +
						'&switchtype=' + $("#dialog-editenergydevice #combometertype").val() + '&EnergyMeterMode=' + $("#dialog-editenergydevice input:radio[name=EnergyMeterMode]:checked").val() +
						'&customimage=' + CustomImage +
						'&used=true',
						async: false,
						dataType: 'json',
						success: function (data) {
							ShowUtilities();
						}
					});

				}
			};
			dialog_editenergydevice_buttons[$.t("Remove Device")] = function () {
				$(this).dialog("close");
				bootbox.confirm($.t("Are you sure to remove this Device?"), function (result) {
					if (result == true) {
						$.ajax({
							url: "json.htm?type=command&param=setused&idx=" + $.devIdx +
							'&name=' + encodeURIComponent($("#dialog-editenergydevice #devicename").val()) +
							'&description=' + encodeURIComponent($("#dialog-editenergydevice #devicedescription").val()) +
							'&used=false',
							async: false,
							dataType: 'json',
							success: function (data) {
								ShowUtilities();
							}
						});
					}
				});
			};
			dialog_editenergydevice_buttons[$.t("Replace")] = function () {
				$(this).dialog("close");
				ReplaceDevice($.devIdx, ShowUtilities);
			};

			dialog_editenergydevice_buttons[$.t("Cancel")] = function () {
				$(this).dialog("close");
			};

			$("#dialog-editenergydevice").dialog({
				autoOpen: false,
				width: 'auto',
				height: 'auto',
				modal: true,
				resizable: false,
				title: $.t("Edit Device"),
				buttons: dialog_editenergydevice_buttons,
				close: function () {
					$(this).dialog("close");
				}
			});

			var dialog_editsetpointdevice_buttons = {};

			dialog_editsetpointdevice_buttons[$.t("Update")] = function () {
				var bValid = true;
				bValid = bValid && checkLength($("#dialog-editsetpointdevice #devicename"), 2, 100);
				if (bValid) {
					var cval = $('#dialog-editsetpointdevice #combosensoricon').data('ddslick').selectedIndex;
					var CustomImage = $.ddData[cval].value;
					$(this).dialog("close");
					
					var devOptions = [];
					var devOptionsParam = [];
					devOptions.push("ValueStep:");
					devOptions.push($("#dialog-editsetpointdevice #step").val());
					devOptions.push(";");
					devOptions.push("ValueMin:");
					devOptions.push($("#dialog-editsetpointdevice #min").val());
					devOptions.push(";");
					devOptions.push("ValueMax:");
					devOptions.push($("#dialog-editsetpointdevice #max").val());
					devOptions.push(";");
					devOptions.push("ValueUnit:");
					devOptions.push($("#dialog-editsetpointdevice #unit").val());
					devOptions.push(";");
					devOptionsParam.push(devOptions.join(''));
					$.ajax({
						url: "json.htm?type=command&param=setused&idx=" + $.devIdx +
						'&name=' + encodeURIComponent($("#dialog-editsetpointdevice #devicename").val()) +
						'&description=' + encodeURIComponent($("#dialog-editsetpointdevice #devicedescription").val()) +
						'&options=' + b64EncodeUnicode(devOptions.join('')) +
						'&protected=' + $('#dialog-editsetpointdevice #protected').is(":checked") +
						'&customimage=' + CustomImage +
						'&used=true',
						async: false,
						dataType: 'json',
						success: function (data) {
							ShowUtilities();
						}
					});

				}
			};
			dialog_editsetpointdevice_buttons[$.t("Remove Device")] = function () {
				$(this).dialog("close");
				bootbox.confirm($.t("Are you sure to remove this Device?"), function (result) {
					if (result == true) {
						$.ajax({
							url: "json.htm?type=command&param=setused&idx=" + $.devIdx +
							'&name=' + encodeURIComponent($("#dialog-editsetpointdevice #devicename").val()) +
							'&description=' + encodeURIComponent($("#dialog-editsetpointdevice #devicedescription").val()) +
							'&used=false',
							async: false,
							dataType: 'json',
							success: function (data) {
								ShowUtilities();
							}
						});
					}
				});
			};
			dialog_editsetpointdevice_buttons[$.t("Replace")] = function () {
				$(this).dialog("close");
				ReplaceDevice($.devIdx, ShowUtilities);
			};
			dialog_editsetpointdevice_buttons[$.t("Cancel")] = function () {
				$(this).dialog("close");
			};

			$("#dialog-editsetpointdevice").dialog({
				autoOpen: false,
				width: 'auto',
				height: 'auto',
				modal: true,
				resizable: false,
				title: $.t("Edit Device"),
				buttons: dialog_editsetpointdevice_buttons,
				close: function () {
					$(this).dialog("close");
				}
			});

			var dialog_editthermostatclockdevice_buttons = {};

			dialog_editthermostatclockdevice_buttons[$.t("Update")] = function () {
				var bValid = true;
				bValid = bValid && checkLength($("#dialog-editthermostatclockdevice #devicename"), 2, 100);
				if (bValid) {
					var cval = $('#dialog-editthermostatclockdevice #combosensoricon').data('ddslick').selectedIndex;
					var CustomImage = $.ddData[cval].value;
					$(this).dialog("close");
					bootbox.alert($.t('Setting the Clock is not finished yet!'));
					var daytimestr = $("#dialog-editthermostatclockdevice #comboclockday").val() + ";" + $("#dialog-editthermostatclockdevice #clockhour").val() + ";" + $("#dialog-editthermostatclockdevice #clockminute").val();
					$.ajax({
						url: "json.htm?type=command&param=setused&idx=" + $.devIdx +
						'&name=' + encodeURIComponent($("#dialog-editthermostatclockdevice #devicename").val()) +
						'&description=' + encodeURIComponent($("#dialog-editthermostatclockdevice #devicedescription").val()) +
						'&clock=' + encodeURIComponent(daytimestr) +
						'&protected=' + $('#dialog-editthermostatclockdevice #protected').is(":checked") +
						'&customimage=' + CustomImage +
						'&used=true',
						async: false,
						dataType: 'json',
						success: function (data) {
							ShowUtilities();
						}
					});
				}
			};
			dialog_editthermostatclockdevice_buttons[$.t("Remove Device")] = function () {
				$(this).dialog("close");
				bootbox.confirm($.t("Are you sure to remove this Device?"), function (result) {
					if (result == true) {
						$.ajax({
							url: "json.htm?type=command&param=setused&idx=" + $.devIdx +
							'&name=' + encodeURIComponent($("#dialog-editthermostatclockdevice #devicename").val()) +
							'&description=' + encodeURIComponent($("#dialog-editthermostatclockdevice #devicedescription").val()) +
							'&used=false',
							async: false,
							dataType: 'json',
							success: function (data) {
								ShowUtilities();
							}
						});
					}
				});
			};
			dialog_editthermostatclockdevice_buttons[$.t("Cancel")] = function () {
				$(this).dialog("close");
			};

			$("#dialog-editthermostatclockdevice").dialog({
				autoOpen: false,
				width: 'auto',
				height: 'auto',
				modal: true,
				resizable: false,
				title: $.t("Edit Device"),
				buttons: dialog_editthermostatclockdevice_buttons,
				close: function () {
					$(this).dialog("close");
				}
			});

			var dialog_editthermostatmode_buttons = {};

			dialog_editthermostatmode_buttons[$.t("Update")] = function () {
				var bValid = true;
				bValid = bValid && checkLength($("#dialog-editthermostatmode #devicename"), 2, 100);
				if (bValid) {
					$(this).dialog("close");
					var modestr = "";
					if ($.isFan == false) {
						modestr = "&tmode=" + $("#dialog-editthermostatmode #combomode").val();
					}
					else {
						modestr = "&fmode=" + $("#dialog-editthermostatmode #combomode").val();
					}
					$.ajax({
						url: "json.htm?type=command&param=setused&idx=" + $.devIdx +
						'&name=' + encodeURIComponent($("#dialog-editthermostatmode #devicename").val()) +
						'&description=' + encodeURIComponent($("#dialog-editthermostatmode #devicedescription").val()) +
						modestr +
						'&protected=' + $('#dialog-editthermostatmode #protected').is(":checked") +
						'&used=true',
						async: false,
						dataType: 'json',
						success: function (data) {
							ShowUtilities();
						}
					});
				}
			};
			dialog_editthermostatmode_buttons[$.t("Remove Device")] = function () {
				$(this).dialog("close");
				bootbox.confirm($.t("Are you sure to remove this Device?"), function (result) {
					if (result == true) {
						$.ajax({
							url: "json.htm?type=command&param=setused&idx=" + $.devIdx +
							'&name=' + encodeURIComponent($("#dialog-editthermostatmode #devicename").val()) +
							'&description=' + encodeURIComponent($("#dialog-editthermostatmode #devicedescription").val()) +
							'&used=false',
							async: false,
							dataType: 'json',
							success: function (data) {
								ShowUtilities();
							}
						});
					}
				});
			};
			dialog_editthermostatmode_buttons[$.t("Cancel")] = function () {
				$(this).dialog("close");
			};

			$("#dialog-editthermostatmode").dialog({
				autoOpen: false,
				width: 'auto',
				height: 'auto',
				modal: true,
				resizable: false,
				title: $.t("Edit Device"),
				buttons: dialog_editthermostatmode_buttons,
				close: function () {
					$(this).dialog("close");
				}
			});


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

			LoadCustomIcons();

			$scope.devices = [];
			$scope.deviceRows = [];
			$scope.showUtilityList = true;
			$scope.loading = true;

			$scope.refreshUtilities = function() {
				ShowUtilities();
			};

			ShowUtilities();
			//WatchLiveSearch();

			$("#dialog-editutilitydevice").keydown(function (event) {
				if (event.keyCode == 13) {
					$(this).siblings('.ui-dialog-buttonpane').find('button:eq(0)').trigger("click");
					return false;
				}
			});
			$("#dialog-edittextdevice").keydown(function (event) {
				if (event.keyCode == 13) {
					$(this).siblings('.ui-dialog-buttonpane').find('button:eq(0)').trigger("click");
					return false;
				}
			});
			$("#dialog-editcustomsensordevice").keydown(function (event) {
				if (event.keyCode == 13) {
					$(this).siblings('.ui-dialog-buttonpane').find('button:eq(0)').trigger("click");
					return false;
				}
			});
		};
		$scope.$on('$destroy', function () {
			var popup = $("#setpoint_popup");
			if (typeof popup != 'undefined') {
				popup.hide();
			}
		});
	});
});