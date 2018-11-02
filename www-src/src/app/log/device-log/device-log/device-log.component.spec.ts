import {async, ComponentFixture, TestBed} from '@angular/core/testing';

import {CommonTestModule} from 'src/app/_testing/common.test.module';
import {FormsModule} from '@angular/forms';
import {DeviceLogComponent} from './device-log.component';
import {DeviceTextLogComponent} from '../device-text-log/device-text-log.component';
import {DeviceLightLogComponent} from '../device-light-log/device-light-log.component';
import {DeviceTemperatureLogComponent} from '../device-temperature-log/device-temperature-log.component';
import {DeviceGraphLogComponent} from '../device-graph-log/device-graph-log.component';
import {SmartLogComponent} from '../smart-log/smart-log.component';
import {CounterSplineLogComponent} from '../counter-spline-log/counter-spline-log.component';
import {CounterLogComponent} from '../counter-log/counter-log.component';
import {DeviceOnOffChartComponent} from '../device-on-off-chart/device-on-off-chart.component';
import {DeviceLevelChartComponent} from '../device-level-chart/device-level-chart.component';
import {TemperatureLogChartComponent} from '../temperature-log-chart/temperature-log-chart.component';
import {DeviceLogChartComponent} from '../device-log-chart/device-log-chart.component';
import {SmartDayGraphComponent} from '../smart-day-graph/smart-day-graph.component';
import {SmartMonthGraphComponent} from '../smart-month-graph/smart-month-graph.component';
import {SmartWeekGraphComponent} from '../smart-week-graph/smart-week-graph.component';
import {SmartYearGraphComponent} from '../smart-year-graph/smart-year-graph.component';
import {CounterSplineDayGraphComponent} from '../counter-spline-day-graph/counter-spline-day-graph.component';
import {CounterSplineMonthGraphComponent} from '../counter-spline-month-graph/counter-spline-month-graph.component';
import {CounterSplineWeekGraphComponent} from '../counter-spline-week-graph/counter-spline-week-graph.component';
import {CounterSplineYearGraphComponent} from '../counter-spline-year-graph/counter-spline-year-graph.component';
import {CounterDayGraphComponent} from '../counter-day-graph/counter-day-graph.component';
import {CounterWeekGraphComponent} from '../counter-week-graph/counter-week-graph.component';
import {CounterMonthGraphComponent} from '../counter-month-graph/counter-month-graph.component';
import {CounterYearGraphComponent} from '../counter-year-graph/counter-year-graph.component';

describe('DeviceLogComponent', () => {
  let component: DeviceLogComponent;
  let fixture: ComponentFixture<DeviceLogComponent>;

  beforeEach(async(() => {
    TestBed.configureTestingModule({
      declarations: [
        DeviceLogComponent,
        DeviceLogChartComponent,
        DeviceTextLogComponent,
        DeviceLightLogComponent,
        DeviceTemperatureLogComponent,
        DeviceGraphLogComponent,
        SmartLogComponent,
        SmartDayGraphComponent,
        SmartMonthGraphComponent,
        SmartWeekGraphComponent,
        SmartYearGraphComponent,
        CounterSplineLogComponent,
        CounterSplineDayGraphComponent,
        CounterSplineMonthGraphComponent,
        CounterSplineWeekGraphComponent,
        CounterSplineYearGraphComponent,
        CounterLogComponent,
        CounterDayGraphComponent,
        CounterWeekGraphComponent,
        CounterMonthGraphComponent,
        CounterYearGraphComponent,
        DeviceOnOffChartComponent,
        DeviceLevelChartComponent,
        TemperatureLogChartComponent
      ],
      imports: [CommonTestModule, FormsModule
      ]
    })
      .compileComponents();
  }));

  beforeEach(() => {
    fixture = TestBed.createComponent(DeviceLogComponent);
    component = fixture.componentInstance;
    fixture.detectChanges();
  });

  it('should create', () => {
    expect(component).toBeTruthy();
  });
});
