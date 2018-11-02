import {Injectable} from '@angular/core';
import {ApiService} from '../../../_shared/_services/api.service';
import {Observable} from 'rxjs';
import {HttpConfigResponse, HttpLinksResponse} from './http';
import {ApiResponse} from '../../../_shared/_models/api';

@Injectable()
export class DpHttpService {

  constructor(private apiService: ApiService) {
  }

  getConfig(): Observable<HttpConfigResponse> {
    return this.apiService.callApi('command', {param: 'gethttplinkconfig'});
  }

  getHttpLinks(): Observable<HttpLinksResponse> {
    return this.apiService.callApi('command', {param: 'gethttplinks'});
  }

  saveHttpLink(idx: string, deviceid: string, valuetosend: string, targettype: string, linkactive: number, otherparams: any): Observable<ApiResponse> {
    return this.apiService.callApi('command', {
      ...otherparams,
      param: 'savehttplink',
      idx: idx,
      deviceid: deviceid, valuetosend: valuetosend,
      targettype: targettype,
      linkactive: linkactive.toString(),
    });
  }

  deleteLink(idx: string): Observable<ApiResponse> {
    return this.apiService.callApi('command', {param: 'deletehttplink', idx: idx});
  }

  saveHttpLinkConfiguration(url: string, method: number, headers: string, data: string, linkactive: number, debugenabled: number, auth: number, authbasiclogin: string, authbasicpassword: string): Observable<ApiResponse> {
    return this.apiService.callApi('command', {
      param: 'savehttplinkconfig',
      url: url,
      method: method.toString(),
      headers: headers,
      data: data,
      linkactive: linkactive.toString(),
      debugenabled: debugenabled.toString(),
      auth: auth.toString(),
      authbasiclogin: authbasiclogin,
      authbasicpassword: authbasicpassword
    });
  }
}
