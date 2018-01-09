/*global ion, $, md5 */

var SecurityPanel = function () {
    
	var secPanel = {
		heartbeatIntervalSecs: 10,
		secOnDelay: 30,
		heartbeatTimer: undefined,
		code: ""/*,
		zones: []*/
	};

	function Show(text) {
		$('#digitdisplay').val(("* " + $.i18n.t(text) + " *").toUpperCase());
	}
	function ShowError(error) {
		if (error=="NoConnect") {
			Show("Disconnected");
		}
		else if (error=="NoOkData") {
			Show($.i18n.t("Disconnected"));
		}
		else if (error=="NoSettingsData") {
			Show($.i18n.t("Disconnected"));
		}
		else if (error=="NoOkCode") {
			Show($.i18n.t("WRONG CODE"));
		}
		else {
			Show($.i18n.t("Disconnected"));
		}
	}

	function ResetHeartbeat(nonstandardDelayMs, keepCode) {

		var delay = secPanel.heartbeatIntervalSecs * 1000;
		
		// Make sure any previous calls are ignored
		clearTimeout(secPanel.heartbeatTimer);

		// Use nonstandardDelayMs if defined
		nonstandardDelayMs = parseInt(nonstandardDelayMs, 10);
		if (!isNaN(nonstandardDelayMs)) {
			delay = nonstandardDelayMs;
		}
		
		// Clear code, unless instructed not to
		if (!keepCode) {
			secPanel.code = "";
		}
		
		// New request
		secPanel.heartbeatTimer = setTimeout(function () {
			Heartbeat();
		}, delay);
		
	}

	function RefreshStatus(callback) {

		// Upate security status
		$.ajax({
			url: "../json.htm?type=command&param=getsecstatus",
			dataType: 'json',
			success: function(data) {
				if (data.status != "OK") {
					ShowError('NoOkData');
				}
				else {
					
					// Update status
					var displaytext="";
					if (data.secstatus==0) displaytext="Disarmed";
					else if (data.secstatus==1) displaytext="Armed Home";
					else if (data.secstatus==2) displaytext="Armed";
					else displaytext="Unknown Error";
					
					Show(displaytext);
					
					// Update SecOnDelay
					var parsedSecOnDelay = parseInt(data.secondelay, 10);
					if (!isNaN(parsedSecOnDelay)) {
						secPanel.secOnDelay = parsedSecOnDelay;
					}
					
				}
				
				callback();
			},
			error: function(data){
				if (data.status==401) { // 401 : Unauthorized
					window.location = "../index.html";
				}
				
				callback();
			}
		});
	}

	function Heartbeat() {

		RefreshStatus(function () {

			// RefreshZones();

			// Iterate!
			ResetHeartbeat();

		});

	}
	
	function Countdown(delay) {

		clearTimeout(secPanel.countdownTimer);

		// Make the heartbeat wait a while
		ResetHeartbeat();

		if (delay > 1) {

			delay = delay - 1;
			Beep('set');
			$('#digitdisplay').val($.i18n.t('Delay') + ': ' + delay);

			secPanel.countdownTimer = setTimeout(function () { Countdown (delay); }, 1000);

		} else {

			Beep('in');
			ResetHeartbeat(0);

		}
	}
	
	function Beep(tone) {
		if (tone=="error") {
			ion.sound.play("wrongcode");
		}
		else if (tone=="set") {
			ion.sound.play("key");
		}
		else if (tone=="in") {
			ion.sound.play("arm");
		}
		else if (tone=="out") {
			ion.sound.play("disarm");
		}
		else {
			ion.sound.play("key");
		}
	}
	
	function SetSecStatus (status) {

		// Bail out if stuff doesn't seem right
		if (isNaN(status)) {
			Beep('error');
			return;
		}
		
		// Temporary storage
		var seccodeHash = md5(secPanel.code);

		// Make the heartbeat wait a while (secPanel.code is reset here)
		ResetHeartbeat();

		$.ajax({
			url: "../json.htm?type=command&param=setsecstatus&secstatus=" + status + "&seccode=" + seccodeHash,
			dataType: 'json',
			success: function(data) {
				if (data.status != "OK") {
					if (data.message=="WRONG CODE") {
						ShowError('NoOkCode');
						Beep('error');
						ResetHeartbeat(3000);
					}
					else {
						ShowError('NoOkData');
						ResetHeartbeat(3000);
					}
				}
				else {
					if (status==1 || status==2) {
						Countdown(secPanel.secOnDelay);
					}
					else {
						ResetHeartbeat(0);
						Beep('out');
					}
				}
			},
			error: function(){
				ShowError('NoConnect');
				return;
			}
		});
		seccodeHash = undefined;
	}
	
	function AddDigit(digit) {

		Beep(); 
		
		// Make the heartbeat wait a whild
		// Note: Heartbeat always resets the cleartext code placeholder (secStatus.code)
		ResetHeartbeat(undefined, true);

		// Only proceed if new digit is a digit, inside range 0-9
		var newDigit = parseInt(digit, 10);
		if ((newDigit >= 0 && newDigit <= 9) || digit == "*" || digit == "#") {

            // Update code
			secPanel.code = "" + secPanel.code + "" + digit;
			
			// Update display
    		$('#digitdisplay').val(secPanel.code.replace(/./g,"#"));

		}
		
	}
	
 	return {
 		AddDigit: AddDigit,
		SetSecStatus: SetSecStatus,
		Beep: Beep,
		ResetHeartbeat: ResetHeartbeat,
		Start: Heartbeat
 	};
		 	
};

// Deprecated code, left for future use

	/*function RefreshZones(callback) {

		$.ajax({
			url: "../json.htm?type=command&param=getlightswitches",
			dataType: 'json',
			success: function(data) {
				if (data.status != "OK") {
					ShowError('NoOkData');
					return;
				}
				else {
					var idxnumbers=[];
					if (typeof(data.result) != "undefined") {
						$.each(data.result, function(i,item) {
							if (item.Name.toLowerCase().indexOf('alarm zone')==0) idxnumbers[idxnumbers.length]=item.idx;
						});
					}
					secPanel.zones = idxnumbers;
					callback(idxnumbers);
				}
			},
			error: function(){
				ShowError('NoConnect');
				callback();
			}
		});
	}*/
	// check if provided zone idx number(s) is/are clear
	/*function CheckZoneClear(zonearray) {
		var openzone=false;
		if (typeof(zonearray) != "undefined") {
			$.each(zonearray, function(i,item) {
				$.ajax({
					url: "../json.htm?type=devices&rid="+item,
					async: false, 
					dataType: 'json',
					success: function(data) {
						if (data.status != "OK") {
							ShowError('NoOkData');
							return;
						}
						else {
							if (typeof(data.result) != "undefined") {
								$.each(data.result, function(j,zone) {
									if (zone.Status=='On') openzone=true;
								});
							}
							return openzone;
						}
					},
					error: function(){
						ShowError('NoConnect');
						return;
					}
				});
			});
		}
		return openzone;
	}*/
	
	
	// SetSecStatus
	
		/*if (typeof(zonearray) != "undefined") {
			if (CheckZoneClear(zonearray)) {
				Beep('error');
				$('#digitdisplay').val(('* ZONE OPEN *'));
				CodeSetTimer=DelayedStatusRefresh(3000);
				return;
			}
		}*/
