import {Inject, Injectable} from '@angular/core';
import {ApiService} from './api.service';
import {Observable} from 'rxjs';
import {CheckForUpdateResponse, DownloadReadyResponse} from '../_models/update';
import {ApiResponse} from '../_models/api';
import {Metadata} from '../_models/metadata';
import {NotyHelper} from '../_utils/noty-helper';
import {I18NEXT_SERVICE, ITranslationService} from 'angular-i18next';

@Injectable()
export class UpdateService {

  constructor(private apiService: ApiService,
              @Inject(I18NEXT_SERVICE) private translationService: ITranslationService) {
  }

  checkForUpdate(forced: boolean): Observable<CheckForUpdateResponse> {
    return this.apiService.callApi('command', {param: 'checkforupdate', forced: forced.toString()});
  }

  downloadUpdate(): Observable<ApiResponse> {
    return this.apiService.callApi('command', {param: 'downloadupdate'});
  }

  downloadReady(): Observable<DownloadReadyResponse> {
    return this.apiService.callApi('command', {param: 'downloadready'});
  }

  getVersion(): Observable<Metadata> {
    return this.apiService.callApi('command', {param: 'getversion'});
  }

  update(): Observable<ApiResponse> {
    return this.apiService.callApi('command', {
      param: 'execute_script',
      scriptname: 'update_domoticz',
      direct: 'true'
    });
  }

  public ShowUpdateNotification(Revision: number, SystemName: string, DownloadURL: string) {
    let msgtxt = this.translationService.t('A new version of Domoticz is Available!...');
    msgtxt += '<br>' + this.translationService.t('Version') + ': <b>' + Revision + '</b>, ' +
      this.translationService.t('Latest Changes') + ': <b><a class="norm-link" href="/History"">' +
      this.translationService.t('Click Here') + '</a></b>';
    if (SystemName === 'windows') {
      msgtxt += '<br><div style="text-align:center;"><a class="btn btn-danger" href="' + DownloadURL + '">' +
        this.translationService.t('Update Now') + '</a></div>';
    } else {
      msgtxt += '<br><div style="text-align:center;"><a class="btn btn-danger" href="/Update">' +
        this.translationService.t('Update Now') + '</a></div>';
    }
    NotyHelper.generate_noty('success', msgtxt, false);
  }

  updateApplication(): Observable<ApiResponse> {
    return this.apiService.callApi('command', {param: 'update_application'});
  }

}
