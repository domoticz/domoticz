import {NgModule} from '@angular/core';
import {RouterModule, Routes} from '@angular/router';
import {OfflineGuard} from '../_shared/_guards/offline.guard';
import {AuthenticationGuard} from '../_shared/_guards/authentication.guard';
import {WeatherDashboardComponent} from './weather-dashboard/weather-dashboard.component';
import {WeatherForecastComponent} from './weather-forecast/weather-forecast.component';

const routes: Routes = [
  {
    path: 'Forecast/:idx',
    component: WeatherForecastComponent,
    canActivate: [OfflineGuard, AuthenticationGuard]
  },
  {
    path: '',
    component: WeatherDashboardComponent,
    canActivate: [OfflineGuard, AuthenticationGuard]
  }
];

@NgModule({
  imports: [RouterModule.forChild(routes)],
  exports: [RouterModule]
})
export class WeatherRoutingModule {
}
