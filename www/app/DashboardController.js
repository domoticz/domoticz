define(['app'], function (app) {
	app.controller('DashboardController', ['$scope', '$rootScope', '$location', '$http', '$interval', '$window', 'permissions', function ($scope, $rootScope, $location, $http, $interval, $window, permissions) {

		$scope.LastUpdateTime = parseInt(0);

		//Evohome...
		//FIXME move evohome functions to a shared js ...see temperaturecontroller.js and lightscontroller.js

		MobilePhoneDetection = function () {
			// mobile device detection
			if (/(android|bb\d+|meego).+mobile|avantgo|bada\/|blackberry|blazer|compal|elaine|fennec|hiptop|iemobile|ip(hone|od)|ipad|iris|kindle|Android|Silk|lge |maemo|midp|mmp|netfront|opera m(ob|in)i|palm( os)?|phone|p(ixi|re)\/|plucker|pocket|psp|series(4|6)0|symbian|treo|up\.(browser|link)|vodafone|wap|windows (ce|phone)|xda|xiino/i.test(navigator.userAgent)
				|| /1207|6310|6590|3gso|4thp|50[1-6]i|770s|802s|a wa|abac|ac(er|oo|s\-)|ai(ko|rn)|al(av|ca|co)|amoi|an(ex|ny|yw)|aptu|ar(ch|go)|as(te|us)|attw|au(di|\-m|r |s )|avan|be(ck|ll|nq)|bi(lb|rd)|bl(ac|az)|br(e|v)w|bumb|bw\-(n|u)|c55\/|capi|ccwa|cdm\-|cell|chtm|cldc|cmd\-|co(mp|nd)|craw|da(it|ll|ng)|dbte|dc\-s|devi|dica|dmob|do(c|p)o|ds(12|\-d)|el(49|ai)|em(l2|ul)|er(ic|k0)|esl8|ez([4-7]0|os|wa|ze)|fetc|fly(\-|_)|g1 u|g560|gene|gf\-5|g\-mo|go(\.w|od)|gr(ad|un)|haie|hcit|hd\-(m|p|t)|hei\-|hi(pt|ta)|hp( i|ip)|hs\-c|ht(c(\-| |_|a|g|p|s|t)|tp)|hu(aw|tc)|i\-(20|go|ma)|i230|iac( |\-|\/)|ibro|idea|ig01|ikom|im1k|inno|ipaq|iris|ja(t|v)a|jbro|jemu|jigs|kddi|keji|kgt( |\/)|klon|kpt |kwc\-|kyo(c|k)|le(no|xi)|lg( g|\/(k|l|u)|50|54|\-[a-w])|libw|lynx|m1\-w|m3ga|m50\/|ma(te|ui|xo)|mc(01|21|ca)|m\-cr|me(rc|ri)|mi(o8|oa|ts)|mmef|mo(01|02|bi|de|do|t(\-| |o|v)|zz)|mt(50|p1|v )|mwbp|mywa|n10[0-2]|n20[2-3]|n30(0|2)|n50(0|2|5)|n7(0(0|1)|10)|ne((c|m)\-|on|tf|wf|wg|wt)|nok(6|i)|nzph|o2im|op(ti|wv)|oran|owg1|p800|pan(a|d|t)|pdxg|pg(13|\-([1-8]|c))|phil|pire|pl(ay|uc)|pn\-2|po(ck|rt|se)|prox|psio|pt\-g|qa\-a|qc(07|12|21|32|60|\-[2-7]|i\-)|qtek|r380|r600|raks|rim9|ro(ve|zo)|s55\/|sa(ge|ma|mm|ms|ny|va)|sc(01|h\-|oo|p\-)|sdk\/|se(c(\-|0|1)|47|mc|nd|ri)|sgh\-|shar|sie(\-|m)|sk\-0|sl(45|id)|sm(al|ar|b3|it|t5)|so(ft|ny)|sp(01|h\-|v\-|v )|sy(01|mb)|t2(18|50)|t6(00|10|18)|ta(gt|lk)|tcl\-|tdg\-|tel(i|m)|tim\-|t\-mo|to(pl|sh)|ts(70|m\-|m3|m5)|tx\-9|up(\.b|g1|si)|utst|v400|v750|veri|vi(rg|te)|vk(40|5[0-3]|\-v)|vm40|voda|vulc|vx(52|53|60|61|70|80|81|83|85|98)|w3c(\-| )|webc|whit|wi(g |nc|nw)|wmlb|wonu|x700|yas\-|your|zeto|zte\-/i.test(navigator.userAgent.substr(0, 4))) {
				$("body").attr('id', 'onMobile');
			} else {
				$("body").attr('id', 'notMobile');
			}
		}

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
					bootbox.alert($.t('Problem sending switch command'));
				}
			});
		}

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
				return item.Status;//Don't convert for non Evohome switches just in case those status above get used anywhere
		}

		EvohomeAddJS = function () {
			return "<script type='text/javascript'> function deselect(e,id) { $(id).slideFadeToggle('swing', function() { e.removeClass('selected'); });} $.fn.slideFadeToggle = function(easing, callback) {  return this.animate({ opacity: 'toggle',height: 'toggle' }, 'fast', easing, callback);};</script>";
		}

		EvohomeImg = function (item, strclass) {
			//see http://www.theevohomeshop.co.uk/evohome-controller-display-icons/
			return '<div title="Quick Actions" class="' + ((item.Status == "Auto") ? "evoimgnorm " : "evoimg ") + strclass + '"><img src="images/evohome/' + item.Status + '.png" class="lcursor" onclick="if($(this).hasClass(\'selected\')){deselect($(this),\'#evopop_' + item.idx + '\');}else{$(this).addClass(\'selected\');$(\'#evopop_' + item.idx + '\').slideFadeToggle();}return false;"></div>';
		}

		EvohomePopupMenu = function (item, strclass) {
			var htm = '\t      <td id="img" class="img img1"><a href="#evohome" id="evohome_' + item.idx + '">' + EvohomeImg(item, strclass) + '</a></td>\n<span class="' + strclass + '"><div id="evopop_' + item.idx + '" class="ui-popup ui-body-b ui-overlay-shadow ui-corner-all pop">  <ul class="ui-listview ui-listview-inset ui-corner-all ui-shadow">         <li class="ui-li-divider ui-bar-inherit ui-first-child">Choose an action</li>';
			$.each([{ "name": "Normal", "data": "Auto" }, { "name": "Economy", "data": "AutoWithEco" }, { "name": "Away", "data": "Away" }, { "name": "Day Off", "data": "DayOff" }, { "name": "Custom", "data": "Custom" }, { "name": "Heating Off", "data": "HeatingOff" }], function (idx, obj) { htm += '<li><a href="#" class="ui-btn ui-btn-icon-right ui-icon-' + obj.data + '" onclick="SwitchModal(\'' + item.idx + '\',\'' + obj.name + '\',\'' + obj.data + '\',RefreshFavorites);deselect($(this),\'#evopop_' + item.idx + '\');return false;">' + obj.name + '</a></li>'; });
			htm += '</ul></div></span>';
			return htm;
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

		RefreshFavorites = function () {
			if (typeof $scope.mytimer != 'undefined') {
				$interval.cancel($scope.mytimer);
				$scope.mytimer = undefined;
			}
			var id = "";
			var bFavorites = 1;
			if (typeof window.myglobals.LastPlanSelected != 'undefined') {
				if (window.myglobals.LastPlanSelected > 0) {
					bFavorites = 0;
				}
			}
			$.ajax({
				url: "json.htm?type=devices&filter=all&used=true&favorite=" + bFavorites + "&order=[Order]&plan=" + window.myglobals.LastPlanSelected + "&lastupdate=" + $scope.LastUpdateTime,
				async: false,
				dataType: 'json',
				success: function (data) {
					if (typeof data.ServerTime != 'undefined') {
						$rootScope.SetTimeAndSun(data.Sunrise, data.Sunset, data.ServerTime);
					}

					if (typeof data.result != 'undefined') {
						if (typeof data.ActTime != 'undefined') {
							$scope.LastUpdateTime = parseInt(data.ActTime);
						}
						$.each(data.result, function (i, item) {
							//Scenes
							if (
								(item.Type.indexOf('Scene') == 0) ||
								(item.Type.indexOf('Group') == 0)
							) {
								id = "#dashcontent #scene_" + item.idx;
								var obj = $(id);
								if (typeof obj != 'undefined') {
									if (($scope.config.DashboardType == 2) || (window.myglobals.ismobile == true)) {
										var status = "";
										if (item.Type.indexOf('Group') == 0) {
											if (item.Status == 'Off') {
												status += '<button class="btn btn-mini" type="button" onclick="SwitchScene(' + item.idx + ',\'On\',RefreshFavorites, ' + item.Protected + ');">' + $.t("On") + '</button> ' +
													'<button class="btn btn-mini btn-info" type="button" onclick="SwitchScene(' + item.idx + ',\'Off\',RefreshFavorites, ' + item.Protected + ');">' + $.t("Off") + '</button>';
											}
											else {
												status += '<button class="btn btn-mini btn-info" type="button" onclick="SwitchScene(' + item.idx + ',\'On\',RefreshFavorites, ' + item.Protected + ');">' + $.t("On") + '</button> ' +
													'<button class="btn btn-mini" type="button" onclick="SwitchScene(' + item.idx + ',\'Off\',RefreshFavorites, ' + item.Protected + ');">' + $.t("Off") + '</button>';
											}
											if ($(id + " #status").html() != status) {
												$(id + " #status").html(status);
											}
										}
									}
									else {
										if (item.Type.indexOf('Group') == 0) {
											var backgroundClass = $rootScope.GetItemBackgroundStatus(item);
											$(id + " #bstatus").removeClass('statusNormal').removeClass('statusProtected').removeClass('statusTimeout').removeClass('statusLowBattery');
											$(id + " #bstatus").addClass(backgroundClass);

											var img1 = "";
											var img2 = "";
											var onclass = "";
											var offclass = "";

											var bigtext = TranslateStatusShort(item.Status);
											if (item.UsedByCamera == true) {
												var streamimg = '<img src="images/webcam.png" title="' + $.t('Stream Video') + '" height="16" width="16">';
												streamurl = "<a href=\"javascript:ShowCameraLiveStream('" + escape(item.Name) + "','" + item.CameraIdx + "')\">" + streamimg + "</a>";
												bigtext += "&nbsp;" + streamurl;
											}

											if (item.Status == 'On') {
												onclass = "transimg";
												offclass = "";
											}
											else if (item.Status == 'Off') {
												onclass = "";
												offclass = "transimg";
											}
											img1 = '<img class="lcursor ' + onclass + '" src="images/push48.png" title="' + $.t("Turn On") + '" onclick="SwitchScene(' + item.idx + ',\'On\',RefreshFavorites, ' + item.Protected + ');" height="40" width="40">';
											img2 = '<img class="lcursor ' + offclass + '"src="images/pushoff48.png" title="' + $.t("Turn Off") + '" onclick="SwitchScene(' + item.idx + ',\'Off\',RefreshFavorites, ' + item.Protected + ');" height="40" width="40">';
											if ($(id + " #img1").html() != img1) {
												$(id + " #img1").html(img1);
											}
											if ($(id + " #img2").html() != img2) {
												$(id + " #img2").html(img2);
											}
											if ($(id + " #bigtext > span").html() != TranslateStatus(item.Status)) {
												$(id + " #bigtext > span").html(bigtext);
											}
											if ($(id + " #lastupdate > span").html() != item.LastUpdate) {
												$(id + " #lastupdate > span").html(item.LastUpdate);
											}
											if ($scope.config.ShowUpdatedEffect == true) {
												$(id + " #name").effect("highlight", { color: '#EEFFEE' }, 1000);
											}
										}
									}
								}
							}
						}); //Scene devices

						$.each(data.result, function (i, item) {
							//Lights
							var isdimmer = false;
							if (
								(
									(item.Type.indexOf('Light') == 0) ||
									(item.Type.indexOf('Blind') == 0) ||
									(item.Type.indexOf('Curtain') == 0) ||
									(item.Type.indexOf('Thermostat 2') == 0) ||
									(item.Type.indexOf('Thermostat 3') == 0) ||
									(item.Type.indexOf('Chime') == 0) ||
									(item.Type.indexOf('RFY') == 0) ||
									(item.Type.indexOf('ASA') == 0) ||
									(item.SubType == "Smartwares Mode") ||
									(item.SubType == "Relay") ||
									((typeof item.SubType != 'undefined') && (item.SubType.indexOf('Itho') == 0)) ||
									((typeof item.SubType != 'undefined') && (item.SubType.indexOf('Lucci') == 0))
								)
								&& (item.Favorite != 0)) {
								id = "#dashcontent #light_" + item.idx;
								var obj = $(id);
								if (typeof obj != 'undefined') {
									if (($scope.config.DashboardType == 2) || (window.myglobals.ismobile == true)) {
										var status = TranslateStatus(item.Status) + " ";
										if (item.SwitchType == "Doorbell") {
											status += '<button class="btn btn-mini" type="button" onclick="SwitchLight(' + item.idx + ',\'On\',RefreshFavorites,' + item.Protected + ');">' + $.t("Ring") + '</button>';
										}
										else if (item.SwitchType == "Push On Button") {
											if (item.InternalState == "On") {
												status = '<button class="btn btn-mini btn-info" type="button" onclick="SwitchLight(' + item.idx + ',\'On\',RefreshFavorites,' + item.Protected + ');">' + $.t("On") + '</button>';
											}
											else {
												status = '<button class="btn btn-mini" type="button" onclick="SwitchLight(' + item.idx + ',\'On\',RefreshFavorites,' + item.Protected + ');">' + $.t("On") + '</button>';
											}
										}
										else if (item.SwitchType == "Door Contact") {
											if (item.InternalState == "Open") {
												status = '<button class="btn btn-mini btn-info" type="button" onclick="SwitchLight(' + item.idx + ',\'On\',RefreshFavorites,' + item.Protected + ');">' + $.t("Open") + '</button> ' +
													'<button class="btn btn-mini" type="button" onclick="SwitchLight(' + item.idx + ',\'Off\',RefreshFavorites,' + item.Protected + ');">' + $.t("Lock") + '</button>';
											}
											else {
												status = '<button class="btn btn-mini" type="button" onclick="SwitchLight(' + item.idx + ',\'On\',RefreshFavorites,' + item.Protected + ');">' + $.t("Open") + '</button> ' +
													'<button class="btn btn-mini btn-info" type="button" onclick="SwitchLight(' + item.idx + ',\'Off\',RefreshFavorites,' + item.Protected + ');">' + $.t("Locked") + '</button>';
											}
										}
										else if (item.SwitchType == "Door Lock") {
											if (item.InternalState == "Unlocked") {
												status = '<button class="btn btn-mini btn-info" type="button" onclick="SwitchLight(' + item.idx + ',\'Off\',RefreshFavorites,' + item.Protected + ');">' + $.t("Unlocked") + '</button> ' +
													'<button class="btn btn-mini" type="button" onclick="SwitchLight(' + item.idx + ',\'On\',RefreshFavorites,' + item.Protected + ');">' + $.t("Locked") + '</button>';
											}
											else {
												status = '<button class="btn btn-mini" type="button" onclick="SwitchLight(' + item.idx + ',\'Off\',RefreshFavorites,' + item.Protected + ');">' + $.t("Unlocked") + '</button> ' +
													'<button class="btn btn-mini btn-info" type="button" onclick="SwitchLight(' + item.idx + ',\'On\',RefreshFavorites,' + item.Protected + ');">' + $.t("Locked") + '</button>';
											}
										}
										else if (item.SwitchType == "Push Off Button") {
											status = '<button class="btn btn-mini" type="button" onclick="SwitchLight(' + item.idx + ',\'Off\',RefreshFavorites,' + item.Protected + ');">' + $.t("Off") + '</button>';
										}
										else if (item.SwitchType == "X10 Siren") {
											if (
												(item.Status == 'On') ||
												(item.Status == 'Chime') ||
												(item.Status == 'Group On') ||
												(item.Status == 'All On')
											) {
												status = '<button class="btn btn-mini btn-info" type="button">' + $.t("SIREN") + '</button>';
											}
											else {
												status = '<button class="btn btn-mini" type="button">' + $.t("Silence") + '</button>';
											}
										}
										else if (item.SwitchType == "Contact") {
											if (item.Status == 'Closed') {
												status = '<button class="btn btn-mini" type="button" onclick="ShowLightLog(' + item.idx + ',\'' + escape(item.Name) + '\', \'#dashcontent\', \'ShowFavorites\');">' + $.t("Closed") + '</button>';
											}
											else {
												status = '<button class="btn btn-mini btn-info" type="button" onclick="ShowLightLog(' + item.idx + ',\'' + escape(item.Name) + '\', \'#dashcontent\', \'ShowFavorites\');">' + $.t("Open") + '</button>';
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
													status =
														'<button class="btn btn-mini" type="button" onclick="SwitchLight(' + item.idx + ',\'Off\',RefreshFavorites,' + item.Protected + ');">' + $.t("Open") + '</button> ' +
														'<button class="btn btn-mini btn-danger" type="button" onclick="SwitchLight(' + item.idx + ',\'Stop\',RefreshFavorites,' + item.Protected + ');">' + $.t("Stop") + '</button> ' +
														'<button class="btn btn-mini btn-info" type="button" onclick="SwitchLight(' + item.idx + ',\'On\',RefreshFavorites,' + item.Protected + ');">' + $.t("Closed") + '</button>';
												}
												else {
													status =
														'<button class="btn btn-mini btn-info" type="button" onclick="SwitchLight(' + item.idx + ',\'Off\',RefreshFavorites,' + item.Protected + ');">' + $.t("Open") + '</button> ' +
														'<button class="btn btn-mini btn-danger" type="button" onclick="SwitchLight(' + item.idx + ',\'Stop\',RefreshFavorites,' + item.Protected + ');">' + $.t("Stop") + '</button> ' +
														'<button class="btn btn-mini" type="button" onclick="SwitchLight(' + item.idx + ',\'On\',RefreshFavorites,' + item.Protected + ');">' + $.t("Close") + '</button>';
												}
											}
											else {
												if (item.Status == 'Closed') {
													status =
														'<button class="btn btn-mini" type="button" onclick="SwitchLight(' + item.idx + ',\'Off\',RefreshFavorites,' + item.Protected + ');">' + $.t("Open") + '</button> ' +
														'<button class="btn btn-mini btn-info" type="button" onclick="SwitchLight(' + item.idx + ',\'On\',RefreshFavorites,' + item.Protected + ');">' + $.t("Closed") + '</button>';
												}
												else {
													status =
														'<button class="btn btn-mini btn-info" type="button" onclick="SwitchLight(' + item.idx + ',\'Off\',RefreshFavorites,' + item.Protected + ');">' + $.t("Open") + '</button> ' +
														'<button class="btn btn-mini" type="button" onclick="SwitchLight(' + item.idx + ',\'On\',RefreshFavorites,' + item.Protected + ');">' + $.t("Close") + '</button>';
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
													status =
														'<button class="btn btn-mini" type="button" onclick="SwitchLight(' + item.idx + ',\'On\',RefreshFavorites,' + item.Protected + ');">' + $.t("Open") + '</button> ' +
														'<button class="btn btn-mini btn-danger" type="button" onclick="SwitchLight(' + item.idx + ',\'Stop\',RefreshFavorites,' + item.Protected + ');">' + $.t("Stop") + '</button> ' +
														'<button class="btn btn-mini btn-info" type="button" onclick="SwitchLight(' + item.idx + ',\'Off\',RefreshFavorites,' + item.Protected + ');">' + $.t("Closed") + '</button>';
												}
												else {
													status =
														'<button class="btn btn-mini btn-info" type="button" onclick="SwitchLight(' + item.idx + ',\'On\',RefreshFavorites,' + item.Protected + ');">' + $.t("Open") + '</button> ' +
														'<button class="btn btn-mini btn-danger" type="button" onclick="SwitchLight(' + item.idx + ',\'Stop\',RefreshFavorites,' + item.Protected + ');">' + $.t("Stop") + '</button> ' +
														'<button class="btn btn-mini" type="button" onclick="SwitchLight(' + item.idx + ',\'Off\',RefreshFavorites,' + item.Protected + ');">' + $.t("Close") + '</button>';
												}
											}
											else {
												if (item.Status == 'Closed') {
													status =
														'<button class="btn btn-mini" type="button" onclick="SwitchLight(' + item.idx + ',\'On\',RefreshFavorites,' + item.Protected + ');">' + $.t("Open") + '</button> ' +
														'<button class="btn btn-mini btn-info" type="button" onclick="SwitchLight(' + item.idx + ',\'Off\',RefreshFavorites,' + item.Protected + ');">' + $.t("Closed") + '</button>';
												}
												else {
													status =
														'<button class="btn btn-mini btn-info" type="button" onclick="SwitchLight(' + item.idx + ',\'On\',RefreshFavorites,' + item.Protected + ');">' + $.t("Open") + '</button> ' +
														'<button class="btn btn-mini" type="button" onclick="SwitchLight(' + item.idx + ',\'Off\',RefreshFavorites),' + item.Protected + ';">' + $.t("Close") + '</button>';
												}
											}
										}
										else if (item.SwitchType == "Blinds Percentage") {
											isdimmer = true;
											if (item.Status == 'Open') {
												status =
													'<button class="btn btn-mini btn-info" type="button" onclick="SwitchLight(' + item.idx + ',\'Off\',RefreshFavorites,' + item.Protected + ');">' + $.t("Open") + '</button> ' +
													'<button class="btn btn-mini" type="button" onclick="SwitchLight(' + item.idx + ',\'On\',RefreshFavorites,' + item.Protected + ');">' + $.t("Close") + '</button>';
											}
											else {
												status =
													'<button class="btn btn-mini" type="button" onclick="SwitchLight(' + item.idx + ',\'Off\',RefreshFavorites,' + item.Protected + ');">' + $.t("Open") + '</button> ' +
													'<button class="btn btn-mini btn-info" type="button" onclick="SwitchLight(' + item.idx + ',\'On\',RefreshFavorites,' + item.Protected + ');">' + $.t("Closed") + '</button>';
											}
										}
										else if (item.SwitchType == "Blinds Percentage Inverted") {
											isdimmer = true;
											if (item.Status == 'Closed') {
												status =
													'<button class="btn btn-mini" type="button" onclick="SwitchLight(' + item.idx + ',\'On\',RefreshFavorites,' + item.Protected + ');">' + $.t("Open") + '</button> ' +
													'<button class="btn btn-mini btn-info" type="button" onclick="SwitchLight(' + item.idx + ',\'Off\',RefreshFavorites,' + item.Protected + ');">' + $.t("Closed") + '</button>';
											}
											else {
												status =
													'<button class="btn btn-mini btn-info" type="button" onclick="SwitchLight(' + item.idx + ',\'On\',RefreshFavorites,' + item.Protected + ');">' + $.t("Open") + '</button> ' +
													'<button class="btn btn-mini" type="button" onclick="SwitchLight(' + item.idx + ',\'Off\',RefreshFavorites,' + item.Protected + ');">' + $.t("Close") + '</button>';
											}
										}
										else if (item.SwitchType == "Dimmer") {
											isdimmer = true;
											var img = "";
											if (
												(item.Status == 'On') ||
												(item.Status == 'Chime') ||
												(item.Status == 'Group On') ||
												(item.Status.indexOf('Set ') == 0)
											) {
												status =
													'<label id=\"statustext\"><button class="btn btn-mini btn-info" type="button" onclick="SwitchLight(' + item.idx + ',\'On\',RefreshFavorites,' + item.Protected + ');">' + $.t("On") + '</button></label> ' +
													'<label id=\"img\"><button class="btn btn-mini" type="button" onclick="SwitchLight(' + item.idx + ',\'Off\',RefreshFavorites,' + item.Protected + ');">' + $.t("Off") + '</button></label>';
											}
											else {
												status =
													'<label id=\"statustext\"><button class="btn btn-mini" type="button" onclick="SwitchLight(' + item.idx + ',\'On\',RefreshFavorites,' + item.Protected + ');">' + $.t("On") + '</button></label> ' +
													'<label id=\"img\"><button class="btn btn-mini btn-info" type="button" onclick="SwitchLight(' + item.idx + ',\'Off\',RefreshFavorites,' + item.Protected + ');">' + $.t("Off") + '</button></label>';
											}
										}
										else if (item.SwitchType == "TPI") {
											var RO = (item.Unit < 64 || item.Unit > 95) ? true : false;
											isdimmer = true;
											var img = "";
											if (item.Status == 'On') {
												status =
													'<label id=\"statustext\"><button class="btn btn-mini btn-info" type="button" ' + (RO ? 'disabled' : '') + ' onclick="SwitchLight(' + item.idx + ',\'On\',RefreshFavorites,' + item.Protected + ');">' + $.t("On") + '</button></label> ' +
													'<label id=\"img\"><button class="btn btn-mini" type="button" ' + (RO ? 'disabled' : '') + ' onclick="SwitchLight(' + item.idx + ',\'Off\',RefreshFavorites,' + item.Protected + ');">' + $.t("Off") + '</button></label>';
											}
											else {
												status =
													'<label id=\"statustext\"><button class="btn btn-mini" type="button" ' + (RO ? 'disabled' : '') + ' onclick="SwitchLight(' + item.idx + ',\'On\',RefreshFavorites,' + item.Protected + ');">' + $.t("On") + '</button></label> ' +
													'<label id=\"img\"><button class="btn btn-mini btn-info" type="button" ' + (RO ? 'disabled' : '') + ' onclick="SwitchLight(' + item.idx + ',\'Off\',RefreshFavorites,' + item.Protected + ');">' + $.t("Off") + '</button></label>';
											}
										}
										else if (item.SwitchType == "Dusk Sensor") {
											if (item.Status == 'On') {
												status = '<button class="btn btn-mini" type="button" onclick="ShowLightLog(' + item.idx + ',\'' + escape(item.Name) + '\', \'#dashcontent\', \'ShowFavorites\');">' + $.t("Dark") + '</button>';
											}
											else {
												status = '<button class="btn btn-mini" type="button" onclick="ShowLightLog(' + item.idx + ',\'' + escape(item.Name) + '\', \'#dashcontent\', \'ShowFavorites\');">' + $.t("Sunny") + '</button>';
											}
										}
										else if (item.SwitchType == "Motion Sensor") {
											if (
												(item.Status == 'On') ||
												(item.Status == 'Chime') ||
												(item.Status == 'Group On') ||
												(item.Status.indexOf('Set ') == 0)
											) {
												status = '<button class="btn btn-mini btn-info" type="button" onclick="ShowLightLog(' + item.idx + ',\'' + escape(item.Name) + '\', \'#dashcontent\', \'ShowFavorites\');">' + $.t("Motion") + '</button>';
											}
											else {
												status = '<button class="btn btn-mini" type="button" onclick="ShowLightLog(' + item.idx + ',\'' + escape(item.Name) + '\', \'#dashcontent\', \'ShowFavorites\');">' + $.t("No Motion") + '</button>';
											}
										}
										else if (item.SwitchType == "Dusk Sensor") {
											if (item.Status == 'On') {
												status = '<button class="btn btn-mini" type="button" onclick="ShowLightLog(' + item.idx + ',\'' + escape(item.Name) + '\', \'#dashcontent\', \'ShowFavorites\');">' + $.t("Dark") + '</button>';
											}
											else {
												status = '<button class="btn btn-mini" type="button" onclick="ShowLightLog(' + item.idx + ',\'' + escape(item.Name) + '\', \'#dashcontent\', \'ShowFavorites\');">' + $.t("Sunny") + '</button>';
											}
										}
										else if (item.SwitchType == "Smoke Detector") {
											if ((item.Status == "Panic") || (item.Status == "On")) {
												status = '<button class="btn btn-mini btn-info" type="button" onclick="ShowLightLog(' + item.idx + ',\'' + escape(item.Name) + '\', \'#dashcontent\', \'ShowFavorites\');">' + $.t("SMOKE") + '</button>';
											}
											else {
												status = '<button class="btn btn-mini" type="button" onclick="ShowLightLog(' + item.idx + ',\'' + escape(item.Name) + '\', \'#dashcontent\', \'ShowFavorites\');">' + $.t("No Smoke") + '</button>';
											}
										}
										else if (item.SwitchType == "Selector") {
											// no status needed in mobile mode
											status = '';
											// update buttons
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
										}
										else if (item.SubType.indexOf("Itho") == 0) {
											var class_1 = "btn btn-mini";
											var class_2 = "btn btn-mini";
											var class_3 = "btn btn-mini";
											var class_timer = "btn btn-mini";
											if (item.Status == "1") {
												class_1 += " btn-info";
											}
											else if (item.Status == "2") {
												class_2 += " btn-info";
											}
											else if (item.Status == "3") {
												class_3 += " btn-info";
											}
											else if (item.Status == "timer") {
												class_timer += " btn-info";
											}
											status =
												'<button class="' + class_1 + '" type="button" onclick="SwitchLight(' + item.idx + ',\'1\',RefreshFavorites,' + item.Protected + ');">' + $.t("1") + '</button> ' +
												'<button class="' + class_2 + '" type="button" onclick="SwitchLight(' + item.idx + ',\'2\',RefreshFavorites,' + item.Protected + ');">' + $.t("2") + '</button> ' +
												'<button class="' + class_3 + '" type="button" onclick="SwitchLight(' + item.idx + ',\'3\',RefreshFavorites,' + item.Protected + ');">' + $.t("3") + '</button> ' +
												'<button class="' + class_timer + '" type="button" onclick="SwitchLight(' + item.idx + ',\'timer\',RefreshFavorites,' + item.Protected + ');">' + $.t("Timer") + '</button>';
										}
										else if (item.SubType.indexOf("Lucci") == 0) {
											var class_1 = "btn btn-mini";
											var class_2 = "btn btn-mini";
											var class_3 = "btn btn-mini";
											var class_4 = "btn btn-mini";
											var class_light = "btn btn-mini";
											if (item.Status == "hi") {
												class_1 += " btn-info";
											}
											else if (item.Status == "med") {
												class_2 += " btn-info";
											}
											else if (item.Status == "low") {
												class_3 += " btn-info";
											}
											else if (item.Status == "off") {
												class_4 += " btn-info";
											}
											else if (item.Status == "light") {
												class_light += " btn-info";
											}
											status =
												'<button class="' + class_1 + '" type="button" onclick="SwitchLight(' + item.idx + ',\'hi\',RefreshFavorites,' + item.Protected + ');">' + $.t("hi") + '</button> ' +
												'<button class="' + class_2 + '" type="button" onclick="SwitchLight(' + item.idx + ',\'med\',RefreshFavorites,' + item.Protected + ');">' + $.t("med") + '</button> ' +
												'<button class="' + class_3 + '" type="button" onclick="SwitchLight(' + item.idx + ',\'low\',RefreshFavorites,' + item.Protected + ');">' + $.t("low") + '</button> ' +
												'<button class="' + class_4 + '" type="button" onclick="SwitchLight(' + item.idx + ',\'off\',RefreshFavorites,' + item.Protected + ');">' + $.t("Off") + '</button> ' +
												'<button class="' + class_light + '" type="button" onclick="SwitchLight(' + item.idx + ',\'light\',RefreshFavorites,' + item.Protected + ');">' + $.t("Light") + '</button>';
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
												status =
													'<button class="btn btn-mini btn-info" type="button" onclick="SwitchLight(' + item.idx + ',\'On\',RefreshFavorites,' + item.Protected + ');">' + $.t("On") + '</button> ' +
													'<button class="btn btn-mini" type="button" onclick="SwitchLight(' + item.idx + ',\'Off\',RefreshFavorites,' + item.Protected + ');">' + $.t("Off") + '</button>';
											}
											else {
												status =
													'<button class="btn btn-mini" type="button" onclick="SwitchLight(' + item.idx + ',\'On\',RefreshFavorites,' + item.Protected + ');">' + $.t("On") + '</button> ' +
													'<button class="btn btn-mini btn-info" type="button" onclick="SwitchLight(' + item.idx + ',\'Off\',RefreshFavorites,' + item.Protected + ');">' + $.t("Off") + '</button>';
											}
										}
										if (isdimmer == true) {
											var dslider = $(id + "_slider");
											if (typeof dslider != 'undefined') {
												dslider.slider("value", item.LevelInt + 1);
											}
										}
										if ($(id + " #status").html() != status) {
											$(id + " #status").html(status);
										}
									}
									else {
										//normal/compact dashboard
										var img = "";
										var img2 = "";
										var img3 = "";

										var bigtext = TranslateStatusShort(item.Status);
										if (item.UsedByCamera == true) {
											var streamimg = '<img src="images/webcam.png" title="' + $.t('Stream Video') + '" height="16" width="16">';
											streamurl = "<a href=\"javascript:ShowCameraLiveStream('" + escape(item.Name) + "','" + item.CameraIdx + "')\">" + streamimg + "</a>";
											bigtext += "&nbsp;" + streamurl;
										}

										if (item.SwitchType == "Doorbell") {
											img = '<img src="images/doorbell48.png" title="' + $.t("Turn On") + '" onclick="SwitchLight(' + item.idx + ',\'On\',RefreshFavorites,' + item.Protected + ');" class="lcursor" height="40" width="40">';
										}
										else if (item.SwitchType == "Push On Button") {
											if (item.InternalState == "On") {
												img = '<img src="images/pushon48.png" title="' + $.t("Turn On") + '" onclick="SwitchLight(' + item.idx + ',\'On\',RefreshFavorites,' + item.Protected + ');" class="lcursor" height="40" width="40">';
											}
											else {
												img = '<img src="images/push48.png" title="' + $.t("Turn On") + '" onclick="SwitchLight(' + item.idx + ',\'On\',RefreshFavorites,' + item.Protected + ');" class="lcursor" height="40" width="40">';
											}
										}
										else if (item.SwitchType == "Door Contact") {
											if (item.InternalState == "Open") {
												img = '<img src="images/door48open.png" title="' + $.t("Close Door") + '" onclick="SwitchLight(' + item.idx + ',\'Off\',RefreshFavorites,' + item.Protected + ');" class="lcursor" height="40" width="40">';
											}
											else {
												img = '<img src="images/door48.png" title="' + $.t("Open Door") + '" onclick="SwitchLight(' + item.idx + ',\'On\',RefreshFavorites,' + item.Protected + ');" class="lcursor" height="40" width="40">';
											}
										}
										else if (item.SwitchType == "Door Lock") {
											if (item.InternalState == "Unlocked") {
												img = '<img src="images/door48open.png" title="' + $.t("Lock") + '" onclick="SwitchLight(' + item.idx + ',\'On\',RefreshFavorites,' + item.Protected + ');" class="lcursor" height="40" width="40">';
											}
											else {
												img = '<img src="images/door48.png" title="' + $.t("Unlock") + '" onclick="SwitchLight(' + item.idx + ',\'Off\',RefreshFavorites,' + item.Protected + ');" class="lcursor" height="40" width="40">';
											}
										}
										else if (item.SwitchType == "Push Off Button") {
											img = '<img src="images/pushoff48.png" title="' + $.t("Turn Off") + '" onclick="SwitchLight(' + item.idx + ',\'Off\',RefreshFavorites,' + item.Protected + ');" class="lcursor" height="40" width="40">';
										}
										else if (item.SwitchType == "X10 Siren") {
											if (
												(item.Status == 'On') ||
												(item.Status == 'Chime') ||
												(item.Status == 'Group On') ||
												(item.Status == 'All On')
											) {
												img = '<img src="images/siren-on.png" height="40" width="40">';
											}
											else {
												img = '<img src="images/siren-off.png" height="40" width="40">';
											}
										}
										else if (item.SwitchType == "Contact") {
											if (item.Status == 'Closed') {
												img = '<img src="images/contact48.png" onclick="ShowLightLog(' + item.idx + ',\'' + escape(item.Name) + '\', \'#dashcontent\', \'ShowFavorites\');" class="lcursor" height="40" width="40" >';
											}
											else {
												img = '<img src="images/contact48_open.png" onclick="ShowLightLog(' + item.idx + ',\'' + escape(item.Name) + '\', \'#dashcontent\', \'ShowFavorites\');" class="lcursor" height="40" width="40">';
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
													img = '<img src="images/blindsopen48.png" title="' + $.t("Open Blinds") + '" onclick="SwitchLight(' + item.idx + ',\'Off\',RefreshFavorites,' + item.Protected + ');" class="lcursor" height="40" width="40">';
													img3 = '<img src="images/blinds48sel.png" title="' + $.t("Close Blinds") + '" onclick="SwitchLight(' + item.idx + ',\'On\',RefreshFavorites,' + item.Protected + ');" class="lcursor" height="40" width="40">';
												}
												else {
													img = '<img src="images/blindsopen48sel.png" title="' + $.t("Open Blinds") + '" onclick="SwitchLight(' + item.idx + ',\'Off\',RefreshFavorites,' + item.Protected + ');" class="lcursor" height="40" width="40">';
													img3 = '<img src="images/blinds48.png" title="' + $.t("Close Blinds") + '" onclick="SwitchLight(' + item.idx + ',\'On\',RefreshFavorites,' + item.Protected + ');" class="lcursor" height="40" width="40">';
												}
											}
											else {
												if (item.Status == 'Closed') {
													img = '<img src="images/blindsopen48.png" title="' + $.t("Open Blinds") + '" onclick="SwitchLight(' + item.idx + ',\'Off\',RefreshFavorites,' + item.Protected + ');" class="lcursor" height="40" width="40">';
													img2 = '<img src="images/blinds48sel.png" title="' + $.t("Close Blinds") + '" onclick="SwitchLight(' + item.idx + ',\'On\',RefreshFavorites,' + item.Protected + ');" class="lcursor" height="40" width="40">';
												}
												else {
													img = '<img src="images/blindsopen48sel.png" title="' + $.t("Open Blinds") + '" onclick="SwitchLight(' + item.idx + ',\'Off\',RefreshFavorites,' + item.Protected + ');" class="lcursor" height="40" width="40">';
													img2 = '<img src="images/blinds48.png" title="' + $.t("Close Blinds") + '" onclick="SwitchLight(' + item.idx + ',\'On\',RefreshFavorites,' + item.Protected + ');" class="lcursor" height="40" width="40">';
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
													img = '<img src="images/blindsopen48.png" title="' + $.t("Open Blinds") + '" onclick="SwitchLight(' + item.idx + ',\'On\',RefreshFavorites,' + item.Protected + ');" class="lcursor" height="40" width="40">';
													img3 = '<img src="images/blinds48sel.png" title="' + $.t("Close Blinds") + '" onclick="SwitchLight(' + item.idx + ',\'Off\',RefreshFavorites,' + item.Protected + ');" class="lcursor" height="40" width="40">';
												}
												else {
													img = '<img src="images/blindsopen48sel.png" title="' + $.t("Open Blinds") + '" onclick="SwitchLight(' + item.idx + ',\'On\',RefreshFavorites,' + item.Protected + ');" class="lcursor" height="40" width="40">';
													img3 = '<img src="images/blinds48.png" title="' + $.t("Close Blinds") + '" onclick="SwitchLight(' + item.idx + ',\'Off\',RefreshFavorites,' + item.Protected + ');" class="lcursor" height="40" width="40">';
												}
											}
											else {
												if (item.Status == 'Closed') {
													img = '<img src="images/blindsopen48.png" title="' + $.t("Open Blinds") + '" onclick="SwitchLight(' + item.idx + ',\'On\',RefreshFavorites,' + item.Protected + ');" class="lcursor" height="40" width="40">';
													img2 = '<img src="images/blinds48sel.png" title="' + $.t("Close Blinds") + '" onclick="SwitchLight(' + item.idx + ',\'Off\',RefreshFavorites,' + item.Protected + ');" class="lcursor" height="40" width="40">';
												}
												else {
													img = '<img src="images/blindsopen48sel.png" title="' + $.t("Open Blinds") + '" onclick="SwitchLight(' + item.idx + ',\'On\',RefreshFavorites,' + item.Protected + ');" class="lcursor" height="40" width="40">';
													img2 = '<img src="images/blinds48.png" title="' + $.t("Close Blinds") + '" onclick="SwitchLight(' + item.idx + ',\'Off\',RefreshFavorites,' + item.Protected + ');" class="lcursor" height="40" width="40">';
												}
											}
										}
										else if (item.SwitchType == "Blinds Percentage") {
											isdimmer = true;
											if (item.Status == 'Open') {
												img = '<img src="images/blindsopen48sel.png" title="' + $.t("Open Blinds") + '" onclick="SwitchLight(' + item.idx + ',\'Off\',RefreshFavorites,' + item.Protected + ');" class="lcursor" height="40" width="40">';
												img2 = '<img src="images/blinds48.png" title="' + $.t("Close Blinds") + '" onclick="SwitchLight(' + item.idx + ',\'On\',RefreshFavorites,' + item.Protected + ');" class="lcursor" height="40" width="40">';
											}
											else {
												img = '<img src="images/blindsopen48.png" title="' + $.t("Open Blinds") + '" onclick="SwitchLight(' + item.idx + ',\'Off\',RefreshFavorites,' + item.Protected + ');" class="lcursor" height="40" width="40">';
												img2 = '<img src="images/blinds48sel.png" title="' + $.t("Close Blinds") + '" onclick="SwitchLight(' + item.idx + ',\'On\',RefreshFavorites,' + item.Protected + ');" class="lcursor" height="40" width="40">';
											}
										}
										else if (item.SwitchType == "Blinds Percentage Inverted") {
											isdimmer = true;
											if (item.Status == 'Closed') {
												img = '<img src="images/blindsopen48.png" title="' + $.t("Open Blinds") + '" onclick="SwitchLight(' + item.idx + ',\'On\',RefreshFavorites,' + item.Protected + ');" class="lcursor" height="40" width="40">';
												img2 = '<img src="images/blinds48sel.png" title="' + $.t("Close Blinds") + '" onclick="SwitchLight(' + item.idx + ',\'Off\',RefreshFavorites,' + item.Protected + ');" class="lcursor" height="40" width="40">';
											}
											else {
												img = '<img src="images/blindsopen48sel.png" title="' + $.t("Open Blinds") + '" onclick="SwitchLight(' + item.idx + ',\'On\',RefreshFavorites,' + item.Protected + ');" class="lcursor" height="40" width="40">';
												img2 = '<img src="images/blinds48.png" title="' + $.t("Close Blinds") + '" onclick="SwitchLight(' + item.idx + ',\'Off\',RefreshFavorites,' + item.Protected + ');" class="lcursor" height="40" width="40">';
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
													img = '<img src="images/RGB48_On.png" onclick="ShowRGBWPopup(event, ' + item.idx + ', \'RefreshFavorites\',' + item.Protected + ',' + item.MaxDimLevel + ',' + item.LevelInt + ',' + item.Hue + ');" class="lcursor" height="40" width="40">';
												}
												else if (item.SubType.indexOf("RGBW") >= 0) {
													img = '<img src="images/RGB48_On.png" onclick="ShowRGBWPopup(event, ' + item.idx + ', \'RefreshFavorites\',' + item.Protected + ',' + item.MaxDimLevel + ',' + item.LevelInt + ',' + item.Hue + ');" class="lcursor" height="40" width="40">';
												}
												else {
													img = '<img src="images/' + item.Image + '48_On.png" title="' + $.t("Turn Off") + '" onclick="SwitchLight(' + item.idx + ',\'Off\',RefreshFavorites,' + item.Protected + ');" class="lcursor" height="40" width="40">';
												}
											}
											else {
												if (item.SubType == "RGB") {
													img = '<img src="images/RGB48_Off.png" onclick="ShowRGBWPopup(event, ' + item.idx + ',\'RefreshFavorites\',' + item.Protected + ',' + item.MaxDimLevel + ',' + item.LevelInt + ',' + item.Hue + ');" class="lcursor" height="40" width="40">';
												}
												else if (item.SubType.indexOf("RGBW") >= 0) {
													img = '<img src="images/RGB48_Off.png" onclick="ShowRGBWPopup(event, ' + item.idx + ',\'RefreshFavorites\',' + item.Protected + ',' + item.MaxDimLevel + ',' + item.LevelInt + ',' + item.Hue + ');" class="lcursor" height="40" width="40">';
												}
												else {
													img = '<img src="images/' + item.Image + '48_Off.png" title="' + $.t("Turn On") + '" onclick="SwitchLight(' + item.idx + ',\'On\',RefreshFavorites,' + item.Protected + ');" class="lcursor" height="40" width="40">';
												}
											}
										}
										else if (item.SwitchType == "TPI") {
											var RO = (item.Unit < 64 || item.Unit > 95) ? true : false;
											isdimmer = true;
											if (
												(item.Status == 'On')
											) {
												img = '<img src="images/Fireplace48_On.png" title="' + $.t(RO ? "On" : "Turn Off") + (RO ? '"' : '" onclick="SwitchLight(' + item.idx + ',\'Off\',RefreshFavorites,' + item.Protected + ');" class="lcursor"') + ' height="40" width="40">';
												$(id + " #div.item").removeClass('Off-fireplace').addClass('on-fireplace');
											}
											else {
												img = '<img src="images/Fireplace48_Off.png" title="' + $.t(RO ? "Off" : "Turn On") + (RO ? '"' : '" onclick="SwitchLight(' + item.idx + ',\'On\',RefreshFavorites,' + item.Protected + ');" class="lcursor"') + ' height="40" width="40">';
												$(id + " #div.item").removeClass('on-fireplace').addClass('off-fireplace');
											}
										}
										else if (item.SwitchType == "Dusk Sensor") {
											if (
												(item.Status == 'On')
											) {
												img = '<img src="images/uvdark.png" onclick="ShowLightLog(' + item.idx + ',\'' + escape(item.Name) + '\', \'#dashcontent\', \'ShowFavorites\');" class="lcursor" height="40" width="40">';
												$(id + " #div.item").removeClass('uvSunny').addClass('fireplaceOn');
											}
											else {
												img = '<img src="images/uvsunny.png" onclick="ShowLightLog(' + item.idx + ',\'' + escape(item.Name) + '\', \'#dashcontent\', \'ShowFavorites\');" class="lcursor" height="40" width="40">';
											}
										}
										else if (item.SwitchType == "Media Player") {
											if (item.CustomImage == 0) item.Image = item.TypeImg;
											if (item.Status == 'Disconnected') {
												img = '<img src="images/' + item.Image + '48_Off.png" height="40" width="40">';
												img2 = '<img src="images/remote48.png" style="opacity:0.4"; height="40" width="40">';
											}
											else if ((item.Status != 'Off') && (item.Status != '0')) {
												img = '<img src="images/' + item.Image + '48_On.png" onclick="SwitchLight(' + item.idx + ',\'Off\',RefreshFavorites,' + item.Protected + ');" class="lcursor" height="40" width="40">';
												img2 = '<img src="images/remote48.png" onclick="ShowMediaRemote(\'' + escape(item.Name) + "'," + item.idx + ",'" + item.HardwareType + '\');" class="lcursor" height="40" width="40">';
											}
											else {
												img = '<img src="images/' + item.Image + '48_Off.png" onclick="SwitchLight(' + item.idx + ',\'On\',RefreshFavorites,' + item.Protected + ');" class="lcursor" height="40" width="40">';
												img2 = '<img src="images/remote48.png" style="opacity:0.4"; height="40" width="40">';
											}
											if (item.Status.length == 1) item.Status = "";
											status = item.Data;
										}
										else if (item.SwitchType == "Motion Sensor") {
											if (
												(item.Status == 'On') ||
												(item.Status == 'Chime') ||
												(item.Status == 'Group On') ||
												(item.Status.indexOf('Set ') == 0)
											) {
												img = '<img src="images/motion48-on.png" onclick="ShowLightLog(' + item.idx + ',\'' + escape(item.Name) + '\', \'#dashcontent\', \'ShowFavorites\');" class="lcursor" height="40" width="40">';
												$(id + " #div.item").removeClass('off-motion').addClass('on-motion');
											}
											else {
												img = '<img src="images/motion48-off.png" onclick="ShowLightLog(' + item.idx + ',\'' + escape(item.Name) + '\', \'#dashcontent\', \'ShowFavorites\');" class="lcursor" height="40" width="40">';
												$(id + " #div.item").removeClass('on-motion').addClass('off-motion');
											}
										}
										else if (item.SwitchType == "Smoke Detector") {
											if (
												(item.Status == "Panic") ||
												(item.Status == "On")
											) {
												img = '<img src="images/smoke48on.png" onclick="ShowLightLog(' + item.idx + ',\'' + escape(item.Name) + '\', \'#dashcontent\', \'ShowFavorites\');" class="lcursor" height="40" width="40">';
											}
											else {
												img = '<img src="images/smoke48off.png" onclick="ShowLightLog(' + item.idx + ',\'' + escape(item.Name) + '\', \'#dashcontent\', \'ShowFavorites\');" class="lcursor" height="40" width="40">';
											}
										}
										else if (item.SwitchType == "Selector") {
											if ((item.Status === "Off")) {
												img += '<img src="images/' + item.Image + '48_Off.png" height="40" width="40">';
												$(id + " #div.item").removeClass('on-selector').addClass('off-selector');
											} else if (item.LevelOffHidden) {
												img += '<img src="images/' + item.Image + '48_On.png" height="40" width="40">';
												$(id + " #div.item").removeClass('off-selector').addClass('on-selector');
											} else {
												img += '<img src="images/' + item.Image + '48_On.png" title="' + $.t("Turn Off") + '" onclick="SwitchLight(' + item.idx + ',\'Off\',RefreshFavorites,' + item.Protected + ');" class="lcursor" height="40" width="40">';
											}
										}
										else if ((item.SubType.indexOf("Itho") == 0) || (item.SubType.indexOf("Lucci") == 0)) {
											img = $(id + " #img").html();
										}
										else {
											if (
												(item.Status == 'On') ||
												(item.Status == 'Chime') ||
												(item.Status.indexOf('Down') != -1) ||
												(item.Status.indexOf('Up') != -1) ||
												(item.Status == 'Group On') ||
												(item.Status.indexOf('Set ') == 0)
											) {
												if (item.Type == "Thermostat 3") {
													img = '<img src="images/' + item.Image + '48_On.png" onclick="ShowTherm3Popup(event, ' + item.idx + ', \'RefreshFavorites\',' + item.Protected + ');" class="lcursor" height="40" width="40">';
												}
												else {
													img = '<img src="images/' + item.Image + '48_On.png" title="' + $.t("Turn Off") + '" onclick="SwitchLight(' + item.idx + ',\'Off\',RefreshFavorites,' + item.Protected + ');" class="lcursor" height="40" width="40">';
												}
											}
											else {
												if (item.Type == "Thermostat 3") {
													img = '<img src="images/' + item.Image + '48_Off.png" onclick="ShowTherm3Popup(event, ' + item.idx + ', \'RefreshFavorites\',' + item.Protected + ');" class="lcursor" height="40" width="40">';
												}
												else {
													img = '<img src="images/' + item.Image + '48_Off.png" title="' + $.t("Turn On") + '" onclick="SwitchLight(' + item.idx + ',\'On\',RefreshFavorites,' + item.Protected + ');" class="lcursor" height="40" width="40">';
												}
											}
										}
										var backgroundClass = $rootScope.GetItemBackgroundStatus(item);
										$(id + " #bstatus").removeClass('statusNormal').removeClass('statusProtected').removeClass('statusTimeout').removeClass('statusLowBattery');
										$(id + " #bstatus").addClass(backgroundClass);

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
										if ($(id + " #bigtext > span").html() != bigtext) {
											$(id + " #bigtext > span").html(bigtext);
										}
										if ((typeof $(id + " #status") != 'undefined') && ($(id + " #status > span").html() != status)) {
											$(id + " #status > span").html(status);
										}
										if ($(id + " #lastupdate > span").html() != item.LastUpdate) {
											$(id + " #lastupdate > span").html(item.LastUpdate);
										}
										if ($scope.config.ShowUpdatedEffect == true) {
											$(id + " #name").effect("highlight", { color: '#EEFFEE' }, 1000);
										}
									}
								}
							}
						}); //light devices

						//Temperature Sensors
						$.each(data.result, function (i, item) {
							if (
								((typeof item.Temp != 'undefined') || (typeof item.Humidity != 'undefined') || (typeof item.Chill != 'undefined')) &&
								(item.Favorite != 0)
							) {
								id = "#dashcontent #temp_" + item.idx;
								var obj = $(id);
								if (typeof obj != 'undefined') {
									var vname = item.Name;
									if (($scope.config.DashboardType == 2) || (window.myglobals.ismobile == true)) {
										var vname = '<img src="images/next.png" onclick="ShowTempLog(\'#dashcontent\',\'ShowFavorites\',' + item.idx + ',\'' + escape(item.Name) + '\');" height="16" width="16">' + " " + item.Name;
									}
									if (($scope.config.DashboardType == 2) || (window.myglobals.ismobile == true)) {
										var status = "";
										var bHaveBefore = false;
										if (typeof item.Temp != 'undefined') {
											status += item.Temp + '&deg; ' + $scope.config.TempSign;
											bHaveBefore = true;
										}
										if (typeof item.Chill != 'undefined') {
											if (status != "") {
												status += ', ';
											}
											status += $.t('Chill') + ': ' + item.Chill + '&deg; ' + $scope.config.TempSign;
											bHaveBefore = true;
										}
										if (typeof item.Humidity != 'undefined') {
											if (bHaveBefore == true) {
												status += ', ';
											}
											status += item.Humidity + ' %';
										}
										if (typeof item.HumidityStatus != 'undefined') {
											status += ' (' + $.t(item.HumidityStatus) + ')';
										}
										if (typeof item.DewPoint != 'undefined') {
											status += "<br>" + $.t("Dew Point") + ": " + item.DewPoint + '&deg; ' + $scope.config.TempSign;
										}
										if ($(id + " #status").html() != status) {
											$(id + " #status").html(status);
										}
									}
									else {
										//Normal/Compact
										var img = '<img src="images/';
										if (typeof item.Temp != 'undefined') {
											img += GetTemp48Item(item.Temp);
										}
										else {
											if (item.Type == "Humidity") {
												img += "gauge48.png";
											}
											else {
												img += GetTemp48Item(item.Chill);
											}
										}
										img += '" class="lcursor" onclick="ShowTempLog(\'#dashcontent\',\'ShowFavorites\',' + item.idx + ',\'' + escape(item.Name) + '\');" height="40" width="40">';
										if ($(id + " #img").html() != img) {
											$(id + " #img").html(img);
										}
										var status = "";
										var bigtext = "";
										var bHaveBefore = false;
										if (typeof item.Temp != 'undefined') {
											bigtext = item.Temp + '\u00B0 ' + $scope.config.TempSign;
										}
										if (typeof item.Chill != 'undefined') {
											if (bigtext != "") {
												bigtext += ' / ';
											}
											bigtext += item.Chill + '\u00B0 ' + $scope.config.TempSign;
										}
										if (typeof item.Humidity != 'undefined') {
											if (bigtext != "") {
												bigtext += ' / ';
											}
											bigtext += item.Humidity + '%';
										}
										if (typeof item.HumidityStatus != 'undefined') {
											status += $.t(item.HumidityStatus);
											bHaveBefore = true;
										}
										if (typeof item.DewPoint != 'undefined') {
											if (bHaveBefore == true) {
												status += ", ";
											}
											status += $.t("Dew Point") + ": " + item.DewPoint + '&deg; ' + $scope.config.TempSign;
										}

										$(id + " #name").addClass('statusNormal');
										if (item.HaveTimeout == true) {
											$(id + " #name").removeClass('statusNormal');
											$(id + " #name").addClass('statusTimeout');
										} else {
											var BatteryLevel = parseInt(item.BatteryLevel);
											if (BatteryLevel != 255) {
												if (BatteryLevel <= 10) {
													$(id + " #name").removeClass('statusNormal');
													$(id + " #name").addClass('statusLowBattery');
												}
											}
										}
										if ($(id + " #status > span").html() != status) {
											$(id + " #status > span").html(status);
										}
										if (typeof $(id + " #bigtext > span") != 'undefined') {
											if ($(id + " #bigtext > span").html() != bigtext) {
												$(id + " #bigtext > span").html(bigtext);
											}
										}
										if ($(id + " #lastupdate > span").html() != item.LastUpdate) {
											$(id + " #lastupdate > span").html(item.LastUpdate);
										}

										var backgroundClass = $rootScope.GetItemBackgroundStatus(item);
										$(id + " #bstatus").removeClass('statusNormal').removeClass('statusProtected').removeClass('statusTimeout').removeClass('statusLowBattery');
										$(id + " #bstatus").addClass(backgroundClass);

										if ($scope.config.ShowUpdatedEffect == true) {
											$(id + " #name").effect("highlight", { color: '#EEFFEE' }, 1000);
										}
									}
								}
							}
						}); //temp devices

						//Weather Sensors
						$.each(data.result, function (i, item) {
							if (
								((typeof item.Rain != 'undefined') || (typeof item.Visibility != 'undefined') || (typeof item.UVI != 'undefined') || (typeof item.Radiation != 'undefined') || (typeof item.Direction != 'undefined') || (typeof item.Barometer != 'undefined')) &&
								(item.Favorite != 0)
							) {
								id = "#dashcontent #weather_" + item.idx;
								var obj = $(id);
								if (typeof obj != 'undefined') {
									if (($scope.config.DashboardType == 2) || (window.myglobals.ismobile == true)) {
										var status = "";
										if (typeof item.Rain != 'undefined') {
											status = item.Rain + ' mm';
											if (typeof item.RainRate != 'undefined') {
												if (item.RainRate != 0) {
													status += ', Rate: ' + item.RainRate + ' mm/h';
												}
											}
										}
										else if (typeof item.Visibility != 'undefined') {
											status = item.Data;
										}
										else if (typeof item.UVI != 'undefined') {
											status = item.UVI + ' UVI';
										}
										else if (typeof item.Radiation != 'undefined') {
											status = item.Data;
										}
										else if (typeof item.Direction != 'undefined') {
											img = '<img src="images/Wind' + item.DirectionStr + '.png" height="40" width="40">';
											status = item.Direction + ' ' + item.DirectionStr;
											if (typeof item.Speed != 'undefined') {
												status += ', ' + $.t('Speed') + ': ' + item.Speed + ' ' + $scope.config.WindSign;
											}
											if (typeof item.Gust != 'undefined') {
												status += ', ' + $.t('Gust') + ': ' + item.Gust + ' ' + $scope.config.WindSign;
											}
										}
										else if (typeof item.Barometer != 'undefined') {
											if (typeof item.ForecastStr != 'undefined') {
												status = item.Barometer + ' hPa, ' + $.t('Prediction') + ': ' + $.t(item.ForecastStr);

												var clistClass = $(id + " #div.item").attr('class');
												if (typeof clistClass != 'undefined') {
													var classList = clistClass.split(' ');
													$.each(classList, function (cssid, cssitem) {
														if (item.indexOf('itlookstobe-') == 0) $(id + " #div.item").removeClass(item);
													});
													$(id + " #div.item").addClass('itslookstobe-' + item.ForecastStr);
												}
											}
											else {
												status = item.Barometer + ' hPa';
											}
											if (typeof item.Altitude != 'undefined') {
												status += ', Altitude: ' + item.Altitude + ' meter';
											}
										}
										if ($(id + " #status").html() != status) {
											$(id + " #status").html(status);
										}
									}
									else {
										//normal/compact
										var img = "";
										var status = "";
										var bigtext = "";
										if (typeof item.Rain != 'undefined') {
											img = '<img src="images/rain48.png" class="lcursor" onclick="ShowRainLog(\'#dashcontent\',\'ShowFavorites\',' + item.idx + ',\'' + escape(item.Name) + '\');" height="40" width="40">';
											status = item.Rain + ' mm';
											bigtext = item.Rain + ' mm';
											if (typeof item.RainRate != 'undefined') {
												if (item.RainRate != 0) {
													status += ', Rate: ' + item.RainRate + ' mm/h';
												}
											}
										}
										else if (typeof item.Visibility != 'undefined') {
											img = '<img src="images/visibility48.png" height="40" width="40">';
											status = item.Data;
											bigtext = item.Data;
										}
										else if (typeof item.UVI != 'undefined') {
											img = '<img src="images/uv48.png" class="lcursor" onclick="ShowUVLog(\'#dashcontent\',\'ShowFavorites\',' + item.idx + ',\'' + escape(item.Name) + '\');" height="40" width="40">';
											status = item.UVI + ' UVI';
											bigtext = item.UVI + ' UVI';
											if (typeof item.Temp != 'undefined') {
												status += ', Temp: ' + item.Temp + '&deg; ' + $scope.config.TempSign;
											}
										}
										else if (typeof item.Radiation != 'undefined') {
											img = '<img src="images/radiation48.png" height="40" width="40">';
											status = item.Data;
											bigtext = item.Data;
										}
										else if (typeof item.Direction != 'undefined') {
											img = '<img src="images/Wind' + item.DirectionStr + '.png" class="lcursor" onclick="ShowWindLog(\'#dashcontent\',\'ShowFavorites\',' + item.idx + ',\'' + escape(item.Name) + '\');" height="40" width="40">';
											status = item.Direction + ' ' + item.DirectionStr;
											if (typeof item.Speed != 'undefined') {
												status += ', ' + $.t('Speed') + ': ' + item.Speed + ' ' + $scope.config.WindSign;
											}
											if (typeof item.Gust != 'undefined') {
												status += ', ' + $.t('Gust') + ': ' + item.Gust + ' ' + $scope.config.WindSign;
											}
											status += '<br>\n';
											if (typeof item.Temp != 'undefined') {
												status += $.t('Temp') + ': ' + item.Temp + '&deg; ' + $scope.config.TempSign;
											}
											if (typeof item.Chill != 'undefined') {
												if (typeof item.Temp != 'undefined') {
													status += ', ';
												}
												status += $.t('Chill') + ': ' + item.Chill + '&deg; ' + $scope.config.TempSign;
											}
											bigtext = item.DirectionStr;
											if (typeof item.Speed != 'undefined') {
												bigtext += ' / ' + item.Speed + ' ' + $scope.config.WindSign;
											}
											else if (typeof item.Gust != 'undefined') {
												bigtext += ' / ' + item.Gust + ' ' + $scope.config.WindSign;
											}
										}
										else if (typeof item.Barometer != 'undefined') {
											img = '<img src="images/baro48.png" class="lcursor" onclick="ShowBaroLog(\'#dashcontent\',\'ShowFavorites\',' + item.idx + ',\'' + escape(item.Name) + '\');" height="40" width="40">';
											bigtext = item.Barometer + ' hPa';
											if (typeof item.ForecastStr != 'undefined') {
												status = item.Barometer + ' hPa, ' + $.t('Prediction') + ': ' + $.t(item.ForecastStr);
											}
											else {
												status = item.Barometer + ' hPa';
											}
											if (typeof item.Altitude != 'undefined') {
												status += ', Altitude: ' + item.Altitude + ' meter';
											}
										}

										$(id + " #name").addClass('statusNormal');
										if (item.HaveTimeout == true) {
											$(id + " #name").removeClass('statusNormal');
											$(id + " #name").addClass('statusTimeout');
										} else {
											var BatteryLevel = parseInt(item.BatteryLevel);
											if (BatteryLevel != 255) {
												if (BatteryLevel <= 10) {
													$(id + " #name").removeClass('statusNormal');
													$(id + " #name").addClass('statusLowBattery');
												}
											}
										}

										if ($(id + " #img").html() != img) {
											$(id + " #img").html(img);
										}
										if ($(id + " #status > span").html() != status) {
											$(id + " #status > span").html(status);
										}
										if ($(id + " #bigtext > span").html() != bigtext) {
											$(id + " #bigtext > span").html(bigtext);
										}
										if ($(id + " #lastupdate > span").html() != item.LastUpdate) {
											$(id + " #lastupdate > span").html(item.LastUpdate);
										}
										var backgroundClass = $rootScope.GetItemBackgroundStatus(item);
										$(id + " #bstatus").removeClass('statusNormal').removeClass('statusProtected').removeClass('statusTimeout').removeClass('statusLowBattery');
										$(id + " #bstatus").addClass(backgroundClass);

										if ($scope.config.ShowUpdatedEffect == true) {
											$(id + " #name").effect("highlight", { color: '#EEFFEE' }, 1000);
										}
									}
								}
							}
						}); //weather devices

						//security devices
						$.each(data.result, function (i, item) {
							if ((item.Type.indexOf('Security') == 0) && (item.Favorite != 0)) {
								id = "#dashcontent #security_" + item.idx;
								var obj = $(id);
								if (typeof obj != 'undefined') {
									if (($scope.config.DashboardType == 2) || (window.myglobals.ismobile == true)) {
										var tmpStatus = TranslateStatus(item.Status);
										if (item.SubType == "Security Panel") {
											tmpStatus += ' <a href="secpanel/"><img src="images/security48.png" class="lcursor" height="16" width="16"></a>';
										}
										else if (item.SubType.indexOf('remote') > 0) {
											if ((item.Status.indexOf('Arm') >= 0) || (item.Status.indexOf('Panic') >= 0)) {
												tmpStatus += ' <img src="images/remote.png" title="' + $.t("Turn Alarm Off") + '" onclick="SwitchLight(' + item.idx + ',\'Off\',RefreshFavorites,' + item.Protected + ');" class="lcursor" height="16" width="16">';
											}
											else {
												tmpStatus += ' <img src="images/remote.png" title="' + $.t("Turn Alarm On") + '" onclick="ArmSystem(' + item.idx + ',\'On\',RefreshFavorites,' + item.Protected + ');" class="lcursor" height="16" width="16">';
											}
										}

										if ($(id + " #bigtext").html() != tmpStatus) {
											$(id + " #bigtext").html(tmpStatus);
										}
									}
									else {
										//normal/compact
										var img = "";
										if (item.SubType == "Security Panel") {
											img = '<a href="secpanel/"><img src="images/security48.png" class="lcursor" height="40" width="40"></a>';
										}
										else if (item.SubType.indexOf('remote') > 0) {
											if ((item.Status.indexOf('Arm') >= 0) || (item.Status.indexOf('Panic') >= 0)) {
												img += '<img src="images/remote48.png" title="' + $.t("Turn Alarm Off") + '" onclick="SwitchLight(' + item.idx + ',\'Off\',RefreshFavorites,' + item.Protected + ');" class="lcursor" height="40" width="40">\n';
											}
											else {
												img += '<img src="images/remote48.png" title="' + $.t("Turn Alarm On") + '" onclick="ArmSystem(' + item.idx + ',\'On\',RefreshFavorites,' + item.Protected + ');" class="lcursor" height="40" width="40">\n';
											}
										}
										else if (item.SwitchType == "Smoke Detector") {
											if (
												(item.Status == "Panic") ||
												(item.Status == "On")
											) {
												img = '<img src="images/smoke48on.png" title="' + $.t("Turn Alarm Off") + '" onclick="SwitchLight(' + item.idx + ',\'On\',RefreshFavorites,' + item.Protected + ');" class="lcursor" height="40" width="40">';
											}
											else {
												img = '<img src="images/smoke48off.png" title="' + $.t("Turn Alarm On") + '" onclick="SwitchLight(' + item.idx + ',\'On\',RefreshFavorites,' + item.Protected + ');" class="lcursor" height="40" width="40">';
											}
										}
										else if (item.SubType == "X10 security") {
											if (item.Status.indexOf('Normal') >= 0) {
												img += '<img src="images/security48.png" title="' + $.t("Turn Alarm On") + '" onclick="SwitchLight(' + item.idx + ',\'' + ((item.Status == "Normal Delayed") ? "Alarm Delayed" : "Alarm") + '\',RefreshFavorites,' + item.Protected + ');" class="lcursor" height="40" width="40">';
											}
											else {
												img += '<img src="images/Alarm48_On.png" title="' + $.t("Turn Alarm Off") + '" onclick="SwitchLight(' + item.idx + ',\'' + ((item.Status == "Alarm Delayed") ? "Normal Delayed" : "Normal") + '\',RefreshFavorites,' + item.Protected + ');" class="lcursor" height="40" width="40">';
											}
										}
										else if (item.SubType == "X10 security motion") {
											if ((item.Status == "No Motion")) {
												img += '<img src="images/security48.png" title="' + $.t("Turn Alarm On") + '" onclick="SwitchLight(' + item.idx + ',\'Motion\',RefreshFavorites,' + item.Protected + ');" class="lcursor" height="40" width="40">';
											}
											else {
												img += '<img src="images/Alarm48_On.png" title="' + $.t("Turn Alarm Off") + '" onclick="SwitchLight(' + item.idx + ',\'No Motion\',RefreshFavorites,' + item.Protected + ');" class="lcursor" height="40" width="40">';
											}
										}
										else if ((item.Status.indexOf('Alarm') >= 0) || (item.Status.indexOf('Tamper') >= 0)) {
											img = '<img src="images/Alarm48_On.png" height="40" width="40">';
										}
										else {
											if (item.SubType.indexOf('Meiantech') >= 0) {
												if ((item.Status.indexOf('Arm') >= 0) || (item.Status.indexOf('Panic') >= 0)) {
													img = '<img src="images/security48.png" title="' + $.t("Turn Alarm Off") + '" onclick="SwitchLight(' + item.idx + ',\'Off\',RefreshFavorites,' + item.Protected + ');" class="lcursor" height="40" width="40">';
												}
												else {
													img = '<img src="images/security48.png" title="' + $.t("Turn Alarm On") + '" onclick="ArmSystemMeiantech(' + item.idx + ',\'On\',RefreshFavorites,' + item.Protected + ');" class="lcursor" height="40" width="40">';
												}
											}
											else {
												if (item.SubType.indexOf('KeeLoq') >= 0) {
													img = '<img src="images/pushon48.png" title="' + $.t("Turn On") + '" onclick="SwitchLight(' + item.idx + ',\'On\',RefreshFavorites,' + item.Protected + ');" class="lcursor" height="40" width="40">';
												}
												else {
													img = '<img src="images/security48.png" height="40" width="40">';
												}
											}
										}

										var backgroundClass = $rootScope.GetItemBackgroundStatus(item);
										$(id + " #bstatus").removeClass('statusNormal').removeClass('statusProtected').removeClass('statusTimeout').removeClass('statusLowBattery');
										$(id + " #bstatus").addClass(backgroundClass);

										if ($(id + " #img").html() != img) {
											$(id + " #img").html(img);
										}
										if ($(id + " #bigtext > span").html() != TranslateStatus(item.Status)) {
											$(id + " #bigtext > span").html(TranslateStatus(item.Status));
										}
										if ($(id + " #lastupdate > span").html() != item.LastUpdate) {
											$(id + " #lastupdate > span").html(item.LastUpdate);
										}
										if ($scope.config.ShowUpdatedEffect == true) {
											$(id + " #name").effect("highlight", { color: '#EEFFEE' }, 1000);
										}
									}
								}
							}
						}); //security devices
/*
						//Gizmocuz: Don't know how did this ? But this should be under utility devices!
						//Please do so

						//evohome devices
						$.each(data.result, function (i, item) {
							if ((item.Type.indexOf('Heating') == 0) && (item.Favorite != 0)) {
								id = "#dashcontent #evohome_" + item.idx;
								var obj = $(id);
								if (typeof obj != 'undefined') {
									if (($scope.config.DashboardType == 2) || (window.myglobals.ismobile == true)) {
										var img = "";
										if (item.SubType == "Evohome") {
											img += EvohomeImg(item, 'evomobile');

											if ($(id + " #img").html() != img) {
												$(id + " #img").html(img);
											}
										}
									}
									else {
										var img = "";
										if (item.SubType == "Evohome") {
											img = EvohomeImg(item, 'evomini');

											$(id + " #name").addClass('statusNormal');
											if (item.HaveTimeout == true) {
												$(id + " #name").removeClass('statusNormal');
												$(id + " #name").addClass('statusTimeout');
											}
											else if (item.Protected == true) {
												$(id + " #name").removeClass('statusNormal');
												$(id + " #name").addClass('statusProtected');
											}

											if ($(id + " #img").html() != img) {
												$(id + " #img").html(img);
											}
											if ($(id + " #bigtext > span").html() != TranslateStatus(EvoDisplayTextMode(item.Status))) {
												$(id + " #bigtext > span").html(TranslateStatus(EvoDisplayTextMode(item.Status)));
											}
											if ($(id + " #lastupdate > span").html() != item.LastUpdate) {
												$(id + " #lastupdate > span").html(item.LastUpdate);
											}
											if ($scope.config.ShowUpdatedEffect == true) {
												$(id + " #name").effect("highlight", { color: '#EEFFEE' }, 1000);
											}
										}
									}
								}
							}
						}); //evohome devices
*/
						//Utility Sensors
						$.each(data.result, function (i, item) {
							if (
								(
									(typeof item.Counter != 'undefined') ||
									(item.Type == "Current") ||
									(item.Type == "Energy") ||
									(item.Type == "Current/Energy") ||
									(item.Type == "Power") ||
									(item.Type == "Air Quality") ||
									(item.Type == "Lux") ||
									(item.Type == "Weight") ||
									(item.Type == "Usage") ||
									(item.SubType == "Percentage") ||
									//((item.Type=="Fan")&&(typeof item.SubType != 'undefined')&&(item.SubType.indexOf('Lucci')!=0))||
									((item.Type == "Thermostat") && (item.SubType == "SetPoint")) ||
									(item.SubType == "kWh") ||
									(item.SubType == "Soil Moisture") ||
									(item.SubType == "Leaf Wetness") ||
									(item.SubType == "Voltage") ||
									(item.SubType == "Distance") ||
									(item.SubType == "Current") ||
									(item.SubType == "Text") ||
									(item.SubType == "Alert") ||
									(item.SubType == "Pressure") ||
									(item.SubType == "A/D") ||
									(item.SubType == "Thermostat Mode") ||
									(item.SubType == "Thermostat Fan Mode") ||
									(item.SubType == "Smartwares") ||
									(item.SubType == "Waterflow") ||
									(item.SubType == "Sound Level") ||
									(item.SubType == "Custom Sensor")
								) &&
								(item.Favorite != 0)
							) {
								id = "#dashcontent #utility_" + item.idx;
								var obj = $(id);
								if (typeof obj != 'undefined') {
									if (($scope.config.DashboardType == 2) || (window.myglobals.ismobile == true)) {
										var status = "";
										if (typeof item.Counter != 'undefined') {
											if ($scope.config.DashboardType == 0) {
												status += '' + $.t("Usage") + ': ' + item.CounterToday;
											}
											else {
												if (typeof item.CounterDeliv == 'undefined') {
													status = item.CounterToday;
												} else {
													if ((typeof item.CounterDeliv != 'undefined') && (item.CounterDeliv != 0)) {
														status += 'U: T: ' + item.CounterToday;
													} else {
														status += 'T: ' + item.CounterToday;
													}
												}
											}
										}
										else if (item.Type == "Current") {
											status += item.Data;
										}
										else if (
											(item.Type == "Energy") ||
											(item.Type == "Current/Energy") ||
											(item.Type == "Power") ||
											(item.SubType == "kWh") ||
											(item.Type == "Air Quality") ||
											(item.Type == "Lux") ||
											(item.Type == "Weight") ||
											(item.Type == "Usage") ||
											(item.SubType == "Percentage") ||
											(item.Type == "Fan") ||
											(item.SubType == "Soil Moisture") ||
											(item.SubType == "Leaf Wetness") ||
											(item.SubType == "Voltage") ||
											(item.SubType == "Distance") ||
											(item.SubType == "Current") ||
											(item.SubType == "Text") ||
											(item.SubType == "Pressure") ||
											(item.SubType == "A/D") ||
											(item.SubType == "Waterflow") ||
											(item.SubType == "Sound Level") ||
											(item.SubType == "Custom Sensor")
										) {
											if (typeof item.CounterToday != 'undefined') {
												status += 'T: ' + item.CounterToday;
											}
											else {
												status = item.Data;
											}
										}
										else if (item.SubType == "Alert") {
											var aLevel = item.Level;
											if (aLevel > 4) aLevel = 4;
											status = item.Data + ' <img src="images/Alert48_' + aLevel + '.png" height="16" width="16">';
										}
										else if ((item.Type == "Thermostat") && (item.SubType == "SetPoint")) {
											status += item.Data + '\u00B0 ' + $scope.config.TempSign;
										}
										else if (item.SubType == "Smartwares") {
											status += item.Data + '\u00B0 ' + $scope.config.TempSign;
										}

										var bHaveReturnUsage = false;
										if (typeof item.CounterDeliv != 'undefined') {
											if (item.UsageDeliv.charAt(0) != 0) {
												bHaveReturnUsage = true;
											}
										}
										if (!bHaveReturnUsage) {
											if (typeof item.Usage != 'undefined') {
												if ($scope.config.DashboardType == 0) {
													status += '<br>' + $.t("Actual") + ': ' + item.Usage;
												}
												else {
													status += ", A: " + item.Usage;
												}
											}
										}
										if (typeof item.CounterDeliv != 'undefined') {
											if (item.CounterDeliv != 0) {
												if ($scope.config.DashboardType == 0) {
													status += '<br>' + $.t("Return") + ': ' + item.CounterDelivToday;
													status += '<br>' + $.t("Actual") + ': ' + item.UsageDeliv;
												}
												else {
													status += '<br>R: T: ' + item.CounterDelivToday;
													if (bHaveReturnUsage) {
														status += ", A: ";
														if (parseInt(item.UsageDeliv) > 0) {
															status += "-";
														}
														status += item.UsageDeliv;
													}
												}
											}
										}
										if ($(id + " #status").html() != status) {
											$(id + " #status").html(status);
										}
									}
									else {
										//normal/compact
										var status = "";
										var bigtext = "";
										var img = "";

										if (typeof item.Counter != 'undefined') {
											if ((item.SubType == "Gas") || (item.SubType == "RFXMeter counter")) {
												status = "&nbsp;";
												bigtext = item.CounterToday;
											}
											else {
												status = '' + $.t("Usage") + ': ' + item.CounterToday;
											}
										}
										else if (item.Type == "Current") {
											status = item.Data;
											bigtext = item.Data;
										}
										else if ((item.Type == "Energy") || (item.Type == "Current/Energy") || (item.Type == "Power") || (item.SubType == "kWh")) {
											status = item.Data;
										}
										else if (item.Type == "Air Quality") {
											status = item.Data + " (" + item.Quality + ")";
											bigtext = item.Data;
										}
										else if (item.SubType == "Percentage") {
											status = item.Data;
											bigtext = item.Data;
										}
										else if (item.SubType == "Custom Sensor") {
											status = item.Data;
											bigtext = item.Data;
										}
										else if (item.Type == "Fan") {
											status = item.Data;
											bigtext = item.Data;
										}
										else if (item.SubType == "Soil Moisture") {
											status = item.Data;
											bigtext = item.Data;
										}
										else if (item.SubType == "Leaf Wetness") {
											status = item.Data;
											bigtext = item.Data;
										}
										else if ((item.SubType == "Voltage") || (item.SubType == "Current") || (item.SubType == "Distance") || (item.SubType == "A/D") || (item.SubType == "Pressure") || (item.SubType == "Sound Level")) {
											status = item.Data;
											bigtext = item.Data;
										}
										else if (item.SubType == "Text") {
											status = item.Data;
										}
										else if (item.SubType == "Alert") {
											status = item.Data;
											var aLevel = item.Level;
											if (aLevel > 4) aLevel = 4;
											img = '<img src="images/Alert48_' + aLevel + '.png" height="40" width="40">';
										}
										else if (item.Type == "Lux") {
											status = item.Data;
											bigtext = item.Data;
										}
										else if (item.Type == "Weight") {
											status = item.Data;
											bigtext = item.Data;
										}
										else if (item.Type == "Usage") {
											status = item.Data;
											bigtext = item.Data;
										}
										else if ((item.Type == "Thermostat") && (item.SubType == "SetPoint")) {
											status = item.Data + '\u00B0 ' + $scope.config.TempSign;
											bigtext = item.Data + '\u00B0 ' + $scope.config.TempSign;
										}
										else if (item.SubType == "Smartwares") {
											status = item.Data + '\u00B0 ' + $scope.config.TempSign;
											bigtext = item.Data + '\u00B0 ' + $scope.config.TempSign;
										}
										else if ((item.SubType == "Thermostat Mode") || (item.SubType == "Thermostat Fan Mode")) {
											status = item.Data;
											bigtext = item.Data;
										}
										else if (item.SubType == "Waterflow") {
											status = item.Data;
											bigtext = item.Data;
										}
										if (typeof item.Usage != 'undefined') {
											bigtext = item.Usage;
											if (item.Type != "P1 Smart Meter") {
												if ($scope.config.DashboardType == 0) {
													if (typeof item.CounterToday != 'undefined') {
														status += '<br>' + $.t("Today") + ': ' + item.CounterToday;
													}
												}
												else {
													if (typeof item.CounterToday != 'undefined') {
														status += ', T: ' + item.CounterToday;
													}
												}
											}
										}
										if (typeof item.CounterDeliv != 'undefined') {
											if (item.CounterDeliv != 0) {
												if (item.UsageDeliv.charAt(0) != 0) {
													bigtext = '-' + item.UsageDeliv;
												}
												status += '<br>';
												if (($scope.config.DashboardType == 2) || (window.myglobals.ismobile == true)) {
													status += 'R: ' + item.CounterDelivToday;
												}
												else {
													status += '' + $.t("Return") + ': ' + item.CounterDelivToday;
												}
											}
										}

										var backgroundClass = $rootScope.GetItemBackgroundStatus(item);
										$(id + " #bstatus").removeClass('statusNormal').removeClass('statusProtected').removeClass('statusTimeout').removeClass('statusLowBattery');
										$(id + " #bstatus").addClass(backgroundClass);

										if ($(id + " #status > span").html() != status) {
											$(id + " #status > span").html(status);
										}
										if ($(id + " #bigtext > span").html() != bigtext) {
											$(id + " #bigtext > span").html(bigtext);
										}
										if ($(id + " #lastupdate > span").html() != item.LastUpdate) {
											$(id + " #lastupdate > span").html(item.LastUpdate);
										}
										if (img != "") {
											if ($(id + " #img").html() != img) {
												$(id + " #img").html(img);
											}
										}
										if ($scope.config.ShowUpdatedEffect == true) {
											$(id + " #name").effect("highlight", { color: '#EEFFEE' }, 1000);
										}
									}
								}
							}
						}); //Utility devices
					}
				}
			});
			$scope.mytimer = $interval(function () {
				RefreshFavorites();
			}, 10000);
		}

		ShowFavorites = function () {
			if (typeof $scope.mytimer != 'undefined') {
				$interval.cancel($scope.mytimer);
				$scope.mytimer = undefined;
			}
			var totdevices = 0;
			var jj = 0;
			var bHaveAddedDivider = false;
			var htmlcontent = "";
			var bShowRoomplan = false;
			$.RoomPlans = [];

			$("body").removeClass();
			$("body").addClass("dashboard");

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

			var bFavorites = 1;
			if (typeof window.myglobals.LastPlanSelected != 'undefined') {
				if (window.myglobals.LastPlanSelected > 0) {
					bFavorites = 0;
				}
			}

			$.ajax({
				url: "json.htm?type=devices&filter=all&used=true&favorite=" + bFavorites + "&order=[Order]&plan=" + window.myglobals.LastPlanSelected,
				async: false,
				dataType: 'json',
				success: function (data) {
					if (typeof data.ActTime != 'undefined') {
						$scope.LastUpdateTime = parseInt(data.ActTime);
					}
					if ($scope.config.DashboardType == 3) {
						$window.location = '/#Floorplans';
						$("body").addClass("dashFloorplan");
						return;
					}

					var rowItems = 3;
					$("body").addClass("3column");
					if ($scope.config.DashboardType == 1) {
						rowItems = 4;
						$("body").removeClass("3column").addClass("4column");
					}
					if (($scope.config.DashboardType == 2) || (window.myglobals.ismobile == true)) {
						rowItems = 1000;
						$("body").addClass("dashMobile");
					}

					if (typeof data.result != 'undefined') {

						//Scenes
						jj = 0;
						bHaveAddedDivider = false;
						$.each(data.result, function (i, item) {
							//Scenes/Groups
							if (
								(item.Type.indexOf('Scene') == 0) ||
								(item.Type.indexOf('Group') == 0)
							) {
								totdevices += 1;
								if (jj == 0) {
									//first time
									htmlcontent += '<section class="dashCategory" id="dashScenes">';
									if (($scope.config.DashboardType == 2) || (window.myglobals.ismobile == true)) {
										htmlcontent += '\t    <table class="mobileitem">\n';
										htmlcontent += '\t    <thead>\n';
										htmlcontent += '\t    <tr>\n';
										htmlcontent += '\t    		<th>' + $.t('Scenes') + '</th>\n';
										htmlcontent += '\t    		<th style="text-align:right"><a id="cLightSwitches" href="javascript:SwitchLayout(\'Scenes\')"><img src="images/next.png"></a></th>\n';
										htmlcontent += '\t    </tr>\n';
										htmlcontent += '\t    </thead>\n';
									}
									else {
										htmlcontent += '<h2>' + $.t('Scenes') + ':</h2>\n';
									}
								}
								if (jj % rowItems == 0) {
									//add divider
									if (bHaveAddedDivider == true) {
										//close previous devider
										htmlcontent += '</div>\n';
									}
									htmlcontent += '<div class="row divider">\n';
									bHaveAddedDivider = true;
								}

								var xhtm = "";
								if (($scope.config.DashboardType == 2) || (window.myglobals.ismobile == true)) {
									xhtm +=
										'\t    <tr id="scene_' + item.idx + '">\n' +
										'\t      <td id="name" class="name">' + item.Name;
									xhtm +=
										'</td>\n';
									var status = "";
									if (item.Type.indexOf('Scene') == 0) {
										status += '<button class="btn btn-mini" type="button" onclick="SwitchScene(' + item.idx + ',\'On\',RefreshFavorites, ' + item.Protected + ');">' + $.t("On") + '</button>';
									}
									else {
										if (item.Status == 'Off') {
											status += '<button class="btn btn-mini" type="button" onclick="SwitchScene(' + item.idx + ',\'On\',RefreshFavorites, ' + item.Protected + ');">' + $.t("On") + '</button> ' +
												'<button class="btn btn-mini btn-info" type="button" onclick="SwitchScene(' + item.idx + ',\'Off\',RefreshFavorites, ' + item.Protected + ');">' + $.t("Off") + '</button>';
										}
										else {
											status += '<button class="btn btn-mini btn-info" type="button" onclick="SwitchScene(' + item.idx + ',\'On\',RefreshFavorites, ' + item.Protected + ');">' + $.t("On") + '</button> ' +
												'<button class="btn btn-mini" type="button" onclick="SwitchScene(' + item.idx + ',\'Off\',RefreshFavorites, ' + item.Protected + ');">' + $.t("Off") + '</button>';
										}
									}
									xhtm +=
										'\t      <td id="status" class="status">' + status + '</td>\n' +
										'\t    </tr>\n';
								}
								else {
									if ($scope.config.DashboardType == 0) {
										xhtm = '\t<div class="span4 movable" id="scene_' + item.idx + '">\n';
									}
									else if ($scope.config.DashboardType == 1) {
										xhtm = '\t<div class="span3 movable" id="scene_' + item.idx + '">\n';
									}
									var backgroundClass = $rootScope.GetItemBackgroundStatus(item);
									xhtm += '\t  <div id="bstatus" class="item ' + backgroundClass + '">\n';
									if (item.Type.indexOf('Scene') == 0) {
										xhtm += '\t    <table id="itemtablesmall" class="itemtablesmall" border="0" cellpadding="0" cellspacing="0">\n';
									}
									else {
										xhtm += '\t    <table id="itemtablesmalldoubleicon" class="itemtablesmalldoubleicon" border="0" cellpadding="0" cellspacing="0">\n';
									}
									xhtm +=
										'\t    <t>\n' +
										'\t      <td id="name" class="name">' + item.Name + '</td>\n' +
										'\t      <td id="bigtext" class="bigtext"><span>';
									var bigtext = TranslateStatusShort(item.Status);
									if (item.UsedByCamera == true) {
										var streamimg = '<img src="images/webcam.png" title="' + $.t('Stream Video') + '" height="16" width="16">';
										streamurl = "<a href=\"javascript:ShowCameraLiveStream('" + escape(item.Name) + "','" + item.CameraIdx + "')\">" + streamimg + "</a>";
										bigtext += "&nbsp;" + streamurl;
									}
									xhtm += bigtext + '</span></td>\n';
									if (item.Type.indexOf('Scene') == 0) {
										xhtm += '<td id="img1" class="img img1"><img src="images/push48.png" title="Activate" onclick="SwitchScene(' + item.idx + ',\'On\',RefreshFavorites, ' + item.Protected + ');" class="lcursor" height="40" width="40"></td>\n';
										xhtm += '\t      <td id="status" class="status"></td>\n';
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
										xhtm += '<td id="img1" class="img img1"><img class="lcursor ' + onclass + '" src="images/push48.png" title="' + $.t("Turn On") + '" onclick="SwitchScene(' + item.idx + ',\'On\',RefreshFavorites, ' + item.Protected + ');" height="40" width="40"></td>\n';
										xhtm += '<td id="img2" class="img2"><img class="lcursor ' + offclass + '"src="images/pushoff48.png" title="' + $.t("Turn Off") + '" onclick="SwitchScene(' + item.idx + ',\'Off\',RefreshFavorites, ' + item.Protected + ');" height="40" width="40"></td>\n';
										xhtm += '\t      <td id="status" class="status"></td>\n';
									}
									xhtm += '\t      <td id="lastupdate" class="lastupdate"><span>' + item.LastUpdate + '</span></td>\n';
									xhtm +=
										'\t    </tr>\n' +
										'\t    </table>\n' +
										'\t  </div><!--item end-->\n' +
										'\t</div>\n';
								}
								htmlcontent += xhtm;
								jj += 1;
							}
						}); //scenes
						if (bHaveAddedDivider == true) {
							//close previous devider
							htmlcontent += '</div>\n';
						}
						if (($scope.config.DashboardType == 2) || (window.myglobals.ismobile == true)) {
							htmlcontent += '\t    </table>\n';
						}
						if (jj > 0) {
							htmlcontent += '</section>';
						}


						//light devices
						jj = 0;
						bHaveAddedDivider = false;
						$.each(data.result, function (i, item) {
							if (
								(item.Favorite != 0) && (
									(item.Type.indexOf('Light') == 0) ||
									(item.SubType == "Smartwares Mode") ||
									(item.Type.indexOf('Blind') == 0) ||
									(item.Type.indexOf('Curtain') == 0) ||
									(item.Type.indexOf('Thermostat 2') == 0) ||
									(item.Type.indexOf('Thermostat 3') == 0) ||
									(item.Type.indexOf('Chime') == 0) ||
									(item.Type.indexOf('RFY') == 0) ||
									(item.Type.indexOf('ASA') == 0) ||
									(item.SubType == "Relay") ||
									((typeof item.SubType != 'undefined') && (item.SubType.indexOf('Itho') == 0)) ||
									((typeof item.SubType != 'undefined') && (item.SubType.indexOf('Lucci') == 0)) ||
									((item.Type.indexOf('Value') == 0) && (typeof item.SwitchType != 'undefined'))
								)
							) {
								totdevices += 1;
								if (jj == 0) {
									//first time
									htmlcontent += '<section class="dashCategory" id="dashSwitches">';
									if (($scope.config.DashboardType == 2) || (window.myglobals.ismobile == true)) {
										if (htmlcontent != "") {
											htmlcontent += '<br>';
										}
										htmlcontent += '\t    <table class="mobileitem">\n';
										htmlcontent += '\t    <thead>\n';
										htmlcontent += '\t    <tr>\n';
										htmlcontent += '\t    		<th>' + $.t('Light/Switch Devices') + '</th>\n';
										htmlcontent += '\t    		<th style="text-align:right"><a id="cLightSwitches" href="javascript:SwitchLayout(\'LightSwitches\')"><img src="images/next.png"></a></th>\n';
										htmlcontent += '\t    </tr>\n';
										htmlcontent += '\t    </thead>\n';
									}
									else {
										htmlcontent += '<h2>' + $.t('Light/Switch Devices') + ':</h2>\n';
									}
								}
								if (jj % rowItems == 0) {
									//add divider
									if (bHaveAddedDivider == true) {
										//close previous devider
										htmlcontent += '</div>\n';
									}
									htmlcontent += '<div class="row divider">\n';
									bHaveAddedDivider = true;
								}
								var backgroundClass = $rootScope.GetItemBackgroundStatus(item);
								var status = "";
								var xhtm = "";
								if (($scope.config.DashboardType == 2) || (window.myglobals.ismobile == true)) {
									xhtm +=
										'\t    <tr id="light_' + item.idx + '">\n' +
										'\t      <td id="name" class="name">' + item.Name;
									xhtm +=
										'</td>\n';
									var status = TranslateStatus(item.Status) + " ";
									if (item.SwitchType == "Doorbell") {
										status += '<button class="btn btn-mini" type="button" onclick="SwitchLight(' + item.idx + ',\'On\',RefreshFavorites,' + item.Protected + ');">' + $.t("Ring") + '</button>';
									}
									else if (item.SwitchType == "Push On Button") {
										if (item.InternalState == "On") {
											status = '<button class="btn btn-mini btn-info" type="button" onclick="SwitchLight(' + item.idx + ',\'On\',RefreshFavorites,' + item.Protected + ');">' + $.t("On") + '</button>';
										}
										else {
											status = '<button class="btn btn-mini" type="button" onclick="SwitchLight(' + item.idx + ',\'On\',RefreshFavorites,' + item.Protected + ');">' + $.t("On") + '</button>';
										}
									}
									else if (item.SwitchType == "Door Contact") {
										if (item.InternalState == "Open") {
											status = '<button class="btn btn-mini btn-info" type="button" onclick="SwitchLight(' + item.idx + ',\'On\',RefreshFavorites,' + item.Protected + ');">' + $.t("Open") + '</button> ' +
												'<button class="btn btn-mini" type="button" onclick="SwitchLight(' + item.idx + ',\'On\',RefreshFavorites,' + item.Protected + ');">' + $.t("Lock") + '</button>';
										}
										else {
											status = '<button class="btn btn-mini" type="button" onclick="SwitchLight(' + item.idx + ',\'On\',RefreshFavorites,' + item.Protected + ');">' + $.t("Open") + '</button> ' +
												'<button class="btn btn-mini btn-info" type="button" onclick="SwitchLight(' + item.idx + ',\'On\',RefreshFavorites,' + item.Protected + ');">' + $.t("Locked") + '</button>';
										}
									}
									else if (item.SwitchType == "Door Lock") {
										if (item.InternalState == "Unlocked") {
											status = '<button class="btn btn-mini btn-info" type="button" onclick="SwitchLight(' + item.idx + ',\'Off\',RefreshFavorites,' + item.Protected + ');">' + $.t("Unlocked") + '</button> ' +
												'<button class="btn btn-mini" type="button" onclick="SwitchLight(' + item.idx + ',\'On\',RefreshFavorites,' + item.Protected + ');">' + $.t("Locked") + '</button>';
										}
										else {
											status = '<button class="btn btn-mini" type="button" onclick="SwitchLight(' + item.idx + ',\'Off\',RefreshFavorites,' + item.Protected + ');">' + $.t("Unlocked") + '</button> ' +
												'<button class="btn btn-mini btn-info" type="button" onclick="SwitchLight(' + item.idx + ',\'On\',RefreshFavorites,' + item.Protected + ');">' + $.t("Locked") + '</button>';
										}
									}
									else if (item.SwitchType == "Push Off Button") {
										status = '<button class="btn btn-mini" type="button" onclick="SwitchLight(' + item.idx + ',\'Off\',RefreshFavorites,' + item.Protected + ');">' + $.t("Off") + '</button>';
									}
									else if (item.SwitchType == "X10 Siren") {
										if (
											(item.Status == 'On') ||
											(item.Status == 'Chime') ||
											(item.Status == 'Group On') ||
											(item.Status == 'All On')
										) {
											status = '<button class="btn btn-mini btn-info" type="button">' + $.t("SIREN") + '</button>';
										}
										else {
											status = '<button class="btn btn-mini" type="button">' + $.t("Silence") + '</button>';
										}
									}
									else if (item.SwitchType == "Contact") {
										if (item.Status == 'Closed') {
											status = '<button class="btn btn-mini" type="button" onclick="ShowLightLog(' + item.idx + ',\'' + escape(item.Name) + '\', \'#dashcontent\', \'ShowFavorites\');">' + $.t("Closed") + '</button>';
										}
										else {
											status = '<button class="btn btn-mini btn-info" type="button" onclick="ShowLightLog(' + item.idx + ',\'' + escape(item.Name) + '\', \'#dashcontent\', \'ShowFavorites\');">' + $.t("Open") + '</button>';
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
												status =
													'<button class="btn btn-mini" type="button" onclick="SwitchLight(' + item.idx + ',\'Off\',RefreshFavorites,' + item.Protected + ');">' + $.t("Open") + '</button> ' +
													'<button class="btn btn-mini btn-danger" type="button" onclick="SwitchLight(' + item.idx + ',\'Stop\',RefreshFavorites,' + item.Protected + ');">' + $.t("Stop") + '</button> ' +
													'<button class="btn btn-mini btn-info" type="button" onclick="SwitchLight(' + item.idx + ',\'On\',RefreshFavorites,' + item.Protected + ');">' + $.t("Closed") + '</button>';
											}
											else {
												status =
													'<button class="btn btn-mini btn-info" type="button" onclick="SwitchLight(' + item.idx + ',\'Off\',RefreshFavorites,' + item.Protected + ');">' + $.t("Open") + '</button> ' +
													'<button class="btn btn-mini btn-danger" type="button" onclick="SwitchLight(' + item.idx + ',\'Stop\',RefreshFavorites,' + item.Protected + ');">' + $.t("Stop") + '</button> ' +
													'<button class="btn btn-mini" type="button" onclick="SwitchLight(' + item.idx + ',\'On\',RefreshFavorites,' + item.Protected + ');">' + $.t("Close") + '</button>';
											}
										}
										else {
											if (item.Status == 'Closed') {
												status =
													'<button class="btn btn-mini" type="button" onclick="SwitchLight(' + item.idx + ',\'Off\',RefreshFavorites,' + item.Protected + ');">' + $.t("Open") + '</button> ' +
													'<button class="btn btn-mini btn-info" type="button" onclick="SwitchLight(' + item.idx + ',\'On\',RefreshFavorites,' + item.Protected + ');">' + $.t("Closed") + '</button>';
											}
											else {
												status =
													'<button class="btn btn-mini btn-info" type="button" onclick="SwitchLight(' + item.idx + ',\'Off\',RefreshFavorites,' + item.Protected + ');">' + $.t("Open") + '</button> ' +
													'<button class="btn btn-mini" type="button" onclick="SwitchLight(' + item.idx + ',\'On\',RefreshFavorites,' + item.Protected + ');">' + $.t("Close") + '</button>';
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
												status =
													'<button class="btn btn-mini" type="button" onclick="SwitchLight(' + item.idx + ',\'On\',RefreshFavorites,' + item.Protected + ');">' + $.t("Open") + '</button> ' +
													'<button class="btn btn-mini btn-danger" type="button" onclick="SwitchLight(' + item.idx + ',\'Stop\',RefreshFavorites,' + item.Protected + ');">' + $.t("Stop") + '</button> ' +
													'<button class="btn btn-mini btn-info" type="button" onclick="SwitchLight(' + item.idx + ',\'Off\',RefreshFavorites,' + item.Protected + ');">' + $.t("Closed") + '</button>';
											}
											else {
												status =
													'<button class="btn btn-mini btn-info" type="button" onclick="SwitchLight(' + item.idx + ',\'On\',RefreshFavorites,' + item.Protected + ');">' + $.t("Open") + '</button> ' +
													'<button class="btn btn-mini btn-danger" type="button" onclick="SwitchLight(' + item.idx + ',\'Stop\',RefreshFavorites,' + item.Protected + ');">' + $.t("Stop") + '</button> ' +
													'<button class="btn btn-mini" type="button" onclick="SwitchLight(' + item.idx + ',\'Off\',RefreshFavorites,' + item.Protected + ');">' + $.t("Close") + '</button>';
											}
										}
										else {
											if (item.Status == 'Closed') {
												status =
													'<button class="btn btn-mini" type="button" onclick="SwitchLight(' + item.idx + ',\'On\',RefreshFavorites,' + item.Protected + ');">' + $.t("Open") + '</button> ' +
													'<button class="btn btn-mini btn-info" type="button" onclick="SwitchLight(' + item.idx + ',\'Off\',RefreshFavorites,' + item.Protected + ');">' + $.t("Closed") + '</button>';
											}
											else {
												status =
													'<button class="btn btn-mini btn-info" type="button" onclick="SwitchLight(' + item.idx + ',\'On\',RefreshFavorites,' + item.Protected + ');">' + $.t("Open") + '</button> ' +
													'<button class="btn btn-mini" type="button" onclick="SwitchLight(' + item.idx + ',\'Off\',RefreshFavorites,' + item.Protected + ');">' + $.t("Close") + '</button>';
											}
										}
									}
									else if (item.SwitchType == "Blinds Percentage") {
										if ((item.Status == 'Open')) {
											status =
												'<button class="btn btn-mini btn-info" type="button" onclick="SwitchLight(' + item.idx + ',\'Off\',RefreshFavorites,' + item.Protected + ');">' + $.t("Open") + '</button> ' +
												'<button class="btn btn-mini" type="button" onclick="SwitchLight(' + item.idx + ',\'On\',RefreshFavorites,' + item.Protected + ');">' + $.t("Close") + '</button>';
										}
										else {
											status =
												'<button class="btn btn-mini" type="button" onclick="SwitchLight(' + item.idx + ',\'Off\',RefreshFavorites,' + item.Protected + ');">' + $.t("Open") + '</button> ' +
												'<button class="btn btn-mini btn-info" type="button" onclick="SwitchLight(' + item.idx + ',\'On\',RefreshFavorites,' + item.Protected + ');">' + $.t("Closed") + '</button>';
										}
									}
									else if (item.SwitchType == "Blinds Percentage Inverted") {
										if (item.Status == 'Closed') {
											status =
												'<button class="btn btn-mini" type="button" onclick="SwitchLight(' + item.idx + ',\'On\',RefreshFavorites,' + item.Protected + ');">' + $.t("Open") + '</button> ' +
												'<button class="btn btn-mini btn-info" type="button" onclick="SwitchLight(' + item.idx + ',\'Off\',RefreshFavorites,' + item.Protected + ');">' + $.t("Closed") + '</button>';
										}
										else {
											status =
												'<button class="btn btn-mini btn-info" type="button" onclick="SwitchLight(' + item.idx + ',\'On\',RefreshFavorites,' + item.Protected + ');">' + $.t("Open") + '</button> ' +
												'<button class="btn btn-mini" type="button" onclick="SwitchLight(' + item.idx + ',\'Off\',RefreshFavorites,' + item.Protected + ');">' + $.t("Close") + '</button>';
										}
									}
									else if (item.SwitchType == "Dimmer") {
										var img = "";
										if (
											(item.Status == 'On') ||
											(item.Status == 'Chime') ||
											(item.Status == 'Group On') ||
											(item.Status.indexOf('Set ') == 0)
										) {
											status =
												'<label id=\"statustext\"><button class="btn btn-mini btn-info" type="button" onclick="SwitchLight(' + item.idx + ',\'On\',RefreshFavorites,' + item.Protected + ');">' + $.t("On") + '</button></label> ' +
												'<label id=\"img\"><button class="btn btn-mini" type="button" onclick="SwitchLight(' + item.idx + ',\'Off\',RefreshFavorites,' + item.Protected + ');">' + $.t("Off") + '</button></label>';
										}
										else {
											status =
												'<label id=\"statustext\"><button class="btn btn-mini" type="button" onclick="SwitchLight(' + item.idx + ',\'On\',RefreshFavorites,' + item.Protected + ');">' + $.t("On") + '</button></label> ' +
												'<label id=\"img\"><button class="btn btn-mini btn-info" type="button" onclick="SwitchLight(' + item.idx + ',\'Off\',RefreshFavorites,' + item.Protected + ');">' + $.t("Off") + '</button></label>';
										}
									}
									else if (item.SwitchType == "TPI") {
										var img = "";
										var RO = (item.Unit < 64 || item.Unit > 95) ? true : false;
										if (item.Status == 'On') {
											status =
												'<label id=\"statustext\"><button class="btn btn-mini btn-info" type="button" ' + (RO ? 'disabled' : '') + ' onclick="SwitchLight(' + item.idx + ',\'On\',RefreshFavorites,' + item.Protected + ');">' + $.t("On") + '</button></label> ' +
												'<label id=\"img\"><button class="btn btn-mini" type="button" ' + (RO ? 'disabled' : '') + ' onclick="SwitchLight(' + item.idx + ',\'Off\',RefreshFavorites,' + item.Protected + ');">' + $.t("Off") + '</button></label>';
										}
										else {
											status =
												'<label id=\"statustext\"><button class="btn btn-mini" type="button" ' + (RO ? 'disabled' : '') + ' onclick="SwitchLight(' + item.idx + ',\'On\',RefreshFavorites,' + item.Protected + ');">' + $.t("On") + '</button></label> ' +
												'<label id=\"img\"><button class="btn btn-mini btn-info" type="button" ' + (RO ? 'disabled' : '') + ' onclick="SwitchLight(' + item.idx + ',\'Off\',RefreshFavorites,' + item.Protected + ');">' + $.t("Off") + '</button></label>';
										}
									}
									else if (item.SwitchType == "Dusk Sensor") {
										if (item.Status == 'On') {
											status = '<button class="btn btn-mini" type="button" onclick="ShowLightLog(' + item.idx + ',\'' + escape(item.Name) + '\', \'#dashcontent\', \'ShowFavorites\');">' + $.t("Dark") + '</button>';
										}
										else {
											status = '<button class="btn btn-mini" type="button" onclick="ShowLightLog(' + item.idx + ',\'' + escape(item.Name) + '\', \'#dashcontent\', \'ShowFavorites\');">' + $.t("Sunny") + '</button>';
										}
									}
									else if (item.SwitchType == "Motion Sensor") {
										if (
											(item.Status == 'On') ||
											(item.Status == 'Chime') ||
											(item.Status == 'Group On') ||
											(item.Status.indexOf('Set ') == 0)
										) {
											status = '<button class="btn btn-mini btn-info" type="button" onclick="ShowLightLog(' + item.idx + ',\'' + escape(item.Name) + '\', \'#dashcontent\', \'ShowFavorites\');">' + $.t("Motion") + '</button>';
										}
										else {
											status = '<button class="btn btn-mini" type="button" onclick="ShowLightLog(' + item.idx + ',\'' + escape(item.Name) + '\', \'#dashcontent\', \'ShowFavorites\');">' + $.t("No Motion") + '</button>';
										}
									}
									else if (item.SwitchType == "Smoke Detector") {
										if (
											(item.Status == "Panic") ||
											(item.Status == "On")
										) {
											status = '<button class="btn btn-mini  btn-info" type="button" onclick="ShowLightLog(' + item.idx + ',\'' + escape(item.Name) + '\', \'#dashcontent\', \'ShowFavorites\');">' + $.t("SMOKE") + '</button>';
										}
										else {
											status = '<button class="btn btn-mini" type="button" onclick="ShowLightLog(' + item.idx + ',\'' + escape(item.Name) + '\', \'#dashcontent\', \'ShowFavorites\');">' + $.t("No Smoke") + '</button>';
										}
									}
									else if (item.SwitchType == "Selector") {
										// no buttons, no status needed on mobile mode
										status = '';
									}
									else if (item.SubType.indexOf("Itho") == 0) {
										var class_1 = "btn btn-mini";
										var class_2 = "btn btn-mini";
										var class_3 = "btn btn-mini";
										var class_timer = "btn btn-mini";
										if (item.Status == "1") {
											class_1 += " btn-info";
										}
										else if (item.Status == "2") {
											class_2 += " btn-info";
										}
										else if (item.Status == "3") {
											class_3 += " btn-info";
										}
										else if (item.Status == "timer") {
											class_timer += " btn-info";
										}
										status =
											'<button class="' + class_1 + '" type="button" onclick="SwitchLight(' + item.idx + ',\'1\',RefreshFavorites,' + item.Protected + ');">' + $.t("1") + '</button> ' +
											'<button class="' + class_2 + '" type="button" onclick="SwitchLight(' + item.idx + ',\'2\',RefreshFavorites,' + item.Protected + ');">' + $.t("2") + '</button> ' +
											'<button class="' + class_3 + '" type="button" onclick="SwitchLight(' + item.idx + ',\'3\',RefreshFavorites,' + item.Protected + ');">' + $.t("3") + '</button> ' +
											'<button class="' + class_timer + '" type="button" onclick="SwitchLight(' + item.idx + ',\'timer\',RefreshFavorites,' + item.Protected + ');">' + $.t("Timer") + '</button>';
									}
									else if (item.SubType.indexOf("Lucci") == 0) {
										var class_1 = "btn btn-mini";
										var class_2 = "btn btn-mini";
										var class_3 = "btn btn-mini";
										var class_4 = "btn btn-mini";
										var class_light = "btn btn-mini";
										if (item.Status == "hi") {
											class_1 += " btn-info";
										}
										else if (item.Status == "med") {
											class_2 += " btn-info";
										}
										else if (item.Status == "low") {
											class_3 += " btn-info";
										}
										else if (item.Status == "off") {
											class_4 += " btn-info";
										}
										else if (item.Status == "light") {
											class_light += " btn-info";
										}
										status =
											'<button class="' + class_1 + '" type="button" onclick="SwitchLight(' + item.idx + ',\'hi\',RefreshFavorites,' + item.Protected + ');">' + $.t("hi") + '</button> ' +
											'<button class="' + class_2 + '" type="button" onclick="SwitchLight(' + item.idx + ',\'med\',RefreshFavorites,' + item.Protected + ');">' + $.t("med") + '</button> ' +
											'<button class="' + class_3 + '" type="button" onclick="SwitchLight(' + item.idx + ',\'low\',RefreshFavorites,' + item.Protected + ');">' + $.t("low") + '</button> ' +
											'<button class="' + class_4 + '" type="button" onclick="SwitchLight(' + item.idx + ',\'off\',RefreshFavorites,' + item.Protected + ');">' + $.t("Off") + '</button> ' +
											'<button class="' + class_timer + '" type="button" onclick="SwitchLight(' + item.idx + ',\'light\',RefreshFavorites,' + item.Protected + ');">' + $.t("Light") + '</button>';
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
											status =
												'<button class="btn btn-mini btn-info" type="button" onclick="SwitchLight(' + item.idx + ',\'On\',RefreshFavorites,' + item.Protected + ');">' + $.t("On") + '</button> ' +
												'<button class="btn btn-mini" type="button" onclick="SwitchLight(' + item.idx + ',\'Off\',RefreshFavorites,' + item.Protected + ');">' + $.t("Off") + '</button>';
										}
										else {
											status =
												'<button class="btn btn-mini" type="button" onclick="SwitchLight(' + item.idx + ',\'On\',RefreshFavorites,' + item.Protected + ');">' + $.t("On") + '</button> ' +
												'<button class="btn btn-mini btn-info" type="button" onclick="SwitchLight(' + item.idx + ',\'Off\',RefreshFavorites,' + item.Protected + ');">' + $.t("Off") + '</button>';
										}
									}
									xhtm +=
										'\t      <td id="status" class="status">' + status + '</td>\n' +
										'\t    </tr>\n';
									if (item.SwitchType == "Dimmer") {
										xhtm += '<tr>';
										xhtm += '<td colspan="2" style="border:0px solid red; padding-top:10px; padding-bottom:10px;">';
										xhtm += '<div style="margin-top: -11px; margin-left: 24px;" class="dimslider dimslidernorm" id="light_' + item.idx + '_slider" data-idx="' + item.idx + '" data-type="norm" data-maxlevel="' + item.MaxDimLevel + '" data-isprotected="' + item.Protected + '" data-svalue="' + item.LevelInt + '"></div>';
										xhtm += '</td>';
										xhtm += '</tr>';
									}
									else if (item.SwitchType == "TPI") {
										var RO = (item.Unit < 64 || item.Unit > 95) ? true : false;
										xhtm += '<tr>';
										xhtm += '<td colspan="2" style="border:0px solid red; padding-top:10px; padding-bottom:10px;">';
										xhtm += '<div style="margin-top: -11px; margin-left: 24px;" class="dimslider dimslidernorm" id="light_' + item.idx + '_slider" data-idx="' + item.idx + '" data-type="relay" data-maxlevel="' + item.MaxDimLevel + '" data-isprotected="' + item.Protected + '" data-svalue="' + item.LevelInt + '"';
										if (item.Unit < 64 || item.Unit > 95)
											xhtm += ' data-disabled="true"';
										xhtm += '></div>';
										xhtm += '</td>';
										xhtm += '</tr>';
									}
									else if ((item.SwitchType == "Blinds Percentage") || (item.SwitchType == "Blinds Percentage Inverted")) {
										xhtm += '<tr>';
										xhtm += '<td colspan="2" style="border:0px solid red; padding-top:10px; padding-bottom:10px;">';
										xhtm += '<div style="margin-top: -11px; margin-left: 24px;" class="dimslider dimslidersmall" id="slider" data-idx="' + item.idx + '" data-type="blinds" data-maxlevel="' + item.MaxDimLevel + '" data-isprotected="' + item.Protected + '" data-svalue="' + item.LevelInt + '"></div>';
										xhtm += '</td>';
										xhtm += '</tr>';
									}
									else if (item.SwitchType == "Selector") {
										xhtm += '<tr>';
										xhtm += '<td colspan="2" style="border:0px solid red; padding-top:4px; padding-bottom:4px;">';
										if (item.SelectorStyle === 0) {
											xhtm += '<div style="margin: -15px -4px -5px 24px; text-align: right;" class="selectorlevels">';
											xhtm += '<div id="selector' + item.idx + '" data-idx="' + item.idx + '" data-isprotected="' + item.Protected + '" data-level="' + item.LevelInt + '" data-levelname="' + escape(GetLightStatusText(item)) + '">';
											var levelNames = item.LevelNames.split('|');
											$.each(levelNames, function (index, levelName) {
												if ((index === 0) && (item.LevelOffHidden)) {
													return;
												}
												xhtm += '<input type="radio" id="dSelector' + item.idx + 'Level' + index + '" name="selector' + item.idx + 'Level" value="' + (index * 10) + '"><label for="dSelector' + item.idx + 'Level' + index + '">' + levelName + '</label>';
											});
											xhtm += '</div>';
										} else if (item.SelectorStyle === 1) {
											xhtm += '<div style="margin: -15px 0px -8px 0px; text-align: right;" class="selectorlevels">';
											xhtm += '<select id="selector' + item.idx + '" data-idx="' + item.idx + '" data-isprotected="' + item.Protected + '" data-level="' + item.LevelInt + '" data-levelname="' + escape(GetLightStatusText(item)) + '">';
											var levelNames = item.LevelNames.split('|');
											$.each(levelNames, function (index, levelName) {
												if ((index === 0) && (item.LevelOffHidden)) {
													return;
												}
												xhtm += '<option value="' + (index * 10) + '">' + levelName + '</option>';
											});
											xhtm += '</select>';
											xhtm += '</div>';
										}
										xhtm += '</td>';
										xhtm += '</tr>';
									}
								}
								else {
									if ($scope.config.DashboardType == 0) {
										xhtm = '\t<div class="span4 movable" id="light_' + item.idx + '">\n';
									}
									else if ($scope.config.DashboardType == 1) {
										xhtm = '\t<div class="span3 movable" id="light_' + item.idx + '">\n';
									}
									xhtm += '\t  <div id="bstatus" class="item ' + backgroundClass + '">\n';
									if ((item.Type.indexOf('Blind') == 0) || (item.SwitchType == "Blinds") || (item.SwitchType == "Blinds Inverted") || (item.SwitchType == "Blinds Percentage") || (item.SwitchType == "Blinds Percentage Inverted") || (item.SwitchType.indexOf("Venetian Blinds") == 0) || (item.SwitchType.indexOf("Media Player") == 0)) {
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
											xhtm += '\t    <table id="itemtablesmalltrippleicon" id="itemtablesmalltripleicon" border="0" cellpadding="0" cellspacing="0">\n';
										}
										else {
											xhtm += '\t    <table id="itemtablesmalldoubleicon" class="itemtablesmalldoubleicon" border="0" cellpadding="0" cellspacing="0">\n';
										}
									}
									else {
										xhtm += '\t    <table id="itemtablesmall" class="itemtablesmall" border="0" cellpadding="0" cellspacing="0">\n';
									}
									xhtm +=
										'\t    <tr>\n' +
										'\t      <td id="name" class="name">' + item.Name + '</td>\n' +
										'\t      <td id="bigtext" class="bigtext"><span>';
									var bigtext = TranslateStatusShort(item.Status);
									if (item.UsedByCamera == true) {
										var streamimg = '<img src="images/webcam.png" title="' + $.t('Stream Video') + '" height="16" width="16">';
										streamurl = "<a href=\"javascript:ShowCameraLiveStream('" + escape(item.Name) + "','" + item.CameraIdx + "')\">" + streamimg + "</a>";
										bigtext += "&nbsp;" + streamurl;
									}
									xhtm += bigtext + '</span></td>\n';
									if (item.SwitchType == "Doorbell") {
										xhtm += '\t      <td id="img" class="img img1"><img src="images/doorbell48.png" title="' + $.t("Turn On") + '" onclick="SwitchLight(' + item.idx + ',\'On\',RefreshFavorites,' + item.Protected + ');" class="lcursor" height="40" width="40"></td>\n';
									}
									else if (item.SwitchType == "Push On Button") {
										if (item.InternalState == "On") {
											xhtm += '\t      <td id="img" class="img img1"><img src="images/pushon48.png" title="' + $.t("Turn On") + '" onclick="SwitchLight(' + item.idx + ',\'On\',RefreshFavorites,' + item.Protected + ');" class="lcursor" height="40" width="40"></td>\n';
										}
										else {
											xhtm += '\t      <td id="img" class="img img1"><img src="images/push48.png" title="' + $.t("Turn On") + '" onclick="SwitchLight(' + item.idx + ',\'On\',RefreshFavorites,' + item.Protected + ');" class="lcursor" height="40" width="40"></td>\n';
										}
									}
									else if (item.SwitchType == "Door Contact") {
										if (item.InternalState == "Open") {
											xhtm += '\t      <td id="img" class="img img1"><img src="images/door48open.png" title="' + $.t("Close Door") + '" onclick="SwitchLight(' + item.idx + ',\'Off\',RefreshFavorites,' + item.Protected + ');" class="lcursor" height="40" width="40"></td>\n';
										}
										else {
											xhtm += '\t      <td id="img" class="img img1"><img src="images/door48.png" title="' + $.t("Open Door") + '" onclick="SwitchLight(' + item.idx + ',\'On\',RefreshFavorites,' + item.Protected + ');" class="lcursor" height="40" width="40"></td>\n';
										}
									}
									else if (item.SwitchType == "Door Lock") {
										if (item.InternalState == "Unlocked") {
											xhtm += '\t      <td id="img" class="img img1"><img src="images/door48open.png" title="' + $.t("Lock") + '" onclick="SwitchLight(' + item.idx + ',\'On\',RefreshFavorites,' + item.Protected + ');" class="lcursor" height="40" width="40"></td>\n';
										}
										else {
											xhtm += '\t      <td id="img" class="img img1"><img src="images/door48.png" title="' + $.t("Unlock") + '" onclick="SwitchLight(' + item.idx + ',\'Off\',RefreshFavorites,' + item.Protected + ');" class="lcursor" height="40" width="40"></td>\n';
										}
									}
									else if (item.SwitchType == "Push Off Button") {
										xhtm += '\t      <td id="img" class="img img1"><img src="images/pushoff48.png" title="' + $.t("Turn Off") + '" onclick="SwitchLight(' + item.idx + ',\'Off\',RefreshFavorites,' + item.Protected + ');" class="lcursor" height="40" width="40"></td>\n';
									}
									else if (item.SwitchType == "X10 Siren") {
										if (
											(item.Status == 'On') ||
											(item.Status == 'Chime') ||
											(item.Status == 'Group On') ||
											(item.Status == 'All On')
										) {
											xhtm += '\t      <td id="img" class="img img1"><img src="images/siren-on.png" height="40" width="40"></td>\n';
										}
										else {
											xhtm += '\t      <td id="img" class="img img1"><img src="images/siren-off.png" height="40" width="40"></td>\n';
										}
									}
									else if (item.SwitchType == "Contact") {
										if (item.Status == 'Closed') {
											xhtm += '\t      <td id="img" class="img img1"><img src="images/contact48.png" onclick="ShowLightLog(' + item.idx + ',\'' + escape(item.Name) + '\', \'#dashcontent\', \'ShowFavorites\');" class="lcursor" height="40" width="40"></td>\n';
										}
										else {
											xhtm += '\t      <td id="img" class="img img1"><img src="images/contact48_open.png" onclick="ShowLightLog(' + item.idx + ',\'' + escape(item.Name) + '\', \'#dashcontent\', \'ShowFavorites\');" class="lcursor" height="40" width="40"></td>\n';
										}
									}
									else if (item.SwitchType == "Media Player") {
										if (item.CustomImage == 0) item.Image = item.TypeImg;
										if (item.Status == 'Disconnected') {
											xhtm += '\t      <td id="img" class="img img1"><img src="images/' + item.Image + '48_Off.png" height="40" width="40"></td>\n';
											xhtm += '\t      <td id="img2" class="img2"><img src="images/remote48.png" style="opacity:0.4"; height="40" width="40"></td>\n';
										}
										else if ((item.Status != 'Off') && (item.Status != '0')) {
											xhtm += '\t      <td id="img" class="img img1"><img src="images/' + item.Image + '48_On.png" onclick="SwitchLight(' + item.idx + ',\'Off\',RefreshFavorites,' + item.Protected + ');" class="lcursor" height="40" width="40"></td>\n';
											xhtm += '\t      <td id="img2" class="img2"><img src="images/remote48.png" onclick="ShowMediaRemote(\'' + escape(item.Name) + "'," + item.idx + ",'" + item.HardwareType + '\');" class="lcursor" height="40" width="40"></td>\n';
										}
										else {
											xhtm += '\t      <td id="img" class="img img1"><img src="images/' + item.Image + '48_Off.png" onclick="SwitchLight(' + item.idx + ',\'On\',RefreshFavorites,' + item.Protected + ');" class="lcursor" height="40" width="40"></td>\n';
											xhtm += '\t      <td id="img2" class="img2"><img src="images/remote48.png" style="opacity:0.4"; height="40" width="40"></td>\n';
										}
										status = item.Data;
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
												xhtm += '\t      <td id="img" class="img img1"><img src="images/blindsopen48.png" title="' + $.t("Open Blinds") + '" onclick="SwitchLight(' + item.idx + ',\'Off\',RefreshFavorites,' + item.Protected + ');" class="lcursor" height="40" width="40"></td>\n';
												xhtm += '\t      <td id="img2" class="img2"><img src="images/blindsstop.png" title="' + $.t("Stop Blinds") + '" onclick="SwitchLight(' + item.idx + ',\'Stop\',RefreshFavorites,' + item.Protected + ');" class="lcursor" height="40" width="24"></td>\n';
												xhtm += '\t      <td id="img3" class="img3"><img src="images/blinds48sel.png" title="' + $.t("Close Blinds") + '" onclick="SwitchLight(' + item.idx + ',\'On\',RefreshFavorites,' + item.Protected + ');" class="lcursor" height="40" width="40"></td>\n';
											}
											else {
												xhtm += '\t      <td id="img" class="img img1"><img src="images/blindsopen48sel.png" title="' + $.t("Open Blinds") + '" onclick="SwitchLight(' + item.idx + ',\'Off\',RefreshFavorites,' + item.Protected + ');" class="lcursor" height="40" width="40"></td>\n';
												xhtm += '\t      <td id="img2" class="img2"><img src="images/blindsstop.png" title="' + $.t("Stop Blinds") + '" onclick="SwitchLight(' + item.idx + ',\'Stop\',RefreshFavorites,' + item.Protected + ');" class="lcursor" height="40" width="24"></td>\n';
												xhtm += '\t      <td id="img3" class="img3"><img src="images/blinds48.png" title="' + $.t("Close Blinds") + '" onclick="SwitchLight(' + item.idx + ',\'On\',RefreshFavorites,' + item.Protected + ');" class="lcursor" height="40" width="40"></td>\n';
											}
										}
										else {
											if (item.Status == 'Closed') {
												xhtm += '\t      <td id="img" class="img img1"><img src="images/blindsopen48.png" title="' + $.t("Open Blinds") + '" onclick="SwitchLight(' + item.idx + ',\'Off\',RefreshFavorites,' + item.Protected + ');" class="lcursor" height="40" width="40"></td>\n';
												xhtm += '\t      <td id="img2" class="img2"><img src="images/blinds48sel.png" title="' + $.t("Close Blinds") + '" onclick="SwitchLight(' + item.idx + ',\'On\',RefreshFavorites,' + item.Protected + ');" class="lcursor" height="40" width="40"></td>\n';
											}
											else {
												xhtm += '\t      <td id="img" class="img img1"><img src="images/blindsopen48sel.png" title="' + $.t("Open Blinds") + '" onclick="SwitchLight(' + item.idx + ',\'Off\',RefreshFavorites,' + item.Protected + ');" class="lcursor" height="40" width="40"></td>\n';
												xhtm += '\t      <td id="img2" class="img2"><img src="images/blinds48.png" title="' + $.t("Close Blinds") + '" onclick="SwitchLight(' + item.idx + ',\'On\',RefreshFavorites,' + item.Protected + ');" class="lcursor" height="40" width="40"></td>\n';
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
												xhtm += '\t      <td id="img" class="img img1"><img src="images/blindsopen48.png" title="' + $.t("Open Blinds") + '" onclick="SwitchLight(' + item.idx + ',\'On\',RefreshFavorites,' + item.Protected + ');" class="lcursor" height="40" width="40"></td>\n';
												xhtm += '\t      <td id="img2" class="img2"><img src="images/blindsstop.png" title="' + $.t("Stop Blinds") + '" onclick="SwitchLight(' + item.idx + ',\'Stop\',RefreshFavorites,' + item.Protected + ');" class="lcursor" height="40" width="24"></td>\n';
												xhtm += '\t      <td id="img3" class="img3"><img src="images/blinds48sel.png" title="' + $.t("Close Blinds") + '" onclick="SwitchLight(' + item.idx + ',\'Off\',RefreshFavorites,' + item.Protected + ');" class="lcursor" height="40" width="40"></td>\n';
											}
											else {
												xhtm += '\t      <td id="img" class="img img1"><img src="images/blindsopen48sel.png" title="' + $.t("Open Blinds") + '" onclick="SwitchLight(' + item.idx + ',\'On\',RefreshFavorites,' + item.Protected + ');" class="lcursor" height="40" width="40"></td>\n';
												xhtm += '\t      <td id="img2" class="img2"><img src="images/blindsstop.png" title="' + $.t("Stop Blinds") + '" onclick="SwitchLight(' + item.idx + ',\'Stop\',RefreshFavorites,' + item.Protected + ');" class="lcursor" height="40" width="24"></td>\n';
												xhtm += '\t      <td id="img3" class="img3"><img src="images/blinds48.png" title="' + $.t("Close Blinds") + '" onclick="SwitchLight(' + item.idx + ',\'Off\',RefreshFavorites,' + item.Protected + ');" class="lcursor" height="40" width="40"></td>\n';
											}
										}
										else {
											if (item.Status == 'Closed') {
												xhtm += '\t      <td id="img" class="img img1"><img src="images/blindsopen48.png" title="' + $.t("Open Blinds") + '" onclick="SwitchLight(' + item.idx + ',\'On\',RefreshFavorites,' + item.Protected + ');" class="lcursor" height="40" width="40"></td>\n';
												xhtm += '\t      <td id="img2" class="img2"><img src="images/blinds48sel.png" title="' + $.t("Close Blinds") + '" onclick="SwitchLight(' + item.idx + ',\'Off\',RefreshFavorites,' + item.Protected + ');" class="lcursor" height="40" width="40"></td>\n';
											}
											else {
												xhtm += '\t      <td id="img" class="img img1"><img src="images/blindsopen48sel.png" title="' + $.t("Open Blinds") + '" onclick="SwitchLight(' + item.idx + ',\'On\',RefreshFavorites,' + item.Protected + ');" class="lcursor" height="40" width="40"></td>\n';
												xhtm += '\t      <td id="img2" class="img2"><img src="images/blinds48.png" title="' + $.t("Close Blinds") + '" onclick="SwitchLight(' + item.idx + ',\'Off\',RefreshFavorites,' + item.Protected + ');" class="lcursor" height="40" width="40"></td>\n';
											}
										}
									}
									else if (item.SwitchType == "Blinds Percentage") {
										if (item.Status == 'Open') {
											xhtm += '\t      <td id="img" class="img img1"><img src="images/blindsopen48sel.png" title="' + $.t("Open Blinds") + '" onclick="SwitchLight(' + item.idx + ',\'Off\',RefreshFavorites,' + item.Protected + ');" class="lcursor" height="40" width="40"></td>\n';
											xhtm += '\t      <td id="img2" class="img2"><img src="images/blinds48.png" title="' + $.t("Close Blinds") + '" onclick="SwitchLight(' + item.idx + ',\'On\',RefreshFavorites,' + item.Protected + ');" class="lcursor" height="40" width="40"></td>\n';
										}
										else {
											xhtm += '\t      <td id="img" class="img img1"><img src="images/blindsopen48.png" title="' + $.t("Open Blinds") + '" onclick="SwitchLight(' + item.idx + ',\'Off\',RefreshFavorites,' + item.Protected + ');" class="lcursor" height="40" width="40"></td>\n';
											xhtm += '\t      <td id="img2" class="img2"><img src="images/blinds48sel.png" title="' + $.t("Close Blinds") + '" onclick="SwitchLight(' + item.idx + ',\'On\',RefreshFavorites,' + item.Protected + ');" class="lcursor" height="40" width="40"></td>\n';
										}
									}
									else if (item.SwitchType == "Blinds Percentage Inverted") {
										if (item.Status == 'Closed') {
											xhtm += '\t      <td id="img" class="img img1"><img src="images/blindsopen48.png" title="' + $.t("Open Blinds") + '" onclick="SwitchLight(' + item.idx + ',\'On\',RefreshFavorites,' + item.Protected + ');" class="lcursor" height="40" width="40"></td>\n';
											xhtm += '\t      <td id="img2" class="img2"><img src="images/blinds48sel.png" title="' + $.t("Close Blinds") + '" onclick="SwitchLight(' + item.idx + ',\'Off\',RefreshFavorites,' + item.Protected + ');" class="lcursor" height="40" width="40"></td>\n';
										}
										else {
											xhtm += '\t      <td id="img" class="img img1"><img src="images/blindsopen48sel.png" title="' + $.t("Open Blinds") + '" onclick="SwitchLight(' + item.idx + ',\'On\',RefreshFavorites,' + item.Protected + ');" class="lcursor" height="40" width="40"></td>\n';
											xhtm += '\t      <td id="img2" class="img2"><img src="images/blinds48.png" title="' + $.t("Close Blinds") + '" onclick="SwitchLight(' + item.idx + ',\'Off\',RefreshFavorites,' + item.Protected + ');" class="lcursor" height="40" width="40"></td>\n';
										}
									}
									else if (item.SwitchType == "Dimmer") {
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
												xhtm += '\t      <td id="img" class="img img1"><img src="images/RGB48_On.png" onclick="ShowRGBWPopup(event, ' + item.idx + ', \'RefreshFavorites\',' + item.Protected + ',' + item.MaxDimLevel + ',' + item.LevelInt + ',' + item.Hue + ');" class="lcursor" height="40" width="40"></td>\n';
											}
											else if (item.SubType.indexOf("RGBW") >= 0) {
												xhtm += '\t      <td id="img" class="img img1"><img src="images/RGB48_On.png" onclick="ShowRGBWPopup(event, ' + item.idx + ', \'RefreshFavorites\',' + item.Protected + ',' + item.MaxDimLevel + ',' + item.LevelInt + ',' + item.Hue + ');" class="lcursor" height="40" width="40"></td>\n';
											}
											else {
												xhtm += '\t      <td id="img" class="img img1"><img src="images/' + item.Image + '48_On.png" title="' + $.t("Turn Off") + '" onclick="SwitchLight(' + item.idx + ',\'Off\',RefreshFavorites,' + item.Protected + ');" class="lcursor" height="40" width="40"></td>\n';
											}
										}
										else {
											if (item.SubType == "RGB") {
												xhtm += '\t      <td id="img" class="img img1"><img src="images/RGB48_Off.png" onclick="ShowRGBWPopup(event, ' + item.idx + ', \'RefreshFavorites\',' + item.Protected + ',' + item.MaxDimLevel + ',' + item.LevelInt + ',' + item.Hue + ');" class="lcursor" height="40" width="40"></td>\n';
											}
											else if (item.SubType.indexOf("RGBW") >= 0) {
												xhtm += '\t      <td id="img" class="img img1"><img src="images/RGB48_Off.png" onclick="ShowRGBWPopup(event, ' + item.idx + ', \'RefreshFavorites\',' + item.Protected + ',' + item.MaxDimLevel + ',' + item.LevelInt + ',' + item.Hue + ');" class="lcursor" height="40" width="40"></td>\n';
											}
											else {
												xhtm += '\t      <td id="img" class="img img1"><img src="images/' + item.Image + '48_Off.png" title="' + $.t("Turn On") + '" onclick="SwitchLight(' + item.idx + ',\'On\',RefreshFavorites,' + item.Protected + ');" class="lcursor" height="40" width="40"></td>\n';
											}
										}
									}
									else if (item.SwitchType == "TPI") {
										var RO = (item.Unit < 64 || item.Unit > 95) ? true : false;
										if (item.Status == 'On') {
											xhtm += '\t      <td id="img" class="img img1"><img src="images/Fireplace48_On.png" title="' + $.t(RO ? "On" : "Turn Off") + (RO ? '"' : '" onclick="SwitchLight(' + item.idx + ',\'Off\',RefreshFavorites,' + item.Protected + ');" class="lcursor"') + ' height="40" width="40"></td>\n';
										}
										else {
											xhtm += '\t      <td id="img" class="img img1"><img src="images/Fireplace48_Off.png" title="' + $.t(RO ? "Off" : "Turn On") + (RO ? '"' : '" onclick="SwitchLight(' + item.idx + ',\'On\',RefreshFavorites,' + item.Protected + ');" class="lcursor"') + ' height="40" width="40"></td>\n';
										}
									}
									else if (item.SwitchType == "Dusk Sensor") {
										if (item.Status == 'On') {
											xhtm += '\t      <td id="img" class="img img1"><img src="images/uvdark.png" onclick="ShowLightLog(' + item.idx + ',\'' + escape(item.Name) + '\', \'#dashcontent\', \'ShowFavorites\');" class="lcursor" height="40" width="40"></td>\n';
										}
										else {
											xhtm += '\t      <td id="img" class="img img1"><img src="images/uvsunny.png" onclick="ShowLightLog(' + item.idx + ',\'' + escape(item.Name) + '\', \'#dashcontent\', \'ShowFavorites\');" class="lcursor" height="40" width="40"></td>\n';
										}
									}
									else if (item.SwitchType == "Motion Sensor") {
										if (
											(item.Status == 'On') ||
											(item.Status == 'Chime') ||
											(item.Status == 'Group On') ||
											(item.Status.indexOf('Set ') == 0)
										) {
											xhtm += '\t      <td id="img" class="img img1"><img src="images/motion48-on.png" onclick="ShowLightLog(' + item.idx + ',\'' + escape(item.Name) + '\', \'#dashcontent\', \'ShowFavorites\');" class="lcursor" height="40" width="40"></td>\n';
										}
										else {
											xhtm += '\t      <td id="img" class="img img1"><img src="images/motion48-off.png" onclick="ShowLightLog(' + item.idx + ',\'' + escape(item.Name) + '\', \'#dashcontent\', \'ShowFavorites\');" class="lcursor" height="40" width="40"></td>\n';
										}
									}
									else if (item.SwitchType == "Smoke Detector") {
										if (
											(item.Status == "Panic") ||
											(item.Status == "On")
										) {
											xhtm += '\t      <td id="img" class="img img1"><img src="images/smoke48on.png" onclick="ShowLightLog(' + item.idx + ',\'' + escape(item.Name) + '\', \'#dashcontent\', \'ShowFavorites\');" class="lcursor" height="40" width="40"></td>\n';
										}
										else {
											xhtm += '\t      <td id="img" class="img img1"><img src="images/smoke48off.png" onclick="ShowLightLog(' + item.idx + ',\'' + escape(item.Name) + '\', \'#dashcontent\', \'ShowFavorites\');" class="lcursor" height="40" width="40"></td>\n';
										}
									}
									else if (item.SwitchType === "Selector") {
										if (item.Status === 'Off') {
											xhtm += '\t      <td id="img" class="img img1"><img src="images/' + item.Image + '48_Off.png" height="40" width="40"></td>\n';
										} else if (item.LevelOffHidden) {
											xhtm += '\t      <td id="img" class="img img1"><img src="images/' + item.Image + '48_On.png" height="40" width="40"></td>\n';
										} else {
											xhtm += '\t      <td id="img" class="img img1"><img src="images/' + item.Image + '48_On.png" onclick="SwitchLight(' + item.idx + ',\'Off\',RefreshFavorites,' + item.Protected + ');" class="lcursor" height="40" width="40"></td>\n';
										}
									}
									else if (item.SubType.indexOf("Itho") == 0) {
										xhtm += '\t      <td id="img" class="img img1"><img src="images/Fan48_On.png" height="40" width="40" class="lcursor" onclick="ShowIthoPopup(event, ' + item.idx + ', RefreshFavorites, ' + item.Protected + ');"></td>\n';
									}
									else if (item.SubType.indexOf("Lucci") == 0) {
										xhtm += '\t      <td id="img" class="img img1"><img src="images/Fan48_On.png" height="40" width="40" class="lcursor" onclick="ShowLucciPopup(event, ' + item.idx + ', RefreshFavorites, ' + item.Protected + ');"></td>\n';
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
												xhtm += '\t      <td id="img" class="img img1"><img src="images/' + item.Image + '48_On.png" onclick="ShowTherm3Popup(event, ' + item.idx + ',\'RefreshFavorites\',' + item.Protected + ');" class="lcursor" height="40" width="40"></td>\n';
											}
											else {
												xhtm += '\t      <td id="img" class="img img1"><img src="images/' + item.Image + '48_On.png" title="' + $.t("Turn Off") + '" onclick="SwitchLight(' + item.idx + ',\'Off\',RefreshFavorites,' + item.Protected + ');" class="lcursor" height="40" width="40"></td>\n';
											}
										}
										else {
											if (item.Type == "Thermostat 3") {
												xhtm += '\t      <td id="img" class="img img1"><img src="images/' + item.Image + '48_Off.png" onclick="ShowTherm3Popup(event, ' + item.idx + ',\'RefreshFavorites\',' + item.Protected + ');" class="lcursor" height="40" width="40"></td>\n';
											}
											else {
												xhtm += '\t      <td id="img" class="img img1"><img src="images/' + item.Image + '48_Off.png" title="' + $.t("Turn On") + '" onclick="SwitchLight(' + item.idx + ',\'On\',RefreshFavorites,' + item.Protected + ');" class="lcursor" height="40" width="40"></td>\n';
											}
										}
									}
									xhtm +=
										'\t      <td id="status" class="status">' + status + '</td>\n' +
										'\t      <td id="lastupdate" class="lastupdate"><span>' + item.LastUpdate + '</span></td>\n';
									if (item.SwitchType == "Dimmer") {
										if ((item.SubType.indexOf("RGBW") >= 0) || (item.SubType == "RGB")) {
										}
										else {
											xhtm += '<td><div style="margin-left:50px; margin-top: 0.2em;" class="dimslider dimslidernorm" id="slider" data-idx="' + item.idx + '" data-type="norm" data-maxlevel="' + item.MaxDimLevel + '" data-isprotected="' + item.Protected + '" data-svalue="' + item.LevelInt + '"></div></td>';
										}
									}
									else if (item.SwitchType == "TPI") {
										xhtm += '<td><div style="margin-left:50px; margin-top: 0.2em;" class="dimslider dimslidernorm" id="slider" data-idx="' + item.idx + '" data-type="relay" data-maxlevel="' + item.MaxDimLevel + '" data-isprotected="' + item.Protected + '" data-svalue="' + item.LevelInt + '"';
										if (item.Unit < 64 || item.Unit > 95)
											xhtm += ' data-disabled="true"';
										xhtm += '></div></td>';
									}
									else if ((item.SwitchType == "Blinds Percentage") || (item.SwitchType == "Blinds Percentage Inverted")) {
										xhtm += '<td><div style="margin-left:94px; margin-top: 0.2em;" class="dimslider dimslidersmalldouble" id="slider" data-idx="' + item.idx + '" data-type="blinds" data-maxlevel="' + item.MaxDimLevel + '" data-isprotected="' + item.Protected + '" data-svalue="' + item.LevelInt + '"></div></td>';
									}
									else if (item.SwitchType == "Selector") {
										if (item.SelectorStyle === 0) {
											xhtm += '<td><div style="margin-top:0.2em;" class="selectorlevels">';
											xhtm += '<div id="selector' + item.idx + '" data-idx="' + item.idx + '" data-isprotected="' + item.Protected + '" data-level="' + item.LevelInt + '" data-levelname="' + escape(GetLightStatusText(item)) + '">';
											var levelNames = item.LevelNames.split('|');
											$.each(levelNames, function (index, levelName) {
												if ((index === 0) && (item.LevelOffHidden)) {
													return;
												}
												xhtm += '<input type="radio" id="dSelector' + item.idx + 'Level' + index + '" name="selector' + item.idx + 'Level" value="' + (index * 10) + '"><label for="dSelector' + item.idx + 'Level' + index + '">' + levelName + '</label>';
											});
											xhtm += '</div></td>';
										} else if (item.SelectorStyle === 1) {
											xhtm += '<td><div style="margin-top:0.2em;" class="selectorlevels">';
											xhtm += '<select id="selector' + item.idx + '" data-idx="' + item.idx + '" data-isprotected="' + item.Protected + '" data-level="' + item.LevelInt + '" data-levelname="' + escape(GetLightStatusText(item)) + '">';
											var levelNames = item.LevelNames.split('|');
											$.each(levelNames, function (index, levelName) {
												if ((index === 0) && (item.LevelOffHidden)) {
													return;
												}
												xhtm += '<option value="' + (index * 10) + '">' + levelName + '</option>';
											});
											xhtm += '</select>';
											xhtm += '</div></td>';
										}
									}
									xhtm +=
										'\t    </tr>\n' +
										'\t    </table>\n' +
										'\t  </div><!--item end-->\n' +
										'\t</div>\n';
								}
								htmlcontent += xhtm;
								jj += 1;
							}
						}); //light devices
						if (bHaveAddedDivider == true) {
							//close previous divider
							htmlcontent += '</div>\n';
						}
						if (($scope.config.DashboardType == 2) || (window.myglobals.ismobile == true)) {
							htmlcontent += '\t    </table>\n';
						}
						if (jj > 0) {
							htmlcontent += '</section>';
						}

						//Temperature Sensors
						jj = 0;
						bHaveAddedDivider = false;
						$.each(data.result, function (i, item) {
							if (
								((typeof item.Temp != 'undefined') || (typeof item.Humidity != 'undefined') || (typeof item.Chill != 'undefined')) &&
								(item.Favorite != 0)
							) {
								totdevices += 1;
								if (jj == 0) {
									//first time
									htmlcontent += '<section class="dashCategory" id="dashTemperature">';
									if (($scope.config.DashboardType == 2) || (window.myglobals.ismobile == true)) {
										if (htmlcontent != "") {
											htmlcontent += '<br>';
										}
										htmlcontent += '\t    <table class="mobileitem">\n';
										htmlcontent += '\t    <thead>\n';
										htmlcontent += '\t    <tr>\n';
										htmlcontent += '\t    		<th>' + $.t('Temperature Sensors') + '</th>\n';
										htmlcontent += '\t    		<th style="text-align:right"><a id="cTemperature" href="javascript:SwitchLayout(\'Temperature\')"><img src="images/next.png"></a></th>\n';
										htmlcontent += '\t    </tr>\n';
										htmlcontent += '\t    </thead>\n';
									}
									else {
										htmlcontent += '<h2>' + $.t('Temperature Sensors') + ':</h2>\n';
									}
								}
								if (jj % rowItems == 0) {
									//add devider
									if (bHaveAddedDivider == true) {
										//close previous devider
										htmlcontent += '</div>\n';
									}
									htmlcontent += '<div class="row divider">\n';
									bHaveAddedDivider = true;
								}
								var xhtm = "";

								var backgroundClass = $rootScope.GetItemBackgroundStatus(item);

								if (($scope.config.DashboardType == 2) || (window.myglobals.ismobile == true)) {
									var vname = '<img src="images/next.png" onclick="ShowTempLog(\'#dashcontent\',\'ShowFavorites\',' + item.idx + ',\'' + escape(item.Name) + '\');" height="16" width="16">' + " " + item.Name;

									xhtm +=
										'\t    <tr id="temp_' + item.idx + '">\n' +
										'\t      <td id="name" class="name">' + vname + '</td>\n';
									var status = "";
									var bHaveBefore = false;
									if (typeof item.Temp != 'undefined') {
										status += item.Temp + '&deg; ' + $scope.config.TempSign;
										bHaveBefore = true;
									}
									if (typeof item.Chill != 'undefined') {
										if (bHaveBefore) {
											status += ', ';
										}
										status += $.t('Chill') + ': ' + item.Chill + '&deg; ' + $scope.config.TempSign;
										bHaveBefore = true;
									}
									if (typeof item.Humidity != 'undefined') {
										if (bHaveBefore == true) {
											status += ', ';
										}
										status += item.Humidity + ' %';
									}
									if (typeof item.HumidityStatus != 'undefined') {
										status += ' (' + $.t(item.HumidityStatus) + ')';
									}
									if (typeof item.DewPoint != 'undefined') {
										status += "<br>" + $.t("Dew Point") + ": " + item.DewPoint + '&deg; ' + $scope.config.TempSign;
									}
									xhtm +=
										'\t      <td id="status" class="status">' + status + '</td>\n' +
										'\t    </tr>\n';
								}
								else {
									if ($scope.config.DashboardType == 0) {
										xhtm = '\t<div class="span4 movable" id="temp_' + item.idx + '">\n';
									}
									else if ($scope.config.DashboardType == 1) {
										xhtm = '\t<div class="span3 movable" id="temp_' + item.idx + '">\n';
									}
									xhtm += '\t  <div id="bstatus" class="item ' + backgroundClass + '">\n';
									xhtm += '\t    <table id="itemtablesmall" class="itemtablesmall" border="0" cellpadding="0" cellspacing="0">\n';
									xhtm += '\t    <tr>\n';
									xhtm += '\t      <td id="name" class="name">' + item.Name + '</td>\n';
									xhtm += '\t      <td id="bigtext" class="bigtext"><span>';
									var bigtext = "";
									if (typeof item.Temp != 'undefined') {
										bigtext = item.Temp + '\u00B0 ' + $scope.config.TempSign;
									}
									if (typeof item.Humidity != 'undefined') {
										if (bigtext != "") {
											bigtext += ' / ';
										}
										bigtext += item.Humidity + '%';
									}
									if (typeof item.Chill != 'undefined') {
										if (bigtext != "") {
											bigtext += ' / ';
										}
										bigtext += item.Chill + '\u00B0 ' + $scope.config.TempSign;
									}
									xhtm += bigtext + '</span></td>\n';
									xhtm += '\t      <td id="img" class="img img1"><img src="images/';
									if (typeof item.Temp != 'undefined') {
										xhtm += GetTemp48Item(item.Temp);
									}
									else {
										if (item.Type == "Humidity") {
											xhtm += "gauge48.png";
										}
										else {
											xhtm += GetTemp48Item(item.Chill);
										}
									}
									xhtm += '" class="lcursor" onclick="ShowTempLog(\'#dashcontent\',\'ShowFavorites\',' + item.idx + ',\'' + escape(item.Name) + '\');" height="40" width="40"></td>\n' +
										'\t      <td id="status" class="status">';
									var bHaveBefore = false;
									if (typeof item.HumidityStatus != 'undefined') {
										xhtm += $.t(item.HumidityStatus);
										bHaveBefore = true;
									}
									if (typeof item.DewPoint != 'undefined') {
										if (bHaveBefore == true) {
											xhtm += ', ';
										}
										xhtm += $.t("Dew Point") + ": " + item.DewPoint + '&deg; ' + $scope.config.TempSign;
									}
									xhtm +=
										'</td>\n' +
										'\t      <td id="lastupdate" class="lastupdate"><span>' + item.LastUpdate + '</span></td>\n' +
										'\t    </tr>\n' +
										'\t    </table>\n' +
										'\t  </div><!--item end-->\n' +
										'\t</div>\n';
								}
								htmlcontent += xhtm;
								jj += 1;

							}
						}); //temp devices
						if (bHaveAddedDivider == true) {
							//close previous devider
							htmlcontent += '</div>\n';
						}
						if (($scope.config.DashboardType == 2) || (window.myglobals.ismobile == true)) {
							htmlcontent += '\t    </table>\n';
						}
						if (jj > 0) {
							htmlcontent += '</section>';
						}


						//Weather Sensors
						jj = 0;
						bHaveAddedDivider = false;
						$.each(data.result, function (i, item) {
							if (
								((typeof item.Rain != 'undefined') || (typeof item.Visibility != 'undefined') || (typeof item.UVI != 'undefined') || (typeof item.Radiation != 'undefined') || (typeof item.Direction != 'undefined') || (typeof item.Barometer != 'undefined')) &&
								(item.Favorite != 0)
							) {
								totdevices += 1;
								if (jj == 0) {
									//first time
									htmlcontent += '<section class="dashCategory" id="dashWeather">';
									if (($scope.config.DashboardType == 2) || (window.myglobals.ismobile == true)) {
										if (htmlcontent != "") {
											htmlcontent += '<br>';
										}
										htmlcontent += '\t    <table class="mobileitem">\n';
										htmlcontent += '\t    <thead>\n';
										htmlcontent += '\t    <tr>\n';
										htmlcontent += '\t    		<th>' + $.t('Weather Sensors') + '</th>\n';
										htmlcontent += '\t    		<th style="text-align:right"><a id="cWeather" href="javascript:SwitchLayout(\'Weather\')"><img src="images/next.png"></a></th>\n';
										htmlcontent += '\t    </tr>\n';
										htmlcontent += '\t    </thead>\n';
									}
									else {
										htmlcontent += '<h2>' + $.t('Weather Sensors') + ':</h2>\n';
									}
								}
								if (jj % rowItems == 0) {
									//add devider
									if (bHaveAddedDivider == true) {
										//close previous devider
										htmlcontent += '</div>\n';
									}
									htmlcontent += '<div class="row divider">\n';
									bHaveAddedDivider = true;
								}
								// generate protected/timeout/lowbattery status
								var backgroundClass = $rootScope.GetItemBackgroundStatus(item);
								var xhtm = "";
								if (($scope.config.DashboardType == 2) || (window.myglobals.ismobile == true)) {
									var vname = item.Name;
									if (typeof item.UVI != 'undefined') {
										vname = '<img src="images/next.png" onclick="ShowUVLog(\'#dashcontent\',\'ShowFavorites\',' + item.idx + ',\'' + escape(item.Name) + '\');" height="16" width="16">' + " " + item.Name;
									}
									else if (typeof item.Visibility != 'undefined') {
										vname = '<img src="images/next.png" onclick="ShowGeneralGraph(\'#dashcontent\',\'ShowFavorites\',' + item.idx + ',\'' + escape(item.Name) + '\',' + item.SwitchTypeVal + ', \'Visibility\');" height="16" width="16">' + " " + item.Name;
									}
									else if (typeof item.Radiation != 'undefined') {
										vname = '<img src="images/next.png" onclick="ShowGeneralGraph(\'#dashcontent\',\'ShowFavorites\',' + item.idx + ',\'' + escape(item.Name) + '\',' + item.SwitchTypeVal + ', \'Radiation\');" height="16" width="16">' + " " + item.Name;
									}
									else if (typeof item.Direction != 'undefined') {
										vname = '<img src="images/next.png" onclick="ShowWindLog(\'#dashcontent\',\'ShowFavorites\',' + item.idx + ',\'' + escape(item.Name) + '\');" height="16" width="16">' + " " + item.Name;
									}
									else if (typeof item.Rain != 'undefined') {
										vname = '<img src="images/next.png" onclick="ShowRainLog(\'#dashcontent\',\'ShowFavorites\',' + item.idx + ',\'' + escape(item.Name) + '\');" height="16" width="16">' + " " + item.Name;
									}
									else if (typeof item.Barometer != 'undefined') {
										vname = '<img src="images/next.png" onclick="ShowBaroLog(\'#dashcontent\',\'ShowFavorites\',' + item.idx + ',\'' + escape(item.Name) + '\');" height="16" width="16">' + " " + item.Name;
									}
									xhtm +=
										'\t    <tr id="weather_' + item.idx + '">\n' +
										'\t      <td id="name" class="name">' + vname + '</td>\n';
									var status = "";
									if (typeof item.Rain != 'undefined') {
										status += item.Rain + ' mm';
										if (typeof item.RainRate != 'undefined') {
											if (item.RainRate != 0) {
												status += ', Rate: ' + item.RainRate + ' mm/h';
											}
										}
									}
									else if (typeof item.Visibility != 'undefined') {
										status += item.Data;
									}
									else if (typeof item.UVI != 'undefined') {
										status += item.UVI + ' UVI';
									}
									else if (typeof item.Radiation != 'undefined') {
										status += item.Data;
									}
									else if (typeof item.Direction != 'undefined') {
										status = item.Direction + ' ' + item.DirectionStr;
										if (typeof item.Speed != 'undefined') {
											status += ', ' + $.t('Speed') + ': ' + item.Speed + ' ' + $scope.config.WindSign;
										}
										if (typeof item.Gust != 'undefined') {
											status += ', ' + $.t('Gust') + ': ' + item.Gust + ' ' + $scope.config.WindSign;
										}
									}
									else if (typeof item.Barometer != 'undefined') {
										if (typeof item.ForecastStr != 'undefined') {
											status = item.Barometer + ' hPa, ' + $.t('Prediction') + ': ' + $.t(item.ForecastStr);
										}
										else {
											status = item.Barometer + ' hPa';
										}
										if (typeof item.Altitude != 'undefined') {
											status += ', Altitude: ' + item.Altitude + ' meter';
										}
									}
									xhtm +=
										'\t      <td id="status" class="status">' + status + '</td>\n' +
										'\t    </tr>\n';
								}
								else {
									if ($scope.config.DashboardType == 0) {
										xhtm = '\t<div class="span4 movable" id="weather_' + item.idx + '">\n';
									}
									else if ($scope.config.DashboardType == 1) {
										xhtm = '\t<div class="span3 movable" id="weather_' + item.idx + '">\n';
									}
									xhtm += '\t  <div id="bstatus" class="item ' + backgroundClass + '">\n';
									xhtm += '\t    <table id="itemtablesmall" class="itemtablesmall" border="0" cellpadding="0" cellspacing="0">\n';
									xhtm += '\t    <tr>\n';
									xhtm += '\t      <td id="name" class="name">' + item.Name + '</td>\n';
									xhtm += '\t      <td id="bigtext" class="bigtext"><span>';
									if (typeof item.Barometer != 'undefined') {
										xhtm += item.Barometer + ' hPa';
									}
									else if (typeof item.Rain != 'undefined') {
										xhtm += item.Rain + ' mm';
									}
									else if (typeof item.Visibility != 'undefined') {
										xhtm += item.Data;
									}
									else if (typeof item.UVI != 'undefined') {
										xhtm += item.UVI + ' UVI';
									}
									else if (typeof item.Radiation != 'undefined') {
										xhtm += item.Data;
									}
									else if (typeof item.Direction != 'undefined') {
										xhtm += item.DirectionStr;
										if (typeof item.Speed != 'undefined') {
											xhtm += ' / ' + item.Speed + ' ' + $scope.config.WindSign;
										}
										else if (typeof item.Gust != 'undefined') {
											xhtm += ' / ' + item.Gust + ' ' + $scope.config.WindSign;
										}
									}
									xhtm += '</span></td>\n';
									xhtm += '\t      <td id="img" class="img img1"><img src="images/';
									if (typeof item.Rain != 'undefined') {
										xhtm += 'rain48.png" class="lcursor" onclick="ShowRainLog(\'#dashcontent\',\'ShowFavorites\',' + item.idx + ',\'' + escape(item.Name) + '\');" height="40" width="40"></td>\n' +
											'\t      <td id="status" class="status">' + item.Rain + ' mm';
										if (typeof item.RainRate != 'undefined') {
											if (item.RainRate != 0) {
												xhtm += ', Rate: ' + item.RainRate + ' mm/h';
											}
										}
									}
									else if (typeof item.Visibility != 'undefined') {
										xhtm += 'visibility48.png" class="lcursor" onclick="ShowGeneralGraph(\'#dashcontent\',\'ShowFavorites\',' + item.idx + ',\'' + escape(item.Name) + '\',' + item.SwitchTypeVal + ', \'Visibility\');" height="40" width="40"></td>\n' +
											'\t      <td id="status" class="status">' + item.Data;
									}
									else if (typeof item.UVI != 'undefined') {
										xhtm += 'uv48.png" class="lcursor" onclick="ShowUVLog(\'#dashcontent\',\'ShowFavorites\',' + item.idx + ',\'' + escape(item.Name) + '\');" height="40" width="40"></td>\n' +
											'\t      <td id="status" class="status">' + item.UVI + ' UVI';
										if (typeof item.Temp != 'undefined') {
											xhtm += ', Temp: ' + item.Temp + '&deg; ' + $scope.config.TempSign;
										}
									}
									else if (typeof item.Radiation != 'undefined') {
										xhtm += 'radiation48.png" class="lcursor" onclick="ShowGeneralGraph(\'#dashcontent\',\'ShowFavorites\',' + item.idx + ',\'' + escape(item.Name) + '\',' + item.SwitchTypeVal + ', \'Radiation\');" height="40" width="40"></td>\n' +
											'\t      <td id="status" class="status">' + item.Data;
									}
									else if (typeof item.Direction != 'undefined') {
										xhtm += 'Wind' + item.DirectionStr + '.png" class="lcursor" onclick="ShowWindLog(\'#dashcontent\',\'ShowFavorites\',' + item.idx + ',\'' + escape(item.Name) + '\');" height="40" width="40"></td>\n' +
											'\t      <td id="status" class="status">' + item.Direction + ' ' + item.DirectionStr;
										if (typeof item.Speed != 'undefined') {
											xhtm += ', ' + $.t('Speed') + ': ' + item.Speed + ' ' + $scope.config.WindSign;
										}
										if (typeof item.Gust != 'undefined') {
											xhtm += ', ' + $.t('Gust') + ': ' + item.Gust + ' ' + $scope.config.WindSign;
										}
										xhtm += '<br>\n';
										if (typeof item.Temp != 'undefined') {
											xhtm += $.t('Temp') + ': ' + item.Temp + '&deg; ' + $scope.config.TempSign;
										}
										if (typeof item.Chill != 'undefined') {
											if (typeof item.Temp != 'undefined') {
												xhtm += ', ';
											}
											xhtm += $.t('Chill') + ': ' + item.Chill + '&deg; ' + $scope.config.TempSign;
										}
									}
									else if (typeof item.Barometer != 'undefined') {
										xhtm += 'baro48.png" class="lcursor" onclick="ShowBaroLog(\'#dashcontent\',\'ShowFavorites\',' + item.idx + ',\'' + escape(item.Name) + '\');" height="40" width="40"></td>\n' +
											'\t      <td id="status" class="status">' + item.Barometer + ' hPa';
										if (typeof item.ForecastStr != 'undefined') {
											xhtm += ', ' + $.t('Prediction') + ': ' + $.t(item.ForecastStr);
										}
										if (typeof item.Altitude != 'undefined') {
											xhtm += ', Altitude: ' + item.Altitude + ' meter';
										}
									}
									xhtm +=
										'</td>\n' +
										'\t      <td id="lastupdate" class="lastupdate"><span>' + item.LastUpdate + '</span></td>\n' +
										'\t    </tr>\n' +
										'\t    </table>\n' +
										'\t  </div><!--item end-->\n' +
										'\t</div>\n';
								}
								htmlcontent += xhtm;
								jj += 1;
							}
						}); //weather devices
						if (bHaveAddedDivider == true) {
							//close previous devider
							htmlcontent += '</div>\n';
						}
						if (($scope.config.DashboardType == 2) || (window.myglobals.ismobile == true)) {
							htmlcontent += '\t    </table>\n';
						}
						if (jj > 0) {
							htmlcontent += '</section>';
						}




						//security devices
						jj = 0;
						bHaveAddedDivider = false;
						$.each(data.result, function (i, item) {
							if ((item.Type.indexOf('Security') == 0) && (item.Favorite != 0)) {
								totdevices += 1;
								if (jj == 0) {
									//first time
									htmlcontent += '<section class="dashCategory" id="dashSecurity">';
									if (($scope.config.DashboardType == 2) || (window.myglobals.ismobile == true)) {
										if (htmlcontent != "") {
											htmlcontent += '<br>';
										}
										htmlcontent += '\t    <table class="mobileitem">\n';
										htmlcontent += '\t    <thead>\n';
										htmlcontent += '\t    <tr>\n';
										htmlcontent += '\t    		<th>' + $.t('Security Devices') + '</th>\n';
										htmlcontent += '\t    		<th style="text-align:right"><a id="cLightSwitches" href="javascript:SwitchLayout(\'LightSwitches\')"><img src="images/next.png"></a></th>\n';
										htmlcontent += '\t    </tr>\n';
										htmlcontent += '\t    </thead>\n';
									}
									else {
										htmlcontent += '<h2>' + $.t('Security Devices') + ':</h2>\n';
									}
								}
								if (jj % rowItems == 0) {
									//add devider
									if (bHaveAddedDivider == true) {
										//close previous devider
										htmlcontent += '</div>\n';
									}
									htmlcontent += '<div class="row divider">\n';
									bHaveAddedDivider = true;
								}
								var xhtm = "";
								if (($scope.config.DashboardType == 2) || (window.myglobals.ismobile == true)) {
									xhtm +=
										'\t    <tr id="security_' + item.idx + '">\n' +
										'\t      <td id="name" class="name">' + item.Name + '</td>\n';
									var status = TranslateStatus(item.Status);

									xhtm += '\t      <td id="status" class="status">';
									xhtm += status;
									if (item.SubType == "Security Panel") {
										xhtm += ' <a href="secpanel/"><img src="images/security48.png" class="lcursor" height="16" width="16"></a>';
									}
									else if (item.SubType.indexOf('remote') > 0) {
										if ((item.Status.indexOf('Arm') >= 0) || (item.Status.indexOf('Panic') >= 0)) {
											xhtm += ' <img src="images/remote.png" title="' + $.t("Turn Alarm Off") + '" onclick="SwitchLight(' + item.idx + ',\'Off\',RefreshFavorites,' + item.Protected + ');" class="lcursor" height="16" width="16">';
										}
										else {
											xhtm += ' <img src="images/remote.png" title="' + $.t("Turn Alarm On") + '" onclick="ArmSystem(' + item.idx + ',\'On\',RefreshFavorites,' + item.Protected + ');" class="lcursor" height="16" width="16">';
										}
									}

									xhtm += '</td>\n\t    </tr>\n';
								}
								else {
									if ($scope.config.DashboardType == 0) {
										xhtm = '\t<div class="span4 movable" id="security_' + item.idx + '">\n';
									}
									else if ($scope.config.DashboardType == 1) {
										xhtm = '\t<div class="span3 movable" id="security_' + item.idx + '">\n';
									}
									var backgroundClass = $rootScope.GetItemBackgroundStatus(item);
									xhtm += '\t  <div id="bstatus" class="item ' + backgroundClass + '">\n';
									if ($scope.config.DashboardType == 0) {
										xhtm += '\t    <table id="itemtablesmall" class="itemtablesmall" border="0" cellpadding="0" cellspacing="0">\n';
									}
									else if ($scope.config.DashboardType == 1) {
										xhtm += '\t    <table id="itemtablesmall" class="itemtablesmall" border="0" cellpadding="0" cellspacing="0">\n';
									}
									xhtm += '\t    <tr>\n' +
										'\t      <td id="name" class="name">' + item.Name + '</td>\n' +
										'\t      <td id="bigtext" class="bigtext"><span>' + TranslateStatusShort(item.Status) + '</span></td>\n';

									if (item.SubType == "Security Panel") {
										xhtm += '\t      <td id="img" class="img img1"><a href="secpanel/"><img src="images/security48.png" class="lcursor" height="40" width="40"></a></td>\n';
									}
									else if (item.SubType.indexOf('remote') > 0) {
										if ((item.Status.indexOf('Arm') >= 0) || (item.Status.indexOf('Panic') >= 0)) {
											xhtm += '\t      <td id="img" class="img img1"><img src="images/remote48.png" title="' + $.t("Turn Alarm Off") + '" onclick="SwitchLight(' + item.idx + ',\'Off\',RefreshFavorites,' + item.Protected + ');" class="lcursor" height="40" width="40"></td>\n';
										}
										else {
											xhtm += '\t      <td id="img" class="img img1"><img src="images/remote48.png" title="' + $.t("Turn Alarm On") + '" onclick="ArmSystem(' + item.idx + ',\'On\',RefreshFavorites,' + item.Protected + ');" class="lcursor" height="40" width="40"></td>\n';
										}
									}
									else if (item.SwitchType == "Smoke Detector") {
										if (
											(item.Status == "Panic") ||
											(item.Status == "On")
										) {
											xhtm += '\t      <td id="img" class="img img1"><img src="images/smoke48on.png" height="40" width="40"></td>\n';
										}
										else {
											xhtm += '\t      <td id="img" class="img img1"><img src="images/smoke48off.png" height="40" width="40"></td>\n';
										}
									}
									else if (item.SubType == "X10 security") {
										if (item.Status.indexOf('Normal') >= 0) {
											xhtm += '\t      <td id="img" class="img img1"><img src="images/security48.png" title="' + $.t("Turn Alarm On") + '" onclick="SwitchLight(' + item.idx + ',\'' + ((item.Status == "Normal Delayed") ? "Alarm Delayed" : "Alarm") + '\',RefreshFavorites,' + item.Protected + ');" class="lcursor" height="40" width="40"></td>\n';
										}
										else {
											xhtm += '\t      <td id="img" class="img img1"><img src="images/Alarm48_On.png" title="' + $.t("Turn Alarm Off") + '" onclick="SwitchLight(' + item.idx + ',\'' + ((item.Status == "Alarm Delayed") ? "Normal Delayed" : "Normal") + '\',RefreshFavorites,' + item.Protected + ');" class="lcursor" height="40" width="40"></td>\n';
										}
									}
									else if (item.SubType == "X10 security motion") {
										if ((item.Status == "No Motion")) {
											xhtm += '\t      <td id="img" class="img img1"><img src="images/security48.png" title="' + $.t("Turn Alarm On") + '" onclick="SwitchLight(' + item.idx + ',\'Motion\',RefreshFavorites,' + item.Protected + ');" class="lcursor" height="40" width="40"></td>\n';
										}
										else {
											xhtm += '\t      <td id="img" class="img img1"><img src="images/Alarm48_On.png" title="' + $.t("Turn Alarm Off") + '" onclick="SwitchLight(' + item.idx + ',\'No Motion\',RefreshFavorites,' + item.Protected + ');" class="lcursor" height="40" width="40"></td>\n';
										}
									}
									else if ((item.Status.indexOf('Alarm') >= 0) || (item.Status.indexOf('Tamper') >= 0)) {
										xhtm += '\t      <td id="img" class="img img1"><img src="images/Alarm48_On.png" height="40" width="40"></td>\n';
									}
									else if (item.SubType.indexOf('Meiantech') >= 0) {
										if ((item.Status.indexOf('Arm') >= 0) || (item.Status.indexOf('Panic') >= 0)) {
											xhtm += '\t      <td id="img" class="img img1"><img src="images/security48.png" title="' + $.t("Turn Alarm Off") + '" onclick="SwitchLight(' + item.idx + ',\'Off\',RefreshFavorites,' + item.Protected + ');" class="lcursor" height="40" width="40"></td>\n';
										}
										else {
											xhtm += '\t      <td id="img" class="img img1"><img src="images/security48.png" title="' + $.t("Turn Alarm On") + '" onclick="ArmSystemMeiantech(' + item.idx + ',\'On\',RefreshFavorites,' + item.Protected + ');" class="lcursor" height="40" width="40"></td>\n';
										}
									}
									else {
										if (item.SubType.indexOf('KeeLoq') >= 0) {
											xhtm += '\t      <td id="img" class="img img1"><img src="images/pushon48.png" title="' + $.t("Turn On") + '" onclick="SwitchLight(' + item.idx + ',\'On\',RefreshFavorites,' + item.Protected + ');" class="lcursor" height="40" width="40"></td>\n';
										}
										else {
											xhtm += '\t      <td id="img" class="img img1"><img src="images/security48.png" height="40" width="40"></td>\n';
										}
									}
									xhtm +=
										'\t      <td id="status" class="status"></td>\n' +
										'\t      <td id="lastupdate" class="lastupdate"><span>' + item.LastUpdate + '</span></td>\n' +
										'\t    </tr>\n' +
										'\t    </table>\n' +
										'\t  </div><!--item end-->\n' +
										'\t</div>\n';
								}
								htmlcontent += xhtm;
								jj += 1;
							}
						}); //security devices
						if (bHaveAddedDivider == true) {
							//close previous divider
							htmlcontent += '</div>\n';
						}
						if (($scope.config.DashboardType == 2) || (window.myglobals.ismobile == true)) {
							htmlcontent += '\t    </table>\n';
						}
						if (jj > 0) {
							htmlcontent += '</section>';
						}

						//Gizmocuz: Don't know how did this ? But this should be under utility devices!
						//Please do so
