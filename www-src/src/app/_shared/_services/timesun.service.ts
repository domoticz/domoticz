import {Observable, Subject} from 'rxjs';
import {ApiService} from './api.service';
import {Injectable} from '@angular/core';
import {SunRiseSet, TimeAndSun} from '../_models/sunriseset';
import {LivesocketService} from './livesocket.service';

@Injectable()
export class TimesunService {

  private data: Subject<TimeAndSun> = new Subject<TimeAndSun>();

  constructor(private apiService: ApiService,
              private livesocketService: LivesocketService) {
    this.livesocketService.time_update.subscribe(data => {
      this.SetTimeAndSun(data);
    });
  }

  public RefreshTimeAndSun(): void {
    this.apiService.callApi<SunRiseSet>('command', {param: 'getSunRiseSet'}).subscribe(sunriseset => {
      if (sunriseset.Sunrise) {
        this.SetTimeAndSun(sunriseset);
      }
    });
  }

  public SetTimeAndSun(timeAndSun: TimeAndSun) {
    this.data.next(timeAndSun);
  }

  public getTimeAndSun(): Observable<TimeAndSun> {
    return this.data;
  }

}
