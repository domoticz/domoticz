import { Injectable } from '@angular/core';
import { ApiService } from '../../_shared/_services/api.service';
import { Observable } from 'rxjs';
import { ApiResponse } from '../../_shared/_models/api';
import {
  ZWaveIncludedNodeResponse, ZWaveExcludedNodeResponse, ZWaveUserCodesResponse,
  ZWaveNetworkInfoResponse, ZWaveGroupInfoResponse
} from './zwave';

@Injectable()
export class ZwaveService {

  constructor(
    private apiService: ApiService
  ) { }

  heal(idx: string, node: string): Observable<ApiResponse> {
    return this.apiService.callApi('command', {
      param: 'zwavenodeheal',
      idx: idx,
      node: node
    });
  }

  include(idx: string, isSecure: boolean): Observable<ApiResponse> {
    return this.apiService.callApi('command', {
      param: 'zwaveinclude',
      idx: idx,
      secure: isSecure.toString()
    });
  }

  isIncluded(idx: string): Observable<ZWaveIncludedNodeResponse> {
    return this.apiService.callApi('command', {
      param: 'zwaveisnodeincluded',
      idx: idx
    });
  }

  exclude(idx: string): Observable<ZWaveExcludedNodeResponse> {
    return this.apiService.callApi('command', {
      param: 'zwaveexclude',
      idx: idx
    });
  }

  isExcluded(idx: string): Observable<ZWaveExcludedNodeResponse> {
    return this.apiService.callApi('command', {
      param: 'zwaveisnodeexcluded',
      idx: idx
    });
  }

  checkState(idx: string): Observable<ApiResponse> {
    return this.apiService.callApi('command', {
      param: 'zwavestatecheck',
      idx: idx
    });
  }

  softReset(idx: string): Observable<ApiResponse> {
    return this.apiService.callApi('command', {
      param: 'zwavesoftreset',
      idx: idx
    });
  }

  hardReset(idx: string): Observable<ApiResponse> {
    return this.apiService.callApi('command', {
      param: 'zwavehardreset',
      idx: idx
    });
  }

  receiveConfigurationFromOtherController(idx: string): Observable<ApiResponse> {
    return this.apiService.callApi('command', {
      param: 'zwavereceiveconfigurationfromothercontroller',
      idx: idx
    });
  }

  sendConfigurationToSecondController(idx: string): Observable<ApiResponse> {
    return this.apiService.callApi('command', {
      param: 'zwavesendconfigurationtosecondcontroller',
      idx: idx
    });
  }

  transferPrimaryRole(idx: string): Observable<ApiResponse> {
    return this.apiService.callApi('command', {
      param: 'zwavetransferprimaryrole',
      idx: idx
    });
  }

  startUserCodeEnrollment(idx: string): Observable<ApiResponse> {
    return this.apiService.callApi('command', {
      param: 'zwavestartusercodeenrollmentmode',
      idx: idx
    });
  }

  deleteNode(idx: string): Observable<ApiResponse> {
    return this.apiService.callApi('command', {
      param: 'deletezwavenode',
      idx: idx
    });
  }

  updateNode(idx: string, name: string, EnablePolling: boolean): Observable<ApiResponse> {
    return this.apiService.callApi('command', {
      param: 'updatezwavenode',
      idx: idx,
      name: name,
      EnablePolling: EnablePolling.toString()
    });
  }

  requestNodeConfig(idx: string): Observable<ApiResponse> {
    return this.apiService.callApi('command', {
      param: 'requestzwavenodeconfig',
      idx: idx
    });
  }

  applyNodeConfig(idx: string, valueList: string): Observable<ApiResponse> {
    return this.apiService.callApi('command', {
      param: 'applyzwavenodeconfig',
      idx: idx,
      valuelist: valueList
    });
  }

  getUserCodes(nodeidx: string): Observable<ZWaveUserCodesResponse> {
    return this.apiService.callApi('command', {
      param: 'zwavegetusercodes',
      idx: nodeidx
    });
  }

  removeUserCode(nodeidx: string, index: string): Observable<ApiResponse> {
    return this.apiService.callApi('command', {
      param: 'zwaveremoveusercode',
      idx: nodeidx,
      codeindex: index
    });
  }

  networkHeal(idx: string): Observable<ApiResponse> {
    return this.apiService.callApi('command', {
      param: 'zwavenetworkheal',
      idx: idx
    });
  }

  cancel(idx: string): Observable<ApiResponse> {
    return this.apiService.callApi('command', {
      param: 'zwavecancel',
      idx: idx
    });
  }

  getNetworkInfo(idx: string): Observable<ZWaveNetworkInfoResponse> {
    return this.apiService.callApi('command', {
      param: 'zwavenetworkinfo',
      idx: idx
    });
  }

  getGroupInfo(idx: string): Observable<ZWaveGroupInfoResponse> {
    return this.apiService.callApi('command', {
      param: 'zwavegroupinfo',
      idx: idx
    });
  }

  removeGroupNode(hwid: string, nodeID: number, groupId: number, removeNodeId: number): Observable<ApiResponse> {
    return this.apiService.callApi('command', {
      param: 'zwaveremovegroupnode',
      idx: hwid,
      node: nodeID.toString(),
      group: groupId.toString(),
      removenode: removeNodeId.toString()
    });
  }

  addGroupNode(hwid: string, nodeID: number, groupId: number, addnode: boolean): Observable<ApiResponse> {
    return this.apiService.callApi('command', {
      param: 'zwaveaddgroupnode',
      idx: hwid,
      node: nodeID.toString(),
      group: groupId.toString(),
      addnode: addnode.toString()
    });
  }

  requestNodeInfo(idx: string): Observable<ApiResponse> {
    return this.apiService.callApi('command', { param: 'requestzwavenodeinfo', idx: idx });
  }

}
