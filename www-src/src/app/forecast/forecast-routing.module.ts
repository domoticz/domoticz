import {NgModule} from '@angular/core';
import {RouterModule, Routes} from '@angular/router';
import {ForecastComponent} from './forecast/forecast.component';
import {OfflineGuard} from '../_shared/_guards/offline.guard';
import {AuthenticationGuard} from '../_shared/_guards/authentication.guard';

const routes: Routes = [
  {
    path: '',
    component: ForecastComponent,
    canActivate: [OfflineGuard, AuthenticationGuard]
  }
];

@NgModule({
  imports: [RouterModule.forChild(routes)],
  exports: [RouterModule]
})
export class ForecastRoutingModule {
}
