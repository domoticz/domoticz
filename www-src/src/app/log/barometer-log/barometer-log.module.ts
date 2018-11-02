import {NgModule} from '@angular/core';
import {CommonModule} from '@angular/common';
import {FormsModule} from '@angular/forms';
import {BarometerLogRoutingModule} from './barometer-log-routing.module';
import {BarometerLogComponent} from './barometer-log/barometer-log.component';
import {BarometerDayGraphComponent} from './barometer-day-graph/barometer-day-graph.component';
import {BarometerMonthGraphComponent} from './barometer-month-graph/barometer-month-graph.component';
import {BarometerYearGraphComponent} from './barometer-year-graph/barometer-year-graph.component';
import {SharedModule} from "../../_shared/shared.module";

@NgModule({
  imports: [
    CommonModule,
    FormsModule,
    /* Routing */
    BarometerLogRoutingModule,
    /* Domoticz */
    SharedModule
  ],
  declarations: [
    BarometerLogComponent,
    BarometerDayGraphComponent,
    BarometerMonthGraphComponent,
    BarometerYearGraphComponent
  ],
  exports: [],
  providers: []
})
export class BarometerLogModule {
}
