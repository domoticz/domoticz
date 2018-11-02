import { async, ComponentFixture, TestBed } from '@angular/core/testing';

import { DeviceOnOffChartComponent } from './device-on-off-chart.component';
import { CommonTestModule } from '../../../_testing/common.test.module';

describe('DeviceOnOffChartComponent', () => {
  let component: DeviceOnOffChartComponent;
  let fixture: ComponentFixture<DeviceOnOffChartComponent>;

  beforeEach(async(() => {
    TestBed.configureTestingModule({
      declarations: [ DeviceOnOffChartComponent ],
      imports: [CommonTestModule]
    })
    .compileComponents();
  }));

  beforeEach(() => {
    fixture = TestBed.createComponent(DeviceOnOffChartComponent);
    component = fixture.componentInstance;
    fixture.detectChanges();
  });

  it('should create', () => {
    expect(component).toBeTruthy();
  });
});
