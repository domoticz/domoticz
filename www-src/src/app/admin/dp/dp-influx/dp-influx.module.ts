import {NgModule} from '@angular/core';
import {CommonModule} from '@angular/common';
import {FormsModule} from '@angular/forms';
import {DpInfluxRoutingModule} from './dp-influx-routing.module';
import {DpInfluxComponent} from './dp-influx.component';
import {SharedModule} from '../../../_shared/shared.module';
import {InfluxService} from './influx.service';

@NgModule({
  imports: [
    CommonModule,
    FormsModule,
    /* Routing */
    DpInfluxRoutingModule,
    /* Domoticz */
    SharedModule
  ],
  declarations: [
    DpInfluxComponent,
  ],
  exports: [],
  providers: [
    InfluxService
  ]
})
export class DpInfluxModule {
}
