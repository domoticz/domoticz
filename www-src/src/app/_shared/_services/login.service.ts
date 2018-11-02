import {Injectable} from '@angular/core';
import {ApiService} from './api.service';
import {Md5} from 'ts-md5/dist/md5';
import {ApiResponse} from '../_models/api';
import {Observable} from 'rxjs';
import {Auth} from '../_models/auth';

@Injectable()
export class LoginService {

  constructor(
    private apiService: ApiService
  ) {
  }

  public getAuth(): Observable<Auth> {
    return this.apiService.callApi<Auth>('command', {param: 'getauth'});
  }

  public loginCheck(username: string, password: string, rememberme: boolean): Observable<LoginResponse> {
    const musername = encodeURIComponent(btoa(username));
    const mpassword = encodeURIComponent(<string>Md5.hashStr(password));

    const fd = new FormData();
    fd.append('username', musername);
    fd.append('password', mpassword);
    fd.append('rememberme', rememberme.toString());

    return this.apiService.postFormData('logincheck', fd);
  }

  public logout(): Observable<ApiResponse> {
    return this.apiService.callApi<ApiResponse>('command', {param: 'dologout'});
  }

}

export interface LoginResponse extends ApiResponse {
  user?: string;
  rights?: number;
}
