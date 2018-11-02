import {Injectable} from '@angular/core';
import {ApiService} from '../_shared/_services/api.service';
import {AbstractTimersService} from '../_shared/_services/timers.service';

@Injectable()
export class SetpointTimersService extends AbstractTimersService {

  constructor(apiService: ApiService) {
    super(apiService, 'setpointtimer');
  }

}
