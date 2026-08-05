define(['app', 'livesocket'], function (app) {
	app.controller('FloorplanController', function ($scope, $rootScope, $location, $window, $http, $interval, $compile, permissions, livesocket) {

		$scope.floorPlans;
		$scope.FloorplanCount;
		$scope.actFloorplan;
		$scope.browser = "unknown";
		$scope.lastUpdateTime = 0;
		$scope.isScrolling = false;		// used on tablets & phones
		$scope.lastTouch = 0;			// used on tablets & phones
		$scope.lastTouchX = 0;			// used on tablets & phones
		$scope.lastTouchY = 0;			// used on tablets & phones
		$scope.touchStartX = 0;			// used on tablets & phones
		$scope.touchStartY = 0;			// used on tablets & phones
		var refreshDebounceTimer = null;

		// Returns the actual bottom of the visible navbar content in viewport pixels.
		// The brand logo image overflows below the navbar's own layout box, so we
		// check all img elements inside .brand and use the deepest one found.
		function getNavbarBottom() {
			var nav = document.querySelector('.navbar.navbar-fixed-top');
			if (!nav) { return 0; }
			var bottom = nav.getBoundingClientRect().bottom;
			var imgs = nav.querySelectorAll('.brand img');
			for (var i = 0; i < imgs.length; i++) {
				var b = imgs[i].getBoundingClientRect().bottom;
				if (b > bottom) { bottom = b; }
			}
			return Math.ceil(bottom);
		}

		$scope.makeHTMLnode = function (tag, attrs) {
			var el = document.createElement(tag);
			for (var k in attrs) el.setAttribute(k, attrs[k]);
			return el;
		}

		function FPtouchstart(e) {
			$scope.isScrolling = false;
			var touch = e.changedTouches ? e.changedTouches[0] : null;
			$scope.touchStartX = touch ? touch.pageX : 0;
			$scope.touchStartY = touch ? touch.pageY : 0;
		};
		function FPtouchmove(e) { $scope.isScrolling = true; };
		function FPtouchend(e) {
			if (e.target.getAttribute('related') != null) {
				$("#BulletImages").children().css({ 'display': 'none' });
				e.preventDefault();
				ScrollFloorplans(e.target.getAttribute('related'));
				return;
			}

			var touch = e.changedTouches ? e.changedTouches[0] : null;
			if (!touch) return;

			var endX   = touch.pageX;
			var endY   = touch.pageY;
			var deltaX = endX - $scope.touchStartX;
			var deltaY = endY - $scope.touchStartY;

			var SWIPE_THRESHOLD = 50;

			if ($scope.isScrolling && Math.abs(deltaX) > SWIPE_THRESHOLD && Math.abs(deltaX) > Math.abs(deltaY)) {
				if (deltaX < 0 && $scope.actFloorplan < $scope.FloorplanCount - 1) {
					$scope.actFloorplan++;
					ScrollFloorplans($scope.floorPlans[$scope.actFloorplan].tagName);
				} else if (deltaX > 0 && $scope.actFloorplan > 0) {
					$scope.actFloorplan--;
					ScrollFloorplans($scope.floorPlans[$scope.actFloorplan].tagName);
				}
			} else if (!$scope.isScrolling) {
				var timeDelta = (new Date()).getTime() - $scope.lastTouch;
				var dx = endX - $scope.lastTouchX;
				var dy = endY - $scope.lastTouchY;
				var distance = Math.sqrt(dx * dx + dy * dy);
				if (timeDelta < 500 && timeDelta > 0 && distance < 50) {
					$scope.doubleClick();
				}
				$scope.lastTouch  = (new Date()).getTime();
				$scope.lastTouchX = endX;
				$scope.lastTouchY = endY;
			}
			$scope.isScrolling = false;
		};

		ScrollFloorplans = function (tagName, animate) {
			var allowAnimation = $.myglobals.AnimateTransitions;
			if (arguments.length > 1) {
				allowAnimation = animate;
			}
			var target = document.getElementById(tagName);
			if (target != null) {
				for (var i = 0, len = $scope.floorPlans.length; i < len; i++) {
					if ($scope.floorPlans[i].idx == target.getAttribute("index")) {
						if (allowAnimation == false) {
							window.scrollTo($("#floorplancontent").width() * i, 0);
						} else {
							var from = { property: window.pageXOffset };  // starting position is current scrolled amount
							var to = { property: $("#floorplancontent").width() * i };
							jQuery(from).animate(to, { duration: 500, easing: 'easeOutQuint', step: function (val) { window.scrollTo(val, 0); } });
						}
						$(".bulletSelected").attr('class', 'bullet');
						var bullet = $(".bullet")[i];
						if (bullet !== undefined) {
							bullet.setAttribute('class', 'bulletSelected');
						}
						$scope.actFloorplan = i;
						updateNavArrows();
						break;
					}
				}
			}
			$("#copyright").attr("style", "position:fixed");
		}

		function updateNavArrows() {
			if ($scope.FloorplanCount <= 1) return;
			$("#fpNavLeft").css('display', $scope.actFloorplan > 0 ? 'flex' : 'none');
			$("#fpNavRight").css('display', $scope.actFloorplan < $scope.FloorplanCount - 1 ? 'flex' : 'none');
		}

		function FPkeydown(e) {
			if ($scope.FloorplanCount <= 1) return;
			if (e.keyCode === 37 && $scope.actFloorplan > 0) {
				$scope.actFloorplan--;
				ScrollFloorplans($scope.floorPlans[$scope.actFloorplan].tagName);
			} else if (e.keyCode === 39 && $scope.actFloorplan < $scope.FloorplanCount - 1) {
				$scope.actFloorplan++;
				ScrollFloorplans($scope.floorPlans[$scope.actFloorplan].tagName);
			}
		}

		ImgLoaded = function (tagName) {
			var target = document.getElementById(tagName + '_img');
			if (target != null) {
				for (var i = 0, len = $scope.floorPlans.length; i < len; i++) {
					if ($scope.floorPlans[i].idx == target.parentNode.getAttribute("index")) {
						$scope.floorPlans[i].xImageSize = target.naturalWidth;
						$scope.floorPlans[i].yImageSize = target.naturalHeight;
						$scope.floorPlans[i].Loaded = true;
						// Image has loaded, now load it into SVG
						var svgCont = document.getElementById(tagName + '_svg');
						svgCont.setAttribute("viewBox", "0 0 " + target.naturalWidth + " " + target.naturalHeight);
						svgCont.appendChild(makeSVGnode('g', { id: tagName + '_grp', transform: 'translate(0,0) scale(1)', style: "", zoomed: "false" }, ''));
						//svgCont.childNodes[0].appendChild(makeSVGnode('rect', { width: "100%", height: "100%", fill: "white" }, ''));
						svgCont.childNodes[0].appendChild(makeSVGnode('image', { width: "100%", height: "100%", "xlink:href": $scope.floorPlans[i].Image }, ''));
						svgCont.childNodes[0].appendChild(makeSVGnode('g', { id: tagName + '_Content', 'class': 'FloorContent' }, ''));
						svgCont.childNodes[0].appendChild(makeSVGnode('g', { id: tagName + '_Rooms', 'class': 'FloorRooms' }, ''));
						svgCont.childNodes[0].appendChild(makeSVGnode('g', { id: tagName + '_Devices', transform: 'scale(1)' }, ''));
						svgCont.setAttribute("style", "display:inline; position: relative;");
						target.parentNode.removeChild(target);
						$scope.ShowRooms(i);
						break;
					}
				}
				// kick off periodic refresh once all images have loaded
				var AllLoaded = true;
				for (var i = 0, len = $scope.floorPlans.length; i < len; i++) {
					if ($scope.floorPlans[i].Loaded != true) {
						AllLoaded = false;
						break;
					}
				}
				if (AllLoaded == true) {
					Device.checkDefs();
					$(".FloorRooms").click(function (event) { $scope.RoomClick(event) });
					RefreshFPDevices();
					$scope.FloorplanResize();
				}
			}
			else generate_noty('error', '<b>ImageLoaded Error</b><br>Element not found: ' + tagName, false);
		}

		$scope.GetFloorplanIdx = function (floorID) {
			for (i = 0; i < $scope.floorPlans.length; i++) { 
				if ($scope.floorPlans[i].floorID == floorID) {
					return i;
				}
			}
			return -1;
		}

		$scope.FloorplanResize = function () {
			if (typeof $("#floorplancontent") != 'undefined') {
				var wrpHeight = $window.innerHeight;
				// when the small menu bar is displayed main-view jumps to the top so force it down
				if ($(".navbar").css('display') != 'none') {
					var navBottom = getNavbarBottom();
					$("#floorplancontent").offset({ top: navBottom });
					var copyrightH = ($("#copyright").css('display') == 'none') ? 0 : ($("#copyright").outerHeight() || 0);
					wrpHeight = $window.innerHeight - navBottom - copyrightH;
				}
				else {
					$("#floorplancontent").offset({ top: 0 });
				}
				$("#floorplancontent").width($("#main-view").width()).height(wrpHeight);
				$(".imageparent").each(function (i) { $("#" + $(this).attr('id') + '_svg').width($("#floorplancontent").width()).height(wrpHeight); });
				if ($scope.FloorplanCount > 1) {
					$("#BulletGroup:first").css("left", ($window.innerWidth - $("#BulletGroup:first").width()) / 2)
						.css("bottom", ($("#copyright").css('display') == 'none') ? 10 : $("#copyright").height() + 10)
						.css("display", "inline");
				}
				if (typeof $scope.actFloorplan != 'undefined') ScrollFloorplans($scope.floorPlans[$scope.actFloorplan].tagName, false);
			}
		}

		$scope.animateMove = function (svgFloor) {
			var element = $("#" + svgFloor)[0];
			var oTransform = new Transform(element);
			if (element.style.paddingLeft != '') {
				oTransform.xOffset = parseInt(element.style.paddingLeft.replace("px", "").replace(";", "")) * -1;
			}
			if (element.style.paddingTop != '') {
				oTransform.yOffset = parseInt(element.style.paddingTop.replace("px", "").replace(";", "")) * -1;
			}
			if (element.style.paddingRight != '') {
				oTransform.scale = parseFloat(element.style.paddingRight.replace("px", "").replace(";", ""));
			}
			$("#" + svgFloor)[0].setAttribute('transform', oTransform.toString());
		}

		$scope.doubleClick = function () {
			if ($.myglobals.FullscreenMode == true) {
				if ($("#fpwrapper").attr('fullscreen') != 'true') {
					$('#copyright').css({ display: 'none' });
					$('.navbar').css({ display: 'none' });
					$("#fpwrapper").css({ position: 'absolute', top: 0, left: 0 }).attr('fullscreen', 'true');
				} else {
					$('#copyright').css({ display: 'block' });
					$('.navbar').css({ display: 'block' });
					$("#fpwrapper").css({ position: 'relative' }).attr('fullscreen', 'false');
				}
				$scope.FloorplanResize();
			}
			else {
				$.cachenoty = generate_noty('warning', '<b>' + $.t('Fullscreen mode is disabled') + '</b>', 3000);
			}
		}

		$scope.RoomClick = function (click) {
			$('.DeviceDetails').css('display', 'none');   // hide all popups
			var borderRect = click.target.getBBox(); // polygon bounding box
			var margin = 0.1;  // 10% margin around polygon
			var marginX = borderRect.width * margin;
			var marginY = borderRect.height * margin;
			var scaleX = $scope.floorPlans[$scope.actFloorplan].xImageSize / (borderRect.width + (marginX * 2));
			var scaleY = $scope.floorPlans[$scope.actFloorplan].yImageSize / (borderRect.height + (marginY * 2));
			var scale = ((scaleX > scaleY) ? scaleY : scaleX);

			var svgFloor = click.target.parentNode.parentNode.getAttribute("id");
			if ($("#" + svgFloor).attr('zoomed') == "true") {
				if ($.myglobals.AnimateTransitions) {
					$("#" + svgFloor).css('paddingTop', (borderRect.y - marginY)).css('paddingLeft', (borderRect.x - marginX)).css('paddingRight', scale)
						.animate({
							paddingTop: 0,
							paddingLeft: 0,
							paddingRight: 1.0
						},
						{
							duration: 300,
							progress: function (animation, progress, remainingMs) { $scope.animateMove(svgFloor); },
							always: function () {
								$("#" + svgFloor).attr("zoomed", "false")
									.css('paddingTop', '').css('paddingLeft', '').css('paddingRight', '')
									.attr("transform", 'translate(0,0) scale(1)');
							}
						});
				}
				else {
					$("#" + svgFloor).attr('zoomed', 'false')
						.css('paddingTop', '').css('paddingLeft', '').css('paddingRight', '')
						.attr("transform", "translate(0,0) scale(1)");
				}
			}
			else {
				var attr = 'scale(' + scale + ')' + ' translate(' + (borderRect.x - marginX) * -1 + ',' + (borderRect.y - marginY) * -1 + ')';

				if ($.myglobals.AnimateTransitions) {
					$("#" + svgFloor).css('paddingTop', 0).css('paddingLeft', 0).css('paddingRight', 1)
						.animate({
							paddingTop: (borderRect.y - marginY),
							paddingLeft: (borderRect.x - marginX),
							paddingRight: scale
						},
						{
							duration: 300,
							progress: function (animation, progress, remainingMs) { $scope.animateMove(svgFloor); },
							always: function () {
								$("#" + svgFloor).attr("zoomed", "true")
									.css('paddingTop', '').css('paddingLeft', '').css('paddingRight', '')
									.attr("transform", attr);
							}
						});
				}
				else {
					// this actually needs to centre in the direction its not scaling but close enough for v1.0
					$("#" + svgFloor)[0].setAttribute("transform", attr);
					$("#" + svgFloor).attr("zoomed", "true");
				}
			}
		}

		RefreshItem = function (item) {
			var compoundDevice = (item.Type.indexOf('+') >= 0);
			$(".Device_" + item.idx).each(function () {
				var aSplit = $(this).attr("id").split("_");
				item.FloorID = aSplit[1];
				item.PlanID = aSplit[2];
				item.Scale = $scope.floorPlans[$scope.GetFloorplanIdx(item.FloorID)].scaleFactor;
				if (compoundDevice) {
					item.Type = aSplit[0];					// handle multi value sensors (Baro, Humidity, Temp etc)
					item.CustomImage = 1;
					item.Image = aSplit[0].toLowerCase();
				}
				try {
					var dev = Device.create(item);
					var existing = document.getElementById(dev.uniquename);
					if (existing != undefined) {
						dev.htmlMinimum(existing.parentNode);
					}
				}
				catch (err) {
					$.cachenoty = generate_noty('error', '<b>Device refresh error</b> ' + dev.name + '<br>' + err, 5000);
				}
			});
		}

		DebouncedRefreshFPDevices = function () {
			if (refreshDebounceTimer) {
				clearTimeout(refreshDebounceTimer);
			}
			refreshDebounceTimer = setTimeout(function () {
				refreshDebounceTimer = null;
				RefreshFPDevices();
			}, 100);
		};

		RefreshFPDevices = function () {
			if (typeof $scope.mytimer != 'undefined') {
				$interval.cancel($scope.mytimer);
				$scope.mytimer = undefined;
			}

			livesocket.getJson("json.htm?type=command&param=getdevices&filter=all&used=true&order=Name&lastupdate=" + $.LastUpdateTime, function (data) {
				if (typeof data.ServerTime != 'undefined') {
					$rootScope.SetTimeAndSun(data.Sunrise, data.Sunset, data.ServerTime);
				}

				if (typeof data.result != 'undefined') {
					if (typeof data.ActTime != 'undefined') {
						$scope.lastUpdateTime = data.ActTime;
					}

					/*
						Render all the widgets at once.
					*/
					$.each(data.result, function (i, item) {
						RefreshItem(item);
					});
				}
			});
		};

		$scope.ShowFPDevices = function (floorIdx) {

			$http({
				url: "json.htm?type=command&param=getdevices&filter=all&used=true&order=Name&floor=" + $scope.floorPlans[floorIdx].idx
			}).then(function successCallback(response) {
				var data = response.data;
				if ((typeof data.ActTime != 'undefined') && ($scope.lastUpdateTime == 0)) {
					$scope.lastUpdateTime = data.ActTime;
				}
				// insert devices into the document
				var dev;
				if (typeof data.result != 'undefined') {
					var elDevices = document.getElementById($scope.floorPlans[floorIdx].tagName + '_Devices');
					var elIcons = makeSVGnode('g', { id: 'DeviceIcons' }, '');
					elDevices.appendChild(elIcons);
					var elDetails = makeSVGnode('g', { id: 'DeviceDetails' }, '');
					elDevices.appendChild(elDetails);
					$.each(data.result, function (i, item) {
						item.Scale = $scope.floorPlans[floorIdx].scaleFactor;
						item.FloorID = $scope.floorPlans[floorIdx].floorID;
						if (item.Type.indexOf('+') >= 0) {
							var aDev = item.Type.split('+');
							var k;
							for (k = 0; k < aDev.length; k++) {
								var sDev = aDev[k].trim();
								item.Name = ((k == 0) ? item.Name : sDev);
								item.Type = sDev;
								item.CustomImage = 1;
								item.Image = sDev.toLowerCase();
								item.XOffset = Math.abs(item.XOffset) + ((k == 0) ? 0 : (50 * $scope.floorPlans[floorIdx].scaleFactor));
								dev = Device.create(item);
								var existing = document.getElementById(dev.uniquename);
								if (dev.onFloorplan == true) {
									try {
										dev.htmlMinimum(existing);
									}
									catch (err) {
										generate_noty('error', '<b>Device draw error</b><br>' + err, false);
									}
								}
							}
						}
						else {
							dev = Device.create(item);
							var existing = document.getElementById(dev.uniquename);
							if (dev.onFloorplan == true) {
								try {
									dev.htmlMinimum(existing);
								}
								catch (err) {
									generate_noty('error', '<b>Device draw error</b><br>' + err, false);
								}
							}
						}
					});
					elIcons.setAttribute("id", $scope.floorPlans[floorIdx].tagName + '_Icons');
					elDetails.setAttribute("id", $scope.floorPlans[floorIdx].tagName + '_Details');
				}
			});
		}

		$scope.ShowRooms = function (floorIdx) {
			$http({
				url: "json.htm?type=command&param=getfloorplanplans&idx=" + $scope.floorPlans[floorIdx].idx
			}).then(function successCallback(response) {
				var data = response.data;
				if (typeof data.result != 'undefined') {
					$.each(data.result, function (i, item) {
						$("#" + $scope.floorPlans[floorIdx].tagName + '_Rooms').append(makeSVGnode('polygon', { id: item.Name, 'class': 'hoverable', points: item.Area }, item.Name));
					});
					if ((typeof $.myglobals.RoomColour != 'undefined') &&
						(typeof $.myglobals.InactiveRoomOpacity != 'undefined')) {
						$(".hoverable").css({ 'fill': $.myglobals.RoomColour, 'fill-opacity': $.myglobals.InactiveRoomOpacity / 100 });
					}
					if (typeof $.myglobals.ActiveRoomOpacity != 'undefined') {
						$(".hoverable").hover(function () { $(this).css({ 'fill-opacity': $.myglobals.ActiveRoomOpacity / 100 }) },
							function () { $(this).css({ 'fill-opacity': $.myglobals.InactiveRoomOpacity / 100 }) });
					}
				}
			});

			$scope.ShowFPDevices(floorIdx);
		}

		ShowFloorplans = function () {
			if (typeof $scope.mytimer != 'undefined') {
				$interval.cancel($scope.mytimer);
				$scope.mytimer = undefined;
			}

			var htmlcontent = "";
			htmlcontent += $('#fphtmlcontent').html();
			$('#floorplancontent').html($compile(htmlcontent)($scope));
			$('#floorplancontent').i18n();

			Device.useSVGtags = true;
			Device.backFunction = 'ShowFloorplans';
			Device.switchFunction = DebouncedRefreshFPDevices;
			Device.contentTag = 'floorplancontent';
			Device.xImageSize = 1280;
			Device.yImageSize = 720;
			Device.checkDefs();

			//Get initial floorplans
			$http({
				url: "json.htm?type=command&param=getfloorplans", async: false
			}).then(function successCallback(response) {
				var data = response.data;
				if (typeof data.result != 'undefined') {
					// Preserve the current floorplan index if it's already set
					var savedFloorplanIdx = (typeof $scope.actFloorplan != "undefined") ? $scope.actFloorplan : 0;
					var wasPreviouslySet = (typeof $scope.actFloorplan != "undefined");
					
					$scope.FloorplanCount = data.result.length;
					$scope.floorPlans = data.result;
					
					// Ensure saved index is valid (in case floorplans were added/removed)
					if (savedFloorplanIdx < 0 || data.result.length === 0 || savedFloorplanIdx >= data.result.length) {
						savedFloorplanIdx = 0;
					}
					$scope.actFloorplan = savedFloorplanIdx;
					
					if (typeof $scope.browser == "undefined") {
						$scope.browser = "unknown";
					}
					

					// handle settings
					if (typeof data.AnimateZoom != 'undefined') {
						$.myglobals.AnimateTransitions = (data.AnimateZoom != 0);
					}
					if (typeof data.FullscreenMode != 'undefined') {
						$.myglobals.FullscreenMode = (data.FullscreenMode != 0);
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
					if (typeof data.ShowSceneNames != 'undefined') {
						Device.showSceneNames = (data.ShowSceneNames == 1);
					}

					//Lets start
					$.each(data.result, function (i, item) {
						var tagName = item.Name.replace(/\s/g, '_') + "_" + item.idx;
						$scope.floorPlans[i].floorID = item.idx;
						$scope.floorPlans[i].xImageSize = 0;
						$scope.floorPlans[i].yImageSize = 0;
						$scope.floorPlans[i].scaleFactor = item.ScaleFactor;
						$scope.floorPlans[i].Name = item.Name;
						$scope.floorPlans[i].tagName = tagName;
						$scope.floorPlans[i].Image = item.Image;
						$scope.floorPlans[i].Loaded = false;
						var thisFP = $scope.makeHTMLnode('div', { id: $scope.floorPlans[i].tagName, index: item.idx, order: item.Order, style: 'display:inline;', 'class': 'imageparent' })
						$("#fpwrapper").append(thisFP);
						thisFP.appendChild($scope.makeHTMLnode('img', { id: $scope.floorPlans[i].tagName + '_img', 'src': item.Image, align: 'top', 'onload': "ImgLoaded('" + $scope.floorPlans[i].tagName + "');", class: 'floorplan', style: "display:none" }));
						thisFP.appendChild(makeSVGnode('svg', { id: tagName + '_svg', width: "100%", height: "100%", version: "1.1", xmlns: "http://www.w3.org/2000/svg", 'xmlns:xlink': "http://www.w3.org/1999/xlink", viewBox: "0 0 0 0", preserveAspectRatio: "xMidYMid meet", overflow: "hidden", style: "display:none" }, ''));
						if ($scope.FloorplanCount > 1) {
							var bulletTd = $scope.makeHTMLnode('td', { 'class': "bulletcell", "width": 100 / $scope.FloorplanCount + "%" });
							$("#BulletRow:first").append(bulletTd);
							bulletTd.appendChild($scope.makeHTMLnode('div', { 'class': "bullet", title: item.Name, related: $scope.floorPlans[i].tagName }));
							$("#BulletImages").append($scope.makeHTMLnode('img', { id: $scope.floorPlans[i].tagName + '_bullet', 'src': item.Image, related: $scope.floorPlans[i].tagName }));
						}
					});

					RefreshFPDevices();

					if ($scope.FloorplanCount > 1) {
						document.addEventListener('keydown', FPkeydown, false);
						$("#fpNavLeft").click(function () {
							if ($scope.actFloorplan > 0) {
								$scope.actFloorplan--;
								ScrollFloorplans($scope.floorPlans[$scope.actFloorplan].tagName);
							}
						});
						$("#fpNavRight").click(function () {
							if ($scope.actFloorplan < $scope.FloorplanCount - 1) {
								$scope.actFloorplan++;
								ScrollFloorplans($scope.floorPlans[$scope.actFloorplan].tagName);
							}
						});
						$("#fpwrapper").on('mouseenter', function () {
							updateNavArrows();
							$(".fp-nav-arrow").addClass("fp-nav-visible");
						}).on('mouseleave', function () {
							$(".fp-nav-arrow").removeClass("fp-nav-visible");
						});
						updateNavArrows();
					}
				}

				$(".bulletcell").hover(function () {
					$(this).children().css({ 'background': $.myglobals.RoomColour });
					$("#" + $(this).children().attr('related') + "_bullet").css({ 'display': 'inline' });
				},
					function () {
						$(this).children().css({ 'background': '' });
						$("#" + $(this).children().attr('related') + "_bullet").css({ 'display': 'none' });
					})
					.mouseup(function () {
						$("#BulletImages").children().css({ 'display': 'none' });
						ScrollFloorplans($(this).children().attr('related'));
					});

				if ((typeof $.myglobals.RoomColour != 'undefined') &&
					(typeof $.myglobals.InactiveRoomOpacity != 'undefined')) {
					$(".hoverable").css({ 'fill': $.myglobals.RoomColour, 'fill-opacity': $.myglobals.InactiveRoomOpacity / 100 });
				}
				if (typeof $.myglobals.ActiveRoomOpacity != 'undefined') {
					$(".hoverable").hover(function () { $(this).css({ 'fill-opacity': $.myglobals.ActiveRoomOpacity / 100 }) },
						function () { $(this).css({ 'fill-opacity': $.myglobals.InactiveRoomOpacity / 100 }) });
				}

				// Hide menus if query string contains 'fullscreen'
				if (window.location.href.search("fullscreen") > 0) {
					$scope.doubleClick();
				}

				$("#fpwrapper").dblclick(function () { $scope.doubleClick(); })
					.attr('fullscreen', ($(".navbar").css('display') == 'none') ? 'true' : 'false');
			});

			$(window).resize(function () { $scope.FloorplanResize(); });

			document.addEventListener('touchstart', FPtouchstart, { passive: true });
			document.addEventListener('touchmove', FPtouchmove, { passive: true });
			document.addEventListener('touchend', FPtouchend, { passive: false });
			$("body").css('overflow', 'hidden')
				.on('pageexit', function () {
					document.removeEventListener('touchstart', FPtouchstart);
					document.removeEventListener('touchmove', FPtouchmove);
					document.removeEventListener('touchend', FPtouchend);
					document.removeEventListener('keydown', FPkeydown);
					$(".fp-nav-arrow").hide();
					$(window).off('resize');
					$("body").off('pageexit').css('overflow', '');

					//Make vertical scrollbar disappear
					$("#fpwrapper").attr("style", "display:none;");
					$("#floorplancontent").addClass("container ng-scope").attr("style", "");

					//Move nav bar with Back and Report button down
					if ($(".navbar").css('display') != 'none') {
						$("#floorplancontent").offset({ top: getNavbarBottom() });
					}
					else {
						$("#floorplancontent").offset({ top: 0 });
					}
					$("#copyright").attr("style", "position:absolute");
				});
		}

		function init() {
			try {
				$scope.MakeGlobalConfig();
				ShowFloorplans();

				$scope.$on('device_update', function (event, deviceData) {
					RefreshItem(deviceData);
				});

			}
			catch (err) {
				generate_noty('error', '<b>Error Initialising Page</b><br>' + err, false);
			}
		};
		$scope.$on('$destroy', function () {
			$interval.cancel($scope.mytimer);
			$scope.mytimer = undefined;
			if (refreshDebounceTimer) {
				clearTimeout(refreshDebounceTimer);
				refreshDebounceTimer = null;
			}
			$("body").trigger("pageexit");
		});

		init();
	});
});
