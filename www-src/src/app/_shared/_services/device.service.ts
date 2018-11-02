import {Observable, of} from 'rxjs';
import {ApiResponse} from '../_models/api';
import {ConfigService} from './config.service';
import {ApiService} from './api.service';
import {Inject, Injectable} from '@angular/core';
import {Transfers} from '../_models/transfers';
import {map, mergeMap, tap} from 'rxjs/operators';
import {TimesunService} from './timesun.service';
import {CostsResponse} from '../_models/costs';
import {Utils} from '../_utils/utils';
import {CustomLightIcon, CustomLightIconApi, CustomLightIconsResponse} from '../_models/custom-light-icons';
import {I18NEXT_SERVICE, ITranslationService} from 'angular-i18next';
import {HardwareResponse} from '../_models/hardware';
import {Learnsw} from '../_models/learn';
import {Router} from '@angular/router';
import {Device, DevicesResponse, DeviceValueOptionsResponse, ListDevicesResponse, Wording} from '../_models/device';
import {ApiHelperService} from './api-helper.service';

@Injectable()
export class DeviceService {

  private _customLightIcons: Array<CustomLightIconApi>;

  constructor(
    private apiService: ApiService,
    private configService: ConfigService,
    private timesunService: TimesunService,
    @Inject(I18NEXT_SERVICE) private translationService: ITranslationService,
    private router: Router,
    private apiHelperService: ApiHelperService
  ) {
  }

  public getDeviceInfo(deviceIdx: string): Observable<Device> {
    return this.apiService.callApi<DevicesResponse>('devices', {rid: deviceIdx}).pipe(
      tap(response => {
        this.timesunService.SetTimeAndSun(response);
      }),
      map(response => {
        if (response.result && response.result.length === 1) {
          // new Device(response.result[0]);
          return response.result[0];
        } else {
          throw new Error('Empty result from the API');
        }
      })
    );
  }

  public getTemperatures(lastPlanSelected: number, lastupdate?: number): Observable<DevicesResponse> {
    return this.getDevices('temp', true, '[Order]', lastupdate, lastPlanSelected);
  }

  public getDevices(filter: string,
                    used: boolean,
                    order: string,
                    lastUpdate?: number,
                    plan?: number,
                    favorite?: number,
                    floor?: string,
                    displayHidden?: boolean):
    Observable<DevicesResponse> {
    let additionalParams: any = {
      filter: filter,
      used: used.toString(),
      order: order
    };
    if (lastUpdate) {
      additionalParams = {...additionalParams, lastupdate: lastUpdate.toString()};
    }
    if (plan) {
      additionalParams = {...additionalParams, plan: plan.toString()};
    }
    if (favorite !== undefined) {
      additionalParams = {...additionalParams, favorite: favorite.toString()};
    }
    if (floor !== undefined) {
      additionalParams = {...additionalParams, floor: floor};
    }
    if (displayHidden === true) {
      additionalParams = {...additionalParams, displayhidden: '1'};
    }
    return this.apiService.callApi<DevicesResponse>('devices', additionalParams);
  }

  public getTextLog(deviceIdx: string): Observable<any> { // FIXME type
    return this.apiService.callApi<any>('textlog', {'idx': deviceIdx});
  }

  public getLightLog(deviceIdx: string): Observable<any> { // FIXME type
    return this.apiService.callApi<any>('lightlog', {'idx': deviceIdx});
  }

  public clearLightLog(deviceIdx: string): Observable<any> { // FIXME type
    return this.apiService.callApi<any>('command', {'param': 'clearlightlog', 'idx': deviceIdx});
  }

  public getCosts(devIdx: string): Observable<CostsResponse> {
    return this.apiService.callApi<CostsResponse>('command', {'param': 'getcosts', 'idx': devIdx});
  }

  public makeFavorite(id: number, isfavorite: number): Observable<ApiResponse> {
    return this.apiHelperService.checkUserPermissions().pipe(
      mergeMap(() => {
        return this.apiService.callApi<ApiResponse>('command', {
          param: 'makefavorite',
          idx: id.toString(),
          isfavorite: isfavorite.toString()
        });
      })
    );
  }

