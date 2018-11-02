import {NgModule} from '@angular/core';
import {CommonModule} from '@angular/common';
import {DeviceNotificationsRoutingModule} from './device-notifications-routing.module';
import {FormsModule} from '@angular/forms';
import {DeviceNotificationsComponent} from './device-notifications/device-notifications.component';
import {NotificationsTableComponent} from './notifications-table/notifications-table.component';
import {NotificationFormComponent} from './notification-form/notification-form.component';
import {SharedModule} from '../_shared/shared.module';
import {DeviceNotificationService} from './device-notification.service';

@NgModule({
  imports: [
    CommonModule,
    FormsModule,
    /* Routing */
    DeviceNotificationsRoutingModule,
    /* Domoticz */
    SharedModule
  ],
  declarations: [
    DeviceNotificationsComponent,
    NotificationsTableComponent,
    NotificationFormComponent
  ],
  exports: [],
  providers: [
    DeviceNotificationService
  ]
})
export class DeviceNotificationsModule {

}
