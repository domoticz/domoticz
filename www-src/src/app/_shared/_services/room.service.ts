import {Observable} from 'rxjs';
import {ApiService} from './api.service';
import {Injectable} from '@angular/core';
import {
  Plan,
  PlanDevice,
  PlanDevicesResponse,
  Plans,
  UnusedPlanDevice,
  UnusedPlanDevicesResponse
} from '../_models/plan';
import {ApiResponse} from '../_models/api';
import {ApiHelperService} from './api-helper.service';
import {map, mergeMap} from 'rxjs/operators';

@Injectable()
export class RoomService {

  constructor(private apiService: ApiService,
              private apiHelperService: ApiHelperService) {
  }

  public getPlans(): Observable<Array<Plan>> {
    return this.apiService.callApi<Plans>('plans', {}).pipe(
      map(response => response.result || [])
    );
  }

  public getPlansWithoutHidden(): Observable<Array<Plan>> {
    return this.apiService.callApi<Plans>('plans', {displayhidden: '0'}).pipe(
      map(response => response.result || [])
    );
  }

  public getPlansWithHidden(): Observable<Array<Plan>> {
    return this.apiService.callApi<Plans>('plans', {displayhidden: '1'}).pipe(
      map(response => response.result || [])
    );
  }

  getPlanAvailableDevices(): Observable<Array<UnusedPlanDevice>> {
    return this.apiService.callApi<UnusedPlanDevicesResponse>('command', {
      param: 'getunusedplandevices',
      unique: 'false'
    }).pipe(
      map(response => response.result || [])
    );
  }

  changePlanOrder(order: number, planid: string): Observable<ApiResponse> {
    return this.apiHelperService.checkAdminPermissions().pipe(
      mergeMap(() => {
        return this.apiService.callApi<ApiResponse>('command', {
          param: 'changeplanorder',
          idx: planid,
          way: order.toString()
        });
      })
    );
  }

  getPlanDevices(idx: string): Observable<Array<PlanDevice>> {
    return this.apiService.callApi<PlanDevicesResponse>('command', {param: 'getplandevices', idx: idx}).pipe(
      map(response => {
        return response.result || [];
      })
    );
  }

  changeDeviceOrder(order: number, planid: string, devid: string): Observable<ApiResponse> {
    return this.apiHelperService.checkAdminPermissions().pipe(
      mergeMap(() => {
        return this.apiService.callApi<ApiResponse>('command', {
          param: 'changeplandeviceorder',
          planid: planid,
          idx: devid,
          way: order.toString()
        });
      })
    );
  }

  removeDeviceFromPlan(idx: string): Observable<ApiResponse> {
    return this.apiHelperService.checkAdminPermissions().pipe(
      mergeMap(() => {
        return this.apiService.callApi<ApiResponse>('command', {param: 'deleteplandevice', idx: idx});
      })
    );
  }

  removePlan(idx: string): Observable<ApiResponse> {
    return this.apiHelperService.checkAdminPermissions().pipe(
      mergeMap(() => {
        return this.apiService.callApi<ApiResponse>('command', {param: 'deleteplan', idx: idx});
      })
    );
  }

  removeAllDevicesFromPlan(planId: string): Observable<ApiResponse> {
    return this.apiHelperService.checkAdminPermissions().pipe(
      mergeMap(() => {
        return this.apiService.callApi<ApiResponse>('command', {param: 'deleteallplandevices', idx: planId});
      })
    );
  }

  addDeviceToPlan(planId: string, deviceId: string, isScene: boolean): Observable<ApiResponse> {
    return this.apiHelperService.checkAdminPermissions().pipe(
      mergeMap(() => {
        return this.apiService.callApi<ApiResponse>('command', {
          param: 'addplanactivedevice',
          idx: planId,
          activetype: isScene ? '1' : '0',
          activeidx: deviceId
        });
      })
    );
  }

  addPlan(name: string): Observable<ApiResponse> {
    return this.apiHelperService.checkAdminPermissions().pipe(
      mergeMap(() => {
        return this.apiService.callApi<ApiResponse>('command', {param: 'addplan', name: name});
      })
    );
  }

  updatePlan(idx: string, name: string): Observable<ApiResponse> {
    return this.apiHelperService.checkAdminPermissions().pipe(
      mergeMap(() => {
        return this.apiService.callApi<ApiResponse>('command', {param: 'updateplan', idx: idx, name: name});
      })
    );
  }

  updateTimerPlan(idx: string, name: string): Observable<ApiResponse> {
    return this.apiService.callApi('command', {param: 'updatetimerplan', idx: idx, name: name});
  }

  duplicateTimerPlan(idx: string, name: string): Observable<ApiResponse> {
    return this.apiService.callApi('command', {param: 'duplicatetimerplan', idx: idx, name: name});
  }

  deleteTimerPlan(idx: string): Observable<ApiResponse> {
    return this.apiService.callApi('command', {param: 'deletetimerplan', idx: idx});
  }

  addTimerPlan(name: string): Observable<ApiResponse> {
    return this.apiService.callApi('command', {param: 'addtimerplan', name: name});
  }
}
