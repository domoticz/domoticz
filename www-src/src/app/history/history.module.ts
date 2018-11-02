import {NgModule} from '@angular/core';
import {CommonModule} from '@angular/common';
import {HistoryRoutingModule} from './history-routing.module';
import {HistoryComponent} from './history.component';
import {HistoryService} from './history.service';

@NgModule({
  imports: [
    CommonModule,
    /* Routing */
    HistoryRoutingModule,
    /* Domoticz */
  ],
  declarations: [
    HistoryComponent
  ],
  exports: [],
  providers: [
    HistoryService
  ]
})
export class HistoryModule {
}
