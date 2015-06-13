define(['app'], function (app) {
	app.controller('HardwareController', [ '$scope', '$rootScope', '$location', '$http', '$interval', function($scope,$rootScope,$location,$http,$interval) {

		$scope.SerialPortStr=[];
		
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

		UpdateHardware= function(idx,Mode1,Mode2,Mode3,Mode4,Mode5,Mode6)
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
			if ((text.indexOf("TE923") >= 0)||(text.indexOf("Volcraft") >= 0)||(text.indexOf("1-Wire") >= 0)||(text.indexOf("GPIO") >= 0)||(text.indexOf("BMP085") >= 0)||(text.indexOf("Dummy") >= 0)||(text.indexOf("System Alive") >= 0)||(text.indexOf("PiFace") >= 0)||(text.indexOf("Motherboard") >= 0)||(text.indexOf("Evohome") >= 0 && text.indexOf("script") >= 0))
			{
				$.ajax({
					 url: "json.htm?type=command&param=updatehardware&htype=" + hardwaretype +
						"&port=1&name=" + encodeURIComponent(name) + 
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
				var address="";
				if (text.indexOf("S0 Meter") >= 0)
				{
					address=$("#hardwarecontent #divremote #tcpaddress").val();
				}

				$.ajax({
					 url: "json.htm?type=command&param=updatehardware&htype=" + hardwaretype +
						"&port=" + encodeURIComponent(serialport) + 
						"&address=" + address + 
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
			else if (text.indexOf("LAN") >= 0 && text.indexOf("YouLess") == -1 && text.indexOf("ETH8020") == -1 && text.indexOf("KMTronic") == -1 && text.indexOf("MQTT") == -1)
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
			else if (text.indexOf("LAN") >= 0 && text.indexOf("YouLess") >= 0 )
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
			else if ((text.indexOf("Domoticz") >= 0) || (text.indexOf("Harmony") >= 0) || (text.indexOf("ETH8020") >= 0) || (text.indexOf("KMTronic") >= 0) || (text.indexOf("MQTT") >= 0))
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
			else if ((text.indexOf("Underground") >= 0)||(text.indexOf("Forecast") >= 0))
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
						"&port=1" + 
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
						"&port=1" + 
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
			else if ((text.indexOf("ICY") >= 0) || (text.indexOf("Toon") >= 0) || (text.indexOf("Nest Th") >= 0) || (text.indexOf("PVOutput") >= 0) || (text.indexOf("Thermosmart") >= 0)) {
				var username = $("#hardwarecontent #divlogin #username").val();
				var password = encodeURIComponent($("#hardwarecontent #divlogin #password").val());
				$.ajax({
					url: "json.htm?type=command&param=updatehardware&htype=" + hardwaretype +
					   "&port=1" +
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
			if (text.indexOf("Motherboard") >= 0) {
				ShowNotify($.t('This device is maintained by the system. Please do not add it manually.'), 3000, true);
				return;
			}

			if ((text.indexOf("TE923") >= 0)||(text.indexOf("Volcraft") >= 0)||(text.indexOf("1-Wire") >= 0)||(text.indexOf("BMP085") >= 0)||(text.indexOf("Dummy") >= 0)||(text.indexOf("System Alive") >= 0)||(text.indexOf("PiFace") >= 0)||(text.indexOf("GPIO") >= 0)||(text.indexOf("Evohome") >= 0 && text.indexOf("script") >= 0))
			{
				$.ajax({
					 url: "json.htm?type=command&param=addhardware&htype=" + hardwaretype + "&port=1&name=" + encodeURIComponent(name) + "&enabled=" + bEnabled + "&datatimeout=" + datatimeout,
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
				var serialport=$("#hardwarecontent #divserial #comboserialport option:selected").text();
				if (typeof serialport == 'undefined')
				{
					ShowNotify($.t('No serial port selected!'), 2500, true);
					return;
				}
				$.ajax({
					 url: "json.htm?type=command&param=addhardware&htype=" + hardwaretype + "&port=" + encodeURIComponent(serialport) + "&name=" + encodeURIComponent(name) + "&enabled=" + bEnabled + "&datatimeout=" + datatimeout,
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
			else if (text.indexOf("LAN") >= 0 && text.indexOf("YouLess") == -1 && text.indexOf("ETH8020") == -1 && text.indexOf("KMTronic") == -1 && text.indexOf("MQTT") == -1)
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
			else if (text.indexOf("LAN") >= 0 && text.indexOf("YouLess") >= 0)
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
				var password=encodeURIComponent($("#hardwarecontent #divlogin #password").val());
				$.ajax({
					 url: "json.htm?type=command&param=addhardware&htype=" + hardwaretype + "&address=" + address + "&port=" + port + "&name=" + encodeURIComponent(name) + "&password=" + encodeURIComponent(password) + "&enabled=" + bEnabled + "&datatimeout=" + datatimeout,
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
			else if ((text.indexOf("Domoticz") >= 0) || (text.indexOf("Harmony") >= 0) || (text.indexOf("ETH8020") >= 0) || (text.indexOf("KMTronic") >= 0) || (text.indexOf("MQTT") >= 0))
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

				if ((text.indexOf("Harmony") >= 0) && (username == "")) {
					ShowNotify($.t('Please enter a username!'), 2500, true);
					return;
				}

				if ((text.indexOf("Harmony") >= 0) && (password == "")) {
					ShowNotify($.t('Please enter a password!'), 2500, true);
					return;
				}
				
				$.ajax({
					 url: "json.htm?type=command&param=addhardware&htype=" + hardwaretype + "&address=" + address + "&port=" + port + "&username=" + encodeURIComponent(username) + "&password=" + encodeURIComponent(password) + "&name=" + encodeURIComponent(name) + "&enabled=" + bEnabled + "&datatimeout=" + datatimeout,
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
			else if ((text.indexOf("Underground") >= 0)||(text.indexOf("Forecast") >= 0))
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
					 url: "json.htm?type=command&param=addhardware&htype=" + hardwaretype + "&port=1" + "&username=" + encodeURIComponent(apikey) + "&password=" + encodeURIComponent(location) + "&name=" + encodeURIComponent(name) + "&enabled=" + bEnabled + "&datatimeout=" + datatimeout,
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
						"&port=1" + 
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
			else if ((text.indexOf("ICY") >= 0)||(text.indexOf("Toon") >= 0)||(text.indexOf("Nest Th") >= 0)||(text.indexOf("PVOutput") >= 0)||(text.indexOf("Thermosmart") >= 0))
			{
				var username=$("#hardwarecontent #divlogin #username").val();
				var password=encodeURIComponent($("#hardwarecontent #divlogin #password").val());
				$.ajax({
					 url: "json.htm?type=command&param=addhardware&htype=" + hardwaretype +
					 "&port=1" + 
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

			$('#hardwarecontent #idx').val(idx);
			$('#hardwarecontent #Keeloq').prop('checked',((Mode6 & 0x01)!=0));
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

		SetP1USBType = function()
		{
		  $.post("setp1usbtype.webem", $("#hardwarecontent #p1usbtype").serialize(), function(data) {
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

		EditSBFSpot = function(idx,name,Mode1,Mode2,Mode3,Mode4,Mode5,Mode6)
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

		EditP1USB = function(idx,name,Mode1,Mode2,Mode3,Mode4,Mode5,Mode6)
		{
			$.devIdx=idx;
			cursordefault();
			var htmlcontent = '';
			htmlcontent='<p><center><h2><span data-i18n="Device"></span>: ' + name + '</h2></center></p>\n';
			htmlcontent+=$('#p1usbeedit').html();
			$('#hardwarecontent').html(GetBackbuttonHTMLTable('ShowHardware')+htmlcontent);
			$('#hardwarecontent').i18n();

			$('#hardwarecontent #submitbuttonp1usb').click(function (e) {
				e.preventDefault();
				SetP1USBType();
			});

			$('#hardwarecontent #idx').val(idx);
			$('#hardwarecontent #P1Baudrate').val(Mode1);
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
					
					valueList+=id+"_"+value+"__";
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
					valueList+=id+"_"+value+"__";
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

		ZWaveIncludeNode = function(isSecure)
		{
			$.ajax({
				 url: "json.htm?type=command&param=zwaveinclude&idx=" + $.devIdx + "&secure=" + isSecure,
				 async: false, 
				 dataType: 'json',
				 success: function(data) {
					bootbox.alert($.t('Inclusion mode Started. You have 20 seconds to include the device...!'));
				 }
			});
		}
		ZWaveExcludeNode = function()
		{
			$.ajax({
				 url: "json.htm?type=command&param=zwaveexclude&idx=" + $.devIdx,
				 async: false, 
				 dataType: 'json',
				 success: function(data) {
					bootbox.alert($.t('Exclusion mode Started. You have 20 seconds to exclude the device...!'));
				 }
			});
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
							text: '<center>' + $.t('ZWave Network Information') + '</center><p><p><iframe src="../zwavetopology.html?hwid='+$.devIdx+'" name="topoframe" frameBorder="0" height="'+window.innerHeight*0.7+'" width="100%"/>',
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
						"3": item.LastUpdate,
						"4": $.t((item.PollEnabled == "true")?"Yes":"No"),
						"5": statusImg+'&nbsp;&nbsp;'+healButton,
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
						$("#hardwarecontent #nodeparamstable #nodename").val(data["1"]);
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
				ShowNotify($.t('Hold bind button on relay...'));
			else
				ShowNotify($.t('Binding Domoticz outdoor temperature device to evohome controller...'));

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
							ShowNotify($.t('Relay bound, and can be found in the devices tab!'));
						else
							ShowNotify($.t('Domoticz outdoor temperature device has been bound to evohome controller'));
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
								 url: "json.htm?type=createevohomesensor&idx=" + $.devIdx + "&sensortype=" + SensorType,
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

		CreateDummySensors = function(idx,name)
		{
			$.devIdx=idx;
			$( "#dialog-createsensor" ).dialog({
				  autoOpen: false,
				  width: 380,
				  height: 160,
				  modal: true,
				  resizable: false,
				  buttons: {
					  "OK": function() {
						  var bValid = true;
						  $( this ).dialog( "close" );
						  
							var SensorType=$("#dialog-createsensor #sensortype option:selected").val();
							if (typeof SensorType == 'undefined') {
								bootbox.alert($.t('No Sensor Type Selected!'));
								return ;
							}
							$.ajax({
								 url: "json.htm?type=createvirtualsensor&idx=" + $.devIdx + "&sensortype=" + SensorType,
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
					else if (item.Type == 12)
					{
						SerialName="System";
					}
					else if (item.Type == 13)
					{
						SerialName="I2C";
					}
					else if ((item.Type == 14)||(item.Type == 25)||(item.Type == 28)||(item.Type == 30)||(item.Type == 34))
					{
						SerialName="WWW";
					}
					else if ((item.Type == 15)||(item.Type == 23)||(item.Type == 26)||(item.Type == 27)||(item.Type == 51))
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
						HwTypeStr+='<br>Firmware version: ' + item.Mode2;
						HwTypeStr+=' <span class="label label-info lcursor" onclick="EditRFXCOMMode(' + item.idx + ',\'' + item.Name + '\',' + item.Mode1 + ',' + item.Mode2+ ',' + item.Mode3+ ',' + item.Mode4+ ',' + item.Mode5 + ',' + item.Mode6 + ');">' + $.t("Set Mode") + '</span>';
					}
					else if (HwTypeStr.indexOf("S0 Meter USB") >= 0) {
						HwTypeStr+=' <span class="label label-info lcursor" onclick="EditS0MeterType(' + item.idx + ',\'' + item.Name + '\',\'' + item.Address + '\');">' + $.t("Set Mode") + '</span>';
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
					else if (HwTypeStr.indexOf("OpenTherm") >= 0) {
						HwTypeStr+=' <span class="label label-info lcursor" onclick="EditOpenTherm(' + item.idx + ',\'' + item.Name + '\',' + item.Mode1 + ',' + item.Mode2+ ',' + item.Mode3+ ',' + item.Mode4+ ',' + item.Mode5 + ',' + item.Mode6 + ');">' + $.t("Setup") + '</span>';
					}
					else if (HwTypeStr.indexOf("Wake-on-LAN") >= 0) {
						HwTypeStr+=' <span class="label label-info lcursor" onclick="EditWOL(' + item.idx + ',\'' + item.Name + '\',' + item.Mode1 + ',' + item.Mode2+ ',' + item.Mode3+ ',' + item.Mode4+ ',' + item.Mode5 + ',' + item.Mode6 + ');">' + $.t("Setup") + '</span>';
					}
					else if (HwTypeStr.indexOf("System Alive") >= 0) {
						HwTypeStr+=' <span class="label label-info lcursor" onclick="EditPinger(' + item.idx + ',\'' + item.Name + '\',' + item.Mode1 + ',' + item.Mode2+ ',' + item.Mode3+ ',' + item.Mode4+ ',' + item.Mode5 + ',' + item.Mode6 + ');">' + $.t("Setup") + '</span>';
					}
					else if (HwTypeStr.indexOf("P1 Smart Meter USB") >= 0) {
						HwTypeStr+=' <span class="label label-info lcursor" onclick="EditP1USB(' + item.idx + ',\'' + item.Name + '\',' + item.Mode1 + ',' + item.Mode2+ ',' + item.Mode3+ ',' + item.Mode4+ ',' + item.Mode5 + ',' + item.Mode6 + ');">' + $.t("Setup") + '</span>';
					}
					else if (HwTypeStr.indexOf("Dummy") >= 0) {
						HwTypeStr+=' <span class="label label-info lcursor" onclick="CreateDummySensors(' + item.idx + ',\'' + item.Name + '\');">' + $.t("Create Virtual Sensors") + '</span>';
					}
					else if (HwTypeStr.indexOf("Evohome") >= 0) {
						if(HwTypeStr.indexOf("script") >= 0)
							HwTypeStr+=' <span class="label label-info lcursor" onclick="CreateEvohomeSensors(' + item.idx + ',\'' + item.Name + '\');">' + $.t("Create Devices") + '</span>';
						else
						{
							HwTypeStr+=' <span class="label label-info lcursor" onclick="BindEvohome(' + item.idx + ',\'' + item.Name + '\',\'Relay\');">Bind Relay</span>';
							HwTypeStr+=' <span class="label label-info lcursor" onclick="BindEvohome(' + item.idx + ',\'' + item.Name + '\',\'OutdoorSensor\');">Outdoor Sensor</span>';
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
					if (HwTypeStr.indexOf("S0 Meter") >= 0) {
						dispAddress="";
					}
							
					var addId = oTable.fnAddData( {
						"DT_RowId": item.idx,
						"Username": item.Username,
						"Password": item.Password,
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

						UpdateHardwareParamControls();
						if ((data["Type"].indexOf("TE923") >= 0)||(data["Type"].indexOf("Volcraft") >= 0)||(data["Type"].indexOf("1-Wire") >= 0)||(data["Type"].indexOf("BMP085") >= 0)||(data["Type"].indexOf("Dummy") >= 0)||(data["Type"].indexOf("System Alive") >= 0) ||(data["Type"].indexOf("PiFace") >= 0))
						{
							//nothing to be set
						}
						else if (data["Type"].indexOf("USB") >= 0) {
							$("#hardwarecontent #hardwareparamsserial #comboserialport").val(data["IntPort"]);
							if (data["Type"].indexOf("S0 Meter") >= 0)
							{
								$("#hardwarecontent #divremote #tcpaddress").val(data["Address"]);
							}
						}
						else if (((data["Type"].indexOf("LAN") >= 0) && (data["Type"].indexOf("YouLess") == -1)) ||(data["Type"].indexOf("Domoticz") >= 0) ||(data["Type"].indexOf("Harmony") >= 0)) {
							$("#hardwarecontent #hardwareparamsremote #tcpaddress").val(data["Address"]);
							$("#hardwarecontent #hardwareparamsremote #tcpport").val(data["Port"]);
						}
						else if (((data["Type"].indexOf("LAN") >= 0) && (data["Type"].indexOf("YouLess") >= 0)) ||(data["Type"].indexOf("Domoticz") >= 0) ||(data["Type"].indexOf("Harmony") >= 0)) {
							$("#hardwarecontent #hardwareparamsremote #tcpaddress").val(data["Address"]);
							$("#hardwarecontent #hardwareparamsremote #tcpport").val(data["Port"]);
							$("#hardwarecontent #hardwareparamslogin #password").val(data["Password"]);
						}
						else if ((data["Type"].indexOf("Underground") >= 0)||(data["Type"].indexOf("Forecast") >= 0)) {
							$("#hardwarecontent #hardwareparamsunderground #apikey").val(data["Username"]);
							$("#hardwarecontent #hardwareparamsunderground #location").val(data["Password"]);
						}
						else if (data["Type"].indexOf("SBFSpot") >= 0) {
							$("#hardwarecontent #hardwareparamslocation #location").val(data["Username"]);
						}
						else if (data["Type"].indexOf("Philips Hue") >= 0) {
							$("#hardwarecontent #hardwareparamsremote #tcpaddress").val(data["Address"]);
							$("#hardwarecontent #hardwareparamsremote #tcpport").val(data["Port"]);
							$("#hardwarecontent #hardwareparamsphilipshue #username").val(data["Username"]);
						}
						if ((data["Type"].indexOf("Domoticz") >= 0)||(data["Type"].indexOf("ICY") >= 0) ||(data["Type"].indexOf("Harmony") >= 0)||(data["Type"].indexOf("Toon") >= 0)||(data["Type"].indexOf("Nest Th") >= 0)||(data["Type"].indexOf("PVOutput") >= 0)||(data["Type"].indexOf("Thermosmart") >= 0)||(data["Type"].indexOf("ETH8020") >= 0)||(data["Type"].indexOf("KMTronic") >= 0)||(data["Type"].indexOf("MQTT") >= 0)) {
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
			var text = $("#hardwarecontent #hardwareparamstable #combotype option:selected").text();

			$("#hardwarecontent #username").show();
			$("#hardwarecontent #lblusername").show();
					
			$("#hardwarecontent #divlocation").hide();
			$("#hardwarecontent #divphilipshue").hide();

			if ((text.indexOf("TE923") >= 0)||(text.indexOf("Volcraft") >= 0)||(text.indexOf("BMP085") >= 0)||(text.indexOf("Dummy") >= 0)||(text.indexOf("System Alive") >= 0)||(text.indexOf("PiFace") >= 0))
			{
				$("#hardwarecontent #divserial").hide();
				$("#hardwarecontent #divremote").hide();
				$("#hardwarecontent #divlogin").hide();
				$("#hardwarecontent #divunderground").hide();
			}
			else if (text.indexOf("USB") >= 0)
			{
				$("#hardwarecontent #divserial").show();
				$("#hardwarecontent #divremote").hide();
				$("#hardwarecontent #divlogin").hide();
				$("#hardwarecontent #divunderground").hide();
			}
			else if (text.indexOf("LAN") >= 0 && text.indexOf("YouLess") == -1)
			{
				$("#hardwarecontent #divserial").hide();
				$("#hardwarecontent #divremote").show();
				$("#hardwarecontent #divlogin").hide();
				$("#hardwarecontent #divunderground").hide();
			}
			else if (text.indexOf("LAN") >= 0 && text.indexOf("YouLess") >= 0)
			{
				$("#hardwarecontent #divserial").hide();
				$("#hardwarecontent #divremote").show();
				$("#hardwarecontent #divlogin").show();
				$("#hardwarecontent #username").hide();
				$("#hardwarecontent #lblusername").hide();
				$("#hardwarecontent #divunderground").hide();
			}
			else if ((text.indexOf("Domoticz") >= 0) || (text.indexOf("Harmony") >= 0))
			{
				$("#hardwarecontent #divserial").hide();
				$("#hardwarecontent #divremote").show();
				$("#hardwarecontent #divlogin").show();
				$("#hardwarecontent #divunderground").hide();
			}
			else if (text.indexOf("SBFSpot") >= 0)
			{
				$("#hardwarecontent #divlocation").show();
				$("#hardwarecontent #divserial").hide();
				$("#hardwarecontent #divremote").hide();
				$("#hardwarecontent #divlogin").hide();
				$("#hardwarecontent #divunderground").hide();
				$("#hardwarecontent #username").hide();
				$("#hardwarecontent #lblusername").hide();
			}
			else if ((text.indexOf("ICY") >= 0)||(text.indexOf("Toon") >= 0)||(text.indexOf("Nest Th") >= 0)||(text.indexOf("PVOutput") >= 0)||(text.indexOf("Thermosmart") >= 0))
			{
				$("#hardwarecontent #divserial").hide();
				$("#hardwarecontent #divremote").hide();
				$("#hardwarecontent #divlogin").show();
				$("#hardwarecontent #divunderground").hide();
			}
			else if ((text.indexOf("Underground") >= 0)||(text.indexOf("Forecast") >= 0))
			{
				$("#hardwarecontent #divserial").hide();
				$("#hardwarecontent #divremote").hide();
				$("#hardwarecontent #divlogin").hide();
				$("#hardwarecontent #divunderground").show();
			}
			else if (text.indexOf("Philips Hue") >= 0)
			{
				$("#hardwarecontent #divserial").hide();
				$("#hardwarecontent #divremote").show();
				$("#hardwarecontent #divlogin").hide();
				$("#hardwarecontent #divphilipshue").show();
				$("#hardwarecontent #divunderground").hide();
				$("#hardwarecontent #hardwareparamsremote #tcpport").val(80);
			}
			else
			{
				$("#hardwarecontent #divserial").hide();
				$("#hardwarecontent #divremote").hide();
				$("#hardwarecontent #divlogin").hide();
				$("#hardwarecontent #divunderground").hide();
			}
			if ((text.indexOf("ETH8020") >= 0)||(text.indexOf("MQTT") >= 0)||(text.indexOf("KMTronic Gateway with LAN") >= 0)) {
				$("#hardwarecontent #divlogin").show();
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
					$.each(data.result, function(i,item) {
						var option = $('<option />');
						option.attr('value', item.idx).text(item.name);
						$("#hardwareparamstable #combotype").append(option);
					});
				}
			 }
			});
			$('#hardwareparamstable #combotype > option').each(function() {
				 $.myglobals.HardwareTypesStr[$(this).val()]=$(this).text();
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
	} ]);
});