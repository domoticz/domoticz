define(['app'], function (app) {

	app.component('matterHardware', {
		bindings: {
			hardware: '<'
		},
		templateUrl: 'app/hardware/setup/Matter.html',
		controller: MatterHardwareController
	});

	function MatterHardwareController($element, dataTableDefaultSettings) {
		var $ctrl = this;
		var selectedNodeId = -1;
		var oNodeTable;

		$ctrl.$onInit = function () {
			$.devIdx = $ctrl.hardware.idx;
			RefreshMatterNodeTable();
		};

		function setNodeButtonsEnabled(enabled) {
			if (enabled) {
				$element.find('#matter_nodedelete').removeClass('btnstyle3-dis').addClass('btnstyle3');
				$element.find('#matter_noderefreshnodeinfo').removeClass('btnstyle3-dis').addClass('btnstyle3');
			} else {
				$element.find('#matter_nodedelete').removeClass('btnstyle3').addClass('btnstyle3-dis');
				$element.find('#matter_noderefreshnodeinfo').removeClass('btnstyle3').addClass('btnstyle3-dis');
			}
		}

		function RefreshMatterNodeTable() {
			selectedNodeId = -1;
			setNodeButtonsEnabled(false);

			if ($.fn.dataTable.isDataTable('#matterNodeTable')) {
				oNodeTable = $('#matterNodeTable').DataTable();
				oNodeTable.destroy();
				$('#matterNodeTable tbody').remove();
			}

			$.ajax({
				url: 'json.htm?type=command&param=getmatternodes&idx=' + $.devIdx,
				async: true,
				dataType: 'json'
			}).done(function (data) {
				var tbody = $('<tbody></tbody>');

				if (typeof data.result !== 'undefined') {
					$.each(data.result, function (i, item) {
						var now = new Date();
						var lastSeen = new Date(item.LastSeen);
						var oneHourAgo = new Date(now.getTime() - 60 * 60 * 1000);
						var statusOk = item.available && lastSeen >= oneHourAgo;
						var statusImg = statusOk
							? '<img src="images/ok.png" />'
							: '<img src="images/failed.png" />';

						var trow = $('<tr class="lcursor"></tr>');
						$('<td align="center"></td>').text(item.NodeID).appendTo(trow);
						$('<td></td>').text(item.DeviceNames || '').appendTo(trow);
						$('<td></td>').text(item.VendorName).appendTo(trow);
						$('<td></td>').text(item.ProductName).appendTo(trow);
						$('<td></td>').text(item.LastSeen).appendTo(trow);
						$('<td align="center"></td>').html(statusImg).appendTo(trow);

						trow.data('nodeId', item.NodeID);
						trow.on('click', function () {
							$('#matterNodeTable tbody tr').removeClass('row_selected');
							$(this).addClass('row_selected');
							selectedNodeId = $(this).data('nodeId');
							setNodeButtonsEnabled(true);
						});

						trow.appendTo(tbody);
					});
				}

				tbody.appendTo($('#matterNodeTable'));

				var oTableSettings = $.extend({}, dataTableDefaultSettings, {
					bFilter: false,
					bSortClasses: false,
					aaSorting: [[0, 'asc']]
				});
				oNodeTable = $('#matterNodeTable').DataTable(oTableSettings);
			}).fail(function () {
				bootbox.alert($.t('Error retrieving Matter nodes'));
			});
		}

		$ctrl.commissionNode = function () {
			$.ajax({
				url: 'json.htm?type=command&param=mattercommissionnode&idx=' + $.devIdx,
				async: true,
				dataType: 'json'
			}).done(function (data) {
				if (data.status === 'ERR') {
					bootbox.alert(data.message);
				} else {
					RefreshMatterNodeTable();
				}
			}).fail(function (jqXHR) {
				var msg = (jqXHR.responseJSON && jqXHR.responseJSON.message) ? jqXHR.responseJSON.message : $.t('Commission node request failed');
				bootbox.alert(msg);
			});
		};

		$ctrl.excludeNode = function () {
			$.ajax({
				url: 'json.htm?type=command&param=matterexcludenode&idx=' + $.devIdx,
				async: true,
				dataType: 'json'
			}).done(function (data) {
				if (data.status === 'ERR') {
					bootbox.alert(data.message);
				} else {
					RefreshMatterNodeTable();
				}
			}).fail(function (jqXHR) {
				var msg = (jqXHR.responseJSON && jqXHR.responseJSON.message) ? jqXHR.responseJSON.message : $.t('Exclude node request failed');
				bootbox.alert(msg);
			});
		};

		$ctrl.deleteNode = function () {
			if (selectedNodeId === -1) {
				return;
			}
			bootbox.confirm($.t('Are you sure you want to delete this node?'), function (result) {
				if (result === true) {
					$.ajax({
						url: 'json.htm?type=command&param=deletematternode&idx=' + $.devIdx + '&node=' + selectedNodeId,
						async: true,
						dataType: 'json'
					}).done(function (data) {
						if (data.status === 'ERR') {
							bootbox.alert(data.message);
						} else {
							RefreshMatterNodeTable();
						}
					}).fail(function () {
						bootbox.alert($.t('Delete node request failed'));
					});
				}
			});
		};

		$ctrl.refreshNodeInfo = function () {
			if (selectedNodeId === -1) {
				return;
			}
			$.ajax({
				url: 'json.htm?type=command&param=requestmatternodeinfo&idx=' + $.devIdx + '&node=' + selectedNodeId,
				async: true,
				dataType: 'json'
			}).done(function (data) {
				if (data.status === 'ERR') {
					bootbox.alert(data.message);
				}
			}).fail(function (jqXHR) {
				var msg = (jqXHR.responseJSON && jqXHR.responseJSON.message) ? jqXHR.responseJSON.message : $.t('Refresh node info request failed');
				bootbox.alert(msg);
			});
		};

		$ctrl.refreshTable = function () {
			RefreshMatterNodeTable();
		};
	}

});
