define(['app', 'livesocket', 'widgets/dzSceneWidget'], function (app) {
	app.controller('ScenesController', function ($scope, $rootScope, $location, $route, $routeParams, $http, $interval, $timeout, permissions, livesocket) {
		var $element = $('#main-view #scenecontent').last();

		var SceneIdx = 0;

		RemoveCode = function (idx, code) {
			if ($element.find("#removecode").hasClass('disabled')) {
				return false;
			}
			bootbox.confirm($.t("Are you sure to delete this Device?\n\nThis action can not be undone..."), function (result) {
				if (result == true) {
					$.ajax({
						url: "json.htm?type=command&param=removescenecode&sceneidx=" + SceneIdx + "&idx=" + idx + "&code=" + code,
						async: false,
						dataType: 'json',
						success: function (data) {
							RefreshActivators();
						}
					});
				}
			});
		}

		AddCode = function () {
			ShowNotify($.t('Press button on Remote...'));

			setTimeout(function () {
				var bHaveFoundDevice = false;
				var deviceidx = 0;
				var Cmd = 0;

				$.ajax({
					url: "json.htm?type=command&param=learnsw",
					async: false,
					dataType: 'json',
					success: function (data) {
						if (typeof data.status != 'undefined') {
							if (data.status == 'OK') {
								bHaveFoundDevice = true;
								deviceidx = data.idx;
								Cmd = data.Cmd;
							}
						}
					}
				});
				HideNotify();

				setTimeout(function () {
					if (bHaveFoundDevice == true) {
						$.ajax({
							url: "json.htm?type=command&param=addscenecode&sceneidx=" + SceneIdx + "&idx=" + deviceidx + "&cmnd=" + Cmd,
							async: false,
							dataType: 'json',
							success: function (data) {
								RefreshActivators();
							}
						});
					}
					else {
						ShowNotify($.t('Timeout...<br>Please try again!'), 2500, true);
					}
				}, 200);
			}, 600);
		}
		
		AddManualCode = function() {
			$('#dialog-addmanualactivationdevice #comboswitch').html("");
			$.each($.LightsAndSwitches, function (i, item) {
				var option = $('<option />');
				option.attr('value', item.idx).text(item.name);
				$('#dialog-addmanualactivationdevice #comboswitch').append(option);
			});
			
			$("#dialog-addmanualactivationdevice").dialog("open");		
		}

		ClearCodes = function () {
			var bValid = false;
			bootbox.confirm($.t("Are you sure to delete ALL Devices?\n\nThis action can not be undone!"), function (result) {
				if (result == true) {
					$.ajax({
						url: "json.htm?type=command&param=clearscenecodes&sceneidx=" + SceneIdx,
						async: false,
						dataType: 'json',
						success: function (data) {
							RefreshActivators();
						}
					});
				}
			});
		}

		AddScene = function () {
			$("#dialog-addscene").dialog("open");
		}

		DeleteScene = function () {
			bootbox.confirm($.t("Are you sure to remove this Scene?"), function (result) {
				if (result == true) {
					$.ajax({
						url: "json.htm?type=command&param=deletescene&idx=" + SceneIdx,
						async: false,
						dataType: 'json',
						success: function (data) {
							ShowScenes();
						}
					});
				}
			});
		}

		SaveScene = function () {
			var bValid = true;
			bValid = bValid && checkLength($element.find("#devicename"), 2, 100);

			var onaction = $element.find("#onaction").val();
			var offaction = $element.find("#offaction").val();

			if (onaction != "") {
				if (
					(onaction.indexOf("http://") == 0) || 
					(onaction.indexOf("https://") == 0) ||
					(onaction.indexOf("script://") == 0)
					)
				{
					if (checkLength($element.find("#onaction"), 10, 500) == false) {
						bootbox.alert($.t("Invalid ON Action!"));
						return;
					}
				}
				else {
					bootbox.alert($.t("Invalid ON Action!"));
					return;
				}
			}
			if (offaction != "") {
				if (
					(offaction.indexOf("http://") == 0) ||
					(offaction.indexOf("https://") == 0) ||
					(offaction.indexOf("script://") == 0)
					)
				{
					if (checkLength($element.find("#offaction"), 10, 500) == false) {
						bootbox.alert($.t("Invalid Off Action!"));
						return;
					}
				}
				else {
					bootbox.alert($.t("Invalid Off Action!"));
					return;
				}
			}

			if (bValid) {
				var SceneType = $element.find("#combotype").val();
				var bIsProtected = $element.find('#protected').is(":checked");
				$.ajax({
					url: "json.htm?type=command&param=updatescene&idx=" + SceneIdx +
					"&scenetype=" + SceneType +
					"&name=" + encodeURIComponent($element.find("#devicename").val()) +
					"&description=" + encodeURIComponent($element.find("#devicedescription").val()) +
					'&onaction=' + btoa(onaction) +
					'&offaction=' + btoa(offaction) +
					"&protected=" + bIsProtected,
					async: false,
					dataType: 'json',
					success: function (data) {
						ShowScenes();
					}
				});
			}
		}

		AddDevice = function () {
			var DeviceIdx = $element.find("#combodevice option:selected").val();
			if (typeof DeviceIdx == 'undefined') {
				bootbox.alert($.t('No Device Selected!'));
				return;
			}

			var Command = $element.find("#combocommand option:selected").val();

			var level = 100;
			var colorJSON = ""; // Empty string, intentionally illegal JSON
			$.each($.LightsAndSwitches, function (i, item) {
				if (item.idx == DeviceIdx) {
					if (isLED(item.SubType)) {
						var color = $element.find('.colorpicker #popup_picker').wheelColorPicker('getColor');
						level = Math.round((color.m*99)+1); // 1..100
						colorJSON = $element.find('.colorpicker #popup_picker')[0].getJSONColor();
					}
					else {
						if (item.isdimmer == true) {
							level = $element.find("#combolevel").val();
						}
					}
				}
			});
			var ondelay = $element.find("#ondelaytime").val();
			var offdelay = $element.find("#offdelaytime").val();

			$.ajax({
				url: "json.htm?type=command&param=addscenedevice&idx=" + SceneIdx + "&isscene=" + $.isScene + "&devidx=" + DeviceIdx + "&command=" + Command + "&level=" + level + "&color=" + colorJSON + "&ondelay=" + ondelay + "&offdelay=" + offdelay,
				async: false,
				dataType: 'json',
				success: function (data) {
					if (data.status == 'OK') {
						RefreshDeviceTable(SceneIdx);
					}
					else {
						ShowNotify($.t('Problem adding Device!'), 2500, true);
					}
				},
				error: function () {
					HideNotify();
					ShowNotify($.t('Problem adding Device!'), 2500, true);
				}
			});
		}

		ClearDevices = function () {
			var bValid = false;
			bootbox.confirm($.t("Are you sure to delete ALL Devices?\n\nThis action can not be undone!"), function (result) {
				if (result == true) {
					$.ajax({
						url: "json.htm?type=command&param=deleteallscenedevices&idx=" + SceneIdx,
						async: false,
						dataType: 'json',
						success: function (data) {
							RefreshDeviceTable(SceneIdx);
						}
					});
				}
			});
		}

		MakeFavorite = function (id, isfavorite) {
			if (!permissions.hasPermission("Admin")) {
				HideNotify();
				ShowNotify($.t('You do not have permission to do that!'), 2500, true);
				return;
			}

			$.ajax({
				url: "json.htm?type=command&param=makescenefavorite&idx=" + id + "&isfavorite=" + isfavorite,
				async: false,
				dataType: 'json',
				success: function (data) {
					ShowScenes();
				}
			});
		}

		ChangeDeviceOrder = function (order, devid) {
			if (!permissions.hasPermission("Admin")) {
				HideNotify();
				ShowNotify($.t('You do not have permission to do that!'), 2500, true);
				return;
			}
			$.ajax({
				url: "json.htm?type=command&param=changescenedeviceorder&idx=" + devid + "&way=" + order,
				async: false,
				dataType: 'json',
				success: function (data) {
					RefreshDeviceTableEx();
				}
			});
		}

		SetColValue = function (idx, color, brightness) {
			clearInterval($.setColValue);
			if (!permissions.hasPermission("User")) {
				HideNotify();
				ShowNotify($.t('You do not have permission to do that!'), 2500, true);
				return;
			}
			$.ajax({
				url: "json.htm?type=command&param=setcolbrightnessvalue&idx=" + idx + "&color=" + color + "&brightness=" + brightness,
				async: false,
				dataType: 'json'
			});
		}

		RefreshDeviceTableEx = function () {
			RefreshDeviceTable(SceneIdx);
		}

		RefreshActivators = function () {
			$element.find('#delclract #removecode').attr("class", "btnstyle3-dis");

			var oTable = $element.find('#scenedactivationtable').dataTable();
			oTable.fnClearTable();

			$.ajax({
				url: "json.htm?type=command&param=getsceneactivations&idx=" + SceneIdx,
				async: false,
				dataType: 'json',
				success: function (data) {

					if (typeof data.result != 'undefined') {
						$.each(data.result, function (i, item) {
							var addId = oTable.fnAddData({
								"DT_RowId": item.idx,
								"code": item.code,
								"0": item.idx,
								"1": item.name,
								"2": item.codestr
							});
						});
					}
				}
			});
			/* Add a click handler to the rows - this could be used as a callback */
			$element.find("#scenedactivationtable tbody").off();
			$element.find("#scenedactivationtable tbody").on('click', 'tr', function () {
				if ($(this).hasClass('row_selected')) {
					$(this).removeClass('row_selected');
					$element.find('#delclract #removecode').attr("class", "btnstyle3-dis");
				}
				else {
					var oTable = $element.find('#scenedactivationtable').dataTable();
					oTable.$('tr.row_selected').removeClass('row_selected');
					$(this).addClass('row_selected');

					$element.find('#delclract #removecode').attr("class", "btnstyle3");
					var anSelected = fnGetSelected(oTable);
					if (anSelected.length !== 0) {
						var data = oTable.fnGetData(anSelected[0]);
						var idx = data["DT_RowId"];
						var code = data["code"];
						$element.find("#delclract #removecode").attr("href", "javascript:RemoveCode(" + idx + ", " + code + ")");
					}
				}
			});

			$('#modal').hide();
		}

		RefreshDeviceTable = function (idx) {
			$('#modal').show();

			$element.find('#delclr #devicedelete').attr("class", "btnstyle3-dis");
			$element.find('#delclr #updatedelete').attr("class", "btnstyle3-dis");

			var oTable = $element.find('#scenedevicestable').dataTable();
			oTable.fnClearTable();

			$.ajax({
				url: "json.htm?type=command&param=getscenedevices&idx=" + idx + "&isscene=" + $.isScene,
				async: false,
				dataType: 'json',
				success: function (data) {

					if (typeof data.result != 'undefined') {
						var totalItems = data.result.length;
						$.each(data.result, function (i, item) {
							var command = "-";
							if ($.isScene == true) {
								command = item.Command;
							}
							var updownImg = "";
							if (i != totalItems - 1) {
								//Add Down Image
								if (updownImg != "") {
									updownImg += "&nbsp;";
								}
								updownImg += '<i class="fa-solid fa-arrow-down dz-chrome-icon dz-act-edit lcursor" onclick="ChangeDeviceOrder(1,' + item.ID + ');"></i>';
							}
							else {
								updownImg += '<img src="images/empty16.png" width="16" height="16"></img>';
							}
							if (i != 0) {
								//Add Up image
								if (updownImg != "") {
									updownImg += "&nbsp;";
								}
								updownImg += '<i class="fa-solid fa-arrow-up dz-chrome-icon dz-act-edit lcursor" onclick="ChangeDeviceOrder(0,' + item.ID + ');"></i>';
							}
							var levelstr = item.Level + " %";

							if (isLED(item.SubType)) {
								var color = {};
								try {
									color = JSON.parse(item.Color);
								}
								catch(e) {
									// forget about it :)
								}
								//TODO: Refactor to some nice helper function, ensuring range of 0..ff etc
								//TODO: Calculate color if color mode is white/temperature.
								var rgbhex = "808080";
								if (color.m == 1 || color.m == 2) { // White or color temperature
									var whex = Math.round(255*item.Level/100).toString(16);
									if( whex.length == 1) {
										whex = "0" + whex;
									}
									rgbhex = whex + whex + whex;
								}
								if (color.m == 3 || color.m == 4) { // RGB or custom
									var rhex = Math.round(color.r).toString(16);
									if( rhex.length == 1) {
										rhex = "0" + rhex;
									}
									var ghex = Math.round(color.g).toString(16);
									if( ghex.length == 1) {
										ghex = "0" + ghex;
									}
									var bhex = Math.round(color.b).toString(16);
									if( bhex.length == 1) {
										bhex = "0" + bhex;
									}
									rgbhex = rhex + ghex + bhex;
								}
								levelstr += '<div id="picker4" class="ex-color-box" style="background-color: #' + rgbhex + ';"></div>';
							}


							var addId = oTable.fnAddData({
								"DT_RowId": item.ID,
								"Command": item.Command,
								"RealIdx": item.DevRealIdx,
								"Level": item.Level,
								"Color": item.Color,
								"OnDelay": item.OnDelay,
								"OffDelay": item.OffDelay,
								"Order": item.Order,
								"IsScene": item.Order,
								"0": item.Name,
								"1": command,
								"2": levelstr,
								"3": item.OnDelay,
								"4": item.OffDelay,
								"5": updownImg
							});
						});
					}
				}
			});
			/* Add a click handler to the rows - this could be used as a callback */
			$element.find("#scenedevicestable tbody").off();
			$element.find("#scenedevicestable tbody").on('click', 'tr', function () {
				if ($(this).hasClass('row_selected')) {
					$(this).removeClass('row_selected');
					$element.find('#delclr #devicedelete').attr("class", "btnstyle3-dis");
					$element.find('#delclr #updatedelete').attr("class", "btnstyle3-dis");
				}
				else {
					var oTable = $element.find('#scenedevicestable').dataTable();
					oTable.$('tr.row_selected').removeClass('row_selected');
					$(this).addClass('row_selected');

					$element.find('#delclr #devicedelete').attr("class", "btnstyle3");

					$element.find('#delclr #updatedelete').attr("class", "btnstyle3");
					$element.find('#delclr #updatedelete').show();

					var anSelected = fnGetSelected(oTable);
					if (anSelected.length !== 0) {
						var data = oTable.fnGetData(anSelected[0]);
						var idx = data["DT_RowId"];
						var devidx = data["RealIdx"];
						$element.find("#delclr #devicedelete").attr("href", "javascript:DeleteDevice(" + idx + ")");
						$element.find("#delclr #updatedelete").attr("href", "javascript:UpdateDevice(" + idx + "," + devidx + ")");
						$.lampIdx = devidx;
						$element.find("#combodevice").val(devidx);
						if ($.isScene == true) {
							$element.find("#combocommand").val(data["Command"]);
						}
						else {
							$element.find("#combocommand").val("On");
						}
						OnSelChangeDevice();

						var level = data["Level"];
						$element.find("#combolevel").val(level);

						var SubType = "";
						var DimmerType = "";
						$.each($.LightsAndSwitches, function (i, item) {
							if (item.idx == devidx) {
								SubType = item.SubType;
								DimmerType = item.DimmerType;
							}
						});
						var MaxDimLevel = 100; // Always 100 for LED type
						if (isLED(SubType))
							ShowRGBWPicker('#scenecontent #ScenesLedColor', devidx, 0, MaxDimLevel, level, data["Color"], SubType, DimmerType);

						$element.find("#ondelaytime").val(data["OnDelay"]);
						$element.find("#offdelaytime").val(data["OffDelay"]);
					}
				}
			});

			$('#modal').hide();
		}

		UpdateDevice = function (idx, devidx) {
			var DeviceIdx = $element.find("#combodevice option:selected").val();
			if (typeof DeviceIdx == 'undefined') {
				bootbox.alert($.t('No Device Selected!'));
				return;
			}
			if (DeviceIdx != devidx) {
				bootbox.alert($.t('Device change not allowed!'));
				return;
			}

			var Command = $element.find("#combocommand option:selected").val();

			var level = 100;
			var colorJSON = ""; // Empty string, intentionally illegal JSON
			var ondelay = $element.find("#ondelaytime").val();
			var offdelay = $element.find("#offdelaytime").val();

			$.each($.LightsAndSwitches, function (i, item) {
				if (item.idx == DeviceIdx) {
					if (isLED(item.SubType)) {
						var color = $element.find('.colorpicker #popup_picker').wheelColorPicker('getColor');
						level = Math.round((color.m*99)+1); // 1..100
						colorJSON = $element.find('.colorpicker #popup_picker')[0].getJSONColor();
					}
					else {
						if (item.isdimmer == true) {
							level = $element.find("#combolevel").val();
						}
					}
				}
			});

			$.ajax({
				url: "json.htm?type=command&param=updatescenedevice&idx=" + idx + "&isscene=" + $.isScene + "&devidx=" + DeviceIdx + "&command=" + Command + "&level=" + level + "&color=" + colorJSON + "&ondelay=" + ondelay + "&offdelay=" + offdelay,
				async: false,
				dataType: 'json',
				success: function (data) {
					if (data.status == 'OK') {
						RefreshDeviceTable(SceneIdx);
					}
					else {
						ShowNotify($.t('Problem updating Device!'), 2500, true);
					}
				},
				error: function () {
					HideNotify();
					ShowNotify($.t('Problem updating Device!'), 2500, true);
				}
			});
		}

		DeleteDevice = function (idx) {
			bootbox.confirm($.t("Are you sure to delete this Device?\n\nThis action can not be undone..."), function (result) {
				if (result == true) {
					$.ajax({
						url: "json.htm?type=command&param=deletescenedevice&idx=" + idx,
						async: false,
						dataType: 'json',
						success: function (data) {
							RefreshDeviceTable(SceneIdx);
						}
					});
				}
			});
		}

		OnSelChangeDevice = function () {
			var DeviceIdx = $element.find("#combodevice option:selected").val();
			if (typeof DeviceIdx == 'undefined') {
				$element.find("#LevelDiv").hide();
				return;
			}
			var bShowLevel = false;
			var dimmerLevels = "none";
			var SubType = "";
			$.each($.LightsAndSwitches, function (i, item) {
				if (item.idx == DeviceIdx) {
					bShowLevel = item.isdimmer;
					dimmerLevels = item.DimmerLevels;
					SubType = item.SubType;
				}
			});

			$("#ScenesLedColor").hide();
			$element.find("#LevelDiv").hide();
			if (isLED(SubType)) {
				$("#ScenesLedColor").show();
			}
			if (bShowLevel == true && !isLED(SubType)) { // TODO: Show level combo box also for LED
				var levelDiv$ = $element.find("#LevelDiv");
				levelDiv$.find("option").show().end().show();

				var dimmerValues = [];

				$.each(dimmerLevels.split(','), function (i, level) {
					dimmerValues[i] = level;
				});

				levelDiv$.find("option").remove();
				for (var levelCounter = 0; levelCounter < dimmerValues.length; levelCounter++) {
					var option = $('<option />');
					option.attr('value', dimmerValues[levelCounter]).text(dimmerValues[levelCounter] + "%");
					$element.find("#combolevel").append(option);
				}
			}
		}

		EditSceneDevice = function (idx, name, description, havecode, type, bIsProtected, onaction, offaction) {
			SceneIdx = idx;

			var bIsScene = (type == "Scene");
			$.isScene = bIsScene;

			var htmlcontent = '';
			htmlcontent += $('#editscene').html();
			$scope.showSceneList = false;
			$timeout(function(){}, 0)
			$('#sceneeditcontent').html(GetBackbuttonHTMLTable('ShowScenes') + htmlcontent);
			$('#sceneeditcontent').i18n();
			$element.find("#LevelDiv").hide();
			$("#ScenesLedColor").hide();

			$element.find("#onaction").val(atob(onaction));
			$element.find("#offaction").val(atob(offaction));

			$element.find('#protected').prop('checked', (bIsProtected == true));

			if (bIsScene == true) {
				$element.find("#combotype").val(0);
				$element.find("#CommandDiv").show();
				$element.find("#CommandHeader").html($.t("Command"));
			}
			else {
				$element.find("#combotype").val(1);
				$element.find("#CommandDiv").hide();
				$element.find("#CommandHeader").html($.t("State"));
			}

			$element.find('#scenedevicestable').dataTable({
				"sDom": '<"H"lfrC>t<"F"ip>',
				"oTableTools": {
					"sRowSelect": "single",
				},
				"aoColumnDefs": [
					{ "bSortable": false, "aTargets": [1] }
				],
				"bSort": false,
				"bProcessing": true,
				"bStateSave": false,
				"bJQueryUI": true,
				"aLengthMenu": [[25, 50, 100, -1], [25, 50, 100, "All"]],
				"iDisplayLength": 25,
				"sPaginationType": "full_numbers",
				language: $.DataTableLanguage
			});
			$element.find('#scenedactivationtable').dataTable({
				"sDom": '<"H"lfrC>t<"F"ip>',
				"oTableTools": {
					"sRowSelect": "single",
				},
				"aoColumnDefs": [
					{ "bSortable": false, "aTargets": [1] }
				],
				"bSort": false,
				"bProcessing": true,
				"bStateSave": false,
				"bJQueryUI": true,
				"aLengthMenu": [[25, 50, 100, -1], [25, 50, 100, "All"]],
				"iDisplayLength": 25,
				"sPaginationType": "full_numbers",
				language: $.DataTableLanguage
			});
			$element.find("#deviceidx").text(idx);
			$element.find("#devicename").val(unescape(name));
			$element.find("#devicedescription").val(unescape(description));

			$element.find("#combodevice").html("");

			if ($.isScene == false) {
				$element.find('#delclr #updatedelete').hide();
			}
			else {
				$element.find('#delclr #updatedelete').show();
			}

			$.each($.LightsAndSwitches, function (i, item) {
				var option = $('<option />');
				option.attr('value', item.idx).text(item.name);
				$element.find("#combodevice").append(option);
			});

			$element.find("#combodevice").change(function () {
				OnSelChangeDevice();

				var DeviceIdx = $element.find("#combodevice option:selected").val();
				if (typeof DeviceIdx != 'undefined') {
					var SubType = "";
					var DimmerType = "";
					$.each($.LightsAndSwitches, function (i, item) {
						if (item.idx == DeviceIdx) {
							SubType = item.SubType;
							DimmerType = item.DimmerType;
						}
					});
					var MaxDimLevel = 100; // Always 100 for LED type
					if (isLED(SubType))
						ShowRGBWPicker('#scenecontent #ScenesLedColor', DeviceIdx, 0, MaxDimLevel, 50, "", SubType, DimmerType);
				}
			});
			$element.find('#combodevice').keypress(function () {
				$(this).change();
			});

            $element.find('#combodevice').trigger('change');

			OnSelChangeDevice();

			var DeviceIdx = $element.find("#combodevice option:selected").val();
			if (typeof DeviceIdx != 'undefined') {
				var SubType = "";
				var DimmerType = "";
				$.each($.LightsAndSwitches, function (i, item) {
					if (item.idx == DeviceIdx) {
						SubType = item.SubType;
						DimmerType = item.DimmerType;
					}
				});
				var MaxDimLevel = 100; // Always 100 for LED type
				if (isLED(SubType))
					ShowRGBWPicker('#scenecontent #ScenesLedColor', DeviceIdx, 0, MaxDimLevel, 50, "", SubType, DimmerType);
			}

			RefreshDeviceTable(idx);
			RefreshActivators();
		}

		RefreshLightSwitchesComboArray = function () {
			$.LightsAndSwitches = [];
			$.ajax({
				url: "json.htm?type=command&param=getlightswitches",
				async: false,
				dataType: 'json',
				success: function (data) {
					if (typeof data.result != 'undefined') {
						$.each(data.result, function (i, item) {
							$.LightsAndSwitches.push({
								idx: item.idx,
								name: item.Name,
								SubType: item.SubType,
								isdimmer: item.IsDimmer,
								DimmerLevels: item.DimmerLevels
							}
							);
						});
					}
				}
			});
		}

		$.strPad = function (i, l, s) {
			var o = i.toString();
			if (!s) { s = '0'; }
			while (o.length < l) {
				o = s + o;
			}
			return o;
		};

		RefreshItem = function (item) {
			if (!$scope.scenes) return;

			for (var i = 0; i < $scope.scenes.length; i++) {
				if ($scope.scenes[i].idx == item.idx) {
					// Update the scene object properties
					angular.extend($scope.scenes[i], item);

					if (!document.hidden) {
						if ($scope.config.ShowUpdatedEffect == true) {
							var id = "#" + item.idx;
							$(id + " #name").effect("highlight", { color: '#EEFFEE' }, 1000);
						}
					}
					RefreshLiveSearch();
					return;
				}
			}
		}

		//We only call this once. After this the widgets are being updated automatically by used of the websocket broadcast event.
		RefreshScenes = function () {
			var roomPlanId = window.myglobals.LastPlanSelected || 0;
			livesocket.getJson("json.htm?type=command&param=getscenes&lastupdate=" + $.LastUpdateTime + "&plan=" + roomPlanId, function (data) {
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
					$timeout(function(){}, 0)
				}
			});
		}

		ShowScenes = function () {
			RefreshLightSwitchesComboArray();

			$scope.showSceneList = true;
			$('#sceneeditcontent').empty();

			var roomPlanId = window.myglobals.LastPlanSelected || 0;
			$.ajax({
				url: "json.htm?type=command&param=getscenes&plan=" + roomPlanId,
				dataType: 'json',
				success: function (data) {
					if (typeof data.result != 'undefined') {
						$scope.bAllowWidgetReorder = data.AllowWidgetOrdering;
						if (typeof data.ActTime != 'undefined') {
							$.LastUpdateTime = parseInt(data.ActTime);
						}

						$scope.scenes = data.result;
					} else {
						$scope.scenes = [];
					}

					$scope.loading = false;

					$timeout(function(){}, 0)

					$rootScope.RefreshTimeAndSun();

					// Set up drag/drop after Angular renders
					$timeout(function() {
						if ($scope.bAllowWidgetReorder == true) {
							if (permissions.hasPermission("Admin")) {
								if (window.myglobals.ismobileint == false) {
									$element.find(".span4").draggable({
										helper: 'clone',
										opacity: 0.7,
										zIndex: 1000,
										revert: 'invalid',
										scrollSensitivity: 40,
										scrollSpeed: 20,
										drag: function () {
											SceneIdx = $(this).attr("id");
										}
									});
									$element.find(".span4").droppable({
										drop: function () {
											var myid = $(this).attr("id");
											$.ajax({
												url: "json.htm?type=command&param=switchsceneorder&idx1=" + myid + "&idx2=" + SceneIdx,
												dataType: 'json',
												success: function (data) {
													ShowScenes();
												}
											});
										}
									});
								}
							}
						}
						$element.i18n();
						RefreshLiveSearch();
					}, 100);

					RefreshScenes();
				}
			});

			return false;
		}

		init();

		function init() {
			SceneIdx = 0;
			$scope.MakeGlobalConfig();

			var ctrl = {};
			ctrl.RoomPlans = $rootScope.GetRoomPlans();
			var roomPlanId = $routeParams.room || window.myglobals.LastPlanSelected;
			if (roomPlanId != null) {
				ctrl.roomSelected = roomPlanId;
				window.myglobals.LastPlanSelected = roomPlanId;
			}
			ctrl.changeRoom = function () {
				var idx = ctrl.roomSelected;
				if (idx == null) { return; }
				window.myglobals.LastPlanSelected = idx;
				window.myglobals.LastSearchFilter = '';
				$('.jsLiveSearch').val('').trigger('change');
				$route.updateParams({ room: idx >= 0 ? idx : undefined });
				$location.replace();
			};
			$scope.ctrl = ctrl;

			$scope.$on('scene_update', function (event, sceneData) {
				RefreshItem(sceneData);
			});

			$("#dialog-addscene").dialog({
				autoOpen: false,
				width: 380,
				height: 200,
				modal: true,
				resizable: false,
				title: $.t("Add Scene"),
				buttons: [{
					text: $.t("Add Scene"),
					click: function () {
						var bValid = true;
						bValid = bValid && checkLength($("#dialog-addscene #scenename"), 2, 100);
						if (bValid) {
							$.pDialog = $(this);
							var SceneName = encodeURIComponent($("#dialog-addscene #scenename").val());
							var SceneType = $("#dialog-addscene #combotype").val();
							$.ajax({
								url: "json.htm?type=command&param=addscene&name=" + SceneName + "&scenetype=" + SceneType,
								async: false,
								dataType: 'json',
								success: function (data) {
									if (typeof data.status != 'undefined') {
										if (data.status == 'OK') {
											$.pDialog.dialog("close");
											ShowScenes();
										}
										else {
											ShowNotify(data.message, 3000, true);
										}
									}
								}
							});

						}
					}
				}, {
					text: $.t("Cancel"),
					click: function () {
						$(this).dialog("close");
					}
				}],
				close: function () {
					$(this).dialog("close");
				}
			}).i18n();

			$("#dialog-addmanualactivationdevice").dialog({
				autoOpen: false,
				width: 380,
				height: 200,
				modal: true,
				resizable: false,
				title: $.t("Add Manual Light/Switch Device"),
				buttons: [{
					text: $.t("Add Device"),
					click: function () {
						var deviceidx = $('#dialog-addmanualactivationdevice #comboswitch').val();
						if (typeof deviceidx == 'undefined') {
							bootbox.alert($.t('No Device Selected!'));
							return;
						}
						var Cmd = $('#dialog-addmanualactivationdevice #combocode').val();
						$.pDialog = $(this);
						$.ajax({
							url: "json.htm?type=command&param=addscenecode&sceneidx=" + SceneIdx + "&idx=" + deviceidx + "&cmnd=" + Cmd,
							async: false,
							dataType: 'json',
							success: function (data) {
								$.pDialog.dialog("close");
								RefreshActivators();
							}
						});
					}
				}, {
					text: $.t("Cancel"),
					click: function () {
						$(this).dialog("close");
					}
				}],
				close: function () {
					$(this).dialog("close");
				}
			}).i18n();


			//handles TopBar Links
			$scope.tblinks=[];
			if (permissions.hasPermission("Admin")) {
				$scope.tblinks = [
					{
						onclick:"AddScene",
						text:"Add Scene",
						i18n: "Add Scene",
						icon: "plus-circle"
					}
				];
			}

			$scope.scenes = [];
				$scope.showSceneList = true;
			$scope.loading = true;

			$scope.refreshScenes = function() {
				ShowScenes();
			};

			$scope.editScene = function(scene) {
				EditSceneDevice(
					scene.idx,
					escape(scene.Name),
					escape(scene.Description),
					scene.HardwareID,
					scene.Type,
					scene.Protected,
					scene.OnAction,
					scene.OffAction
				);
			};

			ShowScenes();
			WatchLiveSearch();
		};

	});
});
