import { Injectable } from '@angular/core';
import { ApiService } from './api.service';
import {
  HardwareTypesResponse, HardwareResponse, SerialDevicesResponse, Hardware,
  RegisterHueResponse, FirmwareUpdatePercentageResponse, LmsNodesResponse, EvoHomeResponse, ZWaveNodesResponse
} from '../_models/hardware';
import { Observable } from 'rxjs';
import { ApiResponse } from '../_models/api';
import { map } from 'rxjs/operators';

@Injectable()
export class HardwareService {

  constructor(
    private apiService: ApiService
  ) { }

  public getHardwares(): Observable<HardwareResponse> {
    return this.apiService.callApi<HardwareResponse>('hardware', {});
  }

  public getHardware(idx: string): Observable<Hardware | null> {
    // FIXME use an existing resource or create one for getting a single hardware
    return this.getHardwares().pipe(
      map(response => {
        return response.result.find(_ => _.idx as string === idx);
      })
    );
  }

  public getHardwareTypes(): Observable<HardwareTypesResponse> {
    return this.apiService.callApi<HardwareTypesResponse>('command', { param: 'gethardwaretypes' });
  }

  public getSerialDevices(): Observable<SerialDevicesResponse> {
    return this.apiService.callApi<SerialDevicesResponse>('command', { param: 'serial_devices' });
  }

  public deleteHardware(idx: number): Observable<ApiResponse> {
    return this.apiService.callApi<ApiResponse>('command', { param: 'deletehardware', idx: idx.toString() });
  }

  public updateHardware(htype: number, idx: string, name: string, Mode1: string, Mode2: string, Mode3: string, Mode4: string, Mode5: string,
    Mode6: string, extra: string, enabled: boolean, datatimeout: number, username?: string, password?: string, address?: string,
    port?: string, serialport?: string): Observable<ApiResponse> {
    let params: { [param: string]: string } = {
      param: 'updatehardware',
      htype: htype.toString(),
      idx: idx,
      name: name,
      Mode1: Mode1,
      Mode2: Mode2,
      Mode3: Mode3,
      Mode4: Mode4,
      Mode5: Mode5,
      Mode6: Mode6,
      enabled: enabled.toString(),
      datatimeout: datatimeout.toString()
    };
    if (extra) {
      params = { ...params, extra: extra };
    }
    if (username) {
      params = { ...params, username: username };
    }
    if (password) {
      params = { ...params, password: password };
    }
    if (address) {
      params = { ...params, address: address };
    }
    if (port) {
      params = { ...params, port: port };
    }
    if (serialport) {
      params = { ...params, serialport: serialport };
    }
    return this.apiService.callApi('command', params);
  }

  addHardware(htype: string, enabled: boolean, datatimeout: number, name: string,
    Mode1: string, Mode2: string, Mode3: string, Mode4: string,
    Mode5: string, Mode6: string, extra?: string, username?: string, password?: string,
    address?: string, port?: string, serialport?: string): Observable<ApiResponse> {
    let params: { [param: string]: string } = {
      param: 'addhardware',
      htype: htype,
      name: name,
      Mode1: Mode1,
      Mode2: Mode2,
      Mode3: Mode3,
      Mode4: Mode4,
      Mode5: Mode5,
      Mode6: Mode6,
      enabled: enabled.toString(),
      datatimeout: datatimeout.toString()
    };
    if (extra) {
      params = { ...params, extra: extra };
    }
    if (username) {
      params = { ...params, username: username };
    }
    if (password) {
      params = { ...params, password: password };
    }
    if (address) {
      params = { ...params, address: address };
    }
    if (port) {
      params = { ...params, port: port };
    }
    if (serialport) {
      params = { ...params, serialport: serialport };
    }
    return this.apiService.callApi('command', params);
  }

  registerHue(address: string, port: string, username: string): Observable<RegisterHueResponse> {
    return this.apiService.callApi<RegisterHueResponse>('command', {
      param: 'registerhue', ipaddress: address, port: port, username: username
    });
  }

  setRFXCOMMode(settings: FormData): Observable<ApiResponse> {
    return this.apiService.postFormData('setrfxcommode.webem', settings);
  }

  setS0MeterType(settings: FormData): Observable<ApiResponse> {
    return this.apiService.postFormData('sets0metertype.webem', settings);
  }

  setLimitlessType(settings: FormData): Observable<ApiResponse> {
    return this.apiService.postFormData('setlimitlesstype.webem', settings);
  }

