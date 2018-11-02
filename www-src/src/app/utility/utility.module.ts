import {NgModule} from '@angular/core';
import {CommonModule} from '@angular/common';
import {UtilityRoutingModule} from './utility-routing.module';
import {UtilityDashboardComponent} from './utility-dashboard/utility-dashboard.component';
import {UtilityWidgetComponent} from './utility-widget/utility-widget.component';
import {SharedModule} from '../_shared/shared.module';

@NgModule({
  imports: [
    CommonModule,
    /* Routing */
    UtilityRoutingModule,
    /* Domoticz */
    SharedModule
  ],
  declarations: [
    UtilityDashboardComponent,
    UtilityWidgetComponent
  ],
  exports: [],
  providers: []
})
export class UtilityModule {
}
