import {NgModule} from '@angular/core';
import {CommonModule} from '@angular/common';
import {FormsModule} from '@angular/forms';
import {SendNotificationRoutingModule} from './send-notification-routing.module';
import {SendNotificationComponent} from './send-notification.component';
import {SharedModule} from '../../_shared/shared.module';
import {SendNotificationService} from './send-notification.service';

@NgModule({
  imports: [
    CommonModule,
    FormsModule,
    /* Routing */
    SendNotificationRoutingModule,
    /* Domoticz */
    SharedModule
  ],
  declarations: [
    SendNotificationComponent,
  ],
  exports: [],
  providers: [
    SendNotificationService
  ]
})
export class SendNotificationModule {
}
