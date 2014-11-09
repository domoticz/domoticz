define(['app'], function (app) {
	app.controller('FloorplanController', [ '$scope', '$rootScope', '$location', '$window', '$http', '$interval', '$compile', 'permissions', function($scope,$rootScope,$location,$window,$http,$interval,$compile,permissions) {

		$scope.floorPlans;
		$scope.selectedFloorplan;
		$scope.FloorplanCount;
		$scope.prevFloorplan;
		$scope.actFloorplan;
		$scope.browser="unknown";
		
		$scope.animateMove = function() {
			var element = $("#FPFloorplangroup")[0];
			var oTransform = new Transform(element);
			if (element.style.paddingLeft != '') {
				oTransform.xOffset = parseInt(element.style.paddingLeft.replace("px", "").replace(";", ""))*-1;
			}
			if (element.style.paddingTop != '') {
				oTransform.yOffset = parseInt(element.style.paddingTop.replace("px", "").replace(";", ""))*-1;
			}
			if (element.style.paddingRight != '') {
				oTransform.scale = parseFloat(element.style.paddingRight.replace("px", "").replace(";", ""));
			}
			$("#FPFloorplangroup")[0].setAttribute('transform', oTransform.toString());
		}
		
		$scope.animateView = function() {
			$("#svgcontainer2")[0].setAttribute("viewBox", '0 0 '+$("#svgcontainer2").css('textIndent').replace("px", "")+' '+parseInt($("#svgcontainer2").css('lineHeight').replace("px", "")));
		}
		
		$scope.switchFloorplan = function(animating) {
				$scope.resetFloorplan();
				$scope.selectedFloorplan = $scope.floorPlans[$scope.actFloorplan];
				if (animating == true) {
					$(".FloorContent").attr('style','display:none'); // hide devices otherwise they will move around during animation
					var ratio = $scope.selectedFloorplan.xImageSize / $scope.selectedFloorplan.yImageSize;
					var svgHeight = $window.innerHeight - $("#floorplancontainer").offset().top - $("#copyright").height();
					var svgWidth = svgHeight * ratio;
					if (svgWidth > $("#floorplancontent").width()) {
						svgWidth = $("#floorplancontent").width();
						svgHeight = svgWidth / ratio;
					}
					$("#svgcontainer2").css('textIndent',$scope.floorPlans[$scope.prevFloorplan].xImageSize+"px");
					$("#svgcontainer2").css('lineHeight',$scope.floorPlans[$scope.prevFloorplan].yImageSize+"px");
					$("#floorplancontainer").attr('ratio', ratio); // just for debug
					
					// Animate the various components to get to target state - for SVG use CSS attributes that are meaningless as placeholders because JQuery doesn't animate SVG
					$("#floorplancontainer").animate({width:svgWidth, height:svgHeight}, 
													 {duration: 400, easing:'easeInQuint', 
													  always:function(){$(".FloorContent").attr("style",""); $scope.resetFloorplan();}});
					$("#FPFloorplangroup").css('paddingLeft',$scope.floorPlans[$scope.prevFloorplan].xOffset)
										  .animate({'paddingLeft':$scope.selectedFloorplan.xOffset},
												   {duration:400, progress:function(animation, progress, remainingMs){$scope.animateMove();}});
					$("#svgcontainer2").animate({width:"95%"},  {duration: 200, easing:'easeOutQuint'})
									   .animate({width:"100%"}, {duration: 200, easing:'easeInQuint'});
					$("#svgcontainer2").animate({textIndent:$scope.selectedFloorplan.xImageSize+"px", 
												 lineHeight:$scope.selectedFloorplan.yImageSize+"px"},
												{duration:400, queue:false, easing:'easeOutQuint', progress:function(animation, progress, remainingMs){$scope.animateView();}});
				}
				else {
					$scope.resetFloorplan();
					$scope.SVGContainerResize();
					RefreshDevices();
				}
		}

		$scope.resetFloorplan = function() {
				$("#FPFloorplangroup").attr('zoomed', 'false').css('paddingTop','').css('paddingLeft','').css('paddingRight','');
				$("#FPFloorplangroup").attr("transform", "translate("+-($scope.selectedFloorplan.xOffset)+",0) scale(1)");
				$("#svgcontainer2")[0].setAttribute("viewBox","0 0 "+$scope.selectedFloorplan.xImageSize+" "+$scope.selectedFloorplan.yImageSize);
				$("#svgcontainer2").width("100%");
		}

		$scope.NextFloorplan= function() {
			if ($scope.actFloorplan < ($scope.FloorplanCount-1)) {
				$scope.prevFloorplan = $scope.actFloorplan;
				$scope.actFloorplan += 1;
				$scope.switchFloorplan($.myglobals.AnimateTransitions);
			}
		}

		$scope.PrevFloorplan = function() {
			if ($scope.actFloorplan > 0) {
				$scope.prevFloorplan = $scope.actFloorplan;
				$scope.actFloorplan -= 1;
				$scope.switchFloorplan($.myglobals.AnimateTransitions);
			}
		}

		$scope.changeFloorplan = function() {
			for (var i = 0, len = $scope.floorPlans.length; i < len; i++) {
			  if ($scope.floorPlans[i].Name == $scope.selectedFloorplan.Name) {
				$scope.prevFloorplan = $scope.actFloorplan;
				$scope.actFloorplan = i;
				$scope.switchFloorplan();
				break;
			  }
			}
		}

		ImageLoaded = function(tagName) {
			var node=document.getElementById(tagName);
			if (node != null) {
				var target=document.getElementById(node.getAttribute("related"));
				if (target != null) {
					target.setAttribute("xImageSize", node.naturalWidth);
					target.setAttribute("yImageSize", node.naturalHeight);
					node.parentNode.removeChild(node);
					for (var i = 0, len = $scope.floorPlans.length; i < len; i++) {
						if ($scope.floorPlans[i].idx == target.getAttribute("index")) {
							$scope.floorPlans[i].xImageSize = node.naturalWidth;
							$scope.floorPlans[i].yImageSize = node.naturalHeight;
							break;
						}
					}
					var xOffset = 0;
					for (var j = 0, len = $scope.floorPlans.length; j < len; j++) {
						$scope.floorPlans[j].xOffset = xOffset;
						xOffset += $scope.floorPlans[j].xImageSize + 100;
						var tag=document.getElementById($scope.floorPlans[j].tagName);
						if (tag != null) {
							tag.setAttribute("transform","translate("+$scope.floorPlans[j].xOffset+",0)");
						}
					}
					if (i<$scope.floorPlans.length) {
						target.insertBefore(makeSVGnode('image', { width:"100%", height:"100%", "xlink:href": $scope.floorPlans[i].Image }, ''),target.firstChild);
					}
					if ($scope.selectedFloorplan.idx == target.getAttribute("index")) {
						$("#svgcontainer2")[0].setAttribute("viewBox","0 0 "+node.naturalWidth+" "+node.naturalHeight);
						$scope.SVGContainerResize();
						$("#svgcontainer2")[0].setAttribute("style","display:inline");
					}
				}
			}
		}

		$scope.SVGContainerResize = function() {
			if (typeof $("#floorplancontainer") != 'undefined') {
				Device.xImageSize = $scope.floorPlans[$scope.actFloorplan].xImageSize;
				Device.yImageSize = $scope.floorPlans[$scope.actFloorplan].yImageSize;
				var ratio = Device.xImageSize / Device.yImageSize;
				var svgHeight = $window.innerHeight - $("#floorplancontainer").offset().top - $("#copyright").height();
				var svgWidth = svgHeight * ratio;
				if (svgWidth > $("#floorplancontent").width()) {
					svgWidth = $("#floorplancontent").width();
					svgHeight = svgWidth / ratio;
				}
				$("#floorplancontainer")[0].setAttribute('ratio', ratio);
				$("#floorplancontainer").width(svgWidth);
				$("#floorplancontainer").height(svgHeight);
			}
		}

		RoomClick = function(click) {
			$('.DeviceDetails').css('display','none');   // hide all popups 
			var borderRect = click.target.getBBox(); // polygon bounding box
			var margin = 0.1;  // 10% margin around polygon
			var marginX = borderRect.width * margin;
			var marginY = borderRect.height * margin;
			var scaleX= $scope.selectedFloorplan.xImageSize / (borderRect.width + (marginX * 2));
			var scaleY= $scope.selectedFloorplan.yImageSize / (borderRect.height + (marginY * 2));
			var scale = ((scaleX > scaleY) ? scaleY : scaleX);
			if ($("#FPFloorplangroup").attr("zoomed") == "true")
			{
				if ($.myglobals.AnimateTransitions) {
					$("#FPFloorplangroup").css('paddingTop',(borderRect.y - marginY)).css('paddingLeft',(($scope.selectedFloorplan.xOffset) + (borderRect.x - marginX))).css('paddingRight',scale)
										  .animate({paddingTop: 0,
													paddingLeft:$scope.selectedFloorplan.xOffset,
													paddingRight: 1.0},
												   {duration:300,
												    progress:function(animation, progress, remainingMs){$scope.animateMove();},
													always:function(){
														$("#FPFloorplangroup").attr("zoomed", "false")
																			  .css('paddingTop','').css('paddingLeft','').css('paddingRight','')
																			  .attr("transform", 'scale(1) translate('+-$scope.selectedFloorplan.xOffset+',0)');}});
				}
				else {
					$scope.resetFloorplan();
				}
			}
			else
			{
				var attr = 'scale('  + scale + ')' + ' translate(' +(($scope.selectedFloorplan.xOffset) + (borderRect.x - marginX))*-1 + ',' + (borderRect.y - marginY)*-1 + ')';

				if ($.myglobals.AnimateTransitions) {
					$("#FPFloorplangroup").css('paddingTop',0).css('paddingLeft',$scope.selectedFloorplan.xOffset).css('paddingRight',1)
										  .animate({paddingTop: (borderRect.y - marginY),
													paddingLeft:(($scope.selectedFloorplan.xOffset) + (borderRect.x - marginX)),
													paddingRight: scale},
												   {duration:300,
												    progress:function(animation, progress, remainingMs){$scope.animateMove();},
													always:function(){
														$("#FPFloorplangroup").attr("zoomed", "true")
																			  .css('paddingTop','').css('paddingLeft','').css('paddingRight','')
																			  .attr("transform", attr);}});
				}
				else {
					// this actually needs to centre in the direction its not scaling but close enough for v1.0
					$("#FPFloorplangroup")[0].setAttribute("transform", attr);
					$("#FPFloorplangroup").attr("zoomed", "true");
				}
			}
		}

		RefreshDevices = function() {
			if (typeof $scope.mytimer != 'undefined') {
				$interval.cancel($scope.mytimer);
				$scope.mytimer = undefined;
			}

			if ((typeof $("#FPFloorplangroup") != 'undefined') &&
				(typeof $("#FPFloorplangroup")[0] != 'undefined')) {
			
				$http({
						 url: "json.htm?type=devices&filter=all&used=true&order=Name&floor=" + $scope.floorPlans[$scope.actFloorplan].idx + "&lastupdate=" + $scope.floorPlans[$scope.actFloorplan].lastUpdateTime
					}).success(function(data) {
						if (typeof data.ActTime != 'undefined') {
							$scope.floorPlans[$scope.actFloorplan].lastUpdateTime = data.ActTime;
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
										var dev = Device.create(item);
										var existing = document.getElementById(dev.uniquename);
										if (existing != undefined) {
											dev.htmlMinimum($("#FPRoomplanGroup")[0]);
										}
									}
								} else {
									var dev = Device.create(item);
									var existing = document.getElementById(dev.uniquename);
									if (existing != undefined) {
										dev.htmlMinimum($("#FPRoomplanGroup")[0]);
									}
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

		$scope.ShowDevices = function(idx, xOffset, parent) {
			if (typeof $("#FPFloorplangroup") != 'undefined') {

				$.ajax({
					 url: "json.htm?type=devices&filter=all&used=true&order=Name&floor="+idx,
					 async: false,
					 dataType: 'json',
					 success: function(data) {
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
										if (dev.onFloorplan == true) {
											dev.xoffset += xOffset;
											dev.htmlMinimum(parent);
										}
									}
								} else {
									dev = Device.create(item);
									if (dev.onFloorplan == true) {
										dev.xoffset += xOffset;
										dev.htmlMinimum(parent);
									}
								}
							});
						}
					}
				});
			}
		}

		ShowFloorplans = function(floorIdx) {
			if (typeof $scope.mytimer != 'undefined') {
				$interval.cancel($scope.mytimer);
				$scope.mytimer = undefined;
			}

			var htmlcontent = "";
			htmlcontent+=$('#fpmaincontent').html();
			$('#floorplancontent').html($compile(htmlcontent)($scope));
			$('#floorplancontent').i18n();

			Device.useSVGtags = true;
			Device.backFunction = 'ShowFloorplans';
			Device.switchFunction = 'ShowFloorplans';
			Device.contentTag = 'floorplancontent';
			Device.xImageSize = 1280;
			Device.yImageSize = 720;
			Device.checkDefs();
			$("#FPFloorplanGroup").empty();
			
			//Get initial floorplans
			$http({ url: "json.htm?type=floorplans"}).success(function(data) {
				if (typeof data.result != 'undefined') {
					if (typeof $scope.actFloorplan == "undefined") {
						$scope.FloorplanCount = data.result.length;
						$scope.floorPlans = data.result;
						$scope.selectedFloorplan = $scope.floorPlans[0];
						$scope.actFloorplan = 0;
						$scope.browser="unknown";
					}
			
					// handle settings
					if (typeof data.AnimateZoom != 'undefined') {
						$.myglobals.AnimateTransitions = (data.AnimateZoom != 0);
					}
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
					var elFloor;
					$.each(data.result, function(i,item) {
						var tagName = item.Name.replace(' ','_')+"_"+i;
						if ($scope.floorPlans[i].tagName != tagName) {
							$scope.floorPlans[i].xOffset = -1;
							$scope.floorPlans[i].xImageSize = 0;
							$scope.floorPlans[i].yImageSize = 0;
							$scope.floorPlans[i].lastUpdateTime = 0;
							$scope.floorPlans[i].tagName = tagName;
							var node=document.getElementById("floorplanimagesize").cloneNode(true);
							node.setAttribute("src", item.Image);
							node.setAttribute("related", $scope.floorPlans[i].tagName);
							node.setAttribute("id", i+"_FloorGroupImage");
							node.setAttribute("onload", "ImageLoaded('"+i+"_FloorGroupImage');");
							$("#floorplancontainer")[0].appendChild(node);
							elFloor = makeSVGnode('g', { id: $scope.floorPlans[i].tagName, index:item.idx, order:item.Order, transform: 'translate(0,0)' }, '');
						}
						else { // floorplan data is already loaded and matches what server has for this floor
							elFloor = makeSVGnode('g', { id: $scope.floorPlans[i].tagName,
														 index: $scope.floorPlans[i].idx, 
														 order: $scope.floorPlans[i].Order, 
														 xImageSize: $scope.floorPlans[i].xImageSize, 
														 yImageSize: $scope.floorPlans[i].yImageSize, 
														 transform: 'translate('+$scope.floorPlans[i].xOffset+',0)' }, '');
							elFloor.appendChild(makeSVGnode('image', { width:"100%", height:"100%", "xlink:href": $scope.floorPlans[i].Image }, ''));
							if ($scope.actFloorplan == i) {
								$scope.switchFloorplan();
								$("#svgcontainer2")[0].setAttribute("style","display:inline");
							}
						}
						$("#FPFloorplangroup")[0].appendChild(elFloor);
						var elContent = makeSVGnode('g', { id: $scope.floorPlans[i].tagName+'_Content', 'class':'FloorContent' }, '');
						elFloor.appendChild(elContent);
						var elRooms = makeSVGnode('g', { id: $scope.floorPlans[i].tagName+'_Rooms', 'class':'FloorRooms' }, ''); // onclick="RoomClick(event);"
						elContent.appendChild(elRooms);
						var elDevices = makeSVGnode('g', { id: $scope.floorPlans[i].tagName+'_Devices' }, '');
						elContent.appendChild(elDevices);
						$.ajax({
							url: "json.htm?type=command&param=getfloorplanplans&idx=" + item.idx,
							async: false, 
							dataType: 'json',
							success: function(data) {
								if (typeof data.result != 'undefined') {
									var planGroup = $("#FPRoomplanGroup")[0];
									$.each(data.result, function(i,item) {
										var el = makeSVGnode('polygon', { id: item.Name + "_Room", 'class': 'hoverable', points: item.Area }, '');
										el.appendChild(makeSVGnode('title', null, item.Name));
										elRooms.appendChild(el);
									});
								}
							}
						});
						var elIcons = makeSVGnode('g', { id: 'DeviceIcons' }, '');
						elDevices.appendChild(elIcons);
						var elDetails = makeSVGnode('g', { id: 'DeviceDetails' }, '');
						elDevices.appendChild(elDetails);
						$scope.ShowDevices(item.idx, 0, elDevices);
						elIcons.setAttribute("id", $scope.floorPlans[i].tagName+'_Icons');
						elDetails.setAttribute("id", $scope.floorPlans[i].tagName+'_Details');
					});
				}
				$(".FloorRooms").click(RoomClick);
			});

			// Initial refresh needs to be short, this will trigger the status elements to be drawn
			if (typeof $scope.mytimer == 'undefined') {
				$scope.mytimer=$interval(function() { RefreshDevices(); }, 500);
			}

			if ((typeof $.myglobals.RoomColour != 'undefined') && 
				(typeof $.myglobals.InactiveRoomOpacity != 'undefined')) {
				$(".hoverable").css({'fill': $.myglobals.RoomColour, 'fill-opacity': $.myglobals.InactiveRoomOpacity/100});
			}
			if (typeof $.myglobals.ActiveRoomOpacity != 'undefined') {
				$(".hoverable").hover(function(){$(this).css({'fill-opacity': $.myglobals.ActiveRoomOpacity/100})},
									  function(){$(this).css({'fill-opacity': $.myglobals.InactiveRoomOpacity/100})});
			}

			$( window ).resize(function() { $scope.SVGContainerResize(); });
		}

		init();

		function init()
		{
			try {
				ShowFloorplans();
			}
			catch(err) {
				alert( "Error: " + err + ".");
			}
		};
		$scope.$on('$destroy', function(){
			if (typeof $scope.mytimer != 'undefined') {
				$interval.cancel($scope.mytimer);
				$scope.mytimer = undefined;
			}
		}); 
		
	} ]);
});