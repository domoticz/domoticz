

// Extensions to Blockly's language and JavaScript generator.

Blockly.JavaScript = Blockly.Generator.get('JavaScript');

var switches = [];
var temperatures = [];
var humidity = [];
var barometer = [];
var weather = [];
var utilities = [];
var scenes = [];

$.ajax({
	url: "json.htm?type=devices&filter=light&used=true&order=Name", 
	async: false, 
	dataType: 'json',
	success: function(data) {
		if (typeof data.result != 'undefined') {
			$.each(data.result, function(i,item){
				switches.push([item.Name,item.idx])
			})
		}
	}
});

$.ajax({
	url: "json.htm?type=devices&filter=temp&used=true&order=Name", 
	async: false, 
	dataType: 'json',
	success: function(data) {
		if (typeof data.result != 'undefined') {
			$.each(data.result, function(i,item){
				if (item.Type.toLowerCase().indexOf("temp") >= 0) {
					temperatures.push([item.Name,item.idx])
				}
				if (item.Type.toLowerCase().indexOf("hum") >= 0) {
					humidity.push([item.Name,item.idx])
				}
				if (item.Type.toLowerCase().indexOf("baro") >= 0) {
					barometer.push([item.Name,item.idx])
				}
			})
		}
	}
});

$.ajax({
	url: "json.htm?type=devices&filter=weather&used=true&order=Name", 
	async: false, 
	dataType: 'json',
	success: function(data) {
		if (typeof data.result != 'undefined') {
			$.each(data.result, function(i,item){
				weather.push([item.Name,item.idx])
			})
		}
	}
});

$.ajax({
	url: "json.htm?type=devices&filter=utility&used=true&order=Name", 
	async: false, 
	dataType: 'json',
	success: function(data) {
		if (typeof data.result != 'undefined') {
			$.each(data.result, function(i,item){
				utilities.push([item.Name,item.idx])
			})
		}
	}
});

$.ajax({
	url: "json.htm?type=scenes&order=Name", 
	async: false, 
	dataType: 'json',
	success: function(data) {
		if (typeof data.result != 'undefined') {
			$.each(data.result, function(i,item){
				scenes.push([item.Name,item.idx])
			})
		}
	}
});


Blockly.Language.switchvariables = {
  // Variable getter.
  category: null,  // Variables are handled specially.
  init: function() {
    this.setColour(30);
    this.appendDummyInput()
        .appendTitle(new Blockly.FieldDropdown(switches), 'Switch');
    this.setOutput(true, null);
  }
 };


Blockly.Language.domoticzcontrols_if = {
  category: null,  // Variables are handled specially.
  init: function() {
    this.setColour(120);
    this.appendValueInput('IF0')
        .appendTitle(Blockly.LANG_CONTROLS_IF_MSG_IF);
    this.appendStatementInput('DO0')
        .appendTitle(Blockly.LANG_CONTROLS_IF_MSG_THEN);
    //this.setPreviousStatement(true);
    //this.setNextStatement(true);
  }
 };

Blockly.Language.temperaturevariables = {
  // Variable getter.
  category: null,  // Variables are handled specially.
  init: function() {
    this.setColour(330);
    this.appendDummyInput()
        .appendTitle(new Blockly.FieldDropdown(temperatures), 'Temperature');
    this.appendDummyInput()
	    .appendTitle('temp.','TemperatureLabel');
	this.setInputsInline(true);
    this.setOutput(true, null);
  }
 };

Blockly.Language.humidityvariables = {
  // Variable getter.
  category: null,  // Variables are handled specially.
  init: function() {
    this.setColour(226);
    this.appendDummyInput()
        .appendTitle(new Blockly.FieldDropdown(humidity), 'Humidity');
    this.appendDummyInput()
	    .appendTitle('hum.','TemperatureLabel');
    this.setInputsInline(true);
    this.setOutput(true, null);
  }
 };

Blockly.Language.barometervariables = {
  // Variable getter.
  category: null,  // Variables are handled specially.
  init: function() {
    this.setColour(68);
    this.appendDummyInput()
        .appendTitle(new Blockly.FieldDropdown(barometer), 'Barometer');
    this.appendDummyInput()
	    .appendTitle('baro.','TemperatureLabel');
    this.setInputsInline(true);
    this.setOutput(true, null);
  }
 };

Blockly.Language.weathervariables = {
  // Variable getter.
  category: null,  // Variables are handled specially.
  init: function() {
    this.setColour(210);
    this.appendDummyInput()
        .appendTitle(new Blockly.FieldDropdown(weather), 'Weather');
    this.setOutput(true, null);
  }
 };
 
