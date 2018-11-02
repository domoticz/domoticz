import {NgModule} from '@angular/core';
import {CommonModule} from '@angular/common';
import {DashboardComponent} from './dashboard.component';
import {DashboardRoutingModule} from './dashboard-routing.module';
import {DashboardUtilityWidgetComponent} from './dashboard-utility-widget/dashboard-utility-widget.component';
import {DashboardWeatherWidgetComponent} from './dashboard-weather-widget/dashboard-weather-widget.component';
import {DashboardSecurityWidgetComponent} from './dashboard-security-widget/dashboard-security-widget.component';
import {DashboardSceneWidgetComponent} from './dashboard-scene-widget/dashboard-scene-widget.component';
import {DashboardLightSwitchesWidgetComponent} from './dashboard-light-switches-widget/dashboard-light-switches-widget.component';
import {DashboardTemperatureWidgetComponent} from './dashboard-temperature-widget/dashboard-temperature-widget.component';
import {DashboardMobileUtilityWidgetComponent} from './dashboard-mobile-utility-widget/dashboard-mobile-utility-widget.component';
import {DashboardMobileWeatherWidgetComponent} from './dashboard-mobile-weather-widget/dashboard-mobile-weather-widget.component';
import {DashboardMobileTemperatureWidgetComponent} from './dashboard-mobile-temperature-widget/dashboard-mobile-temperature-widget.component';
import {DashboardMobileLightSwitchesWidgetComponent} from './dashboard-mobile-light-switches-widget/dashboard-mobile-light-switches-widget.component';
import {DashboardMobileSceneWidgetComponent} from './dashboard-mobile-scene-widget/dashboard-mobile-scene-widget.component';
import {DashboardMobileSecurityWidgetComponent} from './dashboard-mobile-security-widget/dashboard-mobile-security-widget.component';
import {SharedModule} from '../_shared/shared.module';

@NgModule({
  imports: [
    CommonModule,
    /* DOmoticz */
    DashboardRoutingModule,
    SharedModule
  ],
  declarations: [
    DashboardComponent,
    DashboardLightSwitchesWidgetComponent,
    DashboardMobileLightSwitchesWidgetComponent,
    DashboardMobileSceneWidgetComponent,
    DashboardMobileSecurityWidgetComponent,
    DashboardMobileTemperatureWidgetComponent,
    DashboardMobileUtilityWidgetComponent,
    DashboardMobileWeatherWidgetComponent,
    DashboardSceneWidgetComponent,
    DashboardSecurityWidgetComponent,
    DashboardTemperatureWidgetComponent,
    DashboardUtilityWidgetComponent,
    DashboardWeatherWidgetComponent,
  ]
})
export class DashboardModule {
}
