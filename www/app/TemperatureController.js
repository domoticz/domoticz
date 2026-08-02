define(['app', 'livesocket', 'widgets/dzBar'], function (app) {

	app.factory('tempEditService', ['dzBarService', '$rootScope', function (dzBarService, $rootScope) {
		function openDialog(device) {
			var idx      = device.idx;
			var name     = device.Name;
			var isHumOnly = device.Type === 'Humidity';
			var dialogId  = isHumOnly ? '#dialog-edittempdevicesmall' : '#dialog-edittempdevice';
			var sensorKey = isHumOnly ? 'hum' : 'temp';
			var canBar    = typeof device.Temp !== 'undefined' || typeof device.Humidity !== 'undefined';

			$.devIdx = idx;
			$(dialogId + ' #deviceidx').text(idx);
			$(dialogId + ' #deviceid').text(device.ID);
			$(dialogId + ' #deviceunit').text(device.Unit);
			$(dialogId + ' #devicename').val(name);
			$(dialogId + ' #devicedescription').val(device.Description);
			$(dialogId + ' #adjustment').val(device.AddjValue);

			if (!isHumOnly) {
				$(dialogId + ' #tempcf').html($rootScope.config.TempSign);
				if (device.Type === 'Thermostat 6' && typeof device.vunit !== 'undefined') {
					$(dialogId + ' #unit').val(device.vunit);
					$(dialogId + ' #step').val(device.step);
					$(dialogId + ' #min').val(device.min);
					$(dialogId + ' #max').val(device.max);
					$(dialogId + ' #setpointfields').show();
				} else {
					$(dialogId + ' #setpointfields').hide();
				}
			}

			var $form = $(dialogId + ' form');
			$form.find('.dz-bar-btn').remove();
			if (canBar) {
				dzBarService.loadForKey(device.Color || '', sensorKey);
				dzBarService.attachBarButton($form, idx, name);
			}

			$(dialogId).i18n();
			$(dialogId).dialog('open');
		}

		return { openDialog: openDialog };
	}]);

	app.controller('TemperatureController', ['$scope', '$rootScope', '$location', '$http', '$interval', '$window', '$route', '$routeParams', 'deviceApi', 'permissions', 'livesocket', 'dzBarService', function ($scope, $rootScope, $location, $http, $interval, $window, $route, $routeParams, deviceApi, permissions, livesocket, dzBarService) {
		var $element = $('#main-view #tempcontent').last();

		var ctrl = this;

		MakeFavorite = function (id, isfavorite) {
			deviceApi.makeFavorite(id, isfavorite).then(function() {
				ShowTemps();
			});
		};

		//evohome
		//FIXME some of this functionality would be good in a shared js / class library
		//as we might like to use it from the dashboard or in scenes at some point
		MakePerm = function (idt) {
			$(idt).val(''); return false;
		}

		EditSetPoint = function (idx, name, description, setpoint, mode, until, callback, deviceID, unitCode) {
			//HeatingOff does not apply to dhw
			if (mode == "HeatingOff") {
				bootbox.alert($.t('Can\'t change zone when the heating is off'));
				return false;
			}
			$.devIdx = idx;
			$("#dialog-editsetpoint #deviceidx").text(idx);
			$("#dialog-editsetpoint #deviceid").text(deviceID);
			$("#dialog-editsetpoint #deviceunit").text(unitCode);
			$("#dialog-editsetpoint #devicename").val(name);
			$("#dialog-editsetpoint #devicedescription").val(description);
			$("#dialog-editsetpoint #setpoint").val(setpoint);
			if (mode.indexOf("Override") == -1)
				$(":button:contains('Cancel Override')").attr("disabled", "disabled").addClass('ui-state-disabled');
			else
				$(":button:contains('Cancel Override')").removeAttr("disabled").removeClass('ui-state-disabled');
			$("#dialog-editsetpoint #until").datetimepicker({
				dateFormat: window.myglobals.DateFormat,
			});
			if (until != "")
				$("#dialog-editsetpoint #until").datetimepicker("setDate", (new Date(until)));
			$("#dialog-editsetpoint").i18n();
			$("#dialog-editsetpoint").dialog("open");
		}
		EditState = function (idx, name, description, state, mode, until, callback, deviceID, unitCode) {
			//HeatingOff does not apply to dhw
			$.devIdx = idx;
			$("#dialog-editstate #deviceidx").text(idx);
			$("#dialog-editstate #deviceid").text(deviceID);
			$("#dialog-editstate #deviceunit").text(unitCode);
			$("#dialog-editstate #devicename").val(name);
			$("#dialog-editstate #devicedescription").val(description);
			$("#dialog-editstate #state").val(state);
			if (mode.indexOf("Override") == -1)
				$(":button:contains('Cancel Override')").attr("disabled", "d‌​isabled").addClass('ui-state-disabled');
			else
				$(":button:contains('Cancel Override')").removeAttr("disabled").removeClass('ui-state-disabled');
			$("#dialog-editstate #until_state").datetimepicker({
				dateFormat: window.myglobals.DateFormat,
			});
			if (until != "")
				$("#dialog-editstate #until_state").datetimepicker("setDate", (new Date(until)));
			$("#dialog-editstate").i18n();
			$("#dialog-editstate").dialog("open");
		}
		//FIXME move this to a shared js ...see lightscontroller.js
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

		AddTempDevice = function () {
			bootbox.alert($.t('Please use the devices tab for this.'));
		}

		RefreshItem = function (item) {
			item.searchText = GenerateLiveSearchTextT(item);
			var query = $('.jsLiveSearch').val();
			if (query && query.length > 0) {
				var match = item.searchText.toLowerCase().indexOf(query.toLowerCase()) !== -1;
				if (!match) {
					return; // Don't update items that don't match the filter
				}
			}
			ctrl.temperatures.forEach(function (olditem, oldindex, oldarray) {
				if (olditem.idx == item.idx) {
					angular.extend(oldarray[oldindex], item);
					if (!document.hidden) {
						if ($scope.config.ShowUpdatedEffect == true) {
							// Must be delay in another way effect is finished before angular finished to draw the widget
							setTimeout(function() {
								$("#tempwidgets #" + item.idx + " #name").effect("highlight", { color: '#EEFFEE' }, 1000);
							}, 500);
						}
					}
				}
			});
			RefreshLiveSearch();
		}

		//We only call this once. After this the widgets are being updated automatically by used of the 'jsonupdate' broadcast event.
		RefreshTemps = function () {
			var roomPlanId = $routeParams.room || window.myglobals.LastPlanSelected;
			var usedFilter = roomPlanId > 0 ? 'all' : 'true';
			livesocket.getJson("json.htm?type=command&param=getdevices&filter=temp&used=" + usedFilter + "&order=[Order]&lastupdate=" + $.LastUpdateTime + "&plan=" + roomPlanId, function (data) {
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

		ShowForecast = function () {
			SwitchLayout("Forecast");
		}

		ShowTemps = function () {
			$('#modal').show();

			// TODO should belong to a global controller
			ctrl.isNotMobile = function () {
				return $window.myglobals.ismobile == false;
			};

			var roomPlanId = $routeParams.room || window.myglobals.LastPlanSelected;
			var usedFilter = roomPlanId > 0 ? 'all' : 'true';

			$.ajax({
				url: "json.htm?type=command&param=getdevices&filter=temp&used=" + usedFilter + "&order=[Order]&plan=" + roomPlanId,
				dataType: 'json',
				success: function (data) {
					if (typeof data.result != 'undefined') {
						if (typeof data.ActTime != 'undefined') {
							$.LastUpdateTime = parseInt(data.ActTime);
						}
						$.each(data.result, function (i, item) {
							item.searchText = GenerateLiveSearchTextT(item);
						});
						ctrl.temperatures = data.result;
					} else {
						ctrl.temperatures = [];
					}

					$scope.loading = false;

					if (!$scope.$$phase) {
						$scope.$apply();
					}

					$('#modal').hide();
					$('#temptophtm').show();
					$('#temptophtm').i18n();
					$('#tempwidgets').show();
					$('#tempwidgets').i18n();
					$element.html("");
					$element.i18n();

					$rootScope.RefreshTimeAndSun();
					ScheduleLiveSearchRestore();
					RefreshTemps();
				},
				error: function () {
					$('#modal').hide();
				}
			});
			return false;
		};

		$scope.DragWidget = function (idx) {
			$.devIdx = idx;
		};
		$scope.DropWidget = function (idx) {
			var myid = idx;
			var roomid = window.myglobals.LastPlanSelected;
			$.ajax({
				url: "json.htm?type=command&param=switchdeviceorder&idx1=" + myid + "&idx2=" + $.devIdx + "&roomid=" + roomid,
				dataType: 'json',
				success: function (data) {
					ShowTemps();
				}
			});
		};

		// Convert time format taking account the time zone offset. Improved version of toISOString() function.
		// Example from "Wed Apr 01 2020 17:00:00 GMT+0100 (British Summer Time)" to "2020-04-01T17:00:00.000Z"
		ConvertTimeWithTimeZoneOffset = function (tUnit) {
			var tzoffset = (new Date(tUnit)).getTimezoneOffset() * 60000; //offset in millisecondos
			var tUntilWithTimeZoneOffset = (new Date(tUnit.getTime() - tzoffset)).toISOString().slice(0, -1) + 'Z';
			return tUntilWithTimeZoneOffset
		};

		init();

		function init() {
			//global var
			$.devIdx = 0;
			$.LastUpdateTime = parseInt(0);

			$scope.MakeGlobalConfig();

			$scope.$on('device_update', function (event, deviceData) {
				RefreshItem(deviceData);
			});

			var dialog_edittempdevice_buttons = {};
			dialog_edittempdevice_buttons[$.t("Update")] = function () {
				var bValid = true;
				bValid = bValid && checkLength($("#dialog-edittempdevice #edittable #devicename"), 2, 100);
				if (bValid) {
					var aValue = $("#dialog-edittempdevice #edittable #adjustment").val();
					var url = "json.htm?type=command&param=setused&idx=" + $.devIdx +
						'&name=' + encodeURIComponent($("#dialog-edittempdevice #devicename").val()) +
						'&description=' + encodeURIComponent($("#dialog-edittempdevice #devicedescription").val()) +
						'&addjvalue=' + aValue +
						'&color=' + encodeURIComponent(dzBarService.getFullColorJson()) +
						'&used=true';

					if ($("#dialog-edittempdevice #setpointfields").is(":visible")) {
						var devOptions = [];
						devOptions.push("ValueStep:" + $("#dialog-edittempdevice #step").val() + ";");
						devOptions.push("ValueMin:" + $("#dialog-edittempdevice #min").val() + ";");
						devOptions.push("ValueMax:" + $("#dialog-edittempdevice #max").val() + ";");
						devOptions.push("ValueUnit:" + $("#dialog-edittempdevice #unit").val() + ";");
						url += '&options=' + b64EncodeUnicode(devOptions.join(''));
					}

					$(this).dialog("close");
					$.ajax({
						url: url,
						async: false,
						dataType: 'json',
						success: function (data) {
							ShowTemps();
						}
					});

				}
			};
			dialog_edittempdevice_buttons[$.t("Remove Device")] = function () {
				$(this).dialog("close");
				bootbox.confirm($.t("Are you sure to remove this Device?"), function (result) {
					if (result == true) {
						$.ajax({
							url: "json.htm?type=command&param=setused&idx=" + $.devIdx +
							'&name=' + encodeURIComponent($("#dialog-edittempdevice #devicename").val()) +
							'&description=' + encodeURIComponent($("#dialog-edittempdevice #devicedescription").val()) +
							'&used=false',
							async: false,
							dataType: 'json',
							success: function (data) {
								ShowTemps();
							}
						});
					}
				});
			};
			dialog_edittempdevice_buttons[$.t("Replace")] = function () {
				$(this).dialog("close");
				ReplaceDevice($.devIdx, ShowTemps);
			};
			dialog_edittempdevice_buttons[$.t("Cancel")] = function () {
				$(this).dialog("close");
			};
			$("#dialog-edittempdevice").dialog({
				autoOpen: false,
				width: 'auto',
				height: 'auto',
				modal: true,
				resizable: false,
				title: $.t("Edit Device"),
				buttons: dialog_edittempdevice_buttons,
				close: function () {
					$(this).dialog("close");
				}
			});

			var dialog_editsetpoint_buttons = {};
			dialog_editsetpoint_buttons[$.t("Set")] = function () {
				var bValid = true;
				bValid = bValid && checkLength($("#dialog-editsetpoint #edittable #devicename"), 2, 100);
				var setpoint = $("#dialog-editsetpoint #edittable #setpoint").val();
				if (setpoint < 5 || setpoint > 35) {
					bootbox.alert($.t('Set point must be between 5 and 35 degrees'));
					return false;
				}
				var tUntil = "";
				if ($("#dialog-editsetpoint #edittable #until").val() != "") {
					var selectedDate = $("#dialog-editsetpoint #edittable #until").datetimepicker('getDate');
					var now = new Date();
					if (selectedDate < now) {
						bootbox.alert($.t('Temporary set point date / time must be in the future'));
						return false;
					}
					tUntil = ConvertTimeWithTimeZoneOffset(selectedDate);
				}
				if (bValid) {
					$(this).dialog("close");

					$.ajax({
						url: "json.htm?type=command&param=setused&idx=" + $.devIdx +
						'&name=' + encodeURIComponent($("#dialog-editsetpoint #devicename").val()) +
						'&description=' + encodeURIComponent($("#dialog-editsetpoint #devicedescription").val()) +
						'&setpoint=' + setpoint +
						'&mode=' + ((tUntil != "") ? 'TemporaryOverride' : 'PermanentOverride') +
						'&until=' + tUntil +
						'&used=true',
						async: false,
						dataType: 'json',
						success: function (data) {
							ShowTemps();
						}
					});

				}
			};
			dialog_editsetpoint_buttons[$.t("Cancel Override")] = function () {
				var bValid = true;
				bValid = bValid && checkLength($("#dialog-editsetpoint #edittable #devicename"), 2, 100);
				if (bValid) {
					$(this).dialog("close");
					var aValue = $("#dialog-editsetpoint #edittable #setpoint").val();
					if (aValue < 5) aValue = 5;//These values will display but the controller will update back the currently scheduled setpoint in due course
					if (aValue > 35) aValue = 35;//These values will display but the controller will update back the currently scheduled setpoint in due course
					$.ajax({
						url: "json.htm?type=command&param=setused&idx=" + $.devIdx +
						'&name=' + encodeURIComponent($("#dialog-editsetpoint #devicename").val()) +
						'&description=' + encodeURIComponent($("#dialog-editsetpoint #devicedescription").val()) +
						'&setpoint=' + aValue +
						'&mode=Auto&used=true',
						async: false,
						dataType: 'json',
						success: function (data) {
							ShowTemps();
						}
					});

				}
			};
			dialog_editsetpoint_buttons[$.t("Cancel")] = function () {
				$(this).dialog("close");
				ShowTemps();//going into the dialog removes the background timer refresh (see EditSetPoint)
			};

			$("#dialog-editsetpoint").dialog({
				autoOpen: false,
				width: 'auto',
				height: 'auto',
				modal: true,
				resizable: false,
				title: $.t("Edit Set Point"),
				buttons: dialog_editsetpoint_buttons,
				close: function () {
					$(this).dialog("close");
					ShowTemps();//going into the dialog removes the background timer refresh (see EditSetPoint)
				}
			});

			var dialog_editstate_buttons = {};

			dialog_editstate_buttons[$.t("Set")] = function () {
				var bValid = true;
				bValid = bValid && checkLength($("#dialog-editstate #edittable #devicename"), 2, 100);
				if (bValid) {
					$(this).dialog("close");
					var aValue = $("#dialog-editstate #edittable #state").val();
					var tUntil = "";
					if ($("#dialog-editstate #edittable #until_state").val() != "")
						tUntil = $("#dialog-editstate #edittable #until_state").datetimepicker('getDate').toISOString();
					$.ajax({
						url: "json.htm?type=command&param=setused&idx=" + $.devIdx +
						'&name=' + encodeURIComponent($("#dialog-editstate #devicename").val()) +
						'&description=' + encodeURIComponent($("#dialog-editstate #devicedescription").val()) +
						'&state=' + aValue +
						'&mode=' + ((tUntil != "") ? 'TemporaryOverride' : 'PermanentOverride') +
						'&until=' + tUntil +
						'&used=true',
						async: false,
						dataType: 'json',
						success: function (data) {
							ShowTemps();
						}
					});

				}
			};
			dialog_editstate_buttons[$.t("Cancel")] = function () {
				$(this).dialog("close");
				ShowTemps();//going into the dialog removes the background timer refresh (see EditSetPoint)
			};

			$("#dialog-editstate").dialog({
				autoOpen: false,
				width: 'auto',
				height: 'auto',
				modal: true,
				resizable: false,
				title: $.t("Edit State"),
				buttons: dialog_editstate_buttons,
				close: function () {
					$(this).dialog("close");
					ShowTemps();//going into the dialog removes the background timer refresh (see EditState)
				}
			});

			var dialog_edittempdevicesmall_buttons = {};
			dialog_edittempdevicesmall_buttons[$.t("Update")] = function () {
				var bValid = true;
				bValid = bValid && checkLength($("#dialog-edittempdevicesmall #edittable #devicename"), 2, 100);
				if (bValid) {
					$(this).dialog("close");
					$.ajax({
						url: "json.htm?type=command&param=setused&idx=" + $.devIdx +
						'&name=' + encodeURIComponent($("#dialog-edittempdevicesmall #devicename").val()) +
						'&description=' + encodeURIComponent($("#dialog-edittempdevicesmall #devicedescription").val()) +
						'&color=' + encodeURIComponent(dzBarService.getFullColorJson()) +
						'&used=true',
						async: false,
						dataType: 'json',
						success: function (data) {
							ShowTemps();
						}
					});

				}
			};
			dialog_edittempdevicesmall_buttons[$.t("Remove Device")] = function () {
				$(this).dialog("close");
				bootbox.confirm($.t("Are you sure to remove this Device?"), function (result) {
					if (result == true) {
						$.ajax({
							url: "json.htm?type=command&param=setused&idx=" + $.devIdx +
							'&name=' + encodeURIComponent($("#dialog-edittempdevicesmall #devicename").val()) +
							'&description=' + encodeURIComponent($("#dialog-edittempdevicesmall #devicedescription").val()) +
							'&used=false',
							async: false,
							dataType: 'json',
							success: function (data) {
								ShowTemps();
							}
						});
					}
				});
			};
			dialog_edittempdevicesmall_buttons[$.t("Replace")] = function () {
				$(this).dialog("close");
				ReplaceDevice($.devIdx, ShowTemps);
			};
			dialog_edittempdevicesmall_buttons[$.t("Cancel")] = function () {
				$(this).dialog("close");
			};
			$("#dialog-edittempdevicesmall").dialog({
				autoOpen: false,
				width: 'auto',
				height: 'auto',
				modal: true,
				resizable: false,
				title: $.t("Edit Device"),
				buttons: dialog_edittempdevicesmall_buttons,
				close: function () {
					$(this).dialog("close");
				}
			});

			ctrl.temperatures = [];
			$scope.loading = true;
			ShowTemps();
			////WatchLiveSearch();

		};

		//handles TopBar Links
		$scope.tblinks = [
			{
				href:"#/Temperature/CustomTempLog", 
				text:"Custom Graph", 
				i18n: "Custom Graph", 
				icon: "area-chart"
			},
			{
				onclick:"ShowForecast", 
				text:"Forecast", 
				i18n: "Forecast", 
				icon: "cloud-sun-rain"
			}
		];

		//handles RoomPlans
		ctrl.RoomPlans=$rootScope.GetRoomPlans();	
		var roomPlanId = $routeParams.room || window.myglobals.LastPlanSelected;

		if (roomPlanId != null) {
			ctrl.roomSelected = roomPlanId;
			window.myglobals.LastPlanSelected = roomPlanId;
		}
		ctrl.changeRoom = function () {
			var idx = ctrl.roomSelected;
			if (idx == null) { return; }
			window.myglobals.LastPlanSelected = idx;
			window.myglobals.LastSearchFilter = '';
			$('.jsLiveSearch').val('').trigger('change');

			$route.updateParams({
					room: idx >= 0 ? idx : undefined
				});
				$location.replace();
		};

	}])
		.directive('dztemperaturewidget', ['$rootScope', '$location', function ($rootScope,$location) {
			return {
				priority: 0,
				restrict: 'E',
				scope: {
					item: '=',
					tempsign: '=',
					windsign: '=',
					ordering: '=',
					dragwidget: '&',
					dropwidget: '&'
				},
				templateUrl: 'views/temperature_widget.html',
				require: 'permissions',
				controllerAs: 'ctrl',
				controller: ['$scope', '$element', '$attrs', 'permissions', 'tempEditService', function ($scope, $element, $attrs, permissions, tempEditService) {
					var ctrl = this;
					var item = $scope.item;

					$scope.$watch('item', function (newVal) {
						if (newVal) item = newVal;
					});

					ctrl.sHeatMode = function () {
						if (typeof item.Status != 'undefined') { //FIXME only support this for evohome?
							return item.Status;
						} else {
							return "";
						}
					};

					ctrl.nbackstyle = function () {
						var backgroundClass = $rootScope.GetItemBackgroundStatus(item);
						var evoSubTypes = ['Zone', 'Hot Water'];
						if (evoSubTypes.indexOf(item.SubType) !== -1 && ctrl.displaySetPoint()){
							if (ctrl.sHeatMode() == "HeatingOff" || !ctrl.isSetPointOn())//seems to be used whenever the heating is off
                                        			backgroundClass="statusEvoSetPointOff";
                                			else if (item.SetPoint >= 25)
                                        			backgroundClass="statusEvoSetPoint25";
                                			else if (item.SetPoint >= 22)
                                        			backgroundClass="statusEvoSetPoint22";
                                			else if (item.SetPoint >= 19)
                                        			backgroundClass="statusEvoSetPoint19";
                                			else if (item.SetPoint >= 16)
                                        			backgroundClass="statusEvoSetPoint16";
                                			else //min on temp 5 or greater
                                        			backgroundClass="statusEvoSetPointMin";
						}
						return backgroundClass;
					};

					ctrl.displayTrend = $rootScope.DisplayTrend;
					ctrl.trendState  = $rootScope.TrendState;

					// TODO use angular isDefined
					ctrl.displayTemp = function () {
						return typeof item.Temp != 'undefined';
					};
					ctrl.displaySetPoint = function () {
					return (item.SubType == 'Zone' || item.SubType == 'Hot Water' || item.SubType == 'Temp/Setpoint' || item.SubType == 'Temp/Hum/Setpoint' || item.SubType == 'Temp/Baro/Setpoint' || item.SubType == 'Temp/Hum/Baro/Setpoint') && typeof item.SetPoint != 'undefined';
					};
					ctrl.getSetpointUnit = function () {
						return item.vunit || ('°' + $scope.tempsign);
					};
					ctrl.isSetPointOn = function () {
						return item.SetPoint != 325.1;
					};
					ctrl.displayState = function () {
						return (item.SubType == 'Zone' || item.SubType == 'Hot Water') && typeof item.State != 'undefined';
					};
					ctrl.displayHeat = function () {
						return (item.SubType == 'Zone' || item.SubType == 'Hot Water') && ctrl.sHeatMode() != 'Auto' && ctrl.sHeatMode() != 'FollowSchedule';
					};
					ctrl.imgHeat = function () {
						if (ctrl.displayHeat()) {
							return ctrl.sHeatMode() + ((item.SubType == 'Hot Water') ? 'Inv' : '');
						} else {
							return undefined;
						}
					};
					ctrl.displayHumidity = function () {
						return typeof item.Humidity != 'undefined';
					};
					ctrl.displayChill = function () {
						return typeof item.Chill != 'undefined';
					};
					
					ctrl.image = function () {
						if (typeof item.Temp != 'undefined') {
							return GetTemp48Item(item.Temp);
						}
						else {
							if (item.Type == "Humidity") {
								return "gauge48.png";
							}
							else {
								return GetTemp48Item(item.Chill);
							}
						}
					};

					ctrl.displayMode = function () {
						return (item.SubType == "Zone" || item.SubType == "Hot Water");
					};
					ctrl.EvoDisplayTextMode = function () {
						return EvoDisplayTextMode(ctrl.sHeatMode());
					};
					ctrl.displayUntil = function () {
						return (item.SubType == "Zone" || item.SubType == "Hot Water") && typeof item.Until != 'undefined';
					};
					ctrl.dtUntil = function () {
						if (angular.isDefined(item.Until)) {
							//var tUntil = item.Until.replace(/Z/, '').replace(/\..+/, '') + 'Z';
							//console.log(tUntil + ' 2');
							//var dtUntil = new Date(tUntil);
							//dtUntil = new Date(dtUntil.getTime() - dtUntil.getTimezoneOffset() * 60000);
							//var unitReturn_vaule = item.Until.replace(/T/, ' ').replace(/\..+/, '');
							//console.log(unitReturn_vaule + ' 4');
							//return unitReturn_vaule;
							return item.Until.replace(/T/, ' ').replace(/\..+/, '');
						}
					};
					ctrl.displayHumidityStatus = function () {
						return typeof item.HumidityStatus != 'undefined';
					};
					ctrl.HumidityStatus = function () {
						return $.t(item.HumidityStatus);
					};
					ctrl.displayBarometer = function () {
						return typeof item.Barometer != 'undefined' && item.Type === 'Thermostat 6';
					};
					ctrl.displayForecast = function () {
						return typeof item.ForecastStr != 'undefined';
					};
					ctrl.ForecastStr = function () {
						return $.t(item.ForecastStr);
					};
					ctrl.displayDirection = function () {
						return typeof item.Direction != 'undefined';
					};
					ctrl.displayGust = function () {
						return ctrl.displayDirection() && typeof item.Gust != 'undefined';
					};
					ctrl.displayDewPoint = function () {
						return typeof item.DewPoint != 'undefined';
					};


					ctrl.MakeFavorite = function (n) {
						return MakeFavorite(item.idx, n);
					};

					ctrl.EditTempDeviceSmall = function () {
						tempEditService.openDialog(item);
					};

					ctrl.EditTempDevice = function () {
						tempEditService.openDialog(item);
					};

					ctrl.ShowForecast = function (divId, fn) {
						$('#tempwidgets').hide(); // TODO delete when multiple views implemented
						$('#temptophtm').hide();
						return ShowForecast(atob(item.forecast_url), item.Name, divId, fn);
					};

					ctrl.EditSetPoint = function (fn) {
						return EditSetPoint(item.idx, item.Name, item.Description, item.SetPoint, item.Status, item.Until, fn, item.ID, item.Unit);
					};
					
					ctrl.ShowSetpointPopup = function (event) {
						var step = item.step || 0.5;
						var min = item.min || -200;
						var max = item.max || 200;
						ShowSetpointPopup(event, item.idx, item.Protected, item.SetPoint, false, step, min, max);
					};

					ctrl.EditState = function (fn) {
						return EditState(item.idx, item.Name, item.Description, item.State, item.Status, item.Until, fn, item.ID, item.Unit);
					};

					$element.i18n();
					//WatchLiveSearch();
					WatchDescriptions();

					if ($scope.ordering == true) {
						if (permissions.hasPermission("User")) {
							if (window.myglobals.ismobileint == false) {
								$element.draggable({
									helper: 'clone',
									opacity: 0.7,
									zIndex: 1000,
									revert: 'invalid',
									scrollSensitivity: 40,
									scrollSpeed: 20,
									drag: function () {
										$scope.dragwidget({ idx: item.idx });
									}
								});
								$element.droppable({
									drop: function () {
										$scope.dropwidget({ idx: item.idx });
									}
								});
							}
						}
					}

				}]
			};
		}]);
});
