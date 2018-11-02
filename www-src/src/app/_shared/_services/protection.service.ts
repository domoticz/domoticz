import {Inject, Injectable} from '@angular/core';
import {ApiService} from './api.service';
import {I18NEXT_SERVICE, ITranslationService} from 'angular-i18next';
import {ApiResponse} from '../_models/api';

// FIXME manage with NPM+TypeScript
declare var bootbox: any;

@Injectable()
export class ProtectionService {

  constructor(
    private apiService: ApiService,
    @Inject(I18NEXT_SERVICE) private translationService: ITranslationService
  ) {
  }

  HandleProtection(isprotected: boolean, callbackfunction: (passcode: string) => void) {
    if (typeof isprotected === 'undefined') {
      callbackfunction('');
      return;
    }
    if (isprotected === false) {
      callbackfunction('');
      return;
    }
    bootbox.prompt(this.translationService.t('Please enter Password') + ':', (result: string) => {
      if (result === null) {
        return;
      } else {
        if (result === '') {
          return;
        }
        // verify password
        this.apiService.callApi<ApiResponse>('command', {param: 'verifypasscode', passcode: result}).subscribe(data => {
          if (data.status === 'OK') {
            callbackfunction(result);
          }
        }, error => {
          // Nothing
        });
      }
    });
  }

}
