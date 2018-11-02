import {Component, OnInit, Inject} from '@angular/core';
import {ActivatedRoute, Router} from '@angular/router';
import {DeviceService} from '../../_shared/_services/device.service';
import {Utils} from 'src/app/_shared/_utils/utils';
import {DeviceUtils} from 'src/app/_shared/_utils/device-utils';
import {ITranslationService, I18NEXT_SERVICE} from 'angular-i18next';
import {ConfigService} from '../../_shared/_services/config.service';
import {ReplaceDeviceService} from "../../_shared/_services/replace-device.service";
import {Device} from "../../_shared/_models/device";

// FIXME proper declaration
declare var bootbox: any;

@Component({
  selector: 'dz-light-edit',
  templateUrl: './light-edit.component.html',
  styleUrls: ['./light-edit.component.css']
})
export class LightEditComponent implements OnInit {

  deviceIdx: string;

  device: Device;
  switchTypeOptions: Array<{ label: string; value: number; }>;
  levels: Array<{ level: number; name: string; action: string; }> = [];

  constructor(
    private route: ActivatedRoute,
    private router: Router,
    private deviceService: DeviceService,
    @Inject(I18NEXT_SERVICE) private translationService: ITranslationService,
    private configService: ConfigService,
    private replaceDeviceService: ReplaceDeviceService
  ) {
  }

  ngOnInit() {
    this.deviceIdx = this.route.snapshot.paramMap.get('idx');
    this.switchTypeOptions = this.configService.switchTypeOptions;

    this.deviceService.getDeviceInfo(this.deviceIdx).subscribe((device) => {
      this.device = device;
      this.device.StrParam1 = Utils.b64DecodeUnicode(this.device.StrParam1);
      this.device.StrParam2 = Utils.b64DecodeUnicode(this.device.StrParam2);

      const defaultLevelNames = ['Off', 'Level1', 'Level2', 'Level3'];

      const levelNames = DeviceUtils.getLevels(device) || defaultLevelNames;
      const levelActions = DeviceUtils.getLevelActions(device);

      this.levels = levelNames.map((level, index) => {
        return {
          level: index,
          name: level,
          action: levelActions[index] ? levelActions[index] : ''
        };
      });
    });
  }

  removeDevice() {
    bootbox.confirm(this.translationService.t('Are you sure to remove this Light/Switch?'), (result) => {
      if (!result) {
        return;
      }

      this.deviceService.removeDeviceAndSubDevices(this.deviceIdx).subscribe(() => {
        this.router.navigate(['/LightSwitches']);
      });
    });
  }

  updateDevice() {
    const options = [];

    if (this.isLevelsAvailable()) {
      const levelParams: { names: string[]; actions: string[] } = this.levels.reduce((acc, level) => {
        return {
          names: acc.names.concat(level.name),
          actions: acc.actions.concat(level.action)
        };
      }, {names: [], actions: []});

      options.push('LevelNames:' + levelParams.names.join('|'));
      options.push('LevelActions:' + levelParams.actions.join('|'));
      options.push('SelectorStyle:' + this.device.SelectorStyle);
      options.push('LevelOffHidden:' + this.device.LevelOffHidden);
    }
    const params = {
      name: this.device.Name,
      description: this.device.Description,
      strparam1: Utils.b64EncodeUnicode(this.device.StrParam1),
      strparam2: Utils.b64EncodeUnicode(this.device.StrParam2),
      protected: this.device.Protected.toString(),
      options: Utils.b64EncodeUnicode(options.join(';')),
      addjvalue: this.device.AddjValue.toString(),
      addjvalue2: this.device.AddjValue2.toString(),
      customimage: this.device.CustomImage.toString(),
      switchtype: this.device.SwitchTypeVal.toString(),
      used: 'true'
    };

    this.deviceService.updateDeviceInfo(this.deviceIdx, params).subscribe(() => {
      this.router.navigate(['/LightSwitches']);
    });
  }

  replaceDevice() {
    this.replaceDeviceService.ReplaceDevice(this.deviceIdx, undefined);
  }

  isSecurityDevice(): boolean {
    return this.device.Type === 'Security';
  }

  isMotionAvailable(): boolean {
    return this.device.SwitchTypeVal === 8;
  }

  isOnActionAvailable(): boolean {
    return this.device.SwitchTypeVal !== 18;
  }

  isOffActionAvailable(): boolean {
    return this.isOnActionAvailable();
  }

  isOnDelayAvailable(): boolean {
    return [0, 7, 9, 11, 18].includes(this.device.SwitchTypeVal);
  }

  isOffDelayAvailable(): boolean {
    return [0, 7, 9, 11, 18, 19, 20].includes(this.device.SwitchTypeVal);
  }

  isSwitchIconAvailable(): boolean {
    return DeviceUtils.icon(this.device).isConfigurable();
  }

  isLevelsAvailable(): boolean {
    return this.device.SwitchTypeVal === 18;
  }

  isColorSettingsAvailable(): boolean {
    return DeviceUtils.isLED(this.device);
  }

  isWhiteSettingsAvailable(): boolean {
    return this.device.SubType === 'White';
  }

}

