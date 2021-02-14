define(['app', 'livesocket'], function (app) {
	app.controller('TemperatureController', function ($scope, $rootScope, $location, $http, $interval, $window, $route, $routeParams, deviceApi, permissions, livesocket) {
		var $element = $('#main-view #tempcontent').last();

		var ctrl = this;

		MakeFavorite = function (id, isfavorite) {
			deviceApi.makeFavorite(id, isfavorite).then(function() {
				ShowTemps();
			});
		};

		EditTempDevice = function (idx, name, description, addjvalue) {
			$.devIdx = idx;
			$("#dialog-edittempdevice #deviceidx").text(idx);
			$("#dialog-edittempdevice #devicename").val(unescape(name));
			$("#dialog-edittempdevice #devicedescription").val(unescape(description));
			$("#dialog-edittempdevice #adjustment").val(addjvalue);
			$("#dialog-edittempdevice #tempcf").html($scope.config.TempSign);
			$("#dialog-edittempdevice").i18n();
			$("#dialog-edittempdevice").dialog("open");
		}

		EditTempDeviceSmall = function (idx, name, description, addjvalue) {
			$.devIdx = idx;
			$("#dialog-edittempdevicesmall #deviceidx").text(idx);
			$("#dialog-edittempdevicesmall #devicename").val(unescape(name));
			$("#dialog-edittempdevicesmall #devicedescription").val(unescape(description));
			$("#dialog-edittempdevicesmall").i18n();
			$("#dialog-edittempdevicesmall").dialog("open");
		}

		//evohome
		//FIXME some of this functionality would be good in a shared js / class library
		//as we might like to use it from the dashboard or in scenes at some point
		MakePerm = function (idt) {
			$(idt).val(''); return false;
		}

		EditSetPoint = function (idx, name, description, setpoint, mode, until, callback) {
			//HeatingOff does not apply to dhw
			if (mode == "HeatingOff") {
				bootbox.alert($.t('Can\'t change zone when the heating is off'));
				return false;
			}
			$.devIdx = idx;
			$("#dialog-editsetpoint #deviceidx").text(idx);
			$("#dialog-editsetpoint #devicename").val(unescape(name));
			$("#dialog-editsetpoint #devicedescription").val(unescape(description));
			$("#dialog-editsetpoint #setpoint").val(setpoint);
			if (mode.indexOf("Override") == -1)
				$(":button:contains('Cancel Override')").attr("disabled", "d‌​isabled").addClass('ui-state-disabled');
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
		EditState = function (idx, name, description, state, mode, until, callback) {
			//HeatingOff does not apply to dhw
			$.devIdx = idx;
			$("#dialog-editstate #devicename").val(unescape(name));
			$("#dialog-editstate #devicedescription").val(unescape(description));
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
			else if (strstatus == "DayOffWithEco")//FIXME better way to convert?
				strstatus = "Day Off With Eco";
 			else if (strstatus == "HeatingOff")//FIXME better way to convert?
				strstatus = "Heating Off";
			return strstatus;
		}

		AddTempDevice = function () {
			bootbox.alert($.t('Please use the devices tab for this.'));
		}

		RefreshItem = function (item) {
			ctrl.temperatures.forEach(function (olditem, oldindex, oldarray) {
				if (olditem.idx == item.idx) {
					oldarray[oldindex] = item;
					if ($scope.config.ShowUpdatedEffect == true) {
						$("#tempwidgets #" + item.idx + " #name").effect("highlight", { color: '#EEFFEE' }, 1000);
					}
				}
			});
		}

		//We only call this once. After this the widgets are being updated automatically by used of the 'jsonupdate' broadcast event.
		RefreshTemps = function () {
			livesocket.getJson("json.htm?type=devices&filter=temp&used=true&order=[Order]&lastupdate=" + $.LastUpdateTime + "&plan=" + window.myglobals.LastPlanSelected, function (data) {
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

			$.ajax({
				url: "json.htm?type=devices&filter=temp&used=true&order=[Order]&plan=" + roomPlanId,
				async: false,
				dataType: 'json',
				success: function (data) {
					if (typeof data.result != 'undefined') {
						if (typeof data.ActTime != 'undefined') {
							$.LastUpdateTime = parseInt(data.ActTime);
						}
						ctrl.temperatures = data.result;
					} else {
						ctrl.temperatures = [];
					}
				}
			});
			$('#modal').hide();
			$('#temptophtm').show();
			$('#temptophtm').i18n();
			$('#tempwidgets').show();
			$('#tempwidgets').i18n();
			$element.html("");
			$element.i18n();

			$rootScope.RefreshTimeAndSun();
			RefreshTemps();
			return false;
		};

		$scope.DragWidget = function (idx) {
			$.devIdx = idx;
		};
		$scope.DropWidget = function (idx) {
			var myid = idx;
			var roomid = ctrl.roomSelected;
			if (typeof roomid == 'undefined') {
				roomid = 0;
			}
			$.ajax({
				url: "json.htm?type=command&param=switchdeviceorder&idx1=" + myid + "&idx2=" + $.devIdx + "&roomid=" + roomid,
				async: false,
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
					$(this).dialog("close");
					var aValue = $("#dialog-edittempdevice #edittable #adjustment").val();
					$.ajax({
						url: "json.htm?type=setused&idx=" + $.devIdx +
						'&name=' + encodeURIComponent($("#dialog-edittempdevice #devicename").val()) +
						'&description=' + encodeURIComponent($("#dialog-edittempdevice #devicedescription").val()) +
						'&addjvalue=' + aValue +
						'&used=true',
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
							url: "json.htm?type=setused&idx=" + $.devIdx +
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
						url: "json.htm?type=setused&idx=" + $.devIdx +
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
						url: "json.htm?type=setused&idx=" + $.devIdx +
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
						url: "json.htm?type=setused&idx=" + $.devIdx +
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
						url: "json.htm?type=setused&idx=" + $.devIdx +
						'&name=' + encodeURIComponent($("#dialog-edittempdevicesmall #devicename").val()) +
						'&description=' + encodeURIComponent($("#dialog-edittempdevicesmall #devicedescription").val()) +
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
							url: "json.htm?type=setused&idx=" + $.devIdx +
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

			ShowTemps();

			$("#dialog-edittempdevice").keydown(function (event) {
				if (event.keyCode == 13) {
					$(this).siblings('.ui-dialog-buttonpane').find('button:eq(0)').trigger("click");
					return false;
				}
			});
			$("#dialog-edittempdevicesmall").keydown(function (event) {
				if (event.keyCode == 13) {
					$(this).siblings('.ui-dialog-buttonpane').find('button:eq(0)').trigger("click");
					return false;
				}
			});


		};

		ctrl.RoomPlans = [{ idx: 0, name: $.t("All") }];
		$.ajax({
			url: "json.htm?type=plans",
			async: false,
			dataType: 'json',
			success: function (data) {
				if (typeof data.result != 'undefined') {
					var totalItems = data.result.length;
					if (totalItems > 0) {
						$.each(data.result, function (i, item) {
							ctrl.RoomPlans.push({
								idx: item.idx,
								name: item.Name
							});
						});
					}
				}
			}
		});

		var roomPlanId = $routeParams.room || window.myglobals.LastPlanSelected;

		if (typeof roomPlanId != 'undefined') {
			ctrl.roomSelected = roomPlanId;
		}
		ctrl.changeRoom = function () {
			var idx = ctrl.roomSelected;
			window.myglobals.LastPlanSelected = idx;

			$route.updateParams({
					room: idx > 0 ? idx : undefined
				});
				$location.replace();
				$scope.$apply();
		};

	})
		.directive('dztemperaturewidget', function ($rootScope,$location) {
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
				controller: function ($scope, $element, $attrs, permissions) {
					var ctrl = this;
					var item = $scope.item;

					ctrl.sHeatMode = function () {
						if (typeof item.Status != 'undefined') { //FIXME only support this for evohome?
							return item.Status;
						} else {
							return "";
						}
					};

					ctrl.nbackstyle = function () {
						var backgroundClass = $rootScope.GetItemBackgroundStatus(item);
						if(ctrl.displaySetPoint()){
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
						return (item.SubType == 'Zone' || item.SubType == 'Hot Water') && typeof item.SetPoint != 'undefined';
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
						return typeof item.Barometer != 'undefined';
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
						return EditTempDeviceSmall(item.idx, escape(item.Name), escape(item.Description), item.AddjValue);
					};

					ctrl.EditTempDevice = function () {
						return EditTempDevice(item.idx, escape(item.Name), escape(item.Description), item.AddjValue);
					};

					ctrl.ShowForecast = function (divId, fn) {
						$('#tempwidgets').hide(); // TODO delete when multiple views implemented
						$('#temptophtm').hide();
						return ShowForecast(atob(item.forecast_url), escape(item.Name), divId, fn);
					};

					ctrl.EditSetPoint = function (fn) {
						return EditSetPoint(item.idx, escape(item.Name), escape(item.Description), item.SetPoint, item.Status, item.Until, fn);
					};

					ctrl.EditState = function (fn) {
						return EditState(item.idx, escape(item.Name), escape(item.Description), item.State, item.Status, item.Until, fn);
					};

					$element.i18n();

					if ($scope.ordering == true) {
						if (permissions.hasPermission("Admin")) {
							if (window.myglobals.ismobileint == false) {
								$element.draggable({
									drag: function () {
										$scope.dragwidget({ idx: item.idx });
										$element.css("z-index", 2);
									},
									revert: true
								});
								$element.droppable({
									drop: function () {
										$scope.dropwidget({ idx: item.idx });
									}
								});
							}
						}
					}

				}
			};
		});
});
