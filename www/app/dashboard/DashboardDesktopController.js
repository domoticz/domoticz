/**
 * Dashboard Desktop Controller
 *
 * Dedicated controller for desktop/tablet dashboard view, optimized for
 * multi-column layouts and larger screens. This controller manages:
 * - 3-column (default) and 4-column grid layouts
 * - Room plan selection and filtering
 * - Real-time device/scene updates via WebSocket
 * - Drag-and-drop reordering of favorites
 * - Search/filter functionality
 * - Backward compatibility with global legacy functions
 *
 * Key features:
 * - Uses dashboardService for data loading and categorization
 * - Uses widget directives for rendering (dzLightWidget, dzSceneWidget, dzUtilityWidget)
 * - Supports DashboardType configuration (0=3col, 1=4col, 3=floorplan redirect)
 * - Subscribes to real-time updates for live device status
 * - Maintains global function compatibility for legacy code
 */
define([
	'app',
	'dashboard/dashboardService',
	'widgets/dzLightWidget',
	'widgets/dzSceneWidget',
	'widgets/dzUtilityWidget',
	'livesocket'
], function (app) {
	app.controller('DashboardDesktopController', [
		'$scope',
		'$rootScope',
		'$location',
		'$route',
		'$routeParams',
		'$window',
		'$timeout',
		'permissions',
		'livesocket',
		'dashboardService',
		'domoticzApi',
		function (
			$scope,
			$rootScope,
			$location,
			$route,
			$routeParams,
			$window,
			$timeout,
			permissions,
			livesocket,
			dashboardService,
			domoticzApi
		) {
			var $element = $('#main-view #dashcontent').last();
			var unsubscribe = null; // Cleanup function for real-time updates

			// Initialize scope variables
			$scope.LastUpdateTime = 0;
			$scope.scenes = [];
			$scope.lights = [];
			$scope.temperature = [];
			$scope.weather = [];
			$scope.utility = [];
			$scope.loading = true;
			$scope.hasDevices = false;

			// Dashboard layout variables
			$scope.dashboardType = 0; // 0=3col, 1=4col, 3=floorplan
			$scope.rowItems = 3;

			// Search/filter
			$scope.searchFilter = (window.myglobals && window.myglobals.LastSearchFilter) || '';

			// Room plan controller
			$scope.ctrl = {
				RoomPlans: [],
				roomSelected: undefined,
				changeRoom: function () {
					var idx = $scope.ctrl.roomSelected;
					window.myglobals.LastPlanSelected = idx;
					window.myglobals.LastSearchFilter = '';
					$('.jsLiveSearch').val('').trigger('change');
					$route.updateParams({
						room: idx >= 0 ? idx : undefined
					});
					$location.replace();
					loadFavorites();
				}
			};

			// Aliases for mobile template compatibility
			$scope.RoomPlans = $scope.ctrl.RoomPlans;
			$scope.roomSelected = '';
			$scope.changeRoom = function() {
				$scope.ctrl.roomSelected = $scope.roomSelected;
				$scope.ctrl.changeRoom();
			};

			/**
			 * Load favorites and categorize devices
			 */
			function loadFavorites(showLoading) {
				if (showLoading !== false) {
					$scope.loading = true;
				}

				var roomPlanId = $routeParams.room || window.myglobals.LastPlanSelected;

				dashboardService.loadFavorites(roomPlanId)
					.then(function (result) {
						$scope.LastUpdateTime = result.lastUpdateTime;

						if (result.sunrise !== undefined && result.sunset !== undefined && result.serverTime !== undefined) {
							$rootScope.SetTimeAndSun(result.sunrise, result.sunset, result.serverTime);
						}

						// Categorize devices
						var categorized = dashboardService.categorizeDevices(result.devices);

						$scope.scenes = categorized.scenes;
						$scope.lights = categorized.lights;
						$scope.temperature = categorized.temperature;
						$scope.weather = categorized.weather;
						$scope.utility = categorized.utility;

						$scope.hasDevices = result.devices.length > 0;
						$scope.loading = false;

						// Apply body classes based on dashboard type
						applyBodyClasses();

						// Trigger digest if not already in progress
						if (!$scope.$$phase) {
							$scope.$apply();
						}

						// Initialize UI enhancements after Angular has rendered the template
						$timeout(function () {
							initMobileSliders();
							$scope.ResizeDimSliders();
							initDragAndDrop();
							ScheduleLiveSearchRestore();
						}, 100);
					})
					.catch(function (error) {
						$scope.loading = false;
						$scope.hasDevices = false;
					});
			}

			/**
			 * Refresh favorites (mobile template refresh button)
			 */
			$scope.refresh = function() {
				loadFavorites();
			};

			/**
			 * Apply body classes based on dashboard type
			 * DashboardType 0 = 3-column (default)
			 * DashboardType 1 = 4-column
			 * DashboardType 2 = mobile
			 * DashboardType 3 = redirect to floorplan
			 */
			function applyBodyClasses() {
				$("body").removeClass();
				$("body").addClass("dashboard");

				// Handle floorplan redirect
				if ($scope.config.DashboardType == 3) {
					$window.location = '/#Floorplans';
					$("body").addClass("dashFloorplan");
					return;
				}

				// Handle mobile dashboard type
				if ($scope.config.DashboardType == 2 || (window.myglobals && window.myglobals.ismobile)) {
					$scope.dashboardType = 2;
					$("body").addClass("dashMobile");
					return;
				}

				$scope.dashboardType = $scope.config.DashboardType || 0;
				$scope.rowItems = 3;

				if ($scope.config.DashboardType == 1) {
					// 4-column layout
					$scope.rowItems = 4;
					$("body").addClass("4column");
				} else {
					// 3-column layout (default)
					$("body").addClass("3column");
				}
			}

			/**
			 * Handle real-time device update
			 */
			function onDeviceUpdate(deviceData) {
				if (!deviceData || !deviceData.idx) return;

				// Find and update device in the appropriate category
				var categories = ['lights', 'temperature', 'weather', 'utility'];

				categories.forEach(function (category) {
					var index = $scope[category].findIndex(function (item) {
						return item.idx === deviceData.idx;
					});

					if (index !== -1) {
						// Update the device data in-place to preserve object reference
						angular.extend($scope[category][index], deviceData);

						// Show update effect if enabled
						if ($scope.config.ShowUpdatedEffect === true) {
							var itemElement = $('#light_' + deviceData.idx + ', #utility_' + deviceData.idx + ', #temp_' + deviceData.idx + ', #weather_' + deviceData.idx);
							itemElement.find('.item-name').effect("highlight", { color: '#EEFFEE' }, 1000);
						}
					}
				});
			}

			/**
			 * Handle real-time scene update
			 */
			function onSceneUpdate(sceneData) {
				var index = $scope.scenes.findIndex(function (item) {
					return item.idx === sceneData.idx;
				});

				if (index !== -1) {
					angular.extend($scope.scenes[index], sceneData);

					// Show update effect if enabled
					if ($scope.config.ShowUpdatedEffect === true) {
						var sceneElement = $('#scene_' + sceneData.idx);
						sceneElement.find('.item-name').effect("highlight", { color: '#EEFFEE' }, 1000);
					}
				}
			}

		$scope.nl2br = function (text) {
			if (!text) return text;
			return text.replace(/([^>\r\n]?)(\r\n|\n\r|\r|\n)/g, '$1<br />$2');
		};

			/**
			 * Filter devices based on search query.
			 * Matches against the same fields as GenerateLiveSearchTextDefault
			 * so the Angular ng-if section visibility stays in sync with jQuery item hiding.
			 */
			$scope.filterDevices = function (device) {
				if (!$scope.searchFilter || $scope.searchFilter.trim() === '') {
					return true;
				}

				var q = $scope.searchFilter.toLowerCase();
				var fields = [
					device.idx, device.Name, device.Description,
					device.Type, device.HardwareName, device.SubType,
					device.HumidityStatus, device.ForecastStr, device.Status
				];
				return fields.some(function (f) {
					return f && String(f).toLowerCase().indexOf(q) !== -1;
				});
			};

			/**
			 * Keep $scope.searchFilter in sync with the jQuery live-search input.
			 * Uses native capture-phase listeners on document so WatchLiveSearch()'s
			 * jQuery .off() calls cannot remove them.
			 */
			function onCaptureSearch(e) {
				if (e.target && $(e.target).hasClass('jsLiveSearch')) {
					var query = e.target.value || '';
					$scope.$evalAsync(function () {
						$scope.searchFilter = query;
					});
				}
			}
			document.addEventListener('keyup', onCaptureSearch, true);
			document.addEventListener('change', onCaptureSearch, true);

			/**
			 * Initialize drag-and-drop for widget reordering (desktop only).
			 * Uses jQuery UI draggable/droppable on elements with class "movable".
			 * Only enabled when AllowWidgetOrdering is true, user has permission, and not mobile.
			 */
			function initDragAndDrop() {
				if ($scope.config.AllowWidgetOrdering != true) return;
				if (!permissions.hasPermission("User")) return;
				if (window.myglobals.ismobileint == true) return;

				// Make non-scene widgets draggable (User permission)
				$element.find(".movable:not([id^=scene_])").draggable({
					helper: 'clone',
					opacity: 0.7,
					zIndex: 1000,
					revert: 'invalid',
					scrollSensitivity: 40,
					scrollSpeed: 20,
					drag: function () {
						$.devIdx = $(this).attr("id");
					}
				});
				$element.find(".movable:not([id^=scene_])").droppable({
					drop: function () {
						var myid = $(this).attr("id");
						var parts1 = myid.split('_');
						var parts2 = $.devIdx.split('_');
						if (parts1[0] != parts2[0]) {
							bootbox.alert($.t('Only possible between Sensors of the same kind!'));
						} else {
							var roomid = 0;
							if (typeof window.myglobals.LastPlanSelected != 'undefined') {
								roomid = window.myglobals.LastPlanSelected;
							}
							$.ajax({
								url: "json.htm?type=command&param=switchdeviceorder&idx1=" + parts1[1] + "&idx2=" + parts2[1] + "&roomid=" + roomid,
								dataType: 'json',
								success: function (data) {
									loadFavorites(false);
								}
							});
						}
					}
				});

				// Make scene widgets draggable (Admin permission required)
				if (permissions.hasPermission("Admin")) {
					$element.find(".movable[id^=scene_]").draggable({
						helper: 'clone',
						opacity: 0.7,
						zIndex: 1000,
						revert: 'invalid',
						scrollSensitivity: 40,
						scrollSpeed: 20,
						drag: function () {
							$.devIdx = $(this).attr("id");
						}
					});
					$element.find(".movable[id^=scene_]").droppable({
						drop: function () {
							var myid = $(this).attr("id");
							var parts1 = myid.split('_');
							var parts2 = $.devIdx.split('_');
							if (parts1[0] != parts2[0]) {
								bootbox.alert($.t('Only possible between Sensors of the same kind!'));
							} else {
								$.ajax({
									url: "json.htm?type=command&param=switchsceneorder&idx1=" + parts1[1] + "&idx2=" + parts2[1],
									dataType: 'json',
									success: function (data) {
										loadFavorites(false);
									}
								});
							}
						}
					});
				}
			}

			/**
			 * Resize dim sliders (for dimmer widgets)
			 */
			$scope.ResizeDimSliders = function () {
				var nobj = $element.find("#name");
				if (typeof nobj == 'undefined') {
					return;
				}
				var width = $element.find("#name").width() - 40;
				$element.find(".span4 .dimslidernorm").width(width);
				$element.find(".span3 .dimslidernorm").width(width);

				width = $element.find("#name").width() - 40;
				$element.find(".span4 .dimslidersmall").width(width);
				$element.find(".span3 .dimslidersmall").width(width);

				width = $element.find("#name").width() - 85;
				$element.find(".span4 .dimslidersmalldouble").width(width);
				$element.find(".span3 .dimslidersmalldouble").width(width);

				width = $element.find("#name").width() - 115;
				$element.find(".span4 .dimslidersmalltripple").width(width);
				$element.find(".span3 .dimslidersmalltripple").width(width);

				width = $(".mobileitem").width() - 63;
				$("#main-view .mobileitem .dimslidernorm").width(width);

				width = $(".mobileitem").width() - 63;
				$("#main-view .mobileitem .dimslidersmall").width(width);
			};

			/**
			 * Initialize jQuery UI sliders for mobile view dimmer/TPI/blinds items.
			 * The dzLightWidget directive does not run for mobile inline rendering,
			 * so sliders must be initialized here after Angular renders the template.
			 */
			function initMobileSliders() {
				$('#main-view .mobileitem .dimslider').each(function () {
					var $slider = $(this);
					if (!$slider.hasClass('ui-slider')) {
						$slider.slider({
							range: "min",
							min: 0,
							max: 15,
							value: 4,
							create: function (event, ui) {
								$(this).slider("option", "max", $(this).data('maxlevel'));
								$(this).slider("option", "type", $(this).data('type'));
								$(this).slider("option", "isprotected", $(this).data('isprotected'));
								$(this).slider("value", $(this).data('svalue'));
								if ($(this).data('disabled'))
									$(this).slider("option", "disabled", true);
							},
							slide: function (event, ui) {
								clearInterval($.setDimValue);
								var dtype = $(this).slider("option", "type");
								var idx = $(this).data('idx');
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
					}
				});
			}

			// =========================================================================
			// GLOBAL FUNCTIONS (for backward compatibility with legacy code)
			// These are kept as global functions because they may be called from
			// other parts of the app (inline onclick handlers, legacy templates, etc.)
			// =========================================================================

			/**
			 * Global: Switch Evohome modal status
			 */
			window.SwitchModal = function (idx, name, status) {
				clearInterval($.myglobals.refreshTimer);

				ShowNotify($.t('Setting Evohome ') + ' ' + $.t(name));

				dashboardService.switchModal(idx, status, 1)
					.then(function () {
						$timeout(function () {
							HideNotify();
						}, 1000);
					})
					.catch(function (error) {
						HideNotify();
						bootbox.alert($.t('Problem sending switch command'));
					});
			};

			/**
			 * Global: Get Evohome display text
			 */
			window.EvoDisplayTextMode = function (strstatus) {
				return dashboardService.getEvohomeDisplayText(strstatus);
			};

			/**
			 * Global: Get light status text (handles Evohome, Selector, etc.)
			 */
			window.GetLightStatusText = function (item) {
				if (item.SubType == "Evohome") {
					return EvoDisplayTextMode(item.Status);
				} else if (item.SwitchType === "Selector") {
					return b64DecodeUnicode(item.LevelNames).split('|')[(item.LevelInt / 10)];
				} else {
					return item.Status;
				}
			};

			/**
			 * Global: Generate Evohome JavaScript for accordions
			 */
			window.EvohomeAddJS = function () {
				return "<script type='text/javascript'> function deselect(e,id) { $(id).slideFadeToggle('swing', function() { e.removeClass('selected'); });} $.fn.slideFadeToggle = function(easing, callback) {  return this.animate({ opacity: 'toggle',height: 'toggle' }, 'fast', easing, callback);};</script>";
			};

			/**
			 * Global: Generate Evohome image HTML
			 */
			window.EvohomeImg = function (item, strclass) {
				return '<div title="Quick Actions" class="' + ((item.Status == "Auto") ? "evoimgnorm " : "evoimg ") + strclass + '"><img src="images/evohome/' + item.Status + '.png" class="lcursor" onclick="if($(this).hasClass(\'selected\')){deselect($(this),\'#evopop_' + item.idx + '\');}else{$(this).addClass(\'selected\');$(\'#evopop_' + item.idx + '\').slideFadeToggle();}return false;"></div>';
			};

			/**
			 * Global: Generate Evohome popup menu HTML
			 */
			window.EvohomePopupMenu = function (item, strclass) {
				var htm = '\t      <td id="img" class="img img1"><a href="#evohome" id="evohome_' + item.idx + '">' + EvohomeImg(item, strclass) + '</a></td>\n<span class="' + strclass + '"><div id="evopop_' + item.idx + '" class="ui-popup ui-body-b ui-overlay-shadow ui-corner-all pop">  <ul class="ui-listview ui-listview-inset ui-corner-all ui-shadow">         <li class="ui-li-divider ui-bar-inherit ui-first-child">Choose an action</li>';
				$.each([
					{ "name": "Normal", "data": "Auto" },
					{ "name": "Economy", "data": "AutoWithEco" },
					{ "name": "Away", "data": "Away" },
					{ "name": "Day Off", "data": "DayOff" },
					{ "name": "Custom", "data": "Custom" },
					{ "name": "Heating Off", "data": "HeatingOff" }
				], function (idx, obj) {
					htm += '<li><a href="#" class="ui-btn ui-btn-icon-right ui-icon-' + obj.data + '" onclick="SwitchModal(\'' + item.idx + '\',\'' + obj.name + '\',\'' + obj.data + '\');deselect($(this),\'#evopop_' + item.idx + '\');return false;">' + obj.name + '</a></li>';
				});
				htm += '</ul></div></span>';
				return htm;
			};

			/**
			 * Global: Set color value for RGB devices
			 */
			window.SetColValue = function (idx, color, brightness) {
				clearInterval($.setColValue);
				if (!permissions.hasPermission("User")) {
					HideNotify();
					ShowNotify($.t('You do not have permission to do that!'), 2500, true);
					return;
				}
				domoticzApi.sendCommand('setcolbrightnessvalue', {
					idx: idx,
					color: color,
					brightness: brightness
				});
			};

			/**
			 * Global: Switch a light device (called from inline onclick handlers in the mobile template)
			 */
			window.SwitchLight = function (idx, command, isProtectedOrPasscode) {
				if (window.my_config.userrights == 0) {
					HideNotify();
					ShowNotify($.t('You do not have permission to do that!'), 2500, true);
					return;
				}
				// If third arg is a non-empty string, it's already a passcode
				if (typeof isProtectedOrPasscode === 'string' && isProtectedOrPasscode !== '') {
					SwitchLightInt(idx, command, isProtectedOrPasscode);
					return;
				}
				if (isProtectedOrPasscode === true || isProtectedOrPasscode === 1) {
					bootbox.prompt($.t("Please enter Password") + ":", function (result) {
						if (result === null || result === "") return;
						SwitchLightInt(idx, command, result);
					});
					return;
				}

				dashboardService.switchDevice(idx, command)
					.then(function () {
						// Device will be updated via WebSocket
					})
					.catch(function (error) {
						if (error.needsPassword) {
							ShowNotify($.t('Password required'), 2500, true);
						} else {
							bootbox.alert($.t('Problem sending switch command'));
						}
					});
			};

			/**
			 * Global: Switch a scene (called from inline onclick handlers in the mobile template)
			 */
			window.SwitchScene = function (idx, command, isProtectedOrPasscode) {
				if (window.my_config.userrights == 0) {
					HideNotify();
					ShowNotify($.t('You do not have permission to do that!'), 2500, true);
					return;
				}
				// If third arg is a non-empty string, it's already a passcode
				if (typeof isProtectedOrPasscode === 'string' && isProtectedOrPasscode !== '') {
					SwitchSceneInt(idx, command, isProtectedOrPasscode);
					return;
				}
				if (isProtectedOrPasscode === true || isProtectedOrPasscode === 1) {
					bootbox.prompt($.t("Please enter Password") + ":", function (result) {
						if (result === null || result === "") return;
						SwitchSceneInt(idx, command, result);
					});
					return;
				}

				dashboardService.switchScene(idx, command)
					.then(function () {
						// Scene will be updated via WebSocket
					})
					.catch(function (error) {
						if (error.needsPassword) {
							ShowNotify($.t('Password required'), 2500, true);
						} else {
							bootbox.alert($.t('Problem sending switch command'));
						}
					});
			};

			// Expose switch functions to scope for mobile template ng-click bindings
			$scope.switchLight = function(idx, command, isProtected) {
				SwitchLight(idx, command, isProtected);
			};
			$scope.switchScene = function(idx, command, isProtected) {
				SwitchScene(idx, command, isProtected);
			};
			$scope.showSetpointPopup = function($event, idx, isProtected, currentvalue, ismobile, step, min, max) {
				ShowSetpointPopup($event, idx, isProtected, currentvalue, ismobile, step, min, max);
			};

			/**
			 * Get selector levels for a device (used by mobile template).
			 * Cached per device to avoid creating new objects on every digest cycle.
			 */
			var selectorLevelCache = {};
			$scope.getSelectorLevels = function(item) {
				if (!item.LevelNames) return [];
				var cache = selectorLevelCache[item.idx];
				if (cache && cache.levelNames === item.LevelNames && cache.levelInt === item.LevelInt) {
					return cache.levels;
				}
				var levels = b64DecodeUnicode(item.LevelNames).split('|').map(function(name, index) {
					if (index === 0 && item.LevelOffHidden) return null;
					return {
						name: name,
						value: index * 10,
						isActive: (index * 10) === item.LevelInt
					};
				}).filter(function(level) {
					return level !== null;
				});
				selectorLevelCache[item.idx] = { levelNames: item.LevelNames, levelInt: item.LevelInt, levels: levels };
				return levels;
			};

			/**
			 * Get current selector level for dropdown binding.
			 * Derives the value from item.LevelInt on each call so it stays
			 * in sync after WebSocket updates replace the item object.
			 */
			$scope.selectorModels = {};
			$scope.getSelectorModel = function(item) {
				var levels = $scope.getSelectorLevels(item);
				var current = $scope.selectorModels[item.idx];
				if (!current || current.value !== item.LevelInt) {
					for (var i = 0; i < levels.length; i++) {
						if (levels[i].isActive) {
							$scope.selectorModels[item.idx] = levels[i];
							break;
						}
					}
				}
				return $scope.selectorModels[item.idx];
			};

			/**
			 * Switch selector level (used by mobile template)
			 */
			$scope.switchSelectorLevel = function(idx, level, levelName, isProtected) {
				SwitchSelectorLevel(idx, levelName, level, isProtected);
			};
			$scope.displayTrend = $rootScope.DisplayTrend;
			$scope.trendState = $rootScope.TrendState;

			/**
			 * Global: Edit scene (called from legacy templates)
			 */
			window.EditScene = function (idx) {
				$location.path('/Scenes/' + idx + '/Edit');
				$scope.$apply();
			};

			/**
			 * Global: Edit light (called from legacy templates)
			 */
			window.EditLight = function (idx) {
				$location.path('/Devices/' + idx + '/Edit');
				$scope.$apply();
			};

			/**
			 * Global: Refresh a single item (legacy compatibility)
			 * Now handled automatically via WebSocket subscriptions
			 */
			window.RefreshItem = function (item) {
				// Delegated to onDeviceUpdate and onSceneUpdate
				if (item.Type && (item.Type.indexOf('Scene') === 0 || item.Type.indexOf('Group') === 0)) {
					onSceneUpdate(item);
				} else {
					onDeviceUpdate(item);
				}
			};

			/**
			 * Global: Refresh all favorites (legacy polling mechanism)
			 * Now handled by WebSocket subscriptions, but kept for compatibility
			 */
			window.RefreshFavorites = function () {
				var bFavorites = 1;
				if (typeof window.myglobals.LastPlanSelected != 'undefined') {
					if (window.myglobals.LastPlanSelected > 0) {
						bFavorites = 0;
					}
				}

				var usedFilter = window.myglobals.LastPlanSelected > 0 ? 'all' : 'true';
			livesocket.getJson("json.htm?type=command&param=getdevices&filter=all&used=" + usedFilter + "&favorite=" + bFavorites + "&order=[Order]&plan=" + window.myglobals.LastPlanSelected + "&lastupdate=" + $scope.LastUpdateTime, function (data) {
					if (typeof data.ServerTime != 'undefined') {
						$rootScope.SetTimeAndSun(data.Sunrise, data.Sunset, data.ServerTime);
					}

					if (typeof data.result != 'undefined') {
						if (typeof data.ActTime != 'undefined') {
							$scope.LastUpdateTime = parseInt(data.ActTime);
						}

						$.each(data.result, function (i, item) {
							RefreshItem(item);
						});
					}
				});
			};

			/**
			 * Global: Show favorites (legacy function)
			 * Now handled by Angular data binding, but kept for compatibility
			 */
			window.ShowFavorites = function () {
				loadFavorites();
			};

			// =========================================================================
			// INITIALIZATION
			// =========================================================================

			function init() {
				// Setup window resize handler
				$(window).on('resize.dashDesktop', function () {
					$scope.ResizeDimSliders();
				});

				// Initialize globals
				$scope.LastUpdateTime = 0;

				// Initialize config from $rootScope
				if (typeof $scope.MakeGlobalConfig === 'function') {
					$scope.MakeGlobalConfig();
				}

				// Setup room plans
				$scope.ctrl.RoomPlans = $rootScope.GetRoomPlans();
				$scope.RoomPlans = $scope.ctrl.RoomPlans; // Sync alias for mobile template
				var roomPlanId = $routeParams.room || window.myglobals.LastPlanSelected;
				if (typeof roomPlanId != 'undefined') {
					$scope.ctrl.roomSelected = roomPlanId;
				}

				// Load favorites
				loadFavorites();

				// Subscribe to real-time updates
				unsubscribe = dashboardService.subscribeToUpdates($scope, {
					onDeviceUpdate: onDeviceUpdate,
					onSceneUpdate: onSceneUpdate
				});
			}

			// Cleanup on destroy
			$scope.$on('$destroy', function () {
				$(window).off('resize.dashDesktop');
				document.removeEventListener('keyup', onCaptureSearch, true);
			document.removeEventListener('change', onCaptureSearch, true);

				// Cleanup real-time subscriptions
				if (unsubscribe) {
					unsubscribe();
				}

				// Hide any open popups
				var popup = $("#rgbw_popup");
				if (typeof popup != 'undefined') {
					popup.hide();
				}
				popup = $("#setpoint_popup");
				if (typeof popup != 'undefined') {
					popup.hide();
				}
				popup = $("#thermostat3_popup");
				if (typeof popup != 'undefined') {
					popup.hide();
				}
			});

			// Start the controller
			init();
		}
	]);
});
