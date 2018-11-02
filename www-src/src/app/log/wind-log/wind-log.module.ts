import {NgModule} from '@angular/core';
import {CommonModule} from '@angular/common';
import {FormsModule} from '@angular/forms';
import {WindLogRoutingModule} from './wind-log-routing.module';
import {WindLogComponent} from './wind-log/wind-log.component';
import {WindDayGraphComponent} from './wind-day-graph/wind-day-graph.component';
import {WindDirectionGraphComponent} from './wind-direction-graph/wind-direction-graph.component';
import {WindMonthGraphComponent} from './wind-month-graph/wind-month-graph.component';
import {WindYearGraphComponent} from './wind-year-graph/wind-year-graph.component';
import {SharedModule} from "../../_shared/shared.module";

@NgModule({
  imports: [
    CommonModule,
    FormsModule,
    /* Routing */
    WindLogRoutingModule,
    /* Domoticz */
    SharedModule
  ],
  declarations: [
    WindLogComponent,
    WindDayGraphComponent,
    WindDirectionGraphComponent,
    WindMonthGraphComponent,
    WindYearGraphComponent
  ],
  exports: [],
  providers: []
})
export class WindLogModule {
}


