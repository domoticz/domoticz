import {GlobalConfig, GlobalVars} from '../_models/config';
import {Injectable} from '@angular/core';
import {ApiService} from './api.service';
import {BehaviorSubject, Observable} from 'rxjs';
import {Metadata} from '../_models/metadata';
import {UpdateService} from './update.service';

@Injectable()
export class ConfigService {

  // Default configuration
  config: GlobalConfig = {
    EnableTabProxy: false,
    EnableTabDashboard: false,
    EnableTabFloorplans: false,
    EnableTabLights: false,
    EnableTabScenes: false,
    EnableTabTemp: false,
    EnableTabWeather: false,
    EnableTabUtility: false,
    EnableTabCustom: false,
    AllowWidgetOrdering: true,
    FiveMinuteHistoryDays: 1,
    DashboardType: 1,
    MobileType: 0,
    TempScale: 1.0,
    DegreeDaysBaseTemperature: 18.0,
    TempSign: 'C',
    WindScale: 3.600000143051148,
    WindSign: 'km/h',
    language: 'en',
    HaveUpdate: false,
    UseUpdate: true,
    // appversion: '0',
    apphash: '0',
    appdate: '0',
    pythonversion: '',
    versiontooltip: '',
    ShowUpdatedEffect: true,
    DateFormat: 'yy-mm-dd',
    isproxied: false
  };

  globals: GlobalVars = {
    actlayout: '',
    prevlayout: '',
    ismobile: false,
    ismobileint: false,
    historytype: 1,
    LastPlanSelected: 0,
    DashboardType: 0,
    isproxied: false,
    HardwareTypesStr: [],
    HardwareI2CStr: [],
    SelectedHardwareIdx: 0,
    LastUpdate: 0
  };

  online = false;

  LastUpdateTime: number;

  dataTableDefaultSettings = {
    dom: '<"H"lfrC>t<"F"ip>',
    order: [[0, 'desc']],
    bSortClasses: false,
    bJQueryUI: true,
    processing: true,
    stateSave: true,
    paging: true,
    pagingType: 'full_numbers',
    pageLength: 25,
    lengthMenu: [[25, 50, 100, -1], [25, 50, 100, 'All']],
    select: {
      className: 'row_selected',
      style: 'single'
    },
    language: {} // Filled by LanuuageService.MakeDatatableTranslations
  };

  cachenoty: any = {};

  // previously embed switchtypes
  switchTypeOptions: Array<{ value: number; label: string }> = [
    {value: 3, label: 'Blinds'},
    {value: 6, label: 'Blinds Inverted'},
    {value: 13, label: 'Blinds Percentage'},
    {value: 16, label: 'Blinds Percentage Inverted'},
    {value: 2, label: 'Contact'},
    {value: 7, label: 'Dimmer'},
    {value: 11, label: 'Door Contact'},
    {value: 19, label: 'Door Lock'},
    {value: 20, label: 'Door Lock Inverted'},
    {value: 1, label: 'Doorbell'},
    {value: 12, label: 'Dusk Sensor'},
    {value: 17, label: 'Media Player'},
    {value: 8, label: 'Motion Sensor'},
    {value: 0, label: 'On/Off'},
    {value: 10, label: 'Push Off Button'},
    {value: 9, label: 'Push On Button'},
    {value: 18, label: 'Selector'},
    {value: 5, label: 'Smoke Detector'},
    {value: 15, label: 'Venetian Blinds EU'},
    {value: 14, label: 'Venetian Blinds US'},
    {value: 4, label: 'X10 Siren'}
  ];

  templates: Array<string> = [];

  appversion = new BehaviorSubject<string>('0');

  constructor(
    private apiService: ApiService,
    private updateService: UpdateService
  ) {
  }

  public getConfigFromApi(): Observable<ApiConfig> {
    return this.apiService.callApi<ApiConfig>('command', {param: 'getconfig'});
  }

  public forceReloadConfig(): void {
    this.getConfigFromApi().subscribe(apiConfig => {
      this.applyConfigFromApi(apiConfig);
    });
  }

