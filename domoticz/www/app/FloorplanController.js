define(['app'], function (app) {
	app.controller('FloorplanController', [ '$scope', '$location', '$http', '$interval', function($scope,$location,$http,$interval) {

		var aFloorplans = {};

		NextFloorplan= function() {
			if ($("#floorplangroup")[0].getAttribute("zoomed") == "true")
			{
				$("#floorplangroup")[0].setAttribute("transform", "translate(0,0) scale(1)");
				$("#DeviceContainer")[0].setAttribute("transform", "translate(0,0) scale(1)");
				$("#floorplangroup")[0].setAttribute("zoomed", "false");
			}
			RefreshFloorPlan(window.myglobals.FloorplanIndex+1);
		}

		PrevFloorplan = function() {
			if ($("#floorplangroup")[0].getAttribute("zoomed") == "true")
			{
				$("#floorplangroup")[0].setAttribute("transform", "translate(0,0) scale(1)");
				$("#DeviceContainer")[0].setAttribute("transform", "translate(0,0) scale(1)");
				$("#floorplangroup")[0].setAttribute("zoomed", "false");
			}
			RefreshFloorPlan(window.myglobals.FloorplanIndex-1);
		}

		ImageLoaded = function() {
			if (typeof $("#floorplanimagesize") != 'undefined') {
				$('#helptext').attr("title", 'Image width is: ' + $("#floorplanimagesize")[0].naturalWidth + ", Height is: " + $("#floorplanimagesize")[0].naturalHeight);
				$("#svgcontainer")[0].setAttribute("viewBox","0 0 1280 720");
				Device.xImageSize = 1280;
				Device.yImageSize = 720;
				$("#svgcontainer")[0].setAttribute("style","display:inline");
				SVGResize();
			}
		}

		SVGResize = function() {
			var svgHeight;
			if ((typeof $("#floorplancontainer") != 'undefined') && (typeof $("#floorplanimagesize") != 'undefined') && (typeof $("#floorplanimagesize")[0] != 'undefined') && ($("#floorplanimagesize")[0].naturalWidth != 'undefined')){
				$("#floorplancontainer")[0].setAttribute('naturalWidth', $("#floorplanimagesize")[0].naturalWidth);
				$("#floorplancontainer")[0].setAttribute('naturalHeight', $("#floorplanimagesize")[0].naturalHeight);
				$("#floorplancontainer")[0].setAttribute('svgwidth', $("#svgcontainer").width());
				var ratio = $("#floorplanimagesize")[0].naturalWidth / $("#floorplanimagesize")[0].naturalHeight;
				$("#floorplancontainer")[0].setAttribute('ratio', ratio);
				if ($("#svgcontainer").width() == 100) {
					svgHeight = $("#floorplancontainer").width() / ratio;  // FireFox specific
				} else {
					svgHeight = $("#svgcontainer").width() / ratio;
				}
				$("#floorplancontainer").height(svgHeight);
			}
		}

		RoomClick = function(click) {

			$('.DeviceDetails').css('display','none');   // hide all popups 
			if ($("#floorplangroup")[0].getAttribute("zoomed") == "true")
			{
				$("#floorplangroup")[0].setAttribute("transform", "translate(0,0) scale(1)");
				$("#DeviceContainer")[0].setAttribute("transform", "translate(0,0) scale(1)");
				$("#floorplangroup")[0].setAttribute("zoomed", "false");
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
				$("#floorplangroup")[0].setAttribute("transform", attr);
				$("#DeviceContainer")[0].setAttribute("transform", attr);
				$("#floorplangroup")[0].setAttribute("zoomed", "true");
			}
		}

		RefreshDevices = function() {

			if ((typeof $("#floorplangroup") != 'undefined') &&
				(typeof $("#floorplangroup")[0] != 'undefined')) {
			
				if (typeof $scope.mytimer != 'undefined') {
					$interval.cancel($scope.mytimer);
					$scope.mytimer = undefined;
				}

				if ($("#floorplangroup")[0].getAttribute("planidx") != "") {
					$http({
							 url: "json.htm?type=devices&filter=all&used=true&order=Name&floor=" + aFloorplans[window.myglobals.FloorplanIndex].idx + "&lastupdate=" + window.myglobals.LastUpdate
						}).success(function(data) {
							if (typeof data.ActTime != 'undefined') {
								window.myglobals.LastUpdate = data.ActTime;
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
										dev.htmlMinimum($("#roomplangroup")[0]);
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

		ShowDevices = function(idx, parent) {
			if (typeof $("#floorplangroup") != 'undefined') {

				$.ajax({
					 url: "json.htm?type=devices&filter=all&used=true&order=Name&floor="+idx,
					 async: false,
					 dataType: 'json',
					 success: function(data) {
						if (typeof data.ActTime != 'undefined') {
							window.myglobals.LastUpdate = data.ActTime;
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

				RefreshDevices();
			}
		}

		RefreshFloorPlan = function(floorIdx) {
			$("#roomplangroup").empty();
			if (typeof window.myglobals.FloorplanIndex == 'undefined') {
				$('.btnstyle').hide();
				$('.btnstylerev').hide();
			}
			
			if (floorIdx < 0) floorIdx = window.myglobals.FloorplanCount-1;
			if (floorIdx >= window.myglobals.FloorplanCount) floorIdx = 0;
			window.myglobals.FloorplanIndex = floorIdx;
			
			if (window.myglobals.FloorplanCount > 0) {
				$("#floorplanimage").attr("xlink:href", aFloorplans[floorIdx].Image);
				$("#floorplanimagesize").attr("src", aFloorplans[floorIdx].Image);
				Device.initialise();
				$.ajax({
					url: "json.htm?type=command&param=getfloorplanplans&idx=" + aFloorplans[floorIdx].idx, 
					async: false, 
					dataType: 'json',
					success: function(data) {
						if (typeof data.result != 'undefined') {
							var planGroup = $("#roomplangroup")[0];
							$.each(data.result, function(i,item) {
								var el = makeSVGnode('polygon', { id: item.Name + "_Room", 'class': 'hoverable', points: item.Area }, '');
								el.appendChild(makeSVGnode('title', null, item.Name));
								planGroup.appendChild(el);
							});
						}
					}
				});
				ShowDevices(aFloorplans[floorIdx].idx, $("#roomplangroup")[0]);
			}

						

			// no next/previous buttons for a single image or less
			if (window.myglobals.FloorplanCount > 1) {
				$('.btnstyle').show();
				$('.btnstylerev').show();
			}
			// better tell people why they got a blank page
			if (typeof window.myglobals.FloorplanCount == 'undefined') {
				$('#NoFloorplansYet').show();
			}
			else if (window.myglobals.FloorplanCount == 0) {
				$('#NoFloorplansYet').show();
			} else {
				$('#NoFloorplansYet').hide();
			}
		}

		ShowFloorplan = function(floorIdx) {
			var htmlcontent = '';
			htmlcontent+=$('#floorplanmain').html();
			$('#floorplancontent').html(htmlcontent);
			$('#floorplancontent').i18n();

			Device.useSVGtags = true;
			Device.backFunction = 'ShowFloorplan';
			Device.switchFunction = 'RefreshDevices';
			Device.contentTag = 'floorplancontent';

			var fidx=0;
			if (arguments.length != 0) {
				fidx=floorIdx;
			} else {
				if (typeof window.myglobals.FloorplanIndex != 'undefined') {
					fidx=window.myglobals.FloorplanIndex;
				}
			}
			RefreshFloorPlan(fidx);

			$('#roomplangroup')
				.off()
				.on('click', function ( event ) { RoomClick( event ); });
		}

		init();

		function init()
		{
			$('#modal').show();	

			//Get initial floorplans
			aFloorplans = {};	
			$http({
				url: "json.htm?type=floorplans"
				}).success(function(data) {
					if (typeof data.result != 'undefined') {
						window.myglobals.FloorplanCount = data.result.length;
						$.each(data.result, function(i,item) {
							aFloorplans[i] = item;
						});
						//Lets start
						ShowFloorplan();
					}
				});

			$('#modal').hide();
		};
		$scope.$on('$destroy', function(){
			if (typeof $scope.mytimer != 'undefined') {
				$interval.cancel($scope.mytimer);
				$scope.mytimer = undefined;
			}
		}); 
		
	} ]);
});