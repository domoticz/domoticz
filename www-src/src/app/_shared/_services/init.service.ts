import {Metadata} from '../_models/metadata';
import {ApiService} from './api.service';
import {ConfigService, ApiConfig} from './config.service';
import {Injectable, Inject} from '@angular/core';
import {ITranslationService, I18NEXT_SERVICE} from 'angular-i18next';
import {PermissionService} from './permission.service';
import {Permissions} from '../_models/permissions';
import {Title} from '@angular/platform-browser';
import * as Highcharts from 'highcharts';
import HighchartsMore from 'highcharts/highcharts-more';
import HighchartsThemeDarkUnica from 'highcharts/themes/dark-unica';
import HighchartsExporting from 'highcharts/modules/exporting';
import HighchartsSankey from 'highcharts/modules/sankey';
import * as HighchartsDependencyWheel from 'highcharts/modules/dependency-wheel.js';
import * as HighchartsXRange from 'highcharts/modules/xrange.src.js';
//import * as HighchartsExportCSV from 'highcharts-export-csv/export-csv.js';
import {LanguageService} from './language.service';
import {LoginService} from './login.service';
import {LivesocketService} from './livesocket.service';
import {Utils} from '../_utils/utils';
import {ChartService} from './chart.service';

// FIXME preoper declaration
declare var $: any;
declare var bootbox: any;

@Injectable()
export class InitService {

  constructor(private configService: ConfigService,
              private apiService: ApiService,
              private permissionService: PermissionService,
              @Inject(I18NEXT_SERVICE) private translationService: ITranslationService,
              private languageService: LanguageService,
              private loginService: LoginService,
              private titleService: Title,
              private livesocketService: LivesocketService) {
  }

  init(): Promise<any> {
    this.livesocketService.init();

    const authPromise = this.loginService.getAuth().toPromise()
      .then(auth => {
        this.configService.online = true;
        if (auth.status === 'OK') {
          this.permissionService.setPermission(new Permissions(auth.rights, auth.user !== ''));
        }
      }, () => {
        this.configService.online = false;
      });

    // what for? (not as a promise because not mandatory for app init)
    if (typeof sessionStorage.title === 'undefined') {
      sessionStorage.title = 'Domoticz';
    }

    const configPromise = this.configService.getConfigFromApi().toPromise();
    const versionPromise = this.apiService.callApi<Metadata>('command', {param: 'getversion'}).toPromise();

    const i18nextPromise = this.languageService.init();

    return Promise.all([configPromise, versionPromise, i18nextPromise, authPromise])
      .then(values => {
        const apiConfig: ApiConfig = values[0];
        const metadata: Metadata = values[1];

        this.configService.online = true;

        this.configService.applyConfigFromApi(apiConfig);
        this.configService.applyConfigFromMetadata(metadata);

        this.setUpHighcharts();

        this.checkBrowser();

        return this.languageService.setLanguage(apiConfig.language);
      }, () => {
        // No need, already handled by Auth Promise
        // this.configService.online = false;
      });

  }

