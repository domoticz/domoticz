import {NgModule} from '@angular/core';
import {RouterModule, Routes} from '@angular/router';
import {DeviceLogComponent} from './device-log/device-log.component';
import {OfflineGuard} from '../../_shared/_guards/offline.guard';
import {AuthenticationGuard} from '../../_shared/_guards/authentication.guard';

const routes: Routes = [
  {
    path: '',
    component: DeviceLogComponent,
    canActivate: [OfflineGuard, AuthenticationGuard]
  }
];

@NgModule({
  imports: [RouterModule.forChild(routes)],
  exports: [RouterModule]
})
export class DeviceLogRoutingModule {
}
