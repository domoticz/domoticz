import {NgModule} from '@angular/core';
import {RouterModule, Routes} from '@angular/router';
import {OfflineGuard} from '../_shared/_guards/offline.guard';
import {AuthenticationGuard} from '../_shared/_guards/authentication.guard';
import {DeviceReportComponent} from './device-report/device-report.component';

const routes: Routes = [
  {
    path: ':year/:month',
    component: DeviceReportComponent,
    canActivate: [OfflineGuard, AuthenticationGuard]
  },
  {
    path: ':year',
    component: DeviceReportComponent,
    canActivate: [OfflineGuard, AuthenticationGuard]
  },
  {
    path: '',
    component: DeviceReportComponent,
    canActivate: [OfflineGuard, AuthenticationGuard]
  }
];

@NgModule({
  imports: [RouterModule.forChild(routes)],
  exports: [RouterModule]
})
export class DeviceReportRoutingModule {
}
