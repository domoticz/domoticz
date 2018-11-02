import {async, ComponentFixture, TestBed} from '@angular/core/testing';

import {DimmerSliderComponent} from './dimmer-slider.component';
import {CommonTestModule} from 'src/app/_testing/common.test.module';
import {Device} from "../../_models/device";

describe('DimmerSliderComponent', () => {
  let component: DimmerSliderComponent;
  let fixture: ComponentFixture<DimmerSliderComponent>;

  const item: Device = {
    AddjMulti: 1.0,
    AddjMulti2: 1.0,
    AddjValue: 0.0,
    AddjValue2: 0.0,
    BatteryLevel: 255,
    CustomImage: 0,
    Data: 'Open',
    Description: '',
    DimmerType: 'none',
    Favorite: 1,
    HardwareID: 6,
    HardwareName: 'AeonStick',
    HardwareType: 'OpenZWave USB',
    HardwareTypeVal: 21,
    HaveDimmer: true,
    HaveGroupCmd: true,
    HaveTimeout: false,
    ID: '00000201',
    Image: 'Light',
    IsSubDevice: false,
    LastUpdate: '2019-01-22 20:42:30',
    Level: 0,
    LevelInt: 0,
    MaxDimLevel: 100,
    Name: 'StoreToit',
    Notifications: 'false',
    PlanID: '2',
    PlanIDs: [2],
    Protected: false,
    ShowNotifications: true,
    SignalLevel: '-',
    Status: 'Open',
    StrParam1: '',
    StrParam2: '',
    SubType: 'Switch',
    SwitchType: 'Blinds Percentage',
    SwitchTypeVal: 13,
    Timers: 'true',
    Type: 'Light/Switch',
    TypeImg: 'blinds',
    Unit: 1,
    Used: 1,
    UsedByCamera: false,
    XOffset: '79',
    YOffset: '115',
    idx: '64'
  };

  beforeEach(async(() => {
    TestBed.configureTestingModule({
      declarations: [],
      imports: [CommonTestModule]
    })
      .compileComponents();
  }));

  beforeEach(() => {
    fixture = TestBed.createComponent(DimmerSliderComponent);
    component = fixture.componentInstance;

    component.item = item;

    fixture.detectChanges();
  });

  it('should create', () => {
    expect(component).toBeTruthy();
  });
});
