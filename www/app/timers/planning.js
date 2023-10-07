///////////////////////////////////////////////
// Weekly Planning ,  2017, Syrhus
// v1.4.2
// updated for v4.9700
// 2018-09-20 : add  Odd & Even week numbers
// 2018-10-30 : bug on <th> fixed
// 2018-12-11 : Disable/Enable all setPoints (avoid to clear a planning to set it in standby mode)
//							Time precision to 15mins
//							Add i18n translations
//							Highlight on hover
//							Time range selection
// 2018-12-20 : adjust table time range depending on setpoint minutes.
//		listen timers event [timersInitialized] & [timersLoaded]
//              improve speed performance
//		adujst to the right mode if less than 4 modes used and not "heatest"
// 2019-11-18 : clear only previous timers id instead of calling the clearTimers function which erase all timers on all timerplan
// 2019-11-26 : fix deletesetpointtimer instead of deletetimer for thermostat
// 2020-07-29 : add 5mins and 2mins Time range
// 2022-11-11 : support Off level for selector switch if not hidden
///////////////////////////////////////////////

PlanningTimerSheet = function(options){
	$('head').append('<link rel="stylesheet" type="text/css" href="../css/planning.css">');

	var bIsSelecting = false;
	var bAddClass = true;
	var modeSelected = 0;
	var startSelId = 0;
	var endSelId   = 0;
	var mousedownEvt = 0;
	var mouseupEvt = 0;

	const MON = 1;
	const TUE = 2;
	const WED = 4;
	const THU = 8;
	const FRI = 16;
	const SAT = 32;
	const SUN = 64;
	const EVERYDAYS = 128;
	const WEEKDAYS = 256;
	const WEEKENDS = 512;

	const SUM_EVERYDAYS = 127;
	const SUM_WEEKDAYS = 31;
	const SUM_WEEKENDS = 96;

	const ODD = 8;
	const EVEN = 9;
	const ONTIME = 2;

	var $element = null;
	var $table = null;
	var $tbody = null;
	var $thead = null;

	var prevTimers = [];
	var planning = [];

	//////////////////////////////

	var days = [
		{ "name": "Monday" },
		{ "name": "Tuesday" },
		{ "name": "Wednesday" },
		{ "name": "Thursday" },
		{ "name": "Friday" },
		{ "name": "Saturday" },
		{ "name": "Sunday" }];

	var planning_translations = [
		{'lng':'en', 'res':{
			'activate all': 'Activate all',
			'deactivate all': 'Deactivate all',
			'Comfort':'Comfort',
			'Economic':'Economic',
			'Nighttime':'Night time',
			'NoFreeze':'Frost free',
			'timeRange': 'Time range',
			'hour':'hour',
			'mins':'mins'
		}},
		{'lng':'fr','res':{
			'activate all': 'Tout activer',
			'deactivate all': 'Tout désactiver',
			'Comfort':'Confort',
			'Economic':'Economique',
			'Nighttime':'Nuit',
			'NoFreeze':'Hors gel',
			'timeRange': 'Plage horaire',
			'hour':'heure',
			'mins':'mins'
		}}
		/* add others translations here
		,{'lng':'XX', 'res':{
				...
		}}
		*/
	];

	var defaults = $.extend({
		"devid":null,
		"container":"#deviceTimers",
		"commandClear":"clearsetpointtimers",
		"commandAdd":"addsetpointtimer",
		"buttonClass":"btnstyle3",
		"buttonModeClassSelected": "btnstyle3-sel",
		"odd_even_week" :false,
		"modes":
		[{ "value": 20, "class": "m0", "name": "Comfort"},
		{ "value": 19, "class": "m1", "name": "Economic"},
		{ "value": 16, "class": "m2", "name": "Nighttime"},
		{ "value": 4, "class": "m3", "name": "NoFreeze"}],
		"temperatureModes":true,
		"hover_activate":true, //to highlight row and column on mouse hover
		"nbTicksPerHour":1, //6 for 10mins, 4 for 15mins, 2 for 30mins, 1 for 1hour
		"propValue":"Temperature",
		"propValueAjax":"tvalue"}, options);
	//////////////////////////////////////////////////
    if (typeof options.device.ConforTemp != 'undefined') 
		defaults.modes[0].value = options.device.ConforTemp ;
    if (typeof options.device.EcoTemp    != 'undefined') 
		defaults.modes[2].value = options.device.EcoTemp ;

	Clear = function () {
		$tbody.find('td').removeClass();
	};

	//add or remove class to selected cell
	setSel = function (elt) {
		var vclass = getModeClass();
		if (vclass == null)
			return;

		$(elt).removeClass();
		if (bAddClass)
			$(elt).addClass(vclass);
	};

	//apply class to row range
	setRowSel = function ($td) {
		if(!$td.attr('rowspan'))
			$td.siblings('td').removeClass().addClass(getModeClass());
		else {
			// Find span count in first td. Select next rows.
			var spanCount = $td.attr('rowspan');
			$td.parent().nextAll().addBack().slice(0, spanCount).children('td').removeClass().addClass(getModeClass());
		}
	};

	//apply class to col range
	setColSel = function ($col) {
	  var $found = findTdCol($col);
		if($found)
			$found.removeClass().addClass(getModeClass());
	};

	findTdCol = function($col){
		var h = $col.attr("h");
		var m = $col.attr("m");
		var colspan = $col.attr("colspan");
		if(colspan == undefined)
			colspan = 1;
		else colspan = parseInt(colspan);

		var $found = null;
		if (h) {
			var index = parseInt(h);
			$found = $tbody.find("td[h=" + index + "]");
			if(m){
				if(colspan > 1){
					var filters = [];
					var offset = (m === "00" ) ? 0 :30;
					for(var i =0; i < colspan; i++){
						filters.push("td[m='" + pad2(offset + (30/colspan)*i) +"']");
					}
					$found = $found.filter(filters.join(','));
				} else if (colspan === 1) {
					$found = $found.filter("td[m='" + m +"']");
				}
			}
		}
		else
			$found = $tbody.find("td");
		return $found;
	};

	getMode = function () {
		if (modeSelected < 0 || modeSelected >= defaults.modes.length)
			return null;
		else return defaults.modes[modeSelected];
	};

	getModeClass = function () {
		var _mode = getMode();
		return (_mode != null ? _mode.class : null);
	};

	getSlice2 = function(val){  return ("0" + val).slice(-2); };
	pad2 = function(num) { var size=2; var s = num+"";    while (s.length < size) s = "0" + s;    return s;};

	createPlanning = function (isOddEven) {
		planning = [];
		for (var i = 1; i <= (isOddEven ? 14 : 7); i++)
			planning.push({ "day": i, "triggers": [] });
	};

	getPlanning = function () {
		var plan = [];
		$tbody.find('tr').each(function (index, row) {
			var day = { "day": index+1 , "triggers" :[]};

			var prev = null;
			var prevDeactivated = null;
			$(row).find("td[class!='']").each(function (ndx, td) {
				var $this = $(td);
				var deactivated = $this.hasClass("deactivate");
				var cls = $this.prop("classList")[0];
				if (cls != prev || prevDeactivated != deactivated) {
					prev = cls;
					prevDeactivated = deactivated;
					day.triggers.push({ "hour": + $this.attr("h"), "min": + $this.attr("m"), "value":  getValueFromClass(cls) , "class" : cls, "active": !deactivated });
				}
			});

			plan.push(day);
		});
		return plan;
	};

	loadPlanning = function (json) {
//		if(!$element.is(':visible'))
//			return;

		convertSetPointstoTriggers(json);
		if($table && $table.find('tbody tr').length >0 )
			Clear();
		else
			initPlanningTable();
		showPlanning();
	};

	showPlanning = function(){
		setModes();
		$.each(planning, function (index, day) {
			var $tds = $tbody.find("tr[index="+ (day.day) + "] td");
			var prev = { "hour": -1, "class": "", "min":-1};
			var nbticks = defaults.nbTicksPerHour;
			$.each(day.triggers, function (ndx, hour) {
				if (prev.hour !== -1)
					$tds.slice(getNbTicksToAdd(nbticks, prev), getNbTicksToAdd(nbticks, hour)).addClass(prev.class);
				prev = hour;
				$tds.slice(getNbTicksToAdd(nbticks, hour)).removeClass().addClass(hour.class);
			});
		});
	};

	getNbTicksToAdd = function(nbticks, obj){
			if(nbticks === 1)
				return obj.hour;
			else {
				var divider = 60/nbticks;
				return obj.hour *nbticks + (obj.min/divider);
			}
	};

	convertTriggersToSetPoints = function (planning) {
		var setPoints = [];
		$.each(planning, function (index, day) {
			$.each(day.triggers, function (ndx, elt) {
				var setPoint = {"Active":(this.active ? "true" : "false"), "Date": "", "MDay": 0, "Month": 0, "Occurence": 0, "Type": 2 };
				setPoint[defaults.propValue] = this.value;
				setPoint.Hour = this.hour;
				setPoint.Time = getSlice2(this.hour) + ":" + getSlice2(this.min) ;//("0" + this.hour).slice(-2) + ":" + ("0" + this.min).slice(-2);
				setPoint.Min = this.min;

				if(defaults.odd_even_week)
					setPoint.Type = (day.day > 7 ? EVEN : ODD );
				setPoint.Days = Math.pow(2, day.day - (setPoint.Type === EVEN ? 8 : 1));
				setPoints.push(setPoint);
			});
		});

		//search sames setPoint to another days to mutualize setPoint
		for (var i = 0; i < setPoints.length; i++) {
			var sp = setPoints[i];
			var sameSetPoints = [];
			var newSetPoints = setPoints.filter(function (setpoint, index) {
				if (setpoint.Time === sp.Time
					&& setpoint.Active === sp.Active
					&& setpoint[defaults.propValue] == sp[defaults.propValue]
					&& setpoint.Days != sp.Days
				  && setpoint.Type === sp.Type) {
					sameSetPoints.push(setpoint);
					return false;
				}
				else
					return true;
			})

			if (sameSetPoints.length) {
				for (var j = 0; j < sameSetPoints.length; j++) {
					sp.Days |= sameSetPoints[j].Days;
				}
			}

			setPoints = newSetPoints;
		}
		return setPoints;
	};

	convertSetPointstoTriggers = function (SetPoints) {

		//check if you have to display odd&even week
		var isOddEven = false;
		var nbTicks = 1;
		$.each(SetPoints, function (ndx) {
			if (this.Type === ONTIME || this.Type === EVEN || this.Type === ODD){
				if (this.Type === EVEN || this.Type === ODD){
					isOddEven = true;
					return false;
				}

			}
		});


		$element.find('#oew_oddeven').attr('checked', isOddEven);
		if(defaults.odd_even_week !== isOddEven){
			defaults.odd_even_week =isOddEven;
		}

		createPlanning(isOddEven);

		var values = [];
		prevTimers = [];
		$.each(SetPoints, function (ndx) {
			//if (this.Active && (this.Type === ONTIME || this.Type === EVEN || this.Type === ODD)) {
			if (this.Type === ONTIME || this.Type === EVEN || this.Type === ODD) {
				prevTimers.push(this.idx);

				var trigger = {};
				var day = 1;
						

				trigger.hour = parseInt(this.Time.substr(0, 2));
				trigger.min = parseInt(this.Time.substr(3, 2));
				trigger.value = this[defaults.propValue];
				trigger.active = (this.Active === 'true');
				
				if(this.Cmd ===1 && defaults.propValue === "Level" && this.Level === 100)
					trigger.value=0;

				if( values.indexOf(trigger.value) === -1)
					values.push(trigger.value);

				var days = [];

				if(this.Type === EVEN){
					if (this.Days === EVERYDAYS)
							days.push(8, 9, 10, 11, 12, 13, 14);
					else if (this.Days === WEEKDAYS)
							days.push(8, 9, 10, 11, 12);
					else if (this.Days === WEEKENDS)
						days.push(13, 14);
					else {
						if (this.Days & MON)
							days.push(8);
						if (this.Days & TUE)
							days.push(9);
						if (this.Days & WED)
							days.push(10);
						if (this.Days & THU)
							days.push(11);
						if (this.Days & FRI)
							days.push(12);
						if (this.Days & SAT)
							days.push(13);
						if (this.Days & SUN)
							days.push(14);
					}
				}
				else{
					if (this.Days === EVERYDAYS)
							days.push(1, 2, 3, 4, 5, 6, 7);
					else if (this.Days === WEEKDAYS)
							days.push(1, 2, 3, 4, 5);
					else if (this.Days === WEEKENDS)
						days.push(6, 7);
					else {
						if (this.Days & MON)
							days.push(1);
						if (this.Days & TUE)
							days.push(2);
						if (this.Days & WED)
							days.push(3);
						if (this.Days & THU)
							days.push(4);
						if (this.Days & FRI)
							days.push(5);
						if (this.Days & SAT)
							days.push(6);
						if (this.Days & SUN)
							days.push(7);
					}
				}
				$.each(days, function (ndx, val) {
					planning[val-1].triggers.push($.extend({},trigger));
				});
			};
		});


		values.sort(function (a, b) {  return b - a;  }); //desc order
		if(defaults.temperatureModes){
			var valuescount = values.length;
			var modescount = defaults.modes.length;
			var $modes = $element.find('.modes .mode input');
			if(valuescount< modescount){
				for(var i = 0; i < valuescount ; i++){
					var j = i;
					while(j < modescount){
						if(values[i] >= defaults.modes[j].value || j === (modescount-1)){
							defaults.modes[j].value = values[i];
							break;
						} else if(values[i] < defaults.modes[j].value && values[i] > defaults.modes[j+1].value){
								defaults.modes[j].value = values[i];
								break;
						}
						j++;
					}
				}
			} else {
					for(var i= 0; i < defaults.modes.length; i++){
						defaults.modes[i].value = values[i];
					}
			}

			for(var i= 0; i < defaults.modes.length; i++){
				$modes.filter('[index=' + i + ']').val(defaults.modes[i].value);
			}
		}

		$.each(planning, function (ndx, day) {
			$.each(day.triggers, function(index, trigger){
				trigger.class = getClassFromValue(trigger.value);
				if(!trigger.active)
					trigger.class += " deactivate";
			});

			day.triggers.sort(function (a, b) {
				if (a.hour === b.hour) return 0;
				else return (a.hour < b.hour) ? -1 : 1;
			});
			
			$.each(day.triggers, function (ndx, trigger) {
				if(trigger.min === 0)
					return;
				if( trigger.min % 30 === 0)
					nbTicks = (nbTicks < 2) ? 2 : nbTicks;
				else if(trigger.min % 15 === 0 )
					nbTicks = (nbTicks < 4) ? 4 : nbTicks;
				else if(trigger.min % 10 === 0 )
					nbTicks = (nbTicks < 6) ? 6 : nbTicks;
				else if(trigger.min % 5 === 0 )
					nbTicks = (nbTicks < 12) ? 12 : nbTicks;
				else if(trigger.min % 2 === 0 )
					nbTicks = (nbTicks < 30) ? 30 : nbTicks;
					
				if(nbTicks === 30)
					return false;					
			});
		});
		
		if(defaults.nbTicksPerHour < nbTicks){
			defaults.nbTicksPerHour = nbTicks;
		}

	};

	updateSetPoints = function(active){
		var plan = getPlanning();
		var setPoints = convertTriggersToSetPoints(plan);
    var activeForced = (arguments.length === 1 ? true: false);

		var timers= [];
		$.each(setPoints, function(ndx, tsettings){
			if(tsettings.Days === SUM_EVERYDAYS)
				tsettings.Days = EVERYDAYS;
			else if(tsettings.Days === SUM_WEEKDAYS)
				tsettings.Days = WEEKDAYS;
			else if(tsettings.Days === SUM_WEEKENDS)
				tsettings.Days = WEEKENDS;


			var urlcmd =  "json.htm?type=command&param=" + defaults.commandAdd +"&idx=" + defaults.devid +
				"&active=" + (activeForced ? active : tsettings.Active) +
				"&timertype=" + tsettings.Type +
				"&date=" + tsettings.Date +
				"&hour=" + tsettings.Hour +
				"&min=" + tsettings.Min +
				"&days=" + tsettings.Days +
				"&mday=" + tsettings.MDay +
				"&month=" + tsettings.Month +
				"&occurence=" + tsettings.Occurence +
				"&randomness=false";
				
			if(!(defaults.propValue === "Level" && tsettings[defaults.propValue] === 0)) {
				urlcmd += "&" + defaults.propValueAjax +"=" + tsettings[defaults.propValue];
				
				if(!defaults.temperatureModes && defaults.propValueAjax !== "command")
					urlcmd += "&command=0";
			}
			else
				urlcmd += "&command=1";

			timers.push(urlcmd);
		});

		clearPrevSetPoints(addSetPointTimers, timers);

	};

	clearPrevSetPoints = function(callbackFunc, param){
		var timers = [];
		$.each(prevTimers, function(ndx, timerid){
			timers.push("json.htm?type=command&param=delete"+ (defaults.temperatureModes ? "setpoint" : "") + "timer&idx=" + timerid);
		});

		runAjaxLoop(timers,'Problem deleting timer!',0, callbackFunc, param );
	};

	addSetPointTimers = function(param){
		runAjaxLoop(param,'Problem adding timer!',0, defaults.refreshCallback);
	};

	runAjaxLoop = function(loopParams, errorMsg, curIndex, callbackFunc, param){
		if(loopParams.length > 0)
			$.ajax({
				url: loopParams[curIndex],
				dataType: 'json',
				error: function () {
					HideNotify();
					ShowNotify($.t(errorMsg), 2500, true);
				},
				complete: function(){
					curIndex++;
					if(curIndex < loopParams.length)
						runAjaxLoop(loopParams, errorMsg, curIndex, callbackFunc, param);
					else if(callbackFunc)
						callbackFunc(param);
				}
			});
		else if(callbackFunc)
			callbackFunc(param);
	};

	getValueFromClass = function (cls) {
		var res = $.grep(defaults.modes, function (elt, ndx) {
			return elt.class === cls;
		});
		if (res != null && res.length > 0)
			return res[0].value;
		else
			return null;
	};

	getClassFromValue = function (val) {
		var res = $.grep(defaults.modes, function (elt, ndx) {
			return elt.value == val;
		});
		if (res != null && res.length > 0)
			return res[0].class;
		else
			return null;
	};

	setModes = function(){
		var _modes = [];
		$.each(defaults.modes, function(index, mode){
			_modes.push( "<div class='mode " + defaults.buttonClass +" " + ((index === modeSelected)?defaults.buttonModeClassSelected:"") +"' index=" + index +"><label>" + $.t(mode.name) + "</label>");
			if(defaults.temperatureModes)
				_modes.push("<input index=" + index +" type='text' value=" + mode.value +" class='inputmode " + mode.class + "'/><span>°</span>");
			else
				_modes.push("<div index=" + index +" class='inputmode " + mode.class + "'></div>");

			_modes.push("</div>");
		});

		$element.find('.modes').off().empty().append(_modes.join(''));

		/////click event on mode buttons
		$element.find('.modes').on("click", ".mode", function () {
			var $elt = $(this);
			if(defaults.buttonModeClassSelected != undefined){
				$elt.siblings().removeClass(defaults.buttonModeClassSelected);
				$elt.addClass(defaults.buttonModeClassSelected);
			}
			modeSelected = $elt.attr("index");
		}).on("change", ".mode input", function(){
			var $elt = $(this);
			defaults.modes[parseInt($elt.attr('index'))].value = $elt.val();
		});
	};



	getColSpan = function(level){
		if(level === 0)		{
			return defaults.nbTicksPerHour || 4;
		}
		else if (level === 1){
			switch (defaults.nbTicksPerHour) {
				case 2:
					return 1;
				case 6:
					return 3;
				case 12:
					return 6;
				case 30:
					return 15;
				default:
					return 2;
			}
		}
		else return 1;
	};

	createTable = function(){
		var headers = [];
		headers.push("<tr><th ");
		if(defaults.nbTicksPerHour > 1)
			headers.push("rowspan="+ (defaults.nbTicksPerHour === 2 ? 2 : 3));
	  headers.push( (defaults.odd_even_week ? " colspan=2" : "") + ">" + $.t("All") + "</th>");

		for (var h = 0; h < 24 ; h++) {
			headers.push("<th h=" + h + " colspan=" + getColSpan(0) +  " title='" + h + ":00 - " + h + ":59" +"'>" + h + "</th>");
		}
		headers.push("</tr>");

		if(defaults.nbTicksPerHour >=2){
			headers.push("<tr>");
			var ticks = 2;
			for (var h = 0 ; h < 24; h++) {
				for (var m = 0; m < ticks; m++) {
					headers.push("<th h=" + h + " m=" + pad2((m*60)/ticks) + " title='" + h + ":" + pad2((m*60)/ticks) + " - " + h + ":" + pad2((((m+1)*60/ticks)-1)) + "' colspan=" + getColSpan(1) +"></th>");
				}
			}
			headers.push("</tr>");

			if(defaults.nbTicksPerHour >=4){
				headers.push("<tr>");
				ticks = defaults.nbTicksPerHour;
				for (var h = 0 ; h < 24; h++) {
					for (var m = 0; m < ticks; m++) {
						headers.push("<th h=" + h + " m=" + pad2((m*60)/ticks) + " title='" + h + ":" + pad2((m*60)/ticks) + " - " + h + ":" + pad2((((m+1)*60/ticks)-1)) + "'></th>");
					}
				}				
				
				headers.push("</tr>");
			}
		}

		var tds = [];
		createBodyRows(tds, 0);
		if(defaults.odd_even_week)
			createBodyRows(tds, 7);

		if(defaults.odd_even_week)
			$table.addClass('odd-even');
  	else
			 $table.removeClass('odd-even');

		$thead.empty().append(headers.join(''));
		$tbody.empty().append(tds.join(''));
	};

	createBodyRows = function (tds, delta) {

		for (var d = 0; d < days.length; d++) {
			if(d === 0 &&  defaults.odd_even_week)
				tds.push("<tr index=" + (d + 1 + delta) + "><th rowspan=7>" +
					$.t((delta === 0 ? "Odd Week" : "Even Week"))
					+ "</th><th>" + $.t(days[d].name) + "</th>");
			else
					tds.push("<tr index=" + (d + 1 + delta) + "><th>" + $.t(days[d].name) + "</th>");

			var ticks = defaults.nbTicksPerHour;
			for (var h = 0 ; h < 24; h++) {
				for (var m = 0; m < ticks; m++) {
					var id =  (d + 1 + delta) * 100 + (h*ticks+m);
					tds.push("<td id=\"" + id + "\" h=" + h + " m=" + pad2((m*60)/ticks) + " title='" + $.t(days[d].name) + " " + h + ":" + pad2((m*60)/ticks) + " - " + h + ":" + pad2((((m+1)*60/ticks)-1)) + "'></td>");
				}
			}				

			tds.push("</tr>");
		}

	};

	initUI = function(){
		$element = $(defaults.container).find('#ts-planning');
		if($element.length === 0 )
 			$element = $('<div id="ts-planning" class="ts-planning"></div>').appendTo(defaults.container);
		else {
			 $element.off();
			 if($tbody)
			 		$tbody.off();
			 if($table)
			 		$table.off();
			 		

		}
		
		$(defaults.container).find('#tabs').tabs()
//		$(defaults.container).find('#tabs').tabs({active: 1	});

		if(!(defaults.nbTicksPerHour === 1 || defaults.nbTicksPerHour === 2 || defaults.nbTicksPerHour ===4 || defaults.nbTicksPerHour ===6 || defaults.nbTicksPerHour ===12 || defaults.nbTicksPerHour ===30))
				defaults.nbTicksPerHour === 2;

		$element.empty().append('<div class="modes"></div>\
							<div class="timeRangediv"><label for="timeRange">' + $.t('timeRange')+ '</label><select name="timeRange" class="timeRange">\
							<option value="1">1 ' + $.t("hour") + '</option>\
							<option value="2">30 ' + $.t("mins") + '</option>\
							<option value="4">15 ' + $.t("mins") + '</option>\
							<option value="6">10 ' + $.t("mins") + '</option>\
							<option value="12">5 ' + $.t("mins") + '</option>\
						    <option value="30">2 ' + $.t("mins") + '</option>\
							</select></div>\
							<div class="ts-wrapper">\
							<table class="ts-table" >\
								<thead></thead>\
								<tbody></tbody>\
							</table>\
							</div>\
							<div class="ts-actions">\
								<div class="ts-clearTimeSheet" >'+ $.t('Clear') +'</div>\
								<div class="ts-updateSetPoints" >'+ $.t('Update') +'</div>\
								<input id="oew_oddeven" type="checkbox" '+ (defaults.odd_even_week ?"checked" :"") +' /><label for="oew_oddeven">' + $.t("Odd Week Numbers") + ' & ' + $.t("Even Week Numbers")  + '</label>\
								<div class="ts-DisableSetPoints" >'+ $.t('deactivate all') +'</div>\
								<div class="ts-EnableSetPoints" ">'+ $.t('activate all') +'</div>\
							</div>');
		if(defaults.buttonClass != undefined)
			$element.find('.ts-actions>div').addClass(defaults.buttonClass);

		GetId = function(row, col)
		{
			return ( row * 100 + col );
		}
		GetRow = function(pstartSelId)
		{
			return  Math.floor(startSelId / 100 ) ;
		}
		GetCol = function(pstartSelId)
		{
			return  startSelId % 100 ;
		}

		selectRectangle = function (SelClass, pbAdd, pRemove) {
			if (typeof pRemove  == 'undefined') pRemove = true;
			startRow = GetRow(startSelId) ;
			startCol = GetCol(startSelId) ;
			endRow   = Math.floor(endSelId / 100 );
			endCol   = endSelId % 100 ;
//			console.log(startSelId + " " + endSelId);
			if (endRow<startRow){
				var r = endRow ; endRow = startRow ; startRow = r ;
			}
			if (endCol<startCol){
				var r = endCol ; endCol = startCol ; startCol = r ;
			}
			for (var row= startRow ; row<=endRow;row++)
				for (var col= startCol ; col<=endCol;col++)
				{
					var id = GetId(row, col);
					if (pRemove) {
						if (SelClass == 0)
							$("#" + id).removeClass();
						else 
							$("#" + id).removeClass(SelClass);
					}
					if (pbAdd)
						$("#" + id).addClass(SelClass);
				}
		};
		selectRectangleBorder = function () {
			startRow = GetRow(startSelId) ;
			startCol = GetCol(startSelId) ;
			endRow   = Math.floor(endSelId / 100 );
			endCol   = endSelId % 100 ;
			if (endRow<startRow){
				var r = endRow ; endRow = startRow ; startRow = r ;
			}
			if (endCol<startCol){
				var r = endCol ; endCol = startCol ; startCol = r ;
			}
			for (var row = startRow; row <= endRow; row++) {
				$("#" + GetId(row,startCol)).addClass('tsleft');
				$("#" + GetId(row,endCol  )).addClass('tsright');
			}

			for (var col = startCol; col <= endCol; col++)
			{
				$("#" + GetId(startRow,col)).addClass('tstop');
				$("#" + GetId(endRow  ,col)).addClass('tsbottom');
			}
		};


		$table = $element.find('table');
		$tbody = $table.find('tbody');
		$thead = $table.find('thead');
			//////mouse events on tbody

		if ($.myglobals.ismobile == true) {
			mousedownEvt = "touchstart";
			mouseupEvt = "touchend";
		}
		else {
			mousedownEvt = "mousedown";
			mouseupEvt = "mouseup";
		}
		$tbody.on(mousedownEvt, "td", function (event) {
			bIsSelecting = true;
			bAddClass = !$(this).hasClass(getModeClass());
			setSel(this);
			startSelId = parseInt(this.id);
			endSelId   = parseInt(this.id);
//			console.log("start:"+startSelId );
		});
		$tbody.on(mouseupEvt, "td", function (event) {
			bIsSelecting = false;
//			console.log("stop:" + endSelId);
			selectRectangle(0,false);//del
			selectRectangle(getModeClass(),bAddClass);//display
		});
		if ( $.myglobals.ismobile != true  )
			$tbody.on("mouseover", "td", function (event) {
				var elem = this;
				if (bIsSelecting) {
					selectRectangle(0,false);//del
					endSelId = parseInt(elem.id);
					console.log("moves:" + endSelId);
					selectRectangleBorder();
					selectRectangle(getModeClass(),bAddClass,true);//display
				}
			});
		else 
			$tbody.on("touchmove", "td", function (event) {
				X = event.touches[0].clientX;
				Y = event.touches[0].clientY;
				elem = document.elementFromPoint(X, Y);
				if (bIsSelecting) {
					selectRectangle(0,false);//del
					endSelId = parseInt(elem.id);
					console.log("moves:" + endSelId);
					selectRectangleBorder();
					selectRectangle(getModeClass(),bAddClass,true);//display
				}
				//		event.preventDefault();
			});

		$tbody.on("click", "th", function (event) {
			setRowSel($(this));
		});
		$table.on("click", "thead th", function (event) {
			setColSel($(this));
		});

		$element.find('.ts-clearTimeSheet').click(function(){
			Clear();
		});

		$element.find('.ts-updateSetPoints').click(function(){
			updateSetPoints();
		});

		$element.find('.ts-DisableSetPoints').click(function(){
			updateSetPoints('false');
		});

		$element.find('.ts-EnableSetPoints').click(function(){
			updateSetPoints('true');
		});

		$element.find('.timeRange').change(function(){
			defaults.nbTicksPerHour = parseInt($(this).val());
			
			initPlanningTable();
			showPlanning();
		});

		$('#oew_oddeven').change(function () {
			defaults.odd_even_week = $(this).is(':checked');
			initPlanningTable();
			showPlanning();
		});

		if(defaults.hover_activate){
			$tbody.on("mouseover", "th" , function(event){
				$(this).parent().children("td").addClass("hover");
			});
			$tbody.on("mouseout", "th" , function(event){
				$(this).parent().children("td").removeClass("hover");
			});

			$thead.on("mouseover", "th", function (event) {
				findTdCol($(this)).addClass("hover");
			});

			$thead.on("mouseout", "th", function (event) {
				findTdCol($(this)).removeClass("hover");
			});
		}
	};

	initPlanningTable = function () {
		createTable();
			if(defaults.nbTicksPerHour >= 12){
			   $element.find('.ts-wrapper').removeClass('zoom zoom2').addClass((defaults.nbTicksPerHour === 12)?'zoom':'zoom2');

			}else 
				$element.find('.ts-wrapper').removeClass('zoom zoom2');

		$element.find('.timeRange option[value="' + defaults.nbTicksPerHour + '"]').prop('selected', true);
		if ($.myglobals.ismobile == true)
		{
			if (defaults.odd_even_week)
			{
				$tbody.find("tr").css({ "height": '22px' });
				if(defaults.nbTicksPerHour==2)
					$thead.find("tr").css({ "height": '20px' });
				else if(defaults.nbTicksPerHour==4)
					$thead.find("tr").css({ "height": '13px' });
				else if(defaults.nbTicksPerHour>=3)
					$thead.find("tr").css({ "height": '10px' });
			}			else {
//				$(".ts-planning tr").css({ "height": '40px' });
				$tbody.find("tr").css({ "height": '40px' });
				if(defaults.nbTicksPerHour==1)
					$thead.find("tr").css({ "height": '40px' });
				else if(defaults.nbTicksPerHour==2)
					$thead.find("tr").css({ "height": '20px' });
				else if(defaults.nbTicksPerHour==4)
					$thead.find("tr").css({ "height": '13px' });
				else if(defaults.nbTicksPerHour>=3)
					$thead.find("tr").css({ "height": '10px' });
			}
			$("#deviceTimers").css({ "width": 'auto' });

//			$(".navbar-inner").css({ "display": 'none' });
			$(".navbar-inner").hide();

			var bannercontentpaddingtop = $(".bannercontent").css("padding-top");
			var bannercontentpaddingbot = $(".bannercontent").css("padding-bottom");

			$(".bannercontent").css({ "padding-top": '0px' });
			$(".bannercontent").css({ "padding-bottom": '0px' });

			var containerfluid = [];
			containerfluid[0] = $(".container-fluid").css("padding-top"    );
			containerfluid[1] = $(".container-fluid").css("padding-bottom" );
			containerfluid[2] = $(".container-fluid").css("padding-left"   );
			containerfluid[3] = $(".container-fluid").css("padding-right"  );

			$(".container-fluid").css({ "padding-top"   : '0px' });
			$(".container-fluid").css({ "padding-bottom": '0px' });
			$(".container-fluid").css({ "padding-left"  : '0px' });
			$(".container-fluid").css({ "padding-right" : '0px' });

			$("#deviceTimers").on("remove", function () {
				$(".navbar-inner").show();
				$(".bannercontent").css({ "padding-top": bannercontentpaddingtop });
				$(".bannercontent").css({ "padding-bottom": bannercontentpaddingbot });

				$(".container-fluid").css({ "padding-top"   : containerfluid[0]  });
				$(".container-fluid").css({ "padding-bottom": containerfluid[1]  });
				$(".container-fluid").css({ "padding-left"  : containerfluid[3] });
				$(".container-fluid").css({ "padding-right" : containerfluid[4] });

			})
		}

//		$element.show();
	};

	$( document ).on( "timersLoaded", function(event, items){
		loadPlanning(items);
		//update show entry background to white
		var entry = $("select[name*='_length']")
		$(entry).css({ "background-color": 'white' });
	});

	$.each(planning_translations, function(idx, res){
		$.i18n.addResources(res.lng,'translation',res.res);
	});

	return initUI();
};
///////////////////////////////////////////

