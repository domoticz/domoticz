import {Component, Inject, OnInit} from '@angular/core';
import {ActivatedRoute} from '@angular/router';
import {DeviceService} from 'src/app/_shared/_services/device.service';
import {DeviceTimerConfigUtilsService, TimerConfig} from 'src/app/_shared/_services/device-timer-config-utils.service';
import {AbstractTimersService} from '../../_shared/_services/timers.service';
import {Timer} from 'src/app/_shared/_models/timers';
import {DeviceTimerOptionsService, LabelValue} from '../../_shared/_services/device-timer-options.service';
import {DeviceLightService} from 'src/app/_shared/_services/device-light.service';
import {NotificationService} from '../../_shared/_services/notification.service';
import {I18NEXT_SERVICE, ITranslationService} from 'angular-i18next';
import {DeviceUtils} from '../../_shared/_utils/device-utils';
import {Device} from '../../_shared/_models/device';
import {RegularTimersService} from '../regular-timers.service';
import {SetpointTimersService} from '../setpoint-timers.service';

// FIXME proper declaration
declare var bootbox: any;

@Component({
  selector: 'dz-device-timers',
  templateUrl: './device-timers.component.html',
  styleUrls: ['./device-timers.component.css']
})
export class DeviceTimersComponent implements OnInit {

  deviceIdx: string;
  device: Device;

  selectedTimerIdx: string = null;

  isLoaded = false;
  _timerSettings: TimerConfig;
  isLED: boolean;
  levelOptions: Array<LabelValue>;
  isSelector: boolean;
  isDimmer: boolean;
  isSetpointTimers: boolean;
  deviceTimers: AbstractTimersService;
  timers: Array<Timer>;
  typeOptions: LabelValue[];
  isCommandSelectionDisabled: boolean;
  colorSettingsType: string;
  dimmerType: any;

  deleteConfirmationMessage: string;
  clearConfirmationMessage: string;

  constructor(
    private route: ActivatedRoute,
    private deviceService: DeviceService,
    private deviceTimerConfigUtils: DeviceTimerConfigUtilsService,
    private setpointTimersService: SetpointTimersService,
    private regularTimersService: RegularTimersService,
    private deviceTimerOptions: DeviceTimerOptionsService,
    private deviceLightService: DeviceLightService,
    private notificationService: NotificationService,
    @Inject(I18NEXT_SERVICE) private translationService: ITranslationService
  ) {
  }

  ngOnInit() {
    this.deviceIdx = this.route.snapshot.paramMap.get('idx');

    this._timerSettings = this.deviceTimerConfigUtils.getTimerDefaultConfig();
    this.typeOptions = this.deviceTimerOptions.timerTypes;

    this.deleteConfirmationMessage = this.translationService.t('Are you sure to delete this timers?\n\nThis action can not be undone...');
    this.clearConfirmationMessage = this.translationService.t('Are you sure to delete ALL timers?\n\nThis action can not be undone!');

    this.deviceService.getDeviceInfo(this.deviceIdx).subscribe(device => {
      this.device = device;

      this.isLoaded = true;
      // vm.itemName = device.Name;
      this.colorSettingsType = device.SubType;
      this.dimmerType = device.DimmerType;

      this.isDimmer = DeviceUtils.isDimmer(device);
      this.isSelector = DeviceUtils.isSelector(device);
      this.isLED = DeviceUtils.isLED(device);
      this.isCommandSelectionDisabled = this.isSelector && device.LevelOffHidden;
      this.isSetpointTimers = (device.Type === 'Thermostat' && device.SubType === 'SetPoint') || (device.Type === 'Radiator 1');

      this.levelOptions = [];

      this.deviceTimers = this.isSetpointTimers ? this.setpointTimersService : this.regularTimersService;

      if (this.isSelector) {
        this.levelOptions = DeviceUtils.getSelectorLevelOptions(device);
      }

      if (this.isDimmer) {
        this.levelOptions = DeviceUtils.getDimmerLevelOptions(1);
      }

      if (this.levelOptions.length > 0) {
        this.timerSettings.level = this.levelOptions[0].value;
      }

      this.refreshTimers();
    });
  }

  get timerSettings(): TimerConfig {
    return this._timerSettings;
  }

