import { Injectable } from '@angular/core';
import { ApiService } from '../../_shared/_services/api.service';
import { Observable } from 'rxjs';
import { PanasonicTvNodesResponse } from './panasonictv';
import { ApiResponse } from '../../_shared/_models/api';

@Injectable()
export class PanasonicTvService {

  constructor(private apiService: ApiService) { }

  getNodes(idx: string): Observable<PanasonicTvNodesResponse> {
    return this.apiService.callApi('command', {
      param: 'panasonicgetnodes',
      idx: idx
    });
  }

  setMode(idx: string, Mode1: number, Mode2: number): Observable<ApiResponse> {
    return this.apiService.callApi('command', {
      param: 'panasonicsetmode',
      idx: idx,
      mode1: Mode1.toString(),
      mode2: Mode2.toString()
    });
  }

  clearNodes(idx: string): Observable<ApiResponse> {
    return this.apiService.callApi('command', {
      param: 'panasonicclearnodes',
      idx: idx
    });
  }

  addNode(idx: string, name: string, ip: string, Port: number): Observable<ApiResponse> {
    return this.apiService.callApi('command', {
      param: 'panasonicaddnode',
      idx: idx,
      name: name,
      ip: ip,
      port: Port.toString()
    });
  }

  updateNode(idx: string, nodeid: string, name: string, ip: string, Port: number) {
    return this.apiService.callApi('command', {
      param: 'panasonicupdatenode',
      idx: idx,
      nodeid: nodeid,
      name: name,
      ip: ip,
      port: Port.toString()
    });
  }

  removeNode(idx: string, nodeid: string): Observable<ApiResponse> {
    return this.apiService.callApi('command', {
      param: 'panasonicremovenode',
      idx: idx,
      nodeid: nodeid
    });
  }

}
