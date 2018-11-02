import { I18NEXT_SERVICE, ITranslationService, I18NextModule, defaultInterpolationFormat } from 'angular-i18next';
import { Injectable, Inject } from '@angular/core';
import { ApiService } from './api.service';
import { ApiResponse } from '../_models/api';
import { Observable } from 'rxjs';
import { map } from 'rxjs/operators';
import { ConfigService } from './config.service';
import * as i18nextXHRBackend from 'i18next-xhr-backend';

@Injectable()
export class LanguageService {

  private i18nOptions = {
    whitelist: [
      'ar',
      'bg',
      'bs',
      'ca',
      'cs',
      'da',
      'de',
      'el',
      'en',
      'es',
      'et',
      'fa',
      'fi',
      'fr',
      'he',
      'hu',
      'is',
      'it',
      'lv',
      'mk',
      'nl',
      'no',
      'pl',
      'pt',
      'ro',
      'ru',
      'sk',
      'sl',
      'sq',
      'sr',
      'sv',
      'th',
      'tr',
      'uk',
      'zh',
      'zh_TW'
    ],
    fallbackLng: 'en',
    // lng: 'en',
    debug: false,
    returnEmptyString: false,
    ns: [
      'translation'
    ],
    keySeparator: false, // Avoid treating . as a separator
    nsSeparator: false, // Avoid treating : as a separator
    interpolation: {
      format: I18NextModule.interpolationFormat(defaultInterpolationFormat)
    },
    // backend plugin options
    backend: {
      loadPath: function (langs, ns) {
        // We ignore namespace for now
        // return 'locales/{{lng}}.{{ns}}.json';
        return 'i18n/domoticz-{{lng}}.json';
      }
    },
  };

  constructor(
    private apiService: ApiService,
    @Inject(I18NEXT_SERVICE) private i18NextService: ITranslationService,
    private configService: ConfigService
  ) { }

  public init(): Promise<any> {
    return this.i18NextService.use(i18nextXHRBackend).init(this.i18nOptions);
  }

  public getLanguage(): Observable<string> {
    return this.apiService.callApi<LanguageResponse>('command', { param: 'getlanguage' }).pipe(
      map(data => {
        if (typeof data.language !== 'undefined') {
          return data.language;
        } else {
          return 'en';
        }
      })
    );
  }

  public setLanguage(lng: string): Promise<any> {
    console.log(`Setting language to ${lng}...`);
    return this.i18NextService.changeLanguage(lng).then(() => {
      this.MakeDatatableTranslations();
    });
  }

  private MakeDatatableTranslations(): void {
    this.configService.dataTableDefaultSettings.language = {};
    this.configService.dataTableDefaultSettings.language['search'] = this.i18NextService.t('Search') + '&nbsp;:';
    this.configService.dataTableDefaultSettings.language['lengthMenu'] = this.i18NextService.t('Show _MENU_ entries');
    this.configService.dataTableDefaultSettings.language['info'] =
      this.i18NextService.t('Showing _START_ to _END_ of _TOTAL_ entries');
    this.configService.dataTableDefaultSettings.language['infoEmpty'] = this.i18NextService.t('Showing 0 to 0 of 0 entries');
    this.configService.dataTableDefaultSettings.language['infoFiltered'] =
      this.i18NextService.t('(filtered from _MAX_ total entries)');
    this.configService.dataTableDefaultSettings.language['infoPostFix'] = '';
    this.configService.dataTableDefaultSettings.language['zeroRecords'] = this.i18NextService.t('No matching records found');
    this.configService.dataTableDefaultSettings.language['emptyTable'] = this.i18NextService.t('No data available in table');
    this.configService.dataTableDefaultSettings.language['paginate'] = {};
    this.configService.dataTableDefaultSettings.language['paginate']['first'] = this.i18NextService.t('First');
    this.configService.dataTableDefaultSettings.language['paginate']['previous'] = this.i18NextService.t('Previous');
    this.configService.dataTableDefaultSettings.language['paginate']['next'] = this.i18NextService.t('Next');
    this.configService.dataTableDefaultSettings.language['paginate']['last'] = this.i18NextService.t('Last');
  }

}

interface LanguageResponse extends ApiResponse {
  language: string;
}
