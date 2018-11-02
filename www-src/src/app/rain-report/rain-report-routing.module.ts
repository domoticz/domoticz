import {NgModule} from '@angular/core';
import {RouterModule, Routes} from '@angular/router';
import {OfflineGuard} from '../_shared/_guards/offline.guard';
import {AuthenticationGuard} from '../_shared/_guards/authentication.guard';
import {RainReportComponent} from './rain-report/rain-report.component';
import {RainMonthReportComponent} from './rain-month-report/rain-month-report.component';

const routes: Routes = [
  {
    path: '',
    component: RainReportComponent,
    canActivate: [OfflineGuard, AuthenticationGuard]
  },
  {
    path: ':year/:month',
    component: RainMonthReportComponent,
    canActivate: [OfflineGuard, AuthenticationGuard]
  },
];

@NgModule({
  imports: [RouterModule.forChild(routes)],
  exports: [RouterModule]
})
export class RainReportRoutingModule {
}