  public getTransfers(idx: string): Observable<Transfers> {
    return this.apiService.callApi<Transfers>('gettransfers', {idx: idx});
  }

  public transferDevice(devIdx: string, newDeviceIdx: string): Observable<ApiResponse> {
    return this.apiService.callApi<ApiResponse>('transferdevice', {idx: devIdx, newidx: newDeviceIdx});
  }

  public updateDevice(idx: string, devicename: string, devicedescription: string, addjvalue?: number): Observable<ApiResponse> {
    const additionalParams: any = {idx: idx, name: devicename, description: devicedescription};
    if (addjvalue !== undefined) {
      additionalParams.addjvalue = addjvalue.toString();
    }
    additionalParams.used = 'true';
    return this.apiService.callApi<ApiResponse>('setused', additionalParams);
  }

  public updateSetPoint(idx: string, devicename: string, devicedescription: string, setpoint: number, mode: string, until: string)
    : Observable<ApiResponse> {
    return this.apiService.callApi<ApiResponse>('setused', {
      idx: idx,
      name: devicename,
      description: devicedescription,
      setpoint: setpoint.toString(),
      mode: mode,
      until: until,
      used: 'true'
    });
  }

  public updateState(idx: string, devicename: string, devicedescription: string, state: string, mode: string, until: string)
    : Observable<ApiResponse> {
    return this.apiService.callApi<ApiResponse>('setused', {
      idx: idx, name: devicename, description: devicedescription, state: state, mode: mode, until: until, used: 'true'
    });
  }

  public updateSetPointAuto(idx: string, devicename: string, devicedescription: string, setpoint: number): Observable<ApiResponse> {
    return this.apiService.callApi<ApiResponse>('setused', {
      idx: idx,
      name: devicename,
      description: devicedescription,
      setpoint: setpoint.toString(),
      modeAuto: '',
      used: 'true'
    });
  }

  public updateRainDevice(idx: string, devicename: string, devicedescription: string, addjmulti?: number): Observable<ApiResponse> {
    const additionalParams: any = {idx: idx, name: devicename, description: devicedescription};
    if (addjmulti !== undefined) {
      additionalParams.addjmulti = addjmulti.toString();
    }
    additionalParams.used = 'true';
    return this.apiService.callApi<ApiResponse>('setused', additionalParams);
  }

  public updateVisibilityDevice(idx: string, devicename: string, devicedescription: string, switchtype?: number): Observable<ApiResponse> {
    const additionalParams: any = {idx: idx, name: devicename, description: devicedescription};
    if (switchtype !== undefined) {
      additionalParams.switchtype = switchtype.toString();
    }
    additionalParams.used = 'true';
    return this.apiService.callApi<ApiResponse>('setused', additionalParams);
  }

  updateBaroDevice(idx: string, devicename: string, devicedescription: string, addjvalue2?: number): Observable<ApiResponse> {
    const additionalParams: any = {idx: idx, name: devicename, description: devicedescription};
    if (addjvalue2 !== undefined) {
      additionalParams.addjvalue2 = addjvalue2.toString();
    }
    additionalParams.used = 'true';
    return this.apiService.callApi<ApiResponse>('setused', additionalParams);
  }

  updateMeterDevice(idx: string,
                    devicename: string,
                    devicedescription: string,
                    switchtype: number,
                    addjvalue?: number,
                    addjvalue2?: number,
                    options?: string): Observable<ApiResponse> {
    const additionalParams: any = {idx: idx, name: devicename, description: devicedescription};
    if (switchtype !== undefined) {
      additionalParams.switchtype = switchtype.toString();
    }
    if (addjvalue !== undefined) {
      additionalParams.addjvalue = addjvalue.toString();
    }
    if (addjvalue2 !== undefined) {
      additionalParams.addjvalue2 = addjvalue2.toString();
    }
    if (options !== undefined) {
      const opts = Utils.b64EncodeUnicode(options);
      additionalParams.options = opts;
    }
    additionalParams.used = 'true';
    return this.apiService.callApi<ApiResponse>('setused', additionalParams);
  }

