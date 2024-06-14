define(['app'], function (app) {
	app.controller('EnergyDashboardController', ['$scope', '$rootScope', '$location', '$http', '$interval', 'livesocket', function ($scope, $rootScope, $location, $http, $interval, livesocket) {

		$scope.idP1 = -1;
		$scope.idGas = -1;
		$scope.idWater = -1;
		$scope.idSolar = -1;
		$scope.idBattWatt = -1;
		$scope.idBattSoc = -1;
		$scope.idTextObj = -1;
		$scope.idItemH1 = -1;
		$scope.fieldH1 = "Usage";		// See output of JSON API /json.htm?type=command&param=getdevices&rid=xx
		$scope.iconH1 = "car";
		$scope.idItemH2 = -1;
		$scope.fieldH2 = "CounterToday";
		$scope.iconH2 = "heater";
		$scope.idItemH3 = -1;
		$scope.fieldH3 = "Data";
		$scope.iconH3 = "other";
		
		$scope.convertWaterM3ToLiter = true;
		$scope.bEnableServerTime = true;
		$scope.flowAsLines = true;
		$scope.useCustomIcons = false;

		$scope.P1InkWh = false;
		$scope.fDayNetUsage = 0;
		$scope.fDayNetDeliv = 0;
		$scope.fTotalHomeUsage = 0;
		$scope.fDaySolar = 0;
		$scope.fDayHomeUsage = 0;
		$scope.fActualNet = 0;
		$scope.fActualSolar = 0;
		$scope.fActualHomeUsage = 0;
		$scope.finalHomeUsage = 0;
		$scope.fGridToHome = 0;
		$scope.fSolarToGrid = 0;
		$scope.fSolarToHome = 0;
		$scope.fSolarToBatt = 0;
		$scope.fActualBattWatt = 0;
		$scope.fBattSoc = 0;
		$scope.txtDayGasUsage = '-';
		$scope.txtDayWaterUsage = '-';
		$scope.txtDayNetDeliv = "-";
		$scope.txtObjText = "";
		$scope.ServerTime = "";
		$scope.ServerTimeRaw = 0;
		$scope.SunRise = "";
		$scope.SunSet = "";
		$scope.p1Price = 1000;
		$scope.gasPrice = 1000;
		$scope.waterPrice = 1000;
		$scope.h1Price = 1000;
		$scope.h2Price = 1000;
		$scope.h3Price = 1000;
		$scope.txtItemH1 = "";
		$scope.txtItemH2 = "";
		$scope.txtItemH3 = "";
		$scope.txtKwhItemH1 = "";
		$scope.txtKwhItemH2 = "";
		$scope.txtKwhItemH3 = "";
		
		$scope.customIconGrid = "";
		$scope.customIconWater = "";
		$scope.customIconGas = "";
		$scope.customIconSolar = "";
		$scope.customIconBatt = "";
		$scope.customIconH1 = "";
		$scope.customIconH2 = "";
		$scope.customIconH3 = "";
		$scope.lastClockSet = 0;
		$scope.SolarToHomeflowAnim = "";
		$scope.SolarToGridflowAnim = "";
		$scope.SolarToBattflowAnim = "";
		$scope.Grid2HomeFlowAnim = "";
		$scope.BattToGridflowAnim="";
		$scope.BattToHomeflowAnim="";
		
		$scope.flowStrokeBack = ($scope.flowAsLines) ? 0.6 : 0.2;
		
		$scope.iconCar = "m6.5216 2.8645-.2976-.2765-.264-.528a.252.252 90 00-.216-.12h-1.488a.252.252 90 00-.216.12l-.264.528-.2976.2765a.12.12 90 00-.0384.0881v1.0274a.12.12 90 00.12.12h.48c.048 0 .12-.048.12-.096v-.144h1.68v.12c0 .048.048.12.096.12h.504a.12.12 90 00.12-.12v-1.0274a.12.12 90 00-.0384-.0881zm-2.2416-.6845h1.44l.24.48h-1.92zm.12 1.104c0 .048-.072.096-.12.096h-.504c-.048 0-.096-.072-.096-.12v-.264c.024-.072.072-.12.144-.096l.48.096c.048 0 .096.072.096.12zm1.92-.024c0 .048-.048.12-.096.12h-.504c-.048 0-.12-.048-.12-.096v-.168c0-.048.048-.12.096-.12l.48-.096c.072-.024.12.024.144.096z";
		$scope.iconHeater = "M5.9529 4.6828c-.163 0-.2964-.1289-.3038-.29h-.0886c-.0074.1612-.1409.29-.3038.29s-.2964-.1289-.3038-.29h-.0886c-.0074.1612-.1409.29-.3038.29s-.2964-.1289-.3038-.29h-.0886c-.0074.1612-.1409.29-.3038.29C3.6964 4.6828 3.56 4.5464 3.56 4.3786V2.067C3.56 1.8993 3.6964 1.7629 3.8642 1.7629s.3042.1364.3042.3042v.0493h.088v-.0493c0-.1678.1364-.3042.3042-.3042s.3042.1364.3042.3042v.0493h.088v-.0493c0-.1678.1364-.3042.3042-.3042s.3042.1364.3042.3042v.0493h.088v-.0493c0-.1678.1364-.3042.3042-.3042s.3042.1364.3042.3042v.0499h.133c.0016 0 .003.0001.0044.0002.1169.0024.2112.0982.2112.2156 0 .1174-.0942.2131-.211.2156-.0016.0001-.0031.0002-.0047.0002h-.133v1.4123h.133c.0016 0 .0031.0001.0047.0002.1168.0025.211.0983.211.2156s-.0942.2131-.211.2156c-.0016.0001-.0031.0002-.0047.0002h-.1332C6.2493 4.554 6.1159 4.6828 5.9529 4.6828zM5.7387 4.3478v.031c0 .1181.0961.2142.2142.2142s.2142-.0961.2142-.2142V2.067c0-.1181-.0961-.2142-.2142-.2142s-.2142.0961-.2142.2142V4.3478zM5.0425 4.3478v.031c0 .1181.0961.2142.2142.2142s.2142-.0961.2142-.2142V2.067c0-.1181-.0961-.2142-.2142-.2142s-.2142.0961-.2142.2142V4.3478zM4.3462 4.3478v.031c0 .1181.0961.2142.2142.2142s.2142-.0961.2142-.2142V2.067c0-.1181-.0961-.2142-.2142-.2142S4.3462 1.9489 4.3462 2.067V4.3478zM3.8642 1.8528C3.7461 1.8528 3.65 1.9489 3.65 2.067v2.3117c0 .1181.0961.2142.2142.2142s.2142-.0961.2142-.2142V2.067C4.0783 1.9489 3.9823 1.8528 3.8642 1.8528zM6.257 4.3028h.1288c.0014-.0001.0028-.0002.0042-.0002.0694 0 .1256-.0564.1256-.1256s-.0564-.1256-.1256-.1256c-.0014 0-.0028-.0001-.0042-.0002h-.1288V4.3028zM5.5608 4.3028h.088v-.2518h-.088V4.3028zM4.8645 4.3028h.088v-.2518h-.088V4.3028zM4.1683 4.3028h.088v-.2518h-.088V4.3028zM5.5608 3.961h.088v-1.413h-.088V3.961zM4.8645 3.961h.088v-1.413h-.088V3.961zM4.1683 3.961h.088v-1.413h-.088V3.961zM6.257 2.4586h.1288c.0014-.0001.0028-.0002.0042-.0002.0694 0 .1256-.0564.1256-.1256s-.0564-.1256-.1256-.1256c-.0013 0-.0026 0-.004-.0001h-.1289V2.4586zM5.5608 2.458h.088V2.2063h-.088V2.458zM4.8645 2.458h.088V2.2063h-.088V2.458zM4.1683 2.458h.088V2.2063h-.088V2.458z";
		$scope.iconPower = "M5.779 1.3028C5.7665 1.2766 5.742 1.26 5.7152 1.26H4.6625c-.0319 0-.0599.0232-.069.0567l-.376 1.3861c-.0065.0239-.0023.0498.0113.0698.0136.0201.035.0318.0577.0318h.5902l-.2378 1.3106c-.0069.0381.0123.0762.0455.0898.0083.0034.0167.005.0252.005.0253 0 .0495-.0147.0626-.04l.9273-1.7912c.0127-.0245.0126-.0548-.0004-.0792-.0129-.0244-.0366-.0396-.0623-.0396h-.4175l.5549-.8753C5.7896 1.3604 5.7914 1.3288 5.779 1.3028z";
		$scope.iconOther = "M6.0944 4.6434H5.2496c-.0934 0-.169-.0756-.169-.169V3.6296c0-.0934.0756-.169.169-.169h.8448c.0934 0 .169.0756.169.169v.8448c0 .0934-.0756.169-.169.169zm0-1.0138H5.2496v.8448h.8448V3.6296zm0-.5069H5.2496c-.0934 0-.169-.0756-.169-.169V2.109c0-.0934.0756-.169.169-.169h.8448c.0934 0 .169.0756.169.169v.8448c0 .0934-.0756.169-.169.169zm0-1.0138H5.2496v.8448h.8448V2.109zm-1.5206 2.5344H3.729c-.0934 0-.169-.0756-.169-.169V3.6296c0-.0934.0756-.169.169-.169h.8448c.0934 0 .169.0756.169.169v.8448c0 .0934-.0756.169-.169.169zm0-1.0138H3.729v.8448h.8448V3.6296zm0-.5069H3.729c-.0934 0-.169-.0756-.169-.169V2.109C3.56 2.0156 3.6356 1.94 3.729 1.94h.8448c.0934 0 .169.0756.169.169v.8448c0 .0934-.0756.169-.169.169zm0-1.0138H3.729v.8448h.8448V2.109z";
		$scope.iconWave = "M3.8964 2.4535c.2621-.225.469-.4029 1.0308-.0855.2698.1524.4875.21.6685.2097.3173 0 .5229-.1764.7006-.329a.1555.1555 90 00.0183-.2159.1481.1481 90 00-.2115-.0187c-.2619.2253-.469.4032-1.0308.0855-.7422-.4189-1.0899-.1205-1.3692.1196a.1555.1555 90 00-.0181.2159.1479.1479 90 00.2114.0184zm2.2068.3267c-.2619.225-.469.4032-1.0308.0855-.7422-.4192-1.0899-.1206-1.3692.1192a.1555.1555 90 00-.0181.2159.1479.1479 90 00.2114.0187c.2621-.2251.469-.403 1.0308-.0858.2698.1527.4875.21.6685.21.3173 0 .5229-.1764.7006-.3292a.1552.1552 90 00.0183-.2157.1479.1479 90 00-.2115-.0186zm0 .7659c-.2619.2253-.469.4032-1.0308.0858-.7422-.4192-1.0899-.1207-1.3692.1192a.1555.1555 90 00-.0181.2159.1477.1477 90 00.2114.0184c.2621-.225.469-.4027 1.0308-.0855.2698.1524.4875.21.6685.21.3173 0 .5229-.1767.7006-.3292a.1555.1555 90 00.0183-.2159.1482.1482 90 00-.2115-.0187z";

		$scope.calculateViewport = function() {
			//Viewport settings/calculations
			let bSmallHeight = (($scope.idBattWatt == -1) || (($scope.idGas == -1)&&($scope.idWater != -1))) && (!(($scope.idGas != -1)&&($scope.idWater != -1)));
			if ($scope.idItemH3 != -1) {
				bSmallHeight = false;
			}
			let dash_height = (bSmallHeight && ($scope.idBattWatt == -1)) ? 40 : ($scope.idBattWatt != -1) ? 60 : 55;
			let dash_width = (($scope.idItemH1!=-1)||($scope.idItemH2!=-1)||($scope.idItemH3!=-1)) ? 71.5 : 60;

			$scope.viewBox = "0 0 " + dash_width + " " + dash_height;

			$scope.bDisplayRT1 = ($scope.idGas != -1) || (($scope.idGas == -1)&&($scope.idWater != -1));
			$scope.bDisplayRB1 = ($scope.idWater != -1) && (!(($scope.idGas == -1)&&($scope.idWater != -1)));
			
			let water_y_offset = (($scope.idGas == -1)&&($scope.idWater != -1)) ? -32 : 0;
			$scope.water_transform = "matrix(1, 0, 0, 1, 0, " + water_y_offset + ")";
		}

		$scope.GetInitialDevices = function() {
			let devArray = [];
			if ($scope.idP1 != -1) devArray.push($scope.idP1);
			if ($scope.idGas != -1) devArray.push($scope.idGas);
			if ($scope.idWater != -1) devArray.push($scope.idWater);
			if ($scope.idSolar != -1) devArray.push($scope.idSolar);
			if ($scope.idBattWatt != -1) devArray.push($scope.idBattWatt);
			if ($scope.idBattSoc != -1) devArray.push($scope.idBattSoc);
			if ($scope.idTextObj != -1) devArray.push($scope.idTextObj);
			if ($scope.idItemH1 != -1) devArray.push($scope.idItemH1);
			if ($scope.idItemH2 != -1) devArray.push($scope.idItemH2);
			if ($scope.idItemH3 != -1) devArray.push($scope.idItemH3);

			if (devArray.length > 0) {
				livesocket.getJson("json.htm?type=command&param=getdevices&rid=" + devArray.toString(), function (data) {
					if (typeof data.ServerTime != 'undefined') {
						$scope.SetTime(data);
					}

					if (typeof data.result != 'undefined') {

						if (typeof data.ActTime != 'undefined') {
							$.LastUpdateTime = parseInt(data.ActTime);
						}
						$.each(data.result, function (i, item) {
							$scope.RefreshItem(item);
						});
					}
				});
			} else {
				//Not configured
				$scope.txtObjText=$.t("Please configure the Energy Dashboard\nin the application Settings!");
				$scope.makeTextLines();
			}
			$scope.mytimer = $interval(function () {
								$scope.UpdateClockTick();
							}, 1000);
		}

		$scope.GetEnergyDashboardDevices = function() {
			$http({
				url: "json.htm?type=command&param=getenergydashboarddevices",
				async: true,
				dataType: 'json'
			}).then(function successCallback(response) {
				var data = response.data;
				if ((typeof data.status != 'undefined') && (typeof data.result != 'undefined') && (typeof data.result.ESettings != 'undefined')) {
					$scope.idP1 = data.result.ESettings.idP1;
					$scope.idGas = data.result.ESettings.idGas;
					$scope.idWater = data.result.ESettings.idWater;
					$scope.idSolar = data.result.ESettings.idSolar;
					$scope.idBattWatt = data.result.ESettings.idBatteryWatt;
					$scope.idBattSoc = data.result.ESettings.idBatterySoc;
					$scope.idTextObj = data.result.ESettings.idTextSensor;
					$scope.idItemH1 = data.result.ESettings.idExtra1;
					$scope.fieldH1 = data.result.ESettings.Extra1Field;
					$scope.iconH1 = data.result.ESettings.Extra1Icon;
					$scope.idItemH2 = data.result.ESettings.idExtra2;
					$scope.fieldH2 = data.result.ESettings.Extra2Field;
					$scope.iconH2 = data.result.ESettings.Extra2Icon;
					$scope.idItemH3 = data.result.ESettings.idExtra3;
					$scope.fieldH3 = data.result.ESettings.Extra3Field;
					$scope.iconH3 = data.result.ESettings.Extra3Icon;
					
					$scope.convertWaterM3ToLiter = data.result.ESettings.ConvertWaterM3ToLiter;
					$scope.bEnableServerTime = data.result.ESettings.DisplayTime;
					if (data.result.ESettings.DisplayFlowWithLines != 'undefined') {
						$scope.flowAsLines = (data.result.ESettings.DisplayFlowWithLines == true);
						$scope.flowStrokeBack = ($scope.flowAsLines) ? 0.6 : 0.2;
					}
					if (data.result.ESettings.UseCustomIcons != 'undefined') {
						$scope.useCustomIcons = (data.result.ESettings.UseCustomIcons == true);
					}
					
					$scope.iconItemH1 = $scope.assignIcon($scope.iconH1);
					$scope.iconItemH2 = $scope.assignIcon($scope.iconH2);
					$scope.iconItemH3 = $scope.assignIcon($scope.iconH3);
				}
				$scope.GetInitialDevices();
				$scope.calculateViewport();
				$scope.UpdateScreen();
			});
		}
		
		$scope.assignIcon = function(icon_name) {
			switch (icon_name)
			{
				case "car":
					return $scope.iconCar;
				case "heater":
					return $scope.iconHeater;
				case "power":
					return $scope.iconPower;
				case "wave":
					return $scope.iconWave;
				case "other":
					return $scope.iconOther;
			}
			return $scope.iconCar;
		}
		
		$scope.RefreshItem = function(item) {
			let idx = parseInt(item.idx);
			let bHandledData = false;
			switch (idx)
			{
				case $scope.idP1:
					bHandledData = $scope.handlePower(item);
					break;
				case $scope.idGas:
					bHandledData = $scope.handleGas(item);
					break;
				case $scope.idWater:
					bHandledData = $scope.handleWater(item);
					break;
				case $scope.idSolar:
					bHandledData = $scope.handleSolar(item);
					break;
				case $scope.idBattWatt:
					bHandledData = $scope.handleBattSetpoint(item);
					break;
				case $scope.idBattSoc:
					bHandledData = $scope.handleBattSoc(item);
					break;
				case $scope.idTextObj:
					bHandledData = $scope.handleTextObj(item);
					break;
				case $scope.idItemH1:
					bHandledData = $scope.handleItemH1(item);
					break;
				case $scope.idItemH2:
					bHandledData = $scope.handleItemH2(item);
					break;
				case $scope.idItemH3:
					bHandledData = $scope.handleItemH3(item);
					break;
			}
			
			if (bHandledData == true) {
				$scope.UpdateScreen();
			}
		}
		
		$scope.GetIconForItem = function(item) {
			if ($scope.useCustomIcons == false) {
				return "";
			}
			let ficon = "";
			if (item.CustomImage != 0) {
				if ((item.TypeImg == "lightbulb") || (item.TypeImg == "dimmer")) {
					ficon = "images/" + item.Image + "48_";
					if ((item.Data == "On"||(item.Data.search("%") != -1))) {
						ficon += "On";
					} else if (item.Data == "Off") {
						ficon += "Off";
					} else {
						ficon += "On";
					}
					ficon += ".png";
				} else {
					ficon = "images/" + item.Image + "48_On.png";
				}
				return ficon;
			} else {
				if (item.TypeImg == "counter") {
					ficon = "Counter48";
				}
				else if (item.TypeImg == "gas") {
					ficon = "Gas48";
				}
				else if (item.TypeImg == "water") {
					ficon = "Water48_On";
				}
				else if (item.TypeImg == "current") {
					ficon = "current48";
				}
				else if (item.TypeImg == "pv") {
					ficon = "PV48";
				}
				else if (item.TypeImg == "override_mini") {
					ficon = "override";
				}
				else if (item.SubType == "Percentage") {
					ficon = "Percentage48";
				}
				else if (item.TypeImg == "Custom") {
					ficon = "Custom48_On";
				}
				else if (item.TypeImg == "lux") {
					ficon = "lux48";
				}
				else if (item.TypeImg == "text") {
					ficon = "text48";
				}
				else if (item.TypeImg == "gauge") {
					ficon = "gauge48";
				}
				else if (item.TypeImg == "Alert") {
					ficon = "Alert48_1";
				}
				else if (item.TypeImg == "temperature") {
					return "images/" + GetTemp48Item(item.Temp);
				}
				else if (item.TypeImg == "wind") {
					if (typeof item.Direction != 'undefined') {
						ficon = 'Wind' + item.DirectionStr;
					} else {
						ficon = "wind";
					}
				}
				else if ((item.TypeImg == "lightbulb")||(item.TypeImg == "dimmer")) {
					if (item.Data == "On") {
						ficon = "Light48_On";
					} else if (item.Data == "Off") {
						ficon = "Light48_Off";
					} else if (item.Data.search("%") != -1) {
						return "images/Dimmer48_On.png";
					} else {
						ficon = "Light48_On";
					}
				}
				if (ficon == "")
					return "";
				return "images/" + ficon + ".png";
			}
		}
		
		$scope.handlePower = function(item) {
			if (!item.hasOwnProperty("CounterToday")) {
				console.log("Error with Power meter results. Check ID!");
				return false;
			}
			if (item.Type != "P1 Smart Meter") {
				$scope.P1InkWh = true;
				console.log(item);
			}
			
			let fActualNetDeliv = 0;
			
			$scope.fDayNetUsage = parseFloat(item["CounterToday"].replace(' kWh',''));
			let fActualNetUsage = parseFloat(item["Usage"].replace(' Watt',''));
			if ($scope.P1InkWh == false) {
				$scope.fDayNetDeliv = parseFloat(item["CounterDelivToday"].replace(' kWh',''));
				fActualNetDeliv = parseFloat(item["UsageDeliv"].replace(' Watt',''));
			} else {
				$scope.fDayNetDeliv = 0;
			}
			$scope.fActualNet = Math.round(fActualNetUsage - fActualNetDeliv);
			if (item.hasOwnProperty("price")) {
				$scope.p1Price = parseFloat(item["price"]);
			}
			$scope.customIconGrid = $scope.GetIconForItem(item);
			return true;
		}
		
		$scope.handleGas = function(item) {
			if (item.hasOwnProperty("CounterToday")) {
				$scope.txtDayGasUsage = item["CounterToday"];
			} else {
				$scope.txtDayGasUsage = item["Data"];
			}
			if (item.hasOwnProperty("price")) {
				$scope.gasPrice = parseFloat(item["price"]);
			}
			item.TypeImg = "gas";
			$scope.customIconGas = $scope.GetIconForItem(item);
			return true;
		}

		$scope.handleWater = function(item) {
			if (item.hasOwnProperty("CounterToday")) {
				let bWaterMeterInM3 = item["CounterToday"].search("m3") != -1;
				let fDayWaterUsage = parseFloat(item["CounterToday"].replace(' m3','').replace(' Liter',''));
				if ((bWaterMeterInM3) && ($scope.convertWaterM3ToLiter==true)) {
					fDayWaterUsage*=1000;
				}
				$scope.txtDayWaterUsage = fDayWaterUsage;
				if ((bWaterMeterInM3==false)||($scope.convertWaterM3ToLiter==true)) {
					$scope.txtDayWaterUsage+= ' L';
				} else {
					 $scope.txtDayWaterUsage+= ' m3';
				}
			} else {
				$scope.txtDayWaterUsage = item["Data"];
			}
			if (item.hasOwnProperty("price")) {
				$scope.waterPrice = parseFloat(item["price"]);
			}
			item.TypeImg = "water";
			$scope.customIconWater = $scope.GetIconForItem(item);
			return true;
		}

		$scope.handleSolar = function(item) {
			if (!item.hasOwnProperty("CounterToday")) {
				console.log("Error with Power meter results. Check ID!");
				return false;
			}
			$scope.fDaySolar = parseFloat(item["CounterToday"].replace(' kWh',''));
			$scope.fActualSolar = Math.round(Math.abs(parseFloat(item["Usage"].replace(' Watt',''))));
			item.TypeImg = "pv";
			$scope.customIconSolar = $scope.GetIconForItem(item);
			return true;
		}

		$scope.handleBattSetpoint = function(item) {
			let data = 0;
			if (item["Data"].search("kWh") == -1) {
				data = parseFloat(item["Data"]);
			} else {
				data = parseFloat(item["Usage"].replace(' Watt',''));
			}
			$scope.fActualBattWatt = Math.round(data);
			if ($scope.idBattSoc == -1) {
				$scope.customIconBatt = $scope.GetIconForItem(item);
			}
			return true;
		}
		
		$scope.handleBattSoc = function(item) {
			$scope.fBattSoc = parseFloat(item["Data"].replace('%',''));
			$scope.customIconBatt = $scope.GetIconForItem(item);
			return true;
		}

		$scope.handleTextObj = function(item) {
			$scope.txtObjText = item["Data"];
			return true;
		}

		$scope.handleItemH1 = function(item) {
			$scope.txtItemH1 = item[$scope.fieldH1];
			if (item.hasOwnProperty("price")) {
				$scope.h1Price = parseFloat(item["price"]);
			}
			if ((item.hasOwnProperty("CounterToday"))&&($scope.fieldH1!="CounterToday")) {
				if ((item["CounterToday"].search("kWh") != -1) || (item["CounterToday"].search("m3") != -1)) {
					$scope.txtKwhItemH1 = item["CounterToday"];
				}
			}
			$scope.customIconH1 = $scope.GetIconForItem(item);
			return true;
		}
		$scope.handleItemH2 = function(item) {
			$scope.txtItemH2 = item[$scope.fieldH2];
			if (item.hasOwnProperty("price")) {
				$scope.h2Price = parseFloat(item["price"]);
			}
			if ((item.hasOwnProperty("CounterToday"))&&($scope.fieldH2!="CounterToday")) {
				if ((item["CounterToday"].search("kWh") != -1) || (item["CounterToday"].search("m3") != -1)) {
					$scope.txtKwhItemH2 = item["CounterToday"];
				}
			}
			$scope.customIconH2 = $scope.GetIconForItem(item);
			return true;
		}
		$scope.handleItemH3 = function(item) {
			$scope.txtItemH3 = item[$scope.fieldH3];
			if (item.hasOwnProperty("price")) {
				$scope.h3Price = parseFloat(item["price"]);
			}
			if ((item.hasOwnProperty("CounterToday"))&&($scope.fieldH3!="CounterToday")) {
				if ((item["CounterToday"].search("kWh") != -1) || (item["CounterToday"].search("m3") != -1)) {
					$scope.txtKwhItemH3 = item["CounterToday"];
				}
			}
			$scope.customIconH3 = $scope.GetIconForItem(item);
			return true;
		}
		
		$scope.SetTime = function(data) {
			let atime = Math.floor(new Date().getTime()/1000);
			if (atime - $scope.lastClockSet < 600) {
				return;
			}
			$scope.lastClockSet = atime;
			if ($scope.bEnableServerTime == false) {
				return;
			}
			if (data.hasOwnProperty("ServerTime")) {
				$scope.ServerTimeRaw = data.ActTime;
				$scope.SunRise = data.Sunrise;
				$scope.SunSet = data.Sunset;
				$scope.UpdateTime();
			} else {
				$scope.ServerTimeRaw = Date.parse(data.serverTime) / 1000;
				$scope.UpdateTime();
				$scope.SunRise = data.sunrise;
				$scope.SunSet = data.sunset;
				$scope.$apply();
			}
		}
		
		$scope.UpdateTime = function() {
			let date = new Date($scope.ServerTimeRaw * 1000);
			let szTime = date.toTimeString().substring(0,8);
			$scope.ServerTime = szTime;
		}

		$scope.UpdateClockTick = function() {
			if ($scope.bEnableServerTime == false) {
				return;
			}
			if ($scope.ServerTimeRaw == 0) {
				return;
			}
			$scope.ServerTimeRaw++;
			$scope.UpdateTime();
		}
		
		$scope.GetAnimDuration = function(fPower) {
			let aPower = Math.abs(fPower);
			let fSec = 5;
			if (aPower < 250)
				fSec = 5;
			else if (aPower < 500)
				fSec = 4.5;
			else if (aPower < 1000)
				fSec = 4;
			else if (aPower < 2000)
				fSec = 3;
			else if (aPower < 3000)
				fSec = 2;
			else
				fSec = 1.5;
			return fSec;
		}

		$scope.GetAnim = function(fPower, isReverse = false) {
			let fSec = $scope.GetAnimDuration(fPower);
			let ret = 'flow '+fSec.toFixed(1)+'s linear infinite';
			if (isReverse == true) {
				ret += " reverse";
			}
			return ret;
		}
		
		$scope.SetEclipseAnim = function(item, fPower, isReverse = false) {
			let fSec = $scope.GetAnimDuration(fPower);
			let dElement=document.getElementById(item);
			dElement.setAttribute('dur',fSec);
			if (isReverse == false) {
				dElement.setAttribute('keyPoints','0;1');
			} else {
				dElement.setAttribute('keyPoints','1;0');
			}
		}

		$scope.makeTextLines = function() {
			let tspans = "";
			let lines = $scope.txtObjText.split("\n");
			for (i = 0; i < lines.length; i++) {
				let ntline = '<tspan x="1" dy="1.2em">' + lines[i] + '</tspan>\n';
				tspans += ntline;
			}
			let ltext = document.getElementById('ltext');
			ltext.innerHTML = tspans;
		}

		$scope.UpdateScreen = function () {
			//for debugging purposes
			//$scope.fActualNet = 545;
			//$scope.fActualSolar = 4146;
			//$scope.fActualBattWatt = -2294;
			//$scope.fBattSoc = 62.4;
			//$scope.txtDayGasUsage = '0.417 m3';
			//$scope.txtDayWaterUsage = '294 L';
			
			// Total Home usage: Calculated
			$scope.fTotalHomeUsage = $scope.fDayNetUsage + $scope.fDaySolar - $scope.fDayNetDeliv;

			let fActualNet = $scope.fActualNet;
			let fActualSolar = $scope.fActualSolar;
			let fActualBattWatt = $scope.fActualBattWatt;

			let fSolarToGrid = 0;
			let fSolarToBatt = 0;
			let fSolarToHome = 0;
			let fBattToNet = 0;
			let fBattToHome = 0;
			let fGridToHome = 0;
			let fActualHomeUsage = 0;

			//handle Battery
			if (fActualBattWatt > 0) {
				//Charging
				
				//First check if we are using some charge power from our solar panel
				fSolarToBatt = Math.min(fActualSolar, fActualBattWatt);
				fActualSolar -= fSolarToBatt;
				fActualBattWatt -= fSolarToBatt;
				
			}

			if (fActualNet < 0) {
				//we are returning to the grid
				
				//First check if we are using some return power from our solar panel
				fSolarToGrid = Math.min(fActualSolar, Math.abs(fActualNet));
				fActualSolar -= fSolarToGrid;
				fActualNet += fSolarToGrid;
			}
			//Remaining Solart power goes to home
			fSolarToHome += fActualSolar;
			fActualHomeUsage += fSolarToHome;
			
			if (fActualBattWatt < 0) {
				//Discharging
				if (fActualNet < 0) {
					let total_return = Math.abs(fActualBattWatt) + fActualSolar;
					let house_usage = total_return - Math.abs(fActualNet);
					if (house_usage<0) {
						//That's not possible, not enough power to return to the grid
						//The reason is likely that the Solar Wattage is not accurate
						//Or the battery is fully discharged
						house_usage = 0;
					}
					fBattToHome = house_usage;
					fActualBattWatt += house_usage;
					fBattToNet = Math.abs(fActualBattWatt);
					fActualNet -= fActualBattWatt;
				} else {
					fBattToHome = Math.abs(fActualBattWatt);
				}
				fActualHomeUsage += fBattToHome;
			} else if (fActualBattWatt > 0) {
				//We need power from the grid to charge the battery
				if (fActualNet > 0) {
					fBattToNet = -fActualBattWatt;
					fActualNet -= Math.abs(fBattToNet);
				} else {
					//We have a problem!!! Not enough power to charge the battery!!
					//The reason is likely that the Solar Wattage is not accurate
					//Or that the battery is fully charged and not taking any power
					if ($scope.fBattSoc==100) {
						fActualBattWatt = 0;
						fSolarToHome += fSolarToBatt;
						fActualHomeUsage += fSolarToBatt;
						fSolarToBatt = 0;
					} else {
						//Add it to the Solar
						$scope.fActualSolar += Math.abs(fActualBattWatt);
						fSolarToBatt += Math.abs(fActualBattWatt);
						fActualSolar = $scope.fActualSolar;
						fActualBattWatt = 0;
					}
				}
			}
			if ((fActualNet< 0) && ($scope.idSolar!=-1)) {
				//It seems we return more Energy then is possible
				//The reason is likely that the Solar Wattage is not accurate
				//Add it to the Solar
				$scope.fActualSolar += Math.abs(fActualNet);
				fSolarToGrid += Math.abs(fActualNet);
				fActualSolar = $scope.fActualSolar;
				fActualNet = 0;
			}
			fGridToHome += fActualNet;
			fActualHomeUsage += fActualNet;
			
			//--------------------------------
			//Plot our calculated values
			$scope.fSolarToHome = fSolarToHome;
			$scope.fSolarToGrid = fSolarToGrid;
			$scope.fSolarToBatt = fSolarToBatt;
			$scope.fBattToNet = fBattToNet;
			$scope.fBattToHome = fBattToHome;
			$scope.fGridToHome = fGridToHome;
			$scope.fActualHomeUsage = fActualHomeUsage;
			if ($scope.fSolarToHome>=0) {
				let anim = $scope.GetAnim($scope.fSolarToHome);
				if ($scope.SolarToHomeflowAnim != anim) {
					$scope.SolarToHomeflowAnim = anim;
					if ($scope.flowAsLines == true) {
						let SolarToHomeflow=document.getElementById('SolarToHome-flow');
						SolarToHomeflow.style.animation = anim;
					} else {
						$scope.SetEclipseAnim("SolarToHome-sphere", $scope.fSolarToHome);
					}
				}
			}

			if ($scope.fSolarToBatt > 0) {
				let anim = $scope.GetAnim($scope.fSolarToBatt);
				if ($scope.SolarToBattflowAnim != anim) {
					$scope.SolarToBattflowAnim = anim;
					if ($scope.flowAsLines == true) {
						let SolarToBattflow=document.getElementById('SolarToBatt-flow');
						SolarToBattflow.style.animation = anim;
					} else {
						$scope.SetEclipseAnim("SolarToBatt-sphere", $scope.fSolarToBatt);
					}
				}
			}

			if ($scope.fSolarToGrid>=0) {
				let anim = $scope.GetAnim($scope.fSolarToGrid);
				if ($scope.SolarToGridflowAnim != anim) {
					$scope.SolarToGridflowAnim = anim;
					if ($scope.flowAsLines == true) {
						let SolarToGridflow=document.getElementById('SolarToGrid-flow');
						SolarToGridflow.style.animation = anim;	
					} else {
						$scope.SetEclipseAnim("SolarToGrid-sphere", $scope.fSolarToGrid);
					}
				}
			}

			if ($scope.fBattToNet!=0) {
				let anim = $scope.GetAnim($scope.fBattToNet, ($scope.fBattToNet<0));
				if ($scope.BattToGridflowAnim != anim) {
					$scope.BattToGridflowAnim = anim;
					if ($scope.flowAsLines == true) {
						let BattNetflow=document.getElementById('BattNet-flow');
						BattNetflow.style.animation = anim;
					} else {
						$scope.SetEclipseAnim("BattNet-sphere", $scope.fBattToNet, ($scope.fBattToNet<0));
					}
				}
			}

			if ($scope.fBattToHome >= 0) {
				let anim = $scope.GetAnim($scope.fBattToHome);
				if ($scope.BattToHomeflowAnim != anim) {
					$scope.BattToHomeflowAnim = anim;
					if ($scope.flowAsLines == true) {
						let BattHomeflow=document.getElementById('BattHome-flow');
						BattHomeflow.style.animation = anim;
					} else {
						$scope.SetEclipseAnim("BattHome-sphere", $scope.fBattToHome);
					}
				}
			}

			if ($scope.fGridToHome>=0) {
				let anim = $scope.GetAnim($scope.fGridToHome, ($scope.fGridToHome<0));
				if ($scope.Grid2HomeFlowAnim != anim) {
					$scope.Grid2HomeFlowAnim = anim;
					if ($scope.flowAsLines == true) {
						let Grid2HomeFlow = document.getElementById('GridToHome-flow');
						Grid2HomeFlow.style.animation = anim;
					} else {
						$scope.SetEclipseAnim("GridToHome-sphere", $scope.fGridToHome, ($scope.fGridToHome<0));
					}
				}
			}

			$scope.finalHomeUsage = $scope.fActualHomeUsage;
			
			if ($scope.idTextObj != -1) {
				$scope.makeTextLines();
			}
		}
		
		$scope.$on('device_update', function (event, deviceData) {
			$scope.RefreshItem(deviceData);
		});
		$scope.$on('time_update', function (event, data) {
			$scope.SetTime(data);
		});
		
		$scope.init = function () {
			$scope.calculateViewport();
			$scope.GetEnergyDashboardDevices();
			//$scope.RefreshUptime();
		};

		$("#dashcontent").i18n();

		$scope.init();

		$scope.$on('$destroy', function () {
			if (typeof $scope.mytimer != 'undefined') {
				$interval.cancel($scope.mytimer);
				$scope.mytimer = undefined;
			}
		});
	}]);
});
