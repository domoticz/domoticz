import {NgModule} from '@angular/core';
import {CommonModule} from '@angular/common';
import {LightSwitchesRoutingModule} from './light-switches-routing.module';
import {LightSwitchesDashboardComponent} from './light-switches-dashboard/light-switches-dashboard.component';
import {LightSwitchWidgetComponent} from './light-switch-widget/light-switch-widget.component';
import {RfyPopupComponent} from './rfy-popup/rfy-popup.component';
import {FormsModule} from '@angular/forms';
import {SharedModule} from '../_shared/shared.module';

@NgModule({
  imports: [
    CommonModule,
    FormsModule,
    /* Routing */
    LightSwitchesRoutingModule,
    /* Domoticz */
    SharedModule
  ],
  declarations: [
    LightSwitchesDashboardComponent,
    LightSwitchWidgetComponent,
    RfyPopupComponent,
  ],
  exports: [],
  providers: []
})
export class LightSwitchesModule {
}
