/*global ion, $, md5 */

var SecurityPanel = function () {
    
	var secPanel = {
		heartbeatIntervalSecs: 10,
		heartbeatTimer: undefined,
		code: "",
		zones: []
	};

	function ShowError(error) {
		if (error=="NoConnect") {
			$('#digitdisplay').val("no connect");
		}
		else if (error=="NoOkData") {
			$('#digitdisplay').val("no ok data");
		}
		else if (error=="NoSettingsData") {
			$('#digitdisplay').val("no settings");
		}
		else if (error=="NoOkCode") {
			$('#digitdisplay').val("WRONG CODE");
		}
		else {
			$('#digitdisplay').val("unkown err");
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

	function RefreshVariables(callback) {

		// Update user variables
		$.ajax({
			url: "../json.htm?type=command&param=getuservariables",
			dataType: 'json',
			success: function(data) {
				if (data.status != "OK") {
					ShowError('NoOkData');
				}
				else {
					if (typeof(data.result) != "undefined") {
						$.each(data.result, function(i,item) {
							if (item.Name.toLowerCase()=='secpanel-autorefresh') {
							
								// Parse the interval (integer, seconds)
								var parsedInterval = parseInt(item.Value, 10);

								// Verify that value is a number, else default to heartbeatInterval
								if (!isNaN(parsedInterval)) {
									secPanel.heartbeatIntervalSecs = parsedInterval;
								}

							}
						});
					}
				}
				callback();
			},
			error: function(){
				ShowError('NoConnect');
				callback();
			}
		});

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
					var displaytext="";
					if (data.secstatus==0) displaytext="DISARMED";
					else if (data.secstatus==1) displaytext="ARM HOME";
					else if (data.secstatus==2) displaytext="ARM AWAY";
					else displaytext="UNKNOWN";
					$('#digitdisplay').val("* "+displaytext+" *");
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

	function Heartbeat() {

		RefreshVariables(function () {
			RefreshStatus(function () {

				// RefreshZones();

				// Iterate!
				ResetHeartbeat();

			});
		});

	}
	
	function Countdown(delay) {

		clearTimeout(secPanel.countdownTimer);

		// Make the heartbeat wait a while
		ResetHeartbeat();

		if (delay > 1) {

			delay = delay - 1;
			Beep('set');
			$('#digitdisplay').val('Arm Delay: ' + delay);

			secPanel.countdownTimer = setTimeout(function () { Countdown (delay); }, 1000);

		} else {

			Beep('in');
			ResetHeartbeat(0);

		}
	}

	function ArmDelay(callback) {

		$.ajax({
			url: "../json.htm?type=settings",
			dataType: 'json',
			success: function(data) {
				if (data.status != "OK") {
					ShowError('NoOkData');
				}
				else {
					if (typeof(data.SecOnDelay) != "undefined") {
						callback(data.SecOnDelay);
					}
				}
			},
			error: function(){
				ShowError('NoConnect');
			}
		});

	}

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

		// Bail out if code isn't numeric
		if (isNaN(secPanel.code) || isNaN(status)) {
			Beep('error');
			return;
		}
		
		// Temporary storage
		var seccodeHash = md5(secPanel.code);

		// Make the heartbeat wait a while (secPanel.code is reset here)
		ResetHeartbeat();

		/*if (typeof(zonearray) != "undefined") {
			if (CheckZoneClear(zonearray)) {
				Beep('error');
				$('#digitdisplay').val($.t('* ZONE OPEN *'));
				CodeSetTimer=DelayedStatusRefresh(3000);
				return;
			}
		}*/

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
						ArmDelay(function(delay) {
							Countdown(delay);
						});
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

		// Reset code if not a number
		if (isNaN(parseInt(secPanel.code, 10))) {
			secPanel.code = "";
		}

		// Only proceed if new digit is a digit, inside range 0-9
		var newDigit = parseInt(digit, 10);
		if (!isNaN(newDigit) && newDigit >= 0 && newDigit <= 9) {

            // Update code
			secPanel.code = "" + secPanel.code + "" + newDigit;
			
			// Update display
    		$('#digitdisplay').val(secPanel.code.replace(/./g,"#"));

		}
		
	}
	
	function Start() {
 		Heartbeat();
	}
 	
 	return {
 		AddDigit: AddDigit,
		SetSecStatus: SetSecStatus,
		Beep: Beep,
		ResetHeartbeat: ResetHeartbeat,
		Start: Start
 	};
		 	
};