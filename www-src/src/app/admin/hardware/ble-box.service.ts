import { Injectable } from '@angular/core';
import { ApiService } from '../../_shared/_services/api.service';
import { BleBoxNodesResponse } from './blebox';
import { Observable } from 'rxjs';
import { ApiResponse } from '../../_shared/_models/api';

@Injectable()
export class BleBoxService {

  constructor(private apiService: ApiService) { }

  getNodes(idx: string): Observable<BleBoxNodesResponse> {
    return this.apiService.callApi('command', {
      param: 'bleboxgetnodes',
      idx: idx
    });
  }

  setMode(idx: string, Mode1: number, Mode2: number): Observable<ApiResponse> {
    return this.apiService.callApi('command', {
      param: 'bleboxsetmode',
      idx: idx,
      mode1: Mode1.toString(),
      mode2: Mode2.toString()
    });
  }

  autoSearchingNodes(idx: string, ipmask: string): Observable<ApiResponse> {
    return this.apiService.callApi('command', {
      param: 'bleboxautosearchingnodes',
      idx: idx,
      ipmask: ipmask
    });
  }

  updateFirmware(idx: string): Observable<ApiResponse> {
    return this.apiService.callApi('command', {
      param: 'bleboxupdatefirmware',
      idx: idx
    });
  }

  clearNodes(idx: string): Observable<ApiResponse> {
    return this.apiService.callApi('command', {
      param: 'bleboxclearnodes',
      idx: idx
    });
  }

  addNode(idx: string, name: string, ip: string): Observable<ApiResponse> {
    return this.apiService.callApi('command', {
      param: 'bleboxaddnode',
      idx: idx,
      name: name,
      ip: ip
    });
  }

  removeNode(idx: string, nodeid: string): Observable<ApiResponse> {
    return this.apiService.callApi('command', {
      param: 'bleboxremovenode',
      idx: idx,
      nodeid: nodeid
    });
  }

}
