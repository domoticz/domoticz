import {Injectable} from '@angular/core';
import {ApiService} from '../../_shared/_services/api.service';
import {ApiResponse} from '../../_shared/_models/api';
import {Observable} from 'rxjs';

@Injectable()
export class SendNotificationService {

  constructor(private apiService: ApiService) {
  }

  sendNotification(subject: string, body: string): Observable<ApiResponse> {
    return this.apiService.callApi('command', {
      param: 'sendnotification',
      subject: subject,
      body: body
    });
  }
}
