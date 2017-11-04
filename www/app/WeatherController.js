define(['app'], function (app) {
	app.controller('WeatherController', ['$scope', '$rootScope', '$location', '$http', '$interval', 'permissions', function ($scope, $rootScope, $location, $http, $interval, permissions) {

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
					ShowWeathers();
				}
			});
		}

		EditRainDevice = function (idx, name, description, addjmulti) {
			if (typeof $scope.mytimer != 'undefined') {
				$interval.cancel($scope.mytimer);
				$scope.mytimer = undefined;
			}
			$.devIdx = idx;
			$("#dialog-editraindevice #devicename").val(unescape(name));
			$("#dialog-editraindevice #devicedescription").val(unescape(description));
			$("#dialog-editraindevice #multiply").val(addjmulti);
			$("#dialog-editraindevice").i18n();
			$("#dialog-editraindevice").dialog("open");
		}

		EditBaroDevice = function (idx, name, description, addjvalue) {
			if (typeof $scope.mytimer != 'undefined') {
				$interval.cancel($scope.mytimer);
				$scope.mytimer = undefined;
			}
			$.devIdx = idx;
			$("#dialog-editbarodevice #devicename").val(unescape(name));
			$("#dialog-editbarodevice #devicedescription").val(unescape(description));
			$("#dialog-editbarodevice #adjustment").val(addjvalue);
			$("#dialog-editbarodevice").i18n();
			$("#dialog-editbarodevice").dialog("open");
		}

		EditVisibilityDevice = function (idx, name, description, switchtype) {
			if (typeof $scope.mytimer != 'undefined') {
				$interval.cancel($scope.mytimer);
				$scope.mytimer = undefined;
			}
			$.devIdx = idx;
			$("#dialog-editvisibilitydevice #devicename").val(unescape(name));
			$("#dialog-editvisibilitydevice #devicedescription").val(unescape(description));
			$("#dialog-editvisibilitydevice #combometertype").val(switchtype);
			$("#dialog-editvisibilitydevice").i18n();
			$("#dialog-editvisibilitydevice").dialog("open");
		}

		EditWeatherDevice = function (idx, name, description) {
			if (typeof $scope.mytimer != 'undefined') {
				$interval.cancel($scope.mytimer);
				$scope.mytimer = undefined;
			}
			$.devIdx = idx;
			$("#dialog-editweatherdevice #devicename").val(unescape(name));
			$("#dialog-editweatherdevice #devicedescription").val(unescape(description));
			$("#dialog-editweatherdevice").i18n();
			$("#dialog-editweatherdevice").dialog("open");
		}

		AddWeatherDevice = function () {
			bootbox.alert($.t('Please use the devices tab for this.'));
		}

		RefreshWeathers = function () {
			if (typeof $scope.mytimer != 'undefined') {
				$interval.cancel($scope.mytimer);
				$scope.mytimer = undefined;
			}
			var id = "";

			$.ajax({
				url: "json.htm?type=devices&filter=weather&used=true&order=[Order]&lastupdate=" + $.LastUpdateTime,
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
							ctrl.items.forEach(function (olditem, oldindex, oldarray) {
								if (olditem.idx == newitem.idx) {
									oldarray[oldindex] = newitem;
									if ($scope.config.ShowUpdatedEffect == true) {
										$("#weatherwidgets #" + newitem.idx + " #name").effect("highlight", { color: '#EEFFEE' }, 1000);
									}
								}
							});
						});

					}
				}
			});
			$scope.mytimer = $interval(function () {
				RefreshWeathers();
			}, 10000);
		}

		ShowForecast = function () {
			SwitchLayout("Forecast");
		}

		ShowWeathers = function () {
			if (typeof $scope.mytimer != 'undefined') {
				$interval.cancel($scope.mytimer);
				$scope.mytimer = undefined;
			}
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

			$scope.mytimer = $interval(function () {
				RefreshWeathers();
			}, 10000);
			return false;
		}

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
		$scope.$on('$destroy', function () {
			if (typeof $scope.mytimer != 'undefined') {
				$interval.cancel($scope.mytimer);
				$scope.mytimer = undefined;
			}
		});
	}])
		.directive('dzweatherwidget', ['$rootScope', '$location', function ($rootScope,$location) {
			return {
				priority: 0,
				restrict: 'E',
				templateUrl: 'views/weather_widget.html',
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
						return $.t(ctrl.item.ForecastStr);
					};

					ctrl.image = function () {
						if (typeof item.Barometer != 'undefined') {
							return 'baro48.png';
						} else if (typeof item.Rain != 'undefined') {
							return 'rain48.png';
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
						return MakeFavorite(ctrl.item.idx, n);
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
							return ShowGeneralGraph('#weathercontent', 'ShowWeathers', item.idx, escape(item.Name), item.SwitchTypeVal, 'Visibility');
						}
						else if (typeof item.Radiation != 'undefined') {
							return ShowGeneralGraph('#weathercontent', 'ShowWeathers', item.idx, escape(item.Name), item.SwitchTypeVal, 'Radiation');
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
							return EditWeatherDevice(item.idx, escape(item.Name), escape(item.Description));
						}
					};

					ctrl.ShowNotifications = function () {
						$('#weatherwidgets').hide(); // TODO delete when multiple views implemented
						$('#weathertophtm').hide();
						return ShowNotifications(item.idx, escape(item.Name), '#weathercontent', 'ShowWeathers');
					};

					ctrl.ShowForecast = function () {
						$('#weatherwidgets').hide(); // TODO delete when multiple views implemented
						$('#weathertophtm').hide();
						return ShowForecast(atob(item.forecast_url), escape(item.Name), escape(item.Description), '#weathercontent', 'ShowWeathers');
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
