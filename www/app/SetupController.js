define(['app'], function (app) {
	app.controller('SetupController', ['$scope', '$rootScope', '$window', '$location', '$http', '$interval', function ($scope, $rootScope, $window, $location, $http, $interval) {

		googleMapsCallback = function () {
			$("#dialog-findlatlong").dialog("open");
		};

		$scope.GetGeoLocation = function () {
			$.ajax({
				url: "https://maps.googleapis.com/maps/api/js?v=3&callback=googleMapsCallback&sensor=false",
				dataType: "script",
				cache: true
			});
		}

		$scope.AllowNewHardware = function (minutes) {
			$.ajax({
				url: "json.htm?type=command&param=allownewhardware&timeout=" + minutes,
				async: false,
				dataType: 'json',
				success: function (data) {
					SwitchLayout('Log');
					var msg = $.t('Allowing new sensors for ') + minutes + ' ' + $.t('Minutes');
					ShowNotify(msg, 3000);
				}
			});
		}

		$scope.TestNotification = function (subsystem) {
			var extraparams = "";
			switch (subsystem) {
				case "clickatell":
					var ClickatellAPI = encodeURIComponent($("#smstable #ClickatellAPI").val());
					var ClickatellUser = encodeURIComponent($("#smstable #ClickatellUser").val());
					var ClickatellPassword = encodeURIComponent($("#smstable #ClickatellPassword").val());
					var ClickatellTo = encodeURIComponent($("#smstable #ClickatellTo").val());
					var ClickatellFrom = encodeURIComponent($("#smstable #ClickatellFrom").val());
					if (ClickatellAPI == "" || ClickatellUser == "" || ClickatellPassword == "" || ClickatellTo == "" || ClickatellFrom == "") {
						ShowNotify($.t('All Clickatell fields are required!...'), 3500, true);
						return;
					}
					extraparams = "ClickatellAPI=" + ClickatellAPI + "&ClickatellUser=" + ClickatellUser + "&ClickatellPassword=" + ClickatellPassword + "&ClickatellTo=" + ClickatellTo + "&ClickatellFrom=" + ClickatellFrom;
					break;
				case "http":
					var HTTPField1 = encodeURIComponent($("#httptable #HTTPField1").val());
					var HTTPField2 = encodeURIComponent($("#httptable #HTTPField2").val());
					var HTTPField3 = encodeURIComponent($("#httptable #HTTPField3").val());
					var HTTPField4 = encodeURIComponent($("#httptable #HTTPField4").val());
					var HTTPTo = encodeURIComponent($("#httptable #HTTPTo").val());
					var HTTPURL = encodeURIComponent($("#httptable #HTTPURL").val());
					var HTTPPostData = encodeURIComponent($("#httptable #HTTPPostData").val());
					var HTTPPostHeaders = encodeURIComponent($("#httptable #HTTPPostHeaders").val());
					var HTTPPostContentType = encodeURIComponent($("#httptable #HTTPPostContentType").val());
					if (HTTPPostData != "" && HTTPPostContentType == "") {
						ShowNotify($.t('Please specify the content type...'), 3500, true);
						return;
					}
					if (HTTPURL == "") {
						ShowNotify($.t('Please specify the base URL!...'), 3500, true);
						return;
					}
					extraparams =
						"HTTPField1=" + HTTPField1 +
						"&HTTPField2=" + HTTPField2 +
						"&HTTPField3=" + HTTPField3 +
						"&HTTPField4=" + HTTPField4 +
						"&HTTPTo=" + HTTPTo +
						"&HTTPURL=" + HTTPURL +
						"&HTTPPostData=" + HTTPPostData +
						"&HTTPPostContentType=" + HTTPPostContentType +
						"&HTTPPostHeaders=" + HTTPPostHeaders;
					break;
				case "prowl":
					var ProwlAPI = encodeURIComponent($("#prowltable #ProwlAPI").val());
					if (ProwlAPI == "") {
						ShowNotify($.t('Please enter the API key!...'), 3500, true);
						return;
					}
					extraparams = "ProwlAPI=" + ProwlAPI;
					break;
				case "nma":
					var NMAAPI = encodeURIComponent($("#nmatable #NMAAPI").val());
					if (NMAAPI == "") {
						ShowNotify($.t('Please enter the API key!...'), 3500, true);
						return;
					}
					extraparams = "NMAAPI=" + NMAAPI;
					break;
				case "pushbullet":
					var PushbulletAPI = encodeURIComponent($("#pushbullettable #PushbulletAPI").val());
					if (PushbulletAPI == "") {
						ShowNotify($.t('Please enter the API key!...'), 3500, true);
						return;
					}
					extraparams = "PushbulletAPI=" + PushbulletAPI;
					break;
                                case "telegram":
                                        var TelegramAPI = encodeURIComponent($("#telegramtable #TelegramAPI").val());
                                        if (TelegramAPI == "") {
                                                ShowNotify($.t('Please enter the API key!...'), 3500, true);
                                                return;
                                        }
                                        extraparams = "TelegramAPI=" + TelegramAPI;
                                        break;
				case "pushsafer":
					var PushsaferAPI = encodeURIComponent($("#pushsafertable #PushsaferAPI").val());
					var PushsaferImage = encodeURIComponent($("#pushsafertable #PushsaferImage").val());
					if (PushsaferAPI == "") {
						ShowNotify($.t('Please enter the API key!...'), 3500, true);
						return;
					}
					extraparams = "PushsaferAPI=" + PushsaferAPI + "&PushsaferImage=" + PushsaferImage;
					break;
				case "pushover":
					var POAPI = encodeURIComponent($("#pushovertable #PushoverAPI").val());
					if (POAPI == "") {
						ShowNotify($.t('Please enter the API key!...'), 3500, true);
						return;
					}
					var POUSERID = encodeURIComponent($("#pushovertable #PushoverUser").val());
					if (POUSERID == "") {
						ShowNotify($.t('Please enter the user id!...'), 3500, true);
						return;
					}
					extraparams = "PushoverAPI=" + POAPI + "&PushoverUser=" + POUSERID;
					break;
				case "pushalot":
					var PushAlotAPI = encodeURIComponent($("#pushalottable #PushALotAPI").val());
					if (PushAlotAPI == "") {
						ShowNotify($.t('Please enter the API key!...'), 3500, true);
						return;
					}
					extraparams = "PushAlotAPI=" + PushAlotAPI;
					break;
				case "email":
					var EmailServer = encodeURIComponent($("#emailtable #EmailServer").val());
					if (EmailServer == "") {
						ShowNotify($.t('Invalid Email Settings...'), 2000, true);
						return;
					}
					var EmailPort = encodeURIComponent($("#emailtable #EmailPort").val());
					if (EmailPort == "") {
						ShowNotify($.t('Invalid Email Settings...'), 2000, true);
						return;
					}
					var EmailFrom = encodeURIComponent($("#emailtable #EmailFrom").val());
					var EmailTo = encodeURIComponent($("#emailtable #EmailTo").val());
					var EmailUsername = encodeURIComponent($("#emailtable #EmailUsername").val());
					var EmailPassword = encodeURIComponent($("#emailtable #EmailPassword").val());
					if ((EmailFrom == "") || (EmailTo == "")) {
						ShowNotify($.t('Invalid Email From/To Settings...'), 2000, true);
						return;
					}
					if ((EmailUsername != "") && (EmailPassword == "")) {
						ShowNotify($.t('Please enter an Email Password...'), 2000, true);
						return;
					}
					extraparams = "EmailServer=" + EmailServer + "&EmailPort=" + EmailPort + "&EmailFrom=" + EmailFrom + "&EmailTo=" + EmailTo + "&EmailUsername=" + EmailUsername + "&EmailPassword=" + EmailPassword;
					break;
				case "kodi":
					if ($("#koditable #KodiIPAddress").val() == "") $("#koditable #KodiIPAddress").val("224.0.0.1");
					if (($("#koditable #KodiPort").val() == "") || !$.isNumeric($("#koditable #KodiPort").val())) $("#koditable #KodiPort").val("9777");
					if (($("#koditable #KodiTimeToLive").val() == "") || !$.isNumeric($("#koditable #KodiTimeToLive").val())) $("#koditable #KodiTimeToLive").val("5");
					extraparams = 'KodiIPAddress=' + $("#koditable #KodiIPAddress").val() + '&KodiPort=' + $("#koditable #KodiPort").val() + "&KodiTimeToLive=" + $("#koditable #KodiTimeToLive").val();
					break;
				case "lms":
					if (($("#lmstable #LmsDuration").val() == "") || !$.isNumeric($("#lmstable #LmsDuration").val())) $("#lmstable #LmsDuration").val("5");
					var LmsPlayerMac = encodeURIComponent($("#lmstable #LmsPlayerMac").val());
					var LmsDuration = encodeURIComponent($("#lmstable #LmsDuration").val());
					if (LmsPlayerMac == "" || LmsDuration == "") {
						ShowNotify($.t('All Logitech Media Server fields are required!...'), 3500, true);
						return;
					}
					extraparams = 'LmsPlayerMac=' + $("#lmstable #LmsPlayerMac").val() + '&LmsDuration=' + $("#lmstable #LmsDuration").val();
					break;
				case "gcm":
					break;
				default:
					return;
			}
			$.ajax({
				url: "json.htm?type=command&param=testnotification&subsystem=" + subsystem + "&" + extraparams,
				async: false,
				dataType: 'json',
				success: function (data) {
					if (data.status != "OK") {
						HideNotify();
						if ((subsystem == "http") || (subsystem == "kodi") || (subsystem == "lms") || (subsystem == "gcm")) {
							ShowNotify($.t('Problem Sending Notification'), 3000, true);
						}
						else if (subsystem == "email") {
							ShowNotify($.t('Problem sending Email, please check credentials!'), 3000, true);
						}
						else {
							ShowNotify($.t('Problem sending notification, please check the API key!'), 3000, true);
						}
						return;
					}
					else {
						HideNotify();
						ShowNotify($.t('Notification sent!<br>Should arrive at your device soon...'), 3000);
					}
				},
				error: function () {
					HideNotify();
					if (subsystem == "email") {
						ShowNotify($.t('Invalid Email Settings...'), 3000, true);
					}
					else {
						ShowNotify($.t('Problem sending notification, please check the API key!'), 3000, true);
					}
				}
			});
		}

		$scope.ShowSettings = function () {
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

			var suntext = "";
			if (sunRise != "") {
				suntext = '<br>' + $.t('SunRise') + ': ' + sunRise + ', ' + $.t('SunSet') + ': ' + sunSet + '<br><br>\n';
				$("#sunriseset").html(suntext);
			}

			//Get Themes
			var actTheme = "default";
			$.ajax({
				url: "json.htm?type=command&param=getthemes",
				async: false,
				dataType: 'json',
				success: function (data) {
					if (typeof data.result != 'undefined') {
						actTheme = data.acttheme;
						$("#settingscontent #combothemes").html("");
						$.each(data.result, function (i, item) {
							var option = $('<option />');
							option.attr('value', item.theme).text(item.theme);
							$("#settingscontent #combothemes").append(option);
						});
					}
				}
			});

			//Get Timer Plans
			$.ajax({
				url: "json.htm?type=command&param=gettimerplans",
				async: false,
				dataType: 'json',
				success: function (data) {
					if (typeof data.result != 'undefined') {
						$("#settingscontent #comboTimerplan").html("");
						$.each(data.result, function (i, item) {
							var option = $('<option />');
							option.attr('value', item.idx).text(item.Name);
							$("#settingscontent #comboTimerplan").append(option);
						});
					}
				}
			});

			$.ajax({
				url: "json.htm?type=settings",
				async: false,
				dataType: 'json',
				success: function (data) {
					if (typeof data.Location != 'undefined') {
						$("#locationtable #Latitude").val(data.Location.Latitude);
						$("#locationtable #Longitude").val(data.Location.Longitude);
					}
					if (typeof data.ProwlEnabled != 'undefined') {
						$("#prowltable #ProwlEnabled").prop('checked', data.ProwlEnabled == 1);
					}
					if (typeof data.ProwlAPI != 'undefined') {
						$("#prowltable #ProwlAPI").val(data.ProwlAPI);
					}
					if (typeof data.NMAEnabled != 'undefined') {
						$("#nmatable #NMAEnabled").prop('checked', data.NMAEnabled == 1);
					}
					if (typeof data.NMAAPI != 'undefined') {
						$("#nmatable #NMAAPI").val(data.NMAAPI);
					}
					if (typeof data.PushbulletEnabled != 'undefined') {
						$("#pushbullettable #PushbulletEnabled").prop('checked', data.PushbulletEnabled == 1);
					}
					if (typeof data.PushbulletAPI != 'undefined') {
						$("#pushbullettable #PushbulletAPI").val(data.PushbulletAPI);
					}
                                        if (typeof data.TelegramEnabled != 'undefined') {
                                                $("#telegramtable #TelegramEnabled").prop('checked', data.TelegramEnabled == 1);
                                        }
                                        if (typeof data.TelegramAPI != 'undefined') {
                                                $("#telegramtable #TelegramAPI").val(data.TelegramAPI);
                                        }
                                        if (typeof data.TelegramChat != 'undefined') {
                                                $("#telegramtable #TelegramChat").val(data.TelegramChat);
                                        }
					if (typeof data.PushsaferEnabled != 'undefined') {
						$("#pushsafertable #PushsaferEnabled").prop('checked', data.PushsaferEnabled == 1);
					}
					if (typeof data.PushsaferAPI != 'undefined') {
						$("#pushsafertable #PushsaferAPI").val(data.PushsaferAPI);
					}
					if (typeof data.PushsaferImage != 'undefined') {
						$("#pushsafertable #PushsaferImage").val(data.PushsaferImage);
					}
					if (typeof data.PushoverEnabled != 'undefined') {
						$("#pushovertable #PushoverEnabled").prop('checked', data.PushoverEnabled == 1);
					}
					if (typeof data.PushoverAPI != 'undefined') {
						$("#pushovertable #PushoverAPI").val(data.PushoverAPI);
					}
					if (typeof data.PushoverUser != 'undefined') {
						$("#pushovertable #PushoverUser").val(data.PushoverUser);
					}
					if (typeof data.PushALotEnabled != 'undefined') {
						$("#pushalottable #PushALotEnabled").prop('checked', data.PushALotEnabled == 1);
					}
					if (typeof data.PushALotAPI != 'undefined') {
						$("#pushalottable #PushALotAPI").val(data.PushALotAPI);
					}
					if (typeof data.ClickatellEnabled != 'undefined') {
						$("#smstable #ClickatellEnabled").prop('checked', data.ClickatellEnabled == 1);
					}
					if (typeof data.ClickatellAPI != 'undefined') {
						$("#smstable #ClickatellAPI").val(atob(data.ClickatellAPI));
					}
					if (typeof data.ClickatellUser != 'undefined') {
						$("#smstable #ClickatellUser").val(atob(data.ClickatellUser));
					}
					if (typeof data.ClickatellPassword != 'undefined') {
						$("#smstable #ClickatellPassword").val(atob(data.ClickatellPassword));
					}
					if (typeof data.ClickatellTo != 'undefined') {
						$("#smstable #ClickatellTo").val(atob(data.ClickatellTo));
					}
					if (typeof data.ClickatellFrom != 'undefined') {
						$("#smstable #ClickatellFrom").val(atob(data.ClickatellFrom));
					}

					if (typeof data.HTTPEnabled != 'undefined') {
						$("#httptable #HTTPEnabled").prop('checked', data.HTTPEnabled == 1);
					}
					if (typeof data.HTTPField1 != 'undefined') {
						$("#httptable #HTTPField1").val(atob(data.HTTPField1));
					}
					if (typeof data.HTTPField2 != 'undefined') {
						$("#httptable #HTTPField2").val(atob(data.HTTPField2));
					}
					if (typeof data.HTTPField3 != 'undefined') {
						$("#httptable #HTTPField3").val(atob(data.HTTPField3));
					}
					if (typeof data.HTTPField4 != 'undefined') {
						$("#httptable #HTTPField4").val(atob(data.HTTPField4));
					}
					if (typeof data.HTTPTo != 'undefined') {
						$("#httptable #HTTPTo").val(atob(data.HTTPTo));
					}
					if (typeof data.HTTPURL != 'undefined') {
						$("#httptable #HTTPURL").val(atob(data.HTTPURL));
					}
					if (typeof data.HTTPPostData != 'undefined') {
						$("#httptable #HTTPPostData").val(atob(data.HTTPPostData));
					}
					if (typeof data.HTTPPostContentType != 'undefined') {
						$("#httptable #HTTPPostContentType").val(atob(data.HTTPPostContentType));
					}
					if (typeof data.HTTPPostHeaders != 'undefined') {
						$("#httptable #HTTPPostHeaders").val(atob(data.HTTPPostHeaders));
					}

					if (typeof data.KodiEnabled != 'undefined') {
						$("#koditable #KodiEnabled").prop('checked', data.KodiEnabled == 1);
					}
					$("#koditable #KodiIPAddress").val("224.0.0.1");
					if (typeof data.KodiIPAddress != 'undefined') {
						$("#koditable #KodiIPAddress").val(data.KodiIPAddress);
					}
					$("#koditable #KodiPort").val("9777");
					if (typeof data.KodiPort != 'undefined') {
						$("#koditable #KodiPort").val(data.KodiPort);
					}
					$("#koditable #KodiTimeToLive").val("5");
					if (typeof data.KodiTimeToLive != 'undefined') {
						$("#koditable #KodiTimeToLive").val(data.KodiTimeToLive);
					}

					if (typeof data.LmsEnabled != 'undefined') {
						$("#lmstable #LmsEnabled").prop('checked', data.LmsEnabled == 1);
					}
					if (typeof data.LmsPlayerMac != 'undefined') {
						$("#lmstable #LmsPlayerMac").val(data.LmsPlayerMac);
					}
					$("#lmstable #LmsDuration").val("5");
					if (typeof data.LmsDuration != 'undefined') {
						$("#lmstable #LmsDuration").val(data.LmsDuration);
					}
					if (typeof data.GCMEnabled != 'undefined') {
						$("#gcmtable #GCMEnabled").prop('checked', data.GCMEnabled == 1);
					}
					if (typeof data.LightHistoryDays != 'undefined') {
						$("#lightlogtable #LightHistoryDays").val(data.LightHistoryDays);
					}
					if (typeof data.ShortLogDays != 'undefined') {
						$("#shortlogtable #comboshortlogdays").val(data.ShortLogDays);
					}
					if (typeof data.ShortLogInterval != 'undefined') {
						$("#shortlogtable #comboshortloginterval").val(data.ShortLogInterval);
					}
					if (typeof data.DashboardType != 'undefined') {
						$("#dashmodetable #combosdashtype").val(data.DashboardType);
					}
					if (typeof data.MobileType != 'undefined') {
						$("#mobilemodetable #combosmobiletype").val(data.MobileType);
					}
					if (typeof data.WebUserName != 'undefined') {
						$("#webtable #WebUserName").val(data.WebUserName);
					}
					if (typeof data.WebPassword != 'undefined') {
						$("#webtable #WebPassword").val(data.WebPassword);
					}
					if (typeof data.SecPassword != 'undefined') {
						$("#sectable #SecPassword").val(data.SecPassword);
					}
					if (typeof data.ProtectionPassword != 'undefined') {
						$("#protectiontable #ProtectionPassword").val(data.ProtectionPassword);
					}
					if (typeof data.WebLocalNetworks != 'undefined') {
						$("#weblocaltable #WebLocalNetworks").val(data.WebLocalNetworks);
					}
					if (typeof data.EnergyDivider != 'undefined') {
						$("#rfxmetertable #EnergyDivider").val(data.EnergyDivider);
					}
					if (typeof data.CostEnergy != 'undefined') {
						$("#rfxmetertable #CostEnergy").val(data.CostEnergy);
					}
					if (typeof data.CostEnergyT2 != 'undefined') {
						$("#rfxmetertable #CostEnergyT2").val(data.CostEnergyT2);
					}
					if (typeof data.CostEnergyR1 != 'undefined') {
						$("#rfxmetertable #CostEnergyR1").val(data.CostEnergyR1);
					}
					if (typeof data.CostEnergyR2 != 'undefined') {
						$("#rfxmetertable #CostEnergyR2").val(data.CostEnergyR2);
					}
					if (typeof data.GasDivider != 'undefined') {
						$("#rfxmetertable #GasDivider").val(data.GasDivider);
					}
					if (typeof data.CostGas != 'undefined') {
						$("#rfxmetertable #CostGas").val(data.CostGas);
					}
					if (typeof data.WaterDivider != 'undefined') {
						$("#rfxmetertable #WaterDivider").val(data.WaterDivider);
					}
					if (typeof data.CostWater != 'undefined') {
						$("#rfxmetertable #CostWater").val(data.CostWater);
					}
					if (typeof data.RandomTimerFrame != 'undefined') {
						$("#randomtable #RandomSpread").val(data.RandomTimerFrame);
					}
					if (typeof data.SensorTimeout != 'undefined') {
						$("#timeouttable #SensorTimeout").val(data.SensorTimeout);
					}
					if (typeof data.BatterLowLevel != 'undefined') {
						$("#batterytable #BatterLowLevel").val(data.BatterLowLevel);
					}
					if (typeof data.ElectricVoltage != 'undefined') {
						$("#owl113table #ElectricVoltage").val(data.ElectricVoltage);
					}
					if (typeof data.CM113DisplayType != 'undefined') {
						$("#owl113table #comboscm113type").val(data.CM113DisplayType);
					}
					if (typeof data.WindUnit != 'undefined') {
						$("#windmetertable #comboWindUnit").val(data.WindUnit);
					}
					if (typeof data.TempUnit != 'undefined') {
						$("#temperaturetable #comboTempUnit").val(data.TempUnit);
					}
					if (typeof data.WeightUnit != 'undefined') {
						$("#weighttable #comboWeightUnit").val(data.WeightUnit);
					}
					if (typeof data.DegreeDaysBaseTemperature != 'undefined') {
						$("#temperaturetable #DegreeDaysBaseTemperature").val(data.DegreeDaysBaseTemperature);
					}
					if (typeof data.UseAutoUpdate != 'undefined') {
						$("#autoupdatetable #checkforupdates").prop('checked', data.UseAutoUpdate == 1);
					}
					if (typeof data.UseAutoBackup != 'undefined') {
						$("#autobackuptable #enableautobackup").prop('checked', data.UseAutoBackup == 1);
					}
					if (typeof data.EmailEnabled != 'undefined') {
						$("#emailtable #EmailEnabled").prop('checked', data.EmailEnabled == 1);
					}
					if (typeof data.EmailFrom != 'undefined') {
						$("#emailtable #EmailFrom").val(data.EmailFrom);
					}
					if (typeof data.EmailTo != 'undefined') {
						$("#emailtable #EmailTo").val(data.EmailTo);
					}
					if (typeof data.EmailServer != 'undefined') {
						$("#emailtable #EmailServer").val(data.EmailServer);
					}
					if (typeof data.EmailPort != 'undefined') {
						$("#emailtable #EmailPort").val(data.EmailPort);
					}
					if (typeof data.EmailUsername != 'undefined') {
						$("#emailtable #EmailUsername").val(atob(data.EmailUsername));
					}
					if (typeof data.EmailPassword != 'undefined') {
						$("#emailtable #EmailPassword").val(atob(data.EmailPassword));
					}
					if (typeof data.UseEmailInNotifications != 'undefined') {
						$("#emailtable #UseEmailInNotifications").prop('checked', data.UseEmailInNotifications == 1);
					}
					if (typeof data.EmailAsAttachment != 'undefined') {
						$("#emailtable #EmailAsAttachment").prop('checked', data.EmailAsAttachment == 1);
					}
					if (typeof data.ActiveTimerPlan != 'undefined') {
						$("#timerplantable #comboTimerplan").val(data.ActiveTimerPlan);
					}
					if (typeof data.DoorbellCommand != 'undefined') {
						$("#doorbelltable #comboDoorbellCommand").val(data.DoorbellCommand);
					}
					if (typeof data.SmartMeterType != 'undefined') {
						$("#p1metertable #comboP1MeterType").val(data.SmartMeterType);
					}
					if (typeof data.EnableTabFloorplans != 'undefined') {
						$("#activemenustable #EnableTabFloorplans").prop('checked', data.EnableTabFloorplans == 1);
					}
					if (typeof data.EnableTabLights != 'undefined') {
						$("#activemenustable #EnableTabLights").prop('checked', data.EnableTabLights == 1);
					}
					if (typeof data.EnableTabScenes != 'undefined') {
						$("#activemenustable #EnableTabScenes").prop('checked', data.EnableTabScenes == 1);
					}
					if (typeof data.EnableTabTemp != 'undefined') {
						$("#activemenustable #EnableTabTemp").prop('checked', data.EnableTabTemp == 1);
					}
					if (typeof data.EnableTabWeather != 'undefined') {
						$("#activemenustable #EnableTabWeather").prop('checked', data.EnableTabWeather == 1);
					}
					if (typeof data.EnableTabUtility != 'undefined') {
						$("#activemenustable #EnableTabUtility").prop('checked', data.EnableTabUtility == 1);
					}
					if (typeof data.EnableTabCustom != 'undefined') {
						$("#activemenustable #EnableTabCustom").prop('checked', data.EnableTabCustom == 1);
					}
					if (typeof data.NotificationSensorInterval != 'undefined') {
						$("#nitable #comboNotificationSensorInterval").val(data.NotificationSensorInterval);
					}
					if (typeof data.NotificationSwitchInterval != 'undefined') {
						$("#nitable #comboNotificationSwitchesInterval").val(data.NotificationSwitchInterval);
					}
					if (typeof data.RemoteSharedPort != 'undefined') {
						$("#remotesharedtable #RemoteSharedPort").val(data.RemoteSharedPort);
					}
					if (typeof data.Language != 'undefined') {
						$("#languagetable #combolanguage").val(data.Language);
					}
					if (typeof data.WebTheme != 'undefined') {
						$("#settingscontent #combothemes").val(data.WebTheme);
					}
					if (typeof data.Title != 'undefined') {
						sessionStorage.title = data.Title;
					}
					else {
						sessionStorage.title = 'Domoticz';
					}
					document.title = sessionStorage.title;
					$("#settingscontent #Title").val(sessionStorage.title);

					if (typeof data.AuthenticationMethod != 'undefined') {
						$("#webtable #comboauthmethod").val(data.AuthenticationMethod);
					}
					if (typeof data.ReleaseChannel != 'undefined') {
						$("#autoupdatetable #comboReleaseChannel").val(data.ReleaseChannel);
					}
					if (typeof data.RaspCamParams != 'undefined') {
						$("#picamtable #RaspCamParams").val(data.RaspCamParams);
					}
					if (typeof data.UVCParams != 'undefined') {
						$("#uvctable #UVCParams").val(data.UVCParams);
					}
					if (typeof data.AcceptNewHardware != 'undefined') {
						$("#acceptnewhardwaretable #AcceptNewHardware").prop('checked', data.AcceptNewHardware == 1);
					}
					if (typeof data.HideDisabledHardwareSensors != 'undefined') {
						$("#acceptnewhardwaretable #HideDisabledHardwareSensors").prop('checked', data.HideDisabledHardwareSensors == 1);
					}
					if (typeof data.ShowUpdateEffect != 'undefined') {
						$("#acceptnewhardwaretable #ShowUpdateEffect").prop('checked', data.ShowUpdateEffect == 1);
					}

					if (typeof data.EnableEventScriptSystem != 'undefined') {
						$("#eventsystemtable #EnableEventScriptSystem").prop('checked', data.EnableEventScriptSystem == 1);
					}
                    if (typeof data.DisableDzVentsSystem != 'undefined') {
                        $("#DisableDzVentsSystem").prop('checked', data.DisableDzVentsSystem == 0);
                    }
                    if (typeof data.DzVentsLogLevel != 'undefined') {
                        $("#comboDzVentsLogLevel").val(data.DzVentsLogLevel);
                    }
					if (typeof data.LogEventScriptTrigger != 'undefined') {
						$("#eventsystemtable #LogEventScriptTrigger").prop('checked', data.LogEventScriptTrigger == 1);
					}

					if (typeof data.FloorplanPopupDelay != 'undefined') {
						$("#floorplanoptionstable #FloorplanPopupDelay").val(data.FloorplanPopupDelay);
					}
					if (typeof data.FloorplanFullscreenMode != 'undefined') {
						$("#floorplanoptionstable #FloorplanFullscreenMode").prop('checked', data.FloorplanFullscreenMode == 1);
					}
					if (typeof data.FloorplanAnimateZoom != 'undefined') {
						$("#floorplanoptionstable #FloorplanAnimateZoom").prop('checked', data.FloorplanAnimateZoom == 1);
					}
					if (typeof data.FloorplanShowSensorValues != 'undefined') {
						$("#floorplandisplaytable #FloorplanShowSensorValues").prop('checked', data.FloorplanShowSensorValues == 1);
					}
					if (typeof data.FloorplanShowSwitchValues != 'undefined') {
						$("#floorplandisplaytable #FloorplanShowSwitchValues").prop('checked', data.FloorplanShowSwitchValues == 1);
					}
					if (typeof data.FloorplanShowSceneNames != 'undefined') {
						$("#floorplandisplaytable #FloorplanShowSceneNames").prop('checked', data.FloorplanShowSceneNames == 1);
					}
					if (typeof data.FloorplanRoomColour != 'undefined') {
						$("#floorplancolourtable #FloorplanRoomColour").val(data.FloorplanRoomColour);
					}
					if (typeof data.FloorplanActiveOpacity != 'undefined') {
						$("#floorplancolourtable #FloorplanActiveOpacity").val(data.FloorplanActiveOpacity);
					}
					if (typeof data.FloorplanInactiveOpacity != 'undefined') {
						$("#floorplancolourtable #FloorplanInactiveOpacity").val(data.FloorplanInactiveOpacity);
					}

					if (typeof data.AllowWidgetOrdering != 'undefined') {
						$("#dashmodetable #AllowWidgetOrdering").prop('checked', data.AllowWidgetOrdering == 1);
					}
					if (typeof data.SecOnDelay != 'undefined') {
						$("#sectable #SecOnDelay").val(data.SecOnDelay);
					}
					if (typeof data.LogLevel != 'undefined') {
						$("#LogDebug #LogFilterTable #LogLevel").val(data.LogLevel);
						$("#LogDebug").show();
					}
					if (typeof data.LogFilter != 'undefined') {
						$("#LogDebug #LogFilterTable #LogFilter").val(data.LogFilter);
					}
					if (typeof data.LogFileName != 'undefined') {
						$("#LogDebug #LogFilterTable #LogFileName").val(data.LogFileName);
					}
					if (typeof data.cloudenabled != 'undefined') {
						if (!data.cloudenabled) {
							$("#MyDomoticzTab").css("display", "none");
						}
					}
					if (typeof data.MyDomoticzInstanceId != 'undefined') {
						$("#mydomoticztable #mydomoticzinstanceidid").text(data.MyDomoticzInstanceId);
					}
					if (typeof data.MyDomoticzUserId != 'undefined') {
						$("#mydomoticztable #MyDomoticzUserId").val(data.MyDomoticzUserId);
					}
					if (typeof data.MyDomoticzPassword != 'undefined') {
						$("#mydomoticztable #MyDomoticzPassword").val(data.MyDomoticzPassword);
					}
					if (typeof data.MyDomoticzSubsystems != 'undefined') {
						$("#mydomoticztable #SubsystemHttp").prop("checked", (data.MyDomoticzSubsystems & 1) > 0);
						$("#mydomoticztable #SubsystemShared").prop("checked", (data.MyDomoticzSubsystems & 2) > 0);
						$("#mydomoticztable #SubsystemApps").prop("checked", (data.MyDomoticzSubsystems & 4) > 0);
					}
					if (typeof data.SendErrorsAsNotification != 'undefined') {
						$("#emailtable #SendErrorsAsNotification").prop('checked', data.SendErrorsAsNotification == 1);
					}
					if (typeof data.IFTTTEnabled != 'undefined') {
						$("#ifttttable #IFTTTEnabled").prop('checked', data.IFTTTEnabled == 1);
					}
					if (typeof data.IFTTTAPI != 'undefined') {
						$("#ifttttable #IFTTTAPI").val(atob(data.IFTTTAPI));
					}
				}
			});
		}

		$scope.StoreSettings = function (formname) {
			var Latitude = $("#locationtable #Latitude").val();
			var Longitude = $("#locationtable #Longitude").val();
			if (
				((Latitude == "") || (Longitude == "")) ||
				(isNaN(Latitude) || isNaN(Longitude))
			) {
				ShowNotify($.t('Invalid Location Settings...'), 2000, true);
				return;
			}

			var secpanel = $("#sectable #SecPassword").val();
			var switchprotection = $("#protectiontable #ProtectionPassword").val();

			// Apply Title
			sessionStorage.title = $("#settingscontent #Title").val();
			document.title = sessionStorage.title;

			//Check email settings
			var EmailServer = $("#emailtable #EmailServer").val();
			if (EmailServer != "") {
				var EmailPort = $("#emailtable #EmailPort").val();
				if (EmailPort == "") {
					ShowNotify($.t('Please enter an Email Port...'), 2000, true);
					return;
				}
				var EmailFrom = $("#emailtable #EmailFrom").val();
				var EmailTo = $("#emailtable #EmailTo").val();
				var EmailUsername = $("#emailtable #EmailUsername").val();
				var EmailPassword = $("#emailtable #EmailPassword").val();
				if ((EmailFrom == "") || (EmailTo == "")) {
					ShowNotify($.t('Invalid Email From/To Settings...'), 2000, true);
					return;
				}
				if ((EmailUsername != "") && (EmailPassword == "")) {
					ShowNotify($.t('Please enter an Email Password...'), 2000, true);
					return;
				}
			}

			var popupDelay = $("#floorplanoptionstable #FloorplanPopupDelay").val();
			if (popupDelay != "") {
				if (!$.isNumeric(popupDelay)) {
					ShowNotify($.t('Popup Delay can only contain numbers...'), 2000, true);
					return;
				}
			}

			$.post("storesettings.webem", $("#settings").serialize(), function (data) {
				$scope.$apply(function () {
					$window.location = '/#Dashboard';
					$window.location.reload();
				});
			});
		}

		$scope.MakeScrollLink = function (nameclick, namescroll) {
			$(nameclick).click(function (e) {
				var position = $(namescroll).offset();
				scroll(0, position.top - 60);
				e.preventDefault();
			});
		}

		$scope.CleanupShortLog = function () {
			bootbox.confirm($.t("Are you sure to delete the Log?\n\nThis action can not be undone!"), function (result) {
				if (result == true) {
					$.ajax({
						url: "json.htm?type=command&param=clearshortlog&idx=" + $.devIdx,
						async: false,
						dataType: 'json',
						success: function (data) {
							$window.location = '/#Dashboard';
							$window.location.reload();
						},
						error: function () {
							HideNotify();
							ShowNotify($.t('Problem clearing the Log!'), 2500, true);
						}
					});
				}
			});
		}

		init();

		function init() {
			$scope.MakeGlobalConfig();
			$scope.MakeScrollLink("#idsystem", "#system");
			$scope.MakeScrollLink("#idloghistory", "#loghistory");
			$scope.MakeScrollLink("#idnotifications", "#notifications");
			$scope.MakeScrollLink("#idemailsetup", "#emailsetup");
			$scope.MakeScrollLink("#idmetercounters", "#metercounters");
			$scope.MakeScrollLink("#idfloorplans", "#floorplans");
			$scope.MakeScrollLink("#idothersettings", "#othersettings");
			$scope.MakeScrollLink("#idrestoredatabase", "#restoredatabase");

			$("#dialog-findlatlong").dialog({
				autoOpen: false,
				width: 480,
				height: 280,
				modal: true,
				resizable: false,
				buttons: {
					"OK": function () {
						var bValid = true;
						bValid = bValid && checkLength($("#dialog-findlatlong #latitude"), 3, 100);
						bValid = bValid && checkLength($("#dialog-findlatlong #longitude"), 3, 100);
						if (bValid) {
							$("#locationtable #Latitude").val($('#dialog-findlatlong #latitude').val());
							$("#locationtable #Longitude").val($('#dialog-findlatlong #longitude').val());
							$(this).dialog("close");
						} else {
							bootbox.alert($.t('Please enter a Latitude and Longitude!...'), 3500, true);
						}
					},
					Cancel: function () {
						$(this).dialog("close");
					}
				},
				open: function () {
					$('#getlatlong').click(function () {
						var address = $('#dialog-findlatlong #address').val();
						if (address == "") {
							bootbox.alert($.t('Please enter a Address to search for!...'), 3500, true);
							return false;
						}
						geocoder = new google.maps.Geocoder();
						geocoder.geocode({ 'address': address }, function (results, status) {
							if (status == google.maps.GeocoderStatus.OK) {
								$('#dialog-findlatlong #latitude').val(results[0].geometry.location.lat().toFixed(6));
								$('#dialog-findlatlong #longitude').val(results[0].geometry.location.lng().toFixed(6));
							} else {
								bootbox.alert($.t('Geocode was not successful for the following reason') + ': ' + status);
							}
						});
						return false;
					});
				},
				close: function () {
					$(this).dialog("close");
				}
			});
			$("#maindiv").i18n();
			$scope.ShowSettings();
		};
	}]);
});
