define(['app'], function (app) {
	app.controller('WeatherController', [ '$scope', '$rootScope', '$location', '$http', '$interval', 'permissions', function($scope,$rootScope,$location,$http,$interval,permissions) {

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
			  ShowWeathers();
			 }
		  });
		}

		EditRainDevice = function(idx,name,addjmulti)
		{
			if (typeof $scope.mytimer != 'undefined') {
				$interval.cancel($scope.mytimer);
				$scope.mytimer = undefined;
			}
		  $.devIdx=idx;
		  $("#dialog-editraindevice #devicename").val(decodeURIComponent(name));
		  $("#dialog-editraindevice #multiply").val(addjmulti);
		  $( "#dialog-editraindevice" ).i18n();
		  $("#dialog-editraindevice" ).dialog( "open" );
		}

		EditBaroDevice = function(idx,name,addjvalue)
		{
			if (typeof $scope.mytimer != 'undefined') {
				$interval.cancel($scope.mytimer);
				$scope.mytimer = undefined;
			}
		  $.devIdx=idx;
		  $("#dialog-editbarodevice #devicename").val(decodeURIComponent(name));
		  $("#dialog-editbarodevice #adjustment").val(addjvalue);
		  $("#dialog-editbarodevice" ).i18n();
		  $("#dialog-editbarodevice" ).dialog( "open" );
		}

		EditVisibilityDevice = function(idx,name,switchtype)
		{
			if (typeof $scope.mytimer != 'undefined') {
				$interval.cancel($scope.mytimer);
				$scope.mytimer = undefined;
			}
		  $.devIdx=idx;
		  $("#dialog-editvisibilitydevice #devicename").val(decodeURIComponent(name));
		  $("#dialog-editvisibilitydevice #combometertype").val(switchtype);
		  $("#dialog-editvisibilitydevice" ).i18n();
		  $("#dialog-editvisibilitydevice" ).dialog( "open" );
		}

		EditWeatherDevice = function(idx,name)
		{
			if (typeof $scope.mytimer != 'undefined') {
				$interval.cancel($scope.mytimer);
				$scope.mytimer = undefined;
			}
		  $.devIdx=idx;
		  $("#dialog-editweatherdevice #devicename").val(decodeURIComponent(name));
		  $("#dialog-editweatherdevice" ).i18n();
		  $("#dialog-editweatherdevice" ).dialog( "open" );
		}

		AddWeatherDevice = function()
		{
		  bootbox.alert($.t('Please use the devices tab for this.'));
		}

		RefreshWeathers = function()
		{
			if (typeof $scope.mytimer != 'undefined') {
				$interval.cancel($scope.mytimer);
				$scope.mytimer = undefined;
			}
		  var id="";

		  $.ajax({
			 url: "json.htm?type=devices&filter=weather&used=true&order=Name&lastupdate="+$.LastUpdateTime,
			 async: false, 
			 dataType: 'json',
			 success: function(data) {
				$rootScope.SetTimeAndSun(data.Sunrise,data.Sunset,data.ServerTime);

			  if (typeof data.result != 'undefined') {
				if (typeof data.ActTime != 'undefined') {
					$.LastUpdateTime=parseInt(data.ActTime);
				}
			  
				$.each(data.result, function(i,item){
				id="#weathercontent #" + item.idx;
				var obj=$(id);
				if (typeof obj != 'undefined') {
					if ($(id + " #name").html()!=item.Name) {
						$(id + " #name").html(item.Name);
					}

					var status="";
					var img="";
					var bigtext="";
					if (typeof item.Barometer != 'undefined') {
						img='<img src="images/baro48.png" height="48" width="48">';
						bigtext=item.Barometer + ' hPa';
						if (typeof item.ForecastStr != 'undefined') {
							status=item.Barometer + ' hPa, ' + $.t('Prediction') + ': ' + $.t(item.ForecastStr);
						}
						else
						{
							status=item.Barometer + ' hPa';
						}
						if (typeof item.Altitude != 'undefined') {
							status+=', Altitude: ' + item.Altitude + ' meter';
						}
					}
					if (typeof item.Rain != 'undefined') {
						img='<img src="images/rain48.png" height="48" width="48">';
						status=item.Rain + ' mm';
						bigtext=item.Rain + ' mm';
						if (typeof item.RainRate != 'undefined') {
							if (item.RainRate!=0) {
								status+=', Rate: ' + item.RainRate + ' mm/h';
							}
						}
					}
					if (typeof item.Visibility != 'undefined') {
						img='<img src="images/visibility48.png" height="48" width="48">';
						status=item.Data;
						bigtext=item.Data;
					}
					else if (typeof item.UVI != 'undefined') {
						img='<img src="images/uv48.png" height="48" width="48">';
						status=item.UVI + ' UVI';
						bigtext=item.UVI + ' UVI';
						if (typeof item.Temp!= 'undefined') {
							status+=', ' + $.t('Temp') + ': ' + item.Temp + '\u00B0 ' + $scope.config.TempSign;
						}
					}
					if (typeof item.Radiation != 'undefined') {
						img='<img src="images/radiation48.png" height="48" width="48">';
						status=item.Data;
						bigtext=item.Data;
					}
					else if (typeof item.Direction != 'undefined') {
						img='<img src="images/Wind' + item.DirectionStr + '.png" height="48" width="48">';
						status=item.Direction + ' ' + item.DirectionStr;
						if (typeof item.Speed != 'undefined') {
							status+=', ' + $.t('Speed') +': ' + item.Speed + ' ' + $scope.config.WindSign;
						}
						if (typeof item.Gust != 'undefined') {
							status+=', ' + $.t('Gust') + ': ' + item.Gust + ' ' + $scope.config.WindSign;
						}
						status+='<br>\n';
						bigtext=item.DirectionStr;
						if (typeof item.Speed != 'undefined') {
							bigtext+=  ' / ' + item.Speed + ' ' + $scope.config.WindSign;
						}
						else if (typeof item.Gust != 'undefined') {
							bigtext+=  ' / ' + item.Gust + ' ' + $scope.config.WindSign;
						}
						if ((typeof item.Temp != 'undefined')&&(typeof item.Chill != 'undefined')) {
							status+=$.t('Temp') + ': ' + item.Temp + '\u00B0 ' +  $scope.config.TempSign + ', ';
							status+=$.t('Chill') +': ' + item.Chill + '\u00B0 ' + $scope.config.TempSign;
						}
						else {
							if (typeof item.Chill != 'undefined') {
								status+=$.t('Chill') + ': ' + item.Chill + '\u00B0 ' + $scope.config.TempSign;
							}
						}
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
					var obackcolor=rgb2hex($(id + " #name").css( "background-color" ));
					if (obackcolor!=nbackcolor) {
									$(id + " #name").css( "background-color", nbackcolor );
					}
								
								if ($(id + " #img").html()!=img) {
									$(id + " #img").html(img);
								}
								if ($(id + " #status").html()!=status) {
									$(id + " #status").html(status);
								}
								if ($(id + " #bigtext").html()!=bigtext) {
									$(id + " #bigtext").html(bigtext);
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
				RefreshWeathers();
			}, 10000);
		}

		ShowForecast = function()
		{
			SwitchLayout("Forecast");
		}

		ShowWeathers = function()
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
				'\t  <td align="left" valign="top" id="timesun"></td>\n';
		  if ($scope.config.Latitude!="") {
			tophtm+=
				'\t  <td style="width: 150px;" align="right">\n' +
				'\t    <a id="Forecast" class="btnstyle" onclick="ShowForecast();" data-i18n="Forecast">Forecast</a>\n' +
				'\t  </td>\n';
		  }
		  tophtm+=
				'\t</tr>\n' +
				'\t</table>\n';
		  

		  var i=0;
		  $.ajax({
			 url: "json.htm?type=devices&filter=weather&used=true&order=Name", 
			 async: false, 
			 dataType: 'json',
			 success: function(data) {
			  if (typeof data.result != 'undefined') {
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
				  var bHaveVisibility = false;
				  var bHaveRain = false;
				  var bHaveBaro = false;
				  var bHaveUV = false;
				  var bHaveRadiation = false;
				  var bHaveWind = false;
				  
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
							xhtm+='\t      <td id="name" style="background-color: ' + nbackcolor + ';">' + item.Name + '</td>\n';
							xhtm+='\t      <td id="bigtext">';
							if (typeof item.Barometer != 'undefined') {
								xhtm+=item.Barometer + ' hPa';
							}
							else if (typeof item.Rain != 'undefined') {
								xhtm+=item.Rain + ' mm';
							}
							else if (typeof item.Visibility != 'undefined') {
								xhtm+=item.Data;
							}
							else if (typeof item.UVI != 'undefined') {
								xhtm+=item.UVI + ' UVI';
							}
							else if (typeof item.Radiation != 'undefined') {
								xhtm+=item.Data;
							}
							else if (typeof item.Direction != 'undefined') {
								xhtm+=item.DirectionStr;
							}
							if (typeof item.Speed != 'undefined') {
								xhtm+=' / ' + item.Speed + ' ' + $scope.config.WindSign;
							}
							else if (typeof item.Gust != 'undefined') {
								xhtm+=' / ' + item.Gust + ' ' + $scope.config.WindSign;
							}
							xhtm+='</td>\n';
							xhtm+='\t      <td id="img"><img src="images/';
							if (typeof item.Barometer != 'undefined') {
								bHaveBaro=true;
								xhtm+='baro48.png" height="48" width="48"></td>\n' +
									  '\t      <td id="status">' + item.Barometer + ' hPa';
								if (typeof item.ForecastStr != 'undefined') {
									xhtm+=', ' + $.t('Prediction') + ': ' + $.t(item.ForecastStr);
								}
								if (typeof item.Altitude != 'undefined') {
									xhtm+=', Altitude: ' + item.Altitude + ' meter';
								}
							}
							else if (typeof item.Rain != 'undefined') {
								bHaveRain=true;
								xhtm+='rain48.png" height="48" width="48"></td>\n' +
								'\t      <td  id="status">' + item.Rain + ' mm';
								if (typeof item.RainRate != 'undefined') {
									if (item.RainRate!=0) {
										xhtm+=', Rate: ' + item.RainRate + ' mm/h';
									}
								}
							}
							else if (typeof item.Visibility != 'undefined') {
								bHaveVisibility=true;
								xhtm+='visibility48.png" height="48" width="48"></td>\n' +
								'\t      <td  id="status">' + item.Data;
							}
							else if (typeof item.UVI != 'undefined') {
								bHaveUV=true;
								xhtm+='uv48.png" height="48" width="48"></td>\n' +
								'\t      <td id="status">' + item.UVI + ' UVI';
								if (typeof item.Temp!= 'undefined') {
									xhtm+=', ' + $.t('Temp') +': ' + item.Temp + '\u00B0 ' + $scope.config.TempSign;
								}
							}
							else if (typeof item.Radiation != 'undefined') {
								bHaveRadiation=true;
								xhtm+='radiation48.png" height="48" width="48"></td>\n' +
								'\t      <td  id="status">' + item.Data;
							}
							else if (typeof item.Direction != 'undefined') {
								bHaveWind=true;
								xhtm+='Wind' + item.DirectionStr + '.png" height="48" width="48"></td>\n' +
								'\t      <td id="status">' + item.Direction + ' ' + item.DirectionStr;
								if (typeof item.Speed != 'undefined') {
									xhtm+=', ' + $.t('Speed') + ': ' + item.Speed + ' ' + $scope.config.WindSign;
								}
								if (typeof item.Gust != 'undefined') {
									xhtm+=', ' + $.t('Gust') + ': ' + item.Gust + ' ' + $scope.config.WindSign;
								}
								xhtm+='<br>\n';
								if ((typeof item.Temp != 'undefined')&&(typeof item.Chill != 'undefined')) {
									xhtm+=$.t('Temp') +': ' + item.Temp + '\u00B0 ' + $scope.config.TempSign + ', ';
									xhtm+=$.t('Chill') +': ' + item.Chill + '\u00B0 ' + $scope.config.TempSign;
								}
								else  {
									if (typeof item.Chill != 'undefined') {
										xhtm+=$.t('Chill') +': ' + item.Chill + '\u00B0 ' + $scope.config.TempSign;
									}
								}
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

				  if (typeof item.Barometer != 'undefined') {
					xhtm+='<a class="btnsmall" onclick="ShowBaroLog(\'#weathercontent\',\'ShowWeathers\',' + item.idx + ',\'' + encodeURIComponent(item.Name) + '\');" data-i18n="Log">Log</a> ';
				  }
				  else if (typeof item.Rain != 'undefined') {
					xhtm+='<a class="btnsmall" onclick="ShowRainLog(\'#weathercontent\',\'ShowWeathers\',' + item.idx + ',\'' + encodeURIComponent(item.Name) + '\');" data-i18n="Log">Log</a> ';
				  }
				  else if (typeof item.UVI != 'undefined') {
					xhtm+='<a class="btnsmall" onclick="ShowUVLog(\'#weathercontent\',\'ShowWeathers\',' + item.idx + ',\'' + encodeURIComponent(item.Name) + '\');" data-i18n="Log">Log</a> ';
				  }
				  else if (typeof item.Direction != 'undefined') {
					xhtm+='<a class="btnsmall" onclick="ShowWindLog(\'#weathercontent\',\'ShowWeathers\',' + item.idx + ',\'' + encodeURIComponent(item.Name) + '\');" data-i18n="Log">Log</a> ';
				  }
				  else if (typeof item.Visibility != 'undefined') {
					xhtm+='<a class="btnsmall" onclick="ShowGeneralGraph(\'#weathercontent\',\'ShowWeathers\',' + item.idx + ',\'' + encodeURIComponent(item.Name) + '\',' + item.SwitchTypeVal +', \'Visibility\');" data-i18n="Log">Log</a> ';
				  }
				  else if (typeof item.Radiation != 'undefined') {
					xhtm+='<a class="btnsmall" onclick="ShowGeneralGraph(\'#weathercontent\',\'ShowWeathers\',' + item.idx + ',\'' + encodeURIComponent(item.Name) + '\',' + item.SwitchTypeVal +', \'Radiation\');" data-i18n="Log">Log</a> ';
				  }
				  if (permissions.hasPermission("Admin")) {
					  if (bHaveRain) {
						xhtm+='<a class="btnsmall" onclick="EditRainDevice(' + item.idx + ',\'' + encodeURIComponent(item.Name) + '\',' + item.AddjMulti +');" data-i18n="Edit">Edit</a> ';
					  }
					  else if (bHaveVisibility) {
						xhtm+='<a class="btnsmall" onclick="EditVisibilityDevice(' + item.idx + ',\'' + encodeURIComponent(item.Name) + '\',' + item.SwitchTypeVal +');" data-i18n="Edit">Edit</a> ';
					  }
					  else if (bHaveRadiation) {
						xhtm+='<a class="btnsmall" onclick="EditWeatherDevice(' + item.idx + ',\'' + encodeURIComponent(item.Name) + '\');" data-i18n="Edit">Edit</a> ';
					  }
					  else if (bHaveBaro) {
						xhtm+='<a class="btnsmall" onclick="EditBaroDevice(' + item.idx + ',\'' + encodeURIComponent(item.Name) + '\',' + item.AddjValue2 +');" data-i18n="Edit">Edit</a> ';
					  }
					  else {
						xhtm+='<a class="btnsmall" onclick="EditWeatherDevice(' + item.idx + ',\'' + encodeURIComponent(item.Name) + '\');" data-i18n="Edit">Edit</a> ';
					  }
					  if (item.Notifications == "true")
						xhtm+='<a class="btnsmall-sel" onclick="ShowNotifications(' + item.idx + ',\'' + encodeURIComponent(item.Name) + '\', \'#weathercontent\', \'ShowWeathers\');" data-i18n="Notifications">Notifications</a>';
					  else
						xhtm+='<a class="btnsmall" onclick="ShowNotifications(' + item.idx + ',\'' + encodeURIComponent(item.Name) + '\', \'#weathercontent\', \'ShowWeathers\');" data-i18n="Notifications">Notifications</a>';
						if (typeof item.forecast_url != 'undefined') {
							xhtm+='&nbsp;<a class="btnsmall" onclick="ShowForecast(\'' + atob(item.forecast_url) + '\',\'' + encodeURIComponent(item.Name) + '\', \'#weathercontent\', \'ShowWeathers\');" data-i18n="Forecast">Forecast</a>';
						}
				  }
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
			htmlcontent='<h2>' + $.t('No Weather sensors found or added in the system...') + '</h2>';
		  }
		  $('#modal').hide();
		  $('#weathercontent').html(tophtm+htmlcontent);
		  $('#weathercontent').i18n();
		  
			$rootScope.RefreshTimeAndSun();

			if ($scope.config.AllowWidgetOrdering==true) {
				if (permissions.hasPermission("Admin")) {
					if (window.myglobals.ismobileint==false) {
						$("#weathercontent .span4").draggable({
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
						$("#weathercontent .span4").droppable({
								drop: function() {
									var myid=$(this).attr("id");
									$.devIdx.split(' ');
									$.ajax({
										 url: "json.htm?type=command&param=switchdeviceorder&idx1=" + myid + "&idx2=" + $.devIdx,
										 async: false, 
										 dataType: 'json',
										 success: function(data) {
												ShowWeathers();
										 }
									});
								}
						});
					}
				}
			}
			$scope.mytimer=$interval(function() {
				RefreshWeathers();
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
			
			var dialog_editweatherdevice_buttons = {};
			dialog_editweatherdevice_buttons[$.t("Update")]=function() {
			  var bValid = true;
			  bValid = bValid && checkLength( $("#dialog-editweatherdevice #devicename"), 2, 100 );
			  if ( bValid ) {
				  $( this ).dialog( "close" );
				  $.ajax({
					 url: "json.htm?type=setused&idx=" + $.devIdx + '&name=' + encodeURIComponent($("#dialog-editweatherdevice #devicename").val()) + '&used=true',
					 async: false, 
					 dataType: 'json',
					 success: function(data) {
						ShowWeathers();
					 }
				  });
				  
			  }
			};
			dialog_editweatherdevice_buttons[$.t("Remove Device")]=function() {
				$( this ).dialog( "close" );
				bootbox.confirm($.t("Are you sure to remove this Device?"), function(result) {
					if (result==true) {
					  $.ajax({
						 url: "json.htm?type=setused&idx=" + $.devIdx + '&name=' + encodeURIComponent($("#dialog-editweatherdevice #devicename").val()) + '&used=false',
						 async: false, 
						 dataType: 'json',
						 success: function(data) {
							ShowWeathers();
						 }
					  });
					}
				});
			};
			dialog_editweatherdevice_buttons[$.t("Replace")]=function() {
				  $( this ).dialog( "close" );
				  ReplaceDevice($.devIdx,ShowWeathers);
			};
			dialog_editweatherdevice_buttons[$.t("Cancel")]=function() {
			  $( this ).dialog( "close" );
			};

			$( "#dialog-editweatherdevice" ).dialog({
				  autoOpen: false,
				  width: 'auto',
				  height: 'auto',
				  modal: true,
				  resizable: false,
				  title: $.t("Edit Device"),
				  buttons: dialog_editweatherdevice_buttons,
				  close: function() {
					$( this ).dialog( "close" );
				  }
			}).i18n();
			
			var dialog_editraindevice_buttons = {};
			
			dialog_editraindevice_buttons[$.t("Update")]=function() {
			  var bValid = true;
			  bValid = bValid && checkLength( $("#dialog-editraindevice #edittable #devicename"), 2, 100 );
			  if ( bValid ) {
				  $( this ).dialog( "close" );
				  $.ajax({
					 url: "json.htm?type=setused&idx=" + $.devIdx + '&name=' + encodeURIComponent($("#dialog-editraindevice #devicename").val()) + '&addjmulti=' + $("#dialog-editraindevice #edittable #multiply").val() + '&used=true',
					 async: false, 
					 dataType: 'json',
					 success: function(data) {
						ShowWeathers();
					 }
				  });
				  
			  }
			};
			dialog_editraindevice_buttons[$.t("Remove Device")]=function() {
				$( this ).dialog( "close" );
				bootbox.confirm($.t("Are you sure to remove this Device?"), function(result) {
					if (result==true) {
					  $.ajax({
						 url: "json.htm?type=setused&idx=" + $.devIdx + '&name=' + encodeURIComponent($("#dialog-editweatherdevice #devicename").val()) + '&used=false',
						 async: false, 
						 dataType: 'json',
						 success: function(data) {
							ShowWeathers();
						 }
					  });
					}
				});
			};
			dialog_editraindevice_buttons[$.t("Replace")]=function() {
			  $( this ).dialog( "close" );
			  ReplaceDevice($.devIdx,ShowWeathers);
			};
			dialog_editraindevice_buttons[$.t("Cancel")]=function() {
			  $( this ).dialog( "close" );
			};
			
			$( "#dialog-editraindevice" ).dialog({
				  autoOpen: false,
				  width: 'auto',
				  height: 'auto',
				  modal: true,
				  resizable: false,
				  title: $.t("Edit Device"),
				  buttons: dialog_editraindevice_buttons,
				  close: function() {
					$( this ).dialog( "close" );
				  }
			}).i18n();
			
			var dialog_editbarodevice_buttons = {};
			
			dialog_editbarodevice_buttons[$.t("Update")]=function() {
			  var bValid = true;
			  bValid = bValid && checkLength( $("#dialog-editbarodevice #edittable #devicename"), 2, 100 );
			  if ( bValid ) {
				  $( this ).dialog( "close" );
				  var aValue=$("#dialog-editbarodevice #edittable #adjustment").val();
				  $.ajax({
					 url: "json.htm?type=setused&idx=" + $.devIdx + '&name=' + encodeURIComponent($("#dialog-editbarodevice #devicename").val()) + '&addjvalue2=' + aValue + '&used=true',
					 async: false,
					 dataType: 'json',
					 success: function(data) {
						ShowWeathers();
					 }
				  });

			  }
			};
			dialog_editbarodevice_buttons[$.t("Remove Device")]=function() {
				$( this ).dialog( "close" );
				bootbox.confirm($.t("Are you sure to remove this Device?"), function(result) {
					if (result==true) {
					  $.ajax({
						 url: "json.htm?type=setused&idx=" + $.devIdx + '&name=' + encodeURIComponent($("#dialog-editbarodevice #devicename").val()) + '&used=false',
						 async: false,
						 dataType: 'json',
						 success: function(data) {
							ShowWeathers();
						 }
					  });
					}
				});
			};
			dialog_editbarodevice_buttons[$.t("Replace")]=function() {
			  $( this ).dialog( "close" );
			  ReplaceDevice($.devIdx,ShowWeathers);
			};
			dialog_editbarodevice_buttons[$.t("Cancel")]=function() {
			  $( this ).dialog( "close" );
			};
			$( "#dialog-editbarodevice" ).dialog({
				  autoOpen: false,
				  width: 'auto',
				  height: 'auto',
				  modal: true,
				  resizable: false,
				  title: $.t("Edit Device"),
				  buttons: dialog_editbarodevice_buttons,
				  close: function() {
					$( this ).dialog( "close" );
				  }
			}).i18n();
			
			var dialog_editvisibilitydevice_buttons = {};
			dialog_editvisibilitydevice_buttons[$.t("Update")]=function() {
			  var bValid = true;
			  bValid = bValid && checkLength( $("#dialog-editvisibilitydevice #devicename"), 2, 100 );
			  if ( bValid ) {
				  $( this ).dialog( "close" );
				  $.ajax({
					 url: "json.htm?type=setused&idx=" + $.devIdx + '&name=' + encodeURIComponent($("#dialog-editvisibilitydevice #devicename").val()) + '&switchtype=' + $("#dialog-editvisibilitydevice #combometertype").val() + '&used=true',
					 async: false, 
					 dataType: 'json',
					 success: function(data) {
						ShowWeathers();
					 }
				  });
				  
			  }
			};
			dialog_editvisibilitydevice_buttons[$.t("Remove Device")]=function() {
				$( this ).dialog( "close" );
				bootbox.confirm($.t("Are you sure to remove this Device?"), function(result) {
					if (result==true) {
					  $.ajax({
						 url: "json.htm?type=setused&idx=" + $.devIdx + '&name=' + encodeURIComponent($("#dialog-editvisibilitydevice #devicename").val()) + '&used=false',
						 async: false, 
						 dataType: 'json',
						 success: function(data) {
							ShowWeathers();
						 }
					  });
					}
				});
			};
			dialog_editvisibilitydevice_buttons[$.t("Cancel")]=function() {
				  $( this ).dialog( "close" );
			};
			
			$( "#dialog-editvisibilitydevice" ).dialog({
				  autoOpen: false,
				  width: 'auto',
				  height: 'auto',
				  modal: true,
				  resizable: false,
				  title: $.t("Edit Device"),
				  buttons: dialog_editvisibilitydevice_buttons,
				  close: function() {
					$( this ).dialog( "close" );
				  }
			}).i18n();

		  ShowWeathers();
			$( "dialog-editweatherdevice" ).keydown(function (event) {
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