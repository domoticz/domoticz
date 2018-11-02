import {ApiService} from './api.service';
import {map} from 'rxjs/operators';
import {ApiResponse} from '../_models/api';
import {Observable, throwError} from 'rxjs';
import {Timer, TimersResponse} from '../_models/timers';

export abstract class AbstractTimersService {

  constructor(
    private apiService: ApiService,
    private timerType: string
  ) {
  }

  public getTimers(deviceIdx: string): Observable<Array<Timer>> {
    return this.apiService.callApi<TimersResponse>(this.timerType + 's', {idx: deviceIdx}).pipe(
      map(data => {
        if (data.status === 'OK') {
          return data.result || [];
        } else {
          throwError(data);
        }
      })
    );
  }

  public addTimer(deviceIdx: string, config): Observable<ApiResponse> {
    return this.apiService.callApi<ApiResponse>('command', {...config, param: 'add' + this.timerType, idx: deviceIdx});
  }

  public updateTimer(timerIdx: string, config): Observable<ApiResponse> {
    return this.apiService.callApi<ApiResponse>('command', {
      ...config,
      param: 'update' + this.timerType,
      idx: timerIdx
    });
  }

  public clearTimers(deviceIdx: string): Observable<ApiResponse> {
    return this.apiService.callApi<ApiResponse>('command', {param: 'clear' + this.timerType + 's', idx: deviceIdx});
  }

  public deleteTimer(timerIdx: string): Observable<ApiResponse> {
    return this.apiService.callApi<ApiResponse>('command', {param: 'delete' + this.timerType, idx: timerIdx});
  }

}

