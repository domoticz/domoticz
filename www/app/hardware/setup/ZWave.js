define(['app'], function (app) {

	app.component('zWaveHardware', {
		bindings: {
			hardware: '<'
		},
		templateUrl: 'app/hardware/setup/ZWave.html',
		controller: ZWaveHardwareController
	});

	function ZWaveHardwareController($scope, $interval, $http, dataTableDefaultSettings) {
		var $ctrl = this;

		getNodeName = function (node) {
			return node ? node.nodeName : '';
		};
		getGroupName = function (group) {
			return group ? group.groupName : '';
		};
		removeNodeFromGroup = function (event) {
			var node = event.data.node;
			var group = event.data.group;
			var removeNode = event.data.removeNode;
			var removeNodeId = event.data.removeNodeId;
			bootbox.confirm($.t(
				'Are you sure you want to remove this node?<br>'
				+ 'Node: ' + getNodeName(removeNode)
				+ ' (' + removeNodeId
				+ ')<br>From group: ' + getGroupName(group)
				+ '<br>On node ' + getNodeName(node)), function (result) {
					if (result === true) {
						$http({
							url: "json.htm?type=command&param=zwaveremovegroupnode&idx=" + $.devIdx + "&node=" + node.nodeID + "&group=" + group.id + "&removenode=" + removeNodeId,
							async: true,
							dataType: 'json'
						}).then(function successCallback(response) {
							bootbox.alert($.t('Groups updated!'));
							$("#grouptable").empty();
							RefreshGroupTable();
						}, function errorCallback(response) {
						});
					}
				});
		};

		addNodeToGroup = function (event) {
			var node = event.data.node;
			var group = event.data.group;
			bootbox.dialog({
				message: $.t("Please enter the node to add to this group:") + "<input type='text' id='add_node' data-toggle='tooltip' title='NodeId or NodeId.Instance'></input><br>" + group.groupName,
				title: 'Add node to ' + getNodeName(node),
				buttons: {
					main: {
						label: $.t("Save"),
						className: "btn-primary",
						callback: function () {
							var addnode = $("#add_node").val();
							$http({
								url: "json.htm?type=command&param=zwaveaddgroupnode&idx=" + $.devIdx + "&node=" + node.nodeID + "&group=" + group.id + "&addnode=" + addnode,
								async: true,
								dataType: 'json'
							}).then(function successCallback(response) {
								RefreshGroupTable();
							}, function errorCallback(response) {
							});
						}
					},
					nothanks: {
						label: $.t("Cancel"),
						className: "btn-cancel",
						callback: function () {
						}
					}
				}
			});
		};

		RefreshGroupTable = function () {
			$http({
				url: "json.htm?type=command&param=zwavegroupinfo&idx=" + $.devIdx,
				async: true,
				dataType: 'json'
			}).then(function successCallback(response) {
				var data = response.data;
				if (typeof data !== 'undefined') {
					if (data.status === "OK") {
						if (typeof data.result !== 'undefined') {

							if ($.fn.dataTable.isDataTable('#grouptable')) {
								oTable = $('#grouptable').DataTable();
								oTable.destroy();
							}

							var i;
							$("#grouptable").empty();
							grouptable = $('#grouptable');
							var maxgroups = data.result.MaxNoOfGroups;
							var thead = $('<thead></thead>');
							var trow = $('<tr style="height: 35px;"></tr>');

							$('<th></th>').text($.t("Node")).appendTo(trow);
							$('<th></th>').text($.t("Name")).appendTo(trow);
							for (i = 0; i < maxgroups; i++) {
								$('<th></th>').text($.t("Group") + " " + (i + 1)).appendTo(trow);
							}
							trow.appendTo(thead);
							thead.appendTo(grouptable);
							tbody = $('<tbody></tbody>');

							var jsonnodes = data.result.nodes;
							var nodes = {};
							i = jsonnodes.length;
							while (i--) {
								var node = jsonnodes[i];
								nodes[node.nodeID] = node;
							}
							$.each(jsonnodes, function (i, node) {
								var groupsDone = 0;
								noderow = $('<tr>');
								var nodeStr = addLeadingZeros(node.nodeID, 3) + " (0x" + addLeadingZeros(node.nodeID.toString(16), 2) + ")";
								$('<td>').text(nodeStr).appendTo(noderow);
								$('<td>').text(node.nodeName).appendTo(noderow);
								noGroups = node.groupCount;
								if (noGroups > 0) {
									groupnodes = node.groups;
									$.each(groupnodes, function (g, group) {
										td = $('<td>').data('toggle', 'tooltip').attr('title', getGroupName(group));

										function createButton(text) {
											var button = $('<span>');
											button.text(text);
											button.addClass('label label-info lcursor');
											return button;
										}

										$.each(group.nodes.split(','), function () {
											if (this !== "") {
												var button = createButton(this);
												var removeNodeId = this * 1;
												var removeNode = nodes[Math.round(removeNodeId)];
												button.click({ node: node, group: group, removeNode: removeNode, removeNodeId: this }, removeNodeFromGroup);
												button.attr('title', getNodeName(removeNode) + ' on ' + getGroupName(group));
												button.appendTo(td);
											}
										});
										var button = createButton('+');
										button.click({ node: node, group: group }, addNodeToGroup);
										button.appendTo(td);
										td.appendTo(noderow);
										groupsDone++;
									});
								}
								if (groupsDone < maxgroups) {
									var missing = maxgroups - noGroups;
									for (i = 0; i < missing; i++) {
										$('<td></td>').text(' ').appendTo(noderow);
										groupsDone++;
									}
								}
								noderow.appendTo(tbody);
							});
							tbody.appendTo(grouptable);
							tfoot = $('<tfoot></tfoot>');
							tfoot.appendTo(grouptable);

							$('#grouptable').DataTable(Object.assign({}, dataTableDefaultSettings, {
								"sDom": '<"H"lfrC>t<"F"ip>',
								"oTableTools": {
									"sRowSelect": "single"
								},
								"aaSorting": [[0, "desc"]],
								"bSortClasses": false,
								"bProcessing": true,
								"bStateSave": true,
								"bJQueryUI": true,
								"aLengthMenu": [[25, 50, 100, -1], [25, 50, 100, "All"]],
								"iDisplayLength": 25,
								"sPaginationType": "full_numbers",
								language: $.DataTableLanguage
							}));
						}
					}
				}
			}, function errorCallback(response) {
			});
		};

		RefreshNeighbours = function () {
			$.depChart = $('#networkchart');
			$.depChart.highcharts({
				chart: {
					type: 'spline',
					zoomType: 'x',
					resetZoomButton: {
						position: {
							x: -30,
							y: -36
						}
					},
					events: {
						load: function () {
							$http({
								url: "json.htm?type=command&param=zwavenetworkinfo&idx=" + $.devIdx,
								async: true,
								dataType: 'json'
							}).then(function successCallback(response) {
								var data = response.data;
								if (typeof data !== 'undefined') {
									if (data.status === "OK") {
										if (typeof data.result !== 'undefined') {
											var datachart = [];
											$.each(data.result.mesh, function (i, item) {
												if (item.seesNodes) {
													var seesNodes = item.seesNodes.split(',');
													for (sn = 0; sn < seesNodes.length; sn++) {
														datachart.push({ from: item.nodeID, to: seesNodes[sn], weight: 1 });
													}
												} else {
													//node is stand alone
													datachart.push({ from: item.nodeID, to: item.nodeID, weight: 1 });

												}
											});
											var series = $.depChart.highcharts().series[0];
											series.setData(datachart);
										}
									}
								}
							}, function errorCallback(response) {
							});
						}
					}
				},
				title: {
					text: null
				},
				tooltip: {
					enabled: false
				},
				series: [{
					keys: ['from', 'to', 'weight'],
					type: 'dependencywheel',
					name: 'Node Neighbors',
					dataLabels: {
						color: '#fff',
						textPath: {
							enabled: true,
							attributes: {
								dy: 5
							}
						},
						distance: 10
					},
					size: '95%'
				}]
			});
		};

		$ctrl.$onInit = function () {
			$scope.ozw_node_id = "-";
			$scope.ozw_node_desc = "-";

			EditOpenZWave($ctrl.hardware.idx, $ctrl.hardware.Name)
		};

		$scope.ZWaveCheckIncludeReady = function () {
			if (typeof $scope.mytimer !== 'undefined') {
				$interval.cancel($scope.mytimer);
				$scope.mytimer = undefined;
			}
			$http({
				url: "json.htm?type=command&param=zwaveisnodeincluded&idx=" + $.devIdx,
				async: true,
				dataType: 'json'
			}).then(function successCallback(response) {
				var data = response.data;
				if (data.status === "OK") {
					if (data.result === true) {
						//Node included
						$scope.ozw_node_id = data.node_id;
						$scope.ozw_node_desc = data.node_product_name;
						$("#IncludeZWaveDialog #izwd_waiting").hide();
						$("#IncludeZWaveDialog #izwd_result").show();
					}
					else {
						//Not ready yet
						$scope.mytimer = $interval(function () {
							$scope.ZWaveCheckIncludeReady();
						}, 1000);
					}
				}
				else {
					$scope.mytimer = $interval(function () {
						$scope.ZWaveCheckIncludeReady();
					}, 1000);
				}
			}, function errorCallback(response) {
					$scope.mytimer = $interval(function () {
						$scope.ZWaveCheckIncludeReady();
					}, 1000);
			});
		};

		OnZWaveAbortInclude = function () {
			$http({
				url: "json.htm?type=command&param=zwavecancel&idx=" + $.devIdx,
				async: true,
				dataType: 'json'
			}).then(function successCallback(response) {
				$('#IncludeZWaveDialog').modal('hide');
			}, function errorCallback(response) {
					$('#IncludeZWaveDialog').modal('hide');
			});
		};

		OnZWaveCloseInclude = function () {
			$('#IncludeZWaveDialog').modal('hide');
			RefreshOpenZWaveNodeTable();
		};

		ZWaveIncludeNode = function (isSecure) {
			if (typeof $scope.mytimer !== 'undefined') {
				$interval.cancel($scope.mytimer);
				$scope.mytimer = undefined;
			}
			$("#IncludeZWaveDialog #izwd_waiting").show();
			$("#IncludeZWaveDialog #izwd_result").hide();
			$http({
				url: "json.htm?type=command&param=zwaveinclude&idx=" + $.devIdx + "&secure=" + isSecure,
				async: true,
				dataType: 'json'
			}).then(function successCallback(response) {
				var data = response.data;
				$scope.ozw_node_id = "-";
				$scope.ozw_node_desc = "-";
				$('#IncludeZWaveDialog').modal('show');
				$scope.mytimer = $interval(function () {
					$scope.ZWaveCheckIncludeReady();
				}, 1000);
			}, function errorCallback(response) {
			});
		};

		$scope.ZWaveCheckExcludeReady = function () {
			if (typeof $scope.mytimer !== 'undefined') {
				$interval.cancel($scope.mytimer);
				$scope.mytimer = undefined;
			}
			$http({
				url: "json.htm?type=command&param=zwaveisnodeexcluded&idx=" + $.devIdx,
				async: true,
				dataType: 'json'
			}).then(function successCallback(response) {
				var data = response.data;
				if (data.status === "OK") {
					if (data.result === true) {
						//Node excluded
						$scope.ozw_node_id = data.node_id;
						$scope.ozw_node_desc = "-";
						$("#ExcludeZWaveDialog #ezwd_waiting").hide();
						$("#ExcludeZWaveDialog #ezwd_result").show();
					}
					else {
						//Not ready yet
						$scope.mytimer = $interval(function () {
							$scope.ZWaveCheckExcludeReady();
						}, 1000);
					}
				}
				else {
					$scope.mytimer = $interval(function () {
						$scope.ZWaveCheckExcludeReady();
					}, 1000);
				}
			}, function errorCallback(response) {
					$scope.mytimer = $interval(function () {
						$scope.ZWaveCheckExcludeReady();
					}, 1000);
			});
		};

		OnZWaveAbortExclude = function () {
			$http({
				url: "json.htm?type=command&param=zwavecancel&idx=" + $.devIdx,
				async: true,
				dataType: 'json'
			}).then(function successCallback(response) {
				$('#ExcludeZWaveDialog').modal('hide');
			}, function errorCallback(response) {
					$('#ExcludeZWaveDialog').modal('hide');
			});
		};

		OnZWaveCloseExclude = function () {
			$('#ExcludeZWaveDialog').modal('hide');
			RefreshOpenZWaveNodeTable();
		};

		ZWaveExcludeNode = function () {
			if (typeof $scope.mytimer !== 'undefined') {
				$interval.cancel($scope.mytimer);
				$scope.mytimer = undefined;
			}
			$("#ExcludeZWaveDialog #ezwd_waiting").show();
			$("#ExcludeZWaveDialog #ezwd_result").hide();
			$http({
				url: "json.htm?type=command&param=zwaveexclude&idx=" + $.devIdx,
				async: true,
				dataType: 'json'
			}).then(function successCallback(response) {
				var data = response.data;
				$scope.ozw_node_id = data.node_id;
				$scope.ozw_node_desc = "-";
				$('#ExcludeZWaveDialog').modal('show');
				$scope.mytimer = $interval(function () {
					$scope.ZWaveCheckExcludeReady();
				}, 1000);
			}, function errorCallback(response) {
			});
		};

		DeleteNode = function (idx) {
			if ($('#updelclr #nodedelete').attr("class") === "btnstyle3-dis") {
				return;
			}
			bootbox.confirm($.t("Are you sure to remove this Node?"), function (result) {
				if (result === true) {
					$http({
						url: "json.htm?type=command&param=deletezwavenode&idx=" + idx,
						async: true,
						dataType: 'json'
					}).then(function successCallback(response) {
						bootbox.alert($.t('Node marked for Delete. This could take some time!'));
						RefreshOpenZWaveNodeTable();
					}, function errorCallback(response) {
					});
				}
			});
		};

		UpdateNode = function (idx) {
			if ($('#updelclr #nodeupdate').attr("class") === "btnstyle3-dis") {
				return;
			}
			var name = $("#hardwarecontent #nodeparamstable #nodename").val();
			if (name === "") {
				ShowNotify($.t('Please enter a Name!'), 2500, true);
				return;
			}

			var bEnablePolling = $('#hardwarecontent #nodeparamstable #EnablePolling').is(":checked");
			$http({
				url: "json.htm?type=command&param=updatezwavenode" +
					"&idx=" + idx +
					"&name=" + encodeURIComponent(name) +
					"&EnablePolling=" + bEnablePolling,
				async: true,
				dataType: 'json'
			}).then(function successCallback(response) {
				RefreshOpenZWaveNodeTable();
			}, function errorCallback(response) {
					ShowNotify($.t('Problem updating Node!'), 2500, true);
			});
		};

		//Request Node Information Frame
		RefreshNode = function (idx) {
			if ($('#updelclr #noderefresh').attr("class") === "btnstyle3-dis") {
				return;
			}
			$http({
				url: "json.htm?type=command&param=requestzwavenodeinfo&idx=" + idx,
				async: true,
				dataType: 'json'
			}).then(function successCallback(response) {
				bootbox.alert($.t('Node Information Frame requested. This could take some time! (You might need to wake-up the node!)'));
			}, function errorCallback(response) {
			});
		};

		RequestZWaveConfiguration = function (idx) {
			$http({
				url: "json.htm?type=command&param=requestzwavenodeconfig&idx=" + idx,
				async: true,
				dataType: 'json'
			}).then(function successCallback(response) {
				bootbox.alert($.t('Configuration requested from Node. If the Node is asleep, this could take a while!'));
				RefreshOpenZWaveNodeTable();
			}, function errorCallback(response) {
					ShowNotify($.t('Problem requesting Node Configuration!'), 2500, true);
			});
		};

		ApplyZWaveConfiguration = function (idx) {
			var valueList = "";
			var $list = $('#hardwarecontent #configuration input');
			var ControlCntl
			if (typeof $list !== 'undefined') {
				ControlCnt = $list.length;
				// Now loop through list of controls

				$list.each(function () {
					var id = $(this).prop("id");      // get id
					var value = encodeURIComponent(btoa($(this).prop("value")));      // get value

					valueList += id + "_" + value + "_";
				});
			}

			//Now for the Lists
			$list = $('#hardwarecontent #configuration select');
			if (typeof $list !== 'undefined') {
				ControlCnt = $list.length;
				// Now loop through list of controls
				$list.each(function () {
					var id = $(this).prop("id");      // get id
					var value = encodeURIComponent(btoa($(this).find(":selected").text()));      // get value
					valueList += id + "_" + value + "_";
				});
			}

			if (valueList !== "") {
				$http({
					url: "json.htm?type=command&param=applyzwavenodeconfig" +
						"&idx=" + idx +
						"&valuelist=" + valueList,
					async: true,
					dataType: 'json'
				}).then(function successCallback(response) {
					bootbox.alert($.t('Configuration sent to node. If the node is asleep, this could take a while!'));
				}, function errorCallback(response) {
						ShowNotify($.t('Problem updating Node Configuration!'), 2500, true);
				});
			}
		};

		ZWaveSoftResetNode = function () {
			$http({
				url: "json.htm?type=command&param=zwavesoftreset&idx=" + $.devIdx,
				async: true,
				dataType: 'json'
			}).then(function successCallback(response) {
				bootbox.alert($.t('Soft resetting controller device...!'));
			}, function errorCallback(response) {
			});
		};

		ZWaveHardResetNode = function () {
			bootbox.confirm($.t("Are you sure you want to hard reset the controller?\n(All associated nodes will be removed)"), function (result) {
				if (result === true) {
					$http({
						url: "json.htm?type=command&param=zwavehardreset&idx=" + $.devIdx,
						async: true,
						dataType: 'json'
					}).then(function successCallback(response) {
						bootbox.alert($.t('Hard resetting controller device...!'));
					}, function errorCallback(response) {
					});
				}
			});
		};

		ZWaveHealNetwork = function () {
			$http({
				url: "json.htm?type=command&param=zwavenetworkheal&idx=" + $.devIdx,
				async: true,
				dataType: 'json'
			}).then(function successCallback(response) {
				var data = response.data;
			}, function errorCallback(response) {
			});
		};

		ZWaveHealNode = function (node) {
			$http({
				url: "json.htm?type=command&param=zwavenodeheal&idx=" + $.devIdx + "&node=" + node,
				async: true,
				dataType: 'json'
			}).then(function successCallback(response) {
				bootbox.alert($.t('Initiating node heal...!'));
			}, function errorCallback(response) {
			});
		};

		ZWaveReceiveConfiguration = function () {
			$http({
				url: "json.htm?type=command&param=zwavereceiveconfigurationfromothercontroller&idx=" + $.devIdx,
				async: true,
				dataType: 'json'
			}).then(function successCallback(response) {
				bootbox.alert($.t('Initiating Receive Configuration...!'));
			}, function errorCallback(response) {
			});
		};

		ZWaveSendConfiguration = function () {
			$http({
				url: "json.htm?type=command&param=zwavesendconfigurationtosecondcontroller&idx=" + $.devIdx,
				async: true,
				dataType: 'json'
			}).then(function successCallback(response) {
				bootbox.alert($.t('Initiating Send Configuration...!'));
			}, function errorCallback(response) {
			});
		};

		ZWaveTransferPrimaryRole = function () {
			bootbox.confirm($.t("Are you sure you want to transfer the primary role?"), function (result) {
				if (result === true) {
					$http({
						url: "json.htm?type=command&param=zwavetransferprimaryrole&idx=" + $.devIdx,
						async: true,
						dataType: 'json'
					}).then(function successCallback(response) {
						bootbox.alert($.t('Initiating Transfer Primary Role...!'));
					}, function errorCallback(response) {
					});
				}
			});
		};

		RefreshOpenZWaveNodeTable = function () {
			$('#modal').show();

			$('#updelclr #nodeupdate').attr("class", "btnstyle3-dis");
			$('#updelclr #nodedelete').attr("class", "btnstyle3-dis");
			$('#updelclr #noderefresh').attr("class", "btnstyle3-dis");

			$("#hardwarecontent #configuration").html("");
			$("#hardwarecontent #nodeparamstable #nodename").val("");

			var oTable = $('#nodestable').dataTable();
			oTable.fnClearTable();

			$http({
				url: "json.htm?type=openzwavenodes&idx=" + $.devIdx,
				async: true,
				dataType: 'json'
			}).then(function successCallback(response) {
				var data = response.data;
				if (typeof data.result !== 'undefined') {

					if (data.NodesQueried === true) {
						$("#zwavenodesqueried").hide();
					}
					else {
						$("#zwavenodesqueried").show();
					}

					$("#ownNodeId").val(String(data.ownNodeId));

					$.each(data.result, function (i, item) {
						var status = "ok";
						if (item.State === "Dead") {
							status = "failed";
						}
						else if ((item.State === "Sleep") || (item.State === "Sleeping")) {
							status = "sleep";
						}
						else if (item.State === "Unknown") {
							status = "unknown";
						}
						var statusImg = '<img src="images/' + status + '.png" />';
						var healButton = '<img src="images/heal.png" onclick="ZWaveHealNode(' + item.NodeID + ')" class="lcursor" title="' + $.t("Heal node") + '" />';
						var Description = item.Description;
						if (Description.length < 2) {
							Description = '<span class="zwave_no_info">' + item.Generic_type + '</span>';
						}
						if (item.IsPlus === true) {
							Description += "+";
						}
						var nodeStr = addLeadingZeros(item.NodeID, 3) + " (0x" + addLeadingZeros(item.NodeID.toString(16), 2) + ")";
						var addId = oTable.fnAddData({
							"DT_RowId": item.idx,
							"Name": item.Name,
							"PollEnabled": item.PollEnabled,
							"Config": item.config,
							"State": item.State,
							"NodeID": item.NodeID,
							"HaveUserCodes": item.HaveUserCodes,
							"0": nodeStr,
							"1": item.Name,
							"2": Description,
							"3": item.Manufacturer_name,
							"4": item.Product_id,
							"5": item.Product_type,
							"6": item.LastUpdate,
							"7": $.t((item.PollEnabled === "true") ? "Yes" : "No"),
							"8": statusImg + '&nbsp;&nbsp;' + healButton,
						});
					});
				}
			}, function errorCallback(response) {
			});

			/* Add a click handler to the rows - this could be used as a callback */
			$("#nodestable tbody").off();
			$("#nodestable tbody").on('click', 'tr', function () {
				if ($(this).hasClass('row_selected')) {
					$(this).removeClass('row_selected');
					$('#updelclr #nodeupdate').attr("class", "btnstyle3-dis");
					$('#updelclr #nodedelete').attr("class", "btnstyle3-dis");
					$('#updelclr #noderefresh').attr("class", "btnstyle3-dis");
					$("#hardwarecontent #configuration").html("");
					$("#hardwarecontent #nodeparamstable #nodename").val("");
					$('#hardwarecontent #usercodegrp').hide();
				}
				else {
					var iOwnNodeId = parseInt($("#ownNodeId").val());
					var oTable = $('#nodestable').dataTable();
					oTable.$('tr.row_selected').removeClass('row_selected');
					$(this).addClass('row_selected');
					$('#updelclr #nodeupdate').attr("class", "btnstyle3");
					$('#updelclr #noderefresh').attr("class", "btnstyle3");
					var anSelected = fnGetSelected(oTable);
					if (anSelected.length !== 0) {
						var data = oTable.fnGetData(anSelected[0]);
						var idx = data["DT_RowId"];
						var iNode = parseInt(data["NodeID"]);
						$("#updelclr #nodeupdate").attr("href", "javascript:UpdateNode(" + idx + ")");
						$('#updelclr #noderefresh').attr("href", "javascript:RefreshNode(" + idx + ")");

						$("#hardwarecontent #zwavecodemanagement").attr("href", "javascript:ZWaveUserCodeManagement(" + idx + ")");
						if (iNode !== iOwnNodeId) {
							$('#updelclr #nodedelete').attr("class", "btnstyle3");
							$("#updelclr #nodedelete").attr("href", "javascript:DeleteNode(" + idx + ")");
						}
						$("#hardwarecontent #nodeparamstable #nodename").val(data["Name"]);
						$('#hardwarecontent #nodeparamstable #EnablePolling').prop('checked', (data["PollEnabled"] === "true"));
						if (iNode === iOwnNodeId) {
							$("#hardwarecontent #nodeparamstable #trEnablePolling").hide();
						}
						else {
							$("#hardwarecontent #nodeparamstable #trEnablePolling").show();
						}

						if (data["HaveUserCodes"] === true) {
							$('#hardwarecontent #usercodegrp').show();
						}
						else {
							$('#hardwarecontent #usercodegrp').hide();
						}

						var szConfig = "";
						if (typeof data["Config"] !== 'undefined') {
							//Create configuration setup
							szConfig = '<a class="btnstyle3" id="noderequeststoredvalues" data-i18n="RequestConfiguration" onclick="RequestZWaveConfiguration(' + idx + ');">Request current stored values from device</a><br /><br />';
							var bHaveConfiguration = false;
							$.each(data["Config"], function (i, item) {
								bHaveConfiguration = true;
								szConfig += '<span class="zwave_label">' + item.index + ". " + item.label + ":</span>";
								var szComboOption;
								if (item.type === "list") {
									szConfig += '&nbsp;<select style="width:auto" class="combobox ui-corner-all" id="' + item.index + '">';
									var iListItem = 0;
									var totListItems = parseInt(item.list_items);
									for (iListItem = 0; iListItem < totListItems; iListItem++) {
										szComboOption = '<option value="' + iListItem + '"';
										var szOptionTxt = item.listitem[iListItem];
										if (szOptionTxt === item.value) {
											szComboOption += ' selected="selected"';
										}
										szComboOption += '>' + szOptionTxt + '</option>';
										szConfig += szComboOption;
									}
									szConfig += '</select>';
									if (item.units !== "") {
										szConfig += ' (' + item.units + ')';
									}
								}
								else if (item.type === "bool") {
									szConfig += '<br><select style="width:100%" class="combobox ui-corner-all" id="' + item.index + '">';

									szComboOption = '<option value="False"';
									if (item.value === "False") {
										szComboOption += ' selected="selected"';
									}
									szComboOption += '>False</option>';
									szConfig += szComboOption;

									szComboOption = '<option value="True"';
									if (item.value === "True") {
										szComboOption += ' selected="selected"';
									}
									szComboOption += '>True</option>';
									szConfig += szComboOption;

									szConfig += '</select>';
									if (item.units !== "") {
										szConfig += ' (' + item.units + ')';
									}
								}
								else if (item.type === "string") {
									szConfig += '<br><input type="text" id="' + item.index + '" value="' + item.value + '" style="width: 600px; padding: .2em;" class="text ui-widget-content ui-corner-all" /><br>';

									if (item.units !== "") {
										szConfig += ' (' + item.units + ')';
									}
									szConfig += " (" + $.t("actual") + ": " + item.value + ")";
								}
								else {
									szConfig += '&nbsp;<input type="text" id="' + item.index + '" value="' + item.value + '" style="width: 50px; padding: .2em;" class="text ui-widget-content ui-corner-all" />';
									if (item.units !== "") {
										szConfig += ' (' + item.units + ')';
									}
									szConfig += " (" + $.t("actual") + ": " + item.value + ")";
								}
								szConfig += "<br /><br />";
								if (item.help !== "") {
									szConfig += '<span class="zwave_help">' + item.help + '</span><br>';
								}
								if (item.LastUpdate.length > 1) {
									szConfig += '<span class="zwave_last_update">Last Update: ' + item.LastUpdate + '</span><br />';
								}
								szConfig += "<br />";
							});
							if (bHaveConfiguration === true) {
								szConfig += '<a class="btnstyle3" id="nodeapplyconfiguration" data-i18n="ApplyConfiguration" onclick="ApplyZWaveConfiguration(' + idx + ');" >Apply configuration for this device</a><br />';
							}
						}
						$("#hardwarecontent #configuration").html(szConfig);
						$("#hardwarecontent #configuration").i18n();
					}
				}
			});
			RefreshGroupTable();
			RefreshNeighbours();

			$('#modal').hide();
		};

		ZWaveStartUserCodeEnrollment = function () {
			$http({
				url: "json.htm?type=command&param=zwavestartusercodeenrollmentmode&idx=" + $.devIdx,
				async: true,
				dataType: 'json'
			}).then(function successCallback(response) {
				bootbox.alert($.t('User Code Enrollment started. You have 30 seconds to include the new key...!'));
			}, function errorCallback(response) {
			});
		};

		RemoveUserCode = function (index) {
			bootbox.confirm($.t("Are you sure to delete this User Code?"), function (result) {
				if (result === true) {
					$http({
						url: "json.htm?type=command&param=zwaveremoveusercode&idx=" + $.nodeIdx + "&codeindex=" + index,
						async: true,
						dataType: 'json'
					}).then(function successCallback(response) {
						RefreshHardwareTable();
					}, function errorCallback(response) {
					});
				}
			});
		};

		RefreshOpenZWaveUserCodesTable = function () {
			$('#modal').show();

			var oTable = $('#codestable').dataTable();
			oTable.fnClearTable();
			$http({
				url: "json.htm?type=command&param=zwavegetusercodes&idx=" + $.nodeIdx,
				async: true,
				dataType: 'json'
			}).then(function successCallback(response) {
				var data = response.data;
				if (typeof data.result !== 'undefined') {
					$.each(data.result, function (i, item) {
						var removeButton = '<span class="label label-info lcursor" onclick="RemoveUserCode(' + item.index + ');">Remove</span>';
						var addId = oTable.fnAddData({
							"DT_RowId": item.index,
							"Code": item.index,
							"Value": item.code,
							"0": item.index,
							"1": item.code,
							"2": removeButton
						});
					});
				}
			}, function errorCallback(response) {
			});
			/* Add a click handler to the rows - this could be used as a callback */
			$("#codestable tbody").off();
			$("#codestable tbody").on('click', 'tr', function () {
				if ($(this).hasClass('row_selected')) {
					$(this).removeClass('row_selected');
				}
				else {
					var oTable = $('#codestable').dataTable();
					oTable.$('tr.row_selected').removeClass('row_selected');
					$(this).addClass('row_selected');
					var anSelected = fnGetSelected(oTable);
					if (anSelected.length !== 0) {
						var data = oTable.fnGetData(anSelected[0]);
						//var idx= data["DT_RowId"];
					}
				}
			});
			$('#modal').hide();
		};

		ZWaveUserCodeManagement = function (idx) {
			$.nodeIdx = idx;
			cursordefault();
			var htmlcontent = '';
			htmlcontent += $('#openzwaveusercodes').html();
			var bString = "EditOpenZWave(" + $.devIdx + ",'" + $.devName + "',0,0,0,0,0)";
			$('#hardwarecontent').html(GetBackbuttonHTMLTable(bString) + htmlcontent);
			$('#hardwarecontent').i18n();
			$('#hardwarecontent #nodeidx').val(idx);
			var oTable = $('#codestable').dataTable({
				"sDom": '<"H"lfrC>t<"F"ip>',
				"oTableTools": {
					"sRowSelect": "single"
				},
				"aaSorting": [[0, "desc"]],
				"bSortClasses": false,
				"bProcessing": true,
				"bStateSave": true,
				"bJQueryUI": true,
				"aLengthMenu": [[25, 50, 100, -1], [25, 50, 100, "All"]],
				"iDisplayLength": 25,
				"sPaginationType": "full_numbers",
				language: $.DataTableLanguage
			});
			RefreshOpenZWaveUserCodesTable();
		};

		EditOpenZWave = function (idx, name) {
			$.devIdx = idx;
			$.devName = name;
			cursordefault();

			var htmlcontent = $('#openzwave').html();
			$('#hardwarecontent').html(htmlcontent);
			$('#hardwarecontent #zwave_configdownload').attr("href", "zwavegetconfig.php?idx=" + idx);
			$('#hardwarecontent').i18n();

			$('#hardwarecontent #usercodegrp').hide();

			var oTable = $('#nodestable').dataTable({
				"sDom": '<"H"lfrC>t<"F"ip>',
				"oTableTools": {
					"sRowSelect": "single",
				},
				"aaSorting": [[0, "desc"]],
				"bSortClasses": false,
				"bProcessing": true,
				"bStateSave": true,
				"bJQueryUI": true,
				"aLengthMenu": [[25, 50, 100, -1], [25, 50, 100, "All"]],
				"iDisplayLength": 25,
				"sPaginationType": "full_numbers",
				language: $.DataTableLanguage
			});

			$('#hardwarecontent #idx').val(idx);

			RefreshOpenZWaveNodeTable();
		};
	}
});
