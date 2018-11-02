import {Injectable} from '@angular/core';
import {ApiService} from './api.service';
import {FloorplanResponse, FloorplansResponse, UnusedFloorplanPlansResponse} from '../_models/floorplans';
import {Observable} from 'rxjs';
import {ApiResponse} from '../_models/api';

@Injectable()
export class FloorplansService {

  constructor(
    private apiService: ApiService
  ) {
  }

  public getFloorplans(): Observable<FloorplansResponse> {
    return this.apiService.callApi<FloorplansResponse>('floorplans', {});
  }

  public getFloorplan(idx: string): Observable<FloorplanResponse> {
    return this.apiService.callApi<FloorplanResponse>('command', {
      param: 'getfloorplanplans',
      idx: idx
    });
  }

  getUnusedFloorplanPlans(): Observable<UnusedFloorplanPlansResponse> {
    return this.apiService.callApi('command', {param: 'getunusedfloorplanplans', unique: 'false'});
  }

  changeFloorplanOrder(floorplanid: string, order: number): Observable<ApiResponse> {
    return this.apiService.callApi('command', {
      param: 'changefloorplanorder',
      idx: floorplanid,
      way: order.toString()
    });
  }

  deleteFloorplan(idx: string): Observable<ApiResponse> {
    return this.apiService.callApi('command', {param: 'deletefloorplan', idx: idx});
  }

  addFloorplan(idx: string, planidx: string): Observable<ApiResponse> {
    return this.apiService.callApi('command', {param: 'addfloorplanplan', idx: idx, planidx: planidx});
  }

  deleteFloorplanPlan(planidx: string): Observable<ApiResponse> {
    return this.apiService.callApi('command', {param: 'deletefloorplanplan', idx: planidx});
  }

  updateFloorplanPlan(planidx: string, area: string): Observable<ApiResponse> {
    return this.apiService.callApi('command', {param: 'updatefloorplanplan', planidx: planidx, area: area});
  }

  setPlanDeviceCoords(idx: string, planidx: string, xoffset: string, yoffset: string, devscenetype: string): Observable<ApiResponse> {
    return this.apiService.callApi('command', {
      param: 'setplandevicecoords',
      idx: idx,
      planidx: planidx,
      xoffset: xoffset,
      yoffset: yoffset,
      DevSceneType: devscenetype
    });
  }

  uploadFloorplanImage(name: string, scalefactor: string, file: File): Observable<ApiResponse> {
    const formData = new FormData();
    formData.append('planname', name);
    formData.append('scalefactor', scalefactor);
    formData.append('imagefile', file);
    return this.apiService.postFormData('uploadfloorplanimage.webem', formData);
  }

  updateFloorplan(idx: string, name: string, scalefactor: string): Observable<ApiResponse> {
    return this.apiService.callApi('command', {
      param: 'updatefloorplan',
      idx: idx,
      name: name,
      scalefactor: scalefactor
    });
  }
}
