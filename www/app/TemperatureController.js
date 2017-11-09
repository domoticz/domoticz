define(['app'], function (app) {
	app.controller('TemperatureController', ['$scope', '$rootScope', '$location', '$http', '$interval', '$window', 'permissions', function ($scope, $rootScope, $location, $http, $interval, $window, permissions) {

		var ctrl = this;

		MakeFavorite = function (id, isfavorite) {
			if (!permissions.hasPermission("Admin")) {
				HideNotify();
				ShowNotify($.t('You do not have permission to do that!'), 2500, true);
				return;
			}
			if (typeof $scope.mytimer != 'undefined') {
				$interval.cancel($scope.mytimer);
				$scope.mytimer = undefined;
			}
			$.ajax({
				url: "json.htm?type=command&param=makefavorite&idx=" + id + "&isfavorite=" + isfavorite,
				async: false,
				dataType: 'json',
				success: function (data) {
					ShowTemps();
				}
			});
		}

		EditTempDevice = function (idx, name, description, addjvalue) {
			if (typeof $scope.mytimer != 'undefined') {
				$interval.cancel($scope.mytimer);
				$scope.mytimer = undefined;
			}
			$.devIdx = idx;
			$("#dialog-edittempdevice #devicename").val(unescape(name));
			$("#dialog-edittempdevice #devicedescription").val(unescape(description));
			$("#dialog-edittempdevice #adjustment").val(addjvalue);
			$("#dialog-edittempdevice #tempcf").html($scope.config.TempSign);
			$("#dialog-edittempdevice").i18n();
			$("#dialog-edittempdevice").dialog("open");
		}

		EditTempDeviceSmall = function (idx, name, description, addjvalue) {
			if (typeof $scope.mytimer != 'undefined') {
				$interval.cancel($scope.mytimer);
				$scope.mytimer = undefined;
			}
			$.devIdx = idx;
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
			if (typeof $scope.mytimer != 'undefined') {
				$interval.cancel($scope.mytimer);
				$scope.mytimer = undefined;
			}
			$.devIdx = idx;
			$("#dialog-editsetpoint #devicename").val(unescape(name));
			$("#dialog-editsetpoint #devicedescription").val(unescape(description));
			$("#dialog-editsetpoint #setpoint").val(setpoint);
			if (mode.indexOf("Override") == -1)
				$(":button:contains('Cancel Override')").attr("disabled", "d‌​isabled").addClass('ui-state-disabled');
			else
				$(":button:contains('Cancel Override')").removeAttr("disabled").removeClass('ui-state-disabled');
			$("#dialog-editsetpoint #until").datetimepicker();
			if (until != "")
				$("#dialog-editsetpoint #until").datetimepicker("setDate", (new Date(until)));
			$("#dialog-editsetpoint").i18n();
			$("#dialog-editsetpoint").dialog("open");
		}
		EditState = function (idx, name, description, state, mode, until, callback) {
			//HeatingOff does not apply to dhw
			if (typeof $scope.mytimer != 'undefined') {
				$interval.cancel($scope.mytimer);
				$scope.mytimer = undefined;
			}
			$.devIdx = idx;
			$("#dialog-editstate #devicename").val(unescape(name));
			$("#dialog-editstate #devicedescription").val(unescape(description));
			$("#dialog-editstate #state").val(state);
			if (mode.indexOf("Override") == -1)
				$(":button:contains('Cancel Override')").attr("disabled", "d‌​isabled").addClass('ui-state-disabled');
			else
				$(":button:contains('Cancel Override')").removeAttr("disabled").removeClass('ui-state-disabled');
			$("#dialog-editstate #until_state").datetimepicker();
			if (until != "")
				$("#dialog-editstate #until_state").datetimepicker("setDate", (new Date(until)));
			$("#dialog-editstate").i18n();
			$("#dialog-editstate").dialog("open");
		}
		EvoSetPointColor = function (item, sHeatMode, bkcolor) {
			if (typeof item.SetPoint != 'undefined') {
				if (sHeatMode == "HeatingOff" || item.SetPoint == 325.1)//seems to be used whenever the heating is off
					bkcolor = "#9b9b9b";
				else if (item.SetPoint >= 25)
					bkcolor = "#ff0302";
				else if (item.SetPoint >= 22)
					bkcolor = "#ff6a2a";
				else if (item.SetPoint >= 19)
					bkcolor = "#fe9b2d";
				else if (item.SetPoint >= 16)
					bkcolor = "#79bc5c";
				else //min on temp 5 or greater
					bkcolor = "#6ca5fd";
			}
			return bkcolor;
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

		RefreshTemps = function () {
			if (typeof $scope.mytimer != 'undefined') {
				$interval.cancel($scope.mytimer);
				$scope.mytimer = undefined;
			}
			var id = "";
			$.ajax({
				url: "json.htm?type=devices&filter=temp&used=true&order=[Order]&lastupdate=" + $.LastUpdateTime + "&plan=" + window.myglobals.LastPlanSelected,
				async: false,
				dataType: 'json',
				success: function (data) {
					if (typeof data.ServerTime != 'undefined') {
						$rootScope.SetTimeAndSun(data.Sunrise, data.Sunset, data.ServerTime);
					}

					if (typeof data.result != 'undefined') {
						if (typeof data.ActTime != 'undefined') {
							$.LastUpdateTime = parseInt(data.ActTime);
						}

						// Change updated items in temperatures list
						// TODO is there a better way to do this ?
						data.result.forEach(function (newitem) {
							ctrl.temperatures.forEach(function (olditem, oldindex, oldarray) {
								if (olditem.idx == newitem.idx) {
									oldarray[oldindex] = newitem;
									if ($scope.config.ShowUpdatedEffect == true) {
										$("#tempwidgets #" + newitem.idx + " #name").effect("highlight", { color: '#EEFFEE' }, 1000);
									}
								}
							});
						});
					}
				}
			});

			$scope.mytimer = $interval(function () {
				RefreshTemps();
			}, 10000);
		}

		ShowForecast = function () {
			SwitchLayout("Forecast");
		}

		ShowTemps = function () {
			if (typeof $scope.mytimer != 'undefined') {
				$interval.cancel($scope.mytimer);
				$scope.mytimer = undefined;
			}
			$('#modal').show();

			// TODO should belong to a global controller
			ctrl.isNotMobile = function () {
				return $window.myglobals.ismobile == false;
			};

			$.ajax({
				url: "json.htm?type=devices&filter=temp&used=true&order=[Order]&plan=" + window.myglobals.LastPlanSelected,
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
			$('#tempcontent').html("");
			$('#tempcontent').i18n();

			$rootScope.RefreshTimeAndSun();

			$scope.mytimer = $interval(function () {
				RefreshTemps();
			}, 10000);
			return false;
		};

		$scope.DragWidget = function (idx) {
			if (typeof $scope.mytimer != 'undefined') {
				$interval.cancel($scope.mytimer);
				$scope.mytimer = undefined;
			}
			$.devIdx = idx;
		};
		$scope.DropWidget = function (idx) {
			var myid = idx;
			$.devIdx.split(' ');
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

		init();

		function init() {
			//global var
			$.devIdx = 0;
			$.LastUpdateTime = parseInt(0);

			$scope.MakeGlobalConfig();

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
					tUntil = selectedDate.toISOString();
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
		$scope.$on('$destroy', function () {
			if (typeof $scope.mytimer != 'undefined') {
				$interval.cancel($scope.mytimer);
				$scope.mytimer = undefined;
			}
		});

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

		if (typeof window.myglobals.LastPlanSelected != 'undefined') {
			ctrl.roomSelected = window.myglobals.LastPlanSelected;
		}
		ctrl.changeRoom = function () {
			var idx = ctrl.roomSelected;
			window.myglobals.LastPlanSelected = idx;
			ShowTemps();
		};

	}])
		.directive('dztemperaturewidget', ['$rootScope', '$location', function ($rootScope,$location) {
			return {
				priority: 0,
				restrict: 'E',
				templateUrl: 'views/temperature_widget.html',
				scope: {},
				bindToController: {
					item: '=',
					tempsign: '=',
					windsign: '=',
					ordering: '=',
					dragwidget: '&',
					dropwidget: '&'
				},
				require: 'permissions',
				controllerAs: 'ctrl',
				controller: function ($scope, $element, $attrs, permissions) {
					var ctrl = this;
					var item = ctrl.item;

					ctrl.sHeatMode = function () {
						if (typeof item.Status != 'undefined') { //FIXME only support this for evohome?
							return item.Status;
						} else {
							return "";
						}
					};

					ctrl.nbackstyle = function () {
						var backgroundClass = $rootScope.GetItemBackgroundStatus(item);
						return backgroundClass;
					};

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
							var tUntil = item.Until.replace(/Z/, '').replace(/\..+/, '') + 'Z';
							var dtUntil = new Date(tUntil);
							dtUntil = new Date(dtUntil.getTime() - dtUntil.getTimezoneOffset() * 60000);
							return dtUntil.toISOString().replace(/T/, ' ').replace(/\..+/, '');
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
						return MakeFavorite(ctrl.item.idx, n);
					};

					ctrl.EditTempDeviceSmall = function () {
						return EditTempDeviceSmall(ctrl.item.idx, escape(ctrl.item.Name), escape(ctrl.item.Description), ctrl.item.AddjValue);
					};

					ctrl.EditTempDevice = function () {
						return EditTempDevice(ctrl.item.idx, escape(ctrl.item.Name), escape(ctrl.item.Description), ctrl.item.AddjValue);
					};

					ctrl.ShowForecast = function (divId, fn) {
						$('#tempwidgets').hide(); // TODO delete when multiple views implemented
						$('#temptophtm').hide();
						return ShowForecast(atob(ctrl.item.forecast_url), escape(ctrl.item.Name), divId, fn);
					};

					ctrl.EditSetPoint = function (fn) {
						return EditSetPoint(ctrl.item.idx, escape(ctrl.item.Name), escape(ctrl.item.Description), ctrl.item.SetPoint, ctrl.item.Status, ctrl.item.Until, fn);
					};

					ctrl.EditState = function (fn) {
						return EditState(ctrl.item.idx, escape(ctrl.item.Name), escape(ctrl.item.Description), ctrl.item.State, ctrl.item.Status, ctrl.item.Until, fn);
					};

					ctrl.ShowLog = function () {
						$('#tempwidgets').hide(); // TODO delete when multiple views implemented
						$('#temptophtm').hide();
						return ShowTempLog('#tempcontent', "ShowTemps", item.idx, escape(item.Name), escape(item.Type));
					};

					ctrl.ShowNotifications = function () {
						$('#tempwidgets').hide(); // TODO delete when multiple views implemented
						$('#temptophtm').hide();
						return ShowNotifications(item.idx, escape(item.Name), '#tempcontent', 'ShowTemps');
					};

					$element.i18n();

					if (ctrl.ordering == true) {
						if (permissions.hasPermission("Admin")) {
							if (window.myglobals.ismobileint == false) {
								$element.draggable({
									drag: function () {
										ctrl.dragwidget({ idx: ctrl.item.idx });
										$element.css("z-index", 2);
									},
									revert: true
								});
								$element.droppable({
									drop: function () {
										ctrl.dropwidget({ idx: ctrl.item.idx });
									}
								});
							}
						}
					}

				}
			};
		}]);
});
