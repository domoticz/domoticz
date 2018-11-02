import {Component, EventEmitter, Input, OnInit, Output} from '@angular/core';
import {DeviceLightService} from '../../_shared/_services/device-light.service';
import {Utils} from 'src/app/_shared/_utils/utils';
import {Device} from '../../_shared/_models/device';

@Component({
  selector: 'dz-device-color-settings',
  templateUrl: './device-color-settings.component.html',
  styleUrls: ['./device-color-settings.component.css']
})
export class DeviceColorSettingsComponent implements OnInit {

  @Input() device: Device;
  @Output() deviceChange: EventEmitter<Device> = new EventEmitter<Device>();

  constructor(
    private deviceLightService: DeviceLightService
  ) {
  }

  ngOnInit() {
  }

  onDeviceChange() {
    this.deviceLightService.setColor(this.device.idx, this.device.Color, this.device.LevelInt).subscribe(() => {
      this.deviceChange.emit(this.device);
    });
  }

  // returns true if the light does not support absolute dimming, used to control display of Brightness Up / Brightness Down buttons
  public isRelativeDimmer(): boolean {
    return this.device.DimmerType && this.device.DimmerType === 'rel';
  }

  // returns true if the light does not support absolute color temperature, used to control display of Warmer White / Colder White buttons
  public isRelativeColorTemperature(): boolean {
    const ledType = Utils.getLEDType(this.device.SubType);

    return this.device.DimmerType && this.device.DimmerType === 'rel' && ledType.bHasTemperature;
  }

  brightnessUp() {
    this.deviceLightService.brightnessUp(this.device.idx).subscribe(() => {
      // Nothing
    });
  }

  brightnessDown() {
    this.deviceLightService.brightnessDown(this.device.idx).subscribe(() => {
      // Nothing
    });
  }

  nightLight() {
    this.deviceLightService.nightLight(this.device.idx).subscribe(() => {
      // Nothing
    });
  }

  fullLight() {
    this.deviceLightService.fullLight(this.device.idx).subscribe(() => {
      // Nothing
    });
  }

  switchOn() {
    this.deviceLightService.switchOn(this.device.idx).subscribe(() => {
      // Nothing
    });
  }

  switchOff() {
    this.deviceLightService.switchOff(this.device.idx).subscribe(() => {
      // Nothing
    });
  }

  colorWarmer() {
    this.deviceLightService.colorWarmer(this.device.idx).subscribe(() => {
      // Nothing
    });
  }

  colorColder() {
    this.deviceLightService.colorColder(this.device.idx).subscribe(() => {
      // Nothing
    });
  }

}
