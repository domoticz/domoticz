import {NgModule} from '@angular/core';
import {CommonModule} from '@angular/common';
import {FormsModule} from '@angular/forms';
import {UpdateRoutingModule} from './update-routing.module';
import {UpdateComponent} from './update.component';
import {RoundProgressModule} from 'angular-svg-round-progressbar';

@NgModule({
  imports: [
    CommonModule,
    FormsModule,
    RoundProgressModule,
    /* Routing */
    UpdateRoutingModule,
    /* Domoticz */
  ],
  declarations: [
    UpdateComponent
  ],
  exports: [],
  providers: []
})
export class UpdateModule {
}
