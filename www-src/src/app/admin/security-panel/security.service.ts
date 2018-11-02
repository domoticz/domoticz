import {Injectable} from '@angular/core';
import {ApiService} from '../../_shared/_services/api.service';
import {Observable} from 'rxjs';
import {SecurityResponse} from './security';
import {ApiResponse} from '../../_shared/_models/api';

@Injectable()
export class SecurityService {

  constructor(private apiService: ApiService) {
  }

  getSecStatus(): Observable<SecurityResponse> {
    return this.apiService.callApi<SecurityResponse>('command', {param: 'getsecstatus'});
  }

  setSecStatus(status: number, seccode: string): Observable<ApiResponse> {
    return this.apiService.callApi('command', {
      param: 'setsecstatus',
      secstatus: status.toString(),
      seccode: seccode
    });
  }
}
