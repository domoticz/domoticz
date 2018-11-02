import {NgModule} from '@angular/core';
import {CommonModule} from '@angular/common';
import {FormsModule} from '@angular/forms';
import {UserVariablesRoutingModule} from './user-variables-routing.module';
import {UserVariablesComponent} from './user-variables.component';
import {SharedModule} from "../../_shared/shared.module";

@NgModule({
  imports: [
    CommonModule,
    FormsModule,
    /* Routing */
    UserVariablesRoutingModule,
    /* Domoticz */
    SharedModule
  ],
  declarations: [
    UserVariablesComponent,
  ],
  exports: [],
  providers: []
})
export class UserVariablesModule {
}
