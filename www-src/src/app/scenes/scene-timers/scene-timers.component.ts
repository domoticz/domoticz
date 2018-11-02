import {Component, Inject, OnInit} from '@angular/core';
import {ActivatedRoute} from '@angular/router';
import {SceneService} from '../../_shared/_services/scene.service';
import {DeviceTimerOptionsService, LabelValue} from 'src/app/_shared/_services/device-timer-options.service';
import {DeviceTimerConfigUtilsService, TimerConfig} from 'src/app/_shared/_services/device-timer-config-utils.service';
import {Timer} from 'src/app/_shared/_models/timers';
import {NotificationService} from 'src/app/_shared/_services/notification.service';
import {I18NEXT_SERVICE, ITranslationService} from 'angular-i18next';
import {Device} from '../../_shared/_models/device';
import {SceneTimersService} from '../scene-timers.service';

// FIXME proper declaration
declare var bootbox: any;

@Component({
  selector: 'dz-scene-timers',
  templateUrl: './scene-timers.component.html',
  styleUrls: ['./scene-timers.component.css']
})
export class SceneTimersComponent implements OnInit {

  sceneIdx: string;
  scene: Device;

  selectedTimerIdx: any;

  isLoaded = false;

  colorSettingsType: string;
  isCommandSelectionDisabled: boolean;
  levelOptions: any[] = [];
  typeOptions: LabelValue[] = [];

  isSetpointTimers: boolean;
  timerSettings: TimerConfig;
  timers: Timer[];

  deleteConfirmationMessage: string;
  clearConfirmationMessage: string;

  constructor(
    private route: ActivatedRoute,
    private sceneService: SceneService,
    private deviceTimerOptions: DeviceTimerOptionsService,
    private deviceTimerConfigUtils: DeviceTimerConfigUtilsService,
    private sceneTimersService: SceneTimersService,
    private notificationService: NotificationService,
    @Inject(I18NEXT_SERVICE) private translationService: ITranslationService
  ) {
  }

  ngOnInit() {
    this.deleteConfirmationMessage = this.translationService.t('Are you sure to delete this timers?\n\nThis action can not be undone...');
    this.clearConfirmationMessage = this.translationService.t('Are you sure to delete ALL timers?\n\nThis action can not be undone!');

    this.sceneIdx = this.route.snapshot.paramMap.get('idx');
    this.selectedTimerIdx = null;

    this.sceneService.getScene(this.sceneIdx).subscribe((scene) => {
      this.scene = scene;

      this.isLoaded = true;
      // this.itemName = scene.Name;
      this.colorSettingsType = 'Scene';

      this.isCommandSelectionDisabled = scene.Type === 'Scene';
      this.isSetpointTimers = false;

      this.levelOptions = [];
      this.refreshTimers();
    });

    this.typeOptions = this.deviceTimerOptions.timerTypes;
    this.timerSettings = this.deviceTimerConfigUtils.getTimerDefaultConfig();
  }

  private refreshTimers() {
    this.selectedTimerIdx = null;

    this.sceneTimersService.getTimers(this.sceneIdx).subscribe((items) => {
      this.timers = items;
    });
  }


  private getTimerConfig(): TimerConfig | boolean {
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
      color: undefined,
      tvalue: undefined,
    });

    const error = this.deviceTimerConfigUtils.getTimerConfigErrors(config);

    if (error) {
      this.notificationService.ShowNotify(error, 2500, true);
      return false;
    }

    const warning = this.deviceTimerConfigUtils.getTimerConfigWarnings(config);

    if (warning) {
      this.notificationService.ShowNotify(error, 2500, true);
    }

    return config;
  }

  addTimer() {
    const config = this.getTimerConfig();

    if (!config) {
      return false;
    }

    this.sceneTimersService
      .addTimer(this.sceneIdx, config)
      .subscribe(() => {
        this.refreshTimers();
      }, error => {
        this.notificationService.HideNotify();
        this.notificationService.ShowNotify(this.translationService.t('Problem adding timer!'), 2500, true);
      });
  }

  updateTimer(timerIdx: string) {
    const config = this.getTimerConfig();

    if (!config) {
      return false;
    }

    this.sceneTimersService
      .updateTimer(timerIdx, config)
      .subscribe(() => {
        this.refreshTimers();
      }, error => {
        this.notificationService.HideNotify();
        this.notificationService.ShowNotify(this.translationService.t('Problem updating timer!'), 2500, true);
      });
  }


  deleteTimer(timerIdx: string) {
    bootbox.confirm(this.deleteConfirmationMessage, (result) => {
      if (result) {
        return this.sceneTimersService
          .deleteTimer(timerIdx)
          .subscribe(() => {
            this.refreshTimers();
          }, error => {
            this.notificationService.HideNotify();
            this.notificationService.ShowNotify(this.translationService.t('Problem deleting timer!'), 2500, true);
          });
      }
    });
  }

  clearTimers() {
    bootbox.confirm(this.clearConfirmationMessage, (result) => {
      if (result) {
        return this.sceneTimersService
          .clearTimers(this.sceneIdx)
          .subscribe(() => {
            this.refreshTimers();
          }, error => {
            this.notificationService.HideNotify();
            this.notificationService.ShowNotify(this.translationService.t('Problem clearing timers!'), 2500, true);
          });
      }
    });
  }

}
