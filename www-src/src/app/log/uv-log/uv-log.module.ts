import {NgModule} from '@angular/core';
import {CommonModule} from '@angular/common';
import {FormsModule} from '@angular/forms';
import {UvLogComponent} from './uv-log/uv-log.component';
import {UvDayGraphComponent} from './uv-day-graph/uv-day-graph.component';
import {UvMonthGraphComponent} from './uv-month-graph/uv-month-graph.component';
import {UvYearGraphComponent} from './uv-year-graph/uv-year-graph.component';
import {UvLogRoutingModule} from './uv-log-routing.module';
import {SharedModule} from "../../_shared/shared.module";

@NgModule({
  imports: [
    CommonModule,
    FormsModule,
    /* Routing */
    UvLogRoutingModule,
    /* Domoticz */
    SharedModule
  ],
  declarations: [
    UvLogComponent,
    UvDayGraphComponent,
    UvMonthGraphComponent,
    UvYearGraphComponent
  ],
  exports: [],
  providers: []
})
export class UvLogModule {
}