  importOldDataSbfSpot(settings: FormData): Observable<ApiResponse> {
    return this.apiService.postFormData('sbfspotimportolddata.webem', settings);
  }

  setOpenThermSettings(settings: FormData): Observable<ApiResponse> {
    return this.apiService.postFormData('setopenthermsettings.webem', settings);
  }

  upgradeFirmware(idx: string, file: string | Blob): Observable<ApiResponse> {
    const formData = new FormData();
    formData.append('hardwareid', idx);
    formData.append('firmwarefile', file);
    return this.apiService.postFormData('rfxupgradefirmware.webem', formData);
  }

  getRfxFirmwareUpdatePercentage(idx: string): Observable<FirmwareUpdatePercentageResponse> {
    return this.apiService.callApi<FirmwareUpdatePercentageResponse>('command', {
      param: 'rfxfirmwaregetpercentage',
      hardwareid: idx
    });
  }

  sendOpenThermCommand(idx: string, command: string): Observable<ApiResponse> {
    return this.apiService.callApi('command', {
      param: 'sendopenthermcommand',
      idx: idx,
      cmnd: command
    });
  }

  getLmsNodes(idx: string): Observable<LmsNodesResponse> {
    return this.apiService.callApi('command', {
      param: 'lmsgetnodes',
      idx: idx
    });
  }

  setLmsMode(idx: string, mode1: number): Observable<ApiResponse> {
    return this.apiService.callApi('command', {
      param: 'lmssetmode',
      idx: idx,
      mode1: mode1.toString()
    });
  }

  deleteLmsUnusedDevices(idx: string): Observable<ApiResponse> {
    return this.apiService.callApi('command', {
      param: 'lmsdeleteunuseddevices',
      idx: idx,
    });
  }

  createDevice(idx: string, SensorName: string, SensorType: string, sensoroptions?: string): Observable<ApiResponse> {
    let params: any = {
      idx: idx,
      sensorname: SensorName,
      sensormappedtype: SensorType,
    };
    if (sensoroptions) {
      params = { ...params, sensoroptions: sensoroptions };
    }
    return this.apiService.callApi('createdevice', params);
  }

  addYeeLight(idx: string, name: string, ipAddress: string, sensorType: string): Observable<ApiResponse> {
    return this.apiService.callApi('command', {
      param: 'addyeelight',
      idx: idx,
      name: name,
      ipaddress: ipAddress,
      stype: sensorType
    });
  }

  ReloadPiFace(idx: string): Observable<ApiResponse> {
    return this.apiService.post('reloadpiface.webem', { idx: idx });
  }

  createRfLinkDevice(idx: string, SensorName: string): Observable<ApiResponse> {
    return this.apiService.callApi('createrflinkdevice', {
      idx: idx,
      command: SensorName
    });
  }

  createEvohomeSensor(idx: string, SensorType: string): Observable<ApiResponse> {
    return this.apiService.callApi('createevohomesensor', {
      idx: idx,
      sensortype: SensorType
    });
  }

  bindEvoHome(idx: string, devtype: string): Observable<EvoHomeResponse> {
    return this.apiService.callApi('bindevohome', {
      idx: idx,
      devtype: devtype
    });
  }

  setRego6XXType(formData: FormData): Observable<ApiResponse> {
    return this.apiService.postFormData('setrego6xxtype.webem', formData);
  }

  setCCUSBType(formData: FormData): Observable<ApiResponse> {
    return this.apiService.postFormData('setcurrentcostmetertype.webem', formData);
  }

  tellstickApplySettings(idx: string, repeats: number, repeatInterval: number): Observable<ApiResponse> {
    return this.apiService.callApi('command', {
      param: 'tellstickApplySettings',
      idx: idx,
      repeats: repeats.toString(),
      repeatInterval: repeatInterval.toString()
    });
  }

  addArilux(idx: string, SensorName: string, IPAddress: string, SensorType: string): Observable<ApiResponse> {
    return this.apiService.callApi('command', {
      param: 'addArilux',
      idx: idx,
      name: SensorName,
      ipaddress: IPAddress,
      stype: SensorType
    });
  }

  getOpenZWaveNodes(idx: string): Observable<ZWaveNodesResponse> {
    return this.apiService.callApi<ZWaveNodesResponse>('openzwavenodes', {
      idx: idx
    });
  }

}
