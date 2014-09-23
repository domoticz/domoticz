define(['app'], function (app) {
	app.controller('FloorplanController', [ '$scope', '$rootScope', '$location', '$http', '$interval', '$compile', 'permissions', function($scope,$rootScope,$location,$http,$interval,$compile,permissions) {

		$scope.floorPlans = [];
		$scope.selectedFloorplan = [];
		$scope.FloorplanCount = 0;
		$scope.actFloorplan=0;
		
		$scope.NextFloorplan= function() {
			if ($("#floorplangroup2")[0].getAttribute("zoomed") == "true")
			{
				$("#floorplangroup2")[0].setAttribute("transform", "translate(0,0) scale(1)");
				$("#DeviceContainer")[0].setAttribute("transform", "translate(0,0) scale(1)");
				$("#floorplangroup2")[0].setAttribute("zoomed", "false");
			}
			$scope.RefreshFloorPlan($scope.actFloorplan+1);
		}

		$scope.PrevFloorplan = function() {
			if ($("#floorplangroup2")[0].getAttribute("zoomed") == "true")
			{
				$("#floorplangroup2")[0].setAttribute("transform", "translate(0,0) scale(1)");
				$("#DeviceContainer")[0].setAttribute("transform", "translate(0,0) scale(1)");
				$("#floorplangroup2")[0].setAttribute("zoomed", "false");
			}
			$scope.RefreshFloorPlan($scope.actFloorplan-1);
		}

		$scope.ImageLoaded = function() {
			if (typeof $("#floorplanimagesize") != 'undefined') {
				$('#helptext').attr("title", 'Image width is: ' + $("#floorplanimagesize")[0].naturalWidth + ", Height is: " + $("#floorplanimagesize")[0].naturalHeight);
				Device.xImageSize = 1280;
				Device.yImageSize = 720;
				$("#svgcontainer2")[0].setAttribute("style","display:inline");
			}
		}

		$scope.RoomClick = function(click) {

			$('.DeviceDetails').css('display','none');   // hide all popups 
			if ($("#floorplangroup2")[0].getAttribute("zoomed") == "true")
			{
				$("#floorplangroup2")[0].setAttribute("transform", "translate(0,0) scale(1)");
				$("#DeviceContainer")[0].setAttribute("transform", "translate(0,0) scale(1)");
				$("#floorplangroup2")[0].setAttribute("zoomed", "false");
			}
			else
			{
				var borderRect = click.target.getBBox(); // polygon bounding box
				var margin = 0.1;  // 10% margin around polygon
				var marginX = borderRect.width * margin;
				var marginY = borderRect.height * margin;
				var scaleX= 1280 / (borderRect.width + (marginX * 2));
				var scaleY= 720 / (borderRect.height + (marginY * 2));
				var scale = ((scaleX > scaleY) ? scaleY : scaleX);
				var attr = 'scale('  + scale + ')' + ' translate(' + (borderRect.x - marginX)*-1 + ',' + (borderRect.y - marginY)*-1 + ')';

				// this actually needs to centre in the direction its not scaling but close enough for v1.0
				$("#floorplangroup2")[0].setAttribute("transform", attr);
				$("#DeviceContainer")[0].setAttribute("transform", attr);
				$("#floorplangroup2")[0].setAttribute("zoomed", "true");
			}
		}

		RefreshDevices = function() {
			if (typeof $scope.mytimer != 'undefined') {
				$interval.cancel($scope.mytimer);
				$scope.mytimer = undefined;
			}

			if ((typeof $("#floorplangroup2") != 'undefined') &&
				(typeof $("#floorplangroup2")[0] != 'undefined')) {
			
				if ($("#floorplangroup2")[0].getAttribute("planidx") != "") {
					$http({
							 url: "json.htm?type=devices&filter=all&used=true&order=Name&floor=" + $scope.floorPlans[$scope.actFloorplan].idx + "&lastupdate=" + $.LastUpdateTime
						}).success(function(data) {
							if (typeof data.ActTime != 'undefined') {
								$.LastUpdateTime = data.ActTime;
							}
							
							if (typeof data.WindScale != 'undefined') {
								$.myglobals.windscale=parseFloat(data.WindScale);
							}
							if (typeof data.WindSign != 'undefined') {
								$.myglobals.windsign=data.WindSign;
							}
							if (typeof data.TempScale != 'undefined') {
								$.myglobals.tempscale=parseFloat(data.TempScale);
							}
							if (typeof data.TempSign != 'undefined') {
								$.myglobals.tempsign=data.TempSign;
							}

							// update devices that already exist in the DOM
							if (typeof data.result != 'undefined') {
								$.each(data.result, function(i,item) {
									var dev = Device.create(item);
									var existing = document.getElementById(dev.uniquename);
									if (existing != undefined) {
										dev.htmlMinimum($("#roomplangroup2")[0]);
									}
								});
							}
							$scope.mytimer=$interval(function() {
								RefreshDevices();
							}, 10000);
					}).error(function() {
							$scope.mytimer=$interval(function() {
								RefreshDevices();
							}, 10000);
					});
				}
			}
		}

		$scope.ShowDevices = function(idx, parent) {
			if (typeof $("#floorplangroup2") != 'undefined') {

				$.ajax({
					 url: "json.htm?type=devices&filter=all&used=true&order=Name&floor="+idx,
					 async: false,
					 dataType: 'json',
					 success: function(data) {
						if (typeof data.ActTime != 'undefined') {
							$.LastUpdateTime = 0;
						}

						if (typeof data.WindScale != 'undefined') {
							$.myglobals.windscale=parseFloat(data.WindScale);
						}
						if (typeof data.WindSign != 'undefined') {
							$.myglobals.windsign=data.WindSign;
						}
						if (typeof data.TempScale != 'undefined') {
							$.myglobals.tempscale=parseFloat(data.TempScale);
						}
						if (typeof data.TempSign != 'undefined') {
							$.myglobals.tempsign=data.TempSign;
						}
						
						// insert devices into the document
						var dev;
						if (typeof data.result != 'undefined') {
							$.each(data.result, function(i,item) {
								if (item.Type.indexOf('+') >= 0) {
									var aDev = item.Type.split('+');
									var i;
									for (i=0; i < aDev.length; i++) {
										var sDev = aDev[i].trim();
										item.Name = ((i == 0) ? item.Name : sDev);
										item.Type = sDev;
										item.CustomImage = 1;
										item.Image = sDev.toLowerCase();
										item.XOffset = Math.abs(item.XOffset) + ((i == 0) ? 0 : 50);
										dev = Device.create(item);
										dev.htmlMinimum(parent);
									}
								} else {
									dev = Device.create(item);
									dev.htmlMinimum(parent);
								}
							});
						}
					}
				});

				// Initial refresh needs to be short, this will trigger the status elements to be drawn
				$scope.mytimer=$interval(function() {
								RefreshDevices();
							}, 500);
			}
		}

		$scope.RefreshFloorPlan = function(floorIdx) {
			if (typeof $scope.mytimer != 'undefined') {
				$interval.cancel($scope.mytimer);
				$scope.mytimer = undefined;
			}
			$("#roomplangroup2").empty();
			
			if (typeof floorIdx == 'undefined') {
				floorIdx=$scope.actFloorplan;
			}
			
			if (floorIdx < 0) floorIdx = $scope.FloorplanCount-1;
			if (floorIdx >= $scope.FloorplanCount) floorIdx = 0;
			$scope.actFloorplan = floorIdx;
			
			if ($scope.FloorplanCount > 0) {
				$scope.selectedFloorplan = $scope.floorPlans[floorIdx];
				$("#floorplanimage").attr("xlink:href", $scope.floorPlans[floorIdx].Image);
				$("#floorplanimagesize").attr("src", $scope.floorPlans[floorIdx].Image);
				Device.initialise();
				$.ajax({
					url: "json.htm?type=command&param=getfloorplanplans&idx=" + $scope.floorPlans[floorIdx].idx, 
					async: false, 
					dataType: 'json',
					success: function(data) {
						if (typeof data.result != 'undefined') {
							var planGroup = $("#roomplangroup2")[0];
							$.each(data.result, function(i,item) {
								var el = makeSVGnode('polygon', { id: item.Name + "_Room", 'class': 'hoverable', points: item.Area }, '');
								el.appendChild(makeSVGnode('title', null, item.Name));
								planGroup.appendChild(el);
							});
						}
					}
				});

				if ((typeof $.myglobals.RoomColour != 'undefined') && 
					(typeof $.myglobals.InactiveRoomOpacity != 'undefined')) {
					$(".hoverable").css({'fill': $.myglobals.RoomColour, 'fill-opacity': $.myglobals.InactiveRoomOpacity/100});
				}
				if (typeof $.myglobals.ActiveRoomOpacity != 'undefined') {
					$(".hoverable").hover(function(){$(this).css({'fill-opacity': $.myglobals.ActiveRoomOpacity/100})},
										  function(){$(this).css({'fill-opacity': $.myglobals.InactiveRoomOpacity/100})});
				}

				$scope.ShowDevices($scope.floorPlans[floorIdx].idx, $("#roomplangroup2")[0]);
			}
		}

		$scope.changeFloorplan = function() {
			for (var i = 0, len = $scope.floorPlans.length; i < len; i++) {
			  if ($scope.floorPlans[i].Name == $scope.selectedFloorplan.Name) {
				$scope.RefreshFloorPlan(i);
				break;
			  }
			}		
		}

		ShowFloorplan = function(floorIdx) {
			var htmlcontent = "";
			htmlcontent+=$('#fpmaincontent').html();
			$('#floorplancontent').html($compile(htmlcontent)($scope));
			$('#floorplancontent').i18n();

			Device.useSVGtags = true;
			Device.backFunction = 'ShowFloorplan';
			Device.switchFunction = 'RefreshDevices';
			Device.contentTag = 'floorplancontent';

			$scope.RefreshFloorPlan(floorIdx);

			$('#roomplangroup2')
				.off()
				.on('click', function ( event ) { $scope.RoomClick( event ); });
		}

		init();

		function init()
		{
			//Get initial floorplans
			$.LastUpdateTime=parseInt(0);
			$scope.floorPlans = [];
			$scope.actFloorplan=0;
			$http({
				url: "json.htm?type=floorplans"
				}).success(function(data) {
					if (typeof data.result != 'undefined') {
						$scope.FloorplanCount = data.result.length;
						$scope.floorPlans=data.result;
						// handle settings
						if (typeof data.RoomColour != 'undefined') {
							$.myglobals.RoomColour = data.RoomColour;
						}
						if (typeof data.ActiveRoomOpacity != 'undefined') {
							$.myglobals.ActiveRoomOpacity = data.ActiveRoomOpacity;
						}
						if (typeof data.InactiveRoomOpacity != 'undefined') {
							$.myglobals.InactiveRoomOpacity = data.InactiveRoomOpacity;
						}
						if (typeof data.PopupDelay != 'undefined') {
							Device.popupDelay = data.PopupDelay;
						}
						if (typeof data.ShowSensorValues != 'undefined') {
							Device.showSensorValues = (data.ShowSensorValues == 1);
						}
						if (typeof data.ShowSwitchValues != 'undefined') {
							Device.showSwitchValues = (data.ShowSwitchValues == 1);
						}
						//Lets start
						ShowFloorplan(0);
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