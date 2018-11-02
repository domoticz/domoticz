import { Injectable, Inject } from '@angular/core';
import { PermissionService } from './permission.service';
import { ApiService } from './api.service';
import { ApiHelperService } from './api-helper.service';
import { NotificationService } from './notification.service';
import { I18NEXT_SERVICE, ITranslationService } from 'angular-i18next';
import { tap, delay, catchError, mergeMap, map } from 'rxjs/operators';
import { Observable, of, throwError } from 'rxjs';
import { ApiResponse } from '../_models/api';
import { SceneDevices, SceneActivations, SceneLogs, SceneLog } from '../_models/scene';
import { TimesunService } from './timesun.service';
import {Device, DevicesResponse} from "../_models/device";

// FIXME proper declaration
declare var bootbox: any;

@Injectable()
export class SceneService {

  constructor(
    private permissionService: PermissionService,
    private apiService: ApiService,
    private notificationService: NotificationService,
    @Inject(I18NEXT_SERVICE) private translationService: ITranslationService,
    private apiHelperService: ApiHelperService,
    private timesunService: TimesunService
  ) { }

  public getScenes(lastUpdate?: number): Observable<DevicesResponse> {
    const params: any = {};
    if (lastUpdate !== undefined) {
      params.lastupdate = lastUpdate.toString();
    }
    return this.apiService.callApi<DevicesResponse>('scenes', params).pipe(
      tap(response => {
        this.timesunService.SetTimeAndSun(response);
      })
    );
  }

  public getScenesForBlockly(order: string, displayHidden: boolean): Observable<DevicesResponse> {
    return this.apiService.callApi<DevicesResponse>('scenes', {
      order: order,
      displayhidden: displayHidden ? '1' : ''
    });
  }

  public SwitchScene(idx: string, switchcmd: string, isprotected: boolean): Observable<any> | null {
    if (this.permissionService.getRights() === 0) {
      this.notificationService.HideNotify();
      this.notificationService.ShowNotify(this.translationService.t('You do not have permission to do that!'), 2500, true);
      return null;
    }

    let call: Observable<any> = null;
    let passcode = '';
    if (typeof isprotected !== 'undefined') {
      if (isprotected === true) {
        bootbox.prompt(this.translationService.t('Please enter Password') + ':', (result) => {
          if (result === null) {
            return;
          } else {
            if (result === '') {
              return;
            }
            passcode = result;
            call = this.SwitchSceneInt(idx, switchcmd, passcode);
          }
        });
      } else {
        call = this.SwitchSceneInt(idx, switchcmd, passcode);
      }
    } else {
      call = this.SwitchSceneInt(idx, switchcmd, passcode);
    }

    return call;
  }

  private SwitchSceneInt(idx: string, switchcmd: string, passcode: string): Observable<any> {
    this.notificationService.ShowNotify(this.translationService.t('Switching') + ' ' + this.translationService.t(switchcmd));

    return this.apiService.callApi('command', {
      param: 'switchscene',
      idx: idx,
      switchcmd: switchcmd,
      passcode: passcode
    }).pipe(
      tap((data: ApiResponse) => {
        if (data.status === 'ERROR') {
          this.notificationService.HideNotify();
          bootbox.alert(this.translationService.t(data.message));
        }
      }),
      delay(1000),
      tap(() => {
        this.notificationService.HideNotify();
      }),
      catchError(err => {
        this.notificationService.HideNotify();
        bootbox.alert(this.translationService.t('Problem sending switch command'));
        throw err;
      })
    );
  }

  private createSwitchCommand(command: string, deviceIdx: string): Observable<any> {
    const notification = this.notificationService.ShowNotify(this.translationService.t('Switching') + ' ' +
      this.translationService.t(command));

    return this.apiHelperService.checkUserPermissions().pipe(
      mergeMap(() => {
        return this.apiService.callApi<ApiResponse>('command', {
          param: 'switchscene',
          idx: deviceIdx,
          switchcmd: command
        });
      }),
      catchError(result => {
        bootbox.alert(result.message || 'Problem sending switch command');
        throw result;
      }),
      tap(() => {
        this.notificationService.HideNotify();
      })
    );
  }

  public renameScene(sceneIdx: string, name: string): Observable<ApiResponse> {
    return this.apiService.callApi('command', {
      param: 'renamescene',
      idx: sceneIdx,
      name: name
    });
  }

  public switchOff(deviceIdx: string): Observable<any> {
    return this.createSwitchCommand('Off', deviceIdx);
  }

  public switchOn(deviceIdx: string): Observable<any> {
    return this.createSwitchCommand('On', deviceIdx);
  }

  public switchSceneOrder(idx1: string, idx2: string): Observable<ApiResponse> {
    return this.apiService.callApi<ApiResponse>('command', {
      param: 'switchsceneorder',
      idx1: idx1,
      idx2: idx2
    });
  }

