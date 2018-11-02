import {NgModule} from '@angular/core';
import {CommonModule} from '@angular/common';
import {DeviceLogRoutingModule} from './device-log-routing.module';
import {FormsModule} from '@angular/forms';
import {DeviceLogComponent} from './device-log/device-log.component';
import {DeviceTextLogComponent} from './device-text-log/device-text-log.component';
import {DeviceLightLogComponent} from './device-light-log/device-light-log.component';
import {DeviceOnOffChartComponent} from './device-on-off-chart/device-on-off-chart.component';
import {DeviceLevelChartComponent} from './device-level-chart/device-level-chart.component';
import {DeviceTemperatureLogComponent} from './device-temperature-log/device-temperature-log.component';
import {TemperatureLogChartComponent} from './temperature-log-chart/temperature-log-chart.component';
import {DeviceGraphLogComponent} from './device-graph-log/device-graph-log.component';
import {DeviceLogChartComponent} from './device-log-chart/device-log-chart.component';
import {SmartLogComponent} from './smart-log/smart-log.component';
import {SmartDayGraphComponent} from './smart-day-graph/smart-day-graph.component';
import {SmartWeekGraphComponent} from './smart-week-graph/smart-week-graph.component';
import {SmartYearGraphComponent} from './smart-year-graph/smart-year-graph.component';
import {SmartMonthGraphComponent} from './smart-month-graph/smart-month-graph.component';
import {CounterSplineLogComponent} from './counter-spline-log/counter-spline-log.component';
import {CounterSplineMonthGraphComponent} from './counter-spline-month-graph/counter-spline-month-graph.component';
import {CounterSplineYearGraphComponent} from './counter-spline-year-graph/counter-spline-year-graph.component';
import {CounterSplineWeekGraphComponent} from './counter-spline-week-graph/counter-spline-week-graph.component';
import {CounterSplineDayGraphComponent} from './counter-spline-day-graph/counter-spline-day-graph.component';
import {CounterLogComponent} from './counter-log/counter-log.component';
import {CounterWeekGraphComponent} from './counter-week-graph/counter-week-graph.component';
import {CounterYearGraphComponent} from './counter-year-graph/counter-year-graph.component';
import {CounterMonthGraphComponent} from './counter-month-graph/counter-month-graph.component';
import {CounterDayGraphComponent} from './counter-day-graph/counter-day-graph.component';
import {SharedModule} from '../../_shared/shared.module';

@NgModule({
  imports: [
    CommonModule,
    FormsModule,
    /* Routing */
    DeviceLogRoutingModule,
    /* Domoticz */
    SharedModule,
  ],
  declarations: [
    DeviceLogComponent,
    DeviceTextLogComponent,
    DeviceLightLogComponent,
    DeviceOnOffChartComponent,
    DeviceLevelChartComponent,
    DeviceTemperatureLogComponent,
    TemperatureLogChartComponent,
    DeviceGraphLogComponent,
    DeviceLogChartComponent,
    SmartLogComponent,
    SmartDayGraphComponent,
    SmartMonthGraphComponent,
    SmartWeekGraphComponent,
    SmartYearGraphComponent,
    CounterSplineLogComponent,
    CounterSplineDayGraphComponent,
    CounterSplineWeekGraphComponent,
    CounterSplineMonthGraphComponent,
    CounterSplineYearGraphComponent,
    CounterLogComponent,
    CounterDayGraphComponent,
    CounterMonthGraphComponent,
    CounterWeekGraphComponent,
    CounterYearGraphComponent
  ],
  exports: [],
  providers: []
})
export class DeviceLogModule {
}
