define(['app', 'livesocket'], function (app) {
	app.controller('ScenesController', function ($scope, $rootScope, $location, $http, $interval, permissions, livesocket) {
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
						url: "json.htm?type=deletescene&idx=" + SceneIdx,
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
					url: "json.htm?type=updatescene&idx=" + SceneIdx +
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

			if (typeof $scope.mytimer != 'undefined') {
				$interval.cancel($scope.mytimer);
				$scope.mytimer = undefined;
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
			if (permissions.hasPermission("Viewer")) {
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
								updownImg += '<img src="images/down.png" onclick="ChangeDeviceOrder(1,' + item.ID + ');" class="lcursor" width="16" height="16"></img>';
							}
							else {
								updownImg += '<img src="images/empty16.png" width="16" height="16"></img>';
							}
							if (i != 0) {
								//Add Up image
								if (updownImg != "") {
									updownImg += "&nbsp;";
								}
								updownImg += '<img src="images/up.png" onclick="ChangeDeviceOrder(0,' + item.ID + ');" class="lcursor" width="16" height="16"></img>';
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
			if (typeof $scope.mytimer != 'undefined') {
				$interval.cancel($scope.mytimer);
				$scope.mytimer = undefined;
			}
			SceneIdx = idx;

			var bIsScene = (type == "Scene");
			$.isScene = bIsScene;

			var htmlcontent = '';
			htmlcontent += $('#editscene').html();
			$('#scenecontent').html(GetBackbuttonHTMLTable('ShowScenes') + htmlcontent);
			$('#scenecontent').i18n();
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
			var id = "#" + item.idx;
			var obj = $element.find(id);
			if (typeof obj != 'undefined') {
				if ($(id + " #name").html() != item.Name) {
					$(id + " #name").html(item.Name);
				}

				var backgroundClass = $rootScope.GetItemBackgroundStatus(item);
				obj.removeClass('statusNormal').removeClass('statusProtected').removeClass('statusTimeout').removeClass('statusLowBattery');
				obj.addClass(backgroundClass);

				var img1 = "";
				var img2 = "";

				var bigtext = TranslateStatusShort(item.Status);
				
				if (item.UsedByCamera == true) {
					var streamimg = '<img src="images/webcam.png" title="' + $.t('Stream Video') + '" height="16" width="16">';
					streamurl = "<a href=\"javascript:ShowCameraLiveStream('" + escape(item.Name) + "','" + item.CameraIdx + "')\">" + streamimg + "</a>";
					bigtext += "&nbsp;" + streamurl;
				}

				if (item.Type == "Scene") {
					img1 = '<img src="images/Push48_On.png" title="' + $.t('Activate scene') + '" onclick="SwitchScene(' + item.idx + ',\'On\',RefreshScenes,' + item.Protected + ');" class="lcursor" height="48" width="48">';
				}
				else {
					var onclass = "";
					var offclass = "";
					if (item.Status == 'On') {
						onclass = "transimg";
						offclass = "";
					}
					else if (item.Status == 'Off') {
						onclass = "";
						offclass = "transimg";
					}

					img1 = '<img class="lcursor ' + onclass + '" src="images/Push48_On.png" title="' + $.t('Turn On') + '" onclick="SwitchScene(' + item.idx + ',\'On\',RefreshScenes, ' + item.Protected + ');" height="48" width="48">';
					img2 = '<img class="lcursor ' + offclass + '"src="images/Push48_Off.png" title="' + $.t('Turn Off') + '" onclick="SwitchScene(' + item.idx + ',\'Off\',RefreshScenes, ' + item.Protected + ');" height="48" width="48">';
					if ($(id + " #img2").html() != img2) {
						$(id + " #img2").html(img2);
					}

					if ($(id + " #bigtext").html() != bigtext) {
						$(id + " #bigtext").html(bigtext);
					}

					if ($(id + " #status").html() != TranslateStatus(item.Status)) {
						$(id + " #status").html(TranslateStatus(item.Status));
					}
				}

				if ($(id + " #img1").html() != img1) {
					$(id + " #img1").html(img1);
				}

				if ($(id + " #lastupdate").html() != item.LastUpdate) {
					$(id + " #lastupdate").html(item.LastUpdate);
				}
				if ($scope.config.ShowUpdatedEffect == true) {
					$(id + " #name").effect("highlight", { color: '#EEFFEE' }, 1000);
				}
			}
		}

		//We only call this once. After this the widgets are being updated automatically by used of the websocket broadcast event.
		RefreshScenes = function () {
			if (typeof $scope.mytimer != 'undefined') {
				$interval.cancel($scope.mytimer);
				$scope.mytimer = undefined;
			}

			livesocket.getJson("json.htm?type=scenes&lastupdate=" + $.LastUpdateTime, function (data) {
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

			$scope.$on('scene_update', function (event, data) {
				if (typeof data.ServerTime != 'undefined') {
					$rootScope.SetTimeAndSun(data.Sunrise, data.Sunset, data.ServerTime);
				}
				if (typeof data.ActTime != 'undefined') {
					$.LastUpdateTime = parseInt(data.ActTime);
				}
				RefreshItem(data.item);
			});
		}

		ShowScenes = function () {
			if (typeof $scope.mytimer != 'undefined') {
				$interval.cancel($scope.mytimer);
				$scope.mytimer = undefined;
			}

			RefreshLightSwitchesComboArray();

			var htmlcontent = '';
			var bHaveAddedDevider = false;
			var bAllowWidgetReorder = true;

			var tophtm = "";

			tophtm +=
				'\t<table border="0" cellpadding="0" cellspacing="0" width="100%">\n' +
				'\t<tr>\n' +
				'\t  <td align="left" valign="top" id="timesun"></td>\n' +
				'\t</tr>\n' +
				'\t</table>\n';

			if (permissions.hasPermission("Admin")) {
				tophtm +=
					'\t<table id="bannav" class="bannav" border="0" cellpadding="0" cellspacing="0" width="100%">' +
					'\t<tr>' +
					'\t  <td align="left">' +
					'\t    <a class="btnstyle addscenebtn" onclick="AddScene();" data-i18n="Add Scene">Add Scene</a>' +
					'\t  </td>' +
					'\t</tr>' +
					'\t</table>\n';
			}

			var i = 0;
			var j = 0;

			$.ajax({
				url: "json.htm?type=scenes",
				async: false,
				dataType: 'json',
				success: function (data) {
					if (typeof data.result != 'undefined') {
						bAllowWidgetReorder = data.AllowWidgetOrdering;
						if (typeof data.ActTime != 'undefined') {
							$.LastUpdateTime = parseInt(data.ActTime);
						}

						$.each(data.result, function (i, item) {
							if (j % 3 == 0) {
								//add divider
								if (bHaveAddedDevider == true) {
									//close previous devider
									htmlcontent += '</div>\n';
								}
								htmlcontent += '<div class="row divider">\n';
								bHaveAddedDevider = true;
							}

							var backgroundClass = $rootScope.GetItemBackgroundStatus(item);
							var bAddTimer = true;
							var xhtm =
								'\t<div class="item span4 ' + backgroundClass + '" id="' + item.idx + '">\n' +
								'\t  <section>\n';
							if (item.Type == "Scene") {
								xhtm += '\t    <table id="itemtablenostatus" border="0" cellpadding="0" cellspacing="0">\n';
							}
							else {
								xhtm += '\t    <table id="itemtabledoubleicon" border="0" cellpadding="0" cellspacing="0">\n';
							}
							xhtm +=
								'\t    <tr>\n' +
								'\t      <td id="name">' + item.Name + '</td>\n' +
								'\t      <td id="bigtext">';
							var bigtext = TranslateStatusShort(item.Status);
							if (item.UsedByCamera == true) {
								var streamimg = '<img src="images/webcam.png" title="' + $.t('Stream Video') + '" height="16" width="16">';
								streamurl = "<a href=\"javascript:ShowCameraLiveStream('" + escape(item.Name) + "','" + item.CameraIdx + "')\">" + streamimg + "</a>";
								bigtext += "&nbsp;" + streamurl;
							}
							xhtm += bigtext + '</td>\n';

							if (item.Type == "Scene") {
								xhtm += '<td id="img1" class="img img1"><img src="images/Push48_On.png" title="' + $.t('Activate scene') + '" onclick="SwitchScene(' + item.idx + ',\'On\',RefreshScenes, ' + item.Protected + ');" class="lcursor" height="48" width="48"></td>\n';
								xhtm += '\t      <td id="status" class="status"><span>&nbsp;</span></td>\n';
							}
							else {
								var onclass = "";
								var offclass = "";
								if (item.Status == 'On') {
									onclass = "transimg";
									offclass = "";
								}
								else if (item.Status == 'Off') {
									onclass = "";
									offclass = "transimg";
								}

								xhtm += '<td id="img1" class="img img1"><img class="lcursor ' + onclass + '" src="images/Push48_On.png" title="' + $.t('Turn On') + '" onclick="SwitchScene(' + item.idx + ',\'On\',RefreshScenes, ' + item.Protected + ');" height="48" width="48"></td>\n';
								xhtm += '<td id="img2" class="img img2"><img class="lcursor ' + offclass + '"src="images/Push48_Off.png" title="' + $.t('Turn Off') + '" onclick="SwitchScene(' + item.idx + ',\'Off\',RefreshScenes, ' + item.Protected + ');" height="48" width="48"></td>\n';
								xhtm += '\t      <td id="status" class="status">&nbsp;</td>\n';
							}
							xhtm +=
								'\t      <td id="lastupdate" class="lastupdate">' + item.LastUpdate + '</td>\n' +
								'\t      <td id="type">' + $.t(item.Type) + '</td>\n';
							xhtm += '\t      <td class="options">';
							if (item.Favorite == 0) {
								xhtm +=
									'<img src="images/nofavorite.png" title="' + $.t('Add to Dashboard') + '" onclick="MakeFavorite(' + item.idx + ',1);" class="favorite favoriteOff lcursor">&nbsp;&nbsp;&nbsp;&nbsp;';
							}
							else {
								xhtm +=
									'<img src="images/favorite.png" title="' + $.t('Remove from Dashboard') + '" onclick="MakeFavorite(' + item.idx + ',0);" class="favorite favoriteOn lcursor">&nbsp;&nbsp;&nbsp;&nbsp;';
							}
							xhtm += '<a class="btnsmall" href="#/Scenes/' + item.idx + '/Log" data-i18n="Log">Log</a> ';

							if (permissions.hasPermission("Admin")) {
								xhtm += '<a class="btnsmall" onclick="EditSceneDevice(' + item.idx + ',\'' + escape(item.Name) + '\',\'' + escape(item.Description) + '\',' + item.HardwareID + ',\'' + item.Type + '\', ' + item.Protected + ',\'' + item.OnAction + '\', \'' + item.OffAction + '\');" data-i18n="Edit">Edit</a> ';
								if (bAddTimer == true) {
									if (item.Timers == "true") {
										xhtm += '<a class="btnsmall-sel" href="#/Scenes/' + item.idx + '/Timers" data-i18n="Timers">Timers</a> ';
									}
									else {
										xhtm += '<a class="btnsmall" href="#/Scenes/' + item.idx + '/Timers" data-i18n="Timers">Timers</a> ';
									}
								}
							}
							xhtm +=
								'</td>\n' +
								'\t    </tr>\n' +
								'\t    </table>\n' +
								'\t  </section>\n' +
								'\t</div>\n';
							htmlcontent += xhtm;
							j += 1;
						});
					}
				}
			});
			if (bHaveAddedDevider == true) {
				//close previous devider
				htmlcontent += '</div>\n';
			}
			if (htmlcontent == '') {
				htmlcontent = '<h2>' + $.t('No Scenes defined yet...') + '</h2>';
			}
			$element.html(tophtm + htmlcontent);
			$element.i18n();

			$rootScope.RefreshTimeAndSun();

			if (bAllowWidgetReorder == true) {
				if (permissions.hasPermission("Admin")) {
					if (window.myglobals.ismobileint == false) {
						$element.find(".span4").draggable({
							drag: function () {
								if (typeof $scope.mytimer != 'undefined') {
									$interval.cancel($scope.mytimer);
									$scope.mytimer = undefined;
								}
								SceneIdx = $(this).attr("id");
								$(this).css("z-index", 2);
							},
							revert: true
						});
						$element.find(".span4").droppable({
							drop: function () {
								var myid = $(this).attr("id");
								$.ajax({
									url: "json.htm?type=command&param=switchsceneorder&idx1=" + myid + "&idx2=" + SceneIdx,
									async: false,
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
			RefreshScenes();
			return false;
		}

		init();

		function init() {
			SceneIdx = 0;
			$scope.MakeGlobalConfig();

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
								url: "json.htm?type=addscene&name=" + SceneName + "&scenetype=" + SceneType,
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
			ShowScenes();
		};
		$scope.$on('$destroy', function () {
			if (typeof $scope.mytimer != 'undefined') {
				$interval.cancel($scope.mytimer);
				$scope.mytimer = undefined;
			}
		});
	});
});
