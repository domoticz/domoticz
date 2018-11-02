import {async, ComponentFixture, TestBed} from '@angular/core/testing';

import {DeviceLightLogComponent} from './device-light-log.component';
import {CommonTestModule} from 'src/app/_testing/common.test.module';
import {DeviceOnOffChartComponent} from '../device-on-off-chart/device-on-off-chart.component';
import {DeviceLevelChartComponent} from '../device-level-chart/device-level-chart.component';
import {FormsModule} from '@angular/forms';

describe('DeviceLightLogComponent', () => {
  let component: DeviceLightLogComponent;
  let fixture: ComponentFixture<DeviceLightLogComponent>;

  beforeEach(async(() => {
    TestBed.configureTestingModule({
      declarations: [DeviceLightLogComponent, DeviceOnOffChartComponent, DeviceLevelChartComponent],
      imports: [CommonTestModule, FormsModule]
    })
      .compileComponents();
  }));

  beforeEach(() => {
    fixture = TestBed.createComponent(DeviceLightLogComponent);
    component = fixture.componentInstance;
    fixture.detectChanges();
  });

  it('should create', () => {
    expect(component).toBeTruthy();
  });
});
