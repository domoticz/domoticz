define(['app'], function (app) {
	app.controller('LightsController', ['$scope', '$rootScope', '$location', '$http', '$interval', 'permissions', function ($scope, $rootScope, $location, $http, $interval, permissions) {

		$scope.HasInitializedAddManualDialog = false;
		$scope.HasInitializedEditLightDialog = false;

		DeleteTimer = function (idx) {
			bootbox.confirm($.t("Are you sure to delete this timers?\n\nThis action can not be undone..."), function (result) {
				if (result == true) {
					$.ajax({
						url: "json.htm?type=command&param=deletetimer&idx=" + idx,
						async: false,
						dataType: 'json',
						success: function (data) {
							RefreshTimerTable($.devIdx);
						},
						error: function () {
							HideNotify();
							ShowNotify($.t('Problem deleting timer!'), 2500, true);
						}
					});
				}
			});
		}

		ClearTimers = function () {
			bootbox.confirm($.t("Are you sure to delete ALL timers?\n\nThis action can not be undone!"), function (result) {
				if (result == true) {
					$.ajax({
						url: "json.htm?type=command&param=cleartimers&idx=" + $.devIdx,
						async: false,
						dataType: 'json',
						success: function (data) {
							RefreshTimerTable($.devIdx);
						},
						error: function () {
							HideNotify();
							ShowNotify($.t('Problem clearing timers!'), 2500, true);
						}
					});
				}
			});
		}

		GetTimerSettings = function () {
			var tsettings = {};
			tsettings.level = 100;
			tsettings.hue = 0;
			tsettings.Active = $('#lightcontent #timerparamstable #enabled').is(":checked");
			tsettings.timertype = $("#lightcontent #timerparamstable #combotype").val();
			tsettings.date = $("#lightcontent #timerparamstable #sdate").val();
			tsettings.hour = $("#lightcontent #timerparamstable #combotimehour").val();
			tsettings.min = $("#lightcontent #timerparamstable #combotimemin").val();
			tsettings.Randomness = $('#lightcontent #timerparamstable #randomness').is(":checked");
			tsettings.cmd = $("#lightcontent #timerparamstable #combocommand").val();
			tsettings.days = 0;
			var everyday = $("#lightcontent #timerparamstable #when_1").is(":checked");
			var weekdays = $("#lightcontent #timerparamstable #when_2").is(":checked");
			var weekends = $("#lightcontent #timerparamstable #when_3").is(":checked");
			if (everyday == true)
				tsettings.days = 0x80;
			else if (weekdays == true)
				tsettings.days = 0x100;
			else if (weekends == true)
				tsettings.days = 0x200;
			else {
				if ($('#lightcontent #timerparamstable #ChkMon').is(":checked"))
					tsettings.days |= 0x01;
				if ($('#lightcontent #timerparamstable #ChkTue').is(":checked"))
					tsettings.days |= 0x02;
				if ($('#lightcontent #timerparamstable #ChkWed').is(":checked"))
					tsettings.days |= 0x04;
				if ($('#lightcontent #timerparamstable #ChkThu').is(":checked"))
					tsettings.days |= 0x08;
				if ($('#lightcontent #timerparamstable #ChkFri').is(":checked"))
					tsettings.days |= 0x10;
				if ($('#lightcontent #timerparamstable #ChkSat').is(":checked"))
					tsettings.days |= 0x20;
				if ($('#lightcontent #timerparamstable #ChkSun').is(":checked"))
					tsettings.days |= 0x40;
			}
			tsettings.mday = $("#lightcontent #timerparamstable #days").val();
			tsettings.month = $("#lightcontent #timerparamstable #months").val();
			tsettings.occurence = $("#lightcontent #timerparamstable #occurence").val();
			tsettings.weekday = $("#lightcontent #timerparamstable #weekdays").val();
			if (tsettings.cmd == 0) {
				if ($.bIsLED) {
					tsettings.level = $("#lightcontent #Brightness").val();
					tsettings.hue = $("#lightcontent #Hue").val();
					var bIsWhite = $('#lightcontent #ledtable #optionWhite').is(":checked")
					if (bIsWhite == true) {
						tsettings.hue = 1000;
					}
				} else if ($.isDimmer || $.isSelector) {
					tsettings.level = $("#lightcontent #timerparamstable #combolevel").val();
				}
			}
			return tsettings;
		}

		UpdateTimer = function (idx) {
			var tsettings = GetTimerSettings();
			if (tsettings.timertype == 5) {
				if (tsettings.date == "") {
					ShowNotify($.t('Please select a Date!'), 2500, true);
					return;
				}
				//Check if date/time is valid
				var pickedDate = $("#lightcontent #timerparamstable #sdate").datepicker('getDate');
				var checkDate = new Date(pickedDate.getFullYear(), pickedDate.getMonth(), pickedDate.getDate(), tsettings.hour, tsettings.min, 0, 0);
				var nowDate = new Date();
				if (checkDate < nowDate) {
					ShowNotify($.t('Invalid Date selected!'), 2500, true);
					return;
				}
			}
			else if ((tsettings.timertype == 6) || (tsettings.timertype == 7)) {
				tsettings.days = 0x80;
			}
			else if (tsettings.timertype == 10) {
				tsettings.days = 0x80;
				if (tsettings.mday > 28) {
					ShowNotify($.t('Not al months have this amount of days, some months will be skipped!'), 2500, true);
				}
			}
			else if (tsettings.timertype == 12) {
				tsettings.days = 0x80;
				if ((tsettings.month == 4 || tsettings.month == 6 || tsettings.month == 9 || tsettings.month == 11) && tsettings.mday == 31) {
					ShowNotify($.t('This month does not have 31 days!'), 2500, true);
					return;
				}
				if (tsettings.month == 2) {
					if (tsettings.mday > 29) {
						ShowNotify($.t('February does not have more than 29 days!'), 2500, true);
						return;
					}
					if (tsettings.mday == 29) {
						ShowNotify($.t('Not all years have this date, some years will be skipped!'), 2500, true);
					}
				}
			}
			else if ((tsettings.timertype == 11) || (tsettings.timertype == 13)) {
				tsettings.days = Math.pow(2, tsettings.weekday);
			}
			else if (tsettings.days == 0) {
				ShowNotify($.t('Please select some days!'), 2500, true);
				return;
			}
			$.ajax({
				url: "json.htm?type=command&param=updatetimer&idx=" + idx +
				"&active=" + tsettings.Active +
				"&timertype=" + tsettings.timertype +
				"&date=" + tsettings.date +
				"&hour=" + tsettings.hour +
				"&min=" + tsettings.min +
				"&randomness=" + tsettings.Randomness +
				"&command=" + tsettings.cmd +
				"&level=" + tsettings.level +
				"&hue=" + tsettings.hue +
				"&days=" + tsettings.days +
				"&mday=" + tsettings.mday +
				"&month=" + tsettings.month +
				"&occurence=" + tsettings.occurence,
				async: false,
				dataType: 'json',
				success: function (data) {
					RefreshTimerTable($.devIdx);
				},
				error: function () {
					HideNotify();
					ShowNotify($.t('Problem updating timer!'), 2500, true);
				}
			});
		}

		AddTimer = function () {
			var tsettings = GetTimerSettings();
			if (tsettings.timertype == 5) {
				if (tsettings.date == "") {
					ShowNotify($.t('Please select a Date!'), 2500, true);
					return;
				}
				//Check if date/time is valid
				var pickedDate = $("#lightcontent #timerparamstable #sdate").datepicker('getDate');
				var checkDate = new Date(pickedDate.getFullYear(), pickedDate.getMonth(), pickedDate.getDate(), tsettings.hour, tsettings.min, 0, 0);
				var nowDate = new Date();
				if (checkDate < nowDate) {
					ShowNotify($.t('Invalid Date selected!'), 2500, true);
					return;
				}
			}
			else if ((tsettings.timertype == 6) || (tsettings.timertype == 7)) {
				tsettings.days = 0x80;
			}
			else if (tsettings.timertype == 10) {
				tsettings.days = 0x80;
				if (tsettings.mday > 28) {
					ShowNotify($.t('Not al months have this amount of days, some months will be skipped!'), 2500, true);
				}
			}
			else if (tsettings.timertype == 12) {
				tsettings.days = 0x80;
				if ((tsettings.month == 4 || tsettings.month == 6 || tsettings.month == 9 || tsettings.month == 11) && tsettings.mday == 31) {
					ShowNotify($.t('This month does not have 31 days!'), 2500, true);
					return;
				}
				if (tsettings.month == 2) {
					if (tsettings.mday > 29) {
						ShowNotify($.t('February does not have more than 29 days!'), 2500, true);
						return;
					}
					if (tsettings.mday == 29) {
						ShowNotify($.t('Not all years have this date, some years will be skipped!'), 2500, true);
					}
				}
			}
			else if ((tsettings.timertype == 11) || (tsettings.timertype == 13)) {
				tsettings.days = Math.pow(2, tsettings.weekday);
			}
			else if (tsettings.days == 0) {
				ShowNotify($.t('Please select some days!'), 2500, true);
				return;
			}
			$.ajax({
				url: "json.htm?type=command&param=addtimer&idx=" + $.devIdx +
				"&active=" + tsettings.Active +
				"&timertype=" + tsettings.timertype +
				"&date=" + tsettings.date +
				"&hour=" + tsettings.hour +
				"&min=" + tsettings.min +
				"&randomness=" + tsettings.Randomness +
				"&command=" + tsettings.cmd +
				"&level=" + tsettings.level +
				"&hue=" + tsettings.hue +
				"&days=" + tsettings.days +
				"&mday=" + tsettings.mday +
				"&month=" + tsettings.month +
				"&occurence=" + tsettings.occurence,
				async: false,
				dataType: 'json',
				success: function (data) {
					RefreshTimerTable($.devIdx);
				},
				error: function () {
					HideNotify();
					ShowNotify($.t('Problem adding timer!'), 2500, true);
				}
			});
		}

		EnableDisableDays = function (TypeStr, bDisabled) {
			$('#lightcontent #timerparamstable #ChkMon').prop('checked', ((TypeStr.indexOf("Mon") >= 0) || (TypeStr == "Everyday") || (TypeStr == "Weekdays")) ? true : false);
			$('#lightcontent #timerparamstable #ChkTue').prop('checked', ((TypeStr.indexOf("Tue") >= 0) || (TypeStr == "Everyday") || (TypeStr == "Weekdays")) ? true : false);
			$('#lightcontent #timerparamstable #ChkWed').prop('checked', ((TypeStr.indexOf("Wed") >= 0) || (TypeStr == "Everyday") || (TypeStr == "Weekdays")) ? true : false);
			$('#lightcontent #timerparamstable #ChkThu').prop('checked', ((TypeStr.indexOf("Thu") >= 0) || (TypeStr == "Everyday") || (TypeStr == "Weekdays")) ? true : false);
			$('#lightcontent #timerparamstable #ChkFri').prop('checked', ((TypeStr.indexOf("Fri") >= 0) || (TypeStr == "Everyday") || (TypeStr == "Weekdays")) ? true : false);
			$('#lightcontent #timerparamstable #ChkSat').prop('checked', ((TypeStr.indexOf("Sat") >= 0) || (TypeStr == "Everyday") || (TypeStr == "Weekends")) ? true : false);
			$('#lightcontent #timerparamstable #ChkSun').prop('checked', ((TypeStr.indexOf("Sun") >= 0) || (TypeStr == "Everyday") || (TypeStr == "Weekends")) ? true : false);

			$('#lightcontent #timerparamstable #ChkMon').attr('disabled', bDisabled);
			$('#lightcontent #timerparamstable #ChkTue').attr('disabled', bDisabled);
			$('#lightcontent #timerparamstable #ChkWed').attr('disabled', bDisabled);
			$('#lightcontent #timerparamstable #ChkThu').attr('disabled', bDisabled);
			$('#lightcontent #timerparamstable #ChkFri').attr('disabled', bDisabled);
			$('#lightcontent #timerparamstable #ChkSat').attr('disabled', bDisabled);
			$('#lightcontent #timerparamstable #ChkSun').attr('disabled', bDisabled);
		}

		RefreshTimerTable = function (idx) {
			$('#modal').show();

			$('#updelclr #timerupdate').attr("class", "btnstyle3-dis");
			$('#updelclr #timerdelete').attr("class", "btnstyle3-dis");

			var oTable = $('#timertable').dataTable();
			oTable.fnClearTable();

			$.ajax({
				url: "json.htm?type=timers&idx=" + idx,
				async: false,
				dataType: 'json',
				success: function (data) {

					if (typeof data.result != 'undefined') {
						$.each(data.result, function (i, item) {
							var active = "No";
							if (item.Active == "true")
								active = "Yes";
							var Command = "On";
							if (item.Cmd == 1) {
								Command = "Off";
							}
							var tCommand = Command;
							if ((Command == "On") && ($.isDimmer || $.isSelector)) {
								if ($.isSelector) {
									tCommand += " (" + $.selectorSwitchLevels[item.Level / 10] + ")";

								} else {
									tCommand += " (" + item.Level + "%)";
									if ($.bIsLED) {
										var hue = item.Hue;
										var sat = 100;
										if (hue == 1000) {
											hue = 0;
											sat = 0;
										}
										var cHSB = [];
										cHSB.h = hue;
										cHSB.s = sat;
										cHSB.b = item.Level;
										tCommand += '<div id="picker4" class="ex-color-box" style="background-color: #' + $.colpickHsbToHex(cHSB) + ';"></div>';
									}
								}
							}

							var DayStr = "";
							var DayStrOrig = "";
							if ((item.Type <= 4) || (item.Type == 8) || (item.Type == 9)) {
								var dayflags = parseInt(item.Days);
								if (dayflags & 0x80)
									DayStrOrig = "Everyday";
								else if (dayflags & 0x100)
									DayStrOrig = "Weekdays";
								else if (dayflags & 0x200)
									DayStrOrig = "Weekends";
								else {
									if (dayflags & 0x01) {
										if (DayStrOrig != "") DayStrOrig += ", ";
										DayStrOrig += "Mon";
									}
									if (dayflags & 0x02) {
										if (DayStrOrig != "") DayStrOrig += ", ";
										DayStrOrig += "Tue";
									}
									if (dayflags & 0x04) {
										if (DayStrOrig != "") DayStrOrig += ", ";
										DayStrOrig += "Wed";
									}
									if (dayflags & 0x08) {
										if (DayStrOrig != "") DayStrOrig += ", ";
										DayStrOrig += "Thu";
									}
									if (dayflags & 0x10) {
										if (DayStrOrig != "") DayStrOrig += ", ";
										DayStrOrig += "Fri";
									}
									if (dayflags & 0x20) {
										if (DayStrOrig != "") DayStrOrig += ", ";
										DayStrOrig += "Sat";
									}
									if (dayflags & 0x40) {
										if (DayStrOrig != "") DayStrOrig += ", ";
										DayStrOrig += "Sun";
									}
								}
							}
							else if (item.Type == 10) {
								DayStrOrig = "Monthly on Day " + item.MDay;
							}
							else if (item.Type == 11) {
								var Weekday = Math.log(parseInt(item.Days)) / Math.log(2);
								DayStrOrig = "Monthly on " + $.myglobals.OccurenceStr[item.Occurence - 1] + " " + $.myglobals.WeekdayStr[Weekday];
							}
							else if (item.Type == 12) {
								DayStrOrig = "Yearly on " + item.MDay + " " + $.myglobals.MonthStr[item.Month - 1];
							}
							else if (item.Type == 13) {
								var Weekday = Math.log(parseInt(item.Days)) / Math.log(2);
								DayStrOrig = "Yearly on " + $.myglobals.OccurenceStr[item.Occurence - 1] + " " + $.myglobals.WeekdayStr[Weekday] + " in " + $.myglobals.MonthStr[item.Month - 1];
							}

							//translate daystring
							var splitstr = ", ";
							if (item.Type > 5) {
								splitstr = " ";
							}
							var res = DayStrOrig.split(splitstr);
							$.each(res, function (i, item) {
								DayStr += $.t(item);
								if (i != res.length - 1) {
									DayStr += splitstr;
								}
							});

							var rEnabled = "No";
							if (item.Randomness == "true") {
								rEnabled = "Yes";
							}

							var addId = oTable.fnAddData({
								"DT_RowId": item.idx,
								"Active": active,
								"Command": Command,
								"Level": item.Level,
								"Hue": item.Hue,
								"TType": item.Type,
								"TTypeString": $.myglobals.TimerTypesStr[item.Type],
								"Random": rEnabled,
								"Days": DayStrOrig,
								"0": $.t(active),
								"1": $.t($.myglobals.TimerTypesStr[item.Type]),
								"2": item.Date,
								"3": item.Time,
								"4": $.t(rEnabled),
								"5": $.t(tCommand),
								"6": DayStr,
								"7": item.Month,
								"8": item.MDay,
								"9": item.Occurence,
								"10": Math.log(parseInt(item.Days)) / Math.log(2)
							});
						});
					}
				}
			});

			/* Add a click handler to the rows - this could be used as a callback */
			$("#timertable tbody").off();
			$("#timertable tbody").on('click', 'tr', function () {
				if ($(this).hasClass('row_selected')) {
					$(this).removeClass('row_selected');
					$('#updelclr #timerupdate').attr("class", "btnstyle3-dis");
					$('#updelclr #timerdelete').attr("class", "btnstyle3-dis");
				}
				else {
					var oTable = $('#timertable').dataTable();
					oTable.$('tr.row_selected').removeClass('row_selected');
					$(this).addClass('row_selected');
					$('#updelclr #timerupdate').attr("class", "btnstyle3");
					$('#updelclr #timerdelete').attr("class", "btnstyle3");
					var anSelected = fnGetSelected(oTable);
					if (anSelected.length !== 0) {
						var data = oTable.fnGetData(anSelected[0]);
						var idx = data["DT_RowId"];
						$.myglobals.SelectedTimerIdx = idx;
						$("#updelclr #timerupdate").attr("href", "javascript:UpdateTimer(" + idx + ")");
						$("#updelclr #timerdelete").attr("href", "javascript:DeleteTimer(" + idx + ")");
						//update user interface with the paramters of this row
						$('#lightcontent #timerparamstable #enabled').prop('checked', (data["Active"] == "Yes") ? true : false);
						$("#lightcontent #timerparamstable #combotype").val(jQuery.inArray(data["TTypeString"], $.myglobals.TimerTypesStr));
						$("#lightcontent #timerparamstable #combotimehour").val(parseInt(data["3"].substring(0, 2)));
						$("#lightcontent #timerparamstable #combotimemin").val(parseInt(data["3"].substring(3, 5)));
						$('#lightcontent #timerparamstable #randomness').prop('checked', (data["Random"] == "Yes") ? true : false);
						var command = jQuery.inArray(data["Command"], $.myglobals.CommandStr);
						$("#lightcontent #timerparamstable #combocommand").val(command);
						var level = data["Level"];
						if ($.bIsLED) {
							$('#lightcontent #Brightness').val(level & 255);
							var hue = data["Hue"];
							var sat = 100;
							if (hue == 1000) {
								hue = 0;
								sat = 0;
							}
							$('#lightcontent #Hue').val(hue);
							var cHSB = [];
							cHSB.h = hue;
							cHSB.s = sat;
							cHSB.b = level;

							$("#lightcontent #optionRGB").prop('checked', (sat == 100));
							$("#lightcontent #optionWhite").prop('checked', !(sat == 100));

							$('#lightcontent #picker').colpickSetColor(cHSB);
						}
						else if ($.isDimmer || $.isSelector) {
							$("#lightcontent #LevelDiv").hide();
							$("#lightcontent #timerparamstable #combolevel").val(level);
							if (command === 0) { // On
								$("#lightcontent #LevelDiv").show();
							}
						}

						var timerType = data["TType"];
						if (timerType == 5) {
							$("#lightcontent #timerparamstable #sdate").val(data["2"]);
							$("#lightcontent #timerparamstable #rdate").show();
							$("#lightcontent #timerparamstable #rnorm").hide();
							$("#lightcontent #timerparamstable #rdays").hide();
							$("#lightcontent #timerparamstable #roccurence").hide();
							$("#lightcontent #timerparamstable #rmonths").hide();
						}
						else if ((timerType == 6) || (timerType == 7)) {
							$("#lightcontent #timerparamstable #rdate").hide();
							$("#lightcontent #timerparamstable #rnorm").hide();
							$("#lightcontent #timerparamstable #rdays").hide();
							$("#lightcontent #timerparamstable #roccurence").hide();
							$("#lightcontent #timerparamstable #rmonths").hide();
						}
						else if (timerType == 10) {
							$("#lightcontent #timerparamstable #days").val(data["8"]);
							$("#lightcontent #timerparamstable #rdate").hide();
							$("#lightcontent #timerparamstable #rnorm").hide();
							$("#lightcontent #timerparamstable #rdays").show();
							$("#lightcontent #timerparamstable #roccurence").hide();
							$("#lightcontent #timerparamstable #rmonths").hide();
						}
						else if (timerType == 11) {
							$("#lightcontent #timerparamstable #occurence").val(data["9"]);
							$("#lightcontent #timerparamstable #weekdays").val(data["10"]);
							$("#lightcontent #timerparamstable #rdate").hide();
							$("#lightcontent #timerparamstable #rnorm").hide();
							$("#lightcontent #timerparamstable #rdays").hide();
							$("#lightcontent #timerparamstable #roccurence").show();
							$("#lightcontent #timerparamstable #rmonths").hide();
						}
						else if (timerType == 12) {
							$("#lightcontent #timerparamstable #months").val(data["7"]);
							$("#lightcontent #timerparamstable #days").val(data["8"]);
							$("#lightcontent #timerparamstable #rdate").hide();
							$("#lightcontent #timerparamstable #rnorm").hide();
							$("#lightcontent #timerparamstable #rdays").show();
							$("#lightcontent #timerparamstable #roccurence").hide();
							$("#lightcontent #timerparamstable #rmonths").show();
						}
						else if (timerType == 13) {
							$("#lightcontent #timerparamstable #months").val(data["7"]);
							$("#lightcontent #timerparamstable #occurence").val(data["9"]);
							$("#lightcontent #timerparamstable #weekdays").val(data["10"]);
							$("#lightcontent #timerparamstable #rdate").hide();
							$("#lightcontent #timerparamstable #rnorm").hide();
							$("#lightcontent #timerparamstable #rdays").hide();
							$("#lightcontent #timerparamstable #roccurence").show();
							$("#lightcontent #timerparamstable #rmonths").show();
						}
						else {
							$("#lightcontent #timerparamstable #rdate").hide();
							$("#lightcontent #timerparamstable #rnorm").show();
							$("#lightcontent #timerparamstable #rdays").hide();
							$("#lightcontent #timerparamstable #roccurence").hide();
							$("#lightcontent #timerparamstable #rmonths").hide();
						}

						var disableDays = false;
						if (data["Days"] == "Everyday") {
							$("#lightcontent #timerparamstable #when_1").prop('checked', 'checked');
							disableDays = true;
						}
						else if (data["Days"] == "Weekdays") {
							$("#lightcontent #timerparamstable #when_2").prop('checked', 'checked');
							disableDays = true;
						}
						else if (data["Days"] == "Weekends") {
							$("#lightcontent #timerparamstable #when_3").prop('checked', 'checked');
							disableDays = true;
						}
						else
							$("#lightcontent #timerparamstable #when_4").prop('checked', 'checked');

						EnableDisableDays(data["Days"], disableDays);
					}
				}
			});

			$rootScope.RefreshTimeAndSun();

			$('#modal').hide();
		}

		ShowTimers = function (id, name, isdimmer, stype, devsubtype) {
			if (typeof $scope.mytimer != 'undefined') {
				$interval.cancel($scope.mytimer);
				$scope.mytimer = undefined;
			}
			$.devIdx = id;
			$.isDimmer = isdimmer;
			$.isSelector = (devsubtype === "Selector Switch");

			$.bIsRGBWW = (devsubtype.indexOf("RGBWW") >= 0);
			$.bIsRGBW = (devsubtype.indexOf("RGBW") >= 0);
			$.bIsLED = (devsubtype.indexOf("RGB") >= 0);

			if ($.isSelector) {
				// backup selector switch level names before displaying edit edit form
				var selectorSwitch$ = $("#selector" + $.devIdx);
				$.selectorSwitchLevels = unescape(selectorSwitch$.data("levelnames")).split('|');
				$.selectorSwitchLevelOffHidden = selectorSwitch$.data("leveloffhidden");
			}
			var oTable;

			$('#modal').show();
			var htmlcontent = '';
			htmlcontent = '<p><h2><span data-i18n="Name"></span>: ' + unescape(name) + '</h2></p><br>\n';

			var sunRise = "";
			var sunSet = "";
			$.ajax({
				url: "json.htm?type=command&param=getSunRiseSet",
				async: false,
				dataType: 'json',
				success: function (data) {
					if (typeof data.Sunrise != 'undefined') {
						sunRise = data.Sunrise;
						sunSet = data.Sunset;
					}
				}
			});

			var suntext = '<div id="timesun" /><br>\n';
			htmlcontent += suntext;

			htmlcontent += $('#edittimers').html();
			$('#lightcontent').html(GetBackbuttonHTMLTable('ShowLights') + htmlcontent);
			$('#lightcontent').i18n();
			$("#lightcontent #timerparamstable #rdate").hide();
			$("#lightcontent #timerparamstable #rnorm").show();
			$("#lightcontent #timerparamstable #rdays").hide();
			$("#lightcontent #timerparamstable #roccurence").hide();
			$("#lightcontent #timerparamstable #rmonths").hide();

			$rootScope.RefreshTimeAndSun();

			var nowTemp = new Date();
			var now = new Date(nowTemp.getFullYear(), nowTemp.getMonth(), nowTemp.getDate(), 0, 0, 0, 0);

			$("#lightcontent #sdate").datepicker({
				minDate: now,
				defaultDate: now,
				dateFormat: "mm/dd/yy",
				showWeek: true,
				firstDay: 1
			});
			$("#lightcontent #combotype").change(function () {
				var timerType = $("#lightcontent #combotype").val();
				if (timerType == 5) {
					$("#lightcontent #timerparamstable #rdate").show();
					$("#lightcontent #timerparamstable #rnorm").hide();
					$("#lightcontent #timerparamstable #rdays").hide();
					$("#lightcontent #timerparamstable #roccurence").hide();
					$("#lightcontent #timerparamstable #rmonths").hide();
				}
				else if ((timerType == 6) || (timerType == 7)) {
					$("#lightcontent #timerparamstable #rdate").hide();
					$("#lightcontent #timerparamstable #rnorm").hide();
					$("#lightcontent #timerparamstable #rdays").hide();
					$("#lightcontent #timerparamstable #roccurence").hide();
					$("#lightcontent #timerparamstable #rmonths").hide();
				}
				else if (timerType == 10) {
					$("#lightcontent #timerparamstable #rdate").hide();
					$("#lightcontent #timerparamstable #rnorm").hide();
					$("#lightcontent #timerparamstable #rdays").show();
					$("#lightcontent #timerparamstable #roccurence").hide();
					$("#lightcontent #timerparamstable #rmonths").hide();
				}
				else if (timerType == 11) {
					$("#lightcontent #timerparamstable #rdate").hide();
					$("#lightcontent #timerparamstable #rnorm").hide();
					$("#lightcontent #timerparamstable #rdays").hide();
					$("#lightcontent #timerparamstable #roccurence").show();
					$("#lightcontent #timerparamstable #rmonths").hide();
				}
				else if (timerType == 12) {
					$("#lightcontent #timerparamstable #rdate").hide();
					$("#lightcontent #timerparamstable #rnorm").hide();
					$("#lightcontent #timerparamstable #rdays").show();
					$("#lightcontent #timerparamstable #roccurence").hide();
					$("#lightcontent #timerparamstable #rmonths").show();
				}
				else if (timerType == 13) {
					$("#lightcontent #timerparamstable #rdate").hide();
					$("#lightcontent #timerparamstable #rnorm").hide();
					$("#lightcontent #timerparamstable #rdays").hide();
					$("#lightcontent #timerparamstable #roccurence").show();
					$("#lightcontent #timerparamstable #rmonths").show();
				}
				else {
					$("#lightcontent #timerparamstable #rdate").hide();
					$("#lightcontent #timerparamstable #rnorm").show();
					$("#lightcontent #timerparamstable #rdays").hide();
					$("#lightcontent #timerparamstable #roccurence").hide();
					$("#lightcontent #timerparamstable #rmonths").hide();
				}
			});

			var sat = 180;
			var cHSB = [];
			cHSB.h = 128;
			cHSB.s = sat;
			cHSB.b = 100;
			$('#lightcontent #Brightness').val(100);
			$('#lightcontent #Hue').val(128);

			if ($.bIsLED == true) {
				$("#lightcontent #LedColor").show();
			}
			else {
				$("#lightcontent #LedColor").hide();
			}
			if ($.bIsRGBW == true) {
				$("#lightcontent #optionsRGBW").show();
			}
			else {
				$("#lightcontent #optionsRGBW").hide();
			}
			$('#lightcontent #picker').colpick({
				flat: true,
				layout: 'hex',
				submit: 0,
				onChange: function (hsb, hex, rgb, fromSetColor) {
					if (!fromSetColor) {
						$('#lightcontent #Hue').val(hsb.h);
						$('#lightcontent #Brightness').val(hsb.b);
						var bIsWhite = (hsb.s < 20);
						$("#lightcontent #optionRGB").prop('checked', !bIsWhite);
						$("#lightcontent #optionWhite").prop('checked', bIsWhite);
						clearInterval($.setColValue);
						$.setColValue = setInterval(function () { SetColValue($.devIdx, hsb.h, hsb.b, bIsWhite); }, 400);
					}
				}
			});

			$("#lightcontent #optionRGB").prop('checked', (sat == 180));
			$("#lightcontent #optionWhite").prop('checked', !(sat == 180));

			$('#lightcontent #picker').colpickSetColor(cHSB);

			oTable = $('#timertable').dataTable({
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
			$('#timerparamstable #combotimehour >option').remove();
			$('#timerparamstable #combotimemin >option').remove();
			$('#timerparamstable #days >option').remove();

			//fill hour/minute/days comboboxes
			for (ii = 0; ii < 24; ii++) {
				$('#timerparamstable #combotimehour').append($('<option></option>').val(ii).html($.strPad(ii, 2)));
			}
			for (ii = 0; ii < 60; ii++) {
				$('#timerparamstable #combotimemin').append($('<option></option>').val(ii).html($.strPad(ii, 2)));
			}
			for (ii = 1; ii <= 31; ii++) {
				$('#timerparamstable #days').append($('<option></option>').val(ii).html(ii));
			}

			$("#lightcontent #timerparamstable #when_1").click(function () {
				EnableDisableDays("Everyday", true);
			});
			$("#lightcontent #timerparamstable #when_2").click(function () {
				EnableDisableDays("Weekdays", true);
			});
			$("#lightcontent #timerparamstable #when_3").click(function () {
				EnableDisableDays("Weekends", true);
			});
			$("#lightcontent #timerparamstable #when_4").click(function () {
				EnableDisableDays("", false);
			});

			$("#lightcontent #timerparamstable #combocommand").change(function () {
				var cval = $("#lightcontent #timerparamstable #combocommand").val(),
					lval = $("#lightcontent #timerparamstable #combolevel").val(),
					bShowLevel = false;
				if (!$.bIsLED) {
					if ($.isDimmer || $.isSelector) {
						if (cval == 0) {
							bShowLevel = true;
						}
					}
				}
				if (bShowLevel == true) {
					if (lval == null) {
						if ($.isSelector) {
							$("#lightcontent #timerparamstable #combolevel").val(10); // first selector level value
						} else {
							$("#lightcontent #timerparamstable #combolevel").val(5); // first dimmer level value
						}
					}
					$("#lightcontent #LevelDiv").show();
				}
				else {
					$("#lightcontent #LevelDiv").hide();
				}
			});

			if (($.isDimmer) && (!$.bIsLED)) {
				$("#lightcontent #LevelDiv").show();

			} else if ($.isSelector) {
				// Replace dimmer levels by selector level names
				var levelDiv$ = $("#lightcontent #LevelDiv"),
					html = [];
				if ($.selectorSwitchLevelOffHidden) {
					$("#lightcontent #combocommand").prop('disabled', true);
				}
				$.each($.selectorSwitchLevels, function (index, item) {
					var level = index * 10,
						levelName = item;
					if (level === 0) {
						return;
					}
					html.push('<option value="');
					html.push(level);
					html.push('">');
					html.push(levelName);
					html.push('</option>');
				});
				levelDiv$.find('select')
					.attr('style', '')
					.css({ 'width': 'auto' })
					.html(html.join(''));
				levelDiv$.find('span[data-i18n="Level"]')
					.attr('data-i18n', "Level name")
					.html($.t("Level name"));
				levelDiv$.show();

			} else {
				$("#lightcontent #LevelDiv").hide();
			}

			$('#modal').hide();
			RefreshTimerTable(id);
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
				url: "json.htm?type=command&param=makefavorite&idx=" + id + "&isfavorite=" + isfavorite,
				async: false,
				dataType: 'json',
				success: function (data) {
					ShowLights();
				}
			});
		}

		DeleteLightSwitchIntern = function (bRemoveSubDevices) {
			$.ajax({
				url: "json.htm?type=setused&idx=" + $.devIdx +
				'&name=' + encodeURIComponent($("#lightcontent #devicename").val()) +
				'&description=' + encodeURIComponent($("#lightcontent #devicedescription").val()) +
				'&used=false&RemoveSubDevices=' + bRemoveSubDevices,
				async: false,
				dataType: 'json',
				success: function (data) {
					ShowLights();
				}
			});
		}

		DeleteLightSwitch = function () {
			bootbox.confirm($.t("Are you sure to remove this Light/Switch?"), function (result) {
				if (result == true) {
					var bRemoveSubDevices = true;
					if ($.isslave == true) {
						var result = false;
						$("#dialog-confirm-delete-subs").dialog({
							resizable: false,
							width: 440,
							height: 180,
							modal: true,
							buttons: {
								"Yes": function () {
									$(this).dialog("close");
									DeleteLightSwitchIntern(true);
								},
								"No": function () {
									$(this).dialog("close");
									DeleteLightSwitchIntern(false);
								}
							}
						});
					}
					else {
						DeleteLightSwitchIntern(false);
					}
				}
			});
		}

		SaveLightSwitch = function () {
			var bValid = true;
			bValid = bValid && checkLength($("#lightcontent #devicename"), 2, 100);

			var strParam1 = $("#lightcontent #onaction").val();
			var strParam2 = $("#lightcontent #offaction").val();

			var bIsProtected = $('#lightcontent #protected').is(":checked");

			if (strParam1 != "") {
				if ((strParam1.indexOf("http://") != 0) && (strParam1.indexOf("https://") != 0) && (strParam1.indexOf("script://") != 0)) {
					bootbox.alert($.t("Invalid ON Action!"));
					return;
				}
				else {
					if (checkLength($("#lightcontent #onaction"), 10, 500) == false) {
						bootbox.alert($.t("Invalid ON Action!"));
						return;
					}
				}
			}
			if (strParam2 != "") {
				if ((strParam2.indexOf("http://") != 0) && (strParam2.indexOf("https://") != 0) && (strParam2.indexOf("script://") != 0)) {
					bootbox.alert($.t("Invalid Off Action!"));
					return;
				}
				else {
					if (checkLength($("#lightcontent #offaction"), 10, 500) == false) {
						bootbox.alert($.t("Invalid Off Action!"));
						return;
					}
				}
			}
			var devOptionsParam = [], devOptions = [];
			if ($.bIsSelectorSwitch) {
				var levelNames = unescape($("#lightcontent #selectorlevelstable").data('levelNames')),
					levelActions = $("#lightcontent #selectoractionstable").data('levelActions'),
					selectorStyle = $("#lightcontent .selector-switch-options.style input[type=radio]:checked").val(),
					levelOffHidden = $("#lightcontent .selector-switch-options.level-off-hidden input[type=checkbox]").prop('checked');
				devOptions.push("LevelNames:");
				devOptions.push(levelNames);
				devOptions.push(";");
				devOptions.push("LevelActions:");
				devOptions.push(levelActions);
				devOptions.push(";");
				devOptions.push("SelectorStyle:");
				devOptions.push(selectorStyle);
				devOptions.push(";");
				devOptions.push("LevelOffHidden:");
				devOptions.push(levelOffHidden);
				devOptions.push(";");
				devOptionsParam.push(devOptions.join(''));
			}

			if (bValid) {
				if ($.stype == "Security") {
					$.ajax({
						url: "json.htm?type=setused&idx=" + $.devIdx +
						'&name=' + encodeURIComponent($("#lightcontent #devicename").val()) +
						'&description=' + encodeURIComponent($("#lightcontent #devicedescription").val()) +
						'&strparam1=' + btoa(strParam1) +
						'&strparam2=' + btoa(strParam2) +
						'&protected=' + bIsProtected +
						'&used=true' +
						'&options=' + btoa(encodeURIComponent(devOptionsParam.join(''))), // encode before b64 to prevent from character encoding issue
						async: false,
						dataType: 'json',
						success: function (data) {
							ShowLights();
						}
					});
				}
				else {
					var addjvalstr = "";
					var switchtype = $("#lightcontent #comboswitchtype").val();
					if (switchtype == 8) {
						addjvalstr = "&addjvalue=" + $("#lightcontent #motionoffdelay").val();
					}
					else if ((switchtype == 0) || (switchtype == 7) || (switchtype == 9) || (switchtype == 11) || (switchtype == 18)) {
						addjvalstr = "&addjvalue=" + $("#lightcontent #offdelay").val();
						addjvalstr += "&addjvalue2=" + $("#lightcontent #ondelay").val();
					}
					var CustomImage = 0;

					if ((switchtype == 0) || (switchtype == 7) || (switchtype == 17) || (switchtype == 18)) {
						var cval = $('#lightcontent #comboswitchicon').data('ddslick').selectedIndex;
						CustomImage = $.ddData[cval].value;
					}
					$.ajax({
						url: "json.htm?type=setused&idx=" + $.devIdx +
						'&name=' + encodeURIComponent($("#lightcontent #devicename").val()) +
						'&description=' + encodeURIComponent($("#lightcontent #devicedescription").val()) +
						'&strparam1=' + btoa(strParam1) +
						'&strparam2=' + btoa(strParam2) +
						'&protected=' + bIsProtected +
						'&switchtype=' + $("#lightcontent #comboswitchtype").val() +
						'&customimage=' + CustomImage +
						'&used=true' + addjvalstr +
						'&options=' + btoa(encodeURIComponent(devOptionsParam.join(''))), // encode before b64 to prevent from character encoding issue
						async: false,
						dataType: 'json',
						success: function (data) {
							ShowLights();
						}
					});
				}
			}
		}

		ClearSubDevices = function () {
			bootbox.confirm($.t("Are you sure to delete ALL Sub/Slave Devices?\n\nThis action can not be undone!"), function (result) {
				if (result == true) {
					$.ajax({
						url: "json.htm?type=command&param=deleteallsubdevices&idx=" + $.devIdx,
						async: false,
						dataType: 'json',
						success: function (data) {
							RefreshSubDeviceTable($.devIdx);
						}
					});
				}
			});
		}

		DeleteSubDevice = function (idx) {
			bootbox.confirm($.t("Are you sure to delete this Sub/Slave Device?\n\nThis action can not be undone..."), function (result) {
				if (result == true) {
					$.ajax({
						url: "json.htm?type=command&param=deletesubdevice&idx=" + idx,
						async: false,
						dataType: 'json',
						success: function (data) {
							RefreshSubDeviceTable($.devIdx);
						}
					});
				}
			});
		}

		RefreshSubDeviceTable = function (idx) {
			$('#modal').show();

			$('#lightcontent #delclr #subdevicedelete').attr("class", "btnstyle3-dis");

			var oTable = $('#lightcontent #subdevicestable').dataTable();
			oTable.fnClearTable();

			$.ajax({
				url: "json.htm?type=command&param=getsubdevices&idx=" + idx,
				async: false,
				dataType: 'json',
				success: function (data) {
					if (typeof data.result != 'undefined') {
						$.each(data.result, function (i, item) {
							var addId = oTable.fnAddData({
								"DT_RowId": item.ID,
								"0": item.Name
							});
						});
					}
				}
			});

			/* Add a click handler to the rows - this could be used as a callback */
			$("#lightcontent #subdevicestable tbody").off();
			$("#lightcontent #subdevicestable tbody").on('click', 'tr', function () {
				if ($(this).hasClass('row_selected')) {
					$(this).removeClass('row_selected');
					$('#lightcontent #delclr #subdevicedelete').attr("class", "btnstyle3-dis");
				}
				else {
					var oTable = $('#lightcontent #subdevicestable').dataTable();
					oTable.$('tr.row_selected').removeClass('row_selected');
					$(this).addClass('row_selected');
					$('#lightcontent #delclr #subdevicedelete').attr("class", "btnstyle3");

					var anSelected = fnGetSelected(oTable);
					if (anSelected.length !== 0) {
						var data = oTable.fnGetData(anSelected[0]);
						var idx = data["DT_RowId"];
						$("#lightcontent #delclr #subdevicedelete").attr("href", "javascript:DeleteSubDevice(" + idx + ")");
					}
				}
			});

			$('#modal').hide();
		}

		AddSubDevice = function () {
			var SubDeviceIdx = $("#lightcontent #combosubdevice option:selected").val();
			if (typeof SubDeviceIdx == 'undefined') {
				bootbox.alert($.t('No Sub/Slave Device Selected!'));
				return;
			}
			$.ajax({
				url: "json.htm?type=command&param=addsubdevice&idx=" + $.devIdx + "&subidx=" + SubDeviceIdx,
				async: false,
				dataType: 'json',
				success: function (data) {
					if (data.status == 'OK') {
						RefreshSubDeviceTable($.devIdx);
					}
					else {
						ShowNotify($.t('Problem adding Sub/Slave Device!'), 2500, true);
					}
				},
				error: function () {
					HideNotify();
					ShowNotify($.t('Problem adding Sub/Slave Device!'), 2500, true);
				}
			});
		}

		SetColValue = function (idx, hue, brightness, isWhite) {
			clearInterval($.setColValue);
			if (permissions.hasPermission("Viewer")) {
				HideNotify();
				ShowNotify($.t('You do not have permission to do that!'), 2500, true);
				return;
			}
			$.ajax({
				url: "json.htm?type=command&param=setcolbrightnessvalue&idx=" + idx + "&hue=" + hue + "&brightness=" + brightness + "&iswhite=" + isWhite,
				async: false,
				dataType: 'json'
			});
		}

		appLampBrightnessUp = function () {
			$.ajax({
				url: "json.htm?type=command&param=brightnessup&idx=" + $.devIdx,
				async: false,
				dataType: 'json'
			});
		}

		appLampBrightnessDown = function () {
			$.ajax({
				url: "json.htm?type=command&param=brightnessdown&idx=" + $.devIdx,
				async: false,
				dataType: 'json'
			});
		}
		appLampDiscoUp = function () {
			$.ajax({
				url: "json.htm?type=command&param=discoup&idx=" + $.devIdx,
				async: false,
				dataType: 'json'
			});
		}

		appLampDiscoDown = function () {
			$.ajax({
				url: "json.htm?type=command&param=discodown&idx=" + $.devIdx,
				async: false,
				dataType: 'json'
			});
		}

		appLampDiscoMode = function () {
			$.ajax({
				url: "json.htm?type=command&param=discomode&idx=" + $.devIdx,
				async: false,
				dataType: 'json'
			});
		}

		appLampDiscoModeNum = function (mode) {
			$.ajax({
				url: "json.htm?type=command&param=discomodenum" + mode + "&idx=" + $.devIdx,
				async: false,
				dataType: 'json'
			});
		}

		appLampSetKelvin = function (kelvin) {
			$.ajax({
				url: "json.htm?type=command&param=setkelvinlevel&idx=" + $.devIdx + "&kelvin=" + kelvin,
				async: false,
				dataType: 'json'
			});
		}

		appLampSpeedUp = function () {
			$.ajax({
				url: "json.htm?type=command&param=speedup&idx=" + $.devIdx,
				async: false,
				dataType: 'json'
			});
		}
		appLampSpeedUpLong = function () {
			$.ajax({
				url: "json.htm?type=command&param=speeduplong&idx=" + $.devIdx,
				async: false,
				dataType: 'json'
			});
		}

		appLampSpeedDown = function () {
			$.ajax({
				url: "json.htm?type=command&param=speeddown&idx=" + $.devIdx,
				async: false,
				dataType: 'json'
			});
		}

		appLampSpeedMin = function () {
			$.ajax({
				url: "json.htm?type=command&param=speedmin&idx=" + $.devIdx,
				async: false,
				dataType: 'json'
			});
		}

		appLampSpeedMax = function () {
			$.ajax({
				url: "json.htm?type=command&param=speedmax&idx=" + $.devIdx,
				async: false,
				dataType: 'json'
			});
		}

		appLampWarmer = function () {
			$.ajax({
				url: "json.htm?type=command&param=warmer&idx=" + $.devIdx,
				async: false,
				dataType: 'json'
			});
		}

		appLampFull = function () {
			$.ajax({
				url: "json.htm?type=command&param=fulllight&idx=" + $.devIdx,
				async: false,
				dataType: 'json'
			});
		}

		appLampWhite = function () {
			$.ajax({
				url: "json.htm?type=command&param=whitelight&idx=" + $.devIdx,
				async: false,
				dataType: 'json'
			});
		}

		appLampNight = function () {
			$.ajax({
				url: "json.htm?type=command&param=nightlight&idx=" + $.devIdx,
				async: false,
				dataType: 'json'
			});
		}

		CleanDeviceOptionValue = function (value) {
			return value.replace(/[:;|<>]/g, "").trim();
		}
		ChangeSelectorLevelsOrder = function (from, to) {
			if (!permissions.hasPermission("Admin")) {
				HideNotify();
				ShowNotify($.t('You do not have permission to do that!'), 2500, true);
				return;
			}
			var table$ = $("#lightcontent #selectorlevelstable"),
				levelNames = unescape(table$.data('levelNames')).split('|'),
				fromLevel = levelNames[from],
				toLevel = levelNames[to];
			levelNames[to] = fromLevel;
			levelNames[from] = toLevel;
			table$.data('levelNames', escape(levelNames.join('|')));
			BuildSelectorLevelsTable();
		};
		DeleteSelectorLevel = function (index) {
			if (!permissions.hasPermission("Admin")) {
				HideNotify();
				ShowNotify($.t('You do not have permission to do that!'), 2500, true);
				return;
			}
			var table$ = $("#lightcontent #selectorlevelstable"),
				levelNames = unescape(table$.data('levelNames')).split('|');
			levelNames.splice(index, 1);
			table$.data('levelNames', escape(levelNames.join('|')));
			BuildSelectorLevelsTable();
			DeleteSelectorAction(index);
		};
		UpdateSelectorLevel = function (index, levelName) {
			var table$ = $("#lightcontent #selectorlevelstable"),
				levelNames = unescape(table$.data('levelNames')).split('|');
			levelNames[index] = levelName;
			table$.data('levelNames', escape(levelNames.join('|')));
			BuildSelectorLevelsTable();
		};
		RenameSelectorLevel = function (index) {
			if (!permissions.hasPermission("Admin")) {
				HideNotify();
				ShowNotify($.t('You do not have permission to do that!'), 2500, true);
				return;
			}
			if (index >= 0) {
				var table$ = $("#lightcontent #selectorlevelstable"),
					levelNames = unescape(table$.data('levelNames')).split('|'),
					levelName = levelNames[index];
				$("#dialog-renameselectorlevel #selectorlevelindex").val(index);
				$("#dialog-renameselectorlevel #selectorlevelname").val(levelName);
				$("#dialog-renameselectorlevel").i18n();
				$("#dialog-renameselectorlevel").dialog("open");
			}
		};
		AddSelectorLevel = function () {
			var button$ = $("#newselectorlevelbutton"),
				table$ = $("#lightcontent #selectorlevelstable"),
				levelName = $("#lightcontent #newselectorlevel").val(),
				levelNames = unescape(table$.data('levelNames')).split('|');
			// clean unauthorized characters
			levelName = CleanDeviceOptionValue(levelName);
			if ((button$.prop("disabled") === true) ||			// limit length
				(levelName === '') ||						// avoid empty name
				($.inArray(levelName, levelNames) !== -1)) {	// avoid duplicate
				return;
			}
			levelNames.push(levelName);
			table$.data('levelNames', escape(levelNames.join('|')));
			BuildSelectorLevelsTable();
			AddSelectorAction();
		};
		BuildSelectorLevelsTable = function () {
			var table$ = $("#lightcontent #selectorlevelstable"),
				levelNames = unescape(table$.data('levelNames')).split('|'),
				levelNamesMaxLength = 11,
				initializeTable = $('#selectorlevelstable_wrapper').length === 0,
				oTable = (initializeTable) ? table$.dataTable({
					"iDisplayLength": 25,
					"bLengthChange": false,
					"bFilter": false,
					"bInfo": false,
					"bPaginate": false
				}) : table$.dataTable();
			oTable.fnClearTable();
			$.each(levelNames, function (index, item) {
				var level = index * 10,
					updownImg = "",
					rendelImg = "";
				if ((0 < index) && (index < (levelNames.length - 1))) {
					// Add Down image
					if (updownImg !== "") {
						updownImg += "&nbsp;";
					}
					updownImg += '<img src="images/down.png" onclick="ChangeSelectorLevelsOrder(' + index + ',' + (index + 1) + ');" class="lcursor" width="16" height="16"></img>';
				} else {
					updownImg += '<img src="images/empty16.png" width="16" height="16"></img>';
				}
				if (index > 1) {
					// Add Up image
					if (updownImg !== "") {
						updownImg += "&nbsp;";
					}
					updownImg += '<img src="images/up.png" onclick="ChangeSelectorLevelsOrder(' + index + ',' + (index - 1) + ');" class="lcursor" width="16" height="16"></img>';
				}
				if (index > 0) {
					// Add Rename image
					rendelImg = '<img src="images/rename.png" title="' + $.t("Rename") + '" onclick="RenameSelectorLevel(' + index + ');" class="lcursor" width="16" height="16"></img>';
					// Add Delete image
					rendelImg += '&nbsp;';
					rendelImg += '<img src="images/delete.png" title="' + $.t("Delete") + '" onclick="DeleteSelectorLevel(' + index + ');" class="lcursor" width="16" height="16"></img>';
				} else {
					rendelImg = '<img src="images/empty16.png" width="16" height="16"></img>';
				}
				oTable.fnAddData({
					"DT_RowId": index,
					"Name": levelNames[index],
					"Order": index,
					"Delete": index,
					"0": level,
					"1": levelNames[index],
					"2": updownImg,
					"3": rendelImg
				});
			});
			// Limit level length to levelNamesMaxLength
			$("#lightcontent #newselectorlevelbutton").prop("disabled", false).removeClass("ui-state-disabled");
			if (levelNames.length === levelNamesMaxLength) {
				$("#lightcontent #newselectorlevelbutton").prop("disabled", true).addClass("ui-state-disabled");
			}
		};

		UpdateSelectorAction = function (index, levelAction) {
			var table$ = $("#lightcontent #selectoractionstable"),
				levelActions = table$.data('levelActions').split('|');
			levelActions[index] = escape(levelAction);
			table$.data('levelActions', levelActions.join('|'));
			BuildSelectorActionsTable();
		};
		EditSelectorAction = function (index) {
			if (!permissions.hasPermission("Admin")) {
				HideNotify();
				ShowNotify($.t('You do not have permission to do that!'), 2500, true);
				return;
			}
			if (index >= 0) {
				var table$ = $("#lightcontent #selectoractionstable"),
					levelActions = table$.data('levelActions').split('|'),
					levelAction = levelActions[index];
				$("#dialog-editselectoraction #selectorlevelindex").val(index);
				$("#dialog-editselectoraction #selectoraction").val(unescape(levelAction));
				$("#dialog-editselectoraction").i18n();
				$("#dialog-editselectoraction").dialog("open");
			}
		};
		ClearSelectorAction = function (index) {
			if (!permissions.hasPermission("Admin")) {
				HideNotify();
				ShowNotify($.t('You do not have permission to do that!'), 2500, true);
				return;
			}
			var table$ = $("#lightcontent #selectoractionstable"),
				levelActions = table$.data('levelActions').split('|');
			levelActions[index] = '';
			table$.data('levelActions', levelActions.join('|'));
			BuildSelectorActionsTable();
		};
		DeleteSelectorAction = function (index) {
			if (!permissions.hasPermission("Admin")) {
				HideNotify();
				ShowNotify($.t('You do not have permission to do that!'), 2500, true);
				return;
			}
			var table$ = $("#lightcontent #selectoractionstable"),
				levelActions = table$.data('levelActions').split('|');
			levelActions.splice(index, 1);
			table$.data('levelActions', levelActions.join('|'));
			BuildSelectorActionsTable();
		};
		AddSelectorAction = function () {
			var table$ = $("#lightcontent #selectoractionstable"),
				levelActions = table$.data('levelActions').split('|');
			levelActions.push('');
			table$.data('levelActions', levelActions.join('|'));
			BuildSelectorActionsTable();
		};
		BuildSelectorActionsTable = function () {
			var table$ = $("#lightcontent #selectoractionstable"),
				levelActions = table$.data('levelActions').split('|'),
				levelActionsMaxLength = 11,
				initializeTable = $('#selectoractionstable_wrapper').length === 0,
				oTable = (initializeTable) ? table$.dataTable({
					"iDisplayLength": 25,
					"bLengthChange": false,
					"bFilter": false,
					"bInfo": false,
					"bPaginate": false
				}) : table$.dataTable();
			oTable.fnClearTable();
			$.each(levelActions, function (index, item) {
				var level = index * 10,
					levelAction = unescape(levelActions[index]),
					rendelImg = "";
				// Add Rename image
				rendelImg = '<img src="images/rename.png" title="' + $.t("Edit") + '" onclick="EditSelectorAction(' + index + ');" class="lcursor" width="16" height="16"></img>';
				rendelImg += '&nbsp;';
				rendelImg += '<img src="images/delete.png" title="' + $.t("Clear") + '" onclick="ClearSelectorAction(' + index + ');" class="lcursor" width="16" height="16"></img>';
				oTable.fnAddData({
					"DT_RowId": index,
					"Name": levelAction,
					"Edit": index,
					"0": level,
					"1": levelAction.replace('&', '&amp;'),
					"2": rendelImg
				});
			});
		};

		appLampCooler = function () {
			$.ajax({
				url: "json.htm?type=command&param=cooler&idx=" + $.devIdx,
				async: false,
				dataType: 'json'
			});
		}

		ConfigureEditLightSettings = function () {
			if ($scope.HasInitializedEditLightDialog == true) {
				return;
			}
			$scope.HasInitializedEditLightDialog = true;
			//Get Custom icons
			$.ddData = [];
			$.ajax({
				url: "json.htm?type=custom_light_icons",
				async: false,
				dataType: 'json',
				success: function (data) {
					if (typeof data.result != 'undefined') {
						var totalItems = data.result.length;
						$.each(data.result, function (i, item) {
							var bSelected = false;
							if (i == 0) {
								bSelected = true;
							}
							var img = "images/" + item.imageSrc + "48_On.png";
							$.ddData.push({ text: item.text, value: item.idx, selected: bSelected, description: item.description, imageSrc: img });
						});
					}
				}
			});

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
								name: item.Name
							});
						});
					}
				}
			});

		}

		EditLightDevice = function (idx, name, description, stype, switchtype, addjvalue, addjvalue2, isslave, customimage, devsubtype, strParam1, strParam2, bIsProtected, strUnit) {
			if (typeof $scope.mytimer != 'undefined') {
				$interval.cancel($scope.mytimer);
				$scope.mytimer = undefined;
			}
			$.devIdx = idx;
			$.isslave = isslave;
			$.stype = stype;
			$.strUnit = strUnit;
			$.bIsSelectorSwitch = (devsubtype === "Selector Switch");

			ConfigureEditLightSettings();

			var oTable;

			if ($.bIsSelectorSwitch) {
				// backup selector switch level names before displaying edit edit form
				var selectorSwitch$ = $("#selector" + $.devIdx),
					ssLevelNames = unescape(selectorSwitch$.data("levelnames")),
					ssStyle = selectorSwitch$.data("selectorstyle"),
					ssLevelOffHidden = selectorSwitch$.data("leveloffhidden"),
					ssLevelActions = selectorSwitch$.data("levelactions");
				$.selectorSwitchStyle = ssStyle;
				$.selectorSwitchLevelOffHidden = ssLevelOffHidden;
				$.selectorSwitchLevels = ssLevelNames.split('|');
				$.selectorSwitchActions = ssLevelActions.split('|');
				$.each($.selectorSwitchLevels, function (index, item) {
					if (index > ($.selectorSwitchActions.length - 1)) {
						$.selectorSwitchActions.push(""); // force missing action
					}
				});
				if ($.selectorSwitchActions.length > $.selectorSwitchLevels.length) { // truncate if necessary
					$.selectorSwitchActions.splice($.selectorSwitchLevels.length, $.selectorSwitchActions.length - $.selectorSwitchLevels.length);
				}
			}

			$('#modal').show();
			var htmlcontent = GetBackbuttonHTMLTable('ShowLights');
			htmlcontent += $('#editlightswitch').html();
			$('#lightcontent').html(htmlcontent);
			//$('#lightcontent').html($compile(htmlcontent)($scope));
			$('#lightcontent').i18n();

			oTable = $('#lightcontent #subdevicestable').dataTable({
				"sDom": '<"H"frC>t<"F"i>',
				"oTableTools": {
					"sRowSelect": "single",
				},
				"aaSorting": [[0, "desc"]],
				"bSortClasses": false,
				"bProcessing": true,
				"bStateSave": true,
				"bJQueryUI": true,
				"sPaginationType": "full_numbers",
				language: $.DataTableLanguage
			});

			var sat = 100;
			var cHSB = [];
			cHSB.h = 128;
			cHSB.s = sat;
			cHSB.b = 100;
			$('#lightcontent #Brightness').val(100);
			$('#lightcontent #Hue').val(128);

			//For later use with slider: chris
			//			var kelvin=100;
			//			$('#lightcontent #Temperature').val(75);
			//chris

			$.bIsLED = (devsubtype.indexOf("RGB") >= 0);
			$.bIsRGB = (devsubtype == "RGB");
			$.bIsRGBW = (devsubtype.indexOf("RGBW") >= 0);
			$.bIsRGBWW = (devsubtype.indexOf("RGBWW") >= 0);
			$.bIsWhite = (devsubtype == "White");
			//Only Limitless type bulbs
			$.bIsLimitless = (stype.indexOf("Limitless") >= 0);
			//


			if ($.bIsLED == true) {
				$("#lightcontent #LedColor").show();
			}
			else {
				$("#lightcontent #LedColor").hide();
			}
			if ($.bIsRGB == true && $.strUnit == "0" && $.bIsLimitless == true) {
				$("#lightcontent #optionsRGBLimit").show();
			}
			else {
				$("#lightcontent #optionsRGBLimit").hide();
			}
			if ($.bIsRGBWW == true && $.bIsLimitless == true) {
				$("#lightcontent #optionsRGBWWLimit").show();
			}
			else {
				$("#lightcontent #optionsRGBWWLimit").hide();
			}
			if ($.bIsRGBW == true && $.bIsRGBWW == false && $.bIsLimitless == true) {
				$("#lightcontent #optionsRGBWLimit").show();
			}
			else {
				$("#lightcontent #optionsRGBWLimit").hide();
			}
			if ($.bIsRGBW == true) {
				$("#lightcontent #optionsRGBW").show();
			}
			else {
				$("#lightcontent #optionsRGBW").hide();
			}
			if ($.bIsWhite) {
				$("#lightcontent #optionsWhite").show();
			}
			else {
				$("#lightcontent #optionsWhite").hide();
			}
			$('#lightcontent #picker').colpick({
				flat: true,
				layout: 'hex',
				submit: 0,
				onChange: function (hsb, hex, rgb, fromSetColor) {
					if (!fromSetColor) {
						$('#lightcontent #Hue').val(hsb.h);
						$('#lightcontent #Brightness').val(hsb.b);
						var bIsWhite = (hsb.s < 20);
						$("#lightcontent #optionRGB").prop('checked', !bIsWhite);
						$("#lightcontent #optionWhite").prop('checked', bIsWhite);
						clearInterval($.setColValue);
						$.setColValue = setInterval(function () { SetColValue($.devIdx, hsb.h, hsb.b, bIsWhite); }, 400);
					}
				}
			});
			$("#lightcontent #optionRGB").prop('checked', (sat == 100));
			$("#lightcontent #optionWhite").prop('checked', !(sat == 100));

			$('#lightcontent #picker').colpickSetColor(cHSB);

			$("#lightcontent #devicename").val(unescape(name));
			$("#lightcontent #devicedescription").val(unescape(description));

			$("#lightcontent .selector-switch-options").hide();

			if ($.stype == "Security") {
				$("#lightcontent #SwitchType").hide();
				$("#lightcontent #OnDelayDiv").hide();
				$("#lightcontent #OffDelayDiv").hide();
				$("#lightcontent #MotionDiv").hide();
				$("#lightcontent #SwitchIconDiv").hide();
				$("#lightcontent #onaction").val(atob(strParam1));
				$("#lightcontent #offaction").val(atob(strParam2));
				$('#lightcontent #protected').prop('checked', (bIsProtected == true));
			}
			else {
				$("#lightcontent #SwitchType").show();
				$("#lightcontent #comboswitchtype").change(function () {
					var switchtype = $("#lightcontent #comboswitchtype").val();
					$("#lightcontent #OnDelayDiv").hide();
					$("#lightcontent #OffDelayDiv").hide();
					$("#lightcontent #MotionDiv").hide();
					$("#lightcontent #SwitchIconDiv").hide();
					$("#lightcontent .selector-switch-options").hide();
					if (switchtype == 8) {
						$("#lightcontent #MotionDiv").show();
						$("#lightcontent #motionoffdelay").val(addjvalue);
					}
					else if ((switchtype == 0) || (switchtype == 7) || (switchtype == 9) || (switchtype == 11) || (switchtype == 18)) {
						$("#lightcontent #OnDelayDiv").show();
						$("#lightcontent #OffDelayDiv").show();
						$("#lightcontent #offdelay").val(addjvalue);
						$("#lightcontent #ondelay").val(addjvalue2);
					}

					if ((switchtype == 0) || (switchtype == 7) || (switchtype == 17) || (switchtype == 18)) {
						$("#lightcontent #SwitchIconDiv").show();
					}
					if (switchtype == 18) {
						$("#lightcontent .selector-switch-options").show();
					}
				});

				$("#lightcontent #comboswitchtype").val(switchtype);

				$("#lightcontent #OnDelayDiv").hide();
				$("#lightcontent #OffDelayDiv").hide();
				$("#lightcontent #MotionDiv").hide();
				$("#lightcontent #SwitchIconDiv").hide();
				if (switchtype == 8) {
					$("#lightcontent #MotionDiv").show();
					$("#lightcontent #motionoffdelay").val(addjvalue);
				}
				else if ((switchtype == 0) || (switchtype == 7) || (switchtype == 9) || (switchtype == 11) || (switchtype == 18)) {
					$("#lightcontent #OnDelayDiv").show();
					$("#lightcontent #OffDelayDiv").show();
					$("#lightcontent #offdelay").val(addjvalue);
					$("#lightcontent #ondelay").val(addjvalue2);
				}
				if ((switchtype == 0) || (switchtype == 7) || (switchtype == 17) || (switchtype == 18)) {
					$("#lightcontent #SwitchIconDiv").show();
				}
				if (switchtype == 18) {
					var dialog_renameselectorlevel_buttons = {}, dialog_editselectoraction_buttons = {};
					dialog_renameselectorlevel_buttons[$.t("Rename")] = function () {
						var selectorLevelName$ = $("#dialog-renameselectorlevel #selectorlevelname"),
							selectorIndex$ = $("#dialog-renameselectorlevel #selectorlevelindex"),
							levelIndex = selectorIndex$.val(),
							levelName = selectorLevelName$.val(),
							table$ = $("#lightcontent #selectorlevelstable"),
							levelNames = unescape(table$.data('levelNames')).split('|'),
							bValid = true;
						// clean unauthorized characters
						levelName = CleanDeviceOptionValue(levelName);
						bValid = bValid && (levelIndex >= 0);
						bValid = bValid && checkLength(selectorLevelName$, 2, 20);
						bValid = bValid && (levelName !== '') &&			// avoid empty
							($.inArray(levelName, levelNames) === -1);	// avoid duplicate
						if (bValid) {
							$(this).dialog("close");
							UpdateSelectorLevel(levelIndex, levelName);
						}
					};
					dialog_renameselectorlevel_buttons[$.t("Cancel")] = function () {
						$(this).dialog("close");
					};
					$("#dialog-renameselectorlevel").dialog({
						autoOpen: false,
						width: 'auto',
						height: 'auto',
						modal: true,
						resizable: false,
						title: $.t("Rename level name"),
						buttons: dialog_renameselectorlevel_buttons
					});
					$("#lightcontent #selectorlevelstable").data('levelNames', escape($.selectorSwitchLevels.join('|')));
					BuildSelectorLevelsTable();

					dialog_editselectoraction_buttons[$.t("Save")] = function () {
						var selectorAction$ = $("#dialog-editselectoraction #selectoraction"),
							selectorIndex$ = $("#dialog-editselectoraction #selectorlevelindex"),
							levelIndex = selectorIndex$.val(),
							levelAction = selectorAction$.val().trim(),
							bValid = true;
						bValid = bValid && (levelIndex >= 0);
						bValid = bValid && checkLength(selectorAction$, 0, 200);
						bValid = bValid && ((levelAction === '') ||
							(((levelAction.toLowerCase().indexOf('http://') === 0) && (levelAction.length > 7)) ||
								((levelAction.toLowerCase().indexOf('https://') === 0) && (levelAction.length > 8)) ||
								((levelAction.toLowerCase().indexOf('script://') === 0) && (levelAction.length > 9))));
						if (bValid) {
							$(this).dialog("close");
							UpdateSelectorAction(levelIndex, levelAction);
						}
					};
					dialog_editselectoraction_buttons[$.t("Cancel")] = function () {
						$(this).dialog("close");
					};
					$("#dialog-editselectoraction").dialog({
						autoOpen: false,
						width: 'auto',
						height: 'auto',
						modal: true,
						resizable: false,
						title: $.t("Edit level action"),
						buttons: dialog_editselectoraction_buttons
					});
					$("#lightcontent #selectoractionstable").data('levelActions', $.selectorSwitchActions.join('|'));
					BuildSelectorActionsTable();

					$("#lightcontent #OnActionDiv").hide();
					$("#lightcontent #OffActionDiv").hide();
					$("#lightcontent .selector-switch-options.style input[value=" + $.selectorSwitchStyle + "]").attr('checked', true);
					$("#lightcontent .selector-switch-options.level-off-hidden input[type=checkbox]").prop('checked', $.selectorSwitchLevelOffHidden);
					$("#lightcontent .selector-switch-options").show();
				}
				$("#lightcontent #combosubdevice").html("");

				$("#lightcontent #onaction").val(atob(strParam1));
				$("#lightcontent #offaction").val(atob(strParam2));

				$('#lightcontent #protected').prop('checked', (bIsProtected == true));

				$.each($.LightsAndSwitches, function (i, item) {
					var option = $('<option />');
					option.attr('value', item.idx).text(item.name);
					$("#lightcontent #combosubdevice").append(option);
				});

				$('#lightcontent #comboswitchicon').ddslick({
					data: $.ddData,
					width: 260,
					height: 390,
					selectText: "Select Switch Icon",
					imagePosition: "left"
				});
				//find our custom image index and select it
				$.each($.ddData, function (i, item) {
					if (item.value == customimage) {
						$('#lightcontent #comboswitchicon').ddslick('select', { index: i });
					}
				});
			}

			RefreshSubDeviceTable(idx);
		}

		AddManualLightDevice = function () {
			if (typeof $scope.mytimer != 'undefined') {
				$interval.cancel($scope.mytimer);
				$scope.mytimer = undefined;
			}

			$("#dialog-addmanuallightdevice #combosubdevice").html("");

			$.each($.LightsAndSwitches, function (i, item) {
				var option = $('<option />');
				option.attr('value', item.idx).text(item.name);
				$("#dialog-addmanuallightdevice #combosubdevice").append(option);
			});
			$("#dialog-addmanuallightdevice #comboswitchtype").val(0);
			$("#dialog-addmanuallightdevice").i18n();
			$("#dialog-addmanuallightdevice").dialog("open");
		}

		AddLightDevice = function () {
			if (typeof $scope.mytimer != 'undefined') {
				$interval.cancel($scope.mytimer);
				$scope.mytimer = undefined;
			}

			$("#dialog-addlightdevice #combosubdevice").html("");
			$.each($.LightsAndSwitches, function (i, item) {
				var option = $('<option />');
				option.attr('value', item.idx).text(item.name);
				$("#dialog-addlightdevice #combosubdevice").append(option);
			});

			ShowNotify($.t('Press button on Remote...'));

			setTimeout(function () {
				var bHaveFoundDevice = false;
				var deviceID = "";
				var deviceidx = 0;
				var bIsUsed = false;
				var Name = "";

				$.ajax({
					url: "json.htm?type=command&param=learnsw",
					async: false,
					dataType: 'json',
					success: function (data) {
						if (typeof data.status != 'undefined') {
							if (data.status == 'OK') {
								bHaveFoundDevice = true;
								deviceID = data.ID;
								deviceidx = data.idx;
								bIsUsed = data.Used;
								Name = data.Name;
							}
						}
					}
				});
				HideNotify();

				setTimeout(function () {
					if ((bHaveFoundDevice == true) && (bIsUsed == false)) {
						$.devIdx = deviceidx;
						$("#dialog-addlightdevice").i18n();
						$("#dialog-addlightdevice").dialog("open");
					}
					else {
						if (bIsUsed == true)
							ShowNotify($.t('Already used by') + ':<br>"' + Name + '"', 3500, true);
						else
							ShowNotify($.t('Timeout...<br>Please try again!'), 2500, true);
					}
				}, 200);
			}, 600);
		}

		ConfigureAddManualSettings = function () {
			if ($scope.HasInitializedAddManualDialog == true) {
				return;
			}
			$scope.HasInitializedAddManualDialog = true;
			$('#dialog-addmanuallightdevice #lightparams2 #combocmd2 >option').remove();
			$('#dialog-addmanuallightdevice #lightparams2 #combocmd3 >option').remove();
			$('#dialog-addmanuallightdevice #lightparams2 #combocmd4 >option').remove();
			$('#dialog-addmanuallightdevice #lightparams3 #combocmd1 >option').remove();
			$('#dialog-addmanuallightdevice #lightparams3 #combocmd2 >option').remove();
			$('#dialog-addmanuallightdevice #blindsparams #combocmd1 >option').remove();
			$('#dialog-addmanuallightdevice #blindsparams #combocmd2 >option').remove();
			$('#dialog-addmanuallightdevice #blindsparams #combocmd3 >option').remove();
			$('#dialog-addmanuallightdevice #homeconfortparams #combocmd2 >option').remove();
			$('#dialog-addmanuallightdevice #homeconfortparams #combocmd3 >option').remove();
			$('#dialog-addmanuallightdevice #fanparams #combocmd1 >option').remove();
			$('#dialog-addmanuallightdevice #fanparams #combocmd2 >option').remove();
			$('#dialog-addmanuallightdevice #fanparams #combocmd3 >option').remove();
			for (ii = 0; ii < 256; ii++) {
				$('#dialog-addmanuallightdevice #lightparams2 #combocmd2').append($('<option></option>').val(ii).html($.strPad(ii.toString(16).toUpperCase(), 2)));
				$('#dialog-addmanuallightdevice #lightparams2 #combocmd3').append($('<option></option>').val(ii).html($.strPad(ii.toString(16).toUpperCase(), 2)));
				$('#dialog-addmanuallightdevice #lightparams2 #combocmd4').append($('<option></option>').val(ii).html($.strPad(ii.toString(16).toUpperCase(), 2)));
				$('#dialog-addmanuallightdevice #lightparams3 #combocmd1').append($('<option></option>').val(ii).html($.strPad(ii.toString(16).toUpperCase(), 2)));
				$('#dialog-addmanuallightdevice #lightparams3 #combocmd2').append($('<option></option>').val(ii).html($.strPad(ii.toString(16).toUpperCase(), 2)));
				$('#dialog-addmanuallightdevice #blindsparams #combocmd1').append($('<option></option>').val(ii).html($.strPad(ii.toString(16).toUpperCase(), 2)));
				$('#dialog-addmanuallightdevice #blindsparams #combocmd2').append($('<option></option>').val(ii).html($.strPad(ii.toString(16).toUpperCase(), 2)));
				$('#dialog-addmanuallightdevice #blindsparams #combocmd3').append($('<option></option>').val(ii).html($.strPad(ii.toString(16).toUpperCase(), 2)));
				$('#dialog-addmanuallightdevice #homeconfortparams #combocmd2').append($('<option></option>').val(ii).html($.strPad(ii.toString(16).toUpperCase(), 2)));
				$('#dialog-addmanuallightdevice #homeconfortparams #combocmd3').append($('<option></option>').val(ii).html($.strPad(ii.toString(16).toUpperCase(), 2)));
				$('#dialog-addmanuallightdevice #fanparams #combocmd1').append($('<option></option>').val(ii).html($.strPad(ii.toString(16).toUpperCase(), 2)));
				$('#dialog-addmanuallightdevice #fanparams #combocmd2').append($('<option></option>').val(ii).html($.strPad(ii.toString(16).toUpperCase(), 2)));
				$('#dialog-addmanuallightdevice #fanparams #combocmd3').append($('<option></option>').val(ii).html($.strPad(ii.toString(16).toUpperCase(), 2)));
			}
			$('#dialog-addmanuallightdevice #blindsparams #combocmd4 >option').remove();
			$('#dialog-addmanuallightdevice #blindsparams #combounitcode >option').remove();
			for (ii = 0; ii < 16; ii++) {
				$('#dialog-addmanuallightdevice #blindsparams #combocmd4').append($('<option></option>').val(ii).html(ii.toString(16).toUpperCase()));
				$('#dialog-addmanuallightdevice #blindsparams #combounitcode').append($('<option></option>').val(ii).html(ii));
			}
			$('#dialog-addmanuallightdevice #lightparams2 #combounitcode >option').remove();
			for (ii = 1; ii < 16 + 1; ii++) {
				$('#dialog-addmanuallightdevice #lightparams2 #combounitcode').append($('<option></option>').val(ii).html(ii));
			}
			$('#dialog-addmanuallightdevice #he105params #combounitcode >option').remove();
			for (ii = 0; ii < 32; ii++) {
				$('#dialog-addmanuallightdevice #he105params #combounitcode').append($('<option></option>').val(ii).html($.strPad(ii.toString(16).toUpperCase(), 2)));
			}
			//Home confort
			$('#dialog-addmanuallightdevice #homeconfortparams #combocmd1 >option').remove();
			for (ii = 0; ii < 8; ii++) {
				$('#dialog-addmanuallightdevice #homeconfortparams #combocmd1').append($('<option></option>').val(ii).html($.strPad(ii.toString(16).toUpperCase(), 2)));
			}
			$('#dialog-addmanuallightdevice #homeconfortparams #combohousecode >option').remove();
			$('#dialog-addmanuallightdevice #homeconfortparams #combounitcode >option').remove();
			for (ii = 0; ii < 4; ii++) {
				$('#dialog-addmanuallightdevice #homeconfortparams #combohousecode').append($('<option></option>').val(65 + ii).html(String.fromCharCode(65 + ii)));
				$('#dialog-addmanuallightdevice #homeconfortparams #combounitcode').append($('<option></option>').val((ii + 1)).html((ii + 1)));
			}

			RefreshHardwareComboArray();
			$("#dialog-addmanuallightdevice #lighttable #combohardware").html("");
			$.each($.ComboHardware, function (i, item) {
				var option = $('<option />');
				option.attr('value', item.idx).text(item.name);
				$("#dialog-addmanuallightdevice #lighttable #combohardware").append(option);
			});

			RefreshGpioComboArray();
			$("#combogpio").html("");
			$.each($.ComboGpio, function (i, item) {
				var option = $('<option />');
				option.attr('value', item.idx).text(item.name);
				$("#combogpio").append(option);
			});

			RefreshSysfsGpioComboArray();
			$("#combosysfsgpio").html("");
			$.each($.ComboSysfsGpio, function (i, item) {
				var option = $('<option />');
				option.attr('value', item.idx).text(item.name);
				$("#combosysfsgpio").append(option);
            });
		}

		RefreshHardwareComboArray = function () {
			$.ComboHardware = [];
			$.ajax({
				url: "json.htm?type=command&param=getmanualhardware",
				async: false,
				dataType: 'json',
				success: function (data) {
					if (typeof data.result != 'undefined') {
						$.each(data.result, function (i, item) {
							$.ComboHardware.push({
								idx: item.idx,
								name: item.Name
							});
						});
					}
				}
			});
		}

		RefreshGpioComboArray = function () {
			$.ComboGpio = [];
			$.ajax({
				url: "json.htm?type=command&param=getgpio",
				async: false,
				dataType: 'json',
				success: function (data) {
					if (typeof data.result != 'undefined') {
						$.each(data.result, function (i, item) {
							$.ComboGpio.push({
								idx: item.idx,
								name: item.Name
							});
						});
					}
				}
			});
		}

		RefreshSysfsGpioComboArray = function () {
			$.ComboSysfsGpio = [];
			$.ajax({
				url: "json.htm?type=command&param=getsysfsgpio",
				async: false,
				dataType: 'json',
				success: function (data) {
					if (typeof data.result != 'undefined') {
						$.each(data.result, function (i, item) {
							$.ComboSysfsGpio.push({
								idx: item.idx,
								name: item.Name
							});
						});
					}
				}
			});
		}

		//Evohome...

		SwitchModal = function (idx, name, status, refreshfunction) {
			clearInterval($.myglobals.refreshTimer);

			ShowNotify($.t('Setting Evohome ') + ' ' + $.t(name));

			//FIXME avoid conflicts when setting a new status while reading the status from the web gateway at the same time
			//(the status can flick back to the previous status after an update)...now implemented with script side lockout
			$.ajax({
				url: "json.htm?type=command&param=switchmodal" +
				"&idx=" + idx +
				"&status=" + status +
				"&action=1",
				async: false,
				dataType: 'json',
				success: function (data) {
					if (data.status == "ERROR") {
						HideNotify();
						bootbox.alert($.t('Problem sending switch command'));
					}
					//wait 1 second
					setTimeout(function () {
						HideNotify();
						refreshfunction();
					}, 1000);
				},
				error: function () {
					HideNotify();
					alert($.t('Problem sending switch command'));
				}
			});
		}

		//FIXME move this to a shared js ...see temperaturecontroller.js
		EvoDisplayTextMode = function (strstatus) {
			if (strstatus == "Auto")//FIXME better way to convert?
				strstatus = "Normal";
			else if (strstatus == "AutoWithEco")//FIXME better way to convert?
				strstatus = "Economy";
			else if (strstatus == "DayOff")//FIXME better way to convert?
				strstatus = "Day Off";
			else if (strstatus == "HeatingOff")//FIXME better way to convert?
				strstatus = "Heating Off";
			return strstatus;
		}

		GetLightStatusText = function (item) {
			if (item.SubType == "Evohome")
				return EvoDisplayTextMode(item.Status);
			else if (item.SwitchType === "Selector")
				return item.LevelNames.split('|')[(item.LevelInt / 10)];
			else
				return item.Status;
		}

		EvohomeAddJS = function () {
			return "<script type='text/javascript'> function deselect(e,id) { $(id).slideFadeToggle('swing', function() { e.removeClass('selected'); });} $.fn.slideFadeToggle = function(easing, callback) {  return this.animate({ opacity: 'toggle',height: 'toggle' }, 'fast', easing, callback);};</script>";
		}

		EvohomeImg = function (item) {
			return '<div title="Quick Actions" class="' + ((item.Status == "Auto") ? "evoimgnorm" : "evoimg") + '"><img src="images/evohome/' + item.Status + '.png" class="lcursor" onclick="if($(this).hasClass(\'selected\')){deselect($(this),\'#evopop_' + item.idx + '\');}else{$(this).addClass(\'selected\');$(\'#evopop_' + item.idx + '\').slideFadeToggle();}return false;"></div>';
		}

		EvohomePopupMenu = function (item) {
			var htm = '\t      <td id="img"><a href="#evohome" id="evohome_' + item.idx + '">' + EvohomeImg(item) + '</a></td>\n<div id="evopop_' + item.idx + '" class="ui-popup ui-body-b ui-overlay-shadow ui-corner-all pop">  <ul class="ui-listview ui-listview-inset ui-corner-all ui-shadow">         <li class="ui-li-divider ui-bar-inherit ui-first-child">Choose an action</li>';
			$.each([{ "name": "Normal", "data": "Auto" }, { "name": "Economy", "data": "AutoWithEco" }, { "name": "Away", "data": "Away" }, { "name": "Day Off", "data": "DayOff" }, { "name": "Custom", "data": "Custom" }, { "name": "Heating Off", "data": "HeatingOff" }], function (idx, obj) { htm += '<li><a href="#" class="ui-btn ui-btn-icon-right ui-icon-' + obj.data + '" onclick="SwitchModal(\'' + item.idx + '\',\'' + obj.name + '\',\'' + obj.data + '\',RefreshLights);deselect($(this),\'#evopop_' + item.idx + '\');return false;">' + obj.name + '</a></li>'; });
			htm += '</ul></div>';
			return htm;
		}

		RefreshLights = function () {
			if (typeof $scope.mytimer != 'undefined') {
				$interval.cancel($scope.mytimer);
				$scope.mytimer = undefined;
			}
			var id = "";

			$.ajax({
				url: "json.htm?type=devices&filter=light&used=true&order=[Order]&lastupdate=" + $.LastUpdateTime + "&plan=" + window.myglobals.LastPlanSelected,
				async: false,
				dataType: 'json',
				success: function (data) {
					if (typeof data.ServerTime != 'undefined') {
						$rootScope.SetTimeAndSun(data.Sunrise, data.Sunset, data.ServerTime);
					}

					if (typeof data.result != 'undefined') {

						if (typeof data.ActTime != 'undefined') {
							$.LastUpdateTime = parseInt(data.ActTime);
						}
						$.each(data.result, function (i, item) {
							id = "#lightcontent #" + item.idx;
							var obj = $(id);
							if (typeof obj != 'undefined') {
								if ($(id + " #name").html() != item.Name) {
									$(id + " #name").html(item.Name);
								}
								var isdimmer = false;
								var img = "";
								var img2 = "";
								var img3 = "";
								var status = "";

								var bigtext = TranslateStatusShort(item.Status);
								if (item.UsedByCamera == true) {
									var streamimg = '<img src="images/webcam.png" title="' + $.t('Stream Video') + '" height="16" width="16">';
									var streamurl = "<a href=\"javascript:ShowCameraLiveStream('" + escape(item.Name) + "','" + item.CameraIdx + "')\">" + streamimg + "</a>";
									bigtext += "&nbsp;" + streamurl;
								}

								if (item.SubType == "Security Panel") {
									img = '<a href="secpanel/"><img src="images/security48.png" class="lcursor" height="48" width="48"></a>';
								}
								else if (item.SubType == "Evohome") {
									img = EvohomeImg(item);
								}
								else if (item.SwitchType == "X10 Siren") {
									if (
										(item.Status == 'On') ||
										(item.Status == 'Chime') ||
										(item.Status == 'Group On') ||
										(item.Status == 'All On')
									) {
										img = '<img src="images/siren-on.png" title="' + $.t("Turn Off") + '" onclick="SwitchLight(' + item.idx + ',\'Off\',RefreshLights,' + item.Protected + ');" class="lcursor" height="48" width="48">';
									}
									else {
										img = '<img src="images/siren-off.png" title="' + $.t("Turn On") + '" onclick="SwitchLight(' + item.idx + ',\'On\',RefreshLights,' + item.Protected + ');" class="lcursor" height="48" width="48">';
									}
								}
								else if (item.SwitchType == "Doorbell") {
									img = '<img src="images/doorbell48.png" title="' + $.t("Turn On") + '" onclick="SwitchLight(' + item.idx + ',\'On\',RefreshLights,' + item.Protected + ');" class="lcursor" height="48" width="48">';
								}
								else if (item.SwitchType == "Push On Button") {
									if (item.InternalState == "On") {
										img = '<img src="images/pushon48.png" title="' + $.t("Turn On") + '" onclick="SwitchLight(' + item.idx + ',\'On\',RefreshLights,' + item.Protected + ');" class="lcursor" height="48" width="48">';
									}
									else {
										img = '<img src="images/push48.png" title="' + $.t("Turn On") + '" onclick="SwitchLight(' + item.idx + ',\'On\',RefreshLights,' + item.Protected + ');" class="lcursor" height="48" width="48">';
									}
								}
								else if (item.SwitchType == "Door Contact") {
									if (item.InternalState == "Open") {
										img = '<img src="images/door48open.png" title="' + $.t("Close Door") + '" onclick="SwitchLight(' + item.idx + ',\'Off\',RefreshLights,' + item.Protected + ');" class="lcursor" height="48" width="48">';
									}
									else {
										img = '<img src="images/door48.png" title="' + $.t("Open Door") + '" onclick="SwitchLight(' + item.idx + ',\'On\',RefreshLights,' + item.Protected + ');" class="lcursor" height="48" width="48">';
									}
								}
								else if (item.SwitchType == "Door Lock") {
									if (item.InternalState == "Unlocked") {
										img = '<img src="images/door48open.png" title="' + $.t("Lock") + '" onclick="SwitchLight(' + item.idx + ',\'On\',RefreshLights,' + item.Protected + ');" class="lcursor" height="48" width="48">';
									}
									else {
										img = '<img src="images/door48.png" title="' + $.t("Unlock") + '" onclick="SwitchLight(' + item.idx + ',\'Off\',RefreshLights,' + item.Protected + ');" class="lcursor" height="48" width="48">';
									}
								}
								else if (item.SwitchType == "Push Off Button") {
									img = '<img src="images/pushoff48.png" title="' + $.t("Turn Off") + '" onclick="SwitchLight(' + item.idx + ',\'Off\',RefreshLights,' + item.Protected + ');" class="lcursor" height="48" width="48">';
								}
								else if (item.SwitchType == "Contact") {
									if (item.Status == 'Closed') {
										img = '<img src="images/contact48.png" height="48" width="48">';
									}
									else {
										img = '<img src="images/contact48_open.png" height="48" width="48">';
									}
								}
								else if ((item.SwitchType == "Blinds") || (item.SwitchType.indexOf("Venetian Blinds") == 0)) {
									if (
										(item.SubType == "RAEX") ||
										(item.SubType.indexOf('A-OK') == 0) ||
										(item.SubType.indexOf('Hasta') >= 0) ||
										(item.SubType.indexOf('Media Mount') == 0) ||
										(item.SubType.indexOf('Forest') == 0) ||
										(item.SubType.indexOf('Chamberlain') == 0) ||
										(item.SubType.indexOf('Sunpery') == 0) ||
										(item.SubType.indexOf('Dolat') == 0) ||
										(item.SubType.indexOf('ASP') == 0) ||
										(item.SubType == "Harrison") ||
										(item.SubType.indexOf('RFY') == 0) ||
										(item.SubType.indexOf('ASA') == 0) ||
										(item.SubType.indexOf('DC106') == 0) ||
										(item.SubType.indexOf('Confexx') == 0) ||
										(item.SwitchType.indexOf("Venetian Blinds") == 0)
									) {
										if (item.Status == 'Closed') {
											img = '<img src="images/blindsopen48.png" title="' + $.t("Open Blinds") + '" onclick="SwitchLight(' + item.idx + ',\'Off\',RefreshLights,' + item.Protected + ');" class="lcursor" height="48" width="48">';
											img3 = '<img src="images/blinds48sel.png" title="' + $.t("Close Blinds") + '" onclick="SwitchLight(' + item.idx + ',\'On\',RefreshLights,' + item.Protected + ');" class="lcursor" height="48" width="48">';
										}
										else {
											img = '<img src="images/blindsopen48sel.png" title="' + $.t("Open Blinds") + '" onclick="SwitchLight(' + item.idx + ',\'Off\',RefreshLights,' + item.Protected + ');" class="lcursor" height="48" width="48">';
											img3 = '<img src="images/blinds48.png" title="' + $.t("Close Blinds") + '" onclick="SwitchLight(' + item.idx + ',\'On\',RefreshLights,' + item.Protected + ');" class="lcursor" height="48" width="48">';
										}
									}
									else {
										if (item.Status == 'Closed') {
											img = '<img src="images/blindsopen48.png" title="' + $.t("Open Blinds") + '" onclick="SwitchLight(' + item.idx + ',\'Off\',RefreshLights,' + item.Protected + ');" class="lcursor" height="48" width="48">';
											img2 = '<img src="images/blinds48sel.png" title="' + $.t("Close Blinds") + '" onclick="SwitchLight(' + item.idx + ',\'On\',RefreshLights,' + item.Protected + ');" class="lcursor" height="48" width="48">';
										}
										else {
											img = '<img src="images/blindsopen48sel.png" title="' + $.t("Open Blinds") + '" onclick="SwitchLight(' + item.idx + ',\'Off\',RefreshLights,' + item.Protected + ');" class="lcursor" height="48" width="48">';
											img2 = '<img src="images/blinds48.png" title="' + $.t("Close Blinds") + '" onclick="SwitchLight(' + item.idx + ',\'On\',RefreshLights,' + item.Protected + ');" class="lcursor" height="48" width="48">';
										}
									}
								}
								else if (item.SwitchType == "Blinds Inverted") {
									if (
										(item.SubType == "RAEX") ||
										(item.SubType.indexOf('A-OK') == 0) ||
										(item.SubType.indexOf('Hasta') >= 0) ||
										(item.SubType.indexOf('Media Mount') == 0) ||
										(item.SubType.indexOf('Forest') == 0) ||
										(item.SubType.indexOf('Chamberlain') == 0) ||
										(item.SubType.indexOf('Sunpery') == 0) ||
										(item.SubType.indexOf('Dolat') == 0) ||
										(item.SubType.indexOf('ASP') == 0) ||
										(item.SubType == "Harrison") ||
										(item.SubType.indexOf('RFY') == 0) ||
										(item.SubType.indexOf('ASA') == 0) ||
										(item.SubType.indexOf('DC106') == 0) ||
										(item.SubType.indexOf('Confexx') == 0)
									) {
										if (item.Status == 'Closed') {
											img = '<img src="images/blindsopen48.png" title="' + $.t("Open Blinds") + '" onclick="SwitchLight(' + item.idx + ',\'On\',RefreshLights,' + item.Protected + ');" class="lcursor" height="48" width="48">';
											img3 = '<img src="images/blinds48sel.png" title="' + $.t("Close Blinds") + '" onclick="SwitchLight(' + item.idx + ',\'Off\',RefreshLights,' + item.Protected + ');" class="lcursor" height="48" width="48">';
										}
										else {
											img = '<img src="images/blindsopen48sel.png" title="' + $.t("Open Blinds") + '" onclick="SwitchLight(' + item.idx + ',\'On\',RefreshLights,' + item.Protected + ');" class="lcursor" height="48" width="48">';
											img3 = '<img src="images/blinds48.png" title="' + $.t("Close Blinds") + '" onclick="SwitchLight(' + item.idx + ',\'Off\',RefreshLights,' + item.Protected + ');" class="lcursor" height="48" width="48">';
										}
									}
									else {
										if (item.Status == 'Closed') {
											img = '<img src="images/blindsopen48.png" title="' + $.t("Open Blinds") + '" onclick="SwitchLight(' + item.idx + ',\'On\',RefreshLights,' + item.Protected + ');" class="lcursor" height="48" width="48">';
											img2 = '<img src="images/blinds48sel.png" title="' + $.t("Close Blinds") + '" onclick="SwitchLight(' + item.idx + ',\'Off\',RefreshLights,' + item.Protected + ');" class="lcursor" height="48" width="48">';
										}
										else {
											img = '<img src="images/blindsopen48sel.png" title="' + $.t("Open Blinds") + '" onclick="SwitchLight(' + item.idx + ',\'On\',RefreshLights,' + item.Protected + ');" class="lcursor" height="48" width="48">';
											img2 = '<img src="images/blinds48.png" title="' + $.t("Close Blinds") + '" onclick="SwitchLight(' + item.idx + ',\'Off\',RefreshLights,' + item.Protected + ');" class="lcursor" height="48" width="48">';
										}
									}
								}
								else if (item.SwitchType == "Blinds Percentage") {
									isdimmer = true;
									if (item.Status == 'Open') {
										img = '<img src="images/blindsopen48sel.png" title="' + $.t("Open Blinds") + '" onclick="SwitchLight(' + item.idx + ',\'Off\',RefreshLights,' + item.Protected + ');" class="lcursor" height="48">';
										img2 = '<img src="images/blinds48.png" title="' + $.t("Close Blinds") + '" onclick="SwitchLight(' + item.idx + ',\'On\',RefreshLights,' + item.Protected + ');" class="lcursor" height="48">';
									}
									else {
										img = '<img src="images/blindsopen48.png" title="' + $.t("Open Blinds") + '" onclick="SwitchLight(' + item.idx + ',\'Off\',RefreshLights,' + item.Protected + ');" class="lcursor" height="48">';
										img2 = '<img src="images/blinds48sel.png" title="' + $.t("Close Blinds") + '" onclick="SwitchLight(' + item.idx + ',\'On\',RefreshLights,' + item.Protected + ');" class="lcursor" height="48">';
									}
								}
								else if (item.SwitchType == "Blinds Percentage Inverted") {
									isdimmer = true;
									if (item.Status == 'Closed') {
										img = '<img src="images/blindsopen48.png" title="' + $.t("Open Blinds") + '" onclick="SwitchLight(' + item.idx + ',\'On\',RefreshLights,' + item.Protected + ');" class="lcursor" height="48">';
										img2 = '<img src="images/blinds48sel.png" title="' + $.t("Close Blinds") + '" onclick="SwitchLight(' + item.idx + ',\'Off\',RefreshLights,' + item.Protected + ');" class="lcursor" height="48">';
									}
									else if ((item.Status == 'Open') || (item.Status.indexOf('Set ') == 0)) {
										img = '<img src="images/blindsopen48sel.png" title="' + $.t("Open Blinds") + '" onclick="SwitchLight(' + item.idx + ',\'On\',RefreshLights,' + item.Protected + ');" class="lcursor" height="48">';
										img2 = '<img src="images/blinds48.png" title="' + $.t("Close Blinds") + '" onclick="SwitchLight(' + item.idx + ',\'Off\',RefreshLights,' + item.Protected + ');" class="lcursor" height="48">';
									}
								}
								else if (item.SwitchType == "Smoke Detector") {
									if (
										(item.Status == 'Panic') ||
										(item.Status == 'On')
									) {
										img = '<img src="images/smoke48on.png" title="' + $.t("Turn Alarm On") + '" onclick="SwitchLight(' + item.idx + ',\'On\',RefreshLights,' + item.Protected + ');" class="lcursor" height="48" width="48">';
										$(id + ' #resetbtn').attr("class", "btnsmall-sel");
									}
									else {
										img = '<img src="images/smoke48off.png" title="' + $.t("Turn Alarm On") + '" onclick="SwitchLight(' + item.idx + ',\'On\',RefreshLights,' + item.Protected + ');" class="lcursor" height="48" width="48">';
										$(id + ' #resetbtn').attr("class", "btnsmall-dis");
									}

								}
								else if (item.Type == "Security") {
									if (item.SubType.indexOf('remote') > 0) {
										if ((item.Status.indexOf('Arm') >= 0) || (item.Status.indexOf('Panic') >= 0)) {
											img += '<img src="images/remote48.png" title="' + $.t("Turn Alarm Off") + '" onclick="SwitchLight(' + item.idx + ',\'Off\',RefreshLights,' + item.Protected + ');" class="lcursor" height="48" width="48">\n';
										}
										else {
											img += '<img src="images/remote48.png" title="' + $.t("Turn Alarm On") + '" onclick="ArmSystem(' + item.idx + ',\'On\',RefreshLights,' + item.Protected + ');" class="lcursor" height="48" width="48">\n';
										}
									}
									else if (item.SubType == "X10 security") {
										if (item.Status.indexOf('Normal') >= 0) {
											img += '<img src="images/security48.png" title="' + $.t("Turn Alarm On") + '" onclick="SwitchLight(' + item.idx + ',\'' + ((item.Status == "Normal Delayed") ? "Alarm Delayed" : "Alarm") + '\',RefreshLights,' + item.Protected + ');" class="lcursor" height="48" width="48">';
										}
										else {
											img += '<img src="images/Alarm48_On.png" title="' + $.t("Turn Alarm Off") + '" onclick="SwitchLight(' + item.idx + ',\'' + ((item.Status == "Alarm Delayed") ? "Normal Delayed" : "Normal") + '\',RefreshLights,' + item.Protected + ');" class="lcursor" height="48" width="48">';
										}
									}
									else if (item.SubType == "X10 security motion") {
										if ((item.Status == "No Motion")) {
											img += '<img src="images/security48.png" title="' + $.t("Turn Alarm On") + '" onclick="SwitchLight(' + item.idx + ',\'Motion\',RefreshLights,' + item.Protected + ');" class="lcursor" height="48" width="48">';
										}
										else {
											img += '<img src="images/Alarm48_On.png" title="' + $.t("Turn Alarm Off") + '" onclick="SwitchLight(' + item.idx + ',\'No Motion\',RefreshLights,' + item.Protected + ');" class="lcursor" height="48" width="48">';
										}
									}
									else if ((item.Status.indexOf('Alarm') >= 0) || (item.Status.indexOf('Tamper') >= 0)) {
										img = '<img src="images/Alarm48_On.png" height="48" width="48">';
									}
									else {
										if (item.SubType.indexOf('Meiantech') >= 0) {
											if ((item.Status.indexOf('Arm') >= 0) || (item.Status.indexOf('Panic') >= 0)) {
												img = '<img src="images/security48.png" title="' + $.t("Turn Alarm Off") + '" onclick="SwitchLight(' + item.idx + ',\'Off\',RefreshLights,' + item.Protected + ');" class="lcursor" height="48" width="48">';
											}
											else {
												img = '<img src="images/security48.png" title="' + $.t("Turn Alarm On") + '" onclick="ArmSystemMeiantech(' + item.idx + ',\'On\',RefreshLights,' + item.Protected + ');" class="lcursor" height="48" width="48">';
											}
										}
										else {
											if (item.SubType.indexOf('KeeLoq') >= 0) {
												img = '<img src="images/pushon48.png" title="' + $.t("Turn On") + '" onclick="SwitchLight(' + item.idx + ',\'On\',RefreshLights,' + item.Protected + ');" class="lcursor" height="48" width="48">';
											}
											else {
												img = '<img src="images/security48.png" height="48" width="48">';
											}
										}
									}
								}
								else if (item.SwitchType == "Dimmer") {
									isdimmer = true;
									if (item.CustomImage == 0) item.Image = item.TypeImg;
									item.Image = item.Image.charAt(0).toUpperCase() + item.Image.slice(1);
									if (
										(item.Status == 'On') ||
										(item.Status == 'Chime') ||
										(item.Status == 'Group On') ||
										(item.Status.indexOf('Set ') == 0)
									) {
										if (item.SubType == "RGB") {
											img = '<img src="images/RGB48_On.png" onclick="ShowRGBWPopup(event, ' + item.idx + ', \'RefreshLights\',' + item.Protected + ',' + item.MaxDimLevel + ',' + item.LevelInt + ',' + item.Hue + ');" class="lcursor" height="48" width="48">';
										}
										else if (item.SubType.indexOf("RGBW") >= 0) {
											img = '<img src="images/RGB48_On.png" onclick="ShowRGBWPopup(event, ' + item.idx + ', \'RefreshLights\',' + item.Protected + ',' + item.MaxDimLevel + ',' + item.LevelInt + ',' + item.Hue + ');" class="lcursor" height="48" width="48">';
										}
										else {
											img = '<img src="images/' + item.Image + '48_On.png" title="' + $.t("Turn Off") + '" onclick="SwitchLight(' + item.idx + ',\'Off\',RefreshLights,' + item.Protected + ');" class="lcursor" height="48" width="48">';
										}
									}
									else {
										if (item.SubType == "RGB") {
											img = '<img src="images/RGB48_Off.png" onclick="ShowRGBWPopup(event, ' + item.idx + ',\'RefreshLights\',' + item.Protected + ',' + item.MaxDimLevel + ',' + item.LevelInt + ',' + item.Hue + ');" class="lcursor" height="48" width="48">';
										}
										else if (item.SubType.indexOf("RGBW") >= 0) {
											img = '<img src="images/RGB48_Off.png" onclick="ShowRGBWPopup(event, ' + item.idx + ',\'RefreshLights\',' + item.Protected + ',' + item.MaxDimLevel + ',' + item.LevelInt + ',' + item.Hue + ');" class="lcursor" height="48" width="48">';
										}
										else {
											img = '<img src="images/' + item.Image + '48_Off.png" title="' + $.t("Turn On") + '" onclick="SwitchLight(' + item.idx + ',\'On\',RefreshLights,' + item.Protected + ');" class="lcursor" height="48" width="48">';
										}
									}
								}
								else if (item.SwitchType == "TPI") {
									var RO = (item.Unit < 64 || item.Unit > 95) ? true : false;
									isdimmer = true;
									if (
										(item.Status == 'On')
									) {
										img = '<img src="images/Fireplace48_On.png" title="' + $.t(RO ? "On" : "Turn Off") + (RO ? '"' : '" onclick="SwitchLight(' + item.idx + ',\'Off\',RefreshLights,' + item.Protected + ');" class="lcursor"') + ' height="48" width="48">';
									}
									else {
										img = '<img src="images/Fireplace48_Off.png" title="' + $.t(RO ? "Off" : "Turn On") + (RO ? '"' : '" onclick="SwitchLight(' + item.idx + ',\'On\',RefreshLights,' + item.Protected + ');" class="lcursor"') + ' height="48" width="48">';
									}
								}
								else if (item.SwitchType == "Dusk Sensor") {
									if (
										(item.Status == 'On')
									) {
										img = '<img src="images/uvdark.png" title="' + $.t("Nighttime") + '" height="48" width="48">';
									}
									else {
										img = '<img src="images/uvsunny.png" title="' + $.t("Daytime") + '" height="48" width="48">';
									}
								}
								else if (item.SwitchType == "Media Player") {
									if (item.CustomImage == 0) item.Image = item.TypeImg;
									if (item.Status == 'Disconnected') {
										img = '<img src="images/' + item.Image + '48_Off.png" height="48" width="48">';
										img2 = '<img src="images/remote48.png" style="opacity:0.4"; height="48" width="48">';
									}
									else if ((item.Status != 'Off') && (item.Status != '0')) {
										img = '<img src="images/' + item.Image + '48_On.png" onclick="SwitchLight(' + item.idx + ',\'Off\',RefreshLights,' + item.Protected + ');" class="lcursor" height="48" width="48">';
										img2 = '<img src="images/remote48.png" onclick="ShowMediaRemote(\'' + escape(item.Name) + "'," + item.idx + ",'" + item.HardwareType + '\');" class="lcursor" height="48" width="48">';
									}
									else {
										img = '<img src="images/' + item.Image + '48_Off.png" onclick="SwitchLight(' + item.idx + ',\'On\',RefreshLights,' + item.Protected + ');" class="lcursor" height="48" width="48">';
										img2 = '<img src="images/remote48.png" style="opacity:0.4"; height="48" width="48">';
									}
									if (item.Status.length == 1) item.Status = "";
									status = item.Data;
									if (status == '0') status = "";
								}
								else if (item.SwitchType == "Motion Sensor") {
									if (
										(item.Status == 'On') ||
										(item.Status == 'Chime') ||
										(item.Status == 'Group On') ||
										(item.Status.indexOf('Set ') == 0)
									) {
										img = '<img src="images/motion48-on.png" height="48" width="48">';
									}
									else {
										img = '<img src="images/motion48-off.png" height="48" width="48">';
									}
								}
								else if (item.SwitchType === "Selector") {
									if ((item.Status === "Off")) {
										img += '<img src="images/' + item.Image + '48_Off.png" height="48" width="48">';
									} else if (item.LevelOffHidden) {
										img += '<img src="images/' + item.Image + '48_On.png" height="48" width="48">';
									} else {
										img += '<img src="images/' + item.Image + '48_On.png" title="' + $.t("Turn Off") + '" onclick="SwitchLight(' + item.idx + ',\'Off\',RefreshLights,' + item.Protected + ');" class="lcursor" height="48" width="48">';
									}
								}
								else if ((item.SubType.indexOf("Itho") == 0) || (item.SubType.indexOf("Lucci") == 0)) {
									img = $(id + " #img").html();
								}
								else {
									if (
										(item.Status == 'On') ||
										(item.Status == 'Chime') ||
										(item.Status == 'Group On') ||
										(item.Status.indexOf('Down') != -1) ||
										(item.Status.indexOf('Up') != -1) ||
										(item.Status.indexOf('Set ') == 0)
									) {
										if (item.Type == "Thermostat 3") {
											img = '<img src="images/' + item.Image + '48_On.png" onclick="ShowTherm3Popup(event, ' + item.idx + ', \'RefreshLights\',' + item.Protected + ');" class="lcursor" height="48" width="48">';
										}
										else {
											img = '<img src="images/' + item.Image + '48_On.png" title="' + $.t("Turn Off") + '" onclick="SwitchLight(' + item.idx + ',\'Off\',RefreshLights,' + item.Protected + ');" class="lcursor" height="48" width="48">';
										}
									}
									else {
										if (item.Type == "Thermostat 3") {
											img = '<img src="images/' + item.Image + '48_Off.png" onclick="ShowTherm3Popup(event, ' + item.idx + ', \'RefreshLights\',' + item.Protected + ');" class="lcursor" height="48" width="48">';
										}
										else {
											img = '<img src="images/' + item.Image + '48_Off.png" title="' + $.t("Turn On") + '" onclick="SwitchLight(' + item.idx + ',\'On\',RefreshLights,' + item.Protected + ');" class="lcursor" height="48" width="48">';
										}
									}
								}

								var backgroundClass = $rootScope.GetItemBackgroundStatus(item);
								$(id).removeClass('statusNormal').removeClass('statusProtected').removeClass('statusTimeout').removeClass('statusLowBattery');
								$(id).addClass(backgroundClass);

								if ($(id + " #img").html() != img) {
									$(id + " #img").html(img);
								}
								if (img2 != "") {
									if ($(id + " #img2").html() != img2) {
										$(id + " #img2").html(img2);
									}
								}
								if (img3 != "") {
									if ($(id + " #img3").html() != img3) {
										$(id + " #img3").html(img3);
									}
								}
								if (isdimmer == true) {
									var dslider = $(id + " #slider");
									if (typeof dslider != 'undefined') {
										dslider.slider("value", item.LevelInt + 1);
									}
								}
								if (item.SwitchType === "Selector") {
									var selector$ = $("#selector" + item.idx);
									if (typeof selector$ !== 'undefined') {
										if (item.SelectorStyle === 0) {
											selector$
												.find('label')
												.removeClass('ui-state-active')
												.removeClass('ui-state-focus')
												.removeClass('ui-state-hover')
												.end()
												.find('input:radio')
												.removeProp('checked')
												.filter('[value="' + item.LevelInt + '"]')
												.prop('checked', true)
												.end()
												.end()
												.buttonset('refresh');
										} else if (item.SelectorStyle === 1) {
											selector$.val(item.LevelInt);
											selector$.selectmenu('refresh');
										}
									}
									bigtext = GetLightStatusText(item);
								}
								if ($(id + " #bigtext").html() != TranslateStatus(GetLightStatusText(item))) {
									$(id + " #bigtext").html(bigtext);
								}
								if ((typeof $(id + " #status") != 'undefined') && ($(id + " #status").html() != status)) {
									$(id + " #status").html(status);
								}
								if ($(id + " #lastupdate").html() != item.LastUpdate) {
									$(id + " #lastupdate").html(item.LastUpdate);
								}
								if ($scope.config.ShowUpdatedEffect == true) {
									$(id + " #name").effect("highlight", { color: '#EEFFEE' }, 1000);
								}
							}
						});
					}
				}
			});
			$scope.mytimer = $interval(function () {
				RefreshLights();
			}, 10000);
		}

		ShowLights = function () {
			if (typeof $scope.mytimer != 'undefined') {
				$interval.cancel($scope.mytimer);
				$scope.mytimer = undefined;
			}
			$('#modal').show();

			var htmlcontent = '';
			var bShowRoomplan = false;
			$.RoomPlans = [];
			$.ajax({
				url: "json.htm?type=plans",
				async: false,
				dataType: 'json',
				success: function (data) {
					if (typeof data.result != 'undefined') {
						var totalItems = data.result.length;
						if (totalItems > 0) {
							bShowRoomplan = true;
							//				if (window.myglobals.ismobile==true) {
							//				bShowRoomplan=false;
							//		}
							if (bShowRoomplan == true) {
								$.each(data.result, function (i, item) {
									$.RoomPlans.push({
										idx: item.idx,
										name: item.Name
									});
								});
							}
						}
					}
				}
			});

			var bHaveAddedDevider = false;

			var tophtm = "";
			if ($.RoomPlans.length == 0) {
				tophtm +=
					'\t<table border="0" cellpadding="0" cellspacing="0" width="100%">\n' +
					'\t<tr>\n' +
					'\t  <td align="left" valign="top" id="timesun"></td>\n' +
					'\t</tr>\n' +
					'\t</table>\n';
			}
			else {
				tophtm +=
					'\t<table border="0" cellpadding="0" cellspacing="0" width="100%">\n' +
					'\t<tr>\n' +
					'\t  <td align="left" valign="top" id="timesun"></td>\n' +
					'<td align="right">' +
					'<span data-i18n="Room">Room</span>:&nbsp;<select id="comboroom" style="width:160px" class="combobox ui-corner-all">' +
					'<option value="0" data-i18n="All">All</option>' +
					'</select>' +
					'</td>' +
					'\t</tr>\n' +
					'\t</table>\n';
			}
			if (permissions.hasPermission("Admin")) {
				tophtm +=
					'\t<table class="bannav" id="bannav" border="0" cellpadding="0" cellspacing="0" width="100%">\n' +
					'\t<tr>\n' +
					'\t  <td align="left"><a class="btnstylerev" onclick="AddManualLightDevice();" data-i18n="Manual Light/Switch">Manual Light/Switch</a></td>\n' +
					'\t  <td align="right"><a class="btnstyle" onclick="AddLightDevice();" data-i18n="Learn Light/Switch">Learn Light/Switch</a></td>\n' +
					'\t</tr>\n' +
					'\t</table>\n';
			}

			var i = 0;
			var j = 0;

			$.ajax({
				url: "json.htm?type=devices&filter=light&used=true&order=[Order]&plan=" + window.myglobals.LastPlanSelected,
				async: false,
				dataType: 'json',
				success: function (data) {


					htmlcontent += EvohomeAddJS();

					if (typeof data.result != 'undefined') {
						if (typeof data.ActTime != 'undefined') {
							$.LastUpdateTime = parseInt(data.ActTime);
						}

						$.each(data.result, function (i, item) {
							if (j % 3 == 0) {
								//add devider
								if (bHaveAddedDevider == true) {
									//close previous devider
									htmlcontent += '</div>\n';
								}
								htmlcontent += '<div class="row divider">\n';
								bHaveAddedDevider = true;
							}
							var bAddTimer = true;
							var bIsDimmer = false;

							var backgroundClass = $rootScope.GetItemBackgroundStatus(item);

							var status = "";
							var xhtm =
								'\t<div class="item span4 ' + backgroundClass + '" id="' + item.idx + '">\n' +
								'\t  <section>\n';
							if ((item.SwitchType == "Blinds") || (item.SwitchType == "Blinds Inverted") || (item.SwitchType == "Blinds Percentage") || (item.SwitchType == "Blinds Percentage Inverted") || (item.SwitchType.indexOf("Venetian Blinds") == 0) || (item.SwitchType.indexOf("Media Player") == 0)) {
								if (
									(item.SubType == "RAEX") ||
									(item.SubType.indexOf('A-OK') == 0) ||
									(item.SubType.indexOf('Hasta') >= 0) ||
									(item.SubType.indexOf('Media Mount') == 0) ||
									(item.SubType.indexOf('Forest') == 0) ||
									(item.SubType.indexOf('Chamberlain') == 0) ||
									(item.SubType.indexOf('Sunpery') == 0) ||
									(item.SubType.indexOf('Dolat') == 0) ||
									(item.SubType.indexOf('ASP') == 0) ||
									(item.SubType == "Harrison") ||
									(item.SubType.indexOf('RFY') == 0) ||
									(item.SubType.indexOf('ASA') == 0) ||
									(item.SubType.indexOf('DC106') == 0) ||
									(item.SubType.indexOf('Confexx') == 0) ||
									(item.SwitchType.indexOf("Venetian Blinds") == 0)
								) {
									xhtm += '\t    <table id="itemtabletrippleicon" border="0" cellpadding="0" cellspacing="0">\n';
								}
								else {
									xhtm += '\t    <table id="itemtabledoubleicon" border="0" cellpadding="0" cellspacing="0">\n';
								}
							}
							else {
								xhtm += '\t    <table id="itemtablenostatus" border="0" cellpadding="0" cellspacing="0">\n';
							}

							xhtm +=
								'\t    <tr>\n' +
								'\t      <td id="name">' + item.Name + '</td>\n' +
								'\t      <td id="bigtext">';
							var bigtext = TranslateStatusShort(item.Status);
							if (item.UsedByCamera == true) {
								var streamimg = '<img src="images/webcam.png" title="' + $.t('Stream Video') + '" height="16" width="16">';
								var streamurl = "<a href=\"javascript:ShowCameraLiveStream('" + escape(item.Name) + "','" + item.CameraIdx + "')\">" + streamimg + "</a>";
								bigtext += "&nbsp;" + streamurl;
							}
							if (permissions.hasPermission("Admin")) {
								if (item.Type == "RFY") {
									var rfysetup = '<img src="images/devices.png" title="' + $.t('Setup') + '" height="16" width="16" onclick="ShowRFYPopup(event, ' + item.idx + ', ShowLights, ' + item.Protected + ');">';
									bigtext += "&nbsp;" + rfysetup;
								}
							}
							xhtm += bigtext + '</td>\n';
							if (item.SubType == "Security Panel") {
								xhtm += '\t      <td id="img"><a href="secpanel/"><img src="images/security48.png" class="lcursor" height="48" width="48"></a></td>\n';
							}
							else if (item.SubType == "Evohome") {
								xhtm += EvohomePopupMenu(item);
							}
							else if (item.SwitchType == "X10 Siren") {
								if (
									(item.Status == 'On') ||
									(item.Status == 'Chime') ||
									(item.Status == 'Group On') ||
									(item.Status == 'All On')
								) {
									xhtm += '\t      <td id="img"><img src="images/siren-on.png" title="' + $.t("Turn Off") + '" onclick="SwitchLight(' + item.idx + ',\'Off\',RefreshLights,' + item.Protected + ');" class="lcursor" height="48" width="48"></td>\n';
								}
								else {
									xhtm += '\t      <td id="img"><img src="images/siren-off.png" title="' + $.t("Turn On") + '" onclick="SwitchLight(' + item.idx + ',\'On\',RefreshLights,' + item.Protected + ');" class="lcursor" height="48" width="48"></td>\n';
								}
							}
							else if (item.SwitchType == "Doorbell") {
								xhtm += '\t      <td id="img"><img src="images/doorbell48.png" title="' + $.t("Turn On") + '" onclick="SwitchLight(' + item.idx + ',\'On\',RefreshLights,' + item.Protected + ');" class="lcursor" height="48" width="48"></td>\n';
								bAddTimer = false;
							}
							else if (item.SwitchType == "Push On Button") {
								if (item.InternalState == "On") {
									xhtm += '\t      <td id="img"><img src="images/pushon48.png" title="' + $.t("Turn On") + '" onclick="SwitchLight(' + item.idx + ',\'On\',RefreshLights,' + item.Protected + ');" class="lcursor" height="48" width="48"></td>\n';
								}
								else {
									xhtm += '\t      <td id="img"><img src="images/push48.png" title="' + $.t("Turn On") + '" onclick="SwitchLight(' + item.idx + ',\'On\',RefreshLights,' + item.Protected + ');" class="lcursor" height="48" width="48"></td>\n';
								}
							}
							else if (item.SwitchType == "Door Contact") {
								if (item.InternalState == "Open") {
									xhtm += '\t      <td id="img"><img src="images/door48open.png" title="' + $.t("Close Door") + '" onclick="SwitchLight(' + item.idx + ',\'Off\',RefreshLights,' + item.Protected + ');" class="lcursor" height="48" width="48"></td>\n';
								}
								else {
									xhtm += '\t      <td id="img"><img src="images/door48.png" title="' + $.t("Open Door") + '" onclick="SwitchLight(' + item.idx + ',\'On\',RefreshLights,' + item.Protected + ');" class="lcursor" height="48" width="48"></td>\n';
								}
								bAddTimer = false;
							}
							else if (item.SwitchType == "Door Lock") {
								if (item.InternalState == "Unlocked") {
									xhtm += '\t      <td id="img"><img src="images/door48open.png" title="' + $.t("Lock") + '" onclick="SwitchLight(' + item.idx + ',\'On\',RefreshLights,' + item.Protected + ');" class="lcursor" height="48" width="48"></td>\n';
								}
								else {
									xhtm += '\t      <td id="img"><img src="images/door48.png" title="' + $.t("Unlock") + '" onclick="SwitchLight(' + item.idx + ',\'Off\',RefreshLights,' + item.Protected + ');" class="lcursor" height="48" width="48"></td>\n';
								}
								bAddTimer = false;
							}
							else if (item.SwitchType == "Push Off Button") {
								xhtm += '\t      <td id="img"><img src="images/pushoff48.png" title="' + $.t("Turn Off") + '" onclick="SwitchLight(' + item.idx + ',\'Off\',RefreshLights,' + item.Protected + ');" class="lcursor" height="48" width="48"></td>\n';
							}
							else if (item.SwitchType == "Contact") {
								if (item.Status == 'Closed') {
									xhtm += '\t      <td id="img"><img src="images/contact48.png" height="48" width="48"></td>\n';
								}
								else {
									xhtm += '\t      <td id="img"><img src="images/contact48_open.png" height="48" width="48"></td>\n';
								}
								bAddTimer = false;
							}
							else if (item.SwitchType == "Media Player") {
								if (item.CustomImage == 0) item.Image = item.TypeImg;
								if (item.Status == 'Disconnected') {
									xhtm += '\t      <td id="img"><img src="images/' + item.Image + '48_Off.png" height="48" width="48"></td>\n';
									xhtm += '\t      <td id="img2"><img src="images/remote48.png" style="opacity:0.4"; height="48" width="48"></td>\n';
								}
								else if ((item.Status != 'Off') && (item.Status != '0')) {
									xhtm += '\t      <td id="img"><img src="images/' + item.Image + '48_On.png" onclick="SwitchLight(' + item.idx + ',\'Off\',RefreshLights,' + item.Protected + ');" class="lcursor" height="48" width="48"></td>\n';
									xhtm += '\t      <td id="img2"><img src="images/remote48.png" onclick="ShowMediaRemote(\'' + escape(item.Name) + "'," + item.idx + ",'" + item.HardwareType + '\');" class="lcursor" height="48" width="48"></td>\n';
								}
								else {
									xhtm += '\t      <td id="img"><img src="images/' + item.Image + '48_Off.png" onclick="SwitchLight(' + item.idx + ',\'On\',RefreshLights,' + item.Protected + ');" class="lcursor" height="48" width="48"></td>\n';
									xhtm += '\t      <td id="img2"><img src="images/remote48.png" style="opacity:0.4"; height="48" width="48"></td>\n';
								}
								if (item.Status.length == 1) item.Status = "";
								status = item.Data;
								bAddTimer = false;
							}
							else if ((item.SwitchType == "Blinds") || (item.SwitchType.indexOf("Venetian Blinds") == 0)) {
								if (
									(item.SubType == "RAEX") ||
									(item.SubType.indexOf('A-OK') == 0) ||
									(item.SubType.indexOf('Hasta') >= 0) ||
									(item.SubType.indexOf('Media Mount') == 0) ||
									(item.SubType.indexOf('Forest') == 0) ||
									(item.SubType.indexOf('Chamberlain') == 0) ||
									(item.SubType.indexOf('Sunpery') == 0) ||
									(item.SubType.indexOf('Dolat') == 0) ||
									(item.SubType.indexOf('ASP') == 0) ||
									(item.SubType == "Harrison") ||
									(item.SubType.indexOf('RFY') == 0) ||
									(item.SubType.indexOf('ASA') == 0) ||
									(item.SubType.indexOf('DC106') == 0) ||
									(item.SubType.indexOf('Confexx') == 0) ||
									(item.SwitchType.indexOf("Venetian Blinds") == 0)
								) {
									if (item.Status == 'Closed') {
										xhtm += '\t      <td id="img"><img src="images/blindsopen48.png" title="' + $.t("Open Blinds") + '" onclick="SwitchLight(' + item.idx + ',\'Off\',RefreshLights,' + item.Protected + ');" class="lcursor" height="48" width="48"></td>\n';
										xhtm += '\t      <td id="img2"><img src="images/blindsstop.png" title="' + $.t("Stop Blinds") + '" onclick="SwitchLight(' + item.idx + ',\'Stop\',RefreshLights,' + item.Protected + ');" class="lcursor" height="48" width="24"></td>\n';
										xhtm += '\t      <td id="img3"><img src="images/blinds48sel.png" title="' + $.t("Close Blinds") + '" onclick="SwitchLight(' + item.idx + ',\'On\',RefreshLights,' + item.Protected + ');" class="lcursor" height="48" width="48"></td>\n';
									}
									else {
										xhtm += '\t      <td id="img"><img src="images/blindsopen48sel.png" title="' + $.t("Open Blinds") + '" onclick="SwitchLight(' + item.idx + ',\'Off\',RefreshLights,' + item.Protected + ');" class="lcursor" height="48" width="48"></td>\n';
										xhtm += '\t      <td id="img2"><img src="images/blindsstop.png" title="' + $.t("Stop Blinds") + '" onclick="SwitchLight(' + item.idx + ',\'Stop\',RefreshLights,' + item.Protected + ');" class="lcursor" height="48" width="24"></td>\n';
										xhtm += '\t      <td id="img3"><img src="images/blinds48.png" title="' + $.t("Close Blinds") + '" onclick="SwitchLight(' + item.idx + ',\'On\',RefreshLights,' + item.Protected + ');" class="lcursor" height="48" width="48"></td>\n';
									}
								}
								else {
									if (item.Status == 'Closed') {
										xhtm += '\t      <td id="img"><img src="images/blindsopen48.png" title="' + $.t("Open Blinds") + '" onclick="SwitchLight(' + item.idx + ',\'Off\',RefreshLights,' + item.Protected + ');" class="lcursor" height="48" width="48"></td>\n';
										xhtm += '\t      <td id="img2"><img src="images/blinds48sel.png" title="' + $.t("Close Blinds") + '" onclick="SwitchLight(' + item.idx + ',\'On\',RefreshLights,' + item.Protected + ');" class="lcursor" height="48" width="48"></td>\n';
									}
									else {
										xhtm += '\t      <td id="img"><img src="images/blindsopen48sel.png" title="' + $.t("Open Blinds") + '" onclick="SwitchLight(' + item.idx + ',\'Off\',RefreshLights,' + item.Protected + ');" class="lcursor" height="48" width="48"></td>\n';
										xhtm += '\t      <td id="img2"><img src="images/blinds48.png" title="' + $.t("Close Blinds") + '" onclick="SwitchLight(' + item.idx + ',\'On\',RefreshLights,' + item.Protected + ');" class="lcursor" height="48" width="48"></td>\n';
									}
								}
							}
							else if (item.SwitchType == "Blinds Inverted") {
								if (
									(item.SubType == "RAEX") ||
									(item.SubType.indexOf('A-OK') == 0) ||
									(item.SubType.indexOf('Hasta') >= 0) ||
									(item.SubType.indexOf('Media Mount') == 0) ||
									(item.SubType.indexOf('Forest') == 0) ||
									(item.SubType.indexOf('Chamberlain') == 0) ||
									(item.SubType.indexOf('Sunpery') == 0) ||
									(item.SubType.indexOf('Dolat') == 0) ||
									(item.SubType.indexOf('ASP') == 0) ||
									(item.SubType == "Harrison") ||
									(item.SubType.indexOf('RFY') == 0) ||
									(item.SubType.indexOf('ASA') == 0) ||
									(item.SubType.indexOf('DC106') == 0) ||
									(item.SubType.indexOf('Confexx') == 0)
								) {
									if (item.Status == 'Closed') {
										xhtm += '\t      <td id="img"><img src="images/blindsopen48.png" title="' + $.t("Open Blinds") + '" onclick="SwitchLight(' + item.idx + ',\'On\',RefreshLights,' + item.Protected + ');" class="lcursor" height="48" width="48"></td>\n';
										xhtm += '\t      <td id="img2"><img src="images/blindsstop.png" title="' + $.t("Stop Blinds") + '" onclick="SwitchLight(' + item.idx + ',\'Stop\',RefreshLights,' + item.Protected + ');" class="lcursor" height="48" width="24"></td>\n';
										xhtm += '\t      <td id="img3"><img src="images/blinds48sel.png" title="' + $.t("Close Blinds") + '" onclick="SwitchLight(' + item.idx + ',\'Off\',RefreshLights,' + item.Protected + ');" class="lcursor" height="48" width="48"></td>\n';
									}
									else {
										xhtm += '\t      <td id="img"><img src="images/blindsopen48sel.png" title="' + $.t("Open Blinds") + '" onclick="SwitchLight(' + item.idx + ',\'On\',RefreshLights,' + item.Protected + ');" class="lcursor" height="48" width="48"></td>\n';
										xhtm += '\t      <td id="img2"><img src="images/blindsstop.png" title="' + $.t("Stop Blinds") + '" onclick="SwitchLight(' + item.idx + ',\'Stop\',RefreshLights,' + item.Protected + ');" class="lcursor" height="48" width="24"></td>\n';
										xhtm += '\t      <td id="img3"><img src="images/blinds48.png" title="' + $.t("Close Blinds") + '" onclick="SwitchLight(' + item.idx + ',\'Off\',RefreshLights,' + item.Protected + ');" class="lcursor" height="48" width="48"></td>\n';
									}
								}
								else {
									if (item.Status == 'Closed') {
										xhtm += '\t      <td id="img"><img src="images/blindsopen48.png" title="' + $.t("Open Blinds") + '" onclick="SwitchLight(' + item.idx + ',\'On\',RefreshLights,' + item.Protected + ');" class="lcursor" height="48" width="48"></td>\n';
										xhtm += '\t      <td id="img2"><img src="images/blinds48sel.png" title="' + $.t("Close Blinds") + '" onclick="SwitchLight(' + item.idx + ',\'Off\',RefreshLights,' + item.Protected + ');" class="lcursor" height="48" width="48"></td>\n';
									}
									else {
										xhtm += '\t      <td id="img"><img src="images/blindsopen48sel.png" title="' + $.t("Open Blinds") + '" onclick="SwitchLight(' + item.idx + ',\'On\',RefreshLights,' + item.Protected + ');" class="lcursor" height="48" width="48"></td>\n';
										xhtm += '\t      <td id="img2"><img src="images/blinds48.png" title="' + $.t("Close Blinds") + '" onclick="SwitchLight(' + item.idx + ',\'Off\',RefreshLights,' + item.Protected + ');" class="lcursor" height="48" width="48"></td>\n';
									}
								}
							}
							else if (item.SwitchType == "Blinds Percentage") {
								bIsDimmer = true;
								if (item.Status == 'Open') {
									xhtm += '\t      <td id="img"><img src="images/blindsopen48sel.png" title="' + $.t("Open Blinds") + '" onclick="SwitchLight(' + item.idx + ',\'Off\',RefreshLights,' + item.Protected + ');" class="lcursor" height="48"></td>\n';
									xhtm += '\t      <td id="img2"><img src="images/blinds48.png" title="' + $.t("Close Blinds") + '" onclick="SwitchLight(' + item.idx + ',\'On\',RefreshLights,' + item.Protected + ');" class="lcursor" height="48"></td>\n';
								}
								else {
									xhtm += '\t      <td id="img"><img src="images/blindsopen48.png" title="' + $.t("Open Blinds") + '" onclick="SwitchLight(' + item.idx + ',\'Off\',RefreshLights,' + item.Protected + ');" class="lcursor" height="48"></td>\n';
									xhtm += '\t      <td id="img2"><img src="images/blinds48sel.png" title="' + $.t("Close Blinds") + '" onclick="SwitchLight(' + item.idx + ',\'On\',RefreshLights,' + item.Protected + ');" class="lcursor" height="48"></td>\n';
								}
							}
							else if (item.SwitchType == "Blinds Percentage Inverted") {
								bIsDimmer = true;
								if (item.Status == 'Closed') {
									xhtm += '\t	  <td id="img"><img src="images/blindsopen48.png" title="' + $.t("Open Blinds") + '" onclick="SwitchLight(' + item.idx + ',\'On\',RefreshLights,' + item.Protected + ');" class="lcursor" height="48"></td>\n';
									xhtm += '\t	  <td id="img2"><img src="images/blinds48sel.png" title="' + $.t("Close Blinds") + '" onclick="SwitchLight(' + item.idx + ',\'Off\',RefreshLights,' + item.Protected + ');" class="lcursor" height="48"></td>\n';
								}
								else {
									xhtm += '\t	  <td id="img"><img src="images/blindsopen48sel.png" title="' + $.t("Open Blinds") + '" onclick="SwitchLight(' + item.idx + ',\'On\',RefreshLights,' + item.Protected + ');" class="lcursor" height="48"></td>\n';
									xhtm += '\t	  <td id="img2"><img src="images/blinds48.png" title="' + $.t("Close Blinds") + '" onclick="SwitchLight(' + item.idx + ',\'Off\',RefreshLights,' + item.Protected + ');" class="lcursor" height="48"></td>\n';
								}
							}
							else if (item.SwitchType == "Smoke Detector") {
								if (
									(item.Status == 'Panic') ||
									(item.Status == 'On')
								) {
									xhtm += '\t      <td id="img"><img src="images/smoke48on.png" title="' + $.t("Turn Alarm On") + '" onclick="SwitchLight(' + item.idx + ',\'On\',RefreshLights,' + item.Protected + ');" class="lcursor" height="48" width="48"></td>\n';
								}
								else {
									xhtm += '\t      <td id="img"><img src="images/smoke48off.png" title="' + $.t("Turn Alarm On") + '" onclick="SwitchLight(' + item.idx + ',\'On\',RefreshLights,' + item.Protected + ');" class="lcursor" height="48" width="48"></td>\n';
								}
							}
							else if (item.Type == "Security") {
								if (item.SubType.indexOf('remote') > 0) {
									if ((item.Status.indexOf('Arm') >= 0) || (item.Status.indexOf('Panic') >= 0)) {
										xhtm += '\t      <td id="img"><img src="images/remote48.png" title="' + $.t("Turn Alarm Off") + '" onclick="SwitchLight(' + item.idx + ',\'Off\',RefreshLights,' + item.Protected + ');" class="lcursor" height="48" width="48"></td>\n';
									}
									else {
										xhtm += '\t      <td id="img"><img src="images/remote48.png" title="' + $.t("Turn Alarm On") + '" onclick="ArmSystem(' + item.idx + ',\'On\',RefreshLights,' + item.Protected + ');" class="lcursor" height="48" width="48"></td>\n';
									}
								}
								else if (item.SubType == "X10 security") {
									if (item.Status.indexOf('Normal') >= 0) {
										xhtm += '\t      <td id="img"><img src="images/security48.png" title="' + $.t("Turn Alarm On") + '" onclick="SwitchLight(' + item.idx + ',\'' + ((item.Status == "Normal Delayed") ? "Alarm Delayed" : "Alarm") + '\',RefreshLights,' + item.Protected + ');" class="lcursor" height="48" width="48"></td>\n';
									}
									else {
										xhtm += '\t      <td id="img"><img src="images/Alarm48_On.png" title="' + $.t("Turn Alarm Off") + '" onclick="SwitchLight(' + item.idx + ',\'' + ((item.Status == "Alarm Delayed") ? "Normal Delayed" : "Normal") + '\',RefreshLights,' + item.Protected + ');" class="lcursor" height="48" width="48"></td>\n';
									}
								}
								else if (item.SubType == "X10 security motion") {
									if ((item.Status == "No Motion")) {
										xhtm += '\t      <td id="img"><img src="images/security48.png" title="' + $.t("Turn Alarm On") + '" onclick="SwitchLight(' + item.idx + ',\'Motion\',RefreshLights,' + item.Protected + ');" class="lcursor" height="48" width="48"></td>\n';
									}
									else {
										xhtm += '\t      <td id="img"><img src="images/Alarm48_On.png" title="' + $.t("Turn Alarm Off") + '" onclick="SwitchLight(' + item.idx + ',\'No Motion\',RefreshLights,' + item.Protected + ');" class="lcursor" height="48" width="48"></td>\n';
									}
								}
								else if ((item.Status.indexOf('Alarm') >= 0) || (item.Status.indexOf('Tamper') >= 0)) {
									xhtm += '\t      <td id="img"><img src="images/Alarm48_On.png" height="48" width="48"></td>\n';
								}
								else {
									if (item.SubType.indexOf('Meiantech') >= 0) {
										if ((item.Status.indexOf('Arm') >= 0) || (item.Status.indexOf('Panic') >= 0)) {
											xhtm += '\t      <td id="img"><img src="images/security48.png" title="' + $.t("Turn Alarm Off") + '" onclick="SwitchLight(' + item.idx + ',\'Off\',RefreshLights,' + item.Protected + ');" class="lcursor" height="48" width="48"></td>\n';
										}
										else {
											xhtm += '\t      <td id="img"><img src="images/security48.png" title="' + $.t("Turn Alarm On") + '" onclick="ArmSystemMeiantech(' + item.idx + ',\'On\',RefreshLights,' + item.Protected + ');" class="lcursor" height="48" width="48"></td>\n';
										}
									}
									else {
										if (item.SubType.indexOf('KeeLoq') >= 0) {
											xhtm += '\t      <td id="img"><img src="images/pushon48.png" title="' + $.t("Turn On") + '" onclick="SwitchLight(' + item.idx + ',\'On\',RefreshLights,' + item.Protected + ');" class="lcursor" height="48" width="48"></td>\n';
										}
										else {
											xhtm += '\t      <td id="img"><img src="images/security48.png" height="48" width="48"></td>\n';
										}
									}
								}
								bAddTimer = false;
							}
							else if (item.SwitchType == "Dimmer") {
								bIsDimmer = true;
								if (item.CustomImage == 0) item.Image = item.TypeImg;
								item.Image = item.Image.charAt(0).toUpperCase() + item.Image.slice(1);
								if (
									(item.Status == 'On') ||
									(item.Status == 'Chime') ||
									(item.Status == 'Group On') ||
									(item.Status.indexOf('Set ') == 0) ||
									(item.Status.indexOf('NightMode') == 0) ||
									(item.Status.indexOf('Disco ') == 0)
								) {
									if (item.SubType == "RGB") {
										xhtm += '\t      <td id="img"><img src="images/RGB48_On.png" onclick="ShowRGBWPopup(event, ' + item.idx + ', \'RefreshLights\',' + item.Protected + ',' + item.MaxDimLevel + ',' + item.LevelInt + ',' + item.Hue + ');" class="lcursor" height="48" width="48"></td>\n';
									}
									else if (item.SubType.indexOf("RGBW") >= 0) {
										xhtm += '\t      <td id="img"><img src="images/RGB48_On.png" onclick="ShowRGBWPopup(event, ' + item.idx + ', \'RefreshLights\',' + item.Protected + ',' + item.MaxDimLevel + ',' + item.LevelInt + ',' + item.Hue + ');" class="lcursor" height="48" width="48"></td>\n';
									}
									else {
										xhtm += '\t      <td id="img"><img src="images/' + item.Image + '48_On.png" title="' + $.t("Turn Off") + '" onclick="SwitchLight(' + item.idx + ',\'Off\',\'RefreshLights\',' + item.Protected + ');" class="lcursor" height="48" width="48"></td>\n';
									}
								}
								else {
									if (item.SubType == "RGB") {
										xhtm += '\t      <td id="img"><img src="images/RGB48_Off.png" onclick="ShowRGBWPopup(event, ' + item.idx + ',\'RefreshLights\',' + item.Protected + ',' + item.MaxDimLevel + ',' + item.LevelInt + ',' + item.Hue + ');" class="lcursor" height="48" width="48"></td>\n';
									}
									else if (item.SubType.indexOf("RGBW") >= 0) {
										xhtm += '\t      <td id="img"><img src="images/RGB48_Off.png" onclick="ShowRGBWPopup(event, ' + item.idx + ',\'RefreshLights\',' + item.Protected + ',' + item.MaxDimLevel + ',' + item.LevelInt + ',' + item.Hue + ');" class="lcursor" height="48" width="48"></td>\n';
									}
									else {
										xhtm += '\t      <td id="img"><img src="images/' + item.Image + '48_Off.png" title="' + $.t("Turn On") + '" onclick="SwitchLight(' + item.idx + ',\'On\',RefreshLights,' + item.Protected + ');" class="lcursor" height="48" width="48"></td>\n';
									}
								}
							}
							else if (item.SwitchType == "TPI") {
								var RO = (item.Unit < 64 || item.Unit > 95) ? true : false;
								bIsDimmer = true;
								if (item.Status == 'On') {
									xhtm += '\t      <td id="img"><img src="images/Fireplace48_On.png" title="' + $.t(RO ? "On" : "Turn Off") + (RO ? '"' : '" onclick="SwitchLight(' + item.idx + ',\'Off\',RefreshLights,' + item.Protected + ');" class="lcursor"') + ' height="48" width="48"></td>\n';
								}
								else {
									xhtm += '\t      <td id="img"><img src="images/Fireplace48_Off.png" title="' + $.t(RO ? "Off" : "Turn On") + (RO ? '"' : '" onclick="SwitchLight(' + item.idx + ',\'On\',RefreshLights,' + item.Protected + ');" class="lcursor"') + ' height="48" width="48"></td>\n';
								}
							}
							else if (item.SwitchType == "Dusk Sensor") {
								bAddTimer = false;
								if (item.Status == 'On') {
									xhtm += '\t      <td id="img"><img src="images/uvdark.png" title="' + $.t("Nighttime") + '" height="48" width="48"></td>\n';
								}
								else {
									xhtm += '\t      <td id="img"><img src="images/uvsunny.png" title="' + $.t("Daytime") + '" height="48" width="48"></td>\n';
								}
							}
							else if (item.SwitchType == "Motion Sensor") {
								if (
									(item.Status == 'On') ||
									(item.Status == 'Chime') ||
									(item.Status == 'Group On') ||
									(item.Status.indexOf('Set ') == 0)
								) {
									xhtm += '\t      <td id="img"><img src="images/motion48-on.png" height="48" width="48"></td>\n';
								}
								else {
									xhtm += '\t      <td id="img"><img src="images/motion48-off.png" height="48" width="48"></td>\n';
								}
								bAddTimer = false;
							}
							else if (item.SwitchType === "Selector") {
								if (item.Status === 'Off') {
									xhtm += '\t      <td id="img"><img src="images/' + item.Image + '48_Off.png" height="48" width="48"></td>\n';
								} else if (item.LevelOffHidden) {
									xhtm += '\t      <td id="img"><img src="images/' + item.Image + '48_On.png" height="48" width="48"></td>\n';
								} else {
									xhtm += '\t      <td id="img"><img src="images/' + item.Image + '48_On.png" title="' + $.t("Turn Off") + '" onclick="SwitchLight(' + item.idx + ',\'Off\',RefreshLights,' + item.Protected + ');" class="lcursor" height="48" width="48"></td>\n';
								}
							}
							else if (item.SubType.indexOf("Itho") == 0) {
								bAddTimer = false;
								xhtm += '\t      <td id="img"><img src="images/Fan48_On.png" height="48" width="48" class="lcursor" onclick="ShowIthoPopup(event, ' + item.idx + ', ShowLights, ' + item.Protected + ');"></td>\n';
							}
							else if (item.SubType.indexOf("Lucci") == 0) {
								bAddTimer = false;
								xhtm += '\t      <td id="img"><img src="images/Fan48_On.png" height="48" width="48" class="lcursor" onclick="ShowLucciPopup(event, ' + item.idx + ', ShowLights, ' + item.Protected + ');"></td>\n';
							}
							else {
								if (
									(item.Status == 'On') ||
									(item.Status == 'Chime') ||
									(item.Status == 'Group On') ||
									(item.Status.indexOf('Down') != -1) ||
									(item.Status.indexOf('Up') != -1) ||
									(item.Status.indexOf('Set ') == 0)
								) {
									if (item.Type == "Thermostat 3") {
										xhtm += '\t      <td id="img"><img src="images/' + item.Image + '48_On.png" onclick="ShowTherm3Popup(event, ' + item.idx + ',\'RefreshLights\',' + item.Protected + ');" class="lcursor" height="48" width="48"></td>\n';
									}
									else {
										xhtm += '\t      <td id="img"><img src="images/' + item.Image + '48_On.png" title="' + $.t("Turn Off") + '" onclick="SwitchLight(' + item.idx + ',\'Off\',RefreshLights,' + item.Protected + ');" class="lcursor" height="48" width="48"></td>\n';
									}
								}
								else {
									if (item.Type == "Thermostat 3") {
										xhtm += '\t      <td id="img"><img src="images/' + item.Image + '48_Off.png" onclick="ShowTherm3Popup(event, ' + item.idx + ',\'RefreshLights\',' + item.Protected + ');" class="lcursor" height="48" width="48"></td>\n';
									}
									else {
										xhtm += '\t      <td id="img"><img src="images/' + item.Image + '48_Off.png" title="' + $.t("Turn On") + '" onclick="SwitchLight(' + item.idx + ',\'On\',RefreshLights,' + item.Protected + ');" class="lcursor" height="48" width="48"></td>\n';
									}
								}
							}
							xhtm +=
								'\t      <td id="status">' + status + '</td>\n' +
								'\t      <td id="lastupdate">' + item.LastUpdate + '</td>\n' +
								'\t      <td id="type">' + item.Type + ', ' + item.SubType + ', ' + item.SwitchType;
							if (item.SwitchType == "Dimmer") {
								if ((item.SubType.indexOf("RGBW") >= 0) || (item.SubType == "RGB")) {
								}
								else {
									xhtm += '<br><br><div style="margin-left:60px;" class="dimslider" id="slider" data-idx="' + item.idx + '" data-type="norm" data-maxlevel="' + item.MaxDimLevel + '" data-isprotected="' + item.Protected + '" data-svalue="' + item.LevelInt + '"></div>';
								}
							}
							else if (item.SwitchType == "TPI") {
								xhtm += '<br><br><div style="margin-left:60px;" class="dimslider" id="slider" data-idx="' + item.idx + '" data-type="relay" data-maxlevel="' + item.MaxDimLevel + '" data-isprotected="' + item.Protected + '" data-svalue="' + item.LevelInt + '"';
								if (item.Unit < 64 || item.Unit > 95)
									xhtm += ' data-disabled="true"';
								xhtm += '></div>';
							}
							else if (item.SwitchType == "Blinds Percentage") {
								xhtm += '<br><div style="margin-left:108px; margin-top:7px;" class="dimslider dimsmall" id="slider" data-idx="' + item.idx + '" data-type="blinds" data-maxlevel="' + item.MaxDimLevel + '" data-isprotected="' + item.Protected + '" data-svalue="' + item.LevelInt + '"></div>';
							}
							else if (item.SwitchType == "Blinds Percentage Inverted") {
								xhtm += '<br><div style="margin-left:108px; margin-top:7px;" class="dimslider dimsmall" id="slider" data-idx="' + item.idx + '" data-type="blinds_inv" data-maxlevel="' + item.MaxDimLevel + '" data-isprotected="' + item.Protected + '" data-svalue="' + item.LevelInt + '"></div>';
							}
							else if (item.SwitchType == "Selector") {
								xhtm += '<br><div class="selectorlevels" style="margin-top: 0.4em;">';
								if (item.SelectorStyle === 0) {
									xhtm += '<div id="selector' + item.idx + '" data-idx="' + item.idx + '" data-isprotected="' + item.Protected + '" data-level="' + item.LevelInt + '" data-levelnames="' + escape(item.LevelNames) + '" data-selectorstyle="' + item.SelectorStyle + '" data-levelname="' + escape(GetLightStatusText(item)) + '" data-leveloffhidden="' + item.LevelOffHidden + '" data-levelactions="' + item.LevelActions + '">';
									var levelNames = item.LevelNames.split('|');
									$.each(levelNames, function (index, levelName) {
										if ((index === 0) && (item.LevelOffHidden)) {
											return;
										}
										xhtm += '<input type="radio" id="lSelector' + item.idx + 'Level' + index + '" name="selector' + item.idx + 'Level" value="' + (index * 10) + '"><label for="lSelector' + item.idx + 'Level' + index + '">' + levelName + '</label>';
									});
									xhtm += '</div>';
								} else if (item.SelectorStyle === 1) {
									xhtm += '<select id="selector' + item.idx + '" data-idx="' + item.idx + '" data-isprotected="' + item.Protected + '" data-level="' + item.LevelInt + '" data-levelnames="' + escape(item.LevelNames) + '" data-selectorstyle="' + item.SelectorStyle + '" data-levelname="' + escape(GetLightStatusText(item)) + '" data-leveloffhidden="' + item.LevelOffHidden + '" data-levelactions="' + item.LevelActions + '">';
									var levelNames = item.LevelNames.split('|');
									$.each(levelNames, function (index, levelName) {
										if ((index === 0) && (item.LevelOffHidden)) {
											return;
										}
										xhtm += '<option value="' + (index * 10) + '">' + levelName + '</option>';
									});
									xhtm += '</select>';
								}
								xhtm += '</div>';
							}
							xhtm += '</td>\n' +
								'\t      <td>';
							if (item.Favorite == 0) {
								xhtm +=
									'<img src="images/nofavorite.png" title="' + $.t('Add to Dashboard') + '" onclick="MakeFavorite(' + item.idx + ',1);" class="lcursor">&nbsp;&nbsp;&nbsp;&nbsp;';
							}
							else {
								xhtm +=
									'<img src="images/favorite.png" title="' + $.t('Remove from Dashboard') + '" onclick="MakeFavorite(' + item.idx + ',0);" class="lcursor">&nbsp;&nbsp;&nbsp;&nbsp;';
							}
							xhtm +=
								'<a class="btnsmall" onclick="ShowLightLog(' + item.idx + ',\'' + escape(item.Name) + '\', \'#lightcontent\', \'ShowLights\');" data-i18n="Log">Log</a> ';
							if (permissions.hasPermission("Admin")) {
								xhtm +=
									'<a class="btnsmall" onclick="EditLightDevice(' + item.idx + ',\'' + escape(item.Name) + '\',\'' + escape(item.Description) + '\', ' + '\'' + item.Type + '\', ' + item.SwitchTypeVal + ', ' + item.AddjValue + ', ' + item.AddjValue2 + ', ' + item.IsSubDevice + ', ' + item.CustomImage + ', \'' + item.SubType + '\', \'' + item.StrParam1 + '\', \'' + item.StrParam2 + '\',' + item.Protected + ',' + item.Unit + ');" data-i18n="Edit">Edit</a> ';
								if (bAddTimer == true) {
									if (item.Timers == "true") {
										xhtm += '<a class="btnsmall-sel" onclick="ShowTimers(' + item.idx + ',\'' + escape(item.Name) + '\',' + bIsDimmer + ',\'' + item.Type + '\'' + ', \'' + item.SubType + '\');" data-i18n="Timers">Timers</a> ';
									}
									else {
										xhtm += '<a class="btnsmall" onclick="ShowTimers(' + item.idx + ',\'' + escape(item.Name) + '\',' + bIsDimmer + ',\'' + item.Type + '\'' + ', \'' + item.SubType + '\');" data-i18n="Timers">Timers</a> ';
									}
								}
								if (item.SwitchType == "Smoke Detector") {
									if (
										(item.Status == 'Panic') ||
										(item.Status == 'On')
									) {
										xhtm += '<a id="resetbtn" class="btnsmall-sel" onclick="ResetSecurityStatus(' + item.idx + ',\'Normal\',ShowLights);" data-i18n="Reset">Reset</a> ';
									}
									else {
										xhtm += '<a id="resetbtn" class="btnsmall-dis" onclick="ResetSecurityStatus(' + item.idx + ',\'Normal\',ShowLights);" data-i18n="Reset">Reset</a> ';
									}
								}
								if (item.Notifications == "true")
									xhtm += '<a class="btnsmall-sel" onclick="ShowNotifications(' + item.idx + ',\'' + escape(item.Name) + '\', \'#lightcontent\', \'ShowLights\',' + bIsDimmer + ',\'' + item.Type + '\'' + ', \'' + item.SubType + '\');" data-i18n="Notifications">Notifications</a>';
								else
									xhtm += '<a class="btnsmall" onclick="ShowNotifications(' + item.idx + ',\'' + escape(item.Name) + '\', \'#lightcontent\', \'ShowLights\',' + bIsDimmer + ',\'' + item.Type + '\'' + ', \'' + item.SubType + '\');" data-i18n="Notifications">Notifications</a>';
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
			if (j == 0) {
				htmlcontent = '<h2>' + $.t('No Lights/Switches found or added in the system...') + '</h2>';
			}
			$('#modal').hide();
			$('#lightcontent').html(tophtm + htmlcontent);
			$('#lightcontent').i18n();
			if (bShowRoomplan == true) {
				$.each($.RoomPlans, function (i, item) {
					var option = $('<option />');
					option.attr('value', item.idx).text(item.name);
					$("#lightcontent #comboroom").append(option);
				});
				if (typeof window.myglobals.LastPlanSelected != 'undefined') {
					$("#lightcontent #comboroom").val(window.myglobals.LastPlanSelected);
				}
				$("#lightcontent #comboroom").change(function () {
					var idx = $("#lightcontent #comboroom option:selected").val();
					window.myglobals.LastPlanSelected = idx;
					ShowLights();
				});
			}

			if ($scope.config.AllowWidgetOrdering == true) {
				if (permissions.hasPermission("Admin")) {
					if (window.myglobals.ismobileint == false) {
						$("#lightcontent .span4").draggable({
							drag: function () {
								if (typeof $scope.mytimer != 'undefined') {
									$interval.cancel($scope.mytimer);
									$scope.mytimer = undefined;
								}
								$.devIdx = $(this).attr("id");
								$(this).css("z-index", 2);
							},
							revert: true
						});
						$("#lightcontent .span4").droppable({
							drop: function () {
								var myid = $(this).attr("id");
								$.devIdx.split(' ');
								var roomid = $("#lightcontent #comboroom option:selected").val();
								if (typeof roomid == 'undefined') {
									roomid = 0;
								}
								$.ajax({
									url: "json.htm?type=command&param=switchdeviceorder&idx1=" + myid + "&idx2=" + $.devIdx + "&roomid=" + roomid,
									async: false,
									dataType: 'json',
									success: function (data) {
										ShowLights();
									}
								});
							}
						});
					}
				}
			}
			$rootScope.RefreshTimeAndSun();

			//Create Dimmer Sliders
			$('#lightcontent .dimslider').slider({
				//Config
				range: "min",
				min: 1,
				max: 16,
				value: 5,

				//Slider Events
				create: function (event, ui) {
					$(this).slider("option", "max", $(this).data('maxlevel') + 1);
					$(this).slider("option", "type", $(this).data('type'));
					$(this).slider("option", "isprotected", $(this).data('isprotected'));
					$(this).slider("value", $(this).data('svalue') + 1);
					if ($(this).data('disabled'))
						$(this).slider("option", "disabled", true);
				},
				slide: function (event, ui) { //When the slider is sliding
					clearInterval($.setDimValue);
					var maxValue = $(this).slider("option", "max");
					var dtype = $(this).slider("option", "type");
					var isProtected = $(this).slider("option", "isprotected");
					var fPercentage = parseInt((100.0 / (maxValue - 1)) * ((ui.value - 1)));
					var idx = $(this).data('idx');
					id = "#lightcontent #" + idx;
					var obj = $(id);
					if (typeof obj != 'undefined') {
						var img = "";
						var imgname = $('#' + idx + ' .lcursor').prop('src');
						imgname = imgname.substring(imgname.lastIndexOf("/") + 1, imgname.lastIndexOf("_O") + 2);
						if (dtype == "relay")
							imgname = "Fireplace48_O"
						var bigtext;
						if (fPercentage == 0) {
							img = '<img src="images/' + imgname + 'ff.png" title="' + $.t("Turn On") + '" onclick="SwitchLight(' + idx + ',\'On\',RefreshLights,' + isProtected + ');" class="lcursor" height="48" width="48">';
							if (dtype == "blinds") {
								bigtext = "Open";
							}
							else if (dtype == "blinds_inv") {
								bigtext = "Closed";
							}
							else {
								bigtext = "Off";
							}
						}
						else {
							img = '<img src="images/' + imgname + 'n.png" title="' + $.t("Turn Off") + '" onclick="SwitchLight(' + idx + ',\'Off\',RefreshLights,' + isProtected + ');" class="lcursor" height="48" width="48">';
							bigtext = fPercentage + " %";
						}
						if ((dtype != "blinds") && (dtype != "blinds_inv")) {
							if ($(id + " #img").html() != img) {
								$(id + " #img").html(img);
							}
						}
						if ($(id + " #bigtext").html() != bigtext) {
							$(id + " #bigtext").html(bigtext);
						}
					}
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
			$scope.ResizeDimSliders();

			//Create Selector buttonset
			$('#lightcontent .selectorlevels div').buttonset({
				//Selector selectmenu events
				create: function (event, ui) {
					var div$ = $(this),
						idx = div$.data('idx'),
						type = div$.data('type'),
						isprotected = div$.data('isprotected'),
						disabled = div$.data('disabled'),
						level = div$.data('level'),
						levelname = div$.data('levelname');
					if (disabled === true) {
						div$.buttonset("disable");
					}
					div$.find('input[value="' + level + '"]').prop("checked", true);

					div$.find('input').click(function (event) {
						var target$ = $(event.target);
						level = parseInt(target$.val(), 10);
						levelname = div$.find('label[for="' + target$.attr('id') + '"]').text();
						// Send command
						SwitchSelectorLevel(idx, unescape(levelname), level, RefreshLights, isprotected);
						// Synchronize buttons and div attributes
						div$.data('level', level);
						div$.data('levelname', levelname);
					});

					$('#lightcontent #' + idx + " #bigtext").html(unescape(levelname));
				}
			});

			//Create Selector selectmenu
			$('#lightcontent .selectorlevels select').selectmenu({
				//Config
				width: '75%',
				value: 0,
				//Selector selectmenu events
				create: function (event, ui) {
					var select$ = $(this),
						idx = select$.data('idx'),
						isprotected = select$.data('isprotected'),
						disabled = select$.data('disabled'),
						level = select$.data('level'),
						levelname = select$.data('levelname');
					select$.selectmenu("option", "idx", idx);
					select$.selectmenu("option", "isprotected", isprotected);
					select$.selectmenu("option", "disabled", disabled === true);
					select$.selectmenu("menuWidget").addClass('selectorlevels-menu');
					select$.val(level);

					$('#lightcontent #' + idx + " #bigtext").html(unescape(levelname));
				},
				change: function (event, ui) { //When the user selects an option
					var select$ = $(this),
						idx = select$.selectmenu("option", "idx"),
						level = select$.selectmenu().val(),
						levelname = select$.find('option[value="' + level + '"]').text(),
						isprotected = select$.selectmenu("option", "isprotected");
					// Send command
					SwitchSelectorLevel(idx, unescape(levelname), level, RefreshLights, isprotected);
					// Synchronize buttons and select attributes
					select$.data('level', level);
					select$.data('levelname', levelname);
				}
			}).selectmenu('refresh');

			$scope.mytimer = $interval(function () {
				RefreshLights();
			}, 10000);
			return false;
		}

		$scope.ResizeDimSliders = function () {
			var nobj = $("#lightcontent #name");
			if (typeof nobj == 'undefined') {
				return;
			}
			var width = nobj.width() - 50;
			$("#lightcontent .dimslider").width(width);
			$("#lightcontent .dimsmall").width(width - 48);
		}

		$.strPad = function (i, l, s) {
			var o = i.toString();
			if (!s) { s = '0'; }
			while (o.length < l) {
				o = s + o;
			}
			return o;
		};

		UpdateAddManualDialog = function () {
			var lighttype = $("#dialog-addmanuallightdevice #lighttable #combolighttype option:selected").val();
			var bIsARCType = ((lighttype < 20) || (lighttype == 101));
			var bIsType5 = 0;

			var tothousecodes = 1;
			var totunits = 1;
			if ((lighttype == 0) || (lighttype == 1) || (lighttype == 3) || (lighttype == 101)) {
				tothousecodes = 16;
				totunits = 16;
			}
			else if ((lighttype == 2) || (lighttype == 5)) {
				tothousecodes = 16;
				totunits = 64;
			}
			else if (lighttype == 4) {
				tothousecodes = 3;
				totunits = 4;
			}
			else if (lighttype == 6) {
				tothousecodes = 4;
				totunits = 4;
			}
			else if (lighttype == 68) {
				//Do nothing. GPIO uses a custom form
			}
			else if (lighttype == 69) {
				//Do nothing. SysfsGpio uses a custom form
			}
			else if (lighttype == 7) {
				tothousecodes = 16;
				totunits = 8;
			}
			else if (lighttype == 8) {
				tothousecodes = 16;
				totunits = 4;
			}
			else if (lighttype == 9) {
				tothousecodes = 16;
				totunits = 10;
			}
			else if (lighttype == 10) {
				tothousecodes = 4;
				totunits = 4;
			}
			else if (lighttype == 11) {
				tothousecodes = 16;
				totunits = 64;
			}
			else if (lighttype == 106) {
				//Blyss
				tothousecodes = 16;
				totunits = 5;
			}
			else if (lighttype == 70) {
				//EnOcean
				tothousecodes = 128;
				totunits = 2;
			}
			else if (lighttype == 50) {
				//LightwaveRF
				bIsType5 = 1;
				totunits = 16;
			}
			else if (lighttype == 65) {
				//IT (Intertek,FA500,PROmax...)
				bIsType5 = 1;
				totunits = 4;
			}
			else if ((lighttype == 55) || (lighttype == 57)) {
				//Livolo
				bIsType5 = 1;
			}
			else if (lighttype == 100) {
				//Chime/ByronSX
				bIsType5 = 1;
				totunits = 4;
			}
			else if ((lighttype == 102) || (lighttype == 107)) {
				//RFY/RFY2
				bIsType5 = 1;
				totunits = 16;
			}
			else if (lighttype == 103) {
				//Meiantech
				bIsType5 = 1;
				totunits = 0;
			}
			else if (lighttype == 105) {
				//ASA
				bIsType5 = 1;
				totunits = 16;
			}
			else if ((lighttype >= 200) && (lighttype < 300)) {
				//Blinds
			}
			else if (lighttype == 302) {
				//Home Confort
				tothousecodes = 4;
				totunits = 4;
			}
			else if ((lighttype == 400) || (lighttype == 401)) {
				//Openwebnet Bus Blinds/Lights
				totrooms = 11;//area, from 0 to 9 if physical configuration, 0 to 10 if virtual configuration
				totpointofloads = 16;//point of load, from 0 to 9 if physical configuration, 1 to 15 if virtual configuration
				totbus = 10;//maximum 10 local buses
			}
			else if (lighttype == 402) {
				//Openwebnet Bus Auxiliary
				totrooms = 10;
			}
			else if ((lighttype == 403) || (lighttype == 404)) {
				//Openwebnet Zigbee Blinds/Lights
				totunits = 3;//unit number is the button number on the switch (e.g. light1/light2 on a light switch)
			}
			else if (lighttype == 405) {
				//Openwebnet Bus Dry Contact
				totrooms = 200;
			}
			else if (lighttype == 406) {
				//Openwebnet Bus IR Detection
				totrooms = 10;
				totpointofloads = 10
			}

			$("#dialog-addmanuallightdevice #he105params").hide();
			$("#dialog-addmanuallightdevice #blindsparams").hide();
			$("#dialog-addmanuallightdevice #lightingparams_enocean").hide();
			$("#dialog-addmanuallightdevice #lightingparams_gpio").hide();
			$("#dialog-addmanuallightdevice #lightingparams_sysfsgpio").hide();
			$("#dialog-addmanuallightdevice #homeconfortparams").hide();
			$("#dialog-addmanuallightdevice #fanparams").hide();
			$("#dialog-addmanuallightdevice #openwebnetparamsBus").hide();
			$("#dialog-addmanuallightdevice #openwebnetparamsAUX").hide();
			$("#dialog-addmanuallightdevice #openwebnetparamsZigbee").hide();
			$("#dialog-addmanuallightdevice #openwebnetparamsDryContact").hide();
			$("#dialog-addmanuallightdevice #openwebnetparamsIRdetec").hide();

			if (lighttype == 104) {
				//HE105
				$("#dialog-addmanuallightdevice #lighting1params").hide();
				$("#dialog-addmanuallightdevice #lighting2params").hide();
				$("#dialog-addmanuallightdevice #lighting3params").hide();
				$("#dialog-addmanuallightdevice #he105params").show();
			}
			else if (lighttype == 303) {
				$("#dialog-addmanuallightdevice #lighting1params").hide();
				$("#dialog-addmanuallightdevice #lighting2params").show();
				$("#dialog-addmanuallightdevice #lighting3params").hide();
			}
			else if (lighttype == 106) {
				//Blyss
				$('#dialog-addmanuallightdevice #lightparams3 #combogroupcode >option').remove();
				$('#dialog-addmanuallightdevice #lightparams3 #combounitcode >option').remove();
				for (ii = 0; ii < tothousecodes; ii++) {
					$('#dialog-addmanuallightdevice #lightparams3 #combogroupcode').append($('<option></option>').val(41 + ii).html(String.fromCharCode(65 + ii)));
				}
				for (ii = 1; ii < totunits + 1; ii++) {
					$('#dialog-addmanuallightdevice #lightparams3 #combounitcode').append($('<option></option>').val(ii).html(ii));
				}
				$("#dialog-addmanuallightdevice #lighting1params").hide();
				$("#dialog-addmanuallightdevice #lighting2params").hide();
				$("#dialog-addmanuallightdevice #lighting3params").show();
			}
			else if (lighttype == 70) {
				//EnOcean
				$("#dialog-addmanuallightdevice #lightparams_enocean #comboid  >option").remove();
				$("#dialog-addmanuallightdevice #lightparams_enocean #combounitcode  >option").remove();
				for (ii = 1; ii < tothousecodes + 1; ii++) {
					$('#dialog-addmanuallightdevice #lightparams_enocean #comboid').append($('<option></option>').val(ii).html(ii));
				}
				for (ii = 1; ii < totunits + 1; ii++) {
					$('#dialog-addmanuallightdevice #lightparams_enocean #combounitcode').append($('<option></option>').val(ii).html(ii));
				}
				$("#dialog-addmanuallightdevice #lighting1params").hide();
				$("#dialog-addmanuallightdevice #lighting2params").hide();
				$("#dialog-addmanuallightdevice #lighting3params").hide();
				$("#dialog-addmanuallightdevice #lightingparams_enocean").show();
			}
			else if (lighttype == 68) {
				//GPIO
				$("#dialog-addmanuallightdevice #lightparams_enocean #comboid  >option").remove();
				$("#dialog-addmanuallightdevice #lightparams_enocean #combounitcode  >option").remove();
				$("#dialog-addmanuallightdevice #lighting1params").hide();
				$("#dialog-addmanuallightdevice #lighting2params").hide();
				$("#dialog-addmanuallightdevice #lighting3params").hide();
				$("#dialog-addmanuallightdevice #lightingparams_gpio").show();
			}
			else if (lighttype == 69) {
				//SysfsGpio
				$("#dialog-addmanuallightdevice #lightparams_enocean #comboid  >option").remove();
				$("#dialog-addmanuallightdevice #lightparams_enocean #combounitcode  >option").remove();
				$("#dialog-addmanuallightdevice #lighting1params").hide();
				$("#dialog-addmanuallightdevice #lighting2params").show();
				$("#dialog-addmanuallightdevice #lighting3params").hide();
				$("#dialog-addmanuallightdevice #lighting2paramsUnitCode").hide()
				$("#dialog-addmanuallightdevice #lightingparams_sysfsgpio").show();
			}
			else if ((lighttype >= 200) && (lighttype < 300)) {
				//Blinds
				$("#dialog-addmanuallightdevice #blindsparams").show();
				var bShow1 = (lighttype == 205) || (lighttype == 206) || (lighttype == 207) || (lighttype == 210) || (lighttype == 211);
				var bShow4 = (lighttype == 206) || (lighttype == 207) || (lighttype == 209);
				var bShowUnit = (lighttype == 206) || (lighttype == 207) || (lighttype == 208) || (lighttype == 209) || (lighttype == 212) || (lighttype == 213);
				if (bShow1)
					$('#dialog-addmanuallightdevice #blindsparams #combocmd1').show();
				else {
					$('#dialog-addmanuallightdevice #blindsparams #combocmd1').hide();
					$('#dialog-addmanuallightdevice #blindsparams #combocmd1').val(0);
				}
				if (bShow4)
					$('#dialog-addmanuallightdevice #blindsparams #combocmd4').show();
				else {
					$('#dialog-addmanuallightdevice #blindsparams #combocmd4').hide();
					$('#dialog-addmanuallightdevice #blindsparams #combocmd4').val(0);
				}
				if (bShowUnit)
					$('#dialog-addmanuallightdevice #blindparamsUnitCode').show();
				else
					$('#dialog-addmanuallightdevice #blindparamsUnitCode').hide();

				$("#dialog-addmanuallightdevice #lighting1params").hide();
				$("#dialog-addmanuallightdevice #lighting2params").hide();
				$("#dialog-addmanuallightdevice #lighting3params").hide();
			}
			else if (lighttype == 302) {
				//Home Confort
				$("#dialog-addmanuallightdevice #lighting1params").hide();
				$("#dialog-addmanuallightdevice #lighting2params").hide();
				$("#dialog-addmanuallightdevice #lighting3params").hide();
				$("#dialog-addmanuallightdevice #homeconfortparams").show();
			}
			else if (lighttype == 304) {
				//Fan (Itho)
				$("#dialog-addmanuallightdevice #lighting1params").hide();
				$("#dialog-addmanuallightdevice #lighting2params").hide();
				$("#dialog-addmanuallightdevice #lighting3params").hide();
				$("#dialog-addmanuallightdevice #fanparams").show();
			}
			else if (lighttype == 305) {
				//Fan (Lucci Air)
				$("#dialog-addmanuallightdevice #lighting1params").hide();
				$("#dialog-addmanuallightdevice #lighting2params").hide();
				$("#dialog-addmanuallightdevice #lighting3params").hide();
				$("#dialog-addmanuallightdevice #fanparams").show();
			}
			else if ((lighttype == 400) || (lighttype == 401)) {
				//Openwebnet Bus Blinds/Light
				$("#dialog-addmanuallightdevice #openwebnetparamsBus #combocmd1  >option").remove();
				for (ii = 1; ii < totrooms; ii++) {
					$('#dialog-addmanuallightdevice #openwebnetparamsBus #combocmd1').append($('<option></option>').val(ii).html(ii));
				}
				$("#dialog-addmanuallightdevice #openwebnetparamsBus #combocmd2  >option").remove();
				for (ii = 1; ii < totpointofloads; ii++) {
					$('#dialog-addmanuallightdevice #openwebnetparamsBus #combocmd2').append($('<option></option>').val(ii).html(ii));
				}
				$("#dialog-addmanuallightdevice #openwebnetparamsBus #combocmd3  >option").remove();
				$("#dialog-addmanuallightdevice #openwebnetparamsBus #combocmd3").append($('<option></option>').val(0).html("local bus"));
				for (ii = 1; ii < totbus; ii++) {
					$("#dialog-addmanuallightdevice #openwebnetparamsBus #combocmd3").append($('<option></option>').val(ii).html(ii));
				}

				$("#dialog-addmanuallightdevice #lighting1params").hide();
				$("#dialog-addmanuallightdevice #lighting2params").hide();
				$("#dialog-addmanuallightdevice #lighting3params").hide();
				$("#dialog-addmanuallightdevice #openwebnetparamsBus").show();
			}
			else if (lighttype == 402) {
				//Openwebnet Bus Auxiliary
				$("#dialog-addmanuallightdevice #openwebnetparamsAUX #combocmd1  >option").remove();
				for (ii = 1; ii < totrooms; ii++) {
					$('#dialog-addmanuallightdevice #openwebnetparamsAUX #combocmd1').append($('<option></option>').val(ii).html(ii));
				}
				$("#dialog-addmanuallightdevice #lighting1params").hide();
				$("#dialog-addmanuallightdevice #lighting2params").hide();
				$("#dialog-addmanuallightdevice #lighting3params").hide();
				$("#dialog-addmanuallightdevice #openwebnetparamsAUX").show();
			}
			else if ((lighttype == 403) || (lighttype == 404)) {
				//Openwebnet Zigbee Blinds/Light
				$("#dialog-addmanuallightdevice #lighting1params").hide();
				$("#dialog-addmanuallightdevice #lighting2params").hide();
				$("#dialog-addmanuallightdevice #lighting3params").hide();
				$("#dialog-addmanuallightdevice #openwebnetparamsBus").hide();
				$("#dialog-addmanuallightdevice #openwebnetparamsAUX").hide();
				$("#dialog-addmanuallightdevice #openwebnetparamsZigbee").show();
				$("#dialog-addmanuallightdevice #openwebnetparamsZigbee #combocmd2  >option").remove();
				for (ii = 1; ii < totunits + 1; ii++) {
					$('#dialog-addmanuallightdevice #openwebnetparamsZigbee #combocmd2').append($('<option></option>').val(ii).html(ii));
				}
			}
			else if (lighttype == 405) {
				//Openwebnet Dry Contact
				$("#dialog-addmanuallightdevice #openwebnetparamsDryContact #combocmd1  >option").remove();
				for (ii = 1; ii < totrooms; ii++) {
					$('#dialog-addmanuallightdevice #openwebnetparamsDryContact #combocmd1').append($('<option></option>').val(ii).html(ii));
				}

				$("#dialog-addmanuallightdevice #lighting1params").hide();
				$("#dialog-addmanuallightdevice #lighting2params").hide();
				$("#dialog-addmanuallightdevice #lighting3params").hide();
				$("#dialog-addmanuallightdevice #openwebnetparams").hide();
				$("#dialog-addmanuallightdevice #openwebnetparamsDryContact").show();
			}
			else if (lighttype == 406) {
				//Openwebnet IR Detection
				$("#dialog-addmanuallightdevice #openwebnetparamsIRdetec #combocmd1  >option").remove();
				for (ii = 1; ii < totrooms; ii++) {
					$('#dialog-addmanuallightdevice #openwebnetparamsIRdetec #combocmd1').append($('<option></option>').val(ii).html(ii));
				}
				$("#dialog-addmanuallightdevice #openwebnetparamsIRdetec #combocmd2  >option").remove();
				for (ii = 1; ii < totpointofloads; ii++) {
					$('#dialog-addmanuallightdevice #openwebnetparamsIRdetec #combocmd2').append($('<option></option>').val(ii).html(ii));
				}
				$("#dialog-addmanuallightdevice #lighting1params").hide();
				$("#dialog-addmanuallightdevice #lighting2params").hide();
				$("#dialog-addmanuallightdevice #lighting3params").hide();
				$("#dialog-addmanuallightdevice #openwebnetparamsIRdetec").show();
			}
			else if (bIsARCType == 1) {
				$('#dialog-addmanuallightdevice #lightparams1 #combohousecode >option').remove();
				$('#dialog-addmanuallightdevice #lightparams1 #combounitcode >option').remove();
				for (ii = 0; ii < tothousecodes; ii++) {
					$('#dialog-addmanuallightdevice #lightparams1 #combohousecode').append($('<option></option>').val(65 + ii).html(String.fromCharCode(65 + ii)));
				}
				for (ii = 1; ii < totunits + 1; ii++) {
					$('#dialog-addmanuallightdevice #lightparams1 #combounitcode').append($('<option></option>').val(ii).html(ii));
				}
				$("#dialog-addmanuallightdevice #lighting1params").show();
				$("#dialog-addmanuallightdevice #lighting2params").hide();
				$("#dialog-addmanuallightdevice #lighting3params").hide();
			}
			else {
				if (lighttype == 103) {
					$("#dialog-addmanuallightdevice #lighting2paramsUnitCode").hide();
				}
				else {
					$("#dialog-addmanuallightdevice #lighting2paramsUnitCode").show();
				}
				$("#dialog-addmanuallightdevice #lighting2params #combocmd2").show();
				if (bIsType5 == 0) {
					$("#dialog-addmanuallightdevice #lighting2params #combocmd1").show();
				}
				else {
					$("#dialog-addmanuallightdevice #lighting2params #combocmd1").hide();
					if ((lighttype == 55) || (lighttype == 57) || (lighttype == 65) || (lighttype == 100)) {
						$("#dialog-addmanuallightdevice #lighting2params #combocmd2").hide();
						if ((lighttype != 65) && (lighttype != 100)) {
							$("#dialog-addmanuallightdevice #lighting2paramsUnitCode").hide();
						}
					}
				}
				$("#dialog-addmanuallightdevice #lighting1params").hide();
				$("#dialog-addmanuallightdevice #lighting2params").show();
				$("#dialog-addmanuallightdevice #lighting3params").hide();
			}
		}

		GetManualLightSettings = function (isTest) {
			var mParams = "";
			var hwdID = $("#dialog-addmanuallightdevice #lighttable #combohardware option:selected").val();
			if (typeof hwdID == 'undefined') {
				ShowNotify($.t('No Hardware Device selected!'), 2500, true);
				return "";
			}
			mParams += "&hwdid=" + hwdID;

			var name = $("#dialog-addmanuallightdevice #devicename");
			if ((name.val() == "") || (!checkLength(name, 2, 100))) {
				if (!isTest) {
					ShowNotify($.t('Invalid Name!'), 2500, true);
					return "";
				}
			}
			mParams += "&name=" + encodeURIComponent(name.val());

			var description = $("#dialog-addmanuallightdevice #devicedescription");
			mParams += "&description=" + encodeURIComponent(description.val());

			mParams += "&switchtype=" + $("#dialog-addmanuallightdevice #lighttable #comboswitchtype option:selected").val();
			var lighttype = $("#dialog-addmanuallightdevice #lighttable #combolighttype option:selected").val();
			mParams += "&lighttype=" + lighttype;
			if (lighttype == 106) {
				//Blyss
				mParams += "&groupcode=" + $("#dialog-addmanuallightdevice #lightparams3 #combogroupcode option:selected").val();
				mParams += "&unitcode=" + $("#dialog-addmanuallightdevice #lightparams3 #combounitcode option:selected").val();
				ID =
					$("#dialog-addmanuallightdevice #lightparams3 #combocmd1 option:selected").text() +
					$("#dialog-addmanuallightdevice #lightparams3 #combocmd2 option:selected").text();
				mParams += "&id=" + ID;
			}
			else if (lighttype == 70) {
				//EnOcean
				//mParams+="&groupcode="+$("#dialog-addmanuallightdevice #lightingparams_enocean #comboid option:selected").val();
				//mParams+="&unitcode="+$("#dialog-addmanuallightdevice #lightingparams_enocean #combounitcode option:selected").val();
				mParams += "&groupcode=" + $("#dialog-addmanuallightdevice #lightingparams_enocean #combounitcode option:selected").val();
				mParams += "&unitcode=" + $("#dialog-addmanuallightdevice #lightingparams_enocean #comboid option:selected").val();
				ID = "EnOcean";
				mParams += "&id=" + ID;
			}
			else if (lighttype == 68) {
				//GPIO
                mParams += "&id=GPIO&unitcode=" + $("#dialog-addmanuallightdevice #lightingparams_gpio #combogpio option:selected").val();
			}
			else if (lighttype == 69) {
				//SysfsGpio
                ID =
                    $("#dialog-addmanuallightdevice #lightparams2 #combocmd1 option:selected").text() +
                    $("#dialog-addmanuallightdevice #lightparams2 #combocmd2 option:selected").text() +
                    $("#dialog-addmanuallightdevice #lightparams2 #combocmd3 option:selected").text() +
                    $("#dialog-addmanuallightdevice #lightparams2 #combocmd4 option:selected").text();
                mParams += "&id=" + ID + "&unitcode=" + $("#dialog-addmanuallightdevice #lightingparams_sysfsgpio #combosysfsgpio option:selected").val();
            }
			else if ((lighttype < 20) || (lighttype == 101)) {
				mParams += "&housecode=" + $("#dialog-addmanuallightdevice #lightparams1 #combohousecode option:selected").val();
				mParams += "&unitcode=" + $("#dialog-addmanuallightdevice #lightparams1 #combounitcode option:selected").val();
			}
			else if (lighttype == 104) {
				mParams += "&unitcode=" + $("#dialog-addmanuallightdevice #he105params #combounitcode option:selected").text();
			}
			else if ((lighttype >= 200) && (lighttype < 300)) {
				//Blinds
				ID =
					$("#dialog-addmanuallightdevice #blindsparams #combocmd1 option:selected").text() +
					$("#dialog-addmanuallightdevice #blindsparams #combocmd2 option:selected").text() +
					$("#dialog-addmanuallightdevice #blindsparams #combocmd3 option:selected").text() +
					"0" + $("#dialog-addmanuallightdevice #blindsparams #combocmd4 option:selected").text();
				mParams += "&id=" + ID;
				mParams += "&unitcode=" + $("#dialog-addmanuallightdevice #blindsparams #combounitcode option:selected").val();
			}
			else if (lighttype == 302) {
				//Home Confort
				ID =
					$("#dialog-addmanuallightdevice #homeconfortparams #combocmd1 option:selected").text() +
					$("#dialog-addmanuallightdevice #homeconfortparams #combocmd2 option:selected").text() +
					$("#dialog-addmanuallightdevice #homeconfortparams #combocmd3 option:selected").text();
				mParams += "&id=" + ID;
				mParams += "&housecode=" + $("#dialog-addmanuallightdevice #homeconfortparams #combohousecode option:selected").val();
				mParams += "&unitcode=" + $("#dialog-addmanuallightdevice #homeconfortparams #combounitcode option:selected").val();
			}
			else if (lighttype == 304) {
				//Fan (Itho)
				ID =
					$("#dialog-addmanuallightdevice #fanparams #combocmd1 option:selected").text() +
					$("#dialog-addmanuallightdevice #fanparams #combocmd2 option:selected").text() +
					$("#dialog-addmanuallightdevice #fanparams #combocmd3 option:selected").text();
				mParams += "&id=" + ID;
			}
			else if (lighttype == 305) {
				//Fan (Lucci Air)
				ID =
					$("#dialog-addmanuallightdevice #fanparams #combocmd1 option:selected").text() +
					$("#dialog-addmanuallightdevice #fanparams #combocmd2 option:selected").text() +
					$("#dialog-addmanuallightdevice #fanparams #combocmd3 option:selected").text();
				mParams += "&id=" + ID;
			}
			else if (lighttype == 400) {
				//OpenWebNet Bus Blinds
				var appID = parseInt($("#dialog-addmanuallightdevice #openwebnetparamsBus #combocmd1 option:selected").val() +
					$("#dialog-addmanuallightdevice #openwebnetparamsBus #combocmd2 option:selected").val());
				var ID = ("0002" + ("0000" + appID.toString(16)).slice(-4)); // WHO_AUTOMATION
				var unitcode = $("#dialog-addmanuallightdevice #openwebnetparamsBus #combocmd3 option:selected").val();//TODO : handle bus id (interface) in hardware
				mParams += "&id=" + ID.toUpperCase() + "&unitcode=" + unitcode;
			}
			else if (lighttype == 401) {
				//OpenWebNet Bus Lights
				var appID = parseInt($("#dialog-addmanuallightdevice #openwebnetparamsBus #combocmd1 option:selected").val() +
					$("#dialog-addmanuallightdevice #openwebnetparamsBus #combocmd2 option:selected").val());
				var ID = ("0001" + ("0000" + appID.toString(16)).slice(-4)); // WHO_LIGHTING
				var unitcode = $("#dialog-addmanuallightdevice #openwebnetparamsBus #combocmd3 option:selected").val();//TODO : handle bus id (interface) in hardware
				mParams += "&id=" + ID.toUpperCase() + "&unitcode=" + unitcode;
			}
			else if (lighttype == 402) {
				//OpenWebNet Bus Auxiliary
				var appID = parseInt($("#dialog-addmanuallightdevice #openwebnetparamsAUX #combocmd1 option:selected").val());
				var ID = ("0009" + ("0000" + appID.toString(16)).slice(-4)); // WHO_AUXILIARY
				var unitcode = "0";
				mParams += "&id=" + ID.toUpperCase() + "&unitcode=" + unitcode;
			}
			else if (lighttype == 403) {
				//OpenWebNet Zigbee Blinds
				var ID = $("#dialog-addmanuallightdevice #openwebnetparamsZigbee #inputcmd1").val();
				if (parseInt(ID) <= 0 || parseInt(ID) >= 0xFFFFFFFF) {
					ShowNotify($.t('Zigbee id is incorrect!'), 2500, true);
					return "";
				}
				var unitcode = $("#dialog-addmanuallightdevice #openwebnetparamsZigbee #combocmd2 option:selected").val();
				mParams += "&id=" + ID + "&unitcode=" + unitcode;
			}
			else if (lighttype == 404) {
				//OpenWebNet Zigbee Light
				var ID = $("#dialog-addmanuallightdevice #openwebnetparamsZigbee #inputcmd1").val();
				if (parseInt(ID) <= 0 || parseInt(ID) >= 0xFFFFFFFF) {
					ShowNotify($.t('Zigbee id is incorrect!'), 2500, true);
					return "";
				}
				var unitcode = $("#dialog-addmanuallightdevice #openwebnetparamsZigbee #combocmd2 option:selected").val();
				mParams += "&id=" + ID + "&unitcode=" + unitcode;
			}
			else if (lighttype == 405) {
				//OpenWebNet Dry Contact
				var appID = parseInt($("#dialog-addmanuallightdevice #openwebnetparamsDryContact #combocmd1 option:selected").val());
				var ID = ("0019" + ("0000" + appID.toString(16)).slice(-4)); // WHO_DRY_CONTACT_IR_DETECTION (25 = 0x19)
				var unitcode = "0";
				mParams += "&id=" + ID.toUpperCase() + "&unitcode=" + unitcode;
			}
			else if (lighttype == 406) {
				//OpenWebNet IR Detection
				var appID = parseInt($("#dialog-addmanuallightdevice #openwebnetparamsIRdetec #combocmd1 option:selected").val() +
					$("#dialog-addmanuallightdevice #openwebnetparamsIRdetec #combocmd2 option:selected").val());
				var ID = ("0019" + ("0000" + appID.toString(16)).slice(-4)); // WHO_DRY_CONTACT_IR_DETECTION (25 = 0x19)
				var unitcode = "0";
				mParams += "&id=" + ID.toUpperCase() + "&unitcode=" + unitcode;
			}
			else {
				//AC
				var ID = "";
				var bIsType5 = 0;
				if (
					(lighttype == 50) ||
					(lighttype == 55) ||
					(lighttype == 57) ||
					(lighttype == 65) ||
					(lighttype == 100) ||
					(lighttype == 102) ||
					(lighttype == 103) ||
					(lighttype == 105) ||
					(lighttype == 107)
				) {
					bIsType5 = 1;
				}
				if (bIsType5 == 0) {
					ID =
						$("#dialog-addmanuallightdevice #lightparams2 #combocmd1 option:selected").text() +
						$("#dialog-addmanuallightdevice #lightparams2 #combocmd2 option:selected").text() +
						$("#dialog-addmanuallightdevice #lightparams2 #combocmd3 option:selected").text() +
						$("#dialog-addmanuallightdevice #lightparams2 #combocmd4 option:selected").text();
				}
				else {
					if ((lighttype != 55) && (lighttype != 57) && (lighttype != 100)) {
						ID =
							$("#dialog-addmanuallightdevice #lightparams2 #combocmd2 option:selected").text() +
							$("#dialog-addmanuallightdevice #lightparams2 #combocmd3 option:selected").text() +
							$("#dialog-addmanuallightdevice #lightparams2 #combocmd4 option:selected").text();
					}
					else {
						ID =
							$("#dialog-addmanuallightdevice #lightparams2 #combocmd3 option:selected").text() +
							$("#dialog-addmanuallightdevice #lightparams2 #combocmd4 option:selected").text();
					}
				}
				if (ID == "") {
					ShowNotify($.t('Invalid Switch ID!'), 2500, true);
					return "";
				}
				mParams += "&id=" + ID;
				mParams += "&unitcode=" + $("#dialog-addmanuallightdevice #lightparams2 #combounitcode option:selected").val();
			}

			var bIsSubDevice = $("#dialog-addmanuallightdevice #howtable #how_2").is(":checked");
			var MainDeviceIdx = "";
			if (bIsSubDevice) {
				var MainDeviceIdx = $("#dialog-addmanuallightdevice #combosubdevice option:selected").val();
				if (typeof MainDeviceIdx == 'undefined') {
					bootbox.alert($.t('No Main Device Selected!'));
					return "";
				}
			}
			if (MainDeviceIdx != "") {
				mParams += "&maindeviceidx=" + MainDeviceIdx;
			}

			return mParams;
		}

		TestManualLight = function () {
			var mParams = GetManualLightSettings(1);
			if (mParams == "") {
				return;
			}
			$.ajax({
				url: "json.htm?type=command&param=testswitch" + mParams,
				async: false,
				dataType: 'json',
				success: function (data) {
					if (typeof data.status != 'undefined') {
						if (data.status != 'OK') {
							ShowNotify($.t("There was an error!<br>Wrong switch parameters?") + ((typeof data.message != 'undefined') ? "<br>" + data.message : ""), 2500, true);
						}
						else {
							ShowNotify($.t("'On' command send!"), 2500);
						}
					}
				}
			});
		}

		EnableDisableSubDevices = function (ElementID, bEnabled) {
			var trow = $(ElementID);
			if (bEnabled == true) {
				trow.show();
			}
			else {
				trow.hide();
			}
		}

		init();

		function init() {
			//global var
			$.devIdx = 0;
			$.LastUpdateTime = parseInt(0);

			$.myglobals = {
				TimerTypesStr: [],
				CommandStr: [],
				OccurenceStr: [],
				MonthStr: [],
				WeekdayStr: [],
				SelectedTimerIdx: 0
			};
			$.LightsAndSwitches = [];
			$scope.MakeGlobalConfig();

			$('#timerparamstable #combotype > option').each(function () {
				$.myglobals.TimerTypesStr.push($(this).text());
			});
			$('#timerparamstable #combocommand > option').each(function () {
				$.myglobals.CommandStr.push($(this).text());
			});
			$('#timerparamstable #occurence > option').each(function () {
				$.myglobals.OccurenceStr.push($(this).text());
			});
			$('#timerparamstable #months > option').each(function () {
				$.myglobals.MonthStr.push($(this).text());
			});
			$('#timerparamstable #weekdays > option').each(function () {
				$.myglobals.WeekdayStr.push($(this).text());
			});

			$(window).resize(function () { $scope.ResizeDimSliders(); });

			$("#dialog-addlightdevice").dialog({
				autoOpen: false,
				width: 400,
				height: 290,
				modal: true,
				resizable: false,
				title: $.t("Add Light/Switch Device"),
				buttons: {
					"Add Device": function () {
						var bValid = true;
						bValid = bValid && checkLength($("#dialog-addlightdevice #devicename"), 2, 100);
						var bIsSubDevice = $("#dialog-addlightdevice #how_2").is(":checked");
						var MainDeviceIdx = "";
						if (bIsSubDevice) {
							var MainDeviceIdx = $("#dialog-addlightdevice #combosubdevice option:selected").val();
							if (typeof MainDeviceIdx == 'undefined') {
								bootbox.alert($.t('No Main Device Selected!'));
								return;
							}
						}

						if (bValid) {
							$(this).dialog("close");
							$.ajax({
								url: "json.htm?type=setused&idx=" + $.devIdx + '&name=' + encodeURIComponent($("#dialog-addlightdevice #devicename").val()) + '&switchtype=' + $("#dialog-addlightdevice #comboswitchtype").val() + '&used=true&maindeviceidx=' + MainDeviceIdx,
								async: false,
								dataType: 'json',
								success: function (data) {
									ShowLights();
								}
							});

						}
					},
					Cancel: function () {
						$(this).dialog("close");
					}
				},
				close: function () {
					$(this).dialog("close");
				}
			});
			$("#dialog-addlightdevice #how_1").click(function () {
				EnableDisableSubDevices("#dialog-addlightdevice #subdevice", false);
			});
			$("#dialog-addlightdevice #how_2").click(function () {
				EnableDisableSubDevices("#dialog-addlightdevice #subdevice", true);
			});

			var dialog_addmanuallightdevice_buttons = {};
			dialog_addmanuallightdevice_buttons[$.t("Add Device")] = function () {
				var mParams = GetManualLightSettings(0);
				if (mParams == "") {
					return;
				}
				$.pDialog = $(this);
				$.ajax({
					url: "json.htm?type=command&param=addswitch" + mParams,
					async: false,
					dataType: 'json',
					success: function (data) {
						if (typeof data.status != 'undefined') {
							if (data.status == 'OK') {
								$.pDialog.dialog("close");
								ShowLights();
							}
							else {
								ShowNotify(data.message, 3000, true);
							}
						}
					}
				});
			};
			dialog_addmanuallightdevice_buttons[$.t("Cancel")] = function () {
				$(this).dialog("close");
			};

			$("#dialog-addmanuallightdevice").dialog({
				autoOpen: false,
				width: 440,
				height: 480,
				modal: true,
				resizable: false,
				title: $.t("Add Manual Light/Switch Device"),
				buttons: dialog_addmanuallightdevice_buttons,
				open: function () {
					ConfigureAddManualSettings();
					$("#dialog-addmanuallightdevice #lighttable #comboswitchtype").change(function () {
						var switchtype = $("#dialog-addmanuallightdevice #lighttable #comboswitchtype option:selected").val(),
							subtype = -1;
						if (switchtype == 1) {
							subtype = 10;	// Doorbell -> COCO GDR2
						} else if (switchtype == 18) {
							subtype = 303;	// Selector -> Selector Switch
						}
						if (subtype !== -1) {
							$("#dialog-addmanuallightdevice #lighttable #combolighttype").val(subtype);
						}
						UpdateAddManualDialog();
					});
					$("#dialog-addmanuallightdevice #lighttable #combolighttype").change(function () {
						var subtype = $("#dialog-addmanuallightdevice #lighttable #combolighttype option:selected").val(),
							switchtype = -1;
						if (subtype == 303) {
							switchtype = 18;	// Selector -> Selector Switch
						}
						if (switchtype !== -1) {
							$("#dialog-addmanuallightdevice #lighttable #comboswitchtype").val(switchtype);
						}
						UpdateAddManualDialog();
					});
					UpdateAddManualDialog();
				},
				close: function () {
					$(this).dialog("close");
				}
			});

			$("#dialog-addmanuallightdevice #howtable #how_1").click(function () {
				EnableDisableSubDevices("#dialog-addmanuallightdevice #howtable #subdevice", false);
			});
			$("#dialog-addmanuallightdevice #howtable #how_2").click(function () {
				EnableDisableSubDevices("#dialog-addmanuallightdevice #howtable #subdevice", true);
			});

			ShowLights();
		};
		$scope.$on('$destroy', function () {
			if (typeof $scope.mytimer != 'undefined') {
				$interval.cancel($scope.mytimer);
				$scope.mytimer = undefined;
			}
			$(window).off("resize");
			var popup = $("#rgbw_popup");
			if (typeof popup != 'undefined') {
				popup.hide();
			}
			popup = $("#thermostat3_popup");
			if (typeof popup != 'undefined') {
				popup.hide();
			}
		});
	}]);
});
