import {Component, Inject, OnInit} from '@angular/core';
import {ConfigService} from '../../../_shared/_services/config.service';
import {GlobalConfig} from 'src/app/_shared/_models/config';
import {TimesunService} from 'src/app/_shared/_services/timesun.service';
import {SetupService} from '../../../_shared/_services/setup.service';
import {Theme} from 'src/app/_shared/_models/theme';
import {TimerPlan} from 'src/app/_shared/_models/timers';
import {SettingsResponse} from 'src/app/_shared/_models/settings';
import {Md5} from 'ts-md5/dist/md5';
import {Router} from '@angular/router';
import {NotificationService} from '../../../_shared/_services/notification.service';
import {I18NEXT_SERVICE, ITranslationService} from 'angular-i18next';
import {DialogService} from '../../../_shared/_services/dialog.service';
import {FindlatlongDialogComponent} from '../../../_shared/_dialogs/findlatlong-dialog/findlatlong-dialog.component';
import {languages} from '../../../_shared/_constants/languages';

// FIXME proper declaration
declare var $: any;
declare var bootbox: any;

@Component({
  selector: 'dz-setup',
  templateUrl: './setup.component.html',
  styleUrls: ['./setup.component.css']
})
export class SetupComponent implements OnInit {

  config: GlobalConfig;

  sunRise = '';
  sunSet = '';

  themes: Array<Theme> = [];
  timerPlans: Array<TimerPlan> = [];

  languages = languages;

  settings: SettingsResponse;
  ProwlEnabled: boolean;
  PushbulletEnabled: boolean;
  TelegramEnabled: boolean;
  PushsaferEnabled: boolean;
  PushoverEnabled: boolean;
  PushALotEnabled: boolean;
  ClickatellEnabled: boolean;
  HTTPEnabled: boolean;
  HTTPPostHeaders: string;
  HTTPField1: string;
  HTTPField2: string;
  HTTPField3: string;
  HTTPField4: string;
  HTTPTo: string;
  HTTPURL: string;
  HTTPPostData: string;
  HTTPPostContentType: string;
  ClickatellAPI: string;
  ClickatellTo: string;
  KodiEnabled: boolean;
  LmsEnabled: boolean;
  FCMEnabled: boolean;
  OldAdminUser: string;
  WebPassword: string;
  UseAutoUpdate: boolean;
  UseAutoBackup: boolean;
  EmailEnabled: boolean;
  EmailUsername: string;
  EmailPassword: string;
  UseEmailInNotifications: boolean;
  EmailAsAttachment: boolean;
  EnableTabFloorplans: boolean;
  EnableTabLights: boolean;
  EnableTabScenes: boolean;
  EnableTabTemp: boolean;
  EnableTabWeather: boolean;
  EnableTabUtility: boolean;
  EnableTabCustom: boolean;
  AcceptNewHardware: boolean;
  HideDisabledHardwareSensors: boolean;
  ShowUpdateEffect: boolean;
  EnableEventScriptSystem: boolean;
  DisableDzVentsSystem: boolean;
  LogEventScriptTrigger: boolean;
  EventSystemLogFullURL: boolean;
  FloorplanFullscreenMode: boolean;
  FloorplanAnimateZoom: boolean;
  FloorplanShowSensorValues: boolean;
  FloorplanShowSwitchValues: boolean;
  FloorplanShowSceneNames: boolean;
  AllowWidgetOrdering: boolean;
  SubsystemHttp: boolean;
  SubsystemShared: boolean;
  SubsystemApps: boolean;
  SendErrorsAsNotification: boolean;
  IFTTTEnabled: boolean;

  constructor(
    private configService: ConfigService,
    private timeSunService: TimesunService,
    private setupService: SetupService,
    private notificationService: NotificationService,
    @Inject(I18NEXT_SERVICE) private translationService: ITranslationService,
    private router: Router,
    private dialogService: DialogService,
  ) {
  }

