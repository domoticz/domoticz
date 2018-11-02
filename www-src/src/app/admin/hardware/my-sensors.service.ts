import { Injectable } from '@angular/core';
import { MySensorsResponse, MySensorsChildrenResponse } from './my-sensors';
import { ApiService } from '../../_shared/_services/api.service';
import { Observable } from 'rxjs';
import { ApiResponse } from '../../_shared/_models/api';

@Injectable()
export class MySensorsService {

  constructor(private apiService: ApiService) { }

  getNodes(idx: string): Observable<MySensorsResponse> {
    return this.apiService.callApi('command', {
      param: 'mysensorsgetnodes',
      idx: idx
    });
  }

  getChildren(idx: string, nodeid: number): Observable<MySensorsChildrenResponse> {
    return this.apiService.callApi('command', {
      param: 'mysensorsgetchilds',
      idx: idx,
      nodeid: nodeid.toString()
    });
  }

  updateNode(idx: string, nodeid: string, name: string): Observable<ApiResponse> {
    return this.apiService.callApi('command', {
      param: 'mysensorsupdatenode',
      idx: idx,
      nodeid: nodeid,
      name: name
    });
  }

  removeNode(idx: string, nodeid: string): Observable<ApiResponse> {
    return this.apiService.callApi('command', {
      param: 'mysensorsremovenode',
      idx: idx,
      nodeid: nodeid
    });
  }

  updateChild(idx: string, nodeid: string, childid: any, useAck: boolean, AckTimeout: number): Observable<ApiResponse> {
    return this.apiService.callApi('command', {
      param: 'mysensorsupdatechild',
      idx: idx,
      nodeid: nodeid,
      childid: childid,
      useack: useAck.toString(),
      acktimeout: AckTimeout.toString()
    });
  }

  removeChild(idx: string, nodeid: string, childid: any): Observable<ApiResponse> {
    return this.apiService.callApi('command', {
      param: 'mysensorsremovechild',
      idx: idx,
      nodeid: nodeid,
      childid: childid
    });
  }

}
