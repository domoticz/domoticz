import {NgModule} from '@angular/core';
import {CommonModule} from '@angular/common';
import {FormsModule} from '@angular/forms';
import {RainReportRoutingModule} from './rain-report-routing.module';
import {RainReportComponent} from './rain-report/rain-report.component';
import {RainMonthReportComponent} from './rain-month-report/rain-month-report.component';
import {SharedModule} from "../_shared/shared.module";

@NgModule({
  imports: [
    CommonModule,
    FormsModule,
    /* Routing */
    RainReportRoutingModule,
    /* Domoticz */
    SharedModule
  ],
  declarations: [
    RainReportComponent,
    RainMonthReportComponent
  ],
  exports: [],
  providers: []
})
export class RainReportModule {
}
