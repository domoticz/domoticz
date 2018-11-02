import {NgModule} from '@angular/core';
import {CommonModule} from '@angular/common';
import {WeatherRoutingModule} from './weather-routing.module';
import {WeatherDashboardComponent} from './weather-dashboard/weather-dashboard.component';
import {WeatherWidgetComponent} from './weather-widget/weather-widget.component';
import {WeatherForecastComponent} from './weather-forecast/weather-forecast.component';
import {SharedModule} from "../_shared/shared.module";

@NgModule({
  imports: [
    CommonModule,
    /* Routing */
    WeatherRoutingModule,
    /* Domoticz */
    SharedModule
  ],
  declarations: [
    WeatherDashboardComponent,
    WeatherWidgetComponent,
    WeatherForecastComponent
  ],
  exports: [],
  providers: []
})
export class WeatherModule {
}
