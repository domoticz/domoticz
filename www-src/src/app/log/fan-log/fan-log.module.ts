import {NgModule} from '@angular/core';
import {CommonModule} from '@angular/common';
import {FormsModule} from '@angular/forms';
import {FanLogRoutingModule} from './fan-log-routing.module';
import {FanLogComponent} from './fan-log/fan-log.component';
import {FanDayGraphComponent} from './fan-day-graph/fan-day-graph.component';
import {FanMonthGraphComponent} from './fan-month-graph/fan-month-graph.component';
import {FanYearGraphComponent} from './fan-year-graph/fan-year-graph.component';
import {SharedModule} from "../../_shared/shared.module";

@NgModule({
  imports: [
    CommonModule,
    FormsModule,
    /* Routing */
    FanLogRoutingModule,
    /* Domoticz */
   SharedModule
  ],
  declarations: [
    FanLogComponent,
    FanDayGraphComponent,
    FanMonthGraphComponent,
    FanYearGraphComponent
  ],
  exports: [],
  providers: []
})
export class FanLogModule {
}