  ngOnInit() {
    this.config = this.configService.config;

    this.MakeScrollLink('#idsystem', '#system');
    this.MakeScrollLink('#idloghistory', '#loghistory');
    this.MakeScrollLink('#idnotifications', '#notifications');
    this.MakeScrollLink('#idemailsetup', '#emailsetup');
    this.MakeScrollLink('#idmetercounters', '#metercounters');
    this.MakeScrollLink('#idfloorplans', '#floorplans');
    this.MakeScrollLink('#idothersettings', '#othersettings');
    this.MakeScrollLink('#idrestoredatabase', '#restoredatabase');

    this.ShowSettings();
  }

  MakeScrollLink(nameclick: string, namescroll: string) {
    $(nameclick).click(function (e) {
      const position = $(namescroll).offset();
      scroll(0, position.top - 60);
      e.preventDefault();
    });
  }

  ShowSettings() {
    this.timeSunService.RefreshTimeAndSun();
    this.timeSunService.getTimeAndSun().subscribe(data => {
      if (typeof data.Sunrise !== 'undefined') {
        this.sunRise = data.Sunrise;
        this.sunSet = data.Sunset;
      }
    });

    this.setupService.getThemes().subscribe((data) => {
      this.themes = data.result;
    });

    this.setupService.getTimerPlans().subscribe((data) => {
      this.timerPlans = data.result;
    });

    this.setupService.getSettings().subscribe((settings) => {
      this.settings = {Location: {Latitude: '', Longitude: ''}, ...settings};
      this.ProwlEnabled = settings.ProwlEnabled === 1;
      this.PushbulletEnabled = settings.PushbulletEnabled === 1;
      this.TelegramEnabled = settings.TelegramEnabled === 1;
      this.PushsaferEnabled = settings.PushsaferEnabled === 1;
      this.PushoverEnabled = settings.PushoverEnabled === 1;
      this.PushALotEnabled = settings.PushALotEnabled === 1;

      this.ClickatellEnabled = settings.ClickatellEnabled === 1;
      this.ClickatellAPI = atob(settings.ClickatellAPI);
      this.ClickatellTo = atob(settings.ClickatellTo);

      this.HTTPEnabled = settings.HTTPEnabled === 1;
      this.HTTPField1 = atob(settings.HTTPField1);
      this.HTTPField2 = atob(settings.HTTPField2);
      this.HTTPField3 = atob(settings.HTTPField3);
      this.HTTPField4 = atob(settings.HTTPField4);
      this.HTTPTo = atob(settings.HTTPTo);
      this.HTTPURL = atob(settings.HTTPURL);
      this.HTTPPostData = atob(settings.HTTPPostData);
      this.HTTPPostContentType = atob(settings.HTTPPostContentType);
      this.HTTPPostHeaders = atob(settings.HTTPPostHeaders);

      this.KodiEnabled = settings.KodiEnabled === 1;
      if (!settings.KodiIPAddress) {
        settings.KodiIPAddress = '224.0.0.1';
      }
      if (!settings.KodiPort) {
        settings.KodiPort = 9777;
      }
      if (!settings.KodiTimeToLive) {
        settings.KodiTimeToLive = 5;
      }

      this.LmsEnabled = settings.LmsEnabled === 1;
      if (!settings.LmsDuration) {
        settings.LmsDuration = 5;
      }

      this.FCMEnabled = settings.FCMEnabled === 1;

      this.OldAdminUser = settings.WebUserName;

      this.WebPassword = Md5.hashStr('bogus') as string;

      this.UseAutoUpdate = settings.UseAutoUpdate === 1;
      this.UseAutoBackup = settings.UseAutoBackup === 1;
      this.EmailEnabled = settings.EmailEnabled === 1;

      this.EmailUsername = atob(settings.EmailUsername);
      this.EmailPassword = atob(settings.EmailPassword);

      this.UseEmailInNotifications = settings.UseEmailInNotifications === 1;
      this.EmailAsAttachment = settings.EmailAsAttachment === 1;

      this.EnableTabFloorplans = settings.EnableTabFloorplans === 1;
      this.EnableTabLights = settings.EnableTabLights === 1;
      this.EnableTabScenes = settings.EnableTabScenes === 1;
      this.EnableTabTemp = settings.EnableTabTemp === 1;
      this.EnableTabWeather = settings.EnableTabWeather === 1;
      this.EnableTabUtility = settings.EnableTabUtility === 1;
      this.EnableTabCustom = settings.EnableTabCustom === 1;

      this.AcceptNewHardware = settings.AcceptNewHardware === 1;
      this.HideDisabledHardwareSensors = settings.HideDisabledHardwareSensors === 1;
      this.ShowUpdateEffect = settings.ShowUpdateEffect === 1;

      this.EnableEventScriptSystem = settings.EnableEventScriptSystem === 1;
      this.DisableDzVentsSystem = settings.DisableDzVentsSystem === 0;
      this.LogEventScriptTrigger = settings.LogEventScriptTrigger === 1;
      this.EventSystemLogFullURL = settings.EventSystemLogFullURL === 1;
      this.FloorplanFullscreenMode = settings.FloorplanFullscreenMode === 1;
      this.FloorplanAnimateZoom = settings.FloorplanAnimateZoom === 1;
      this.FloorplanShowSensorValues = settings.FloorplanShowSensorValues === 1;
      this.FloorplanShowSwitchValues = settings.FloorplanShowSwitchValues === 1;
      this.FloorplanShowSceneNames = settings.FloorplanShowSceneNames === 1;

      this.AllowWidgetOrdering = settings.AllowWidgetOrdering === 1;

      // tslint:disable-next-line:no-bitwise
      this.SubsystemHttp = (settings.MyDomoticzSubsystems & 1) > 0;
      // tslint:disable-next-line:no-bitwise
      this.SubsystemShared = (settings.MyDomoticzSubsystems & 2) > 0;
      // tslint:disable-next-line:no-bitwise
      this.SubsystemApps = (settings.MyDomoticzSubsystems & 4) > 0;

      this.SendErrorsAsNotification = settings.SendErrorsAsNotification === 1;
      this.IFTTTEnabled = settings.IFTTTEnabled === 1;

      if (typeof settings.Title !== 'undefined') {
        sessionStorage.title = settings.Title;
      } else {
        sessionStorage.title = 'Domoticz';
      }
      document.title = sessionStorage.title;
      settings.Title = sessionStorage.title;
    });
  }

