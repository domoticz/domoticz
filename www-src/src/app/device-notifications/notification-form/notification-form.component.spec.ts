import { async, ComponentFixture, TestBed } from '@angular/core/testing';

import { NotificationFormComponent } from './notification-form.component';
import { FormsModule } from '@angular/forms';
import { CommonTestModule } from 'src/app/_testing/common.test.module';
import { SimpleChanges, SimpleChange } from '@angular/core';
import {Device} from "../../_shared/_models/device";
import {DeviceNotificationsModule} from "../device-notifications.module";

describe('NotificationFormComponent', () => {
  let component: NotificationFormComponent;
  let fixture: ComponentFixture<NotificationFormComponent>;

  beforeEach(async(() => {
    TestBed.configureTestingModule({
      declarations: [],
      imports: [
        CommonTestModule,
        FormsModule,
        DeviceNotificationsModule
      ]
    })
      .compileComponents();
  }));

  beforeEach(() => {
    fixture = TestBed.createComponent(NotificationFormComponent);
    component = fixture.componentInstance;

    const item: Device = {
      AddjMulti: 1.0,
      AddjMulti2: 1.0,
      AddjValue: 0.0,
      AddjValue2: 0.0,
      BatteryLevel: 100,
      CustomImage: 0,
      Data: '9.0 C, 84 %',
      Description: 'Outdoor sensor',
      DewPoint: '6.44',
      Favorite: 1,
      HardwareID: 2,
      HardwareName: 'RFXCOM',
      HardwareType: 'RFXCOM - RFXtrx433 USB 433.92MHz Transceiver',
      HardwareTypeVal: 1,
      HaveTimeout: false,
      Humidity: 84,
      HumidityStatus: 'Wet',
      ID: '5704',
      LastUpdate: '2018-12-09 09:41:41',
      Name: 'Terrasse',
      Notifications: 'false',
      PlanID: '3',
      PlanIDs: [3],
      Protected: false,
      ShowNotifications: true,
      SignalLevel: 5,
      SubType: 'THGN122/123/132, THGR122/228/238/268',
      Temp: 9.0,
      Timers: 'false',
      Type: 'Temp + Humidity',
      TypeImg: 'temperature',
      Unit: 4,
      Used: 1,
      XOffset: '29',
      YOffset: '44',
      idx: '152'
    };

    const notif = {
      Priority: 0,
      CustomMessage: '',
      SendAlways: false,
      ActiveSystems: '',
      Params: 'T'
    };

    component.device = item;
    component.notification = notif;
    component.notifiers = [
      {
        'description': 'browser',
        'name': 'browser'
      },
      {
        'description': 'clickatell',
        'name': 'clickatell'
      },
      {
        'description': 'email',
        'name': 'email'
      },
      {
        'description': 'gcm',
        'name': 'gcm'
      },
      {
        'description': 'http',
        'name': 'http'
      },
      {
        'description': 'kodi',
        'name': 'kodi'
      },
      {
        'description': 'lms',
        'name': 'lms'
      },
      {
        'description': 'prowl',
        'name': 'prowl'
      },
      {
        'description': 'pushalot',
        'name': 'pushalot'
      },
      {
        'description': 'pushbullet',
        'name': 'pushbullet'
      },
      {
        'description': 'pushover',
        'name': 'pushover'
      },
      {
        'description': 'pushsafer',
        'name': 'pushsafer'
      },
      {
        'description': 'telegram',
        'name': 'telegram'
      }
    ];

    component.ngOnChanges({
      notification: new SimpleChange(null, notif, true)
    });
    fixture.detectChanges();
  });

  it('should create', () => {
    expect(component).toBeTruthy();
  });
});
