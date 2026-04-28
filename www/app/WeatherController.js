define(['app', 'livesocket', 'widgets/dzBar'], function (app) {
	app.controller('WeatherController', function ($scope, $rootScope, $location, $http, $interval, $route, $routeParams, deviceApi, permissions, livesocket, dzBarService) {

		var ctrl = this;

		MakeFavorite = function (id, isfavorite) {
			deviceApi.makeFavorite(id, isfavorite).then(function() {
				ShowWeathers();
			});
		};

		EditRainDevice = function (idx, name, description, addjmulti, deviceID, unitCode, colorJson) {
			$.devIdx = idx;
			$("#dialog-editraindevice #deviceidx").text(idx);
			$("#dialog-editraindevice #deviceid").text(deviceID);
			$("#dialog-editraindevice #deviceunit").text(unitCode);
			$("#dialog-editraindevice #devicename").val(unescape(name));
			$("#dialog-editraindevice #devicedescription").val(unescape(description));
			$("#dialog-editraindevice #multiply").val(addjmulti);
			$("#dialog-editraindevice").i18n();
			dzBarService.loadForKey(colorJson || '', 'rain');
			var $form = $('#dialog-editraindevice form');
			$form.css('position', 'relative');
			$form.find('.dz-bar-btn').remove();
			$('<a class="btnsmall dz-bar-btn"><i class="fa-solid fa-chart-bar"></i></a>')
				.css({position: 'absolute', top: '4px', right: '4px'})
				.on('click', function() { window.dzOpenBarPopup(idx, unescape(name)); })
				.appendTo($form);
			$("#dialog-editraindevice").dialog("open");
		}

		EditBaroDevice = function (idx, name, description, addjvalue, deviceID, unitCode, colorJson) {
			$.devIdx = idx;
			$("#dialog-editbarodevice #deviceidx").text(idx);
			$("#dialog-editbarodevice #deviceid").text(deviceID);
			$("#dialog-editbarodevice #deviceunit").text(unitCode);
			$("#dialog-editbarodevice #devicename").val(unescape(name));
			$("#dialog-editbarodevice #devicedescription").val(unescape(description));
			$("#dialog-editbarodevice #adjustment").val(addjvalue);
			$("#dialog-editbarodevice").i18n();
			dzBarService.loadForKey(colorJson || '', 'baro');
			var $form = $('#dialog-editbarodevice form');
			$form.css('position', 'relative');
			$form.find('.dz-bar-btn').remove();
			$('<a class="btnsmall dz-bar-btn"><i class="fa-solid fa-chart-bar"></i></a>')
				.css({position: 'absolute', top: '4px', right: '4px'})
				.on('click', function() { window.dzOpenBarPopup(idx, unescape(name)); })
				.appendTo($form);
			$("#dialog-editbarodevice").dialog("open");
		}

		EditVisibilityDevice = function (idx, name, description, switchtype, deviceID, unitCode, colorJson) {
			$.devIdx = idx;
			$("#dialog-editvisibilitydevice #deviceidx").text(idx);
			$("#dialog-editvisibilitydevice #deviceid").text(deviceID);
			$("#dialog-editvisibilitydevice #deviceunit").text(unitCode);
			$("#dialog-editvisibilitydevice #devicename").val(unescape(name));
			$("#dialog-editvisibilitydevice #devicedescription").val(unescape(description));
			$("#dialog-editvisibilitydevice #combometertype").val(switchtype);
			$("#dialog-editvisibilitydevice").i18n();
			dzBarService.loadForKey(colorJson || '', 'data');
			var $form = $('#dialog-editvisibilitydevice form');
			$form.css('position', 'relative');
			$form.find('.dz-bar-btn').remove();
			$('<a class="btnsmall dz-bar-btn"><i class="fa-solid fa-chart-bar"></i></a>')
				.css({position: 'absolute', top: '4px', right: '4px'})
				.on('click', function() { window.dzOpenBarPopup(idx, unescape(name)); })
				.appendTo($form);
			$("#dialog-editvisibilitydevice").dialog("open");
		}

		EditWindDevice = function (idx, name, description, addjvalue2, addjmulti, deviceID, unitCode, colorJson) {
			$.devIdx = idx;
			$("#dialog-editwinddevice #deviceidx").text(idx);
			$("#dialog-editwinddevice #deviceid").text(deviceID);
			$("#dialog-editwinddevice #deviceunit").text(unitCode);
			$("#dialog-editwinddevice #devicename").val(unescape(name));
			$("#dialog-editwinddevice #devicedescription").val(unescape(description));
			$("#dialog-editwinddevice #arotation").val(addjvalue2);
			$("#dialog-editwinddevice #multiply").val(addjmulti);
			$("#dialog-editwinddevice").i18n();
			dzBarService.loadForKey(colorJson || '', 'speed');
			var $form = $('#dialog-editwinddevice form');
			$form.css('position', 'relative');
			$form.find('.dz-bar-btn').remove();
			$('<a class="btnsmall dz-bar-btn"><i class="fa-solid fa-chart-bar"></i></a>')
				.css({position: 'absolute', top: '4px', right: '4px'})
				.on('click', function() { window.dzOpenBarPopup(idx, unescape(name)); })
				.appendTo($form);
			$("#dialog-editwinddevice").dialog("open");
		}

		EditUviDevice = function (idx, name, description, addjmulti2, deviceID, unitCode, colorJson) {
			$.devIdx = idx;
			$("#dialog-edituvidevice #deviceidx").text(idx);
			$("#dialog-edituvidevice #deviceid").text(deviceID);
			$("#dialog-edituvidevice #deviceunit").text(unitCode);
			$("#dialog-edituvidevice #devicename").val(unescape(name));
			$("#dialog-edituvidevice #devicedescription").val(unescape(description));
			$("#dialog-edituvidevice #multiply").val(addjmulti2);
			$("#dialog-edituvidevice").i18n();
			dzBarService.loadForKey(colorJson || '', 'uvi');
			var $form = $('#dialog-edituvidevice form');
			$form.css('position', 'relative');
			$form.find('.dz-bar-btn').remove();
			$('<a class="btnsmall dz-bar-btn"><i class="fa-solid fa-chart-bar"></i></a>')
				.css({position: 'absolute', top: '4px', right: '4px'})
				.on('click', function() { window.dzOpenBarPopup(idx, unescape(name)); })
				.appendTo($form);
			$("#dialog-edituvidevice").dialog("open");
		}

		EditWeatherDevice = function (idx, name, description, addjvalue, addjmulti, deviceID, unitCode, colorJson) {
			$.devIdx = idx;
			$("#dialog-editweatherdevice #deviceidx").text(idx);
			$("#dialog-editweatherdevice #deviceid").text(deviceID);
			$("#dialog-editweatherdevice #deviceunit").text(unitCode);
			$("#dialog-editweatherdevice #devicename").val(unescape(name));
			$("#dialog-editweatherdevice #devicedescription").val(unescape(description));
			$("#dialog-editweatherdevice").i18n();
			dzBarService.loadForKey(colorJson || '', 'data');
			var $form = $('#dialog-editweatherdevice form');
			$form.css('position', 'relative');
			$form.find('.dz-bar-btn').remove();
			$('<a class="btnsmall dz-bar-btn"><i class="fa-solid fa-chart-bar"></i></a>')
				.css({position: 'absolute', top: '4px', right: '4px'})
				.on('click', function() { window.dzOpenBarPopup(idx, unescape(name)); })
				.appendTo($form);
			$("#dialog-editweatherdevice").dialog("open");
		}

		AddWeatherDevice = function () {
			bootbox.alert($.t('Please use the devices tab for this.'));
		}

		RefreshItem = function (item) {
			item.searchText = GenerateLiveSearchTextW(item);
			var query = $('.jsLiveSearch').val();
			if (query && query.length > 0) {
				var match = item.searchText.toLowerCase().indexOf(query.toLowerCase()) !== -1;
				if (!match) {
					return; // Don't update items that don't match the filter
				}
			}
			ctrl.items.forEach(function (olditem, oldindex, oldarray) {
				if (olditem.idx == item.idx) {
					angular.extend(oldarray[oldindex], item);
					if (!document.hidden) {
						if ($scope.config.ShowUpdatedEffect == true) {
							setTimeout(function() {
								$("#weatherwidgets #" + item.idx + " #name").effect("highlight", { color: '#EEFFEE' }, 1000);
							}, 500);

						}
					}
				}
			});
			RefreshLiveSearch();
		}

		//We only call this once. After this the widgets are being updated automatically by used of the 'jsonupdate' broadcast event.
		RefreshWeathers = function () {
			var roomPlanId = $routeParams.room || window.myglobals.LastPlanSelected;
			var usedFilter = roomPlanId > 0 ? 'all' : 'true';
			livesocket.getJson("json.htm?type=command&param=getdevices&filter=weather&used=" + usedFilter + "&order=[Order]&lastupdate=" + $.LastUpdateTime + "&plan=" + roomPlanId, function (data) {
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

			var roomPlanId = $routeParams.room || window.myglobals.LastPlanSelected;
			var usedFilter = roomPlanId > 0 ? 'all' : 'true';

			$.ajax({
				url: "json.htm?type=command&param=getdevices&filter=weather&used=" + usedFilter + "&order=[Order]&plan=" + roomPlanId,
				dataType: 'json',
				success: function (data) {
					if (typeof data.result != 'undefined') {
						if (typeof data.ActTime != 'undefined') {
							$.LastUpdateTime = parseInt(data.ActTime);
						}
						$.each(data.result, function (i, item) {
							item.searchText = GenerateLiveSearchTextW(item);
						});
						ctrl.items = data.result;
					} else {
						ctrl.items = [];
					}

					$scope.loading = false;

					if (!$scope.$$phase) {
						$scope.$apply();
					}

					$('#modal').hide();
					$('#weathercontent').html("");
					$('#weathercontent').i18n();
					$('#weatherwidgets').show();
					$('#weatherwidgets').i18n();
					$('#weathertophtm').show();
					$('#weathertophtm').i18n();

					$rootScope.RefreshTimeAndSun();
					ScheduleLiveSearchRestore();
					RefreshWeathers();
				},
				error: function () {
					$('#modal').hide();
				}
			});
			return false;
		}

		$scope.DragWidget = function (idx) {
			$.devIdx = idx;
		};
		$scope.DropWidget = function (idx) {
			var myid = idx;
			var roomid = window.myglobals.LastPlanSelected;
			if (typeof roomid == 'undefined') {
				roomid = 0;
			}
			$.ajax({
				url: "json.htm?type=command&param=switchdeviceorder&idx1=" + myid + "&idx2=" + $.devIdx + "&roomid=" + roomid,
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


			//Default weather
			var dialog_editweatherdevice_buttons = {};
			dialog_editweatherdevice_buttons[$.t("Update")] = function () {
				var bValid = true;
				bValid = bValid && checkLength($("#dialog-editweatherdevice #devicename"), 2, 100);
				if (bValid) {
					$(this).dialog("close");
					$.ajax({
						url: "json.htm?type=command&param=setused&idx=" + $.devIdx +
						'&name=' + encodeURIComponent($("#dialog-editweatherdevice #devicename").val()) +
						'&description=' + encodeURIComponent($("#dialog-editweatherdevice #devicedescription").val()) +
						'&color=' + encodeURIComponent(dzBarService.getFullColorJson()) +
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
							url: "json.htm?type=command&param=setused&idx=" + $.devIdx +
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

			//wind
			var dialog_editwinddevice_buttons = {};
			dialog_editwinddevice_buttons[$.t("Update")] = function () {
				var bValid = true;
				bValid = bValid && checkLength($("#dialog-editwinddevice #devicename"), 2, 100);
				if (bValid) {
					$(this).dialog("close");
					$.ajax({
						url: "json.htm?type=command&param=setused&idx=" + $.devIdx +
						'&name=' + encodeURIComponent($("#dialog-editwinddevice #devicename").val()) +
						'&description=' + encodeURIComponent($("#dialog-editwinddevice #devicedescription").val()) +
						'&addjvalue2=' + $("#dialog-editwinddevice #edittable #arotation").val() +
						'&addjmulti=' + $("#dialog-editwinddevice #edittable #multiply").val() +
						'&color=' + encodeURIComponent(dzBarService.getFullColorJson()) +
						'&used=true',
						async: false,
						dataType: 'json',
						success: function (data) {
							ShowWeathers();
						}
					});

				}
			};
			dialog_editwinddevice_buttons[$.t("Remove Device")] = function () {
				$(this).dialog("close");
				bootbox.confirm($.t("Are you sure to remove this Device?"), function (result) {
					if (result == true) {
						$.ajax({
							url: "json.htm?type=command&param=setused&idx=" + $.devIdx +
							'&name=' + encodeURIComponent($("#dialog-editwinddevice #devicename").val()) +
							'&description=' + encodeURIComponent($("#dialog-editwinddevice #devicedescription").val()) +
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
			dialog_editwinddevice_buttons[$.t("Replace")] = function () {
				$(this).dialog("close");
				ReplaceDevice($.devIdx, ShowWeathers);
			};
			dialog_editwinddevice_buttons[$.t("Cancel")] = function () {
				$(this).dialog("close");
			};

			$("#dialog-editwinddevice").dialog({
				autoOpen: false,
				width: 'auto',
				height: 'auto',
				modal: true,
				resizable: false,
				title: $.t("Edit Device"),
				buttons: dialog_editwinddevice_buttons,
				close: function () {
					$(this).dialog("close");
				}
			}).i18n();
			
			//Rain
			var dialog_editraindevice_buttons = {};

			dialog_editraindevice_buttons[$.t("Update")] = function () {
				var bValid = true;
				bValid = bValid && checkLength($("#dialog-editraindevice #edittable #devicename"), 2, 100);
				if (bValid) {
					$(this).dialog("close");
					$.ajax({
						url: "json.htm?type=command&param=setused&idx=" + $.devIdx +
						'&name=' + encodeURIComponent($("#dialog-editraindevice #devicename").val()) +
						'&description=' + encodeURIComponent($("#dialog-editraindevice #devicedescription").val()) +
						'&addjmulti=' + $("#dialog-editraindevice #edittable #multiply").val() +
						'&color=' + encodeURIComponent(dzBarService.getFullColorJson()) +
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
							url: "json.htm?type=command&param=setused&idx=" + $.devIdx +
							'&name=' + encodeURIComponent($("#dialog-editraindevice #devicename").val()) +
							'&description=' + encodeURIComponent($("#dialog-editraindevice #devicedescription").val()) +
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
						url: "json.htm?type=command&param=setused&idx=" + $.devIdx +
						'&name=' + encodeURIComponent($("#dialog-editbarodevice #devicename").val()) +
						'&description=' + encodeURIComponent($("#dialog-editbarodevice #devicedescription").val()) +
						'&addjvalue2=' + aValue +
						'&color=' + encodeURIComponent(dzBarService.getFullColorJson()) +
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
							url: "json.htm?type=command&param=setused&idx=" + $.devIdx +
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
						url: "json.htm?type=command&param=setused&idx=" + $.devIdx +
						'&name=' + encodeURIComponent($("#dialog-editvisibilitydevice #devicename").val()) +
						'&description=' + encodeURIComponent($("#dialog-editvisibilitydevice #devicedescription").val()) +
						'&switchtype=' + $("#dialog-editvisibilitydevice #combometertype").val() +
						'&color=' + encodeURIComponent(dzBarService.getFullColorJson()) +
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
							url: "json.htm?type=command&param=setused&idx=" + $.devIdx +
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
			dialog_editvisibilitydevice_buttons[$.t("Replace")] = function () {
				$(this).dialog("close");
				ReplaceDevice($.devIdx, ShowWeathers);
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

			//UV
			var dialog_edituvidevice_buttons = {};

			dialog_edituvidevice_buttons[$.t("Update")] = function () {
				var bValid = true;
				bValid = bValid && checkLength($("#dialog-edituvidevice #edittable #devicename"), 2, 100);
				if (bValid) {
					$(this).dialog("close");
					$.ajax({
						url: "json.htm?type=command&param=setused&idx=" + $.devIdx +
						'&name=' + encodeURIComponent($("#dialog-edituvidevice #devicename").val()) +
						'&description=' + encodeURIComponent($("#dialog-edituvidevice #devicedescription").val()) +
						'&addjmulti2=' + $("#dialog-edituvidevice #edittable #multiply").val() +
						'&color=' + encodeURIComponent(dzBarService.getFullColorJson()) +
						'&used=true',
						async: false,
						dataType: 'json',
						success: function (data) {
							ShowWeathers();
						}
					});

				}
			};
			dialog_edituvidevice_buttons[$.t("Remove Device")] = function () {
				$(this).dialog("close");
				bootbox.confirm($.t("Are you sure to remove this Device?"), function (result) {
					if (result == true) {
						$.ajax({
							url: "json.htm?type=command&param=setused&idx=" + $.devIdx +
							'&name=' + encodeURIComponent($("#dialog-edituvidevice #devicename").val()) +
							'&description=' + encodeURIComponent($("#dialog-edituvidevice #devicedescription").val()) +
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
			dialog_edituvidevice_buttons[$.t("Replace")] = function () {
				$(this).dialog("close");
				ReplaceDevice($.devIdx, ShowWeathers);
			};
			dialog_edituvidevice_buttons[$.t("Cancel")] = function () {
				$(this).dialog("close");
			};

			$("#dialog-edituvidevice").dialog({
				autoOpen: false,
				width: 'auto',
				height: 'auto',
				modal: true,
				resizable: false,
				title: $.t("Edit Device"),
				buttons: dialog_edituvidevice_buttons,
				close: function () {
					$(this).dialog("close");
				}
			}).i18n();

			$scope.tblinks = [
				{
					onclick:"ShowForecast", 
					text:"Forecast", 
					i18n: "Forecast", 
					icon: "cloud-sun-rain"
				}
			];

			ctrl.RoomPlans = $rootScope.GetRoomPlans();
			var roomPlanId = $routeParams.room || window.myglobals.LastPlanSelected;
			if (typeof roomPlanId != 'undefined') {
				ctrl.roomSelected = roomPlanId;
				window.myglobals.LastPlanSelected = roomPlanId;
			}
			ctrl.changeRoom = function () {
				var idx = ctrl.roomSelected;
				window.myglobals.LastPlanSelected = idx;
				window.myglobals.LastSearchFilter = '';
				$('.jsLiveSearch').val('').trigger('change');
				$route.updateParams({
					room: idx >= 0 ? idx : undefined
				});
				$location.replace();
			};

			ctrl.items = [];
			$scope.loading = true;
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
						return $location.path('/Devices/' + item.idx + '/Log').search('sensor', 'baro');
					}
					else if (typeof item.Rain != 'undefined') {
						return $location.path('/Devices/' + item.idx + '/Log');
					}
					else if (typeof item.UVI != 'undefined') {
						return $location.path('/Devices/' + item.idx + '/Log');
					}
					else if (typeof item.Direction != 'undefined') {
						return $location.path('/Devices/' + item.idx + '/Log');
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
						return EditRainDevice(item.idx, escape(item.Name), escape(item.Description), item.AddjMulti, item.ID, item.Unit, item.Color || '');
					}
					else if (typeof item.Direction != 'undefined') {
						return EditWindDevice(item.idx, escape(item.Name), escape(item.Description), item.AddjValue2, item.AddjMulti, item.ID, item.Unit, item.Color || '');
					} else if (typeof item.Visibility != 'undefined') {
						return EditVisibilityDevice(item.idx, escape(item.Name), escape(item.Description), item.SwitchTypeVal, item.ID, item.Unit, item.Color || '');
					} else if (typeof item.UVI != 'undefined') {
						return EditUviDevice(item.idx, escape(item.Name), escape(item.Description), item.AddjMulti2, item.ID, item.Unit, item.Color || '');
					}
					else if (typeof item.Barometer != 'undefined') {
						return EditBaroDevice(item.idx, escape(item.Name), escape(item.Description), item.AddjValue2, item.ID, item.Unit, item.Color || '');
					} else {
						return EditWeatherDevice(item.idx, escape(item.Name), escape(item.Description), item.AddjValue, item.AddjMulti, item.ID, item.Unit, item.Color || '');
					}
				};

				ctrl.ShowForecast = function () {
					$('#weatherwidgets').hide(); // TODO delete when multiple views implemented
					$('#weathertophtm').hide();
					return ShowForecast(atob(item.forecast_url), escape(item.Name), escape(item.Description), '#weathercontent', 'ShowWeathers');
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
			}
		};
	}]);
});
