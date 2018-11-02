import {NgModule} from '@angular/core';
import {CommonModule} from '@angular/common';
import {FormsModule} from '@angular/forms';
import {SecurityPanelComponent} from './security-panel.component';
import {SecurityPanelRoutingModule} from './security-panel-routing.module';
import {SharedModule} from '../../_shared/shared.module';
import {SecurityService} from './security.service';

@NgModule({
  imports: [
    CommonModule,
    FormsModule,
    /* Routing */
    SecurityPanelRoutingModule,
    /* Domoticz */
    SharedModule,
  ],
  declarations: [
    SecurityPanelComponent
  ],
  exports: [],
  providers: [
    SecurityService
  ]
})
export class SecurityPanelModule {
}
