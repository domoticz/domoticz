import {NgModule} from '@angular/core';
import {CommonModule} from '@angular/common';
import {SetupRoutingModule} from './setup-routing.module';
import {FormsModule} from '@angular/forms';
import {SetupComponent} from './setup/setup.component';
import {SharedModule} from "../../_shared/shared.module";

@NgModule({
  imports: [
    CommonModule,
    FormsModule,
    /* Routing */
    SetupRoutingModule,
    /* Domoticz */
    SharedModule,
  ],
  declarations: [
    SetupComponent
  ],
  exports: [],
  providers: []
})
export class SetupModule {
}
