import { Injectable, Inject } from '@angular/core';
import { ITranslationService, I18NEXT_SERVICE } from 'angular-i18next';

@Injectable()
export class DeviceTimerOptionsService {

  timerTypes: Array<LabelValue>;
  command: Array<LabelValue>;
  month: Array<LabelValue>;
  monthday: Array<LabelValue>;
  weekday: Array<LabelValue>;
  hour: Array<LabelValue>;
  minute: Array<LabelValue>;
  occurence: Array<LabelValue>;

  constructor(
    @Inject(I18NEXT_SERVICE) private translationService: ITranslationService
  ) {
    this.timerTypes = this.getTimerTypes();
    this.command = this.getCommandOptions();
    this.month = this.getMonthOptions();
    this.monthday = this.getMonthdayOptions();
    this.weekday = this.getWeekdayOptions();
    this.hour = this.getHourOptions();
    this.minute = this.getMinuteOptions();
    this.occurence = this.getOccurenceOptions();
  }

  getLabelForValue(options: Array<LabelValue>, value: number): string {
    const option = options.find((_option) => {
      return _option.value === value;
    });

    return option ? option.label : '';
  }

  private getTimerTypes(): Array<LabelValue> {
    return [
      'Before Sunrise', 'After Sunrise', 'On Time', 'Before Sunset', 'After Sunset',
      'Fixed Date/Time', 'Odd Day Numbers', 'Even Day Numbers', 'Odd Week Numbers',
      'Even Week Numbers', 'Monthly', 'Monthly (Weekday)', 'Yearly', 'Yearly (Weekday)',
      'Before Sun at South', 'After Sun at South', 'Before Civil Twilight Start',
      'After Civil Twilight Start', 'Before Civil Twilight End', 'After Civil Twilight End',
      'Before Nautical Twilight Start', 'After Nautical Twilight Start',
      'Before Nautical Twilight End', 'After Nautical Twilight End',
      'Before Astronomical Twilight Start', 'After Astronomical Twilight Start',
      'Before Astronomical Twilight End', 'After Astronomical Twilight End'
    ].map((item: string, index: number) => {
      return {
        label: this.translationService.t(item),
        value: index
      };
    });
  }

  private getCommandOptions(): Array<LabelValue> {
    return [
      { label: this.translationService.t('On'), value: 0 },
      { label: this.translationService.t('Off'), value: 1 },
    ];
  }

  private getMonthOptions(): Array<LabelValue> {
    return ['January', 'February', 'March', 'April', 'May', 'June', 'July', 'August', 'September', 'October', 'November', 'December']
      .map((month, index) => {
        return {
          label: this.translationService.t(month),
          value: index + 1
        };
      });
  }

  private getMonthdayOptions(): Array<LabelValue> {
    const options = [];

    for (let i = 1; i <= 31; i++) {
      options.push({
        label: i,
        value: i
      });
    }

    return options;
  }

  private getWeekdayOptions(): Array<LabelValue> {
    return ['Monday', 'Tuesday', 'Wednesday', 'Thursday', 'Friday', 'Saturday', 'Sunday']
      .map((day, index) => {
        return {
          label: this.translationService.t(day),
          value: index
        };
      });
  }

  private getHourOptions(): Array<LabelValue> {
    const options = [];

    for (let i = 0; i < 24; i++) {
      options.push({
        label: ('0' + i).slice(-2),
        value: i
      });
    }

    return options;
  }

  private getMinuteOptions(): Array<LabelValue> {
    const options = [];

    for (let i = 0; i < 60; i++) {
      options.push({
        label: ('0' + i).slice(-2),
        value: i
      });
    }

    return options;
  }

  private getOccurenceOptions(): Array<LabelValue> {
    return ['First', '2nd', '3rd', '4th', 'Last']
      .map((item, index) => {
        return {
          label: this.translationService.t(item),
          value: index + 1
        };
      });
  }

}

export interface LabelValue {
  label: string;
  value: number;
}
