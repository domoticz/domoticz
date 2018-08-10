// Extensions to Blockly's language and JavaScript generator.

var switchesAF = [];
var switchesGL = [];
var switchesMR = [];
var switchesSZ = [];

var utilities = [];
var utilitiesAF = [];
var utilitiesGL = [];
var utilitiesMR = [];
var utilitiesSZ = [];

var texts = [];
var temperatures = [];
var humidity = [];
var dewpoint = [];
var barometer = [];
var weather = [];
var scenes = [];
var groups = [];
var cameras = [];
var setpoints = [];

var variablesAF = [];
var variablesGL = [];
var variablesMR = [];
var variablesSZ = [];

var zwavealarms = [];

$.ajax({
	url: "json.htm?type=devices&filter=light&used=true&order=Name&displayhidden=1",
	async: false,
	dataType: 'json',
	success: function (data) {
		if (typeof data.result != 'undefined') {
			$.each(data.result, function (i, item) {
				if ("ghijkl".indexOf(item.Name.charAt(0).toLowerCase()) > -1) {
					switchesGL.push([item.Name, item.idx])
				}
				else if ("mnopqr".indexOf(item.Name.charAt(0).toLowerCase()) > -1) {
					switchesMR.push([item.Name, item.idx])
				}
				else if ("stuvwxyz".indexOf(item.Name.charAt(0).toLowerCase()) > -1) {
					switchesSZ.push([item.Name, item.idx])
				}
				// numbers etc with the a list
				else {
					switchesAF.push([item.Name, item.idx])
				}
			})
		}
	}
});

if (switchesAF.length === 0) { switchesAF.push(["No devices found", 0]); }
if (switchesGL.length === 0) { switchesGL.push(["No devices found", 0]); }
if (switchesMR.length === 0) { switchesMR.push(["No devices found", 0]); }
if (switchesSZ.length === 0) { switchesSZ.push(["No devices found", 0]); }

$.ajax({
	url: "json.htm?type=devices&filter=temp&used=true&order=Name&displayhidden=1",
	async: false,
	dataType: 'json',
	success: function (data) {
		if (typeof data.result != 'undefined') {
			$.each(data.result, function (i, item) {
				if (item.Type.toLowerCase().indexOf("temp") >= 0) {
					temperatures.push([item.Name, item.idx])
				}
				if ((item.Type == "RFXSensor") && (item.SubType == "Temperature")) {
					temperatures.push([item.Name, item.idx]);
				}
				if (item.Type.toLowerCase().indexOf("hum") >= 0) {
					humidity.push([item.Name, item.idx])
				}
				if (item.Type.toLowerCase().indexOf("baro") >= 0) {
					barometer.push([item.Name, item.idx])
				}
				if (typeof item.DewPoint != 'undefined') {
					dewpoint.push([item.Name, item.idx])
				}
			})
		}
	}
});

$.ajax({
	url: "json.htm?type=devices&filter=weather&used=true&order=Name&displayhidden=1",
	async: false,
	dataType: 'json',
	success: function (data) {
		if (typeof data.result != 'undefined') {
			$.each(data.result, function (i, item) {
				weather.push([item.Name, item.idx])
			})
		}
	}
});

$.ajax({
	url: "json.htm?type=devices&filter=utility&used=true&order=Name&displayhidden=1",
	async: false,
	dataType: 'json',
	success: function (data) {
		if (typeof data.result != 'undefined') {
			$.each(data.result, function (i, item) {
				if (item.SubType == "Text") {
					texts.push([item.Name, item.idx])
				}
				else {
					utilities.push([item.Name, item.idx])
					if ("ghijkl".indexOf(item.Name.charAt(0).toLowerCase()) > -1) {
						utilitiesGL.push([item.Name, item.idx])
					}
					else if ("mnopqr".indexOf(item.Name.charAt(0).toLowerCase()) > -1) {
						utilitiesMR.push([item.Name, item.idx])
					}
					else if ("stuvwxyz".indexOf(item.Name.charAt(0).toLowerCase()) > -1) {
						utilitiesSZ.push([item.Name, item.idx])
					}
					// numbers etc with the a list
					else {
						utilitiesAF.push([item.Name, item.idx])
					}
					if (item.SubType == 'SetPoint') {
						setpoints.push([item.Name, item.idx])
					}
				}
			})
		}
	}
});
if (utilities.length === 0) { utilities.push(["No utilities found", 0]); }
if (utilitiesAF.length === 0) { utilitiesAF.push(["No utilities found", 0]); }
if (utilitiesGL.length === 0) { utilitiesGL.push(["No utilities found", 0]); }
if (utilitiesMR.length === 0) { utilitiesMR.push(["No utilities found", 0]); }
if (utilitiesSZ.length === 0) { utilitiesSZ.push(["No utilities found", 0]); }
if (texts.length === 0) { texts.push(["No text devices found", 0]); }


$.ajax({
	url: "json.htm?type=devices&filter=zwavealarms&used=true&order=Name&displayhidden=1",
	async: false,
	dataType: 'json',
	success: function (data) {
		if (typeof data.result != 'undefined') {
			$.each(data.result, function (i, item) {
				zwavealarms.push([item.Name, item.idx])
			})
		}
	}
});
if (zwavealarms.length === 0) { zwavealarms.push(["No ZWave Alarms found", 0]); }

$.ajax({
	url: "json.htm?type=scenes&order=Name&displayhidden=1",
	async: false,
	dataType: 'json',
	success: function (data) {
		if (typeof data.result != 'undefined') {
			$.each(data.result, function (i, item) {
				if (item.Type == 'Scene') {
					scenes.push([item.Name, item.idx])
				}
				else if (item.Type == 'Group') {
					groups.push([item.Name, item.idx])
				}
			})
		}
	}
});