/*
						//evohome devices
						jj = 0;
						bHaveAddedDivider = false;
						$.each(data.result, function (i, item) {
							if ((item.Type.indexOf('Heating') == 0) && (item.Favorite != 0)) {
								totdevices += 1;
								if (jj == 0) {
									//first time
									htmlcontent += '<section class="dashCategory" id="dashEvohome">';
									if (($scope.config.DashboardType == 2) || (window.myglobals.ismobile == true)) {
										if (htmlcontent != "") {
											htmlcontent += '<br>';
										}
										htmlcontent += '\t    <table class="mobileitem">\n';
										htmlcontent += '\t    <thead>\n';
										htmlcontent += '\t    <tr>\n';
										htmlcontent += '\t    		<th>' + $.t('evohome Devices') + '</th>\n';
										htmlcontent += '\t    		<th style="text-align:right"><a id="cevohome" href="javascript:SwitchLayout(\'LightSwitches\')"><img src="images/next.png"></a></th>\n';
										htmlcontent += '\t    </tr>\n';
										htmlcontent += '\t    </thead>\n';
									}
									else {
										htmlcontent += '<h2>' + $.t('evohome Devices') + ':</h2>\n';
									}
								}
								if (jj % rowItems == 0) {
									//add devider
									if (bHaveAddedDivider == true) {
										//close previous devider
										htmlcontent += '</div>\n';
									}
									htmlcontent += '<div class="row divider">\n';
									bHaveAddedDivider = true;
								}
								var xhtm = "";
								if (($scope.config.DashboardType == 2) || (window.myglobals.ismobile == true)) {
									if (item.SubType == "Evohome") {
										xhtm +=
											'\t    <tr id="evohome_' + item.idx + '">\n' +
											'\t      <td id="name" class="name">' + item.Name + '</td>\n';
										xhtm += EvohomePopupMenu(item, 'evomobile');
										xhtm += '\n\r  </tr>\n';
									}
								}
								else {
									if (item.SubType == "Evohome") {
										if ($scope.config.DashboardType == 0) {
											xhtm = '\t<div class="span4 movable" id="evohome_' + item.idx + '">\n';
										}
										else if ($scope.config.DashboardType == 1) {
											xhtm = '\t<div class="span3 movable" id="evohome_' + item.idx + '">\n';
										}
										xhtm += '\t  <div class="item">\n';
										if ($scope.config.DashboardType == 0) {
											xhtm += '\t    <table id="itemtablesmall" class="itemtablesmall" border="0" cellpadding="0" cellspacing="0">\n';
										}
										else if ($scope.config.DashboardType == 1) {
											xhtm += '\t    <table id="itemtablesmall" class="itemtablesmall" border="0" cellpadding="0" cellspacing="0">\n';
										}
										var backgroundClass = $rootScope.GetItemBackgroundStatus(item);

										xhtm +=
											'\t    <tr class="' + backgroundClass + '">\n' +
											'\t      <td id="name" class="name ' + backgroundClass + '">' + item.Name + '</td>\n' +
											'\t      <td id="bigtext" class="bigtext"><span></span></td>\n';
										xhtm += EvohomePopupMenu(item, 'evomini');
										xhtm +=
											'\t      <td id="status" class="status">' + TranslateStatus(EvoDisplayTextMode(item.Status)) + '</td>\n' +
											'\t      <td id="lastupdate" class="lastupdate"><span>' + item.LastUpdate + '</span></td>\n' +
											'\t    </tr>\n' +
											'\t    </table>\n' +
											'\t  </div><!--item end-->\n' +
											'\t</div>\n';
									}
								}
								htmlcontent += xhtm;
								jj += 1;
							}
						}); //evohome devices
						if (bHaveAddedDivider == true) {
							//close previous devider
							htmlcontent += '</div>\n';
						}
						if (($scope.config.DashboardType == 2) || (window.myglobals.ismobile == true)) {
							htmlcontent += '\t    </table>\n';
						}
						if (jj > 0) {
							htmlcontent += '</section>';
						}
*/
						//Utility Sensors
						jj = 0;
						bHaveAddedDivider = false;
						$.each(data.result, function (i, item) {
							if (
								(
									(typeof item.Counter != 'undefined') ||
									(item.Type == "Current") ||
									(item.Type == "Energy") ||
									(item.SubType == "kWh") ||
									(item.Type == "Current/Energy") ||
									(item.Type == "Power") ||
									(item.Type == "Air Quality") ||
									(item.Type == "Lux") ||
									(item.Type == "Weight") ||
									(item.Type == "Usage") ||
									(item.SubType == "Percentage") ||
									//((item.Type=="Fan")&&(typeof item.SubType != 'undefined')&&(item.SubType.indexOf('Lucci')!=0))||
									((item.Type == "Thermostat") && (item.SubType == "SetPoint")) ||
									(item.SubType == "Soil Moisture") ||
									(item.SubType == "Leaf Wetness") ||
									(item.SubType == "Voltage") ||
									(item.SubType == "Distance") ||
									(item.SubType == "Current") ||
									(item.SubType == "Text") ||
									(item.SubType == "Alert") ||
									(item.SubType == "Pressure") ||
									(item.SubType == "A/D") ||
									(item.SubType == "Thermostat Mode") ||
									(item.SubType == "Thermostat Fan Mode") ||
									(item.SubType == "Smartwares") ||
									(item.SubType == "Waterflow") ||
									(item.SubType == "Sound Level") ||
									(item.SubType == "Custom Sensor")
								) &&
								(item.Favorite != 0)
							) {
								totdevices += 1;
								if (jj == 0) {
									//first time
									htmlcontent += '<section class="dashCategory" id="dashUtility">';
									// mobile util start
									if (($scope.config.DashboardType == 2) || (window.myglobals.ismobile == true)) {
										if (htmlcontent != "") {
											htmlcontent += '<br>';
										}
										htmlcontent += '\t    <table class="mobileitem">\n';
										htmlcontent += '\t    <thead>\n';
										htmlcontent += '\t    <tr>\n';
										htmlcontent += '\t    		<th>' + $.t('Utility Sensors') + '</th>\n';
										htmlcontent += '\t    		<th style="text-align:right"><a id="cUtility" href="javascript:SwitchLayout(\'Utility\')"><img src="images/next.png"></a></th>\n';
										htmlcontent += '\t    </tr>\n';
										htmlcontent += '\t    </thead>\n';
									}
									else {
										htmlcontent += '<h2>' + $.t('Utility Sensors') + ':</h2>\n';
									}
								}
								if (jj % rowItems == 0) {
									//add devider
									if (bHaveAddedDivider == true) {
										//close previous devider
										htmlcontent += '</div>\n';
									}
									htmlcontent += '<div class="row divider">\n';
									bHaveAddedDivider = true;
								}
								var xhtm = "";
								if (($scope.config.DashboardType == 2) || (window.myglobals.ismobile == true)) {
									var vname = item.Name;
									if (typeof item.Counter != 'undefined') {
										if (item.Type == "RFXMeter") {
											vname = '<img src="images/next.png" onclick="ShowCounterLog(\'#dashcontent\',\'ShowFavorites\',' + item.idx + ',\'' + escape(item.Name) + '\', ' + item.SwitchTypeVal + ');" height="16" width="16">' + " " + item.Name;
										}
										else {
											if ((item.Type == "P1 Smart Meter") && (item.SubType == "Energy")) {
												vname = '<img src="images/next.png" onclick="ShowSmartLog(\'#dashcontent\',\'ShowFavorites\',' + item.idx + ',\'' + escape(item.Name) + '\', ' + item.SwitchTypeVal + ');" height="16" width="16">' + " " + item.Name;
											}
											else if ((item.Type == "P1 Smart Meter") && (item.SubType == "Gas")) {
												vname = '<img src="images/next.png" onclick="ShowCounterLog(\'#dashcontent\',\'ShowFavorites\',' + item.idx + ',\'' + escape(item.Name) + '\', ' + item.SwitchTypeVal + ');" height="16" width="16">' + " " + item.Name;
											}
											else if ((item.Type == "YouLess Meter") && (item.SwitchTypeVal == 0 || item.SwitchTypeVal == 4)) {
												vname = '<img src="images/next.png" onclick="ShowCounterLogSpline(\'#dashcontent\',\'ShowFavorites\',' + item.idx + ',\'' + escape(item.Name) + '\', ' + item.SwitchTypeVal + ');" height="16" width="16">' + " " + item.Name;
											}
											else {
												vname = '<img src="images/next.png" onclick="ShowCounterLog(\'#dashcontent\',\'ShowFavorites\',' + item.idx + ',\'' + escape(item.Name) + '\', ' + item.SwitchTypeVal + ');" height="16" width="16">' + " " + item.Name;
											}
										}
									}
									else if ((item.Type == "Current") || (item.Type == "Current/Energy")) {
										vname = '<img src="images/next.png" onclick="ShowCurrentLog(\'#dashcontent\',\'ShowFavorites\',' + item.idx + ',\'' + escape(item.Name) + '\', ' + item.displaytype + ');" height="16" width="16">' + " " + item.Name;
									}
									else if ((item.Type == "Energy") || (item.SubType == "kWh") || (item.SubType == "Power")) {
										vname = '<img src="images/next.png" onclick="ShowCounterLogSpline(\'#dashcontent\',\'ShowFavorites\',' + item.idx + ',\'' + escape(item.Name) + '\', ' + item.SwitchTypeVal + ');" height="16" width="16">' + " " + item.Name;
									}
									else if (item.Type == "Air Quality") {
										vname = '<img src="images/next.png" onclick="ShowAirQualityLog(\'#dashcontent\',\'ShowFavorites\',' + item.idx + ',\'' + escape(item.Name) + '\');" height="16" width="16">' + " " + item.Name;
									}
									else if (item.SubType == "Percentage") {
										vname = '<img src="images/next.png" onclick="ShowPercentageLog(\'#dashcontent\',\'ShowFavorites\',' + item.idx + ',\'' + escape(item.Name) + '\');" height="16" width="16">' + " " + item.Name;
									}
									else if (item.SubType == "Custom Sensor") {
										vname = '<img src="images/' + item.Image + '48_On.png" onclick="ShowGeneralGraph(\'#dashcontent\',\'ShowFavorites\',' + item.idx + ',\'' + escape(item.Name) + '\', \'' + escape(item.SensorUnit) + '\', \'' + item.SubType + '\');" height="16" width="16">' + " " + item.Name;
									}
									else if (item.Type == "Fan") {
										vname = '<img src="images/next.png" class="fanicon" onclick="ShowFanLog(\'#dashcontent\',\'ShowFavorites\',' + item.idx + ',\'' + escape(item.Name) + '\');" height="16" width="16">' + " " + item.Name;
									}
									else if (item.Type == "Lux") {
										vname = '<img src="images/next.png" onclick="ShowLuxLog(\'#dashcontent\',\'ShowFavorites\',' + item.idx + ',\'' + escape(item.Name) + '\');" height="16" width="16">' + " " + item.Name;
									}
									else if (item.Type == "Usage") {
										vname = '<img src="images/next.png" onclick="ShowUsageLog(\'#dashcontent\',\'ShowFavorites\',' + item.idx + ',\'' + escape(item.Name) + '\', ' + item.SwitchTypeVal + ');" height="16" width="16">' + " " + item.Name;
									}
									else if (item.SubType == "Soil Moisture") {
										vname = '<img src="images/next.png" onclick="ShowGeneralGraph(\'#dashcontent\',\'ShowFavorites\',' + item.idx + ',\'' + escape(item.Name) + '\', ' + item.SwitchTypeVal + ', \'' + item.SubType + '\');" height="16" width="16">' + " " + item.Name;
									}
									else if (item.SubType == "Distance") {
										vname = '<img src="images/next.png" onclick="ShowGeneralGraph(\'#dashcontent\',\'ShowFavorites\',' + item.idx + ',\'' + escape(item.Name) + '\', ' + item.SwitchTypeVal + ', \'DistanceGeneral\');" height="16" width="16">' + " " + item.Name;
									}
									else if ((item.SubType == "Voltage") || (item.SubType == "Current") || (item.SubType == "A/D")) {
										vname = '<img src="images/next.png" onclick="ShowGeneralGraph(\'#dashcontent\',\'ShowFavorites\',' + item.idx + ',\'' + escape(item.Name) + '\', ' + item.SwitchTypeVal + ', \'VoltageGeneral\');" height="16" width="16">' + " " + item.Name;
									}
									else if (item.SubType == "Text") {
										vname = '<img src="images/next.png" onclick="ShowTextLog(\'#dashcontent\',\'ShowFavorites\',' + item.idx + ',\'' + escape(item.Name) + '\');" height="16" width="16">' + " " + item.Name;
									}
									else if (item.SubType == "Alert") {
										vname = '<img src="images/next.png" onclick="ShowTextLog(\'#dashcontent\',\'ShowFavorites\',' + item.idx + ',\'' + escape(item.Name) + '\');" height="16" width="16">' + " " + item.Name;
									}
									else if (item.SubType == "Pressure") {
										vname = '<img src="images/next.png" onclick="ShowGeneralGraph(\'#dashcontent\',\'ShowFavorites\',' + item.idx + ',\'' + escape(item.Name) + '\',' + item.SwitchTypeVal + ', \'' + item.SubType + '\');" height="16" width="16">' + " " + item.Name;
									}
									else if ((item.Type == "Thermostat") && (item.SubType == "SetPoint")) {
										vname = '<img src="images/next.png" onclick="ShowTempLog(\'#dashcontent\',\'ShowFavorites\',' + item.idx + ',\'' + escape(item.Name) + '\');" height="16" width="16">' + " " + item.Name;
									}
									else if (item.SubType == "Smartwares") {
										vname = '<img src="images/next.png" onclick="ShowTempLog(\'#dashcontent\',\'ShowFavorites\',' + item.idx + ',\'' + escape(item.Name) + '\');" height="16" width="16">' + " " + item.Name;
									}
									else if (item.SubType == "Sound Level") {
										vname = '<img src="images/next.png" onclick="ShowGeneralGraph(\'#dashcontent\',\'ShowFavorites\',' + item.idx + ',\'' + escape(item.Name) + '\',' + item.SwitchTypeVal + ', \'' + item.SubType + '\');" height="16" width="16">' + " " + item.Name;
									}
									else if (item.SubType == "Waterflow") {
										vname = '<img src="images/next.png" onclick="ShowGeneralGraph(\'#dashcontent\',\'ShowFavorites\',' + item.idx + ',\'' + escape(item.Name) + '\', ' + item.SwitchTypeVal + ', \'' + item.SubType + '\');" height="16" width="16">' + " " + item.Name;
									}

									var status = "";
									if (typeof item.Counter != 'undefined') {
										if ($scope.config.DashboardType == 0) {
											status = '' + $.t("Usage") + ': ' + item.CounterToday;
										}
										else {
											if ((typeof item.CounterDeliv != 'undefined') && (item.CounterDeliv != 0)) {
												status = 'U: T: ' + item.CounterToday;
											} else {
												status = 'T: ' + item.CounterToday;
											}
										}
									}
									else if (item.Type == "Current") {
										status = item.Data;
									}
									else if (
										(item.Type == "Energy") ||
										(item.Type == "Current/Energy") ||
										(item.Type == "Power") ||
										(item.SubType == "kWh") ||
										(item.Type == "Air Quality") ||
										(item.Type == "Lux") ||
										(item.Type == "Weight") ||
										(item.Type == "Usage") ||
										(item.SubType == "Percentage") ||
										(item.Type == "Fan") ||
										(item.SubType == "Soil Moisture") ||
										(item.SubType == "Leaf Wetness") ||
										(item.SubType == "Voltage") ||
										(item.SubType == "Distance") ||
										(item.SubType == "Current") ||
										(item.SubType == "Text") ||
										(item.SubType == "Pressure") ||
										(item.SubType == "A/D") ||
										(item.SubType == "Waterflow") ||
										(item.SubType == "Sound Level") ||
										(item.SubType == "Custom Sensor")
									) {
										if (typeof item.CounterToday != 'undefined') {
											status += 'T: ' + item.CounterToday;
										}
										else {
											status = item.Data;
										}
									}
									else if (item.SubType == "Alert") {
										var aLevel = item.Level;
										if (aLevel > 4) aLevel = 4;
										status = item.Data + ' <img src="images/Alert48_' + aLevel + '.png" height="16" width="16">';
									}
									else if ((item.Type == "Thermostat") && (item.SubType == "SetPoint")) {
										status = ' <button class="btn btn-mini btn-info" type="button" onclick="ShowSetpointPopup(event, ' + item.idx + ', ShowFavorites, ' + item.Protected + ', ' + item.Data + ',true);">' + item.Data + '\u00B0 ' + $scope.config.TempSign + '</button> ';
									}
									else if (item.SubType == "Smartwares") {
										status = item.Data + '\u00B0 ' + $scope.config.TempSign;
										status += ' <button class="btn btn-mini btn-info" type="button" onclick="ShowSetpointPopup(event, ' + item.idx + ', ShowFavorites, ' + item.Protected + ', ' + item.Data + ',true);">' + $.t("Set") + '</button> ';
									}
									else if ((item.SubType == "Thermostat Mode") || (item.SubType == "Thermostat Fan Mode")) {
										status = item.Data;
									}

									var bHaveReturnUsage = false;
									if (typeof item.CounterDeliv != 'undefined') {
										if (item.UsageDeliv.charAt(0) != 0) {
											bHaveReturnUsage = true;
										}
									}

									if (typeof item.Usage != 'undefined') {
										if ($scope.config.DashboardType == 0) {
											status += '<br>' + $.t("Actual") + ': ' + item.Usage;
										}
										else {
											if (!bHaveReturnUsage) {
												status += ", A: " + item.Usage;
											}
										}
									}

									if (typeof item.CounterDeliv != 'undefined') {
										if (item.CounterDeliv != 0) {
											if ($scope.config.DashboardType == 0) {
												status += '<br>' + $.t("Return") + ': ' + item.CounterDelivToday;
												status += '<br>' + $.t("Actual") + ': -' + item.UsageDeliv;
											}
											else {
												status += '<br>R: T: ' + item.CounterDelivToday;
												if (bHaveReturnUsage) {
													status += ", A: ";
													if (parseInt(item.UsageDeliv) > 0) {
														status += "-";
													}
													status += item.UsageDeliv;
												}
											}
										}
									}
									xhtm +=
										'\t    <tr id="utility_' + item.idx + '">\n' +
										'\t      <td id="name" class="name">' + vname + '</td>\n' +
										'\t      <td id="status" class="status"><span>' + status + '</span></td>\n' +
										'\t    </tr>\n';
								}
								// end of mobile utilities
								else {
									if ($scope.config.DashboardType == 0) {
										xhtm = '\t<div class="span4 movable" id="utility_' + item.idx + '">\n';
									}
									else if ($scope.config.DashboardType == 1) {
										xhtm = '\t<div class="span3 movable" id="utility_' + item.idx + '">\n';
									}
									/* generate item classes.  */

									/* type of device */
									var itemtypeclass = "";
									var itemsubtypeclass = "";
									if (typeof item.Type != 'undefined') {
										var itemtypeclass = ' ' + item.Type.slice(0);
										itemtypeclass = itemtypeclass.replace(/\s/g, '');
										itemtypeclass = itemtypeclass.replace(/\\/g, '');
										itemtypeclass = itemtypeclass.replace(/\//g, '');
									}
									if (typeof item.SubType != 'undefined') {
										var itemsubtypeclass = item.SubType.split(' ').join('');
										itemsubtypeclass = itemsubtypeclass.replace(/\\/g, '');
										itemsubtypeclass = itemsubtypeclass.replace(/\//g, '');
									}
									/* generate bigtext html */
									var bigtexthtml = "";
									bigtexthtml += '<span class="value1">';
									if ((typeof item.Usage != 'undefined') && (typeof item.UsageDeliv == 'undefined')) {
										bigtexthtml += item.Usage;
									}
									else if ((typeof item.Usage != 'undefined') && (typeof item.UsageDeliv != 'undefined')) {
										if (parseInt(item.Usage) > 0) {
											bigtexthtml += item.Usage;
										}
										else if (parseInt(item.UsageDeliv) > 0) {
											bigtexthtml += "-" + item.UsageDeliv;
										}
										else {
											bigtexthtml += item.Usage;
										}
									}
									else if ((item.SubType == "Gas") || (item.SubType == "RFXMeter counter")) {
										bigtexthtml += item.CounterToday;
									}
									else if (
										(item.Type == "Air Quality") ||
										(item.Type == "Lux") ||
										(item.Type == "Weight") ||
										(item.Type == "Usage") ||
										(item.SubType == "Percentage") ||
										(item.Type == "Fan") ||
										(item.SubType == "Soil Moisture") ||
										(item.SubType == "Leaf Wetness") ||
										(item.SubType == "Voltage") ||
										(item.SubType == "Distance") ||
										(item.SubType == "Current") ||
										(item.SubType == "Pressure") ||
										(item.SubType == "A/D") ||
										(item.SubType == "Sound Level") ||
										(item.SubType == "Waterflow") ||
										(item.Type == "Current") ||
										(item.SubType == "Alert") ||
										(item.SubType == "Gas") ||
										(item.SubType == "RFXMeter counter") ||
										(item.SubType == "Counter Incremental") ||
										(item.SubType == "Custom Sensor")
									) {
										bigtexthtml += item.Data;
									}
									else if ((item.Type == "Thermostat") && (item.SubType == "SetPoint")) {
										bigtexthtml += item.Data + '\u00B0 ' + $scope.config.TempSign;
									}
									else if (item.SubType == "Smartwares") {
										bigtexthtml += item.Data + '\u00B0 ' + $scope.config.TempSign;
									}
									else if (item.SubType == "Text") {
										if (item.Data.length > 12) {
											bigtexthtml += '' + item.Data.substr(0, 11) + '..';
										} else {
											bigtexthtml += item.Data;
										}
									}
									bigtexthtml += '</span>';

									/*  generate image and status html  */
									var statushtml = "";
									var imagehtml = '<img src="images/';

									if (typeof item.Counter != 'undefined') {
										if ((item.Type == "RFXMeter") || (item.Type == "YouLess Meter")) {
											if (item.SwitchTypeVal == 1) {
												imagehtml += 'Gas48.png" class="lcursor" onclick="ShowCounterLog(\'#dashcontent\',\'ShowFavorites\',' + item.idx + ',\'' + escape(item.Name) + '\', ' + item.SwitchTypeVal + ');" height="40" width="40"></td>\n';
											}
											else if (item.SwitchTypeVal == 2) {
												imagehtml += 'Water48_On.png" class="lcursor" onclick="ShowCounterLog(\'#dashcontent\',\'ShowFavorites\',' + item.idx + ',\'' + escape(item.Name) + '\', ' + item.SwitchTypeVal + ');" height="40" width="40"></td>\n';
											}
											else if (item.SwitchTypeVal == 3) {
												imagehtml += 'Counter48.png" class="lcursor" onclick="ShowCounterLog(\'#dashcontent\',\'ShowFavorites\',' + item.idx + ',\'' + escape(item.Name) + '\', ' + item.SwitchTypeVal + ');" height="40" width="40"></td>\n';
											}
											else if (item.SwitchTypeVal == 4) {
												imagehtml += 'PV48.png" class="lcursor" onclick="ShowCounterLogSpline(\'#dashcontent\',\'ShowFavorites\',' + item.idx + ',\'' + escape(item.Name) + '\', ' + item.SwitchTypeVal + ');" height="40" width="40"></td>\n';
											}
											else {
												imagehtml += 'Counter48.png" class="lcursor" onclick="ShowCounterLogSpline(\'#dashcontent\',\'ShowFavorites\',' + item.idx + ',\'' + escape(item.Name) + '\', ' + item.SwitchTypeVal + ');" height="40" width="40"></td>\n';
											}
										}
										else {
											if ((item.Type == "P1 Smart Meter") && (item.SubType == "Energy")) {
												imagehtml += 'Counter48.png" class="lcursor" onclick="ShowSmartLog(\'#dashcontent\',\'ShowFavorites\',' + item.idx + ',\'' + escape(item.Name) + '\', ' + item.SwitchTypeVal + ');" height="40" width="40"></td>\n';
											}
											else if ((item.Type == "P1 Smart Meter") && (item.SubType == "Gas")) {
												imagehtml += 'Gas48.png" class="lcursor" onclick="ShowCounterLog(\'#dashcontent\',\'ShowFavorites\',' + item.idx + ',\'' + escape(item.Name) + '\', ' + item.SwitchTypeVal + ');" height="40" width="40"></td>\n';
											}
											else {
												imagehtml += 'Counter48.png" class="lcursor" onclick="ShowCounterLog(\'#dashcontent\',\'ShowFavorites\',' + item.idx + ',\'' + escape(item.Name) + '\', ' + item.SwitchTypeVal + ');" height="40" width="40"></td>\n';
											}
										}
										if ((item.SubType != "Gas") && (item.SubType != "RFXMeter counter")) { // this is weird..
											statushtml = '' + $.t("Usage") + ': ' + item.CounterToday;
										}
										else if ((item.SubType == "Gas") || (item.SubType == "RFXMeter counter")) { // added this to fill the status value. If it's the same as the bigtext, then it won't be shown again.
											statushtml += item.CounterToday;
										} else {
											statushtml = "&nbsp;";
										}
									}
									else if ((item.Type == "Energy") || (item.Type == "Power") || (item.SubType == "kWh")) {
										if (((item.Type == "Energy") || (item.Type == "Power") || (item.SubType == "kWh")) && (item.SwitchTypeVal == 4)) {
											imagehtml += 'PV48.png" class="lcursor" onclick="ShowCounterLogSpline(\'#dashcontent\',\'ShowFavorites\',' + item.idx + ',\'' + escape(item.Name) + '\', ' + item.SwitchTypeVal + ');" height="40" width="40"></td>\n';
										}
										else {
											imagehtml += 'current48.png" class="lcursor" onclick="ShowCounterLogSpline(\'#dashcontent\',\'ShowFavorites\',' + item.idx + ',\'' + escape(item.Name) + '\', ' + item.SwitchTypeVal + ');" height="40" width="40"></td>\n';
										}
										statushtml = item.Data;
									}
									else if ((item.Type == "Current") || (item.Type == "Current/Energy")) {
										imagehtml += 'current48.png" class="lcursor" onclick="ShowCurrentLog(\'#dashcontent\',\'ShowFavorites\',' + item.idx + ',\'' + escape(item.Name) + '\', ' + item.displaytype + ');" height="40" width="40"></td>\n';
										statushtml = item.Data;
									}
									else if (item.Type == "Air Quality") {
										imagehtml += 'air48.png" class="lcursor" onclick="ShowAirQualityLog(\'#dashcontent\',\'ShowFavorites\',' + item.idx + ',\'' + escape(item.Name) + '\');" height="40" width="40"></td>\n';
										statushtml = item.Data + " (" + item.Quality + ")";
									}
									else if (item.SubType == "Percentage") {
										imagehtml += 'Percentage48.png" class="lcursor" onclick="ShowPercentageLog(\'#dashcontent\',\'ShowFavorites\',' + item.idx + ',\'' + escape(item.Name) + '\');" height="40" width="40"></td>\n';
										statushtml = item.Data;
									}
									else if (item.Type == "Fan") {
										imagehtml += 'Fan48_On.png" class="lcursor fanicon" onclick="ShowFanLog(\'#dashcontent\',\'ShowFavorites\',' + item.idx + ',\'' + escape(item.Name) + '\');" height="40" width="40"></td>\n';
										statushtml = item.Data;
									}
									else if (item.Type == "Lux") {
										imagehtml += 'lux48.png" class="lcursor" onclick="ShowLuxLog(\'#dashcontent\',\'ShowFavorites\',' + item.idx + ',\'' + escape(item.Name) + '\', ' + item.SwitchTypeVal + ');" height="40" width="40"></td>\n';
										statushtml = item.Data;
									}
									else if (item.Type == "Weight") {
										imagehtml += 'scale48.png" class="lcursor" onclick="ShowGeneralGraph(\'#dashcontent\',\'ShowFavorites\',' + item.idx + ',\'' + escape(item.Name) + '\',\'' + item.Type + '\', \'' + item.SubType + '\');" height="40" width="40"></td>\n';
										statushtml = item.Data;
									}
									else if (item.Type == "Usage") {
										imagehtml += 'current48.png" class="lcursor" onclick="ShowUsageLog(\'#dashcontent\',\'ShowFavorites\',' + item.idx + ',\'' + escape(item.Name) + '\', ' + item.SwitchTypeVal + ');" height="40" width="40"></td>\n';
										statushtml = item.Data;
									}
									else if (item.SubType == "Soil Moisture") {
										imagehtml += 'moisture48.png" class="lcursor" onclick="ShowGeneralGraph(\'#dashcontent\',\'ShowFavorites\',' + item.idx + ',\'' + escape(item.Name) + '\',' + item.SwitchTypeVal + ', \'' + item.SubType + '\');" height="40" width="40"></td>\n';
										statushtml = item.Data;
									}
									else if (item.SubType == "Custom Sensor") {
										imagehtml += item.Image + '48_On.png" class="lcursor" onclick="ShowGeneralGraph(\'#dashcontent\',\'ShowFavorites\',' + item.idx + ',\'' + escape(item.Name) + '\',\'' + escape(item.SensorUnit) + '\', \'' + item.SubType + '\');" height="40" width="40"></td>\n';
										statushtml = item.Data;
									}
									else if (item.SubType == "Waterflow") {
										imagehtml += 'moisture48.png" class="lcursor" onclick="ShowGeneralGraph(\'#dashcontent\',\'ShowFavorites\',' + item.idx + ',\'' + escape(item.Name) + '\',' + item.SwitchTypeVal + ', \'' + item.SubType + '\');" height="40" width="40"></td>\n';
										statushtml = item.Data;
									}
									else if (item.SubType == "Leaf Wetness") {
										imagehtml += 'leaf48.png" height="40" width="40"></td>\n';
										statushtml = item.Data;
									}
									else if (item.SubType == "Distance") {
										imagehtml += 'visibility48.png" class="lcursor" onclick="ShowGeneralGraph(\'#dashcontent\',\'ShowFavorites\',' + item.idx + ',\'' + escape(item.Name) + '\',' + item.SwitchTypeVal + ', \'DistanceGeneral\');" height="40" width="40"></td>\n';
										statushtml = item.Data;
									}
									else if ((item.SubType == "Voltage") || (item.SubType == "Current") || (item.SubType == "A/D")) {
										imagehtml += 'current48.png" class="lcursor" onclick="ShowGeneralGraph(\'#dashcontent\',\'ShowFavorites\',' + item.idx + ',\'' + escape(item.Name) + '\',' + item.SwitchTypeVal + ', \'VoltageGeneral\');" height="40" width="40"></td>\n';
										statushtml = item.Data;
									}
									else if (item.SubType == "Text") {
										imagehtml += 'text48.png" class="lcursor" onclick="ShowTextLog(\'#dashcontent\',\'ShowFavorites\',' + item.idx + ',\'' + escape(item.Name) + '\');" height="40" width="40"></td>\n';
										statushtml = item.Data;
									}
									else if (item.SubType == "Alert") {
										imagehtml += 'Alert48_' + item.Level + '.png" class="lcursor" onclick="ShowTextLog(\'#dashcontent\',\'ShowFavorites\',' + item.idx + ',\'' + escape(item.Name) + '\');" height="40" width="40"></td>\n';
										statushtml = item.Data;
									}
									else if (item.SubType == "Pressure") {
										imagehtml += 'gauge48.png" class="lcursor" onclick="ShowGeneralGraph(\'#dashcontent\',\'ShowFavorites\',' + item.idx + ',\'' + escape(item.Name) + '\',' + item.SwitchTypeVal + ', \'' + item.SubType + '\');" height="40" width="40"></td>\n';
										statushtml = item.Data;
									}
									else if ((item.Type == "Thermostat") && (item.SubType == "SetPoint")) {
										imagehtml += 'override.png" class="lcursor" onclick="ShowSetpointPopup(event, ' + item.idx + ', ShowFavorites, ' + item.Protected + ', ' + item.Data + ');" height="40" width="40"></td>\n';
										statushtml = item.Data + '\u00B0 ' + $scope.config.TempSign;
									}
									else if (item.SubType == "Smartwares") {
										imagehtml += 'override.png" class="lcursor" onclick="ShowSetpointPopup(event, ' + item.idx + ', ShowFavorites, ' + item.Protected + ', ' + item.Data + ');" height="40" width="40"></td>\n';
										statushtml = item.Data + '\u00B0 ' + $scope.config.TempSign;
									}
									else if ((item.SubType == "Thermostat Mode") || (item.SubType == "Thermostat Fan Mode")) {
										imagehtml += 'mode48.png" height="40" width="40"></td>\n';
										statushtml = item.Data;
									}
									else if (item.SubType == "Sound Level") {
										imagehtml += 'Speaker48_On.png" class="lcursor" onclick="ShowGeneralGraph(\'#dashcontent\',\'ShowFavorites\',' + item.idx + ',\'' + escape(item.Name) + '\',' + item.SwitchTypeVal + ', \'' + item.SubType + '\');" height="40" width="40"></td>\n';
										statushtml = item.Data;
									}

									if (typeof item.Usage != 'undefined') {
										if (item.Type != "P1 Smart Meter") {
											if ($scope.config.DashboardType == 0) {
												//status+='<br>' + $.t("Actual") + ': ' + item.Usage;
												if (typeof item.CounterToday != 'undefined') {
													statushtml += '</span><span class="value2">' + $.t("Today") + ': ' + item.CounterToday;
												}
											}
											else {
												//status+=", A: " + item.Usage;
												if (typeof item.CounterToday != 'undefined') {
													statushtml += ', T: ' + item.CounterToday;
												}
											}
										}
									}
									if (typeof item.CounterDeliv != 'undefined') {
										if (item.CounterDeliv != 0) {
											statushtml += '</span><span class="value2">';
											statushtml += '<br>' + $.t("Return") + ': ' + item.CounterDelivToday;
										}
									}
									statushtml = '<span class="value1">' + statushtml + '</span>';

									// generate protected/timeout/lowbattery status
									var backgroundClass = $rootScope.GetItemBackgroundStatus(item);
									/* checking the generated html for even more classes, then fill in the HTML */
									var count = (statushtml.match(/<span/g) || []).length;//$(statushtml).find("span").length;
									// if ($(escape(statushtml)).text().length != $(escape(bigtexthtml)).text().length){
									if (statushtml.length != bigtexthtml.length) {
										xhtm += '\t  <div id="bstatus" class="item ' + itemtypeclass + ' ' + itemsubtypeclass + ' ' + backgroundClass + ' withstatus statuscount' + count + '">\n';
									} else {
										xhtm += '\t  <div id="bstatus" class="item ' + itemtypeclass + ' ' + itemsubtypeclass + ' ' + backgroundClass + ' withoutstatus statuscount' + count + '">\n';
									}

									xhtm += '\t    <table id="itemtablesmall" class="itemtablesmall" border="0" cellpadding="0" cellspacing="0">\n';
									xhtm += '\t    <tr">\n';
									xhtm += '\t      <td id="name" class="name">' + item.Name + '</td>\n';
									xhtm += '\t      <td id="bigtext" class="bigtext"><span class="wrapper">' + bigtexthtml + '</span></td>\n';
									xhtm += '\t      <td id="img" class="img img1">' + imagehtml + '</td>';
									xhtm += '\t      <td id="status" class="status"><span class="wrapper">' + statushtml + '</span></td>\n' +
										'\t      <td id="lastupdate" class="lastupdate"><span>' + item.LastUpdate + '</span></td>\n' +
										'\t    </tr>\n' +
										'\t    </table>\n' +
										'\t  </div>\n' +
										'\t</div>\n';
								}

								htmlcontent += xhtm;
								jj += 1;
							}
						}); //Utility devices
						if (bHaveAddedDivider == true) {
							//close previous divider
							htmlcontent += '</div>\n';
						}
						if (($scope.config.DashboardType == 2) || (window.myglobals.ismobile == true)) {
							htmlcontent += '\t    </table>\n';
						}
						if (jj > 0) {
							htmlcontent += '</section>';
						}
					}
				}
			});

			if (htmlcontent == "") {
				htmlcontent = '<h2>' +
					$.t('No favorite devices defined ... (Or communication Lost!)') +
					'</h2><p>\n' +
					$.t('If this is your first time here, please setup your') + ' <a href="javascript:SwitchLayout(\'Hardware\')" data-i18n="Hardware">Hardware</a>, ' +
					$.t('and add some') + ' <a href="javascript:SwitchLayout(\'Devices\')" data-i18n="Devices">Devices</a></p><p>.' +
					$.t('Visit the Getting Started page at the') + ' <a href="https://www.domoticz.com/wiki/Getting_started">Domoticz Wiki</a>.';

			}
			else {
				htmlcontent += "<br>";
			}

			var suntext = "";
			if (bShowRoomplan == false) {
				suntext =
					'<div class="beforebannav">' +
					'\t<table border="0" cellpadding="0" cellspacing="0" width="100%">\n' +
					'\t<tr>\n' +
					'\t  <td align="left" valign="top" id="timesun"></td>\n' +
					'\t</tr>\n' +
					'\t</table>\n' +
					'\t</div>\n';
			}
			else {
				suntext =
					'<div class="beforebannav">' +
					'<table "border="0" cellpadding="0" cellspacing="0" width="100%">' +
					'<tr>' +
					'<td align="left" valign="top" id="timesun"></td>' +
					'<td align="right">' +
					'<span data-i18n="Room">Room</span>:&nbsp;<select id="comboroom" class="combobox ui-corner-all">' +
					'<option value="0" data-i18n="All">All</option>' +
					'</select>' +
					'</td>' +
					'</tr>' +
					'</table>' +
					'</div>';
			}


			$('#dashcontent').html(suntext + htmlcontent + EvohomeAddJS());
			$('#dashcontent').i18n();

			if (bShowRoomplan == true) {
				$.each($.RoomPlans, function (i, item) {
					var option = $('<option />');
					option.attr('value', item.idx).text(item.name);
					$("#dashcontent #comboroom").append(option);
				});
				if (typeof window.myglobals.LastPlanSelected != 'undefined') {
					$("#dashcontent #comboroom").val(window.myglobals.LastPlanSelected);
				}
				$("#dashcontent #comboroom").change(function () {
					var idx = $("#dashcontent #comboroom option:selected").val();
					window.myglobals.LastPlanSelected = idx;
					ShowFavorites();
				});
			}


			// Store variables
			var accordion_head = $('#dashcontent .accordion > li > a'),
				accordion_body = $('#dashcontent .accordion li > .sub-menu');

			// Open the first tab on load
			accordion_head.first().addClass('active').next().slideDown('normal');

			// Click function
			accordion_head.on('click', function (event) {
				// Disable header links
				event.preventDefault();

				// Show and hide the tabs on click
				if ($(this).attr('class') != 'active') {
					accordion_body.slideUp('normal');
					$(this).next().stop(true, true).slideToggle('normal');
					accordion_head.removeClass('active');
					$(this).addClass('active');
				}
			});

			$rootScope.RefreshTimeAndSun();

			//Create Dimmer Sliders
			$('#dashcontent .dimslider').slider({
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
					id = "#dashcontent #light_" + idx;
					var obj = $(id);
					if (typeof obj != 'undefined') {
						var img = "";
						var status = "";
						var TxtOn = "On";
						var TxtOff = "Off";
						if (dtype == "blinds") {
							TxtOn = "Open";
							TxtOff = "Close";
						}
						if (($scope.config.DashboardType == 2) || (window.myglobals.ismobile == true)) {
							if (fPercentage == 0) {
								status = '<button class="btn btn-mini" type="button">' + $.t(TxtOn) + '</button> ' +
									'<button class="btn btn-mini btn-info" type="button">' + $.t(TxtOff) + '</button>';
							}
							else {
								status = '<button class="btn btn-mini btn-info" type="button">' + $.t(TxtOn) + ': ' + fPercentage + "% </button> " +
									'<button class="btn btn-mini" type="button">' + $.t(TxtOff) + '</button>';
							}
							if ($(id + " #status > span").html() != status) {
								$(id + " #status > span").html(status);
							}
						}
						else {
							var imgname = $('#light_' + idx + ' .lcursor').prop('src');
							imgname = imgname.substring(imgname.lastIndexOf("/") + 1, imgname.lastIndexOf("_O") + 2);
							if (dtype == "relay")
								imgname = "Fireplace48_O"
							if (fPercentage == 0) {
								img = '<img src="images/' + imgname + 'ff.png" title="' + $.t("Turn On") + '" onclick="SwitchLight(' + idx + ',\'On\',RefreshFavorites,' + isProtected + ');" class="lcursor" height="40" width="40">';
								status = "Off";
							}
							else {
								img = '<img src="images/' + imgname + 'n.png" title="' + $.t("Turn Off") + '" onclick="SwitchLight(' + idx + ',\'Off\',RefreshFavorites,' + isProtected + ');" class="lcursor" height="40" width="40">';
								status = fPercentage + " %";
							}
							if (dtype != "blinds") {
								if ($(id + " #img").html() != img) {
									$(id + " #img").html(img);
								}
							}
							if ($(id + " #bigtext > span").html() != status) {
								$(id + " #bigtext > span").html(status);
							}
							if ($scope.config.ShowUpdatedEffect == true) {
								$(id + " #name").effect("highlight", { color: '#EEFFEE' }, 1000);
							}
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
			$('#dashcontent .selectorlevels div').buttonset({
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
						SwitchSelectorLevel(idx, unescape(levelname), level, RefreshFavorites, isprotected);
						// Synchronize buttons and div attributes
						div$.data('level', level);
						div$.data('levelname', levelname);
					});

					if (($scope.config.DashboardType === 2) || (window.myglobals.ismobile === true)) {
						$('#dashcontent #light_' + idx + " #status").html('');
					} else {
						$('#dashcontent #light_' + idx + " #bigtext > span").html(unescape(levelname));
					}
				}
			});

			//Create Selector selectmenu
			$('#dashcontent .selectorlevels select').selectmenu({
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

					if (($scope.config.DashboardType === 2) || (window.myglobals.ismobile === true)) {
						$('#dashcontent #light_' + idx + " #status").html('');
					} else {
						$('#dashcontent #light_' + idx + " #bigtext > span").html(unescape(levelname));
					}
				},
				change: function (event, ui) { //When the user selects an option
					var select$ = $(this),
						idx = select$.selectmenu("option", "idx"),
						level = select$.selectmenu().val(),
						levelname = select$.find('option[value="' + level + '"]').text(),
						isprotected = select$.selectmenu("option", "isprotected");
					// Send command
					SwitchSelectorLevel(idx, unescape(levelname), level, RefreshFavorites, isprotected);
					// Synchronize buttons and select attributes
					select$.data('level', level);
					select$.data('levelname', levelname);
				}
			}).selectmenu('refresh');

			if ($scope.config.AllowWidgetOrdering == true) {
				if (permissions.hasPermission("Admin")) {
					if (window.myglobals.ismobileint == false) {
						$("#dashcontent .movable").draggable({
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
						$("#dashcontent .movable").droppable({
							drop: function () {
								var myid = $(this).attr("id");
								var parts1 = myid.split('_');
								var parts2 = $.devIdx.split('_');
								if (parts1[0] != parts2[0]) {
									bootbox.alert($.t('Only possible between Sensors of the same kind!'));
									$scope.mytimer = $interval(function () {
										ShowFavorites();
									}, 10000);
								} else {
									var roomid = 0;
									if (typeof window.myglobals.LastPlanSelected != 'undefined') {
										roomid = window.myglobals.LastPlanSelected;
									}
									$.ajax({
										url: "json.htm?type=command&param=switchdeviceorder&idx1=" + parts1[1] + "&idx2=" + parts2[1] + "&roomid=" + roomid,
										async: false,
										dataType: 'json',
										success: function (data) {
											ShowFavorites();
										}
									});
								}
							}
						});
					}
				}
			}
			$scope.mytimer = $interval(function () {
				RefreshFavorites();
			}, 10000);
		}

		$scope.ResizeDimSliders = function () {
			var nobj = $("#dashcontent #name");
			if (typeof nobj == 'undefined') {
				return;
			}
			var width = $("#dashcontent #name").width() - 40;
			$("#dashcontent .span4 .dimslidernorm").width(width);
			//width=$(".span3").width()-70;
			$("#dashcontent .span3 .dimslidernorm").width(width);
			width = $(".mobileitem").width() - 63;
			$("#dashcontent .mobileitem .dimslidernorm").width(width);

			width = $("#dashcontent #name").width() - 40;
			//width=$(".span4").width()-118;
			$("#dashcontent .span4 .dimslidersmall").width(width);
			//width=$(".span3").width()-112;
			$("#dashcontent .span3 .dimslidersmall").width(width);
			width = $(".mobileitem").width() - 63;
			$("#dashcontent .mobileitem .dimslidersmall").width(width);

			width = $("#dashcontent #name").width() - 85;
			$("#dashcontent .span4 .dimslidersmalldouble").width(width);
			$("#dashcontent .span3 .dimslidersmalldouble").width(width);
		}

		init();

		function init() {
			$(window).resize(function () { $scope.ResizeDimSliders(); });
			$scope.LastUpdateTime = parseInt(0);
			$scope.MakeGlobalConfig();
			MobilePhoneDetection();
			ShowFavorites();
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
			popup = $("#setpoint_popup");
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
