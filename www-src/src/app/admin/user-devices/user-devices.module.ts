import {NgModule} from '@angular/core';
import {CommonModule} from '@angular/common';
import {UserDevicesRoutingModule} from './user-devices-routing.module';
import {FormsModule} from '@angular/forms';
import {UserDevicesComponent} from './user-devices/user-devices.component';
import {SharedModule} from "../../_shared/shared.module";

@NgModule({
  imports: [
    CommonModule,
    FormsModule,
    /* Routing */
    UserDevicesRoutingModule,
    /* Domoticz */
    SharedModule
  ],
  declarations: [
    UserDevicesComponent,
  ],
  exports: [],
  providers: []
})
export class UserDevicesModule {
}