$.ajax({
	url: "json.htm?type=cameras&order=Name&displayhidden=1",
	async: false,
	dataType: 'json',
	success: function (data) {
		if (typeof data.result != 'undefined') {
			$.each(data.result, function (i, item) {
				cameras.push([item.Name, item.idx])
			})
		}
	}
});

if (temperatures.length === 0) { temperatures.push(["No temperatures found", 0]); }
if (humidity.length === 0) { humidity.push(["No humidity found", 0]); }
if (dewpoint.length === 0) { dewpoint.push(["No dew points found", 0]); }
if (barometer.length === 0) { barometer.push(["No barometer found", 0]); }
if (weather.length === 0) { weather.push(["No weather found", 0]); }
if (groups.length === 0) { groups.push(["No groups found", 0]); }
if (scenes.length === 0) { scenes.push(["No scenes found", 0]); }
if (cameras.length === 0) { cameras.push(["No cameras found", 0]); }
if (setpoints.length === 0) { setpoints.push(["No SetPoints found", 0]); }

$.ajax({
	url: "json.htm?type=command&param=getuservariables",
	async: false,
	dataType: 'json',
	success: function (data) {
		if (typeof data.result != 'undefined') {
			$.each(data.result, function (i, item) {
				if ("ghijkl".indexOf(item.Name.charAt(0).toLowerCase()) > -1) {
					variablesGL.push([item.Name, item.idx])
				}
				else if ("mnopqr".indexOf(item.Name.charAt(0).toLowerCase()) > -1) {
					variablesMR.push([item.Name, item.idx])
				}
				else if ("stuvwxyz".indexOf(item.Name.charAt(0).toLowerCase()) > -1) {
					variablesSZ.push([item.Name, item.idx])
				}
				// numbers etc with the a list
				else {
					variablesAF.push([item.Name, item.idx])
				}

			})
		}
	}
});

if (variablesAF.length === 0) { variablesAF.push(["No devices found", 0]); }
if (variablesGL.length === 0) { variablesGL.push(["No devices found", 0]); }
if (variablesMR.length === 0) { variablesMR.push(["No devices found", 0]); }
if (variablesSZ.length === 0) { variablesSZ.push(["No devices found", 0]); }


switchesAF.sort();
switchesGL.sort();
switchesMR.sort();
switchesSZ.sort();

utilities.sort();
utilitiesAF.sort();
utilitiesGL.sort();
utilitiesMR.sort();
utilitiesSZ.sort();
texts.sort();

temperatures.sort();
humidity.sort();
dewpoint.sort();
barometer.sort();
weather.sort();
groups.sort();
scenes.sort();
cameras.sort();
setpoints.sort();

variablesAF.sort();
variablesGL.sort();
variablesMR.sort();
variablesSZ.sort();

Blockly.Blocks['switchvariablesAF'] = {
	// Variable getter.
	category: null,  // Variables are handled specially.
	init: function () {
		this.setColour(30);
		this.appendDummyInput()
			.appendField('A-F ')
			.appendField(new Blockly.FieldDropdown(switchesAF), 'Switch');
		this.setOutput(true, null);
		this.setTooltip(Blockly.DOMOTICZSWITCHES_TOOLTIP);
	}
};

Blockly.Blocks['switchvariablesGL'] = {
	// Variable getter.
	category: null,  // Variables are handled specially.
	init: function () {
		this.setColour(30);
		this.appendDummyInput()
			.appendField('G-L ')
			.appendField(new Blockly.FieldDropdown(switchesGL), 'Switch');
		this.setOutput(true, null);
		this.setTooltip(Blockly.DOMOTICZSWITCHES_TOOLTIP);
	}
};

Blockly.Blocks['switchvariablesMR'] = {
	// Variable getter.
	category: null,  // Variables are handled specially.
	init: function () {
		this.setColour(30);
		this.appendDummyInput()
			.appendField('M-R ')
			.appendField(new Blockly.FieldDropdown(switchesMR), 'Switch');
		this.setOutput(true, null);
		this.setTooltip(Blockly.DOMOTICZSWITCHES_TOOLTIP);
	}
};

Blockly.Blocks['switchvariablesSZ'] = {
	// Variable getter.
	category: null,  // Variables are handled specially.
	init: function () {
		this.setColour(30);
		this.appendDummyInput()
			.appendField('S-Z ')
			.appendField(new Blockly.FieldDropdown(switchesSZ), 'Switch');
		this.setOutput(true, null);
		this.setTooltip(Blockly.DOMOTICZSWITCHES_TOOLTIP);
	}
};

Blockly.Blocks['utilityvariables'] = {
	// Variable getter.
	category: null,  // Variables are handled specially.
	init: function () {
		this.setColour(330);
		this.appendDummyInput()
			.appendField(new Blockly.FieldDropdown(utilities), 'Utility');
		this.appendDummyInput()
			.appendField(Blockly.DOMOTICZCONTROLS_MSG_UTILITY, 'UtilityLabel');
		this.setInputsInline(true);
		this.setOutput(true, null);
		this.setTooltip(Blockly.DOMOTICZVARIABLES_UTILITY_TOOLTIP);
	}
};

Blockly.Blocks['utilityvariablesAF'] = {
	// Variable getter.
	category: null,  // Variables are handled specially.
	init: function () {
		this.setColour(290);
		this.appendDummyInput()
			.appendField('A-F ')
			.appendField(new Blockly.FieldDropdown(utilitiesAF), 'Utility');
		this.appendDummyInput()
			.appendField(Blockly.DOMOTICZCONTROLS_MSG_UTILITY, 'UtilityLabel');
		this.setInputsInline(true);
		this.setOutput(true, null);
		this.setTooltip(Blockly.DOMOTICZVARIABLES_UTILITY_TOOLTIP);
	}
};

