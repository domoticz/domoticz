import {NgModule} from '@angular/core';
import {CommonModule} from '@angular/common';
import {FormsModule} from '@angular/forms';
import {RainLogRoutingModule} from './rain-log-routing.module';
import {RainLogComponent} from './rain-log/rain-log.component';
import {RainDayGraphComponent} from './rain-day-graph/rain-day-graph.component';
import {RainWeekGraphComponent} from './rain-week-graph/rain-week-graph.component';
import {RainMonthGraphComponent} from './rain-month-graph/rain-month-graph.component';
import {RainYearGraphComponent} from './rain-year-graph/rain-year-graph.component';
import {SharedModule} from "../../_shared/shared.module";

@NgModule({
  imports: [
    CommonModule,
    FormsModule,
    /* Routing */
    RainLogRoutingModule,
    /* Domoticz */
    SharedModule
  ],
  declarations: [
    RainLogComponent,
    RainDayGraphComponent,
    RainWeekGraphComponent,
    RainMonthGraphComponent,
    RainYearGraphComponent
  ],
  exports: [],
  providers: []
})
export class RainLogModule {
}
