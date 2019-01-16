define(function () {

    return function () {
        return Device;
    };

    function Device(rawData) {
        Object.assign(this, rawData);

        this.isDimmer = function () {
            return ['Dimmer', 'Blinds Percentage', 'Blinds Percentage Inverted', 'TPI'].includes(this.SwitchType);
        };

        this.isSelector = function () {
            return this.SwitchType === 'Selector';
        };

        this.isLED = function () {
            return (this.SubType.indexOf('RGB') >= 0 || this.SubType.indexOf('WW') >= 0);
        };

        this.getLevels = function () {
            return this.LevelNames ? b64DecodeUnicode(this.LevelNames).split('|') : [];
        };

        this.getLevelActions = function () {
            return this.LevelActions ? b64DecodeUnicode(this.LevelActions).split('|') : [];
        };

        this.getSelectorLevelOptions = function () {
            return this.getLevels()
                .slice(1)
                .map(function (levelName, index) {
                    return {
                        label: levelName,
                        value: (index + 1) * 10
                    }
                });
        };

        this.getDimmerLevelOptions = function (step) {
            var options = [];
            var step = step || 5;

            for (var i = step; i <= 100; i += step) {
                options.push({
                    label: i + '%',
                    value: i
                });
            }

            return options;
        };

        this.getUnit = function () {
            if (this.SubType === 'Custom Sensor') {
                return this.SensorUnit;
            } else if (this.Type === 'General' && this.SubType === 'Voltage') {
                return 'V';
            } else if (this.Type === 'General' && this.SubType === 'Distance') {
                return this.SwitchTypeVal === 1 ? 'in' : 'cm'
            } else if (this.Type === 'General' && this.SubType === 'Current') {
                return 'A';
            } else if (this.Type === 'General' && this.SubType === 'Pressure') {
                return 'Bar';
            } else if (this.Type === 'General' && this.SubType === 'Sound Level') {
                return 'dB';
            } else if (this.Type === 'General' && this.SubType === 'kWh') {
                return 'kWh';
            } else if (this.Type === 'General' && this.SubType === 'Managed Counter') {
                switch (this.SwitchTypeVal) {
                    case 0: 
                    case 4: return 'kWh';
                    case 1:
                    case 2: return 'm3';
                    case 3: return this.ValueUnits;
                    case 5: return 's';
                    default: return '?';
                }                
            } else if (this.Type === 'General' && this.SubType === 'Counter Incremental') {
                return '';
            } else if (this.Type === 'P1 Smart Meter' && this.SubType === 'Energy') {
                return 'kWh';
            } else if (this.Type === 'YouLess Meter') {
                return 'kWh';
            } else if (this.Type === 'RFXMeter' && this.SwitchTypeVal === 2) {
                return 'm3';
            } else if (this.Type === 'Usage' && this.SubType === 'Electric') {
                return 'W';
            } else if (this.SubType === 'Gas' || this.SubType === 'Water') {
                return 'm3'
            } else if (this.SubType === 'Visibility') {
                return this.SwitchTypeVal === 1 ? 'mi' : 'km';
            } else if (this.SubType === 'Solar Radiation') {
                return 'Watt/m2';
            } else if (this.SubType === 'Soil Moisture') {
                return 'cb';
            } else if (this.SubType === 'Leaf Wetness') {
                return 'Range';
            } else if (this.SubType === 'Weight') {
                return 'kg';
            } else if (['Voltage', 'A/D'].includes(this.SubType)) {
                return 'mV';
            } else if (this.SubType === 'Waterflow') {
                return 'l/min';
            } else if (this.SubType === 'Lux') {
                return 'lx';
            } else if (this.SubType === 'Percentage') {
                return '%';
            } else {
                return '?';
            }
        }
    }
});
