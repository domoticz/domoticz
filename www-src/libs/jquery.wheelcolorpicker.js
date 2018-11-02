/**
 * jQuery Wheel Color Picker
 * 
 * https://raffer.one/projects/jquery-wheelcolorpicker
 * 
 * Author : Fajar Chandra
 * Date   : 2018.02.09
 * 
 * Copyright © 2011-2018 Fajar Chandra. All rights reserved.
 * Released under MIT License.
 * http://www.opensource.org/licenses/mit-license.php
 */

(function ($) {
	
	/**
	 * Function: wheelColorPicker
	 * 
	 * The wheelColorPicker plugin wrapper. Firing all functions and 
	 * setting/getting all options in this plugin should be called via 
	 * this function.
	 * 
	 * Before that, if wheelColorPicker instance is not yet initialized, 
	 * this will initialize ColorPicker widget.
	 * 
	 * This function will look for the options parameter passed in, and 
	 * try to do something as specified in this order:
	 * 1. If no argument is passed, then initialize the plugin or do nothing
	 * 2. If object is passed, then call setOptions()
	 * 3. If string is passed, then try to fire a method with that name
	 * 4. If string is passed and no method matches the name, then try 
	 *    to set/get an option with that name. If a setter/getter method 
	 *    available (i.e. setSomething), it will set/get that option via that method.
	 */
	$.fn.wheelColorPicker = function() {
		var returnValue = this; // Allows method chaining
		
		// Separate first argument and the rest..
		// First argument is used to determine function/option name
		// e.g. wheelColorPicker('setColor', { r: 0, g: 0, b: 1 })
		if(arguments.length > 0) {
			var shift = [].shift;
			var firstArg = shift.apply(arguments);
			var firstArgUc = (typeof firstArg === "string") ? firstArg.charAt(0).toUpperCase() + firstArg.slice(1) : firstArg;
		} 
		else {
			var firstArg = undefined;
			var firstArgUc = undefined;
		}
		var args = arguments;
		
		
		this.each(function() {
			
			// Grab ColorPicker object instance
			var instance = $(this).data('jQWCP.instance');
			
			// Initialize if not yet created
			if(instance == undefined || instance == null) {
				// Get init options
				var options = {};
				if(typeof firstArg === "object") {
					options = firstArg;
				}
				
				instance = new WCP.ColorPicker(this, options);
				$(this).data('jQWCP.instance', instance);
			}
			
			/// What to do? ///
			
			// No arguments provided, do nothing
			// wheelColorPicker()
			if(firstArg === undefined || typeof firstArg === "object") {
			}
			
			// Call a method
			// wheelColorPicker('show')
			else if(typeof instance[firstArg] === "function") {
				//console.log('method');
				var ret = instance[firstArg].apply(instance, args);
				
				// If instance is not returned, no method chaining
				if(ret !== instance) {
					returnValue = ret;
					return false;
				}
			}
			
			// Try option setter
			// wheelColorPicker('color', '#ff00aa')
			else if(typeof instance['set'+firstArgUc] === "function" && args.length > 0) {
				//console.log('setter');
				var ret = instance['set'+firstArgUc].apply(instance, args);
				
				// If instance is not returned, no method chaining
				if(ret !== instance) {
					returnValue = ret;
					return false;
				}
			}
			
			// Try option getter
			// wheelColorPicker('color')
			else if(typeof instance['get'+firstArgUc] === "function") {
				//console.log('getter');
				var ret = instance['get'+firstArgUc].apply(instance, args);
				
				// If instance is not returned, no method chaining
				if(ret !== instance) {
					returnValue = ret;
					return false;
				}
			}
			
			// Set option value
			// wheelColorPicker('format', 'hex')
			else if(instance.options[firstArg] !== undefined && args.length > 0) {
				//console.log('set option');
				instance.options[firstArg] = args[0];
			}
			
			// Get option value
			// wheelColorPicker('format')
			else if(instance.options[firstArg] !== undefined) {
				//console.log('get option');
				returnValue = instance.options[firstArg];
				return false;
			}
			
			// Nothing matches, throw error
			else {
				$.error( 'Method/option named ' +  firstArg + ' does not exist on jQuery.wheelColorPicker' );
			}
			
		});
		
		return returnValue;
	};



	/******************************************************************/

	/////////////////////////////////////////
	// Shorthand for $.fn.wheelColorPicker //
	/////////////////////////////////////////
	var WCP = $.fn.wheelColorPicker;
	/////////////////////////////////////////

	/**
	 * Object: defaults
	 * 
	 * Contains default options for the wheelColorPicker plugin.
	 * 
	 * Member properties:
	 * 
	 *   format        - <string> Color naming style. See colorToRgb for all 
	 *                   available formats.
	 *   live          - <boolean> Enable dynamic slider gradients.
	 *   preview       - <boolean> Enable live color preview on input field
	 *   userinput     - (Deprecated) <boolean> Enable picking color by typing directly
	 *   validate      - <boolean> When userinput is enabled, force the value to be 
	 *                   a valid format. If user input an invalid color, the value 
	 *                   will be reverted to last valid color.
     *   autoResize    - <boolean> Automatically resize container width.
     *                   If set to false, you could manually adjust size with CSS.
	 *   autoFormat    - <boolean> Automatically convert input value to 
	 *                   specified format. For example, if format is "rgb", 
	 *                   "#ff0000" will automatically converted into "rgb(255,0,0)".
	 *   color         - <object|string> Initial value in any of supported color 
	 *                   value format or as an object. Setting this value will 
	 *                   override the current input value.
	 *   alpha         - (Deprecated) <boolean> Force the color picker to use alpha value 
	 *                   despite its selected color format. This option is 
	 *                   deprecated. Use sliders = "a" instead.
	 *   inverseLabel  - (deprecated) Boolean use inverse color for 
	 *                   input label instead of black/white color.
	 *   preserveWheel - Boolean preserve color wheel shade when slider 
	 *                   position changes. If set to true, changing 
	 *                   color wheel from black will reset selectedColor.val 
	 *                   (shade) to 1.
	 *   interactive   - Boolean enable interactive sliders where slider bar
	 *                   gradients change dynamically as user drag a slider 
	 *                   handle. Set to false if this affect performance.
	 *                   See also 'quality' option if you wish to keep 
	 *                   interactive slider but with reduced quality.
	 *   cssClass      - Object CSS Classes to be added to the color picker.
	 *   layout        - String [block|popup] Layout mode.
	 *   animDuration  - Number Duration for transitions such as fade-in 
	 *                   and fade-out.
	 *   quality       - Rendering details quality. The normal quality is 1. 
	 *                   Setting less than 0.1 may make the sliders ugly, 
	 *                   while setting the value too high might affect performance.
	 *   sliders       - String combination of sliders. If null then the color 
	 *                   picker will show default values, which is "wvp" for 
	 *                   normal color or "wvap" for color with alpha value. 
	 *                   Possible combinations are "whsvrgbap". 
     *                   Order of letters will affect slider positions.
	 *   sliderLabel   - Boolean Show labels for each slider.
	 *   sliderValue   - Boolean Show numeric value of each slider.
	 *   hideKeyboard  - Boolean Keep input blurred to avoid on screen keyboard appearing. 
	 *                   If this is set to true, avoid assigning handler to blur event.
	 *   rounding      - Round the alpha value to N decimal digits. Default is 2.
	 *                   Set -1 to disable rounding.
	 *   mobile        - Display mobile-friendly layout when opened in mobile device.
     *   mobileWidth   - Max screen width to use mobile layout instead of default one.
	 *   mobileAutoScroll - Automatically scroll the page if focused input element 
	 *                      gets obstructed by color picker dialog.
	 *   htmlOptions   - Load options from HTML attributes. 
	 *                   To set options using HTML attributes, 
	 *                   prefix these options with 'data-wcp-' as attribute names.
     *   snap          - Snap color wheel and slider on 0, 0.5, and 1.0
     *   snapTolerance - Snap if slider position falls within defined 
     *                   tolerance value.
	 */
	WCP.defaults = {
		format: 'hex', /* 1.x */
		preview: false, /* 1.x */
		live: true, /* 2.0 */
		userinput: true, /* DEPRECATED 1.x */
		validate: true, /* 1.x */
		autoResize: true, /* 3.0 */
		autoFormat: true, /* 3.0 */
		//color: null, /* DEPRECATED 1.x */ /* OBSOLETE 3.0 */ /* Init-time only */
		//alpha: null, /* DEPRECATED 1.x */ /* OBSOLETE 3.0 */ /* See methods.alpha */
		preserveWheel: null, /* DEPRECATED 1.x */ /* Use live */
		cssClass: '', /* 2.0 */
		layout: 'popup', /* 2.0 */ /* Init-time only */
		animDuration: 200, /* 2.0 */
		quality: 1, /* 2.0 */
		sliders: null, /* 2.0 */
		//sliderLabel: true, /* 2.0 */ /* NOT IMPLEMENTED */
		//sliderValue: false, /* 2.0 */ /* NOT IMPLEMENTED */
		rounding: 2, /* 2.3 */
		mobile: true, /* 3.0 */
        mobileWidth: 480, /* 3.0 */
		hideKeyboard: false, /* 2.4 */
		htmlOptions: true, /* 2.3 */
        snap: false, /* 2.5 */
		snapTolerance: 0.05 /* 2.5 */
	};



	/******************************************************************/

	//////////////////////////////
	// STATIC OBJECTS AND FLAGS //
	//////////////////////////////

	/* 
	 * Note: To determine input position (top and left), use the following:
	 * WCP.ORIGIN.top + this.input.getBoundingClientRect().top
	 * instead of using $(this.input).offset().top because on mobile browsers 
	 * (chrome) jQuery's offset() function returns wrong value.
	 */

	/// Top left of the page is not on (0,0), making window.scrollX/Y and offset() useless
	/// See WCP.ORIGIN
	WCP.BUG_RELATIVE_PAGE_ORIGIN = false;

	/// Coordinate of the top left page (mobile chrome workaround)
	WCP.ORIGIN = { left: 0, top: 0 };
	
	
	/******************************************************************/

	//////////////////////
	// HELPER FUNCTIONS //
	//////////////////////

	/**
	 * Function: colorToStr
	 * 
	 * Since 2.0
	 * 
	 * Convert color object to string in specified format
	 * 
	 * Available formats:
	 * - hex
	 * - hexa
	 * - css
	 * - cssa
	 * - rgb
	 * - rgb%
	 * - rgba
	 * - rgba%
	 * - hsv
	 * - hsv%
	 * - hsva
	 * - hsva%
	 * - hsb
	 * - hsb%
	 * - hsba
	 * - hsba%
	 */
	WCP.colorToStr = function( color, format ) {
		var result = "";
		switch( format ) {
			case 'css':
				result = "#";
			case 'hex': 
				var r = Math.round(color.r * 255).toString(16);
				if( r.length == 1) {
					r = "0" + r;
				}
				var g = Math.round(color.g * 255).toString(16);
				if( g.length == 1) {
					g = "0" + g;
				}
				var b = Math.round(color.b * 255).toString(16);
				if( b.length == 1) {
					b = "0" + b;
				}
				result += r + g + b;
				break;
				
			case 'cssa':
				result = "#";
			case 'hexa': 
				var r = Math.round(color.r * 255).toString(16);
				if( r.length == 1) {
					r = "0" + r;
				}
				var g = Math.round(color.g * 255).toString(16);
				if( g.length == 1) {
					g = "0" + g;
				}
				var b = Math.round(color.b * 255).toString(16);
				if( b.length == 1) {
					b = "0" + b;
				}
				var a = Math.round(color.a * 255).toString(16);
				if( a.length == 1) {
					a = "0" + a;
				}
				result += r + g + b + a;
				break;
				
			case 'rgb':
				result = "rgb(" + 
					Math.round(color.r * 255) + "," + 
					Math.round(color.g * 255) + "," + 
					Math.round(color.b * 255) + ")";
				break;
				
			case 'rgb%':
				result = "rgb(" + 
					(color.r * 100) + "%," + 
					(color.g * 100) + "%," + 
					(color.b * 100) + "%)";
				break;
				
			case 'rgba':
				result = "rgba(" + 
					Math.round(color.r * 255) + "," + 
					Math.round(color.g * 255) + "," + 
					Math.round(color.b * 255) + "," + 
					color.a + ")";
				break;
				
			case 'rgba%':
				result = "rgba(" + 
					(color.r * 100) + "%," + 
					(color.g * 100) + "%," + 
					(color.b * 100) + "%," + 
					(color.a * 100) + "%)";
				break;
				
			case 'hsv':
				result = "hsv(" + 
					(color.h * 360) + "," + 
					color.s + "," + 
					color.v + ")";
				break;
				
			case 'hsv%':
				result = "hsv(" + 
					(color.h * 100) + "%," + 
					(color.s * 100) + "%," + 
					(color.v * 100) + "%)";
				break;
				
			case 'hsva':
				result = "hsva(" + 
					(color.h * 360) + "," + 
					color.s + "," + 
					color.v + "," + 
					color.a + ")";
				break;
				
			case 'hsva%':
				result = "hsva(" + 
					(color.h * 100) + "%," + 
					(color.s * 100) + "%," + 
					(color.v * 100) + "%," + 
					(color.a * 100) + "%)";
				break;
				
				
			case 'hsb':
				result = "hsb(" + 
					color.h + "," + 
					color.s + "," + 
					color.v + ")";
				break;
				
			case 'hsb%':
				result = "hsb(" + 
					(color.h * 100) + "%," + 
					(color.s * 100) + "%," + 
					(color.v * 100) + "%)";
				break;
				
			case 'hsba':
				result = "hsba(" + 
					color.h + "," + 
					color.s + "," + 
					color.v + "," + 
					color.a + ")";
				break;
				
			case 'hsba%':
				result = "hsba(" + 
					(color.h * 100) + "%," + 
					(color.s * 100) + "%," + 
					(color.v * 100) + "%," + 
					(color.a * 100) + "%)";
				break;
				
		}
		return result;
	};


	/**
	 * Function: strToColor
	 * 
	 * Since 2.0
	 * 
	 * Convert string to color object.
	 * Please note that if RGB color is supplied, the returned value 
	 * will only contain RGB.
	 * 
	 * If invalid string is supplied, FALSE will be returned.
	 */
	WCP.strToColor = function( val ) {
		var color = { a: 1 };
		var tmp;
		var hasAlpha;
		
		// #fff
		// #ffff
		if(
			val.match(/^#[0-9a-f]{3}$/i) != null ||
			val.match(/^#[0-9a-f]{4}$/i)
		) {
			if( isNaN( color.r = parseInt(val.substr(1, 1), 16) * 17 / 255 ) ) {
				return false;
			}
			if( isNaN( color.g = parseInt(val.substr(2, 1), 16) * 17 / 255 ) ) {
				return false;	
			}
			if( isNaN( color.b = parseInt(val.substr(3, 1), 16) * 17 / 255 ) ) {
				return false;
			}
			
			// Alpha
			if(val.length == 5) {
				if( isNaN( color.a = parseInt(val.substr(4, 1), 16) * 17 / 255 ) ) {
					return false;
				}
			}
		}
		
		// fff
		// ffff
		else if(
			val.match(/^[0-9a-f]{3}$/i) != null ||
			val.match(/^[0-9a-f]{4}$/i) != null
		) {
			if( isNaN( color.r = parseInt(val.substr(0, 1), 16) * 17 / 255 ) ) {
				return false;
			}
			if( isNaN( color.g = parseInt(val.substr(1, 1), 16) * 17 / 255 ) ) {
				return false;	
			}
			if( isNaN( color.b = parseInt(val.substr(2, 1), 16) * 17 / 255 ) ) {
				return false;
			}
			
			// Alpha
			if(val.length == 4) {
				if( isNaN( color.a = parseInt(val.substr(3, 1), 16) * 17 / 255 ) ) {
					return false;
				}
			}
		}
		
		// #ffffff
		// #ffffffff
		else if(
			val.match(/^#[0-9a-f]{6}$/i) != null ||
			val.match(/^#[0-9a-f]{8}$/i) != null
		) {
			if( isNaN( color.r = parseInt(val.substr(1, 2), 16) / 255 ) ) {
				return false;
			}
			if( isNaN( color.g = parseInt(val.substr(3, 2), 16) / 255 ) ) {
				return false;	
			}
			if( isNaN( color.b = parseInt(val.substr(5, 2), 16) / 255 ) ) {
				return false;
			}
			
			// Alpha
			if(val.length == 9) {
				if( isNaN( color.a = parseInt(val.substr(7, 2), 16) / 255 ) ) {
					return false;
				}
			}
		}
		
		// ffffff
		// ffffffff
		else if(
			val.match(/^[0-9a-f]{6}$/i) != null ||
			val.match(/^[0-9a-f]{8}$/i) != null
		) {
			if( isNaN( color.r = parseInt(val.substr(0, 2), 16) / 255 ) ) {
				return false;
			}
			if( isNaN( color.g = parseInt(val.substr(2, 2), 16) / 255 ) ) {
				return false;
			}
			if( isNaN( color.b = parseInt(val.substr(4, 2), 16) / 255 ) ) {
				return false;
			}
			
			// Alpha
			if(val.length == 8) {
				if( isNaN( color.a = parseInt(val.substr(6, 2), 16) / 255 ) ) {
					return false;
				}
			}
		}
		
		// rgb(100%,100%,100%)
		// rgba(100%,100%,100%,100%)
		// rgba(255,255,255,1)
		// rgba(100%,1, 0.5,.2)
		else if(
			val.match(/^rgba\s*\(\s*([0-9\.]+%|[01]?\.?[0-9]*)\s*,\s*([0-9\.]+%|[01]?\.?[0-9]*)\s*,\s*([0-9\.]+%|[01]?\.?[0-9]*)\s*,\s*([0-9\.]+%|[01]?\.?[0-9]*)\s*\)$/i) != null ||
			val.match(/^rgb\s*\(\s*([0-9\.]+%|[01]?\.?[0-9]*)\s*,\s*([0-9\.]+%|[01]?\.?[0-9]*)\s*,\s*([0-9\.]+%|[01]?\.?[0-9]*)\s*\)$/i) != null 
		) {
			if(val.match(/a/i) != null) {
				hasAlpha = true;
			}
			else {
				hasAlpha = false;
			}
			
			tmp = val.substring(val.indexOf('(')+1, val.indexOf(','));
			if( tmp.charAt( tmp.length-1 ) == '%') {
				if( isNaN( color.r = parseFloat(tmp) / 100 ) ) {
					return false;
				}
			}
			else {
				if( isNaN( color.r = parseInt(tmp) / 255 ) ) {
					return false;
				}
			}
			
			tmp = val.substring(val.indexOf(',')+1, val.indexOf(',', val.indexOf(',')+1));
			if( tmp.charAt( tmp.length-1 ) == '%') {
				if( isNaN( color.g = parseFloat(tmp) / 100 ) ) {
					return false;
				}
			}
			else {
				if( isNaN( color.g = parseInt(tmp) / 255 ) ) {
					return false;
				}
			}
			
			if(hasAlpha) {
				tmp = val.substring(val.indexOf(',', val.indexOf(',')+1)+1, val.lastIndexOf(','));
			}
			else {
				tmp = val.substring(val.lastIndexOf(',')+1, val.lastIndexOf(')'));
			}
			if( tmp.charAt( tmp.length-1 ) == '%') {
				if( isNaN( color.b = parseFloat(tmp) / 100 ) ) {
					return false;
				}
			}
			else {
				if( isNaN( color.b = parseInt(tmp) / 255 ) ) {
					return false;
				}
			}
			
			if(hasAlpha) {
				tmp = val.substring(val.lastIndexOf(',')+1, val.lastIndexOf(')'));
				if( tmp.charAt( tmp.length-1 ) == '%') {
					if( isNaN( color.a = parseFloat(tmp) / 100 ) ) {
						return false;
					}
				}
				else {
					if( isNaN( color.a = parseFloat(tmp) ) ) {
						return false;
					}
				}
			}
		}
		
		// hsv(100%,100%,100%)
		// hsva(100%,100%,100%,100%)
		// hsv(360,1,1,1)
		// hsva(360,1, 0.5,.2)
		// hsb(100%,100%,100%)
		// hsba(100%,100%,100%,100%)
		// hsb(360,1,1,1)
		// hsba(360,1, 0.5,.2)
		else if(
			val.match(/^hsva\s*\(\s*([0-9\.]+%|[01]?\.?[0-9]*)\s*,\s*([0-9\.]+%|[01]?\.?[0-9]*)\s*,\s*([0-9\.]+%|[01]?\.?[0-9]*)\s*,\s*([0-9\.]+%|[01]?\.?[0-9]*)\s*\)$/i) != null ||
			val.match(/^hsv\s*\(\s*([0-9\.]+%|[01]?\.?[0-9]*)\s*,\s*([0-9\.]+%|[01]?\.?[0-9]*)\s*,\s*([0-9\.]+%|[01]?\.?[0-9]*)\s*\)$/i) != null ||
			val.match(/^hsba\s*\(\s*([0-9\.]+%|[01]?\.?[0-9]*)\s*,\s*([0-9\.]+%|[01]?\.?[0-9]*)\s*,\s*([0-9\.]+%|[01]?\.?[0-9]*)\s*,\s*([0-9\.]+%|[01]?\.?[0-9]*)\s*\)$/i) != null ||
			val.match(/^hsb\s*\(\s*([0-9\.]+%|[01]?\.?[0-9]*)\s*,\s*([0-9\.]+%|[01]?\.?[0-9]*)\s*,\s*([0-9\.]+%|[01]?\.?[0-9]*)\s*\)$/i) != null 
		) {
			if(val.match(/a/i) != null) {
				hasAlpha = true;
			}
			else {
				hasAlpha = false;
			}
			
			tmp = val.substring(val.indexOf('(')+1, val.indexOf(','));
			if( tmp.charAt( tmp.length-1 ) == '%') {
				if( isNaN( color.h = parseFloat(tmp) / 100 ) ) {
					return false;
				}
			}
			else {
				if( isNaN( color.h = parseFloat(tmp) / 360 ) ) {
					return false;
				}
			}
			
			tmp = val.substring(val.indexOf(',')+1, val.indexOf(',', val.indexOf(',')+1));
			if( tmp.charAt( tmp.length-1 ) == '%') {
				if( isNaN( color.s = parseFloat(tmp) / 100 ) ) {
					return false;
				}
			}
			else {
				if( isNaN( color.s = parseFloat(tmp) ) ) {
					return false;
				}
			}
			
			if(hasAlpha) {
				tmp = val.substring(val.indexOf(',', val.indexOf(',')+1)+1, val.lastIndexOf(','));
			}
			else {
				tmp = val.substring(val.lastIndexOf(',')+1, val.lastIndexOf(')'));
			}
			if( tmp.charAt( tmp.length-1 ) == '%') {
				if( isNaN( color.v = parseFloat(tmp) / 100 ) ) {
					return false;
				}
			}
			else {
				if( isNaN( color.v = parseFloat(tmp) ) ) {
					return false;
				}
			}
			
			if(hasAlpha) {
				tmp = val.substring(val.lastIndexOf(',')+1, val.lastIndexOf(')'));
				if( tmp.charAt( tmp.length-1 ) == '%') {
					if( isNaN( color.a = parseFloat(tmp) / 100 ) ) {
						return false;
					}
				}
				else {
					if( isNaN( color.a = parseFloat(tmp) ) ) {
						return false;
					}
				}
			}
		}
		
		else {
			return false;
		}
		
		return color;
	};


	/**
	 * Function: hsvToRgb
	 * 
	 * Since 2.0
	 * 
	 * Perform HSV to RGB conversion
	 */
	WCP.hsvToRgb = function( h, s, v ) {
		
		// Calculate RGB from hue (1st phase)
		var cr = (h <= (1/6) || h >= (5/6)) ? 1
			: (h < (1/3) ? 1 - ((h - (1/6)) * 6)
			: (h > (4/6) ? (h - (4/6)) * 6
			: 0));
		var cg = (h >= (1/6) && h <= (3/6)) ? 1
			: (h < (1/6) ? h * 6
			: (h < (4/6) ? 1 - ((h - (3/6)) * 6)
			: 0));
		var cb = (h >= (3/6) && h <= (5/6)) ? 1
			: (h > (2/6) && h < (3/6) ? (h - (2/6)) * 6
			: (h > (5/6) ? 1 - ((h - (5/6)) * 6)
			: 0));
			
		//~ console.log(cr + ' ' + cg + ' ' + cb);
		
		// Calculate RGB with saturation & value applied
		var r = (cr + (1-cr)*(1-s)) * v;
		var g = (cg + (1-cg)*(1-s)) * v;
		var b = (cb + (1-cb)*(1-s)) * v;
		
		//~ console.log(r + ' ' + g + ' ' + b + ' ' + v);
		
		return { r: r, g: g, b: b };
	};


	/**
	 * Function: rgbToHsv
	 * 
	 * Since 2.0
	 * 
	 * Perform RGB to HSV conversion
	 */
	WCP.rgbToHsv = function( r, g, b ) {
		
		var h;
		var s;
		var v;

		var maxColor = Math.max(r, g, b);
		var minColor = Math.min(r, g, b);
		var delta = maxColor - minColor;
		
		// Calculate saturation
		if(maxColor != 0) {
			s = delta / maxColor;
		}
		else {
			s = 0;
		}
		
		// Calculate hue
		// To simplify the formula, we use 0-6 range.
		if(delta == 0) {
			h = 0;
		}
		else if(r == maxColor) {
			h = (6 + (g - b) / delta) % 6;
		}
		else if(g == maxColor) {
			h = 2 + (b - r) / delta;
		}
		else if(b == maxColor) {
			h = 4 + (r - g) / delta;
		}
		else {
			h = 0;
		}
		// Then adjust the range to be 0-1
		h = h/6;
		
		// Calculate value
		v = maxColor;
		
		//~ console.log(h + ' ' + s + ' ' + v);
		
		return { h: h, s: s, v: v };
	};



	/******************************************************************/

	////////////////////////
	// COLOR PICKER CLASS //
	////////////////////////

	/**
	 * Class: ColorPicker
	 * 
	 * Since 3.0
	 */
	WCP.ColorPicker = function ( elm, options ) {
		
		// Assign reference to input DOM element
		this.input = elm;
		
		// Setup selected color in the following priority:
		// 1. options.color
		// 2. input.value
		// 3. default
		this.color = { h: 0, s: 0, v: 1, r: 1, g: 1, b: 1, a: 1, t: 0.5, w: 1, m: 1 };
		this.setValue(this.input.value);
		
		// Set options
		this.options = $.extend(true, {}, WCP.defaults);
		this.setOptions(options);
		
		// Check sliders option, if not defined, set default sliders
		if(this.options.sliders == null)
			this.options.sliders = 'wvp' + (this.options.format.indexOf('a') >= 0 ? 'a' : '');
		
		this.init();
	};


	////////////////////
	// Static members //
	////////////////////


	/**
	 * Static Property: ColorPicker.widget
	 * 
	 * Reference to global color picker widget (popup)
	 */
	WCP.ColorPicker.widget = null;


	/**
	 * Property: ColorPicker.overlay
	 * 
	 * Reference to overlay DOM element (overlay for global popup)
	 */
	WCP.ColorPicker.overlay = null;


	/**
	 * Function: init
	 * 
	 * Since 3.0
	 * 2.0 was methods.staticInit
	 * 
	 * Initialize wheel color picker globally.
	 */
	WCP.ColorPicker.init = function() {
		// Only perform initialization once
		if(WCP.ColorPicker.init.hasInit == true)
			return;
		WCP.ColorPicker.init.hasInit = true;
			
		// Insert overlay element to handle popup closing
		// when hideKeyboard is true, hence input is always blurred
		var $overlay = $('<div class="jQWCP-overlay" style="display: none;"></div>');
		$overlay.on('click', WCP.Handler.overlay_click);
		WCP.ColorPicker.overlay = $overlay.get(0);
		$('body').append($overlay);
        
        // Insert CSS for color wheel
        var wheelImage = WCP.ColorPicker.getWheelDataUrl(200);
        $('head').append(
            '<style type="text/css">' + 
                '.jQWCP-wHueWheel {' + 
                    'background-image: url(' + wheelImage + ');' +
                '}' +
            '</style>'
        );
		
		// Insert CSS for temperature color wheel
        var temperatureWheelImage = WCP.ColorPicker.getTemperatureWheelDataUrl(200);
        $('head').append(
            '<style type="text/css">' + 
                '.jQWCP-wTemperatureWheel {' + 
                    'background-image: url(' + temperatureWheelImage + ');' +
                '}' +
            '</style>'
        );
		
		// Attach events
		$('html').on('mouseup.wheelColorPicker', WCP.Handler.html_mouseup);
		$('html').on('touchend.wheelColorPicker', WCP.Handler.html_mouseup);
		$('html').on('mousemove.wheelColorPicker', WCP.Handler.html_mousemove);
		$('html').on('touchmove.wheelColorPicker', WCP.Handler.html_mousemove);
        $(window).on('resize.wheelColorPicker', WCP.Handler.window_resize);
	};


	/**
	 * Function: createWidget
	 * 
	 * Since 3.0
	 * 2.5 was private.initWidget
	 * 
	 * Create color picker widget.
	 */
	WCP.ColorPicker.createWidget = function() {
		/// WIDGET ///
		// Notice: We won't use canvas to draw the color wheel since 
		// it may takes time and cause performance issue.
		var $widget = $(
			"<div class='jQWCP-wWidget'>" + 
				"<div class='jQWCP-wWheel jQWCP-wHueWheel '>" + 
					"<div class='jQWCP-wWheelOverlay'></div>" +
					"<span class='jQWCP-wWheelCursor jQWCP-wHueWheelCursor'></span>" +
				"</div>" +
				"<div class='jQWCP-wWheel jQWCP-wTemperatureWheel'>" + 
					//"<div class='jQWCP-wTemperatureWheelOverlay'></div>" +
					"<span class='jQWCP-wWheelCursor jQWCP-wTemperatureWheelCursor'></span>" +
				"</div>" +
				"<div class='jQWCP-wHue jQWCP-slider-wrapper'>" +
					"<canvas class='jQWCP-wHueSlider jQWCP-slider' width='1' height='50' title='Hue'></canvas>" +
					"<span class='jQWCP-wHueCursor jQWCP-scursor'></span>" +
				"</div>" +
				"<div class='jQWCP-wSat jQWCP-slider-wrapper'>" +
					"<canvas class='jQWCP-wSatSlider jQWCP-slider' width='1' height='50' title='Saturation'></canvas>" +
					"<span class='jQWCP-wSatCursor jQWCP-scursor'></span>" +
				"</div>" +
				"<div class='jQWCP-wVal jQWCP-slider-wrapper'>" +
					"<canvas class='jQWCP-wValSlider jQWCP-slider' width='1' height='50' title='Value'></canvas>" +
					"<span class='jQWCP-wValCursor jQWCP-scursor'></span>" +
				"</div>" +
				"<div class='jQWCP-wRed jQWCP-slider-wrapper'>" +
					"<canvas class='jQWCP-wRedSlider jQWCP-slider' width='1' height='50' title='Red'></canvas>" +
					"<span class='jQWCP-wRedCursor jQWCP-scursor'></span>" +
				"</div>" +
				"<div class='jQWCP-wGreen jQWCP-slider-wrapper'>" +
					"<canvas class='jQWCP-wGreenSlider jQWCP-slider' width='1' height='50' title='Green'></canvas>" +
					"<span class='jQWCP-wGreenCursor jQWCP-scursor'></span>" +
				"</div>" +
				"<div class='jQWCP-wBlue jQWCP-slider-wrapper'>" +
					"<canvas class='jQWCP-wBlueSlider jQWCP-slider' width='1' height='50' title='Blue'></canvas>" +
					"<span class='jQWCP-wBlueCursor jQWCP-scursor'></span>" +
				"</div>" +
				"<div class='jQWCP-wAlpha jQWCP-slider-wrapper'>" +
					"<canvas class='jQWCP-wAlphaSlider jQWCP-slider' width='1' height='50' title='Alpha'></canvas>" +
					"<span class='jQWCP-wAlphaCursor jQWCP-scursor'></span>" +
				"</div>" +
				"<div class='jQWCP-wTemperature jQWCP-slider-wrapper'>" +
					"<canvas class='jQWCP-wTemperatureSlider jQWCP-slider' width='1' height='50' title='Temperature'></canvas>" +
					"<span class='jQWCP-wTemperatureCursor jQWCP-scursor'></span>" +
				"</div>" +
				"<div class='jQWCP-wWhite jQWCP-slider-wrapper'>" +
					"<canvas class='jQWCP-wWhiteSlider jQWCP-slider' width='1' height='50' title='White'></canvas>" +
					"<span class='jQWCP-wWhiteCursor jQWCP-scursor'></span>" +
				"</div>" +
				"<div class='jQWCP-wMaster jQWCP-slider-wrapper'>" +
					"<canvas class='jQWCP-wMasterSlider jQWCP-slider' width='1' height='50' title='Master'></canvas>" +
					"<span class='jQWCP-wMasterCursor jQWCP-scursor'></span>" +
				"</div>" +
				"<div class='jQWCP-wPreview'>" +
					"<canvas class='jQWCP-wPreviewBox' width='1' height='1' title='Selected Color'></canvas>" +
				"</div>" +
			"</div>"
		);
			
		// Small UI fix to disable highlighting the widget
		// Also UI fix to disable touch context menu 
		$widget.find('.jQWCP-wWheel, .jQWCP-slider-wrapper, .jQWCP-scursor, .jQWCP-slider')
			.attr('unselectable', 'on')
			.css('-moz-user-select', 'none')
			.css('-webkit-user-select', 'none')
			.css('user-select', 'none')
			.css('-webkit-touch-callout', 'none');
			
		// Disable context menu on sliders
		// Workaround for touch browsers
		$widget.on('contextmenu.wheelColorPicker', function() { return false; });
			
		// Bind widget events
		$widget.on('mousedown.wheelColorPicker', '.jQWCP-wWheel', WCP.Handler.wheel_mousedown);
		$widget.on('touchstart.wheelColorPicker', '.jQWCP-wWheel', WCP.Handler.wheel_mousedown);
		$widget.on('mousedown.wheelColorPicker', '.jQWCP-wWheelCursor', WCP.Handler.wheelCursor_mousedown);
		$widget.on('touchstart.wheelColorPicker', '.jQWCP-wWheelCursor', WCP.Handler.wheelCursor_mousedown);
		$widget.on('mousedown.wheelColorPicker', '.jQWCP-slider', WCP.Handler.slider_mousedown);
		$widget.on('touchstart.wheelColorPicker', '.jQWCP-slider', WCP.Handler.slider_mousedown);
		$widget.on('mousedown.wheelColorPicker', '.jQWCP-scursor', WCP.Handler.sliderCursor_mousedown);
		$widget.on('touchstart.wheelColorPicker', '.jQWCP-scursor', WCP.Handler.sliderCursor_mousedown);
		
		return $widget.get(0);
	};
    
    
    /**
     * Function: getWheelDataUrl
     * 
     * Create color wheel image and return as base64 encoded data url.
     */
    WCP.ColorPicker.getWheelDataUrl = function( size ) {
        var r = size / 2; // radius
        var center = r;
        var canvas = document.createElement('canvas');
        canvas.width = size;
        canvas.height = size;
        var context = canvas.getContext('2d');
        
        // Fill the wheel with colors
        for(var y = 0; y < size; y++) {
            for(var x = 0; x < size; x++) {
                // Get the offset from central position
                var offset = Math.sqrt(Math.pow(x - center, 2) + Math.pow(y - center, 2));
                
                // Skip pixels outside picture area (plus 2 pixels)
                if(offset > r + 2) {
                    continue;
                }
                
                // Get the position in degree (hue)
                var deg = (
                    (x - center == 0 
                        ? (y < center ? 90 : 270)
                        : (Math.atan((center - y) / (x - center)) / Math.PI * 180)
                    )
                    + (x < center ? 180 : 0)
                    + 360
                ) % 360;
                
                // Relative offset (sat)
                var sat = offset / r;
                
                // Value is always 1
                var val = 1;
                
                // Calculate color
                var cr = (Math.abs(deg + 360) + 60) % 360 < 120 ? 1
                    : (deg > 240 ? (120 - Math.abs(deg - 360)) / 60
                    : (deg < 120 ? (120 - deg) / 60
                    : 0));
                var cg = Math.abs(deg - 120) < 60 ? 1
                    : (Math.abs(deg - 120) < 120 ? (120 - Math.abs(deg - 120)) / 60
                    : 0);
                
                var cb = Math.abs(deg - 240) < 60 ? 1
                    : (Math.abs(deg - 240) < 120 ? (120 - Math.abs(deg - 240)) / 60
                    : 0);
                var pr = Math.round((cr + (1 - cr) * (1 - sat)) * 255);
                var pg = Math.round((cg + (1 - cg) * (1 - sat)) * 255);
                var pb = Math.round((cb + (1 - cb) * (1 - sat)) * 255);
                
                context.fillStyle = 'rgb(' + pr + ',' + pg + ',' + pb + ')';
                context.fillRect(x, y, 1, 1);
            }
        }
        
        return canvas.toDataURL();
    };


	/**
     * Function: getWheelDataUrl
     * 
     * Create color wheel image and return as base64 encoded data url.
     */
    WCP.ColorPicker.getTemperatureWheelDataUrl = function( size ) {
        var r = size / 2; // radius
        var center = r;
        var canvas = document.createElement('canvas');
        canvas.width = size;
        canvas.height = size;
        var context = canvas.getContext('2d');
        
		// Fill the wheel with temperature gradient
		var temperatureGradient = context.createLinearGradient(0, 0, 0, size);
		temperatureGradient.addColorStop(0.00, "#FF8500");
		temperatureGradient.addColorStop(0.02, "#FF8F24");
		temperatureGradient.addColorStop(0.06, "#FF9F43");
		temperatureGradient.addColorStop(0.11, "#FFB062");
		temperatureGradient.addColorStop(0.16, "#FFC081");
		temperatureGradient.addColorStop(0.23, "#FFD0A0");
		temperatureGradient.addColorStop(0.30, "#FFDFC0");
		temperatureGradient.addColorStop(0.40, "#FFEDE1");
		temperatureGradient.addColorStop(0.51, "#FCF7FF");
		temperatureGradient.addColorStop(0.67, "#DEE6FF");
		temperatureGradient.addColorStop(0.99, "#C0D3FF");
		temperatureGradient.addColorStop(1.00, "#BFD3FF");
		context.fillStyle = temperatureGradient;
		context.fillRect(0, 0, size, size);
        
        return canvas.toDataURL();
    };


	/////////////
	// Members //
	/////////////


	/**
	 * Property: ColorPicker.options
	 * 
	 * Plugin options for the color picker instance, extended from WCP.defaults.
	 */
	WCP.ColorPicker.prototype.options = null;


	/**
	 * Property: ColorPicker.input
	 * 
	 * Reference to input DOM element
	 */
	WCP.ColorPicker.prototype.input = null;


	/** 
	 * Property: ColorPicker.widget
	 * 
	 * Reference to widget DOM element (global popup or private inline widget)
	 */
	WCP.ColorPicker.prototype.widget = null;


	/**
	 * Property: ColorPicker.color
	 * 
	 * Selected color object.
	 */
	WCP.ColorPicker.prototype.color = null;


	/**
	 * Property: ColorPicker.lastValue
	 * 
	 * Store last input value
	 */
	WCP.ColorPicker.prototype.lastValue = null;


	/**
	 * Function: ColorPicker.setOptions
	 * 
	 * Since 3.0
	 * 
	 * Set options to the color picker. If htmlOptions is set to true, 
	 * options set via html attributes are also reloaded. If both html 
	 * attribute and argument exists, option set via options argument 
	 * gets priority.
	 */
	WCP.ColorPicker.prototype.setOptions = function( options ) {
		
		// options should be a separate object (passed by value)
		// Make a copy of options
		options = $.extend(true, {}, options);
		
		// Load options from HTML attributes
		if(this.options.htmlOptions) {
			for(var key in WCP.defaults) {
				// Only if option key is valid and not set via function argument
				if(this.input.hasAttribute('data-wcp-'+key) && options[key] === undefined) {
					options[key] = this.input.getAttribute('data-wcp-'+key);
					// Change true/false string to boolean
					if(options[key] == 'true') {
						options[key] = true;
					}
					else if(options[key] == 'false') {
						options[key] = false;
					}
				}
			}
		}
		
		// Set options
		for(var key in options) {
			// Skip undefined option key
			if(this.options[key] === undefined)
				continue;
			
			var keyUc = key.charAt(0).toUpperCase() + key.slice(1);
			
			// If setter is available, try setting it via setter
			if(typeof this['set'+keyUc] === "function") {
				this['set'+keyUc](options[key]);
			}
			// Otherwise directly update options
			else {
				this.options[key] = options[key];
			}
		}
		
		return this; // Allow chaining
	};


	/**
	 * Function: ColorPicker.init
	 * 
	 * Initialize wheel color picker widget
	 */
	WCP.ColorPicker.prototype.init = function() {
		WCP.ColorPicker.init();

		// Initialization must only occur once
		if(this.hasInit == true)
			return;
		this.hasInit = true;
		
		var instance = this;
		var $input = $(this.input);
		var $widget = null;
		
		/// LAYOUT & BINDINGS ///
		// Setup block mode layout
		if( this.options.layout == 'block' ) {
			// Create widget
			this.widget = WCP.ColorPicker.createWidget();
			$widget = $(this.widget);
			
			// Store object instance reference
			$widget.data('jQWCP.instance', this);
			
			// Wrap widget around the input elm and put the input 
			// elm inside widget
			$widget.insertAfter(this.input);
			// Retain display CSS property
			if($input.css('display') == "inline") {
				$widget.css('display', "inline-block");
			}
			else {
				$widget.css('display', $input.css('display'));
			}
			$widget.append(this.input);
			$input.hide();
			
			// Add tabindex attribute to make the widget focusable
			if($input.attr('tabindex') != undefined) {
				$widget.attr('tabindex', $input.attr('tabindex'));
			}
			else {
				$widget.attr('tabindex', 0);
			}
			
			// Further widget adjustments based on options
			this.refreshWidget();
			
			// Draw shading
			this.redrawSliders(true);
			this.updateSliders();
			
			// Bind widget element events
			$widget.on('focus.wheelColorPicker', WCP.Handler.widget_focus_block);
			$widget.on('blur.wheelColorPicker', WCP.Handler.widget_blur_block);
		}
		
		// Setup popup mode layout
		else {
			// Only need to create one widget, used globally
			if(WCP.ColorPicker.widget == null) {
				WCP.ColorPicker.widget = WCP.ColorPicker.createWidget();
				$widget = $(WCP.ColorPicker.widget);
				
				// Assign widget to global
				$widget.hide();
				$('body').append($widget);
				
				// Bind popup events
				$widget.on('mousedown.wheelColorPicker', WCP.Handler.widget_mousedown_popup);
				//$widget.on('mouseup.wheelColorPicker', WCP.Handler.widget_mouseup_popup);
				
			}
			this.widget = WCP.ColorPicker.widget;
			
			// Bind input element events
			$input.on('focus.wheelColorPicker', WCP.Handler.input_focus_popup);
			$input.on('blur.wheelColorPicker', WCP.Handler.input_blur_popup);
		}
		
		// Bind input events
		$input.on('keyup.wheelColorPicker', WCP.Handler.input_keyup);
		$input.on('change.wheelColorPicker', WCP.Handler.input_change);
		
		// Set color value
		// DEPRECATED by 3.0
		if(typeof this.options.color == "object") {
			this.setColor(this.options.color);
			this.options.color = undefined;
		}
		else if(typeof this.options.color == "string") {
			this.setValue(this.options.color);
			this.options.color = undefined;
		}
		
		// Set readonly mode
		/* DEPRECATED */
		if(this.options.userinput) {
			$input.removeAttr('readonly');
		}
		else {
			$input.attr('readonly', true);
		}
	};


	/**
	 * Function: destroy
	 * 
	 * Destroy the color picker and return it to normal element.
	 */
	WCP.ColorPicker.prototype.destroy = function() {
		var $widget = $(this.widget);
		var $input = $(this.input);
		
		// Reset layout
		// No need to delete global popup
		if(this.options.layout == 'block') {
			// Check if active control is the same widget as destroyed widget, remove the reference if it's true
			var $control = $( $('body').data('jQWCP.activeControl') ); // Refers to slider wrapper or wheel
			if ($control.length) {
				var controlWidget = $control.closest('.jQWCP-wWidget');
				if ($widget.is(controlWidget)) {
					$('body').data('jQWCP.activeControl', null);
				}
			}
			
			$widget.before(this.input);
			$widget.remove();
			$input.show();
		}
		
		// Unbind events
		$input.off('focus.wheelColorPicker');
		$input.off('blur.wheelColorPicker');
		$input.off('keyup.wheelColorPicker');
		$input.off('change.wheelColorPicker');
		
		// Remove data
		$input.data('jQWCP.instance', null);
		
		// remove self
		delete this;
	};


	/**
	 * Function: refreshWidget
	 * 
	 * Since 3.0
	 * 2.5 was private.adjustWidget
	 * 
	 * Update widget to match current option values.
	 */
	WCP.ColorPicker.prototype.refreshWidget = function() {
		var $widget = $(this.widget);
		var options = this.options;
        var mobileLayout = false;
		
		// Set CSS classes
		$widget.attr('class', 'jQWCP-wWidget');
		if(options.layout == 'block') {
			$widget.addClass('jQWCP-block');
		}
		$widget.addClass(options.cssClass);
		//$widget.addClass(this.input.getAttribute('class'));
        
        // Check whether to use mobile layout
        if(window.innerWidth <= options.mobileWidth && options.layout != 'block' && options.mobile) {
            mobileLayout = true;
            $widget.addClass('jQWCP-mobile');
        }
		
		// Rearrange sliders
		$widget.find('.jQWCP-wWheel, .jQWCP-slider-wrapper, .jQWCP-wPreview')
			.hide()
			.addClass('hidden');
			
		for(var i in options.sliders) {
			var $slider = null;
			switch(this.options.sliders[i]) {
				case 'w':
					$slider = $widget.find('.jQWCP-wHueWheel'); break;
				case 'h':
					$slider = $widget.find('.jQWCP-wHue'); break;
				case 's':
					$slider = $widget.find('.jQWCP-wSat'); break;
				case 'v':
					$slider = $widget.find('.jQWCP-wVal'); break;
				case 'r':
					$slider = $widget.find('.jQWCP-wRed'); break;
				case 'g':
					$slider = $widget.find('.jQWCP-wGreen'); break;
				case 'b':
					$slider = $widget.find('.jQWCP-wBlue'); break;
				case 'a':
					$slider = $widget.find('.jQWCP-wAlpha'); break;
				case 'p':
					$slider = $widget.find('.jQWCP-wPreview'); break;
				case 'x':
					$slider = $widget.find('.jQWCP-wTemperatureWheel'); break;
				case 'k':
					$slider = $widget.find('.jQWCP-wTemperature'); break;
				case 'l':
					$slider = $widget.find('.jQWCP-wWhite'); break;
				case 'm':
					$slider = $widget.find('.jQWCP-wMaster'); break;
			}
			
			if($slider != null) {
				$slider.appendTo(this.widget);
				$slider.show().removeClass('hidden');
			}
		}
		
		// If widget is hidden, show it first so we can calculate dimensions correctly
		//var widgetIsHidden = false;
		//if($widget.is(':hidden')) {
			//widgetIsHidden = true;
			//$widget.css({ opacity: '0' }).show();
		//}
		
		// Adjust sliders height based on quality
		var sliderHeight = options.quality * 50;
		$widget.find('.jQWCP-slider').attr('height', sliderHeight);
		
		var $visElms = $widget.find('.jQWCP-wWheel, .jQWCP-slider-wrapper, .jQWCP-wPreview').not('.hidden');
			
		// Adjust container and sliders width
        // Only if not on mobile layout (force fixed on mobile)
		if(options.autoResize && !mobileLayout) {
			// Auto resize
			var width = 0
			
			// Set slider size first, then adjust container
			$visElms.css({ width: '', height: '' });
			
			$visElms.each(function(index, item) {
				var $item = $(item);
				width += parseFloat($item.css('margin-left').replace('px', '')) + parseFloat($item.css('margin-right').replace('px', '')) + $item.outerWidth();
			});
			$widget.css({ width: width + 'px' });
		}
		else {
			// Fixed size
			
			// Set container size first, then adjust sliders
			$widget.css({ width: '' });
			
			var $visHueWheel = $widget.find('.jQWCP-wHueWheel').not('.hidden');
			var $visTemperatureWheel = $widget.find('.jQWCP-wTemperatureWheel').not('.hidden');
			var $visSliders = $widget.find('.jQWCP-slider-wrapper, .jQWCP-wPreview').not('.hidden');
			$visHueWheel.css({ height: $widget.height() + 'px', width: $widget.height() });
			if($visHueWheel.length > 0) {
				var horzSpace = $widget.width() - $visHueWheel.outerWidth() - parseFloat($visHueWheel.css('margin-left').replace('px', '')) - parseFloat($visHueWheel.css('margin-right').replace('px', ''));
			}
			else {
				var horzSpace = $widget.width();
			}
			$visTemperatureWheel.css({ height: $widget.height() + 'px', width: $widget.height() });
			if($visTemperatureWheel.length > 0) {
				var horzSpace = $widget.width() - $visTemperatureWheel.outerWidth() - parseFloat($visTemperatureWheel.css('margin-left').replace('px', '')) - parseFloat($visTemperatureWheel.css('margin-right').replace('px', ''));
			}
			else {
				var horzSpace = $widget.width();
			}/*if($visTemperatureWheel.length > 0) {
				horzSpace = horzSpace - $visTemperatureWheel.outerWidth() - parseFloat($visTemperatureWheel.css('margin-left').replace('px', '')) - parseFloat($visTemperatureWheel.css('margin-right').replace('px', ''));
			}*/
			if($visSliders.length > 0) {
				var sliderMargins = parseFloat($visSliders.css('margin-left').replace('px', '')) + parseFloat($visSliders.css('margin-right').replace('px', ''));
				$visSliders.css({ height: $widget.height() + 'px', width: (horzSpace - ($visSliders.length - 1) * sliderMargins) / $visSliders.length + 'px' });
			}
		}
		
		// Reset visibility
		//if(widgetIsHidden) {
			//$widget.css({ opacity: '' }).hide();
		//}
		
		return this; // Allows method chaining
	};


	/**
	 * Function: redrawSliders
	 * 
	 * Introduced in 2.0
	 * 
	 * Redraw slider gradients. Hidden sliders are not redrawn as to 
	 * improve performance. If options.live is FALSE, sliders are not redrawn.
	 * 
	 * Parameter:
	 *   force   - Boolean force redraw all sliders.
	 */
	WCP.ColorPicker.prototype.redrawSliders = function( force ) {
		
		// Skip if widget not yet initialized
		if(this.widget == null) 
			return this;
		
		var $widget = $(this.widget);
		
		// DEPRECATED 3.0
		// In 2.0, parameters are ( sliders, force )
		if(typeof arguments[0] === "string") {
			force = arguments[1];
		}
		
		// No need to redraw sliders on global popup widget if not 
		// attached to the input elm in current iteration
		if(this != $widget.data('jQWCP.instance'))
			return this;
			
		var options = this.options;
		var color = this.color;
			
		var w = 1;
		var h = options.quality * 50;
		
		var A = 1;
		var R = 0;
		var G = 0;
		var B = 0;
		var H = 0;
		var S = 0;
		var V = 1;
		
		// Dynamic colors
		if(options.live) {
			A = color.a;
			R = Math.round(color.r * 255);
			G = Math.round(color.g * 255);
			B = Math.round(color.b * 255);
			H = color.h;
			S = color.s;
			V = color.v;
		}
		
		/// PREVIEW ///
		// Preview box must always be redrawn, if not hidden
		var $previewBox = $widget.find('.jQWCP-wPreviewBox');
		if(!$previewBox.hasClass('hidden')) {
			var previewBoxCtx = $previewBox.get(0).getContext('2d');
			previewBoxCtx.fillStyle = "rgba(" + R + "," + G + "," + B + "," + A + ")";
			previewBoxCtx.clearRect(0, 0, 1, 1);
			previewBoxCtx.fillRect(0, 0, 1, 1);
		}
		
		/// SLIDERS ///
		if(!this.options.live && !force)
			return this;
		
		/// ALPHA ///
		// The top color is (R, G, B, 1)
		// The bottom color is (R, G, B, 0)
		var $alphaSlider = $widget.find('.jQWCP-wAlphaSlider');
		if(!$alphaSlider.hasClass('hidden') || force) {
			var alphaSliderCtx = $alphaSlider.get(0).getContext('2d');
			var alphaGradient = alphaSliderCtx.createLinearGradient(0, 0, 0, h);
			alphaGradient.addColorStop(0, "rgba("+R+","+G+","+B+",1)");
			alphaGradient.addColorStop(1, "rgba("+R+","+G+","+B+",0)");
			alphaSliderCtx.fillStyle = alphaGradient;
			alphaSliderCtx.clearRect(0, 0, w, h);
			alphaSliderCtx.fillRect(0, 0, w, h);
		}
		
		/// RED ///
		// The top color is (255, G, B)
		// The bottom color is (0, G, B)
		var $redSlider = $widget.find('.jQWCP-wRedSlider');
		if(!$redSlider.hasClass('hidden') || force) {
			var redSliderCtx = $redSlider.get(0).getContext('2d');
			var redGradient = redSliderCtx.createLinearGradient(0, 0, 0, h);
			redGradient.addColorStop(0, "rgb(255,"+G+","+B+")");
			redGradient.addColorStop(1, "rgb(0,"+G+","+B+")");
			redSliderCtx.fillStyle = redGradient;
			redSliderCtx.fillRect(0, 0, w, h);
		}
		
		/// GREEN ///
		// The top color is (R, 255, B)
		// The bottom color is (R, 0, B)
		var $greenSlider = $widget.find('.jQWCP-wGreenSlider');
		if(!$greenSlider.hasClass('hidden') || force) {
			var greenSliderCtx = $greenSlider.get(0).getContext('2d');
			var greenGradient = greenSliderCtx.createLinearGradient(0, 0, 0, h);
			greenGradient.addColorStop(0, "rgb("+R+",255,"+B+")");
			greenGradient.addColorStop(1, "rgb("+R+",0,"+B+")");
			greenSliderCtx.fillStyle = greenGradient;
			greenSliderCtx.fillRect(0, 0, w, h);
		}
		
		/// BLUE ///
		// The top color is (R, G, 255)
		// The bottom color is (R, G, 0)
		var $blueSlider = $widget.find('.jQWCP-wBlueSlider');
		if(!$blueSlider.hasClass('hidden') || force) {
			var blueSliderCtx = $blueSlider.get(0).getContext('2d');
			var blueGradient = blueSliderCtx.createLinearGradient(0, 0, 0, h);
			blueGradient.addColorStop(0, "rgb("+R+","+G+",255)");
			blueGradient.addColorStop(1, "rgb("+R+","+G+",0)");
			blueSliderCtx.fillStyle = blueGradient;
			blueSliderCtx.fillRect(0, 0, w, h);
		}
		
		/// HUE ///
		// The hue slider is static.
		var $hueSlider = $widget.find('.jQWCP-wHueSlider');
		if(!$hueSlider.hasClass('hidden') || force) {
			var hueSliderCtx = $hueSlider.get(0).getContext('2d');
			var hueGradient = hueSliderCtx.createLinearGradient(0, 0, 0, h);
			hueGradient.addColorStop(0, "#f00");
			hueGradient.addColorStop(0.166666667, "#ff0");
			hueGradient.addColorStop(0.333333333, "#0f0");
			hueGradient.addColorStop(0.5, "#0ff");
			hueGradient.addColorStop(0.666666667, "#00f");
			hueGradient.addColorStop(0.833333333, "#f0f");
			hueGradient.addColorStop(1, "#f00");
			hueSliderCtx.fillStyle = hueGradient;
			hueSliderCtx.fillRect(0, 0, w, h);
		}
		
		/// SAT ///
		// The top color is hsv(h, 1, v)
		// The bottom color is hsv(0, 0, v)
		var $satSlider = $widget.find('.jQWCP-wSatSlider');
		if(!$satSlider.hasClass('hidden') || force) {
			var satTopRgb = $.fn.wheelColorPicker.hsvToRgb(H, 1, V);
			satTopRgb.r = Math.round(satTopRgb.r * 255);
			satTopRgb.g = Math.round(satTopRgb.g * 255);
			satTopRgb.b = Math.round(satTopRgb.b * 255);
			var satSliderCtx = $satSlider.get(0).getContext('2d');
			var satGradient = satSliderCtx.createLinearGradient(0, 0, 0, h);
			satGradient.addColorStop(0, "rgb("+satTopRgb.r+","+satTopRgb.g+","+satTopRgb.b+")");
			satGradient.addColorStop(1, "rgb("+Math.round(V*255)+","+Math.round(V*255)+","+Math.round(V*255)+")");
			satSliderCtx.fillStyle = satGradient;
			satSliderCtx.fillRect(0, 0, w, h);
		}
		
		/// VAL ///
		// The top color is hsv(h, s, 1)
		// The bottom color is always black.
		var $valSlider = $widget.find('.jQWCP-wValSlider');
		if(!$valSlider.hasClass('hidden') || force) {
			var valTopRgb = $.fn.wheelColorPicker.hsvToRgb(H, S, 1);
			valTopRgb.r = Math.round(valTopRgb.r * 255);
			valTopRgb.g = Math.round(valTopRgb.g * 255);
			valTopRgb.b = Math.round(valTopRgb.b * 255);
			var valSliderCtx = $valSlider.get(0).getContext('2d');
			var valGradient = valSliderCtx.createLinearGradient(0, 0, 0, h);
			valGradient.addColorStop(0, "rgb("+valTopRgb.r+","+valTopRgb.g+","+valTopRgb.b+")");
			valGradient.addColorStop(1, "#000");
			valSliderCtx.fillStyle = valGradient;
			valSliderCtx.fillRect(0, 0, w, h);
		}

		/// TEMPERATURE ///
		// The temperature slider is static.
		var $temperatureSlider = $widget.find('.jQWCP-wTemperatureSlider');
		if(!$temperatureSlider.hasClass('hidden') || force) {
			var temperatureSliderCtx = $temperatureSlider.get(0).getContext('2d');
			var temperatureGradient = temperatureSliderCtx.createLinearGradient(0, 0, 0, h);
			temperatureGradient.addColorStop(0.00, "#FF8500");
			temperatureGradient.addColorStop(0.02, "#FF8F24");
			temperatureGradient.addColorStop(0.06, "#FF9F43");
			temperatureGradient.addColorStop(0.11, "#FFB062");
			temperatureGradient.addColorStop(0.16, "#FFC081");
			temperatureGradient.addColorStop(0.23, "#FFD0A0");
			temperatureGradient.addColorStop(0.30, "#FFDFC0");
			temperatureGradient.addColorStop(0.40, "#FFEDE1");
			temperatureGradient.addColorStop(0.51, "#FCF7FF");
			temperatureGradient.addColorStop(0.67, "#DEE6FF");
			temperatureGradient.addColorStop(0.99, "#C0D3FF");
			temperatureGradient.addColorStop(1.00, "#BFD3FF");
			temperatureSliderCtx.fillStyle = temperatureGradient;
			temperatureSliderCtx.fillRect(0, 0, w, h);
		}

		/// WHITE ///
		// The white slider is static.
		var $whiteSlider = $widget.find('.jQWCP-wWhiteSlider');
		if(!$whiteSlider.hasClass('hidden') || force) {
			var whiteSliderCtx = $whiteSlider.get(0).getContext('2d');
			var whiteGradient = whiteSliderCtx.createLinearGradient(0, 0, 0, h);
			whiteGradient.addColorStop(0.00, "#FFFFFF");
			whiteGradient.addColorStop(1.00, "#000000");
			whiteSliderCtx.fillStyle = whiteGradient;
			whiteSliderCtx.fillRect(0, 0, w, h);
		}

		/// MASTER ///
		// The master slider is static.
		var $masterSlider = $widget.find('.jQWCP-wMasterSlider');
		if(!$masterSlider.hasClass('hidden') || force) {
			var masterSliderCtx = $masterSlider.get(0).getContext('2d');
			var masterGradient = masterSliderCtx.createLinearGradient(0, 0, 0, h);
			masterGradient.addColorStop(0.00, "#FFFFFF");
			masterGradient.addColorStop(1.00, "#000000");
			masterSliderCtx.fillStyle = masterGradient;
			masterSliderCtx.fillRect(0, 0, w, h);
		}

		return this; // Allows method chaining
	};


	/**
	 * Function: updateSliders
	 * 
	 * Introduced in 2.0
	 * 
	 * Update slider cursor positions based on this.color value. 
	 * Only displayed sliders are updated.
	 */
	WCP.ColorPicker.prototype.updateSliders = function() {
		
		// Skip if not yet initialized
		if(this.widget == null)
			return this;
			
		var $widget = $(this.widget);
		var color = this.color;
		
		// No need to redraw sliders on global popup widget if not 
		// attached to the input elm in current iteration
		if(this != $widget.data('jQWCP.instance'))
			return this;
			
		// Wheel
		var $wheel = $widget.find('.jQWCP-wHueWheel');
		if(!$wheel.hasClass('hidden')) {
			var $wheelCursor = $widget.find('.jQWCP-wHueWheelCursor');
			var $wheelOverlay = $widget.find('.jQWCP-wWheelOverlay');
			var wheelX = Math.cos(2 * Math.PI * color.h) * color.s;
			var wheelY = Math.sin(2 * Math.PI * color.h) * color.s;
			var wheelOffsetX = $wheel.width() / 2;
			var wheelOffsetY = $wheel.height() / 2;
			$wheelCursor.css('left', (wheelOffsetX + (wheelX * $wheel.width() / 2)) + 'px');
			$wheelCursor.css('top', (wheelOffsetY - (wheelY * $wheel.height() / 2)) + 'px');
			// Keep shading to 1 if preserveWheel is true (DEPRECATED) or live is true
			if(this.options.preserveWheel == true || (this.options.preserveWheel == null && this.options.live == false)) {
				$wheelOverlay.css('opacity', 0);
			}
			else {
				$wheelOverlay.css('opacity', 1 - (color.v < 0.2 ? 0.2 : color.v));
			}
		}
		
		// Temparature Wheel
		var $wheel = $widget.find('.jQWCP-wTemperatureWheel');
		if(!$wheel.hasClass('hidden')) {
			var $wheelCursor = $widget.find('.jQWCP-wTemperatureWheelCursor');
			//var $wheelOverlay = $widget.find('.jQWCP-wWheelOverlay');
			//var wheelX = Math.cos(2 * Math.PI * color.h) * color.s;
			//var wheelY = Math.sin(2 * Math.PI * color.h) * color.s;
			wheelY = 2 * color.t;
			var wheelOffsetX = $wheel.width() / 2;
			var wheelOffsetY = $wheel.height();
			$wheelCursor.css('left', (wheelOffsetX + (wheelX * $wheel.width() / 2)) + 'px');
			$wheelCursor.css('top', (wheelOffsetY - (wheelY * $wheel.height() / 2)) + 'px');
			// Keep shading to 1 if preserveWheel is true (DEPRECATED) or live is true
			//if(this.options.preserveWheel == true || (this.options.preserveWheel == null && this.options.live == false)) {
			//	$wheelOverlay.css('opacity', 0);
			//}
			//else {
			//	$wheelOverlay.css('opacity', 1 - (color.v < 0.2 ? 0.2 : color.v));
			//}
		}
		
		// Hue
		var $hueSlider = $widget.find('.jQWCP-wHueSlider');
		if(!$hueSlider.hasClass('hidden')) {
			var $hueCursor = $widget.find('.jQWCP-wHueCursor');
			$hueCursor.css('top', (color.h * $hueSlider.height()) + 'px');
		}
		
		// Saturation
		var $satSlider = $widget.find('.jQWCP-wSatSlider');
		if(!$satSlider.hasClass('hidden')) {
			var $satCursor = $widget.find('.jQWCP-wSatCursor');
			$satCursor.css('top', ((1 - color.s) * $satSlider.height()) + 'px');
		}
		
		// Value
		var $valSlider = $widget.find('.jQWCP-wValSlider');
		if(!$valSlider.hasClass('hidden')) {
			var $valCursor = $widget.find('.jQWCP-wValCursor');
			$valCursor.css('top', ((1 - color.v) * $valSlider.height()) + 'px');
		}
		
		// Red
		var $redSlider = $widget.find('.jQWCP-wRedSlider');
		if(!$redSlider.hasClass('hidden')) {
			var $redCursor = $widget.find('.jQWCP-wRedCursor');
			$redCursor.css('top', ((1 - color.r) * $redSlider.height()) + 'px');
		}
		
		// Green
		var $greenSlider = $widget.find('.jQWCP-wGreenSlider');
		if(!$greenSlider.hasClass('hidden')) {
			var $greenCursor = $widget.find('.jQWCP-wGreenCursor');
			$greenCursor.css('top', ((1 - color.g) * $greenSlider.height()) + 'px');
		}
		
		// Blue
		var $blueSlider = $widget.find('.jQWCP-wBlueSlider');
		if(!$blueSlider.hasClass('hidden')) {
			var $blueCursor = $widget.find('.jQWCP-wBlueCursor');
			$blueCursor.css('top', ((1 - color.b) * $blueSlider.height()) + 'px');
		}
		
		// Alpha
		var $alphaSlider = $widget.find('.jQWCP-wAlphaSlider');
		if(!$alphaSlider.hasClass('hidden')) {
			var $alphaCursor = $widget.find('.jQWCP-wAlphaCursor');
			$alphaCursor.css('top', ((1 - color.a) * $alphaSlider.height()) + 'px');
		}
		
		// Temperature
		var $temperatureSlider = $widget.find('.jQWCP-wTemperatureSlider');
		if(!$temperatureSlider.hasClass('hidden')) {
			var $temperatureCursor = $widget.find('.jQWCP-wTemperatureCursor');
			$temperatureCursor.css('top', ((1 - color.t) * $temperatureSlider.height()) + 'px');
		}
		
		// White
		var $whiteSlider = $widget.find('.jQWCP-wWhiteSlider');
		if(!$whiteSlider.hasClass('hidden')) {
			var $whiteCursor = $widget.find('.jQWCP-wWhiteCursor');
			$whiteCursor.css('top', ((1 - color.w) * $whiteSlider.height()) + 'px');
		}
		
		// Master
		var $masterSlider = $widget.find('.jQWCP-wMasterSlider');
		if(!$masterSlider.hasClass('hidden')) {
			var $masterCursor = $widget.find('.jQWCP-wMasterCursor');
			$masterCursor.css('top', ((1 - color.m) * $masterSlider.height()) + 'px');
		}
		
		return this; // Allows method chaining
	};


	/**
	 * Function: updateSelection
	 * 
	 * DEPRECATED by 2.0
	 * 
	 * Update color dialog selection to match current selectedColor value.
	 */
	WCP.ColorPicker.prototype.updateSelection = function() {
		this.redrawSliders();
		this.updateSliders();
		return this;
	};


	/**
	 * Function: updateInput
	 * 
	 * Since 3.0
	 * 
	 * Update input value and background color (if preview is on)
	 */
	WCP.ColorPicker.prototype.updateInput = function() {
		// Skip if not yet initialized
		if(this.widget == null)
			return this;
			
		var $input = $(this.input);
		
		// #13 only update if value is different to avoid cursor repositioned to the end of text on some browsers
		if(this.input.value != this.getValue()) {
			this.input.value = this.getValue();
		}
		
		if( this.options.preview ) {
			$input.css('background', WCP.colorToStr( this.color, 'rgba' ));
			if( this.color.v > .5 ) {
				$input.css('color', 'black');
			}
			else {
				$input.css('color', 'white');
			}
		}
	};


	/**
	 * Function: updateActiveControl
	 * 
	 * Move the active control.
	 */
	WCP.ColorPicker.prototype.updateActiveControl = function( e ) {
		var $control = $( $('body').data('jQWCP.activeControl') ); // Refers to slider wrapper
		
		if($control.length == 0)
			return;
		
		var $input = $(this.input);
		var options = this.options;
		var color = this.color;
		
		// pageX and pageY wrapper for touches
		if(e.pageX == undefined && e.originalEvent.touches.length > 0) {
			e.pageX = e.originalEvent.touches[0].pageX;
			e.pageY = e.originalEvent.touches[0].pageY;
		}
		//$('#log').html(e.pageX + '/' + e.pageY);
		
		/// WHEEL CONTROL ///
		if($control.hasClass('jQWCP-wWheel')) {
			var $cursor = $control.find('.jQWCP-wWheelCursor');
			var $overlay = $control.find('.jQWCP-wWheelOverlay');
			
			var relX = (e.pageX - $control.offset().left - ($control.width() / 2)) / ($control.width() / 2);
			var relY = - (e.pageY - $control.offset().top - ($control.height() / 2)) / ($control.height() / 2);
			
			// BUG_RELATIVE_PAGE_ORIGIN workaround
			if(WCP.BUG_RELATIVE_PAGE_ORIGIN) {
				var relX = (e.pageX - ($control.get(0).getBoundingClientRect().left - WCP.ORIGIN.left) - ($control.width() / 2)) / ($control.width() / 2);
				var relY = - (e.pageY - ($control.get(0).getBoundingClientRect().top - WCP.ORIGIN.top) - ($control.height() / 2)) / ($control.height() / 2);
			}
			
			//console.log(relX + ' ' + relY);
			
			if($control.hasClass('jQWCP-wHueWheel')) {
				// Sat value is calculated from the distance of the cursor from the central point
				var sat = Math.sqrt(Math.pow(relX, 2) + Math.pow(relY, 2));
				// If distance is out of bound, reset to the upper bound
				if(sat > 1) {
					sat = 1;
				}
				
				// Snap to 0,0
				if(options.snap && sat < options.snapTolerance) {
					sat = 0;
				}
				
				// Hue is calculated from the angle of the cursor. 0deg is set to the right, and increase counter-clockwise.
				var hue = (relX == 0 && relY == 0) ? 0 : Math.atan( relY / relX ) / ( 2 * Math.PI );
				// If hue is negative, then fix the angle value (meaning angle is in either Q2 or Q4)
				if( hue < 0 ) {
					hue += 0.5;
				}
				// If y is negative, then fix the angle value (meaning angle is in either Q3 or Q4)
				if( relY < 0 ) {
					hue += 0.5;
				}
				
				this.setHsv(hue, sat, color.v);
			}
			
			if($control.hasClass('jQWCP-wTemperatureWheel')) {
				var temperature = (relY+1)/2;
				temperature = Math.max(0, temperature);
				temperature = Math.min(1, temperature);
				
				this.setTemperature(temperature);
			}
		}
		
		/// SLIDER CONTROL ///
		else if($control.hasClass('jQWCP-slider-wrapper')) {
			var $cursor = $control.find('.jQWCP-scursor');
			
			var relY = (e.pageY - $control.offset().top) / $control.height();
			
			// BUG_RELATIVE_PAGE_ORIGIN workaround
			if(WCP.BUG_RELATIVE_PAGE_ORIGIN) {
				var relY = (e.pageY - ($control.get(0).getBoundingClientRect().top - WCP.ORIGIN.top)) / $control.height();
			}
			
			var value = relY < 0 ? 0 : relY > 1 ? 1 : relY;
			
			// Snap to 0.0, 0.5, and 1.0
			//console.log(value);
			if(options.snap && value < options.snapTolerance) {
				value = 0;
			}
			else if(options.snap && value > 1-options.snapTolerance) {
				value = 1;
			}
			if(options.snap && value > 0.5-options.snapTolerance && value < 0.5+options.snapTolerance) {
				value = 0.5;
			}
			
			$cursor.css('top', (value * $control.height()) + 'px');
			
			/// Update color value ///
			// Red
			if($control.hasClass('jQWCP-wRed')) {
				this.setRgb(1-value, color.g, color.b);
			}
			// Green
			if($control.hasClass('jQWCP-wGreen')) {
				this.setRgb(color.r, 1-value, color.b);
			}
			// Blue
			if($control.hasClass('jQWCP-wBlue')) {
				this.setRgb(color.r, color.g, 1-value);
			}
			// Hue
			if($control.hasClass('jQWCP-wHue')) {
				this.setHsv(value, color.s, color.v);
			}
			// Saturation
			if($control.hasClass('jQWCP-wSat')) {
				this.setHsv(color.h, 1-value, color.v);
			}
			// Value
			if($control.hasClass('jQWCP-wVal')) {
				this.setHsv(color.h, color.s, 1-value);
			}
			// Alpha
			if($control.hasClass('jQWCP-wAlpha')) {
				this.setAlpha(1-value);
			}
			// Temperature
			if($control.hasClass('jQWCP-wTemperature')) {
				this.setTemperature(1-value);
			}
			// White
			if($control.hasClass('jQWCP-wWhite')) {
				this.setWhite(1-value);
			}
			// Master
			if($control.hasClass('jQWCP-wMaster')) {
				this.setMaster(1-value);
			}
		}
	};


	/**
	 * Function: getColor
	 * 
	 * Since 2.0
	 * 
	 * Return color components as an object. The object consists of:
	 * { 
	 *   r: red
	 *   g: green
	 *   b: blue
	 *   h: hue
	 *   s: saturation
	 *   v: value
	 *   a: alpha
	 * }
	 */
	WCP.ColorPicker.prototype.getColor = function() {
		return this.color;
	};


	/**
	 * Function: getValue
	 * 
	 * Get the color value as string.
	 */
	WCP.ColorPicker.prototype.getValue = function( format ) {
		var options = this.options;
		
		if( format == null ) {
			format = options.format;
		}
			
		// If settings.rounding is TRUE, round alpha value to N decimal digits
		if(options.rounding >= 0) {
			this.color.a = Math.round(this.color.a * Math.pow(10, options.rounding)) / Math.pow(10, options.rounding);
		}
		return WCP.colorToStr( this.color, format );
	};


	/**
	 * Function: setValue
	 * 
	 * Set the color value as string.
	 * 
	 * Parameters:
	 *   value       - <string> String representation of color.
	 *   updateInput - <boolean> Whether to update input text. Default is true.
	 */
	WCP.ColorPicker.prototype.setValue = function( value, updateInput ) {
		var color = WCP.strToColor(value);
		if(!color)
			return this;
			
		return this.setColor(color, updateInput);
	}


	/**
	 * Function: setColor
	 * 
	 * Introduced in 2.0
	 * 
	 * Set color by passing an object consisting of:
	 * { r, g, b, a } or
	 * { h, s, v, a }
	 * 
	 * For consistency with options.color value, this function also 
	 * accepts string value.
	 * 
	 * Parameters:
	 *   color       - <object> Color object or string representation of color.
	 *   updateInput - <boolean> Whether to update input text. Default is true.
	 */
	WCP.ColorPicker.prototype.setColor = function( color, updateInput ) {
		if(typeof color === "string") {
			return this.setValue(color, updateInput);
		}
		else if(color.r != null) {
			return this.setRgba(color.r, color.g, color.b, color.a, updateInput);
		}
		else if(color.h != null) {
			return this.setHsva(color.h, color.s, color.v, color.a, updateInput);
		}
		else if(color.a != null) {
			return this.setAlpha(color.a, updateInput);
		}
		return this;
	};


	/**
	 * Function: setRgba
	 * 
	 * Introduced in 2.0
	 * 
	 * Set color using RGBA combination.
	 * 
	 * Parameters:
	 *   r           - <number> Red component [0..1]
	 *   g           - <number> Green component [0..1]
	 *   b           - <number> Blue component [0..1]
	 *   a           - <number> Alpha value [0..1]
	 *   updateInput - <boolean> Whether to update input text. Default is true.
	 */
	WCP.ColorPicker.prototype.setRgba = function( r, g, b, a, updateInput ) {
		// Default value
		if(updateInput === undefined) updateInput = true;
		
		var color = this.color;
		color.r = r;
		color.g = g;
		color.b = b;
		
		if(a != null) {
			color.a = a;
		}
		
		var hsv = WCP.rgbToHsv(r, g, b);
		color.h = hsv.h;
		color.s = hsv.s;
		color.v = hsv.v;

		this.updateSliders();
		this.redrawSliders();
		if(updateInput) {
			this.updateInput();
		}
		return this;
	};


	/**
	 * Function: setRgb
	 * 
	 * Introduced in 2.0
	 * 
	 * Set color using RGB combination.
	 * 
	 * Parameters:
	 *   r           - <number> Red component [0..1]
	 *   g           - <number> Green component [0..1]
	 *   b           - <number> Blue component [0..1]
	 *   updateInput - <boolean> Whether to update input text.
	 */
	WCP.ColorPicker.prototype.setRgb = function( r, g, b, updateInput ) {
		return this.setRgba(r, g, b, null, updateInput);
	};


	/**
	 * Function: setHsva
	 * 
	 * Introduced in 2.0
	 * 
	 * Set color using HSVA combination.
	 * 
	 * Parameters:
	 *   h           - <number> Hue component [0..1]
	 *   s           - <number> Saturation component [0..1]
	 *   v           - <number> Value component [0..1]
	 *   a           - <number> Alpha value [0..1]
	 *   updateInput - <boolean> Whether to update input text. Default is true.
	 */
	WCP.ColorPicker.prototype.setHsva = function( h, s, v, a, updateInput ) {
		// Default value
		if(updateInput === undefined) updateInput = true;
		
		var color = this.color;
		color.h = h;
		color.s = s;
		color.v = v;
		
		if(a != null) {
			color.a = a;
		}
		
		var rgb = WCP.hsvToRgb(h, s, v);
		color.r = rgb.r;
		color.g = rgb.g;
		color.b = rgb.b;
		
		this.updateSliders();
		this.redrawSliders();
		if(updateInput) {
			this.updateInput();
		}
		return this;
	};


	/**
	 * Function: setHsv
	 * 
	 * Introduced in 2.0
	 * 
	 * Set color using HSV combination.
	 * 
	 * Parameters:
	 *   h           - <number> Hue component [0..1]
	 *   s           - <number> Saturation component [0..1]
	 *   v           - <number> Value component [0..1]
	 *   updateInput - <boolean> Whether to update input text.
	 */
	WCP.ColorPicker.prototype.setHsv = function( h, s, v, updateInput ) {
		return this.setHsva(h, s, v, null, updateInput);
	};


	/**
	 * Function: setAlpha
	 * 
	 * Introduced in 2.0
	 * 
	 * Set alpha value.
	 * 
	 * Parameters:
	 *   value       - <number> Alpha value [0..1]
	 *   updateInput - <boolean> Whether to update input text. Default is true.
	 */
	WCP.ColorPicker.prototype.setAlpha = function( value, updateInput ) {
		// Default value
		if(updateInput === undefined) updateInput = true;
		
		this.color.a = value;
		
		this.updateSliders();
		this.redrawSliders();
		if(updateInput) {
			this.updateInput();
		}
		return this;
	};


	/**
	 * Function: setTemperature
	 * 
	 * Introduced in 2.0
	 * 
	 * Set color temperature.
	 * 
	 * Parameters:
	 *   value       - <number> Color temperature [0..1]
	 *   updateInput - <boolean> Whether to update input text. Default is true.
	 */
	WCP.ColorPicker.prototype.setTemperature = function( value, updateInput ) {
		// Default value
		if(updateInput === undefined) updateInput = true;
		
		this.color.t = value;
		
		this.updateSliders();
		this.redrawSliders();
		if(updateInput) {
			this.updateInput();
		}
		return this;
	};


	/**
	 * Function: setWhite
	 * 
	 * Introduced in 2.0
	 * 
	 * Set white level.
	 * 
	 * Parameters:
	 *   value       - <number> White level [0..1]
	 *   updateInput - <boolean> Whether to update input text. Default is true.
	 */
	WCP.ColorPicker.prototype.setWhite = function( value, updateInput ) {
		// Default value
		if(updateInput === undefined) updateInput = true;
		
		this.color.w = value;
		
		this.updateSliders();
		this.redrawSliders();
		if(updateInput) {
			this.updateInput();
		}
		return this;
	};


	/**
	 * Function: setMaster
	 * 
	 * Introduced in 2.0
	 * 
	 * Set master level.
	 * 
	 * Parameters:
	 *   value       - <number> Master level [0..1]
	 *   updateInput - <boolean> Whether to update input text. Default is true.
	 */
	WCP.ColorPicker.prototype.setMaster = function( value, updateInput ) {
		// Default value
		if(updateInput === undefined) updateInput = true;
		
		this.color.m = value;
		
		this.updateSliders();
		this.redrawSliders();
		if(updateInput) {
			this.updateInput();
		}
		return this;
	};


	/**
	 * Function: show
	 * 
	 * Show the color picker dialog. This function is only applicable to 
	 * popup mode color picker layout.
	 */
	WCP.ColorPicker.prototype.show = function() {
		var input = this.input;
		var $input = $(input); // Refers to input elm
		var $widget = $(this.widget);
		var options = this.options;
		
		// Don't do anything if not using popup layout
		if( options.layout != "popup" )
			return;
			
		// Don't do anything if the popup is already shown and attached 
		// to the correct input elm
		//if( this == $widget.data('jQWCP.instance') )
			//return;
			
		// Attach instance to widget (because popup widget is global)
		$widget.data('jQWCP.instance', this);
		
		// Terminate ongoing transitions
		$widget.stop( true, true );
		
		// Reposition the popup window
		$widget.css({
			top: (input.getBoundingClientRect().top - WCP.ORIGIN.top + $input.outerHeight()) + 'px',
			left: (input.getBoundingClientRect().left - WCP.ORIGIN.left) + 'px'
		});
		
		// Refresh widget with this instance's options
		this.refreshWidget();
		
		// Redraw sliders
		this.redrawSliders();
		this.updateSliders();
		
		// Store last textfield value (to determine whether to trigger onchange event later)
		this.lastValue = input.value;
		
		$widget.fadeIn( options.animDuration );
		
		// If hideKeyboard is true, force to hide soft keyboard
		if(options.hideKeyboard) {
			$input.blur();
			$(WCP.ColorPicker.overlay).show();
		}
        
        // On mobile layout, autoscroll page to keep input visible
        if($widget.hasClass('jQWCP-mobile')) {
            var scrollTop = $('html').scrollTop();
            var inputTop = input.getBoundingClientRect().top - WCP.ORIGIN.top;
            
            // If input is way too top
            if(inputTop < scrollTop) {
                $('html').animate({ scrollTop: inputTop});
            }
            
            // If input is way too bottom
            else if(inputTop + $input.outerHeight() > scrollTop + window.innerHeight - $widget.outerHeight()) {
                $('html').animate({ scrollTop: inputTop + $input.outerHeight() - window.innerHeight + $widget.outerHeight()});
            }
        }
	};



	/**
	 * Function: hide
	 * 
	 * Hide the color picker dialog. This function is only applicable to 
	 * popup mode color picker layout.
	 */
	WCP.ColorPicker.prototype.hide = function() {
		var $widget = $(this.widget);
		
		// Only hide if popup belongs to this instance
		if(this != $widget.data('jQWCP.instance'))
			return;
		
		$widget.fadeOut( this.options.animDuration );
		$(WCP.ColorPicker.overlay).hide();
	};



	////////////////////
	// Event Handlers //
	////////////////////

	WCP.Handler = {};

	/**
	 * input.onFocus.popup
	 */
	WCP.Handler.input_focus_popup = function( e ) {
		var instance = $(this).data('jQWCP.instance');
		instance.show();
		
		// Workaround to prevent on screen keyboard from appearing
		if($(this).attr('readonly') == null) {
			$(this).attr('readonly', true);
			setTimeout(function() {
				$(instance.input).removeAttr('readonly');
			});
			
			// Firefox on Android
			if(navigator.userAgent.match(/Android .* Firefox/) != null) {
				setTimeout(function() {
					$(instance.input).attr('readonly', true);
					$(instance.input).one('blur', function() {
						$(instance.input).removeAttr('readonly');
					});
				});
			}
		}
	};


	/**
	 * input.onBlur.popup
	 * 
	 * onBlur event handler for popup layout.
	 */
	WCP.Handler.input_blur_popup = function( e ) {
		var instance = $(this).data('jQWCP.instance');
		
		// If keyboard is hidden, input is always blurred so 
		// no point in hiding
		if(instance.options.hideKeyboard)
			return;
			
		instance.hide();
		
		// Trigger 'change' event only when it was modified by widget
		// because user typing on the textfield will automatically
		// trigger 'change' event on blur.
		if(instance.lastValue != this.value) {
			$(this).trigger('change');
		}
	};
	
	
	/**
	 * input.onKeyUp
	 * 
	 * Update the color picker while user type in color value.
	 */
	WCP.Handler.input_keyup = function( e ) {
		var instance = $(this).data('jQWCP.instance');
		var color = WCP.strToColor(this.value);
		if(color) {
			instance.setColor(color, false);
		}
	};


	/**
	 * input.onChange
	 * 
	 * Reformat the input value based on selected color and configurations.
	 */
	WCP.Handler.input_change = function( e ) {
		var instance = $(this).data('jQWCP.instance');
		var color = WCP.strToColor(this.value);
		
		// If autoFormat option is enabled, try to reformat the value if it is a valid color
		if(instance.options.autoFormat && color) {
			instance.setColor(color, true);
		}
		
		// If validate option is enabled, revert the value if it is not a valid color
		else if(instance.options.validate && !color && this.value != '') {
			this.value = instance.getValue();
		}
	};


	/**
	 * widget.onFocus.block
	 * 
	 * Prepare runtime widget data
	 */
	WCP.Handler.widget_focus_block = function( e ) {
		var instance = $(this).data('jQWCP.instance');
		var $input = $(instance.input);
		
		// Store last textfield value
		instance.lastValue = instance.input.value;
		
		// Trigger focus event
		$input.triggerHandler('focus');
	};


	/**
	 * widget.onMouseDown.popup
	 * 
	 * Prevent loss focus of the input causing the dialog to be hidden
	 * because of input blur event.
	 */
	WCP.Handler.widget_mousedown_popup = function( e ) {
		var instance = $(this).data('jQWCP.instance');
		var $input = $(instance.input);
		
		// Temporarily unbind blur and focus event until mouse is released
		$input.off('focus.wheelColorPicker');
		$input.off('blur.wheelColorPicker');
		
		// Temporarily unbind all blur events until mouse is released
		// data('events') is deprecated since jquery 1.8
		if($input.data('events') != undefined) {
			var blurEvents = $input.data('events').blur;
		}
		else {
			var blurEvents = undefined;
		}
		var suspendedEvents = { blur: [] };
		//suspendedEvents.blur = blurEvents;
		//$input.off('blur');
		if(blurEvents != undefined) {
			for(var i = 0; i < blurEvents.length; i++) {
				suspendedEvents.blur.push(blurEvents[i]);
				//suspendedEvents.blur['blur' + (blurEvents[i].namespace != '' ? blurEvents[i].namespace : '')] = blurEvents[i].handler;
			}
		}
		$input.data('jQWCP.suspendedEvents', suspendedEvents);
		//console.log(blurEvents);
		//console.log($input.data('jQWCP.suspendedEvents'));
	};

	/**
	 * widget.onMouseUp
	 * 
	 * Re-bind events that was unbound by widget_mousedown_popup.
	 */
	/*WCP.Handler.widget_mouseup_popup = function( e ) {
		var instance = $(this).data('jQWCP.instance');
		var $input = $(instance.input);
		
		 //Input elm must always be focused, unless hideKeyboard is set to true
		if(!instance.options.hideKeyboard) {
			$input.trigger('focus.jQWCP_DONT_TRIGGER_EVENTS'); // This allow input to be focused without triggering events
		}
		
		 //Rebind blur & focusevent
		$input.on('focus.wheelColorPicker', WCP.Handler.input_focus_popup);
		$input.on('blur.wheelColorPicker', WCP.Handler.input_blur_popup);
		
	};*/



	/**
	 * widget.onBlur
	 * 
	 * Try to trigger onChange event if value has been changed.
	 */
	WCP.Handler.widget_blur_block = function( e ) {
		var instance = $(this).data('jQWCP.instance');
		var $input = $(instance.input);
		
		// Trigger 'change' event only when it was modified by widget
		// because user typing on the textfield will automatically
		// trigger 'change' event on blur.
		if(instance.lastValue != instance.input.value) {
			$input.trigger('change');
		}
		
		// Trigger blur event
		$input.triggerHandler('blur');
	};


	/**
	 * wheelCursor.onMouseDown
	 * 
	 * Begin clicking the wheel down. This will allow user to move 
	 * the crosshair although the mouse is outside the wheel.
	 */
	WCP.Handler.wheelCursor_mousedown = function( e ) {
		var $this = $(this); // Refers to cursor
		var $widget = $this.closest('.jQWCP-wWidget');
		var instance = $widget.data('jQWCP.instance');
		var $input = $(instance.input);
		
		$('body').data('jQWCP.activeControl', $this.parent().get(0));
		
		// Trigger sliderdown event
		$input.trigger('sliderdown');
	};


	/**
	 * wheel.onMouseDown
	 * 
	 * Begin clicking the wheel down. This will allow user to move 
	 * the crosshair although the mouse is outside the wheel.
	 * 
	 * Basically this is the same as wheelCursor_mousedown handler
	 */
	WCP.Handler.wheel_mousedown = function( e ) {
		var $this = $(this); // Refers to wheel
		var $widget = $this.closest('.jQWCP-wWidget');
		var instance = $widget.data('jQWCP.instance');
		var $input = $(instance.input);
		
		$('body').data('jQWCP.activeControl', $this.get(0));
		
		// Trigger sliderdown event
		$input.trigger('sliderdown');
	};


	/**
	 * slider.onMouseDown
	 * 
	 * Begin clicking the slider down. This will allow user to move 
	 * the slider although the mouse is outside the slider.
	 */
	WCP.Handler.slider_mousedown = function( e ) {
		var $this = $(this); // Refers to slider
		var $widget = $this.closest('.jQWCP-wWidget');
		var instance = $widget.data('jQWCP.instance');
		var $input = $(instance.input);
		
		$('body').data('jQWCP.activeControl', $this.parent().get(0));
		
		// Trigger sliderdown event
		$input.trigger('sliderdown');
	};

	/**
	 * sliderCursor.onMouseDown
	 * 
	 * Begin clicking the slider down. This will allow user to move 
	 * the slider although the mouse is outside the slider.
	 */
	WCP.Handler.sliderCursor_mousedown = function( e ) {
		var $this = $(this); // Refers to slider cursor
		var $widget = $this.closest('.jQWCP-wWidget');
		var instance = $widget.data('jQWCP.instance');
		var $input = $(instance.input);
		
		$('body').data('jQWCP.activeControl', $this.parent().get(0));
		
		// Trigger sliderdown event
		$input.trigger('sliderdown');
	};



	/**
	 * html.onMouseUp
	 * 
	 * Clear active control reference.
	 * Also do cleanups after widget.onMouseDown.popup
	 * 
	 * Note: This event handler is also applied to touchend
	 */
	WCP.Handler.html_mouseup = function( e ) {
		var $control = $( $('body').data('jQWCP.activeControl') ); // Refers to slider wrapper or wheel
		
		// Do stuffs when there's active control
		if($control.length == 0)
			return;
			
		var $widget = $control.closest('.jQWCP-wWidget');
		var instance = $widget.data('jQWCP.instance');
		var $input = $(instance.input);
		
		
		// Rebind blur and focus event to input elm which was 
		// temporarily released when popup dialog is shown
		if(instance.options.layout == 'popup') {
			// Focus first before binding event so it wont get fired
			// Input elm must always be focused, unless hideKeyboard is set to true
			if(!instance.options.hideKeyboard) {
				$input.trigger('focus.jQWCP_DONT_TRIGGER_EVENTS'); // This allow input to be focused without triggering events
			}
			
			// Rebind blur & focusevent
			$input.on('focus.wheelColorPicker', WCP.Handler.input_focus_popup);
			$input.on('blur.wheelColorPicker', WCP.Handler.input_blur_popup);
			
			// Rebind suspended events
			var suspendedEvents = $input.data('jQWCP.suspendedEvents');
			if(suspendedEvents != undefined) {
				var blurEvents = suspendedEvents.blur;
				for(var i = 0; i < blurEvents.length; i++) {
					$input.on('blur' + (blurEvents[i].namespace == '' ? '' : '.' + blurEvents[i].namespace), blurEvents[i].handler);
				}
			}
		}
		
		
		// Update active control
		if($control.length != 0) {
			// Last time update active control before clearing
			// Only call this function if mouse position is known
			// On touch action, touch point is not available
			if(e.pageX != undefined) {
				instance.updateActiveControl( e );
			}
			
			// Clear active control reference
			$('body').data('jQWCP.activeControl', null);
			
			// Trigger sliderup event
			$input.trigger('sliderup');
		}
	};


	/**
	 * html.onMouseMove
	 * 
	 * Move the active slider (when mouse click is down).
	 * 
	 * Note: This event handler is also applied to touchmove
	 */
	WCP.Handler.html_mousemove = function( e ) {
		var $control = $( $('body').data('jQWCP.activeControl') ); // Refers to slider wrapper or wheel
		
		// Do stuffs when there's active control
		if($control.length == 0)
			return;
			
		// If active, prevent default
		e.preventDefault();
		
		var $widget = $control.closest('.jQWCP-wWidget');
		var instance = $widget.data('jQWCP.instance');
		var $input = $(instance.input);
		
		instance.updateActiveControl( e );
		
		// Trigger slidermove event
		$input.trigger('slidermove');
		
		return false;
	};


	/**
	 * window.onResize
	 * 
	 * Adjust block widgets
	 */
	WCP.Handler.window_resize = function( e ) {
        var $widgets = $('body .jQWCP-wWidget.jQWCP-block');
        
        $widgets.each(function() {
            var instance = $(this).data('jQWCP.instance');
            instance.refreshWidget();
            instance.redrawSliders();
        });
	};


	/**
	 * overlay.onClick
	 * 
	 * Hide colorpicker popup dialog if overlay is clicked.
	 * This has the same effect as blurring input element if hideKeyboard = false.
	 */
	WCP.Handler.overlay_click = function( e ) {
		if(WCP.ColorPicker.widget == null)
			return;
		
		var $widget = $(WCP.ColorPicker.widget);
		var instance = $widget.data('jQWCP.instance');
		
		// If no instance set, do nothing
		if(instance != null) {
			var $input = $(instance.input);
			
			// Trigger 'change' event only when it was modified by widget
			// because user typing on the textfield will automatically
			// trigger 'change' event on blur.
			if(instance.lastValue != instance.input.value) {
				$input.trigger('change');
			}
			
			instance.hide();
		}
	};

	/******************************************************************/

	////////////////////////////////////////////////////////
	// Automatically initialize color picker on page load //
	// for elements with data-wheelcolorpicker attribute. //
	////////////////////////////////////////////////////////

	$(document).ready(function() {
		$('[data-wheelcolorpicker]').wheelColorPicker({ htmlOptions: true });
	});



	/******************************************************************/

	//////////////////////////////////
	// Browser specific workarounds //
	//////////////////////////////////

	(function() {
		// MOZILLA //
		
		// Force low resolution slider canvases to improve performance
		// Note: Do not rely on $.browser since it's obsolete by jQuery 2.x
		if($.browser != undefined && $.browser.mozilla) {
			$.fn.wheelColorPicker.defaults.quality = 0.2;
		}
		
		// MOBILE CHROME //
		
		// BUG_RELATIVE_PAGE_ORIGIN
		// Calibrate the coordinate of top left point of the page
		// On mobile chrome, the top left of the page is not always set at (0,0)
		// making window.scrollX/Y and $.offset() useless
		$(document).ready(function() {
			$('body').append(
				'<div id="jQWCP-PageOrigin" style="position: absolute; top: 0; left: 0; height: 0; width: 0;"></div>'
			);
			
			var origin = document.getElementById('jQWCP-PageOrigin').getBoundingClientRect();
			WCP.ORIGIN = origin;
		
			$(window).on('scroll.jQWCP_RelativePageOriginBugFix', function() {
				var origin = document.getElementById('jQWCP-PageOrigin').getBoundingClientRect();
				WCP.ORIGIN = origin;
				if(origin.left != 0 || origin.top != 0) {
					WCP.BUG_RELATIVE_PAGE_ORIGIN = true;
				}
			});
		});
		
	})();

}) (jQuery);
