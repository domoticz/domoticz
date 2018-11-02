import { Injectable } from '@angular/core';
import { ApiService } from './api.service';
import { ThemesResponse } from '../_models/theme';
import { Observable } from 'rxjs';
import { TimerPlansResponse } from '../_models/timers';
import { SettingsResponse } from '../_models/settings';
import { ApiResponse } from '../_models/api';

@Injectable()
export class SetupService {

  constructor(
    private apiService: ApiService
  ) { }

  public getThemes(): Observable<ThemesResponse> {
    return this.apiService.callApi<ThemesResponse>('command', { param: 'getthemes' });
  }

  public getTimerPlans(): Observable<TimerPlansResponse> {
    return this.apiService.callApi<TimerPlansResponse>('command', { param: 'gettimerplans' });
  }

  public getSettings(): Observable<SettingsResponse> {
    return this.apiService.callApi<SettingsResponse>('settings', {});
  }

  public allowNewHardware(minutes: number): Observable<ApiResponse> {
    return this.apiService.callApi<ApiResponse>('command', { param: 'allownewhardware', timeout: minutes.toString() });
  }

  public clearShortLog(idx: string): Observable<ApiResponse> {
    return this.apiService.callApi<ApiResponse>('command', { param: 'clearshortlog', idx: idx });
  }

  public storeSettings(settings: FormData): Observable<ApiResponse> {
    return this.apiService.postFormData('storesettings', settings);
  }

  public testNotification(subsystem: string, extraparams: { [param: string]: string }): Observable<ApiResponse> {
    return this.apiService.callApi('command', { ...extraparams, param: 'testnotification', subsystem: subsystem });
  }

}
