import {NgModule} from '@angular/core';
import {CommonModule} from '@angular/common';
import {RfxcomFirmwareRoutingModule} from './rfxcom-firmware-routing.module';
import {FormsModule} from '@angular/forms';
import {RfxcomFirmwareComponent} from './rfxcom-firmware/rfxcom-firmware.component';
import {RoundProgressModule} from 'angular-svg-round-progressbar';
import {SharedModule} from "../../_shared/shared.module";

@NgModule({
  imports: [
    CommonModule,
    FormsModule,
    RoundProgressModule,
    /* Routing */
    RfxcomFirmwareRoutingModule,
    /* Domoticz */
    SharedModule
  ],
  declarations: [
    RfxcomFirmwareComponent
  ],
  exports: [],
  providers: []
})
export class RfxcomFirmwareModule {
}