Blockly.Blocks['utilityvariablesGL'] = {
	// Variable getter.
	category: null,  // Variables are handled specially.
	init: function () {
		this.setColour(290);
		this.appendDummyInput()
			.appendField('G-L ')
			.appendField(new Blockly.FieldDropdown(utilitiesGL), 'Utility');
		this.appendDummyInput()
			.appendField(Blockly.DOMOTICZCONTROLS_MSG_UTILITY, 'UtilityLabel');
		this.setInputsInline(true);
		this.setOutput(true, null);
		this.setTooltip(Blockly.DOMOTICZVARIABLES_UTILITY_TOOLTIP);
	}
};

Blockly.Blocks['utilityvariablesMR'] = {
	// Variable getter.
	category: null,  // Variables are handled specially.
	init: function () {
		this.setColour(290);
		this.appendDummyInput()
			.appendField('M-R ')
			.appendField(new Blockly.FieldDropdown(utilitiesMR), 'Utility');
		this.appendDummyInput()
			.appendField(Blockly.DOMOTICZCONTROLS_MSG_UTILITY, 'UtilityLabel');
		this.setInputsInline(true);
		this.setOutput(true, null);
		this.setTooltip(Blockly.DOMOTICZVARIABLES_UTILITY_TOOLTIP);
	}
};

Blockly.Blocks['utilityvariablesSZ'] = {
	// Variable getter.
	category: null,  // Variables are handled specially.
	init: function () {
		this.setColour(290);
		this.appendDummyInput()
			.appendField('S-Z ')
			.appendField(new Blockly.FieldDropdown(utilitiesSZ), 'Utility');
		this.appendDummyInput()
			.appendField(Blockly.DOMOTICZCONTROLS_MSG_UTILITY, 'UtilityLabel');
		this.setInputsInline(true);
		this.setOutput(true, null);
		this.setTooltip(Blockly.DOMOTICZVARIABLES_UTILITY_TOOLTIP);
	}
};

Blockly.Blocks['zwavealarms'] = {
	// Variable getter.
	category: null,
	init: function () {
		this.setColour(330);
		this.appendDummyInput()
			.appendField(new Blockly.FieldDropdown(zwavealarms), 'ZWave Alarms');
		this.appendDummyInput()
			.appendField(Blockly.DOMOTICZCONTROLS_MSG_ZWAVEALARMS, 'ZWaveAlarmsLabel');
		this.setInputsInline(true);
		this.setOutput(true, null);
		this.setTooltip(Blockly.DOMOTICZVARIABLES_ZWAVEALARMS_TOOLTIP);
	}
};

Blockly.Blocks['uservariablesAF'] = {
	// Variable getter.
	category: null,  // Variables are handled specially.
	init: function () {
		this.setColour(80);
		this.appendDummyInput()
			.appendField('Var A-F ')
			.appendField(new Blockly.FieldDropdown(variablesAF), 'Variable');
		this.setOutput(true, null);
		this.setTooltip(Blockly.DOMOTICZUSERVARIABLES_TOOLTIP);
	}
};

Blockly.Blocks['uservariablesGL'] = {
	// Variable getter.
	category: null,  // Variables are handled specially.
	init: function () {
		this.setColour(80);
		this.appendDummyInput()
			.appendField('Var G-L ')
			.appendField(new Blockly.FieldDropdown(variablesGL), 'Variable');
		this.setOutput(true, null);
		this.setTooltip(Blockly.DOMOTICZUSERVARIABLES_TOOLTIP);
	}
};

Blockly.Blocks['uservariablesMR'] = {
	// Variable getter.
	category: null,  // Variables are handled specially.
	init: function () {
		this.setColour(80);
		this.appendDummyInput()
			.appendField('Var M-R ')
			.appendField(new Blockly.FieldDropdown(variablesMR), 'Variable');
		this.setOutput(true, null);
		this.setTooltip(Blockly.DOMOTICZUSERVARIABLES_TOOLTIP);
	}
};

Blockly.Blocks['uservariablesSZ'] = {
	// Variable getter.
	category: null,  // Variables are handled specially.
	init: function () {
		this.setColour(80);
		this.appendDummyInput()
			.appendField('Var S-Z ')
			.appendField(new Blockly.FieldDropdown(variablesSZ), 'Variable');
		this.setOutput(true, null);
		this.setTooltip(Blockly.DOMOTICZUSERVARIABLES_TOOLTIP);
	}
};

Blockly.Blocks.logic.HUE = 120;
Blockly.Blocks.texts.HUE = Blockly.Blocks.math.HUE;

Blockly.Blocks['domoticzcontrols_if'] = {
	category: null,  // Variables are handled specially.
	init: function () {
		this.setColour(Blockly.Blocks.logic.HUE);
		this.appendValueInput('IF0')
			.appendField(Blockly.DOMOTICZCONTROLS_MSG_IF);
		this.appendStatementInput('DO0')
			.appendField(Blockly.DOMOTICZCONTROLS_MSG_DO);
		this.setTooltip(Blockly.DOMOTICZCONTROLS_IF_TOOLTIP);
		//this.setPreviousStatement(true);
		//this.setNextStatement(true);
	}
};

