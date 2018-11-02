import { Injectable } from '@angular/core';
import { ApiService } from '../../_shared/_services/api.service';
import { Observable } from 'rxjs';
import { AboutResponse } from './about';

@Injectable()
export class AboutService {

  constructor(
    private apiService: ApiService
  ) { }

  public getUpTime(): Observable<AboutResponse> {
    return this.apiService.callApi<AboutResponse>('command', { param: 'getuptime' });
  }

}
