import {NgModule} from '@angular/core';
import {CommonModule} from '@angular/common';
import {FormsModule} from '@angular/forms';
import {TemperatureRoutingModule} from './temperature-routing.module';
import {TemperatureComponent} from './temperature/temperature.component';
import {TemperatureWidgetComponent} from './temperature-widget/temperature-widget.component';
import {TemperatureForecastComponent} from './temperature-forecast/temperature-forecast.component';
import {CustomTempLogComponent} from './custom-temp-log/custom-temp-log.component';
import {SharedModule} from '../_shared/shared.module';

@NgModule({
  imports: [
    CommonModule,
    FormsModule,
    /* Routing */
    TemperatureRoutingModule,
    /* Domoticz */
    SharedModule
  ],
  declarations: [
    TemperatureComponent,
    TemperatureWidgetComponent,
    TemperatureForecastComponent,
    CustomTempLogComponent
  ],
  exports: [],
  providers: []
})
export class TemperatureModule {
}
