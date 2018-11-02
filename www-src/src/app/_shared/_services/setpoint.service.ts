import {Injectable} from '@angular/core';
import {ApiService} from './api.service';
import {ApiResponse} from '../_models/api';
import {Observable} from 'rxjs';

@Injectable()
export class SetpointService {

  constructor(private apiService: ApiService) {
  }

  public setSetpoint(devIdx: string, setpoint: number): Observable<ApiResponse> {
    return this.apiService.callApi<ApiResponse>('command', {
      param: 'setsetpoint',
      idx: devIdx,
      setpoint: setpoint.toString()
    });
  }

}