  set timerSettings(newTimerSettings: TimerConfig) {
    const oldTimerSettings = this._timerSettings;
    this._timerSettings = newTimerSettings;

    // $scope.$watch(function () {
    //   return vm.timerSettings.color + vm.timerSettings.level;
    // }, setDeviceColor);
    if (this.isLED) {
      if (oldTimerSettings.color + oldTimerSettings.level !== this._timerSettings.color + this._timerSettings.level) {
        this.setDeviceColor();
      }
    }
  }

  private refreshTimers() {
    this.selectedTimerIdx = null;

    this.deviceTimers.getTimers(this.deviceIdx).subscribe(items => {
      this.timers = items;
    });
  }

  private setDeviceColor() {
    if (!this.timerSettings.color || !this.timerSettings.level) {
      return;
    }

    this.deviceLightService.setColor(this.deviceIdx, this.timerSettings.color, this.timerSettings.level)
      .subscribe(() => {
        // Nothing
      });
  }

  public addTimer() {
    const config = this.getTimerConfig();

    if (!config) {
      return false;
    }

    this.deviceTimers.addTimer(this.deviceIdx, config).subscribe(() => {
      this.refreshTimers();
    }, error => {
      this.notificationService.HideNotify();
      this.notificationService.ShowNotify(this.translationService.t('Problem adding timer!'), 2500, true);
    });
  }

  public updateTimer(timerIdx: string) {
    const config = this.getTimerConfig();

    if (!config) {
      return false;
    }

    this.deviceTimers.updateTimer(timerIdx, config).subscribe(() => {
      this.refreshTimers();
    }, error => {
      this.notificationService.HideNotify();
      this.notificationService.ShowNotify(this.translationService.t('Problem updating timer!'), 2500, true);
    });
  }

  public deleteTimer(timerIdx: string) {
    bootbox.confirm(this.deleteConfirmationMessage, (result) => {
      if (result) {
        this.deviceTimers.deleteTimer(timerIdx).subscribe(() => {
          this.refreshTimers();
        }, error => {
          this.notificationService.HideNotify();
          this.notificationService.ShowNotify(this.translationService.t('Problem deleting timer!'), 2500, true);
        });
      }
    });
  }

  public clearTimers() {
    bootbox.confirm(this.clearConfirmationMessage, (result) => {
      if (result) {
        this.deviceTimers.clearTimers(this.deviceIdx).subscribe(() => {
          this.refreshTimers();
        }, error => {
          this.notificationService.HideNotify();
          this.notificationService.ShowNotify(this.translationService.t('Problem clearing timers!'), 2500, true);
        });
      }
    });
  }

  private getTimerConfig(): TimerConfig {
    const utils = this.deviceTimerConfigUtils;
    let config = Object.assign({}, this.timerSettings);

    if (!utils.isDayScheduleApplicable(config.timertype)) {
      config.days = 0;
    }

    if ([6, 7, 10, 12].includes(config.timertype)) {
      config.days = 0x80;
    }

    if (utils.isOccurrenceApplicable(config.timertype)) {
      config.days = Math.pow(2, config.weekday);
    }

    config = Object.assign({}, config, {
      date: utils.isDateApplicable(config.timertype) ? config.date : undefined,
      mday: utils.isRDaysApplicable(config.timertype) ? config.mday : undefined,
      month: utils.isRMonthsApplicable(config.timertype) ? config.month : undefined,
      occurence: utils.isOccurrenceApplicable(config.timertype) ? config.occurence : undefined,
      weekday: undefined,
      color: this.isLED ? config.color : undefined,
      tvalue: this.isSetpointTimers ? config.tvalue : undefined,
      command: this.isSetpointTimers ? undefined : config.command,
      randomness: this.isSetpointTimers ? undefined : config.randomness
    });

    const error = this.deviceTimerConfigUtils.getTimerConfigErrors(config);

    if (error) {
      this.notificationService.ShowNotify(error, 2500, true);
      return null;
    }

    const warning = this.deviceTimerConfigUtils.getTimerConfigWarnings(config);

    if (warning) {
      this.notificationService.ShowNotify(error, 2500, true);
    }

    return config;
  }

}
