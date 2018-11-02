import {NgModule} from '@angular/core';
import {CommonModule} from '@angular/common';
import {MobileRoutingModule} from './mobile-routing.module';
import {FormsModule} from '@angular/forms';
import {MobileNotificationsComponent} from './mobile-notifications/mobile-notifications.component';
import {SharedModule} from '../../_shared/shared.module';
import {MobileService} from './mobile.service';

@NgModule({
  imports: [
    CommonModule,
    FormsModule,
    /* Routing */
    MobileRoutingModule,
    /* Domoticz */
    SharedModule
  ],
  declarations: [
    MobileNotificationsComponent
  ],
  exports: [],
  providers: [
    MobileService
  ]
})
export class MobileModule {
}
