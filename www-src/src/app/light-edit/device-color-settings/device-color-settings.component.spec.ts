import {async, ComponentFixture, TestBed} from '@angular/core/testing';

import {DeviceColorSettingsComponent} from './device-color-settings.component';
import {CommonTestModule} from 'src/app/_testing/common.test.module';
import {Device} from "../../_shared/_models/device";

describe('DeviceColorSettingsComponent', () => {
  let component: DeviceColorSettingsComponent;
  let fixture: ComponentFixture<DeviceColorSettingsComponent>;

  const item: Device = {
    AddjMulti: 1.0,
    AddjMulti2: 1.0,
    AddjValue: 0.0,
    AddjValue2: 0.0,
    BatteryLevel: 255,
    Color: '{"b":66,"cw":0,"g":255,"m":3,"r":96,"t":0,"ww":0}',
    CustomImage: 0,
    Data: 'On',
    Description: '',
    DimmerType: 'abs',
    Favorite: 0,
    HardwareID: 3,
    HardwareName: 'Virtuel',
    HardwareType: 'Dummy (Does nothing, use for virtual switches only)',
    HardwareTypeVal: 15,
    HaveDimmer: true,
    HaveGroupCmd: false,
    HaveTimeout: false,
    ID: '00082033',
    Image: 'Light',
    IsSubDevice: false,
    LastUpdate: '2019-01-26 20:36:26',
    Level: 35,
    LevelInt: 35,
    MaxDimLevel: 100,
    Name: 'RGBWSwitch',
    Notifications: 'false',
    PlanID: '0',
    PlanIDs: [0],
    Protected: false,
    ShowNotifications: true,
    SignalLevel: '-',
    Status: 'On',
    StrParam1: '',
    StrParam2: '',
    SubType: 'RGBW',
    SwitchType: 'Dimmer',
    SwitchTypeVal: 7,
    Timers: 'true',
    Type: 'Color Switch',
    TypeImg: 'dimmer',
    Unit: 1,
    Used: 1,
    UsedByCamera: false,
    XOffset: '0',
    YOffset: '0',
    idx: '34'
  };

  beforeEach(async(() => {
    TestBed.configureTestingModule({
      declarations: [DeviceColorSettingsComponent],
      imports: [CommonTestModule]
    })
      .compileComponents();
  }));

  beforeEach(() => {
    fixture = TestBed.createComponent(DeviceColorSettingsComponent);
    component = fixture.componentInstance;

    component.device = item;

    fixture.detectChanges();
  });

  it('should create', () => {
    expect(component).toBeTruthy();
  });
});
