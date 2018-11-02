import {Injectable} from '@angular/core';
import {ApiService} from '../_shared/_services/api.service';
import {AbstractTimersService} from '../_shared/_services/timers.service';

@Injectable()
export class SceneTimersService extends AbstractTimersService {

  constructor(apiService: ApiService) {
    super(apiService, 'scenetimer');
  }

}
