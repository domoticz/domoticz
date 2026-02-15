// Bar Widget for Temperature and Utility Sensors
// Renders a horizontal bar with color-coded ranges

function renderBarWidget(value, options) {
	if (!options || !options.enabled) {
		return '';
	}
	
	var min = parseFloat(options.min) || 0;
	var max = parseFloat(options.max) || 100;
	var ranges = options.ranges || '';
	var numValue = parseFloat(value);
	
	// Validate value
	if (isNaN(numValue)) {
		return '';
	}
	
	// Clamp value to min/max
	var clampedValue = Math.max(min, Math.min(max, numValue));
	
	// Calculate percentage
	var percentage = ((clampedValue - min) / (max - min)) * 100;
	
	// Parse ranges: format is "min-max:color|min-max:color"
	var rangeArray = [];
	if (ranges && ranges.length > 0) {
		var rangeParts = ranges.split('|');
		for (var i = 0; i < rangeParts.length; i++) {
			var part = rangeParts[i].trim();
			if (part) {
				var rangeDef = part.split(':');
				if (rangeDef.length === 2) {
					var bounds = rangeDef[0].split('-');
					if (bounds.length === 2) {
						var rangeMin = parseFloat(bounds[0]);
						var rangeMax = parseFloat(bounds[1]);
						var color = rangeDef[1].trim();
						rangeArray.push({
							min: rangeMin,
							max: rangeMax,
							color: color
						});
					}
				}
			}
		}
	}
	
	// Build HTML
	var html = '<div class="bar-widget-container">';
	html += '<div class="bar-widget-track">';
	
	// Render colored range segments
	if (rangeArray.length > 0) {
		for (var i = 0; i < rangeArray.length; i++) {
			var range = rangeArray[i];
			var segmentStart = ((range.min - min) / (max - min)) * 100;
			var segmentWidth = ((range.max - range.min) / (max - min)) * 100;
			
			// Clamp segment to 0-100%
			segmentStart = Math.max(0, Math.min(100, segmentStart));
			segmentWidth = Math.max(0, Math.min(100 - segmentStart, segmentWidth));
			
			html += '<div class="bar-widget-segment" style="left: ' + segmentStart + '%; width: ' + segmentWidth + '%; background-color: ' + range.color + ';"></div>';
		}
	} else {
		// Default gray background if no ranges defined
		html += '<div class="bar-widget-segment" style="left: 0%; width: 100%; background-color: #ddd;"></div>';
	}
	
	// Render value indicator
	html += '<div class="bar-widget-indicator" style="left: ' + percentage + '%;"></div>';
	html += '</div>';
	html += '</div>';
	
	return html;
}

function parseBarWidgetOptions(device) {
	var options = {
		enabled: false,
		min: 0,
		max: 100,
		ranges: ''
	};
	
	if (!device || !device.Options) {
		return options;
	}
	
	try {
		var optPairs = device.Options.split(';');
		var optMap = {};
		for (var i = 0; i < optPairs.length; i++) {
			var pair = optPairs[i].split(':');
			if (pair.length === 2) {
				optMap[pair[0]] = pair[1] ? b64DecodeUnicode(pair[1]) : '';
			}
		}
		
		if (optMap.BarWidget === 'true') {
			options.enabled = true;
			options.min = optMap.BarMin || '0';
			options.max = optMap.BarMax || '100';
			options.ranges = optMap.BarRanges || '';
		}
	} catch (e) {
		// Ignore parsing errors
	}
	
	return options;
}
