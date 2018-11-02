import { Injectable, Inject } from '@angular/core';
import { ApiService } from './api.service';
import { PermissionService } from './permission.service';
import { NotificationService } from './notification.service';
import { ITranslationService, I18NEXT_SERVICE } from 'angular-i18next';
import { ApiResponse } from '../_models/api';
import { Observable } from 'rxjs';
import { catchError, tap, delay, mergeMap } from 'rxjs/operators';
import { DialogService } from './dialog.service';
import { ArmSystemDialogComponent } from '../_dialogs/arm-system-dialog/arm-system-dialog.component';
import { LightSwitchesResponse, SubDevicesResponse, LightSwitchesScenesResponse } from '../_models/lightswitches';

// FIXME proper declaration
declare var bootbox: any;

@Injectable()
export class LightSwitchesService {

  constructor(
    private apiService: ApiService,
    private permissionService: PermissionService,
    private notificationService: NotificationService,
    @Inject(I18NEXT_SERVICE) private translationService: ITranslationService,
    private dialogService: DialogService
  ) { }

  public getlightswitches(): Observable<LightSwitchesResponse> {
    return this.apiService.callApi<LightSwitchesResponse>('command', { param: 'getlightswitches' });
  }

  public getlightswitchesscenes(): Observable<LightSwitchesScenesResponse> {
    return this.apiService.callApi<LightSwitchesScenesResponse>('command', { param: 'getlightswitchesscenes' });
  }

  public getSubDevices(idx: string): Observable<SubDevicesResponse> {
    return this.apiService.callApi<SubDevicesResponse>('command', { param: 'getsubdevices', idx: idx });
  }

  public addSubDevice(idx: string, subidx: string): Observable<ApiResponse> {
    return this.apiService.callApi<ApiResponse>('command', { param: 'addsubdevice', idx: idx, subidx: subidx });
  }

  public deleteSubDevice(idx: string): Observable<ApiResponse> {
    return this.apiService.callApi<ApiResponse>('command', { param: 'deletesubdevice', idx: idx });
  }

  public deleteAllSubDevices(idx: string): Observable<ApiResponse> {
    return this.apiService.callApi<ApiResponse>('command', { param: 'deleteallsubdevices', idx: idx });
  }

  public SetDimValue(idx: string, value: number): Observable<ApiResponse> {
    if (this.permissionService.getRights() === 0) {
      this.notificationService.HideNotify();
      this.notificationService.ShowNotify(this.translationService.t('You do not have permission to do that!'), 2500, true);
      return;
    }

    return this.apiService.callApi<ApiResponse>('command', {
      param: 'switchlight',
      idx: idx,
      switchcmd: 'Set Level',
      level: value.toString()
    });
  }

  public SwitchSelectorLevelInt(idx: string, levelName: string, levelValue: string, passcode: string): Observable<ApiResponse> {
    // clearInterval($.myglobals.refreshTimer);

    this.notificationService.ShowNotify(this.translationService.t('Switching') + ' ' + levelName);

    return this.apiService.callApi<ApiResponse>('command', {
      param: 'switchlight',
      idx: idx,
      switchcmd: 'Set Level',
      level: levelValue,
      passcode: passcode
    });
  }

  public SwitchSelectorLevel(idx: string, levelName: string, levelValue: string, isprotected: boolean): Observable<any> | null {
    if (this.permissionService.getRights() === 0) {
      this.notificationService.HideNotify();
      this.notificationService.ShowNotify(this.translationService.t('You do not have permission to do that!'), 2500, true);
      return null;
    }

    let call: Observable<ApiResponse>;

    let passcode = '';
    if (typeof isprotected !== 'undefined') {
      if (isprotected === true) {
        bootbox.prompt(this.translationService.t('Please enter Password') + ':', (result) => {
          if (result === null) {
            return;
          } else {
            if (result === '') {
              return;
            } else {
              passcode = result;
              call = this.SwitchSelectorLevelInt(idx, levelName, levelValue, passcode);
            }
          }
        });
      } else {
        call = this.SwitchSelectorLevelInt(idx, levelName, levelValue, passcode);
      }
    } else {
      call = this.SwitchSelectorLevelInt(idx, levelName, levelValue, passcode);
    }

    if (call) {
      return call.pipe(
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
        catchError((err) => {
          this.notificationService.HideNotify();
          bootbox.alert(this.translationService.t('Problem sending switch command'));
          throw err;
        })
      );
    } else {
      return null;
    }
  }

  public SwitchModal(idx: string, name: string, status: string): Observable<any> {
    // clearInterval($.myglobals.refreshTimer);

    this.notificationService.ShowNotify(this.translationService.t('Setting Evohome ') + ' ' + this.translationService.t(name));

    // FIXME avoid conflicts when setting a new status while reading the status from the web gateway at the same time
    // (the status can flick back to the previous status after an update)...now implemented with script side lockout
    return this.apiService.callApi<ApiResponse>('command', { param: 'switchmodal', idx: idx, status: status, action: '1' }).pipe(
      tap((data: ApiResponse) => {
        if (data.status === 'ERROR') {
          this.notificationService.HideNotify();
          bootbox.alert(this.translationService.t('Problem sending switch command'));
        }
      }),
      delay(1000),
      tap(() => {
        this.notificationService.HideNotify();
      }),
      catchError((err) => {
        this.notificationService.HideNotify();
        // FIXME really an alert ??
        alert(this.translationService.t('Problem sending switch command'));
        throw err;
      })
    );
  }