Blockly.Blocks['domoticzcontrols_ifelseif'] = {
	// If/elseif/else condition.
	helpUrl: Blockly.LANG_CONTROLS_IF_HELPURL,
	init: function () {
		this.setColour(Blockly.Blocks.logic.HUE);
		this.appendValueInput('IF0')
			.setCheck('Boolean')
			.appendField(Blockly.DOMOTICZCONTROLS_MSG_IF);
		this.appendStatementInput('DO0')
			.appendField(Blockly.DOMOTICZCONTROLS_MSG_DO);
		this.setPreviousStatement(true);
		this.setNextStatement(true);
		this.setMutator(new Blockly.Mutator(['controls_if_elseif']));
		// Assign 'this' to a variable for use in the tooltip closure below.
		this.setTooltip(Blockly.DOMOTICZCONTROLS_IF_TOOLTIP);
		this.elseifCount_ = 0;
		this.elseCount_ = 0;
	},
	mutationToDom: function () {
		if (!this.elseifCount_) {
			return null;
		}
		var container = document.createElement('mutation');
		if (this.elseifCount_) {
			container.setAttribute('elseif', this.elseifCount_);
		}
		return container;
	},
	domToMutation: function (xmlElement) {
		this.elseifCount_ = parseInt(xmlElement.getAttribute('elseif'), 10) || 0;
		for (var x = 1; x <= this.elseifCount_; x++) {
			this.appendValueInput('IF' + x)
				.setCheck('Boolean')
				.appendField(Blockly.DOMOTICZCONTROLS_MSG_ELSEIF);
			this.appendStatementInput('DO' + x)
				.appendField(Blockly.DOMOTICZCONTROLS_MSG_DO);
		}
	},
	decompose: function (workspace) {
		var containerBlock = workspace.newBlock('controls_if_if');
		containerBlock.initSvg();
		var connection = containerBlock.nextConnection;
		for (var x = 1; x <= this.elseifCount_; x++) {
			var elseifBlock = workspace.newBlock('controls_if_elseif');
			elseifBlock.initSvg();
			connection.connect(elseifBlock.previousConnection);
			connection = elseifBlock.nextConnection;
		}
		return containerBlock;
	},
	compose: function (containerBlock) {
		// Disconnect all the elseif input blocks and remove the inputs.
		for (var x = this.elseifCount_; x > 0; x--) {
			this.removeInput('IF' + x);
			this.removeInput('DO' + x);
		}
		this.elseifCount_ = 0;
		// Rebuild the block's optional inputs.
		var clauseBlock = containerBlock.nextConnection.targetBlock();
		while (clauseBlock) {
			switch (clauseBlock.type) {
				case 'controls_if_elseif':
					this.elseifCount_++;
					var ifInput = this.appendValueInput('IF' + this.elseifCount_)
						.setCheck('Boolean')
						.appendField(Blockly.DOMOTICZCONTROLS_MSG_ELSEIF);
					var doInput = this.appendStatementInput('DO' + this.elseifCount_);
					doInput.appendField(Blockly.DOMOTICZCONTROLS_MSG_DO);
					// Reconnect any child blocks.
					if (clauseBlock.valueConnection_) {
						ifInput.connection.connect(clauseBlock.valueConnection_);
					}
					if (clauseBlock.statementConnection_) {
						doInput.connection.connect(clauseBlock.statementConnection_);
					}
					break;
				default:
					throw 'Unknown block type.';
			}
			clauseBlock = clauseBlock.nextConnection &&
				clauseBlock.nextConnection.targetBlock();
		}
	},
	saveConnections: function (containerBlock) {
		// Store a pointer to any connected child blocks.
		var clauseBlock = containerBlock.nextConnection.targetBlock();
		var x = 1;
		while (clauseBlock) {
			switch (clauseBlock.type) {
				case 'controls_if_elseif':
					var inputIf = this.getInput('IF' + x);
					var inputDo = this.getInput('DO' + x);
					clauseBlock.valueConnection_ =
						inputIf && inputIf.connection.targetConnection;
					clauseBlock.statementConnection_ =
						inputDo && inputDo.connection.targetConnection;
					x++;
					break;
				default:
					throw 'Unknown block type.';
			}
			clauseBlock = clauseBlock.nextConnection &&
				clauseBlock.nextConnection.targetBlock();
		}
	}
};


Blockly.Blocks['temperaturevariables'] = {
	// Variable getter.
	category: null,  // Variables are handled specially.
	init: function () {
		this.setColour(330);
		this.appendDummyInput()
			.appendField(new Blockly.FieldDropdown(temperatures), 'Temperature');
		this.appendDummyInput()
			.appendField(Blockly.DOMOTICZCONTROLS_MSG_TEMP, 'TemperatureLabel');
		this.setInputsInline(true);
		this.setOutput(true, null);
		this.setTooltip(Blockly.DOMOTICZVARIABLES_TEMPERATURE_TOOLTIP);
	}
};

Blockly.Blocks['humidityvariables'] = {
	// Variable getter.
	category: null,  // Variables are handled specially.
	init: function () {
		this.setColour(226);
		this.appendDummyInput()
			.appendField(new Blockly.FieldDropdown(humidity), 'Humidity');
		this.appendDummyInput()
			.appendField(Blockly.DOMOTICZCONTROLS_MSG_HUM, 'HumidityLabel');
		this.setInputsInline(true);
		this.setOutput(true, null);
		this.setTooltip(Blockly.DOMOTICZVARIABLES_HUMIDITY_TOOLTIP);
	}
};

Blockly.Blocks['dewpointvariables'] = {
	// Variable getter.
	category: null,  // Variables are handled specially.
	init: function () {
		this.setColour(330);
		this.appendDummyInput()
			.appendField(new Blockly.FieldDropdown(dewpoint), 'Dewpoint');
		this.appendDummyInput()
			.appendField(Blockly.DOMOTICZCONTROLS_MSG_DEWPOINT, 'DewpointLabel');
		this.setInputsInline(true);
		this.setOutput(true, null);
		this.setTooltip(Blockly.DOMOTICZVARIABLES_DEWPOINT_TOOLTIP);
	}
};

