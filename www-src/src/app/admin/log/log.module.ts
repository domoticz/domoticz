import {NgModule} from '@angular/core';
import {CommonModule} from '@angular/common';
import {FormsModule} from '@angular/forms';
import {LogRoutingModule} from './log-routing.module';
import {LogComponent} from './log.component';
import {SharedModule} from '../../_shared/shared.module';
import {LogService} from './log.service';

@NgModule({
  imports: [
    CommonModule,
    FormsModule,
    /* Routing */
    LogRoutingModule,
    /* Domoticz */
    SharedModule
  ],
  declarations: [
    LogComponent
  ],
  exports: [],
  providers: [
    LogService
  ]
})
export class LogModule {
}
