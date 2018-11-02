import {NgModule} from '@angular/core';
import {RouterModule, Routes} from '@angular/router';
import {OfflineGuard} from '../_shared/_guards/offline.guard';
import {AuthenticationGuard} from '../_shared/_guards/authentication.guard';
import {TemperatureComponent} from './temperature/temperature.component';
import {TemperatureForecastComponent} from './temperature-forecast/temperature-forecast.component';
import {CustomTempLogComponent} from './custom-temp-log/custom-temp-log.component';

const routes: Routes = [
  {
    path: 'Forecast/:idx',
    component: TemperatureForecastComponent,
    canActivate: [OfflineGuard, AuthenticationGuard]
  },
  {
    path: 'CustomTempLog',
    component: CustomTempLogComponent,
    canActivate: [OfflineGuard, AuthenticationGuard]
  },
  {
    path: '',
    component: TemperatureComponent,
    canActivate: [OfflineGuard, AuthenticationGuard]
  }
];

@NgModule({
  imports: [RouterModule.forChild(routes)],
  exports: [RouterModule]
})
export class TemperatureRoutingModule {
}