  public makeFavorite(id: number, isfavorite: number): Observable<ApiResponse> {
    return this.apiService.callApi<ApiResponse>('command', {
      param: 'makescenefavorite',
      idx: id.toString(),
      isfavorite: isfavorite.toString()
    });
  }

  public addScene(sceneName: string, sceneType: string): Observable<ApiResponse> {
    return this.apiService.callApi('addscene', {
      name: sceneName,
      scenetype: sceneType
    });
  }

  // FIXME expose an API for this instead of querying all scenes...
  public getScene(idx: string): Observable<Device> {
    return this.getScenes().pipe(
      map(response => {
        const scene = response.result.find(d => d.idx === idx);
        if (scene !== undefined) {
          return scene;
        } else {
          throw new Error('Scene not found');
        }
      })
    );
  }

  public getSceneDevices(idx: string, isScene: boolean): Observable<SceneDevices> {
    return this.apiService.callApi<SceneDevices>('command', {
      param: 'getscenedevices',
      idx: idx,
      isscene: isScene.toString()
    });
  }

  public getSceneActivations(sceneIdx: string): Observable<SceneActivations> {
    return this.apiService.callApi<SceneActivations>('command', { param: 'getsceneactivations', idx: sceneIdx });
  }

  public removeSceneCode(SceneIdx: string, idx: string, code: string): Observable<ApiResponse> {
    return this.apiService.callApi<ApiResponse>('command', {
      param: 'removescenecode',
      sceneidx: SceneIdx,
      idx: idx,
      code: code
    });
  }

  public deleteSceneDevice(idx: string): Observable<ApiResponse> {
    return this.apiService.callApi('command', {
      param: 'deletescenedevice',
      idx: idx
    });
  }

  public updateSceneDevice(idx: string, isScene: boolean, DeviceIdx: string, Command: string,
    level: string, colorJSON: string, ondelay: string, offdelay: string): Observable<ApiResponse> {
    return this.apiService.callApi('command', {
      param: 'updatescenedevice',
      idx: idx,
      isscene: isScene.toString(),
      devidx: DeviceIdx,
      command: Command,
      level: level,
      color: colorJSON,
      ondelay: ondelay,
      offdelay: offdelay
    });
  }

  public updateScene(SceneIdx: string, SceneType: string, Name: string, Description: string, onaction: string,
    offaction: string, bIsProtected: boolean): Observable<ApiResponse> {
    return this.apiService.callApi('updatescene', {
      idx: SceneIdx,
      scenetype: SceneType,
      name: Name,
      description: Description,
      onaction: onaction,
      offaction: offaction,
      protected: bIsProtected.toString()
    });
  }

  public deleteScene(SceneIdx: string): Observable<ApiResponse> {
    return this.apiService.callApi('deletescene', { idx: SceneIdx });
  }

  public clearSceneCodes(SceneIdx: string): Observable<ApiResponse> {
    return this.apiService.callApi('command', {
      param: 'clearscenecodes',
      sceneidx: SceneIdx
    });
  }

  public deleteAllSceneDevices(SceneIdx: string): Observable<ApiResponse> {
    return this.apiService.callApi('command', {
      param: 'deleteallscenedevices',
      idx: SceneIdx
    });
  }

  public addSceneCode(SceneIdx: string, deviceidx: number, cmd: number): Observable<ApiResponse> {
    return this.apiService.callApi<ApiResponse>('command', {
      param: 'addscenecode',
      sceneidx: SceneIdx,
      idx: deviceidx.toString(),
      cmnd: cmd.toString()
    });
  }

  public addSceneDevice(SceneIdx: string, isScene: boolean, DeviceIdx: string, Command: string,
    level: string, colorJSON: string, ondelay: string, offdelay: string): Observable<ApiResponse> {
    return this.apiService.callApi<ApiResponse>('command', {
      param: 'addscenedevice',
      idx: SceneIdx,
      isscene: isScene.toString(),
      devidx: DeviceIdx,
      command: Command,
      level: level,
      color: colorJSON,
      ondelay: ondelay,
      offdelay: offdelay
    });
  }

  public changeSceneDeviceOrder(idx: string, way: string): Observable<ApiResponse> {
    return this.apiService.callApi('command', {
      param: 'changescenedeviceorder',
      idx: idx,
      way: way
    });
  }

  public getSceneLog(idx: string): Observable<Array<SceneLog>> {
    return this.apiService.callApi<SceneLogs>('scenelog', {
      idx: idx
    }).pipe(
      map(response => {
        if (response && response.status !== 'OK') {
          throw new Error(response.message);
        } else {
          return response.result || [];
        }
      }));
  }

  public clearSceneLog(idx: string): Observable<ApiResponse> {
    if (!this.permissionService.hasPermission('Admin')) {
      const message = this.translationService.t('You do not have permission to do that!');
      this.notificationService.HideNotify();
      this.notificationService.ShowNotify(message, 2500, true);
      return throwError(message);
    }

    return this.apiService.callApi<ApiResponse>('command', {
      param: 'clearscenelog',
      idx: idx
    });
  }

}
