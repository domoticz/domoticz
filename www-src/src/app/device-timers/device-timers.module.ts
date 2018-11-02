import {NgModule} from '@angular/core';
import {CommonModule} from '@angular/common';
import {DeviceTimersRoutingModule} from './device-timers-routing.module';
import {FormsModule} from '@angular/forms';
import {DeviceTimersComponent} from './device-timers/device-timers.component';
import {SharedModule} from '../_shared/shared.module';
import {RegularTimersService} from './regular-timers.service';
import {SetpointTimersService} from './setpoint-timers.service';

@NgModule({
  imports: [
    CommonModule,
    FormsModule,
    /* Routing */
    DeviceTimersRoutingModule,
    /* Domoticz */
    SharedModule,
  ],
  declarations: [
    DeviceTimersComponent
  ],
  exports: [],
  providers: [
    RegularTimersService,
    SetpointTimersService
  ]
})
export class DeviceTimersModule {

}
