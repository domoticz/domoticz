import { Component, OnInit, EventEmitter, Input, Output, AfterViewInit, OnChanges, SimpleChanges } from '@angular/core';
import { TimerConfig, DeviceTimerConfigUtilsService } from '../../_services/device-timer-config-utils.service';
import { LabelValue } from 'src/app/_shared/_services/device-timer-options.service';
import { DeviceTimerOptionsService } from '../../_services/device-timer-options.service';
import { Utils } from 'src/app/_shared/_utils/utils';
import { ConfigService } from '../../_services/config.service';

// FIXME proper declaration
declare var $: any;

@Component({
  selector: 'dz-timer-form',
  templateUrl: './timer-form.component.html',
  styleUrls: ['./timer-form.component.css']
})
export class TimerFormComponent implements OnInit, OnChanges, AfterViewInit {

  @Input() timerSettings: TimerConfig;
  @Output() timerSettingsChange: EventEmitter<TimerConfig> = new EventEmitter<TimerConfig>();

  @Input() levelOptions: Array<LabelValue>;
  @Input() isCommandSelectionDisabled: boolean;
  @Input() colorSettingsType: string;
  @Input() isSetpointTimers: boolean;
  @Input() dimmerType: any;

  typeOptions: LabelValue[];
  commandOptions: LabelValue[];
  monthOptions: LabelValue[];
  dayOptions: LabelValue[];
  weekdayOptions: LabelValue[];
  hourOptions: LabelValue[];
  minuteOptions: LabelValue[];
  occurenceOptions: LabelValue[];
  isColorSettingsAvailable: any;

  // TODO remove if not used, put in configService otherwise?
  // $.myglobals = {
  //   TimerTypesStr: [],
  //   OccurenceStr: [],
  //   MonthStr: [],
  //   WeekdayStr: [],
  //   SelectedTimerIdx: 0
  // };
  // $('#timerparamstable #combotype > option').each(function () {
  //   $.myglobals.TimerTypesStr.push($(this).text());
  // });
  // $('#timerparamstable #occurence > option').each(function () {
  //   $.myglobals.OccurenceStr.push($(this).text());
  // });
  // $('#timerparamstable #months > option').each(function () {
  //   $.myglobals.MonthStr.push($(this).text());
  // });
  // $('#timerparamstable #weekdays > option').each(function () {
  //   $.myglobals.WeekdayStr.push($(this).text());
  // });

  // @ViewChild('sdate') sdateRef: ElementRef;

  week: Week;

  constructor(
    private deviceTimerOptions: DeviceTimerOptionsService,
    private deviceTimerConfigUtils: DeviceTimerConfigUtilsService,
    private configService: ConfigService
  ) { }

  ngOnInit() {
    this.typeOptions = this.deviceTimerOptions.timerTypes;
    this.commandOptions = this.deviceTimerOptions.command;
    this.monthOptions = this.deviceTimerOptions.month;
    this.dayOptions = this.deviceTimerOptions.monthday;
    this.weekdayOptions = this.deviceTimerOptions.weekday;
    this.hourOptions = this.deviceTimerOptions.hour;
    this.minuteOptions = this.deviceTimerOptions.minute;
    this.occurenceOptions = this.deviceTimerOptions.occurence;

    this.isColorSettingsAvailable = Utils.isLED(this.colorSettingsType);

    this.initDaysSelection();
  }

  ngAfterViewInit() {
    this.initDatePicker();
  }

  ngOnChanges(changes: SimpleChanges) {
    this.initDaysSelection();
    this.onTimerSettingsDateChange();
    this.onTimerSettingsDaysChange();
  }

  // tslint:disable:no-bitwise
  onWeekDaysChange() {
    let days = 0x00;

    if (this.week.type === 'Everyday') {
      days = 0x80;
    } else if (this.week.type === 'Weekdays') {
      days = 0x100;
    } else if (this.week.type === 'Weekends') {
      days = 0x200;
    } else {
      days |= this.week.days.mon && 0x01;
      days |= this.week.days.tue && 0x02;
      days |= this.week.days.wed && 0x04;
      days |= this.week.days.thu && 0x08;
      days |= this.week.days.fri && 0x10;
      days |= this.week.days.sat && 0x20;
      days |= this.week.days.sun && 0x40;
    }

    this.timerSettings.days = days;
    this.timerSettingsChange.emit(this.timerSettings);
    // this.onTimerSettingsDaysChange();
  }

