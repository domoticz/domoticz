import {Injectable} from '@angular/core';
import {Observable} from 'rxjs';
import {ApiService} from '../../../_shared/_services/api.service';
import {FibaroConfig, FibaroLinksResponse} from './fibaro';
import {ApiResponse} from '../../../_shared/_models/api';

@Injectable()
export class FibaroService {

  constructor(private apiService: ApiService) {
  }

  getConfig(): Observable<FibaroConfig> {
    return this.apiService.callApi('command', {param: 'getfibarolinkconfig'});
  }

  getLinks(): Observable<FibaroLinksResponse> {
    return this.apiService.callApi('command', {param: 'getfibarolinks'});
  }

  saveFibaroLink(idx: string, deviceid: string, valuetosend: string, targettype: string, linkactive: number, otherparams: any): Observable<ApiResponse> {
    return this.apiService.callApi('command', {
      ...otherparams,
      param: 'savefibarolink',
      idx: idx,
      deviceid: deviceid,
      valuetosend: valuetosend,
      targettype: targettype,
      linkactive: linkactive.toString()
    });
  }

  deleteFibaroLink(idx: string): Observable<ApiResponse> {
    return this.apiService.callApi('command', {param: 'deletefibarolink', idx: idx});
  }

  saveFibaroLinkConfig(remote: string, username: string, password: string, linkactive: number, isversion4: number, debugenabled: number): Observable<ApiResponse> {
    return this.apiService.callApi('command', {
      param: 'savefibarolinkconfig',
      remote: remote,
      username: username,
      password: password,
      linkactive: linkactive.toString(),
      isversion4: isversion4.toString(),
      debugenabled: debugenabled.toString()
    });
  }
}
