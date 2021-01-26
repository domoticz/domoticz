define(['app'], function (app) {
	app.controller('HardwareController', function ($scope, $rootScope, $timeout) {

		$scope.SerialPortStr = [];

		DeleteHardware = function (idx) {
			bootbox.confirm($.t("Are you sure to delete this Hardware?\n\nThis action can not be undone...\nAll Devices attached will be removed!"), function (result) {
				if (result == true) {
					$.ajax({
						url: "json.htm?type=command&param=deletehardware&idx=" + idx,
						async: false,
						dataType: 'json',
						success: function (data) {
							RefreshHardwareTable();
						},
						error: function () {
							HideNotify();
							ShowNotify($.t('Problem deleting hardware!'), 2500, true);
						}
					});
				}
			});
		}

		function hideAndRefreshHardwareTable() {
			HideNotify();
			RefreshHardwareTable();
		}
		
		UpdateHardware = function (idx, Mode1, Mode2, Mode3, Mode4, Mode5, Mode6) {
			var name = $("#hardwarecontent #hardwareparamstable #hardwarename").val();
			if (name == "") {
				ShowNotify($.t('Please enter a Name!'), 2500, true);
				return;
			}
			var hardwaretype = $("#hardwarecontent #hardwareparamstable #combotype option:selected").val();
			if (typeof hardwaretype == 'undefined') {
				ShowNotify($.t('Unknown device selected!'), 2500, true);
				return;
			}

			var bEnabled = $('#hardwarecontent #hardwareparamstable #enabled').is(":checked");
			var datatimeout = $('#hardwarecontent #hardwareparamstable #combodatatimeout').val();
			
			var logLevel = 0;
			if ($("#hardwarecontent #hardwareparamstable #loglevelInfo").prop("checked"))
				logLevel |= 1;
			if ($("#hardwarecontent #hardwareparamstable #loglevelStatus").prop("checked"))
				logLevel |= 2;
			if ($("#hardwarecontent #hardwareparamstable #loglevelError").prop("checked"))
				logLevel |= 4;

			var text = $("#hardwarecontent #hardwareparamstable #combotype option:selected").text();

			// Handle plugins 1st because all the text indexof logic below will have unpredictable impacts for plugins
			if (!$.isNumeric(hardwaretype)) {
				var selector = "#hardwarecontent #divpythonplugin #" + hardwaretype;
				var bIsOK = true;
				// Make sure that all required fields have values
				$(selector + " .text").each(function () {
					if ((typeof (this.attributes.required) != "undefined") && (this.value == "")) {
						$(selector + " #" + this.id).focus();
						ShowNotify($.t('Please enter value for required field'), 2500, true);
						bIsOK = false;
					}
					return bIsOK;
				});
				if (bIsOK) {
					$.ajax({
						url: "json.htm?type=command&param=updatehardware&htype=94" +
						"&loglevel=" + logLevel +
						"&idx=" + idx +
						"&name=" + encodeURIComponent(name) +
						"&username=" + encodeURIComponent(($(selector + " #Username").length == 0) ? "" : $(selector + " #Username").val()) +
						"&password=" + encodeURIComponent(($(selector + " #Password").length == 0) ? "" : $(selector + " #Password").val()) +
						"&address=" + encodeURIComponent(($(selector + " #Address").length == 0) ? "" : $(selector + " #Address").val()) +
						"&port=" + encodeURIComponent(($(selector + " #Port").length == 0) ? "" : $(selector + " #Port").val()) +
						"&serialport=" + encodeURIComponent(($(selector + " #SerialPort").length == 0) ? "" : $(selector + " #SerialPort").val()) +
						"&Mode1=" + encodeURIComponent(($(selector + " #Mode1").length == 0) ? "" : $(selector + " #Mode1").val()) +
						"&Mode2=" + encodeURIComponent(($(selector + " #Mode2").length == 0) ? "" : $(selector + " #Mode2").val()) +
						"&Mode3=" + encodeURIComponent(($(selector + " #Mode3").length == 0) ? "" : $(selector + " #Mode3").val()) +
						"&Mode4=" + encodeURIComponent(($(selector + " #Mode4").length == 0) ? "" : $(selector + " #Mode4").val()) +
						"&Mode5=" + encodeURIComponent(($(selector + " #Mode5").length == 0) ? "" : $(selector + " #Mode5").val()) +
						"&Mode6=" + encodeURIComponent(($(selector + " #Mode6").length == 0) ? "" : $(selector + " #Mode6").val()) +
						"&extra=" + encodeURIComponent(hardwaretype) +
						"&enabled=" + bEnabled +
						"&datatimeout=" + datatimeout,
						async: false,
						dataType: 'json',
						success: function (data) {
							RefreshHardwareTable();
						},
						error: function () {
							ShowNotify($.t('Problem adding hardware!'), 2500, true);
						}
					});
				}
				return;
			}
			if (text.indexOf("1-Wire") >= 0) {
				var extra = $("#hardwarecontent #div1wire #owfspath").val();
				var Mode1 = $("#hardwarecontent #div1wire #OneWireSensorPollPeriod").val();
				var Mode2 = $("#hardwarecontent #div1wire #OneWireSwitchPollPeriod").val();
				$.ajax({
					url: "json.htm?type=command&param=updatehardware&htype=" + hardwaretype +
					"&loglevel=" + logLevel +
					"&extra=" + encodeURIComponent(extra) +
					"&name=" + encodeURIComponent(name) +
					"&enabled=" + bEnabled +
					"&idx=" + idx +
					"&datatimeout=" + datatimeout +
					"&Mode1=" + Mode1 + "&Mode2=" + Mode2 + "&Mode3=" + Mode3 + "&Mode4=" + Mode4 + "&Mode5=" + Mode5 + "&Mode6=" + Mode6,
					async: false,
					dataType: 'json',
					success: function (data) {
						RefreshHardwareTable();
					},
					error: function () {
						ShowNotify($.t('Problem updating hardware!'), 2500, true);
					}
				});
			}
			else if (
				(text.indexOf("Panasonic") >= 0) ||
				(text.indexOf("BleBox") >= 0) ||
				(text.indexOf("TE923") >= 0) ||
				(text.indexOf("Volcraft") >= 0) ||
				(text.indexOf("GPIO") >= 0) ||
				(text.indexOf("Dummy") >= 0) ||
				(text.indexOf("System Alive") >= 0) ||
				(text.indexOf("PiFace") >= 0) ||
				(text.indexOf("I2C ") >= 0) ||
				(text.indexOf("Motherboard") >= 0) ||
				(text.indexOf("Kodi Media") >= 0) ||
				(text.indexOf("Evohome") >= 0 && text.indexOf("script") >= 0) ||
				(text.indexOf("YeeLight") >= 0) ||
				(text.indexOf("Arilux AL-LC0x") >= 0) ||
				(text.indexOf("sysfs GPIO") >= 0)
				)
			 {
				// if hardwaretype == 1000 => I2C sensors grouping
				if (hardwaretype == 1000) {
					hardwaretype = $("#hardwareparamsi2clocal #comboi2clocal").find('option:selected').val();
				}
				var text1 = $("#hardwareparamsi2clocal #comboi2clocal").find('option:selected').text();
				if (text1.indexOf("I2C sensor") >= 0) {
					var i2cpath = $("#hardwareparamsi2clocal #i2cpath").val();
					var i2caddress = "";
					var i2cinvert = "";
					Mode1 = "";
				}
				if (text1.indexOf("I2C sensor PIO 8bit expander PCF8574") >= 0) {
					i2caddress = $("#hardwareparami2caddress #i2caddress").val();
					i2cinvert = $("#hardwareparami2cinvert #i2cinvert").prop("checked") ? 1 : 0;
					Mode1 = encodeURIComponent(i2cinvert);
				}
				else if (text1.indexOf("I2C sensor GPIO 16bit expander MCP23017") >= 0) {
					i2caddress = $("#hardwareparami2caddress #i2caddress").val();
					i2cinvert = $("#hardwareparami2cinvert #i2cinvert").prop("checked") ? 1 : 0;
					Mode1 = encodeURIComponent(i2cinvert);
				}
				if ((text.indexOf("GPIO") >= 0) && (text.indexOf("sysfs GPIO") == -1)) {
					var gpiodebounce = $("#hardwareparamsgpio #gpiodebounce").val();
					var gpioperiod = $("#hardwareparamsgpio #gpioperiod").val();
					var gpiopollinterval = $("#hardwareparamsgpio #gpiopollinterval").val();
					if (gpiodebounce == "") {
						gpiodebounce = "50";
					}
					if (gpioperiod == "") {
						gpioperiod = "50";
					}
					if (gpiopollinterval == "") {
						gpiopollinterval = "0";
					}
					Mode1 = gpiodebounce;
					Mode2 = gpioperiod;
					Mode3 = gpiopollinterval;
				}
				if (text.indexOf("sysfs GPIO") >= 0) {
					Mode1 = $('#hardwarecontent #hardwareparamssysfsgpio #sysfsautoconfigure').prop("checked") ? 1 : 0;
					Mode2 = $('#hardwarecontent #hardwareparamssysfsgpio #sysfsdebounce').val();
				}
				$.ajax({
					url: "json.htm?type=command&param=updatehardware&htype=" + hardwaretype +
					"&loglevel=" + logLevel +
					"&address=" + encodeURIComponent(i2caddress) +
					"&name=" + encodeURIComponent(name) +
					"&enabled=" + bEnabled +
					"&idx=" + idx +
					"&datatimeout=" + datatimeout +
					"&Mode1=" + Mode1 + "&Mode2=" + Mode2 + "&Mode3=" + Mode3 + "&Mode4=" + Mode4 + "&Mode5=" + Mode5 + "&Mode6=" + Mode6 +
					"&port=" + encodeURIComponent(i2cpath),
					async: false,
					dataType: 'json',
					success: function (data) {
						RefreshHardwareTable();
					},
					error: function () {
						ShowNotify($.t('Problem updating hardware!'), 2500, true);
					}
				});
			}
			else if (text.indexOf("USB") >= 0 || text.indexOf("Teleinfo EDF") >= 0) {
				var Mode1 = "0";
				var password = "";
				var serialport = $("#hardwarecontent #divserial #comboserialport option:selected").text();
				if (typeof serialport == 'undefined') {
					if (bEnabled == true) {
						ShowNotify($.t('No serial port selected!'), 2500, true);
						return;
					}
					else {
						serialport = "";
					}
				}

				var extra = "";
				if (text.indexOf("Evohome") >= 0) {
					var baudrate = $("#hardwarecontent #divevohome #combobaudrateevohome option:selected").val();
					extra = $("#hardwarecontent #divevohome #controllerid").val();

					if (typeof baudrate == 'undefined') {
						ShowNotify($.t('No baud rate selected!'), 2500, true);
						return;
					}

					Mode1 = baudrate;
				}

				if (text.indexOf("MySensors") >= 0) {
					var baudrate = $("#hardwarecontent #divbaudratemysensors #combobaudrate option:selected").val();

					if (typeof baudrate == 'undefined') {
						ShowNotify($.t('No baud rate selected!'), 2500, true);
						return;
					}

					Mode1 = baudrate;
				}

				if (text.indexOf("P1 Smart Meter") >= 0) {
					var baudrate = $("#hardwarecontent #divbaudratep1 #combobaudratep1 option:selected").val();

					if (typeof baudrate == 'undefined') {
						ShowNotify($.t('No baud rate selected!'), 2500, true);
						return;
					}

					Mode1 = baudrate;
					Mode2 = $("#hardwarecontent #divcrcp1 #disablecrcp1").prop("checked") ? 0 : 1;
					var ratelimitp1 = $("#hardwarecontent #hardwareparamsratelimitp1 #ratelimitp1").val();
					if (ratelimitp1 == "") {
						ratelimitp1 = "0";
					}
					Mode3 = ratelimitp1;
					var decryptionkey = $("#hardwarecontent #divkeyp1p1 #decryptionkey").val();
					if (decryptionkey.length % 2 != 0 ) {
						ShowNotify($.t("Invallid Descryption Key Length!"), 2500, true);
						return;
					}
					password = decryptionkey;
				}
				if (text.indexOf("Teleinfo EDF") >= 0) {
					var baudrate = $("#hardwarecontent #divbaudrateteleinfo #combobaudrateteleinfo option:selected").val();

					if (typeof baudrate == 'undefined') {
						ShowNotify($.t('No baud rate selected!'), 2500, true);
						return;
					}

					Mode1 = baudrate;
					Mode2 = $("#hardwarecontent #divcrcp1 #disablecrcp1").prop("checked") ? 0 : 1;
					var ratelimitp1 = $("#hardwarecontent #hardwareparamsratelimitp1 #ratelimitp1").val();
					if (ratelimitp1 == "") {
						ratelimitp1 = "60";
					}
					Mode3 = ratelimitp1;
				}

				if (text.indexOf("S0 Meter") >= 0) {
					extra = $.devExtra;
				}

                if (text.indexOf("Denkovi") >= 0) {
                    Mode1 = $("#hardwarecontent #divmodeldenkoviusbdevices #combomodeldenkoviusbdevices option:selected").val();
                }

				if (text.indexOf("USBtin") >= 0) {
					//var Typecan = $("#hardwarecontent #divusbtin #combotypecanusbtin option:selected").val();
					var ActivateMultiblocV8 = $("#hardwarecontent #divusbtin #activateMultiblocV8").prop("checked") ? 1 : 0;
					var ActivateCanFree = $("#hardwarecontent #divusbtin #activateCanFree").prop("checked") ? 1 : 0;
					var DebugActiv = $("#hardwarecontent #divusbtin #combodebugusbtin option:selected").val();
					Mode1 = (ActivateCanFree&0x01);
					Mode1 <<= 1;
					Mode1 += (ActivateMultiblocV8&0x01);
					Mode2 = DebugActiv;
				}

				$.ajax({
					url: "json.htm?type=command&param=updatehardware&htype=" + hardwaretype +
					"&loglevel=" + logLevel +
					"&port=" + encodeURIComponent(serialport) +
					"&extra=" + extra +
					"&name=" + encodeURIComponent(name) +
					"&enabled=" + bEnabled +
					"&idx=" + idx +
					"&datatimeout=" + datatimeout +
					"&password=" + encodeURIComponent(password) +
					"&Mode1=" + Mode1 + "&Mode2=" + Mode2 + "&Mode3=" + Mode3 + "&Mode4=" + Mode4 + "&Mode5=" + Mode5 + "&Mode6=" + Mode6,
					async: false,
					dataType: 'json',
					success: function (data) {
						if ((bEnabled)&&(text.indexOf("RFXCOM") >= 0)) {
							ShowNotify($.t('Please wait. Updating ....!'), 2500);
							setTimeout(hideAndRefreshHardwareTable, 3000)
						} else {
							RefreshHardwareTable();
						}
					},
					error: function () {
						ShowNotify($.t('Problem updating hardware!'), 2500, true);
					}
				});
			}
			else if (
				(text.indexOf("LAN") >= 0 &&
					text.indexOf("YouLess") == -1 &&
					text.indexOf("Denkovi") == -1 &&
					text.indexOf("Relay-Net") == -1 &&
					text.indexOf("Satel Integra") == -1 &&
					text.indexOf("eHouse") == -1 &&
					text.indexOf("ETH8020") == -1 &&
					text.indexOf("Daikin") == -1 &&
					text.indexOf("Sterbox") == -1 &&
					text.indexOf("Anna") == -1 &&
					text.indexOf("KMTronic") == -1 &&
					text.indexOf("MQTT") == -1 &&
					text.indexOf("Razberry") == -1 &&
                    text.indexOf("MyHome OpenWebNet with LAN interface") == -1 &&
                    text.indexOf("EnphaseAPI") == -1
				)
			) {
				var password = "";
				var address = $("#hardwarecontent #divremote #tcpaddress").val();
				if (address == "") {
					ShowNotify($.t('Please enter an Address!'), 2500, true);
					return;
				}
				var port = $("#hardwarecontent #divremote #tcpport").val();
				if (port == "") {
					ShowNotify($.t('Please enter an Port!'), 2500, true);
					return;
				}
				var intRegex = /^\d+$/;
				if (!intRegex.test(port)) {
					ShowNotify($.t('Please enter an Valid Port!'), 2500, true);
					return;
				}
				var extra = "";
				if (text.indexOf("Evohome") >= 0) {
					extra = $("#hardwarecontent #divevohometcp #controlleridevohometcp").val();
				}

				if (text.indexOf("S0 Meter") >= 0) {
					extra = $.devExtra;
				}

				if (text.indexOf("P1 Smart Meter") >= 0) {
					Mode2 = $("#hardwarecontent #divcrcp1 #disablecrcp1").prop("checked") ? 0 : 1;
					var ratelimitp1 = $("#hardwarecontent #hardwareparamsratelimitp1 #ratelimitp1").val();
					if (ratelimitp1 == "") {
						ratelimitp1 = "0";
					}
					Mode3 = ratelimitp1;
					var decryptionkey = $("#hardwarecontent #divkeyp1p1 #decryptionkey").val();
					if (decryptionkey.length % 2 != 0 ) {
						ShowNotify($.t("Invallid Descryption Key Length!"), 2500, true);
						return;
					}
					password = decryptionkey;
				}
				if (text.indexOf("Teleinfo EDF") >= 0) {
					Mode2 = $("#hardwarecontent #divcrcp1 #disablecrcp1").prop("checked") ? 0 : 1;
					var ratelimitp1 = $("#hardwarecontent #hardwareparamsratelimitp1 #ratelimitp1").val();
					if (ratelimitp1 == "") {
						ratelimitp1 = "60";
					}
					Mode3 = ratelimitp1;
				}

				$.ajax({
					url: "json.htm?type=command&param=updatehardware&htype=" + hardwaretype +
					"&loglevel=" + logLevel +
					"&address=" + address +
					"&port=" + port +
					"&extra=" + extra +
					"&name=" + encodeURIComponent(name) +
					"&enabled=" + bEnabled +
					"&idx=" + idx +
					"&datatimeout=" + datatimeout +
					"&password=" + encodeURIComponent(password) +
					"&Mode1=" + Mode1 + "&Mode2=" + Mode2 + "&Mode3=" + Mode3 + "&Mode4=" + Mode4 + "&Mode5=" + Mode5 + "&Mode6=" + Mode6,
					async: false,
					dataType: 'json',
					success: function (data) {
						if ((bEnabled)&&(text.indexOf("RFXCOM") >= 0)) {
							ShowNotify($.t('Please wait. Updating ....!'), 2500);
							setTimeout(hideAndRefreshHardwareTable, 3000)
						} else {
							RefreshHardwareTable();
						}
					},
					error: function () {
						ShowNotify($.t('Problem updating hardware!'), 2500, true);
					}
				});
			}
			else if (
				(text.indexOf("LAN") >= 0 && ((text.indexOf("YouLess") >= 0) || (text.indexOf("Denkovi") >= 0))) ||
				(text.indexOf("Relay-Net") >= 0) || (text.indexOf("Satel Integra") >= 0) || (text.indexOf("eHouse") >= 0) || (text.indexOf("Harmony") >= 0) || (text.indexOf("Xiaomi Gateway") >= 0) || (text.indexOf("MyHome OpenWebNet with LAN interface") >= 0)
			) {
				var address = $("#hardwarecontent #divremote #tcpaddress").val();
				if (text.indexOf("eHouse") >= 0) {
						if (address == "") address="192.168.0.200";
						}
				if (address == "") {
					ShowNotify($.t('Please enter an Address!'), 2500, true);
					return;
				}
				var port = $("#hardwarecontent #divremote #tcpport").val();
				if (text.indexOf("eHouse") >= 0) {
						if (port == "") port="9876";
						}

				if (port == "") {
					ShowNotify($.t('Please enter an Port!'), 2500, true);
					return;
				}
				var intRegex = /^\d+$/;
				if (!intRegex.test(port)) {
					ShowNotify($.t('Please enter an Valid Port!'), 2500, true);
					return;
				}

				if (text.indexOf("Satel Integra") >= 0) {
					var pollinterval = $("#hardwarecontent #hardwareparamspollinterval #pollinterval").val();
					if (pollinterval == "") {
						ShowNotify($.t('Please enter poll interval!'), 2500, true);
						return;
					}
					Mode1 = pollinterval;
				}
				else if (text.indexOf("eHouse") >= 0) {
					var pollinterval = $("#hardwarecontent #hardwareparamspollinterval #pollinterval").val();
					if (pollinterval == "") {
						ShowNotify($.t('Please enter poll interval!'), 2500, true);
						return;
						}
					Mode1 = pollinterval;
					Mode2 = $('#hardwarecontent #hardwareparamsehouse #ehouseautodiscovery').prop("checked") ? 1 : 0;
					Mode3 = $("#hardwarecontent #hardwareparamsehouse #ehouseaddalarmin").prop("checked") ? 1 : 0;
					Mode4 = $("#hardwarecontent #hardwareparamsehouse #ehouseprodiscovery").prop("checked") ? 1 : 0;
					Mode5 = $("#hardwarecontent #hardwareparamsehouse #ehouseopts").val();
					Mode6 = $("#hardwarecontent #hardwareparamsehouse #ehouseopts2").val();
				}
				if (text.indexOf("Denkovi") >= 0) {
					var pollinterval = $("#hardwarecontent #hardwareparamspollinterval #pollinterval").val();
					if (pollinterval == "") {
						ShowNotify($.t('Please enter poll interval!'), 2500, true);
						return;
					}
					Mode1 = pollinterval;
					if (text.indexOf("Modules with LAN (HTTP)") >= 0)
						Mode2 = $("#hardwarecontent #divmodeldenkovidevices #combomodeldenkovidevices option:selected").val();
					//else if (text.indexOf("Modules with LAN (TCP)") >= 0)
					//	Mode2 = $("#hardwarecontent #divmodeldenkovitcpdevices #combomodeldenkovitcpdevices option:selected").val();
					else if (text.indexOf("Modules with LAN (TCP)") >= 0) {
						Mode2 = $("#hardwarecontent #divmodeldenkovitcpdevices #combomodeldenkovitcpdevices option:selected").val();
						Mode3 = $("#hardwarecontent #divmodeldenkovitcpdevices #denkovislaveid").val();
						if(Mode2 == "1"){
							var intRegex = /^\d+$/;
							if (isNaN(Mode3) || Number(Mode3) < 1 || Number(Mode3) > 247) {
								ShowNotify($.t('Invalid Slave ID! Enter value from 1 to 247!'), 2500, true);
								return;
							}
						} else
							Mode3 = "0";
					}
					Mode4 = "0";
					Mode5 = "0";
					Mode6 = "0";

				/*$.ajax({
					url: "json.htm?type=command&param=updatehardware&htype=" + hardwaretype +
					"&loglevel=" + logLevel +
					"&port=" + refresh +
					"&username=" + encodeURIComponent(username) +
					"&password=" + encodeURIComponent(password) +
					"&name=" + encodeURIComponent(name) +
					"&enabled=" + bEnabled +
					"&idx=" + idx +
					"&datatimeout=" + datatimeout +
					"&address=" + encodeURIComponent(url) +
					"&Mode1=" + Mode1 + "&Mode2=" + Mode2 + "&Mode3=" + Mode3 + "&Mode4=" + Mode4 + "&Mode5=" + Mode5 + "&Mode6=" + Mode6 +
					"&extra=" + extra,
					async: false,
					dataType: 'json',
					success: function (data) {
						RefreshHardwareTable();
					},
					error: function () {
						ShowNotify($.t('Problem updating hardware!'), 2500, true);
					}
				});*/

				}
				else if (text.indexOf("Relay-Net") >= 0) {
					Mode1 = $('#hardwarecontent #hardwareparamsrelaynet #relaynetpollinputs').prop("checked") ? 1 : 0;
					Mode2 = $('#hardwarecontent #hardwareparamsrelaynet #relaynetpollrelays').prop("checked") ? 1 : 0;
					var pollinterval = $("#hardwarecontent #hardwareparamsrelaynet #relaynetpollinterval").val();
					var inputcount = $("#hardwarecontent #hardwareparamsrelaynet #relaynetinputcount").val();
					var relaycount = $("#hardwarecontent #hardwareparamsrelaynet #relaynetrelaycount").val();

					if (pollinterval == "") {
						ShowNotify($.t('Please enter poll interval!'), 2500, true);
						return;
					}

					if (inputcount == "") {
						ShowNotify($.t('Please enter input count!'), 2500, true);
						return;
					}

					if (relaycount == "") {
						ShowNotify($.t('Please enter relay count!'), 2500, true);
						return;
					}

					Mode3 = pollinterval;
					Mode4 = inputcount;
					Mode5 = relaycount;
				}
				var password = encodeURIComponent($("#hardwarecontent #divlogin #password").val());
				if (text.indexOf("eHouse") >= 0) {
					if (password == "") {
								ShowNotify($.t('Please enter ASCI password - 6 characters'), 2500, true);
								}
					}
                if (text.indexOf("MyHome OpenWebNet with LAN interface") >= 0) {
                    if (password != "") {

                        if ((password.length < 5) || (password.length > 16)) {
                            ShowNotify($.t('Please enter a password between 5 and 16 characters!'), 2500, true);
                            return;
                        }

                        //var intRegex = /^[a-zA-Z0-9]*$/; 
                        //if (!intRegex.test(password)) {
                        //    ShowNotify($.t('Please enter a numeric or alphanumeric (for HMAC) password'), 2500, true);
                        //    return;
                        //}
                    }

                    var ratelimitp1 = $("#hardwarecontent #hardwareparamsratelimitp1 #ratelimitp1").val();
                    if ((ratelimitp1 == "") || (isNaN(ratelimitp1))) {
                        ShowNotify($.t('Please enter rate limit!'), 2500, true);
                        return;
                    }
                    Mode1 = ratelimitp1;

					var ensynchro = $("#hardwarecontent #hardwareparamsensynchro #ensynchro").val();
                    if ((ensynchro == "") || (isNaN(ensynchro))) {
                        ShowNotify($.t('Please enter time sinchronization!'), 2500, true);
                        return;
                    }
                    Mode2 = ensynchro;
                }
				$.ajax({
					url: "json.htm?type=command&param=updatehardware&htype=" + hardwaretype +
					"&loglevel=" + logLevel +
					"&address=" + address +
					"&port=" + port +
					"&name=" + encodeURIComponent(name) +
					"&password=" + encodeURIComponent(password) +
					"&enabled=" + bEnabled +
					"&idx=" + idx +
					"&datatimeout=" + datatimeout +
					"&Mode1=" + Mode1 + "&Mode2=" + Mode2 + "&Mode3=" + Mode3 + "&Mode4=" + Mode4 + "&Mode5=" + Mode5 + "&Mode6=" + Mode6,
					async: false,
					dataType: 'json',
					success: function (data) {
						RefreshHardwareTable();
					},
					error: function () {
						ShowNotify($.t('Problem updating hardware!'), 2500, true);
					}
				});
			}
			else if (
				(text.indexOf("Domoticz") >= 0) ||
				(text.indexOf("Eco Devices") >= 0) ||
				(text.indexOf("ETH8020") >= 0) ||
				(text.indexOf("Daikin") >= 0) ||
				(text.indexOf("Sterbox") >= 0) ||
				(text.indexOf("Anna") >= 0) ||
				(text.indexOf("KMTronic") >= 0) ||
				(text.indexOf("MQTT") >= 0) ||
				(text.indexOf("Razberry") >= 0)
			) {
				var address = $("#hardwarecontent #divremote #tcpaddress").val();
				if (address == "") {
					ShowNotify($.t('Please enter an Address!'), 2500, true);
					return;
				}
				var port = $("#hardwarecontent #divremote #tcpport").val();
				if (port == "") {
					ShowNotify($.t('Please enter an Port!'), 2500, true);
					return;
				}
				var intRegex = /^\d+$/;
				if (!intRegex.test(port)) {
					ShowNotify($.t('Please enter an Valid Port!'), 2500, true);
					return;
				}
				var username = $("#hardwarecontent #divlogin #username").val();
				var password = $("#hardwarecontent #divlogin #password").val();
				var extra = "";
				var Mode1 = "";
				if (text.indexOf("MySensors Gateway with MQTT") >= 0) {
					extra = $("#hardwarecontent #divmysensorsmqtt #filename").val();
					Mode1 = $("#hardwarecontent #divmysensorsmqtt #combotopicselect").val();
					Mode2 = $("#hardwarecontent #divmysensorsmqtt #combotlsversion").val();
					Mode3 = $("#hardwarecontent #divmysensorsmqtt #combopreventloop").val();

					if ($("#hardwarecontent #divmysensorsmqtt #filename").val().indexOf("#") >= 0) {
						ShowNotify($.t('CA Filename cannot contain a "#" symbol!'), 2500, true);
						return;
					}
					if ($("#hardwarecontent #divmysensorsmqtt #mqtttopicin").val().indexOf("#") >= 0) {
						ShowNotify($.t('Publish Prefix cannot contain a "#" symbol!'), 2500, true);
						return;
					}
					if ($("#hardwarecontent #divmysensorsmqtt #mqtttopicout").val().indexOf("#") >= 0) {
						ShowNotify($.t('Subscribe Prefix cannot contain a "#" symbol!'), 2500, true);
						return;
					}
					if ((Mode1 == 2) && (($("#hardwarecontent #divmysensorsmqtt #mqtttopicin").val() == "") || ($("#hardwarecontent #divmysensorsmqtt #mqtttopicout").val() == ""))) {
						ShowNotify($.t('Please enter Topic Prefixes!'), 2500, true);
						return;
					}
					if (Mode1 == 2) {
						if (($("#hardwarecontent #divmysensorsmqtt #mqtttopicin").val() == "") ||
						    ($("#hardwarecontent #divmysensorsmqtt #mqtttopicout").val() == "")
						) {
							ShowNotify($.t('Please enter Topic Prefixes!'), 2500, true);
							return;
						}
						extra += "#";
					        extra += $("#hardwarecontent #divmysensorsmqtt #mqtttopicin").val();
						extra += "#";
						extra += $("#hardwarecontent #divmysensorsmqtt #mqtttopicout").val();
					}
					extra = encodeURIComponent(extra);
				}
				else if ((text.indexOf("MQTT") >= 0)) {
					extra = $("#hardwarecontent #divmqtt #filename").val().trim();
					var mqtttopicin = $("#hardwarecontent #divmqtt #mqtttopicin").val().trim();
					var mqtttopicout = $("#hardwarecontent #divmqtt #mqtttopicout").val().trim();
					if (mqtttopicin.indexOf("#") >= 0) {
						ShowNotify($.t('Publish Prefix cannot contain a "#" symbol!'), 2500, true);
						return;
					}
					if (mqtttopicout.indexOf("#") >= 0) {
						ShowNotify($.t('Subscribe Prefix cannot contain a "#" symbol!'), 2500, true);
						return;
					}
					if ((mqtttopicin!="")||(mqtttopicout!="")) {
						extra += ";" + mqtttopicin + ";" + mqtttopicout;
					}
					
					Mode1 = $("#hardwarecontent #divmqtt #combotopicselect").val();
					Mode2 = $("#hardwarecontent #divmqtt #combotlsversion").val();
					Mode3 = $("#hardwarecontent #divmqtt #combopreventloop").val();
				}
				if (text.indexOf("Eco Devices") >= 0) {
					Mode1 = $("#hardwarecontent #divmodelecodevices #combomodelecodevices option:selected").val();
					var ratelimitp1 = $("#hardwarecontent #hardwareparamsratelimitp1 #ratelimitp1").val();
					if (ratelimitp1 == "") {
						ratelimitp1 = "60";
					}
					Mode2 = ratelimitp1;
				}

				$.ajax({
					url: "json.htm?type=command&param=updatehardware&htype=" + hardwaretype +
					"&loglevel=" + logLevel +
					"&address=" + address +
					"&port=" + port +
					"&username=" + encodeURIComponent(username) +
					"&password=" + encodeURIComponent(password) +
					"&extra=" + encodeURIComponent(extra) +
					"&name=" + encodeURIComponent(name) +
					"&enabled=" + bEnabled +
					"&idx=" + idx +
					"&datatimeout=" + datatimeout +
					"&Mode1=" + Mode1 + "&Mode2=" + Mode2 + "&Mode3=" + Mode3 + "&Mode4=" + Mode4 + "&Mode5=" + Mode5 + "&Mode6=" + Mode6,
					async: false,
					dataType: 'json',
					success: function (data) {
						RefreshHardwareTable();
					},
					error: function () {
						ShowNotify($.t('Problem updating hardware!'), 2500, true);
					}
				});
			}
			else if (text.indexOf("Philips Hue") >= 0) {
				var address = $("#hardwarecontent #divremote #tcpaddress").val();
				if (address == "") {
					ShowNotify($.t('Please enter an Address!'), 2500, true);
					return;
				}
				var port = $("#hardwarecontent #divremote #tcpport").val();
				if (port == "") {
					ShowNotify($.t('Please enter an Port!'), 2500, true);
					return;
				}
				var intRegex = /^\d+$/;
				if (!intRegex.test(port)) {
					ShowNotify($.t('Please enter an Valid Port!'), 2500, true);
					return;
				}
				var username = $("#hardwarecontent #hardwareparamsphilipshue #username").val();
				if (username == "") {
					ShowNotify($.t('Please enter a username!'), 2500, true);
					return;
				}
				var pollinterval = $("#hardwarecontent #hardwareparamsphilipshue #pollinterval").val();
				if (pollinterval == "") {
					ShowNotify($.t('Please enter poll interval!'), 2500, true);
					return;
				}
				Mode1 = pollinterval;
				Mode2 = 0;
				if($("#hardwarecontent #hardwareparamsphilipshue #addgroups").prop('checked')) {
					Mode2 |= 1;
				}
				if($("#hardwarecontent #hardwareparamsphilipshue #addscenes").prop('checked')) {
					Mode2 |= 2;
				}
				$.ajax({
					url: "json.htm?type=command&param=updatehardware&htype=" + hardwaretype +
					"&loglevel=" + logLevel +
					"&address=" + address +
					"&port=" + port +
					"&username=" + encodeURIComponent(username) +
					"&name=" + encodeURIComponent(name) +
					"&enabled=" + bEnabled +
					"&idx=" + idx +
					"&datatimeout=" + datatimeout +
					"&Mode1=" + Mode1 + "&Mode2=" + Mode2 + "&Mode3=" + Mode3 + "&Mode4=" + Mode4 + "&Mode5=" + Mode5 + "&Mode6=" + Mode6,
					async: false,
					dataType: 'json',
					success: function (data) {
						RefreshHardwareTable();
					},
					error: function () {
						ShowNotify($.t('Problem updating hardware!'), 2500, true);
					}
				});
			}
			else if ((text.indexOf("HTTP/HTTPS") >= 0)) {
				var url = $("#hardwarecontent #divhttppoller #url").val();
				if (url == "") {
					ShowNotify($.t('Please enter an url!'), 2500, true);
					return;
				}
				var script = $("#hardwarecontent #divhttppoller #script").val();
				if (script == "") {
					ShowNotify($.t('Please enter a script!'), 2500, true);
					return;
				}
				var refresh = $("#hardwarecontent #divhttppoller #refresh").val();
				if (refresh == "") {
					ShowNotify($.t('Please enter a refresh rate!'), 2500, true);
					return;
				}
				var username = $("#hardwarecontent #divlogin #username").val();
				var password = $("#hardwarecontent #divlogin #password").val();
				var method = $("#hardwarecontent #divhttppoller #combomethod option:selected").val();
				if (typeof method == 'undefined') {
					ShowNotify($.t('No HTTP method selected!'), 2500, true);
					return;
				}
				var contenttype = $("#hardwarecontent #divhttppoller #contenttype").val();
				var headers = $("#hardwarecontent #divhttppoller #headers").val();
				var postdata = $("#hardwarecontent #divhttppoller #postdata").val();
				var extra = btoa(script) + "|" + btoa(method) + "|" + btoa(contenttype) + "|" + btoa(headers);
				if (method == "1") {
					extra = extra + "|" + btoa(postdata);
				}

				$.ajax({
					url: "json.htm?type=command&param=updatehardware&htype=" + hardwaretype +
					"&loglevel=" + logLevel +
					"&port=" + refresh +
					"&username=" + encodeURIComponent(username) +
					"&password=" + encodeURIComponent(password) +
					"&name=" + encodeURIComponent(name) +
					"&enabled=" + bEnabled +
					"&idx=" + idx +
					"&datatimeout=" + datatimeout +
					"&address=" + encodeURIComponent(url) +
					"&extra=" + extra,
					async: false,
					dataType: 'json',
					success: function (data) {
						RefreshHardwareTable();
					},
					error: function () {
						ShowNotify($.t('Problem updating hardware!'), 2500, true);
					}
				});
			}
			else if ((text.indexOf("Underground") >= 0) || (text.indexOf("DarkSky") >= 0) || (text.indexOf("AccuWeather") >= 0)) {
				var apikey = $("#hardwarecontent #divunderground #apikey").val();
				if (apikey == "") {
					ShowNotify($.t('Please enter an API Key!'), 2500, true);
					return;
				}
				var location = $("#hardwarecontent #divunderground #location").val();
				if (location == "") {
					ShowNotify($.t('Please enter an Location!'), 2500, true);
					return;
				}
				$.ajax({
					url: "json.htm?type=command&param=updatehardware&htype=" + hardwaretype +
					"&loglevel=" + logLevel +
					"&username=" + encodeURIComponent(apikey) +
					"&password=" + encodeURIComponent(location) +
					"&name=" + encodeURIComponent(name) +
					"&enabled=" + bEnabled +
					"&idx=" + idx +
					"&datatimeout=" + datatimeout +
					"&Mode1=" + Mode1 + "&Mode2=" + Mode2 + "&Mode3=" + Mode3 + "&Mode4=" + Mode4 + "&Mode5=" + Mode5 + "&Mode6=" + Mode6,
					async: false,
					dataType: 'json',
					success: function (data) {
						RefreshHardwareTable();
					},
					error: function () {
						ShowNotify($.t('Problem updating hardware!'), 2500, true);
					}
				});
			}
			else if(text.indexOf("Meteorologisk") >= 0){
				var location = $("#hardwarecontent #divlocation #location").val();
				$.ajax({
					url: "json.htm?type=command&param=updatehardware&htype=" + hardwaretype +
					"&loglevel=" + logLevel +
					"&password=" + encodeURIComponent(location) +
					"&name=" + encodeURIComponent(name) +
					"&enabled=" + bEnabled +
					"&idx=" + idx +
					"&datatimeout=" + datatimeout +
					"&Mode1=" + Mode1 + "&Mode2=" + Mode2 + "&Mode3=" + Mode3 + "&Mode4=" + Mode4 + "&Mode5=" + Mode5 + "&Mode6=" + Mode6,
					async: false,
					dataType: 'json',
					success: function (data) {
						RefreshHardwareTable();
					},
					error: function () {
						ShowNotify($.t('Problem updating hardware!'), 2500, true);
					}
				});
			}
			else if(text.indexOf("Open Weather Map") >= 0){
				var apikey = $("#hardwarecontent #divopenweathermap #apikey").val();
				if (apikey == "") {
					ShowNotify($.t('Please enter an API Key!'), 2500, true);
					return;
				}
				var location = $("#hardwarecontent #divopenweathermap #location").val();
				if (location == "") {
					ShowNotify($.t('Please enter an Location (or 0 to use Domoticz home location)!'), 2500, true);
					return;
				}
				var adddayforecast = $("#hardwarecontent #divopenweathermap #adddayforecast").prop("checked") ? 1 : 0;
				var addhourforecast = $("#hardwarecontent #divopenweathermap #addhourforecast").prop("checked") ? 1 : 0;
				$.ajax({
					url: "json.htm?type=command&param=updatehardware&htype=" + hardwaretype +
					"&loglevel=" + logLevel +
					"&username=" + encodeURIComponent(apikey) +
					"&password=" + encodeURIComponent(location) +
					"&name=" + encodeURIComponent(name) +
					"&enabled=" + bEnabled +
					"&idx=" + idx +
					"&datatimeout=" + datatimeout +
					"&Mode1=" + adddayforecast + "&Mode2=" + addhourforecast + "&Mode3=" + Mode3 + "&Mode4=" + Mode4 + "&Mode5=" + Mode5 + "&Mode6=" + Mode6,
					async: false,
					dataType: 'json',
					success: function (data) {
						RefreshHardwareTable();
					},
					error: function () {
						ShowNotify($.t('Problem updating hardware!'), 2500, true);
					}
				});
			}
			else if (text.indexOf("Buienradar") >= 0) {
				var timeframe = $("#hardwarecontent #divbuienradar #timeframe").val();
				if (timeframe == 0) {
					timeframe = 30;
				}
				var threshold = $("#hardwarecontent #divbuienradar #threshold").val();
				if (threshold == 0) {
					threshold = 25;
				}
				var location = $("#hardwarecontent #divbuienradar #location").val();
				$.ajax({
					url: "json.htm?type=command&param=updatehardware&htype=" + hardwaretype +
					"&loglevel=" + logLevel +
					"&password=" + location +
					"&name=" + encodeURIComponent(name) +
					"&enabled=" + bEnabled +
					"&idx=" + idx +
					"&datatimeout=" + datatimeout +
					"&Mode1=" + timeframe + "&Mode2=" + threshold + "&Mode3=" + Mode3 + "&Mode4=" + Mode4 + "&Mode5=" + Mode5 + "&Mode6=" + Mode6,
					async: false,
					dataType: 'json',
					success: function (data) {
						RefreshHardwareTable();
					},
					error: function () {
						ShowNotify($.t('Problem updating hardware!'), 2500, true);
					}
				});
			} else if (text.indexOf("SolarEdge via Web") >= 0) {
				var apikey = $("#hardwarecontent #divsolaredgeapi #apikey").val();
				if (apikey == "") {
					ShowNotify($.t('Please enter an API Key!'), 2500, true);
					return;
				}

				$.ajax({
					url: "json.htm?type=command&param=updatehardware&htype=" + hardwaretype +
					"&loglevel=" + logLevel +
					"&username=" + encodeURIComponent(apikey) +
					"&name=" + encodeURIComponent(name) +
					"&enabled=" + bEnabled +
					"&idx=" + idx +
					"&datatimeout=" + datatimeout,
					async: false,
					dataType: 'json',
					success: function (data) {
						RefreshHardwareTable();
					},
					error: function () {
						ShowNotify($.t('Problem updating hardware!'), 2500, true);
					}
				});
			}
			else if (text.indexOf("Nest Th") >= 0 && text.indexOf("OAuth") >= 0) {
			    var apikey = $("#hardwarecontent #divnestoauthapi #apikey").val();
			    var productid = $("#hardwarecontent #divnestoauthapi #productid").val();
			    var productsecret = $("#hardwarecontent #divnestoauthapi #productsecret").val();
			    var productpin = $("#hardwarecontent #divnestoauthapi #productpin").val();

			    if (apikey == "" && (productid == "" || productsecret == "" || productpin == "")) {
			        ShowNotify($.t('Please enter an API Key or a combination of Product Id, Product Secret and PIN!'), 2500, true);
			        return;
			    }

			    var extra = btoa(productid) + "|" + btoa(productsecret) + "|" + btoa(productpin);
			    console.log("Updating extra1: " + extra);

			    $.ajax({
					url: "json.htm?type=command&param=updatehardware&htype=" + hardwaretype +
						"&loglevel=" + logLevel +
						"&username=" + encodeURIComponent(apikey) +
						"&name=" + encodeURIComponent(name) +
						"&enabled=" + bEnabled +
						"&idx=" + idx +
						"&extra=" + extra +
						"&datatimeout=" + datatimeout,
			        async: false,
			        dataType: 'json',
			        success: function (data) {
			            RefreshHardwareTable();
			        },
			        error: function () {
			            ShowNotify($.t('Problem updating hardware!'), 2500, true);
			        }
			    });
			}
			else if (text.indexOf("SBFSpot") >= 0) {
				var configlocation = $("#hardwarecontent #divlocation #location").val();
				if (configlocation == "") {
					ShowNotify($.t('Please enter an Location!'), 2500, true);
					return;
				}
				$.ajax({
					url: "json.htm?type=command&param=updatehardware&htype=" + hardwaretype +
					"&loglevel=" + logLevel +
					"&username=" + encodeURIComponent(configlocation) +
					"&name=" + encodeURIComponent(name) +
					"&enabled=" + bEnabled +
					"&idx=" + idx +
					"&datatimeout=" + datatimeout +
					"&Mode1=" + Mode1 + "&Mode2=" + Mode2 + "&Mode3=" + Mode3 + "&Mode4=" + Mode4 + "&Mode5=" + Mode5 + "&Mode6=" + Mode6,
					async: false,
					dataType: 'json',
					success: function (data) {
						RefreshHardwareTable();
					},
					error: function () {
						ShowNotify($.t('Problem updating hardware!'), 2500, true);
					}
				});
			}
			else if (text.indexOf("Toon") >= 0) {
				var username = $("#hardwarecontent #divlogin #username").val();
				var password = encodeURIComponent($("#hardwarecontent #divlogin #password").val());
				var agreement = $("#hardwarecontent #divenecotoon #agreement").val();
				$.ajax({
					url: "json.htm?type=command&param=updatehardware&htype=" + hardwaretype +
					"&loglevel=" + logLevel +
					"&username=" + encodeURIComponent(username) +
					"&password=" + encodeURIComponent(password) +
					"&name=" + encodeURIComponent(name) +
					"&enabled=" + bEnabled +
					"&idx=" + idx +
					"&datatimeout=" + datatimeout +
					"&Mode1=" + agreement,
					async: false,
					dataType: 'json',
					success: function (data) {
						RefreshHardwareTable();
					},
					error: function () {
						ShowNotify($.t('Problem updating hardware!'), 2500, true);
					}
				});
			}
			else if (text.indexOf("Tesla") >= 0) {
				var username = $("#hardwarecontent #divlogin #username").val();
				var password = encodeURIComponent($("#hardwarecontent #divlogin #password").val());
				var vinnr = $("#hardwarecontent #divtesla #vinnr").val();
				var activeinterval = parseInt($("#hardwarecontent #divtesla #activeinterval").val());
				if (activeinterval < 1) {
					activeinterval = 1;
				}
				var defaultinterval = parseInt($("#hardwarecontent #divtesla #defaultinterval").val());
				if (defaultinterval < 1) {
					defaultinterval = 20;
				}
				var allowwakeup = $("#hardwarecontent #divtesla #comboallowwakeup").val();
				$.ajax({
					url: "json.htm?type=command&param=updatehardware&htype=" + hardwaretype +
					"&loglevel=" + logLevel +
					"&username=" + encodeURIComponent(username) +
					"&password=" + encodeURIComponent(password) +
					"&name=" + encodeURIComponent(name) +
					"&enabled=" + bEnabled +
					"&idx=" + idx +
					"&datatimeout=" + datatimeout +
					"&extra=" + vinnr +
					"&Mode1=" + defaultinterval +
					"&Mode2=" + activeinterval + 
					"&Mode3=" + allowwakeup,
					async: false,
					dataType: 'json',
					success: function (data) {
						RefreshHardwareTable();
					},
					error: function () {
						ShowNotify($.t('Problem updating hardware!'), 2500, true);
					}
				});
			}
			else if (text.indexOf("Mercedes") >= 0) {
				var username = $("#hardwarecontent #divlogin #username").val();
				var password = encodeURIComponent($("#hardwarecontent #divlogin #password").val());
				var vinnr = $("#hardwarecontent #divmercedes #vinnr").val();
				var activeinterval = parseInt($("#hardwarecontent #divmercedes #activeinterval").val());
				if (activeinterval < 1) {
					activeinterval = 1;
				}
				var defaultinterval = parseInt($("#hardwarecontent #divmercedes #defaultinterval").val());
				if (defaultinterval < 1) {
					defaultinterval = 20;
				}
				var allowwakeup = $("#hardwarecontent #divmercedes #comboallowwakeup").val();
				$.ajax({
					url: "json.htm?type=command&param=updatehardware&htype=" + hardwaretype +
					"&loglevel=" + logLevel +
					"&username=" + encodeURIComponent(username) +
					"&password=" + encodeURIComponent(password) +
					"&name=" + encodeURIComponent(name) +
					"&enabled=" + bEnabled +
					"&idx=" + idx +
					"&datatimeout=" + datatimeout +
					"&extra=" + vinnr +
					"&Mode1=" + defaultinterval +
					"&Mode2=" + activeinterval + 
					"&Mode3=" + allowwakeup,
					async: false,
					dataType: 'json',
					success: function (data) {
						RefreshHardwareTable();
					},
					error: function () {
						ShowNotify($.t('Problem updating hardware!'), 2500, true);
					}
				});
			}
			else if (
				(text.indexOf("ICY") >= 0) ||
				(text.indexOf("Atag") >= 0) ||
				(text.indexOf("Nest Th") >= 0 && text.indexOf("OAuth") === -1) ||
				(text.indexOf("PVOutput") >= 0) ||
				(text.indexOf("Netatmo") >= 0) ||
				(text.indexOf("Thermosmart") >= 0) ||
                (text.indexOf("Tado") >= 0)
			) {
				var username = $("#hardwarecontent #divlogin #username").val();
				var password = encodeURIComponent($("#hardwarecontent #divlogin #password").val());
				$.ajax({
					url: "json.htm?type=command&param=updatehardware&htype=" + hardwaretype +
					"&loglevel=" + logLevel +
					"&username=" + encodeURIComponent(username) +
					"&password=" + encodeURIComponent(password) +
					"&name=" + encodeURIComponent(name) +
					"&enabled=" + bEnabled +
					"&idx=" + idx +
					"&datatimeout=" + datatimeout +
					"&Mode1=" + Mode1 + "&Mode2=" + Mode2 + "&Mode3=" + Mode3 + "&Mode4=" + Mode4 + "&Mode5=" + Mode5 + "&Mode6=" + Mode6,
					async: false,
					dataType: 'json',
					success: function (data) {
						RefreshHardwareTable();
					},
					error: function () {
						ShowNotify($.t('Problem updating hardware!'), 2500, true);
					}
				});
			}
			else if (text.indexOf("Winddelen") >= 0) {
				var mill_id = $("#hardwarecontent #divwinddelen #combomillselect").val();
				var mill_name = $("#hardwarecontent #divwinddelen #combomillselect").find("option:selected").text()
				var nrofwinddelen = $("#hardwarecontent #divwinddelen #nrofwinddelen").val();
				var intRegex = /^\d+$/;
				if (!intRegex.test(nrofwinddelen)) {
					ShowNotify($.t('Please enter an Valid Number!'), 2500, true);
					return;
				}
				$.ajax({
					url: "json.htm?type=command&param=updatehardware&htype=" + hardwaretype +
					"&loglevel=" + logLevel +
					"&name=" + encodeURIComponent(name) +
					"&address=" + encodeURIComponent(mill_name) +
					"&port=" + encodeURIComponent(nrofwinddelen) +
					"&Mode1=" + encodeURIComponent(mill_id) +
					"&enabled=" + bEnabled +
					"&idx=" + idx +
					"&datatimeout=" + datatimeout +
					"&Mode2=" + Mode2 + "&Mode3=" + Mode3 + "&Mode4=" + Mode4 + "&Mode5=" + Mode5 + "&Mode6=" + Mode6,
					async: false,
					dataType: 'json',
					success: function (data) {
						RefreshHardwareTable();
					},
					error: function () {
						ShowNotify($.t('Problem updating hardware!'), 2500, true);
					}
				});
			}
			else if (text.indexOf("Honeywell") >= 0) {
				var apiKey = $("#hardwarecontent #divhoneywell #hwApiKey").val();
				var apiSecret = $("#hardwarecontent #divhoneywell #hwApiSecret").val();
				var accessToken = $("#hardwarecontent #divhoneywell #hwAccessToken").val();
				var refreshToken = $("#hardwarecontent #divhoneywell #hwRefreshToken").val();
				var extra = btoa(apiKey) + "|" + btoa(apiSecret);

				$.ajax({
					url: "json.htm?type=command&param=updatehardware&htype=" + hardwaretype +
					"&loglevel=" + logLevel +
					"&name=" + encodeURIComponent(name) +
					"&username=" + encodeURIComponent(accessToken) +
					"&password=" + encodeURIComponent(refreshToken) +
					"&Mode1=" + Mode1 +
					"&enabled=" + bEnabled +
					"&idx=" + idx +
					"&extra=" + extra +
					"&datatimeout=" + datatimeout +
					"&Mode2=" + Mode2 + "&Mode3=" + Mode3 + "&Mode4=" + Mode4 + "&Mode5=" + Mode5 + "&Mode6=" + Mode6,
					async: false,
					dataType: 'json',
					success: function (data) {
						RefreshHardwareTable();
					},
					error: function () {
						ShowNotify($.t('Problem updating hardware!'), 2500, true);
					}
				});
			}
			else if (text.indexOf("Logitech Media Server") >= 0) {
				var address = $("#hardwarecontent #divremote #tcpaddress").val();
				if (address == "") {
					ShowNotify($.t('Please enter an Address!'), 2500, true);
					return;
				}
				var port = $("#hardwarecontent #divremote #tcpport").val();
				if (port == "") {
					ShowNotify($.t('Please enter an Port!'), 2500, true);
					return;
				}
				var intRegex = /^\d+$/;
				if (!intRegex.test(port)) {
					ShowNotify($.t('Please enter an Valid Port!'), 2500, true);
					return;
				}
				var username = $("#hardwarecontent #divlogin #username").val();
				var password = $("#hardwarecontent #divlogin #password").val();

				$.ajax({
					url: "json.htm?type=command&param=updatehardware&htype=" + hardwaretype +
					"&loglevel=" + logLevel +
					"&address=" + address +
					"&port=" + port +
					"&username=" + encodeURIComponent(username) +
					"&password=" + encodeURIComponent(password) +
					"&name=" + encodeURIComponent(name) +
					"&enabled=" + bEnabled +
					"&idx=" + idx +
					"&datatimeout=" + datatimeout +
					"&Mode1=" + Mode1 + "&Mode2=" + Mode2 + "&Mode3=" + Mode3 + "&Mode4=" + Mode4 + "&Mode5=" + Mode5 + "&Mode6=" + Mode6,
					async: false,
					dataType: 'json',
					success: function (data) {
						RefreshHardwareTable();
					},
					error: function () {
						ShowNotify($.t('Problem updating hardware!'), 2500, true);
					}
				});
			}
			else if (text.indexOf("HEOS by DENON") >= 0) {
				var address = $("#hardwarecontent #divremote #tcpaddress").val();
				if (address == "") {
					ShowNotify($.t('Please enter an Address!'), 2500, true);
					return;
				}
				var port = $("#hardwarecontent #divremote #tcpport").val();
				if (port == "") {
					ShowNotify($.t('Please enter an Port!'), 2500, true);
					return;
				}
				var intRegex = /^\d+$/;
				if (!intRegex.test(port)) {
					ShowNotify($.t('Please enter an Valid Port!'), 2500, true);
					return;
				}
				var username = $("#hardwarecontent #divlogin #username").val();
				var password = $("#hardwarecontent #divlogin #password").val();

				$.ajax({
					url: "json.htm?type=command&param=updatehardware&htype=" + hardwaretype +
					"&loglevel=" + logLevel +
					"&address=" + address +
					"&port=" + port +
					"&username=" + encodeURIComponent(username) +
					"&password=" + encodeURIComponent(password) +
					"&name=" + encodeURIComponent(name) +
					"&enabled=" + bEnabled +
					"&idx=" + idx +
					"&datatimeout=" + datatimeout +
					"&Mode1=" + Mode1 + "&Mode2=" + Mode2 + "&Mode3=" + Mode3 + "&Mode4=" + Mode4 + "&Mode5=" + Mode5 + "&Mode6=" + Mode6,
					async: false,
					dataType: 'json',
					success: function (data) {
						RefreshHardwareTable();
					},
					error: function () {
						ShowNotify($.t('Problem updating hardware!'), 2500, true);
					}
				});
			}
			else if (text.indexOf("Goodwe solar inverter via Web") >= 0) {
				var username = $("#hardwarecontent #divgoodweweb #username").val();
				if (username == "") {
					ShowNotify($.t('Please enter your Goodwe username!'), 2500, true);
					return;
				}
				Mode1 = $("#hardwarecontent #divgoodweweb #comboserverselect").val();

				$.ajax({
					url: "json.htm?type=command&param=updatehardware&htype=" + hardwaretype +
					"&loglevel=" + logLevel +
					"&username=" + encodeURIComponent(username) +
					"&name=" + encodeURIComponent(name) +
					"&enabled=" + bEnabled +
					"&idx=" + idx +
					"&datatimeout=" + datatimeout +
					"&Mode1=" + Mode1 + "&Mode2=" + Mode2 + "&Mode3=" + Mode3 + "&Mode4=" + Mode4 + "&Mode5=" + Mode5 + "&Mode6=" + Mode6,
					async: false,
					dataType: 'json',
					success: function (data) {
						RefreshHardwareTable();
					},
					error: function () {
						ShowNotify($.t('Problem updating hardware!'), 2500, true);
					}
				});
			}
			else if (text.indexOf("Evohome via Web") >= 0) {
				var username = $("#hardwarecontent #divlogin #username").val();
				var password = encodeURIComponent($("#hardwarecontent #divlogin #password").val());

				var Pollseconds = parseInt($("#hardwarecontent #divevohomeweb #updatefrequencyevohomeweb").val());
				if ( Pollseconds < 10 ) {
					Pollseconds = 60;
				}

				var UseFlags = 0;
				if ($("#hardwarecontent #divevohomeweb #showlocationevohomeweb").prop("checked"))
				{
					$("#hardwarecontent #divevohomeweb #disableautoevohomeweb").prop("checked", 1);
					UseFlags = UseFlags | 4;
				}

				if (!$("#hardwarecontent #divevohomeweb #disableautoevohomeweb").prop("checked")) // reverted value - default 0 is true
				{
					UseFlags = UseFlags | 1;
				}

				if ($("#hardwarecontent #divevohomeweb #showscheduleevohomeweb").prop("checked"))
				{
					UseFlags = UseFlags | 2;
				}

				var Precision = parseInt($("#hardwarecontent #divevohomeweb #comboevoprecision").val());
				UseFlags += Precision;

				var evo_installation = $("#hardwarecontent #divevohomeweb #comboevolocation").val()*4096;
				evo_installation = evo_installation + $("#hardwarecontent #divevohomeweb #comboevogateway").val()*256;
				evo_installation = evo_installation + $("#hardwarecontent #divevohomeweb #comboevotcs").val()*16;

				$.ajax({
					url: "json.htm?type=command&param=updatehardware&htype=" + hardwaretype +
					"&loglevel=" + logLevel +
					"&username=" + encodeURIComponent(username) +
					"&password=" + encodeURIComponent(password) +
					"&name=" + encodeURIComponent(name) +
					"&enabled=" + bEnabled +
					"&idx=" + idx +
					"&datatimeout=" + datatimeout +
					"&Mode1=" + Pollseconds +
					"&Mode2=" + UseFlags +
					"&Mode3=" + evo_installation,
					async: false,
					dataType: 'json',
					success: function (data) {
						RefreshHardwareTable();
					},
					error: function () {
						ShowNotify($.t('Problem updating hardware!'), 2500, true);
					}
				});
			} else if (text.indexOf("Rtl433 RTL-SDR receiver") >= 0) {
				$.ajax({
					url: "json.htm?type=command&param=updatehardware&htype=" + hardwaretype + "&name=" + encodeURIComponent(name) +
					"&loglevel=" + logLevel +
					"&enabled=" + bEnabled + "&datatimeout=" + datatimeout	+
					"&idx=" + idx +
					"&extra=" + encodeURIComponent($("#hardwarecontent #hardwareparamsrtl433 #rtl433cmdline").val()),
					async: false,
					dataType: 'json',
					success: function (data) {
						RefreshHardwareTable();
					},
					error: function () {
						ShowNotify($.t('Problem updating hardware!'), 2500, true);
					}
				});
		    }
			else if (text.indexOf("AirconWithMe") >= 0) 
			{
				var address = $("#hardwarecontent #divremote #tcpaddress").val();
				if (address == "") {
					ShowNotify($.t('Please enter an Address!'), 2500, true);
					return;
				}

				var username = $("#hardwarecontent #divlogin #username").val();
				var password = encodeURIComponent($("#hardwarecontent #divlogin #password").val());
				
                $.ajax({
					url: "json.htm?type=command&param=updatehardware&htype=" + hardwaretype +
					"&loglevel=" + logLevel +
					"&address=" + address +
					"&username=" + encodeURIComponent(username) +
					"&password=" + encodeURIComponent(password) +
					"&name=" + encodeURIComponent(name) +
					"&enabled=" + bEnabled +
					"&idx=" + idx +
					"&datatimeout=" + datatimeout,
					async: false,
					dataType: 'json',
					success: function (data) {
						RefreshHardwareTable();
					},
					error: function () {
						ShowNotify($.t('Problem updating hardware!'), 2500, true);
					}
				});
		    }
		}

		AddHardware = function () {
			var name = $("#hardwarecontent #hardwareparamstable #hardwarename").val();
			if (name == "") {
				ShowNotify($.t('Please enter a Name!'), 2500, true);
				return false;
			}

			var hardwaretype = $("#hardwarecontent #hardwareparamstable #combotype option:selected").val();
			if (typeof hardwaretype == 'undefined') {
				ShowNotify($.t('Unknown device selected!'), 2500, true);
				return;
			}

			var bEnabled = $('#hardwarecontent #hardwareparamstable #enabled').is(":checked");
			var datatimeout = $('#hardwarecontent #hardwareparamstable #combodatatimeout').val();

			var logLevel = 0;
			if ($("#hardwarecontent #hardwareparamstable #loglevelInfo").prop("checked"))
				logLevel |= 1;
			if ($("#hardwarecontent #hardwareparamstable #loglevelStatus").prop("checked"))
				logLevel |= 2;
			if ($("#hardwarecontent #hardwareparamstable #loglevelError").prop("checked"))
				logLevel |= 4;

			var text = $("#hardwarecontent #hardwareparamstable #combotype option:selected").text();

			// Handle plugins 1st because all the text indexof logic below will have unpredictable impacts for plugins
			if (!$.isNumeric(hardwaretype)) {
				var selector = "#hardwarecontent #divpythonplugin #" + hardwaretype;
				var bIsOK = true;
				// Make sure that all required fields have values
				$(selector + " .text").each(function () {
					if ((typeof (this.attributes.required) != "undefined") && (this.value == "")) {
						$(selector + " #" + this.id).focus();
						ShowNotify($.t('Please enter value for required field'), 2500, true);
						bIsOK = false;
					}
					return bIsOK;
				});
				if (bIsOK) {
					$.ajax({
						url: "json.htm?type=command&param=addhardware&htype=94" +
						"&loglevel=" + logLevel +
						"&name=" + encodeURIComponent(name) +
						"&username=" + encodeURIComponent(($(selector + " #Username").length == 0) ? "" : $(selector + " #Username").val()) +
						"&password=" + encodeURIComponent(($(selector + " #Password").length == 0) ? "" : $(selector + " #Password").val()) +
						"&address=" + encodeURIComponent(($(selector + " #Address").length == 0) ? "" : $(selector + " #Address").val()) +
						"&port=" + encodeURIComponent(($(selector + " #Port").length == 0) ? "" : $(selector + " #Port").val()) +
						"&serialport=" + encodeURIComponent(($(selector + " #SerialPort").length == 0) ? "" : $(selector + " #SerialPort").val()) +
						"&Mode1=" + encodeURIComponent(($(selector + " #Mode1").length == 0) ? "" : $(selector + " #Mode1").val()) +
						"&Mode2=" + encodeURIComponent(($(selector + " #Mode2").length == 0) ? "" : $(selector + " #Mode2").val()) +
						"&Mode3=" + encodeURIComponent(($(selector + " #Mode3").length == 0) ? "" : $(selector + " #Mode3").val()) +
						"&Mode4=" + encodeURIComponent(($(selector + " #Mode4").length == 0) ? "" : $(selector + " #Mode4").val()) +
						"&Mode5=" + encodeURIComponent(($(selector + " #Mode5").length == 0) ? "" : $(selector + " #Mode5").val()) +
						"&Mode6=" + encodeURIComponent(($(selector + " #Mode6").length == 0) ? "" : $(selector + " #Mode6").val()) +
						"&extra=" + encodeURIComponent(hardwaretype) +
						"&enabled=" + bEnabled +
						"&datatimeout=" + datatimeout,
						async: false,
						dataType: 'json',
						success: function (data) {
							ShowNotify($.t('Hardware created, devices can be found in the devices tab!'), 2500);
							RefreshHardwareTable();
						},
						error: function () {
							ShowNotify($.t('Problem adding hardware!'), 2500, true);
						}
					});
				}
				return;
			}
			if (text.indexOf("1-Wire") >= 0) {
				var owfspath = $("#hardwarecontent #div1wire #owfspath").val();
				var oneWireSensorPollPeriod = $("#hardwarecontent #div1wire #OneWireSensorPollPeriod").val();
				if (oneWireSensorPollPeriod == "") {
					ShowNotify($.t('Please enter a poll period for the sensors'), 2500, true);
					return;
				}
				var oneWireSwitchPollPeriod = $("#hardwarecontent #div1wire #OneWireSwitchPollPeriod").val();
				if (oneWireSwitchPollPeriod == "") {
					ShowNotify($.t('Please enter a poll period for the switches'), 2500, true);
					return;
				}
				var intRegex = /^\d+$/;
				if (!intRegex.test(oneWireSensorPollPeriod)) {
					ShowNotify($.t('Please enter a valid poll period for the sensors'), 2500, true);
					return;
				}

				if (!intRegex.test(oneWireSwitchPollPeriod)) {
					ShowNotify($.t('Please enter a valid poll period for the switches'), 2500, true);
					return;
				}

				$.ajax({
					url: "json.htm?type=command&param=addhardware&htype=" + hardwaretype + "&extra=" + encodeURIComponent(owfspath) +
					"&loglevel=" + logLevel +
					"&Mode1=" + oneWireSensorPollPeriod + "&Mode2=" + oneWireSwitchPollPeriod + "&name=" + encodeURIComponent(name) +
					"&enabled=" + bEnabled + "&datatimeout=" + datatimeout,
					async: false,
					dataType: 'json',
					success: function (data) {
						RefreshHardwareTable();
					},
					error: function () {
						ShowNotify($.t('Problem adding hardware!'), 2500, true);
					}
				});
			} else if (text.indexOf("Rtl433 RTL-SDR receiver") >= 0) {
				$.ajax({
					url: "json.htm?type=command&param=addhardware&htype=" + hardwaretype + "&name=" + encodeURIComponent(name) +
					"&loglevel=" + logLevel +
					"&enabled=" + bEnabled + "&datatimeout=" + datatimeout	+
					"&extra=" + encodeURIComponent($("#hardwarecontent #hardwareparamsrtl433 #rtl433cmdline").val()),
					async: false,
					dataType: 'json',
					success: function (data) {
						RefreshHardwareTable();
					},
					error: function () {
						ShowNotify($.t('Problem adding hardware!'), 2500, true);
					}
				});
		        }
			else if (
				(text.indexOf("LAN") >= 0 &&
					text.indexOf("YouLess") == -1 &&
					text.indexOf("Denkovi") == -1 &&
					text.indexOf("ETH8020") == -1 &&
					text.indexOf("Daikin") == -1 &&
					text.indexOf("Sterbox") == -1 &&
					text.indexOf("Anna") == -1 &&
					text.indexOf("KMTronic") == -1 &&
					text.indexOf("MQTT") == -1 &&
					text.indexOf("Relay-Net") == -1 &&
					text.indexOf("Satel Integra") == -1 &&
					text.indexOf("eHouse") == -1 &&
					text.indexOf("Razberry") == -1 &&
					text.indexOf("MyHome OpenWebNet with LAN interface") == -1
				)
			) {
				var password = "";
				var Mode1 = "";
				var Mode2 = "";
				var Mode3 = "";
			
				var address = $("#hardwarecontent #divremote #tcpaddress").val();
				if (address == "") {
					ShowNotify($.t('Please enter an Address!'), 2500, true);
					return;
				}
				var port = $("#hardwarecontent #divremote #tcpport").val();
				if (port == "") {
					ShowNotify($.t('Please enter an Port!'), 2500, true);
					return;
				}
				var intRegex = /^\d+$/;
				if (!intRegex.test(port)) {
					ShowNotify($.t('Please enter an Valid Port!'), 2500, true);
					return;
				}
				var extra = "";
				if (text.indexOf("Evohome") >= 0) {
					extra = $("#hardwarecontent #divevohometcp #controlleridevohometcp").val();
				}
				else if (text.indexOf("P1 Smart Meter") >= 0) {
					Mode2 = $("#hardwarecontent #divcrcp1 #disablecrcp1").prop("checked") ? 0 : 1;
					var ratelimitp1 = $("#hardwarecontent #hardwareparamsratelimitp1 #ratelimitp1").val();
					if (ratelimitp1 == "") {
						ratelimitp1 = "0";
					}
					Mode3 = ratelimitp1;
					var decryptionkey = $("#hardwarecontent #divkeyp1p1 #decryptionkey").val();
					if (decryptionkey.length % 2 != 0 ) {
						ShowNotify($.t("Invallid Descryption Key Length!"), 2500, true);
						return;
					}
					password = decryptionkey;
				}
				$.ajax({
					url: "json.htm?type=command&param=addhardware&htype=" + hardwaretype +
					"&loglevel=" + logLevel +
					"&address=" + address +
					"&port=" + port +
					"&name=" + encodeURIComponent(name) +
					"&enabled=" + bEnabled +
					"&password=" + encodeURIComponent(password) +
					"&datatimeout=" + datatimeout +
					"&Mode1=" + Mode1 +
					"&Mode2=" + Mode2 +
					"&Mode3=" + Mode3 +
					"&extra=" + extra,
					async: false,
					dataType: 'json',
					success: function (data) {
						if ((bEnabled)&&(text.indexOf("RFXCOM") >= 0)) {
							ShowNotify($.t('Please wait. Updating ....!'), 2500);
							setTimeout(hideAndRefreshHardwareTable, 3000)
						} else {
							RefreshHardwareTable();
						}
					},
					error: function () {
						ShowNotify($.t('Problem adding hardware!'), 2500, true);
					}
				});
			}
			else if (
				(text.indexOf("Panasonic") >= 0) ||
				(text.indexOf("BleBox") >= 0) ||
				(text.indexOf("TE923") >= 0) ||
				(text.indexOf("Volcraft") >= 0) ||
				(text.indexOf("Dummy") >= 0) ||
				(text.indexOf("System Alive") >= 0) ||
				(text.indexOf("Kodi Media") >= 0) ||
				(text.indexOf("PiFace") >= 0) ||
				(text.indexOf("Evohome") >= 0 && text.indexOf("script") >= 0) ||
				(text.indexOf("Tellstick") >= 0) ||
				(text.indexOf("Motherboard") >= 0) ||
				(text.indexOf("YeeLight") >= 0) ||
				(text.indexOf("Arilux AL-LC0x") >= 0)
			) {
				$.ajax({
					url: "json.htm?type=command&param=addhardware&htype=" + hardwaretype +
					"&loglevel=" + logLevel +
					"&name=" + encodeURIComponent(name) + "&enabled=" + bEnabled + "&datatimeout=" + datatimeout,
					async: false,
					dataType: 'json',
					success: function (data) {
						RefreshHardwareTable();
					},
					error: function () {
						ShowNotify($.t('Problem adding hardware!'), 2500, true);
					}
				});
			}
			else if ((text.indexOf("GPIO") >= 0) && (text.indexOf("sysfs GPIO") == -1)) {
				var gpiodebounce = $("#hardwarecontent #hardwareparamsgpio #gpiodebounce").val();
				var gpioperiod = $("#hardwarecontent #hardwareparamsgpio #gpioperiod").val();
				var gpiopollinterval = $("#hardwarecontent #hardwareparamsgpio #gpiopollinterval").val();
				if (gpiodebounce == "") {
					gpiodebounce = "50";
				}
				if (gpioperiod == "") {
					gpioperiod = "50";
				}
				if (gpiopollinterval == "") {
					gpiopollinterval = "0";
				}
				Mode1 = gpiodebounce;
				Mode2 = gpioperiod;
				Mode3 = gpiopollinterval;
				$.ajax({
					url: "json.htm?type=command&param=addhardware&htype=" + hardwaretype +
					"&loglevel=" + logLevel +
					"&name=" + encodeURIComponent(name) +
					"&enabled=" + bEnabled +
					"&datatimeout=" + datatimeout +
					"&Mode1=" + Mode1 + "&Mode2=" + Mode2 + "&Mode3=" + Mode3,
					async: false,
					dataType: 'json',
					success: function (data) {
						RefreshHardwareTable();
					},
					error: function () {
						ShowNotify($.t('Problem adding hardware!'), 2500, true);
					}
				});
			}
			else if (text.indexOf("sysfs GPIO") >= 0) {
				Mode1 = $('#hardwarecontent #hardwareparamssysfsgpio #sysfsautoconfigure').prop("checked") ? 1 : 0;
				Mode2 = $('#hardwarecontent #hardwareparamssysfsgpio #sysfsdebounce').val();
				$.ajax({
					url: "json.htm?type=command&param=addhardware&htype=" + hardwaretype +
					"&loglevel=" + logLevel +
					"&name=" + encodeURIComponent(name) +
					"&enabled=" + bEnabled +
					"&datatimeout=" + datatimeout +
					"&Mode1=" + Mode1 +
					"&Mode2=" + Mode2,
					async: false,
					dataType: 'json',
					success: function (data) {
					RefreshHardwareTable();
					},
					error: function () {
						ShowNotify($.t('Problem adding hardware!'), 2500, true);
					}
				});
			}
			else if (text.indexOf("I2C ") >= 0 ) {
				hardwaretype = $("#hardwareparamsi2clocal #comboi2clocal").find('option:selected').val();
				var i2cpath = $("#hardwareparamsi2clocal #i2cpath").val();
				var i2caddress = "";

				var text1 = $("#hardwareparamsi2clocal #comboi2clocal").find('option:selected').text();
				if (text1.indexOf("I2C sensor PIO 8bit expander PCF8574") >= 0) {
					var i2caddress = $("#hardwareparami2caddress #i2caddress").val();
					var i2cinvert = $("#hardwareparami2cinvert #i2cinvert").prop("checked") ? 1 : 0;

				}
				else if (text1.indexOf("I2C sensor GPIO 16bit expander MCP23017") >= 0) {
					var i2caddress = $("#hardwareparami2caddress #i2caddress").val();
					var i2cinvert = $("#hardwareparami2cinvert #i2cinvert").prop("checked") ? 1 : 0;
				}

				$.ajax({
					url: "json.htm?type=command&param=addhardware&htype=" + hardwaretype + "&name=" + encodeURIComponent(name) + "&enabled=" + bEnabled + "&datatimeout=" + datatimeout +
					"&loglevel=" + logLevel +
					"&address=" + encodeURIComponent(i2caddress) +
					"&port=" + encodeURIComponent(i2cpath) +
					"&Mode1=" + encodeURIComponent(i2cinvert),
					async: false,
					dataType: 'json',
					success: function (data) {
						RefreshHardwareTable();
					},
					error: function () {
						ShowNotify($.t('Problem adding I2C hardware!'), 2500, true);
					}
				});
			}
			else if (text.indexOf("USB") >= 0 || text.indexOf("Teleinfo EDF") >= 0) {
				var Mode1 = "0";
				var extra = "";
				var password = "";
				var serialport = $("#hardwarecontent #divserial #comboserialport option:selected").text();
				if (typeof serialport == 'undefined') {
					ShowNotify($.t('No serial port selected!'), 2500, true);
					return;
				}

				if (text.indexOf("Evohome") >= 0) {
					var baudrate = $("#hardwarecontent #divevohome #combobaudrateevohome option:selected").val();
					extra = $("#hardwarecontent #divevohome #controllerid").val();

					if (typeof baudrate == 'undefined') {
						ShowNotify($.t('No baud rate selected!'), 2500, true);
						return;
					}

					Mode1 = baudrate;
				}

				if (text.indexOf("MySensors") >= 0) {
					var baudrate = $("#hardwarecontent #divbaudratemysensors #combobaudrate option:selected").val();

					if (typeof baudrate == 'undefined') {
						ShowNotify($.t('No baud rate selected!'), 2500, true);
						return;
					}

					Mode1 = baudrate;
				}

				if (text.indexOf("P1 Smart Meter") >= 0) {
					var baudrate = $("#hardwarecontent #divbaudratep1 #combobaudratep1 option:selected").val();

					if (typeof baudrate == 'undefined') {
						ShowNotify($.t('No baud rate selected!'), 2500, true);
						return;
					}

					Mode1 = baudrate;
					Mode2 = $("#hardwarecontent #divcrcp1 #disablecrcp1").prop("checked") ? 0 : 1;
					var ratelimitp1 = $("#hardwarecontent #hardwareparamsratelimitp1 #ratelimitp1").val();
					if (ratelimitp1 == "") {
						ratelimitp1 = "0";
					}
					Mode3 = ratelimitp1;
					var decryptionkey = $("#hardwarecontent #divkeyp1p1 #decryptionkey").val();
					if (decryptionkey.length % 2 != 0 ) {
						ShowNotify($.t("Invallid Descryption Key Length!"), 2500, true);
						return;
					}
					password = decryptionkey;
				}
				if (text.indexOf("Teleinfo EDF") >= 0) {
					var baudrate = $("#hardwarecontent #divbaudrateteleinfo #combobaudrateteleinfo option:selected").val();

					if (typeof baudrate == 'undefined') {
						ShowNotify($.t('No baud rate selected!'), 2500, true);
						return;
					}

					Mode1 = baudrate;
					Mode2 = $("#hardwarecontent #divcrcp1 #disablecrcp1").prop("checked") ? 0 : 1;
					var ratelimitp1 = $("#hardwarecontent #hardwareparamsratelimitp1 #ratelimitp1").val();
					if (ratelimitp1 == "") {
						ratelimitp1 = "60";
					}
					Mode3 = ratelimitp1;
				}

				if (text.indexOf("USBtin") >= 0) {
					//Mode1 = $("#hardwarecontent #divusbtin #combotypecanusbtin option:selected").val();
					var ActivateMultiblocV8 = $("#hardwarecontent #divusbtin #activateMultiblocV8").prop("checked") ? 1 : 0;
					var ActivateCanFree = $("#hardwarecontent #divusbtin #activateCanFree").prop("checked") ? 1 : 0;
					var DebugActiv = $("#hardwarecontent #divusbtin #combodebugusbtin option:selected").val();
					Mode1 = (ActivateCanFree&0x01);
					Mode1 <<= 1;
					Mode1 += (ActivateMultiblocV8&0x01);
					Mode2 = DebugActiv;
				}

                if (text.indexOf("Denkovi") >= 0) {
                    Mode1 = $("#hardwarecontent #divmodeldenkoviusbdevices #combomodeldenkoviusbdevices option:selected").val();
                }

				$.ajax({
					url: "json.htm?type=command&param=addhardware&htype=" + hardwaretype +
					"&loglevel=" + logLevel +
					"&port=" + encodeURIComponent(serialport) +
					"&extra=" + extra +
					"&name=" + encodeURIComponent(name) +
					"&enabled=" + bEnabled +
					"&datatimeout=" + datatimeout +
					"&password=" + encodeURIComponent(password) +
					"&Mode1=" + Mode1,
					async: false,
					dataType: 'json',
					success: function (data) {
						if ((bEnabled)&&(text.indexOf("RFXCOM") >= 0)) {
							ShowNotify($.t('Please wait. Updating ....!'), 2500);
							setTimeout(hideAndRefreshHardwareTable, 3000)
						} else {
							RefreshHardwareTable();
						}
					},
					error: function () {
						ShowNotify($.t('Problem adding hardware!'), 2500, true);
					}
				});
			}
			else if (
				(text.indexOf("LAN") >= 0 &&
					text.indexOf("YouLess") == -1 &&
					text.indexOf("Denkovi") == -1 &&
					text.indexOf("ETH8020") == -1 &&
					text.indexOf("Daikin") == -1 &&
					text.indexOf("Sterbox") == -1 &&
					text.indexOf("Anna") == -1 &&
					text.indexOf("KMTronic") == -1 &&
					text.indexOf("MQTT") == -1 &&
					text.indexOf("Relay-Net") == -1 &&
					text.indexOf("Satel Integra") == -1 &&
					text.indexOf("eHouse") == -1 &&
					text.indexOf("Razberry") == -1 &&
					text.indexOf("MyHome OpenWebNet with LAN interface") == -1
				)
			) {
				var address = $("#hardwarecontent #divremote #tcpaddress").val();
				if (address == "") {
					ShowNotify($.t('Please enter an Address!'), 2500, true);
					return;
				}
				var port = $("#hardwarecontent #divremote #tcpport").val();
				if (port == "") {
					ShowNotify($.t('Please enter an Port!'), 2500, true);
					return;
				}
				var intRegex = /^\d+$/;
				if (!intRegex.test(port)) {
					ShowNotify($.t('Please enter an Valid Port!'), 2500, true);
					return;
				}
				var extra = "";
				if (text.indexOf("Evohome") >= 0) {
					extra = $("#hardwarecontent #divevohometcp #controlleridevohometcp").val();
				}
				$.ajax({
					url: "json.htm?type=command&param=addhardware&htype=" + hardwaretype +
					"&loglevel=" + logLevel +
					"&address=" + address +
					"&port=" + port +
					"&name=" + encodeURIComponent(name) +
					"&enabled=" + bEnabled + "&datatimeout=" + datatimeout + "&extra=" + extra,
					async: false,
					dataType: 'json',
					success: function (data) {
						RefreshHardwareTable();
					},
					error: function () {
						ShowNotify($.t('Problem adding hardware!'), 2500, true);
					}
				});
			}
			else if (
				(text.indexOf("LAN") >= 0 && ((text.indexOf("YouLess") >= 0) || (text.indexOf("Denkovi") >= 0))) ||
				(text.indexOf("Relay-Net") >= 0) || (text.indexOf("Satel Integra") >= 0) || (text.indexOf("eHouse") >= 0) || (text.indexOf("Harmony") >= 0) || (text.indexOf("Xiaomi Gateway") >= 0) ||
				(text.indexOf("MyHome OpenWebNet with LAN interface") >= 0)
			) {
				Mode1 = 0;
				Mode2 = 0;
				Mode3 = 0;
				Mode4 = 0;
				Mode5 = 0;

				var address = $("#hardwarecontent #divremote #tcpaddress").val();
				if (address == "") {
					ShowNotify($.t('Please enter an Address!'), 2500, true);
					return;
				}
				var port = $("#hardwarecontent #divremote #tcpport").val();
				if (port == "") {
					ShowNotify($.t('Please enter an Port!'), 2500, true);
					return;
				}
				var intRegex = /^\d+$/;
				if (!intRegex.test(port)) {
					ShowNotify($.t('Please enter an Valid Port!'), 2500, true);
					return;
				}
				if (text.indexOf("Satel Integra") >= 0) {
					var pollinterval = $("#hardwarecontent #hardwareparamspollinterval #pollinterval").val();
					if (pollinterval == "") {
						ShowNotify($.t('Please enter poll interval!'), 2500, true);
						return;
					}
					Mode1 = pollinterval;
				}
				else if (text.indexOf("Denkovi") >= 0) {
					var pollinterval = $("#hardwarecontent #hardwareparamspollinterval #pollinterval").val();
					if (pollinterval == "") {
						ShowNotify($.t('Please enter poll interval!'), 2500, true);
						return;
					}
					Mode1 = pollinterval;
					if (text.indexOf("Modules with LAN (HTTP)") >= 0)
						Mode2 = $("#hardwarecontent #divmodeldenkovidevices #combomodeldenkovidevices option:selected").val();
					else if (text.indexOf("Modules with LAN (TCP)") >= 0) {
						Mode2 = $("#hardwarecontent #divmodeldenkovitcpdevices #combomodeldenkovitcpdevices option:selected").val();
						Mode3 = $("#hardwarecontent #divmodeldenkovitcpdevices #denkovislaveid").val();
						if(Mode2 == "1"){
							var intRegex = /^\d+$/;
							if (isNaN(Mode3) || Number(Mode3) < 1 || Number(Mode3) > 247) {
								ShowNotify($.t('Invalid Slave ID! Enter value from 1 to 247!'), 2500, true);
								return;
							}
						} else
							Mode3 = "0";
					}
				}
				else if (text.indexOf("eHouse") >= 0) {
					var pollinterval = $("#hardwarecontent #hardwareparamspollinterval #pollinterval").val();
					Mode2 = $('#hardwarecontent #hardwareparamsehouse #ehouseautodiscovery').prop("checked") ? 1 : 0;
					Mode3 = $('#hardwarecontent #hardwareparamsehouse #ehouseaddalarmin').prop("checked") ? 1 : 0;
					Mode4 = $('#hardwarecontent #hardwareparamsehouse #ehouseprodiscovery').prop("checked") ? 1 : 0;
					Mode5 = $('#hardwarecontent #hardwareparamsehouse #ehouseopts').val();
					Mode6 = $('#hardwarecontent #hardwareparamsehouse #ehouseopts2').val();
					if (pollinterval == "") {
						ShowNotify($.t('Please enter poll interval!'), 2500, true);
						return;
						}
					Mode1 = pollinterval;
				}
				else if (text.indexOf("Relay-Net") >= 0) {
					Mode1 = $('#hardwarecontent #hardwareparamsrelaynet #relaynetpollinputs').prop("checked") ? 1 : 0;
					Mode2 = $('#hardwarecontent #hardwareparamsrelaynet #relaynetpollrelays').prop("checked") ? 1 : 0;
					var pollinterval = $("#hardwarecontent #hardwareparamsrelaynet #relaynetpollinterval").val();
					var inputcount = $("#hardwarecontent #hardwareparamsrelaynet #relaynetinputcount").val();
					var relaycount = $("#hardwarecontent #hardwareparamsrelaynet #relaynetrelaycount").val();

					if (pollinterval == "") {
						ShowNotify($.t('Please enter poll interval!'), 2500, true);
						return;
					}

					if (inputcount == "") {
						ShowNotify($.t('Please enter input count!'), 2500, true);
						return;
					}

					if (relaycount == "") {
						ShowNotify($.t('Please enter relay count!'), 2500, true);
						return;
					}

					Mode3 = pollinterval;
					Mode4 = inputcount;
					Mode5 = relaycount;
				}
				var password = encodeURIComponent($("#hardwarecontent #divlogin #password").val());
				if (text.indexOf("MyHome OpenWebNet with LAN interface") >= 0) {
                    if (password != "") {
                        if ((password.length < 5) || (password.length > 16)) {
                            ShowNotify($.t('Please enter a password between 5 and 16 characters!'), 2500, true);
                            return;
                        }

                        //var intRegex = /^[a-zA-Z0-9]*$/; 
                        //if (!intRegex.test(password)) {
                        //    ShowNotify($.t('Please enter a numeric or alphanumeric (for HMAC) password'), 2500, true);
                        //    return;
                        //}
                    }
                    var ratelimitp1 = $("#hardwarecontent #hardwareparamsratelimitp1 #ratelimitp1").val();
                    if ((ratelimitp1 == "") || (isNaN(ratelimitp1))) {
                        ShowNotify($.t('Please enter rate limit!'), 2500, true);
                        return;
                    }
                    Mode1 = ratelimitp1;

					var ensynchro = $("#hardwarecontent #hardwareparamsensynchro #ensynchro").val();
                    if ((ensynchro == "") || (isNaN(ensynchro))) {
                        ShowNotify($.t('Please enter time sinchronization!'), 2500, true);
                        return;
                    }
                    Mode2 = ensynchro;
				}
				$.ajax({
					url: "json.htm?type=command&param=addhardware&htype=" + hardwaretype +
					"&loglevel=" + logLevel +
					"&address=" + address +
					"&port=" + port +
					"&name=" + encodeURIComponent(name) +
					"&password=" + encodeURIComponent(password) +
					"&enabled=" + bEnabled + "&datatimeout=" +
					datatimeout +
					"&Mode1=" + Mode1 + "&Mode2=" + Mode2 + "&Mode3=" + Mode3 + "&Mode4=" + Mode4 + "&Mode5=" + Mode5,
					async: false,
					dataType: 'json',
					success: function (data) {
						RefreshHardwareTable();
					},
					error: function () {
						ShowNotify($.t('Problem adding hardware!'), 2500, true);
					}
				});
			}
			else if (text.indexOf("Philips Hue") >= 0) {
				var address = $("#hardwarecontent #divremote #tcpaddress").val();
				if (address == "") {
					ShowNotify($.t('Please enter an Address!'), 2500, true);
					return;
				}
				var port = $("#hardwarecontent #divremote #tcpport").val();
				if (port == "") {
					ShowNotify($.t('Please enter an Port!'), 2500, true);
					return;
				}
				var intRegex = /^\d+$/;
				if (!intRegex.test(port)) {
					ShowNotify($.t('Please enter an Valid Port!'), 2500, true);
					return;
				}
				var username = $("#hardwarecontent #hardwareparamsphilipshue #username").val();

				if (username == "") {
					ShowNotify($.t('Please enter a username!'), 2500, true);
					return;
				}
				$.ajax({
					url: "json.htm?type=command&param=addhardware&htype=" + hardwaretype +
					"&loglevel=" + logLevel +
					"&address=" + address +
					"&port=" + port +
					"&username=" + encodeURIComponent(username) +
					"&name=" + encodeURIComponent(name) + "&enabled=" + bEnabled + "&datatimeout=" + datatimeout,
					async: false,
					dataType: 'json',
					success: function (data) {
						RefreshHardwareTable();
					},
					error: function () {
						ShowNotify($.t('Problem adding hardware!'), 2500, true);
					}
				});
			}
			else if (
				(text.indexOf("Domoticz") >= 0) ||
				(text.indexOf("Eco Devices") >= 0) ||
				(text.indexOf("ETH8020") >= 0) ||
				(text.indexOf("Daikin") >= 0) ||
				(text.indexOf("Sterbox") >= 0) ||
				(text.indexOf("Anna") >= 0) ||
				(text.indexOf("KMTronic") >= 0) ||
				(text.indexOf("MQTT") >= 0) ||
				(text.indexOf("Logitech Media Server") >= 0) ||
				(text.indexOf("HEOS by DENON") >= 0) ||
				(text.indexOf("Razberry") >= 0)
			) {
				var address = $("#hardwarecontent #divremote #tcpaddress").val();
				if (address == "") {
					ShowNotify($.t('Please enter an Address!'), 2500, true);
					return;
				}
				var port = $("#hardwarecontent #divremote #tcpport").val();
				if (port == "") {
					ShowNotify($.t('Please enter an Port!'), 2500, true);
					return;
				}
				var intRegex = /^\d+$/;
				if (!intRegex.test(port)) {
					ShowNotify($.t('Please enter an Valid Port!'), 2500, true);
					return;
				}
				var username = $("#hardwarecontent #divlogin #username").val();
				var password = encodeURIComponent($("#hardwarecontent #divlogin #password").val());
				var extra = "";
				var Mode1 = "";
				var Mode2 = "";
				var Mode3 = "";
				if (text.indexOf("MySensors Gateway with MQTT") >= 0) {
					extra = $("#hardwarecontent #divmysensorsmqtt #filename").val();
					Mode1 = $("#hardwarecontent #divmysensorsmqtt #combotopicselect").val();
					Mode2 = $("#hardwarecontent #divmysensorsmqtt #combotlsversion").val();
					Mode3 = $("#hardwarecontent #divmysensorsmqtt #combopreventloop").val();
					if ($("#hardwarecontent #divmysensorsmqtt #filename").val().indexOf("#") >= 0) {
						ShowNotify($.t('CA Filename cannot contain a "#" symbol!'), 2500, true);
						return;
					}
					if ($("#hardwarecontent #divmysensorsmqtt #mqtttopicin").val().indexOf("#") >= 0) {
						ShowNotify($.t('Publish Prefix cannot contain a "#" symbol!'), 2500, true);
						return;
					}
					if ($("#hardwarecontent #divmysensorsmqtt #mqtttopicout").val().indexOf("#") >= 0) {
						ShowNotify($.t('Subscribe Prefix cannot contain a "#" symbol!'), 2500, true);
						return;
					}
					if ((Mode1 == 2) && (($("#hardwarecontent #divmysensorsmqtt #mqtttopicin").val() == "") || ($("#hardwarecontent #divmysensorsmqtt #mqtttopicout").val() == ""))) {
						ShowNotify($.t('Please enter Topic Prefixes!'), 2500, true);
						return;
					}
					if (Mode1 == 2) {
						if (($("#hardwarecontent #divmysensorsmqtt #mqtttopicin").val() == "") ||
						    ($("#hardwarecontent #divmysensorsmqtt #mqtttopicout").val() == "")
						) {
							ShowNotify($.t('Please enter Topic Prefixes!'), 2500, true);
							return;
						}
						extra += "#";
						extra += $("#hardwarecontent #divmysensorsmqtt #mqtttopicin").val();
						extra += "#";
						extra += $("#hardwarecontent #divmysensorsmqtt #mqtttopicout").val();
					}
					extra = encodeURIComponent(extra);
				}
				else if (text.indexOf("MQTT") >= 0) {
					extra = encodeURIComponent($("#hardwarecontent #divmqtt #filename").val());
					var mqtttopicin = $("#hardwarecontent #divmqtt #mqtttopicin").val().trim();
					var mqtttopicout = $("#hardwarecontent #divmqtt #mqtttopicout").val().trim();
					if (mqtttopicin.indexOf("#") >= 0) {
						ShowNotify($.t('Publish Prefix cannot contain a "#" symbol!'), 2500, true);
						return;
					}
					if (mqtttopicout.indexOf("#") >= 0) {
						ShowNotify($.t('Subscribe Prefix cannot contain a "#" symbol!'), 2500, true);
						return;
					}
					if ((mqtttopicin!="")||(mqtttopicout!="")) {
						extra += ";" + mqtttopicin + ";" + mqtttopicout;
					}
					Mode1 = $("#hardwarecontent #divmqtt #combotopicselect").val();
					Mode2 = $("#hardwarecontent #divmqtt #combotlsversion").val();
					Mode3 = $("#hardwarecontent #divmqtt #combopreventloop").val();
				}
				if (text.indexOf("Eco Devices") >= 0) {
					Mode1 = $("#hardwarecontent #divmodelecodevices #combomodelecodevices option:selected").val();
					var ratelimitp1 = $("#hardwarecontent #hardwareparamsratelimitp1 #ratelimitp1").val();
					if (ratelimitp1 == "") {
						ratelimitp1 = "60";
					}
					Mode2 = ratelimitp1;
				}
				$.ajax({
					url: "json.htm?type=command&param=addhardware&htype=" + hardwaretype +
						"&loglevel=" + logLevel +
						"&address=" + address + "&port=" + port +
						"&username=" + encodeURIComponent(username) + "&password=" + encodeURIComponent(password) +
						"&name=" + encodeURIComponent(name) +
						"&enabled=" + bEnabled +
						"&datatimeout=" + datatimeout +
						"&extra=" + encodeURIComponent(extra) +
						"&Mode1=" + Mode1 + "&Mode2=" + Mode2 + "&Mode3=" + Mode3,
					async: false,
					dataType: 'json',
					success: function (data) {
						RefreshHardwareTable();
					},
					error: function () {
						ShowNotify($.t('Problem adding hardware!'), 2500, true);
					}
				});
			}
			else if (
					(text.indexOf("Underground") >= 0) ||
					(text.indexOf("DarkSky") >= 0) ||
					(text.indexOf("AccuWeather") >= 0)
					) {
				var apikey = $("#hardwarecontent #divunderground #apikey").val();
				if (apikey == "") {
					ShowNotify($.t('Please enter an API Key!'), 2500, true);
					return;
				}
				var location = $("#hardwarecontent #divunderground #location").val();
				if (location == "") {
					ShowNotify($.t('Please enter an Location!'), 2500, true);
					return;
				}
				$.ajax({
					url: "json.htm?type=command&param=addhardware&htype=" + hardwaretype +
					"&loglevel=" + logLevel +
					"&username=" + encodeURIComponent(apikey) + "&password=" + encodeURIComponent(location) +
					"&name=" + encodeURIComponent(name) + "&enabled=" + bEnabled + "&datatimeout=" + datatimeout,
					async: false,
					dataType: 'json',
					success: function (data) {
						RefreshHardwareTable();
					},
					error: function () {
						ShowNotify($.t('Problem adding hardware!'), 2500, true);
					}
				});
			}
			else if(text.indexOf("Meteorologisk") >= 0){
				var location = $("#hardwarecontent #divlocation #location").val();
				$.ajax({
					url: "json.htm?type=command&param=addhardware&htype=" + hardwaretype +
					"&loglevel=" + logLevel +
					"&password=" + encodeURIComponent(location) +
					"&name=" + encodeURIComponent(name) + "&enabled=" + bEnabled + "&datatimeout=" + datatimeout,
					async: false,
					dataType: 'json',
					success: function (data) {
						RefreshHardwareTable();
					},
					error: function () {
						ShowNotify($.t('Problem adding hardware!'), 2500, true);
					}
				});
			}
			else if (text.indexOf("Open Weather Map") >= 0) {
			var apikey = $("#hardwarecontent #divopenweathermap #apikey").val();
			if (apikey == "") {
				ShowNotify($.t('Please enter an API Key!'), 2500, true);
				return;
			}
			var location = $("#hardwarecontent #divopenweathermap #location").val();
			if (location == "") {
				ShowNotify($.t('Please enter an Location (or 0 to use Domoticz own location)!'), 2500, true);
				return;
			}
			var adddayforecast = $("#hardwarecontent #divopenweathermap #adddayforecast").prop("checked") ? 1 : 0;
			var addhourforecast = $("#hardwarecontent #divopenweathermap #addhourforecast").prop("checked") ? 1 : 0;
			$.ajax({
				url: "json.htm?type=command&param=addhardware&htype=" + hardwaretype + 
				"&loglevel=" + logLevel +
				"&username=" + encodeURIComponent(apikey) + "&password=" + encodeURIComponent(location) + 
				"&name=" + encodeURIComponent(name) + "&enabled=" + bEnabled + "&datatimeout=" + datatimeout +
				"&Mode1=" + adddayforecast + "&Mode2=" + addhourforecast,
				async: false,
				dataType: 'json',
				success: function (data) {
					RefreshHardwareTable();
				},
				error: function () {
					ShowNotify($.t('Problem adding hardware!'), 2500, true);
				}
			});
		}
		else if (text.indexOf("Buienradar") >= 0) {
				var timeframe = $("#hardwarecontent #divbuienradar #timeframe").val();
				if (timeframe == 0) {
					timeframe = 30;
				}
				var threshold = $("#hardwarecontent #divbuienradar #threshold").val();
				if (threshold == 0) {
					threshold = 25;
				}
				var location = $("#hardwarecontent #divbuienradar #location").val();
				$.ajax({
					url: "json.htm?type=command&param=addhardware&htype=" + hardwaretype +
					"&loglevel=" + logLevel +
					"&username=" + encodeURIComponent(apikey) + "&password=" + encodeURIComponent(location) +
					"&name=" + encodeURIComponent(name) + "&enabled=" + bEnabled + "&datatimeout=" + datatimeout +
					"&Mode1=" + timeframe + "&Mode2=" + threshold,
					async: false,
					dataType: 'json',
					success: function (data) {
						RefreshHardwareTable();
					},
					error: function () {
						ShowNotify($.t('Problem adding hardware!'), 2500, true);
					}
				});
			}
			else if ((text.indexOf("HTTP/HTTPS") >= 0)) {
				var url = $("#hardwarecontent #divhttppoller #url").val();
				if (url == "") {
					ShowNotify($.t('Please enter an url!'), 2500, true);
					return;
				}
				var script = $("#hardwarecontent #divhttppoller #script").val();
				if (script == "") {
					ShowNotify($.t('Please enter a script!'), 2500, true);
					return;
				}
				var refresh = $("#hardwarecontent #divhttppoller #refresh").val();
				if (refresh == "") {
					ShowNotify($.t('Please enter a refresh rate!'), 2500, true);
					return;
				}
				var username = $("#hardwarecontent #divlogin #username").val();
				var password = $("#hardwarecontent #divlogin #password").val();
				var method = $("#hardwarecontent #divhttppoller #combomethod option:selected").val();
				if (typeof method == 'undefined') {
					ShowNotify($.t('No HTTP method selected!'), 2500, true);
					return;
				}
				var contenttype = $("#hardwarecontent #divhttppoller #contenttype").val();
				var headers = $("#hardwarecontent #divhttppoller #headers").val();
				var postdata = $("#hardwarecontent #divhttppoller #postdata").val();

				var extra = btoa(script) + "|" + btoa(method) + "|" + btoa(contenttype) + "|" + btoa(headers);
				if (method == "1") {
					extra = extra + "|" + btoa(postdata);
				}
				$.ajax({
					url: "json.htm?type=command&param=addhardware&htype=" + hardwaretype +
					"&loglevel=" + logLevel +
					"&port=" + refresh +
					"&username=" + encodeURIComponent(username) + "&password=" + encodeURIComponent(password) +
					"&name=" + encodeURIComponent(name) + "&address=" + encodeURIComponent(url) + "&extra=" + extra + "&enabled=" + bEnabled + "&datatimeout=" + datatimeout,
					async: false,
					dataType: 'json',
					success: function (data) {
						RefreshHardwareTable();
					},
					error: function () {
						ShowNotify($.t('Problem adding hardware!'), 2500, true);
					}
				});
			}
			else if (text.indexOf("SBFSpot") >= 0) {
				var configlocation = $("#hardwarecontent #divlocation #location").val();
				if (configlocation == "") {
					ShowNotify($.t('Please enter an Location!'), 2500, true);
					return;
				}
				$.ajax({
					url: "json.htm?type=command&param=addhardware&htype=" + hardwaretype +
					"&loglevel=" + logLevel +
					"&username=" + encodeURIComponent(configlocation) +
					"&name=" + encodeURIComponent(name) +
					"&enabled=" + bEnabled +
					"&datatimeout=" + datatimeout,
					async: false,
					dataType: 'json',
					success: function (data) {
						RefreshHardwareTable();
					},
					error: function () {
						ShowNotify($.t('Problem updating hardware!'), 2500, true);
					}
				});
			}
			else if (text.indexOf("Winddelen") >= 0) {
				var mill_id = $("#hardwarecontent #divwinddelen #combomillselect").val();
				var mill_name = $("#hardwarecontent #divwinddelen #combomillselect").find("option:selected").text()
				var nrofwinddelen = $("#hardwarecontent #divwinddelen #nrofwinddelen").val();
				var intRegex = /^\d+$/;
				if (!intRegex.test(nrofwinddelen)) {
					ShowNotify($.t('Please enter an Valid Number!'), 2500, true);
					return;
				}
				$.ajax({
					url: "json.htm?type=command&param=addhardware&htype=" + hardwaretype +
					"&loglevel=" + logLevel +
					"&name=" + encodeURIComponent(name) +
					"&address=" + encodeURIComponent(mill_name) +
					"&port=" + encodeURIComponent(nrofwinddelen) +
					"&Mode1=" + encodeURIComponent(mill_id) +
					"&enabled=" + bEnabled +
					"&datatimeout=" + datatimeout,
					async: false,
					dataType: 'json',
					success: function (data) {
						RefreshHardwareTable();
					},
					error: function () {
						ShowNotify($.t('Problem adding hardware!'), 2500, true);
					}
				});
			}
			else if (text.indexOf("Honeywell") >= 0) {
				var apiKey = $("#hardwarecontent #divhoneywell #hwApiKey").val();
				var apiSecret = $("#hardwarecontent #divhoneywell #hwApiSecret").val();
				var accessToken = $("#hardwarecontent #divhoneywell #hwAccessToken").val();
				var refreshToken = $("#hardwarecontent #divhoneywell #hwRefreshToken").val();
				var extra = btoa(apiKey) + "|" + btoa(apiSecret);

				$.ajax({
					url: "json.htm?type=command&param=addhardware&htype=" + hardwaretype +
					"&loglevel=" + logLevel +
					"&name=" + encodeURIComponent(name) +
					"&username=" + encodeURIComponent(accessToken) +
					"&password=" + encodeURIComponent(refreshToken) +
					"&enabled=" + bEnabled +
					"&extra=" + extra +
					"&datatimeout=" + datatimeout,
					async: false,
					dataType: 'json',
					success: function (data) {
						RefreshHardwareTable();
					},
					error: function () {
						ShowNotify($.t('Problem adding hardware!'), 2500, true);
					}
				});
			}
			else if (text.indexOf("SolarEdge via Web") >= 0) {
				var apikey = $("#hardwarecontent #divsolaredgeapi #apikey").val();
				if (apikey == "") {
					ShowNotify($.t('Please enter an API Key!'), 2500, true);
					return;
				}
				$.ajax({
					url: "json.htm?type=command&param=addhardware&htype=" + hardwaretype +
					"&loglevel=" + logLevel +
					"&name=" + encodeURIComponent(name) +
					"&username=" + encodeURIComponent(apikey) +
					"&enabled=" + bEnabled +
					"&idx=" + idx +
					"&datatimeout=" + datatimeout,
					async: false,
					dataType: 'json',
					success: function (data) {
						RefreshHardwareTable();
					},
					error: function () {
						ShowNotify($.t('Problem adding hardware!'), 2500, true);
					}
				});
			}
			else if (text.indexOf("Nest Th") >= 0 && text.indexOf("OAuth") >= 0) {
			    var apikey = $("#hardwarecontent #divnestoauthapi #apikey").val();
			    var productid = $("#hardwarecontent #divnestoauthapi #productid").val();
			    var productsecret = $("#hardwarecontent #divnestoauthapi #productsecret").val();
			    var productpin = $("#hardwarecontent #divnestoauthapi #productpin").val();

			    if (apikey == "" && (productid == "" || productsecret == "" || productpin == "")) {
			        ShowNotify($.t('Please enter an API Key or a combination of Product Id, Product Secret and PIN!'), 2500, true);
			        return;
			    }

			    var extra = btoa(productid) + "|" + btoa(productsecret) + "|" + btoa(productpin);
			    console.log("Updating extra2: " + extra);

			    $.ajax({
			        url: "json.htm?type=command&param=addhardware&htype=" + hardwaretype +
						"&loglevel=" + logLevel +
						"&name=" + encodeURIComponent(name) +
						"&username=" + encodeURIComponent(apikey) +
						"&enabled=" + bEnabled +
						"&extra=" + extra +
						"&datatimeout=" + datatimeout,
			        async: false,
			        dataType: 'json',
			        success: function (data) {
			            RefreshHardwareTable();
			        },
			        error: function () {
			            ShowNotify($.t('Problem adding hardware!'), 2500, true);
			        }
			    });
			}
			else if (text.indexOf("Toon") >= 0) {
				var username = $("#hardwarecontent #divlogin #username").val();
				var password = encodeURIComponent($("#hardwarecontent #divlogin #password").val());
				var agreement = encodeURIComponent($("#hardwarecontent #divenecotoon #agreement").val());
				$.ajax({
					url: "json.htm?type=command&param=addhardware&htype=" + hardwaretype +
					"&loglevel=" + logLevel +
					"&username=" + encodeURIComponent(username) +
					"&password=" + encodeURIComponent(password) +
					"&name=" + encodeURIComponent(name) +
					"&enabled=" + bEnabled +
					"&datatimeout=" + datatimeout +
					"&Mode1=" + agreement,
					async: false,
					dataType: 'json',
					success: function (data) {
						RefreshHardwareTable();
					},
					error: function () {
						ShowNotify($.t('Problem adding hardware!'), 2500, true);
					}
				});
			}
			else if (text.indexOf("Tesla") >= 0) {
				var username = $("#hardwarecontent #divlogin #username").val();
				var password = encodeURIComponent($("#hardwarecontent #divlogin #password").val());
				var vinnr = encodeURIComponent($("#hardwarecontent #divtesla #vinnr").val());
				var activeinterval = parseInt($("#hardwarecontent #divtesla #activeinterval").val());
				if (activeinterval < 1) {
					activeinterval = 1;
				}
				var defaultinterval = parseInt($("#hardwarecontent #divtesla #defaultinterval").val());
				if (defaultinterval < 1) {
					defaultinterval = 20;
				}
				var allowwakeup = $("#hardwarecontent #divtesla #comboallowwakeup").val();
				$.ajax({
					url: "json.htm?type=command&param=addhardware&htype=" + hardwaretype +
					"&loglevel=" + logLevel +
					"&username=" + encodeURIComponent(username) +
					"&password=" + encodeURIComponent(password) +
					"&name=" + encodeURIComponent(name) +
					"&enabled=" + bEnabled +
					"&datatimeout=" + datatimeout +
					"&extra=" + vinnr +
					"&Mode1=" + defaultinterval +
					"&Mode2=" + activeinterval +
					"&Mode3=" + allowwakeup,
					async: false,
					dataType: 'json',
					success: function (data) {
						RefreshHardwareTable();
					},
					error: function () {
						ShowNotify($.t('Problem adding hardware!'), 2500, true);
					}
				});
			}
			else if (text.indexOf("Mercedes") >= 0) {
				var username = $("#hardwarecontent #divlogin #username").val();
				var password = encodeURIComponent($("#hardwarecontent #divlogin #password").val());
				var vinnr = encodeURIComponent($("#hardwarecontent #divmercedes #vinnr").val());
				var activeinterval = parseInt($("#hardwarecontent #divmercedes #activeinterval").val());
				if (activeinterval < 1) {
					activeinterval = 1;
				}
				var defaultinterval = parseInt($("#hardwarecontent #divmercedes #defaultinterval").val());
				if (defaultinterval < 1) {
					defaultinterval = 20;
				}
				var allowwakeup = $("#hardwarecontent #divmercedes #comboallowwakeup").val();
				$.ajax({
					url: "json.htm?type=command&param=addhardware&htype=" + hardwaretype +
					"&loglevel=" + logLevel +
					"&username=" + encodeURIComponent(username) +
					"&password=" + encodeURIComponent(password) +
					"&name=" + encodeURIComponent(name) +
					"&enabled=" + bEnabled +
					"&datatimeout=" + datatimeout +
					"&extra=" + vinnr +
					"&Mode1=" + defaultinterval +
					"&Mode2=" + activeinterval +
					"&Mode3=" + allowwakeup,
					async: false,
					dataType: 'json',
					success: function (data) {
						RefreshHardwareTable();
					},
					error: function () {
						ShowNotify($.t('Problem adding hardware!'), 2500, true);
					}
				});
			}
			else if (
				(text.indexOf("ICY") >= 0) ||
				(text.indexOf("Atag") >= 0) ||
				(text.indexOf("Nest Th") >= 0 && text.indexOf("OAuth") === -1) ||
				(text.indexOf("PVOutput") >= 0) ||
				(text.indexOf("Netatmo") >= 0) ||
				(text.indexOf("Thermosmart") >= 0) ||
                (text.indexOf("Tado") >= 0) ||
				(text.indexOf("HTTP") >= 0)
			) {
				var username = $("#hardwarecontent #divlogin #username").val();
				var password = encodeURIComponent($("#hardwarecontent #divlogin #password").val());
				$.ajax({
					url: "json.htm?type=command&param=addhardware&htype=" + hardwaretype +
					"&loglevel=" + logLevel +
					"&username=" + encodeURIComponent(username) +
					"&password=" + encodeURIComponent(password) +
					"&name=" + encodeURIComponent(name) +
					"&enabled=" + bEnabled +
					"&datatimeout=" + datatimeout,
					async: false,
					dataType: 'json',
					success: function (data) {
						RefreshHardwareTable();
					},
					error: function () {
						ShowNotify($.t('Problem adding hardware!'), 2500, true);
					}
				});
			}
			else if (text.indexOf("Goodwe solar inverter via Web") >= 0) {
				var username = $("#hardwarecontent #divgoodweweb #username").val();

				if (username == "") {
					ShowNotify($.t('Please enter your Goodwe username!'), 2500, true);
					return;
				}
				Mode1 = $("#hardwarecontent #divgoodweweb #comboserverselect").val();

				$.ajax({
					url: "json.htm?type=command&param=addhardware&htype=" + hardwaretype +
					"&loglevel=" + logLevel +
					"&username=" + encodeURIComponent(username) +
					"&name=" + encodeURIComponent(name) + "&enabled=" + bEnabled + "&datatimeout=" + datatimeout +
					"&Mode1=" + Mode1,
					async: false,
					dataType: 'json',
					success: function (data) {
						RefreshHardwareTable();
					},
					error: function () {
						ShowNotify($.t('Problem adding hardware!'), 2500, true);
					}
				});
			}
			else if (text.indexOf("Evohome via Web") >= 0) {
				var username = $("#hardwarecontent #divlogin #username").val();
				var password = encodeURIComponent($("#hardwarecontent #divlogin #password").val());
				var Pollseconds = parseInt($("#hardwarecontent #divevohomeweb #updatefrequencyevohomeweb").val());
				if ( Pollseconds < 10 ) {
					Pollseconds = 60;
				}

				var UseFlags = 0;
				if ($("#hardwarecontent #divevohomeweb #showlocationevohomeweb").prop("checked"))
				{
					$("#hardwarecontent #divevohomeweb #disableautoevohomeweb").prop("checked", 1);
					UseFlags = UseFlags | 4;
				}

				if (!$("#hardwarecontent #divevohomeweb #disableautoevohomeweb").prop("checked")) // reverted value - default 0 is true
				{
					UseFlags = UseFlags | 1;
				}

				if ($("#hardwarecontent #divevohomeweb #showscheduleevohomeweb").prop("checked"))
				{
					UseFlags = UseFlags | 2;
				}

				var Precision = parseInt($("#hardwarecontent #divevohomeweb #comboevoprecision").val());
				UseFlags += Precision;

				var evo_installation = $("#hardwarecontent #divevohomeweb #comboevolocation").val()*4096;
				evo_installation = evo_installation + $("#hardwarecontent #divevohomeweb #comboevogateway").val()*256;
				evo_installation = evo_installation + $("#hardwarecontent #divevohomeweb #comboevotcs").val()*16;

				$.ajax({
					url: "json.htm?type=command&param=addhardware&htype=" + hardwaretype +
					"&loglevel=" + logLevel +
					"&username=" + encodeURIComponent(username) +
					"&password=" + encodeURIComponent(password) +
					"&name=" + encodeURIComponent(name) +
					"&enabled=" + bEnabled +
					"&idx=" + idx +
					"&datatimeout=" + datatimeout +
					"&Mode1=" + Pollseconds +
					"&Mode2=" + UseFlags +
					"&Mode3=" + evo_installation,
					async: false,
					dataType: 'json',
					success: function (data) {
						RefreshHardwareTable();
					},
					error: function () {
						ShowNotify($.t('Problem adding hardware!'), 2500, true);
					}
				});
			}
			else if (text.indexOf("AirconWithMe") >= 0) {
				var address = $("#hardwarecontent #divremote #tcpaddress").val();
				if (address == "") 
				{
					ShowNotify($.t('Please enter an Address!'), 2500, true);
					return;
				}

				var username = $("#hardwarecontent #divlogin #username").val();
				var password = encodeURIComponent($("#hardwarecontent #divlogin #password").val());

				$.ajax({
					url: "json.htm?type=command&param=addhardware&htype=" + hardwaretype +
					"&loglevel=" + logLevel +
					"&address=" + address +
					"&username=" + encodeURIComponent(username) +
					"&password=" + encodeURIComponent(password) +
					"&name=" + encodeURIComponent(name) +
					"&enabled=" + bEnabled +
					"&idx=" + idx +
					"&datatimeout=" + datatimeout,
					async: false,
					dataType: 'json',
					success: function (data) {
						RefreshHardwareTable();
					},
					error: function () {
						ShowNotify($.t('Problem updating hardware!'), 2500, true);
					}
				});
			}
		}

		EditRFXCOMMode = function (idx, name, Mode1, Mode2, Mode3, Mode4, Mode5, Mode6, Extra, version) {
			cursordefault();
			var htmlcontent = '';
			htmlcontent = '<p><center><h2><span data-i18n="Device"></span>: ' + name + '</h2></center></p>\n';
			htmlcontent += $('#rfxhardwaremode').html();
			$('#hardwarecontent').html(GetBackbuttonHTMLTable('ShowHardware') + htmlcontent);
			$('#hardwarecontent').i18n();

			$('#hardwarecontent #submitbutton').click(function (e) {
				e.preventDefault();
				SetRFXCOMMode();
			});
			$('#hardwarecontent #firmwarebutton').click(function (e) {
				e.preventDefault();
				$rootScope.hwidx = $('#hardwarecontent #idx').val();
				SwitchLayout('RFXComFirmware');
			});

			$('#hardwarecontent #idx').val(idx);
			$('#hardwarecontent #Keeloq').prop('checked', ((Mode6 & 0x01) != 0));
			$('#hardwarecontent #HC').prop('checked', ((Mode6 & 0x02) != 0));
			$('#hardwarecontent #undecon').prop('checked', ((Mode3 & 0x80) != 0));
			$('#hardwarecontent #X10').prop('checked', ((Mode5 & 0x01) != 0));
			$('#hardwarecontent #ARC').prop('checked', ((Mode5 & 0x02) != 0));
			$('#hardwarecontent #AC').prop('checked', ((Mode5 & 0x04) != 0));
			$('#hardwarecontent #HomeEasyEU').prop('checked', ((Mode5 & 0x08) != 0));
			$('#hardwarecontent #Meiantech').prop('checked', ((Mode5 & 0x10) != 0));
			$('#hardwarecontent #OregonScientific').prop('checked', ((Mode5 & 0x20) != 0));
			$('#hardwarecontent #ATIremote').prop('checked', ((Mode5 & 0x40) != 0));
			$('#hardwarecontent #Visonic').prop('checked', ((Mode5 & 0x80) != 0));
			$('#hardwarecontent #Mertik').prop('checked', ((Mode4 & 0x01) != 0));
			$('#hardwarecontent #ADLightwaveRF').prop('checked', ((Mode4 & 0x02) != 0));
			$('#hardwarecontent #HidekiUPM').prop('checked', ((Mode4 & 0x04) != 0));
			$('#hardwarecontent #LaCrosse').prop('checked', ((Mode4 & 0x08) != 0));
			$('#hardwarecontent #Legrand').prop('checked', ((Mode4 & 0x10) != 0));
			$('#hardwarecontent #ProGuard').prop('checked', ((Mode4 & 0x20) != 0));
			$('#hardwarecontent #BlindT0').prop('checked', ((Mode4 & 0x40) != 0));
			$('#hardwarecontent #BlindT1T2T3T4').prop('checked', ((Mode4 & 0x80) != 0));
			$('#hardwarecontent #AEBlyss').prop('checked', ((Mode3 & 0x01) != 0));
			$('#hardwarecontent #Rubicson').prop('checked', ((Mode3 & 0x02) != 0));
			$('#hardwarecontent #FineOffsetViking').prop('checked', ((Mode3 & 0x04) != 0));
			$('#hardwarecontent #Lighting4').prop('checked', ((Mode3 & 0x08) != 0));
			$('#hardwarecontent #RSL').prop('checked', ((Mode3 & 0x10) != 0));
			$('#hardwarecontent #ByronSX').prop('checked', ((Mode3 & 0x20) != 0));
			$('#hardwarecontent #ImaginTronix').prop('checked', ((Mode3 & 0x40) != 0));

			$('#hardwarecontent #comborego6xxtype').val(Mode1);

			var ASyncType = 0;
			if (version.indexOf("Pro XL")==0) {
				var ASyncType = (Extra=="")?0:parseInt(Extra);
				$('#hardwarecontent #rfx_xl_settings').show();
			} else {
				$('#hardwarecontent #rfx_xl_settings').hide();
			}
			$('#hardwarecontent #combo_rfx_xl_async_type').val(ASyncType);

			$('#hardwarecontent #defaultbutton').click(function (e) {
				e.preventDefault();
				$('#hardwarecontent #combo_rfx_xl_async_type').val(0);
				$('#hardwarecontent #Keeloq').prop('checked', false);
				$('#hardwarecontent #HC').prop('checked', false);
				$('#hardwarecontent #undecon').prop('checked', false);
				$('#hardwarecontent #X10').prop('checked', true);
				$('#hardwarecontent #ARC').prop('checked', true);
				$('#hardwarecontent #AC').prop('checked', true);
				$('#hardwarecontent #HomeEasyEU').prop('checked', true);
				$('#hardwarecontent #Meiantech').prop('checked', false);
				$('#hardwarecontent #OregonScientific').prop('checked', true);
				$('#hardwarecontent #ATIremote').prop('checked', false);
				$('#hardwarecontent #Visonic').prop('checked', false);
				$('#hardwarecontent #Mertik').prop('checked', false);
				$('#hardwarecontent #ADLightwaveRF').prop('checked', false);
				$('#hardwarecontent #HidekiUPM').prop('checked', true);
				$('#hardwarecontent #LaCrosse').prop('checked', true);
				$('#hardwarecontent #Legrand').prop('checked', false);
				$('#hardwarecontent #ProGuard').prop('checked', false);
				$('#hardwarecontent #BlindT0').prop('checked', false);
				$('#hardwarecontent #BlindT1T2T3T4').prop('checked', false);
				$('#hardwarecontent #AEBlyss').prop('checked', false);
				$('#hardwarecontent #Rubicson').prop('checked', false);
				$('#hardwarecontent #FineOffsetViking').prop('checked', false);
				$('#hardwarecontent #Lighting4').prop('checked', false);
				$('#hardwarecontent #RSL').prop('checked', false);
				$('#hardwarecontent #ByronSX').prop('checked', false);
				$('#hardwarecontent #ImaginTronix').prop('checked', false);
			});
		}

		EditRFXCOMMode868 = function (idx, name, Mode1, Mode2, Mode3, Mode4, Mode5, Mode6) {
			cursordefault();
			var htmlcontent = '';
			htmlcontent = '<p><center><h2><span data-i18n="Device"></span>: ' + name + '</h2></center></p>\n';
			htmlcontent += $('#rfx868hardwaremode').html();
			$('#hardwarecontent').html(GetBackbuttonHTMLTable('ShowHardware') + htmlcontent);
			$('#hardwarecontent').i18n();

			$('#hardwarecontent #submitbutton').click(function (e) {
				e.preventDefault();
				SetRFXCOMMode868();
			});
			$('#hardwarecontent #firmwarebutton').click(function (e) {
				e.preventDefault();
				$rootScope.hwidx = $('#hardwarecontent #idx').val();
				SwitchLayout('RFXComFirmware');
			});

			$('#hardwarecontent #idx').val(idx);

			$('#hardwarecontent #defaultbutton').click(function (e) {
				e.preventDefault();
				$('#hardwarecontent #LaCrosse').prop('checked', false);
				$('#hardwarecontent #Alecto').prop('checked', false);
				$('#hardwarecontent #Legrand').prop('checked', false);
				$('#hardwarecontent #ProGuard').prop('checked', false);
				$('#hardwarecontent #VionicPowerCode').prop('checked', false);
				$('#hardwarecontent #Hideki').prop('checked', false);
				$('#hardwarecontent #FHT8').prop('checked', false);
				$('#hardwarecontent #VionicCodeSecure').prop('checked', false);
			});
		}

		EditRego6XXType = function (idx, name, Mode1, Mode2, Mode3, Mode4, Mode5, Mode6) {
			cursordefault();
			var htmlcontent = '';
			htmlcontent = '<p><center><h2><span data-i18n="Device"></span>: ' + name + '</h2></center></p>\n';
			htmlcontent += $('#rego6xxtypeedit').html();
			$('#hardwarecontent').html(GetBackbuttonHTMLTable('ShowHardware') + htmlcontent);
			$('#hardwarecontent').i18n();

			$('#hardwarecontent #submitbuttonrego').click(function (e) {
				e.preventDefault();
				SetRego6XXType();
			});

			$('#hardwarecontent #idx').val(idx);
			$('#hardwarecontent #comborego6xxtype').val(Mode1);
		}

		SetCCUSBType = function () {
			$.post("setcurrentcostmetertype.webem", $("#hardwarecontent #ccusbtype").serialize(), function (data) {
				ShowHardware();
			});
		}

		RefreshLMSNodeTable = function () {
			$('#modal').show();
			$('#updelclr #nodeupdate').attr("class", "btnstyle3-dis");
			$('#updelclr #nodedelete').attr("class", "btnstyle3-dis");
			$("#hardwarecontent #lmsnodeparamstable #nodename").val("");
			$("#hardwarecontent #lmsnodeparamstable #nodeip").val("");
			$("#hardwarecontent #lmsnodeparamstable #nodeport").val("9000");

			var oTable = $('#lmsnodestable').dataTable();
			oTable.fnClearTable();

			$.ajax({
				url: "json.htm?type=command&param=lmsgetnodes&idx=" + $.devIdx,
				async: false,
				dataType: 'json',
				success: function (data) {
					if (typeof data.result != 'undefined') {
						$.each(data.result, function (i, item) {
							var addId = oTable.fnAddData({
								"DT_RowId": item.idx,
								"Name": item.Name,
								"Mac": item.Mac,
								"Timeout": item.Status,
								"0": item.idx,
								"1": item.Name,
								"2": item.Mac,
								"3": item.Status
							});
						});
					}
				}
			});

			$('#modal').hide();
		}

		SetLMSSettings = function () {
			var Mode1 = parseInt($("#hardwarecontent #lmssettingstable #pollinterval").val());
			if (Mode1 < 1)
				Mode1 = 30;
			$.ajax({
				url: "json.htm?type=command&param=lmssetmode" +
				"&idx=" + $.devIdx +
				"&mode1=" + Mode1,
				async: false,
				dataType: 'json',
				success: function (data) {
					bootbox.alert($.t('Settings saved'));
				},
				error: function () {
					ShowNotify($.t('Problem Updating Settings!'), 2500, true);
				}
			});
		}

		EditLMS = function (idx, name, Mode1, Mode2, Mode3, Mode4, Mode5, Mode6) {
			$.devIdx = idx;
			cursordefault();
			var htmlcontent = '';
			htmlcontent = '<p><center><h2><span data-i18n="Device"></span>: ' + name + '</h2></center></p>\n';
			htmlcontent += $('#lms').html();
			$('#hardwarecontent').html(GetBackbuttonHTMLTable('ShowHardware') + htmlcontent);
			$('#hardwarecontent').i18n();

			$("#hardwarecontent #lmssettingstable #pollinterval").val(Mode1);

			var oTable = $('#lmsnodestable').dataTable({
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

			RefreshLMSNodeTable();
		}

		DeleteUnusedLMSDevices = function () {
			bootbox.confirm($.t("Are you sure to delete all unused devices?"), function (result) {
				if (result == true) {
					$.ajax({
						url: "json.htm?type=command&param=lmsdeleteunuseddevices" +
							"&idx=" + $.devIdx,
						async: false,
						dataType: 'json',
						success: function (data) {
							RefreshLMSNodeTable();
							bootbox.alert($.t('Devices deleted'));
						},
						error: function () {
							ShowNotify($.t('Problem Deleting devices!'), 2500, true);
						}
					});
				}
			});
		}

		EditSBFSpot = function (idx, name, Mode1, Mode2, Mode3, Mode4, Mode5, Mode6) {
			$.devIdx = idx;
			cursordefault();
			var htmlcontent = '';
			htmlcontent = '<p><center><h2><span data-i18n="Device"></span>: ' + name + '</h2></center></p>\n';
			htmlcontent += $('#sbfspot').html();
			$('#hardwarecontent').html(GetBackbuttonHTMLTable('ShowHardware') + htmlcontent);
			$('#hardwarecontent').i18n();
			$('#hardwarecontent #idx').val(idx);
			$('#hardwarecontent #btnimportolddata').click(function (e) {
				e.preventDefault();
				bootbox.alert($.t('Importing old data, this could take a while!<br>You should automaticly return on the Dashboard'));
				$.post("sbfspotimportolddata.webem", $("#hardwarecontent #sbfspot").serialize(), function (data) {
					SwitchLayout('Dashboard');
				});
			});
		}

		EditS0MeterType = function (idx, name, Address) {
			cursordefault();
			var htmlcontent = '';
			htmlcontent = '<p><center><h2><span data-i18n="Device"></span>: ' + name + '</h2></center></p>\n';
			htmlcontent += $('#s0metertypeedit').html();
			$('#hardwarecontent').html(GetBackbuttonHTMLTable('ShowHardware') + htmlcontent);
			$('#hardwarecontent').i18n();

			$('#hardwarecontent #submitbuttons0meter').click(function (e) {
				e.preventDefault();
				SetS0MeterType();
			});
			$('#hardwarecontent #idx').val(idx);

			var res = Address.split(";");

			$('#hardwarecontent #combom1type').val(res[0]);
			if (res[1] != 0) {
				$('#hardwarecontent #M1PulsesPerHour').val(res[1]);
			}
			else {
				$('#hardwarecontent #M1PulsesPerHour').val(2000);
			}
			$('#hardwarecontent #combom2type').val(res[2]);
			if (res[3] != 0) {
				$('#hardwarecontent #M2PulsesPerHour').val(res[3]);
			}
			else {
				$('#hardwarecontent #M2PulsesPerHour').val(2000);
			}
			$('#hardwarecontent #combom3type').val(res[4]);
			if (res[5] != 0) {
				$('#hardwarecontent #M3PulsesPerHour').val(res[5]);
			}
			else {
				$('#hardwarecontent #M3PulsesPerHour').val(2000);
			}
			$('#hardwarecontent #combom4type').val(res[6]);
			if (res[7] != 0) {
				$('#hardwarecontent #M4PulsesPerHour').val(res[7]);
			}
			else {
				$('#hardwarecontent #M4PulsesPerHour').val(2000);
			}
			$('#hardwarecontent #combom5type').val(res[8]);
			if (res[9] != 0) {
				$('#hardwarecontent #M5PulsesPerHour').val(res[9]);
			}
			else {
				$('#hardwarecontent #M5PulsesPerHour').val(2000);
			}
		}

		EditLimitlessType = function (idx, name, Mode1, Mode2, Mode3, Mode4, Mode5, Mode6) {
			cursordefault();
			var htmlcontent = '';
			htmlcontent = '<p><center><h2><span data-i18n="Device"></span>: ' + name + '</h2></center></p>\n';
			htmlcontent += $('#limitlessmetertype').html();
			$('#hardwarecontent').html(GetBackbuttonHTMLTable('ShowHardware') + htmlcontent);
			$('#hardwarecontent').i18n();

			$('#hardwarecontent #submitbuttonlimitless').click(function (e) {
				e.preventDefault();
				SetLimitlessType();
			});

			$('#hardwarecontent #idx').val(idx);
			$('#hardwarecontent #combom1type').val(Mode1);
			$('#hardwarecontent #CCBridgeType').val(Mode2);
		}

		EditCCUSB = function (idx, name, Mode1, Mode2, Mode3, Mode4, Mode5, Mode6) {
			$.devIdx = idx;
			cursordefault();
			var htmlcontent = '';
			htmlcontent = '<p><center><h2><span data-i18n="Device"></span>: ' + name + '</h2></center></p>\n';
			htmlcontent += $('#ccusbeedit').html();
			$('#hardwarecontent').html(GetBackbuttonHTMLTable('ShowHardware') + htmlcontent);
			$('#hardwarecontent').i18n();

			$('#hardwarecontent #submitbuttonccusb').click(function (e) {
				e.preventDefault();
				SetCCUSBType();
			});

			$('#hardwarecontent #idx').val(idx);
			$('#hardwarecontent #CCBaudrate').val(Mode1);
		}

		SetOpenThermSettings = function () {
			$.post("setopenthermsettings.webem", $("#hardwarecontent #openthermform").serialize(), function (data) {
				ShowHardware();
			});
		}

		SendOTGWCommand = function () {
			var idx = $('#hardwarecontent #idx').val();
			var cmnd = $('#hardwarecontent #otgwcmnd').val();
			$.ajax({
				url: "json.htm?type=command&param=sendopenthermcommand" +
				"&idx=" + idx +
				"&cmnd=" + cmnd,
				async: false,
				dataType: 'json',
				success: function (data) {
				},
				error: function () {
				}
			});
		}

		EditOpenTherm = function (idx, name, Mode1, Mode2, Mode3, Mode4, Mode5, Mode6) {
			//Mode1=Outside Temperature Sensor DeviceIdx, 0=Not Using
			$.devIdx = idx;
			cursordefault();
			var htmlcontent = '';
			htmlcontent = '<p><center><h2><span data-i18n="Device"></span>: ' + name + '</h2></center></p>\n';
			htmlcontent += $('#opentherm').html();
			$('#hardwarecontent').html(GetBackbuttonHTMLTable('ShowHardware') + htmlcontent);
			$('#hardwarecontent').i18n();

			$('#hardwarecontent #idx').val(idx);
			$('#hardwarecontent #combooutsidesensor >option').remove();
			var option = $('<option />');
			option.attr('value', 0).text("Use Build In");
			$("#hardwarecontent #combooutsidesensor").append(option);

			$('#hardwarecontent #submitbuttonopentherm').click(function (e) {
				e.preventDefault();
				SetOpenThermSettings();
			});
			$('#hardwarecontent #buttonsendotgwcmnd').click(function (e) {
				e.preventDefault();
				SendOTGWCommand();
			});


			//Get Temperature Sensors
			$.ajax({
				url: "json.htm?type=devices&filter=temp&used=true&order=Name",
				async: false,
				dataType: 'json',
				success: function (data) {
					if (typeof data.result != 'undefined') {
						$.each(data.result, function (i, item) {
							if (typeof item.Temp != 'undefined') {
								var option = $('<option />');
								option.attr('value', item.idx).text(item.Name + " (" + item.Temp + "\u00B0)");
								$("#hardwarecontent #combooutsidesensor").append(option);
							}
						});
					}
					$("#hardwarecontent #combooutsidesensor").val(Mode1);
				}
			});
			$("#hardwarecontent #combooutsidesensor").val(Mode1);
		}

		SetRFXCOMMode = function () {
			$.post("setrfxcommode.webem", $("#hardwarecontent #settings").serialize(), function (data) {
				SwitchLayout('Dashboard');
			});
		}
		SetRFXCOMMode868 = function () {
			HideNotify();
			ShowNotify($.t('This should (for now) be set via the RFXmngr application!'), 2500, true);
/*
			$.post("setrfxcommode.webem", $("#hardwarecontent #settings").serialize(), function (data) {
				SwitchLayout('Dashboard');
			});
*/
		}

		SetRego6XXType = function () {
			$.post("setrego6xxtype.webem", $("#hardwarecontent #regotype").serialize(), function (data) {
				ShowHardware();
			});
		}

		SetS0MeterType = function () {
			$.post("sets0metertype.webem", $("#hardwarecontent #s0metertype").serialize(), function (data) {
				ShowHardware();
			});
		}

		SetLimitlessType = function () {
			$.post("setlimitlesstype.webem", $("#hardwarecontent #limitform").serialize(), function (data) {
				ShowHardware();
			});
		}

		BindEvohome = function (idx, name, devtype) {
			$.devIdx = idx;
			if (devtype == "Relay")
				ShowNotify($.t('Hold bind button on relay...'), 2500);
			else if (devtype == "AllSensors")
				ShowNotify($.t('Creating additional sensors...'));
			else if (devtype == "ZoneSensor")
				ShowNotify($.t('Binding Domoticz zone temperature sensor to controller...'), 2500);
			else
				ShowNotify($.t('Binding Domoticz outdoor temperature device to evohome controller...'), 2500);

			setTimeout(function () {
				var bNewDevice = false;
				var bIsUsed = false;
				var Name = "";

				$.ajax({
					url: "json.htm?type=bindevohome&idx=" + $.devIdx + "&devtype=" + devtype,
					async: false,
					dataType: 'json',
					success: function (data) {
						if (typeof data.status != 'undefined') {
							bIsUsed = data.Used;
							if (data.status == 'OK')
								bNewDevice = true;
							else
								Name = data.Name;
						}
					}
				});
				HideNotify();

				setTimeout(function () {
					if ((bNewDevice == true) && (bIsUsed == false)) {
						if (devtype == "Relay")
							ShowNotify($.t('Relay bound, and can be found in the devices tab!'), 2500);
						else if (devtype == "AllSensors")
							ShowNotify($.t('New sensors will appear in the devices tab after 10min'), 2500);
						else if (devtype == "ZoneSensor")
							ShowNotify($.t('Sensor bound, and can be found in the devices tab!'), 2500);
						else
							ShowNotify($.t('Domoticz outdoor temperature device has been bound to evohome controller'), 2500);
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

		CreateEvohomeSensors = function (idx, name) {
			$.devIdx = idx;
			$("#dialog-createevohomesensor").dialog({
				autoOpen: false,
				width: 380,
				height: 160,
				modal: true,
				resizable: false,
				buttons: {
					"OK": function () {
						var bValid = true;
						$(this).dialog("close");
						var SensorType = $("#dialog-createevohomesensor #sensortype option:selected").val();
						if (typeof SensorType == 'undefined') {
							bootbox.alert($.t('No Sensor Type Selected!'));
							return;
						}
						$.ajax({
							url: "json.htm?type=createevohomesensor&idx=" + $.devIdx +
							"&sensortype=" + SensorType,
							async: false,
							dataType: 'json',
							success: function (data) {
								if (data.status == 'OK') {
									ShowNotify($.t('Sensor Created, and can be found in the devices tab!'), 2500);
								}
								else {
									ShowNotify($.t('Problem creating Sensor!'), 2500, true);
								}
							},
							error: function () {
								HideNotify();
								ShowNotify($.t('Problem creating Sensor!'), 2500, true);
							}
						});
					},
					Cancel: function () {
						$(this).dialog("close");
					}
				},
				close: function () {
					$(this).dialog("close");
				}
			});

			$("#dialog-createevohomesensor").i18n();
			$("#dialog-createevohomesensor").dialog("open");
		}

		CreateRFLinkDevices = function (idx, name) {
			$.devIdx = idx;
			$("#dialog-createrflinkdevice #vsensoraxis").hide();
			$("#dialog-createrflinkdevice #sensoraxis").val("");
			$("#dialog-createrflinkdevice").dialog({
				autoOpen: false,
				width: 420,
				height: 250,
				modal: true,
				resizable: false,
				buttons: {
					"OK": function () {
						var bValid = true;
						$(this).dialog("close");
						var SensorName = $("#dialog-createrflinkdevice #sensorname").val();
						if (SensorName == "") {
							ShowNotify($.t('Please enter a command!'), 2500, true);
							return;
						}
						$.ajax({
							url: "json.htm?type=createrflinkdevice&idx=" + $.devIdx +
							"&command=" + SensorName,
							async: false,
							dataType: 'json',
							success: function (data) {
								if (data.status == 'OK') {
									ShowNotify($.t('Device created, it can be found in the devices tab!'), 2500);
								}
								else {
									ShowNotify($.t('Problem creating Sensor!'), 2500, true);
								}
							},
							error: function () {
								HideNotify();
								ShowNotify($.t('Problem creating Sensor!'), 2500, true);
							}
						});
					},
					Cancel: function () {
						$(this).dialog("close");
					}
				},
				close: function () {
					$(this).dialog("close");
				}
			});

			$("#dialog-createrflinkdevice").i18n();
			$("#dialog-createrflinkdevice").dialog("open");
		}

		function OnDummySensorTypeChange() {
			var stype = $("#dialog-createsensor #sensortype option:selected").val();
			$("#dialog-createsensor #sensoraxis").val("");
			if (stype == 0xF31F) {
				$("#dialog-createsensor #vsensoraxis").show();
			}
			else {
				$("#dialog-createsensor #vsensoraxis").hide();
			}
		}

		CreateDummySensors = function (idx, name) {
			$.devIdx = idx;

			$("#dialog-createsensor #vsensoraxis").hide();
			$("#dialog-createsensor #sensoraxis").val("");

			$("#dialog-createsensor #sensortype").change(function () {
				OnDummySensorTypeChange();
			});

			$("#dialog-createsensor").dialog({
				autoOpen: false,
				width: 420,
				height: 250,
				modal: true,
				resizable: false,
				buttons: {
					"OK": function () {
						var bValid = true;
						$(this).dialog("close");
						var SensorName = $("#dialog-createsensor #sensorname").val();
						if (SensorName == "") {
							ShowNotify($.t('Please enter a Name!'), 2500, true);
							return;
						}
						var SensorType = $("#dialog-createsensor #sensortype option:selected").val();
						if (typeof SensorType == 'undefined') {
							bootbox.alert($.t('No Sensor Type Selected!'));
							return;
						}
						var extraSendData = "";
                        if (SensorType == 0xF31F) {
							var AxisLabel = $("#dialog-createsensor #sensoraxis").val();
							if (AxisLabel == "") {
								ShowNotify($.t('Please enter a Axis Label!'), 2500, true);
								return;
							}
							extraSendData = "&sensoroptions=1;" + encodeURIComponent(AxisLabel);
						}
						$.ajax({
							url: "json.htm?type=createdevice&idx=" + $.devIdx +
							"&sensorname=" + encodeURIComponent(SensorName) +
							"&sensormappedtype=" + SensorType +
							extraSendData,
							async: false,
							dataType: 'json',
							success: function (data) {
								if (data.status == 'OK') {
									ShowNotify($.t('Sensor Created, and can be found in the devices tab!'), 2500);
								}
								else {
									ShowNotify($.t('Problem creating Sensor!'), 2500, true);
								}
							},
							error: function () {
								HideNotify();
								ShowNotify($.t('Problem creating Sensor!'), 2500, true);
							}
						});
					},
					Cancel: function () {
						$(this).dialog("close");
					}
				},
				close: function () {
					$(this).dialog("close");
				}
			});

			$("#dialog-createsensor").i18n();
			$("#dialog-createsensor").dialog("open");
			OnDummySensorTypeChange();
		}

		AddYeeLight = function (idx, name) {
			$.devIdx = idx;

			$("#dialog-addyeelight").dialog({
				autoOpen: false,
				width: 420,
				height: 250,
				modal: true,
				resizable: false,
				buttons: {
					"OK": function () {
						var bValid = true;
						var SensorName = $("#dialog-addyeelight #name").val();
						if (SensorName == "") {
							ShowNotify($.t('Please enter a Name!'), 2500, true);
							return;
						}
						var IPAddress = $("#dialog-addyeelight #ipaddress").val();
						if (IPAddress == "") {
							ShowNotify($.t('Please enter a IP Address!'), 2500, true);
							return;
						}
						var SensorType = $("#dialog-addyeelight #lighttype option:selected").val();
						if (typeof SensorType == 'undefined') {
							bootbox.alert($.t('No Light Type Selected!'));
							return;
						}
						$(this).dialog("close");
						$.ajax({
							url: "json.htm?type=command&param=addyeelight&idx=" + $.devIdx +
							"&name=" + encodeURIComponent(SensorName) +
							"&ipaddress=" + encodeURIComponent(IPAddress) +
							"&stype=" + SensorType,
							async: false,
							dataType: 'json',
							success: function (data) {
								if (data.status == 'OK') {
									ShowNotify($.t('Light created, and can be found in the devices tab!'), 2500);
								}
								else {
									ShowNotify($.t('Problem adding Light!'), 2500, true);
								}
							},
							error: function () {
								HideNotify();
								ShowNotify($.t('Problem adding Light!'), 2500, true);
							}
						});
					},
					Cancel: function () {
						$(this).dialog("close");
					}
				},
				close: function () {
					$(this).dialog("close");
				}
			});

			$("#dialog-addyeelight").i18n();
			$("#dialog-addyeelight").dialog("open");
		}

		AddArilux = function (idx, name) {
			$.devIdx = idx;

			$("#dialog-addArilux").dialog({
				autoOpen: false,
				width: 420,
				height: 250,
				modal: true,
				resizable: false,
				buttons: {
					"OK": function () {
						var bValid = true;
						var SensorName = $("#dialog-addArilux #name").val();
						if (SensorName == "") {
							ShowNotify($.t('Please enter a Name!'), 2500, true);
							return;
						}
						var IPAddress = $("#dialog-addArilux #ipaddress").val();
						if (IPAddress == "") {
							ShowNotify($.t('Please enter a IP Address!'), 2500, true);
							return;
						}
						var SensorType = $("#dialog-addArilux #lighttype option:selected").val();
						if (typeof SensorType == 'undefined') {
							bootbox.alert($.t('No Light Type Selected!'));
							return;
						}
						$(this).dialog("close");
						$.ajax({
							url: "json.htm?type=command&param=addArilux&idx=" + $.devIdx +
							"&name=" + encodeURIComponent(SensorName) +
							"&ipaddress=" + encodeURIComponent(IPAddress) +
							"&stype=" + SensorType,
							async: false,
							dataType: 'json',
							success: function (data) {
								if (data.status == 'OK') {
									ShowNotify($.t('Light created, and can be found in the devices tab!'), 2500);
								}
								else {
									ShowNotify($.t('Problem adding Light!'), 2500, true);
								}
							},
							error: function () {
								HideNotify();
								ShowNotify($.t('Problem adding Light!'), 2500, true);
							}
						});
					},
					Cancel: function () {
						$(this).dialog("close");
					}
				},
				close: function () {
					$(this).dialog("close");
				}
			});

			$("#dialog-addArilux").i18n();
			$("#dialog-addArilux").dialog("open");
		}

		ReloadPiFace = function (idx, name) {
			$.post("reloadpiface.webem", { 'idx': idx }, function (data) {
				ShowNotify($.t('PiFace config reloaded!'), 2500);
			});
		}

		TellstickSettings = function (idx, name, repeats, repeatInterval) {
			$.idx = idx;
			cursordefault();
			var htmlcontent = '';
			htmlcontent = '<p><center><h2><span data-i18n="Device"></span>: ' + name + '</h2></center></p>\n';
			htmlcontent += $('#tellstick').html();
			$('#hardwarecontent').html(GetBackbuttonHTMLTable('ShowHardware') + htmlcontent);
			$('#hardwarecontent').i18n();

			$('#hardwarecontent #idx').val(idx);
			$("#hardwarecontent #tellstickSettingsTable #repeats").val(repeats);
			$("#hardwarecontent #tellstickSettingsTable #repeatInterval").val(repeatInterval);
		}

		TellstickApplySettings = function () {
			var repeats = parseInt($("#hardwarecontent #tellstickSettingsTable #repeats").val());
			if (repeats < 0)
				repeats = 0;
			if (repeats > 10)
				repeats = 10;
			var repeatInterval = parseInt($("#hardwarecontent #tellstickSettingsTable #repeatInterval").val());
			if (repeatInterval < 10)
				repeatInterval = 10;
			if (repeatInterval > 2000)
				repeatInterval = 2000;
			$.ajax({
				url: "json.htm?type=command&param=tellstickApplySettings" +
				"&idx=" + $.idx +
				"&repeats=" + repeats +
				"&repeatInterval=" + repeatInterval,
				async: false,
				dataType: 'json',
				success: function (data) {
					bootbox.alert($.t('Settings saved'));
				},
				error: function () {
					ShowNotify($.t('Failed saving settings'), 2500, true);
				}
			});
		}

		RefreshHardwareTable = function () {
			$('#modal').show();

			$('#updelclr #hardwareupdate').attr("class", "btnstyle3-dis");
			$('#updelclr #hardwaredelete').attr("class", "btnstyle3-dis");

			var oTable = $('#hardwaretable').dataTable();
			oTable.fnClearTable();

			$.ajax({
				url: "json.htm?type=hardware",
				async: false,
				dataType: 'json',
				success: function (data) {

					if (typeof data.result != 'undefined') {
						$.each(data.result, function (i, item) {

							var HwTypeStrOrg = $.myglobals.HardwareTypesStr[item.Type];
							var HwTypeStr = HwTypeStrOrg;
                            var hardwareSetupLink = '<a href="#/Hardware/' + item.idx + '" class="label label-info lcursor btn-link">' + $.t("Setup") + '</a>';

							if (typeof HwTypeStr == 'undefined') {
								HwTypeStr = "???? Unknown (NA/Not supported)";
							}

							var SerialName = "Unknown!?";
							var intport = 0;
							if ((HwTypeStr.indexOf("LAN") >= 0) || (HwTypeStr.indexOf("MySensors Gateway with MQTT") >= 0) || (HwTypeStr.indexOf("Domoticz") >= 0) || (HwTypeStr.indexOf("Harmony") >= 0) || (HwTypeStr.indexOf("Philips Hue") >= 0)) {
								SerialName = item.Port;
							}
							else if ((item.Type == 7) || (item.Type == 11)) {
								SerialName = "USB";
							}
							else if ((item.Type == 14) || (item.Type == 25) || (item.Type == 28) || (item.Type == 30) || (item.Type == 34)) {
								SerialName = "WWW";
							}
							else if ((item.Type == 15) || (item.Type == 23) || (item.Type == 26) || (item.Type == 27) || (item.Type == 51) || (item.Type == 54)) {
								SerialName = "";
							}
							else if ((item.Type == 16) || (item.Type == 32)) {
								SerialName = "GPIO";
							}
							else if (HwTypeStr.indexOf("Evohome") >= 0 && HwTypeStr.indexOf("script") >= 0) {
								SerialName = "Script";
							}
							else if (item.Type == 74) {
								intport = item.Port;
								SerialName = "";
							}
							else if (item.Type == 94) // For Python plugins show the actual plug description
							{
								HwTypeStr = eval('$("#' + item.Extra + '").text()')
								HwTypeStrOrg = "PLUGIN";
								intport = item.Port;
								SerialName = item.SerialPort;
							}
							else {
								SerialName = item.SerialPort;
								intport = jQuery.inArray(item.SerialPort, $scope.SerialPortStr);
							}

							var enabledstr = $.t("No");
							if (item.Enabled == "true") {
								enabledstr = $.t("Yes");
							}
							if (HwTypeStr.indexOf("RFXCOM") >= 0) {
								HwTypeStr += '<br>Version: ' + item.version;
								if (item.noiselvl != 0) {
									HwTypeStr += ', Noise: ' + item.noiselvl + ' dB';
								}
								if (HwTypeStr.indexOf("868") >= 0) {
									HwTypeStr += ' <span class="label label-info lcursor" onclick="EditRFXCOMMode868(' + item.idx + ',\'' + item.Name + '\',' + item.Mode1 + ',' + item.Mode2 + ',' + item.Mode3 + ',' + item.Mode4 + ',' + item.Mode5 + ',' + item.Mode6 + ');">' + $.t("Set Mode") + '</span>';
								}
								else {
									HwTypeStr += ' <span class="label label-info lcursor" onclick="EditRFXCOMMode(' + item.idx + ',\'' + item.Name + '\',' + item.Mode1 + ',' + item.Mode2 + ',' + item.Mode3 + ',' + item.Mode4 + ',' + item.Mode5 + ',' + item.Mode6 + ',\'' + item.Extra + '\'' + ',\'' + item.version + '\');">' + $.t("Set Mode") + '</span>';
								}
							}
							else if (HwTypeStr.indexOf("S0 Meter") >= 0) {
								HwTypeStr += ' <span class="label label-info lcursor" onclick="EditS0MeterType(' + item.idx + ',\'' + item.Name + '\',\'' + item.Extra + '\');">' + $.t("Set Mode") + '</span>';
							}
							else if (HwTypeStr.indexOf("Limitless") >= 0) {
								HwTypeStr += ' <span class="label label-info lcursor" onclick="EditLimitlessType(' + item.idx + ',\'' + item.Name + '\',' + item.Mode1 + ',' + item.Mode2 + ',' + item.Mode3 + ',' + item.Mode4 + ',' + item.Mode5 + ',' + item.Mode6 + ');">' + $.t("Set Mode") + '</span>';
							}
							else if (HwTypeStr.indexOf("OpenZWave") >= 0) {
								HwTypeStr += '<br>Version: ' + item.version;

								if (typeof item.NodesQueried != 'undefined') {
									var lblStatus = "label-info";
									if (item.NodesQueried != true) {
										lblStatus = "label-important";
									}
									HwTypeStr += ' <a href="#/Hardware/' + item.idx + '" class="label ' + lblStatus + ' btn-link">' + $.t("Setup") + '</a>';
								}
							}
							else if (HwTypeStr.indexOf("SBFSpot") >= 0) {
								HwTypeStr += ' <span class="label label-info lcursor" onclick="EditSBFSpot(' + item.idx + ',\'' + item.Name + '\',' + item.Mode1 + ',' + item.Mode2 + ',' + item.Mode3 + ',' + item.Mode4 + ',' + item.Mode5 + ',' + item.Mode6 + ');">' + $.t("Setup") + '</span>';
							}
							else if (HwTypeStr.indexOf("MySensors") >= 0) {
								HwTypeStr += '<br>Version: ' + item.version;
                                HwTypeStr += ' ' + hardwareSetupLink;
							}
							else if ((HwTypeStr.indexOf("OpenTherm") >= 0) || (HwTypeStr.indexOf("Thermosmart") >= 0)) {
								HwTypeStr += '<br>Version: ' + item.version;
								HwTypeStr += ' <span class="label label-info lcursor" onclick="EditOpenTherm(' + item.idx + ',\'' + item.Name + '\',' + item.Mode1 + ',' + item.Mode2 + ',' + item.Mode3 + ',' + item.Mode4 + ',' + item.Mode5 + ',' + item.Mode6 + ');">' + $.t("Setup") + '</span>';
							}
							else if (HwTypeStr.indexOf("Wake-on-LAN") >= 0) {
                                HwTypeStr += ' ' + hardwareSetupLink;
							}
							else if (HwTypeStr.indexOf("System Alive") >= 0) {
                                HwTypeStr += ' ' + hardwareSetupLink;
							}
							else if (HwTypeStr.indexOf("Kodi Media") >= 0) {
                                HwTypeStr += ' ' + hardwareSetupLink;
							}
							else if (HwTypeStr.indexOf("Panasonic") >= 0) {
                                HwTypeStr += ' ' + hardwareSetupLink;
							}
							else if (HwTypeStr.indexOf("BleBox") >= 0) {
								HwTypeStr += ' ' + hardwareSetupLink;
							}
							else if (HwTypeStr.indexOf("Logitech Media Server") >= 0) {
								HwTypeStr += ' <span class="label label-info lcursor" onclick="EditLMS(' + item.idx + ',\'' + item.Name + '\',' + item.Mode1 + ',' + item.Mode2 + ',' + item.Mode3 + ',' + item.Mode4 + ',' + item.Mode5 + ',' + item.Mode6 + ');">' + $.t("Setup") + '</span>';
							}
							else if (HwTypeStr.indexOf("HEOS by DENON") >= 0) {
								HwTypeStr += ' <span class="label label-info lcursor" onclick="EditHEOS by DENON(' + item.idx + ',\'' + item.Name + '\',' + item.Mode1 + ',' + item.Mode2 + ',' + item.Mode3 + ',' + item.Mode4 + ',' + item.Mode5 + ',' + item.Mode6 + ');">' + $.t("Setup") + '</span>';
							}
							else if (HwTypeStr.indexOf("Dummy") >= 0) {
								HwTypeStr += ' <span class="label label-info lcursor" onclick="CreateDummySensors(' + item.idx + ',\'' + item.Name + '\');">' + $.t("Create Virtual Sensors") + '</span>';
							}
							else if (HwTypeStr.indexOf("YeeLight") >= 0) {
								HwTypeStr += ' <span class="label label-info lcursor" onclick="AddYeeLight(' + item.idx + ',\'' + item.Name + '\');">' + $.t("Add Light") + '</span>';
							}
							else if (HwTypeStr.indexOf("PiFace") >= 0) {
								HwTypeStr += ' <span class="label label-info lcursor" onclick="ReloadPiFace(' + item.idx + ',\'' + item.Name + '\');">' + $.t("Reload") + '</span>';
							}
							else if (HwTypeStr.indexOf("HTTP/HTTPS") >= 0) {
								HwTypeStr += ' <span class="label label-info lcursor" onclick="CreateDummySensors(' + item.idx + ',\'' + item.Name + '\');">' + $.t("Create Virtual Sensors") + '</span>';
							}
							else if (HwTypeStr.indexOf("RFLink") >= 0) {
								HwTypeStr += '<br>Version: ' + item.version;
								HwTypeStr += ' <span class="label label-info lcursor" onclick="CreateRFLinkDevices(' + item.idx + ',\'' + item.Name + '\');">' + $.t("Create RFLink Devices") + '</span>';
							}
							else if (HwTypeStr.indexOf("Evohome") >= 0) {
								if (HwTypeStr.indexOf("script") >= 0 || HwTypeStr.indexOf("Web") >= 0)
									HwTypeStr += ' <span class="label label-info lcursor" onclick="CreateEvohomeSensors(' + item.idx + ',\'' + item.Name + '\');">' + $.t("Create Devices") + '</span>';
								else {
									HwTypeStr += ' <span class="label label-info lcursor" onclick="BindEvohome(' + item.idx + ',\'' + item.Name + '\',\'Relay\');">Bind Relay</span>';
									HwTypeStr += ' <span class="label label-info lcursor" onclick="BindEvohome(' + item.idx + ',\'' + item.Name + '\',\'OutdoorSensor\');">Outdoor Sensor</span>';
									HwTypeStr += ' <span class="label label-info lcursor" onclick="BindEvohome(' + item.idx + ',\'' + item.Name + '\',\'AllSensors\');">All Sensors</span>';
									HwTypeStr += ' <span class="label label-info lcursor" onclick="BindEvohome(' + item.idx + ',\'' + item.Name + '\',\'ZoneSensor\');">Bind Temp Sensor</span>';
								}
							}
							else if (HwTypeStr.indexOf("Rego 6XX") >= 0) {
								HwTypeStr += '<br>Type: ';
								if (item.Mode1 == "0") {
									HwTypeStr += '600-635, 637 single language';
								}
								else if (item.Mode1 == "1") {
									HwTypeStr += '636';
								}
								else if (item.Mode1 == "2") {
									HwTypeStr += '637 multi language';
								}
								HwTypeStr += ' <span class="label label-info lcursor" onclick="EditRego6XXType(' + item.idx + ',\'' + item.Name + '\',' + item.Mode1 + ',' + item.Mode2 + ',' + item.Mode3 + ',' + item.Mode4 + ',' + item.Mode5 + ',' + item.Mode6 + ');">Change Type</span>';
							}
							else if (HwTypeStr.indexOf("CurrentCost Meter USB") >= 0) {
								HwTypeStr += ' <span class="label label-info lcursor" onclick="EditCCUSB(' + item.idx + ',\'' + item.Name + '\',\'' + item.Address + '\');">' + $.t("Set Mode") + '</span>';
							}
							else if (HwTypeStr.indexOf("Tellstick") >= 0) {
								HwTypeStr += ' <span class="label label-info lcursor" onclick="TellstickSettings(' + item.idx + ',\'' + item.Name + '\',' + item.Mode1 + ',' + item.Mode2 + ');">' + $.t("Settings") + '</span>';
							}
							else if (HwTypeStr.indexOf("Arilux AL-LC0x") >= 0) {
								HwTypeStr += ' <span class="label label-info lcursor" onclick="AddArilux(' + item.idx + ',\'' + item.Name + '\');">' + $.t("Add Light") + '</span>';
							}

							var sDataTimeout = "";
							if (item.DataTimeout == 0) {
								sDataTimeout = $.t("Disabled");
							}
							else if (item.DataTimeout < 60) {
								sDataTimeout = item.DataTimeout + " " + $.t("Seconds");
							}
							else if (item.DataTimeout < 3600) {
								var minutes = item.DataTimeout / 60;
								if (minutes == 1) {
									sDataTimeout = minutes + " " + $.t("Minute");
								}
								else {
									sDataTimeout = minutes + " " + $.t("Minutes");
								}
							}
							else if (item.DataTimeout <= 86400) {
								var hours = item.DataTimeout / 3600;
								if (hours == 1) {
									sDataTimeout = hours + " " + $.t("Hour");
								}
								else {
									sDataTimeout = hours + " " + $.t("Hours");
								}
							}
							else {
								var days = item.DataTimeout / 86400;
								if (days == 1) {
									sDataTimeout = days + " " + $.t("Day");
								}
								else {
									sDataTimeout = days + " " + $.t("Days");
								}
							}

							var dispAddress = item.Address;
							if ((item.Type == 13) || (item.Type == 71) || (item.Type == 85) || (item.Type == 96)) {
								dispAddress = "I2C";
							}
							else if (item.Type == 93 || item.Type == 109) {
								dispAddress = "I2C-" + dispAddress;
							}

							var addId = oTable.fnAddData({
								"DT_RowId": item.idx,
								"Username": item.Username,
								"Password": item.Password,
								"Extra": item.Extra,
								"Enabled": item.Enabled,
								"Name": item.Name,
								"Mode1": item.Mode1,
								"Mode2": item.Mode2,
								"Mode3": item.Mode3,
								"Mode4": item.Mode4,
								"Mode5": item.Mode5,
								"Mode6": item.Mode6,
								"Type": HwTypeStrOrg,
								"IntPort": intport,
								"Address": item.Address,
								"Port": SerialName,
								"DataTimeout": item.DataTimeout,
								"LogLevel": item.LogLevel,
								"0": item.idx,
								"1": item.Name,
								"2": enabledstr,
								"3": HwTypeStr,
								"4": dispAddress,
								"5": SerialName,
								"6": sDataTimeout
							});
						});
					}
				}
			});

			/* Add a click handler to the rows - this could be used as a callback */
			$("#hardwaretable tbody").off();
			$("#hardwaretable tbody").on('click', 'tr', function () {
				if ($(this).hasClass('row_selected')) {
					$(this).removeClass('row_selected');
					$('#updelclr #hardwareupdate').attr("class", "btnstyle3-dis");
					$('#updelclr #hardwaredelete').attr("class", "btnstyle3-dis");
				}
				else {
					var oTable = $('#hardwaretable').dataTable();
					oTable.$('tr.row_selected').removeClass('row_selected');
					$(this).addClass('row_selected');
					$('#updelclr #hardwareupdate').attr("class", "btnstyle3");
					$('#updelclr #hardwaredelete').attr("class", "btnstyle3");
					var anSelected = fnGetSelected(oTable);
					if (anSelected.length !== 0) {
						var data = oTable.fnGetData(anSelected[0]);
						var idx = data["DT_RowId"];
						if (data["Type"] != "PLUGIN") { // Plugins can have non-numeric Mode data
							$("#updelclr #hardwareupdate").attr("href", "javascript:UpdateHardware(" + idx + "," + data["Mode1"] + "," + data["Mode2"] + "," + data["Mode3"] + "," + data["Mode4"] + "," + data["Mode5"] + "," + data["Mode6"] + ")");
						}
						else {
							$("#updelclr #hardwareupdate").attr("href", "javascript:UpdateHardware(" + idx + ",'" + data["Mode1"] + "','" + data["Mode2"] + "','" + data["Mode3"] + "','" + data["Mode4"] + "','" + data["Mode5"] + "','" + data["Mode6"] + "')");
						}
						$("#updelclr #hardwaredelete").attr("href", "javascript:DeleteHardware(" + idx + ")");
						$("#hardwarecontent #hardwareparamstable #hardwarename").val(data["Name"]);
						if (data["Type"] != "PLUGIN")
							$("#hardwarecontent #hardwareparamstable #combotype").val(jQuery.inArray(data["Type"], $.myglobals.HardwareTypesStr));
						else
							$("#hardwarecontent #hardwareparamstable #combotype").val(data["Extra"]);
						if (data["Type"].indexOf("I2C ") >= 0) {
							$("#hardwarecontent #hardwareparamstable #combotype").val(1000);
						}

						$.devExtra = data["Extra"];
						
						// Handle plugins 1st because all the text indexof logic below will have unpredictable impacts for plugins
						// Handle plugins generically.  If the plugin requires a data field it will have been created on page load.
						if (data["Type"] == "PLUGIN") {
							$('#hardwarecontent #hardwareparamstable #enabled').prop('checked', (data["Enabled"] == "true"));
							$('#hardwarecontent #hardwareparamstable #combodatatimeout').val(data["DataTimeout"]);
							$("#hardwarecontent #divpythonplugin #" + data["Extra"] + " #Username").val(data["Username"]);
							$("#hardwarecontent #divpythonplugin #" + data["Extra"] + " #Password").val(data["Password"]);
							$("#hardwarecontent #divpythonplugin #" + data["Extra"] + " #Address").val(data["Address"])
							$("#hardwarecontent #divpythonplugin #" + data["Extra"] + " #Port").val(data["IntPort"]);
							$("#hardwarecontent #divpythonplugin #" + data["Extra"] + " #SerialPort").val(data["Port"]);
							$("#hardwarecontent #divpythonplugin #" + data["Extra"] + " #Mode1").val(data["Mode1"]);
							$("#hardwarecontent #divpythonplugin #" + data["Extra"] + " #Mode2").val(data["Mode2"]);
							$("#hardwarecontent #divpythonplugin #" + data["Extra"] + " #Mode3").val(data["Mode3"]);
							$("#hardwarecontent #divpythonplugin #" + data["Extra"] + " #Mode4").val(data["Mode4"]);
							$("#hardwarecontent #divpythonplugin #" + data["Extra"] + " #Mode5").val(data["Mode5"]);
							$("#hardwarecontent #divpythonplugin #" + data["Extra"] + " #Mode6").val(data["Mode6"]);
							$("#hardwarecontent #divpythonplugin #" + data["Extra"] + " #Extra").val(data["Extra"]);
							UpdateHardwareParamControls();
							$('#hardwarecontent #hardwareparamstable #loglevelInfo').prop('checked', ((data["LogLevel"] & 1)!=0));
							$('#hardwarecontent #hardwareparamstable #loglevelStatus').prop('checked', ((data["LogLevel"] & 2)!=0));
							$('#hardwarecontent #hardwareparamstable #loglevelError').prop('checked', ((data["LogLevel"] & 4)!=0));
							return;
						}
						
						UpdateHardwareParamControls();

						$('#hardwarecontent #hardwareparamstable #enabled').prop('checked', (data["Enabled"] == "true"));
						$('#hardwarecontent #hardwareparamstable #loglevelInfo').prop('checked', ((data["LogLevel"] & 1)!=0));
						$('#hardwarecontent #hardwareparamstable #loglevelStatus').prop('checked', ((data["LogLevel"] & 2)!=0));
						$('#hardwarecontent #hardwareparamstable #loglevelError').prop('checked', ((data["LogLevel"] & 4)!=0));
						$('#hardwarecontent #hardwareparamstable #combodatatimeout').val(data["DataTimeout"]);

						if (
							(data["Type"].indexOf("TE923") >= 0) ||
							(data["Type"].indexOf("Volcraft") >= 0) ||
							(data["Type"].indexOf("Dummy") >= 0) ||
							(data["Type"].indexOf("System Alive") >= 0) ||
							(data["Type"].indexOf("PiFace") >= 0) ||
							(data["Type"].indexOf("Tellstick") >= 0) ||
							(data["Type"].indexOf("Yeelight") >= 0) ||
							(data["Type"].indexOf("Arilux AL-LC0x") >= 0)
						) {
							//nothing to be set
						}
						else if (data["Type"].indexOf("1-Wire") >= 0) {
							$("#hardwarecontent #hardwareparams1wire #owfspath").val(data["Extra"]);
							$("#hardwarecontent #hardwareparams1wire #OneWireSensorPollPeriod").val(data["Mode1"]);
							$("#hardwarecontent #hardwareparams1wire #OneWireSwitchPollPeriod").val(data["Mode2"]);
						}
						else if (data["Type"].indexOf("I2C ") >= 0) {
							$("#hardwareparamsi2clocal #comboi2clocal").val(jQuery.inArray(data["Type"], $.myglobals.HardwareI2CStr));
							$("#hardwareparamsi2clocal #i2cpath").val(data["Port"]);
							if (data["Type"].indexOf("I2C sensor PIO 8bit expander PCF8574") >= 0) {
								$("#hardwareparami2caddress #i2caddress").val(data["Address"]);
								$("#hardwareparami2cinvert #i2cinvert").prop("checked", data["Mode1"] == 1);
							}
							else if (data["Type"].indexOf("I2C sensor GPIO 16bit expander MCP23017") >= 0) {
								$("#hardwareparami2caddress #i2caddress").val(data["Address"]);
								$("#hardwareparami2cinvert #i2cinvert").prop("checked", data["Mode1"] == 1);
							}
						}
						else if ((data["Type"].indexOf("GPIO") >= 0) && (data["Type"].indexOf("sysfs GPIO") == -1)) {
							$("#hardwareparamsgpio #gpiodebounce").val(data["Mode1"]);
							$("#hardwareparamsgpio #gpioperiod").val(data["Mode2"]);
							$("#hardwareparamsgpio #gpiopollinterval").val(data["Mode3"]);
						}
						else if (data["Type"].indexOf("sysfs GPIO") >= 0) {
							$("#hardwarecontent #hardwareparamssysfsgpio #sysfsautoconfigure").prop("checked", data["Mode1"] == 1);
							$("#hardwarecontent #hardwareparamssysfsgpio #sysfsdebounce").val(data["Mode2"]);
						}
						else if (data["Type"].indexOf("USB") >= 0 || data["Type"].indexOf("Teleinfo EDF") >= 0) {
							$("#hardwarecontent #hardwareparamsserial #comboserialport").val(data["IntPort"]);
							if (data["Type"].indexOf("Evohome") >= 0) {
								$("#hardwarecontent #divevohome #combobaudrateevohome").val(data["Mode1"]);
								$("#hardwarecontent #divevohome #controllerid").val(data["Extra"]);
							}
							if (data["Type"].indexOf("MySensors") >= 0) {
								$("#hardwarecontent #divbaudratemysensors #combobaudrate").val(data["Mode1"]);
							}
							if (data["Type"].indexOf("P1 Smart Meter") >= 0) {
								$("#hardwarecontent #divbaudratep1 #combobaudratep1").val(data["Mode1"]);
								$("#hardwarecontent #divcrcp1 #disablecrcp1").prop("checked", data["Mode2"] == 0);
								$("#hardwarecontent #hardwareparamsratelimitp1 #ratelimitp1").val(data["Mode3"]);
								$("#hardwarecontent #divkeyp1p1 #decryptionkey").val(data["Password"]);
								if (data["Mode1"] == 0) {
									$("#hardwarecontent #divcrcp1").hide();
								}
								else {
									$("#hardwarecontent #divcrcp1").show();
								}
							}
							else if (data["Type"].indexOf("Teleinfo EDF") >= 0) {
								$("#hardwarecontent #divbaudrateteleinfo #combobaudrateteleinfo").val(data["Mode1"]);
								$("#hardwarecontent #divcrcp1 #disablecrcp1").prop("checked", data["Mode2"] == 0);
								$("#hardwarecontent #hardwareparamsratelimitp1 #ratelimitp1").val(data["Mode3"]);
								if (data["Mode1"] == 0) {
									$("#hardwarecontent #divcrcp1").hide();
								}
								else {
									$("#hardwarecontent #divcrcp1").show();
								}
							}
							else if (data["Type"].indexOf("USBtin") >= 0) {
								//$("#hardwarecontent #divusbtin #combotypecanusbtin").val( data["Mode1"] );
								$("#hardwarecontent #divusbtin #activateMultiblocV8").prop("checked", (data["Mode1"] &0x01) > 0 );
								$("#hardwarecontent #divusbtin #activateCanFree").prop("checked", (data["Mode1"] &0x02) > 0 );
								$("#hardwarecontent #divusbtin #combodebugusbtin").val( data["Mode2"] );

							} else if (data["Type"].indexOf("Denkovi") >= 0) {
                                $("#hardwarecontent #divmodeldenkoviusbdevices #combomodeldenkoviusbdevices").val(data["Mode1"]);
                            }
						}
						else if ((((data["Type"].indexOf("LAN") >= 0) || (data["Type"].indexOf("Eco Devices") >= 0) || data["Type"].indexOf("MySensors Gateway with MQTT") >= 0) && (data["Type"].indexOf("YouLess") == -1) && (data["Type"].indexOf("Denkovi") == -1) && (data["Type"].indexOf("Relay-Net") == -1) && (data["Type"].indexOf("Satel Integra") == -1) && (data["Type"].indexOf("eHouse") == -1) && (data["Type"].indexOf("MyHome OpenWebNet with LAN interface") == -1)) || (data["Type"].indexOf("Domoticz") >= 0) || (data["Type"].indexOf("Harmony") >= 0)) {
							$("#hardwarecontent #hardwareparamsremote #tcpaddress").val(data["Address"]);
							$("#hardwarecontent #hardwareparamsremote #tcpport").val(data["Port"]);
							if (data["Type"].indexOf("P1 Smart Meter") >= 0) {
								$("#hardwarecontent #divcrcp1 #disablecrcp1").prop("checked", data["Mode2"] == 0);
								$("#hardwarecontent #hardwareparamsratelimitp1 #ratelimitp1").val(data["Mode3"]);
								$("#hardwarecontent #divkeyp1p1 #decryptionkey").val(data["Password"]);
							}
							if (data["Type"].indexOf("Eco Devices") >= 0) {
								$("#hardwarecontent #divmodelecodevices #combomodelecodevices").val(data["Mode1"]);
								$("#hardwarecontent #hardwareparamsratelimitp1 #ratelimitp1").val(data["Mode2"]);
							}
							if (data["Type"].indexOf("Evohome") >= 0) {
								$("#hardwarecontent #divevohometcp #controlleridevohometcp").val(data["Extra"]);
							}
						}
						else if (
							(((data["Type"].indexOf("LAN") >= 0) || data["Type"].indexOf("MySensors Gateway with MQTT") >= 0) && (data["Type"].indexOf("YouLess") >= 0)) ||
							(data["Type"].indexOf("Domoticz") >= 0) ||
							(data["Type"].indexOf("Denkovi") >= 0) ||
							(data["Type"].indexOf("Relay-Net") >= 0) ||
							(data["Type"].indexOf("Satel Integra") >= 0) ||
							(data["Type"].indexOf("eHouse") >= 0) ||
							(data["Type"].indexOf("Logitech Media Server") >= 0) ||
							(data["Type"].indexOf("HEOS by DENON") >= 0) ||
							(data["Type"].indexOf("Xiaomi Gateway") >= 0) ||
							(data["Type"].indexOf("MyHome OpenWebNet with LAN interface") >= 0)
							) {
							$("#hardwarecontent #hardwareparamsremote #tcpaddress").val(data["Address"]);
							$("#hardwarecontent #hardwareparamsremote #tcpport").val(data["Port"]);
							$("#hardwarecontent #hardwareparamslogin #password").val(data["Password"]);

							if (data["Type"].indexOf("Satel Integra") >= 0) {
								$("#hardwarecontent #hardwareparamspollinterval #pollinterval").val(data["Mode1"]);
							}
                            else if (data["Type"].indexOf("MyHome OpenWebNet with LAN interface") >= 0) {
                                var RateLimit = parseInt(data["Mode1"]);
                                if (RateLimit && (RateLimit < 300)) {
                                    RateLimit = 300;
                                }
                                $("#hardwarecontent #hardwareparamsratelimitp1 #ratelimitp1").val(RateLimit);

								var ensynchro = parseInt(data["Mode2"]);
                                if (ensynchro && (ensynchro < 5)) {
                                    ensynchro = 5;
                                }
                                $("#hardwarecontent #hardwareparamsensynchro #ensynchro").val(ensynchro);
                            }
							else if (data["Type"].indexOf("eHouse") >= 0) {
								$("#hardwarecontent #hardwareparamspollinterval #pollinterval").val(data["Mode1"]);
					 			$('#hardwarecontent #hardwareparamsehouse #ehouseautodiscovery').prop("checked",data["Mode2"] == 1);
								$('#hardwarecontent #hardwareparamsehouse #ehouseaddalarmin').prop("checked",data["Mode3"] == 1);
								$('#hardwarecontent #hardwareparamsehouse #ehouseprodiscovery').prop("checked",data["Mode4"] == 1);
								$('#hardwarecontent #hardwareparamsehouse #ehouseopts').val(data["Mode5"]);
								$('#hardwarecontent #hardwareparamsehouse #ehouseopts2').val(data["Mode6"]);
							}
							else if (data["Type"].indexOf("Relay-Net") >= 0) {
								$("#hardwarecontent #hardwareparamsrelaynet #relaynetpollinputs").prop("checked", data["Mode1"] == 1);
								$("#hardwarecontent #hardwareparamsrelaynet #relaynetpollrelays").prop("checked", data["Mode2"] == 1);
								$("#hardwarecontent #hardwareparamsrelaynet #relaynetpollinterval").val(data["Mode3"]);
								$("#hardwarecontent #hardwareparamsrelaynet #relaynetinputcount").val(data["Mode4"]);
								$("#hardwarecontent #hardwareparamsrelaynet #relaynetrelaycount").val(data["Mode5"]);
							}
							else if (data["Type"].indexOf("Denkovi") >= 0) {
								$("#hardwarecontent #hardwareparamspollinterval #pollinterval").val(data["Mode1"]);
								if (data["Type"].indexOf("Modules with LAN (HTTP)") >= 0)
									$("#hardwarecontent #divmodeldenkovidevices #combomodeldenkovidevices").val(data["Mode2"]);
								else if (data["Type"].indexOf("Modules with LAN (TCP)") >= 0) {
									$("#hardwarecontent #divmodeldenkovitcpdevices #combomodeldenkovitcpdevices").val(data["Mode2"]);
									if(data["Mode2"] == "1"){
										$("#hardwarecontent #hardwareparamsmodeldenkovitcpdevices #lbldenkovislaveid").show();
										$("#hardwarecontent #hardwareparamsmodeldenkovitcpdevices #denkovislaveid").show();
										$("#hardwarecontent #hardwareparamsmodeldenkovitcpdevices #denkovislaveid").val(data["Mode3"]);
									} else {
										$("#hardwarecontent #hardwareparamsmodeldenkovitcpdevices #lbldenkovislaveid").hide();
										$("#hardwarecontent #hardwareparamsmodeldenkovitcpdevices #denkovislaveid").hide();
									}
								}
							}
						}
						else if ((data["Type"].indexOf("Underground") >= 0) || (data["Type"].indexOf("DarkSky") >= 0) || (data["Type"].indexOf("AccuWeather") >= 0)) {
							$("#hardwarecontent #hardwareparamsunderground #apikey").val(data["Username"]);
							$("#hardwarecontent #hardwareparamsunderground #location").val(data["Password"]);
						}
						else if ((data["Type"].indexOf("Meteorologisk") >= 0)) {
							$("#hardwarecontent #hardwareparamslocation #location").val(data["Password"]);
						}
						else if (data["Type"].indexOf("Open Weather Map") >= 0) {
							$("#hardwarecontent #hardwareparamsopenweathermap #apikey").val(data["Username"]);
							$("#hardwarecontent #hardwareparamsopenweathermap #location").val(data["Password"]);
							$("#hardwarecontent #hardwareparamsopenweathermap #adddayforecast").prop("checked", data["Mode1"] == 1);
							$("#hardwarecontent #hardwareparamsopenweathermap #addhourforecast").prop("checked", data["Mode2"] == 1);
						}
						else if (data["Type"].indexOf("Buienradar") >= 0) {
							var timeframe = parseInt(data["Mode1"]);
							var threshold = parseInt(data["Mode2"]);
							if (timeframe == 0) timeframe = 15;
							if (threshold == 0) threshold = 25;
							$("#hardwarecontent #divbuienradar #timeframe").val(timeframe);
							$("#hardwarecontent #divbuienradar #threshold").val(threshold);
							$("#hardwarecontent #divbuienradar #location").val(data["Password"]);
						}
						else if ((data["Type"].indexOf("HTTP/HTTPS") >= 0)) {
							$("#hardwarecontent #hardwareparamshttp #url").val(data["Address"]);
							var tmp = data["Extra"];
							var tmparray = tmp.split("|");
							if (tmparray.length >= 4) {
								$("#hardwarecontent #hardwareparamshttp #script").val(atob(tmparray[0]));
								$("#hardwarecontent #hardwareparamshttp #combomethod").val(atob(tmparray[1]));
								$("#hardwarecontent #hardwareparamshttp #contenttype").val(atob(tmparray[2]));
								$("#hardwarecontent #hardwareparamshttp #headers").val(atob(tmparray[3]));
								if (tmparray.length >= 5) {
									$("#hardwarecontent #hardwareparamshttp #postdata").val(atob(tmparray[4]));
								}
								if (atob(tmparray[1]) == 1) {
									$("#hardwarecontent #hardwareparamshttp #divpostdatalabel").show();
									$("#hardwarecontent #hardwareparamshttp #divpostdatatextarea").show();
								}
								else {
									$("#hardwarecontent #hardwareparamshttp #divpostdatalabel").hide();
									$("#hardwarecontent #hardwareparamshttp #divpostdatatextarea").hide();
								}
							}
							$("#hardwarecontent #hardwareparamshttp #refresh").val(data["IntPort"]);
						}
						else if (data["Type"].indexOf("SBFSpot") >= 0) {
							$("#hardwarecontent #hardwareparamslocation #location").val(data["Username"]);
						}
						else if (data["Type"].indexOf("SolarEdge via") >= 0) {
							$("#hardwarecontent #hardwareparamssolaredgeapi #apikey").val(data["Username"]);
						}
						else if (data["Type"].indexOf("Nest Th") >= 0 && data["Type"].indexOf("OAuth") >= 0) {
						    $("#hardwarecontent #hardwareparamsnestoauthapi #apikey").val(data["Username"]);

						    var tmp = data["Extra"];
						    var tmparray = tmp.split("|");
						    if (tmparray.length == 3) {
						        $("#hardwarecontent #divnestoauthapi #productid").val(atob(tmparray[0]));
						        $("#hardwarecontent #divnestoauthapi #productsecret").val(atob(tmparray[1]));
						        $("#hardwarecontent #divnestoauthapi #productpin").val(atob(tmparray[2]));
						    }
						}
						else if (data["Type"].indexOf("Toon") >= 0) {
							$("#hardwarecontent #hardwareparamsenecotoon #agreement").val(data["Mode1"]);
						}
						else if (data["Type"].indexOf("Tesla") >= 0) {
							$("#hardwarecontent #hardwareparamstesla #vinnr").val(data["Extra"]);
							$("#hardwarecontent #hardwareparamstesla #defaultinterval").val(data["Mode1"]);
							$("#hardwarecontent #hardwareparamstesla #activeinterval").val(data["Mode2"]);
							$("#hardwarecontent #hardwareparamstesla #comboallowwakeup").val(data["Mode3"]);
						}
						else if (data["Type"].indexOf("Mercedes") >= 0) {
							$("#hardwarecontent #hardwareparamsmercedes #vinnr").val(data["Extra"]);
							$("#hardwarecontent #hardwareparamsmercedes #defaultinterval").val(data["Mode1"]);
							$("#hardwarecontent #hardwareparamsmercedes #activeinterval").val(data["Mode2"]);
							$("#hardwarecontent #hardwareparamsmercedes #comboallowwakeup").val(data["Mode3"]);
						}
						else if (data["Type"].indexOf("Satel Integra") >= 0) {
							$("#hardwarecontent #hardwareparamspollinterval #pollinterval").val(data["Mode1"]);
						}
						else if (data["Type"].indexOf("eHouse") >= 0) {
                            $("#hardwarecontent #hardwareparamspollinterval #pollinterval").val(data["Mode1"]);
					 		$('#hardwarecontent #hardwareparamsehouse #ehouseautodiscovery').prop("checked",data["Mode2"] == 1);
							$('#hardwarecontent #hardwareparamsehouse #ehouseaddalarmin').prop("checked",data["Mode3"] == 1);
							$('#hardwarecontent #hardwareparamsehouse #ehouseprodiscovery').prop("checked",data["Mode4"] == 1);
							$('#hardwarecontent #hardwareparamsehouse #ehouseopts').val(data["Mode5"]);
							$('#hardwarecontent #hardwareparamsehouse #ehouseopts2').val(data["Mode6"]);
						}
						else if (data["Type"].indexOf("Relay-Net") >= 0) {
							$("#hardwarecontent #hardwareparamsrelaynet #relaynetpollinputs").prop("checked", data["Mode1"] == 1);
							$("#hardwarecontent #hardwareparamsrelaynet #relaynetpollrelays").prop("checked", data["Mode2"] == 1);
							$("#hardwarecontent #hardwareparamsrelaynet #relaynetpollinterval").val(data["Mode3"]);
							$("#hardwarecontent #hardwareparamsrelaynet #relaynetinputcount").val(data["Mode4"]);
							$("#hardwarecontent #hardwareparamsrelaynet #relaynetrelaycount").val(data["Mode5"]);
						}
						else if (data["Type"].indexOf("Philips Hue") >= 0) {
							$("#hardwarecontent #hardwareparamsremote #tcpaddress").val(data["Address"]);
							$("#hardwarecontent #hardwareparamsremote #tcpport").val(data["Port"]);
							$("#hardwarecontent #hardwareparamsphilipshue #username").val(data["Username"]);
							$("#hardwarecontent #hardwareparamsphilipshue #pollinterval").val(data["Mode1"]);
							$("#hardwarecontent #hardwareparamsphilipshue #addgroups").prop('checked', (data["Mode2"]&1));
							$("#hardwarecontent #hardwareparamsphilipshue #addscenes").prop('checked', (data["Mode2"]&2));
						}
						else if (data["Type"].indexOf("Winddelen") >= 0) {
							$("#hardwarecontent #hardwareparamswinddelen #combomillselect").val(data["Mode1"]);
							$("#hardwarecontent #hardwareparamswinddelen #nrofwinddelen").val(data["Port"]);
						}
						else if (data["Type"].indexOf("Honeywell") >= 0) {
							$("#hardwarecontent #hardwareparamshoneywell #hwAccessToken").val(data["Username"]);
							$("#hardwarecontent #hardwareparamshoneywell #hwRefreshToken").val(data["Password"]);
							var tmp = data["Extra"];
							var tmparray = tmp.split("|");
							if (tmparray.length == 2) {
								$("#hardwarecontent #hardwareparamshoneywell #hwApiKey").val(atob(tmparray[0]));
								$("#hardwarecontent #hardwareparamshoneywell #hwApiSecret").val(atob(tmparray[1]));
							}
						}
						else if (data["Type"].indexOf("Goodwe solar inverter via Web") >= 0) {
							$("#hardwarecontent #hardwareparamsgoodweweb #comboserverselect").val(data["Mode1"]);
							$("#hardwarecontent #hardwareparamsgoodweweb #username").val(data["Username"]);
						}
						if (data["Type"].indexOf("MySensors Gateway with MQTT") >= 0) {

							// Break out any possible topic prefix pieces.
							var CAfilenameParts = data["Extra"].split("#");

							// There should be 1 piece or 3 pieces.
							switch (CAfilenameParts.length) {
								case 2:
									console.log("MySensorsMQTT: Truncating CAfilename; Stray topic was present.");
									$("#hardwarecontent #hardwareparamsmysensorsmqtt #filename").val(CAfilenameParts[0]);
									$("#hardwarecontent #hardwareparamsmysensorsmqtt #mqtttopicin").val("");
									$("#hardwarecontent #hardwareparamsmysensorsmqtt #mqtttopicout").val("");
									break;
								case 1:
								case 0:
									console.log("MySensorsMQTT: Only a CAfilename present.");
									$("#hardwarecontent #hardwareparamsmysensorsmqtt #filename").val(data["Extra"]);
									$("#hardwarecontent #hardwareparamsmysensorsmqtt #mqtttopicin").val("");
									$("#hardwarecontent #hardwareparamsmysensorsmqtt #mqtttopicout").val("");
									break;
								default:
									console.log("MySensorsMQTT: Stacked data in CAfilename present. Separating out topic prefixes.");
									$("#hardwarecontent #hardwareparamsmysensorsmqtt #filename").val(CAfilenameParts[0]);
									$("#hardwarecontent #hardwareparamsmysensorsmqtt #mqtttopicin").val(CAfilenameParts[1]);
									$("#hardwarecontent #hardwareparamsmysensorsmqtt #mqtttopicout").val(CAfilenameParts[2]);
									break;
							}

							$("#hardwarecontent #hardwareparamsmysensorsmqtt #combotopicselect").val(data["Mode1"]);
							$("#hardwarecontent #hardwareparamsmysensorsmqtt #combotlsversion").val(data["Mode2"]);
							$("#hardwarecontent #hardwareparamsmysensorsmqtt #combopreventloop").val(data["Mode3"]);
						}
						else if (data["Type"].indexOf("MQTT") >= 0) {
							$("#hardwarecontent #hardwareparamsmqtt #filename").val("");
							$("#hardwarecontent #divmqtt #mqtttopicin").val("");
							$("#hardwarecontent #divmqtt #mqtttopicout").val("");

							// Break out any possible topic prefix pieces.
							var CAfilenameParts = data["Extra"].split(";");
							if (CAfilenameParts.length > 0)
								$("#hardwarecontent #hardwareparamsmqtt #filename").val(CAfilenameParts[0]);
							if (CAfilenameParts.length > 1)
								$("#hardwarecontent #hardwareparamsmqtt #mqtttopicin").val(CAfilenameParts[1]);
							if (CAfilenameParts.length > 2)
								$("#hardwarecontent #hardwareparamsmqtt #mqtttopicout").val(CAfilenameParts[2]);
						
							$("#hardwarecontent #hardwareparamsmqtt #combotopicselect").val(data["Mode1"]);
							$("#hardwarecontent #hardwareparamsmqtt #combotlsversion").val(data["Mode2"]);
							$("#hardwarecontent #hardwareparamsmqtt #combopreventloop").val(data["Mode3"]);
						}
						else if (data["Type"].indexOf("Rtl433") >= 0) {
							$("#hardwarecontent #hardwareparamsrtl433 #rtl433cmdline").val(data["Extra"]);
						}
						else if (data["Type"].indexOf("AirconWithMe") >= 0) {
							$("#hardwarecontent #hardwareparamsremote #tcpaddress").val(data["Address"]);
							$("#hardwarecontent #hardwareparamslogin #username").val(data["Username"]);
							$("#hardwarecontent #hardwareparamslogin #password").val(data["Password"]);
							
						}
						if (
							(data["Type"].indexOf("Domoticz") >= 0) ||
							(data["Type"].indexOf("ICY") >= 0) ||
							(data["Type"].indexOf("Eco Devices") >= 0) ||
							(data["Type"].indexOf("Toon") >= 0) ||
							(data["Type"].indexOf("Atag") >= 0) ||
							(data["Type"].indexOf("Nest Th") >= 0 && data["Type"].indexOf("OAuth") === -1) ||
							(data["Type"].indexOf("PVOutput") >= 0) ||
							(data["Type"].indexOf("ETH8020") >= 0) ||
							(data["Type"].indexOf("Daikin") >= 0) ||
							(data["Type"].indexOf("Sterbox") >= 0) ||
							(data["Type"].indexOf("Anna") >= 0) ||
							(data["Type"].indexOf("KMTronic") >= 0) ||
							(data["Type"].indexOf("MQTT") >= 0) ||
							(data["Type"].indexOf("Netatmo") >= 0) ||
							(data["Type"].indexOf("HTTP") >= 0) ||
							(data["Type"].indexOf("Thermosmart") >= 0) ||
                            (data["Type"].indexOf("Tado") >= 0) ||
                            (data["Type"].indexOf("Tesla") >= 0) ||
                            (data["Type"].indexOf("Mercedes") >= 0) ||
							(data["Type"].indexOf("Logitech Media Server") >= 0) ||
							(data["Type"].indexOf("HEOS by DENON") >= 0) ||
							(data["Type"].indexOf("Razberry") >= 0) ||
							(data["Type"].indexOf("Comm5") >= 0)
						) {
							$("#hardwarecontent #hardwareparamslogin #username").val(data["Username"]);
							$("#hardwarecontent #hardwareparamslogin #password").val(data["Password"]);
						}
						if (data["Type"].indexOf("Evohome via Web") >= 0) {
							$("#hardwarecontent #hardwareparamslogin #username").val(data["Username"]);
							$("#hardwarecontent #hardwareparamslogin #password").val(data["Password"]);

							var Pollseconds = parseInt(data["Mode1"]);
							if ( Pollseconds < 10 ) {
								Pollseconds = 60;
							}
							$("#hardwarecontent #divevohomeweb #updatefrequencyevohomeweb").val(Pollseconds);

							var UseFlags = parseInt(data["Mode2"]);
							$("#hardwarecontent #divevohomeweb #disableautoevohomeweb").prop("checked",((UseFlags & 1) ^ 1));
							$("#hardwarecontent #divevohomeweb #showscheduleevohomeweb").prop("checked",((UseFlags & 2) >>> 1));
							$("#hardwarecontent #divevohomeweb #showlocationevohomeweb").prop("checked",((UseFlags & 4) >>> 2));
							$("#hardwarecontent #divevohomeweb #comboevoprecision").val((UseFlags & 24));

							var Location = parseInt(data["Mode3"]);
							for (var i=1;i<10;i++){
								$("#hardwarecontent #divevohomeweb #comboevolocation")[0].options[i]=new Option(i,i);
								$("#hardwarecontent #divevohomeweb #comboevogateway")[0].options[i]=new Option(i,i);
								$("#hardwarecontent #divevohomeweb #comboevotcs")[0].options[i]=new Option(i,i);
							}
							$("#hardwarecontent #divevohomeweb #comboevolocation").val(Location >>> 12);
							$("#hardwarecontent #divevohomeweb #comboevogateway").val((Location >>> 8) & 15);
							$("#hardwarecontent #divevohomeweb #comboevotcs").val((Location >>> 4) & 15);
						}
					}
				}
			});

			$('#modal').hide();
		}

		RegisterPhilipsHue = function () {
			var address = $("#hardwarecontent #divremote #tcpaddress").val();
			if (address == "") {
				ShowNotify($.t('Please enter an Address!'), 2500, true);
				return;
			}
			var port = $("#hardwarecontent #divremote #tcpport").val();
			if (port == "") {
				ShowNotify($.t('Please enter an Port!'), 2500, true);
				return;
			}
			var username = $("#hardwarecontent #hardwareparamsphilipshue #username").val();
			$.ajax({
				url: "json.htm?type=command&param=registerhue" +
				"&ipaddress=" + address +
				"&port=" + port +
				"&username=" + encodeURIComponent(username),
				async: false,
				dataType: 'json',
				success: function (data) {
					if (data.status == "ERR") {
						ShowNotify(data.statustext, 2500, true);
						return;
					}
					$("#hardwarecontent #hardwareparamsphilipshue #username").val(data.username)
					ShowNotify($.t('Registration successful!'), 2500);
				},
				error: function () {
					HideNotify();
					ShowNotify($.t('Problem registrating with the Philips Hue bridge!'), 2500, true);
				}
			});
		}

		//credits: http://www.netlobo.com/url_query_string_javascript.html
		function gup(url, name) {
			name = name.replace(/[[]/, "\[").replace(/[]]/, "\]");
			var regexS = "[\?&]" + name + "=([^&#]*)";
			var regex = new RegExp(regexS);
			var results = regex.exec(url);
			if (results == null)
				return "";
			else
				return results[1];
		}

		function validateToonToken(token) {
			alert(code);
		}

		OnAuthenticateToon = function () {
			var pwidth = 800;
			var pheight = 600;

			var dualScreenLeft = window.screenLeft != undefined ? window.screenLeft : screen.left;
			var dualScreenTop = window.screenTop != undefined ? window.screenTop : screen.top;

			var width = window.innerWidth ? window.innerWidth : document.documentElement.clientWidth ? document.documentElement.clientWidth : screen.width;
			var height = window.innerHeight ? window.innerHeight : document.documentElement.clientHeight ? document.documentElement.clientHeight : screen.height;

			var left = ((width / 2) - (pwidth / 2)) + dualScreenLeft;
			var top = ((height / 2) - (pheight / 2)) + dualScreenTop;

			var REDIRECT = 'http://127.0.0.1/domoticiz_toon';
			var CLIENT_ID = '7gQMPclYzm8haCHAgdvjq1yILLwa';
			var _url = 'https://api.toonapi.com/authorize?response_type=code&redirect_uri=' + REDIRECT + '&client_id=' + CLIENT_ID;
			//_url = "http://127.0.0.1:8081/11";
			var win = window.open(_url, "windowtoonaith", 'scrollbars=yes, width=' + pwidth + ', height=' + pheight + ', left=' + left + ', top=' + top);
			if (window.focus) {
				win.focus();
			}
			var pollTimer = window.setInterval(function () {
				if (win.closed !== false) { // !== is required for compatibility with Opera
					window.clearInterval(pollTimer);
				}
				else if (win.document.URL.indexOf(REDIRECT) != -1) {
					window.clearInterval(pollTimer);
					console.log(win.document.URL);
					var url = win.document.URL;
					var code = gup(url, 'code');
					win.close();
					validateToonToken(code);
				}
			}, 200);
		}

		UpdateHardwareParamControls = function () {
			$("#hardwarecontent #hardwareparamstable #enabled").prop('disabled', false);
			$("#hardwarecontent #hardwareparamstable #hardwarename").prop('disabled', false);
			$("#hardwarecontent #hardwareparamstable #combotype").prop('disabled', false);
			$("#hardwarecontent #hardwareparamstable #combodatatimeout").prop('disabled', false);
			$('#hardwarecontent #hardwareparamstable #loglevelInfo').prop('checked', true);
			$('#hardwarecontent #hardwareparamstable #loglevelStatus').prop('checked', true);
			$('#hardwarecontent #hardwareparamstable #loglevelError').prop('checked', true);

			var text = $("#hardwarecontent #hardwareparamstable #combotype option:selected").text();
			$("#hardwarecontent #username").show();
			$("#hardwarecontent #lblusername").show();
			$("#hardwarecontent #divehouse").hide();
			$("#hardwarecontent #divevohome").hide();
			$("#hardwarecontent #divevohometcp").hide();
			$("#hardwarecontent #divevohomeweb").hide();
			$("#hardwarecontent #divusbtin").hide();
			$("#hardwarecontent #divbaudratemysensors").hide();
			$("#hardwarecontent #divbaudratep1").hide();
			$("#hardwarecontent #divbaudrateteleinfo").hide();
			$("#hardwarecontent #divmodelecodevices").hide();
			$("#hardwarecontent #divcrcp1").hide();
			$("#hardwarecontent #divratelimitp1").hide();
			$("#hardwarecontent #divkeyp1p1").hide();
			$("#hardwarecontent #divensynchro").hide();
			$("#hardwarecontent #divlocation").hide();
			$("#hardwarecontent #divphilipshue").hide();
			$("#hardwarecontent #divwinddelen").hide();
			$("#hardwarecontent #divhoneywell").hide();
			$("#hardwarecontent #divmqtt").hide();
			$("#hardwarecontent #divmysensorsmqtt").hide();
			$("#hardwarecontent #divsolaredgeapi").hide();
			$("#hardwarecontent #divnestoauthapi").hide();
			$("#hardwarecontent #divenecotoon").hide();
			$("#hardwarecontent #divtesla").hide();
			$("#hardwarecontent #divmercedes").hide();
			$("#hardwarecontent #div1wire").hide();
			$("#hardwarecontent #divgoodweweb").hide();
			$("#hardwarecontent #divi2clocal").hide();
			$("#hardwarecontent #divi2caddress").hide();
			$("#hardwarecontent #divi2cinvert").hide();
			$("#hardwarecontent #divpollinterval").hide();
			$("#hardwarecontent #divpythonplugin").hide();
			$("#hardwarecontent #divrelaynet").hide();
			$("#hardwarecontent #divgpio").hide();
			$("#hardwarecontent #divsysfsgpio").hide();
			$("#hardwarecontent #divmodeldenkovidevices").hide();
            $("#hardwarecontent #divmodeldenkoviusbdevices").hide();
            $("#hardwarecontent #divmodeldenkovitcpdevices").hide();
			$("#hardwarecontent #divunderground").hide();
			$("#hardwarecontent #divopenweathermap").hide();
			$("#hardwarecontent #divbuienradar").hide();
			$("#hardwarecontent #divserial").hide();
			$("#hardwarecontent #divremote").hide();
			$("#hardwarecontent #divlogin").hide();
			$("#hardwarecontent #divhttppoller").hide();

			// Handle plugins 1st because all the text indexof logic below will have unpredictable impacts for plugins
			// Python Plugins have the plugin name, not the hardware type id, as the value
			if (!$.isNumeric($("#hardwarecontent #hardwareparamstable #combotype option:selected").val())) {
				$("#hardwarecontent #divpythonplugin .plugin").hide();
				var plugin = $("#hardwarecontent #hardwareparamstable #combotype option:selected").attr("id");
				$("#hardwarecontent #divpythonplugin .plugin").each(function () { if ($(this).attr("id") === plugin) $(this).show(); });
				$("#hardwarecontent #divpythonplugin").show();
				return;
			}

			if (text.indexOf("eHouse") >= 0) {
				$("#hardwarecontent #divehouse").show();
			}
			else if (text.indexOf("I2C ") >= 0) {
				$("#hardwarecontent #divi2clocal").show();
				$("#hardwarecontent #divi2caddress").hide();
				$("#hardwarecontent #divi2cinvert").hide();
				var text1 = $("#hardwarecontent #divi2clocal #hardwareparamsi2clocal #comboi2clocal option:selected").text();
				if (text1.indexOf("I2C sensor PIO 8bit expander PCF8574") >= 0) {
					$("#hardwarecontent #divi2caddress").show();
					$("#hardwarecontent #divi2cinvert").show();
				}
				else if (text1.indexOf("I2C sensor GPIO 16bit expander MCP23017") >= 0) {
					$("#hardwarecontent #divi2caddress").show();
					$("#hardwarecontent #divi2cinvert").show();
				}
			}
			else if ((text.indexOf("GPIO") >= 0) && (text.indexOf("sysfs GPIO") == -1)) {
				$("#hardwarecontent #divgpio").show();
			}
			else if (text.indexOf("sysfs GPIO") >= 0) {
				$("#hardwarecontent #divsysfsgpio").show();
			}
			else if (text.indexOf("USB") >= 0 || text.indexOf("Teleinfo EDF") >= 0) {
				if (text.indexOf("Evohome") >= 0) {
					$("#hardwarecontent #divevohome").show();
				}
				if (text.indexOf("MySensors") >= 0) {
					$("#hardwarecontent #divbaudratemysensors").show();
				}
				if (text.indexOf("P1 Smart Meter") >= 0) {
					$("#hardwarecontent #divbaudratep1").show();
					$("#hardwarecontent #divratelimitp1").show();
					$("#hardwarecontent #divkeyp1p1").show();
					$("#hardwarecontent #divcrcp1").show();
				}
				if (text.indexOf("Teleinfo EDF") >= 0) {
					$("#hardwarecontent #divbaudrateteleinfo").show();
					$("#hardwarecontent #divratelimitp1").show();
					$("#hardwarecontent #divcrcp1").show();
				}
				if (text.indexOf("USBtin") >= 0){
					$("#hardwarecontent #divusbtin").show();
				}
                if (text.indexOf("Denkovi") >= 0) {
                    $("#hardwarecontent #divmodeldenkoviusbdevices").show();
                }
				$("#hardwarecontent #divserial").show();
			}
			else if (
				(text.indexOf("LAN") >= 0 ||
				text.indexOf("Harmony") >= 0 ||
				text.indexOf("Eco Devices") >= 0 ||
				text.indexOf("MySensors Gateway with MQTT") >= 0) &&
				text.indexOf("YouLess") == -1 && 
				text.indexOf("Denkovi") == -1 &&
				text.indexOf("Relay-Net") == -1 &&
				text.indexOf("Satel Integra") == -1 &&
				text.indexOf("eHouse") == -1 &&
				text.indexOf("MyHome OpenWebNet with LAN interface") == -1) {
					$("#hardwarecontent #divremote").show();
					if (text.indexOf("Eco Devices") >= 0) {
						$("#hardwarecontent #divmodelecodevices").show();
						$("#hardwarecontent #divratelimitp1").show();
						$("#hardwarecontent #divlogin").show();
					}
					if (text.indexOf("P1 Smart Meter") >= 0) {
						$("#hardwarecontent #divratelimitp1").show();
						$("#hardwarecontent #divcrcp1").show();
						$("#hardwarecontent #divkeyp1p1").show();
					}
					if (text.indexOf("Evohome") >= 0) {
						$("#hardwarecontent #divevohometcp").show();
					}
			}
			else if (
					(text.indexOf("LAN") >= 0 || text.indexOf("MySensors Gateway with MQTT") >= 0) &&
					(text.indexOf("YouLess") >= 0 ||
					text.indexOf("Denkovi") >= 0 ||
					text.indexOf("Relay-Net") >= 0 ||
					text.indexOf("Satel Integra") >= 0) ||
					text.indexOf("eHouse") >= 0 ||
					(text.indexOf("Xiaomi Gateway") >= 0) || text.indexOf("MyHome OpenWebNet with LAN interface") >= 0) {
						$("#hardwarecontent #divremote").show();
						$("#hardwarecontent #divlogin").show();

						if (text.indexOf("Relay-Net") >= 0) {
							$("#hardwarecontent #username").show();
							$("#hardwarecontent #lblusername").show();
							$("#hardwarecontent #password").show();
							$("#hardwarecontent #lblpassword").show();
							$("#hardwarecontent #divrelaynet").show();
						}
						else if (text.indexOf("Satel Integra") >= 0) {
							$("#hardwarecontent #divpollinterval").show();
							$("#hardwarecontent #hardwareparamspollinterval #pollinterval").val(1000);
						}
						else if (text.indexOf("eHouse") >= 0) {
							$("#hardwarecontent #divpollinterval").show();
							$("#hardwarecontent #hardwareparamspollinterval #pollinterval").val(1000);
							//$("#hardwarecontent #password").show();
							//$("#hardwarecontent #lblpassword").show();
							$("#hardwarecontent #divehouse").show();
						}
						else if (text.indexOf("MyHome OpenWebNet with LAN interface") >= 0) {
							$("#hardwarecontent #divratelimitp1").show();
							$("#hardwarecontent #divensynchro").show();
							$("#hardwarecontent #hardwareparamsremote #tcpport").val(20000);
							$("#hardwarecontent #hardwareparamsratelimitp1 #ratelimitp1").val(0);
							$("#hardwarecontent #hardwareparamsensynchro #ensynchro").val(0);
						}
						else if (text.indexOf("Denkovi") >= 0) {
							$("#hardwarecontent #divpollinterval").show();
							$("#hardwarecontent #hardwareparamspollinterval #pollinterval").val(10000);
							if (text.indexOf("Modules with LAN (HTTP)") >= 0){
								$("#hardwarecontent #divmodeldenkovidevices").show();
								$("#hardwarecontent #password").show();
								$("#hardwarecontent #lblpassword").show();
							}
							else if (text.indexOf("Modules with LAN (TCP)") >= 0) {
								$("#hardwarecontent #divmodeldenkovitcpdevices").show();
								var board = $("#hardwarecontent #divmodeldenkovitcpdevices #combomodeldenkovitcpdevices option:selected").val();
								if (board == 0) {
									$("#hardwarecontent #hardwareparamsmodeldenkovitcpdevices #lbldenkovislaveid").hide();
									$("#hardwarecontent #hardwareparamsmodeldenkovitcpdevices #denkovislaveid").hide();
								}
								else {
									$("#hardwarecontent #hardwareparamsmodeldenkovitcpdevices #lbldenkovislaveid").show();
									$("#hardwarecontent #hardwareparamsmodeldenkovitcpdevices #denkovislaveid").show();
								}
								$("#hardwarecontent #password").hide();
								$("#hardwarecontent #lblpassword").hide();
							}							
							$("#hardwarecontent #username").hide();
							$("#hardwarecontent #lblusername").hide();
						}
			}
			else if (text.indexOf("Domoticz") >= 0) {
				$("#hardwarecontent #divremote").show();
				$("#hardwarecontent #divlogin").show();
				$("#hardwarecontent #hardwareparamsremote #tcpport").val(6144);
			}
			else if (text.indexOf("SolarEdge via") >= 0) {
				$("#hardwarecontent #divsolaredgeapi").show();
			}
			else if (text.indexOf("Nest Th") >= 0 && text.indexOf("OAuth") >= 0) {
			    $("#hardwarecontent #divnestoauthapi").show();
			}
			else if (text.indexOf("Toon") >= 0) {
				$("#hardwarecontent #divlogin").show();
				$("#hardwarecontent #divenecotoon").show();
			}
			else if (text.indexOf("Tesla") >= 0) {
				$("#hardwarecontent #divlogin").show();
				$("#hardwarecontent #divtesla").show();
			}
			else if (text.indexOf("Mercedes") >= 0) {
				$("#hardwarecontent #divlogin").show();
				$("#hardwarecontent #divmercedes").show();
			}
			else if (text.indexOf("SBFSpot") >= 0) {
				$("#hardwarecontent #divlocation").show();
			}
			else if (
				(text.indexOf("ICY") >= 0) ||
				(text.indexOf("Atag") >= 0) ||
				(text.indexOf("Nest Th") >= 0 && text.indexOf("OAuth") === -1) ||
				(text.indexOf("PVOutput") >= 0) ||
				(text.indexOf("Netatmo") >= 0) ||
				(text.indexOf("Thermosmart") >= 0) ||
                (text.indexOf("Tado") >= 0)
			) {
				$("#hardwarecontent #divlogin").show();
			}
			else if (text.indexOf("HTTP") >= 0) {
				$("#hardwarecontent #divlogin").show();
				$("#hardwarecontent #divhttppoller").show();

				var method = $("#hardwarecontent #divhttppoller #combomethod option:selected").val();
				if (method == 0) {
					$("#hardwarecontent #hardwareparamshttp #divpostdatalabel").hide();
					$("#hardwarecontent #hardwareparamshttp #divpostdatatextarea").hide();
				}
				else {
					$("#hardwarecontent #hardwareparamshttp #divpostdatalabel").show();
					$("#hardwarecontent #hardwareparamshttp #divpostdatatextarea").show();
				}
			}
			else if ((text.indexOf("Underground") >= 0) || (text.indexOf("DarkSky") >= 0) || (text.indexOf("AccuWeather") >= 0)) {
				$("#hardwarecontent #divunderground").show();
			}
			else if(text.indexOf("Meteorologisk") >= 0){
				$("#hardwarecontent #divlocation").show();
			}
			else if(text.indexOf("Open Weather Map") >= 0){
				$("#hardwarecontent #divopenweathermap").show();
			}
			else if (text.indexOf("Buienradar") >= 0) {
				$("#hardwarecontent #divbuienradar").show();
			}
			else if (text.indexOf("Philips Hue") >= 0) {
				$("#hardwarecontent #divremote").show();
				$("#hardwarecontent #divphilipshue").show();
			}
			else if (text.indexOf("Yeelight") >= 0) {
			}
			else if (text.indexOf("Arilux AL-LC0x") >= 0) {
			}
			else if (text.indexOf("Winddelen") >= 0) {
				$("#hardwarecontent #divwinddelen").show();
			}
			else if (text.indexOf("Honeywell") >= 0) {
				$("#hardwarecontent #divhoneywell").show();
			}
			else if (text.indexOf("Logitech Media Server") >= 0) {
				$("#hardwarecontent #divremote").show();
				$("#hardwarecontent #divlogin").show();
				$("#hardwarecontent #hardwareparamsremote #tcpport").val(9000);
			}
			else if (text.indexOf("HEOS by DENON") >= 0) {
				$("#hardwarecontent #divremote").show();
				$("#hardwarecontent #divlogin").show();
				$("#hardwarecontent #hardwareparamsremote #tcpport").val(1255);
			}
			else if (text.indexOf("MyHome OpenWebNet") >= 0) {
				$("#hardwarecontent #divremote").show();
				$("#hardwarecontent #hardwareparamsremote #tcpport").val(20000);
			}
			else if (text.indexOf("1-Wire") >= 0) {
				$("#hardwarecontent #div1wire").show();
			}
			else if (text.indexOf("Goodwe solar inverter via Web") >= 0) {
				$("#hardwarecontent #divgoodweweb").show();
			}
			else if (text.indexOf("Evohome via Web") >= 0) {
				$("#hardwarecontent #divevohomeweb").show();
				$("#hardwarecontent #divlogin").show();
			}
			else if (text.indexOf("AirconWithMe") >= 0) {
				$("#hardwarecontent #divremote").show();
				$("#hardwarecontent #divremote #lblremoteport").hide();
				$("#hardwarecontent #divremote #tcpport").hide();
				$("#hardwarecontent #divlogin #username").val("operator")
				$("#hardwarecontent #divlogin #password").val("operator")
				$("#hardwarecontent #divlogin").show();
			}
			if (
				(text.indexOf("ETH8020") >= 0) ||
				(text.indexOf("Daikin") >= 0) ||
				(text.indexOf("Sterbox") >= 0) ||
				(text.indexOf("Anna") >= 0) ||
				(text.indexOf("MQTT") >= 0) ||
				(text.indexOf("KMTronic Gateway with LAN") >= 0) ||
				(text.indexOf("Razberry") >= 0)
			) {
				$("#hardwarecontent #divlogin").show();
			}
			if (text.indexOf("Rtl433") >= 0) {
				$("#hardwarecontent #divrtl433").show();
			} else {
				$("#hardwarecontent #divrtl433").hide();
			}
			if (text.indexOf("MySensors Gateway with MQTT") >= 0) {
				$("#hardwarecontent #divmysensorsmqtt").show();
			}
			else if (text.indexOf("MQTT") >= 0) {
			    $("#hardwarecontent #divmqtt").show();
			    if (
					(text.indexOf("The Things Network (MQTT") >= 0)
					||(text.indexOf("OctoPrint") >= 0)
					) {
			        $("#hardwarecontent #divmqtt #mqtt_publish").hide();
			    }
			    else {
			        $("#hardwarecontent #divmqtt #mqtt_publish").show();
			    }
			}
		}

		ShowHardware = function () {
			$('#modal').show();
			var htmlcontent = "";
			htmlcontent += $('#hardwaremain').html();
			$('#hardwarecontent').html(htmlcontent);
			$('#hardwarecontent').i18n();
			var oTable = $('#hardwaretable').dataTable({
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


			$("#hardwarecontent #hardwareparamstable #combotype").change(function () {
				UpdateHardwareParamControls();
			});

			$("#hardwarecontent #hardwareparamsmodeldenkovitcpdevices #combomodeldenkovitcpdevices").change(function () {
				UpdateHardwareParamControls();
			});

			$("#hardwarecontent #divi2clocal #hardwareparamsi2clocal #comboi2clocal").change(function () {
				UpdateHardwareParamControls();
			});


			$("#hardwarecontent #divbaudratep1 #combobaudratep1").change(function () {
				if ($("#hardwarecontent #divbaudratep1 #combobaudratep1 option:selected").val() == 0) {
					$("#hardwarecontent #divcrcp1 #disablecrcp1").prop("checked", 0);
					$("#hardwarecontent #divcrcp1").hide();
				}
				else {
					$("#hardwarecontent #divcrcp1 #disablecrcp1").prop("checked", 1);
					$("#hardwarecontent #divcrcp1").show();
				}
			});

			$("#hardwarecontent #divhttppoller #combomethod").change(function () {
				if ($("#hardwarecontent #divhttppoller #combomethod option:selected").val() == 0) {
					$("#hardwarecontent #hardwareparamshttp #divpostdatalabel").hide();
					$("#hardwarecontent #hardwareparamshttp #divpostdatatextarea").hide();
				}
				else {
					$("#hardwarecontent #hardwareparamshttp #divpostdatalabel").show();
					$("#hardwarecontent #hardwareparamshttp #divpostdatatextarea").show();
				}
			});

			$('#modal').hide();
			RefreshHardwareTable();
			UpdateHardwareParamControls();
		}



		function SortByName(a, b) {
			var aName = a.name.toLowerCase();
			var bName = b.name.toLowerCase();
			return ((aName < bName) ? -1 : ((aName > bName) ? 1 : 0));
		}

		function init() {
			//global var
			$.devIdx = 0;
			$.myglobals = {
				HardwareTypesStr: [],
				HardwareI2CStr: [],
				SelectedHardwareIdx: 0
			};
			$scope.SerialPortStr = [];
			$scope.MakeGlobalConfig();

			//Get Serial devices
			$("#hardwareparamsserial #comboserialport").html("");
			$.ajax({
				url: "json.htm?type=command&param=serial_devices",
				async: false,
				dataType: 'json',
				success: function (data) {
					if (typeof data.result != 'undefined') {
						$.each(data.result, function (i, item) {
							var option = $('<option />');
							option.attr('value', item.value).text(item.name);
							$("#hardwareparamsserial #comboserialport").append(option);
						});
					}
				}
			});

			$('#hardwareparamsserial #comboserialport > option').each(function () {
				$scope.SerialPortStr.push($(this).text());
			});

			//Get hardware types
			$("#hardwareparamstable #combotype").html("");
			$.ajax({
				url: "json.htm?type=command&param=gethardwaretypes",
				async: false,
				dataType: 'json',
				success: function (data) {
					if (typeof data.result != 'undefined') {
						data.result.sort(SortByName);  // Plugins will not be in order so sort the array
						var i2cidx = 0, idx = 0;
						$.each(data.result, function (i, item) {
							$.myglobals.HardwareTypesStr[item.idx] = item.name;
							// Don't show I2C sensors
							if (item.name.indexOf("I2C sensor") != -1) {
								$.myglobals.HardwareI2CStr[item.idx] = item.name;
								i2cidx = idx;
								return true;
							}
							// Show other sensors
							var option = $('<option />');
							if (item.idx != 94) {
								option.attr('value', item.idx).text(item.name);
							}
							else {  // For Python Plugins build the input fields
								option.attr('value', item.key).text(item.name);
								option.attr('id', item.key).text(item.name);
								var PluginParams = '<table class="display plugin" id="' + item.key + '" border="0" cellpadding="0" cellspacing="20"><tr><td> </td></tr>';
								if (item.wikiURL.length > 0) {
									PluginParams += '<tr><td align="right" style="width:110px"><span data-i18n="Wiki URL">Wiki URL</span>:</td>' +
										'<td><a href="' + item.wikiURL + '" target="_blank">' + item.wikiURL + '</a></td></tr>';
								}
								if (item.externalURL.length > 0) {
									PluginParams += '<tr><td align="right" style="width:110px"><span data-i18n="Product URL">Product URL</span>:</td>' +
										'<td><a href="' + item.externalURL + '" target="_blank">' + item.externalURL + '</a></td></tr>';
								}
								if (item.description.length > 0) {
									PluginParams += '<tr><td></td><td>' + item.description + '</td></tr>';
								}
								$.each(item.parameters, function (i, param) {
									PluginParams += '<tr><td align="right" style="width:110px"><label id="lbl' + param.field + '"><span data-i18n="' + param.label + '">' + param.label + '</span>:</label></td>';
									if (typeof (param.options) == "undefined") {
										if (param.field == "SerialPort") {
											PluginParams += '<td><select id="' + param.field + '" style="width:' + param.width + '" class="combobox ui-corner-all">';
											$.each($("#hardwareparamsserial #comboserialport > option"), function (i, option) {
												PluginParams += '<option data-i18n="' + option.innerText + '" value="' + option.innerText + '"';
												PluginParams += '>' + option.innerText + '</option>';
											});
											PluginParams += '</select></td>';
										} else {
											PluginParams += '<td>'
											var nbRows=parseInt(param.rows);
											if (nbRows >= 0) {
												PluginParams += '<textarea id="' + param.field + '" style="width:' + param.width + '; padding: .2em;" class="text ui-widget-content ui-corner-all" rows="' + nbRows + '" ';
												if ((typeof (param.required) != "undefined") && (param.required == "true")) PluginParams += 'required';
												PluginParams += '>';
												if (typeof (param.default) != "undefined") PluginParams +=  param.default;
												PluginParams +='</textarea>';
											} else {
												if ((typeof (param.password) != "undefined") && (param.password == "true"))
													PluginParams += '<input type="password" ';
												else
													PluginParams += '<input type="text" ';
												PluginParams += 'id="' + param.field + '" style="width:' + param.width + '; padding: .2em;" class="text ui-widget-content ui-corner-all" '
												if (typeof (param.default) != "undefined") PluginParams += 'value="' + param.default + '"';
												if ((typeof (param.required) != "undefined") && (param.required == "true")) PluginParams += ' required';
  												PluginParams += ' />';
											}
											PluginParams += '</td>';
										}
									}
									else {
										PluginParams += '<td><select id="' + param.field + '" style="width:' + param.width + '" class="combobox ui-corner-all">';
										$.each(param.options, function (i, option) {
											PluginParams += '<option data-i18n="' + option.label + '" value="' + option.value + '"';
											if ((typeof (option.default) != "undefined") && (option.default == "true")) PluginParams += ' selected';
											PluginParams += '>' + option.label + '</option>';
										});
										PluginParams += '</select></td>';
									}
									PluginParams += '</tr>';
								});
								PluginParams += '</table>';
								$("#divpythonplugin").append(PluginParams);
							}
							$("#hardwareparamstable #combotype").append(option);
							idx++;
						});
						// regroup I2C sensors under index 1000
						var option = $('<option />');
						option.attr('value', 1000).text("I2C sensors");
						option.insertAfter('#hardwareparamstable #combotype :nth-child(' + i2cidx + ')')
					}
				}
			});

			//Build I2C devices combo
			$("#hardwareparamsi2clocal #comboi2clocal").html("");
			$.each($.myglobals.HardwareI2CStr, function (idx, name) {
				if (name) {
					var option = $('<option />');
					option.attr('value', idx).text(name);
					$("#hardwareparamsi2clocal #comboi2clocal").append(option);
				}
			});

			ShowHardware();
		}

        $scope.$on('$viewContentLoaded', function(){
            $timeout(init);
        });
	});
});
