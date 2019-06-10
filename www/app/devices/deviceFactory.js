define(function () {

    return function (dzDefaultSwitchIcons, deviceLightApi, sceneApi) {
        return Device;

        function DeviceIcon(device) {
            this.isConfigurable = function() {
                return ['Light/Switch', 'Lighting 1', 'Lighting 2', 'Color Switch'].includes(device.Type) &&
                    [0, 2, 7, 9, 10, 11, 17, 18, 19, 20].includes(device.SwitchTypeVal);
            };

            this.getIcon = function() {
                var image;
                var TypeImg = device.TypeImg;

                if (this.isConfigurable()) {
                    image = device.CustomImage === 0
                        ? dzDefaultSwitchIcons[device.SwitchTypeVal][device.isActive() ? 0 : 1]
                        : device.Image + '48_' + (device.isActive() ? 'On' : 'Off') + '.png'
                } else if (TypeImg.indexOf('Alert') === 0) {
                    image = 'Alert48_' + Math.min(device.Level, 4) + '.png';
                } else if (TypeImg.indexOf('motion') === 0) {
                    image = device.isActive() ? 'motion.png' : 'motionoff.png';
                } else if (TypeImg.indexOf('smoke') === 0) {
                    image = device.isActive() ? 'smoke.png' : 'smokeoff.png';
                } else if (device.Type === 'Scene' || device.Type === 'Group') {
                    image = device.isActive() ? 'push.png' : 'pushoff.png'
                } else {
                    image = device.TypeImg + '.png'
                }

                return 'images/' + image;
            }
        }

        function Device(rawData) {
            Object.assign(this, rawData);

            this.icon = new DeviceIcon(this);

            this.isDimmer = function () {
                return ['Dimmer', 'Blinds Percentage', 'Blinds Percentage Inverted', 'TPI'].includes(this.SwitchType);
            };

            this.isSelector = function () {
                return this.SwitchType === 'Selector';
            };

            this.isLED = function () {
                return (this.SubType.indexOf('RGB') >= 0 || this.SubType.indexOf('WW') >= 0);
            };

            this.toggle = function () {
                if (['Group', 'Scene'].includes(this.Type)) {
                    return this.isActive()
                        ? sceneApi.switchOff(this.idx)
                        : sceneApi.switchOn(this.idx)
                } else if (
                    (['Light/Switch', 'Lighting 2'].includes(this.Type) && [0, 7, 9, 10].includes(this.SwitchTypeVal))
                    || this.Type === 'Color Switch'
                ) {
                    return this.isActive()
                        ? deviceLightApi.switchOff(this.idx)
                        : deviceLightApi.switchOn(this.idx)
                }
            };

            this.isActive = function() {
                return this.Status && (
                    ['On', 'Chime', 'Group On', 'Panic', 'Mixed'].includes(this.Status)
                    || this.Status.indexOf('Set ') === 0
                    || this.Status.indexOf('NightMode') === 0
                    || this.Status.indexOf('Disco ') === 0);
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
                    return 'kWh';
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
                } else if (this.Type === 'Weight') {
					return this.SwitchTypeVal === 0 ? 'kg' : 'lbs';
                } else {
                    return '?';
                }
            }

            this.getLogLink = function () {
                var deviceType = this.Type;
                var logLink = '#/Devices/' + this.idx + '/Log';

                var deviceTypes = ['Light', 'Color Switch', 'Chime', 'Security', 'RFY', 'ASA', 'Usage', 'Energy'];
                var deviceSubTypes = [
                    'Voltage', 'Current', 'Pressure', 'Custom Sensor', 'kWh',
                    'Sound Level', 'Solar Radiation', 'Visibility', 'Distance',
                    'Soil Moisture', 'Leaf Wetness', 'Waterflow', 'Lux', 'Percentage',
                    'Text', 'Alert'
                ];

                if (deviceTypes.some(function(item) {
                    return deviceType.indexOf(item) === 0
                })) {
                    return logLink;
                }

                if (/Temp|Thermostat|Humidity/i.test(deviceType)) {
                    return logLink;
                }

                if (deviceSubTypes.includes(this.SubType)) {
                    return logLink;
                }

                if (this.Counter !== undefined) {
                    return logLink;
                }
            };

            this.openCustomLog = function (container, backFn) {
                GlobalBackFn = backFn;

                if (this.Direction !== undefined) {
                    ShowWindLog(container, 'GlobalBackFn', this.idx, this.Name);
                } else if (this.UVI !== undefined) {
                    ShowUVLog(container, 'GlobalBackFn', this.idx, this.Name);
                } else if (this.Rain !== undefined) {
                    ShowRainLog(container, 'GlobalBackFn', this.idx, this.Name);
                } else if (this.Type.indexOf('Current') === 0) {
                    ShowCurrentLog(container, 'GlobalBackFn', this.idx, this.Name);
                } else if (this.Type === 'Air Quality') {
                    ShowAirQualityLog(container, 'GlobalBackFn', this.idx, this.Name);
                } else if (this.SubType === 'Barometer') {
                    ShowBaroLog(container, 'GlobalBackFn', this.idx, this.Name);
                }
            };


        }
    };
});
