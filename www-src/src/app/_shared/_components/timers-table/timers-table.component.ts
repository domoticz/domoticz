import { Component, OnInit, Input, Inject, AfterViewInit, ElementRef, ViewChild, OnChanges, SimpleChanges, Output, EventEmitter } from '@angular/core';
import { TimerConfig } from 'src/app/_shared/_services/device-timer-config-utils.service';
import { Timer } from 'src/app/_shared/_models/timers';
import { I18NEXT_SERVICE, ITranslationEvents, ITranslationService } from 'angular-i18next';
import { ConfigService } from '../../_services/config.service';
import { DeviceTimerOptionsService } from 'src/app/_shared/_services/device-timer-options.service';
import { LabelValue } from '../../_services/device-timer-options.service';

// FIXME use proper ts def
declare var $: any;

@Component({
  selector: 'dz-timers-table',
  templateUrl: './timers-table.component.html',
  styleUrls: ['./timers-table.component.css']
})
export class TimersTableComponent implements OnInit, AfterViewInit, OnChanges {

  @Input() timerSettings: TimerConfig;
  @Output() timerSettingsChange: EventEmitter<TimerConfig> = new EventEmitter<TimerConfig>();

  @Input() selectedTimerIdx: string;
  @Output() selectedTimerIdxChange: EventEmitter<string> = new EventEmitter<string>();

  @Input() timers: Array<Timer>;
  @Input() levelOptions: Array<LabelValue>;
  @Input() isSetpointTimers: boolean;

  @ViewChild('table', { static: false }) tableRef: ElementRef;

  private table: any;

  constructor(
    @Inject(I18NEXT_SERVICE) private translationService: ITranslationService,
    private configService: ConfigService,
    private deviceTimerOptions: DeviceTimerOptionsService
  ) { }

  ngOnInit() {
  }

  ngAfterViewInit() {
    const columns = [
      { title: this.translationService.t('Active'), data: 'Active', render: (val) => this.activeRenderer(val) },
      { title: this.translationService.t('Type'), data: 'Type', render: (val) => this.timerTypeRenderer(val) },
      { title: this.translationService.t('Date'), data: 'Date', type: 'date-us' },
      { title: this.translationService.t('Time'), data: 'Time' },
      { title: this.translationService.t('Randomness'), data: 'Randomness', render: (val) => this.activeRenderer(val) },
      { title: this.translationService.t('Command'), data: 'idx', render: (val) => this.commandRenderer(val) },
      { title: this.translationService.t('Days'), data: 'idx', render: (val) => this.daysRenderer(val) },
    ];

    const tableOptions = Object.assign({}, this.configService.dataTableDefaultSettings, { columns: columns });
    this.table = $(this.tableRef.nativeElement).dataTable(tableOptions);

    this.table.api().column('4').visible(this.isSetpointTimers === false);

    const _this = this;

    this.table.on('select.dt', (e, dt, type, indexes) => {
      const timer = dt.rows(indexes).data()[0];

      _this.selectedTimerIdx = timer.idx;
      _this.timerSettings = Object.assign({}, _this.timerSettings);
      _this.timerSettings.active = timer.Active === 'true';
      _this.timerSettings.randomness = timer.Randomness === 'true';
      _this.timerSettings.timertype = timer.Type;
      _this.timerSettings.hour = Number(timer.Time.substring(0, 2));
      _this.timerSettings.min = Number(timer.Time.substring(3, 5));
      _this.timerSettings.command = timer.Cmd;
      _this.timerSettings.color = timer.Color;
      _this.timerSettings.level = timer.Level;
      _this.timerSettings.tvalue = timer.Temperature;
      _this.timerSettings.date = timer.Date.replace(/-/g, '/');
      _this.timerSettings.days = timer.Days;
      _this.timerSettings.mday = timer.MDay;
      _this.timerSettings.weekday = Math.log(Number(timer.Days)) / Math.log(2);
      _this.timerSettings.occurence = timer.Occurence;

      // $scope.$apply();
      _this.selectedTimerIdxChange.emit(_this.selectedTimerIdx);
      _this.timerSettingsChange.emit(_this.timerSettings);
    });

    this.table.on('deselect.dt', () => {
      _this.selectedTimerIdx = null;
      // $scope.$apply();
      _this.selectedTimerIdxChange.emit(_this.selectedTimerIdx);
    });
  }

  ngOnChanges(changes: SimpleChanges) {
    if (!this.table) {
      return;
    }

    if (changes.timers) {
      this.table.api().clear();
      this.table.api().rows
        .add(this.timers)
        .draw();
    }
  }

