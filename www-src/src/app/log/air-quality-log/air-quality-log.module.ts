import {NgModule} from '@angular/core';
import {CommonModule} from '@angular/common';
import {FormsModule} from '@angular/forms';
import {AirQualityLogRoutingModule} from './air-quality-log-routing.module';
import {AirQualityLogComponent} from './air-quality-log/air-quality-log.component';
import {AirQualityDayGraphComponent} from './air-quality-day-graph/air-quality-day-graph.component';
import {AirQualityMonthGraphComponent} from './air-quality-month-graph/air-quality-month-graph.component';
import {AirQualityYearGraphComponent} from './air-quality-year-graph/air-quality-year-graph.component';
import {SharedModule} from "../../_shared/shared.module";

@NgModule({
  imports: [
    CommonModule,
    FormsModule,
    /* Routing */
    AirQualityLogRoutingModule,
    /* Domoticz */
    SharedModule
  ],
  declarations: [
    AirQualityLogComponent,
    AirQualityDayGraphComponent,
    AirQualityMonthGraphComponent,
    AirQualityYearGraphComponent
  ],
  exports: [],
  providers: []
})
export class AirQualityLogModule {
}