  public SwitchLight(idx: string, switchcmd: string, isprotected?: boolean): Observable<any> | null {
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
            call = this.SwitchLightInt(idx, switchcmd, passcode);
          }
        });
      } else {
        call = this.SwitchLightInt(idx, switchcmd, passcode);
      }
    } else {
      call = this.SwitchLightInt(idx, switchcmd, passcode);
    }

    return call;
  }

  public SwitchLightInt(idx: string, switchcmd: string, passcode: string): Observable<any> {
    this.notificationService.ShowNotify(this.translationService.t('Switching') + ' ' + this.translationService.t(switchcmd));

    return this.apiService.callApi<ApiResponse>('command', {
      param: 'switchlight',
      idx: idx,
      switchcmd: switchcmd,
      level: '0',
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
      catchError((err) => {
        this.notificationService.HideNotify();
        bootbox.alert(this.translationService.t('Problem sending switch command'));
        throw err;
      })
    );
  }

  public ResetSecurityStatus(idx: string, switchcmd: string): Observable<any> | null {
    if (this.permissionService.getRights() === 0) {
      this.notificationService.HideNotify();
      this.notificationService.ShowNotify(this.translationService.t('You do not have permission to do that!'), 2500, true);
      return null;
    }

    this.notificationService.ShowNotify(this.translationService.t('Switching') + ' ' + this.translationService.t(switchcmd));

    return this.apiService.callApi<ApiResponse>('command', {
      param: 'resetsecuritystatus',
      idx: idx,
      switchcmd: switchcmd
    }).pipe(
      delay(1000),
      tap(() => this.notificationService.HideNotify()),
      catchError((err) => {
        this.notificationService.HideNotify();
        bootbox.alert(this.translationService.t('Problem sending switch command'));
        throw err;
      })
    );
  }

  public ArmSystem(idx: string, isprotected?: boolean): Observable<any> | null {
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
            call = this.ArmSystemInt(idx, passcode);
          }
        });
      } else {
        call = this.ArmSystemInt(idx, passcode);
      }
    } else {
      call = this.ArmSystemInt(idx, passcode);
    }

    return call;
  }

  private ArmSystemInt(idx: string, passcode: string): Observable<any> {

    const dialog = this.dialogService.addDialog(ArmSystemDialogComponent, { meiantech: false }).instance;

    dialog.init();
    dialog.open();

    return dialog.onArm.pipe(
      mergeMap(switchcmd => this.SendX10Command(idx, switchcmd, passcode))
    );
  }

  public ArmSystemMeiantech(idx: string, isprotected?: boolean): Observable<any> | null {
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
            call = this.ArmSystemMeiantechInt(idx, passcode);
          }
        });
      } else {
        call = this.ArmSystemMeiantechInt(idx, passcode);
      }
    } else {
      call = this.ArmSystemMeiantechInt(idx, passcode);
    }

    return call;
  }

  private ArmSystemMeiantechInt(idx: string, passcode: string): Observable<any> {
    const dialog = this.dialogService.addDialog(ArmSystemDialogComponent, { meiantech: true }).instance;

    dialog.init();
    dialog.open();

    return dialog.onArm.pipe(
      mergeMap(switchcmd => this.SendX10Command(idx, switchcmd, passcode))
    );
  }

  public SendX10Command(idx: string, switchcmd: string, passcode: string): Observable<ApiResponse> {
    this.notificationService.ShowNotify(this.translationService.t('Switching') + ' ' + this.translationService.t(switchcmd));

    return this.apiService.callApi<ApiResponse>('command', {
      param: 'switchlight',
      idx: idx,
      switchcmd: switchcmd,
      level: '0',
      passcode: passcode
    }).pipe(
      tap((data) => {
        if (data.status === 'ERROR') {
          this.notificationService.HideNotify();
          bootbox.alert(this.translationService.t(data.message));
        }
      }),
      delay(1000),
      tap(() => {
        this.notificationService.HideNotify();
      }),
      catchError((err) => {
        this.notificationService.HideNotify();
        bootbox.alert(this.translationService.t('Problem sending switch command'));
        throw err;
      })
    );
  }

  public mediaRemote(param: string, idx: string, action: string): Observable<any> {
    return this.apiService.callApi<any>('command', {
      param: param,
      idx: idx,
      action: action
    });
  }

  public brightnessUp(idx: string): Observable<ApiResponse> {
    return this.apiService.callApi<ApiResponse>('command', { param: 'brightnessup', idx: idx });
  }

  public brightnessDown(idx: string): Observable<ApiResponse> {
    return this.apiService.callApi<ApiResponse>('command', { param: 'brightnessdown', idx: idx });
  }

  public warmer(idx: string): Observable<ApiResponse> {
    return this.apiService.callApi<ApiResponse>('command', { param: 'warmer', idx: idx });
  }

  public cooler(idx: string): Observable<ApiResponse> {
    return this.apiService.callApi<ApiResponse>('command', { param: 'cooler', idx: idx });
  }

  public thermCmd(idx: string, switchcmd: string): Observable<ApiResponse> {
    return this.apiService.callApi<ApiResponse>('command', {
      param: 'switchlight',
      idx: idx,
      switchcmd: switchcmd,
      level: '0'
    }).pipe(
      tap((data) => {
        if (data.status === 'ERROR') {
          this.notificationService.HideNotify();
          bootbox.alert(this.translationService.t(data.message));
        }
      }),
      catchError((err) => {
        this.notificationService.HideNotify();
        bootbox.alert(this.translationService.t('Problem sending switch command'));
        throw err;
      })
    );
  }

}
