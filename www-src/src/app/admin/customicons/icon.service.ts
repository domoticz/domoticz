import { Injectable } from '@angular/core';
import { ApiService } from '../../_shared/_services/api.service';
import { Observable } from 'rxjs';
import { IconsetResponse } from '../../_shared/_models/icon';
import { ApiResponse } from '../../_shared/_models/api';

@Injectable()
export class IconService {

  constructor(
    private apiService: ApiService
  ) { }

  getCustomIconset(): Observable<IconsetResponse> {
    return this.apiService.callApi<IconsetResponse>('command', { 'param': 'getcustomiconset' });
  }

  uploadCustomIcon(file): Observable<ApiResponse> {
    const fd = new FormData();
    fd.append('file', file);
    return this.apiService.postFormData('uploadcustomicon', fd);
  }

  deleteCustomIcon(idx: string): Observable<ApiResponse> {
    return this.apiService.callApi('command', { param: 'deletecustomicon', idx: idx });
  }

  updateCustomIcon(idx: string, name: string, description: string): Observable<ApiResponse> {
    return this.apiService.callApi('command', {
      param: 'updatecustomicon',
      idx: idx,
      name: name,
      description: description
    });
  }

}
