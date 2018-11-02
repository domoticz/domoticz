import { Injectable } from '@angular/core';
import { ApiService } from '../../_shared/_services/api.service';
import { Observable } from 'rxjs';
import { PingerNodesResponse } from './pinger';
import { ApiResponse } from '../../_shared/_models/api';

@Injectable()
export class PingerService {

  constructor(private apiService: ApiService) { }

  getNodes(idx: string): Observable<PingerNodesResponse> {
    return this.apiService.callApi('command', {
      param: 'pingergetnodes',
      idx: idx
    });
  }

  setMode(idx: string, Mode1: number, Mode2: number): Observable<ApiResponse> {
    return this.apiService.callApi('command', {
      param: 'pingersetmode',
      idx: idx,
      mode1: Mode1.toString(),
      mode2: Mode2.toString()
    });
  }

  clearNodes(idx: string): Observable<ApiResponse> {
    return this.apiService.callApi('command', {
      param: 'pingerclearnodes',
      idx: idx
    });
  }

  addNode(idx: string, name: string, ip: string, Timeout: number): Observable<ApiResponse> {
    return this.apiService.callApi('command', {
      param: 'pingeraddnode',
      idx: idx,
      name: name,
      ip: ip,
      timeout: Timeout.toString()
    });
  }

  updateNode(idx: string, nodeid: any, name: string, ip: string, Timeout: number): Observable<ApiResponse> {
    return this.apiService.callApi('command', {
      param: 'pingerupdatenode',
      idx: idx,
      nodeid: nodeid.toString(),
      name: name,
      ip: ip,
      timeout: Timeout.toString()
    });
  }

  removeNode(idx: string, nodeid: any): Observable<ApiResponse> {
    return this.apiService.callApi('command', {
      param: 'pingerremovenode',
      idx: idx,
      nodeid: nodeid.toString()
    });
  }

}
