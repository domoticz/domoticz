import { Injectable } from '@angular/core';
import { Observable } from 'rxjs';
import { CamerasResponse, CameraDevicesResponse, Camera } from '../_models/camera';
import { ApiService } from './api.service';
import { ApiResponse } from '../_models/api';

@Injectable()
export class CamService {

  constructor(
    private apiService: ApiService
  ) { }

  getCameras(): Observable<CamerasResponse> {
    return this.apiService.callApi<CamerasResponse>('cameras', {});
  }

  getCamerasForBlockly(order: string, displayHidden: boolean): Observable<CamerasResponse> {
    return this.apiService.callApi<CamerasResponse>('cameras', {
      order: order,
      displayhidden: displayHidden ? '1' : ''
    });
  }

  getActiveDevices(idx: string): Observable<CameraDevicesResponse> {
    return this.apiService.callApi('command', {
      param: 'getcamactivedevices',
      idx: idx
    });
  }

  deleteActiveDevice(idx: string): Observable<ApiResponse> {
    return this.apiService.callApi('command', { param: 'deleteamactivedevice', idx: idx });
  }

  deleteAllActiveCamDevices(idx: string): Observable<ApiResponse> {
    return this.apiService.callApi('command', { param: 'deleteallactivecamdevices', idx: idx });
  }

  addCamActiveDevice(idx: string, ActiveDeviceIdx: string, ADType: number, ADWhen: string, ADDelay: number): Observable<ApiResponse> {
    return this.apiService.callApi('command', {
      param: 'addcamactivedevice',
      idx: idx,
      activeidx: ActiveDeviceIdx,
      activetype: ADType.toString(),
      activewhen: ADWhen,
      activedelay: ADDelay.toString()
    });
  }

  deleteCamera(idx: string): Observable<ApiResponse> {
    return this.apiService.callApi('command', {
      param: 'deletecamera',
      idx: idx
    });
  }

  addCamera(csettings: Camera): Observable<ApiResponse> {
    return this.apiService.callApi('command', {
      param: 'addcamera',
      address: csettings.Address,
      port: csettings.Port.toString(),
      name: csettings.Name,
      enabled: csettings.Enabled.toString(),
      username: csettings.Username,
      password: csettings.Password,
      imageurl: btoa(csettings.ImageURL),
      protocol: csettings.Protocol.toString()
    });
  }

  updateCamera(csettings: Camera): Observable<ApiResponse> {
    return this.apiService.callApi('command', {
      param: 'updatecamera',
      idx: csettings.idx,
      address: csettings.Address,
      port: csettings.Port.toString(),
      name: csettings.Name,
      enabled: csettings.Enabled.toString(),
      username: csettings.Username,
      password: csettings.Password,
      imageurl: btoa(csettings.ImageURL),
      protocol: csettings.Protocol.toString()
    });
  }

}