  StoreSettings() {
    const Latitude = this.settings.Location.Latitude;
    const Longitude = this.settings.Location.Longitude;
    if (
      ((Latitude === '') || (Longitude === '')) ||
      (isNaN(Number(Latitude)) || isNaN(Number(Longitude)))
    ) {
      this.notificationService.ShowNotify(this.translationService.t('Invalid Location Settings...'), 2000, true);
      return;
    }

    const adminuser = this.settings.WebUserName;
    let adminpwd = this.WebPassword;
    if (adminpwd === (Md5.hashStr('bogus') as string)) {
      this.WebPassword = '';
      adminpwd = '';
    }
    if ((adminuser !== '') && (this.OldAdminUser !== adminuser)) {
      if (adminpwd === '') {
        this.notificationService.ShowNotify(this.translationService.t('Please enter a Admin password!'), 2000, true);
        return;
      }
    }
    if (adminpwd !== '') {
      this.WebPassword = Md5.hashStr(adminpwd) as string;
    }

    const secpanel = this.settings.SecPassword;
    const switchprotection = this.settings.ProtectionPassword;

    // Apply Title
    sessionStorage.title = this.settings.Title;
    document.title = sessionStorage.title;

    // Check email settings
    const EmailServer = this.settings.EmailServer;
    if (EmailServer !== '') {
      const EmailPort = String(this.settings.EmailPort);
      if (EmailPort === '') {
        this.notificationService.ShowNotify(this.translationService.t('Please enter an Email Port...'), 2000, true);
        return;
      }
      const EmailFrom = this.settings.EmailFrom;
      const EmailTo = this.settings.EmailTo;
      const EmailUsername = this.EmailUsername;
      const EmailPassword = this.EmailPassword;
      if ((EmailFrom === '') || (EmailTo === '')) {
        this.notificationService.ShowNotify(this.translationService.t('Invalid Email From/To Settings...'), 2000, true);
        return;
      }
      if ((EmailUsername !== '') && (EmailPassword === '')) {
        this.notificationService.ShowNotify(this.translationService.t('Please enter an Email Password...'), 2000, true);
        return;
      }
    }

    const popupDelay = this.settings.FloorplanPopupDelay;
    if (String(popupDelay) !== '') {
      if (Number.isNaN(popupDelay)) {
        this.notificationService.ShowNotify(this.translationService.t('Popup Delay can only contain numbers...'), 2000, true);
        return;
      }
    }

    const formData = new FormData();
    formData.append('Language', this.settings.Language);
    formData.append('Themes', this.settings.WebTheme);
    formData.append('Title', this.settings.Title);
    formData.append('Latitude', this.settings.Location.Latitude);
    formData.append('Longitude', this.settings.Location.Longitude);
    formData.append('DashboardType', this.settings.DashboardType.toString());
    if (this.AllowWidgetOrdering) {
      formData.append('AllowWidgetOrdering', 'on');
    }
    formData.append('MobileType', this.settings.MobileType.toString());
    formData.append('WebUserName', this.settings.WebUserName);
    formData.append('WebPassword', this.WebPassword);
    formData.append('AuthenticationMethod', this.settings.AuthenticationMethod.toString());
    formData.append('SecPassword', this.settings.SecPassword);
    formData.append('SecOnDelay', this.settings.SecOnDelay.toString());
    formData.append('ProtectionPassword', this.settings.ProtectionPassword);
    formData.append('WebLocalNetworks', this.settings.WebLocalNetworks);
    formData.append('RemoteSharedPort', this.settings.RemoteSharedPort.toString());
    if (this.UseAutoUpdate) {
      formData.append('checkforupdates', 'on');
    }
    formData.append('ReleaseChannel', this.settings.ReleaseChannel.toString());
    if (this.UseAutoBackup) {
      formData.append('enableautobackup', 'on');
    }
    formData.append('WebRemoteProxyIPs', this.settings.WebRemoteProxyIPs);
    if (this.AcceptNewHardware) {
      formData.append('AcceptNewHardware', 'on');
    }
    if (this.HideDisabledHardwareSensors) {
      formData.append('HideDisabledHardwareSensors', 'on');
    }
    if (this.ShowUpdateEffect) {
      formData.append('ShowUpdateEffect', 'on');
    }
    formData.append('MyDomoticzUserId', this.settings.MyDomoticzUserId);
    formData.append('MyDomoticzPassword', this.settings.MyDomoticzPassword);
    if (this.SubsystemHttp) {
      formData.append('SubsystemHttp', 'on');
    }
    if (this.SubsystemShared) {
      formData.append('SubsystemShared', 'on');
    }
    if (this.SubsystemApps) {
      formData.append('SubsystemApps', 'on');
    }
    if (this.EnableTabFloorplans) {
      formData.append('EnableTabFloorplans', 'on');
    }
    if (this.EnableTabLights) {
      formData.append('EnableTabLights', 'on');
    }
    if (this.EnableTabScenes) {
      formData.append('EnableTabScenes', 'on');
    }
    if (this.EnableTabTemp) {
      formData.append('EnableTabTemp', 'on');
    }
    if (this.EnableTabWeather) {
      formData.append('EnableTabWeather', 'on');
    }
    if (this.EnableTabUtility) {
      formData.append('EnableTabUtility', 'on');
    }
    if (this.EnableTabCustom) {
      formData.append('EnableTabCustom', 'on');
    }
    formData.append('LightHistoryDays', this.settings.LightHistoryDays.toString());
    formData.append('ShortLogDays', this.settings.ShortLogDays.toString());
    if (this.ProwlEnabled) {
      formData.append('ProwlEnabled', 'on');
    }
    formData.append('ProwlAPI', this.settings.ProwlAPI);
    if (this.PushbulletEnabled) {
      formData.append('PushbulletEnabled', 'on');
    }
    formData.append('PushbulletAPI', this.settings.PushbulletAPI);
    if (this.PushsaferEnabled) {
      formData.append('PushsaferEnabled', 'on');
    }
    formData.append('PushsaferAPI', this.settings.PushsaferAPI);
    formData.append('PushsaferImage', this.settings.PushsaferImage);
    if (this.PushoverEnabled) {
      formData.append('PushoverEnabled', 'on');
    }
    formData.append('PushoverUser', this.settings.PushoverUser);
    formData.append('PushoverAPI', this.settings.PushoverAPI);
    if (this.PushALotEnabled) {
      formData.append('PushALotEnabled', 'on');
    }
    formData.append('PushALotAPI', this.settings.PushALotAPI);
    if (this.ClickatellEnabled) {
      formData.append('ClickatellEnabled', 'on');
    }
    formData.append('ClickatellAPI', this.ClickatellAPI);
    formData.append('ClickatellTo', this.ClickatellTo);
    if (this.IFTTTEnabled) {
      formData.append('IFTTTEnabled', 'on');
    }
    formData.append('IFTTTAPI', this.settings.IFTTTAPI);
    if (this.HTTPEnabled) {
      formData.append('HTTPEnabled', 'on');
    }
    formData.append('HTTPField1', this.HTTPField1);
    formData.append('HTTPField2', this.HTTPField2);
    formData.append('HTTPField3', this.HTTPField3);
    formData.append('HTTPField4', this.HTTPField4);
    formData.append('HTTPTo', this.HTTPTo);
    formData.append('HTTPURL', this.HTTPURL);
    formData.append('HTTPPostData', this.HTTPPostData);
    formData.append('HTTPPostContentType', this.HTTPPostContentType);
    formData.append('HTTPPostHeaders', this.HTTPPostHeaders);
    if (this.KodiEnabled) {
      formData.append('KodiEnabled', 'on');
    }
    formData.append('KodiIPAddress', this.settings.KodiIPAddress);
    formData.append('KodiPort', this.settings.KodiPort.toString());
    formData.append('KodiTimeToLive', this.settings.KodiTimeToLive.toString());
    if (this.LmsEnabled) {
      formData.append('LmsEnabled', 'on');
    }
    formData.append('LmsPlayerMac', this.settings.LmsPlayerMac);
    formData.append('LmsDuration', this.settings.LmsDuration.toString());
    if (this.FCMEnabled) {
      formData.append('FCMEnabled', 'on');
    }
    if (this.TelegramEnabled) {
      formData.append('TelegramEnabled', 'on');
    }
    formData.append('TelegramAPI', this.settings.TelegramAPI);
    formData.append('TelegramChat', this.settings.TelegramChat);
    formData.append('NotificationSensorInterval', this.settings.NotificationSensorInterval.toString());
    formData.append('NotificationSwitchInterval', this.settings.NotificationSwitchInterval.toString());
    if (this.EmailEnabled) {
      formData.append('EmailEnabled', 'on');
    }
    formData.append('EmailFrom', this.settings.EmailFrom);
    formData.append('EmailTo', this.settings.EmailTo);
    formData.append('EmailServer', this.settings.EmailServer);
    formData.append('EmailPort', this.settings.EmailPort.toString());
    formData.append('EmailUsername', this.settings.EmailUsername);
    formData.append('EmailPassword', this.settings.EmailPassword);
    if (this.UseEmailInNotifications) {
      formData.append('UseEmailInNotifications', 'on');
    }
    if (this.EmailAsAttachment) {
      formData.append('EmailAsAttachment', 'on');
    }
    if (this.SendErrorsAsNotification) {
      formData.append('SendErrorsAsNotification', 'on');
    }
    formData.append('TempUnit', this.settings.TempUnit.toString());
    formData.append('DegreeDaysBaseTemperature', this.settings.DegreeDaysBaseTemperature);
    formData.append('WindUnit', this.settings.WindUnit.toString());
    formData.append('WeightUnit', this.settings.WeightUnit.toString());
    formData.append('EnergyDivider', this.settings.EnergyDivider.toString());
    formData.append('CostEnergy', this.settings.CostEnergy);
    formData.append('CostEnergyT2', this.settings.CostEnergyT2);
    formData.append('CostEnergyR1', this.settings.CostEnergyR1);
    formData.append('CostEnergyR2', this.settings.CostEnergyR2);
    formData.append('GasDivider', this.settings.GasDivider.toString());
    formData.append('CostGas', this.settings.CostGas);
    formData.append('WaterDivider', this.settings.WaterDivider.toString());
    formData.append('CostWater', this.settings.CostWater);
    formData.append('ElectricVoltage', this.settings.ElectricVoltage.toString());
    formData.append('CM113DisplayType', this.settings.CM113DisplayType.toString());
    formData.append('SmartMeterType', this.settings.SmartMeterType.toString());
    formData.append('FloorplanPopupDelay', this.settings.FloorplanPopupDelay.toString());
    if (this.FloorplanFullscreenMode) {
      formData.append('FloorplanFullscreenMode', 'on');
    }
    if (this.FloorplanAnimateZoom) {
      formData.append('FloorplanAnimateZoom', 'on');
    }
    if (this.FloorplanShowSensorValues) {
      formData.append('FloorplanShowSensorValues', 'on');
    }
    if (this.FloorplanShowSwitchValues) {
      formData.append('FloorplanShowSwitchValues', 'on');
    }
    if (this.FloorplanShowSceneNames) {
      formData.append('FloorplanShowSceneNames', 'on');
    }
    formData.append('FloorplanRoomColour', this.settings.FloorplanRoomColour);
    formData.append('FloorplanActiveOpacity', this.settings.FloorplanActiveOpacity.toString());
    formData.append('FloorplanInactiveOpacity', this.settings.FloorplanInactiveOpacity.toString());
    formData.append('RandomSpread', this.settings.RandomTimerFrame.toString());
    formData.append('SensorTimeout', this.settings.SensorTimeout.toString());
    formData.append('BatterLowLevel', this.settings.BatterLowLevel.toString());
    formData.append('ActiveTimerPlan', this.settings.ActiveTimerPlan.toString());
    formData.append('DoorbellCommand', this.settings.DoorbellCommand.toString());
    formData.append('RaspCamParams', this.settings.RaspCamParams);
    formData.append('UVCParams', this.settings.UVCParams);
    if (this.EnableEventScriptSystem) {
      formData.append('EnableEventScriptSystem', 'on');
    }
    if (this.LogEventScriptTrigger) {
      formData.append('LogEventScriptTrigger', 'on');
    }
    if (this.EventSystemLogFullURL) {
      formData.append('EventSystemLogFullURL', 'on');
    }
    if (this.DisableDzVentsSystem) {
      formData.append('DisableDzVentsSystem', 'on');
    }
    formData.append('DzVentsLogLevel', this.settings.DzVentsLogLevel.toString());

    this.setupService.storeSettings(formData).subscribe((data) => {
      if (data.status !== 'OK') {
        this.notificationService.HideNotify();
        this.notificationService.ShowNotify(this.translationService.t('Problem saving settings!'), 2000, true);
        return;
      }
      this.configService.forceReloadConfig();
      this.router.navigate(['/Dashboard']);
    }, error => {
      this.notificationService.HideNotify();
      this.notificationService.ShowNotify(this.translationService.t('Problem saving settings!'), 2000, true);
    });
  }

