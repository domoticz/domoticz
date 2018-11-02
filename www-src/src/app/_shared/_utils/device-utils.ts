import {defaultSwitchIcons} from '../_constants/default-switch-icons';
import {Utils} from './utils';
import {LabelValue} from '../_services/device-timer-options.service';
import {Device} from '../_models/device';

export class DeviceUtils {

  static icon(t: Device): { isConfigurable: () => boolean, getIcon: () => string } {
    const o: any = {};

    o.isConfigurable = () => {
      return ['Light/Switch', 'Lighting 1', 'Lighting 2', 'Lighting 5', 'Lighting 6', 'Color Switch', 'Home Confort'].includes(t.Type) &&
        [0, 2, 7, 9, 10, 11, 17, 18, 19, 20].includes(t.SwitchTypeVal);
    };

    o.getIcon = () => {
      let image;
      const TypeImg = t.TypeImg;

      if (o.isConfigurable()) {
        image = t.CustomImage === 0
          ? defaultSwitchIcons[t.SwitchTypeVal][DeviceUtils.isActive(t) ? 0 : 1]
          : t.Image + '48_' + (DeviceUtils.isActive(t) ? 'On' : 'Off') + '.png';
      } else if (TypeImg.indexOf('Alert') === 0) {
        image = 'Alert48_' + Math.min(t.Level, 4) + '.png';
      } else if (TypeImg.indexOf('motion') === 0) {
        image = DeviceUtils.isActive(t) ? 'motion.png' : 'motionoff.png';
      } else if (TypeImg.indexOf('smoke') === 0) {
        image = DeviceUtils.isActive(t) ? 'smoke.png' : 'smokeoff.png';
      } else if (t.Type === 'Scene' || t.Type === 'Group') {
        image = DeviceUtils.isActive(t) ? 'push.png' : 'pushoff.png';
      } else {
        image = t.TypeImg + '.png';
      }

      return 'images/' + image;
    };

    return o;
  }

  static isDimmer(t: Device): boolean {
    return ['Dimmer', 'Blinds Percentage', 'Blinds Percentage Inverted', 'TPI'].includes(t.SwitchType);
  }

  static isSelector(t: Device): boolean {
    return t.SwitchType === 'Selector';
  }

  static isLED(t: Device): boolean {
    return Utils.isLED(t.SubType);
  }

  static isScene(d: Device): boolean {
    return ['Group', 'Scene'].includes(d.Type);
  }

  static isActive(t: Device): boolean {
    return t.Status && (
      ['On', 'Chime', 'Group On', 'Panic', 'Mixed'].includes(t.Status)
      || t.Status.indexOf('Set ') === 0
      || t.Status.indexOf('NightMode') === 0
      || t.Status.indexOf('Disco ') === 0);
  }

  static getLevels(t: Device): Array<string> {
    return t.LevelNames ? Utils.b64DecodeUnicode(t.LevelNames).split('|') : [];
  }

  static getLevelActions(t: Device) {
    return t.LevelActions ? Utils.b64DecodeUnicode(t.LevelActions).split('|') : [];
  }

  static getSelectorLevelOptions(t: Device): Array<LabelValue> {
    return DeviceUtils.getLevels(t)
      .slice(1)
      .map((levelName, index) => {
        return {
          label: levelName,
          value: (index + 1) * 10
        };
      });
  }

  static getDimmerLevelOptions(_step: number): Array<LabelValue> {
    const options: Array<LabelValue> = [];
    const step = _step || 5;

    for (let i = step; i <= 100; i += step) {
      options.push({
        label: i + '%',
        value: i
      });
    }

    return options;
  }

  static getUnit(t: Device): string {
    if (t.SubType === 'Custom Sensor') {
      return t.SensorUnit;
    } else if (t.Type === 'General' && t.SubType === 'Voltage') {
      return 'V';
    } else if (t.Type === 'General' && t.SubType === 'Distance') {
      return t.SwitchTypeVal === 1 ? 'in' : 'cm';
    } else if (t.Type === 'General' && t.SubType === 'Current') {
      return 'A';
    } else if (t.Type === 'General' && t.SubType === 'Pressure') {
      return 'Bar';
    } else if (t.Type === 'General' && t.SubType === 'Sound Level') {
      return 'dB';
    } else if (t.Type === 'General' && t.SubType === 'kWh') {
      return 'kWh';
    } else if (t.Type === 'General' && t.SubType === 'Managed Counter') {
      return 'kWh';
    } else if (t.Type === 'General' && t.SubType === 'Counter Incremental') {
      return '';
    } else if (t.Type === 'P1 Smart Meter' && t.SubType === 'Energy') {
      return 'kWh';
    } else if (t.Type === 'YouLess Meter') {
      return 'kWh';
    } else if (t.Type === 'RFXMeter' && t.SwitchTypeVal === 2) {
      return 'm3';
    } else if (t.Type === 'Usage' && t.SubType === 'Electric') {
      return 'W';
    } else if (t.SubType === 'Gas' || t.SubType === 'Water') {
      return 'm3';
    } else if (t.SubType === 'Visibility') {
      return t.SwitchTypeVal === 1 ? 'mi' : 'km';
    } else if (t.SubType === 'Solar Radiation') {
      return 'Watt/m2';
    } else if (t.SubType === 'Soil Moisture') {
      return 'cb';
    } else if (t.SubType === 'Leaf Wetness') {
      return 'Range';
    } else if (t.SubType === 'Weight') {
      return 'kg';
    } else if (['Voltage', 'A/D'].includes(t.SubType)) {
      return 'mV';
    } else if (t.SubType === 'Waterflow') {
      return 'l/min';
    } else if (t.SubType === 'Lux') {
      return 'lx';
    } else if (t.SubType === 'Percentage') {
      return '%';
    } else if (t.Type === 'Weight') {
      return t.SwitchTypeVal === 0 ? 'kg' : 'lbs';
    } else {
      return '?';
    }
  }

  static getLogLink(t: Device): string[] {
    const deviceType = t.Type;
    const logLink = ['/Devices', t.idx, '/Log'];

    const deviceTypes = ['Light', 'Color Switch', 'Chime', 'Security', 'RFY', 'ASA', 'Usage', 'Energy'];
    const deviceSubTypes = [
      'Voltage', 'Current', 'Pressure', 'Custom Sensor', 'kWh',
      'Sound Level', 'Solar Radiation', 'Visibility', 'Distance',
      'Soil Moisture', 'Leaf Wetness', 'Waterflow', 'Lux', 'Percentage',
      'Text', 'Alert'
    ];

    if (deviceTypes.some((item) => {
      return deviceType.indexOf(item) === 0;
    })) {
      return logLink;
    }

    if (/Temp|Thermostat|Humidity/i.test(deviceType)) {
      return logLink;
    }

    if (deviceSubTypes.includes(t.SubType)) {
      return logLink;
    }

    if (t.Counter !== undefined) {
      return logLink;
    }
  }

}
