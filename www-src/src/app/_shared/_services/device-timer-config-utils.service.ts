import { Injectable, Inject } from '@angular/core';
import { I18NEXT_SERVICE, ITranslationService } from 'angular-i18next';

@Injectable()
export class DeviceTimerConfigUtilsService {

  constructor(
    @Inject(I18NEXT_SERVICE) private translationService: ITranslationService
  ) { }

  public isDateApplicable(timerType: number) {
    return timerType === 5;
  }

  public isDayScheduleApplicable(timerType: number) {
    return ![5, 6, 7, 10, 11, 12, 13].includes(timerType);
  }

  public isRDaysApplicable(timerType: number) {
    return [10, 12].includes(timerType);
  }

  public isOccurrenceApplicable(timerType: number) {
    return [11, 13].includes(timerType);
  }

  public isRMonthsApplicable(timerType: number) {
    return [12, 13].includes(timerType);
  }

  public getTimerDefaultConfig(): TimerConfig {
    return {
      active: true,
      timertype: 0,
      date: '',
      hour: 0,
      min: 0,
      randomness: false,
      command: 0,
      level: null,
      tvalue: '',
      days: 0x80,
      color: '', // Empty string, intentionally illegal JSON
      mday: 1,
      month: 1,
      occurence: 1,
      weekday: 0,
    };
  }

  public getTimerConfigErrors(config: TimerConfig): string {
    if (config.timertype === 5) {
      if (!config.date) {
        return this.translationService.t('Please select a Date!');
      }

      // Check if date/time is valid
      const pickedDate = new Date(config.date);
      const checkDate = new Date(pickedDate.getFullYear(), pickedDate.getMonth(), pickedDate.getDate(), config.hour, config.min, 0, 0);
      const nowDate = new Date();

      if (checkDate < nowDate) {
        return this.translationService.t('Invalid Date selected!');
      }
    }

    if (config.timertype === 12) {
      if ([4, 6, 9, 11].includes(config.month) && config.mday === 31) {
        return this.translationService.t('This month does not have 31 days!');
      }

      if (config.month === 2 && config.mday > 29) {
        return this.translationService.t('February does not have more than 29 days!')
      }
    }

    if (![5, 6, 7, 10, 11, 12, 13].includes(config.timertype) && config.days === 0) {
      return this.translationService.t('Please select some days!');
    }
  }

  public getTimerConfigWarnings(config: TimerConfig) {
    if (config.timertype === 10 && config.mday > 28) {
      return this.translationService.t('Not al months have this amount of days, some months will be skipped!');
    }

    if (config.timertype === 12 && config.month === 2 && config.mday === 29) {
      return this.translationService.t('Not all years have this date, some years will be skipped!');
    }
  }

}

export interface TimerConfig {
  active: boolean;
  timertype: number;
  date?: string;
  hour: number;
  min: number;
  randomness: boolean;
  command: number;
  level: number;
  tvalue: string;
  days: number;
  color: string;
  mday: number;
  month: number;
  occurence: number;
  weekday: number;
}
