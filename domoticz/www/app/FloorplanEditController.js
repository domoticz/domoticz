define(['app'], function (app) {
	app.controller('FloorplanEditController', [ '$scope', '$location', '$http', '$interval', function($scope,$location,$http,$interval) {

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
			if ((typeof $("#floorplaneditor") != 'undefined') && (typeof $("#floorplanimagesize") != 'undefined') && (typeof $("#floorplanimagesize")[0] != 'undefined') && ($("#floorplanimagesize")[0].naturalWidth != 'undefined')){
				$("#floorplaneditor")[0].setAttribute('naturalWidth', $("#floorplanimagesize")[0].naturalWidth);
				$("#floorplaneditor")[0].setAttribute('naturalHeight', $("#floorplanimagesize")[0].naturalHeight);
				$("#floorplaneditor")[0].setAttribute('svgwidth', $("#svgcontainer").width());
				var ratio = $("#floorplanimagesize")[0].naturalWidth / $("#floorplanimagesize")[0].naturalHeight;
				$("#floorplaneditor")[0].setAttribute('ratio', ratio);
				svgHeight = $("#floorplaneditor").width() / ratio;
				$("#floorplaneditor").height(svgHeight);
			}
		}

		PolyClick = function(click) {
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
				var scaleX = Device.xImageSize / (borderRect.width + (marginX * 2));
				var scaleY = Device.yImageSize / (borderRect.height + (marginY * 2));
				var scale = ((scaleX > scaleY) ? scaleY : scaleX);
				var attr = 'scale('  + scale + ')' + ' translate(' + (borderRect.x - marginX)*-1 + ',' + (borderRect.y - marginY)*-1 + ')';

				// this actually needs to centre in the direction its not scaling but close enough for v1.0
				$("#floorplangroup")[0].setAttribute("transform", attr);
				$("#DeviceContainer")[0].setAttribute("transform", attr);
				$("#floorplangroup")[0].setAttribute("zoomed", "true");
			}
			window.myglobals.LastUpdate = 0;
			RefreshDevices(); // force redraw to change 'moveability' of icons
		}

		FloorplanClick = function(click) {
			// make sure we aren't zoomed in.
			if ($("#floorplangroup")[0].getAttribute("zoomed") != "true")
			{
				if ($("#roompolyarea").attr("title") != "")
				{
					var Scale = Device.xImageSize / $("#floorplaneditor").width();
					var offset = $("#floorplanimage").offset();
					var points = $("#roompolyarea").attr("points");
					var xPoint = Math.round((click.pageX - offset.left) * Scale);
					var yPoint = Math.round((click.pageY - offset.top) * Scale);
					if (points != "") {
						points = points + ",";
					} else {
						$("#floorplangroup")[0].appendChild(makeSVGnode('circle', { id: "firstclick", cx:xPoint, cy:yPoint, r:2, class:'hoverable' }, ''));
					}
					points = points + xPoint + "," + yPoint;
					$("#roompolyarea").attr("points",  points );
					$('#floorplancontent #delclractive #activeplanupdate').attr("class", "btnstyle3");
				}
				else ShowNotify('Select a Floorplan and Room first.', 2500, true);
			}
			else
			{
				PolyClick(click);
			}
		}

		SetButtonStates = function() {

			$('#updelclr #floorplanadd').attr("class", "btnstyle3-dis");
			$('#updelclr #floorplanedit').attr("class", "btnstyle3-dis");
			$('#updelclr #floorplandelete').attr("class", "btnstyle3-dis");
			$('#floorplancontent #delclractive #activeplanadd').attr("class", "btnstyle3-dis");
			$('#floorplancontent #delclractive #activeplanclear').attr("class", "btnstyle3-dis");
			$('#floorplancontent #delclractive #activeplandelete').attr("class", "btnstyle3-dis");
			$('#floorplancontent #delclractive #activeplanupdate').attr("class", "btnstyle3-dis");

			var anSelected = fnGetSelected( $('#floorplantable').dataTable() );
			if ( anSelected.length !== 0 ) {
				$('#updelclr #floorplanedit').attr("class", "btnstyle3");
				$('#updelclr #floorplandelete').attr("class", "btnstyle3");

				if ($("#floorplancontent #comboactiveplan").children().length > 0) {
					$('#floorplancontent #delclractive #activeplanadd').attr("class", "btnstyle3");
				}
				
				anSelected = fnGetSelected( $('#plantable2').dataTable() );
				if ( anSelected.length !== 0 ) {
					$('#floorplancontent #delclractive #activeplandelete').attr("class", "btnstyle3");
					var data = $('#plantable2').dataTable().fnGetData( anSelected[0] );
					if (data["Area"].length != 0 ) {
						$('#floorplancontent #delclractive #activeplanclear').attr("class", "btnstyle3");
					}
				}
			} else {
				$('#updelclr #floorplanadd').attr("class", "btnstyle3");
			}
		}

		ChangeFloorplanOrder = function(order, floorplanid) {

			if (window.my_config.userrights!=2) {
				HideNotify();
				ShowNotify($.i18n('You do not have permission to do that!'), 2500, true);
				return;
			}
		  $.ajax({
			 url: "json.htm?type=command&param=changefloorplanorder&idx=" + floorplanid + "&way=" + order, 
			 async: false, 
			 dataType: 'json',
			 success: function(data) {
				RefreshFloorPlanTable();
			 }
		  });
		}

		LoadImageNames = function() {
			var oTable = $('#imagetable').dataTable();
			
			oTable.fnClearTable();

			$.ajax({url: "json.htm?type=command&param=getfloorplanimages", 
					async: false, 
					dataType: 'json',
					success: function(data) {
				
					if (typeof data.result != 'undefined') {
						$.each(data.result, function(tag,images){

							for (var i=0; i < images.length; i++) {
								var addId = oTable.fnAddData( {
									"DT_RowId": i,
									"ImageName": images[i],
									"0": images[i]
								} );
							}
						});
						$('#imagetable tbody').off();
						/* Add a click handler to the rows - this could be used as a callback */
						$('#imagetable tbody').on( 'click', 'tr', function () {
							if ( $(this).hasClass('row_selected') ) {
								$(this).removeClass('row_selected');
								$("#dialog-add-edit-floorplan #imagename").val('');
							}
							else {
								oTable.$('tr.row_selected').removeClass('row_selected');
								$(this).addClass('row_selected');
								var anSelected = fnGetSelected( oTable );
								if ( anSelected.length !== 0 ) {
									var data = oTable.fnGetData( anSelected[0] );
									$("#dialog-add-edit-floorplan #imagename").val('images/floorplans/'+data["ImageName"]);
								}
							}
						}); 
					}
				}
			});
		}

		AddNewFloorplan = function() {
			LoadImageNames();
			$("#dialog-add-edit-floorplan #floorplanname").val('');
			$("#dialog-add-edit-floorplan #imagename").val('');
			$( "#dialog-add-edit-floorplan" ).dialog({
				resizable: false,
				width: 520,
				height:400,
				modal: true,
				title: 'Add New Floorplan',
				buttons: {
					"Cancel": function() {
						$( this ).dialog( "close" );
					},
					"Add": function() {
						var csettings=GetFloorplanSettings();
						if (typeof csettings == 'undefined') {
							return;
						}
						$( this ).dialog( "close" );
						AddFloorplan();
					}
				},
				close: function() {
					$( this ).dialog( "close" );
				}
			});
		}

		EditFloorplan = function(idx) {
			LoadImageNames();
			$( "#dialog-add-edit-floorplan" ).dialog({
				resizable: false,
				width: 460,
				height:500,
				modal: true,
				title: 'Edit Floorplan',
				buttons: {
					"Cancel": function() {
						$( this ).dialog( "close" );
					},
					"Update": function() {
						var csettings=GetFloorplanSettings();
						if (typeof csettings == 'undefined') {
							return;
						}
						$( this ).dialog( "close" );
						UpdateFloorplan(idx);
					}
				},
				close: function() {
					$( this ).dialog( "close" );
				}
			});
		}

		DeleteFloorplan = function(idx) {
			bootbox.confirm($.i18n("Are you sure you want to delete this Floorplan?"), function(result) {
				if (result==true) {
					$.ajax({
						 url: "json.htm?type=command&param=deletefloorplan&idx=" + idx,
						 async: false, 
						 dataType: 'json',
						 error: function(){
								HideNotify();
								ShowNotify('Problem deleting Floorplan!', 2500, true);
						 }     
					});

					RefreshUnusedDevicesComboArray();
					RefreshFloorPlanTable();
				}
			});
		}

		GetFloorplanSettings = function() {
			var csettings = {};

			csettings.name=$("#dialog-add-edit-floorplan #floorplanname").val();
			if (csettings.name=="")
			{
				ShowNotify('Please enter a Name!', 2500, true);
				return;
			}
			csettings.image=$("#dialog-add-edit-floorplan #imagename").val();
			if (csettings.image=="")
			{
				ShowNotify('Please enter an image filename!', 2500, true);
				return;
			}
			return csettings;
		}

		UpdateFloorplan = function(idx) {
			var csettings=GetFloorplanSettings();
			if (typeof csettings == 'undefined') {
				return;
			}

			$.ajax({
				 url: "json.htm?type=command&param=updatefloorplan&idx=" + idx +"&name=" + csettings.name + "&image=" + csettings.image,
				 async: false, 
				 dataType: 'json',
				 success: function(data) {
					RefreshFloorPlanTable();
				 },
				 error: function(){
					ShowNotify('Problem updating Plan settings!', 2500, true);
				 }     
			});
		}

		AddFloorplan = function() {
			var csettings=GetFloorplanSettings();
			if (typeof csettings == 'undefined') {
				return;
			}

			$.ajax({
				 url: "json.htm?type=command&param=addfloorplan&name=" + csettings.name + "&image=" + csettings.image,
				 async: false, 
				 dataType: 'json',
				 success: function(data) {
					RefreshFloorPlanTable();
				 },
				 error: function(){
					ShowNotify('Problem adding Floorplan!', 2500, true);
				 }
			});
		}

		RefreshFloorPlanTable = function() {
			$('#modal').show();

			$.devIdx=-1;

			$("#roomplangroup").empty();
			$("#floorplanimage").attr("xlink:href", "");
			$("#floorplanimagesize").attr("src", "");
			clearInterval($.myglobals.refreshTimer);
			Device.initialise();

			var oTable = $('#floorplancontent #plantable2').dataTable();
			oTable.fnClearTable();
			oTable = $('#floorplantable').dataTable();
			oTable.fnClearTable();
			
			$.ajax({
			 url: "json.htm?type=floorplans", 
			 async: false, 
			 dataType: 'json',
			 success: function(data) {
				
			  if (typeof data.result != 'undefined') {
				var totalItems=data.result.length;
				$.each(data.result, function(i,item){
					var updownImg="";
					if (i!=totalItems-1) {
						//Add Down Image
						if (updownImg!="") {
							updownImg+="&nbsp;";
						}
						updownImg+='<img src="images/down.png" onclick="ChangeFloorplanOrder(1,' + item.idx + ');" class="lcursor" width="16" height="16"></img>';
					}
					else {
						updownImg+='<img src="images/empty16.png" width="16" height="16"></img>';
					}
					if (i!=0) {
						//Add Up image
						if (updownImg!="") {
							updownImg+="&nbsp;";
						}
						updownImg+='<img src="images/up.png" onclick="ChangeFloorplanOrder(0,' + item.idx + ');" class="lcursor" width="16" height="16"></img>';
					}

					var addId = oTable.fnAddData( {
						"DT_RowId": item.idx,
						"Name": item.Name,
						"Image": item.Image,
						"Order": item.Order,
						"0": item.Name,
						"1": item.Image,
						"2": updownImg
					} );
				});
				/* Add a click handler to the rows - this could be used as a callback */
				$('#floorplantable tbody').off()
										  .on( 'click', 'tr', function () {
					$.devIdx=-1;
					ConfirmNoUpdate(this, function( param ){
						if ( $(param).hasClass('row_selected') ) {
							$(param).removeClass('row_selected');
							$("#dialog-add-edit-floorplan #floorplanname").val("");
							$("#dialog-add-edit-floorplan #imagename").val("");
							RefreshPlanTable(-1);
							$("#floorplanimage").attr("xlink:href", "");
							$("#floorplanimagesize").attr("src", "");
						}
						else {
							oTable.$('tr.row_selected').removeClass('row_selected');
							$(param).addClass('row_selected');
							var anSelected = fnGetSelected( oTable );
							if ( anSelected.length !== 0 ) {
								var data = oTable.fnGetData( anSelected[0] );
								var idx= data["DT_RowId"];
								$.devIdx=idx;
								$("#updelclr #floorplanedit").attr("href", "javascript:EditFloorplan(" + idx + ")");
								$("#updelclr #floorplandelete").attr("href", "javascript:DeleteFloorplan(" + idx + ")");
								$("#dialog-add-edit-floorplan #floorplanname").val(data["Name"]);
								$("#dialog-add-edit-floorplan #imagename").val(data["Image"]);
								RefreshPlanTable(idx);
								$("#floorplanimage").attr("xlink:href", data["Image"]);
								$("#floorplanimagesize").attr("src", data["Image"]);
								SVGResize();
							}
						}
						SetButtonStates();
					});
				}); 
			  }
			 }
			});
		  
			SetButtonStates();
			$('#modal').hide();
		}

		RefreshUnusedDevicesComboArray = function() {
			$.UnusedDevices = [];
			$("#floorplancontent #comboactiveplan").empty();
			$.ajax({
				url: "json.htm?type=command&param=getunusedfloorplanplans&unique=false", 
				async: false, 
				dataType: 'json',
				success: function(data) {
					if (typeof data.result != 'undefined') {
						$.each(data.result, function(i,item) {
							$.UnusedDevices.push({
								idx: item.idx,
								name: item.Name
							});
						});
						$.each($.UnusedDevices, function(i,item){
							var option = $('<option />');
							option.attr('value', item.idx).text(item.name);
							$("#floorplancontent #comboactiveplan").append(option);
						});
					}
				}
			});
		}

		ShowFloorplans = function() {
			var oTable;
			
			$('#modal').show();
			
			Device.useSVGtags = true;
			Device.backFunction = 'ShowFloorplans';
			Device.switchFunction = 'RefreshDevices';
			Device.contentTag = 'floorplancontent';
			
			var htmlcontent = "";
			htmlcontent+=$('#floorplanmain').html();
			$('#floorplancontent').html(htmlcontent);
			$('#floorplancontent').i18n();
			
			oTable = $('#floorplantable').dataTable( {
				"sDom": '<"H"lfrC>t<"F"ip>',
				"oTableTools": {
					"sRowSelect": "single",
				},
				"bSort": false,
				"bProcessing": true,
				"bStateSave": true,
				"bJQueryUI": true,
				"aLengthMenu": [[5, 10, 25, 100, -1], [5, 10, 25, 100, "All"]],
				"iDisplayLength" : 5,
				"sPaginationType": "full_numbers"
				} );

			oTable = $('#plantable2').dataTable( {
				"sDom": '<"H"lfrC>t<"F"ip>',
				"oTableTools": {
					"sRowSelect": "single",
				},
				"bSort": false,
				"bProcessing": true,
				"bStateSave": false,
				"bJQueryUI": true,
				"aLengthMenu": [[5, 10, 25, 100, -1], [5, 10, 25, 100, "All"]],
				"iDisplayLength" : 10,
			});

			oTable = $('#imagetable').dataTable( {
				"sDom": '<"H"lfrC>t<"F"ip>',
				"oTableTools": {
					"sRowSelect": "single",
				},
				"bSort": true,
				"bProcessing": true,
				"bStateSave": false,
				"bJQueryUI": true,
				"bFilter": false,
				"bLengthChange": false,
				"iDisplayLength" : 5 });

			RefreshUnusedDevicesComboArray();
			
			RefreshFloorPlanTable();
			SetButtonStates();
			$('#modal').hide();
		}

		RefreshPlanTable = function(idx) {
			$.LastPlan=idx;
			$('#modal').show();

			// if we are zoomed in, zoom out before changing rooms by faking a click in the polygon
			if ($("#floorplangroup")[0].getAttribute("zoomed") == "true") {
				PolyClick();
			}
			$("#roomplangroup").empty();
			$("#roompolyarea").attr("title", "");
			$("#roompolyarea").attr("points", "");
			$("#firstclick").remove();

			clearInterval($.myglobals.refreshTimer);
			var oTable = $('#floorplancontent #plantable2').dataTable();
			oTable.fnClearTable();
			Device.initialise();
		  
			$.ajax({
				url: "json.htm?type=command&param=getfloorplanplans&idx=" + idx, 
				async: false, 
				dataType: 'json',
				success: function(data) {
					if (typeof data.result != 'undefined') {
						var totalItems=data.result.length;
						var planGroup = $("#roomplangroup")[0];
						$.each(data.result, function(i,item){
							var addId = oTable.fnAddData( {
								"DT_RowId": item.idx,
								"Area": item.Area,
								"0": item.Name,
								"1": ((item.Area.length == 0) ? '<img src="images/failed.png"/>' : '<img src="images/ok.png"/>'),
								"2": item.Area
							} );
							var el = makeSVGnode('polygon', { id: item.Name + "_Room", 'class':"nothoverable", points: item.Area }, '');
							el.appendChild(makeSVGnode('title', null, item.Name));
							planGroup.appendChild(el);
						});
						/* Add a click handler to the rows - this could be used as a callback */
						$("#floorplancontent #plantable2 tbody tr").click( function( e ) {
							ConfirmNoUpdate(this, function( param ){
									// if we are zoomed in, zoom out before changing rooms by faking a click in the polygon
									if ($("#floorplangroup")[0].getAttribute("zoomed") == "true") {
										PolyClick();
									}
									if ( $(param).hasClass('row_selected') ) {
										$(param).removeClass('row_selected');
										Device.initialise();
										$("#roompolyarea").attr("title", "");
										$("#roompolyarea").attr("points", "");
										$("#firstclick").remove();
										$("#floorplangroup").attr("planidx", "");
										$('#floorplanimage').css('cursor', 'auto');
									}
									else {
										oTable.$('tr.row_selected').removeClass('row_selected');
										$(param).addClass('row_selected');
										Device.initialise();
										var anSelected = fnGetSelected( oTable );
										if ( anSelected.length !== 0 ) {
											var data = oTable.fnGetData( anSelected[0] );
											var idx= data["DT_RowId"];
											$("#floorplancontent #delclractive #activeplandelete").attr("href", "javascript:DeleteFloorplanPlan(" + idx + ")");
											$("#floorplancontent #delclractive #activeplanupdate").attr("href", "javascript:UpdateFloorplanPlan(" + idx + ",false)");
											$("#floorplancontent #delclractive #activeplanclear").attr("href", "javascript:UpdateFloorplanPlan(" + idx + ",true)");
											$("#roompolyarea").attr("title", data["0"]);
											$("#roompolyarea").attr("points", data["2"]);
											$("#floorplangroup").attr("planidx", idx);
											$('#floorplanimage').css('cursor', 'crosshair');
											ShowDevices(idx);
										}
									}
									SetButtonStates();
								});
						}); 
					}
				}
			});

			SetButtonStates();
			$('#modal').hide();
		}

		AddUnusedPlan = function() {
			if ($.devIdx==-1) {
				return;
			}

			var PlanIdx=$("#floorplancontent #comboactiveplan option:selected").val();
			if (typeof PlanIdx == 'undefined') {
				return ;
			}
			$.ajax({
				url: "json.htm?type=command&param=addfloorplanplan&idx=" + $.devIdx + 
					"&planidx=" + PlanIdx,
				async: false, 
				dataType: 'json',
				success: function(data) {
					if (data.status == 'OK') {
						RefreshPlanTable($.devIdx);
						RefreshUnusedDevicesComboArray();
					}
					else {
						ShowNotify('Problem adding Plan!', 2500, true);
					}
				},
				error: function(){
					HideNotify();
					ShowNotify('Problem adding Plan!', 2500, true);
				}     
			});

			SetButtonStates();
		}

		UpdateFloorplanPlan = function(planidx,clear) {
			var PlanArea='';
			if (clear != true) {
				PlanArea = $("#roompolyarea").attr("points");
			}
			$.ajax({
				url: "json.htm?type=command&param=updatefloorplanplan&planidx=" + planidx + 
					"&area=" + PlanArea,
				async: false, 
				dataType: 'json',
				success: function(data) {
					if (data.status == 'OK') {
		//				RefreshPlanTable($.devIdx);
					}
					else {
						ShowNotify('Problem updating Plan!', 2500, true);
					}
				},
				error: function(){
					HideNotify();
					ShowNotify('Problem updating Plan!', 2500, true);
				}     
			});	
			var deviceParent = $("#DeviceIcons")[0];
			for (var i=0; i<deviceParent.childNodes.length; i++) {
				$.ajax({
						url: "json.htm?type=command&param=setplandevicecoords" + 
										"&idx=" + deviceParent.childNodes[i].getAttribute('idx') + 
										"&planidx=" + planidx + 
										"&xoffset=" + deviceParent.childNodes[i].getAttribute('xoffset') +
										"&yoffset=" + deviceParent.childNodes[i].getAttribute('yoffset') +
										"&DevSceneType=" + deviceParent.childNodes[i].getAttribute('devscenetype'),
						async: false, 
						dataType: 'json',
						success: function(data) {
							if (data.status == 'OK') {
		//						RefreshPlanTable($.devIdx);
							}
							else {
								ShowNotify('Problem udating Device Coordinates!', 2500, true);
							}
						},
						error: function(){
							HideNotify();
							ShowNotify('Problem updating Device Coordinates!', 2500, true);
						}     
					});	
			}

			RefreshPlanTable($.devIdx);
		}

		ConfirmNoUpdate = function(param, yesFunc) {
			if ($('#floorplancontent #delclractive #activeplanupdate').attr("class") == "btnstyle3") {
				bootbox.confirm($.i18n("You have unsaved changes, do you want to continue?"), 
					function(result) {
						if (result==true) {
							yesFunc(param);
						}
					});
			} else {
				yesFunc(param);
			}
		}

		DeleteFloorplanPlan = function(planidx) {
			bootbox.confirm($.i18n("Are you sure to delete this Plan from the Floorplan?"), function(result) {

				if (result==true) {
						$.ajax({
							 url: "json.htm?type=command&param=deletefloorplanplan&idx=" + planidx,
							 async: false, 
							 dataType: 'json',
							 success: function(data) {
								RefreshUnusedDevicesComboArray();
								RefreshPlanTable($.devIdx);
							 },
							 error: function(){
									HideNotify();
									ShowNotify('Problem deleting Plan!', 2500, true);
							 }     
						});
				}
			});
		}

		RefreshDevices = function() {
			if ((typeof $("#floorplangroup") != 'undefined') &&
				(typeof $("#floorplangroup")[0] != 'undefined')) {
			
				clearInterval($.myglobals.refreshTimer);

				if ($("#floorplangroup")[0].getAttribute("planidx") != "") {
					Device.useSVGtags = true;
					$.ajax({
							 url: "json.htm?type=devices&filter=all&used=true&order=Name&plan="+$("#floorplangroup")[0].getAttribute("planidx")+"&lastupdate=" + window.myglobals.LastUpdate,
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
								if (typeof data.result != 'undefined') {
									$.each(data.result, function(i,item) {
										var dev = Device.create(item);
										dev.setDraggable($("#floorplangroup")[0].getAttribute("zoomed") == "false");
										dev.htmlMinimum($("#roomplandevices")[0]);
									});
								}
							}
						});

					// Add Drag and Drop handler
					$('.DeviceIcon')
						.draggable()
						.bind('drag', function(event, ui){
								// update coordinates manually, since top/left style props don't work on SVG
								var parent = event.target.parentNode;
								if (parent) {
									var Scale = Device.xImageSize / $("#floorplaneditor").width();
									var offset = $("#floorplanimage").offset();
									var xoffset = Math.round((event.pageX - offset.left - (Device.iconSize/2)) * Scale);
									var yoffset = Math.round((event.pageY - offset.top - (Device.iconSize/2)) * Scale);
									if (xoffset < 0) xoffset = 0;
									if (yoffset < 0) yoffset = 0;
									if (xoffset > (Device.xImageSize-Device.iconSize)) xoffset = Device.xImageSize-Device.iconSize;
									if (yoffset > (Device.yImageSize-Device.iconSize)) yoffset = Device.yImageSize-Device.iconSize;
									parent.setAttribute("xoffset", xoffset);
									parent.setAttribute("yoffset", yoffset);
									parent.setAttribute("transform", 'translate(' + xoffset + ',' + yoffset + ')');
									var objData = $('#DeviceDetails #'+event.target.parentNode.id)[0];
									if (objData != undefined) {
										objData.setAttribute("xoffset", xoffset);
										objData.setAttribute("yoffset", yoffset);
										objData.setAttribute("transform", 'translate(' + xoffset + ',' + yoffset + ')');
									}
									$('#floorplancontent #delclractive #activeplanupdate').attr("class", "btnstyle3");
									if ($.browser.mozilla) // nasty hack for FireFox.  FireFox forgets to display the icon once you start dragging so we need to remind it
									{
										(event.target.style.display == "inline") ? event.target.style.display = "none" : event.target.style.display = "inline";
									}

								}
							})
						.bind('dragstop', function(event, ui){
								event.target.style.display="inline";
							})	;

						$.myglobals.refreshTimer = setInterval(RefreshDevices, 10000);
				}
			}
		}

		ShowDevices = function(idx) {
			if (typeof $("#floorplangroup") != 'undefined') {
				Device.useSVGtags = true;
				Device.initialise();
				$("#roomplandevices").empty();
				$("#floorplangroup")[0].setAttribute("planidx", idx);
				window.myglobals.LastUpdate = 0;
				RefreshDevices();
			}
		}

		init();

		function init()
		{
			Device.initialise();
			ShowFloorplans();
			$("#floorplanimage").on("click", function ( event ) {FloorplanClick(event);});
			$("#roompolyarea").on("click", function ( event ) {PolyClick(event);});
			$("#svgcontainer").on("mousemove", function ( event ) {MouseXY(event);});
		};
	} ]);
});