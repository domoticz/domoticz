import {Injectable} from '@angular/core';
import {ApiService} from './api.service';
import {Observable} from 'rxjs';
import {UserVariablesResponse} from '../_models/user-variables';
import {ApiResponse} from '../_models/api';

@Injectable()
export class UserVariablesService {

  constructor(private apiService: ApiService) {
  }

  getUserVariables(): Observable<UserVariablesResponse> {
    return this.apiService.callApi<UserVariablesResponse>('command', {param: 'getuservariables'});
  }

  deleteUserVariable(idx: string): Observable<ApiResponse> {
    return this.apiService.callApi('command', {param: 'deleteuservariable', 'idx': idx});
  }

  addUserVariable(uservariablename: string, uservariabletype: string, uservariablevalue: string): Observable<ApiResponse> {
    return this.apiService.callApi('command', {
      param: 'adduservariable',
      vname: uservariablename,
      vtype: uservariabletype,
      vvalue: uservariablevalue
    });
  }

  updateUserVariable(idx: string, uservariablename: string, uservariabletype: string, uservariablevalue: string): Observable<ApiResponse> {
    return this.apiService.callApi('command', {
      param: 'updateuservariable',
      idx: idx,
      vname: uservariablename,
      vtype: uservariabletype,
      vvalue: uservariablevalue
    });
  }
}
