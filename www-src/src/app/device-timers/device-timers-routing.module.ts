import {NgModule} from '@angular/core';
import {RouterModule, Routes} from '@angular/router';
import {OfflineGuard} from '../_shared/_guards/offline.guard';
import {AuthenticationGuard} from '../_shared/_guards/authentication.guard';
import {DeviceTimersComponent} from './device-timers/device-timers.component';

const routes: Routes = [
  {
    path: '',
    component: DeviceTimersComponent,
    canActivate: [OfflineGuard, AuthenticationGuard]
  }
];

@NgModule({
  imports: [RouterModule.forChild(routes)],
  exports: [RouterModule]
})
export class DeviceTimersRoutingModule {
}
