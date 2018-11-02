import {NgModule} from '@angular/core';
import {CommonModule} from '@angular/common';
import {FormsModule} from '@angular/forms';
import {CurrentLogRoutingModule} from './current-log-routing.module';
import {CurrentLogComponent} from './current-log/current-log.component';
import {CurrentYearGraphComponent} from './current-year-graph/current-year-graph.component';
import {CurrentDayGraphComponent} from './current-day-graph/current-day-graph.component';
import {CurrentMonthGraphComponent} from './current-month-graph/current-month-graph.component';
import {SharedModule} from "../../_shared/shared.module";

@NgModule({
  imports: [
    CommonModule,
    FormsModule,
    /* Routing */
    CurrentLogRoutingModule,
    /* Domoticz */
    SharedModule
  ],
  declarations: [
    CurrentLogComponent,
    CurrentDayGraphComponent,
    CurrentMonthGraphComponent,
    CurrentYearGraphComponent
  ],
  exports: [],
  providers: []
})
export class CurrentLogModule {
}