Blockly.Blocks['barometervariables'] = {
	// Variable getter.
	category: null,  // Variables are handled specially.
	init: function () {
		this.setColour(68);
		this.appendDummyInput()
			.appendField(new Blockly.FieldDropdown(barometer), 'Barometer');
		this.appendDummyInput()
			.appendField(Blockly.DOMOTICZCONTROLS_MSG_BARO, 'BarometerLabel');
		this.setInputsInline(true);
		this.setOutput(true, null);
		this.setTooltip(Blockly.DOMOTICZVARIABLES_BAROMETER_TOOLTIP);
	}
};

Blockly.Blocks['weathervariables'] = {
	// Variable getter.
	category: null,  // Variables are handled specially.
	init: function () {
		this.setColour(210);
		this.appendDummyInput()
			.appendField(new Blockly.FieldDropdown(weather), 'Weather');
		this.setOutput(true, null);
		this.setTooltip(Blockly.DOMOTICZVARIABLES_WEATHER_TOOLTIP);
	}
};

Blockly.Blocks['scenevariables'] = {
	// Variable getter.
	category: null,  // Variables are handled specially.
	init: function () {
		this.setColour(200);
		this.setPreviousStatement(true);
		this.setNextStatement(true);
		this.appendDummyInput()
			.appendField(Blockly.DOMOTICZCONTROLS_MSG_SET)
			.appendField(Blockly.DOMOTICZCONTROLS_MSG_SCENE)
			.appendField(new Blockly.FieldDropdown(scenes), 'Scene')
			.appendField(" = ")
			.appendField(new Blockly.FieldDropdown(this.STATE), 'Status');
		this.setInputsInline(true);
		this.setTooltip(Blockly.DOMOTICZVARIABLES_SCENES_TOOLTIP);
	}
};

Blockly.Blocks['scenevariables'].STATE =
	[["Active", 'Active'],
	["Inactive", 'Inactive'],
	["On", 'On']];

Blockly.Blocks['groupvariables'] = {
	// Variable getter.
	category: null,  // Variables are handled specially.
	init: function () {
		this.setColour(200);
		this.setPreviousStatement(true);
		this.setNextStatement(true);
		this.appendDummyInput()
			.appendField(Blockly.DOMOTICZCONTROLS_MSG_SET)
			.appendField(Blockly.DOMOTICZCONTROLS_MSG_GROUP)
			.appendField(new Blockly.FieldDropdown(groups), 'Group')
			.appendField(" = ")
			.appendField(new Blockly.FieldDropdown(this.STATE), 'Status');
		this.setInputsInline(true);
		this.setTooltip(Blockly.DOMOTICZVARIABLES_GROUPS_TOOLTIP);
	}
};

Blockly.Blocks['groupvariables'].STATE =
	[["Active", 'Active'],
	["Inactive", 'Inactive'],
	["On", 'On'],
	["Off", 'Off']];

Blockly.Blocks['cameravariables'] = {
	// Variable getter.
	category: null,  // Variables are handled specially.
	init: function () {
		this.setColour(100);
		this.setPreviousStatement(true);
		this.setNextStatement(true);

		this.appendDummyInput()
			.appendField(Blockly.DOMOTICZCONTROLS_MSG_SENDCAMERA)
			.appendField(new Blockly.FieldDropdown(cameras), 'Camera')
			.appendField(Blockly.DOMOTICZCONTROLS_MSG_WITHSUBJECT)
			.appendField(new Blockly.FieldTextInput(''), 'Subject')
			.appendField(Blockly.DOMOTICZCONTROLS_MSG_AFTER)
			.appendField(new Blockly.FieldTextInput('0'), 'NUM')
			.appendField(Blockly.DOMOTICZCONTROLS_MSG_SECONDS);
		this.setInputsInline(true);
		this.setTooltip(Blockly.DOMOTICZ_LOGIC_CAMERA_TOOLTIP);
	}
};

Blockly.Blocks['setpointvariables'] = {
	// Variable getter.
	category: null,  // Variables are handled specially.
	init: function () {
		this.setColour(100);
		this.setPreviousStatement(true);
		this.setNextStatement(true);

		this.appendDummyInput()
			.appendField(Blockly.DOMOTICZCONTROLS_MSG_SETSETPOINT)
			.appendField(new Blockly.FieldDropdown(setpoints), 'SetPoint')
			.appendField(Blockly.DOMOTICZCONTROLS_MSG_TO)
			.appendField(new Blockly.FieldTextInput('0'), 'NUM')
			.appendField(Blockly.DOMOTICZCONTROLS_MSG_DEGREES);
		this.setInputsInline(true);
		this.setTooltip(Blockly.DOMOTICZ_LOGIC_SETPOINT_TOOLTIP);
	}
};

Blockly.Blocks['textvariables'] = {
	// Variable getter.
	category: null,
	init: function () {
		this.setColour(330);
		this.appendDummyInput()
			.appendField(new Blockly.FieldDropdown(texts), 'Text');
		this.appendDummyInput()
			.appendField(Blockly.DOMOTICZCONTROLS_MSG_TEXT, 'TextLabel');
		this.setInputsInline(true);
		this.setOutput(true, null);
		this.setTooltip(Blockly.DOMOTICZVARIABLES_TEXT_TOOLTIP);
	}
};

Blockly.Blocks['logic_states'] = {
	helpUrl: null,
	init: function () {
		this.setColour(Blockly.Blocks.math.HUE);
		this.setOutput(true, null);
		this.appendDummyInput()
			.appendField(new Blockly.FieldDropdown(this.STATES), 'State');
	}
};

Blockly.Blocks['logic_set'] = {
	// Comparison operator.
	init: function () {
		this.setColour(Blockly.Blocks.logic.HUE);
		this.setPreviousStatement(true);
		this.setNextStatement(true);
		this.appendValueInput('A')
			.appendField(Blockly.DOMOTICZCONTROLS_MSG_SET);
		this.appendValueInput('B')
			.appendField("=");
		this.setInputsInline(true);
		this.setTooltip(Blockly.DOMOTICZ_LOGIC_SET_TOOLTIP);

	}
};

