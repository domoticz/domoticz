import {NgModule} from '@angular/core';
import {RouterModule, Routes} from '@angular/router';
import {OfflineGuard} from '../../_shared/_guards/offline.guard';
import {AuthenticationGuard} from '../../_shared/_guards/authentication.guard';
import {BarometerLogComponent} from './barometer-log/barometer-log.component';

const routes: Routes = [
  {
    path: '',
    component: BarometerLogComponent,
    canActivate: [OfflineGuard, AuthenticationGuard]
  }
];

@NgModule({
  imports: [RouterModule.forChild(routes)],
  exports: [RouterModule]
})
export class BarometerLogRoutingModule {
}
