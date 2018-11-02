import { NotificationTypes, NotificationType, Notifications } from '../_shared/_models/notification-type';
import { Injectable, Inject } from '@angular/core';
import { Observable } from 'rxjs';
import { ApiService } from '../_shared/_services/api.service';
import { map } from 'rxjs/operators';
import { ApiResponse } from '../_shared/_models/api';
import { I18NEXT_SERVICE, ITranslationService } from 'angular-i18next';
import { ConfigService } from '../_shared/_services/config.service';

@Injectable()
export class DeviceNotificationService {

  deviceNotificationsConstants: DeviceNotificationsConstants;

  constructor(
    private apiService: ApiService,
    private configService: ConfigService,
    @Inject(I18NEXT_SERVICE) private translationService: ITranslationService) {

    this.deviceNotificationsConstants = {
      typeNameByTypeMap: {
        'T': this.translationService.t('Temperature'),
        'D': this.translationService.t('Dew Point'),
        'H': this.translationService.t('Humidity'),
        'R': this.translationService.t('Rain'),
        'W': this.translationService.t('Wind'),
        'U': this.translationService.t('UV'),
        'M': this.translationService.t('Usage'),
        'B': this.translationService.t('Baro'),
        'S': this.translationService.t('Switch On'),
        'O': this.translationService.t('Switch Off'),
        'E': this.translationService.t('Today'),
        'G': this.translationService.t('Today'),
        'C': this.translationService.t('Today'),
        '1': this.translationService.t('Ampere 1'),
        '2': this.translationService.t('Ampere 2'),
        '3': this.translationService.t('Ampere 3'),
        'P': this.translationService.t('Percentage'),
        'V': this.translationService.t('Play Video'),
        'A': this.translationService.t('Play Audio'),
        'X': this.translationService.t('View Photo'),
        'Y': this.translationService.t('Pause Stream'),
        'Q': this.translationService.t('Stop Stream'),
        'a': this.translationService.t('Play Stream'),
        'J': this.translationService.t('Last Update')
      },
      unitByTypeMap: {
        'T': '° ' + this.configService.config.TempSign,
        'D': '° ' + this.configService.config.TempSign,
        'H': '%',
        'R': 'mm',
        'W': this.configService.config.WindSign,
        'U': 'UVI',
        'B': 'hPa',
        'E': 'kWh',
        'G': 'm3',
        'C': 'cnt',
        '1': 'A',
        '2': 'A',
        '3': 'A',
        'P': '%',
        'J': 'min'
      },
      whenByTypeMap: {
        'S': this.translationService.t('On'),
        'O': this.translationService.t('Off'),
        'D': this.translationService.t('Dew Point'),
        'V': this.translationService.t('Video'),
        'A': this.translationService.t('Audio'),
        'X': this.translationService.t('Photo'),
        'Y': this.translationService.t('Paused'),
        'Q': this.translationService.t('Stopped'),
        'a': this.translationService.t('Playing')
      },
      whenByConditionMap: {
        '>': this.translationService.t('Greater'),
        '>=': this.translationService.t('Greater or Equal'),
        '=': this.translationService.t('Equal'),
        '!=': this.translationService.t('Not Equal'),
        '<=': this.translationService.t('Less or Equal'),
        '<': this.translationService.t('Less')
      },
      priorities: [
        this.translationService.t('Very Low'),
        this.translationService.t('Moderate'),
        this.translationService.t('Normal'),
        this.translationService.t('High'),
        this.translationService.t('Emergency')
      ]
    };

  }

  getNotifications(deviceIdx: string): Observable<Notifications> {
    const params = {
      idx: deviceIdx
    };
    return this.apiService.callApi<Notifications>('notifications', params);
  }

  getNotificationTypes(deviceIdx: string): Observable<Array<NotificationType>> {
    const params = {
      param: 'getnotificationtypes',
      idx: deviceIdx
    };
    return this.apiService.callApi<NotificationTypes>('command', params).pipe(
      map(response => {
        return response.result;
      })
    );
  }

  addNotification(deviceIdx: string, options: { [key: string]: string }): Observable<ApiResponse> {
    const params = Object.assign({
      param: 'addnotification',
      idx: deviceIdx
    }, options);
    return this.apiService.callApi<ApiResponse>('command', params);
  }

  updateNotification(deviceIdx: string, idx: number, options: { [key: string]: string }): Observable<ApiResponse> {
    const params = Object.assign({
      param: 'updatenotification',
      idx: idx.toString(),
      devidx: deviceIdx
    }, options);
    return this.apiService.callApi<ApiResponse>('command', params);
  }

  deleteNotification(idx: number): Observable<ApiResponse> {
    const params = {
      param: 'deletenotification',
      idx: idx.toString()
    };
    return this.apiService.callApi<ApiResponse>('command', params);
  }

  clearNotifications(deviceIdx: string): Observable<ApiResponse> {
    return this.apiService.callApi<ApiResponse>('command', { param: 'clearnotifications', idx: deviceIdx });
  }

}

export interface DeviceNotificationsConstants {
  typeNameByTypeMap: { [type: string]: string };
  unitByTypeMap: { [type: string]: string };
  whenByTypeMap: { [type: string]: string };
  whenByConditionMap: { [condition: string]: string };
  priorities: Array<string>;
}