Blockly.Blocks['logic_setafter'] = {
	// Comparison operator.
	init: function () {
		this.setColour(Blockly.Blocks.logic.HUE);
		this.setPreviousStatement(true);
		this.setNextStatement(true);
		this.appendValueInput('A')
			.appendField(Blockly.DOMOTICZCONTROLS_MSG_SET);
		this.appendValueInput('B')
			.appendField("=");
		this.appendValueInput('C')
			.appendField(Blockly.DOMOTICZCONTROLS_MSG_AFTER);
		this.appendDummyInput()
			.appendField(Blockly.DOMOTICZCONTROLS_MSG_SECONDS);
		this.setInputsInline(true);
		this.setTooltip(Blockly.DOMOTICZ_LOGIC_SETAFTER_TOOLTIP);
	}
};

Blockly.Blocks['logic_setdelayed'] = {
	// Comparison operator.
	init: function () {
		this.setColour(Blockly.Blocks.logic.HUE);
		this.setPreviousStatement(true);
		this.setNextStatement(true);
		this.appendValueInput('A')
			.appendField(Blockly.DOMOTICZCONTROLS_MSG_SET);
		this.appendValueInput('B')
			.appendField("=");
		this.appendValueInput('C')
			.appendField(Blockly.DOMOTICZCONTROLS_MSG_FOR);
		this.appendDummyInput()
			.appendField(Blockly.DOMOTICZCONTROLS_MSG_MINUTES);
		this.setInputsInline(true);
		this.setTooltip(Blockly.DOMOTICZ_LOGIC_SETDELAYED_TOOLTIP);
	}
};

Blockly.Blocks['logic_setrandom'] = {
	// Comparison operator.
	init: function () {
		this.setColour(Blockly.Blocks.logic.HUE);
		this.setPreviousStatement(true);
		this.setNextStatement(true);
		this.appendValueInput('A')
			.appendField(Blockly.DOMOTICZCONTROLS_MSG_SET);
		this.appendValueInput('B')
			.appendField("=");
		this.appendValueInput('C')
			.appendField(Blockly.DOMOTICZCONTROLS_MSG_RANDOM);
		this.appendDummyInput()
			.appendField(Blockly.DOMOTICZCONTROLS_MSG_MINUTES);
		this.setInputsInline(true);
		this.setTooltip(Blockly.DOMOTICZ_LOGIC_SETRANDOM_TOOLTIP);
	}
};

Blockly.Blocks['logic_setlevel'] = {
	init: function () {
		this.setColour(Blockly.Blocks.math.HUE);
		this.appendDummyInput()
			.appendField(Blockly.DOMOTICZCONTROLS_MSG_SETLEVEL);
		this.appendDummyInput()
			.appendField(new Blockly.FieldTextInput('0',
				this.percentageValidator), 'NUM');
		this.setOutput(true, 'Number');
		this.setInputsInline(true);
		this.setTooltip(Blockly.DOMOTICZ_LOGIC_SETLEVEL_TOOLTIP);
	}
};

Blockly.Blocks['logic_setlevel'].percentageValidator = function (text) {
	var n = parseFloat(text || 0);
	if (!isNaN(n)) {
		if (n > 100) { n = 100 }
		if (n < 0) { n = 0 }
	}

	return isNaN(n) ? null : String(n);
};


Blockly.Blocks['logic_timeofday'] = {
	// Comparison operator.
	init: function () {
		this.setColour(Blockly.Blocks.math.HUE);
		this.setOutput(true, null);
		this.appendValueInput(Blockly.DOMOTICZCONTROLS_MSG_TIME)
			.appendField("Time:")
			.appendField(new Blockly.FieldDropdown(this.OPERATORS), 'OP');
		this.setInputsInline(true);
		var thisBlock = this;
		this.setTooltip(function () {
			var op = thisBlock.getTitleValue('OP');
			return thisBlock.TOOLTIPS[op];
		});
	}
};

Blockly.Blocks['logic_timevalue'] = {
	init: function () {
		this.setColour(Blockly.Blocks.math.HUE);
		this.appendDummyInput()
			.appendField(new Blockly.FieldTextInput('00:00',
				this.TimeValidator), 'TEXT');
		this.setOutput(true, 'String');
		this.setTooltip(Blockly.DOMOTICZ_LOGIC_TIMEVALUE_TOOLTIP);
	}
};

Blockly.Blocks['logic_timevalue.TimeValidator'] = function (text) {
	if (text.match(/^([01]?[0-9]|2[0-3]):[0-5][0-9]/)) return text;
	return "00:00";
};


Blockly.Blocks['logic_timeofday'].TOOLTIPS = {
	EQ: Blockly.LANG_LOGIC_COMPARE_TOOLTIP_EQ,
	NEQ: Blockly.LANG_LOGIC_COMPARE_TOOLTIP_NEQ,
	LT: Blockly.LANG_LOGIC_COMPARE_TOOLTIP_LT,
	LTE: Blockly.LANG_LOGIC_COMPARE_TOOLTIP_LTE,
	GT: Blockly.LANG_LOGIC_COMPARE_TOOLTIP_GT,
	GTE: Blockly.LANG_LOGIC_COMPARE_TOOLTIP_GTE
};

Blockly.Blocks['logic_sunrisesunset'] = {
	init: function () {
		this.setOutput(true, null);
		this.setColour(Blockly.Blocks.math.HUE);
		this.appendDummyInput()
			.appendField(new Blockly.FieldDropdown(this.VALUES), 'SunriseSunset')
			.appendField(" ");
		this.setTooltip(Blockly.DOMOTICZ_LOGIC_SUNRISESUNSET_TOOLTIP);
	}
};