  GetGeoLocation() {
    const dialogRef = this.dialogService.addDialog(FindlatlongDialogComponent, {}).instance;
    dialogRef.init();
    dialogRef.open();

    dialogRef.whenOk.subscribe(([lat, long]) => {
      this.settings.Location.Latitude = lat;
      this.settings.Location.Longitude = long;
    });
  }

  AllowNewHardware(minutes: number) {
    this.setupService.allowNewHardware(minutes).subscribe((data) => {
      const msg = this.translationService.t('Allowing new sensors for ') + minutes + ' ' + this.translationService.t('Minutes');
      this.notificationService.ShowNotify(msg, 3000);
      this.router.navigate(['/Log']);
    });
  }

  CleanupShortLog() {
    bootbox.confirm(this.translationService.t('Are you sure to delete the Log?\n\nThis action can not be undone!'), (result) => {
      if (result === true) {
        this.setupService.clearShortLog('0').subscribe((data) => {
          this.router.navigate(['/Dashboard']);
        }, error => {
          this.notificationService.HideNotify();
          this.notificationService.ShowNotify(this.translationService.t('Problem clearing the Log!'), 2500, true);
        });
      }
    });
  }

  TestNotification(subsystem: string) {
    let extraparams = {};
    switch (subsystem) {
      case 'clickatell':
        const ClickatellAPI = this.ClickatellAPI;
        const ClickatellTo = this.ClickatellTo;
        if (ClickatellAPI === '' || ClickatellTo === '') {
          this.notificationService.ShowNotify(this.translationService.t('All Clickatell fields are required!...'), 3500, true);
          return;
        }
        extraparams = {
          ClickatellAPI: ClickatellAPI,
          ClickatellTo: ClickatellTo,
        };
        break;
      case 'http':
        const HTTPField1 = this.HTTPField1;
        const HTTPField2 = this.HTTPField2;
        const HTTPField3 = this.HTTPField3;
        const HTTPField4 = this.HTTPField4;
        const HTTPTo = this.HTTPTo;
        const HTTPURL = this.HTTPURL;
        const HTTPPostData = this.HTTPPostData;
        const HTTPPostHeaders = this.HTTPPostHeaders;
        const HTTPPostContentType = this.HTTPPostContentType;
        if (HTTPPostData !== '' && HTTPPostContentType === '') {
          this.notificationService.ShowNotify(this.translationService.t('Please specify the content type...'), 3500, true);
          return;
        }
        if (HTTPURL === '') {
          this.notificationService.ShowNotify(this.translationService.t('Please specify the base URL!...'), 3500, true);
          return;
        }
        extraparams = {
          HTTPField1: HTTPField1,
          HTTPField2: HTTPField2,
          HTTPField3: HTTPField3,
          HTTPField4: HTTPField4,
          HTTPTo: HTTPTo,
          HTTPURL: HTTPURL,
          HTTPPostData: HTTPPostData,
          HTTPPostContentType: HTTPPostContentType,
          HTTPPostHeaders: HTTPPostHeaders
        };
        break;
      case 'prowl':
        const ProwlAPI = this.settings.ProwlAPI;
        if (ProwlAPI === '') {
          this.notificationService.ShowNotify(this.translationService.t('Please enter the API key!...'), 3500, true);
          return;
        }
        extraparams = {ProwlAPI: ProwlAPI};
        break;
      case 'pushbullet':
        const PushbulletAPI = this.settings.PushbulletAPI;
        if (PushbulletAPI === '') {
          this.notificationService.ShowNotify(this.translationService.t('Please enter the API key!...'), 3500, true);
          return;
        }
        extraparams = {PushbulletAPI: PushbulletAPI};
        break;
      case 'telegram':
        const TelegramAPI = this.settings.TelegramAPI;
        if (TelegramAPI === '') {
          this.notificationService.ShowNotify(this.translationService.t('Please enter the API key!...'), 3500, true);
          return;
        }
        extraparams = {TelegramAPI: TelegramAPI};
        break;
      case 'pushsafer':
        const PushsaferAPI = this.settings.PushsaferAPI;
        const PushsaferImage = this.settings.PushsaferImage;
        if (PushsaferAPI === '') {
          this.notificationService.ShowNotify(this.translationService.t('Please enter the API key!...'), 3500, true);
          return;
        }
        extraparams = {PushsaferAPI: PushsaferAPI, PushsaferImage: PushsaferImage};
        break;
      case 'pushover':
        const POAPI = this.settings.PushoverAPI;
        if (POAPI === '') {
          this.notificationService.ShowNotify(this.translationService.t('Please enter the API key!...'), 3500, true);
          return;
        }
        const POUSERID = this.settings.PushoverUser;
        if (POUSERID === '') {
          this.notificationService.ShowNotify(this.translationService.t('Please enter the user id!...'), 3500, true);
          return;
        }
        extraparams = {PushoverAPI: POAPI, PushoverUser: POUSERID};
        break;
      case 'pushalot':
        const PushAlotAPI = this.settings.PushALotAPI;
        if (PushAlotAPI === '') {
          this.notificationService.ShowNotify(this.translationService.t('Please enter the API key!...'), 3500, true);
          return;
        }
        extraparams = {PushAlotAPI: PushAlotAPI};
        break;
      case 'email':
        const EmailServer = this.settings.EmailServer;
        if (EmailServer === '') {
          this.notificationService.ShowNotify(this.translationService.t('Invalid Email Settings...'), 2000, true);
          return;
        }
        const EmailPort = this.settings.EmailPort;
        if (String(EmailPort) === '') {
          this.notificationService.ShowNotify(this.translationService.t('Invalid Email Settings...'), 2000, true);
          return;
        }
        const EmailFrom = this.settings.EmailFrom;
        const EmailTo = this.settings.EmailTo;
        const EmailUsername = this.EmailUsername;
        const EmailPassword = this.EmailPassword;
        if ((EmailFrom === '') || (EmailTo === '')) {
          this.notificationService.ShowNotify(this.translationService.t('Invalid Email From/To Settings...'), 2000, true);
          return;
        }
        if ((EmailUsername !== '') && (EmailPassword === '')) {
          this.notificationService.ShowNotify(this.translationService.t('Please enter an Email Password...'), 2000, true);
          return;
        }
        extraparams = {
          EmailServer: EmailServer,
          EmailPort: EmailPort,
          EmailFrom: EmailFrom,
          EmailTo: EmailTo,
          EmailUsername: EmailUsername,
          EmailPassword: EmailPassword
        };
        break;
      case 'kodi':
        if (this.settings.KodiIPAddress === '') {
          this.settings.KodiIPAddress = '224.0.0.1';
        }
        if (typeof this.settings.KodiPort === 'undefined') {
          this.settings.KodiPort = 9777;
        }
        if (typeof this.settings.KodiTimeToLive === 'undefined') {
          this.settings.KodiTimeToLive = 5;
        }
        extraparams = {
          KodiIPAddress: this.settings.KodiIPAddress,
          KodiPort: this.settings.KodiPort,
          KodiTimeToLive: this.settings.KodiTimeToLive
        };
        break;
      case 'lms':
        if (typeof this.settings.LmsDuration === 'undefined') {
          this.settings.LmsDuration = 5;
        }
        const LmsPlayerMac = this.settings.LmsPlayerMac;
        const LmsDuration = this.settings.LmsDuration;
        if (LmsPlayerMac === '') {
          this.notificationService.ShowNotify(this.translationService.t('All Logitech Media Server fields are required!...'), 3500, true);
          return;
        }
        extraparams = {LmsPlayerMac: LmsPlayerMac, LmsDuration: LmsDuration};
        break;
      case 'fcm':
        break;
      default:
        return;
    }

    this.setupService.testNotification(subsystem, extraparams).subscribe((data) => {
      if (data.status !== 'OK') {
        this.notificationService.HideNotify();
        if ((subsystem === 'http') || (subsystem === 'kodi') || (subsystem === 'lms') || (subsystem === 'fcm')) {
          this.notificationService.ShowNotify(this.translationService.t('Problem Sending Notification'), 3000, true);
        } else if (subsystem === 'email') {
          this.notificationService.ShowNotify(this.translationService.t('Problem sending Email, please check credentials!'), 3000, true);
        } else {
          this.notificationService.ShowNotify(this.translationService.t('Problem sending notification, please check the API key!'),
            3000, true);
        }
        return;
      } else {
        this.notificationService.HideNotify();
        this.notificationService.ShowNotify(this.translationService.t('Notification sent!<br>Should arrive at your device soon...'), 3000);
      }
    }, error => {
      this.notificationService.HideNotify();
      if (subsystem === 'email') {
        this.notificationService.ShowNotify(this.translationService.t('Invalid Email Settings...'), 3000, true);
      } else {
        this.notificationService.ShowNotify(this.translationService.t('Problem sending notification, please check the API key!'),
          3000, true);
      }
    });
  }

}
