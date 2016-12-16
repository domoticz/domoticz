define(['app'], function (app) {
    app.controller('HardwareController', [ '$scope', '$rootScope', '$location', '$http', '$interval', function($scope,$rootScope,$location,$http,$interval) {

        $scope.SerialPortStr=[];
        $scope.ozw_node_id = "-";
        $scope.ozw_node_desc = "-";

        DeleteHardware = function(idx)
        {
            bootbox.confirm($.t("Are you sure to delete this Hardware?\n\nThis action can not be undone...\nAll Devices attached will be removed!"), function(result) {
                if (result==true) {
                    $.ajax({
                         url: "json.htm?type=command&param=deletehardware&idx=" + idx,
                         async: false,
                         dataType: 'json',
                         success: function(data) {
                            RefreshHardwareTable();
                         },
                         error: function(){
                                HideNotify();
                                ShowNotify($.t('Problem deleting hardware!'), 2500, true);
                         }
                    });
                }
            });
        }

        UpdateHardware = function(idx,Mode1,Mode2,Mode3,Mode4,Mode5,Mode6)
        {
            var name=$("#hardwarecontent #hardwareparamstable #hardwarename").val();
            if (name=="")
            {
                ShowNotify($.t('Please enter a Name!'), 2500, true);
                return;
            }
            var hardwaretype=$("#hardwarecontent #hardwareparamstable #combotype option:selected").val();
            if (typeof hardwaretype == 'undefined') {
                ShowNotify($.t('Unknown device selected!'), 2500, true);
                return;
            }

            var bEnabled=$('#hardwarecontent #hardwareparamstable #enabled').is(":checked");

            var datatimeout=$('#hardwarecontent #hardwareparamstable #combodatatimeout').val();

            var text = $("#hardwarecontent #hardwareparamstable #combotype option:selected").text();

            if (text.indexOf("1-Wire") >= 0)
            {
    			var extra=$("#hardwarecontent #div1wire #owfspath").val();
    			var Mode1 = $("#hardwarecontent #div1wire #OneWireSensorPollPeriod").val();
    			var Mode2 = $("#hardwarecontent #div1wire #OneWireSwitchPollPeriod").val();
    			$.ajax({
    				url: "json.htm?type=command&param=updatehardware&htype=" + hardwaretype +
    				"&extra=" + encodeURIComponent(extra) +
    				"&name=" + encodeURIComponent(name) +
    				"&enabled=" + bEnabled +
    				"&idx=" + idx +
    				"&datatimeout=" + datatimeout +
    				"&Mode1=" + Mode1 + "&Mode2=" + Mode2 + "&Mode3=" + Mode3 + "&Mode4=" + Mode4 + "&Mode5=" + Mode5 + "&Mode6=" + Mode6,
    				async: false,
    				dataType: 'json',
    				success: function(data) {
    					RefreshHardwareTable();
    				},
    				error: function(){
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
				(text.indexOf("Local I2C sensor") >= 0) ||
				(text.indexOf("Dummy") >= 0) ||
				(text.indexOf("System Alive") >= 0) ||
				(text.indexOf("PiFace") >= 0) ||
				(text.indexOf("Motherboard") >= 0) ||
				(text.indexOf("Kodi") >= 0) ||
				(text.indexOf("Evohome") >= 0 && text.indexOf("script") >= 0) ||
                (text.indexOf("YeeLight") >= 0)
				)
            {
				// if hardwaretype == 1000 => I2C sensors grouping
				if (hardwaretype == 1000) {
							hardwaretype = $("#hardwareparamsi2clocal #comboi2clocal").find('option:selected').val();
				}

                $.ajax({
                     url: "json.htm?type=command&param=updatehardware&htype=" + hardwaretype +
                        "&name=" + encodeURIComponent(name) +
                        "&enabled=" + bEnabled +
                        "&idx=" + idx +
                        "&datatimeout=" + datatimeout +
                        "&Mode1=" + Mode1 + "&Mode2=" + Mode2 + "&Mode3=" + Mode3 + "&Mode4=" + Mode4 + "&Mode5=" + Mode5 + "&Mode6=" + Mode6,
                     async: false,
                     dataType: 'json',
                     success: function(data) {
                        RefreshHardwareTable();
                     },
                     error: function(){
                            ShowNotify($.t('Problem updating hardware!'), 2500, true);
                     }
                });
            }
            else if (text.indexOf("USB") >= 0)
            {
                var Mode1 = "0";
                var serialport=$("#hardwarecontent #divserial #comboserialport option:selected").text();
                if (typeof serialport == 'undefined')
                {
                    if (bEnabled==true) {
                        ShowNotify($.t('No serial port selected!'), 2500, true);
                        return;
                    }
                    else {
                        serialport="";
                    }
                }

                if (text.indexOf("MySensors") >= 0)
                {
                    var baudrate=$("#hardwarecontent #divbaudratemysensors #combobaudrate option:selected").val();

                    if (typeof baudrate == 'undefined')
                    {
                        ShowNotify($.t('No baud rate selected!'), 2500, true);
                        return;
                    }

                    Mode1 = baudrate;
                }

                if (text.indexOf("P1 Smart Meter") >= 0)
                {
                    var baudrate=$("#hardwarecontent #divbaudratep1 #combobaudratep1 option:selected").val();

                    if (typeof baudrate == 'undefined')
                    {
                        ShowNotify($.t('No baud rate selected!'), 2500, true);
                        return;
                    }

                    Mode1 = baudrate;
                    Mode2 = $("#hardwarecontent #divcrcp1 #disablecrcp1").prop("checked")?0:1;
                }

                var extra="";
                if (text.indexOf("S0 Meter") >= 0)
                {
					extra = $.devExtra;
                }

                if (text.indexOf("P1 Smart Meter") >= 0)
                {
                    Mode2 = $("#hardwarecontent #divcrcp1 #disablecrcp1").prop("checked")?0:1;
                }

                $.ajax({
                     url: "json.htm?type=command&param=updatehardware&htype=" + hardwaretype +
                        "&port=" + encodeURIComponent(serialport) +
                        "&extra=" + extra +
                        "&name=" + encodeURIComponent(name) +
                        "&enabled=" + bEnabled +
                        "&idx=" + idx +
                        "&datatimeout=" + datatimeout +
                        "&Mode1=" + Mode1 + "&Mode2=" + Mode2 + "&Mode3=" + Mode3 + "&Mode4=" + Mode4 + "&Mode5=" + Mode5 + "&Mode6=" + Mode6,
                     async: false,
                     dataType: 'json',
                     success: function(data) {
                        RefreshHardwareTable();
                     },
                     error: function(){
                            ShowNotify($.t('Problem updating hardware!'), 2500, true);
                     }
                });
            }
            else if (
					(text.indexOf("LAN") >= 0 &&
					text.indexOf("YouLess") == -1 &&
					text.indexOf("Denkovi") == -1 &&
					text.indexOf("Satel Integra") == -1 &&
					text.indexOf("ETH8020") == -1 &&
					text.indexOf("Daikin") == -1 &&
					text.indexOf("Sterbox") == -1 &&
					text.indexOf("Anna") == -1 &&
					text.indexOf("KMTronic") == -1  &&
					text.indexOf("MQTT") == -1 &&
					text.indexOf("Razberry") == -1
					)
				)
            {
                var address=$("#hardwarecontent #divremote #tcpaddress").val();
                if (address=="")
                {
                    ShowNotify($.t('Please enter an Address!'), 2500, true);
                    return;
                }
                var port=$("#hardwarecontent #divremote #tcpport").val();
                if (port=="")
                {
                    ShowNotify($.t('Please enter an Port!'), 2500, true);
                    return;
                }
                var intRegex = /^\d+$/;
                if(!intRegex.test(port)) {
                    ShowNotify($.t('Please enter an Valid Port!'), 2500, true);
                    return;
                }
                var extra="";
                if (text.indexOf("S0 Meter") >= 0)
                {
					extra = $.devExtra;
                }

                if (text.indexOf("P1 Smart Meter") >= 0)
                {
                    Mode2 = $("#hardwarecontent #divcrcp1 #disablecrcp1").prop("checked")?0:1;
                }
 
                $.ajax({
                     url: "json.htm?type=command&param=updatehardware&htype=" + hardwaretype +
                        "&address=" + address +
                        "&port=" + port +
                        "&extra=" + extra +
                        "&name=" + encodeURIComponent(name) +
                        "&enabled=" + bEnabled +
                        "&idx=" + idx +
                        "&datatimeout=" + datatimeout +
                        "&Mode1=" + Mode1 + "&Mode2=" + Mode2 + "&Mode3=" + Mode3 + "&Mode4=" + Mode4 + "&Mode5=" + Mode5 + "&Mode6=" + Mode6,
                     async: false,
                     dataType: 'json',
                     success: function(data) {
                        RefreshHardwareTable();
                     },
                     error: function(){
                            ShowNotify($.t('Problem updating hardware!'), 2500, true);
                     }
                });
            }
            else if (
				(text.indexOf("LAN") >= 0 && ((text.indexOf("YouLess") >= 0)||(text.indexOf("Denkovi") >= 0)) ) ||
				(text.indexOf("Satel Integra") >= 0)
				)
            {
                var address=$("#hardwarecontent #divremote #tcpaddress").val();
                if (address=="")
                {
                    ShowNotify($.t('Please enter an Address!'), 2500, true);
                    return;
                }
                var port=$("#hardwarecontent #divremote #tcpport").val();
                if (port=="")
                {
                    ShowNotify($.t('Please enter an Port!'), 2500, true);
                    return;
                }
                var intRegex = /^\d+$/;
                if(!intRegex.test(port)) {
                    ShowNotify($.t('Please enter an Valid Port!'), 2500, true);
                    return;
                }
				
				if (text.indexOf("Satel Integra") >= 0)
                {
					var pollinterval=$("#hardwarecontent #hardwareparamspollinterval #pollinterval").val();
                    if (pollinterval=="")
                    {
                        ShowNotify($.t('Please enter poll interval!'), 2500, true);
                        return;
                    }
                    Mode1 = pollinterval;
                }
                var password=encodeURIComponent($("#hardwarecontent #divlogin #password").val());
                $.ajax({
                     url: "json.htm?type=command&param=updatehardware&htype=" + hardwaretype +
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
                     success: function(data) {
                        RefreshHardwareTable();
                     },
                     error: function(){
                            ShowNotify($.t('Problem updating hardware!'), 2500, true);
                     }
                });
            }
            else if (
				(text.indexOf("Domoticz") >= 0) ||
				(text.indexOf("Harmony") >= 0) ||
				(text.indexOf("ETH8020") >= 0) ||
				(text.indexOf("Daikin") >= 0) ||
				(text.indexOf("Sterbox") >= 0) ||
				(text.indexOf("Anna") >= 0) ||
				(text.indexOf("KMTronic") >= 0) ||
				(text.indexOf("MQTT") >= 0) ||
				(text.indexOf("Razberry") >= 0)
				)
            {
                var address=$("#hardwarecontent #divremote #tcpaddress").val();
                if (address=="")
                {
                    ShowNotify($.t('Please enter an Address!'), 2500, true);
                    return;
                }
                var port=$("#hardwarecontent #divremote #tcpport").val();
                if (port=="")
                {
                    ShowNotify($.t('Please enter an Port!'), 2500, true);
                    return;
                }
                var intRegex = /^\d+$/;
                if(!intRegex.test(port)) {
                    ShowNotify($.t('Please enter an Valid Port!'), 2500, true);
                    return;
                }
                var username=$("#hardwarecontent #divlogin #username").val();
                var password=$("#hardwarecontent #divlogin #password").val();
                var extra="";
                var Mode1="";
                if ((text.indexOf("MQTT") >= 0)) {
					extra=$("#hardwarecontent #divmqtt #filename").val();
					Mode1 = $("#hardwarecontent #divmqtt #combotopicselect").val();
				}
                $.ajax({
                     url: "json.htm?type=command&param=updatehardware&htype=" + hardwaretype +
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
                     success: function(data) {
                        RefreshHardwareTable();
                     },
                     error: function(){
                            ShowNotify($.t('Problem updating hardware!'), 2500, true);
                     }
                });
            }
            else if (text.indexOf("Philips Hue") >= 0)
            {
                var address=$("#hardwarecontent #divremote #tcpaddress").val();
                if (address=="")
                {
                    ShowNotify($.t('Please enter an Address!'), 2500, true);
                    return;
                }
                var port=$("#hardwarecontent #divremote #tcpport").val();
                if (port=="")
                {
                    ShowNotify($.t('Please enter an Port!'), 2500, true);
                    return;
                }
                var intRegex = /^\d+$/;
                if(!intRegex.test(port)) {
                    ShowNotify($.t('Please enter an Valid Port!'), 2500, true);
                    return;
                }
                var username=$("#hardwarecontent #hardwareparamsphilipshue #username").val();
                if (username == "") {
                    ShowNotify($.t('Please enter a username!'), 2500, true);
                    return;
                }

                $.ajax({
                     url: "json.htm?type=command&param=updatehardware&htype=" + hardwaretype +
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
                     success: function(data) {
                        RefreshHardwareTable();
                     },
                     error: function(){
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
                $.ajax({
                    url: "json.htm?type=command&param=updatehardware&htype=" + hardwaretype +
                       "&port=" + refresh +
                       "&username=" + encodeURIComponent(username) +
                       "&password=" + encodeURIComponent(password) +
                       "&name=" + encodeURIComponent(name) +
                       "&enabled=" + bEnabled +
                       "&idx=" + idx +
                       "&datatimeout=" + datatimeout +
                       "&address=" + encodeURIComponent(url) +
                       "&extra=" + encodeURIComponent(script) +
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
            else if ((text.indexOf("Underground") >= 0) || (text.indexOf("DarkSky") >= 0) || (text.indexOf("AccuWeather") >= 0) || (text.indexOf("Open Weather Map") >= 0))
            {
                var apikey=$("#hardwarecontent #divunderground #apikey").val();
                if (apikey=="")
                {
                    ShowNotify($.t('Please enter an API Key!'), 2500, true);
                    return;
                }
                var location=$("#hardwarecontent #divunderground #location").val();
                if (location=="")
                {
                    ShowNotify($.t('Please enter an Location!'), 2500, true);
                    return;
                }
                $.ajax({
                     url: "json.htm?type=command&param=updatehardware&htype=" + hardwaretype +
                        "&username=" + encodeURIComponent(apikey) +
                        "&password=" + encodeURIComponent(location) +
                        "&name=" + encodeURIComponent(name) +
                        "&enabled=" + bEnabled +
                        "&idx=" + idx +
                        "&datatimeout=" + datatimeout +
                        "&Mode1=" + Mode1 + "&Mode2=" + Mode2 + "&Mode3=" + Mode3 + "&Mode4=" + Mode4 + "&Mode5=" + Mode5 + "&Mode6=" + Mode6,
                     async: false,
                     dataType: 'json',
                     success: function(data) {
                        RefreshHardwareTable();
                     },
                     error: function(){
                            ShowNotify($.t('Problem updating hardware!'), 2500, true);
                     }
                });
            }
            else if (text.indexOf("SolarEdge via Web") >= 0)
            {
                var siteid=$("#hardwarecontent #divsolaredgeapi #siteid").val();
                if (siteid=="")
                {
                    ShowNotify($.t('Please enter an Site ID!'), 2500, true);
                    return;
                }
                var serial=$("#hardwarecontent #divsolaredgeapi #serial").val();
                if (serial=="")
                {
                    ShowNotify($.t('Please enter an Serial!'), 2500, true);
                    return;
                }
                var apikey=$("#hardwarecontent #divsolaredgeapi #apikey").val();
                if (apikey=="")
                {
                    ShowNotify($.t('Please enter an API Key!'), 2500, true);
                    return;
                }

                $.ajax({
                     url: "json.htm?type=command&param=updatehardware&htype=" + hardwaretype +
                        "&username=" + encodeURIComponent(serial) +
                        "&password=" + encodeURIComponent(apikey) +
                        "&name=" + encodeURIComponent(name) +
                        "&enabled=" + bEnabled +
                        "&idx=" + idx +
                        "&datatimeout=" + datatimeout +
                        "&Mode1=" + siteid,
                     async: false,
                     dataType: 'json',
                     success: function(data) {
                        RefreshHardwareTable();
                     },
                     error: function(){
                            ShowNotify($.t('Problem updating hardware!'), 2500, true);
                     }
                });
            }
            else if (text.indexOf("SBFSpot") >= 0)
            {
                var configlocation=$("#hardwarecontent #divlocation #location").val();
                if (configlocation=="")
                {
                    ShowNotify($.t('Please enter an Location!'), 2500, true);
                    return;
                }
                $.ajax({
                     url: "json.htm?type=command&param=updatehardware&htype=" + hardwaretype +
                        "&username=" + encodeURIComponent(configlocation) +
                        "&name=" + encodeURIComponent(name) +
                        "&enabled=" + bEnabled +
                        "&idx=" + idx +
                        "&datatimeout=" + datatimeout +
                        "&Mode1=" + Mode1 + "&Mode2=" + Mode2 + "&Mode3=" + Mode3 + "&Mode4=" + Mode4 + "&Mode5=" + Mode5 + "&Mode6=" + Mode6,
                     async: false,
                     dataType: 'json',
                     success: function(data) {
                        RefreshHardwareTable();
                     },
                     error: function(){
                            ShowNotify($.t('Problem updating hardware!'), 2500, true);
                     }
                });
            }
            else if (text.indexOf("Toon") >= 0)  
            {
                var username = $("#hardwarecontent #divlogin #username").val();
                var password = encodeURIComponent($("#hardwarecontent #divlogin #password").val());
                var agreement = $("#hardwarecontent #divenecotoon #agreement").val();
                $.ajax({
                    url: "json.htm?type=command&param=updatehardware&htype=" + hardwaretype +
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
            else if (
				(text.indexOf("ICY") >= 0) ||
				(text.indexOf("Atag") >= 0) ||
				(text.indexOf("Nest Th") >= 0) ||
				(text.indexOf("PVOutput") >= 0) ||
				(text.indexOf("Netatmo") >= 0) ||
				(text.indexOf("Fitbit") >= 0) ||
				(text.indexOf("Thermosmart") >= 0)
			) {
                var username = $("#hardwarecontent #divlogin #username").val();
                var password = encodeURIComponent($("#hardwarecontent #divlogin #password").val());
                $.ajax({
                    url: "json.htm?type=command&param=updatehardware&htype=" + hardwaretype +
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
            else if (text.indexOf("Winddelen") >= 0)
            {
                var mill_id = $("#hardwarecontent #divwinddelen #combomillselect").val();
                var mill_name = $("#hardwarecontent #divwinddelen #combomillselect").find("option:selected").text()
                var nrofwinddelen = $("#hardwarecontent #divwinddelen #nrofwinddelen").val();
                var intRegex = /^\d+$/;
                if(!intRegex.test(nrofwinddelen)) {
                    ShowNotify($.t('Please enter an Valid Number!'), 2500, true);
                    return;
                }
                $.ajax({
                     url: "json.htm?type=command&param=updatehardware&htype=" + hardwaretype +
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
                     success: function(data) {
                        RefreshHardwareTable();
                     },
                     error: function(){
                            ShowNotify($.t('Problem updating hardware!'), 2500, true);
                     }
                });
            }
            else if (text.indexOf("Logitech Media Server") >= 0)
            {
                var address=$("#hardwarecontent #divremote #tcpaddress").val();
                if (address=="")
                {
                    ShowNotify($.t('Please enter an Address!'), 2500, true);
                    return;
                }
                var port=$("#hardwarecontent #divremote #tcpport").val();
                if (port=="")
                {
                    ShowNotify($.t('Please enter an Port!'), 2500, true);
                    return;
                }
                var intRegex = /^\d+$/;
                if(!intRegex.test(port)) {
                    ShowNotify($.t('Please enter an Valid Port!'), 2500, true);
                    return;
                }
                var username=$("#hardwarecontent #divlogin #username").val();
                var password=$("#hardwarecontent #divlogin #password").val();

                $.ajax({
                     url: "json.htm?type=command&param=updatehardware&htype=" + hardwaretype +
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
                     success: function(data) {
                        RefreshHardwareTable();
                     },
                     error: function(){
                            ShowNotify($.t('Problem updating hardware!'), 2500, true);
                     }
                });
            }
			else if (text.indexOf("HEOS by DENON") >= 0)
            {
                var address=$("#hardwarecontent #divremote #tcpaddress").val();
                if (address=="")
                {
                    ShowNotify($.t('Please enter an Address!'), 2500, true);
                    return;
                }
                var port=$("#hardwarecontent #divremote #tcpport").val();
                if (port=="")
                {
                    ShowNotify($.t('Please enter an Port!'), 2500, true);
                    return;
                }
                var intRegex = /^\d+$/;
                if(!intRegex.test(port)) {
                    ShowNotify($.t('Please enter an Valid Port!'), 2500, true);
                    return;
                }
                var username=$("#hardwarecontent #divlogin #username").val();
                var password=$("#hardwarecontent #divlogin #password").val();

                $.ajax({
                     url: "json.htm?type=command&param=updatehardware&htype=" + hardwaretype +
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
                     success: function(data) {
                        RefreshHardwareTable();
                     },
                     error: function(){
                            ShowNotify($.t('Problem updating hardware!'), 2500, true);
                     }
                });
            }
	    else if (text.indexOf("MyHome OpenWebNet") >= 0)
            {
				var address=$("#hardwarecontent #divremote #tcpaddress").val();
                if (address=="")
                {
                    ShowNotify($.t('Please enter an Address!'), 2500, true);
                    return;
                }
                var port=$("#hardwarecontent #divremote #tcpport").val();
                if (port=="")
                {
                    ShowNotify($.t('Please enter an Port!'), 2500, true);
                    return;
                }
                var intRegex = /^\d+$/;
                if(!intRegex.test(port)) {
                    ShowNotify($.t('Please enter an Valid Port!'), 2500, true);
                    return;
                }

                $.ajax({
                     url: "json.htm?type=command&param=updatehardware&htype=" + hardwaretype +
                        "&address=" + address +
                        "&port=" + port +
                        //"&username=" + encodeURIComponent(username) +
                        "&name=" + encodeURIComponent(name) +
                        "&enabled=" + bEnabled +
                        "&idx=" + idx +
                        "&datatimeout=" + datatimeout +
                        "&Mode1=" + Mode1 + "&Mode2=" + Mode2 + "&Mode3=" + Mode3 + "&Mode4=" + Mode4 + "&Mode5=" + Mode5 + "&Mode6=" + Mode6,
                     async: false,
                     dataType: 'json',
                     success: function(data) {
                        RefreshHardwareTable();
                     },
                    error: function(){
                            ShowNotify($.t('Problem updating hardware!'), 2500, true);
                     }
                });
            }
	    else if (text.indexOf("Goodwe solar inverter via Web") >= 0)
            {
		var username=$("#hardwarecontent #divgoodweweb #username").val();
                if (username=="")
                {
                    ShowNotify($.t('Please enter your Goodwe username!'), 2500, true);
                    return;
                }

                $.ajax({
                     url: "json.htm?type=command&param=updatehardware&htype=" + hardwaretype +
                        "&username=" + encodeURIComponent(username) +
			"&name=" + encodeURIComponent(name) +
                        "&enabled=" + bEnabled +
                        "&idx=" + idx +
                        "&datatimeout=" + datatimeout +
                        "&Mode1=" + Mode1 + "&Mode2=" + Mode2 + "&Mode3=" + Mode3 + "&Mode4=" + Mode4 + "&Mode5=" + Mode5 + "&Mode6=" + Mode6,
                     async: false,
                     dataType: 'json',
                     success: function(data) {
                        RefreshHardwareTable();
                     },
                     error: function(){
                            ShowNotify($.t('Problem updating hardware!'), 2500, true);
                     }
                });
            }
        }

        AddHardware = function()
        {
            var name=$("#hardwarecontent #hardwareparamstable #hardwarename").val();
            if (name=="")
            {
                ShowNotify($.t('Please enter a Name!'), 2500, true);
                return false;
            }

            var hardwaretype=$("#hardwarecontent #hardwareparamstable #combotype option:selected").val();
            if (typeof hardwaretype == 'undefined') {
                ShowNotify($.t('Unknown device selected!'), 2500, true);
                return;
            }

            var bEnabled=$('#hardwarecontent #hardwareparamstable #enabled').is(":checked");
            var datatimeout=$('#hardwarecontent #hardwareparamstable #combodatatimeout').val();

            var text = $("#hardwarecontent #hardwareparamstable #combotype option:selected").text();

            if (text.indexOf("1-Wire") >= 0)
            {
                var owfspath=$("#hardwarecontent #div1wire #owfspath").val();
                var oneWireSensorPollPeriod=$("#hardwarecontent #div1wire #OneWireSensorPollPeriod").val();
                if (oneWireSensorPollPeriod=="")
                {
                    ShowNotify($.t('Please enter a poll period for the sensors'), 2500, true);
                    return;
                }
                var oneWireSwitchPollPeriod=$("#hardwarecontent #div1wire #OneWireSwitchPollPeriod").val();
                if (oneWireSwitchPollPeriod=="")
                {
                    ShowNotify($.t('Please enter a poll period for the switches'), 2500, true);
                    return;
                }
                var intRegex = /^\d+$/;
                if(!intRegex.test(oneWireSensorPollPeriod)) {
                    ShowNotify($.t('Please enter a valid poll period for the sensors'), 2500, true);
                    return;
                }

                if(!intRegex.test(oneWireSwitchPollPeriod)) {
                    ShowNotify($.t('Please enter a valid poll period for the switches'), 2500, true);
                    return;
                }

                $.ajax({
                	url: "json.htm?type=command&param=addhardware&htype=" + hardwaretype + "&extra=" + encodeURIComponent(owfspath) +
                    	"&Mode1=" + oneWireSensorPollPeriod + "&Mode2=" + oneWireSwitchPollPeriod + "&name=" + encodeURIComponent(name) +
                    	"&enabled=" + bEnabled + "&datatimeout=" + datatimeout,
                    async: false,
                    dataType: 'json',
                    success: function(data) {
                    	RefreshHardwareTable();
                    },
                    error: function(){
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
					text.indexOf("KMTronic") == -1
					&& text.indexOf("MQTT") == -1 &&
					text.indexOf("Satel Integra") == -1 &&
					text.indexOf("Razberry") == -1
					)
				)
            {
                var address=$("#hardwarecontent #divremote #tcpaddress").val();
                if (address=="")
                {
                    ShowNotify($.t('Please enter an Address!'), 2500, true);
                    return;
                }
                var port=$("#hardwarecontent #divremote #tcpport").val();
                if (port=="")
                {
                    ShowNotify($.t('Please enter an Port!'), 2500, true);
                    return;
                }
                var intRegex = /^\d+$/;
                if(!intRegex.test(port)) {
                    ShowNotify($.t('Please enter an Valid Port!'), 2500, true);
                    return;
                }
                $.ajax({
                     url: "json.htm?type=command&param=addhardware&htype=" + hardwaretype + "&address=" + address + "&port=" + port + "&name=" + encodeURIComponent(name) + "&enabled=" + bEnabled + "&datatimeout=" + datatimeout,
                     async: false,
                     dataType: 'json',
                     success: function(data) {
                        RefreshHardwareTable();
                     },
                     error: function(){
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
				(text.indexOf("Kodi") >= 0) ||
				(text.indexOf("PiFace") >= 0) ||
				(text.indexOf("GPIO") >= 0) ||
				(text.indexOf("Evohome") >= 0 && text.indexOf("script") >= 0) ||
				(text.indexOf("Tellstick") >= 0) ||
				(text.indexOf("Motherboard") >= 0) ||
                (text.indexOf("YeeLight") >= 0)
				)
            {
                $.ajax({
                     url: "json.htm?type=command&param=addhardware&htype=" + hardwaretype + "&name=" + encodeURIComponent(name) + "&enabled=" + bEnabled + "&datatimeout=" + datatimeout,
                     async: false,
                     dataType: 'json',
                     success: function(data) {
                        RefreshHardwareTable();
                     },
                     error: function(){
                            ShowNotify($.t('Problem adding hardware!'), 2500, true);
                     }
                });
            }
	    else if (text.indexOf("Local I2C sensor") >= 0)
	    {
                hardwaretype = $("#hardwareparamsi2clocal #comboi2clocal").find('option:selected').val();
		
                $.ajax({
                     url: "json.htm?type=command&param=addhardware&htype=" + hardwaretype + "&name=" + encodeURIComponent(name) + "&enabled=" + bEnabled + "&datatimeout=" + datatimeout,
                     async: false,
                     dataType: 'json',
                     success: function(data) {
                        RefreshHardwareTable();
                     },
                     error: function(){
                            ShowNotify($.t('Problem adding hardware!'), 2500, true);
                     }
                });		
	    }
            else if (text.indexOf("USB") >= 0)
            {
                var Mode1 = "0";
                var serialport=$("#hardwarecontent #divserial #comboserialport option:selected").text();
                if (typeof serialport == 'undefined')
                {
                    ShowNotify($.t('No serial port selected!'), 2500, true);
                    return;
                }

                if (text.indexOf("MySensors") >= 0)
                {
                    var baudrate=$("#hardwarecontent #divbaudratemysensors #combobaudrate option:selected").val();

                    if (typeof baudrate == 'undefined')
                    {
                        ShowNotify($.t('No baud rate selected!'), 2500, true);
                        return;
                    }

                    Mode1 = baudrate;
                }

                if (text.indexOf("P1 Smart Meter") >= 0)
                {
                    var baudrate=$("#hardwarecontent #divbaudratep1 #combobaudratep1 option:selected").val();

                    if (typeof baudrate == 'undefined')
                    {
                        ShowNotify($.t('No baud rate selected!'), 2500, true);
                        return;
                    }

                    Mode1 = baudrate;
                    Mode2 = $("#hardwarecontent #divcrcp1 #disablecrcp1").prop("checked")?0:1;
                }

                $.ajax({
                     url: "json.htm?type=command&param=addhardware&htype=" + hardwaretype + "&port=" + encodeURIComponent(serialport) + "&name=" + encodeURIComponent(name) + "&enabled=" + bEnabled + "&datatimeout=" + datatimeout +
                          "&Mode1=" + Mode1,
                     async: false,
                     dataType: 'json',
                     success: function(data) {
                        RefreshHardwareTable();
                     },
                     error: function(){
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
					text.indexOf("KMTronic") == -1
					&& text.indexOf("MQTT") == -1 &&
					text.indexOf("Satel Integra") == -1 &&
					text.indexOf("Razberry") == -1
					)
				)
            {
                var address=$("#hardwarecontent #divremote #tcpaddress").val();
                if (address=="")
                {
                    ShowNotify($.t('Please enter an Address!'), 2500, true);
                    return;
                }
                var port=$("#hardwarecontent #divremote #tcpport").val();
                if (port=="")
                {
                    ShowNotify($.t('Please enter an Port!'), 2500, true);
                    return;
                }
                var intRegex = /^\d+$/;
                if(!intRegex.test(port)) {
                    ShowNotify($.t('Please enter an Valid Port!'), 2500, true);
                    return;
                }
                $.ajax({
                     url: "json.htm?type=command&param=addhardware&htype=" + hardwaretype + "&address=" + address + "&port=" + port + "&name=" + encodeURIComponent(name) + "&enabled=" + bEnabled + "&datatimeout=" + datatimeout,
                     async: false,
                     dataType: 'json',
                     success: function(data) {
                        RefreshHardwareTable();
                     },
                     error: function(){
                            ShowNotify($.t('Problem adding hardware!'), 2500, true);
                     }
                });
            }
            else if (
				(text.indexOf("LAN") >= 0 && ((text.indexOf("YouLess") >= 0) || (text.indexOf("Denkovi") >= 0) )) ||
				(text.indexOf("Satel Integra") >= 0)
				)
            {
                var address=$("#hardwarecontent #divremote #tcpaddress").val();
                if (address=="")
                {
                    ShowNotify($.t('Please enter an Address!'), 2500, true);
                    return;
                }
                var port=$("#hardwarecontent #divremote #tcpport").val();
                if (port=="")
                {
                    ShowNotify($.t('Please enter an Port!'), 2500, true);
                    return;
                }
                var intRegex = /^\d+$/;
                if(!intRegex.test(port)) {
                    ShowNotify($.t('Please enter an Valid Port!'), 2500, true);
                    return;
                }
				if (text.indexOf("Satel Integra") >= 0)
                {
					var pollinterval=$("#hardwarecontent #hardwareparamspollinterval #pollinterval").val();
                    if (pollinterval=="")
                    {
                        ShowNotify($.t('Please enter poll interval!'), 2500, true);
                        return;
                    }
                    Mode1 = pollinterval;
                }
                var password=encodeURIComponent($("#hardwarecontent #divlogin #password").val());
                $.ajax({
                     url: "json.htm?type=command&param=addhardware&htype=" + hardwaretype + "&address=" + address + "&port=" + port + "&name=" + encodeURIComponent(name) + "&password=" + encodeURIComponent(password) + "&enabled=" + bEnabled + "&datatimeout=" + datatimeout + "&Mode1=" + Mode1,
                     async: false,
                     dataType: 'json',
                     success: function(data) {
                        RefreshHardwareTable();
                     },
                     error: function(){
                            ShowNotify($.t('Problem adding hardware!'), 2500, true);
                     }
                });
            }
            else if (text.indexOf("Philips Hue") >= 0)
            {
                var address=$("#hardwarecontent #divremote #tcpaddress").val();
                if (address=="")
                {
                    ShowNotify($.t('Please enter an Address!'), 2500, true);
                    return;
                }
                var port=$("#hardwarecontent #divremote #tcpport").val();
                if (port=="")
                {
                    ShowNotify($.t('Please enter an Port!'), 2500, true);
                    return;
                }
                var intRegex = /^\d+$/;
                if(!intRegex.test(port)) {
                    ShowNotify($.t('Please enter an Valid Port!'), 2500, true);
                    return;
                }
                var username=$("#hardwarecontent #hardwareparamsphilipshue #username").val();

                if (username == "") {
                    ShowNotify($.t('Please enter a username!'), 2500, true);
                    return;
                }
                $.ajax({
                     url: "json.htm?type=command&param=addhardware&htype=" + hardwaretype + "&address=" + address + "&port=" + port + "&username=" + encodeURIComponent(username) + "&name=" + encodeURIComponent(name) + "&enabled=" + bEnabled + "&datatimeout=" + datatimeout,
                     async: false,
                     dataType: 'json',
                     success: function(data) {
                        RefreshHardwareTable();
                     },
                     error: function(){
                            ShowNotify($.t('Problem adding hardware!'), 2500, true);
                     }
                });
            }
            else if (
				(text.indexOf("Domoticz") >= 0) ||
				(text.indexOf("Harmony") >= 0) ||
				(text.indexOf("ETH8020") >= 0) ||
				(text.indexOf("Daikin") >= 0) ||
				(text.indexOf("Sterbox") >= 0) ||
				(text.indexOf("Anna") >= 0) ||
				(text.indexOf("KMTronic") >= 0) ||
				(text.indexOf("MQTT") >= 0) ||
				(text.indexOf("Logitech Media Server") >= 0) ||
				(text.indexOf("HEOS by DENON") >= 0) ||
				(text.indexOf("Razberry") >= 0)
				)
            {
                var address=$("#hardwarecontent #divremote #tcpaddress").val();
                if (address=="")
                {
                    ShowNotify($.t('Please enter an Address!'), 2500, true);
                    return;
                }
                var port=$("#hardwarecontent #divremote #tcpport").val();
                if (port=="")
                {
                    ShowNotify($.t('Please enter an Port!'), 2500, true);
                    return;
                }
                var intRegex = /^\d+$/;
                if(!intRegex.test(port)) {
                    ShowNotify($.t('Please enter an Valid Port!'), 2500, true);
                    return;
                }
                var username=$("#hardwarecontent #divlogin #username").val();
                var password = encodeURIComponent($("#hardwarecontent #divlogin #password").val());
                var extra = "";
                var mode1 = "";
				if (text.indexOf("MQTT") >= 0) {
					extra = encodeURIComponent($("#hardwarecontent #divmqtt #filename").val());
					mode1 = $("#hardwarecontent #divmqtt #combotopicselect").val();
				}
                if ((text.indexOf("Harmony") >= 0) && (username == "")) {
                    ShowNotify($.t('Please enter a username!'), 2500, true);
                    return;
                }

                if ((text.indexOf("Harmony") >= 0) && (password == "")) {
                    ShowNotify($.t('Please enter a password!'), 2500, true);
                    return;
                }

                $.ajax({
                     url: "json.htm?type=command&param=addhardware&htype=" + hardwaretype + "&address=" + address + "&port=" + port + "&username=" + encodeURIComponent(username) + "&password=" + encodeURIComponent(password) + "&name=" + encodeURIComponent(name) + "&enabled=" + bEnabled + "&datatimeout=" + datatimeout + "&extra=" + encodeURIComponent(extra) + "&mode1=" + mode1,
                     async: false,
                     dataType: 'json',
                     success: function(data) {
                        RefreshHardwareTable();
                     },
                     error: function(){
                            ShowNotify($.t('Problem adding hardware!'), 2500, true);
                     }
                });
            }
            else if ((text.indexOf("Underground") >= 0) || (text.indexOf("DarkSky") >= 0) || (text.indexOf("AccuWeather") >= 0) || (text.indexOf("Open Weather Map") >= 0))
            {
                var apikey=$("#hardwarecontent #divunderground #apikey").val();
                if (apikey=="")
                {
                    ShowNotify($.t('Please enter an API Key!'), 2500, true);
                    return;
                }
                var location=$("#hardwarecontent #divunderground #location").val();
                if (location=="")
                {
                    ShowNotify($.t('Please enter an Location!'), 2500, true);
                    return;
                }
                $.ajax({
                     url: "json.htm?type=command&param=addhardware&htype=" + hardwaretype + "&username=" + encodeURIComponent(apikey) + "&password=" + encodeURIComponent(location) + "&name=" + encodeURIComponent(name) + "&enabled=" + bEnabled + "&datatimeout=" + datatimeout,
                     async: false,
                     dataType: 'json',
                     success: function(data) {
                        RefreshHardwareTable();
                     },
                     error: function(){
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
                $.ajax({
                    url: "json.htm?type=command&param=addhardware&htype=" + hardwaretype + "&port=" + refresh + "&username=" + encodeURIComponent(username) + "&password=" + encodeURIComponent(password) + "&name=" + encodeURIComponent(name) + "&address=" + encodeURIComponent(url) + "&extra=" + encodeURIComponent(script) + "&enabled=" + bEnabled + "&datatimeout=" + datatimeout,
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
            else if (text.indexOf("SBFSpot") >= 0)
            {
                var configlocation=$("#hardwarecontent #divlocation #location").val();
                if (configlocation=="")
                {
                    ShowNotify($.t('Please enter an Location!'), 2500, true);
                    return;
                }
                $.ajax({
                     url: "json.htm?type=command&param=addhardware&htype=" + hardwaretype +
                        "&username=" + encodeURIComponent(configlocation) +
                        "&name=" + encodeURIComponent(name) +
                        "&enabled=" + bEnabled +
                        "&datatimeout=" + datatimeout,
                     async: false,
                     dataType: 'json',
                     success: function(data) {
                        RefreshHardwareTable();
                     },
                     error: function(){
                            ShowNotify($.t('Problem updating hardware!'), 2500, true);
                     }
                });
            }
            else if (text.indexOf("Winddelen") >= 0)
            {
                var mill_id = $("#hardwarecontent #divwinddelen #combomillselect").val();
                var mill_name = $("#hardwarecontent #divwinddelen #combomillselect").find("option:selected").text()
                var nrofwinddelen = $("#hardwarecontent #divwinddelen #nrofwinddelen").val();
                var intRegex = /^\d+$/;
                if(!intRegex.test(nrofwinddelen)) {
                    ShowNotify($.t('Please enter an Valid Number!'), 2500, true);
                    return;
                }
                $.ajax({
                     url: "json.htm?type=command&param=addhardware&htype=" + hardwaretype +
                     "&name=" + encodeURIComponent(name) +
                     "&address=" + encodeURIComponent(mill_name) +
                     "&port=" + encodeURIComponent(nrofwinddelen) +
                     "&Mode1=" + encodeURIComponent(mill_id) +
                     "&enabled=" + bEnabled +
                     "&datatimeout=" + datatimeout,
                     async: false,
                     dataType: 'json',
                     success: function(data) {
                        RefreshHardwareTable();
                     },
                     error: function(){
                            ShowNotify($.t('Problem adding hardware!'), 2500, true);
                     }
                });
            }
            else if (text.indexOf("SolarEdge via Web") >= 0)
            {
                var siteid=$("#hardwarecontent #divsolaredgeapi #siteid").val();
                if (siteid=="")
                {
                    ShowNotify($.t('Please enter an Site ID!'), 2500, true);
                    return;
                }
                var serial=$("#hardwarecontent #divsolaredgeapi #serial").val();
                if (serial=="")
                {
                    ShowNotify($.t('Please enter an Serial!'), 2500, true);
                    return;
                }
                var apikey=$("#hardwarecontent #divsolaredgeapi #apikey").val();
                if (apikey=="")
                {
                    ShowNotify($.t('Please enter an API Key!'), 2500, true);
                    return;
                }
                $.ajax({
                     url: "json.htm?type=command&param=addhardware&htype=" + hardwaretype +
						"&name=" + encodeURIComponent(name) +
                        "&username=" + encodeURIComponent(serial) +
                        "&password=" + encodeURIComponent(apikey) +
                        "&enabled=" + bEnabled +
                        "&idx=" + idx +
                        "&datatimeout=" + datatimeout +
                        "&Mode1=" + siteid,
                     async: false,
                     dataType: 'json',
                     success: function(data) {
                        RefreshHardwareTable();
                     },
                     error: function(){
                            ShowNotify($.t('Problem adding hardware!'), 2500, true);
                     }
                });
            }
            else if (text.indexOf("Toon") >= 0) 
	     {
                var username=$("#hardwarecontent #divlogin #username").val();
                var password=encodeURIComponent($("#hardwarecontent #divlogin #password").val());
                var agreement=encodeURIComponent($("#hardwarecontent #divenecotoon #agreement").val());
                $.ajax({
                     url: "json.htm?type=command&param=addhardware&htype=" + hardwaretype +
                     "&username=" + encodeURIComponent(username) +
                     "&password=" + encodeURIComponent(password) +
                     "&name=" + encodeURIComponent(name) +
                     "&enabled=" + bEnabled +
                     "&datatimeout=" + datatimeout+
                     "&Mode1=" + agreement,
                     async: false,
                     dataType: 'json',
                     success: function(data) {
                        RefreshHardwareTable();
                     },
                     error: function(){
                            ShowNotify($.t('Problem adding hardware!'), 2500, true);
                     }
                });
	    }
            else if (
				(text.indexOf("ICY") >= 0) ||
				(text.indexOf("Atag") >= 0) ||
				(text.indexOf("Nest Th") >= 0) ||
				(text.indexOf("PVOutput") >= 0) ||
				(text.indexOf("Netatmo") >= 0) ||
				(text.indexOf("Fitbit") >= 0) ||
				(text.indexOf("Thermosmart") >= 0) ||
				(text.indexOf("HTTP") >= 0)
			) {
                var username=$("#hardwarecontent #divlogin #username").val();
                var password=encodeURIComponent($("#hardwarecontent #divlogin #password").val());
                $.ajax({
                     url: "json.htm?type=command&param=addhardware&htype=" + hardwaretype +
                     "&username=" + encodeURIComponent(username) +
                     "&password=" + encodeURIComponent(password) +
                     "&name=" + encodeURIComponent(name) +
                     "&enabled=" + bEnabled +
                     "&datatimeout=" + datatimeout,
                     async: false,
                     dataType: 'json',
                     success: function(data) {
                        RefreshHardwareTable();
                     },
                     error: function(){
                            ShowNotify($.t('Problem adding hardware!'), 2500, true);
                     }
                });
            }
	    else if (text.indexOf("MyHome OpenWebNet") >= 0)
            {
                var address=$("#hardwarecontent #divremote #tcpaddress").val();
                if (address=="")
                {
                    ShowNotify($.t('Please enter an Address!'), 2500, true);
                    return;
                }
                var port=$("#hardwarecontent #divremote #tcpport").val();
                if (port=="")
                {
                    ShowNotify($.t('Please enter an Port!'), 2500, true);
                    return;
                }
                var intRegex = /^\d+$/;
                if(!intRegex.test(port)) {
                    ShowNotify($.t('Please enter an Valid Port!'), 2500, true);
                    return;
                }
                /*var username=$("#hardwarecontent #hardwareparamsphilipshue #username").val();

                if (username == "") {
                    ShowNotify($.t('Please enter a username!'), 2500, true);
                    return;
                }*/

                $.ajax({
                     url: "json.htm?type=command&param=addhardware&htype=" + hardwaretype + "&address=" + address + "&port=" + port +
					 //"&username=" + encodeURIComponent(username) +
					 "&name=" + encodeURIComponent(name) + "&enabled=" + bEnabled + "&datatimeout=" + datatimeout,
                     async: false,
                     dataType: 'json',
                     success: function(data) {
                        RefreshHardwareTable();
                     },
                     error: function(){
                            ShowNotify($.t('Problem adding hardware!'), 2500, true);
                     }
                });
            }
	    else if (text.indexOf("Goodwe solar inverter via Web") >= 0)
            {
                var username=$("#hardwarecontent #divgoodweweb #username").val();

                if (username == "") {
                    ShowNotify($.t('Please enter your Goodwe username!'), 2500, true);
                    return;
                }

                $.ajax({
                     url: "json.htm?type=command&param=addhardware&htype=" + hardwaretype +
					 "&username=" + encodeURIComponent(username) +
					 "&name=" + encodeURIComponent(name) + "&enabled=" + bEnabled + "&datatimeout=" + datatimeout,
                     async: false,
                     dataType: 'json',
                     success: function(data) {
                        RefreshHardwareTable();
                     },
                     error: function(){
                            ShowNotify($.t('Problem adding hardware!'), 2500, true);
                     }
                });
            }
        }

        EditRFXCOMMode = function(idx,name,Mode1,Mode2,Mode3,Mode4,Mode5,Mode6)
        {
            cursordefault();
            var htmlcontent = '';
            htmlcontent='<p><center><h2><span data-i18n="Device"></span>: ' + name + '</h2></center></p>\n';
            htmlcontent+=$('#rfxhardwaremode').html();
            $('#hardwarecontent').html(GetBackbuttonHTMLTable('ShowHardware')+htmlcontent);
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
            $('#hardwarecontent #Keeloq').prop('checked',((Mode6 & 0x01)!=0));
            $('#hardwarecontent #HC').prop('checked',((Mode6 & 0x02)!=0));
            $('#hardwarecontent #undecon').prop('checked',((Mode3 & 0x80)!=0));
            $('#hardwarecontent #X10').prop('checked',((Mode5 & 0x01)!=0));
            $('#hardwarecontent #ARC').prop('checked',((Mode5 & 0x02)!=0));
            $('#hardwarecontent #AC').prop('checked',((Mode5 & 0x04)!=0));
            $('#hardwarecontent #HomeEasyEU').prop('checked',((Mode5 & 0x08)!=0));
            $('#hardwarecontent #Meiantech').prop('checked',((Mode5 & 0x10)!=0));
            $('#hardwarecontent #OregonScientific').prop('checked',((Mode5 & 0x20)!=0));
            $('#hardwarecontent #ATIremote').prop('checked',((Mode5 & 0x40)!=0));
            $('#hardwarecontent #Visonic').prop('checked',((Mode5 & 0x80)!=0));
            $('#hardwarecontent #Mertik').prop('checked',((Mode4 & 0x01)!=0));
            $('#hardwarecontent #ADLightwaveRF').prop('checked',((Mode4 & 0x02)!=0));
            $('#hardwarecontent #HidekiUPM').prop('checked',((Mode4 & 0x04)!=0));
            $('#hardwarecontent #LaCrosse').prop('checked',((Mode4 & 0x08)!=0));
            $('#hardwarecontent #FS20').prop('checked',((Mode4 & 0x10)!=0));
            $('#hardwarecontent #ProGuard').prop('checked',((Mode4 & 0x20)!=0));
            $('#hardwarecontent #BlindT0').prop('checked',((Mode4 & 0x40)!=0));
            $('#hardwarecontent #BlindT1T2T3T4').prop('checked',((Mode4 & 0x80)!=0));
            $('#hardwarecontent #AEBlyss').prop('checked',((Mode3 & 0x01)!=0));
            $('#hardwarecontent #Rubicson').prop('checked',((Mode3 & 0x02)!=0));
            $('#hardwarecontent #FineOffsetViking').prop('checked',((Mode3 & 0x04)!=0));
            $('#hardwarecontent #Lighting4').prop('checked',((Mode3 & 0x08)!=0));
            $('#hardwarecontent #RSL').prop('checked',((Mode3 & 0x10)!=0));
            $('#hardwarecontent #ByronSX').prop('checked',((Mode3 & 0x20)!=0));
            $('#hardwarecontent #ImaginTronix').prop('checked',((Mode3 & 0x40)!=0));

            $('#hardwarecontent #defaultbutton').click(function (e) {
                e.preventDefault();
                $('#hardwarecontent #Keeloq').prop('checked',false);
                $('#hardwarecontent #HC').prop('checked',false);
                $('#hardwarecontent #undecon').prop('checked',false);
                $('#hardwarecontent #X10').prop('checked',true);
                $('#hardwarecontent #ARC').prop('checked',true);
                $('#hardwarecontent #AC').prop('checked',true);
                $('#hardwarecontent #HomeEasyEU').prop('checked',true);
                $('#hardwarecontent #Meiantech').prop('checked',false);
                $('#hardwarecontent #OregonScientific').prop('checked',true);
                $('#hardwarecontent #ATIremote').prop('checked',false);
                $('#hardwarecontent #Visonic').prop('checked',false);
                $('#hardwarecontent #Mertik').prop('checked',false);
                $('#hardwarecontent #ADLightwaveRF').prop('checked',false);
                $('#hardwarecontent #HidekiUPM').prop('checked',true);
                $('#hardwarecontent #LaCrosse').prop('checked',true);
                $('#hardwarecontent #FS20').prop('checked',false);
                $('#hardwarecontent #ProGuard').prop('checked',false);
                $('#hardwarecontent #BlindT0').prop('checked',false);
                $('#hardwarecontent #BlindT1T2T3T4').prop('checked',false);
                $('#hardwarecontent #AEBlyss').prop('checked',false);
                $('#hardwarecontent #Rubicson').prop('checked',false);
                $('#hardwarecontent #FineOffsetViking').prop('checked',false);
                $('#hardwarecontent #Lighting4').prop('checked',false);
                $('#hardwarecontent #RSL').prop('checked',false);
                $('#hardwarecontent #ByronSX').prop('checked',false);
                $('#hardwarecontent #ImaginTronix').prop('checked',false);
            });
        }

        EditRFXCOMMode868 = function(idx,name,Mode1,Mode2,Mode3,Mode4,Mode5,Mode6)
        {
            cursordefault();
            var htmlcontent = '';
            htmlcontent='<p><center><h2><span data-i18n="Device"></span>: ' + name + '</h2></center></p>\n';
            htmlcontent+=$('#rfx868hardwaremode').html();
            $('#hardwarecontent').html(GetBackbuttonHTMLTable('ShowHardware')+htmlcontent);
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
                $('#hardwarecontent #LaCrosse').prop('checked',false);
                $('#hardwarecontent #Alecto').prop('checked',false);
                $('#hardwarecontent #FS20').prop('checked',false);
                $('#hardwarecontent #ProGuard').prop('checked',false);
                $('#hardwarecontent #VionicPowerCode').prop('checked',false);
                $('#hardwarecontent #Hideki').prop('checked',false);
                $('#hardwarecontent #FHT8').prop('checked',false);
                $('#hardwarecontent #VionicCodeSecure').prop('checked',false);
            });
        }

        EditRego6XXType = function(idx,name,Mode1,Mode2,Mode3,Mode4,Mode5,Mode6)
        {
            cursordefault();
            var htmlcontent = '';
            htmlcontent='<p><center><h2><span data-i18n="Device"></span>: ' + name + '</h2></center></p>\n';
            htmlcontent+=$('#rego6xxtypeedit').html();
            $('#hardwarecontent').html(GetBackbuttonHTMLTable('ShowHardware')+htmlcontent);
            $('#hardwarecontent').i18n();

            $('#hardwarecontent #submitbuttonrego').click(function (e) {
                e.preventDefault();
                SetRego6XXType();
            });

            $('#hardwarecontent #idx').val(idx);
            $('#hardwarecontent #comborego6xxtype').val(Mode1);
        }

        SetCCUSBType = function()
        {
          $.post("setcurrentcostmetertype.webem", $("#hardwarecontent #ccusbtype").serialize(), function(data) {
           ShowHardware();
          });
        }

        AddWOLNode = function()
        {
            var name=$("#hardwarecontent #nodeparamstable #nodename").val();
            if (name=="")
            {
                ShowNotify($.t('Please enter a Name!'), 2500, true);
                return;
            }
            var mac=$("#hardwarecontent #nodeparamstable #nodemac").val();
            if (mac=="")
            {
                ShowNotify($.t('Please enter a MAC Address!'), 2500, true);
                return;
            }

            $.ajax({
                 url: "json.htm?type=command&param=woladdnode" +
                    "&idx=" + $.devIdx +
                    "&name=" + encodeURIComponent(name) +
                    "&mac=" + mac,
                 async: false,
                 dataType: 'json',
                 success: function(data) {
                    RefreshWOLNodeTable();
                 },
                 error: function(){
                    ShowNotify($.t('Problem Adding Node!'), 2500, true);
                 }
            });
        }

        WOLDeleteNode = function(nodeid)
        {
            if ($('#updelclr #nodedelete').attr("class")=="btnstyle3-dis") {
                return;
            }
            bootbox.confirm($.t("Are you sure to remove this Node?"), function(result) {
                if (result==true) {
                    $.ajax({
                         url: "json.htm?type=command&param=wolremovenode" +
                            "&idx=" + $.devIdx +
                            "&nodeid=" + nodeid,
                         async: false,
                         dataType: 'json',
                         success: function(data) {
                            RefreshWOLNodeTable();
                         },
                         error: function(){
                            ShowNotify($.t('Problem Deleting Node!'), 2500, true);
                         }
                    });
                }
            });
        }

        WOLClearNodes = function()
        {
            bootbox.confirm($.t("Are you sure to delete ALL Nodes?\n\nThis action can not be undone!"), function(result) {
                if (result==true) {
                    $.ajax({
                         url: "json.htm?type=command&param=wolclearnodes" +
                            "&idx=" + $.devIdx,
                         async: false,
                         dataType: 'json',
                         success: function(data) {
                            RefreshWOLNodeTable();
                         }
                    });
                }
            });
        }

        WOLUpdateNode = function(nodeid)
        {
            if ($('#updelclr #nodedelete').attr("class")=="btnstyle3-dis") {
                return;
            }

            var name=$("#hardwarecontent #nodeparamstable #nodename").val();
            if (name=="")
            {
                ShowNotify($.t('Please enter a Name!'), 2500, true);
                return;
            }
            var mac=$("#hardwarecontent #nodeparamstable #nodemac").val();
            if (mac=="")
            {
                ShowNotify($.t('Please enter a MAC Address!'), 2500, true);
                return;
            }

            $.ajax({
                 url: "json.htm?type=command&param=wolupdatenode" +
                    "&idx=" + $.devIdx +
                    "&nodeid=" + nodeid +
                    "&name=" + encodeURIComponent(name) +
                    "&mac=" + mac,
                 async: false,
                 dataType: 'json',
                 success: function(data) {
                    RefreshWOLNodeTable();
                 },
                 error: function(){
                    ShowNotify($.t('Problem Updating Node!'), 2500, true);
                 }
            });
        }

        RefreshWOLNodeTable = function()
        {
          $('#modal').show();
            $('#updelclr #nodeupdate').attr("class", "btnstyle3-dis");
            $('#updelclr #nodedelete').attr("class", "btnstyle3-dis");
            $("#hardwarecontent #nodeparamstable #nodename").val("");
            $("#hardwarecontent #nodeparamstable #nodemac").val("");

          var oTable = $('#nodestable').dataTable();
          oTable.fnClearTable();

          $.ajax({
             url: "json.htm?type=command&param=wolgetnodes&idx="+$.devIdx,
             async: false,
             dataType: 'json',
             success: function(data) {
              if (typeof data.result != 'undefined') {
                $.each(data.result, function(i,item){
                    var addId = oTable.fnAddData( {
                        "DT_RowId": item.idx,
                        "Name": item.Name,
                        "Mac": item.Mac,
                        "0": item.idx,
                        "1": item.Name,
                        "2": item.Mac
                    } );
                });
              }
             }
          });

            /* Add a click handler to the rows - this could be used as a callback */
            $("#nodestable tbody").off();
            $("#nodestable tbody").on( 'click', 'tr', function () {
                $('#updelclr #nodedelete').attr("class", "btnstyle3-dis");
                if ( $(this).hasClass('row_selected') ) {
                    $(this).removeClass('row_selected');
                    $('#updelclr #nodeupdate').attr("class", "btnstyle3-dis");
                    $("#hardwarecontent #nodeparamstable #nodename").val("");
                    $("#hardwarecontent #nodeparamstable #nodemac").val("");
                }
                else {
                    var oTable = $('#nodestable').dataTable();
                    oTable.$('tr.row_selected').removeClass('row_selected');
                    $(this).addClass('row_selected');
                    $('#updelclr #nodeupdate').attr("class", "btnstyle3");
                    var anSelected = fnGetSelected( oTable );
                    if ( anSelected.length !== 0 ) {
                        var data = oTable.fnGetData( anSelected[0] );
                        var idx= data["DT_RowId"];
                        $("#updelclr #nodeupdate").attr("href", "javascript:WOLUpdateNode(" + idx + ")");
                        $('#updelclr #nodedelete').attr("class", "btnstyle3");
                        $("#updelclr #nodedelete").attr("href", "javascript:WOLDeleteNode(" + idx + ")");
                        $("#hardwarecontent #nodeparamstable #nodename").val(data["1"]);
                        $("#hardwarecontent #nodeparamstable #nodemac").val(data["2"]);
                    }
                }
            });

          $('#modal').hide();
        }

        EditWOL = function(idx,name,Mode1,Mode2,Mode3,Mode4,Mode5,Mode6)
        {
            $.devIdx=idx;
            cursordefault();
            var htmlcontent = '';
            htmlcontent='<p><center><h2><span data-i18n="Device"></span>: ' + name + '</h2></center></p>\n';
            htmlcontent+=$('#wakeonlan').html();
            $('#hardwarecontent').html(GetBackbuttonHTMLTable('ShowHardware')+htmlcontent);
            $('#hardwarecontent').i18n();

            var oTable = $('#nodestable').dataTable( {
              "sDom": '<"H"lfrC>t<"F"ip>',
              "oTableTools": {
                "sRowSelect": "single",
              },
              "aaSorting": [[ 0, "desc" ]],
              "bSortClasses": false,
              "bProcessing": true,
              "bStateSave": true,
              "bJQueryUI": true,
              "aLengthMenu": [[25, 50, 100, -1], [25, 50, 100, "All"]],
              "iDisplayLength" : 25,
              "sPaginationType": "full_numbers",
              language: $.DataTableLanguage
            } );

            $('#hardwarecontent #idx').val(idx);

            RefreshWOLNodeTable();
        }

        AddPingerNode = function()
        {
            var name=$("#hardwarecontent #ipnodeparamstable #nodename").val();
            if (name=="")
            {
                ShowNotify($.t('Please enter a Name!'), 2500, true);
                return;
            }
            var ip=$("#hardwarecontent #ipnodeparamstable #nodeip").val();
            if (ip=="")
            {
                ShowNotify($.t('Please enter a IP Address!'), 2500, true);
                return;
            }
            var Timeout=parseInt($("#hardwarecontent #ipnodeparamstable #nodetimeout").val());
            if (Timeout<1)
                Timeout=5;

            $.ajax({
                 url: "json.htm?type=command&param=pingeraddnode" +
                    "&idx=" + $.devIdx +
                    "&name=" + encodeURIComponent(name) +
                    "&ip=" + ip +
                    "&timeout=" + Timeout,
                 async: false,
                 dataType: 'json',
                 success: function(data) {
                    RefreshPingerNodeTable();
                 },
                 error: function(){
                    ShowNotify($.t('Problem Adding Node!'), 2500, true);
                 }
            });
        }

        PingerDeleteNode = function(nodeid)
        {
            if ($('#updelclr #nodedelete').attr("class")=="btnstyle3-dis") {
                return;
            }
            bootbox.confirm($.t("Are you sure to remove this Node?"), function(result) {
                if (result==true) {
                    $.ajax({
                         url: "json.htm?type=command&param=pingerremovenode" +
                            "&idx=" + $.devIdx +
                            "&nodeid=" + nodeid,
                         async: false,
                         dataType: 'json',
                         success: function(data) {
                            RefreshPingerNodeTable();
                         },
                         error: function(){
                            ShowNotify($.t('Problem Deleting Node!'), 2500, true);
                         }
                    });
                }
            });
        }

        PingerClearNodes = function()
        {
            bootbox.confirm($.t("Are you sure to delete ALL Nodes?\n\nThis action can not be undone!"), function(result) {
                if (result==true) {
                    $.ajax({
                         url: "json.htm?type=command&param=pingerclearnodes" +
                            "&idx=" + $.devIdx,
                         async: false,
                         dataType: 'json',
                         success: function(data) {
                            RefreshPingerNodeTable();
                         }
                    });
                }
            });
        }

        PingerUpdateNode = function(nodeid)
        {
            if ($('#updelclr #nodedelete').attr("class")=="btnstyle3-dis") {
                return;
            }

            var name=$("#hardwarecontent #ipnodeparamstable #nodename").val();
            if (name=="")
            {
                ShowNotify($.t('Please enter a Name!'), 2500, true);
                return;
            }
            var ip=$("#hardwarecontent #ipnodeparamstable #nodeip").val();
            if (ip=="")
            {
                ShowNotify($.t('Please enter a IP Address!'), 2500, true);
                return;
            }
            var Timeout=parseInt($("#hardwarecontent #ipnodeparamstable #nodetimeout").val());
            if (Timeout<1)
                Timeout=5;

            $.ajax({
                 url: "json.htm?type=command&param=pingerupdatenode" +
                    "&idx=" + $.devIdx +
                    "&nodeid=" + nodeid +
                    "&name=" + encodeURIComponent(name) +
                    "&ip=" + ip +
                    "&timeout=" + Timeout,
                 async: false,
                 dataType: 'json',
                 success: function(data) {
                    RefreshPingerNodeTable();
                 },
                 error: function(){
                    ShowNotify($.t('Problem Updating Node!'), 2500, true);
                 }
            });
        }

        RefreshPingerNodeTable = function()
        {
          $('#modal').show();
            $('#updelclr #nodeupdate').attr("class", "btnstyle3-dis");
            $('#updelclr #nodedelete').attr("class", "btnstyle3-dis");
            $("#hardwarecontent #ipnodeparamstable #nodename").val("");
            $("#hardwarecontent #ipnodeparamstable #nodeip").val("");
            $("#hardwarecontent #ipnodeparamstable #nodetimeout").val("5");

          var oTable = $('#ipnodestable').dataTable();
          oTable.fnClearTable();

          $.ajax({
             url: "json.htm?type=command&param=pingergetnodes&idx="+$.devIdx,
             async: false,
             dataType: 'json',
             success: function(data) {
              if (typeof data.result != 'undefined') {
                $.each(data.result, function(i,item){
                    var addId = oTable.fnAddData( {
                        "DT_RowId": item.idx,
                        "Name": item.Name,
                        "IP": item.IP,
                        "0": item.idx,
                        "1": item.Name,
                        "2": item.IP,
                        "3": item.Timeout
                    } );
                });
              }
             }
          });

            /* Add a click handler to the rows - this could be used as a callback */
            $("#ipnodestable tbody").off();
            $("#ipnodestable tbody").on( 'click', 'tr', function () {
                $('#updelclr #nodedelete').attr("class", "btnstyle3-dis");
                if ( $(this).hasClass('row_selected') ) {
                    $(this).removeClass('row_selected');
                    $('#updelclr #nodeupdate').attr("class", "btnstyle3-dis");
                    $("#hardwarecontent #ipnodeparamstable #nodename").val("");
                    $("#hardwarecontent #ipnodeparamstable #nodeip").val("");
                    $("#hardwarecontent #ipnodeparamstable #nodetimeout").val("5");
                }
                else {
                    var oTable = $('#ipnodestable').dataTable();
                    oTable.$('tr.row_selected').removeClass('row_selected');
                    $(this).addClass('row_selected');
                    $('#updelclr #nodeupdate').attr("class", "btnstyle3");
                    var anSelected = fnGetSelected( oTable );
                    if ( anSelected.length !== 0 ) {
                        var data = oTable.fnGetData( anSelected[0] );
                        var idx= data["DT_RowId"];
                        $("#updelclr #nodeupdate").attr("href", "javascript:PingerUpdateNode(" + idx + ")");
                        $('#updelclr #nodedelete').attr("class", "btnstyle3");
                        $("#updelclr #nodedelete").attr("href", "javascript:PingerDeleteNode(" + idx + ")");
                        $("#hardwarecontent #ipnodeparamstable #nodename").val(data["1"]);
                        $("#hardwarecontent #ipnodeparamstable #nodeip").val(data["2"]);
                        $("#hardwarecontent #ipnodeparamstable #nodetimeout").val(data["3"]);
                    }
                }
            });

          $('#modal').hide();
        }

        SetPingerSettings = function()
        {
            var Mode1=parseInt($("#hardwarecontent #pingsettingstable #pollinterval").val());
            if (Mode1<1)
                Mode1=30;
            var Mode2=parseInt($("#hardwarecontent #pingsettingstable #pingtimeout").val());
            if (Mode2<500)
                Mode2=500;
            $.ajax({
                 url: "json.htm?type=command&param=pingersetmode" +
                    "&idx=" + $.devIdx +
                    "&mode1=" + Mode1 +
                    "&mode2=" + Mode2,
                 async: false,
                 dataType: 'json',
                 success: function(data) {
                    bootbox.alert($.t('Settings saved'));
                 },
                 error: function(){
                    ShowNotify($.t('Problem Updating Settings!'), 2500, true);
                 }
            });
        }

        EditPinger = function(idx,name,Mode1,Mode2,Mode3,Mode4,Mode5,Mode6)
        {
            $.devIdx=idx;
            cursordefault();
            var htmlcontent = '';
            htmlcontent='<p><center><h2><span data-i18n="Device"></span>: ' + name + '</h2></center></p>\n';
            htmlcontent+=$('#pinger').html();
            $('#hardwarecontent').html(GetBackbuttonHTMLTable('ShowHardware')+htmlcontent);
            $('#hardwarecontent').i18n();

            $("#hardwarecontent #pingsettingstable #pollinterval").val(Mode1);
            $("#hardwarecontent #pingsettingstable #pingtimeout").val(Mode2);

            var oTable = $('#ipnodestable').dataTable( {
              "sDom": '<"H"lfrC>t<"F"ip>',
              "oTableTools": {
                "sRowSelect": "single",
              },
              "aaSorting": [[ 0, "desc" ]],
              "bSortClasses": false,
              "bProcessing": true,
              "bStateSave": true,
              "bJQueryUI": true,
              "aLengthMenu": [[25, 50, 100, -1], [25, 50, 100, "All"]],
              "iDisplayLength" : 25,
              "sPaginationType": "full_numbers",
              language: $.DataTableLanguage
            } );

            $('#hardwarecontent #idx').val(idx);

            RefreshPingerNodeTable();
        }

        MySensorsUpdateNode = function(nodeid)
        {
            if ($('#updelclr #nodeupdate').attr("class")=="btnstyle3-dis") {
                return;
            }
            var name = $("#updelclr #nodename").val();
			$.ajax({
				 url: "json.htm?type=command&param=mysensorsupdatenode" +
					"&idx=" + $.devIdx +
					"&nodeid=" + nodeid +
                   "&name=" + encodeURIComponent(name),
				 async: false,
				 dataType: 'json',
				 success: function(data) {
					RefreshMySensorsNodeTable();
				 },
				 error: function(){
					ShowNotify($.t('Problem Updating Node!'), 2500, true);
				 }
			});
        }

        MySensorsDeleteNode = function(nodeid)
        {
            if ($('#updelclr #nodedelete').attr("class")=="btnstyle3-dis") {
                return;
            }
            bootbox.confirm($.t("Are you sure to remove this Node?"), function(result) {
                if (result==true) {
                    $.ajax({
                         url: "json.htm?type=command&param=mysensorsremovenode" +
                            "&idx=" + $.devIdx +
                            "&nodeid=" + nodeid,
                         async: false,
                         dataType: 'json',
                         success: function(data) {
                            RefreshMySensorsNodeTable();
                         },
                         error: function(){
                            ShowNotify($.t('Problem Deleting Node!'), 2500, true);
                         }
                    });
                }
            });
        }

        MySensorsDeleteChild = function(nodeid,childid)
        {
            if ($('#updelclr #activedevicedelete').attr("class")=="btnstyle3-dis") {
                return;
            }
            bootbox.confirm($.t("Are you sure to remove this Child?"), function(result) {
                if (result==true) {
                    $.ajax({
                         url: "json.htm?type=command&param=mysensorsremovechild" +
                            "&idx=" + $.devIdx +
                            "&nodeid=" + nodeid +
                            "&childid=" + childid,
                         async: false,
                         dataType: 'json',
                         success: function(data) {
                            MySensorsRefreshActiveDevicesTable();
                         },
                         error: function(){
                            ShowNotify($.t('Problem Deleting Child!'), 2500, true);
                         }
                    });
                }
            });
        }
        MySensorsUpdateChild = function(nodeid,childid)
        {
            if ($('#updelclr #activedeviceupdate').attr("class")=="btnstyle3-dis") {
                return;
            }
            var bUseAck=$('#hardwarecontent #mchildsettings #Ack').is(":checked");
            var AckTimeout=parseInt($('#hardwarecontent #mchildsettings #AckTimeout').val());
            if (AckTimeout<100)
				AckTimeout=100;
			else if (AckTimeout>10000)
				AckTimeout=10000;
			$.ajax({
				 url: "json.htm?type=command&param=mysensorsupdatechild" +
					"&idx=" + $.devIdx +
					"&nodeid=" + nodeid +
					"&childid=" + childid +
					"&useack=" + bUseAck +
					"&acktimeout=" + AckTimeout,
				 async: false,
				 dataType: 'json',
				 success: function(data) {
					MySensorsRefreshActiveDevicesTable();
				 },
				 error: function(){
					ShowNotify($.t('Problem Updating Child!'), 2500, true);
				 }
			});
        }

		MySensorsRefreshActiveDevicesTable = function()
		{
			//$('#plancontent #delclractive #activedevicedelete').attr("class", "btnstyle3-dis");
			$('#hardwarecontent #trChildSettings').hide();
			var oTable = $('#mysensorsactivetable').dataTable();
			oTable.fnClearTable();
			if ($.nodeid==-1) {
				return;
			}
			$.ajax({
				url: "json.htm?type=command&param=mysensorsgetchilds" +
					"&idx=" + $.devIdx +
					"&nodeid=" + $.nodeid,
				async: false,
				dataType: 'json',
				success: function(data) {
					if (typeof data.result != 'undefined') {
						var totalItems=data.result.length;
						$.each(data.result, function(i,item){
							var addId = oTable.fnAddData( {
								"DT_RowId": item.child_id,
								"AckEnabled": item.use_ack,
								"AckTimeout": item.ack_timeout,
								"0": item.child_id,
								"1": item.type,
								"2": item.name,
								"3": item.Values,
								"4": item.use_ack,
								"5": item.ack_timeout,
								"6": item.LastReceived
							});
						});
					}
				}
			});
			/* Add a click handler to the rows - this could be used as a callback */
			$("#mysensorsactivetable tbody").off();
			$("#mysensorsactivetable tbody").on( 'click', 'tr', function () {
				if ( $(this).hasClass('row_selected') ) {
					$(this).removeClass('row_selected');
					$('#delclractive #activedevicedelete').attr("class", "btnstyle3-dis");
					$('#delclractive #activedeviceupdate').attr("class", "btnstyle3-dis");
					$('#hardwarecontent #trChildSettings').hide();
				}
				else {
					var oTable = $('#mysensorsactivetable').dataTable();
					oTable.$('tr.row_selected').removeClass('row_selected');
					$(this).addClass('row_selected');
					$('#activedevicedelete').attr("class", "btnstyle3");
					$('#activedeviceupdate').attr("class", "btnstyle3");
					$('#hardwarecontent #trChildSettings').show();
					var anSelected = fnGetSelected( oTable );
					if ( anSelected.length !== 0 ) {
						var data = oTable.fnGetData( anSelected[0] );
						var idx= data["DT_RowId"];
						$('#hardwarecontent #mchildsettings #Ack').prop('checked',(data["AckEnabled"]=="true"));
						$('#hardwarecontent #mchildsettings #AckTimeout').val(data["AckTimeout"]);
						$("#activedevicedelete").attr("href", "javascript:MySensorsDeleteChild(" + $.nodeid + "," + idx + ")");
						$("#activedeviceupdate").attr("href", "javascript:MySensorsUpdateChild(" + $.nodeid + "," + idx + ")");
					}
				}
			});

		  $('#modal').hide();
		}

        RefreshMySensorsNodeTable = function()
        {
		  $('#hardwarecontent #trChildSettings').hide();
		  $('#updelclr #trNodeSettings').hide();
          $('#modal').show();
          var oTable = $('#mysensorsnodestable').dataTable();
          oTable.fnClearTable();
		  $.nodeid=-1;
          $.ajax({
             url: "json.htm?type=command&param=mysensorsgetnodes&idx="+$.devIdx,
             async: false,
             dataType: 'json',
             success: function(data) {
              if (typeof data.result != 'undefined') {
                $.each(data.result, function(i,item){
                    var addId = oTable.fnAddData( {
                        "DT_RowId": item.idx,
                        "Name": item.Name,
                        "SketchName": item.SketchName,
                        "Version": item.Version,
                        "0": item.idx,
                        "1": item.Name,
                        "2": item.SketchName,
                        "3": item.Version,
                        "4": item.Childs,
                        "5": item.LastReceived
                    } );
                });
              }
             }
          });

            /* Add a click handler to the rows - this could be used as a callback */
            $("#mysensorsnodestable tbody").off();
            $("#mysensorsnodestable tbody").on( 'click', 'tr', function () {
                $('#updelclr #nodedelete').attr("class", "btnstyle3-dis");
                $('#updelclr #nodeupdate').attr("class", "btnstyle3-dis");
				$('#updelclr #trNodeSettings').hide();
                if ( $(this).hasClass('row_selected') ) {
                    $(this).removeClass('row_selected');
                }
                else {
                    var oTable = $('#mysensorsnodestable').dataTable();
                    oTable.$('tr.row_selected').removeClass('row_selected');
                    $(this).addClass('row_selected');
                    var anSelected = fnGetSelected( oTable );
                    if ( anSelected.length !== 0 ) {
                        var data = oTable.fnGetData( anSelected[0] );
                        var idx= data["DT_RowId"];
                        $('#updelclr #nodedelete').attr("class", "btnstyle3");
                        $('#updelclr #nodeupdate').attr("class", "btnstyle3");
						$('#updelclr #nodename').val(data["Name"]);
						$('#updelclr #trNodeSettings').show();
                        $("#updelclr #nodeupdate").attr("href", "javascript:MySensorsUpdateNode(" + idx + ")");
                        $("#updelclr #nodedelete").attr("href", "javascript:MySensorsDeleteNode(" + idx + ")");
                        $.nodeid = idx;
                        MySensorsRefreshActiveDevicesTable();
                    }
                }
            });

          $('#modal').hide();
        }

        EditMySensors = function(idx,name,Mode1,Mode2,Mode3,Mode4,Mode5,Mode6)
        {
            $.devIdx=idx;
            cursordefault();
            var htmlcontent = '';
            htmlcontent='<p><center><h2><span data-i18n="Device"></span>: ' + name + '</h2></center></p>\n';
            htmlcontent+=$('#mysensors').html();
            $('#hardwarecontent').html(GetBackbuttonHTMLTable('ShowHardware')+htmlcontent);
            $('#hardwarecontent').i18n();

            var oTable = $('#mysensorsnodestable').dataTable( {
              "sDom": '<"H"lfrC>t<"F"ip>',
              "oTableTools": {
                "sRowSelect": "single"
              },
              "aaSorting": [[ 0, "asc" ]],
              "bSortClasses": false,
              "bProcessing": true,
              "bStateSave": true,
              "bJQueryUI": true,
              "aLengthMenu": [[25, 50, 100, -1], [25, 50, 100, "All"]],
              "iDisplayLength" : 25,
              "sPaginationType": "full_numbers",
              language: $.DataTableLanguage
            } );
			oTable = $('#mysensorsactivetable').dataTable( {
				"sDom": '<"H"lfrC>t<"F"ip>',
					"oTableTools": {
					"sRowSelect": "single"
				},
				"aaSorting": [[ 0, "asc" ]],
				"bSortClasses": false,
				"bProcessing": true,
				"bStateSave": true,
				"bJQueryUI": true,
				"aLengthMenu": [[25, 50, 100, -1], [25, 50, 100, "All"]],
				"iDisplayLength" : 25,
				"sPaginationType": "full_numbers",
				language: $.DataTableLanguage
			});

            $('#hardwarecontent #idx').val(idx);
			RefreshMySensorsNodeTable();
        }

        AddKodiNode = function () {
            var name = $("#hardwarecontent #kodinodeparamstable #nodename").val();
            if (name == "") {
                ShowNotify($.t('Please enter a Name!'), 2500, true);
                return;
            }
            var ip = $("#hardwarecontent #kodinodeparamstable #nodeip").val();
            if (ip == "") {
                ShowNotify($.t('Please enter a IP Address!'), 2500, true);
                return;
            }
            var Port = parseInt($("#hardwarecontent #kodinodeparamstable #nodeport").val());
            if (Port == "") {
                ShowNotify($.t('Please enter a Port!'), 2500, true);
                return;
            }

            $.ajax({
                url: "json.htm?type=command&param=kodiaddnode" +
                   "&idx=" + $.devIdx +
                   "&name=" + encodeURIComponent(name) +
                   "&ip=" + ip +
                   "&port=" + Port,
                async: false,
                dataType: 'json',
                success: function (data) {
                    RefreshKodiNodeTable();
                },
                error: function () {
                    ShowNotify($.t('Problem Adding Node!'), 2500, true);
                }
            });
        }

        KodiDeleteNode = function (nodeid) {
            if ($('#updelclr #nodedelete').attr("class") == "btnstyle3-dis") {
                return;
            }
            bootbox.confirm($.t("Are you sure to remove this Node?"), function (result) {
                if (result == true) {
                    $.ajax({
                        url: "json.htm?type=command&param=kodiremovenode" +
                           "&idx=" + $.devIdx +
                           "&nodeid=" + nodeid,
                        async: false,
                        dataType: 'json',
                        success: function (data) {
                            RefreshKodiNodeTable();
                        },
                        error: function () {
                            ShowNotify($.t('Problem Deleting Node!'), 2500, true);
                        }
                    });
                }
            });
        }

        KodiClearNodes = function () {
            bootbox.confirm($.t("Are you sure to delete ALL Nodes?\n\nThis action can not be undone!"), function (result) {
                if (result == true) {
                    $.ajax({
                        url: "json.htm?type=command&param=kodiclearnodes" +
                           "&idx=" + $.devIdx,
                        async: false,
                        dataType: 'json',
                        success: function (data) {
                            RefreshKodiNodeTable();
                        }
                    });
                }
            });
        }

        KodiUpdateNode = function (nodeid) {
            if ($('#updelclr #nodedelete').attr("class") == "btnstyle3-dis") {
                return;
            }

            var name = $("#hardwarecontent #kodinodeparamstable #nodename").val();
            if (name == "") {
                ShowNotify($.t('Please enter a Name!'), 2500, true);
                return;
            }
            var ip = $("#hardwarecontent #kodinodeparamstable #nodeip").val();
            if (ip == "") {
                ShowNotify($.t('Please enter a IP Address!'), 2500, true);
                return;
            }
            var Port = parseInt($("#hardwarecontent #kodinodeparamstable #nodeport").val());
            if (Port == "") {
                ShowNotify($.t('Please enter a Port!'), 2500, true);
                return;
            }

            $.ajax({
                url: "json.htm?type=command&param=kodiupdatenode" +
                   "&idx=" + $.devIdx +
                   "&nodeid=" + nodeid +
                   "&name=" + encodeURIComponent(name) +
                   "&ip=" + ip +
                   "&port=" + Port,
                async: false,
                dataType: 'json',
                success: function (data) {
                    RefreshKodiNodeTable();
                },
                error: function () {
                    ShowNotify($.t('Problem Updating Node!'), 2500, true);
                }
            });
        }

        RefreshKodiNodeTable = function () {
            $('#modal').show();
            $('#updelclr #nodeupdate').attr("class", "btnstyle3-dis");
            $('#updelclr #nodedelete').attr("class", "btnstyle3-dis");
            $("#hardwarecontent #kodinodeparamstable #nodename").val("");
            $("#hardwarecontent #kodinodeparamstable #nodeip").val("");
            $("#hardwarecontent #kodinodeparamstable #nodeport").val("9090");

            var oTable = $('#kodinodestable').dataTable();
            oTable.fnClearTable();

            $.ajax({
                url: "json.htm?type=command&param=kodigetnodes&idx=" + $.devIdx,
                async: false,
                dataType: 'json',
                success: function (data) {
                    if (typeof data.result != 'undefined') {
                        $.each(data.result, function (i, item) {
                            var addId = oTable.fnAddData({
                                "DT_RowId": item.idx,
                                "Name": item.Name,
                                "IP": item.IP,
                                "0": item.idx,
                                "1": item.Name,
                                "2": item.IP,
                                "3": item.Port
                            });
                        });
                    }
                }
            });

            /* Add a click handler to the rows - this could be used as a callback */
            $("#kodinodestable tbody").off();
            $("#kodinodestable tbody").on('click', 'tr', function () {
                $('#updelclr #nodedelete').attr("class", "btnstyle3-dis");
                if ($(this).hasClass('row_selected')) {
                    $(this).removeClass('row_selected');
                    $('#updelclr #nodeupdate').attr("class", "btnstyle3-dis");
                    $("#hardwarecontent #kodinodeparamstable #nodename").val("");
                    $("#hardwarecontent #kodinodeparamstable #nodeip").val("");
                    $("#hardwarecontent #kodinodeparamstable #nodeport").val("9090");
                }
                else {
                    var oTable = $('#kodinodestable').dataTable();
                    oTable.$('tr.row_selected').removeClass('row_selected');
                    $(this).addClass('row_selected');
                    $('#updelclr #nodeupdate').attr("class", "btnstyle3");
                    var anSelected = fnGetSelected(oTable);
                    if (anSelected.length !== 0) {
                        var data = oTable.fnGetData(anSelected[0]);
                        var idx = data["DT_RowId"];
                        $("#updelclr #nodeupdate").attr("href", "javascript:KodiUpdateNode(" + idx + ")");
                        $('#updelclr #nodedelete').attr("class", "btnstyle3");
                        $("#updelclr #nodedelete").attr("href", "javascript:KodiDeleteNode(" + idx + ")");
                        $("#hardwarecontent #kodinodeparamstable #nodename").val(data["1"]);
                        $("#hardwarecontent #kodinodeparamstable #nodeip").val(data["2"]);
                        $("#hardwarecontent #kodinodeparamstable #nodeport").val(data["3"]);
                    }
                }
            });

            $('#modal').hide();
        }

        SetKodiSettings = function () {
            var Mode1 = parseInt($("#hardwarecontent #kodisettingstable #pollinterval").val());
            if (Mode1 < 1)
                Mode1 = 30;
            var Mode2 = parseInt($("#hardwarecontent #kodisettingstable #pingtimeout").val());
            if (Mode2 < 500)
                Mode2 = 500;
            $.ajax({
                url: "json.htm?type=command&param=kodisetmode" +
                   "&idx=" + $.devIdx +
                   "&mode1=" + Mode1 +
                   "&mode2=" + Mode2,
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

        EditKodi = function (idx, name, Mode1, Mode2, Mode3, Mode4, Mode5, Mode6) {
            $.devIdx = idx;
            cursordefault();
            var htmlcontent = '';
            htmlcontent = '<p><center><h2><span data-i18n="Device"></span>: ' + name + '</h2></center></p>\n';
            htmlcontent += $('#kodi').html();
            $('#hardwarecontent').html(GetBackbuttonHTMLTable('ShowHardware') + htmlcontent);
            $('#hardwarecontent').i18n();

            $("#hardwarecontent #kodisettingstable #pollinterval").val(Mode1);
            $("#hardwarecontent #kodisettingstable #pingtimeout").val(Mode2);

            var oTable = $('#kodinodestable').dataTable({
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

            RefreshKodiNodeTable();
        }

        /* Start of Panasonic Plugin Code */

        PanasonicAddNode = function () {
            var name = $("#hardwarecontent #panasonicnodeparamstable #nodename").val();
            if (name == "") {
                ShowNotify($.t('Please enter a Name!'), 2500, true);
                return;
            }
            var ip = $("#hardwarecontent #panasonicnodeparamstable #nodeip").val();
            if (ip == "") {
                ShowNotify($.t('Please enter a IP Address!'), 2500, true);
                return;
            }
            var Port = parseInt($("#hardwarecontent #panasonicnodeparamstable #nodeport").val());
            if (Port == "") {
                ShowNotify($.t('Please enter a Port!'), 2500, true);
                return;
            }

            $.ajax({
                url: "json.htm?type=command&param=panasonicaddnode" +
                   "&idx=" + $.devIdx +
                   "&name=" + encodeURIComponent(name) +
                   "&ip=" + ip +
                   "&port=" + Port,
                async: false,
                dataType: 'json',
                success: function (data) {
                    RefreshPanasonicNodeTable();
                },
                error: function () {
                    ShowNotify($.t('Problem Adding Node!'), 2500, true);
                }
            });
        }

        PanasonicDeleteNode = function (nodeid) {
            if ($('#updelclr #nodedelete').attr("class") == "btnstyle3-dis") {
                return;
            }
            bootbox.confirm($.t("Are you sure to remove this Node?"), function (result) {
                if (result == true) {
                    $.ajax({
                        url: "json.htm?type=command&param=panasonicremovenode" +
                           "&idx=" + $.devIdx +
                           "&nodeid=" + nodeid,
                        async: false,
                        dataType: 'json',
                        success: function (data) {
                            RefreshPanasonicNodeTable();
                        },
                        error: function () {
                            ShowNotify($.t('Problem Deleting Node!'), 2500, true);
                        }
                    });
                }
            });
        }

        PanasonicClearNodes = function () {
            bootbox.confirm($.t("Are you sure to delete ALL Nodes?\n\nThis action can not be undone!"), function (result) {
                if (result == true) {
                    $.ajax({
                        url: "json.htm?type=command&param=panasonicclearnodes" +
                           "&idx=" + $.devIdx,
                        async: false,
                        dataType: 'json',
                        success: function (data) {
                            RefreshPanasonicNodeTable();
                        }
                    });
                }
            });
        }

        PanasonicUpdateNode = function (nodeid) {
            if ($('#updelclr #nodedelete').attr("class") == "btnstyle3-dis") {
                return;
            }

            var name = $("#hardwarecontent #panasonicnodeparamstable #nodename").val();
            if (name == "") {
                ShowNotify($.t('Please enter a Name!'), 2500, true);
                return;
            }
            var ip = $("#hardwarecontent #panasonicnodeparamstable #nodeip").val();
            if (ip == "") {
                ShowNotify($.t('Please enter a IP Address!'), 2500, true);
                return;
            }
            var Port = parseInt($("#hardwarecontent #panasonicnodeparamstable #nodeport").val());
            if (Port == "") {
                ShowNotify($.t('Please enter a Port!'), 2500, true);
                return;
            }

            $.ajax({
                url: "json.htm?type=command&param=panasonicupdatenode" +
                   "&idx=" + $.devIdx +
                   "&nodeid=" + nodeid +
                   "&name=" + encodeURIComponent(name) +
                   "&ip=" + ip +
                   "&port=" + Port,
                async: false,
                dataType: 'json',
                success: function (data) {
                    RefreshPanasonicNodeTable();
                },
                error: function () {
                    ShowNotify($.t('Problem Updating Node!'), 2500, true);
                }
            });
        }

        RefreshPanasonicNodeTable = function () {
            $('#modal').show();
            $('#updelclr #nodeupdate').attr("class", "btnstyle3-dis");
            $('#updelclr #nodedelete').attr("class", "btnstyle3-dis");
            $("#hardwarecontent #panasonicnodeparamstable #nodename").val("");
            $("#hardwarecontent #panasonicnodeparamstable #nodeip").val("");
            $("#hardwarecontent #panasonicnodeparamstable #nodeport").val("55000");

            var oTable = $('#panasonicnodestable').dataTable();
            oTable.fnClearTable();

            $.ajax({
                url: "json.htm?type=command&param=panasonicgetnodes&idx=" + $.devIdx,
                async: false,
                dataType: 'json',
                success: function (data) {
                    if (typeof data.result != 'undefined') {
                        $.each(data.result, function (i, item) {
                            var addId = oTable.fnAddData({
                                "DT_RowId": item.idx,
                                "Name": item.Name,
                                "IP": item.IP,
                                "0": item.idx,
                                "1": item.Name,
                                "2": item.IP,
                                "3": item.Port
                            });
                        });
                    }
                }
            });

            /* Add a click handler to the rows - this could be used as a callback */
            $("#panasonicnodestable tbody").off();
            $("#panasonicnodestable tbody").on('click', 'tr', function () {
                $('#updelclr #nodedelete').attr("class", "btnstyle3-dis");
                if ($(this).hasClass('row_selected')) {
                    $(this).removeClass('row_selected');
                    $('#updelclr #nodeupdate').attr("class", "btnstyle3-dis");
                    $("#hardwarecontent #panasonicnodeparamstable #nodename").val("");
                    $("#hardwarecontent #panasonicnodeparamstable #nodeip").val("");
                    $("#hardwarecontent #panasonicnodeparamstable #nodeport").val("55000");
                }
                else {
                    var oTable = $('#panasonicnodestable').dataTable();
                    oTable.$('tr.row_selected').removeClass('row_selected');
                    $(this).addClass('row_selected');
                    $('#updelclr #nodeupdate').attr("class", "btnstyle3");
                    var anSelected = fnGetSelected(oTable);
                    if (anSelected.length !== 0) {
                        var data = oTable.fnGetData(anSelected[0]);
                        var idx = data["DT_RowId"];
                        $("#updelclr #nodeupdate").attr("href", "javascript:PanasonicUpdateNode(" + idx + ")");
                        $('#updelclr #nodedelete').attr("class", "btnstyle3");
                        $("#updelclr #nodedelete").attr("href", "javascript:PanasonicDeleteNode(" + idx + ")");
                        $("#hardwarecontent #panasonicnodeparamstable #nodename").val(data["1"]);
                        $("#hardwarecontent #panasonicnodeparamstable #nodeip").val(data["2"]);
                        $("#hardwarecontent #panasonicnodeparamstable #nodeport").val(data["3"]);
                    }
                }
            });

            $('#modal').hide();
        }

        SetPanasonicSettings = function () {
            var Mode1 = parseInt($("#hardwarecontent #panasonicsettingstable #pollinterval").val());
            if (Mode1 < 1)
                Mode1 = 30;
            var Mode2 = parseInt($("#hardwarecontent #panasonicsettingstable #pingtimeout").val());
            if (Mode2 < 500)
                Mode2 = 500;
            $.ajax({
                url: "json.htm?type=command&param=panasonicsetmode" +
                   "&idx=" + $.devIdx +
                   "&mode1=" + Mode1 +
                   "&mode2=" + Mode2,
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

        EditPanasonic = function (idx, name, Mode1, Mode2, Mode3, Mode4, Mode5, Mode6) {
            $.devIdx = idx;
            cursordefault();
            var htmlcontent = '';
            htmlcontent = '<p><center><h2><span data-i18n="Device"></span>: ' + name + '</h2></center></p>\n';
            htmlcontent += $('#panasonic').html();
            $('#hardwarecontent').html(GetBackbuttonHTMLTable('ShowHardware') + htmlcontent);
            $('#hardwarecontent').i18n();

            $("#hardwarecontent #panasonicsettingstable #pollinterval").val(Mode1);
            $("#hardwarecontent #panasonicsettingstable #pingtimeout").val(Mode2);

            var oTable = $('#panasonicnodestable').dataTable({
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

            RefreshPanasonicNodeTable();
        }

        /* End of Panasonic Plugin Code */

        /* Start of BleBox Plugin Code */

        BleBoxAddNode = function () {
            var name = $("#hardwarecontent #bleboxnodeparamstable #nodename").val();
            if (name == "") {
                ShowNotify($.t('Please enter a Name!'), 2500, true);
                return;
            }
            var ip = $("#hardwarecontent #bleboxnodeparamstable #nodeip").val();
            if (ip == "") {
                ShowNotify($.t('Please enter a IP Address!'), 2500, true);
                return;
            }

            $.ajax({
                url: "json.htm?type=command&param=bleboxaddnode" +
                   "&idx=" + $.devIdx +
                   "&name=" + encodeURIComponent(name) +
                   "&ip=" + ip,
                async: false,
                dataType: 'json',
                success: function (data) {
                    RefreshBleBoxNodeTable();
                },
                error: function () {
                    ShowNotify($.t('Problem Adding Node!'), 2500, true);
                }
            });
        }

        BleBoxDeleteNode = function (nodeid) {
            if ($('#updelclr #nodedelete').attr("class") == "btnstyle3-dis") {
                return;
            }
            bootbox.confirm($.t("Are you sure to remove this Node?"), function (result) {
                if (result == true) {
                    $.ajax({
                        url: "json.htm?type=command&param=bleboxremovenode" +
                           "&idx=" + $.devIdx +
                           "&nodeid=" + nodeid,
                        async: false,
                        dataType: 'json',
                        success: function (data) {
                            RefreshBleBoxNodeTable();
                        },
                        error: function () {
                            ShowNotify($.t('Problem Deleting Node!'), 2500, true);
                        }
                    });
                }
            });
        }

        BleBoxClearNodes = function () {
            bootbox.confirm($.t("Are you sure to delete ALL Nodes?\n\nThis action can not be undone!"), function (result) {
                if (result == true) {
                    $.ajax({
                        url: "json.htm?type=command&param=bleboxclearnodes" +
                           "&idx=" + $.devIdx,
                        async: false,
                        dataType: 'json',
                        success: function (data) {
                            RefreshBleBoxNodeTable();
                        }
                    });
                }
            });
        }

        BleBoxUpdateNode = function (nodeid) {
            if ($('#updelclr #nodedelete').attr("class") == "btnstyle3-dis") {
                return;
            }

            var name = $("#hardwarecontent #bleboxnodeparamstable #nodename").val();
            if (name == "") {
                ShowNotify($.t('Please enter a Name!'), 2500, true);
                return;
            }
            var ip = $("#hardwarecontent #bleboxnodeparamstable #nodeip").val();
            if (ip == "") {
                ShowNotify($.t('Please enter a IP Address!'), 2500, true);
                return;
            }

            $.ajax({
                url: "json.htm?type=command&param=bleboxupdatenode" +
                   "&idx=" + $.devIdx +
                   "&nodeid=" + nodeid +
                   "&name=" + encodeURIComponent(name) +
                   "&ip=" + ip,
                async: false,
                dataType: 'json',
                success: function (data) {
                    RefreshBleBoxNodeTable();
                },
                error: function () {
                    ShowNotify($.t('Problem Updating Node!'), 2500, true);
                }
            });
        }

        RefreshBleBoxNodeTable = function () {
            $('#modal').show();
            $('#updelclr #nodeupdate').attr("class", "btnstyle3-dis");
            $('#updelclr #nodedelete').attr("class", "btnstyle3-dis");
            $("#hardwarecontent #bleboxnodeparamstable #nodename").val("");
            $("#hardwarecontent #bleboxnodeparamstable #nodeip").val("");
			$("#hardwarecontent #bleboxnodeparamstable #type").val("");

            var oTable = $('#bleboxnodestable').dataTable();
            oTable.fnClearTable();

            $.ajax({
                url: "json.htm?type=command&param=bleboxgetnodes&idx=" + $.devIdx,
                async: false,
                dataType: 'json',
                success: function (data) {
                    if (typeof data.result != 'undefined') {
                        $.each(data.result, function (i, item) {
                            var addId = oTable.fnAddData({
                                "DT_RowId": item.idx,
                                "Name": item.Name,
                                "IP": item.IP,
                                "0": item.idx,
                                "1": item.Name,
                                "2": item.IP,
								"3": item.Type
                            });
                        });
                    }
                }
            });

            /* Add a click handler to the rows - this could be used as a callback */
            $("#bleboxnodestable tbody").off();
            $("#bleboxnodestable tbody").on('click', 'tr', function () {
                $('#updelclr #nodedelete').attr("class", "btnstyle3-dis");
                if ($(this).hasClass('row_selected')) {
                    $(this).removeClass('row_selected');
                    $('#updelclr #nodeupdate').attr("class", "btnstyle3-dis");
                    $("#hardwarecontent #bleboxnodeparamstable #nodename").val("");
                    $("#hardwarecontent #bleboxnodeparamstable #nodeip").val("");
					$("#hardwarecontent #bleboxnodeparamstable #type").val("");
                }
                else {
                    var oTable = $('#bleboxnodestable').dataTable();
                    oTable.$('tr.row_selected').removeClass('row_selected');
                    $(this).addClass('row_selected');
                    $('#updelclr #nodeupdate').attr("class", "btnstyle3");
                    var anSelected = fnGetSelected(oTable);
                    if (anSelected.length !== 0) {
                        var data = oTable.fnGetData(anSelected[0]);
                        var idx = data["DT_RowId"];
                        $("#updelclr #nodeupdate").attr("href", "javascript:BleBoxUpdateNode(" + idx + ")");
                        $('#updelclr #nodedelete').attr("class", "btnstyle3");
                        $("#updelclr #nodedelete").attr("href", "javascript:BleBoxDeleteNode(" + idx + ")");
                        $("#hardwarecontent #bleboxnodeparamstable #nodename").val(data["1"]);
                        $("#hardwarecontent #bleboxnodeparamstable #nodeip").val(data["2"]);
						$("#hardwarecontent #bleboxnodeparamstable #type").val(data["3"]);
                    }
                }
            });

            $('#modal').hide();
        }

        SetBleBoxSettings = function () {
            var Mode1 = parseInt($("#hardwarecontent #bleboxsettingstable #pollinterval").val());
            if (Mode1 < 1)
                Mode1 = 30;
            var Mode2 = parseInt($("#hardwarecontent #bleboxsettingstable #pingtimeout").val());
            if (Mode2 < 1000)
                Mode2 = 1000;
            $.ajax({
                url: "json.htm?type=command&param=bleboxsetmode" +
                   "&idx=" + $.devIdx +
                   "&mode1=" + Mode1 +
                   "&mode2=" + Mode2,
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

        EditBleBox = function (idx, name, Mode1, Mode2, Mode3, Mode4, Mode5, Mode6) {
            $.devIdx = idx;
            cursordefault();
            var htmlcontent = '';
            htmlcontent = '<p><center><h2><span data-i18n="Device"></span>: ' + name + '</h2></center></p>\n';
            htmlcontent += $('#blebox').html();
            $('#hardwarecontent').html(GetBackbuttonHTMLTable('ShowHardware') + htmlcontent);
            $('#hardwarecontent').i18n();

            $("#hardwarecontent #bleboxsettingstable #pollinterval").val(Mode1);
            $("#hardwarecontent #bleboxsettingstable #pingtimeout").val(Mode2);

            var oTable = $('#bleboxnodestable').dataTable({
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

            RefreshBleBoxNodeTable();
        }

        /* End of BleBox Plugin Code */

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
                                "0": item.idx,
                                "1": item.Name,
                                "2": item.Mac
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
            var Mode2 = parseInt($("#hardwarecontent #lmssettingstable #pingtimeout").val());
            if (Mode2 < 500)
                Mode2 = 500;
            $.ajax({
                url: "json.htm?type=command&param=lmssetmode" +
                   "&idx=" + $.devIdx +
                   "&mode1=" + Mode1 +
                   "&mode2=" + Mode2,
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
            $("#hardwarecontent #lmssettingstable #pingtimeout").val(Mode2);

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

        EditSBFSpot = function (idx, name, Mode1, Mode2, Mode3, Mode4, Mode5, Mode6)
        {
            $.devIdx=idx;
            cursordefault();
            var htmlcontent = '';
            htmlcontent='<p><center><h2><span data-i18n="Device"></span>: ' + name + '</h2></center></p>\n';
            htmlcontent+=$('#sbfspot').html();
            $('#hardwarecontent').html(GetBackbuttonHTMLTable('ShowHardware')+htmlcontent);
            $('#hardwarecontent').i18n();
            $('#hardwarecontent #idx').val(idx);
            $('#hardwarecontent #btnimportolddata').click(function (e) {
                e.preventDefault();
                bootbox.alert($.t('Importing old data, this could take a while!<br>You should automaticly return on the Dashboard'));
                $.post("sbfspotimportolddata.webem", $("#hardwarecontent #sbfspot").serialize(), function(data) {
                    SwitchLayout('Dashboard');
                });
            });
        }

        EditS0MeterType = function(idx,name,Address)
        {
            cursordefault();
            var htmlcontent = '';
            htmlcontent='<p><center><h2><span data-i18n="Device"></span>: ' + name + '</h2></center></p>\n';
            htmlcontent+=$('#s0metertypeedit').html();
            $('#hardwarecontent').html(GetBackbuttonHTMLTable('ShowHardware')+htmlcontent);
            $('#hardwarecontent').i18n();

            $('#hardwarecontent #submitbuttons0meter').click(function (e) {
                e.preventDefault();
                SetS0MeterType();
            });
            $('#hardwarecontent #idx').val(idx);

            var res=Address.split(";");

            $('#hardwarecontent #combom1type').val(res[0]);
            if (res[1]!=0) {
                $('#hardwarecontent #M1PulsesPerHour').val(res[1]);
            }
            else {
                $('#hardwarecontent #M1PulsesPerHour').val(2000);
            }
            $('#hardwarecontent #combom2type').val(res[2]);
            if (res[3]!=0) {
                $('#hardwarecontent #M2PulsesPerHour').val(res[3]);
            }
            else {
                $('#hardwarecontent #M2PulsesPerHour').val(2000);
            }
            $('#hardwarecontent #combom3type').val(res[4]);
            if (res[5]!=0) {
                $('#hardwarecontent #M3PulsesPerHour').val(res[5]);
            }
            else {
                $('#hardwarecontent #M3PulsesPerHour').val(2000);
            }
            $('#hardwarecontent #combom4type').val(res[6]);
            if (res[7]!=0) {
                $('#hardwarecontent #M4PulsesPerHour').val(res[7]);
            }
            else {
                $('#hardwarecontent #M4PulsesPerHour').val(2000);
            }
            $('#hardwarecontent #combom5type').val(res[8]);
            if (res[9]!=0) {
                $('#hardwarecontent #M5PulsesPerHour').val(res[9]);
            }
            else {
                $('#hardwarecontent #M5PulsesPerHour').val(2000);
            }
        }

        EditLimitlessType = function(idx,name,Mode1,Mode2,Mode3,Mode4,Mode5,Mode6)
        {
            cursordefault();
            var htmlcontent = '';
            htmlcontent='<p><center><h2><span data-i18n="Device"></span>: ' + name + '</h2></center></p>\n';
            htmlcontent+=$('#limitlessmetertype').html();
            $('#hardwarecontent').html(GetBackbuttonHTMLTable('ShowHardware')+htmlcontent);
            $('#hardwarecontent').i18n();

            $('#hardwarecontent #submitbuttonlimitless').click(function (e) {
                e.preventDefault();
                SetLimitlessType();
            });

            $('#hardwarecontent #idx').val(idx);
            $('#hardwarecontent #combom1type').val(Mode1);
        }

        EditCCUSB = function(idx,name,Mode1,Mode2,Mode3,Mode4,Mode5,Mode6)
        {
            $.devIdx=idx;
            cursordefault();
            var htmlcontent = '';
            htmlcontent='<p><center><h2><span data-i18n="Device"></span>: ' + name + '</h2></center></p>\n';
            htmlcontent+=$('#ccusbeedit').html();
            $('#hardwarecontent').html(GetBackbuttonHTMLTable('ShowHardware')+htmlcontent);
            $('#hardwarecontent').i18n();

            $('#hardwarecontent #submitbuttonccusb').click(function (e) {
                e.preventDefault();
                SetCCUSBType();
            });

            $('#hardwarecontent #idx').val(idx);
            $('#hardwarecontent #CCBaudrate').val(Mode1);
        }

		$scope.ZWaveCheckIncludeReady = function() {
			if (typeof $scope.mytimer != 'undefined') {
				$interval.cancel($scope.mytimer);
				$scope.mytimer = undefined;
			}
			$.ajax({
			 url: "json.htm?type=command&param=zwaveisnodeincluded&idx="+$.devIdx,
			 async: false,
			 dataType: 'json',
				 success: function(data) {
					if (data.status == "OK") {
						if (data.result==true) {
							//Node included
					        $scope.ozw_node_id = data.node_id;
							$scope.ozw_node_desc = data.node_product_name;
							$("#IncludeZWaveDialog #izwd_waiting").hide();
							$("#IncludeZWaveDialog #izwd_result").show();
						}
						else {
							//Not ready yet
							$scope.mytimer=$interval(function() {
								$scope.ZWaveCheckIncludeReady();
							}, 1000);
						}
					}
					else {
						$scope.mytimer=$interval(function() {
							$scope.ZWaveCheckIncludeReady();
						}, 1000);
					}
				 },
				 error: function(){
					$scope.mytimer=$interval(function() {
						$scope.ZWaveCheckIncludeReady();
					}, 1000);
				 }
			});
		}

		OnZWaveAbortInclude = function()
		{
			$http({
			 url: "json.htm?type=command&param=zwavecancel&idx="+$.devIdx,
			 async: true,
			 dataType: 'json'
			}).success(function(data) {
				$('#IncludeZWaveDialog').modal('hide');
			 }).error(function() {
				$('#IncludeZWaveDialog').modal('hide');
			 });
		}

		OnZWaveCloseInclude = function()
		{
			$('#IncludeZWaveDialog').modal('hide');
			RefreshOpenZWaveNodeTable();
		}

        ZWaveIncludeNode = function(isSecure)
        {
			if (typeof $scope.mytimer != 'undefined') {
				$interval.cancel($scope.mytimer);
				$scope.mytimer = undefined;
			}
			$("#IncludeZWaveDialog #izwd_waiting").show();
			$("#IncludeZWaveDialog #izwd_result").hide();
            $.ajax({
                 url: "json.htm?type=command&param=zwaveinclude&idx=" + $.devIdx + "&secure=" + isSecure,
                 async: false,
                 dataType: 'json',
                 success: function(data) {
					$scope.ozw_node_id = "-";
					$scope.ozw_node_desc = "-";
           			$('#IncludeZWaveDialog').modal('show');
					$scope.mytimer=$interval(function() {
						$scope.ZWaveCheckIncludeReady();
					}, 1000);
                 }
            });
        }

		$scope.ZWaveCheckExcludeReady = function() {
			if (typeof $scope.mytimer != 'undefined') {
				$interval.cancel($scope.mytimer);
				$scope.mytimer = undefined;
			}
			$.ajax({
			 url: "json.htm?type=command&param=zwaveisnodeexcluded&idx="+$.devIdx,
			 async: false,
			 dataType: 'json',
				 success: function(data) {
					if (data.status == "OK") {
						if (data.result==true) {
							//Node excluded
							$scope.ozw_node_id = data.node_id;
							$scope.ozw_node_desc = "-";
							$("#ExcludeZWaveDialog #ezwd_waiting").hide();
							$("#ExcludeZWaveDialog #ezwd_result").show();
						}
						else {
							//Not ready yet
							$scope.mytimer=$interval(function() {
								$scope.ZWaveCheckExcludeReady();
							}, 1000);
						}
					}
					else {
						$scope.mytimer=$interval(function() {
							$scope.ZWaveCheckExcludeReady();
						}, 1000);
					}
				 },
				 error: function(){
					$scope.mytimer=$interval(function() {
						$scope.ZWaveCheckExcludeReady();
					}, 1000);
				 }
			});
		}

		OnZWaveAbortExclude = function()
		{
			$http({
			 url: "json.htm?type=command&param=zwavecancel&idx="+$.devIdx,
			 async: true,
			 dataType: 'json'
			}).success(function(data) {
				$('#ExcludeZWaveDialog').modal('hide');
			 }).error(function() {
				$('#ExcludeZWaveDialog').modal('hide');
			 });
		}

		OnZWaveCloseExclude = function()
		{
			$('#ExcludeZWaveDialog').modal('hide');
			RefreshOpenZWaveNodeTable();
		}

        ZWaveExcludeNode = function()
        {
			if (typeof $scope.mytimer != 'undefined') {
				$interval.cancel($scope.mytimer);
				$scope.mytimer = undefined;
			}
			$("#ExcludeZWaveDialog #ezwd_waiting").show();
			$("#ExcludeZWaveDialog #ezwd_result").hide();
            $.ajax({
                 url: "json.htm?type=command&param=zwaveexclude&idx=" + $.devIdx,
                 async: false,
                 dataType: 'json',
                 success: function(data) {
					$scope.ozw_node_id = data.node_id;
					$scope.ozw_node_desc = "-";
           			$('#ExcludeZWaveDialog').modal('show');
					$scope.mytimer=$interval(function() {
						$scope.ZWaveCheckExcludeReady();
					}, 1000);
                 }
            });
        }

        DeleteNode = function(idx)
        {
            if ($('#updelclr #nodedelete').attr("class")=="btnstyle3-dis") {
                return;
            }
            bootbox.confirm($.t("Are you sure to remove this Node?"), function(result) {
                if (result==true) {
                  $.ajax({
                    url: "json.htm?type=command&param=deletezwavenode" +
                        "&idx=" + idx,
                     async: false,
                     dataType: 'json',
                     success: function(data) {
                        bootbox.alert($.t('Node marked for Delete. This could take some time!'));
                        RefreshOpenZWaveNodeTable();
                     }
                  });
                }
            });
        }

        UpdateNode = function(idx)
        {
            if ($('#updelclr #nodeupdate').attr("class")=="btnstyle3-dis") {
                return;
            }
            var name=$("#hardwarecontent #nodeparamstable #nodename").val();
            if (name=="")
            {
                ShowNotify($.t('Please enter a Name!'), 2500, true);
                return;
            }

            var bEnablePolling=$('#hardwarecontent #nodeparamstable #EnablePolling').is(":checked");
            $.ajax({
                 url: "json.htm?type=command&param=updatezwavenode" +
                    "&idx=" + idx +
                    "&name=" + encodeURIComponent(name) +
                    "&EnablePolling=" + bEnablePolling,
                 async: false,
                 dataType: 'json',
                 success: function(data) {
                    RefreshOpenZWaveNodeTable();
                 },
                 error: function(){
                    ShowNotify($.t('Problem updating Node!'), 2500, true);
                 }
            });
        }

        RequestZWaveConfiguration = function(idx)
        {
            $.ajax({
                 url: "json.htm?type=command&param=requestzwavenodeconfig" +
                    "&idx=" + idx,
                 async: false,
                 dataType: 'json',
                 success: function(data) {
                    bootbox.alert($.t('Configuration requested from Node. If the Node is asleep, this could take a while!'));
                    RefreshOpenZWaveNodeTable();
                 },
                 error: function(){
                        ShowNotify($.t('Problem requesting Node Configuration!'), 2500, true);
                 }
            });
        }

        ApplyZWaveConfiguration = function(idx)
        {
            var valueList="";
            var $list = $('#hardwarecontent #configuration input');
            if (typeof $list != 'undefined') {
                var ControlCnt = $list.length;
                // Now loop through list of controls

                $list.each( function() {
                    var id = $(this).prop("id");      // get id
                    var value = encodeURIComponent(btoa($(this).prop("value")));      // get value

                    valueList+=id+"_"+value+"_";
                });
            }

            //Now for the Lists
            $list = $('#hardwarecontent #configuration select');
            if (typeof $list != 'undefined') {
                var ControlCnt = $list.length;
                // Now loop through list of controls
                $list.each( function() {
                    var id = $(this).prop("id");      // get id
                    var value = encodeURIComponent(btoa($(this).find(":selected").text()));      // get value
                    valueList+=id+"_"+value+"_";
                });
            }

            if (valueList!="") {
                $.ajax({
                     url: "json.htm?type=command&param=applyzwavenodeconfig" +
                        "&idx=" + idx +
                        "&valuelist=" + valueList,
                     async: false,
                     dataType: 'json',
                     success: function(data) {
                        bootbox.alert($.t('Configuration send to node. If the node is asleep, this could take a while!'));
                     },
                     error: function(){
                            ShowNotify($.t('Problem updating Node Configuration!'), 2500, true);
                     }
                });
            }
        }

        ZWaveSoftResetNode = function()
        {
            $.ajax({
                 url: "json.htm?type=command&param=zwavesoftreset&idx=" + $.devIdx,
                 async: false,
                 dataType: 'json',
                 success: function(data) {
                    bootbox.alert($.t('Soft resetting controller device...!'));
                 }
            });
        }
        ZWaveHardResetNode = function()
        {
            bootbox.confirm($.t("Are you sure you want to hard reset the controller?\n(All associated nodes will be removed)"), function(result) {
                if (result==true) {
                    $.ajax({
                         url: "json.htm?type=command&param=zwavehardreset&idx=" + $.devIdx,
                         async: false,
                         dataType: 'json',
                         success: function(data) {
                            bootbox.alert($.t('Hard resetting controller device...!'));
                         }
                    });
                }
            });


        }

        ZWaveHealNetwork = function()
        {
            $.ajax({
                 url: "json.htm?type=command&param=zwavenetworkheal&idx=" + $.devIdx,
                 async: false,
                 dataType: 'json',
                 success: function(data) {
                    bootbox.alert($.t('Initiating network heal...!'));
                 }
            });
        }

        ZWaveHealNode = function(node)
        {
            $.ajax({
                 url: "json.htm?type=command&param=zwavenodeheal&idx=" + $.devIdx + "&node=" + node,
                 async: false,
                 dataType: 'json',
                 success: function(data) {
                    bootbox.alert($.t('Initiating node heal...!'));
                 }
            });
        }


        ZWaveReceiveConfiguration = function()
        {
            $.ajax({
                 url: "json.htm?type=command&param=zwavereceiveconfigurationfromothercontroller&idx=" + $.devIdx,
                 async: false,
                 dataType: 'json',
                 success: function(data) {
                    bootbox.alert($.t('Initiating Receive Configuration...!'));
                 }
            });
        }

        ZWaveSendConfiguration = function()
        {
            $.ajax({
                 url: "json.htm?type=command&param=zwavesendconfigurationtosecondcontroller&idx=" + $.devIdx,
                 async: false,
                 dataType: 'json',
                 success: function(data) {
                    bootbox.alert($.t('Initiating Send Configuration...!'));
                 }
            });
        }

        ZWaveTransferPrimaryRole = function()
        {
            bootbox.confirm($.t("Are you sure you want to transfer the primary role?"), function(result) {
                if (result==true) {
                    $.ajax({
                         url: "json.htm?type=command&param=zwavetransferprimaryrole&idx=" + $.devIdx,
                         async: false,
                         dataType: 'json',
                         success: function(data) {
                            bootbox.alert($.t('Initiating Transfer Primary Role...!'));
                         }
                    });
                }
            });
        }

        ZWaveTopology = function()
        {
            $.ajax({
                 url: "json.htm?type=command&param=zwavestatecheck&idx=" + $.devIdx,
                 async: false,
                 dataType: 'json',
                 success: function(data) {
                    if (data.status == 'OK') {
                        noty({
                            text: '<center>' + $.t('ZWave Network Information') + '</center><p><p><iframe src="zwavetopology.html?hwid='+$.devIdx+'" name="topoframe" frameBorder="0" height="'+window.innerHeight*0.7+'" width="100%"/>',
                            type: 'alert',
                            modal: true,
                            buttons: [
                                {addClass: 'btn btn-primary', text: $.t("Close"), onClick: function($noty)
                                    {$noty.close();

                                    }
                                }]
                        });
                    }
                    else {
                        ShowNotify($.t('Error communicating with zwave controller!'), 2500, true);
                    }
                 }
            });
        }

        RefreshOpenZWaveNodeTable = function()
        {
          $('#modal').show();

            $('#updelclr #nodeupdate').attr("class", "btnstyle3-dis");
            $('#updelclr #nodedelete').attr("class", "btnstyle3-dis");
            $("#hardwarecontent #configuration").html("");
            $("#hardwarecontent #nodeparamstable #nodename").val("");

          var oTable = $('#nodestable').dataTable();
          oTable.fnClearTable();

          $.ajax({
             url: "json.htm?type=openzwavenodes&idx="+$.devIdx,
             async: false,
             dataType: 'json',
             success: function(data) {
              if (typeof data.result != 'undefined') {

                if (data.NodesQueried == true) {
                    $("#zwavenodesqueried").hide();
                }
                else {
                    $("#zwavenodesqueried").show();
                }

                $("#ownNodeId").val(String(data.ownNodeId));

                $.each(data.result, function(i,item){
                    var status="ok";
                    if (item.State == "Dead") {
                        status="failed";
                    }
                    else if ((item.State == "Sleep")||(item.State == "Sleeping")) {
                        status="sleep";
                    }
                    else if (item.State == "Unknown") {
                        status="unknown";
                    }
                    var statusImg='<img src="images/' + status + '.png" />';
                    var healButton='<img src="images/heal.png" onclick="ZWaveHealNode('+item.NodeID+')" class="lcursor" title="'+$.t("Heal node")+'" />';

                    var nodeStr = addLeadingZeros(item.NodeID,3) + " (0x" + addLeadingZeros(item.NodeID.toString(16),2) + ")";
                    var addId = oTable.fnAddData( {
                        "DT_RowId": item.idx,
                        "Name": item.Name,
                        "PollEnabled": item.PollEnabled,
                        "Config": item.config,
                        "State": item.State,
                        "NodeID": item.NodeID,
                        "HaveUserCodes": item.HaveUserCodes,
                        "0": nodeStr,
                        "1": item.Name,
                        "2": item.Description,
                        "3": item.Manufacturer_name,
                        "4": item.Product_id,
                        "5": item.Product_type,
                        "6": item.LastUpdate,
                        "7": $.t((item.PollEnabled == "true")?"Yes":"No"),
                        "8": statusImg+'&nbsp;&nbsp;'+healButton,
                    } );
                });
              }
             }
          });

            /* Add a click handler to the rows - this could be used as a callback */
            $("#nodestable tbody").off();
            $("#nodestable tbody").on( 'click', 'tr', function () {
                $('#updelclr #nodedelete').attr("class", "btnstyle3-dis");
                if ( $(this).hasClass('row_selected') ) {
                    $(this).removeClass('row_selected');
                    $('#updelclr #nodeupdate').attr("class", "btnstyle3-dis");
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
                    var anSelected = fnGetSelected( oTable );
                    if ( anSelected.length !== 0 ) {
                        var data = oTable.fnGetData( anSelected[0] );
                        var idx= data["DT_RowId"];
                        var iNode=parseInt(data["NodeID"]);
                        $("#updelclr #nodeupdate").attr("href", "javascript:UpdateNode(" + idx + ")");
                        $("#hardwarecontent #zwavecodemanagement").attr("href", "javascript:ZWaveUserCodeManagement(" + idx + ")");
                        if (iNode != iOwnNodeId) {
                            $('#updelclr #nodedelete').attr("class", "btnstyle3");
                            $("#updelclr #nodedelete").attr("href", "javascript:DeleteNode(" + idx + ")");
                        }
                        $("#hardwarecontent #nodeparamstable #nodename").val(data["Name"]);
                        $('#hardwarecontent #nodeparamstable #EnablePolling').prop('checked',(data["PollEnabled"]=="true"));
                        if (iNode==iOwnNodeId) {
                            $("#hardwarecontent #nodeparamstable #trEnablePolling").hide();
                        }
                        else {
                            $("#hardwarecontent #nodeparamstable #trEnablePolling").show();
                        }

                        if (data["HaveUserCodes"] == true) {
                            $('#hardwarecontent #usercodegrp').show();
                        }
                        else {
                            $('#hardwarecontent #usercodegrp').hide();
                        }

                        var szConfig="";
                        if (typeof data["Config"] != 'undefined') {
                            //Create configuration setup
                            szConfig='<a class="btnstyle3" id="noderequeststoredvalues" data-i18n="RequestConfiguration" onclick="RequestZWaveConfiguration(' + idx + ');">Request current stored values from device</a><br /><br />';
                            var bHaveConfiguration=false;
                            $.each(data["Config"], function(i,item){
                                bHaveConfiguration=true;
                                if (item.type=="list") {
                                    szConfig+="<b>"+item.index+". "+item.label+":</b><br>";
                                    szConfig+='<select style="width:100%" class="combobox ui-corner-all" id="'+item.index+'">';
                                    var iListItem=0;
                                    var totListItems=parseInt(item.list_items);
                                    for (iListItem=0; iListItem<totListItems; iListItem++)
                                    {
                                        var szComboOption='<option value="' + iListItem + '"';
                                        var szOptionTxt=item.listitem[iListItem];
                                        if (szOptionTxt==item.value) {
                                            szComboOption+=' selected="selected"';
                                        }
                                        szComboOption+='>' + szOptionTxt +'</option>';
                                        szConfig+=szComboOption;
                                    }
                                    szConfig+='</select>'
                                    if (item.units!="") {
                                        szConfig+=' (' + item.units + ')';
                                    }
                                }
                                else if (item.type=="bool") {
                                    szConfig+="<b>"+item.index+". "+item.label+":</b><br>";
                                    szConfig+='<select style="width:100%" class="combobox ui-corner-all" id="'+item.index+'">';

                                    var szComboOption='<option value="False"';
                                    if (item.value=="False") {
                                        szComboOption+=' selected="selected"';
                                    }
                                    szComboOption+='>False</option>';
                                    szConfig+=szComboOption;

                                    szComboOption='<option value="True"';
                                    if (item.value=="True") {
                                        szComboOption+=' selected="selected"';
                                    }
                                    szComboOption+='>True</option>';
                                    szConfig+=szComboOption;

                                    szConfig+='</select>'
                                    if (item.units!="") {
                                        szConfig+=' (' + item.units + ')';
                                    }
                                }
                                else if (item.type=="string") {
                                    szConfig+="<b>"+item.index+". "+item.label+":</b><br>";
                                    szConfig+='<input type="text" id="'+item.index+'" value="' + item.value + '" style="width: 600px; padding: .2em;" class="text ui-widget-content ui-corner-all" /><br>';

                                    if (item.units!="") {
                                        szConfig+=' (' + item.units + ')';
                                    }
                                    szConfig+=" (" + $.t("actual") + ": " + item.value + ")";
                                }
                                else {
                                    szConfig+="<b>"+item.index+". "+item.label+":</b> ";
                                    szConfig+='<input type="text" id="'+item.index+'" value="' + item.value + '" style="width: 50px; padding: .2em;" class="text ui-widget-content ui-corner-all" />';
                                    if (item.units!="") {
                                        szConfig+=' (' + item.units + ')';
                                    }
                                    szConfig+=" (" + $.t("actual") + ": " + item.value + ")";
                                }
                                szConfig+="<br /><br />";
                                if (item.help!="") {
                                    szConfig+=item.help+"<br>";
                                }
                                szConfig+="Last Update: " + item.LastUpdate;
                                szConfig+="<br /><br />";
                            });
                            if (bHaveConfiguration==true) {
                                szConfig+='<a class="btnstyle3" id="nodeapplyconfiguration" data-i18n="ApplyConfiguration" onclick="ApplyZWaveConfiguration(' + idx + ');" >Apply configuration for this device</a><br />';
                            }
                        }
                        $("#hardwarecontent #configuration").html(szConfig);
                        $("#hardwarecontent #configuration").i18n();
                    }
                }
            });

          $('#modal').hide();
        }

        ZWaveStartUserCodeEnrollment = function()
        {
            $.ajax({
                 url: "json.htm?type=command&param=zwavestartusercodeenrollmentmode&idx=" + $.devIdx,
                 async: false,
                 dataType: 'json',
                 success: function(data) {
                    bootbox.alert($.t('User Code Enrollment started. You have 30 seconds to include the new key...!'));
                 }
            });
        }

        RemoveUserCode = function(index)
        {
            bootbox.confirm($.t("Are you sure to delete this User Code?"), function(result) {
                if (result==true) {
                    $.ajax({
                         url: "json.htm?type=command&param=zwaveremoveusercode&idx=" + $.nodeIdx +"&codeindex=" + index,
                         async: false,
                         dataType: 'json',
                         success: function(data) {
                            RefreshHardwareTable();
                         },
                         error: function(){
                                HideNotify();
                                ShowNotify($.t('Problem deleting User Code!'), 2500, true);
                         }
                    });
                }
            });
        }

        RefreshOpenZWaveUserCodesTable = function()
        {
          $('#modal').show();

          var oTable = $('#codestable').dataTable();
          oTable.fnClearTable();
          $.ajax({
             url: "json.htm?type=command&param=zwavegetusercodes&idx="+$.nodeIdx,
             dataType: 'json',
             async: false,
             success: function(data) {
                if (typeof data.result != 'undefined') {
                    $.each(data.result, function(i,item){
                        var removeButton='<span class="label label-info lcursor" onclick="RemoveUserCode(' + item.index + ');">Remove</span>';
                        var addId = oTable.fnAddData( {
                            "DT_RowId": item.index,
                            "Code": item.index,
                            "Value": item.code,
                            "0": item.index,
                            "1": item.code,
                            "2": removeButton
                        } );
                    });
                }
             }
          });
            /* Add a click handler to the rows - this could be used as a callback */
            $("#codestable tbody").off();
            $("#codestable tbody").on( 'click', 'tr', function () {
                if ( $(this).hasClass('row_selected') ) {
                    $(this).removeClass('row_selected');
                }
                else {
                    var oTable = $('#codestable').dataTable();
                    oTable.$('tr.row_selected').removeClass('row_selected');
                    $(this).addClass('row_selected');
                    var anSelected = fnGetSelected( oTable );
                    if ( anSelected.length !== 0 ) {
                        var data = oTable.fnGetData( anSelected[0] );
                        //var idx= data["DT_RowId"];
                    }
                }
            });
          $('#modal').hide();
        }

        ZWaveUserCodeManagement = function(idx)
        {
            $.nodeIdx=idx;
            cursordefault();
            var htmlcontent = '';
            htmlcontent+=$('#openzwaveusercodes').html();
            var bString="EditOpenZWave("+$.devIdx+",'"+$.devName + "',0,0,0,0,0)";
            $('#hardwarecontent').html(GetBackbuttonHTMLTable(bString)+htmlcontent);
            $('#hardwarecontent').i18n();
            $('#hardwarecontent #nodeidx').val(idx);
            var oTable = $('#codestable').dataTable( {
              "sDom": '<"H"lfrC>t<"F"ip>',
              "oTableTools": {
                "sRowSelect": "single",
              },
              "aaSorting": [[ 0, "desc" ]],
              "bSortClasses": false,
              "bProcessing": true,
              "bStateSave": true,
              "bJQueryUI": true,
              "aLengthMenu": [[25, 50, 100, -1], [25, 50, 100, "All"]],
              "iDisplayLength" : 25,
              "sPaginationType": "full_numbers",
              language: $.DataTableLanguage
            } );
            RefreshOpenZWaveUserCodesTable();
        }

        EditOpenZWave = function(idx,name,Mode1,Mode2,Mode3,Mode4,Mode5,Mode6)
        {
            $.devIdx=idx;
            $.devName=name;
            cursordefault();
            var htmlcontent = '';
            htmlcontent='<p><center><h2><span data-i18n="Device"></span>: ' + name + '</h2></center></p>\n';
            htmlcontent+=$('#openzwave').html();
            $('#hardwarecontent').html(GetBackbuttonHTMLTable('ShowHardware')+htmlcontent);
            $('#hardwarecontent #zwave_configdownload').attr("href", "zwavegetconfig.php?idx="+idx);
            $('#hardwarecontent').i18n();

            $('#hardwarecontent #usercodegrp').hide();

            var oTable = $('#nodestable').dataTable( {
              "sDom": '<"H"lfrC>t<"F"ip>',
              "oTableTools": {
                "sRowSelect": "single",
              },
              "aaSorting": [[ 0, "desc" ]],
              "bSortClasses": false,
              "bProcessing": true,
              "bStateSave": true,
              "bJQueryUI": true,
              "aLengthMenu": [[25, 50, 100, -1], [25, 50, 100, "All"]],
              "iDisplayLength" : 25,
              "sPaginationType": "full_numbers",
              language: $.DataTableLanguage
            } );

            $('#hardwarecontent #idx').val(idx);

            RefreshOpenZWaveNodeTable();
        }

        SetOpenThermSettings = function()
        {
          $.post("setopenthermsettings.webem", $("#hardwarecontent #openthermform").serialize(), function(data) {
           ShowHardware();
          });
        }

		SendOTGWCommand = function()
		{
			var idx=$('#hardwarecontent #idx').val();
			var cmnd = $('#hardwarecontent #otgwcmnd').val();
            $.ajax({
                url: "json.htm?type=command&param=sendopenthermcommand" +
                    "&idx=" + idx +
                    "&cmnd=" + cmnd,
                 async: false,
                 dataType: 'json',
                 success: function(data) {
                 },
                 error: function(){
                 }
            });
		}

        EditOpenTherm = function(idx,name,Mode1,Mode2,Mode3,Mode4,Mode5,Mode6)
        {
        //Mode1=Outside Temperature Sensor DeviceIdx, 0=Not Using
            $.devIdx=idx;
            cursordefault();
            var htmlcontent = '';
            htmlcontent='<p><center><h2><span data-i18n="Device"></span>: ' + name + '</h2></center></p>\n';
            htmlcontent+=$('#opentherm').html();
            $('#hardwarecontent').html(GetBackbuttonHTMLTable('ShowHardware')+htmlcontent);
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
                 success: function(data) {
                    if (typeof data.result != 'undefined')
                    {
                        $.each(data.result, function(i,item){
                            if (typeof item.Temp != 'undefined') {
                                var option = $('<option />');
                                option.attr('value', item.idx).text(item.Name+" ("+item.Temp +"\u00B0)");
                                $("#hardwarecontent #combooutsidesensor").append(option);
                            }
                        });
                    }
                    $("#hardwarecontent #combooutsidesensor").val(Mode1);
                 }
            });
            $("#hardwarecontent #combooutsidesensor").val(Mode1);
        }

        SetRFXCOMMode = function()
        {
          $.post("setrfxcommode.webem", $("#hardwarecontent #settings").serialize(), function(data) {
           SwitchLayout('Dashboard');
          });
        }

        SetRego6XXType = function()
        {
          $.post("setrego6xxtype.webem", $("#hardwarecontent #regotype").serialize(), function(data) {
           ShowHardware();
          });
        }

        SetS0MeterType = function()
        {
          $.post("sets0metertype.webem", $("#hardwarecontent #s0metertype").serialize(), function(data) {
           ShowHardware();
          });
        }

        SetLimitlessType = function()
        {
          $.post("setlimitlesstype.webem", $("#hardwarecontent #limitform").serialize(), function(data) {
           ShowHardware();
          });
        }

        BindEvohome= function(idx,name,devtype)
        {
            $.devIdx=idx;
            if(devtype=="Relay")
                ShowNotify($.t('Hold bind button on relay...'),2500);
            else if (devtype == "AllSensors")
                ShowNotify($.t('Creating additional sensors...'));
            else if (devtype == "ZoneSensor")
                ShowNotify($.t('Binding Domoticz zone temperature sensor to controller...'),2500);
            else
                ShowNotify($.t('Binding Domoticz outdoor temperature device to evohome controller...'),2500);

            setTimeout(function() {
                var bNewDevice = false;
                var bIsUsed = false;
                var Name = "";

                $.ajax({
                   url: "json.htm?type=bindevohome&idx=" + $.devIdx + "&devtype=" + devtype,
                   async: false,
                   dataType: 'json',
                   success: function(data) {
                    if (typeof data.status != 'undefined') {
                      bIsUsed=data.Used;
                      if (data.status == 'OK')
                        bNewDevice=true;
                      else
                        Name=data.Name;
                    }
                   }
                });
                HideNotify();

                setTimeout(function() {
                    if ((bNewDevice == true) && (bIsUsed == false))
                    {
                        if(devtype=="Relay")
                            ShowNotify($.t('Relay bound, and can be found in the devices tab!'), 2500);
                        else if (devtype == "AllSensors")
                            ShowNotify($.t('New sensors will appear in the devices tab after 10min'), 2500);
                        else if (devtype == "ZoneSensor")
                            ShowNotify($.t('Sensor bound, and can be found in the devices tab!'),2500);
                        else
                            ShowNotify($.t('Domoticz outdoor temperature device has been bound to evohome controller'),2500);
                    }
                    else {
                        if (bIsUsed == true)
                            ShowNotify($.t('Already used by') + ':<br>"' + Name +'"', 3500, true);
                        else
                            ShowNotify($.t('Timeout...<br>Please try again!'), 2500, true);
                    }
                }, 200);
            }, 600);
        }

        CreateEvohomeSensors= function(idx,name)
        {
            $.devIdx=idx;
            $( "#dialog-createevohomesensor" ).dialog({
                  autoOpen: false,
                  width: 380,
                  height: 160,
                  modal: true,
                  resizable: false,
                  buttons: {
                      "OK": function() {
                          var bValid = true;
                          $( this ).dialog( "close" );
                            var SensorType=$("#dialog-createevohomesensor #sensortype option:selected").val();
                            if (typeof SensorType == 'undefined') {
                                bootbox.alert($.t('No Sensor Type Selected!'));
                                return ;
                            }
                            $.ajax({
                                 url: "json.htm?type=createevohomesensor&idx=" + $.devIdx +
									"&sensortype=" + SensorType,
                                 async: false,
                                 dataType: 'json',
                                 success: function(data) {
                                    if (data.status == 'OK') {
                                        ShowNotify($.t('Sensor Created, and can be found in the devices tab!'), 2500);
                                    }
                                    else {
                                        ShowNotify($.t('Problem creating Sensor!'), 2500, true);
                                    }
                                 },
                                 error: function(){
                                        HideNotify();
                                        ShowNotify($.t('Problem creating Sensor!'), 2500, true);
                                 }
                            });
                      },
                      Cancel: function() {
                          $( this ).dialog( "close" );
                      }
                  },
                  close: function() {
                    $( this ).dialog( "close" );
                  }
            });

            $( "#dialog-createevohomesensor" ).i18n();
            $( "#dialog-createevohomesensor" ).dialog( "open" );
        }

        CreateRFLinkDevices= function(idx,name)
        {
            $.devIdx=idx;
            $("#dialog-createrflinkdevice #vsensoraxis").hide();
            $("#dialog-createrflinkdevice #sensoraxis").val("");
            $( "#dialog-createrflinkdevice" ).dialog({
                  autoOpen: false,
                  width: 420,
                  height: 250,
                  modal: true,
                  resizable: false,
                  buttons: {
                      "OK": function() {
                          var bValid = true;
                          $( this ).dialog( "close" );
                            var SensorName=$("#dialog-createrflinkdevice #sensorname").val();
							if (SensorName=="")
							{
								ShowNotify($.t('Please enter a command!'), 2500, true);
								return;
							}
                            $.ajax({
                                 url: "json.htm?type=createrflinkdevice&idx=" + $.devIdx +
									"&command=" + SensorName,
                                 async: false,
                                 dataType: 'json',
                                 success: function(data) {
                                    if (data.status == 'OK') {
                                        ShowNotify($.t('Device created, it can be found in the devices tab!'), 2500);
                                    }
                                    else {
                                        ShowNotify($.t('Problem creating Sensor!'), 2500, true);
                                    }
                                 },
                                 error: function(){
                                        HideNotify();
                                        ShowNotify($.t('Problem creating Sensor!'), 2500, true);
                                 }
                            });
                      },
                      Cancel: function() {
                          $( this ).dialog( "close" );
                      }
                  },
                  close: function() {
                    $( this ).dialog( "close" );
                  }
            });

            $( "#dialog-createrflinkdevice" ).i18n();
            $( "#dialog-createrflinkdevice" ).dialog( "open" );
        }

		function OnDummySensorTypeChange()
		{
			var stype=$("#dialog-createsensor #sensortype option:selected").val();
			$("#dialog-createsensor #sensoraxis").val("");
			if (stype == 1004) {
				$("#dialog-createsensor #vsensoraxis").show();
			}
			else {
				$("#dialog-createsensor #vsensoraxis").hide();
			}
		}

        CreateDummySensors = function(idx,name)
        {
			$.devIdx=idx;

            $("#dialog-createsensor #vsensoraxis").hide();
            $("#dialog-createsensor #sensoraxis").val("");

			$("#dialog-createsensor #sensortype").change(function() {
				OnDummySensorTypeChange();
			});

            $( "#dialog-createsensor" ).dialog({
                  autoOpen: false,
                  width: 420,
                  height: 250,
                  modal: true,
                  resizable: false,
                  buttons: {
                      "OK": function() {
                          var bValid = true;
                          $( this ).dialog( "close" );
                            var SensorName=$("#dialog-createsensor #sensorname").val();
							if (SensorName=="")
							{
								ShowNotify($.t('Please enter a Name!'), 2500, true);
								return;
							}
                            var SensorType=$("#dialog-createsensor #sensortype option:selected").val();
                            if (typeof SensorType == 'undefined') {
                                bootbox.alert($.t('No Sensor Type Selected!'));
                                return ;
                            }
                            var extraSendData="";
                            if (SensorType==1004) {
								var AxisLabel=$("#dialog-createsensor #sensoraxis").val();
								if (AxisLabel=="")
								{
									ShowNotify($.t('Please enter a Axis Label!'), 2500, true);
									return;
								}
								extraSendData="&sensoroptions=1;" + encodeURIComponent(AxisLabel);
                            }
                            $.ajax({
                                 url: "json.htm?type=createvirtualsensor&idx=" + $.devIdx +
									"&sensorname=" + encodeURIComponent(SensorName) +
									"&sensortype=" + SensorType +
									extraSendData,
                                 async: false,
                                 dataType: 'json',
                                 success: function(data) {
                                    if (data.status == 'OK') {
                                        ShowNotify($.t('Sensor Created, and can be found in the devices tab!'), 2500);
                                    }
                                    else {
                                        ShowNotify($.t('Problem creating Sensor!'), 2500, true);
                                    }
                                 },
                                 error: function(){
                                        HideNotify();
                                        ShowNotify($.t('Problem creating Sensor!'), 2500, true);
                                 }
                            });
                      },
                      Cancel: function() {
                          $( this ).dialog( "close" );
                      }
                  },
                  close: function() {
                    $( this ).dialog( "close" );
                  }
            });

            $( "#dialog-createsensor" ).i18n();
            $( "#dialog-createsensor" ).dialog( "open" );
            OnDummySensorTypeChange();
        }

        AddYeeLight = function(idx,name)
        {
			$.devIdx=idx;

            $( "#dialog-addyeelight" ).dialog({
                  autoOpen: false,
                  width: 420,
                  height: 250,
                  modal: true,
                  resizable: false,
                  buttons: {
                      "OK": function() {
                          var bValid = true;
                            var SensorName=$("#dialog-addyeelight #name").val();
							if (SensorName=="")
							{
								ShowNotify($.t('Please enter a Name!'), 2500, true);
								return;
							}
                            var IPAddress=$("#dialog-addyeelight #ipaddress").val();
							if (IPAddress=="")
							{
								ShowNotify($.t('Please enter a IP Address!'), 2500, true);
								return;
							}
                            var SensorType=$("#dialog-addyeelight #lighttype option:selected").val();
                            if (typeof SensorType == 'undefined') {
                                bootbox.alert($.t('No Light Type Selected!'));
                                return ;
                            }
							$( this ).dialog( "close" );
                            $.ajax({
                                 url: "json.htm?type=command&param=addyeelight&idx=" + $.devIdx +
									"&name=" + encodeURIComponent(SensorName) +
									"&ipaddress=" + encodeURIComponent(IPAddress) +
									"&stype=" + SensorType,
                                 async: false,
                                 dataType: 'json',
                                 success: function(data) {
                                    if (data.status == 'OK') {
                                        ShowNotify($.t('Light created, and can be found in the devices tab!'), 2500);
                                    }
                                    else {
                                        ShowNotify($.t('Problem adding Light!'), 2500, true);
                                    }
                                 },
                                 error: function(){
                                        HideNotify();
                                        ShowNotify($.t('Problem adding Light!'), 2500, true);
                                 }
                            });
                      },
                      Cancel: function() {
                          $( this ).dialog( "close" );
                      }
                  },
                  close: function() {
                    $( this ).dialog( "close" );
                  }
            });

            $( "#dialog-addyeelight" ).i18n();
            $( "#dialog-addyeelight" ).dialog( "open" );
        }

		ReloadPiFace = function(idx,name)
        {
          $.post("reloadpiface.webem", { 'idx':idx }, function(data) {
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

        RefreshHardwareTable = function()
        {
          $('#modal').show();

            $('#updelclr #hardwareupdate').attr("class", "btnstyle3-dis");
            $('#updelclr #hardwaredelete').attr("class", "btnstyle3-dis");

          var oTable = $('#hardwaretable').dataTable();
          oTable.fnClearTable();

          $.ajax({
             url: "json.htm?type=hardware",
             async: false,
             dataType: 'json',
             success: function(data) {

              if (typeof data.result != 'undefined') {
                $.each(data.result, function(i,item){

                    var HwTypeStrOrg=$.myglobals.HardwareTypesStr[item.Type];
                    var HwTypeStr=HwTypeStrOrg;

                    if (typeof HwTypeStr == 'undefined') {
                        HwTypeStr="???? Unknown (NA/Not supported)";
                    }

                    var SerialName="Unknown!?";
                    var intport=0;
                    if ((HwTypeStr.indexOf("LAN") >= 0)||(HwTypeStr.indexOf("Domoticz") >= 0) ||(HwTypeStr.indexOf("Harmony") >= 0)||(HwTypeStr.indexOf("Philips Hue") >= 0))
                    {
                        SerialName=item.Port;
                    }
                    else if ((item.Type == 7)||(item.Type == 11))
                    {
                        SerialName="USB";
                    }
                    else if ((item.Type == 13)||(item.Type == 71)||(item.Type == 85))
                    {
                        SerialName="I2C";
                    }
                    else if ((item.Type == 14)||(item.Type == 25)||(item.Type == 28)||(item.Type == 30)||(item.Type == 34))
                    {
                        SerialName="WWW";
                    }
                    else if ((item.Type == 15)||(item.Type == 23)||(item.Type == 26)||(item.Type == 27)||(item.Type == 51)||(item.Type == 54))
                    {
                        SerialName="";
                    }
                    else if ((item.Type == 16) || (item.Type == 32))
                    {
                        SerialName="GPIO";
                    }
                    else if(HwTypeStr.indexOf("Evohome") >= 0 && HwTypeStr.indexOf("script") >= 0)
                    {
                        SerialName="Script";
                    }
                    else if (item.Type == 74)
                    {
                        intport = item.Port;
                        SerialName = "";
                    }
                    else
                    {
                        SerialName=item.SerialPort;
                        intport=jQuery.inArray(item.SerialPort, $scope.SerialPortStr);
                    }

                    var enabledstr=$.t("No");
                    if (item.Enabled=="true") {
                        enabledstr=$.t("Yes");
                    }
                    if (HwTypeStr.indexOf("RFXCOM") >= 0)
                    {
                        HwTypeStr+='<br>Version: ' + item.version;
                        if (HwTypeStr.indexOf("868") >= 0) {
							HwTypeStr+=' <span class="label label-info lcursor" onclick="EditRFXCOMMode868(' + item.idx + ',\'' + item.Name + '\',' + item.Mode1 + ',' + item.Mode2+ ',' + item.Mode3+ ',' + item.Mode4+ ',' + item.Mode5 + ',' + item.Mode6 + ');">' + $.t("Set Mode") + '</span>';
                        }
                        else {
							HwTypeStr+=' <span class="label label-info lcursor" onclick="EditRFXCOMMode(' + item.idx + ',\'' + item.Name + '\',' + item.Mode1 + ',' + item.Mode2+ ',' + item.Mode3+ ',' + item.Mode4+ ',' + item.Mode5 + ',' + item.Mode6 + ');">' + $.t("Set Mode") + '</span>';
						}
                    }
                    else if (HwTypeStr.indexOf("S0 Meter") >= 0) {
                        HwTypeStr+=' <span class="label label-info lcursor" onclick="EditS0MeterType(' + item.idx + ',\'' + item.Name + '\',\'' + item.Extra + '\');">' + $.t("Set Mode") + '</span>';
                    }
                    else if (HwTypeStr.indexOf("Limitless") >= 0) {
                        HwTypeStr+=' <span class="label label-info lcursor" onclick="EditLimitlessType(' + item.idx + ',\'' + item.Name + '\',' + item.Mode1 + ',' + item.Mode2+ ',' + item.Mode3+ ',' + item.Mode4+ ',' + item.Mode5 + ',' + item.Mode6 + ');">' + $.t("Set Mode") + '</span>';
                    }
                    else if (HwTypeStr.indexOf("OpenZWave") >= 0) {
                        HwTypeStr+='<br>Version: ' + item.version;

                        if (typeof item.NodesQueried != 'undefined') {
                            var lblStatus="label-info";
                            if (item.NodesQueried != true) {
                                lblStatus="label-important";
                            }
                            HwTypeStr+=' <span class="label ' + lblStatus + ' lcursor" onclick="EditOpenZWave(' + item.idx + ',\'' + item.Name + '\',' + item.Mode1 + ',' + item.Mode2+ ',' + item.Mode3+ ',' + item.Mode4+ ',' + item.Mode5 + ',' + item.Mode6 + ');">' + $.t("Setup") + '</span>';
                        }
                    }
                    else if (HwTypeStr.indexOf("SBFSpot") >= 0) {
                        HwTypeStr+=' <span class="label label-info lcursor" onclick="EditSBFSpot(' + item.idx + ',\'' + item.Name + '\',' + item.Mode1 + ',' + item.Mode2+ ',' + item.Mode3+ ',' + item.Mode4+ ',' + item.Mode5 + ',' + item.Mode6 + ');">' + $.t("Setup") + '</span>';
                    }
                    else if (HwTypeStr.indexOf("MySensors") >= 0) {
                        HwTypeStr+='<br>Version: ' + item.version;
                        HwTypeStr+=' <span class="label label-info lcursor" onclick="EditMySensors(' + item.idx + ',\'' + item.Name + '\',' + item.Mode1 + ',' + item.Mode2+ ',' + item.Mode3+ ',' + item.Mode4+ ',' + item.Mode5 + ',' + item.Mode6 + ');">' + $.t("Setup") + '</span>';
                    }
                    else if ((HwTypeStr.indexOf("OpenTherm") >= 0)||(HwTypeStr.indexOf("Thermosmart") >= 0)) {
						HwTypeStr+='<br>Version: ' + item.version;
                        HwTypeStr+=' <span class="label label-info lcursor" onclick="EditOpenTherm(' + item.idx + ',\'' + item.Name + '\',' + item.Mode1 + ',' + item.Mode2+ ',' + item.Mode3+ ',' + item.Mode4+ ',' + item.Mode5 + ',' + item.Mode6 + ');">' + $.t("Setup") + '</span>';
                    }
                    else if (HwTypeStr.indexOf("Wake-on-LAN") >= 0) {
                        HwTypeStr+=' <span class="label label-info lcursor" onclick="EditWOL(' + item.idx + ',\'' + item.Name + '\',' + item.Mode1 + ',' + item.Mode2+ ',' + item.Mode3+ ',' + item.Mode4+ ',' + item.Mode5 + ',' + item.Mode6 + ');">' + $.t("Setup") + '</span>';
                    }
                    else if (HwTypeStr.indexOf("System Alive") >= 0) {
                        HwTypeStr+=' <span class="label label-info lcursor" onclick="EditPinger(' + item.idx + ',\'' + item.Name + '\',' + item.Mode1 + ',' + item.Mode2+ ',' + item.Mode3+ ',' + item.Mode4+ ',' + item.Mode5 + ',' + item.Mode6 + ');">' + $.t("Setup") + '</span>';
                    }
                    else if (HwTypeStr.indexOf("Kodi") >= 0) {
                        HwTypeStr += ' <span class="label label-info lcursor" onclick="EditKodi(' + item.idx + ',\'' + item.Name + '\',' + item.Mode1 + ',' + item.Mode2 + ',' + item.Mode3 + ',' + item.Mode4 + ',' + item.Mode5 + ',' + item.Mode6 + ');">' + $.t("Setup") + '</span>';
                    }
                    else if (HwTypeStr.indexOf("Panasonic") >= 0) {
                        HwTypeStr += ' <span class="label label-info lcursor" onclick="EditPanasonic(' + item.idx + ',\'' + item.Name + '\',' + item.Mode1 + ',' + item.Mode2 + ',' + item.Mode3 + ',' + item.Mode4 + ',' + item.Mode5 + ',' + item.Mode6 + ');">' + $.t("Setup") + '</span>';
                    }
                    else if (HwTypeStr.indexOf("BleBox") >= 0) {
                        HwTypeStr += ' <span class="label label-info lcursor" onclick="EditBleBox(' + item.idx + ',\'' + item.Name + '\',' + item.Mode1 + ',' + item.Mode2 + ',' + item.Mode3 + ',' + item.Mode4 + ',' + item.Mode5 + ',' + item.Mode6 + ');">' + $.t("Setup") + '</span>';
                    }
                    else if (HwTypeStr.indexOf("Logitech Media Server") >= 0) {
                        HwTypeStr += ' <span class="label label-info lcursor" onclick="EditLMS(' + item.idx + ',\'' + item.Name + '\',' + item.Mode1 + ',' + item.Mode2 + ',' + item.Mode3 + ',' + item.Mode4 + ',' + item.Mode5 + ',' + item.Mode6 + ');">' + $.t("Setup") + '</span>';
                    }
                    else if (HwTypeStr.indexOf("HEOS by DENON") >= 0) {
                        HwTypeStr += ' <span class="label label-info lcursor" onclick="EditHEOS by DENON(' + item.idx + ',\'' + item.Name + '\',' + item.Mode1 + ',' + item.Mode2 + ',' + item.Mode3 + ',' + item.Mode4 + ',' + item.Mode5 + ',' + item.Mode6 + ');">' + $.t("Setup") + '</span>';
                    }
                    else if (HwTypeStr.indexOf("Dummy") >= 0) {
                        HwTypeStr+=' <span class="label label-info lcursor" onclick="CreateDummySensors(' + item.idx + ',\'' + item.Name + '\');">' + $.t("Create Virtual Sensors") + '</span>';
                    }
                    else if (HwTypeStr.indexOf("YeeLight") >= 0) {
                        HwTypeStr+=' <span class="label label-info lcursor" onclick="AddYeeLight(' + item.idx + ',\'' + item.Name + '\');">' + $.t("Add Light") + '</span>';
                    }
                    else if (HwTypeStr.indexOf("PiFace") >= 0) {
                        HwTypeStr+=' <span class="label label-info lcursor" onclick="ReloadPiFace(' + item.idx + ',\'' + item.Name + '\');">' + $.t("Reload") + '</span>';
                    }
                    else if (HwTypeStr.indexOf("HTTP/HTTPS") >= 0) {
                        HwTypeStr+=' <span class="label label-info lcursor" onclick="CreateDummySensors(' + item.idx + ',\'' + item.Name + '\');">' + $.t("Create Virtual Sensors") + '</span>';
                    }
                    else if (HwTypeStr.indexOf("RFLink") >= 0) {
                        HwTypeStr+='<br>Version: ' + item.version;
                        HwTypeStr+=' <span class="label label-info lcursor" onclick="CreateRFLinkDevices(' + item.idx + ',\'' + item.Name + '\');">' + $.t("Create RFLink Devices") + '</span>';
                    }
                    else if (HwTypeStr.indexOf("Evohome") >= 0) {
                        if(HwTypeStr.indexOf("script") >= 0)
                            HwTypeStr+=' <span class="label label-info lcursor" onclick="CreateEvohomeSensors(' + item.idx + ',\'' + item.Name + '\');">' + $.t("Create Devices") + '</span>';
                        else
                        {
                            HwTypeStr+=' <span class="label label-info lcursor" onclick="BindEvohome(' + item.idx + ',\'' + item.Name + '\',\'Relay\');">Bind Relay</span>';
                            HwTypeStr += ' <span class="label label-info lcursor" onclick="BindEvohome(' + item.idx + ',\'' + item.Name + '\',\'OutdoorSensor\');">Outdoor Sensor</span>';
                            HwTypeStr += ' <span class="label label-info lcursor" onclick="BindEvohome(' + item.idx + ',\'' + item.Name + '\',\'AllSensors\');">All Sensors</span>';
                            HwTypeStr += ' <span class="label label-info lcursor" onclick="BindEvohome(' + item.idx + ',\'' + item.Name + '\',\'ZoneSensor\');">Bind Temp Sensor</span>';
                        }
                    }
                    else if (HwTypeStr.indexOf("Rego 6XX") >= 0)
                    {
                        HwTypeStr+='<br>Type: ';
                        if (item.Mode1=="0")
                        {
                            HwTypeStr+='600-635, 637 single language';
                        }
                        else if(item.Mode1=="1")
                        {
                            HwTypeStr+='636';
                        }
                        else if(item.Mode1=="2")
                        {
                            HwTypeStr+='637 multi language';
                        }
                        HwTypeStr+=' <span class="label label-info lcursor" onclick="EditRego6XXType(' + item.idx + ',\'' + item.Name + '\',' + item.Mode1 + ',' + item.Mode2+ ',' + item.Mode3+ ',' + item.Mode4+ ',' + item.Mode5 + ',' + item.Mode6 + ');">Change Type</span>';
                    }
                    else if (HwTypeStr.indexOf("CurrentCost Meter USB") >= 0) {
                        HwTypeStr+=' <span class="label label-info lcursor" onclick="EditCCUSB(' + item.idx + ',\'' + item.Name + '\',\'' + item.Address + '\');">' + $.t("Set Mode") + '</span>';
                    }
                    else if (HwTypeStr.indexOf("Tellstick") >= 0) {
                        HwTypeStr += ' <span class="label label-info lcursor" onclick="TellstickSettings(' + item.idx + ',\'' + item.Name + '\',' + item.Mode1 + ',' + item.Mode2 + ');">' + $.t("Settings") + '</span>';
                    }

                    var sDataTimeout="";
                    if (item.DataTimeout==0) {
                        sDataTimeout=$.t("Disabled");
                    }
                    else if (item.DataTimeout<60) {
                        sDataTimeout=item.DataTimeout + " " + $.t("Seconds");
                    }
                    else if (item.DataTimeout<3600) {
                        var minutes=item.DataTimeout/60;
                        if (minutes==1) {
                            sDataTimeout=minutes + " " + $.t("Minute");
                        }
                        else {
                            sDataTimeout=minutes + " " + $.t("Minutes");
                        }
                    }
                    else if (item.DataTimeout<=86400) {
                        var hours=item.DataTimeout/3600;
                        if (hours==1) {
                            sDataTimeout=hours + " " + $.t("Hour");
                        }
                        else {
                            sDataTimeout=hours + " " + $.t("Hours");
                        }
                    }
                    else {
                        var days=item.DataTimeout/86400;
                        if (days==1) {
                            sDataTimeout=days + " " + $.t("Day");
                        }
                        else {
                            sDataTimeout=days + " " + $.t("Days");
                        }
                    }

                    var dispAddress=item.Address;
                    var addId = oTable.fnAddData( {
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
                        "Type" : HwTypeStrOrg,
                        "IntPort": intport,
                        "Address": item.Address,
                        "Port": SerialName,
                        "DataTimeout": item.DataTimeout,
                        "0": item.idx,
                        "1": item.Name,
                        "2": enabledstr,
                        "3": HwTypeStr,
                        "4": dispAddress,
                        "5": SerialName,
                        "6": sDataTimeout
                    } );
                });
              }
             }
          });

            /* Add a click handler to the rows - this could be used as a callback */
            $("#hardwaretable tbody").off();
            $("#hardwaretable tbody").on( 'click', 'tr', function () {
                if ( $(this).hasClass('row_selected') ) {
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
                    var anSelected = fnGetSelected( oTable );
                    if ( anSelected.length !== 0 ) {
                        var data = oTable.fnGetData( anSelected[0] );
                        var idx= data["DT_RowId"];
                        $("#updelclr #hardwareupdate").attr("href", "javascript:UpdateHardware(" + idx + "," + data["Mode1"] + "," + data["Mode2"] + "," + data["Mode3"] + "," + data["Mode4"] + "," + data["Mode5"] + "," + data["Mode6"] + ")");
                        $("#updelclr #hardwaredelete").attr("href", "javascript:DeleteHardware(" + idx + ")");
                        $("#hardwarecontent #hardwareparamstable #hardwarename").val(data["Name"]);
                        $("#hardwarecontent #hardwareparamstable #combotype").val(jQuery.inArray(data["Type"], $.myglobals.HardwareTypesStr));

                        $('#hardwarecontent #hardwareparamstable #enabled').prop('checked',(data["Enabled"]=="true"));
                        $('#hardwarecontent #hardwareparamstable #combodatatimeout').val(data["DataTimeout"]);

			if (data["Type"].indexOf("Local I2C sensor") >= 0) {
                            $("#hardwarecontent #hardwareparamstable #combotype").val(1000);
                        }

			$.devExtra=data["Extra"];
			UpdateHardwareParamControls();

                        if ((data["Type"].indexOf("TE923") >= 0)||
                           (data["Type"].indexOf("Volcraft") >= 0)||
                           (data["Type"].indexOf("Dummy") >= 0)||
                           (data["Type"].indexOf("System Alive") >= 0)||
                           (data["Type"].indexOf("PiFace") >= 0)||
                           (data["Type"].indexOf("Tellstick") >= 0) ||
                           (data["Type"].indexOf("Yeelight") >= 0))
                        {
                            //nothing to be set
                        }
                        else if (data["Type"].indexOf("1-Wire") >= 0) {
                            $("#hardwarecontent #hardwareparams1wire #owfspath").val(data["Extra"]);
                            $("#hardwarecontent #hardwareparams1wire #OneWireSensorPollPeriod").val(data["Mode1"]);
                            $("#hardwarecontent #hardwareparams1wire #OneWireSwitchPollPeriod").val(data["Mode2"]);
                        }
                        else if (data["Type"].indexOf("Local I2C sensor") >= 0) {
                            $("#hardwareparamsi2clocal #comboi2clocal").val(jQuery.inArray(data["Type"], $.myglobals.HardwareI2CStr));
                        }			
                        else if (data["Type"].indexOf("USB") >= 0) {
                            $("#hardwarecontent #hardwareparamsserial #comboserialport").val(data["IntPort"]);
                            if (data["Type"].indexOf("MySensors") >= 0)
                            {
                                $("#hardwarecontent #divbaudratemysensors #combobaudrate").val(data["Mode1"]);
                            }
                            if (data["Type"].indexOf("P1 Smart Meter") >= 0)
                            {
                                $("#hardwarecontent #divbaudratep1 #combobaudratep1").val(data["Mode1"]);
                                $("#hardwarecontent #divcrcp1 #disablecrcp1").prop("checked",data["Mode2"]==0);
                                if (data["Mode1"]==0)
                                {
                                    $("#hardwarecontent #divcrcp1").hide();
                                }
                                else
                                {
                                    $("#hardwarecontent #divcrcp1").show();
                                }
                            }
                        }
                        else if (((data["Type"].indexOf("LAN") >= 0) && (data["Type"].indexOf("YouLess") == -1) && (data["Type"].indexOf("Denkovi") == -1) && (data["Type"].indexOf("Satel Integra") == -1)) ||(data["Type"].indexOf("Domoticz") >= 0) ||(data["Type"].indexOf("Harmony") >= 0)) {
                            $("#hardwarecontent #hardwareparamsremote #tcpaddress").val(data["Address"]);
                            $("#hardwarecontent #hardwareparamsremote #tcpport").val(data["Port"]);
                            if (data["Type"].indexOf("P1 Smart Meter") >= 0)
                            {
                                $("#hardwarecontent #divcrcp1 #disablecrcp1").prop("checked",data["Mode2"]==0);
                            }
                        }
                        else if (((data["Type"].indexOf("LAN") >= 0) && (data["Type"].indexOf("YouLess") >= 0)) || (data["Type"].indexOf("Domoticz") >= 0) || (data["Type"].indexOf("Denkovi") >= 0) ||(data["Type"].indexOf("Harmony") >= 0) ||(data["Type"].indexOf("Satel Integra") >= 0) ||(data["Type"].indexOf("Logitech Media Server") >= 0) ||(data["Type"].indexOf("HEOS by DENON") >= 0)) {
                            $("#hardwarecontent #hardwareparamsremote #tcpaddress").val(data["Address"]);
                            $("#hardwarecontent #hardwareparamsremote #tcpport").val(data["Port"]);
                            $("#hardwarecontent #hardwareparamslogin #password").val(data["Password"]);
                            if (data["Type"].indexOf("Satel Integra") >= 0)
                            {
                                $("#hardwarecontent #hardwareparamspollinterval #pollinterval").val(data["Mode1"]);
                            }
						}
                        else if ((data["Type"].indexOf("Underground") >= 0) || (data["Type"].indexOf("DarkSky") >= 0) || (data["Type"].indexOf("AccuWeather") >= 0) || (data["Type"].indexOf("Open Weather Map") >= 0)) {
                            $("#hardwarecontent #hardwareparamsunderground #apikey").val(data["Username"]);
                            $("#hardwarecontent #hardwareparamsunderground #location").val(data["Password"]);
                        }
                        else if ((data["Type"].indexOf("HTTP/HTTPS") >= 0)) {
                            $("#hardwarecontent #hardwareparamshttp #url").val(data["Address"]);
                            $("#hardwarecontent #hardwareparamshttp #script").val(data["Extra"]);
                            $("#hardwarecontent #hardwareparamshttp #refresh").val(data["IntPort"]);
                        }
                        else if (data["Type"].indexOf("SBFSpot") >= 0) {
                            $("#hardwarecontent #hardwareparamslocation #location").val(data["Username"]);
                        }
                        else if (data["Type"].indexOf("SolarEdge via") >= 0) {
                            $("#hardwarecontent #hardwareparamssolaredgeapi #siteid").val(data["Mode1"]);
                            $("#hardwarecontent #hardwareparamssolaredgeapi #serial").val(data["Username"]);
                            $("#hardwarecontent #hardwareparamssolaredgeapi #apikey").val(data["Password"]);
                        }
                        else if (data["Type"].indexOf("Toon") >= 0) {
                            $("#hardwarecontent #hardwareparamsenecotoon #agreement").val(data["Mode1"]);
                        }
						else if (data["Type"].indexOf("Satel Integra") >= 0) {
                            $("#hardwarecontent #hardwareparamspollinterval #pollinterval").val(data["Mode1"]);
                        }
                        else if (data["Type"].indexOf("Philips Hue") >= 0) {
                            $("#hardwarecontent #hardwareparamsremote #tcpaddress").val(data["Address"]);
                            $("#hardwarecontent #hardwareparamsremote #tcpport").val(data["Port"]);
                            $("#hardwarecontent #hardwareparamsphilipshue #username").val(data["Username"]);
                        }
                        else if (data["Type"].indexOf("Winddelen") >= 0) {
                            $("#hardwarecontent #hardwareparamswinddelen #combomillselect").val(data["Mode1"]);
                            $("#hardwarecontent #hardwareparamswinddelen #nrofwinddelen").val(data["Port"]);
                        }
			else if (data["Type"].indexOf("Goodwe solar inverter via Web") >= 0) {
			    $("#hardwarecontent #hardwareparamsgoodweweb #username").val(data["Username"]);
			}
                        if (data["Type"].indexOf("MQTT") >= 0) {
                            $("#hardwarecontent #hardwareparamsmqtt #filename").val(data["Extra"]);
                            $("#hardwarecontent #hardwareparamsmqtt #combotopicselect").val(data["Mode1"]);
                        }
                        if (
                            (data["Type"].indexOf("Domoticz") >= 0)||
                            (data["Type"].indexOf("ICY") >= 0) ||
                            (data["Type"].indexOf("Toon") >= 0) ||
                            (data["Type"].indexOf("Harmony") >= 0)||
                            (data["Type"].indexOf("Atag") >= 0)||
                            (data["Type"].indexOf("Nest Th") >= 0)||
                            (data["Type"].indexOf("PVOutput") >= 0)||
                            (data["Type"].indexOf("ETH8020") >= 0)||
                            (data["Type"].indexOf("Daikin") >= 0)||
                            (data["Type"].indexOf("Sterbox") >= 0)||
                            (data["Type"].indexOf("Anna") >= 0)||
                            (data["Type"].indexOf("KMTronic") >= 0)||
                            (data["Type"].indexOf("MQTT") >= 0)||
                            (data["Type"].indexOf("Netatmo") >= 0)||
                            (data["Type"].indexOf("Fitbit") >= 0)||
							(data["Type"].indexOf("HTTP") >= 0)||
                            (data["Type"].indexOf("Thermosmart") >= 0) ||
							(data["Type"].indexOf("Logitech Media Server") >= 0) ||
							(data["Type"].indexOf("HEOS by DENON") >= 0) ||
							(data["Type"].indexOf("Razberry") >= 0) ||
							(data["Type"].indexOf("Comm5") >= 0)
                            )
                        {
                            $("#hardwarecontent #hardwareparamslogin #username").val(data["Username"]);
                            $("#hardwarecontent #hardwareparamslogin #password").val(data["Password"]);
                        }
                    }
                }
            });

          $('#modal').hide();
        }

        RegisterPhilipsHue = function()
        {
            var address=$("#hardwarecontent #divremote #tcpaddress").val();
            if (address=="")
            {
                ShowNotify($.t('Please enter an Address!'), 2500, true);
                return;
            }
            var port=$("#hardwarecontent #divremote #tcpport").val();
            if (port=="")
            {
                ShowNotify($.t('Please enter an Port!'), 2500, true);
                return;
            }
            var username=$("#hardwarecontent #hardwareparamsphilipshue #username").val();
            $.ajax({
                url: "json.htm?type=command&param=registerhue" +
                    "&ipaddress=" +address +
                    "&port=" + port +
                    "&username=" + encodeURIComponent(username),
                 async: false,
                 dataType: 'json',
                 success: function(data) {
                    if (data.status=="ERR") {
                        ShowNotify(data.statustext, 2500, true);
                        return;
                    }
                    $("#hardwarecontent #hardwareparamsphilipshue #username").val(data.username)
                    ShowNotify($.t('Registration successful!'),2500);
                 },
                 error: function(){
                        HideNotify();
                        ShowNotify($.t('Problem registrating with the Philips Hue bridge!'), 2500, true);
                 }
            });
        }

        UpdateHardwareParamControls = function()
        {
            $("#hardwarecontent #hardwareparamstable #enabled").prop('disabled', false);
            $("#hardwarecontent #hardwareparamstable #hardwarename").prop('disabled', false);
            $("#hardwarecontent #hardwareparamstable #combotype").prop('disabled', false);
            $("#hardwarecontent #hardwareparamstable #combodatatimeout").prop('disabled', false);

            var text = $("#hardwarecontent #hardwareparamstable #combotype option:selected").text();
            $("#hardwarecontent #username").show();
            $("#hardwarecontent #lblusername").show();

            $("#hardwarecontent #divbaudratemysensors").hide();
            $("#hardwarecontent #divbaudratep1").hide();
            $("#hardwarecontent #divcrcp1").hide();
            $("#hardwarecontent #divlocation").hide();
            $("#hardwarecontent #divphilipshue").hide();
            $("#hardwarecontent #divwinddelen").hide();
            $("#hardwarecontent #divmqtt").hide();
            $("#hardwarecontent #divsolaredgeapi").hide();
            $("#hardwarecontent #divenecotoon").hide();
            $("#hardwarecontent #div1wire").hide();
            $("#hardwarecontent #divgoodweweb").hide();
            $("#hardwarecontent #divi2clocal").hide();
			$("#hardwarecontent #divpollinterval").hide();

            if ((text.indexOf("TE923") >= 0)||
               (text.indexOf("Volcraft") >= 0)||
               (text.indexOf("Dummy") >= 0)||
               (text.indexOf("System Alive") >= 0)||
               (text.indexOf("PiFace") >= 0) ||
               (text.indexOf("Yeelight") >= 0))
            {
                $("#hardwarecontent #divserial").hide();
                $("#hardwarecontent #divremote").hide();
                $("#hardwarecontent #divlogin").hide();
                $("#hardwarecontent #divunderground").hide();
                $("#hardwarecontent #divhttppoller").hide();
            }
	    else if (text.indexOf("Local I2C sensor") >= 0)
	    {
                $("#hardwarecontent #divi2clocal").show();
                $("#hardwarecontent #divserial").hide();
                $("#hardwarecontent #divremote").hide();
                $("#hardwarecontent #divlogin").hide();
                $("#hardwarecontent #divunderground").hide();
                $("#hardwarecontent #divhttppoller").hide();
	    }
            else if (text.indexOf("USB") >= 0)
            {
                if (text.indexOf("MySensors") >= 0)
                {
                    $("#hardwarecontent #divbaudratemysensors").show();
                }
                if (text.indexOf("P1 Smart Meter") >= 0)
                {
                    $("#hardwarecontent #divbaudratep1").show();
                    $("#hardwarecontent #divcrcp1").show();
                 }
                $("#hardwarecontent #divserial").show();
                $("#hardwarecontent #divremote").hide();
                $("#hardwarecontent #divlogin").hide();
                $("#hardwarecontent #divunderground").hide();
                $("#hardwarecontent #divhttppoller").hide();
            }
            else if (text.indexOf("LAN") >= 0 && text.indexOf("YouLess") == -1 && text.indexOf("Denkovi") == -1 && text.indexOf("Satel Integra") == -1)
            {
                $("#hardwarecontent #divserial").hide();
                $("#hardwarecontent #divremote").show();
                $("#hardwarecontent #divlogin").hide();
                $("#hardwarecontent #divunderground").hide();
                $("#hardwarecontent #divhttppoller").hide();
                if (text.indexOf("P1 Smart Meter") >= 0)
                {
                    $("#hardwarecontent #divcrcp1").show();
                }
            }
            else if (text.indexOf("LAN") >= 0 && (text.indexOf("YouLess") >= 0 || text.indexOf("Denkovi") >= 0 || text.indexOf("Satel Integra") >= 0))
            {
                $("#hardwarecontent #divserial").hide();
                $("#hardwarecontent #divremote").show();
                $("#hardwarecontent #divlogin").show();
                $("#hardwarecontent #username").hide();
                $("#hardwarecontent #lblusername").hide();
                $("#hardwarecontent #divunderground").hide();
                $("#hardwarecontent #divhttppoller").hide();
				if (text.indexOf("Satel Integra") >= 0)
                {
                    $("#hardwarecontent #divpollinterval").show();
					$("#hardwarecontent #hardwareparamspollinterval #pollinterval").val(1000);
                }
            }
            else if (text.indexOf("Domoticz") >= 0)
            {
                $("#hardwarecontent #divserial").hide();
                $("#hardwarecontent #divremote").show();
                $("#hardwarecontent #divlogin").show();
                $("#hardwarecontent #divunderground").hide();
                $("#hardwarecontent #divhttppoller").hide();
                $("#hardwarecontent #hardwareparamsremote #tcpport").val(6144);
            }
            else if (text.indexOf("Harmony") >= 0)
            {
                $("#hardwarecontent #divserial").hide();
                $("#hardwarecontent #divremote").show();
                $("#hardwarecontent #divlogin").show();
                $("#hardwarecontent #divunderground").hide();
                $("#hardwarecontent #divhttppoller").hide();
            }
            else if (text.indexOf("SolarEdge via") >= 0)
            {
				$("#hardwarecontent #divsolaredgeapi").show();
                $("#hardwarecontent #divremote").hide();
                $("#hardwarecontent #divserial").hide();
                $("#hardwarecontent #divunderground").hide();
                $("#hardwarecontent #divhttppoller").hide();
            }
            else if (text.indexOf("Toon") >= 0)
            {
		$("#hardwarecontent #divlogin").show();
		$("#hardwarecontent #divenecotoon").show();
                $("#hardwarecontent #divremote").hide();
                $("#hardwarecontent #divserial").hide();
                $("#hardwarecontent #divunderground").hide();
                $("#hardwarecontent #divhttppoller").hide();
            }
            else if (text.indexOf("SBFSpot") >= 0)
            {
                $("#hardwarecontent #divlocation").show();
                $("#hardwarecontent #divserial").hide();
                $("#hardwarecontent #divremote").hide();
                $("#hardwarecontent #divlogin").hide();
                $("#hardwarecontent #divunderground").hide();
                $("#hardwarecontent #divhttppoller").hide();
                $("#hardwarecontent #username").hide();
                $("#hardwarecontent #lblusername").hide();
            }
            else if (
				(text.indexOf("ICY") >= 0) ||
				(text.indexOf("Atag") >= 0) ||
				(text.indexOf("Nest Th") >= 0) ||
				(text.indexOf("PVOutput") >= 0) ||
				(text.indexOf("Netatmo") >= 0) ||
				(text.indexOf("Fitbit") >= 0) ||
				(text.indexOf("Thermosmart") >= 0)
			) {
                $("#hardwarecontent #divserial").hide();
                $("#hardwarecontent #divremote").hide();
                $("#hardwarecontent #divlogin").show();
                $("#hardwarecontent #divunderground").hide();
                $("#hardwarecontent #divhttppoller").hide();
            }
            else if (text.indexOf("HTTP") >= 0) {
                $("#hardwarecontent #divserial").hide();
                $("#hardwarecontent #divremote").hide();
                $("#hardwarecontent #divlogin").show();
                $("#hardwarecontent #divunderground").hide();
                $("#hardwarecontent #divhttppoller").show();
                $("#hardwarecontent #hardwareparamshttp #refresh").val(300);
            }
            else if ((text.indexOf("Underground") >= 0) || (text.indexOf("DarkSky") >= 0) || (text.indexOf("AccuWeather") >= 0) || (text.indexOf("Open Weather Map") >= 0))
            {
                $("#hardwarecontent #divserial").hide();
                $("#hardwarecontent #divremote").hide();
                $("#hardwarecontent #divlogin").hide();
                $("#hardwarecontent #divunderground").show();
                $("#hardwarecontent #divhttppoller").hide();
                if (text.indexOf("Open Weather Map") >= 0)
                {
                    $("#hardwarecontent #hardwareparamsunderground #location").val("lat=53.40&lon=14.58");
                }
            }
            else if (text.indexOf("Philips Hue") >= 0)
            {
                $("#hardwarecontent #divserial").hide();
                $("#hardwarecontent #divremote").show();
                $("#hardwarecontent #divlogin").hide();
                $("#hardwarecontent #divphilipshue").show();
                $("#hardwarecontent #divunderground").hide();
                $("#hardwarecontent #divhttppoller").hide();
                $("#hardwarecontent #hardwareparamsremote #tcpport").val(80);
            }
            else if (text.indexOf("Yeelight") >= 0) {
                $("#hardwarecontent #divserial").hide();
                $("#hardwarecontent #divremote").hide();
                $("#hardwarecontent #divlogin").hide();
                $("#hardwarecontent #divunderground").hide();
                $("#hardwarecontent #divhttppoller").hide();
            }
            else if (text.indexOf("Winddelen") >= 0)
            {
                $("#hardwarecontent #divserial").hide();
                $("#hardwarecontent #divremote").hide();
                $("#hardwarecontent #divlogin").hide();
                $("#hardwarecontent #username").hide();
                $("#hardwarecontent #lblusername").hide();
                $("#hardwarecontent #divunderground").hide();
                $("#hardwarecontent #divhttppoller").hide();
                $("#hardwarecontent #divwinddelen").show();
            }
            else if (text.indexOf("Logitech Media Server") >= 0)
            {
                $("#hardwarecontent #divserial").hide();
                $("#hardwarecontent #divremote").show();
                $("#hardwarecontent #divlogin").show();
                $("#hardwarecontent #hardwareparamsremote #tcpport").val(9000);
            }
            else if (text.indexOf("HEOS by DENON") >= 0)
            {
                $("#hardwarecontent #divserial").hide();
                $("#hardwarecontent #divremote").show();
                $("#hardwarecontent #divlogin").show();
                $("#hardwarecontent #hardwareparamsremote #tcpport").val(1255);
            }
	    else if (text.indexOf("MyHome OpenWebNet") >= 0)
            {
                $("#hardwarecontent #divserial").hide();
                $("#hardwarecontent #divremote").show();
                $("#hardwarecontent #divlogin").hide();
                $("#hardwarecontent #divunderground").hide();
                $("#hardwarecontent #divhttppoller").hide();
                $("#hardwarecontent #hardwareparamsremote #tcpport").val(20000);
            }
	    else if (text.indexOf("1-Wire") >= 0)
            {
	        $("#hardwarecontent #div1wire").show();
                $("#hardwarecontent #divserial").hide();
                $("#hardwarecontent #divremote").hide();
                $("#hardwarecontent #divlogin").hide();
                $("#hardwarecontent #divunderground").hide();
                $("#hardwarecontent #divhttppoller").hide();
            }
	    else if (text.indexOf("Goodwe solar inverter via Web") >= 0)
            {
	        $("#hardwarecontent #divgoodweweb").show();
	        $("#hardwarecontent #div1wire").hide();
                $("#hardwarecontent #divserial").hide();
                $("#hardwarecontent #divremote").hide();
                $("#hardwarecontent #divlogin").hide();
                $("#hardwarecontent #divunderground").hide();
                $("#hardwarecontent #divhttppoller").hide();
            }
	    
            else
            {
                $("#hardwarecontent #divserial").hide();
                $("#hardwarecontent #divremote").hide();
                $("#hardwarecontent #divlogin").hide();
                $("#hardwarecontent #divunderground").hide();
                $("#hardwarecontent #divhttppoller").hide();
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
            if (text.indexOf("MQTT") >= 0)
            {
                $("#hardwarecontent #divmqtt").show();
            }
        }

        ShowHardware = function()
        {
            $('#modal').show();
            var htmlcontent = "";
            htmlcontent+=$('#hardwaremain').html();
            $('#hardwarecontent').html(htmlcontent);
            $('#hardwarecontent').i18n();
            var oTable = $('#hardwaretable').dataTable( {
              "sDom": '<"H"lfrC>t<"F"ip>',
              "oTableTools": {
                "sRowSelect": "single",
              },
              "aaSorting": [[ 0, "desc" ]],
              "bSortClasses": false,
              "bProcessing": true,
              "bStateSave": true,
              "bJQueryUI": true,
              "aLengthMenu": [[25, 50, 100, -1], [25, 50, 100, "All"]],
              "iDisplayLength" : 25,
              "sPaginationType": "full_numbers",
              language: $.DataTableLanguage
            } );


            $("#hardwarecontent #hardwareparamstable #combotype").change(function() {
                UpdateHardwareParamControls();
            });

            $("#hardwarecontent #divbaudratep1 #combobaudratep1").change(function() {
                if ($("#hardwarecontent #divbaudratep1 #combobaudratep1 option:selected").val() == 0)
                {
                    $("#hardwarecontent #divcrcp1 #disablecrcp1").prop("checked",0);
                    $("#hardwarecontent #divcrcp1").hide();
                }
                else
                {
                    $("#hardwarecontent #divcrcp1 #disablecrcp1").prop("checked",1);
                    $("#hardwarecontent #divcrcp1").show();
                }
            });

            $('#modal').hide();
            RefreshHardwareTable();
            UpdateHardwareParamControls();
        }

        init();

        function init()
        {
            //global var
            $.devIdx=0;
            $.myglobals = {
                HardwareTypesStr : [],
		HardwareI2CStr : [],
                SelectedHardwareIdx: 0
            };
            $scope.SerialPortStr=[];
            $scope.MakeGlobalConfig();

            //Get hardware types
            $("#hardwareparamstable #combotype").html("");
            $.ajax({
		url: "json.htm?type=command&param=gethardwaretypes",
		async: false,
		dataType: 'json',
		success: function(data) {
                    if (typeof data.result != 'undefined') {
			var i2cidx = 0, idx = 0;
			$.each(data.result, function(i,item) {
			    $.myglobals.HardwareTypesStr[item.idx] = item.name;
			    // Don't show I2C sensors
			    if (item.name.indexOf("Local I2C sensor") != -1) {
				$.myglobals.HardwareI2CStr[item.idx] = item.name;
				i2cidx = idx;
				return true;
			    }
			    // Show other sensors
                            var option = $('<option />');
                            option.attr('value', item.idx).text(item.name);
                            $("#hardwareparamstable #combotype").append(option);
			    idx++;
			});
			// regroup local I2C sensors under index 1000
                        var option = $('<option />');
                        option.attr('value', 1000).text("Local I2C sensors");
			option.insertAfter('#hardwareparamstable #combotype :nth-child(' + i2cidx + ')')
                    }
             }
            });

	    //Build local I2C devices combo
            $("#hardwareparamsi2clocal #comboi2clocal").html("");
            $.each($.myglobals.HardwareI2CStr, function(idx,name) {
		if (name) {
                    var option = $('<option />');
                    option.attr('value', idx).text(name);
                    $("#hardwareparamsi2clocal #comboi2clocal").append(option);
		}
	    });

            //Get Serial devices
            $("#hardwareparamsserial #comboserialport").html("");
            $.ajax({
             url: "json.htm?type=command&param=serial_devices",
             async: false,
             dataType: 'json',
             success: function(data) {
                if (typeof data.result != 'undefined') {
                    $.each(data.result, function(i,item) {
                        var option = $('<option />');
                        option.attr('value', item.value).text(item.name);
                        $("#hardwareparamsserial #comboserialport").append(option);
                    });
                }
             }
            });

            $('#hardwareparamsserial #comboserialport > option').each(function() {
                 $scope.SerialPortStr.push($(this).text());
            });

            ShowHardware();
        };
		$scope.$on('$destroy', function(){
			if (typeof $scope.mytimer != 'undefined') {
				$interval.cancel($scope.mytimer);
				$scope.mytimer = undefined;
			}
			if (typeof $scope.mytimer2 != 'undefined') {
				$interval.cancel($scope.mytimer2);
				$scope.mytimer2 = undefined;
			}
		});
    } ]);
});
