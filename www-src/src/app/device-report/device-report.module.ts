import {NgModule} from '@angular/core';
import {CommonModule} from '@angular/common';
import {DeviceReportRoutingModule} from './device-report-routing.module';
import {FormsModule} from '@angular/forms';
import {DeviceReportComponent} from './device-report/device-report.component';
import {DeviceCounterReportComponent} from './device-counter-report/device-counter-report.component';
import {DeviceEnergyMultiCounterReportComponent} from './device-energy-multi-counter-report/device-energy-multi-counter-report.component';
import {DeviceTemperatureReportComponent} from './device-temperature-report/device-temperature-report.component';
import {SharedModule} from '../_shared/shared.module';
import {DeviceCounterReportDataService} from './device-counter-report-data.service';
import {DeviceEnergyMultiCounterReportDataService} from './device-energy-multi-counter-report-data.service';
import {DeviceTemperatureReportDataService} from './device-temperature-report-data.service';

@NgModule({
  imports: [
    CommonModule,
    FormsModule,
    /* Routing */
    DeviceReportRoutingModule,
    /* Domoticz */
    SharedModule
  ],
  declarations: [
    DeviceReportComponent,
    DeviceCounterReportComponent,
    DeviceEnergyMultiCounterReportComponent,
    DeviceTemperatureReportComponent
  ],
  exports: [],
  providers: [
    DeviceCounterReportDataService,
    DeviceEnergyMultiCounterReportDataService,
    DeviceTemperatureReportDataService
  ]
})
export class DeviceReportModule {

}
