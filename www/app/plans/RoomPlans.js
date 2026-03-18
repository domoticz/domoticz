define(['app'], function (app) {

	app.factory('dzRoomPlanApi', function(domoticzApi, dzApiHelper) {
		return {
			addPlan: addPlan,
			updatePlan: updatePlan,
			removePlan: removePlan,
			getPlans: getPlans,
			getPlanDevices: getPlanDevices,
			getPlanAvailableDevices: getPlanAvailableDevices,
			addDeviceToPlan: addDeviceToPlan,
			removeDeviceFromPlan: removeDeviceFromPlan,
			removeAllDevicesFromPlan: removeAllDevicesFromPlan,
			changePlanOrder: changePlanOrder,
			changeDeviceOrder: changeDeviceOrder
		};

		function addPlan(planName) {
			return dzApiHelper.checkAdminPermissions().then(function () {
				return domoticzApi.sendCommand('addplan', {
					name: planName,
				});
			});
		}

		function updatePlan(planId, planName) {
			return dzApiHelper.checkAdminPermissions().then(function () {
				return domoticzApi.sendCommand('updateplan', {
					idx: planId,
					name: planName,
				});
			});
		}

		function removePlan(planId) {
			return dzApiHelper.checkAdminPermissions().then(function () {
				return domoticzApi.sendCommand('deleteplan', {
					idx: planId,
				});
			});
		}

		function getPlans() {
			return domoticzApi.sendCommand('getplans',{
				displayhidden: '1'
			})
				.then(domoticzApi.errorHandler)
				.then(function(response) {
					return response.result || []
				})
		}

		function getPlanDevices(planId) {
			return domoticzApi.sendCommand('getplandevices',{
				idx: planId
			})
				.then(domoticzApi.errorHandler)
				.then(function(response) {
					return response.result || []
				})
		}

		function getPlanAvailableDevices(planId) {
			return domoticzApi.sendCommand('getunusedplandevices', {
				actplan: planId,
				unique: 'true'
			})
				.then(domoticzApi.errorHandler)
				.then(function(response) {
					return response.result || []
				})
		}

		function addDeviceToPlan(planId, deviceId, isScene) {
			return dzApiHelper.checkAdminPermissions().then(function () {
				return domoticzApi.sendCommand('addplanactivedevice', {
					idx: planId,
					activeidx: deviceId,
					activetype: isScene ? 1 : 0
				});
			});
		}

		function removeDeviceFromPlan(deviceIdx) {
			return dzApiHelper.checkAdminPermissions().then(function () {
				return domoticzApi.sendCommand('deleteplandevice', {
					idx: deviceIdx,
				});
			});
		}

		function removeAllDevicesFromPlan(planId) {
			return dzApiHelper.checkAdminPermissions().then(function () {
				return domoticzApi.sendCommand('deleteallplandevices', {
					idx: planId,
				});
			});
		}

		function changePlanOrder(order, planId) {
			return dzApiHelper.checkAdminPermissions().then(function () {
				return domoticzApi.sendCommand('changeplanorder', {
					idx: planId,
					way: order
				});
			});
		}

		function changeDeviceOrder(planId, orderedIds) {
			return dzApiHelper.checkAdminPermissions().then(function () {
				return domoticzApi.sendCommand('changeplandeviceorder', {
					planid: planId,
					order: orderedIds.join(',')
				});
			});
		}
	});

	var addPlanModal = {
		templateUrl: 'app/plans/roomPlanAddModal.html',
		controllerAs: '$ctrl',
		controller: function ($scope, dzRoomPlanApi) {
			var $ctrl = this;
			init();

			function init() {
				$ctrl.planName = $scope.planName;
			}

			$ctrl.add= function () {
				$ctrl.isSaving = true;
				dzRoomPlanApi.addPlan($ctrl.planName).then($scope.$close);
			}
		}
	};

	var editPlanModal = {
		templateUrl: 'app/plans/roomPlanEditModal.html',
		controllerAs: '$ctrl',
		controller: function ($scope, dzRoomPlanApi) {
			var $ctrl = this;
			init();

			function init() {
				$ctrl.planId = $scope.planId;
				$ctrl.planName = $scope.planName;
			}

			$ctrl.update= function () {
				$ctrl.isSaving = true;
				dzRoomPlanApi.updatePlan($ctrl.planId, $ctrl.planName).then($scope.$close);
			}
		}
	};

	var selectDeviceModal = {
		templateUrl: 'app/plans/roomPlanDeviceSelectorModal.html',
		windowTopClass: 'device-selector-modal',
		controllerAs: '$ctrl',
		controller: function (dzRoomPlanApi) {
			var $ctrl = this;
			init();

			function init() {
				refreshPlanAvailableDevices()
			}

			function refreshPlanAvailableDevices() {
				dzRoomPlanApi.getPlanAvailableDevices(dzRoomPlanApi.selectedPlan).then(function(devices) {
					var regex = new RegExp('^\\[(.*)\] (.*)$');

					$ctrl.devices = devices.map(function (device) {
						var result = regex.exec(device.Name);
						if (!result) {
							return Object.assign({}, device, { Hardware: '(unknown)', Name: device.Name });
						}
						return Object.assign({}, device, {
							Hardware: result[1],
							Name: result[2],
						});
					});
				})
			}

			$ctrl.selectedDevices = [];

			$ctrl.onDevicesSelected = function(devices) {
				$ctrl.selectedDevices = devices || [];
			};
		}
	};

	app.component('roomPlansTable', {
		bindings: {
			plans: '<',
			onUpdate: '&',
			onSelect: '&',
		},
		template:  '<table id="plantable" class="display" width="100%"></table>',
		controller: function ($element, $scope, $uibModal, $timeout, bootbox, dzRoomPlanApi, dataTableDefaultSettings) {
			var $ctrl = this;
			var table;

			$ctrl.$onInit = function () {
				table = $element.find('table').dataTable(Object.assign({}, dataTableDefaultSettings, {
					order: [[2, 'asc']],
					ordering: false,
					columns: [
						{ title: $.t('Idx'), width: '40px', data: 'idx' },
						{ title: $.t('Name'), data: 'Name' },
						{ title: $.t('Order'), className: 'actions-column', width: '50px', data: 'Order', render: orderRenderer},
						{ title: '', className: 'actions-column', width: '40px', data: 'idx', render: actionsRenderer },
					]
				}));

				table.on('click', '.js-move-up', function () {
					var plan = table.api().row($(this).closest('tr')).data();
					dzRoomPlanApi.changePlanOrder(0, plan.idx).then($ctrl.onUpdate);
					$scope.$apply();
					return false;
				});

				table.on('click', '.js-move-down', function () {
					var plan = table.api().row($(this).closest('tr')).data();
					dzRoomPlanApi.changePlanOrder(1, plan.idx).then($ctrl.onUpdate);
					$scope.$apply();
					return false;
				});

				table.on('click', '.js-rename', function () {
					var plan = table.api().row($(this).closest('tr')).data();
					var scope = $scope.$new(true);
					scope.planId = plan.idx;
					scope.planName = plan.Name;

					$uibModal
						.open(Object.assign({ scope: scope }, editPlanModal)).result
						.then($ctrl.onUpdate);

					$scope.$apply();
					return false;
				});

				table.on('click', '.js-remove', function () {
					var plan = table.api().row($(this).closest('tr')).data();

					bootbox.confirm($.t("Are you sure you want to delete this Plan?"))
						.then(function () {
							return dzRoomPlanApi.removePlan(plan.idx);
						})
						.then($ctrl.onUpdate);

					$scope.$apply();
					return false;
				});

				table.on('select.dt', function (event, row) {
					$ctrl.onSelect({plan: row.data()});
					$scope.$apply();
				});

				table.on('deselect.dt', function () {
					//Timeout to prevent flickering when we select another item in the table
					$timeout(function() {
						if (table.api().rows({ selected: true }).count() > 0) {
							return;
						}

						$ctrl.onSelect({plan: null});
					});

					$scope.$apply();
				});
			};

			$ctrl.$onChanges = function (changes) {
				if (!table) {
					return;
				}

				if (changes.plans) {
					table.api().clear();
					table.api().rows
						.add($ctrl.plans)
						.draw();
				}
			};

			function orderRenderer(value) {
				var upIcon = '<button class="btn btn-icon js-move-up"><img src="./images/up.png" /></button>';
				var downIcon = '<button class="btn btn-icon js-move-down"><img src="./images/down.png" /></button>';
				var emptyIcon = '<img src="./images/empty16.png" width="16" height="16" />';

				if (value === '1') {
					return downIcon + emptyIcon;
				} else if (parseInt(value, 10) === $ctrl.plans.length) {
					return  emptyIcon + upIcon;
				} else {
					return downIcon + upIcon;
				}
			}

			function actionsRenderer() {
				var actions = [];
				actions.push('<button class="btn btn-icon js-rename" title="' + $.t('Rename') + '"><img src="./images/rename.png" /></button>');
				actions.push('<button class="btn btn-icon js-remove" title="' + $.t('Remove') + '"><img src="./images/delete.png" /></button>');
				return actions.join('&nbsp;');
			}
		}
	});

	app.component('roomPlanDevicesTable', {
		bindings: {
			planId: '<',
			devices: '<',
			onUpdate: '&',
		},
		template: '<div>' +
		'<div class="btn-panel align-right" ng-show="$ctrl.hasSelection">' +
			'<button class="btn btn-default js-remove-selected" data-i18n="Remove Selected"></button>' +
		'</div>' +
		'<table id="plan-devices-table" class="display" width="100%"></table>' +
	'</div>',
		controller: function($element, $scope, $q, bootbox, dzRoomPlanApi, dataTableDefaultSettings) {
			var $ctrl = this;
			var table;
			$ctrl.hasSelection = false;

			$ctrl.$onInit = function () {
				table = $element.find('table').dataTable(Object.assign({}, dataTableDefaultSettings, {
					ordering: false,
					select: { style: 'multi' },
					createdRow: function(row, data) {
						$(row).attr('data-idx', data.idx);
					},
					columns: [
						{ title: '', width: '20px', data: null, orderable: false, render: dragHandleRenderer },
						{ title: $.t('Idx'), width: '40px', data: 'devidx' },
						{ title: $.t('Name'), data: 'Name' },
						{ title: '', className: 'actions-column', width: '40px', data: 'idx', render: actionsRenderer },
					]
				}));

				// Enable drag-to-reorder via jQuery UI Sortable
				initSortable($element.find('table tbody'));

				table.on('select.dt deselect.dt', function () {
					$ctrl.hasSelection = table.api().rows({ selected: true }).count() > 0;
					$scope.$apply();
				});

				table.on('click', '.js-remove', function () {
					var device = table.api().row($(this).closest('tr')).data();

					bootbox.confirm($.t("Are you sure to delete this Active Device?\n\nThis action can not be undone...?"))
						.then(function () {
							return dzRoomPlanApi.removeDeviceFromPlan(device.idx);
						})
						.then($ctrl.onUpdate);

					$scope.$apply();
					return false;
				});

				table.on('click', '.js-remove-selected', function () {
					var selected = table.api().rows({ selected: true }).data().toArray();
					if (!selected.length) return false;

					bootbox.confirm($.t('Are you sure to delete the selected Active Devices?\n\nThis action can not be undone...?'))
						.then(function () {
							return selected.reduce(function(promise, device) {
								return promise.then(function() {
									return dzRoomPlanApi.removeDeviceFromPlan(device.idx);
								});
							}, $q.when());
						})
						.then(function() {
							$ctrl.hasSelection = false;
							return $ctrl.onUpdate();
						});

					$scope.$apply();
					return false;
				});
			};

			$ctrl.$onChanges = function (changes) {
				if (!table) {
					return;
				}

				if (changes.devices) {
					table.api().clear();
					table.api().rows
						.add($ctrl.devices)
						.draw();

					// Reinitialize sortable after redraw so new rows are draggable
					var $tbody = $element.find('table tbody');
					if ($tbody.hasClass('ui-sortable')) {
						$tbody.sortable('destroy');
					}
					initSortable($tbody);
				}
			};

			function initSortable($tbody) {
				$tbody.sortable({
					axis: 'y',
					handle: '.drag-handle',
					helper: function(e, tr) {
						var $originals = tr.children();
						var $helper = tr.clone();
						$helper.children().each(function(index) {
							$(this).width($originals.eq(index).width());
						});
						return $helper;
					},
					update: function() {
						var orderedIds = $element.find('table tbody tr').map(function() {
							return $(this).attr('data-idx');
						}).get();
						dzRoomPlanApi.changeDeviceOrder($ctrl.planId, orderedIds)
							.then($ctrl.onUpdate);
						$scope.$apply();
					}
				});
			}

			function dragHandleRenderer() {
				return '<span class="drag-handle" style="cursor:grab;">&#9776;</span>';
			}

			function actionsRenderer() {
				return '<button class="btn btn-icon js-remove" title="' + $.t('Remove') + '"><img src="./images/delete.png" /></button>';
			}
		}
	});

	app.component('roomPlanDeviceSelector', {
		bindings: {
			devices: '<',
			onSelect: '&'
		},
		templateUrl: 'app/plans/roomPlanDeviceSelector.html',
		controller: function($filter) {
			var $ctrl = this;

			$ctrl.$onInit = function() {
				$ctrl.hardwareFilter = '';
				$ctrl.hardwareItems = [];
				updateDevices();
			};

			$ctrl.$onChanges = function (changes) {
				if (changes.devices) {
					updateDevices()
				}
			};

			$ctrl.selectDevices = function(devices) {
				$ctrl.selectedDevices = devices;
				$ctrl.onSelect({ devices: $ctrl.selectedDevices });
			};

			$ctrl.filterByHardware = function(hardware) {
				$ctrl.hardwareFilter = $ctrl.hardwareFilter !== hardware ? hardware : '';

				$ctrl.filteredDevices = $filter('filter')($ctrl.devices, {
					Hardware: $ctrl.hardwareFilter
				});
			};

			function updateDevices() {
				$ctrl.hardwareItems = Array.from(new Set($ctrl.devices.map(device => device.Hardware))).sort();
				$ctrl.filterByHardware($ctrl.hardwareFilter);
			}
		}
	});

	app.component('roomPlansDeviceSelectorTable', {
		bindings: {
			devices: '<',
			onSelect: '&'
		},
		template: '<table id="roomPlansDeviceSelectorTable" class="display" width="100%"></table>',
		controller: function($scope, $element, dataTableDefaultSettings) {
			var $ctrl = this;
			var table;

			$ctrl.$onInit = function () {
				table = $element.find('table').dataTable(Object.assign({}, dataTableDefaultSettings, {
					dom: '<"H"lfrC>t',
					order: [[1, 'asc']],
					paging: false,
					select: { style: 'multi' },
					columns: [
						{title: $.t('Idx'), width: '40px', data: 'idx'},
						{title: $.t('Name'), data: 'Name'},
					]
				}));

				table.on('select.dt deselect.dt', function () {
					var selected = table.api().rows({ selected: true }).data().toArray();
					$ctrl.onSelect({ values: selected });
					$scope.$apply();
				});

				showDevices($ctrl.devices)
			};

			$ctrl.$onChanges = function (changes) {
				if (changes.devices) {
					showDevices($ctrl.devices)
				}
			};

			function showDevices(devices) {
				if (!table) {
					return;
				}

				if (devices) {
					table.api().clear();
					table.api().rows
						.add(devices)
						.draw();
				}
			}
		}
	});

	app.controller('RoomPlansController', function ($scope, $uibModal, $q, bootbox, dzRoomPlanApi) {
		var $ctrl = this;

		$ctrl.addPlan = addPlan;
		$ctrl.addDeviceToPlan = addDeviceToPlan;
		$ctrl.refreshPlans = refreshPlans;
		$ctrl.refreshPlanDevices = refreshPlanDevices;
		$ctrl.selectPlan = selectPlan;
		$ctrl.removeAllDevicesFromPlan = removeAllDevicesFromPlan;


		goBack = function () {
			window.history.back();
		}


		init();

		function init() {
			$ctrl.plans = [];
			$ctrl.planDevices = [];
			$ctrl.planAvailableDevices = [];
			$ctrl.refreshPlans();

			//handles topBar Links
			$scope.tblinks = [
				{
					onclick: "goBack", 
					text: "Back", 
					i18n: "Back", 
					icon: "reply"
				}
			];

		}

		function addPlan() {
			var scope = $scope.$new(true);

			$uibModal
				.open(Object.assign({ scope: scope }, addPlanModal)).result
				.then($ctrl.refreshPlans);
		}

		function addDeviceToPlan() {
			var scope = $scope.$new(true);
			dzRoomPlanApi.selectedPlan = $ctrl.selectedPlan.idx;

			$uibModal
				.open(Object.assign({ scope: scope }, selectDeviceModal)).result
				.then(function(selectedDevices) {
					if (!selectedDevices || !selectedDevices.length) return;
					return $q.all(selectedDevices.map(function(device) {
						return dzRoomPlanApi.addDeviceToPlan($ctrl.selectedPlan.idx, device.idx, device.type);
					}));
				})
				.then(refreshPlanDevices)
				.catch(angular.noop);
		}

		function refreshPlans() {
			dzRoomPlanApi.getPlans().then(function(plans) {
				$ctrl.plans = plans;
			});
		}

		function selectPlan(plan) {
			$ctrl.selectedPlan = plan;
			$ctrl.refreshPlanDevices();
		}

		function refreshPlanDevices() {
			if (!$ctrl.selectedPlan) {
				$ctrl.planDevices = [];
				return;
			}

			dzRoomPlanApi.getPlanDevices($ctrl.selectedPlan.idx).then(function(devices) {
				$ctrl.planDevices = devices;
			});
		}

		function removeAllDevicesFromPlan() {
			bootbox.confirm($.t("Are you sure to delete ALL Active Devices?\n\nThis action can not be undone!!"))
				.then(function () {
					return dzRoomPlanApi.removeAllDevicesFromPlan($ctrl.selectedPlan.idx);
				})
				.then($ctrl.refreshPlanDevices);
		}
	});
});
