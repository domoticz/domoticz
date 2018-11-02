import { Injectable } from '@angular/core';
import { ApiService } from '../../_shared/_services/api.service';
import { Observable } from 'rxjs';
import { KodiNodesResponse } from './kodi';
import { ApiResponse } from '../../_shared/_models/api';

@Injectable()
export class KodiService {

  constructor(private apiService: ApiService) { }

  getNodes(idx: string): Observable<KodiNodesResponse> {
    return this.apiService.callApi('command', {
      param: 'kodigetnodes',
      idx: idx
    });
  }

  setMode(idx: string, Mode1: number, Mode2: number): Observable<ApiResponse> {
    return this.apiService.callApi('command', {
      param: 'kodisetmode',
      idx: idx,
      mode1: Mode1.toString(),
      mode2: Mode2.toString()
    });
  }

  addNode(idx: string, name: string, ip: string, Port: number): Observable<ApiResponse> {
    return this.apiService.callApi('command', {
      param: 'kodiaddnode',
      idx: idx,
      name: name,
      ip: ip,
      port: Port.toString()
    });
  }

  clearNodes(idx: string): Observable<ApiResponse> {
    return this.apiService.callApi('command', {
      param: 'kodiclearnodes',
      idx: idx
    });
  }

  updateNode(idx: string, nodeid: string, name: string, ip: string, Port: number): Observable<ApiResponse> {
    return this.apiService.callApi('command', {
      param: 'kodiupdatenode',
      idx: idx,
      nodeid: nodeid,
      name: name,
      ip: ip,
      port: Port.toString()
    });
  }

  removeNode(idx: string, nodeid: string): Observable<ApiResponse> {
    return this.apiService.callApi('command', {
      param: 'kodiremovenode',
      idx: idx,
      nodeid: nodeid
    });
  }

}
