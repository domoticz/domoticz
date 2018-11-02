import {NgModule} from '@angular/core';
import {CommonModule} from '@angular/common';
import {FormsModule} from '@angular/forms';
import {TimerplanRoutingModule} from './timerplan-routing.module';
import {TimerplanComponent} from './timerplan/timerplan.component';
import {SharedModule} from '../../_shared/shared.module';

@NgModule({
  imports: [
    CommonModule,
    FormsModule,
    /* Routing */
    TimerplanRoutingModule,
    /* Domoticz */
    SharedModule,
  ],
  declarations: [
    TimerplanComponent
  ],
  exports: [],
  providers: []
})
export class TimerplanModule {
}
