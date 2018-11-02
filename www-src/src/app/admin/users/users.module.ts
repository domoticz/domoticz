import {NgModule} from '@angular/core';
import {CommonModule} from '@angular/common';
import {UsersRoutingModule} from './users-routing.module';
import {FormsModule} from '@angular/forms';
import {UsersComponent} from './users/users.component';
import {SharedModule} from "../../_shared/shared.module";

@NgModule({
  imports: [
    CommonModule,
    FormsModule,
    /* Routing */
    UsersRoutingModule,
    /* Domoticz */
    SharedModule
  ],
  declarations: [
    UsersComponent,
  ],
  exports: [],
  providers: []
})
export class UsersModule {
}
