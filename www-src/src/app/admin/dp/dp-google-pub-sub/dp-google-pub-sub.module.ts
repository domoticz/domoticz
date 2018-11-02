import {NgModule} from '@angular/core';
import {CommonModule} from '@angular/common';
import {FormsModule} from '@angular/forms';
import {DpGooglePubSubRoutingModule} from './dp-google-pub-sub-routing.module';
import {DpGooglePubSubComponent} from './dp-google-pub-sub.component';
import {SharedModule} from '../../../_shared/shared.module';
import {DpGooglePubSubService} from './dp-google-pub-sub.service';

@NgModule({
  imports: [
    CommonModule,
    FormsModule,
    /* Routing */
    DpGooglePubSubRoutingModule,
    /* Domoticz */
    SharedModule
  ],
  declarations: [
    DpGooglePubSubComponent,
  ],
  exports: [],
  providers: [
    DpGooglePubSubService
  ]
})
export class DpGooglePubSubModule {
}
