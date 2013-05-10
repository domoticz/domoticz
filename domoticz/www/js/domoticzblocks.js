

// Extensions to Blockly's language and JavaScript generator.

Blockly.JavaScript = Blockly.Generator.get('JavaScript');

var switches = [];
var temperatures = [];
var weather = [];
var utilities = [];


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
				temperatures.push([item.Name,item.idx])
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


Blockly.Language.temperaturevariables = {
  // Variable getter.
  category: null,  // Variables are handled specially.
  init: function() {
    this.setColour(330);
    this.appendDummyInput()
        .appendTitle(new Blockly.FieldDropdown(temperatures), 'Temperature');
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

Blockly.Language.logic_states = {
  helpUrl: null,
  init: function() {
    this.setColour(120);
    this.setOutput(true, null);
    this.appendDummyInput()
        .appendTitle(new Blockly.FieldDropdown(this.STATES), 'State');
    this.setTooltip("tip");
  }
};

Blockly.Language.logic_states.STATES =
    [["on", '1'],
     ["off", '0']];

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

Blockly.Language.send_notification = {
  // Comparison operator.
  init: function() {
    this.setColour(120);
    this.setPreviousStatement(true);
    this.setNextStatement(true);
    this.appendValueInput('notificationText')
    	.appendTitle("Send notification with text:");
    this.setInputsInline(true);

  }
};

Blockly.Language.send_email = {
  // Comparison operator.
  init: function() {
    this.setColour(120);
    this.setPreviousStatement(true);
    this.setNextStatement(true);
    this.appendValueInput('emailText')
    	.appendTitle("Send email with text:");
    this.setInputsInline(true);
  }
};

