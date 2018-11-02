import { async, ComponentFixture, TestBed } from '@angular/core/testing';

import { SetupComponent } from './setup.component';
import { CommonTestModule } from '../../../_testing/common.test.module';
import { FormsModule } from '@angular/forms';
import { SetupService } from '../../../_shared/_services/setup.service';
import { of } from 'rxjs';
import { SettingsResponse } from '../../../_shared/_models/settings';

describe('SetupComponent', () => {
  let component: SetupComponent;
  let fixture: ComponentFixture<SetupComponent>;

  const FAKE_SETTINGS: SettingsResponse = {
    'AcceptNewHardware': 1,
    'ActiveTimerPlan': 0,
    'AllowWidgetOrdering': 1,
    'AuthenticationMethod': 0,
    'BatterLowLevel': 0,
    'CM113DisplayType': 0,
    'ClickatellAPI': '',
    'ClickatellEnabled': 0,
    'ClickatellTo': '',
    'CostEnergy': '0.2149',
    'CostEnergyR1': '0.0800',
    'CostEnergyR2': '0.0800',
    'CostEnergyT2': '0.2149',
    'CostGas': '0.6218',
    'CostWater': '1.6473',
    'DashboardType': 0,
    'DegreeDaysBaseTemperature': '18.0',
    'DisableDzVentsSystem': 0,
    'DoorbellCommand': 0,
    'DzVentsLogLevel': 3,
    'ElectricVoltage': 230,
    'EmailAsAttachment': 0,
    'EmailEnabled': 1,
    'EmailFrom': '',
    'EmailPassword': '',
    'EmailPort': 25,
    'EmailServer': '',
    'EmailTo': '',
    'EmailUsername': '',
    'EnableEventScriptSystem': 1,
    'EnableTabCustom': 1,
    'EnableTabFloorplans': 1,
    'EnableTabLights': 1,
    'EnableTabScenes': 1,
    'EnableTabTemp': 1,
    'EnableTabUtility': 1,
    'EnableTabWeather': 1,
    'EnergyDivider': 1000,
    'FloorplanActiveOpacity': 25,
    'FloorplanAnimateZoom': 1,
    'FloorplanFullscreenMode': 0,
    'FloorplanInactiveOpacity': 5,
    'FloorplanPopupDelay': 750,
    'FloorplanRoomColour': 'Blue',
    'FloorplanShowSceneNames': 1,
    'FloorplanShowSensorValues': 1,
    'FloorplanShowSwitchValues': 0,
    'FCMEnabled': 0,
    'GasDivider': 100,
    'HTTPEnabled': 0,
    'HTTPField1': '',
    'HTTPField2': '',
    'HTTPField3': '',
    'HTTPField4': '',
    'HTTPPostContentType': '',
    'HTTPPostData': '',
    'HTTPPostHeaders': '',
    'HTTPTo': '',
    'HTTPURL': '',
    'HideDisabledHardwareSensors': 1,
    'IFTTTAPI': '',
    'IFTTTEnabled': 0,
    'KodiEnabled': 0,
    'KodiIPAddress': '224.0.0.1',
    'KodiPort': 9777,
    'KodiTimeToLive': 5,
    'Language': 'fr',
    'LightHistoryDays': 30,
    'LmsDuration': 5,
    'LmsEnabled': 0,
    'LmsPlayerMac': '',
    'Location': {
      'Latitude': '45.182479',
      'Longitude': '5.721077'
    },
    'LogEventScriptTrigger': 1,
    'MobileType': 0,
    'MyDomoticzSubsystems': 0,
    'MyDomoticzUserId': '',
    'NotificationSensorInterval': 43200,
    'NotificationSwitchInterval': 0,
    'ProtectionPassword': '',
    'ProwlAPI': '',
    'ProwlEnabled': 0,
    'PushALotAPI': '',
    'PushALotEnabled': 0,
    'PushbulletAPI': '',
    'PushbulletEnabled': 0,
    'PushoverAPI': '',
    'PushoverEnabled': 0,
    'PushoverUser': '',
    'PushsaferAPI': '',
    'PushsaferEnabled': 0,
    'PushsaferImage': '',
    'RandomTimerFrame': 15,
    'RaspCamParams': '-w 800 -h 600 -t 1',
    'ReleaseChannel': 0,
    'RemoteSharedPort': 6144,
    'SecOnDelay': 30,
    'SecPassword': '',
    'SendErrorsAsNotification': 0,
    'SensorTimeout': 60,
    'ShortLogDays': 3,
    'ShortLogInterval': 5,
    'ShowUpdateEffect': 0,
    'SmartMeterType': 0,
    'TelegramAPI': '',
    'TelegramChat': '',
    'TelegramEnabled': 0,
    'TempUnit': 0,
    'Title': 'Domoticz',
    'UVCParams': '-S80 -B128 -C128 -G80 -x800 -y600 -q100',
    'UseAutoBackup': 1,
    'UseAutoUpdate': 1,
    'UseEmailInNotifications': 1,
    'WaterDivider': 100,
    'WebLocalNetworks': '127.0.0.*;192.168.1.*',
    'WebRemoteProxyIPs': '',
    'WebTheme': 'default',
    'WeightUnit': 0,
    'WindUnit': 0,
    'cloudenabled': true,
    'status': 'OK',
    'title': 'settings'
  };


  beforeEach(async(() => {
    TestBed.configureTestingModule({
      declarations: [SetupComponent],
      imports: [CommonTestModule, FormsModule]
    })
      .compileComponents();
  }));

  beforeEach(() => {
    fixture = TestBed.createComponent(SetupComponent);
    component = fixture.componentInstance;
    fixture.detectChanges();

    const setupService = TestBed.get(SetupService);
    spyOn(setupService, 'getSettings').and.returnValue(of(FAKE_SETTINGS));
  });

  it('should create', () => {
    expect(component).toBeTruthy();
  });
});