$( document ).on( "timersInitialized", function(event, vm,refreshTimers){
	var options = {"devid":vm.deviceIdx, "refreshCallback":function(){refreshTimers();} };
	if( !vm.isSetpointTimers && ((vm.isDimmer || vm.isSelector) || !vm.IsLED)){
		$.extend(options, {"commandAdd":"addtimer", "commandClear":"cleartimers","temperatureModes":false} );
		if((vm.isDimmer || vm.isSelector)) {

			var levels = vm.levelOptions;
			if(!vm.isCommandSelectionDisabled)
				levels.unshift({ "label": "Off", "value": 0 });
			if (levels.length > 10) {
				levels = [];
				for (var i = 0; i <= 5; i++) {
					levels[i] = { value: i*20 , label: (i*20).toString() + "%" }
				}
			}
			if (vm.isBlind ) {
				levels = [];
				levels[0] = { value: 0 , label: "Close" }
				for (var i = 1; i <= 5; i++) {
					levels[i] = { value: i * 20, label: "Open ("+(i * 20).toString() + "%)" }
				}
			}

					 
			$.extend(options, {
				"modes": $.map( levels, function(val,i){
					 return {"value":val.value, "class":"m"+ (val.value == null? i: val.value/10) , "name":val.label };}),
				"propValue":"Level",
				"propValueAjax":"level"});
		}
		else
			$.extend(options, {	"modes": [{"value":0,"name":$.t("On"), "class":"green"},{"value":1,"name":$.t("Off"), "class":"red"}],
							 "propValue":"Cmd",
							 "propValueAjax":"command"});
	}
	 options.device = vm.device;
	PlanningTimerSheet(options);
	$("#copyright").hide();
});
///////////////////////////////////////////