  public applyConfigFromApi(apiConfig: ApiConfig) {
    this.config.AllowWidgetOrdering = apiConfig.AllowWidgetOrdering;
    this.config.FiveMinuteHistoryDays = apiConfig.FiveMinuteHistoryDays;
    this.config.DashboardType = apiConfig.DashboardType;
    this.config.MobileType = apiConfig.MobileType;
    this.config.TempScale = apiConfig.TempScale;
    this.config.TempSign = apiConfig.TempSign;
    this.config.WindScale = apiConfig.WindScale;
    this.config.WindSign = apiConfig.WindSign;
    this.config.language = apiConfig.language;
    this.config.EnableTabProxy = apiConfig.result.EnableTabProxy;
    this.config.EnableTabDashboard = apiConfig.result.EnableTabDashboard;
    this.config.EnableTabFloorplans = apiConfig.result.EnableTabFloorplans;
    this.config.EnableTabLights = apiConfig.result.EnableTabLights;
    this.config.EnableTabScenes = apiConfig.result.EnableTabScenes;
    this.config.EnableTabTemp = apiConfig.result.EnableTabTemp;
    this.config.EnableTabWeather = apiConfig.result.EnableTabWeather;
    this.config.EnableTabUtility = apiConfig.result.EnableTabUtility;
    this.config.ShowUpdatedEffect = apiConfig.result.ShowUpdatedEffect;
    this.config.DegreeDaysBaseTemperature = apiConfig.DegreeDaysBaseTemperature;

    if (typeof this.config.MobileType !== 'undefined') {
      if (/Android|webOS|iPhone|iPad|iPod|BlackBerry/i.test(navigator.userAgent)) {
        this.globals.ismobile = true;
        this.globals.ismobileint = true;
      }
      if (this.config.MobileType !== 0) {
        if (!(/iPhone/i.test(navigator.userAgent))) {
          this.globals.ismobile = false;
        }
      }
    }

    if (/Android|webOS|iPhone|iPad|iPod|ZuneWP7|BlackBerry/i.test(navigator.userAgent)) {
      this.globals.ismobile = true;
      this.globals.ismobileint = true;
    }

    if (apiConfig.result.templates) {
      this.templates = apiConfig.result.templates;
      if (this.templates.length > 0) {
        this.config.EnableTabCustom = apiConfig.result.EnableTabCustom;
      }
    }
  }

  public applyConfigFromMetadata(metadata: Metadata) {
    // this.config.appversion = metadata.version;
    this.config.apphash = metadata.hash;
    this.config.appdate = metadata.build_time;
    this.config.dzventsversion = metadata.dzvents_version;
    this.config.pythonversion = metadata.python_version;
    this.config.isproxied = metadata.isproxied;
    this.config.versiontooltip =
      'Build Hash: <b>' + metadata.hash + '</b><br>' + 'Build Date: ' + metadata.build_time;

    this.config.HaveUpdate = metadata.HaveUpdate;
    this.config.UseUpdate = metadata.UseUpdate;
    if ((metadata.HaveUpdate === true) && (metadata.UseUpdate)) {
      this.globals.historytype = 2;
      this.updateService.ShowUpdateNotification(metadata.Revision, metadata.SystemName, metadata.DomoticzUpdateURL);
    }

    this.appversion.next(metadata.version);
  }

}

export class ApiConfig {
  AllowWidgetOrdering: boolean;
  DashboardType: number;
  DegreeDaysBaseTemperature: number;
  FiveMinuteHistoryDays: number;
  MobileType: number;
  TempScale: number;
  TempSign: string;
  WindScale: number;
  WindSign: string;
  language: string;
  result: {
    EnableTabCustom: boolean;
    EnableTabDashboard: boolean;
    EnableTabFloorplans: boolean;
    EnableTabLights: boolean;
    EnableTabProxy: boolean;
    EnableTabScenes: boolean;
    EnableTabTemp: boolean;
    EnableTabUtility: boolean;
    EnableTabWeather: boolean;
    ShowUpdatedEffect: boolean;
    templates?: Array<string>;
  };
  status: string;
  title: string;
}
