import { Injectable } from '@angular/core';
import { ApiService } from '../../_shared/_services/api.service';
import { Observable } from 'rxjs';
import { MobilesResponse } from './mobile';
import { ApiResponse } from '../../_shared/_models/api';

@Injectable()
export class MobileService {

  constructor(private apiService: ApiService) { }

  getMobiles(): Observable<MobilesResponse> {
    return this.apiService.callApi('mobiles', {});
  }

  testNotification(subsystem: string, extradata: string): Observable<ApiResponse> {
    return this.apiService.callApi('command', {
      param: 'testnotification',
      subsystem: subsystem,
      extradata: extradata
    });
  }

  updateMobileDevice(idx: string, enabled: boolean, name: string): Observable<ApiResponse> {
    return this.apiService.callApi('command', {
      param: 'updatemobiledevice',
      idx: idx,
      enabled: enabled.toString(),
      name: name
    });
  }

  deleteMobileDevice(uuid: string): Observable<ApiResponse> {
    return this.apiService.callApi('command', {
      param: 'deletemobiledevice',
      uuid: uuid
    });
  }

}