  private getTimerByIdx(idx: string): Timer {
    return this.timers.find((timer: Timer) => {
      return timer.idx === idx;
    });
  }

  activeRenderer(value: string): string {
    return value === 'true' ? this.translationService.t('Yes') : this.translationService.t('No');
  }

  timerTypeRenderer(value): string {
    return this.deviceTimerOptions.timerTypes[value].label;
  }

  commandRenderer(value: string): string {
    const timer = this.getTimerByIdx(value);
    const command = timer.Cmd === 1 ? 'Off' : 'On';

    if (this.isSetpointTimers) {
      return this.translationService.t('Temperature') + ', ' + timer.Temperature;
    } else if (command === 'On' && this.levelOptions.length > 0) {
      const levelName = this.deviceTimerOptions.getLabelForValue(this.levelOptions, timer.Level);
      return this.translationService.t(command) + ' (' + levelName + ')';
    } else if (timer.Color) {
      return this.getTimerColorBox(timer) + this.translationService.t(command) + ' (' + timer.Level + '%)';
    }

    return this.translationService.t(command);
  }

  daysRenderer(value: string) {
    const timer = this.getTimerByIdx(value);
    const separator = timer.Type > 5 ? ' ' : ', ';
    const DayStr = this.getDayStr(timer);

    return DayStr
      .split(separator)
      .map((item) => {
        return this.translationService.t(item);
      })
      .join(separator);
  }

  // tslint:disable:no-bitwise
  private getDayStr(timer: Timer) {
    let DayStrOrig = '';

    if ((timer.Type <= 4) || (timer.Type === 8) || (timer.Type === 9) || ((timer.Type >= 14) && (timer.Type <= 27))) {
      const dayflags = Number(timer.Days);

      if (dayflags & 0x80) {
        DayStrOrig = 'Everyday';
      } else if (dayflags & 0x100) {
        DayStrOrig = 'Weekdays';
      } else if (dayflags & 0x200) {
        DayStrOrig = 'Weekends';
      } else {
        const days = [];

        if (dayflags & 0x01) {
          days.push('Mon');
        }
        if (dayflags & 0x02) {
          days.push('Tue');
        }
        if (dayflags & 0x04) {
          days.push('Wed');
        }
        if (dayflags & 0x08) {
          days.push('Thu');
        }
        if (dayflags & 0x10) {
          days.push('Fri');
        }
        if (dayflags & 0x20) {
          days.push('Sat');
        }
        if (dayflags & 0x40) {
          days.push('Sun');
        }

        DayStrOrig = days.join(', ');
      }
    } else if (timer.Type === 10) {
      DayStrOrig = 'Monthly on Day ' + timer.MDay;
    } else if (timer.Type === 11) {
      const Weekday = Math.log(Number(timer.Days)) / Math.log(2);

      DayStrOrig = [
        'Monthly on',
        this.deviceTimerOptions.occurence[timer.Occurence - 1].label,
        this.deviceTimerOptions.weekday[Weekday].label
      ].join(' ');
    } else if (timer.Type === 12) {
      DayStrOrig = [
        'Yearly on',
        timer.MDay,
        this.deviceTimerOptions.month[timer.Month - 1].label
      ].join(' ');
    } else if (timer.Type === 13) {
      const Weekday = Math.log(Number(timer.Days)) / Math.log(2);

      DayStrOrig = [
        'Yearly on',
        this.deviceTimerOptions.occurence[timer.Occurence - 1].label,
        this.deviceTimerOptions.weekday[Weekday].label,
        'in',
        this.deviceTimerOptions.month[timer.Month - 1].label
      ].join(' ');
    }

    return DayStrOrig;
  }

  private getTimerColorBox(timer: Timer) {
    if (!timer.Color) {
      return '';
    }

    const color = JSON.parse(timer.Color);
    let backgroundColor;

    if (color.m === 1 || color.m === 2) { // White or color temperature
      let whex = Math.round(255 * timer.Level / 100).toString(16);
      if (whex.length === 1) {
        whex = '0' + whex;
      }

      backgroundColor = '#' + whex + whex + whex;
    }

    if (color.m === 3 || color.m === 4) { // RGB or custom
      backgroundColor = 'rgb(' + [color.r, color.g, color.b].join(', ') + ')';
    }

    return '<div class="ex-color-box" style="background-color: ' + backgroundColor + ';"></div>';
  }

}
