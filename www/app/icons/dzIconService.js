define(['app'], function (app) {
    'use strict';

    var CHROME_ICONS = {
        'images/desktop.png':      'fa-solid fa-gauge',
        'images/house.png':        'fa-solid fa-house',
        'images/lightbulb.png':    'fa-solid fa-lightbulb',
        'images/lightbulboff.png': 'fa-regular fa-lightbulb',
        'images/scenes.png':       'fa-solid fa-layer-group',
        'images/temperature.png':  'fa-solid fa-temperature-half',
        'images/rain.png':         'fa-solid fa-cloud-rain',
        'images/utility.png':      'fa-solid fa-bolt',

        'images/setup.png':        'fa-solid fa-gear',
        'images/hardware.png':     'fa-solid fa-microchip',
        'images/devices.png':      'fa-solid fa-sliders',
        'images/energy.png':       'fa-solid fa-charging-station',
        'images/users.png':        'fa-solid fa-users',
        'images/update.png':       'fa-solid fa-download',
        'images/log.png':          'fa-solid fa-terminal',
        'images/about.png':        'fa-solid fa-circle-info',
        'images/logout.png':       'fa-solid fa-right-from-bracket',
        'images/restart.png':      'fa-solid fa-rotate-right',
        'images/shutdown.png':     'fa-solid fa-power-off',

        'images/events.png':       'fa-solid fa-code',
        'images/customicons.png':  'fa-solid fa-icons',
        'images/variables.png':    'fa-solid fa-list',
        'images/contact.png':      'fa-solid fa-share-nodes',
        'images/camera-web.png':   'fa-solid fa-video',
        'images/security.png':     'fa-solid fa-shield-halved',
        'images/notification.png': 'fa-solid fa-bell',
        'images/floorplans.png':   'fa-solid fa-map',
        'images/report.png':       'fa-solid fa-chart-bar',

        'images/delete.png':       'fa-solid fa-trash-can',
        'images/rename.png':       'fa-solid fa-pen-to-square',
        'images/add.png':          'fa-solid fa-plus',

        'images/webcam.png':       'fa-solid fa-video',
        'images/override.png':     'fa-solid fa-sliders',
        'images/next.png':         'fa-solid fa-chevron-right',
        'images/capture.png':      'fa-solid fa-camera',
        'images/location.png':     'fa-solid fa-location-dot',

        'images/arrow_up.png':     'fa-solid fa-arrow-trend-up',
        'images/arrow_down.png':   'fa-solid fa-arrow-trend-down',
        'images/arrow_stable.png': 'fa-solid fa-right-long',
        'images/arrow_unk.png':    'fa-solid fa-question',

        'images/blindsstop.png':   'fa-solid fa-stop',

        'images/up.png':           'fa-solid fa-arrow-up',
        'images/down.png':         'fa-solid fa-arrow-down',
        'images/remove.png':       'fa-solid fa-circle-minus',

        'images/ok.png':           'fa-solid fa-circle-check',
        'images/failed.png':       'fa-solid fa-circle-xmark',
        'images/unknown.png':      'fa-solid fa-circle-question',
        'images/sleep.png':        'fa-solid fa-moon',
        'images/heal.png':         'fa-solid fa-heart-pulse',

        'images/battery-ok.png':   'fa-solid fa-battery-full',
        'images/battery-low.png':  'fa-solid fa-battery-quarter',
        'images/battery.png':      'fa-solid fa-battery-half',
        'images/air_signal.png':   'fa-solid fa-signal',

        'images/equal.png':        'fa-solid fa-minus',
        'images/gup.png':          'fa-solid fa-arrow-trend-up',
        'images/gdown.png':        'fa-solid fa-arrow-trend-down',
        'images/gequal.png':       'fa-solid fa-minus',

        'images/favorite.png':     'fa-solid fa-star',
        'images/nofavorite.png':   'fa-regular fa-star'
    };

    var TYPE_ICONS = {
        'light':               'fa-solid fa-lightbulb',
        'dimmer':              'fa-solid fa-circle-half-stroke',
        'glight':              'fa-solid fa-lightbulb',
        'strip':               'fa-solid fa-grip-lines',

        'rgb':                 'fa-solid fa-palette',

        'generic':             'fa-solid fa-toggle-on',
        'push':                'fa-solid fa-circle-dot',
        'onoff':               'fa-solid fa-power-off',
        'pushon':              'fa-solid fa-circle-dot',

        'contact':             'fa-solid fa-door-closed',
        'door':                'fa-solid fa-door-open',
        'window':              'fa-solid fa-window-maximize',

        'blinds':              'fa-solid fa-chevron-down',
        'blindsopen':          'fa-solid fa-chevron-up',

        'heating':             'fa-solid fa-fire',
        'cooling':             'fa-solid fa-snowflake',
        'radiator':            'fa-solid fa-fire-flame-curved',
        'fireplace':           'fa-solid fa-fire',
        'fan':                 'fa-solid fa-fan',
        'ac':                  'fa-solid fa-snowflake',
        'ehome':               'fa-solid fa-house-chimney',

        'water':               'fa-solid fa-droplet',
        'tap':                 'fa-solid fa-faucet',
        'irrigation':          'fa-solid fa-hand-holding-droplet',
        'pool':                'fa-solid fa-water-ladder',
        'pump':                'fa-solid fa-pump-soap',

        'solar':               'fa-solid fa-solar-panel',
        'pv':                  'fa-solid fa-solar-panel',
        'inverter':            'fa-solid fa-bolt',
        'charger':             'fa-solid fa-charging-station',
        'laadpaal':            'fa-solid fa-charging-station',
        'wallsocket':          'fa-solid fa-plug',
        'current':             'fa-solid fa-bolt',

        'tv':                  'fa-solid fa-tv',
        'media':               'fa-solid fa-play',
        'speaker':             'fa-solid fa-volume-high',
        'amplifier':           'fa-solid fa-volume-high',
        'logitechmediaserver': 'fa-solid fa-music',
        'remote':              'fa-solid fa-gamepad',

        'computer':            'fa-solid fa-display',
        'computerpc':          'fa-solid fa-computer',
        'harddisk':            'fa-solid fa-hard-drive',
        'phone':               'fa-solid fa-phone',
        'printer':             'fa-solid fa-print',

        'alarm':               'fa-solid fa-bell',
        'smoke':               'fa-solid fa-triangle-exclamation',
        'motion':              'fa-solid fa-person-running',
        'security':            'fa-solid fa-shield-halved',

        'coffee':              'fa-solid fa-mug-hot',
        'washingmachine':      'fa-solid fa-shirt',
        'christmastree':       'fa-solid fa-tree',

        'temp':                'fa-solid fa-temperature-half',
        'humidity':            'fa-solid fa-droplet',
        'baro':                'fa-solid fa-gauge',
        'rain':                'fa-solid fa-cloud-showers-heavy',
        'wind':                'fa-solid fa-wind',
        'uv':                  'fa-solid fa-sun',
        'lux':                 'fa-solid fa-sun',
        'visibility':          'fa-solid fa-eye',
        'radiation':           'fa-solid fa-radiation',
        'gauge':               'fa-solid fa-gauge',
        'counter':             'fa-solid fa-hashtag',
        'percentage':          'fa-solid fa-percent',
        'scale':               'fa-solid fa-scale-balanced',
        'gas':                 'fa-solid fa-gas-pump',
        'leaf':                'fa-solid fa-leaf',
        'moisture':            'fa-solid fa-hand-holding-droplet',
        'soil':                'fa-solid fa-seedling',
        'air':                 'fa-solid fa-wind',
        'airmeasure':          'fa-solid fa-lungs',
        'sun':                 'fa-solid fa-sun',
        'victron':             'fa-solid fa-car-battery',

        'doorlock':            'fa-solid fa-lock',
        'doorlockcontact':     'fa-solid fa-lock',

        'smartmeter':          'fa-solid fa-bolt',
        'p1smartmeter':        'fa-solid fa-bolt',
        'electricityusage':    'fa-solid fa-bolt',

        'airquality':          'fa-solid fa-smog',
        'pm25':                'fa-solid fa-smog',
        'co2':                 'fa-solid fa-cloud',
        'co':                  'fa-solid fa-cloud',

        'leaksensor':          'fa-solid fa-droplet',
        'flood':               'fa-solid fa-droplet',

        'curtain':             'fa-solid fa-table-columns',

        'presence':            'fa-solid fa-circle-dot',
        'pir':                 'fa-solid fa-person-running',

        'text':                'fa-solid fa-align-left',
        'alert':               'fa-solid fa-circle-exclamation',
        'clock':               'fa-solid fa-clock',
        'mode':                'fa-solid fa-sliders',
        'doorbell':            'fa-solid fa-bell',
        'adjust':              'fa-solid fa-sliders',
        'custom':              'fa-solid fa-gear',

        'scene':               'fa-solid fa-layer-group',
        'group':               'fa-solid fa-layer-group'
    };

    var TYPE_ALIASES = {
        'hardware':      'gauge',
        'hum':           'humidity',
        'temphum':       'temp',
        'temphumbaroew': 'temp',
        'zwavemelding':  'alarm',
        'elec':          'electricityusage',

        'lightbulb':     'light',
        'temperature':   'temp',
        'temp + rain':   'temp',
        'setpoint':      'temp',
        'bbq':           'temp',
        'evohome':       'heating',
        'weather':       'sun',
        'general':       'gauge',
        'utility':       'gauge',
        'siren':         'alarm',
        'pushoff':       'push',
        'override_mini': 'adjust'
    };

    var SAFE_CLASS_RE = /^[A-Za-z0-9 _-]+$/;

    var CHROME_ICONS_BY_NAME = {};
    Object.keys(CHROME_ICONS).forEach(function (key) {
        CHROME_ICONS_BY_NAME[key.substring(key.lastIndexOf('/') + 1)] = CHROME_ICONS[key];
    });

    app.factory('dzIconService', ['domoticzApi', 'dzDefaultSwitchIcons', function (domoticzApi, dzDefaultSwitchIcons) {

        var builtinFaClasses = null;
        var builtinRequest = null;

        return {
            resolve: resolve,
            resolveIconClass: resolveIconClass,
            chromeIconFor: chromeIconFor,
            typeIconFor: typeIconFor,
            preloadBuiltinIcons: loadBuiltinIcons
        };

        function resolve(device, isActive) {
            if (!device) {
                return { kind: 'img', src: 'images/unknown.png' };
            }

            var active = isDeviceActive(device, isActive);

            var iconClass = resolveIconClass(device.Icon, active);
            if (iconClass) {
                return { kind: 'font', cls: iconClass };
            }

            var customImage = parseInt(device.CustomImage, 10);
            if (customImage > 0) {
                var builtinClass = customImage < 100 ? builtinFaClassFor(customImage) : null;
                if (builtinClass) {
                    return { kind: 'font', cls: builtinClass };
                }

                return { kind: 'img', src: legacyImageFor(device, active) };
            }

            var typeClass = typeIconFor(device.TypeImg);
            if (typeClass) {
                return { kind: 'font', cls: typeClass };
            }

            return { kind: 'img', src: legacyImageFor(device, active) };
        }

        function resolveIconClass(icon, isActive) {
            if (!icon) {
                return null;
            }

            var parsed = icon;
            if (typeof icon === 'string') {
                try {
                    parsed = JSON.parse(icon);
                } catch (e) {
                    return null;
                }
            }
            if (!parsed || typeof parsed !== 'object') {
                return null;
            }

            var cls = isActive ? parsed.on : (parsed.off || parsed.on);
            if (typeof cls !== 'string') {
                return null;
            }
            cls = cls.replace(/\s+/g, ' ').trim();

            return (cls && SAFE_CLASS_RE.test(cls)) ? cls : null;
        }

        function chromeIconFor(src) {
            if (!src || typeof src !== 'string') {
                return null;
            }

            var path = src.split('?')[0].split('#')[0];
            if (CHROME_ICONS[path]) {
                return CHROME_ICONS[path];
            }

            return CHROME_ICONS_BY_NAME[path.substring(path.lastIndexOf('/') + 1)] || null;
        }

        function typeIconFor(typeImg) {
            if (!typeImg || typeof typeImg !== 'string') {
                return null;
            }

            var key = typeImg.toLowerCase();
            return TYPE_ICONS[key] || TYPE_ICONS[TYPE_ALIASES[key]] || null;
        }

        function loadBuiltinIcons() {
            if (builtinRequest) {
                return builtinRequest;
            }

            builtinRequest = domoticzApi.sendCommand('custom_light_icons', {})
                .then(function (data) {
                    var classes = {};
                    (data.result || []).forEach(function (item) {
                        if (item && item.FaClass) {
                            classes[item.idx] = item.FaClass;
                        }
                    });
                    builtinFaClasses = classes;
                    return classes;
                })
                .catch(function () {
                    builtinFaClasses = {};
                    return builtinFaClasses;
                });

            return builtinRequest;
        }

        function builtinFaClassFor(customImage) {
            if (builtinFaClasses === null) {
                loadBuiltinIcons();
                return null;
            }

            return builtinFaClasses[customImage] || null;
        }

        function isConfigurable(device) {
            return ['Light/Switch', 'Lighting 1', 'Lighting 2', 'Lighting 5', 'Lighting 6', 'Color Switch', 'Home Confort', 'Thermostat 3'].indexOf(device.Type) !== -1 &&
                [0, 2, 7, 9, 10, 11, 17, 18, 19, 20].indexOf(device.SwitchTypeVal) !== -1;
        }

        function isDeviceActive(device, isActive) {
            if (isActive !== undefined && isActive !== null) {
                return !!isActive;
            }
            if (typeof device.isActive === 'function') {
                return !!device.isActive();
            }

            var status = device.Status;
            return !!status && (
                ['On', 'Chime', 'Group On', 'Panic', 'Mixed'].indexOf(status) !== -1
                || status.indexOf('Set ') === 0
                || status.indexOf('NightMode') === 0
                || status.indexOf('Disco ') === 0);
        }

        function legacyImageFor(device, isActive) {
            var image;
            var typeImg = device.TypeImg || '';

            if (isConfigurable(device)) {
                if (device.CustomImage === 0) {
                    var defaults = dzDefaultSwitchIcons[device.SwitchTypeVal] || dzDefaultSwitchIcons[0];
                    image = defaults[isActive ? 0 : 1];
                } else {
                    image = device.Image + '48_' + (isActive ? 'On' : 'Off') + '.png';
                }
            } else if (typeImg.indexOf('Alert') === 0) {
                image = 'Alert48_' + Math.min(device.Level, 4) + '.png';
            } else if (typeImg.indexOf('motion') === 0) {
                image = isActive ? 'motion.png' : 'motionoff.png';
            } else if (typeImg.indexOf('smoke') === 0) {
                image = isActive ? 'smoke.png' : 'smokeoff.png';
            } else if (device.Type === 'Scene' || device.Type === 'Group') {
                image = isActive ? 'push.png' : 'pushoff.png';
            } else if (device.CustomImage == 0) {
                image = typeImg + '.png';
            } else {
                image = device.Image + '48_On.png';
            }

            return 'images/' + image;
        }
    }]);
});