  updateEnergyDevice(idx: string,
                     devicename: string,
                     devicedescription: string,
                     switchtype: number,
                     EnergyMeterMode: string): Observable<ApiResponse> {
    const additionalParams: any = {idx: idx, name: devicename, description: devicedescription};
    if (switchtype !== undefined) {
      additionalParams.switchtype = switchtype.toString();
    }
    if (EnergyMeterMode !== undefined) {
      additionalParams.EnergyMeterMode = EnergyMeterMode;
    }
    additionalParams.used = 'true';
    return this.apiService.callApi<ApiResponse>('setused', additionalParams);
  }

  updateCustomSensorDevice(idx: string,
                           devicename: string,
                           devicedescription: string,
                           customimage: number,
                           devoptions: string): Observable<ApiResponse> {
    const additionalParams: any = {idx: idx, name: devicename, description: devicedescription};
    additionalParams.switchtype = '0';
    if (customimage !== undefined) {
      additionalParams.customimage = customimage;
    }
    if (devoptions !== undefined) {
      additionalParams.devoptions = devoptions;
    }
    additionalParams.used = 'true';
    return this.apiService.callApi<ApiResponse>('setused', additionalParams);
  }

  updateDistanceDevice(idx: string, devicename: string, devicedescription: string, switchtype?: number): Observable<ApiResponse> {
    const additionalParams: any = {idx: idx, name: devicename, description: devicedescription};
    if (switchtype !== undefined) {
      additionalParams.switchtype = switchtype.toString();
    }
    additionalParams.used = 'true';
    return this.apiService.callApi<ApiResponse>('setused', additionalParams);
  }

  updateThermostatClockDevice(idx: string, devicename: string, devicedescription: string,
                              clock: string, isprotected: boolean): Observable<ApiResponse> {
    const additionalParams: any = {idx: idx, name: devicename, description: devicedescription};
    if (clock !== undefined) {
      additionalParams.clock = clock;
    }
    if (isprotected !== undefined) {
      additionalParams.protected = isprotected.toString();
    }
    additionalParams.used = 'true';
    return this.apiService.callApi<ApiResponse>('setused', additionalParams);
  }

  updateThermostatModeDevice(idx: string, devicename: string, devicedescription: string,
                             isFan: boolean, mode: any, isprotected: boolean): Observable<ApiResponse> {
    const additionalParams: any = {idx: idx, name: devicename, description: devicedescription};
    if (!isFan) {
      additionalParams.tmode = mode;
    } else {
      additionalParams.fmode = mode;
    }
    if (isprotected !== undefined) {
      additionalParams.protected = isprotected.toString();
    }
    additionalParams.used = 'true';
    return this.apiService.callApi<ApiResponse>('setused', additionalParams);
  }

  updateSetPointDevice(idx: string, devicename: string, devicedescription: string,
                       setpoint: number, isprotected: boolean): Observable<ApiResponse> {
    const additionalParams: any = {idx: idx, name: devicename, description: devicedescription};
    if (setpoint !== undefined) {
      additionalParams.setpoint = setpoint.toString();
    }
    if (isprotected !== undefined) {
      additionalParams.protected = isprotected.toString();
    }
    additionalParams.used = 'true';
    return this.apiService.callApi<ApiResponse>('setused', additionalParams);
  }

  public updateLightDevice(idx: string, devicename: string, switchtype?: number, maindeviceidx?: string): Observable<ApiResponse> {
    const additionalParams: any = {idx: idx, name: devicename};
    if (switchtype !== undefined) {
      additionalParams.switchtype = switchtype.toString();
    }
    if (maindeviceidx !== undefined) {
      additionalParams.maindeviceidx = maindeviceidx;
    }
    additionalParams.used = 'true';
    return this.apiService.callApi<ApiResponse>('setused', additionalParams);
  }

  public removeDevice(idx: string, devicename: string, devicedescription: string): Observable<ApiResponse> {
    return this.apiService.callApi<ApiResponse>('setused', {
      idx: idx, name: devicename, description: devicedescription, used: 'false'
    });
  }

