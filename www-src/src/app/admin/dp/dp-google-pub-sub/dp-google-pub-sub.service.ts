import {Injectable} from '@angular/core';
import {ApiService} from '../../../_shared/_services/api.service';
import {Observable} from 'rxjs';
import {GooglePubSubConfigResponse, GooglePubSubLinksResponse} from './googlepubsub';
import {ApiResponse} from '../../../_shared/_models/api';

@Injectable()
export class DpGooglePubSubService {

  constructor(private apiService: ApiService) {
  }

  getConfig(): Observable<GooglePubSubConfigResponse> {
    return this.apiService.callApi('command', {param: 'getgooglepubsublinkconfig'});
  }

  getLinks(): Observable<GooglePubSubLinksResponse> {
    return this.apiService.callApi('command', {param: 'getgooglepubsublinks'});
  }

  deleteLink(idx: string): Observable<ApiResponse> {
    return this.apiService.callApi('command', {param: 'deletegooglepubsublink', idx: idx});
  }

  saveLink(idx: string, deviceid: string, valuetosend: string, targettype: string, linkactive: number, otherparams: any): Observable<ApiResponse> {
    return this.apiService.callApi('command', {
      ...otherparams,
      param: 'savegooglepubsublink',
      idx: idx,
      deviceid: deviceid,
      valuetosend: valuetosend,
      targettype: targettype,
      linkactive: linkactive.toString()
    });
  }

  saveConfig(httpdata: string, linkactive: number, debugenabled: number): Observable<ApiResponse> {
    return this.apiService.callApi('command', {
      param: 'savegooglepubsublinkconfig',
      data: httpdata,
      linkactive: linkactive.toString(),
      debugenabled: debugenabled.toString()
    });
  }
}
