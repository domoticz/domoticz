import {Injectable} from '@angular/core';
import {ApiService} from '../../../_shared/_services/api.service';
import {Observable} from 'rxjs';
import {InfluxConfigResponse, InfluxLinksResponse} from './influx';
import {ApiResponse} from '../../../_shared/_models/api';

@Injectable()
export class InfluxService {

  constructor(private apiService: ApiService) {
  }

  getConfig(): Observable<InfluxConfigResponse> {
    return this.apiService.callApi('command', {param: 'getinfluxlinkconfig'});
  }

  getInfluxLinks(): Observable<InfluxLinksResponse> {
    return this.apiService.callApi('command', {param: 'getinfluxlinks'});
  }

  saveInfluxLink(idx: string, deviceid: string, valuetosend: string, targettype: string, linkactive: number): Observable<ApiResponse> {
    return this.apiService.callApi('command', {
      param: 'saveinfluxlink',
      idx: idx,
      deviceid: deviceid,
      valuetosend: valuetosend,
      targettype: targettype,
      linkactive: linkactive.toString()
    });
  }

  deleteInfluxLink(idx: string): Observable<ApiResponse> {
    return this.apiService.callApi('command', {param: 'deleteinfluxlink', idx: idx});
  }

  saveInfluxConfig(linkactive: number, remote: string, port: string, path: string, database: string, username: string, password: string, debugenabled: number): Observable<ApiResponse> {
    return this.apiService.callApi('command', {
      param: 'saveinfluxlinkconfig',
      linkactive: linkactive.toString(),
      remote: remote,
      port: port,
      path: path,
      database: database,
      username: username,
      password: password,
      debugenabled: debugenabled.toString(),
    });
  }
}
