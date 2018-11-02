import {Injectable} from '@angular/core';
import {Observable} from 'rxjs';
import {ApiService} from '../_shared/_services/api.service';
import {History} from './history';

@Injectable()
export class HistoryService {

  constructor(private apiService: ApiService) {
  }

  private getHistory(historytype: string): Observable<History> {
    return this.apiService.callApi<History>('command', {param: historytype});
  }

  public getActualHistory(): Observable<History> {
    return this.getHistory('getactualhistory');
  }

  public getNewHistory(): Observable<History> {
    return this.getHistory('getnewhistory');
  }

}
