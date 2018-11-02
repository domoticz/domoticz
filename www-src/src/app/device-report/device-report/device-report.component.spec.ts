import { async, ComponentFixture, TestBed } from '@angular/core/testing';

import { DeviceReportComponent } from './device-report.component';
import { CommonTestModule } from '../../_testing/common.test.module';
import { DeviceTemperatureReportComponent } from '../device-temperature-report/device-temperature-report.component';
import { FormsModule } from '@angular/forms';
import { DeviceCounterReportComponent } from '../device-counter-report/device-counter-report.component';
import { DeviceEnergyMultiCounterReportComponent } from '../device-energy-multi-counter-report/device-energy-multi-counter-report.component';

describe('DeviceReportComponent', () => {
  let component: DeviceReportComponent;
  let fixture: ComponentFixture<DeviceReportComponent>;

  beforeEach(async(() => {
    TestBed.configureTestingModule({
      declarations: [
        DeviceReportComponent,
        DeviceTemperatureReportComponent,
        DeviceCounterReportComponent,
        DeviceEnergyMultiCounterReportComponent
      ],
      imports: [
        CommonTestModule,
        FormsModule
      ]
    })
      .compileComponents();
  }));

  beforeEach(() => {
    fixture = TestBed.createComponent(DeviceReportComponent);
    component = fixture.componentInstance;
    fixture.detectChanges();
  });

  it('should create', () => {
    expect(component).toBeTruthy();
  });
});
