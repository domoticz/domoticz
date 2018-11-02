import {NgModule} from '@angular/core';
import {CommonModule} from '@angular/common';
import {FormsModule} from '@angular/forms';
import {RestoreDatabaseRoutingModule} from './restore-database-routing.module';
import {RestoreDatabaseComponent} from './restore-database.component';
import {SharedModule} from "../../_shared/shared.module";

@NgModule({
  imports: [
    CommonModule,
    FormsModule,
    /* Routing */
    RestoreDatabaseRoutingModule,
    /* Domoticz */
    SharedModule
  ],
  declarations: [
    RestoreDatabaseComponent
  ],
  exports: [],
  providers: []
})
export class RestoreDatabaseModule {
}