  public removeDeviceAndSubDevices(deviceIdx: string | Array<string>): Observable<ApiResponse> {
    return this.apiService.callApi('deletedevice', {
      idx: Array.isArray(deviceIdx) ? deviceIdx.join(';') : deviceIdx
    });
  }

  public removeScene(deviceIdx: string | Array<string>): Observable<ApiResponse> {
    return this.apiService.callApi('deletescene', {
      idx: Array.isArray(deviceIdx) ? deviceIdx.join(';') : deviceIdx
    });
  }

  public updateDeviceInfo(deviceIdx: string, data: { [param: string]: string | string[]; }): Observable<ApiResponse> {
    return this.apiService.callApi('setused', Object.assign({}, data, {
      idx: deviceIdx
    }));
  }

  public switchDeviceOrder(idx1: string, idx2: string, roomId?: number): Observable<ApiResponse> {
    let params: any = {
      param: 'switchdeviceorder',
      idx1: idx1,
      idx2: idx2
    };
    if (roomId !== undefined) {
      params = {...params, roomid: roomId.toString()};
    }
    return this.apiService.callApi<ApiResponse>('command', params);
  }

  // FIXME make it a pipe
  public GetTemp48Item(temp: number) {
    if (this.configService.config.TempSign === 'C') {
      if (temp <= 0) {
        return 'ice.png';
      }
      if (temp < 5) {
        return 'temp-0-5.png';
      }
      if (temp < 10) {
        return 'temp-5-10.png';
      }
      if (temp < 15) {
        return 'temp-10-15.png';
      }
      if (temp < 20) {
        return 'temp-15-20.png';
      }
      if (temp < 25) {
        return 'temp-20-25.png';
      }
      if (temp < 30) {
        return 'temp-25-30.png';
      }
      return 'temp-gt-30.png';
    } else {
      if (temp <= 32) {
        return 'ice.png';
      }
      if (temp < 41) {
        return 'temp-0-5.png';
      }
      if (temp < 50) {
        return 'temp-5-10.png';
      }
      if (temp < 59) {
        return 'temp-10-15.png';
      }
      if (temp < 68) {
        return 'temp-15-20.png';
      }
      if (temp < 77) {
        return 'temp-20-25.png';
      }
      if (temp < 86) {
        return 'temp-25-30.png';
      }
      return 'temp-gt-30.png';
    }
  }

  private _getCustomLightIcons(): Observable<Array<CustomLightIconApi>> {
    if (!this._customLightIcons) {
      return this.apiService.callApi<CustomLightIconsResponse>('custom_light_icons', {}).pipe(
        map(response => response.result),
        // Store for cache
        tap(lighticons => this._customLightIcons = lighticons)
      );
    } else {
      return of(this._customLightIcons);
    }
  }

  public getCustomLightIcons(): Observable<Array<CustomLightIcon>> {
    return this._getCustomLightIcons().pipe(
      map(lighticons => {
        const ddData: Array<CustomLightIcon> = [];
        if (typeof lighticons !== 'undefined') {
          // const totalItems = response.result.length;
          lighticons.forEach((item: CustomLightIconApi, i: number) => {
            let bSelected = false;
            if (i === 0) {
              bSelected = true;
            }
            let itext = item.text;
            let idescription = item.description;

            let img = 'images/';
            if (item.idx === 0) {
              img += 'Custom';
              itext = 'Custom';
              idescription = 'Custom Sensor';
            } else {
              img += item.imageSrc;
            }
            img += '48_On.png';
            ddData.push({text: itext, value: item.idx, selected: bSelected, description: idescription, imageSrc: img});
          });
        }
        return ddData;
      })
    );
  }

  public getCustomLightIconsForDeviceIconSelect(): Observable<Array<CustomLightIcon>> {
    return this._getCustomLightIcons().pipe(
      map(lighticons => {
        return (lighticons || [])
          .filter((item) => {
            return item.idx !== 0;
          })
          .map((item) => {
            return {
              text: item.text,
              value: item.idx,
              selected: false,
              description: item.description,
              imageSrc: 'images/' + item.imageSrc + '48_On.png'
            };
          });
      })
    );
  }