Blockly.Blocks['logic_sunrisesunset'].VALUES =
	[["Sunrise", 'Sunrise'],
	["Sunset", 'Sunset']];

Blockly.Blocks['logic_notification_priority'] = {
	init: function () {
		this.setOutput(true, null);
		this.setColour(230);
		this.appendDummyInput()
			.appendField(new Blockly.FieldDropdown(this.PRIORITY), 'NotificationPriority')
			.appendField(" ");
		this.setTooltip(Blockly.DOMOTICZ_LOGIC_NOTIFICATION_PRIORITY_TOOLTIP);
	}
};

Blockly.Blocks['send_notification'] = {
	// Comparison operator.
	init: function () {
		this.setColour(Blockly.Blocks.logic.HUE);
		this.setPreviousStatement(true);
		this.setNextStatement(true);
		this.appendValueInput('notificationTextSubject')
			.appendField("Send notification with subject:");
		this.appendValueInput('notificationTextBody')
			.appendField("and message:");
		this.appendDummyInput()
			.appendField("through subsystem:")
			.appendField(new Blockly.FieldDropdown(Blockly.Blocks['logic_notification_priority'].SUBSYSTEM), 'notificationSubsystem');
		this.appendDummyInput()
			.appendField("with priority:")
			.appendField(new Blockly.FieldDropdown(Blockly.Blocks['logic_notification_priority'].PRIORITY), 'notificationPriority');
		this.appendDummyInput()
			.appendField("with sound (Pushover):")
			.appendField(new Blockly.FieldDropdown(Blockly.Blocks['logic_notification_priority'].SOUND), 'notificationSound');
		this.setInputsInline(true);
		this.setTooltip(Blockly.DOMOTICZ_LOGIC_NOTIFICATION_TOOLTIP);

	}
};

Blockly.Blocks['send_email'] = {
	// Comparison operator.
	init: function () {
		this.setColour(120);
		this.setPreviousStatement(true);
		this.setNextStatement(true);
		this.appendDummyInput()
			.appendField("Send email with subject:")
			.appendField(new Blockly.FieldTextInput(''), 'TextSubject');
		this.appendDummyInput()
			.appendField("and message:")
			.appendField(new Blockly.FieldTextInput(''), 'TextBody');
		this.appendDummyInput()
			.appendField("to:")
			.appendField(new Blockly.FieldTextInput(''), 'TextTo');
		this.setInputsInline(true);
		this.setTooltip(Blockly.DOMOTICZ_LOGIC_EMAIL_TOOLTIP);
	}
};

Blockly.Blocks['send_sms'] = {
	// Comparison operator.
	init: function () {
		this.setColour(120);
		this.setPreviousStatement(true);
		this.setNextStatement(true);
		this.appendDummyInput()
			.appendField("Send SMS with subject:")
			.appendField(new Blockly.FieldTextInput(''), 'TextSubject');
		this.setInputsInline(true);
		this.setTooltip(Blockly.DOMOTICZ_LOGIC_SMS_TOOLTIP);
	}
};

Blockly.Blocks['trigger_ifttt'] = {
	// Comparison operator.
	init: function () {
		this.setColour(120);
		this.setPreviousStatement(true);
		this.setNextStatement(true);
		this.appendDummyInput()
			.appendField("Trigger IFTTT. Event_ID:")
			.appendField(new Blockly.FieldTextInput(''), 'EventID');
		this.appendDummyInput()
			.appendField("Optional: value1:")
			.appendField(new Blockly.FieldTextInput(''), 'TextValue1');
		this.appendDummyInput()
			.appendField("value2:")
			.appendField(new Blockly.FieldTextInput(''), 'TextValue2');
		this.appendDummyInput()
			.appendField("value3:")
			.appendField(new Blockly.FieldTextInput(''), 'TextValue3');
		this.setInputsInline(true);
		this.setTooltip(Blockly.DOMOTICZ_LOGIC_IFTTT_TOOLTIP);
	}
};

Blockly.Blocks['start_script'] = {
	// Comparison operator.
	init: function () {
		this.setColour(120);
		this.setPreviousStatement(true);
		this.setNextStatement(true);
		this.appendDummyInput()
			.appendField("Start Script:")
			.appendField(new Blockly.FieldTextInput(''), 'TextPath');
		this.appendDummyInput()
			.appendField("with parameter(s):")
			.appendField(new Blockly.FieldTextInput(''), 'TextParam');
		this.setInputsInline(true);
		this.setTooltip(Blockly.DOMOTICZ_LOGIC_SCRIPT_TOOLTIP);
	}
};

Blockly.Blocks['open_url'] = {
	// Comparison operator.
	init: function () {
		this.setColour(120);
		this.setPreviousStatement(true);
		this.setNextStatement(true);
		this.appendValueInput('urlToOpen')
			.appendField("Open url:");
		this.setInputsInline(true);
		this.setTooltip(Blockly.DOMOTICZ_LOGIC_OPENURL_TOOLTIP);

	}
};

Blockly.Blocks['url_text'] = {
	// Text value.
	init: function () {
		this.setColour(160);
		this.appendDummyInput()
			.appendField("http://")
			.appendField(new Blockly.FieldTextInput('', this.URLValidator), 'TEXT')
		this.setOutput(true, 'String');
		this.setTooltip(Blockly.DOMOTICZ_LOGIC_URL_TEXT_TOOLTIP);
	}
};

Blockly.Blocks['url_text'].URLValidator = function (text) {
	if (text.substr(0, 7).toLowerCase() == "http://") {
		text = text.substr(7);
	}
	return text;

};

Blockly.Blocks['writetolog'] = {
	// Comparison operator.
	init: function () {
		this.setColour(90);
		this.setPreviousStatement(true);
		this.setNextStatement(true);
		this.appendValueInput('writeToLog')
			.appendField("Write to log:");
		this.setInputsInline(true);
		this.setTooltip(Blockly.DOMOTICZ_LOGIC_WRITETOLOG_TOOLTIP);

	}
};

