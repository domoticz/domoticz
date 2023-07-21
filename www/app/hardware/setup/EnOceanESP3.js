define(['app'], function (app) {

	/*
	WARNING :
	The app component name below MUST CORRESPOND to the tag name in HardwareSetup.html converted from kebab to camel case.
	e.g. esp3-hardware (kebab case) in HardwareSetup.html corresponds to esp3Hardware (camel case) in setup/EnOceanESP3.js
	*/

	app.component('esp3Hardware', {
		bindings: {
			hardware: '<'
		},
		templateUrl: 'app/hardware/setup/EnOceanESP3.html',
		controller: ESP3HardwareController
	});

	function ESP3HardwareController($scope, $interval, $http, dataTableDefaultSettings) {
		var $ctrl = this;

		$ctrl.$onInit = function () {
			$.esp3hwdid = $ctrl.hardware.idx;

			cursordefault();

			// Load EnOcean Hardware screen template
			var htmlcontent = $("#enoceansp3").html();
			$("#hardwarecontent").html(htmlcontent);
			$("#hardwarecontent").i18n();

			// Prepare EnOcean Nodes table
			var oTable = $("#nodestable").dataTable({
				"sDom": '<"H"lfrC>t<"F"ip>',
				"oTableTools": {
					"sRowSelect": "single",
				},
				"aaSorting": [[0, "asc"]],
				"bSortClasses": false,
				"bProcessing": true,
				"bStateSave": true,
				"bJQueryUI": true,
				"aLengthMenu": [[25, 50, 100, -1], [25, 50, 100, "All"]],
				"iDisplayLength": 25,
				"sPaginationType": "full_numbers",
				language: $.DataTableLanguage
			});

			// Get EnOcean manufacturers table values
			$.manufacturertbl = [];
			$.ajax({
				url: "json.htm?type=command&param=enoceangetmanufacturers",
				async: false,
				dataType: "json",
				success: function (data, status) {
					if (typeof data.mantbl != "undefined") {
						data.mantbl.sort(SortManufacturersByName);
						$.each(data.mantbl, function (i, item) {
							$.manufacturertbl.push({
								idx : item.idx,
								name: item.name,
							});
						});
					}
				},
			});

			// Get EnOcean RORG table values
			$.rorgtbl = [];
			$.ajax({
				url: "json.htm?type=command&param=enoceangetrorgs",
				async: false,
				dataType: "json",
				success: function (data, status) {
					if (typeof data.rorgtbl != "undefined") {
						$.each(data.rorgtbl, function (i, item) {
							$.rorgtbl.push({
								rorg: item.rorg,
								label: item.label,
								description: item.description,
							});
						});
					}
				},
			});

			// Get EnOcean EEP table values
			$.eeptbl = [];
			$.ajax({
				url: "json.htm?type=command&param=enoceangetprofiles",
				async: false,
				dataType: "json",
				success: function (data, status) {
					if (typeof data.eeptbl != "undefined") {
						$.each(data.eeptbl, function (i, item) {
							$.eeptbl.push({
								rorg: item.rorg,
								func: item.func,
								type: item.type,
								eep: item.eep,
								label: item.label,
								description: item.description,
							});
						});
					}
				},
			});

			// Display EnOcean Hardware screen
			RefreshNodesTable();
		};

		SortManufacturersByName = function(a, b) {
			// make sure "Unknown" value comes first
			if (a.name === "Unknown")
			    return -1;

			if (b.name === "Unknown")
			    return 1;

			// Sort all other values aplhabetically
			var aName = a.name.toLowerCase();
			var bName = b.name.toLowerCase();
			return ((aName < bName) ? -1 : ((aName > bName) ? 1 : 0));
		}

		RefreshNodesTable = function () {
			$("#modal").show();

			// Get EnOcean Nodes and initialize Nodes table rows
			var oTable = $("#nodestable").dataTable();
			oTable.fnClearTable();
			$.ajax({
				url: "json.htm?type=command&param=esp3getnodes&hwdid=" + $.esp3hwdid,
				async: false,
				dataType: "json",
				success: function (data, status) {
					if (typeof data.nodetbl != "undefined") {
						$.each(data.nodetbl, function (i, node) {
							// Convert node.nodeid to 8 hex digits string
							var nodeidStr = addLeadingZeros(parseInt(node.nodeid).toString(16).toUpperCase(), 8);

							var teachinmode = "Unknown";
							if (node.teachinmode === 0)
								teachinmode = "Generic";
							else if (node.teachinmode === 1)
								teachinmode = "Teached-in";
							else if (node.teachinmode === 2)
								teachinmode = "Virtual";

							var state = '<img src="images/unknown.png" />';
							if (node.state === "Awake")
								state = '<img src="images/ok.png" />';
							else if (node.state === "Dead")
								state = '<img src="images/failed.png" />';
							else if (node.state === "Sleeping")
								state = '<img src="images/sleep.png" />';
	
							oTable.fnAddData({
								"nodeid": node.nodeid,
								"name": node.name,
								"manufacturerid": node.manufacturerid,
								"rorg": node.rorg,
								"eep": node.eep,
								"description": node.description,
								"teachinmode": node.teachinmode,
								"0": nodeidStr,
								"1": teachinmode,
								"2": node.name,
								"3": node.manufacturername,
								"4": node.eep,
								"5": node.description,
								"6": node.signallevel,
								"7": node.batterylevel,
								"8": state,
								"9": node.lastupdate,
							});
						});
					}
				},
			});
			// Add a click handler to the rows - this could be used as a callback
			$("#nodestable tbody").off();
			$("#nodestable tbody").on("click", "tr", function () {
				if ($(this).hasClass("row_selected")) {
					// A row is selected : deselect it
					$(this).removeClass("row_selected");

					// Reset values of node parameter controls					
					ResetNodeParameters();
				} else {
					// No row is selected : select current row
					var oTable = $("#nodestable").dataTable();
					oTable.$("tr.row_selected").removeClass("row_selected");
					$(this).addClass("row_selected");

					var selectedrow = fnGetSelected(oTable);
					if (selectedrow.length !== 0) {
						var data = oTable.fnGetData(selectedrow[0]);

						// Set values of node parameter controls					
						RefreshNodeParameters(
							$.esp3hwdid,
							data["nodeid"],
							data["name"],
							data["manufacturerid"],
							data["rorg"],
							data["eep"],
							data["description"],
							);
					}
				}
			});
			// Reset values of node parameter controls					
			ResetNodeParameters();

			$("#modal").hide();
		};

		ResetNodeParameters = function () {
			// Reset scope variables
			$scope.selectednoderorg = 0;
			$scope.selectednodeeep = "";
							
			// Disable node parameters action buttons
			$("#updatenode").attr("class", "btnstyle3-dis");
			$("#deletenode").attr("class", "btnstyle3-dis");

			// Reset node parameter controls
			$("#nodename").val("");
			$("#nodemanufacturer").html("");
			$("#nodemanufacturer").val("");
			$("#noderorg").html("");
			$("#noderorg").val("");
			$("#nodeeep").html("");
			$("#nodeeep").val("");
			$("#nodeeepdesc").html("");
			$("#nodedescription").val("");

			// Reset values of node optional parameter controls (reserved for future use)
			$("#optionalnodeconfigurationpane").hide();
			$("#nodeconfiguration").html("");
		};

		RefreshNodeParameters = function (hwdid, nodeid, name, manufacturerid, selectednoderorg, selectednodeeep, description) {
			// Set scope variables
			$scope.selectednoderorg = selectednoderorg;
			$scope.selectednodeeep = selectednodeeep;

			// Enable node action buttons
			$("#updatenode").attr("class", "btnstyle3");
			$("#updatenode").attr("href", "javascript:UpdateNode(" + hwdid + ",\"" + nodeid + "\")");
			$("#deletenode").attr("class", "btnstyle3");
			$("#deletenode").attr("href", "javascript:DeleteNode(" + hwdid + ",\"" + nodeid + "\")");

			// Populate EnOcean manufacturers combo
			$("#nodemanufacturer").html("");
			$.each($.manufacturertbl, function (i, item) {
				var option = $("<option />");
				option.attr("value", item.idx).text(item.name);
				$("#nodemanufacturer").append(option);
			});

			// Populate EnOcean RORG combo
			$("#noderorg").html("");
			$.each($.rorgtbl, function (i, item) {

				// TODO : if node is virtual, put only allowed values in RORG combo

				var option = $("<option />");
				option.attr("value", item.rorg).text(item.label + " (" + addLeadingZeros(parseInt(item.rorg).toString(16).toUpperCase(), 2) +")");
				$("#noderorg").append(option);
			});

			$("#nodename").val(name);
			$("#nodemanufacturer").val(manufacturerid);

			// Set combo default selected value
			if (selectednoderorg !== 0)
				$("#noderorg").val(selectednoderorg);
			else {
				var option = $("<option />");
				option.attr("value", "").text("Select...");
				$("#noderorg").prepend(option);
				$("#noderorg").val("");
			}

			RefreshNodeEEP(selectednoderorg, selectednoderorg, selectednodeeep);

			$("#nodedescription").val(description);

		};

		RefreshNodeEEP = function (rorg, selectednoderorg, selectednodeeep) {
			if (rorg === 0) {
				RefreshNodeEEPDescription("");
				return;
			}
			if (selectednoderorg === 0)
				selectednodeeep = selectednodeeep.replace("00", addLeadingZeros(parseInt(rorg).toString(16).toUpperCase(), 2));
			
			// Populate EnOcean EEP combo
			$("#nodeeep").html("");

			var eepfound = false;
			$.each($.eeptbl, function (i, item) {
				if (item.rorg == rorg) {

					// TODO : if node is virtual, put only allowed values in EEP combo

					var option = $("<option />");
					option.attr("value", item.eep).text(item.eep);
					$("#nodeeep").append(option);

					if (item.eep === selectednodeeep)
						eepfound = true;
				}
			});

			// Select default combo value
			if (eepfound) {
				$("#nodeeep").val(selectednodeeep);
				RefreshNodeEEPDescription(selectednodeeep);
			} else {
				var option = $("<option />");
				option.attr("value", "").text("Select...");
				$("#nodeeep").prepend(option);
				$("#nodeeep").val("");
				RefreshNodeEEPDescription("");
			}
		};

		RefreshNodeEEPDescription = function (eep) {
			var description = "";
			$.each($.eeptbl, function (i, item) {
				if (item.eep === eep) {
					description = item.description;
					return;
				}
			});
			$("#nodeeepdesc").html(description);
		};

		OnChangeNodeRORG = function (combo) {
			// Once a RORG value has been selected, remove "Select..." option
			if (combo.value !== "" && combo.options[0].value === "")
				combo.remove(0);
			
			RefreshNodeEEP(combo.value, $scope.selectednoderorg, $scope.selectednodeeep);
		};

		OnChangeNodeEEP = function (combo) {
			// Once an EEP value has been selected, remove "Select..." option
			if (combo.value !== "" && combo.options[0].value === "")
				combo.remove(0);
			
			RefreshNodeEEPDescription(combo.value);
		};

		EnableLearnMode = function () {
			if (typeof $scope.mytimer !== "undefined") {
				$interval.cancel($scope.mytimer);
				$scope.mytimer = undefined;
			}
			$.ajax({
				url: "json.htm?type=command&param=esp3enablelearnmode&hwdid=" + $.esp3hwdid + "&minutes=1",
				async: true,
				dataType: "json",
				success: function (data, status) {
					$("#esp3lmdwaiting").show();
					$("#esp3lmdteachedin").hide();
					$("#esp3lmdtimedout").hide();
					$("#esp3learnmodedialog").modal("show");

					$scope.mytimer = $interval(function () { $scope.IsNodeTeachedIn(); }, 1000);
				},
				error: function (result, status, error) {
					ShowNotify($.t("Problem enabling learn mode!"), 2500, true);
				},
			});
		};

		$scope.IsNodeTeachedIn = function () {
			if (typeof $scope.mytimer !== "undefined") {
				$interval.cancel($scope.mytimer);
				$scope.mytimer = undefined;
			}
			$.ajax({
				url: "json.htm?type=command&param=esp3isnodeteachedin&hwdid=" + $.esp3hwdid,
				async: true,
				dataType: "json",
				success: function (data, status) {
					if (data.result === 1) { // An EnOcean node has been teached-in
						$scope.esp3_nodeid = addLeadingZeros(parseInt(data.nodeid).toString(16).toUpperCase(), 8);
						$scope.esp3_manufacturername = data.manufacturername;
						$scope.esp3_eep = data.eep;
						$scope.esp3_description = data.description;
						$("#esp3lmdwaiting").hide();
						$("#esp3lmdteachedin").show();
						$scope.$apply();
						return;
					}
					if (data.result === 2) { // Learn mode timed out
						$("#esp3lmdwaiting").hide();
						$("#esp3lmdtimedout").show();
						return;
					}
					// Keep waiting
					$scope.mytimer = $interval(function () { $scope.IsNodeTeachedIn(); }, 1000);
				},
				error: function (result, status, error) {
					ShowNotify($.t("Problem teachin-in node!"), 2500, true);
				},
			});
		};

		OnCancelTeachIn = function () {
			$.ajax({
				url: "json.htm?type=command&param=esp3cancelteachin&hwdid=" + $.esp3hwdid,
				async: true,
				dataType: "json",
			});
			$interval.cancel($scope.mytimer);
			$scope.mytimer = undefined;
			$("#esp3learnmodedialog").modal("hide");
		};

		OnCloseLearnMode = function () {
			$("#esp3learnmodedialog").modal("hide");
			RefreshNodesTable();
		};

		ResetController = function () {
			$.ajax({
				url: "json.htm?type=command&param=esp3controllerreset&hwdid=" + $.esp3hwdid,
				async: true,
				dataType: "json",
				success: function (data, status) {
					bootbox.alert($.t("Resetting EnOcean controller..."));
					RefreshNodesTable();
				},
				error: function (result, status, error) {
					ShowNotify($.t("Problem resetting EnOcean controller!"), 2500, true);
				},
			});
		};
						
		UpdateNode = function (hwdid, nodeid) {
			if ($("#updatenode").attr("class") === "btnstyle3-dis")
				return;

			var name = $("#nodename").val().trim();
			if (name === "") {
				ShowNotify($.t("Please enter a node name!"), 2500, true);
				return;
			}
			var manufacturerid = $("#nodemanufacturer option:selected").val();
			if (typeof manufacturerid == "undefined") {
				ShowNotify($.t("Please select a manufacturer!"), 2500, true);
				return;
			}
			var eep = $("#nodeeep option:selected").val();
			if (typeof eep == "undefined" || eep === "") {
				ShowNotify($.t("Please select an EnOcean Profile (EEP)!"), 2500, true);
				return;
			}
			var nodeidStr = addLeadingZeros(parseInt(nodeid).toString(16).toUpperCase(), 8);
			bootbox.confirm($.t("Are you sure you want to update node") + " \'"+ nodeidStr + "\'?", function (confirmed) {
				if (confirmed) {
					$.ajax({
						url: "json.htm?type=command&param=esp3updatenode" +
							"&hwdid=" + hwdid +
							"&nodeid=" + nodeid +
							"&name=" + encodeURIComponent(name) +
							"&manufacturerid=" + manufacturerid +
							"&eep=" + encodeURIComponent(eep) +
							"&description=" + encodeURIComponent($("#nodedescription").val().trim()),
						async: true,
						dataType: "json",
						success: function (data, status) {
							RefreshNodesTable();
						},
						error: function (result, status, error) {
							ShowNotify($.t("Problem updating node") + " \'" + nodeidStr + "\'!", 2500, true);
						},
					});
				}
			});
		};

		DeleteNode = function (hwdid, nodeid) {
			if ($("#deletenode").attr("class") === "btnstyle3-dis")
				return;

			var nodeidStr = addLeadingZeros(parseInt(nodeid).toString(16).toUpperCase(), 8);
			bootbox.confirm($.t("Are you sure you want to delete node") + " \'" + nodeidStr + "\' " + $.t("and all it's devices ?"), function (confirmed) {
				if (confirmed) {
					$.ajax({
						url: "json.htm?type=command&param=esp3deletenode" +
							"&hwdid=" + hwdid +
							"&nodeid=" + nodeid,
						async: true,
						dataType: "json",
						success: function (data, status) {
							RefreshNodesTable();
						},
						error: function (result, status, error) {
							ShowNotify($.t("Problem deleting node") + " \'" + nodeidStr + "\'!", 2500, true);
						},
					});
				}
			});
		};
	}
});
