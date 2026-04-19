define(['app'], function (app) {
	app.directive('dzSceneWidget', function ($rootScope, $location, sceneApi, permissions) {
		return {
			restrict: 'E',
			replace: true,
			scope: {
				scene: '<',
				dashboardType: '<',
				viewMode: '@',
				onUpdate: '&',
				onEdit: '&'
			},
			templateUrl: function(element, attrs) {
				if (attrs.viewMode === 'tab') {
					return 'views/widgets/scene_widget_tab.html';
				}
				var isMobile = window.myglobals && window.myglobals.ismobile;
				var dashboardType = window.myglobals && window.myglobals.DashboardType;
				if (isMobile || dashboardType == 2) {
					return 'views/widgets/scene_widget_mobile.html';
				}
				return 'views/widgets/scene_widget.html';
			},
			controllerAs: 'ctrl',
			bindToController: true,
			controller: function ($scope, $element) {
				var ctrl = this;

				ctrl.isScene = function() {
					return ctrl.scene.Type && ctrl.scene.Type.indexOf('Scene') === 0;
				};

				ctrl.isGroup = function() {
					return ctrl.scene.Type && ctrl.scene.Type.indexOf('Group') === 0;
				};

				ctrl.isOn = function() {
					if (!ctrl.isGroup()) return false;
					return ctrl.scene.Status === 'On' || ctrl.scene.Status === 'Group On';
				};

				ctrl.isOff = function() {
					if (!ctrl.isGroup()) return false;
					return ctrl.scene.Status === 'Off' || ctrl.scene.Status === 'Group Off' ||
						   ctrl.scene.Status === 'Mixed';
				};

				ctrl.isProtected = function() {
					return ctrl.scene.Protected === true || ctrl.scene.Protected === 1;
				};

				ctrl.hasTimers = function() {
					return ctrl.scene.Timers === 'true' || ctrl.scene.Timers === true;
				};

				ctrl.getBackgroundClass = function() {
					var backgroundClass = $rootScope.GetItemBackgroundStatus(ctrl.scene);
					var nbackstyle = backgroundClass || '';
					if (ctrl.scene.PlanID && ctrl.scene.PlanID > 0) {
						if (ctrl.scene.Name) {
							nbackstyle += ' ' + ctrl.scene.Name.substring(0, 1).toUpperCase();
						}
					}
					return nbackstyle;
				};

				ctrl.getSpanClass = function () {
					if (ctrl.dashboardType == 1) {
						return 'span3';
					}
					return 'span4';
				};

				ctrl.getStatusText = function() {
					if (!ctrl.scene) return '';
					return TranslateStatusShort(ctrl.scene.Status);
				};

				ctrl.switchOn = function() {
					if (ctrl.isProtected()) {
						bootbox.prompt({
						title: $.t("Please enter Password") + ":",
						inputType: 'password',
						callback: function(result) {
							if (result === null || result === "") return;
							SwitchSceneInt(ctrl.scene.idx, 'On', result);
						}
					});
					} else {
						ctrl.executeSwitch('On');
					}
				};

				ctrl.switchOff = function() {
					if (ctrl.isProtected()) {
						bootbox.prompt({
						title: $.t("Please enter Password") + ":",
						inputType: 'password',
						callback: function(result) {
							if (result === null || result === "") return;
							SwitchSceneInt(ctrl.scene.idx, 'Off', result);
						}
					});
					} else {
						ctrl.executeSwitch('Off');
					}
				};

				ctrl.executeSwitch = function(command, password) {
					var switchFunction = command === 'On' ? sceneApi.switchOn : sceneApi.switchOff;

					switchFunction(ctrl.scene.idx, password).then(
						function(response) {
							if (response.status === 'OK') {
								if (ctrl.onUpdate) {
									ctrl.onUpdate();
								}
							} else {
								ShowNotify($.t('Problem sending switch command!'), 2500, true);
							}
						},
						function(error) {
							ShowNotify($.t('Problem sending switch command!'), 2500, true);
						}
					);
				};

				ctrl.makeFavorite = function(favorite) {
					sceneApi.makeFavorite(ctrl.scene.idx, favorite).then(
						function(response) {
							if (response.status === 'OK') {
								ctrl.scene.Favorite = favorite;
								if (ctrl.onUpdate) {
									ctrl.onUpdate();
								}
							}
						}
					);
				};

				ctrl.editScene = function() {
					if (!permissions.hasPermission('Admin')) {
						ShowNotify($.t('You do not have permission to do that!'), 2500, true);
						return;
					}
					// Delegate to parent controller via on-edit binding
					if (ctrl.onEdit) {
						ctrl.onEdit();
					}
				};

				ctrl.isAdmin = permissions.hasPermission('Admin');

				ctrl.showCameraStream = function() {
					ShowCameraLiveStream(escape(ctrl.scene.Name), ctrl.scene.CameraIdx, ctrl.scene.CameraAspect);
				};

				ctrl.getTypeText = function() {
					if (!ctrl.scene) return '';
					return $.t(ctrl.scene.Type);
				};

				ctrl.searchText = '';
				ctrl.updateSearchText = function() {
					if (!ctrl.scene) return;
					var bigtext = TranslateStatusShort(ctrl.scene.Status);
					ctrl.searchText = GenerateLiveSearchTextSG(ctrl.scene, bigtext);
				};
				ctrl.updateSearchText();

				$scope.$watch('ctrl.scene', function() {
					ctrl.updateSearchText();
				}, true);
			},
			link: function(scope, element) {
				element.i18n();
			}
		};
	});
});