Blockly.Blocks['security_status'] = {
	// Variable getter.
	init: function () {
		this.setColour(0);
		this.setOutput(true, null);
		this.appendDummyInput()
			.appendField(Blockly.DOMOTICZCONTROLS_MSG_SECURITY)
			.appendField(new Blockly.FieldDropdown(this.OPERATORS), 'OP')
			.appendField(" ")
			.appendField(new Blockly.FieldDropdown(this.STATE), 'Status');
		this.setTooltip(Blockly.DOMOTICZ_SECURITY_STATUS_TOOLTIP);
	}
};

Blockly.Blocks['security_status'].STATE =
	[["Disarmed", '0'],
	["Armed Home", '1'],
	["Armed Away", '2']];

Blockly.Blocks['security_status'].OPERATORS =
	[['=', 'EQ'],
	['\u2260', 'NEQ']];


Blockly.Blocks['logic_weekday'] = {
	// Variable getter.
	init: function () {
		this.setColour(Blockly.Blocks.math.HUE);
		this.setOutput(true, null);
		this.appendDummyInput()
			.appendField(Blockly.DOMOTICZCONTROLS_MSG_DAY)
			.appendField(new Blockly.FieldDropdown(this.OPERATORS), 'OP')
			.appendField(" ")
			.appendField(new Blockly.FieldDropdown(this.DAYS), 'Weekday');
		this.setTooltip(Blockly.DOMOTICZ_LOGIC_WEEKDAY_TOOLTIP);
	}
};

Blockly.Blocks['logic_weekday'].DAYS =
	[["Monday", '2'],
	["Tuesday", '3'],
	["Wednesday", '4'],
	["Thursday", '5'],
	["Friday", '6'],
	["Saturday", '7'],
	["Sunday", '1']];

Blockly.Blocks['logic_notification_priority'].PRIORITY =
	[["-2 (Prowl: Very Low, Pushover: N/A)", '-2'],
	["-1 (Prowl: Moderate, Pushover:Quiet)", '-1'],
	["0 (All: Normal)", '0'],
	["1 (All: High)", '1'],
	["2 (Prowl: Emergency, Pushover: confirm)", '2']];

Blockly.Blocks['logic_notification_priority'].SOUND =
	[["Pushover (default)", 'pushover'],
	["Bike", 'bike'],
	["Bugle", 'bugle'],
	["Cash Register", 'cashregister'],
	["Classical", 'classical'],
	["Cash Register", 'cashregister'],
	["Cosmic", 'cosmic'],
	["Falling", 'falling'],
	["Gamelan", 'gamelan'],
	["Incoming", 'incoming'],
	["Intermission", 'intermission'],
	["Magic", 'magic'],
	["Mechanical", 'mechanical'],
	["Piano Bar", 'pianobar'],
	["Siren", 'siren'],
	["Space Alarm", 'spacealarm'],
	["Tug Boat", 'tugboat'],
	["Alien Alarm (long)", 'alien'],
	["Climb (long)", 'climb'],
	["Persistent (long)", 'persistent'],
	["Pushover Echo (long)", 'echo'],
	["Up Down (long)", 'updown'],
	["None (silent)", 'none']];

Blockly.Blocks['logic_notification_priority'].SUBSYSTEM =
	[
	["all (default)", ''],
	["gcm", 'gcm'],
	["http", 'http'],
	["kodi", 'kodi'],
	["lms", 'lms'],
	["prowl", 'prowl'],
	["pushalot", 'pushalot'],
	["pushbullet", 'pushbullet'],
	["pushover", 'pushover'],
	["pushsafer", 'pushsafer'],
	["telegram", 'telegram']
	];


Blockly.Blocks['logic_states'].STATES =
	[
		["On", 'On'],
		["Off", 'Off'],
		["Group On", 'Group On'],
		["Group Off", 'Group Off'],
		["Open", 'Open'],
		["Closed", 'Closed'],
		["Locked", 'Locked'],
		["Unlocked", 'Unlocked'],
		["Panic", 'Panic'],
		["Panic End", 'Panic End'],
		["Normal", 'Normal'],
		["Alarm", 'Alarm'],
		["Motion", 'Motion'],
		["No Motion", 'No Motion'],
		["Chime", 'Chime'],
		["Stop", 'Stop'],
		["Stopped", 'Stopped'],
		["Video", 'Video'],
		["Audio", 'Audio'],
		["Photo", 'Photo'],
		["Paused", 'Paused'],
		["Playing", 'Playing'],
		["1", '1'],
		["2", '2'],
		["3", '3'],
		["timer", 'timer'],
		["Disco Mode 1", 'Disco Mode 1'],
		["Disco Mode 2", 'Disco Mode 2'],
		["Disco Mode 3", 'Disco Mode 3'],
		["Disco Mode 4", 'Disco Mode 4'],
		["Disco Mode 5", 'Disco Mode 5'],
		["Disco Mode 6", 'Disco Mode 6'],
		["Disco Mode 7", 'Disco Mode 7'],
		["Disco Mode 8", 'Disco Mode 8'],
		["Disco Mode 9", 'Disco Mode 9'],
	];

Blockly.Blocks['logic_weekday'].OPERATORS =
	[['=', 'EQ'],
	['\u2260', 'NEQ'],
	['<', 'LT'],
	['\u2264', 'LTE'],
	['>', 'GT'],
	['\u2265', 'GTE']];


Blockly.Blocks['logic_timeofday'].OPERATORS =
	[['=', 'EQ'],
	['\u2260', 'NEQ'],
	['<', 'LT'],
	['\u2264', 'LTE'],
	['>', 'GT'],
	['\u2265', 'GTE']];



