import {NgModule} from '@angular/core';
import {RouterModule, Routes} from '@angular/router';
import {OfflineGuard} from '../../_shared/_guards/offline.guard';
import {AuthenticationGuard} from '../../_shared/_guards/authentication.guard';
import {FanLogComponent} from './fan-log/fan-log.component';

const routes: Routes = [
  {
    path: '',
    component: FanLogComponent,
    canActivate: [OfflineGuard, AuthenticationGuard]
  }
];

@NgModule({
  imports: [RouterModule.forChild(routes)],
  exports: [RouterModule]
})
export class FanLogRoutingModule {
}