Blockly.Language.utilityvariables = {
  // Variable getter.
  category: null,  // Variables are handled specially.
  init: function() {
    this.setColour(290);
    this.appendDummyInput()
        .appendTitle(new Blockly.FieldDropdown(utilities), 'Utility');
    this.setOutput(true, null);
  }
 };

Blockly.Language.scenevariables = {
  // Variable getter.
  category: null,  // Variables are handled specially.
  init: function() {
    this.setColour(290);
    this.appendDummyInput()
        .appendTitle(new Blockly.FieldDropdown(scenes), 'Scene');
    this.setOutput(true, null);
  }
 };


Blockly.Language.logic_states = {
  helpUrl: null,
  init: function() {
    this.setColour(120);
    this.setOutput(true, null);
    this.appendDummyInput()
        .appendTitle(new Blockly.FieldDropdown(this.STATES), 'State');
  }
};

Blockly.Language.logic_states.STATES =
    [["On", 'On'],
     ["Off", 'Off'],
     ["Group On", 'Group On'],
     ["Group Off", 'Group Off'],
     ["Open", 'Open'],
     ["Closed", 'Closed']];

Blockly.Language.logic_set = {
  // Comparison operator.
  init: function() {
    this.setColour(120);
    this.setPreviousStatement(true);
    this.setNextStatement(true);
    this.appendValueInput('A')
    	.appendTitle("Set");
    this.appendValueInput('B')
        .appendTitle("=");
    this.setInputsInline(true);

  }
};

Blockly.Language.logic_setdelayed = {
  // Comparison operator.
  init: function() {
    this.setColour(120);
    this.setPreviousStatement(true);
    this.setNextStatement(true);
    this.appendValueInput('A')
    	.appendTitle("Set");
    this.appendValueInput('B')
        .appendTitle("=");
    this.appendValueInput('C')
        .appendTitle("For");
    this.appendDummyInput()
	    .appendTitle('minutes');
    this.setInputsInline(true);
  }
};

Blockly.Language.logic_setrandom = {
  // Comparison operator.
  init: function() {
    this.setColour(120);
    this.setPreviousStatement(true);
    this.setNextStatement(true);
    this.appendValueInput('A')
    	.appendTitle("Set");
    this.appendValueInput('B')
        .appendTitle("=");
    this.appendValueInput('C')
        .appendTitle("Random within");
    this.appendDummyInput()
	    .appendTitle('minutes');
    this.setInputsInline(true);
  }
};

Blockly.Language.logic_timeofday = {
  // Comparison operator.
  init: function() {
    this.setColour(120);
    this.setOutput(true, Boolean);
    this.appendDummyInput()
	    .appendTitle("Time ")
        .appendTitle(new Blockly.FieldDropdown(this.OPERATORS), 'OP')
        .appendTitle(" ")
        .appendTitle(new Blockly.FieldTextInput('00:00'), 'Time');
    this.setInputsInline(true);
  }
}; 
 
 Blockly.Language.logic_timeofday.OPERATORS =
    [['=', 'EQ'],
     ['\u2260', 'NEQ'],
     ['<', 'LT'],
     ['\u2264', 'LTE'],
     ['>', 'GT'],
     ['\u2265', 'GTE']];


Blockly.Language.logic_weekday = {
  // Variable getter.
  init: function() {
    this.setColour(120);
    this.setOutput(true, null);
    this.appendDummyInput()
	    .appendTitle("Day ")
	    .appendTitle(new Blockly.FieldDropdown(this.OPERATORS), 'OP')
	    .appendTitle(" ")
    	.appendTitle(new Blockly.FieldDropdown(this.DAYS), 'Weekday');
    }
 };   
 
  Blockly.Language.logic_weekday.OPERATORS =
    [['=', 'EQ'],
     ['\u2260', 'NEQ'],
     ['<', 'LT'],
     ['\u2264', 'LTE'],
     ['>', 'GT'],
     ['\u2265', 'GTE']];  

Blockly.Language.logic_weekday.DAYS =
    [["Monday", '2'],
    ["Tuesday",'3'],
    ["Wednesday",'4'],
    ["Thursday",'5'],
    ["Friday",'6'],
    ["Saturday",'7'],
     ["Sunday", '1']];


Blockly.Language.send_notification = {
  // Comparison operator.
  init: function() {
    this.setColour(120);
    this.setPreviousStatement(true);
    this.setNextStatement(true);
    this.appendValueInput('notificationTextSubject')
    	.appendTitle("Send notification with subject:");
    this.appendValueInput('notificationTextBody')
    	.appendTitle("and message:");
    this.setInputsInline(true);

  }
};

