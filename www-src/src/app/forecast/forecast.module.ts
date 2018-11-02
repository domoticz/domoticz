import {NgModule} from '@angular/core';
import {CommonModule} from '@angular/common';
import {ForecastComponent} from './forecast/forecast.component';
import {ForecastRoutingModule} from './forecast-routing.module';
import {LocationService} from './location.service';

@NgModule({
  imports: [
    CommonModule,
    /* Routing */
    ForecastRoutingModule,
    /* Domoticz */
  ],
  declarations: [
    ForecastComponent
  ],
  exports: [],
  providers: [
    LocationService
  ]
})
export class ForecastModule {
}
