import { Injectable } from '@angular/core';
import { ApiService } from './api.service';
import { UsersResponse, UserDevicesResponse } from '../_models/users';
import { Observable } from 'rxjs';
import { ApiResponse } from '../_models/api';

@Injectable()
export class UsersService {

  constructor(private apiService: ApiService) { }

  getUsers(): Observable<UsersResponse> {
    return this.apiService.callApi<UsersResponse>('users', {});
  }

  getSharedUserDevices(idx: string): Observable<UserDevicesResponse> {
    return this.apiService.callApi('getshareduserdevices', { idx: idx });
  }

  setSharedUserDevices(idx: string, selecteddevices: string): Observable<ApiResponse> {
    return this.apiService.callApi('setshareduserdevices', { idx: idx, devices: selecteddevices });
  }

  deleteUser(idx: string): Observable<ApiResponse> {
    return this.apiService.callApi('command', { param: 'deleteuser', idx: idx });
  }

  addUser(bEnabled: boolean, username: string, password: string, rights: number,
    bEnableSharing: boolean, TabsEnabled: number): Observable<ApiResponse> {
    return this.apiService.callApi('command', {
      param: 'adduser',
      enabled: bEnabled.toString(),
      username: username,
      password: password,
      rights: rights.toString(),
      RemoteSharing: bEnableSharing.toString(),
      TabsEnabled: TabsEnabled.toString()
    });
  }

  updateUser(idx: string, bEnabled: boolean, username: string, password: string, rights: number,
    bEnableSharing: boolean, TabsEnabled: number): Observable<ApiResponse> {
    return this.apiService.callApi('command', {
      param: 'updateuser',
      idx: idx,
      enabled: bEnabled.toString(),
      username: username,
      password: password,
      rights: rights.toString(),
      RemoteSharing: bEnableSharing.toString(),
      TabsEnabled: TabsEnabled.toString()
    });
  }


}
