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
		var oLegendTable;
		var legendNodeMap = {}; // kept at module level so createdRow callback always sees current data
		var hwIdx;
		var chartType = localStorage.getItem('matter_chartType');
		if (chartType !== 'wheel' && chartType !== 'network') chartType = 'wheel';
		var lastGraphData = null;
		var networkAnimFrame = null;

		$ctrl.$onInit = function () {
			hwIdx = $ctrl.hardware.idx;
			RefreshMatterNodeTable();
			$element.find('a[data-target="#mattertabneighbours"]').on('shown.bs.tab', function () {
				RefreshNetworkGraph();
			});
			$element.find('a[data-target="#mattertabsettings"]').on('shown.bs.tab', function () {
				RefreshServerInfo();
			});
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

		function matterAjax(url, onSuccess, failMsg) {
			$.ajax({
				url: url,
				async: true,
				dataType: 'json'
			}).done(function (data) {
				if (data.status === 'ERR') {
					bootbox.alert(data.message);
				} else if (onSuccess) {
					onSuccess();
				}
			}).fail(function (jqXHR) {
				var msg = (jqXHR.responseJSON && jqXHR.responseJSON.message) ? jqXHR.responseJSON.message : $.t(failMsg);
				bootbox.alert(msg);
			});
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
				url: 'json.htm?type=command&param=getmatternodes&idx=' + hwIdx,
				async: true,
				dataType: 'json'
			}).done(function (data) {
				var tbody = $('<tbody></tbody>');

				if (typeof data.result !== 'undefined') {
					$.each(data.result, function (i, item) {
						var now = Math.floor(Date.now() / 1000);
						var oneHourAgo = now - 3600;
						var statusOk = item.available && item.LastSeenTimestamp >= oneHourAgo;

						var tdStatus = $('<td align="center"></td>');
						tdStatus.append($('<img>').attr('src', statusOk ? 'images/ok.png' : 'images/failed.png'));

						// Battery cell
						var tdBattery = $('<td align="center"></td>');
						if (item.Battery !== undefined && item.Battery !== null) {
							var batPct = item.Battery;
							var batIcon = batPct > 75 ? 'images/battery4.png' : batPct > 50 ? 'images/battery3.png' : batPct > 25 ? 'images/battery2.png' : 'images/battery1.png';
							tdBattery.append($('<img>').attr({ src: batIcon, title: batPct + '%' }));
							tdBattery.append($('<span></span>').text(' ' + batPct + '%'));
						}

						// LQI cell
						var tdLqi = $('<td align="center"></td>');
						if (item.LQI !== undefined && item.LQI !== null) {
							var lqi = item.LQI;
							var lqiColor = lqi >= 200 ? '#4CAF50' : lqi >= 100 ? '#FF9800' : '#F44336';
							tdLqi.append($('<span></span>').css('color', lqiColor).text(lqi));
						}

						var trow = $('<tr class="lcursor"></tr>');
						$('<td align="center"></td>').text(item.NodeID).appendTo(trow);
						$('<td></td>').text(item.DeviceNames || '').appendTo(trow);
						$('<td></td>').text(item.VendorName).appendTo(trow);
						$('<td></td>').text(item.ProductName).appendTo(trow);
						$('<td></td>').text(item.LastSeen).appendTo(trow);
						tdBattery.appendTo(trow);
						tdLqi.appendTo(trow);
						tdStatus.appendTo(trow);

						trow.data('nodeId', item.NodeID);
						trow.data('nodeItem', item);
						trow.on('click', function () {
							var wasSelected = $(this).hasClass('row_selected');
							$('#matterNodeTable tbody tr').removeClass('row_selected');
							$(this).addClass('row_selected');
							selectedNodeId = $(this).data('nodeId');
							setNodeButtonsEnabled(true);
							if (wasSelected) showNodeDetailPopup($(this).data('nodeItem'));
						});
						trow.on('dblclick', function () {
							showNodeDetailPopup($(this).data('nodeItem'));
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

		function showNodeDetailPopup(item) {
			function fmtUptime(s) {
				if (!s) return '-';
				var d = Math.floor(s / 86400), h = Math.floor((s % 86400) / 3600), m = Math.floor((s % 3600) / 60);
				var parts = [];
				if (d) parts.push(d + 'd');
				if (h) parts.push(h + 'h');
				if (m || !parts.length) parts.push(m + 'm');
				return parts.join(' ');
			}
			var roleNames = ['Unspecified','Unassigned','SleepyEndDevice','EndDevice','REED','Router','Leader','Controller'];
			var roleName = roleNames[item.RoutingRole] || 'Unknown';

			var rows = [
				['Node ID',          item.NodeID],
				['Vendor',           item.VendorName + (item.VendorId ? ' (' + item.VendorId + ')' : '')],
				['Product',          item.ProductName + (item.ProductId ? ' (' + item.ProductId + ')' : '')],
				['Hardware Version', item.HardwareVersion || '-'],
				['Software Version', item.SoftwareVersion || '-'],
				['Role',             roleName],
				['Uptime',           fmtUptime(item.Uptime)],
				['Last Seen',        item.LastSeen || '-'],
			];
			if (item.Battery !== undefined && item.Battery !== null)
				rows.push(['Battery', item.Battery + '%']);
			if (item.LQI !== undefined && item.LQI !== null)
				rows.push(['LQI', item.LQI]);
			if (item.IPAddresses)
				rows.push(['IP Addresses', item.IPAddresses]);

			var tbl = '<table class="table table-condensed table-bordered" style="margin-bottom:0">';
			$.each(rows, function (i, r) {
				tbl += '<tr><td style="width:140px;font-weight:bold">' + $.t(r[0]) + '</td><td>' + r[1] + '</td></tr>';
			});
			tbl += '</table>';

			bootbox.dialog({
				title: $.t('Node') + ' ' + item.NodeID + ' — ' + item.VendorName + ' ' + item.ProductName,
				message: tbl,
				buttons: { ok: { label: $.t('Close'), className: 'btn-default' } }
			});
		}

		$ctrl.commissionNode = function () {
			var dialog = bootbox.dialog({
				title: $.t('Commission Node'),
				message: '<div class="form-group">' +
					'<label>' + $.t('Enter pairing code') + '</label>' +
					'<input type="text" id="matter_pairing_code" class="form-control" placeholder="1234-456-1023" />' +
					'</div>',
				buttons: {
					cancel: { label: $.t('Cancel'), className: 'btn-default' },
					ok: {
						label: $.t('Commission'),
						className: 'btn-primary',
						callback: function () {
							var code = $('#matter_pairing_code').val().replace(/[\s-]/g, '');
							if (!code) return false;
							$.ajax({
								url: 'json.htm?type=command&param=mattercommissionnode&idx=' + hwIdx + '&code=' + encodeURIComponent(code),
								async: true,
								dataType: 'json'
							}).done(function () {
								startCommissionPolling();
							}).fail(function (jqXHR) {
								var msg = (jqXHR.responseJSON && jqXHR.responseJSON.message) ? jqXHR.responseJSON.message : $.t('Commission node request failed');
								bootbox.alert(msg);
							});
						}
					}
				}
			});
			dialog.on('shown.bs.modal', function () {
				$('#matter_pairing_code').focus();
			});
		};

		function startCommissionPolling() {
			var pollCount = 0;
			var maxPolls = 30; // 60 seconds
			var progressDialog = bootbox.dialog({
				title: $.t('Commissioning Node'),
				message: '<p><i class="fa fa-spinner fa-spin"></i> ' + $.t('Commissioning in progress, please follow device instructions...') + '</p>',
				closeButton: false,
				buttons: {
					cancel: {
						label: $.t('Cancel'),
						className: 'btn-default',
						callback: function () { clearInterval(pollTimer); }
					}
				}
			});
			var pollTimer = setInterval(function () {
				pollCount++;
				if (pollCount > maxPolls) {
					clearInterval(pollTimer);
					progressDialog.modal('hide');
					bootbox.alert($.t('Commissioning timed out'));
					return;
				}
				$.ajax({
					url: 'json.htm?type=command&param=getmattercommissionstatus&idx=' + hwIdx,
					async: true,
					dataType: 'json'
				}).done(function (data) {
					if (data.state === 'Failed') {
						clearInterval(pollTimer);
						progressDialog.modal('hide');
						bootbox.alert(data.message || $.t('Commission failed'));
					} else if (data.state === 'Succeeded') {
						clearInterval(pollTimer);
						progressDialog.modal('hide');
						bootbox.alert(
							'<b>' + $.t('Node commissioned successfully') + '</b><br>' +
							$.t('Node ID') + ': ' + data.nodeId + '<br>' +
							data.vendorName + ' ' + data.productName
						);
						RefreshMatterNodeTable();
					}
				});
			}, 2000);
		}

		$ctrl.excludeNode = function () {
			if (selectedNodeId === -1) {
				bootbox.alert($.t('Please select a node first'));
				return;
			}
			bootbox.confirm($.t('Are you sure you want to remove this node?'), function (result) {
				if (result !== true) return;
				matterAjax(
					'json.htm?type=command&param=matterexcludenode&idx=' + hwIdx + '&node=' + selectedNodeId,
					RefreshMatterNodeTable,
					'Exclude node request failed'
				);
			});
		};

		$ctrl.deleteNode = function () {
			if (selectedNodeId === -1) return;
			bootbox.confirm($.t('Are you sure you want to delete this node?'), function (result) {
				if (result !== true) return;
				matterAjax(
					'json.htm?type=command&param=deletematternode&idx=' + hwIdx + '&node=' + selectedNodeId,
					RefreshMatterNodeTable,
					'Delete node request failed'
				);
			});
		};

		$ctrl.refreshNodeInfo = function () {
			if (selectedNodeId === -1) return;
			matterAjax(
				'json.htm?type=command&param=requestmatternodeinfo&idx=' + hwIdx + '&node=' + selectedNodeId,
				null,
				'Refresh node info request failed'
			);
		};

		$ctrl.refreshTable = function () {
			RefreshMatterNodeTable();
		};

		function RefreshServerInfo() {
			$.ajax({
				url: 'json.htm?type=command&param=mattergetserverinfo&idx=' + hwIdx,
				async: true,
				dataType: 'json'
			}).done(function (data) {
				if (data.status !== 'OK') return;
				$('#matter_sdk_version').text(data.SdkVersion || '-');
				$('#matter_fabric_id').text(data.FabricId || '-');
				$('#matter_compressed_fabric_id').text(data.CompressedFabricId || '-');
				$('#matter_wifi_set').text(data.WifiCredentialsSet ? $.t('Configured') : $.t('Not configured'));
				$('#matter_thread_set').text(data.ThreadCredentialsSet ? $.t('Configured') : $.t('Not configured'));
			});
		}

		$ctrl.setWifiCredentials = function () {
			var ssid  = $('#matter_wifi_ssid').val().trim();
			var creds = $('#matter_wifi_password').val();
			if (!ssid) { bootbox.alert($.t('Please enter WiFi SSID')); return; }
			matterAjax(
				'json.htm?type=command&param=mattersetwificredentials&idx=' + hwIdx +
					'&ssid=' + encodeURIComponent(ssid) + '&credentials=' + encodeURIComponent(creds),
				function () { bootbox.alert($.t('WiFi credentials set')); RefreshServerInfo(); },
				'Set WiFi credentials failed'
			);
		};

		$ctrl.setThreadDataset = function () {
			var dataset = $('#matter_thread_dataset').val().trim();
			if (!dataset) { bootbox.alert($.t('Please enter Thread dataset')); return; }
			matterAjax(
				'json.htm?type=command&param=mattersetthreaddataset&idx=' + hwIdx +
					'&dataset=' + encodeURIComponent(dataset),
				function () { bootbox.alert($.t('Thread dataset set')); RefreshServerInfo(); },
				'Set Thread dataset failed'
			);
		};

		$ctrl.setFabricLabel = function () {
			var label = $('#matter_fabric_label').val().trim();
			matterAjax(
				'json.htm?type=command&param=mattersetfabriclabel&idx=' + hwIdx +
					'&label=' + encodeURIComponent(label),
				function () { bootbox.alert($.t('Fabric label set')); },
				'Set fabric label failed'
			);
		};

		$ctrl.refreshNetworkGraph = function () {
			RefreshNetworkGraph();
		};

		$ctrl.chartType = chartType;
		$ctrl.setChartType = function (type) {
			chartType = type;
			$ctrl.chartType = type;
			localStorage.setItem('matter_chartType', type);
			if (lastGraphData) {
				renderChart(lastGraphData.nodes, lastGraphData.edges, lastGraphData.nodeMap);
			}
		};

		function RefreshNetworkGraph() {
			$.ajax({
				url: 'json.htm?type=command&param=getmatternetworkgraph&idx=' + hwIdx,
				async: true,
				dataType: 'json'
			}).done(function (data) {
				if (data.status !== 'OK') return;

				var nodes = data.nodes || [];
				var edges = data.edges || [];

				// Build legend table — initialize once, update data on subsequent refreshes
				var $legend = $('#matterNeighbourLegend');
				var nodeMap = {};
				legendNodeMap = {};
				$.each(nodes, function (i, node) {
					nodeMap[String(node.NodeID)] = node;
					legendNodeMap[String(node.NodeID)] = node;
				});

				if (!$.fn.dataTable.isDataTable('#matterNeighbourLegend')) {
					oLegendTable = $legend.DataTable({
						bFilter: false,
						bSortClasses: false,
						aaSorting: [[0, 'asc']],
						iDisplayLength: 50,
						createdRow: function (row, data) {
							var node = legendNodeMap[String(data[0])];
							if (!node) return;
							$(row).css('color', roleColor(node.RoutingRole));
						}
					});
				} else {
					oLegendTable = $legend.DataTable();
				}
				oLegendTable.clear();
				$.each(nodes, function (i, node) {
					oLegendTable.row.add([node.NodeID, node.Name, node.RoleName || '']);
				});
				oLegendTable.draw();

				lastGraphData = { nodes: nodes, edges: edges, nodeMap: nodeMap };
				renderChart(nodes, edges, nodeMap);
			}).fail(function () {
				bootbox.alert($.t('Error retrieving Matter network graph'));
			});
		}

		function renderChart(nodes, edges, nodeMap) {
			if (networkAnimFrame) {
				cancelAnimationFrame(networkAnimFrame);
				networkAnimFrame = null;
			}
			$(window).off('resize.mattergraph');
			if (chartType === 'network') {
				renderNetworkGraph(nodes, edges, nodeMap);
			} else {
				renderDependencyWheel(nodes, edges, nodeMap);
			}
		}

		// Builds a bidirectional edge lookup. Both m['a|b'] and m['b|a'] point to the same
		// edge object (undirected). Neighbor edges (fromRoute=false) are preferred over route-table
		// edges (fromRoute=true) because route-table edges carry no real RSSI (hardcoded 0).
		function buildEdgeMap(edges) {
			var m = {};
			$.each(edges, function (i, e) {
				var key = String(e.from) + '|' + String(e.to);
				if (!m[key] || (!e.fromRoute && m[key].fromRoute)) m[key] = e;
				var rev = String(e.to) + '|' + String(e.from);
				if (!m[rev] || (!e.fromRoute && m[rev].fromRoute)) m[rev] = e;
			});
			return m;
		}

		function roleColor(r) {
			if (r === 7)                            return '#FFA726';
			if (r === 6 || r === 5 || r === 4)      return '#42A5F5';
			if (r === 2)                            return '#7E57C2';
			if (r === 3)                            return '#26C6DA';
			return '#90A4AE';
		}

		function buildNodeTipHtml(id, nodeMap, edges) {
			var n = nodeMap[String(id)];
			if (!n) return String(id);
			var html = '<b>' + n.NodeID + ' ' + n.Name + ' (' + n.RoleName + ')</b>';
			var peerBest = {};
			$.each(edges, function (i, e) {
				var f = String(e.from), t = String(e.to);
				if (f !== String(id) && t !== String(id)) return;
				var other = f === String(id) ? t : f;
				if (!peerBest[other] || (!e.fromRoute && peerBest[other].fromRoute)) peerBest[other] = e;
			});
			var links = [];
			$.each(peerBest, function (other, e) {
				var om = nodeMap[other];
				var line = om ? (om.NodeID + ' ' + om.Name) : other;
				line += '&nbsp;&nbsp;LQI: ' + (e.lqi || 0);
				if (!e.fromRoute && e.rssi !== undefined) line += '&nbsp;&nbsp;RSSI: ' + e.rssi + ' dBm';
				links.push(line);
			});
			if (links.length) html += '<hr style="margin:4px 0;border-color:#555">' + links.join('<br>');
			return html;
		}

		function edgeTooltipLine(edgeMap, from, to) {
			var e = edgeMap[String(from) + '|' + String(to)];
			if (!e) return '';
			var parts = [];
			if (e.lqi  !== undefined) parts.push('LQI: ' + e.lqi);
			// Route-table edges carry no real RSSI (hardcoded 0), so skip them
			if (!e.fromRoute && e.rssi !== undefined) parts.push('RSSI: ' + e.rssi + ' dBm');
			return parts.length ? '<br>' + parts.join(' &nbsp; ') : '';
		}

		function renderDependencyWheel(nodes, edges, nodeMap) {
			var $chart = $('#matterNetworkChart');
			$chart.empty();

			var edgeMap = buildEdgeMap(edges);
			var datachart = [];
			$.each(edges, function (i, edge) {
				var weight = edge.lqi > 0 ? Math.round(edge.lqi / 25.5) : 1;
				datachart.push({ from: String(edge.from), to: String(edge.to), weight: weight || 1 });
			});
			var seenIds = {};
			$.each(datachart, function (i, d) { seenIds[d.from] = true; seenIds[d.to] = true; });
			$.each(nodes, function (i, node) {
				var id = String(node.NodeID);
				if (!seenIds[id]) {
					datachart.push({ from: id, to: id, weight: 1 });
				}
			});

			$chart.highcharts({
				chart: { type: 'dependencywheel' },
				title: { text: null },
				tooltip: {
					enabled: true,
					nodeFormatter: function () {
						return buildNodeTipHtml(this.id, nodeMap, edges);
					},
					pointFormatter: function () {
						var from = nodeMap[String(this.from)];
						var to   = nodeMap[String(this.to)];
						return (from ? from.Name : this.from) + ' &#8594; ' + (to ? to.Name : this.to) +
							   edgeTooltipLine(edgeMap, this.from, this.to);
					}
				},
				accessibility: { point: { valueDescriptionFormat: '{index}. {point.from} to {point.to}: {point.weight}.' } },
				plotOptions: { series: { turboThreshold: 0 } },
				series: [{
					keys: ['from', 'to', 'weight'],
					type: 'dependencywheel',
					name: $.t('Thread Network'),
					dataLabels: { color: '#fff', textPath: { enabled: true, attributes: { dy: 5 } }, distance: 10 },
					size: '95%',
					data: datachart
				}]
			});
		}

		function renderNetworkGraph(nodes, edges, nodeMap) {
			// Remove handlers from any previously rendered canvas before emptying the container
			$('#matterNetworkChart').find('canvas').off('mousemove mouseleave mousedown');
			$(document).off('mouseup.mattergraph');

			var $chart = $('#matterNetworkChart');
			$chart.empty().css('position', 'relative');

			var edgeMap = buildEdgeMap(edges);

			// Floating tooltip div
			var $tip = $('<div></div>').css({
				position: 'absolute', background: 'rgba(15,20,35,0.88)', color: '#eee',
				padding: '6px 10px', borderRadius: '5px', fontSize: '12px',
				pointerEvents: 'none', display: 'none', zIndex: 10,
				lineHeight: '1.6', whiteSpace: 'nowrap', boxShadow: '0 2px 8px rgba(0,0,0,0.5)'
			}).appendTo($chart);

			var width  = $chart.width()  || 700;
			var height = $chart.height() || 500;

			var $canvas = $('<canvas></canvas>').attr({ width: width, height: height, 'aria-label': $.t('Matter Thread network topology') });
			$chart.append($canvas);
			var canvas = $canvas[0];
			var ctx    = canvas.getContext('2d');

			// Initial circular layout
			var pos = {}, vel = {};
			var step = (2 * Math.PI) / (nodes.length || 1);
			var R = Math.min(width, height) * 0.32;
			$.each(nodes, function (i, node) {
				var id = String(node.NodeID);
				pos[id] = { x: width / 2 + R * Math.cos(i * step - Math.PI / 2),
				            y: height / 2 + R * Math.sin(i * step - Math.PI / 2) };
				vel[id] = { x: 0, y: 0 };
			});

			var LQI_GOOD = 200, LQI_OK = 100;
			function lqiColor(lqi) {
				if (lqi > LQI_GOOD) return '#4CAF50';
				if (lqi > LQI_OK)   return '#FF9800';
				return '#F44336';
			}

			function nodeColors(node) {
				var r = node.RoutingRole;
				var c = roleColor(r);
				var strokes = { '#FFA726': '#E65100', '#42A5F5': '#1565C0', '#7E57C2': '#4527A0', '#26C6DA': '#00838F', '#90A4AE': '#546E7A' };
				return { fill: c, stroke: strokes[c] || '#546E7A', text: '#fff' };
			}

			var NR = 24; // node radius
			var REPEL = 7000, SPRING = 0.04, REST = 170, DAMP = 0.82, GRAVITY = 0.005;
			var iter = 0, maxIter = 400, dragged = null;

			function tick() {
				iter++;
				var forces = {};
				$.each(nodes, function (i, n) { forces[String(n.NodeID)] = { x: 0, y: 0 }; });

				for (var i = 0; i < nodes.length; i++) {
					for (var j = i + 1; j < nodes.length; j++) {
						var ai = String(nodes[i].NodeID), aj = String(nodes[j].NodeID);
						var dx = pos[aj].x - pos[ai].x, dy = pos[aj].y - pos[ai].y;
						var dist = Math.sqrt(dx * dx + dy * dy) || 0.1;
						var f = REPEL / (dist * dist);
						forces[ai].x -= f * dx / dist; forces[ai].y -= f * dy / dist;
						forces[aj].x += f * dx / dist; forces[aj].y += f * dy / dist;
					}
				}

				$.each(edges, function (i, edge) {
					var af = String(edge.from), at = String(edge.to);
					if (!pos[af] || !pos[at]) return;
					var dx = pos[at].x - pos[af].x, dy = pos[at].y - pos[af].y;
					var dist = Math.sqrt(dx * dx + dy * dy) || 0.1;
					var f = SPRING * (dist - REST);
					forces[af].x += f * dx / dist; forces[af].y += f * dy / dist;
					forces[at].x -= f * dx / dist; forces[at].y -= f * dy / dist;
				});

				var maxV = 0;
				$.each(nodes, function (i, node) {
					var id = String(node.NodeID);
					if (id === dragged) return;
					forces[id].x += GRAVITY * (width  / 2 - pos[id].x);
					forces[id].y += GRAVITY * (height / 2 - pos[id].y);
					vel[id].x = (vel[id].x + forces[id].x) * DAMP;
					vel[id].y = (vel[id].y + forces[id].y) * DAMP;
					pos[id].x = Math.max(NR + 4, Math.min(width  - NR - 4, pos[id].x + vel[id].x));
					pos[id].y = Math.max(NR + 4, Math.min(height - NR - 32, pos[id].y + vel[id].y));
					var v = Math.abs(vel[id].x) + Math.abs(vel[id].y);
					if (v > maxV) maxV = v;
				});

				draw();
				if (iter < maxIter && (maxV > 0.15 || dragged)) {
					networkAnimFrame = requestAnimationFrame(tick);
				} else {
					networkAnimFrame = null;
				}
			}

			function drawLabelBelow(text, x, y) {
				var maxLen = 24;
				var label = text.length > maxLen ? text.substring(0, maxLen - 1) + '\u2026' : text;
				ctx.font = '11px sans-serif';
				ctx.textAlign = 'center';
				ctx.textBaseline = 'top';
				var tw = ctx.measureText(label).width;
				var pad = 4, lh = 15;
				ctx.fillStyle = 'rgba(10,15,30,0.65)';
				ctx.fillRect(x - tw / 2 - pad, y, tw + pad * 2, lh);
				ctx.fillStyle = '#e8e8e8';
				ctx.fillText(label, x, y + 1);
			}

			function draw() {
				ctx.clearRect(0, 0, width, height);
				ctx.save();

				// Edges
				$.each(edges, function (i, edge) {
					var af = String(edge.from), at = String(edge.to);
					if (!pos[af] || !pos[at]) return;
					// Shorten line to node border
					var dx = pos[at].x - pos[af].x, dy = pos[at].y - pos[af].y;
					var len = Math.sqrt(dx * dx + dy * dy) || 1;
					var x1 = pos[af].x + dx / len * NR, y1 = pos[af].y + dy / len * NR;
					var x2 = pos[at].x - dx / len * NR, y2 = pos[at].y - dy / len * NR;
					ctx.beginPath();
					ctx.moveTo(x1, y1);
					ctx.lineTo(x2, y2);
					ctx.strokeStyle = lqiColor(edge.lqi || 0);
					ctx.lineWidth   = edge.fromRoute ? 1.2 : 2.2;
					ctx.globalAlpha = edge.fromRoute ? 0.55 : 0.85;
					ctx.setLineDash(edge.fromRoute ? [5, 4] : []);
					ctx.stroke();
					ctx.globalAlpha = 1;
				});
				ctx.setLineDash([]);

				// Nodes
				$.each(nodes, function (i, node) {
					var id  = String(node.NodeID);
					if (!pos[id]) return;
					var x = pos[id].x, y = pos[id].y;
					var col = nodeColors(node);
					var r   = node.isBorderRouter ? NR + 4 : NR;

					// Drop shadow
					ctx.shadowColor = 'rgba(0,0,0,0.4)';
					ctx.shadowBlur  = id === dragged ? 18 : 6;
					ctx.shadowOffsetY = 2;

					ctx.beginPath();
					ctx.arc(x, y, r, 0, Math.PI * 2);
					ctx.fillStyle   = col.fill;
					ctx.fill();
					ctx.shadowBlur  = 0;
					ctx.shadowOffsetY = 0;
					ctx.strokeStyle = col.stroke;
					ctx.lineWidth   = id === dragged ? 3.5 : 2.5;
					ctx.stroke();

					// ID label inside
					ctx.fillStyle    = col.text;
					ctx.font         = 'bold 12px sans-serif';
					ctx.textAlign    = 'center';
					ctx.textBaseline = 'middle';
					ctx.fillText(node.isBorderRouter ? node.NodeID : String(node.NodeID), x, y);

					// Name label below with background
					drawLabelBelow(node.Name, x, y + r + 5);
				});

				ctx.restore();
			}

			function nodeAt(mx, my) {
				var found = null;
				$.each(nodes, function (i, node) {
					var id = String(node.NodeID);
					if (!pos[id]) return;
					var r  = node.isBorderRouter ? NR + 4 : NR;
					var dx = mx - pos[id].x, dy = my - pos[id].y;
					if (dx * dx + dy * dy <= r * r) { found = id; return false; }
				});
				return found;
			}

			function edgeAt(mx, my) {
				var THR = 7, found = null;
				$.each(edges, function (i, edge) {
					var af = String(edge.from), at = String(edge.to);
					if (!pos[af] || !pos[at]) return;
					var x1 = pos[af].x, y1 = pos[af].y, x2 = pos[at].x, y2 = pos[at].y;
					var dx = x2 - x1, dy = y2 - y1, lenSq = dx * dx + dy * dy;
					var t  = lenSq > 0 ? Math.max(0, Math.min(1, ((mx - x1) * dx + (my - y1) * dy) / lenSq)) : 0;
					var dist = Math.sqrt(Math.pow(mx - x1 - t * dx, 2) + Math.pow(my - y1 - t * dy, 2));
					if (dist < THR) { found = edge; return false; }
				});
				return found;
			}

			function showNodeTip(id, px, py) {
				if (!nodeMap[id]) return;
				positionTip(buildNodeTipHtml(id, nodeMap, edges), px, py);
			}

			function showEdgeTip(edge, px, py) {
				var from = nodeMap[String(edge.from)];
				var to   = nodeMap[String(edge.to)];
				var html = (from ? from.Name : edge.from) + ' \u2194 ' + (to ? to.Name : edge.to) +
				           '<br>LQI: ' + (edge.lqi || 0);
				if (!edge.fromRoute && edge.rssi !== undefined) html += ' &nbsp; RSSI: ' + edge.rssi + ' dBm';
				positionTip(html, px, py);
			}

			// Cache tooltip dimensions to avoid repeated off-screen layout thrashing on every mousemove.
			var tipCache = { html: null, w: 180, h: 80 };
			function positionTip(html, px, py) {
				if (tipCache.html !== html) {
					$tip.html(html).css({ left: -9999, top: -9999, display: 'block' });
					tipCache = { html: html, w: $tip.outerWidth(true) || 180, h: $tip.outerHeight(true) || 80 };
				} else {
					$tip.html(html);
				}
				var tw = tipCache.w, th = tipCache.h;
				var tx = px + 14, ty = py - 10;
				if (tx + tw > width)      tx = px - tw - 14;
				if (tx < 0)               tx = 0;
				if (ty + th > height - 4) ty = py - th - 14;
				if (ty < 0)               ty = 0;
				$tip.css({ left: tx, top: ty, display: 'block' });
			}

			$canvas.on('mousemove', function (e) {
				var rect = canvas.getBoundingClientRect();
				var mx = e.clientX - rect.left, my = e.clientY - rect.top;
				var nid = nodeAt(mx, my);
				if (nid) {
					$canvas.css('cursor', 'grab');
					showNodeTip(nid, mx, my);
				} else {
					var edge = edgeAt(mx, my);
					if (edge) {
						$canvas.css('cursor', 'crosshair');
						showEdgeTip(edge, mx, my);
					} else {
						$canvas.css('cursor', 'default');
						$tip.hide();
					}
				}
				if (dragged && pos[dragged]) {
					pos[dragged].x = Math.max(NR + 4, Math.min(width  - NR - 4, mx));
					pos[dragged].y = Math.max(NR + 4, Math.min(height - NR - 32, my));
					vel[dragged]   = { x: 0, y: 0 };
				}
			});

			$canvas.on('mouseleave', function () { $tip.hide(); });

			$canvas.on('mousedown', function (e) {
				var rect = canvas.getBoundingClientRect();
				var mx = e.clientX - rect.left, my = e.clientY - rect.top;
				var nid = nodeAt(mx, my);
				if (nid) {
					dragged = nid;
					if (!networkAnimFrame) { iter = 0; networkAnimFrame = requestAnimationFrame(tick); }
				}
			});

			$(document).on('mouseup.mattergraph', function () {
				if (dragged) {
					dragged = null;
					iter = 0; // reset so physics gets a fresh budget to settle after drag
				}
			});

			networkAnimFrame = requestAnimationFrame(tick);

			// Resize: update canvas dimensions and redraw with clamped positions
			var resizeTimer = null;
			$(window).on('resize.mattergraph', function () {
				clearTimeout(resizeTimer);
				resizeTimer = setTimeout(function () {
					var newW = $chart.width() || 700;
					var newH = $chart.height() || 500;
					if (newW === width && newH === height) return;
					width = newW; height = newH;
					$canvas.attr({ width: width, height: height });
					$.each(nodes, function (i, node) {
						var id = String(node.NodeID);
						if (!pos[id]) return;
						pos[id].x = Math.max(NR + 4, Math.min(width  - NR - 4,  pos[id].x));
						pos[id].y = Math.max(NR + 4, Math.min(height - NR - 32, pos[id].y));
					});
					if (!networkAnimFrame) draw();
				}, 150);
			});
		}

	}

});
