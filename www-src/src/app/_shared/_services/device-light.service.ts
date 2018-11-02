import {Injectable} from '@angular/core';
import {ApiService} from './api.service';
import {Observable} from 'rxjs';
import {ApiResponse} from '../_models/api';
import {mergeMap} from 'rxjs/operators';
import {SceneService} from './scene.service';
import {ApiHelperService} from './api-helper.service';
import {DeviceUtils} from '../_utils/device-utils';
import {Device} from "../_models/device";

@Injectable()
export class DeviceLightService {

  constructor(
    private apiService: ApiService,
    private sceneService: SceneService,
    private apiHelperService: ApiHelperService
  ) { }

  public switchOff(deviceIdx: string): Observable<ApiResponse> {
    return this.switchCommand('Off', deviceIdx);
  }

  public switchOn(deviceIdx: string): Observable<ApiResponse> {
    return this.switchCommand('On', deviceIdx);
  }

  public setColor(deviceIdx: string, color: string, brightness: number): Observable<ApiResponse> {
    return this.apiHelperService.checkUserPermissions().pipe(
      mergeMap(() => {
        return this.apiService.callApi<ApiResponse>('command', {
          param: 'setcolbrightnessvalue',
          idx: deviceIdx,
          color: color,
          brightness: brightness.toString()
        });
      }));
  }

  public brightnessUp(deviceIdx: string): Observable<ApiResponse> {
    return this.command('brightnessup', deviceIdx);
  }

  public brightnessDown(deviceIdx: string): Observable<ApiResponse> {
    return this.command('brightnessdown', deviceIdx);
  }

  public nightLight(deviceIdx: string): Observable<ApiResponse> {
    return this.command('nightlight', deviceIdx);
  }

  public fullLight(deviceIdx: string): Observable<ApiResponse> {
    return this.command('fulllight', deviceIdx);
  }

  public whiteLight(deviceIdx: string): Observable<ApiResponse> {
    return this.command('whitelight', deviceIdx);
  }

  public colorWarmer(deviceIdx: string): Observable<ApiResponse> {
    return this.command('warmer', deviceIdx);
  }

  public colorColder(deviceIdx: string): Observable<ApiResponse> {
    return this.command('cooler', deviceIdx);
  }

  public discoUp(deviceIdx: string): Observable<ApiResponse> {
    return this.command('discoup', deviceIdx);
  }

  public discoDown(deviceIdx: string): Observable<ApiResponse> {
    return this.command('discodown', deviceIdx);
  }

  public discoMode(deviceIdx: string): Observable<ApiResponse> {
    return this.command('discomode', deviceIdx);
  }

  public speedUp(deviceIdx: string): Observable<ApiResponse> {
    return this.command('speedup', deviceIdx);
  }

  public speedDown(deviceIdx: string): Observable<ApiResponse> {
    return this.command('speeddown', deviceIdx);
  }

  public speedMin(deviceIdx: string): Observable<ApiResponse> {
    return this.command('speedmin', deviceIdx);
  }

  public speedMax(deviceIdx: string): Observable<ApiResponse> {
    return this.command('speedmax', deviceIdx);
  }

  private switchCommand(command: string, deviceIdx: string): Observable<ApiResponse> {
    return this.apiHelperService.checkUserPermissions().pipe(
      mergeMap(() => {
        return this.apiService.callApi<ApiResponse>('command', { param: 'switchlight', idx: deviceIdx, switchcmd: command });
      })
    );
  }

  private command(command: string, deviceIdx: string): Observable<ApiResponse> {
    return this.apiHelperService.checkUserPermissions().pipe(
      mergeMap(() => {
        return this.apiService.callApi<ApiResponse>('command', { param: command, idx: deviceIdx });
      })
    );
  }

  public toggle(t: Device): Observable<ApiResponse> {
    if (DeviceUtils.isScene(t)) {
      return DeviceUtils.isActive(t) ? this.sceneService.switchOff(t.idx) : this.sceneService.switchOn(t.idx);
    } else if (
      (['Light/Switch', 'Lighting 2'].includes(t.Type) && [0, 7, 9, 10].includes(t.SwitchTypeVal))
      || t.Type === 'Color Switch'
    ) {
      return DeviceUtils.isActive(t) ? this.switchOff(t.idx) : this.switchOn(t.idx);
    }
  }

}