  private setUpHighcharts(): void {

    HighchartsMore(Highcharts);
    HighchartsExporting(Highcharts);
    HighchartsXRange(Highcharts);
    HighchartsSankey(Highcharts);
    HighchartsDependencyWheel(Highcharts);

    HighchartsThemeDarkUnica(Highcharts);

    Highcharts.setOptions({
      chart: {
        style: {
          fontFamily: '"Lucida Grande", "Lucida Sans Unicode", Arial, Helvetica, sans-serif'
        }
      },
      credits: {
        enabled: true,
        href: 'http://www.domoticz.com',
        text: 'Domoticz.com'
      },
      title: {
        style: {
          textTransform: 'none',
          fontFamily: 'Trebuchet MS, Verdana, sans-serif',
          fontSize: '16px',
          fontWeight: 'bold'
        }
      },
      legend: {
        itemStyle: {
          fontFamily: 'Trebuchet MS, Verdana, sans-serif',
          fontWeight: 'normal'
        }
      }
    });

    // Translate Highcharts (partly)
    Highcharts.setOptions({
      lang: {
        months: [
          this.translationService.t('January'),
          this.translationService.t('February'),
          this.translationService.t('March'),
          this.translationService.t('April'),
          this.translationService.t('May'),
          this.translationService.t('June'),
          this.translationService.t('July'),
          this.translationService.t('August'),
          this.translationService.t('September'),
          this.translationService.t('October'),
          this.translationService.t('November'),
          this.translationService.t('December')
        ],
        shortMonths: [
          this.translationService.t('Jan'),
          this.translationService.t('Feb'),
          this.translationService.t('Mar'),
          this.translationService.t('Apr'),
          this.translationService.t('May'),
          this.translationService.t('Jun'),
          this.translationService.t('Jul'),
          this.translationService.t('Aug'),
          this.translationService.t('Sep'),
          this.translationService.t('Oct'),
          this.translationService.t('Nov'),
          this.translationService.t('Dec')
        ],
        weekdays: [
          this.translationService.t('Sunday'),
          this.translationService.t('Monday'),
          this.translationService.t('Tuesday'),
          this.translationService.t('Wednesday'),
          this.translationService.t('Thursday'),
          this.translationService.t('Friday'),
          this.translationService.t('Saturday')
        ]
      }
    });

    // Add custom behaviour to highcharts.
    if (typeof (Storage) !== 'undefined') {
      this.addCustomBehaviorToHighcharts();
    }
  }

  private addCustomBehaviorToHighcharts() {
    /*
        // Use this code to debug excessive redrawing of graphs.
        Highcharts.wrap( Highcharts.Chart.prototype, 'redraw', function ( fProceed_ ) {
          console.log( 'draw ' + $( this.container ).parent().attr( 'id' ) );
          fProceed_.apply( this, Array.prototype.slice.call( arguments, 1 ) );
        } );
    */

    Highcharts.wrap(Highcharts.Series.prototype, 'setVisible', function (fProceed_) {
      let iVisibles = 0;
      const me = this;
      this.chart.series.forEach((oSerie_, iIndex_) => {
        if (oSerie_ !== me && oSerie_.visible) {
          iVisibles++;
        }
      });
      if (iVisibles > 0 || this.visible === false) {
        fProceed_.apply(this, Array.prototype.slice.call(arguments, 1));
        try {
          const sStorageId = 'highcharts_series_visibility_' + ChartService.getDeviceIdx();
          const sStateId = $(this.chart.container).parent().attr('id') + '_' + this.options.id;
          const sCurrentState = localStorage.getItem(sStorageId) || '{}';
          const oCurrentState = JSON.parse(sCurrentState);
          oCurrentState[sStateId] = this.visible;
          localStorage.setItem(sStorageId, JSON.stringify(oCurrentState));
        } catch (oException_) { /* too bad, no state */
        }
      }
    });

    Highcharts.wrap(Highcharts.Series.prototype, 'init', function (fProceed_, oChart_, oOptions_) {
      try {
        const sStorageId = 'highcharts_series_visibility_' + ChartService.getDeviceIdx();
        const sStateId = $(oChart_.container).parent().attr('id') + '_' + oOptions_.id;
        const sCurrentState = localStorage.getItem(sStorageId) || '{}';
        const oCurrentState = JSON.parse(sCurrentState);
        if (sStateId in oCurrentState) {
          oOptions_.visible = oCurrentState[sStateId];
        }
      } catch (oException_) { /* too bad, no state */
      }
      fProceed_.apply(this, Array.prototype.slice.call(arguments, 1));
    });
  }

  private checkBrowser() {
    const matched = Utils.matchua(navigator.userAgent);
    if (matched.browser === 'msie') {
      const bVersion = Number(matched.version);
      if (bVersion < 10.0) {
        bootbox.alert(this.translationService.t('This Application is is designed for Internet Explorer version 10+!\nPlease upgrade your browser!'));
      }
    }
  }

}

