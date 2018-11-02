import { Injectable } from '@angular/core';
import { ApiService } from '../../_shared/_services/api.service';
import { Observable } from 'rxjs';
import { WolNodesResponse } from './wol';
import { ApiResponse } from '../../_shared/_models/api';

@Injectable()
export class WolService {

  constructor(
    private apiService: ApiService
  ) { }

  getNodes(idx: string): Observable<WolNodesResponse> {
    return this.apiService.callApi('command', {
      param: 'wolgetnodes',
      idx: idx
    });
  }

  clearNodes(idx: string): Observable<ApiResponse> {
    return this.apiService.callApi('command', {
      param: 'wolclearnodes',
      idx: idx
    });
  }

  addNode(idx: string, name: string, mac: string): Observable<ApiResponse> {
    return this.apiService.callApi('command', {
      param: 'woladdnode',
      idx: idx,
      name: name,
      mac: mac
    });
  }

  updateNode(idx: string, nodeid: string, name: string, mac: string): Observable<ApiResponse> {
    return this.apiService.callApi('command', {
      param: 'wolupdatenode',
      idx: idx,
      nodeid: nodeid,
      name: name,
      mac: mac
    });
  }

  removeNode(idx: string, nodeid: string): Observable<ApiResponse> {
    return this.apiService.callApi('command', {
      param: 'wolremovenode',
      idx: idx,
      nodeid: nodeid
    });
  }

}
