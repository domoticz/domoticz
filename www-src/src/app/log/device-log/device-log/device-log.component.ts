import {Component, OnDestroy, OnInit} from '@angular/core';
import {ActivatedRoute} from '@angular/router';
import {DeviceService} from '../../../_shared/_services/device.service';
import {Device} from '../../../_shared/_models/device';

@Component({
  selector: 'dz-device-log',
  templateUrl: './device-log.component.html',
  styleUrls: ['./device-log.component.css']
})
export class DeviceLogComponent implements OnInit, OnDestroy {

  deviceIdx: string;
  device: Device;

  pageName: string;

  constructor(
    private route: ActivatedRoute,
    private deviceService: DeviceService) {
  }

  ngOnInit() {
    this.route.params.subscribe(params => {
      this.deviceIdx = params['idx'];

      this.deviceService.getDeviceInfo(this.deviceIdx).subscribe(device => {
        this.device = device;
        this.pageName = device.Name;
      });
    });
  }

  ngOnDestroy(): void {
  }

  isTextLog(): boolean {
    if (!this.device) {
      return undefined;
    }

    return ['Text', 'Alert'].includes(this.device.SubType) || this.device.SwitchType === 'Media Player';
  }

  isLightLog(): boolean {
    if (!this.device) {
      return undefined;
    }

    if (this.device.Type === 'Heating') {
      return ((this.device.SubType !== 'Zone') && (this.device.SubType !== 'Hot Water'));
    }

    const isLightType = [
      'Lighting 1', 'Lighting 2', 'Lighting 3', 'Lighting 4', 'Lighting 5',
      'Light', 'Light/Switch', 'Color Switch', 'Chime',
      'Security', 'RFY', 'ASA', 'Blinds'
    ].includes(this.device.Type);

    const isLightSwitchType = [
      'Contact', 'Door Contact', 'Doorbell', 'Dusk Sensor', 'Motion Sensor',
      'Smoke Detector', 'On/Off', 'Dimmer'
    ].includes(this.device.SwitchType);

    return (isLightType || isLightSwitchType) && !this.isTextLog();
  }

  isTemperatureLog(): boolean {
    if (!this.device) {
      return undefined;
    }

    if (this.device.Type === 'Heating') {
      return ((this.device.SubType === 'Zone') || (this.device.SubType === 'Hot Water'));
    }

    // This goes wrong (when we also use this log call from the weather tab), for wind sensors
    // as this is placed in weather and temperature, we might have to set a parameter in the url
    // for now, we assume it is a temperature
    return (/Temp|Thermostat|Humidity|Radiator|Wind/i).test(this.device.Type);
  }

  isGraphLog(): boolean {
    if (!this.device) {
      return undefined;
    }

    return this.device.Type === 'Usage' || this.device.Type === 'Weight' || [
      'Voltage', 'Current', 'Pressure', 'Custom Sensor',
      'Sound Level', 'Solar Radiation', 'Visibility', 'Distance',
      'Soil Moisture', 'Leaf Wetness', 'Waterflow', 'Lux', 'Percentage'
    ].includes(this.device.SubType);
  }

  isSmartLog(): boolean {
    if (!this.device) {
      return undefined;
    }
    return (this.device.Type === 'P1 Smart Meter' && this.device.SubType === 'Energy');
  }

  isCounterLog(): boolean {
    if (!this.device) {
      return undefined;
    }

    if (this.isCounterLogSpline() || this.isSmartLog()) {
      return false;
    }

    return this.device.Type === 'RFXMeter'
      || (this.device.Type === 'P1 Smart Meter' && this.device.SubType === 'Gas')
      || (typeof this.device.Counter !== 'undefined' && !this.isCounterLogSpline());
  }

  isCounterLogSpline(): boolean {
    if (!this.device) {
      return undefined;
    }

    return ['Power', 'Energy'].includes(this.device.Type)
      || ['kWh'].includes(this.device.SubType)
      || (this.device.Type === 'YouLess Meter' && [0, 4].includes(this.device.SwitchTypeVal));
  }

  isReportAvailable(): boolean {
    if (!this.device) {
      return undefined;
    }

    return this.isTemperatureLog()
      || ((this.isCounterLogSpline() || this.isCounterLog() || this.isSmartLog()) && [0, 1, 2, 3, 4].includes(this.device.SwitchTypeVal));
  }

}
