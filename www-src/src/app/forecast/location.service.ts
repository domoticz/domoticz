import {Injectable} from '@angular/core';
import {ApiService} from '../_shared/_services/api.service';
import {Observable} from 'rxjs';
import {LocationResponse} from './location';

@Injectable()
export class LocationService {

  constructor(private apiService: ApiService) {
  }

  getLocation(): Observable<LocationResponse> {
    return this.apiService.callApi('command', {param: 'getlocation'});
  }

}