  // FIXME use datepicker directive
  private initDatePicker() {
    const _this = this;

    const nowTemp = new Date();
    const now = new Date(nowTemp.getFullYear(), nowTemp.getMonth(), nowTemp.getDate(), 0, 0, 0, 0);
    const element = $('#sdate');

    element.datepicker({
      minDate: now,
      defaultDate: now,
      dateFormat: this.configService.config.DateFormat,
      showWeek: true,
      firstDay: 1,
      onSelect: (date) => {
        _this.timerSettings.date = date;
        _this.timerSettingsChange.emit(_this.timerSettings);
        // $scope.$apply();
      }
    });
  }

  onTimerSettingsDateChange() {
    const element = $('#sdate');
    if ((+new Date(this.timerSettings.date)) !== (+element.datepicker('getDate'))) {
      element.datepicker('setDate', this.timerSettings.date);
    }
  }

  private initDaysSelection() {
    this.week = {
      type: 'Everyday',
      days: {
        mon: true,
        tue: true,
        wed: true,
        thu: true,
        fri: true,
        sat: true,
        sun: true
      }
    };
  }

  onWeekTypeChange() {
    const value = this.week.type;
    if (value === 'Everyday') {
      this.week.days.mon = this.week.days.tue = this.week.days.wed = this.week.days.thu = this.week.days.fri = true;
      this.week.days.sat = this.week.days.sun = true;
    } else if (value === 'Weekdays') {
      this.week.days.mon = this.week.days.tue = this.week.days.wed = this.week.days.thu = this.week.days.fri = true;
      this.week.days.sat = this.week.days.sun = false;
    } else if (value === 'Weekends') {
      this.week.days.mon = this.week.days.tue = this.week.days.wed = this.week.days.thu = this.week.days.fri = false;
      this.week.days.sat = this.week.days.sun = true;
    }
    this.onWeekDaysChange();
  }

  onTimerSettingsDaysChange() {
    const value = this.timerSettings.days;
    if (value & 0x80) {
      this.week.type = 'Everyday';
    } else if (value & 0x100) {
      this.week.type = 'Weekdays';
    } else if (value & 0x200) {
      this.week.type = 'Weekends';
    } else {
      this.week.type = 'SelectedDays';
      this.week.days.mon = !!(value & 0x01);
      this.week.days.tue = !!(value & 0x02);
      this.week.days.wed = !!(value & 0x04);
      this.week.days.thu = !!(value & 0x08);
      this.week.days.fri = !!(value & 0x10);
      this.week.days.sat = !!(value & 0x20);
      this.week.days.sun = !!(value & 0x40);
    }
    this.onWeekTypeChange();
  }

  public isLevelVisible(): boolean {
    return this.levelOptions && this.levelOptions.length > 0;
  }

  public isDateVisible(): boolean {
    return this.deviceTimerConfigUtils.isDateApplicable(this.timerSettings.timertype);
  }

  public isDayScheduleVisible(): boolean {
    return this.deviceTimerConfigUtils.isDayScheduleApplicable(this.timerSettings.timertype);
  }

  public isRDaysVisible(): boolean {
    return this.deviceTimerConfigUtils.isRDaysApplicable(this.timerSettings.timertype);
  }

  public isROccurenceVisible(): boolean {
    return this.deviceTimerConfigUtils.isOccurrenceApplicable(this.timerSettings.timertype);
  }

  public isRMonthsVisible(): boolean {
    return this.deviceTimerConfigUtils.isRMonthsApplicable(this.timerSettings.timertype);
  }

  public isDaySelectonAvailable(): boolean {
    return this.week.type === 'SelectedDays';
  }

}

class Week {
  type: string;
  days: {
    mon: boolean;
    tue: boolean;
    wed: boolean;
    thu: boolean;
    fri: boolean;
    sat: boolean;
    sun: boolean;
  };
}
