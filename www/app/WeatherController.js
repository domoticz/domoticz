define(['app', 'livesocket'], function (app) {
	app.controller('WeatherController', function ($scope, $rootScope, $location, $http, $interval, deviceApi, permissions, livesocket) {

		var ctrl = this;

		MakeFavorite = function (id, isfavorite) {
			deviceApi.makeFavorite(id, isfavorite).then(function() {
				ShowWeathers();
			});
		};

		EditRainDevice = function (idx, name, description, addjmulti) {
			$.devIdx = idx;
			$("#dialog-editraindevice #deviceidx").text(idx);
			$("#dialog-editraindevice #devicename").val(unescape(name));
			$("#dialog-editraindevice #devicedescription").val(unescape(description));
			$("#dialog-editraindevice #multiply").val(addjmulti);
			$("#dialog-editraindevice").i18n();
			$("#dialog-editraindevice").dialog("open");
		}

		EditBaroDevice = function (idx, name, description, addjvalue) {
			$.devIdx = idx;
			$("#dialog-editbarodevice #deviceidx").text(idx);
			$("#dialog-editbarodevice #devicename").val(unescape(name));
			$("#dialog-editbarodevice #devicedescription").val(unescape(description));
			$("#dialog-editbarodevice #adjustment").val(addjvalue);
			$("#dialog-editbarodevice").i18n();
			$("#dialog-editbarodevice").dialog("open");
		}

		EditVisibilityDevice = function (idx, name, description, switchtype) {
			$.devIdx = idx;
			$("#dialog-editvisibilitydevice #deviceidx").text(idx);
			$("#dialog-editvisibilitydevice #devicename").val(unescape(name));
			$("#dialog-editvisibilitydevice #devicedescription").val(unescape(description));
			$("#dialog-editvisibilitydevice #combometertype").val(switchtype);
			$("#dialog-editvisibilitydevice").i18n();
			$("#dialog-editvisibilitydevice").dialog("open");
		}

		EditWeatherDevice = function (idx, name, description, addjmulti) {
			$.devIdx = idx;
			$("#dialog-editweatherdevice #deviceidx").text(idx);
			$("#dialog-editweatherdevice #devicename").val(unescape(name));
			$("#dialog-editweatherdevice #devicedescription").val(unescape(description));
			$("#dialog-editweatherdevice #multiply").val(addjmulti);
			$("#dialog-editweatherdevice").i18n();
			$("#dialog-editweatherdevice").dialog("open");
		}

		AddWeatherDevice = function () {
			bootbox.alert($.t('Please use the devices tab for this.'));
		}

		RefreshItem = function (item) {
			ctrl.items.forEach(function (olditem, oldindex, oldarray) {
				if (olditem.idx == item.idx) {
					oldarray[oldindex] = item;
					if ($scope.config.ShowUpdatedEffect == true) {
						$("#weatherwidgets #" + item.idx + " #name").effect("highlight", { color: '#EEFFEE' }, 1000);
					}
				}
			});
		}

		//We only call this once. After this the widgets are being updated automatically by used of the 'jsonupdate' broadcast event.
		RefreshWeathers = function () {
			var id = "";

			livesocket.getJson("json.htm?type=devices&filter=weather&used=true&order=[Order]&lastupdate=" + $.LastUpdateTime + "&plan=" + window.myglobals.LastPlanSelected, function (data) {
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

		ShowWeathers = function () {
			$('#modal').show();

			$.ajax({
				url: "json.htm?type=devices&filter=weather&used=true&order=[Order]",
				async: false,
				dataType: 'json',
				success: function (data) {
					if (typeof data.result != 'undefined') {
						if (typeof data.ActTime != 'undefined') {
							$.LastUpdateTime = parseInt(data.ActTime);
						}
						ctrl.items = data.result;
					} else {
						ctrl.items = [];
					}
				}
			});
			$('#modal').hide();
			$('#weathercontent').html("");
			$('#weathercontent').i18n();
			$('#weatherwidgets').show();
			$('#weatherwidgets').i18n();
			$('#weathertophtm').show();
			$('#weathertophtm').i18n();

			$rootScope.RefreshTimeAndSun();
			RefreshWeathers();
			return false;
		}

		$scope.DragWidget = function (idx) {
			$.devIdx = idx;
		};
		$scope.DropWidget = function (idx) {
			var myid = idx;
			$.ajax({
				url: "json.htm?type=command&param=switchdeviceorder&idx1=" + myid + "&idx2=" + $.devIdx,
				async: false,
				dataType: 'json',
				success: function (data) {
					ShowWeathers();
				}
			});
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

			var dialog_editweatherdevice_buttons = {};
			dialog_editweatherdevice_buttons[$.t("Update")] = function () {
				var bValid = true;
				bValid = bValid && checkLength($("#dialog-editweatherdevice #devicename"), 2, 100);
				if (bValid) {
					$(this).dialog("close");
					$.ajax({
						url: "json.htm?type=setused&idx=" + $.devIdx +
						'&name=' + encodeURIComponent($("#dialog-editweatherdevice #devicename").val()) +
						'&description=' + encodeURIComponent($("#dialog-editweatherdevice #devicedescription").val()) +
						'&addjmulti=' + $("#dialog-editweatherdevice #edittable #multiply").val() +
						'&used=true',
						async: false,
						dataType: 'json',
						success: function (data) {
							ShowWeathers();
						}
					});

				}
			};
			dialog_editweatherdevice_buttons[$.t("Remove Device")] = function () {
				$(this).dialog("close");
				bootbox.confirm($.t("Are you sure to remove this Device?"), function (result) {
					if (result == true) {
						$.ajax({
							url: "json.htm?type=setused&idx=" + $.devIdx +
							'&name=' + encodeURIComponent($("#dialog-editweatherdevice #devicename").val()) +
							'&description=' + encodeURIComponent($("#dialog-editweatherdevice #devicedescription").val()) +
							'&used=false',
							async: false,
							dataType: 'json',
							success: function (data) {
								ShowWeathers();
							}
						});
					}
				});
			};
			dialog_editweatherdevice_buttons[$.t("Replace")] = function () {
				$(this).dialog("close");
				ReplaceDevice($.devIdx, ShowWeathers);
			};
			dialog_editweatherdevice_buttons[$.t("Cancel")] = function () {
				$(this).dialog("close");
			};

			$("#dialog-editweatherdevice").dialog({
				autoOpen: false,
				width: 'auto',
				height: 'auto',
				modal: true,
				resizable: false,
				title: $.t("Edit Device"),
				buttons: dialog_editweatherdevice_buttons,
				close: function () {
					$(this).dialog("close");
				}
			}).i18n();

			var dialog_editraindevice_buttons = {};

			dialog_editraindevice_buttons[$.t("Update")] = function () {
				var bValid = true;
				bValid = bValid && checkLength($("#dialog-editraindevice #edittable #devicename"), 2, 100);
				if (bValid) {
					$(this).dialog("close");
					$.ajax({
						url: "json.htm?type=setused&idx=" + $.devIdx +
						'&name=' + encodeURIComponent($("#dialog-editraindevice #devicename").val()) +
						'&description=' + encodeURIComponent($("#dialog-editraindevice #devicedescription").val()) +
						'&addjmulti=' + $("#dialog-editraindevice #edittable #multiply").val() +
						'&used=true',
						async: false,
						dataType: 'json',
						success: function (data) {
							ShowWeathers();
						}
					});

				}
			};
			dialog_editraindevice_buttons[$.t("Remove Device")] = function () {
				$(this).dialog("close");
				bootbox.confirm($.t("Are you sure to remove this Device?"), function (result) {
					if (result == true) {
						$.ajax({
							url: "json.htm?type=setused&idx=" + $.devIdx +
							'&name=' + encodeURIComponent($("#dialog-editweatherdevice #devicename").val()) +
							'&description=' + encodeURIComponent($("#dialog-editweatherdevice #devicedescription").val()) +
							'&used=false',
							async: false,
							dataType: 'json',
							success: function (data) {
								ShowWeathers();
							}
						});
					}
				});
			};
			dialog_editraindevice_buttons[$.t("Replace")] = function () {
				$(this).dialog("close");
				ReplaceDevice($.devIdx, ShowWeathers);
			};
			dialog_editraindevice_buttons[$.t("Cancel")] = function () {
				$(this).dialog("close");
			};

			$("#dialog-editraindevice").dialog({
				autoOpen: false,
				width: 'auto',
				height: 'auto',
				modal: true,
				resizable: false,
				title: $.t("Edit Device"),
				buttons: dialog_editraindevice_buttons,
				close: function () {
					$(this).dialog("close");
				}
			}).i18n();

			var dialog_editbarodevice_buttons = {};

			dialog_editbarodevice_buttons[$.t("Update")] = function () {
				var bValid = true;
				bValid = bValid && checkLength($("#dialog-editbarodevice #edittable #devicename"), 2, 100);
				if (bValid) {
					$(this).dialog("close");
					var aValue = $("#dialog-editbarodevice #edittable #adjustment").val();
					$.ajax({
						url: "json.htm?type=setused&idx=" + $.devIdx +
						'&name=' + encodeURIComponent($("#dialog-editbarodevice #devicename").val()) +
						'&description=' + encodeURIComponent($("#dialog-editbarodevice #devicedescription").val()) +
						'&addjvalue2=' + aValue +
						'&used=true',
						async: false,
						dataType: 'json',
						success: function (data) {
							ShowWeathers();
						}
					});

				}
			};
			dialog_editbarodevice_buttons[$.t("Remove Device")] = function () {
				$(this).dialog("close");
				bootbox.confirm($.t("Are you sure to remove this Device?"), function (result) {
					if (result == true) {
						$.ajax({
							url: "json.htm?type=setused&idx=" + $.devIdx +
							'&name=' + encodeURIComponent($("#dialog-editbarodevice #devicename").val()) +
							'&description=' + encodeURIComponent($("#dialog-editbarodevice #devicedescription").val()) +
							'&used=false',
							async: false,
							dataType: 'json',
							success: function (data) {
								ShowWeathers();
							}
						});
					}
				});
			};
			dialog_editbarodevice_buttons[$.t("Replace")] = function () {
				$(this).dialog("close");
				ReplaceDevice($.devIdx, ShowWeathers);
			};
			dialog_editbarodevice_buttons[$.t("Cancel")] = function () {
				$(this).dialog("close");
			};
			$("#dialog-editbarodevice").dialog({
				autoOpen: false,
				width: 'auto',
				height: 'auto',
				modal: true,
				resizable: false,
				title: $.t("Edit Device"),
				buttons: dialog_editbarodevice_buttons,
				close: function () {
					$(this).dialog("close");
				}
			}).i18n();

			var dialog_editvisibilitydevice_buttons = {};
			dialog_editvisibilitydevice_buttons[$.t("Update")] = function () {
				var bValid = true;
				bValid = bValid && checkLength($("#dialog-editvisibilitydevice #devicename"), 2, 100);
				if (bValid) {
					$(this).dialog("close");
					$.ajax({
						url: "json.htm?type=setused&idx=" + $.devIdx +
						'&name=' + encodeURIComponent($("#dialog-editvisibilitydevice #devicename").val()) +
						'&description=' + encodeURIComponent($("#dialog-editvisibilitydevice #devicedescription").val()) +
						'&switchtype=' + $("#dialog-editvisibilitydevice #combometertype").val() +
						'&used=true',
						async: false,
						dataType: 'json',
						success: function (data) {
							ShowWeathers();
						}
					});

				}
			};
			dialog_editvisibilitydevice_buttons[$.t("Remove Device")] = function () {
				$(this).dialog("close");
				bootbox.confirm($.t("Are you sure to remove this Device?"), function (result) {
					if (result == true) {
						$.ajax({
							url: "json.htm?type=setused&idx=" + $.devIdx +
							'&name=' + encodeURIComponent($("#dialog-editvisibilitydevice #devicename").val()) +
							'&description=' + encodeURIComponent($("#dialog-editvisibilitydevice #devicedescription").val()) +
							'&used=false',
							async: false,
							dataType: 'json',
							success: function (data) {
								ShowWeathers();
							}
						});
					}
				});
			};
			dialog_editvisibilitydevice_buttons[$.t("Cancel")] = function () {
				$(this).dialog("close");
			};

			$("#dialog-editvisibilitydevice").dialog({
				autoOpen: false,
				width: 'auto',
				height: 'auto',
				modal: true,
				resizable: false,
				title: $.t("Edit Device"),
				buttons: dialog_editvisibilitydevice_buttons,
				close: function () {
					$(this).dialog("close");
				}
			}).i18n();

			ShowWeathers();
			$("dialog-editweatherdevice").keydown(function (event) {
				if (event.keyCode == 13) {
					$(this).siblings('.ui-dialog-buttonpane').find('button:eq(0)').trigger("click");
					return false;
				}
			});

		};
	}).directive('dzweatherwidget', ['$rootScope', '$location', function ($rootScope,$location) {
		return {
			priority: 0,
			restrict: 'E',
			templateUrl: 'views/weather_widget.html',
			scope: {
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
				var item = $scope.item;

				ctrl.nbackstyle = function () {
					var backgroundClass = $rootScope.GetItemBackgroundStatus(item);
					return backgroundClass;
				};

				ctrl.displayBarometer = function () {
					return typeof item.Barometer != 'undefined';
				};
				ctrl.displayForecast = function () {
					return ctrl.displayBarometer() && typeof item.ForecastStr != 'undefined';
				};
				ctrl.displayAltitude = function () {
					return ctrl.displayBarometer() && typeof item.Altitude != 'undefined';
				};
				ctrl.displayRain = function () {
					return typeof item.Rain != 'undefined';
				};
				ctrl.displayRainRate = function () {
					return ctrl.displayRain() && typeof item.RainRate != 'undefined';
				};
				ctrl.displayVisibility = function () {
					return typeof item.Visibility != 'undefined';
				};
				ctrl.displayData = function () {
					return ctrl.displayVisibility() || ctrl.displayRadiation();
				};
				ctrl.displayUVI = function () {
					return typeof item.UVI != 'undefined';
				};
				ctrl.displayRadiation = function () {
					return typeof item.Radiation != 'undefined';
				};
				ctrl.displayTemp = function () {
					return ctrl.displayUVI() && typeof item.Temp != 'undefined';
				};
				ctrl.displayDirection = function () {
					return typeof item.Direction != 'undefined';
				};
				ctrl.displaySpeed = function () {
					return typeof item.Speed != 'undefined';
				};
				ctrl.displayGust = function () {
					return typeof item.Gust != 'undefined';
				};
				ctrl.displayTempChill = function () {
					return ctrl.displayChill() && typeof item.Temp != 'undefined';
				};
				ctrl.displayChill = function () {
					return ctrl.displayDirection() && typeof item.Chill != 'undefined';
				};
				ctrl.displayForecastAdmin = function () {
					return typeof item.forecast_url != 'undefined';
				};

				ctrl.Forecast = function () {
					return $.t(item.ForecastStr);
				};

				ctrl.image = function () {
					if (typeof item.Barometer != 'undefined') {
						return 'baro48.png';
					} else if (typeof item.Rain != 'undefined') {
						return 'Rain48_On.png';
					} else if (typeof item.Visibility != 'undefined') {
						return 'visibility48.png';
					} else if (typeof item.UVI != 'undefined') {
						return 'uv48.png';
					} else if (typeof item.Radiation != 'undefined') {
						return 'radiation48.png';
					} else if (typeof item.Direction != 'undefined') {
						return 'Wind' + item.DirectionStr + '.png';
					}
				};

				ctrl.MakeFavorite = function (n) {
					return MakeFavorite(item.idx, n);
				};

				ctrl.ShowLog = function () {
					$('#weatherwidgets').hide(); // TODO delete when multiple views implemented
					$('#weathertophtm').hide();
					if (typeof item.Barometer != 'undefined') {
						return ShowBaroLog('#weathercontent', 'ShowWeathers', item.idx, escape(item.Name));
					}
					else if (typeof item.Rain != 'undefined') {
						return ShowRainLog('#weathercontent', 'ShowWeathers', item.idx, escape(item.Name));
					}
					else if (typeof item.UVI != 'undefined') {
						return ShowUVLog('#weathercontent', 'ShowWeathers', item.idx, escape(item.Name));
					}
					else if (typeof item.Direction != 'undefined') {
						return ShowWindLog('#weathercontent', 'ShowWeathers', item.idx, escape(item.Name));
					}
					else if (typeof item.Visibility != 'undefined') {
						return $location.path('/Devices/' + item.idx + '/Log');
					}
					else if (typeof item.Radiation != 'undefined') {
                        return $location.path('/Devices/' + item.idx + '/Log');
					}
				};

				ctrl.EditDevice = function () {
					if (typeof item.Rain != 'undefined') {
						return EditRainDevice(item.idx, escape(item.Name), escape(item.Description), item.AddjMulti);
					} else if (typeof item.Visibility != 'undefined') {
						return EditVisibilityDevice(item.idx, escape(item.Name), escape(item.Description), item.SwitchTypeVal);
					} else if (typeof item.Radiation != 'undefined') {
						return EditWeatherDevice(item.idx, escape(item.Name), escape(item.Description));
					} else if (typeof item.Barometer != 'undefined') {
						return EditBaroDevice(item.idx, escape(item.Name), escape(item.Description), item.AddjValue2);
					} else {
						return EditWeatherDevice(item.idx, escape(item.Name), escape(item.Description), item.AddjMulti);
					}
				};

				ctrl.ShowForecast = function () {
					$('#weatherwidgets').hide(); // TODO delete when multiple views implemented
					$('#weathertophtm').hide();
					return ShowForecast(atob(item.forecast_url), escape(item.Name), escape(item.Description), '#weathercontent', 'ShowWeathers');
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
	}]);
});
