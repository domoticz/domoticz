import { LogRespone } from '../../_shared/_models/log';
import { ApiService } from '../../_shared/_services/api.service';
import { Injectable } from '@angular/core';
import { Observable } from 'rxjs';

@Injectable()
export class LogService {

  static LOG_NORM = 0x0000001;
  static LOG_STATUS = 0x0000002;
  static LOG_ERROR = 0x0000004;
  static LOG_DEBUG = 0x0000008;
  static LOG_ALL = 0xFFFFFFF;

  constructor(
    private apiService: ApiService
  ) { }

  public getLog(lastLogTime: number, loglevel: number): Observable<LogRespone> {
    return this.apiService.callApi<LogRespone>('command', {
      param: 'getlog',
      lastlogtime: lastLogTime.toString(),
      loglevel: loglevel.toString()
    });
  }

  public clearLog(): Observable<any> {
    return this.apiService.callApi('command', {
      param: 'clearlog'
    });
  }

}
