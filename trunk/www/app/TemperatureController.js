define(['app'], function (app) {
	app.controller('TemperatureController', [ '$scope', '$rootScope', '$location', '$http', '$interval', 'permissions', function($scope,$rootScope,$location,$http,$interval,permissions) {

		MakeFavorite = function(id,isfavorite)
		{
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
			 success: function(data) {
			  ShowTemps();
			 }
		  });
		}
			
		EditTempDevice = function(idx,name,addjvalue)
		{
			if (typeof $scope.mytimer != 'undefined') {
				$interval.cancel($scope.mytimer);
				$scope.mytimer = undefined;
			}
			$.devIdx=idx;
			$("#dialog-edittempdevice #devicename").val(decodeURIComponent(name));
			$("#dialog-edittempdevice #adjustment").val(addjvalue);
			$("#dialog-edittempdevice #tempcf").html($scope.config.TempSign);
			$("#dialog-edittempdevice" ).i18n();
			$("#dialog-edittempdevice" ).dialog( "open" );
		}

		EditTempDeviceSmall = function(idx,name,addjvalue)
		{
			if (typeof $scope.mytimer != 'undefined') {
				$interval.cancel($scope.mytimer);
				$scope.mytimer = undefined;
			}
			$.devIdx=idx;
			$("#dialog-edittempdevicesmall #devicename").val(decodeURIComponent(name));
			$("#dialog-edittempdevicesmall" ).i18n();
			$("#dialog-edittempdevicesmall" ).dialog( "open" );
		}
		
		//evohome
		//FIXME some of this functionality would be good in a shared js / class library
		//as we might like to use it from the dashboard or in scenes at some point
		MakePerm = function(idt){
			$(idt).val('');return false;
		}
		
		EditSetPoint = function(idx,name,setpoint,mode,until,callback)
		{
			//HeatingOff does not apply to dhw
			if (mode=="HeatingOff"){
				bootbox.alert($.t('Can\'t change zone when the heating is off'));
				return false;
			}
			if (typeof $scope.mytimer != 'undefined') {
				$interval.cancel($scope.mytimer);
				$scope.mytimer = undefined;
			}
			$.devIdx=idx;
			$("#dialog-editsetpoint #devicename").val(decodeURIComponent(name));
			$("#dialog-editsetpoint #setpoint").val(setpoint);
			if(mode.indexOf("Override")==-1)
				$(":button:contains('Cancel Override')").attr("disabled","d‌​isabled").addClass( 'ui-state-disabled' );
			else
				$(":button:contains('Cancel Override')").removeAttr("disabled").removeClass( 'ui-state-disabled' );
			$("#dialog-editsetpoint #until").datetimepicker();
			if(until!="")
				$("#dialog-editsetpoint #until").datetimepicker("setDate", (new Date(until)));
			$("#dialog-editsetpoint" ).i18n();
			$("#dialog-editsetpoint" ).dialog( "open" );
		}
		EditState = function(idx,name,state,mode,until,callback)
		{
			//HeatingOff does not apply to dhw
			if (typeof $scope.mytimer != 'undefined') {
				$interval.cancel($scope.mytimer);
				$scope.mytimer = undefined;
			}
			$.devIdx=idx;
			$("#dialog-editstate #devicename").val(decodeURIComponent(name));
			$("#dialog-editstate #state").val(state);
			if(mode.indexOf("Override")==-1)
				$(":button:contains('Cancel Override')").attr("disabled","d‌​isabled").addClass( 'ui-state-disabled' );
			else
				$(":button:contains('Cancel Override')").removeAttr("disabled").removeClass( 'ui-state-disabled' );
			$("#dialog-editstate #until_state").datetimepicker();
			if(until!="")
				$("#dialog-editstate #until_state").datetimepicker("setDate", (new Date(until)));
			$("#dialog-editstate" ).i18n();
			$("#dialog-editstate" ).dialog( "open" );
		}
		EvoSetPointColor = function(item, sHeatMode, bkcolor){
			if (typeof item.SetPoint != 'undefined') {
				if(sHeatMode=="HeatingOff")//seems to be used whenever the heating is off
					bkcolor="#9b9b9b";
				else if(item.SetPoint>=25)
					bkcolor="#ff0302";
				else if(item.SetPoint>=22)
					bkcolor="#ff6a2a";
				else if(item.SetPoint>=19)
					bkcolor="#fe9b2d";
				else if(item.SetPoint>=16)
					bkcolor="#79bc5c";
				else //min on temp 5 or greater
					bkcolor="#6ca5fd";
			}
			return bkcolor;
		}
		//FIXME move this to a shared js ...see lightscontroller.js
		EvoDisplayTextMode = function(strstatus){
			if(strstatus=="Auto")//FIXME better way to convert?
				strstatus="Normal";
			else if(strstatus=="AutoWithEco")//FIXME better way to convert?
				strstatus="Economy";
			else if(strstatus=="DayOff")//FIXME better way to convert?
				strstatus="Day Off";
			else if(strstatus=="HeatingOff")//FIXME better way to convert?
				strstatus="Heating Off";
			return strstatus;
		}
		
		AddTempDevice = function()
		{
		  bootbox.alert($.t('Please use the devices tab for this.'));
		}


			
		AddMultipleDataToTempChart = function(data,chart,isday,deviceid,devicename)
		{
			var datatablete = [];
			var datatabletm = [];
			var datatablehu = [];
			var datatablech = [];
			var datatablecm = [];
			var datatabledp = [];
			var datatableba = [];
			var datatablese = [];
			var datatablesm = [];
			var datatablesx = [];

			$.each(data.result, function(i,item)
			{
			  if (isday==1) {
				if (typeof item.te != 'undefined') {
				  datatablete.push( [GetUTCFromString(item.d), parseFloat(item.te) ] );
				}
				if (typeof item.hu != 'undefined') {
				  datatablehu.push( [GetUTCFromString(item.d), parseFloat(item.hu) ] );
				}
				if (typeof item.ch != 'undefined') {
				  datatablech.push( [GetUTCFromString(item.d), parseFloat(item.ch) ] );
				}
				if (typeof item.dp != 'undefined') {
				  datatabledp.push( [GetUTCFromString(item.d), parseFloat(item.dp) ] );
				}
				if (typeof item.ba != 'undefined') {
				  datatableba.push( [GetUTCFromString(item.d), parseFloat(item.ba) ] );
				}
				if (typeof item.se != 'undefined') {
				  datatablese.push( [GetUTCFromString(item.d), parseFloat(item.se) ] );
				}
			  } else {
				if (typeof item.te != 'undefined') {
				  datatablete.push( [GetDateFromString(item.d), parseFloat(item.te) ] );
				  datatabletm.push( [GetDateFromString(item.d), parseFloat(item.tm) ] );
				}
				if (typeof item.hu != 'undefined') {
				  datatablehu.push( [GetDateFromString(item.d), parseFloat(item.hu) ] );
				}
				if (typeof item.ch != 'undefined') {
				  datatablech.push( [GetDateFromString(item.d), parseFloat(item.ch) ] );
				  datatablecm.push( [GetDateFromString(item.d), parseFloat(item.cm) ] );
				}
				if (typeof item.dp != 'undefined') {
				  datatabledp.push( [GetDateFromString(item.d), parseFloat(item.dp) ] );
				}
				if (typeof item.ba != 'undefined') {
				  datatableba.push( [GetDateFromString(item.d), parseFloat(item.ba) ] );
				}
				if (typeof item.se != 'undefined') {
				  datatablese.push( [GetDateFromString(item.d), parseFloat(item.se) ] );
				  datatablesm.push( [GetDateFromString(item.d), parseFloat(item.sm) ] );
				  datatablesx.push( [GetDateFromString(item.d), parseFloat(item.sx) ] );
				}
			  }
			});
			var series;

			if (datatablehu.length!=0)
			{
			  chart.addSeries(
				{
				  id: 'humidity'+deviceid,
				  name: devicename+':'+$.t('Humidity'),
				  yAxis: 1
				}
			  );
			  series = chart.get('humidity'+deviceid);
			  series.setData(datatablehu);
			}

			if (datatablech.length!=0)
			{
			  chart.addSeries(
				{
				  id: 'chill'+deviceid,
				  name: devicename+':'+$.t('Chill'),
				  yAxis: 0
				}
			  );
			  series = chart.get('chill'+deviceid);
			  series.setData(datatablech);
			  
			  if (isday==0) {
				chart.addSeries(
				  {
					id: 'chillmin'+deviceid,
					name: devicename+':'+$.t('Chill')+'_min',
					yAxis: 0
				  }
				);
				series = chart.get('chillmin'+deviceid);
				series.setData(datatablecm);
			  }
			}
			if (datatablete.length!=0)
			{
			  //Add Temperature series
			  chart.addSeries(
				{
				  id: 'temperature'+deviceid,
				  name: devicename+':'+$.t('Temperature'),
				  yAxis: 0
				}
			  );
			  series = chart.get('temperature'+deviceid);
			  series.setData(datatablete);
			  if (isday==0) {
				chart.addSeries(
				  {
					id: 'temperaturemin'+deviceid,
					name: devicename+':'+$.t('Temperature')+'_min',
					yAxis: 0
				  }
				);
				series = chart.get('temperaturemin'+deviceid);
				series.setData(datatabletm);
			  }
			}
			
			if (datatablese.length!=0)
			{
			  //Add Temperature series
			  chart.addSeries(
				{
				  id: 'setpoint'+deviceid,
				  name: devicename+':'+$.t('SetPoint'),
				  yAxis: 0
				}
			  );
			  series = chart.get('setpoint'+deviceid);
			  series.setData(datatablese);
			  if (isday==0) {
				chart.addSeries(
				  {
					id: 'setpointmin'+deviceid,
					name: devicename+':'+$.t('SetPoint')+'_min',
					yAxis: 0
				  }
				);
				series = chart.get('setpointmin'+deviceid);
				series.setData(datatablesm);
				
				chart.addSeries(
				  {
					id: 'setpointmax'+deviceid,
					name: devicename+':'+$.t('SetPoint')+'_max',
					yAxis: 0
				  }
				);
				series = chart.get('setpointmax'+deviceid);
				series.setData(datatablesx);
			  }
			}
			
			if (datatabledp.length!=0)
			{
			  chart.addSeries(
				{
				  id: 'dewpoint'+deviceid,
				  name: devicename+':'+$.t('Dew Point'),
				  yAxis: 0
				}
			  );
			  series = chart.get('dewpoint'+deviceid);
			  series.setData(datatabledp);
			}
			
			if (datatableba.length!=0)
			{
			  chart.addSeries(
				{
				  id: 'baro'+deviceid,
				  name: devicename+':'+$.t('Barometer'),
				  yAxis: 2
				}
			  );
			  series = chart.get('baro'+deviceid);
			  series.setData(datatableba);
			}

		}    

		RemoveMultipleDataFromTempChart = function(chart,deviceid)
		{
			hum=chart.get('humidity'+deviceid);
			if(hum!=null) {hum.remove()};
			chill=chart.get('chill'+deviceid)
			if(chill!=null) {chill.remove()};
			chillmin=chart.get('chillmin'+deviceid);
			if(chillmin!=null) {chillmin.remove()};
			temperature=chart.get('temperature'+deviceid);
			if(temperature!=null) {temperature.remove()};
			temperaturemin=chart.get('temperaturemin'+deviceid);
			if(temperaturemin!=null) {temperaturemin.remove()};
			dew=chart.get('dewpoint'+deviceid);
			if(dew!=null) {dew.remove()};
			baro=chart.get('baro'+deviceid);
			if(baro!=null) {baro.remove()};
		        setpoint=chart.get('setpoint'+deviceid);
			if(setpoint!=null) {setpoint.remove()};
			setpointmin=chart.get('setpointmin'+deviceid);
			if(setpointmin!=null) {setpointmin.remove()};
		        setpointmax=chart.get('setpointmax'+deviceid);
			if(setpointmax!=null) {setpointmax.remove()};
		}

		ClearCustomGraph = function()
		{
			$('div[id="devicecontainer"] input:checkbox:checked').each(function() {
				RemoveMultipleDataFromTempChart($.CustomChart.highcharts(),$(this).attr('id'));
				$(this).prop("checked", false);
			});   
		}
			
		SelectGraphDevices = function()
		{
			if (typeof $scope.mytimer != 'undefined') {
				$interval.cancel($scope.mytimer);
				$scope.mytimer = undefined;
			}
			
			$.ajax({
				   url: "json.htm?type=devices&filter=temp&used=true&order=Name",
				   async: false,
				   dataType: 'json',
				   success: function(data) {
					if (typeof data.result != 'undefined') {
						$.each(data.result, function(i,item){
							$("#tempcontent #devicecontainer").append('<input type="checkbox" class="devicecheckbox" id="'+item.idx+'" value="'+item.Name+'" onChange="AddDeviceToGraph(this)">'+item.Name+'<br />');
							});
					}
				   }
			});
		}

		AddDeviceToGraph = function(cb)
		{
			if (cb.checked==true) {
				$.ajax({
				   url: "json.htm?type=graph&sensor=temp&idx="+cb.id+"&range="+$("#tempcontent #graphfrom").val()+"T"+$("#tempcontent #graphto").val()+"&graphtype="+$("#tempcontent #combocustomgraphtype").val()+
				   "&graphTemp="+$("#tempcontent #graphTemp").prop("checked")+"&graphChill="+$("#tempcontent #graphChill").prop("checked")+"&graphHum="+$("#tempcontent #graphHum").prop("checked")+"&graphBaro="+$("#tempcontent #graphBaro").prop("checked")+"&graphDew="+$("#tempcontent #graphDew").prop("checked")+"&graphSet="+$("#tempcontent #graphSet").prop("checked"),
				   async: false,
				   dataType: 'json',
				   success: function(data) {
					AddMultipleDataToTempChart(data,$.CustomChart.highcharts(),$("#tempcontent #combocustomgraphtype").val(),cb.id,cb.value);
				   }
				});
			}
			else {
				RemoveMultipleDataFromTempChart($.CustomChart.highcharts(),cb.id);
			}
		}
		datePickerChanged = function(dpicker)
		{
			if ($("#graphfrom").val()!='') {
				$("#tempcontent #graphfrom").datepicker("setDate",$("#graphfrom").val());
			}
			else {
				$("#tempcontent #graphto").datepicker("setDate",$("#graphto").val());
			}
			$( "#tempcontent #graphfrom" ).datepicker('option', 'maxDate', $("#tempcontent #graphto").val());
			$( "#tempcontent #graphto" ).datepicker('option', 'minDate', $("#tempcontent #graphfrom").val());

			$('div[id="devicecontainer"] input:checkbox:checked').each(function() {
				RemoveMultipleDataFromTempChart($.CustomChart.highcharts(),$(this).attr('id'));
				$.ajax({
				   url: "json.htm?type=graph&sensor=temp&idx="+$(this).attr('id')+"&range="+$("#tempcontent #graphfrom").val()+"T"+$("#tempcontent #graphto").val()+"&graphtype="+$("#tempcontent #combocustomgraphtype").val()+
				   "&graphTemp="+$("#tempcontent #graphTemp").prop("checked")+"&graphChill="+$("#tempcontent #graphChill").prop("checked")+"&graphHum="+$("#tempcontent #graphHum").prop("checked")+"&graphBaro="+$("#tempcontent #graphBaro").prop("checked")+"&graphDew="+$("#tempcontent #graphDew").prop("checked")+"&graphSet="+$("#tempcontent #graphSet").prop("checked"),
				   async: false,
				   dataType: 'json',
				   graphid: $(this).attr('id'),
				   graphval: $(this).val(),
				   success: function(data) {
					AddMultipleDataToTempChart(data,$.CustomChart.highcharts(),$("#tempcontent #combocustomgraphtype").val(),this.graphid,this.graphval);
				   }
				});
			}); 
			return false;
		}
			
		ShowCustomTempLog = function()
		{
			if (typeof $scope.mytimer != 'undefined') {
				$interval.cancel($scope.mytimer);
				$scope.mytimer = undefined;
			}
		  $('#modal').show();
		  var htmlcontent = '';
		  htmlcontent+=$('#customlog').html();
		  $('#tempcontent').html(GetBackbuttonHTMLTable('ShowTemps')+htmlcontent);
		  $('#tempcontent').i18n();
		  
		  $.content="#tempcontent";

			$('.datepick').datepicker({
								   dateFormat: 'yy-mm-dd',
								   onClose:function() {
										datePickerChanged(this);
								   }
								  });
			$("#graphfrom").datepicker('setDate', '-7');
			$("#graphto").datepicker('setDate', '-0');
			$("#tempcontent #graphfrom").datepicker('setDate', '-7');
			$("#tempcontent #graphto").datepicker('setDate', '-0');
			
		  $.CustomChart = $($.content + ' #customgraph');
		  $.CustomChart.highcharts({
			  chart: {
				  type: 'line',
				  zoomType: 'x',
				  alignTicks: false,
				  
				  events: {
					  load: function() {
					  }
				  }
			  },
			  colors: ['#FF99CC','#FFCC99','#FFFF99','#CCFFCC','#CCFFFF','#99CCFF','#CC99FF','#FFFFFF',
					   '#9999FF','#993366','#FFFFCC','#CCFFFF','#660066','#FF8080','#0066CC','#CCCCFF',
					   '#000080','#FF00FF','#FFFF00','#00FFFF','#800080','#800000','#008080','#0000FF',
					   '#00C0C0','#993300','#333300','#003300','#003366','#000080','#333399','#333333',
					   '#800000','#FF6600','#808000','#008000','#008080','#0000FF','#666699','#808080',
					   '#FF0000','#FF9900','#99CC00','#339966','#33CCCC','#3366FF','#800080','#969696',
					   '#FF00FF','#FFCC00','#FFFF00','#00FF00','#00FFFF','#00CCFF','#993366','#C0C0C0'],
			  loading: {
				  hideDuration: 1000,
				  showDuration: 1000
			  },
			  credits: {
				enabled: true,
				href: "http://www.domoticz.com",
				text: "Domoticz.com"
			  },
			  title: {
				  text: 'Custom Temperature Graph'
			  },
			  xAxis: {
				  type: 'datetime'
			  },
			  yAxis: [{ //temp label
				  labels:  {
						   formatter: function() {
								return this.value +'\u00B0 ' + $scope.config.TempSign;
						   },
						   style: {
							  color: 'white'
						   }
				  },
				  title: {
					   text: 'degrees Celsius',
					   style: {
						  color: 'white'
					   }
				  },
				  showEmpty: false
			  }, { //humidity label
				  labels:  {
						   formatter: function() {
								return this.value +'%';
						   },
						   style: {
							  color: 'white'
						   }
				  },
				  title: {
					   text: $.t('Humidity') +' %',
					   style: {
						  color: 'white'
					   }
				  },
				  showEmpty: false
			  }, { //pressure label
				  labels:  {
						   formatter: function() {
								return this.value;
						   },
						   style: {
							  color: 'white'
						   }
				  },
				  title: {
					   text: $.t('Pressure') + ' (hPa)',
					   style: {
						  color: 'white'
					   }
				  },
				  showEmpty: false
			  }],
			  tooltip: {
				  formatter: function() {
						var unit = '';
						var baseName = this.series.name.split(':')[1];
						if (baseName==$.t("Humidity")) {unit = '%'} else {unit = '\u00B0 ' + $scope.config.TempSign};
						return $.t(Highcharts.dateFormat('%A',this.x)) + '<br/>' + Highcharts.dateFormat('%Y-%m-%d %H:%M', this.x) +'<br/>'+ this.series.name + ': ' + this.y + unit ;
				  }
			  },
			  legend: {
				  enabled: true
			  },
			  plotOptions: {
					   line: {
							lineWidth: 3,
							states: {
								hover: {
									lineWidth: 3
								}
							},
							marker: {
								enabled: false,
								states: {
									hover: {
										enabled: true,
										symbol: 'circle',
										radius: 5,
										lineWidth: 1
									}
								}
							}
						}
			  }
		  });
		  SelectGraphDevices();
		  $('#modal').hide();
		  return false;
		}

		RefreshTemps = function()
		{
			if (typeof $scope.mytimer != 'undefined') {
				$interval.cancel($scope.mytimer);
				$scope.mytimer = undefined;
			}
		  var id="";
		  $.ajax({
			 url: "json.htm?type=devices&filter=temp&used=true&order=Name&lastupdate="+$.LastUpdateTime,
			 async: false,
			 dataType: 'json',
			 success: function(data) {
				$rootScope.SetTimeAndSun(data.Sunrise,data.Sunset,data.ServerTime);
				
			  if (typeof data.result != 'undefined') {
				if (typeof data.ActTime != 'undefined') {
					$.LastUpdateTime=parseInt(data.ActTime);
				}
				$.each(data.result, function(i,item){
							id="#tempcontent #" + item.idx;
							var obj=$(id);
							if (typeof obj != 'undefined') {
								if ($(id + " #name").html()!=item.Name) {
									$(id + " #name").html(item.Name);
								}
								var img='<img src="images/';
								if (typeof item.Temp != 'undefined') {
									img+=GetTemp48Item(item.Temp);
								}
								else {
									if (item.Type=="Humidity") {
										img+="gauge48.png";
									}
									else {
										img+=GetTemp48Item(item.Chill);
									}
								}
					img+='" height="48" width="48">';
					
					if ($(id + " #img").html()!=img) {
									$(id + " #img").html(img);
								}
					
					var status="";
					var bigtext="";
					var setonclick="";
					var bHaveBefore=false;
					//Evohome...
					var sHeatMode="";
					if (typeof item.Status != 'undefined') { //FIXME only support this for evohome?
						sHeatMode=item.Status;
					}
					if (typeof item.Temp != 'undefined') {
						 bigtext=item.Temp + '\u00B0 ' + $scope.config.TempSign;
					}
					if (item.SubType=="Zone" || item.SubType=="Hot Water") {
						var tUntil="";
						if (typeof item.Until != 'undefined')
							tUntil=item.Until;
						if (typeof item.SetPoint != 'undefined'){
							bigtext+=' ('+item.SetPoint + '\u00B0 ' + $scope.config.TempSign + ')';
							status+=', '+$.t('Set Point') + ': ' + item.SetPoint + '\u00B0 ' + $scope.config.TempSign;
							setonclick='EditSetPoint(' + item.idx + ',\'' + item.Name + '\',' + item.SetPoint + ', \''+sHeatMode+'\', \''+tUntil+'\', \'ShowTemps\');';
						}
						if (typeof item.State != 'undefined'){
							bigtext+=' <img height="12" src="images/evohome/'+item.State+'.png" />'
							status+=', '+$.t('State') + ': ' + item.State;
							setonclick='EditState(' + item.idx + ',\'' + item.Name + '\',\'' + item.State + '\', \''+sHeatMode+'\', \''+tUntil+'\', \'ShowTemps\');';
						}
						if(sHeatMode!="Auto")
							bigtext+=' <img height="15" src="images/evohome/'+sHeatMode+((item.SubType=="Hot Water")?"Inv":"")+'.png" />'
						status+=', '+$.t('Mode') + ': ' + EvoDisplayTextMode(sHeatMode);
						if (tUntil!=""){
							status+=', '+$.t('Until') + ': ' + tUntil.replace(/T/,' ').replace(/\..+/, '');
						}
						bHaveBefore=true;
					}
					if (typeof item.Chill != 'undefined') {
						if (bigtext!="") {
							bigtext+=' / ';
						}
						bigtext+=item.Chill + '\u00B0 ' + $scope.config.TempSign;
					}
					if (typeof item.Humidity != 'undefined') {
						if (bigtext!="") {
							bigtext+=' / ';
						}
						bigtext+=item.Humidity + '%';
					}
					if (typeof item.HumidityStatus != 'undefined') {
					  status+=$.t(item.HumidityStatus);
					  bHaveBefore=true;
					}
					if (typeof item.Barometer != 'undefined') {
						if (bHaveBefore==true) {
							status+=', ';
						}
						status+=$.t('Barometer') + ': ' + item.Barometer + ' hPa';
						bHaveBefore=true;
					}
					if (typeof item.ForecastStr != 'undefined') {
					  status+=', ' + $.t('Prediction') + ': ' + $.t(item.ForecastStr) + '<br>';
					  bHaveBefore=false;
					}
					if (typeof item.Direction != 'undefined') {
					  status+=item.Direction + ' ' + item.DirectionStr + ', ' + $.t('Speed') + ': ' + item.Speed + ' ' + $scope.config.WindSign;
					  if (typeof item.Gust != 'undefined') {
						status+=', ' + $.t('Gust') + ': ' + item.Gust + ' ' + $scope.config.WindSign;
					  }
					}
					if (typeof item.DewPoint != 'undefined') {
						if (bHaveBefore==true) {
							status+=', ';
						}
						status+=$.t("Dew Point") + ": " + item.DewPoint + '\u00B0 ' + $scope.config.TempSign;
					}

					var nbackcolor="#D4E1EE";
					if (item.HaveTimeout==true) {
						nbackcolor="#DF2D3A";
					}
					else {
						var BatteryLevel=parseInt(item.BatteryLevel);
						if (BatteryLevel!=255) {
							if (BatteryLevel<=10) {
								nbackcolor="#DDDF2D";
							}
						}
					}
					//Evohome...
					nbackcolor=EvoSetPointColor(item,sHeatMode,nbackcolor);
					
					var obackcolor=rgb2hex($(id + " #name").css( "background-color" ));
					if (obackcolor!=nbackcolor) {
									$(id + " #name").css( "background-color", nbackcolor );
					}
					
					if ($(id + " #status").html()!=status) {
									$(id + " #bigtext").html(bigtext);
									$(id + " #status").html(status);
									if (setonclick!="")
										$(id + " #set").attr("onclick",setonclick);
								}
								if ($(id + " #lastupdate").html()!=item.LastUpdate) {
									$(id + " #lastupdate").html(item.LastUpdate);
								}
				   }
				});
			  }
			 }
		  });
		  
			$scope.mytimer=$interval(function() {
				RefreshTemps();
			}, 10000);
		}

		ShowForecast = function()
		{
			SwitchLayout("Forecast");
		}

		ShowTemps = function()
		{
			if (typeof $scope.mytimer != 'undefined') {
				$interval.cancel($scope.mytimer);
				$scope.mytimer = undefined;
			}
		  $('#modal').show();

		  var htmlcontent = '';
		  var bHaveAddedDevider = false;
		  var tophtm=
				'\t<table class="bannav" id="bannav" border="0" cellpadding="0" cellspacing="0" width="100%">\n' +
				'\t<tr>\n' +
				'\t  <td align="left" valign="top" id="timesun"></td>\n' +
				'\t  <td align="right">\n' +
				'\t    <a class="btnstyle" onclick="ShowCustomTempLog();" data-i18n="Custom Graph">Custom Graph</a>\n';
		  if ($scope.config.Latitude!="") {
			tophtm+=
				'\t    <a id="Forecast" class="btnstyle" onclick="ShowForecast();" data-i18n="Forecast">Forecast</a>\n';
		  }
		  tophtm+=     
				'\t  </td>\n' +
				'\t</tr>\n' +
				'\t</table>\n';


		  var i=0;

		  $.ajax({
			 url: "json.htm?type=devices&filter=temp&used=true&order=Name",
			 async: false,
			 dataType: 'json',
			 success: function(data) {
			  if (typeof data.result != 'undefined') {
				if (typeof data.ActTime != 'undefined') {
					$.LastUpdateTime=parseInt(data.ActTime);
				}
				$.each(data.result, function(i,item){
				  if (i % 3 == 0)
				  {
					//add devider
					if (bHaveAddedDevider == true) {
					  //close previous devider
					  htmlcontent+='</div>\n';
					}
					htmlcontent+='<div class="row divider">\n';
					bHaveAddedDevider=true;
				  }
				  var xhtm=
						'\t<div class="span4" id="' + item.idx + '">\n' +
						'\t  <section>\n' +
						'\t    <table id="itemtablenotype" border="0" cellpadding="0" cellspacing="0">\n' +
						'\t    <tr>\n';
						var nbackcolor="#D4E1EE";
						if (item.HaveTimeout==true) {
							nbackcolor="#DF2D3A";
						}
						else {
							var BatteryLevel=parseInt(item.BatteryLevel);
							if (BatteryLevel!=255) {
								if (BatteryLevel<=10) {
									nbackcolor="#DDDF2D";
								}
							}
						}
											
						//Evohome...
						var sHeatMode="";
						if (typeof item.Status != 'undefined') { //FIXME only support this for evohome?
							sHeatMode=item.Status;
						}
						nbackcolor=EvoSetPointColor(item,sHeatMode,nbackcolor);
						
						xhtm+='\t      <td id="name" style="background-color: ' + nbackcolor + ';">' + item.Name + '</td>\n';
				  xhtm+='\t      <td id="bigtext">';
				  var tUntil="";
				  var bigtext="";
					if (typeof item.Temp != 'undefined') {
						bigtext=item.Temp + '\u00B0 ' + $scope.config.TempSign;
					}
					if (item.SubType=="Zone" || item.SubType=="Hot Water") {
						if (typeof item.SetPoint != 'undefined'){
							bigtext+=' ('+item.SetPoint + '\u00B0 ' + $scope.config.TempSign + ')';
						}
						if (typeof item.State != 'undefined')
							bigtext+=' <img height="12" src="images/evohome/'+item.State+'.png" />'
						if(sHeatMode!="Auto")
							bigtext+=' <img height="15" src="images/evohome/'+sHeatMode+((item.SubType=="Hot Water")?"Inv":"")+'.png" />'
					}
					if (typeof item.Humidity != 'undefined') {
						if (bigtext!="") {
							bigtext+=' / ';
						}
						bigtext+=item.Humidity + '%';
					}
					if (typeof item.Chill != 'undefined') {
						if (bigtext!="") {
							bigtext+=' / ';
						}
						bigtext+=item.Chill + '\u00B0 ' + $scope.config.TempSign;
					}
				  xhtm+=bigtext+'</td>\n';
				  xhtm+='\t      <td id="img"><img src="images/';
				  if (typeof item.Temp != 'undefined') {
						xhtm+=GetTemp48Item(item.Temp);
					}
					else {
						if (item.Type=="Humidity") {
							xhtm+="gauge48.png";
						}
						else {
							xhtm+=GetTemp48Item(item.Chill);
						}
					}
							xhtm+='" height="48" width="48"></td>\n' +
									'\t      <td id="status">';
							var bHaveBefore=false;
							if (item.SubType=="Zone" || item.SubType=="Hot Water") {
								if (typeof item.SetPoint != 'undefined'){
									xhtm+=', '+$.t('Set Point') + ': ' + item.SetPoint + '\u00B0 ' + $scope.config.TempSign;
								}
								if (typeof item.State != 'undefined'){
									xhtm+=', '+$.t('State') + ': ' + item.State;
								}
								xhtm+=', '+$.t('Mode') + ': ' + EvoDisplayTextMode(sHeatMode);
								if (typeof item.Until != 'undefined'){
									tUntil=item.Until;
									xhtm+=', '+$.t('Until') + ': ' + tUntil.replace(/T/,' ').replace(/\..+/, '');
								}
								bHaveBefore=true;
							}
							if (typeof item.HumidityStatus != 'undefined') {
								xhtm+=$.t(item.HumidityStatus);
								bHaveBefore=true;
							}
							if (typeof item.Barometer != 'undefined') {
								if (bHaveBefore==true) {
									xhtm+=', ';
								}
								xhtm+=$.t('Barometer') + ': ' + item.Barometer + ' hPa';
							}
							if (typeof item.ForecastStr != 'undefined') {
								xhtm+=', ' + $.t('Prediction') + ': ' + $.t(item.ForecastStr) + '<br>';
								bHaveBefore=false;
							}
							if (typeof item.Direction != 'undefined') {
								xhtm+=item.Direction + ' ' + item.DirectionStr + ', ' + $.t('Speed') + ': ' + item.Speed + ' ' + $scope.config.WindSign;
								if (typeof item.Gust != 'undefined') {
									xhtm+=', ' + $.t('Gust') + ': ' + item.Gust + ' ' + $scope.config.WindSign;
								}
							}
							if (typeof item.DewPoint != 'undefined') {
								if (bHaveBefore==true) {
									xhtm+=', ';
								}
								xhtm+=$.t("Dew Point") + ": " + item.DewPoint + '\u00B0 ' + $scope.config.TempSign;
							}
							xhtm+=
									'</td>\n' +
									'\t      <td id="lastupdate">' + item.LastUpdate + '</td>\n' +
									'\t      <td>';
				  if (item.Favorite == 0) {
					xhtm+=
						  '<img src="images/nofavorite.png" title="' + $.t('Add to Dashboard') +'" onclick="MakeFavorite(' + item.idx + ',1);" class="lcursor">&nbsp;&nbsp;&nbsp;&nbsp;';
				  }
				  else {
					xhtm+=
						  '<img src="images/favorite.png" title="' + $.t('Remove from Dashboard') +'" onclick="MakeFavorite(' + item.idx + ',0);" class="lcursor">&nbsp;&nbsp;&nbsp;&nbsp;';
				  }
				  xhtm+=
						'<a class="btnsmall" onclick="ShowTempLog(\'#tempcontent\',\'ShowTemps\',' + item.idx + ',\'' + encodeURIComponent(item.Name) + '\');" data-i18n="Log">Log</a> ';
				if (permissions.hasPermission("Admin")) {
				  if (item.Type=="Humidity") {
					  xhtm+='<a class="btnsmall" onclick="EditTempDeviceSmall(' + item.idx + ',\'' + encodeURIComponent(item.Name) + '\',' + item.AddjValue + ');" data-i18n="Edit">Edit</a> ';
				  }
				  else {
					  xhtm+='<a class="btnsmall" onclick="EditTempDevice(' + item.idx + ',\'' + encodeURIComponent(item.Name) + '\',' + item.AddjValue + ');" data-i18n="Edit">Edit</a> ';
				  }
				  if (item.Notifications == "true")
					xhtm+='<a class="btnsmall-sel" onclick="ShowNotifications(' + item.idx + ',\'' + encodeURIComponent(item.Name) + '\', \'#tempcontent\', \'ShowTemps\');" data-i18n="Notifications">Notifications</a>';
				  else
					xhtm+='<a class="btnsmall" onclick="ShowNotifications(' + item.idx + ',\'' + encodeURIComponent(item.Name) + '\', \'#tempcontent\', \'ShowTemps\');" data-i18n="Notifications">Notifications</a>';
				}
				if (typeof item.forecast_url != 'undefined') {
					xhtm+='&nbsp;<a class="btnsmall" onclick="ShowForecast(\'' + atob(item.forecast_url) + '\',\'' + encodeURIComponent(item.Name) + '\', \'#tempcontent\', \'ShowTemps\');" data-i18n="Forecast">Forecast</a>';
				}
				if (typeof item.SetPoint != 'undefined')
					xhtm+='&nbsp;<a id="set" class="btnsmall" onclick="EditSetPoint(' + item.idx + ',\'' + encodeURIComponent(item.Name) + '\',' + item.SetPoint + ', \''+item.Status+'\', \''+tUntil+'\', \'ShowTemps\');" data-i18n="Set">Set</a>';
				if (typeof item.State != 'undefined')
					xhtm+='&nbsp;<a id="set" class="btnsmall" onclick="EditState(' + item.idx + ',\'' + encodeURIComponent(item.Name) + '\',\'' + item.State + '\', \''+item.Status+'\', \''+tUntil+'\', \'ShowTemps\');" data-i18n="Set">Set</a>';
				
							xhtm+=
						'</td>\n' +
						'\t    </tr>\n' +
						'\t    </table>\n' +
						'\t  </section>\n' +
						'\t</div>\n';
				  htmlcontent+=xhtm;
				});
			  }
			 }
		  });
		  if (bHaveAddedDevider == true) {
			//close previous devider
			htmlcontent+='</div>\n';
		  }
		  if (htmlcontent == '')
		  {
			htmlcontent='<h2>' + $.t('No Temperature sensors found or added in the system...') + '</h2>';
		  }
		  $('#modal').hide();
		  $('#tempcontent').html(tophtm+htmlcontent);
		  $('#tempcontent').i18n();

			$rootScope.RefreshTimeAndSun();
			
			if ($scope.config.AllowWidgetOrdering==true) {
				if (permissions.hasPermission("Admin")) {
					if (window.myglobals.ismobileint==false) {
						$("#tempcontent .span4").draggable({
								drag: function() {
									if (typeof $scope.mytimer != 'undefined') {
										$interval.cancel($scope.mytimer);
										$scope.mytimer = undefined;
									}
									$.devIdx=$(this).attr("id");
									$(this).css("z-index", 2);
								},
								revert: true
						});
						$("#tempcontent .span4").droppable({
								drop: function() {
									var myid=$(this).attr("id");
									$.devIdx.split(' ');
									$.ajax({
										 url: "json.htm?type=command&param=switchdeviceorder&idx1=" + myid + "&idx2=" + $.devIdx,
										 async: false, 
										 dataType: 'json',
										 success: function(data) {
												ShowTemps();
										 }
									});
								}
						});
					}
				}
			}
			$scope.mytimer=$interval(function() {
				RefreshTemps();
			}, 10000);
		  return false;
		}


		init();

		function init()
		{
			//global var
			$.devIdx=0;
			$.LastUpdateTime=parseInt(0);
			
			$scope.MakeGlobalConfig();
			
			var dialog_edittempdevice_buttons = {};
			dialog_edittempdevice_buttons[$.t("Update")]=function() {
				  var bValid = true;
				  bValid = bValid && checkLength( $("#dialog-edittempdevice #edittable #devicename"), 2, 100 );
				  if ( bValid ) {
					  $( this ).dialog( "close" );
					  var aValue=$("#dialog-edittempdevice #edittable #adjustment").val();
					  $.ajax({
						 url: "json.htm?type=setused&idx=" + $.devIdx + '&name=' + encodeURIComponent($("#dialog-edittempdevice #devicename").val()) + '&addjvalue=' + aValue + '&used=true',
						 async: false,
						 dataType: 'json',
						 success: function(data) {
							ShowTemps();
						 }
					  });

				  }
			};
			dialog_edittempdevice_buttons[$.t("Remove Device")]=function() {
				$( this ).dialog( "close" );
				bootbox.confirm($.t("Are you sure to remove this Device?"), function(result) {
					if (result==true) {
					  $.ajax({
						 url: "json.htm?type=setused&idx=" + $.devIdx + '&name=' + encodeURIComponent($("#dialog-edittempdevicesmall #devicename").val()) + '&used=false',
						 async: false,
						 dataType: 'json',
						 success: function(data) {
							ShowTemps();
						 }
					  });
					}
				});
			};
			dialog_edittempdevice_buttons[$.t("Replace")]=function() {
				  $( this ).dialog( "close" );
				  ReplaceDevice($.devIdx,ShowTemps);
			};
			dialog_edittempdevice_buttons[$.t("Cancel")]=function() {
				$( this ).dialog( "close" );
			};
			$( "#dialog-edittempdevice" ).dialog({
				  autoOpen: false,
				  width: 'auto',
				  height: 'auto',
				  modal: true,
				  resizable: false,
				  title: $.t("Edit Device"),
				  buttons: dialog_edittempdevice_buttons,
				  close: function() {
					$( this ).dialog( "close" );
				  }
			});
			
			var dialog_editsetpoint_buttons = {};
			dialog_editsetpoint_buttons[$.t("Set")]=function() {
				  var bValid = true;
				  bValid = bValid && checkLength( $("#dialog-editsetpoint #edittable #devicename"), 2, 100 );
				  var setpoint=$("#dialog-editsetpoint #edittable #setpoint").val();
				  if (setpoint<5 || setpoint>35){
					bootbox.alert($.t('Set point must be between 5 and 35 degrees'));
					return false;
				  }
				  var tUntil="";
				  if($("#dialog-editsetpoint #edittable #until").val()!=""){
					var selectedDate = $("#dialog-editsetpoint #edittable #until").datetimepicker('getDate');
					var now = new Date();
					if (selectedDate < now) {
						bootbox.alert($.t('Temporary set point date / time must be in the future'));
						return false;
					}
					tUntil=selectedDate.toISOString();
				  }
				  if ( bValid ) {
					  $( this ).dialog( "close" );
					 
					  $.ajax({
						 url: "json.htm?type=setused&idx=" + $.devIdx + '&name=' + encodeURIComponent($("#dialog-editsetpoint #devicename").val()) + '&setpoint=' + setpoint + '&mode='+((tUntil!="")?'TemporaryOverride':'PermanentOverride')+'&until='+tUntil+'&used=true',
						 async: false,
						 dataType: 'json',
						 success: function(data) {
							ShowTemps();
						 }
					  });

				  }
			  };
			dialog_editsetpoint_buttons[$.t("Cancel Override")]=function() {
				  var bValid = true;
				  bValid = bValid && checkLength( $("#dialog-editsetpoint #edittable #devicename"), 2, 100 );
				  if ( bValid ) {
					  $( this ).dialog( "close" );
					  var aValue=$("#dialog-editsetpoint #edittable #setpoint").val();
					  if(aValue<5) aValue=5;//These values will display but the controller will update back the currently scheduled setpoint in due course
					  if(aValue>35) aValue=35;//These values will display but the controller will update back the currently scheduled setpoint in due course
					  $.ajax({
						 url: "json.htm?type=setused&idx=" + $.devIdx + '&name=' + encodeURIComponent($("#dialog-editsetpoint #devicename").val()) + '&setpoint=' + aValue + '&mode=Auto&used=true',
						 async: false,
						 dataType: 'json',
						 success: function(data) {
							ShowTemps();
						 }
					  });

				  }
			  };
			dialog_editsetpoint_buttons[$.t("Cancel")]=function() {
				$( this ).dialog( "close" );
				ShowTemps();//going into the dialog removes the background timer refresh (see EditSetPoint)
			};
			  
			$( "#dialog-editsetpoint" ).dialog({
				  autoOpen: false,
				  width: 'auto',
				  height: 'auto',
				  modal: true,
				  resizable: false,
				  title: $.t("Edit Set Point"),
				  buttons: dialog_editsetpoint_buttons,
				  close: function() {
					$( this ).dialog( "close" );
					ShowTemps();//going into the dialog removes the background timer refresh (see EditSetPoint)
				  }
			});
			
			var dialog_editstate_buttons = {};

			dialog_editstate_buttons[$.t("Set")]=function() {
			  var bValid = true;
			  bValid = bValid && checkLength( $("#dialog-editstate #edittable #devicename"), 2, 100 );
			  if ( bValid ) {
				  $( this ).dialog( "close" );
				  var aValue=$("#dialog-editstate #edittable #state").val();
				  var tUntil="";
				  if($("#dialog-editstate #edittable #until_state").val()!="")
					tUntil=$("#dialog-editstate #edittable #until_state").datetimepicker('getDate').toISOString();
				  $.ajax({
					 url: "json.htm?type=setused&idx=" + $.devIdx + '&name=' + encodeURIComponent($("#dialog-editstate #devicename").val()) + '&state=' + aValue + '&mode='+((tUntil!="")?'TemporaryOverride':'PermanentOverride')+'&until='+tUntil+'&used=true',
					 async: false,
					 dataType: 'json',
					 success: function(data) {
						ShowTemps();
					 }
				  });

			  }
			};
			dialog_editstate_buttons[$.t("Cancel")]=function() {
				$( this ).dialog( "close" );
				ShowTemps();//going into the dialog removes the background timer refresh (see EditSetPoint)
			};
			
			$( "#dialog-editstate" ).dialog({
				  autoOpen: false,
				  width: 'auto',
				  height: 'auto',
				  modal: true,
				  resizable: false,
				  title: $.t("Edit State"),
				  buttons: dialog_editstate_buttons,
				  close: function() {
					$( this ).dialog( "close" );
					ShowTemps();//going into the dialog removes the background timer refresh (see EditState)
				  }
			});
			
			var dialog_edittempdevicesmall_buttons = {};
			dialog_edittempdevicesmall_buttons[$.t("Update")]=function() {
				  var bValid = true;
				  bValid = bValid && checkLength( $("#dialog-edittempdevicesmall #edittable #devicename"), 2, 100 );
				  if ( bValid ) {
					  $( this ).dialog( "close" );
					  $.ajax({
						 url: "json.htm?type=setused&idx=" + $.devIdx + '&name=' + encodeURIComponent($("#dialog-edittempdevicesmall #devicename").val()) + '&used=true',
						 async: false,
						 dataType: 'json',
						 success: function(data) {
							ShowTemps();
						 }
					  });

				  }
			  };
			dialog_edittempdevicesmall_buttons[$.t("Remove Device")]=function() {
				$( this ).dialog( "close" );
				bootbox.confirm($.t("Are you sure to remove this Device?"), function(result) {
					if (result==true) {
					  $.ajax({
						 url: "json.htm?type=setused&idx=" + $.devIdx + '&name=' + encodeURIComponent($("#dialog-edittempdevicesmall #devicename").val()) + '&used=false',
						 async: false,
						 dataType: 'json',
						 success: function(data) {
							ShowTemps();
						 }
					  });
					}
				});
			  };
			dialog_edittempdevicesmall_buttons[$.t("Replace")]=function() {
			  $( this ).dialog( "close" );
			  ReplaceDevice($.devIdx,ShowTemps);
			};
			dialog_edittempdevicesmall_buttons[$.t("Cancel")]=function() {
			  $( this ).dialog( "close" );
			};
			$( "#dialog-edittempdevicesmall" ).dialog({
				  autoOpen: false,
				  width: 'auto',
				  height: 'auto',
				  modal: true,
				  resizable: false,
				  title: $.t("Edit Device"),
				  buttons: dialog_edittempdevicesmall_buttons,
				  close: function() {
					$( this ).dialog( "close" );
				  }
			});

		  ShowTemps();
			
			$( "#dialog-edittempdevice" ).keydown(function (event) {
				if (event.keyCode == 13) {
					$(this).siblings('.ui-dialog-buttonpane').find('button:eq(0)').trigger("click");
					return false;
				}
			});
			$( "#dialog-edittempdevicesmall" ).keydown(function (event) {
				if (event.keyCode == 13) {
					$(this).siblings('.ui-dialog-buttonpane').find('button:eq(0)').trigger("click");
					return false;
				}
			});


		};
		$scope.$on('$destroy', function(){
			if (typeof $scope.mytimer != 'undefined') {
				$interval.cancel($scope.mytimer);
				$scope.mytimer = undefined;
			}
		}); 
	} ]);
});