  public TranslateStatusShort(status?: string) {
    if (typeof status === 'undefined') {
      return '-?-';
    }

    // will remove the Set Level
    if (status.indexOf('Set Level') !== -1) {
      if (status.substring(11) === '100 %') {
        return 'On';
      } else {
        return status.substring(11);
      }
    } else {
      return this.translationService.t(status);
    }
  }

  public TranslateStatus(status: string) {
    // should of course be changed, but for now a quick solution
    if (status.indexOf('Set Level') !== -1) {
      return status.replace('Set Level', this.translationService.t('Set Level'));
    } else {
      return this.translationService.t(status);
    }
  }

  addSwitch(mParams: { [param: string]: string | string[]; }): Observable<ApiResponse> {
    return this.apiService.callApi<ApiResponse>('command', {...mParams, param: 'addswitch'});
  }

  testSwitch(mParams: { [param: string]: string | string[]; }): Observable<ApiResponse> {
    return this.apiService.callApi<ApiResponse>('command', {...mParams, param: 'testswitch'});
  }

  getManualHardware(): Observable<HardwareResponse> {
    return this.apiService.callApi<HardwareResponse>('command', {param: 'getmanualhardware'});
  }

  getGpio(): Observable<HardwareResponse> {
    return this.apiService.callApi<HardwareResponse>('command', {param: 'getgpio'});
  }

  getSysfsGpio(): Observable<HardwareResponse> {
    return this.apiService.callApi<HardwareResponse>('command', {param: 'getsysfsgpio'});
  }

  learnsw(): Observable<Learnsw> {
    return this.apiService.callApi<Learnsw>('command', {param: 'learnsw'});
  }

  openCustomLog(t: Device) {
    if (t.Direction !== undefined) {
      this.router.navigate(['/WindLog', t.idx]);
    } else if (t.UVI !== undefined) {
      this.router.navigate(['/UVLog', t.idx]);
    } else if (t.Rain !== undefined) {
      this.router.navigate(['/RainLog', t.idx]);
    } else if (t.Type.indexOf('Current') === 0) {
      this.router.navigate(['/CurrentLog', t.idx]);
    } else if (t.Type === 'Air Quality') {
      this.router.navigate(['/AirQualityLog', t.idx]);
    } else if (t.SubType === 'Barometer') {
      this.router.navigate(['/BaroLog', t.idx]);
    }
  }

  public includeDevice(deviceIdx: string, name: string, mainDeviceIdx?: string): Observable<ApiResponse> {
    return this.apiService.callApi('setused', {
      idx: deviceIdx,
      name: name,
      used: 'true',
      maindeviceidx: mainDeviceIdx || ''
    });
  }

  public excludeDevice(deviceIdx: string): Observable<ApiResponse> {
    return this.apiService.callApi('command', {
      param: 'setunused',
      idx: deviceIdx,
    });
  }

  public disableDevice(deviceIdx: string): Observable<ApiResponse> {
    return this.excludeDevice(deviceIdx);
  }

  public getAllDevices(): Observable<DevicesResponse> {
    return this.apiService.callApi('devices', {
      displayhidden: '1',
      filter: 'all',
      used: 'all'
    });
  }

  renameDevice(idx: string, name: string): Observable<ApiResponse> {
    return this.apiService.callApi('command', {
      param: 'renamedevice',
      idx: idx,
      name: name
    });
  }

  listDevices(): Observable<ListDevicesResponse> {
    return this.apiService.callApi('command', {
      param: 'devices_list'
    });
  }

  listOnOffDevices(): Observable<ListDevicesResponse> {
    return this.apiService.callApi('command', {
      param: 'devices_list_onoff'
    });
  }

  getDeviceValueOptionWording(idx: string, pos: string): Observable<Wording> {
    return this.apiService.callApi('command', {param: 'getdevicevalueoptionwording', idx: idx, pos: pos});
  }

  getDeviceValueOptions(deviceid: string): Observable<DeviceValueOptionsResponse> {
    return this.apiService.callApi('command', {param: 'getdevicevalueoptions', idx: deviceid});
  }

}
